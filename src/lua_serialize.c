/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Sandbox serialization implementation @file */

#include <lualib.h>
#include <lauxlib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lua_serialize.h"
#include "lua_circular_buffer.h"
#include "lua_bloom_filter.h"
#include "lua_hyperloglog.h"

const char* not_a_number = "nan";

int preserve_global_data(lua_sandbox* lsb, const char* data_file)
{
  static const char* G = "_G";

  // make sure the string library is loaded before we start
  lua_getglobal(lsb->lua, LUA_STRLIBNAME);
  if (!lua_istable(lsb->lua, -1)) {
    load_library(lsb->lua, LUA_STRLIBNAME, luaopen_string, disable_none);
  }
  lua_pop(lsb->lua, 1); // Remove string table.

  lua_getglobal(lsb->lua, G);
  if (!lua_istable(lsb->lua, -1)) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "preserve_global_data cannot access the global table");
    return 1;
  }

  FILE* fh = fopen(data_file, "wb");
  if (fh == NULL) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "preserve_global_data could not open: %s", data_file);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return 1;
  }

  int result = 0;
  serialization_data data;
  data.fh = fh;
  data.keys.maxsize = 0;

// Clear the sandbox limits during preservation.
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT, 0);
#else
//  unsigned limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif
  lua_sethook(lsb->lua, instruction_manager, 0, 0);
//  size_t cur_output_size = lsb->output.size;
//  size_t max_output_size = lsb->output.maxsize;
  lsb->output.maxsize = 0;
// end clear

  data.keys.size = OUTPUT_SIZE;
  data.keys.pos = 0;
  data.keys.data = malloc(data.keys.size);
  data.tables.size = 64;
  data.tables.pos = 0;
  data.tables.array = malloc(data.tables.size * sizeof(table_ref));
  if (data.tables.array == NULL || data.keys.data == NULL) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "preserve_global_data out of memory");
    result = 1;
  } else {
    appendf(&data.keys, "%s", G);
    data.keys.pos += 1;
    data.globals = lua_topointer(lsb->lua, -1);
    lua_checkstack(lsb->lua, 2);
    lua_pushnil(lsb->lua);
    while (result == 0 && lua_next(lsb->lua, -2) != 0) {
      result = serialize_kvp(lsb, &data, 0);
      lua_pop(lsb->lua, 1);
    }
    lua_pop(lsb->lua, lua_gettop(lsb->lua));
    // Wipe the entire Lua stack.  Since incremental cleanup on failure
    // was added the stack should only contain table _G.
  }
  free(data.tables.array);
  free(data.keys.data);
  fclose(fh);
  if (result != 0) {
    remove(data_file);
  }

// Uncomment if we start preserving state when not destroying the sandbox
/*
// Restore the sandbox limits after preservation
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT,
         lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT]);
#else
  lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = limit;
  lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM] =
    lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT];
#endif
  lsb->output.maxsize = max_output_size;
  lsb->output.pos = 0;
  if (lsb->output.size > cur_output_size) {
    void* ptr = realloc(lsb->output.data, cur_output_size);
    if (!ptr) return 1;
    lsb->output.data = ptr;
    lsb->output.size = cur_output_size;
  }
// end restore
*/

  return result;
}


int serialize_double(output_data* output, double d)
{
  if (isnan(d)) {
    return appends(output, not_a_number);
  }
  if (d > INT_MAX) {
    return appendf(output, "%0.17g", d);
  }

  const int precision = 8;
  const unsigned magnitude = 100000000;
  char buffer[20];
  char* p = buffer;
  int negative = 0;

  if (d < 0) {
    negative = 1;
    d = -d;
  }

  int number = (int)d;
  double tmp = (d - number) * magnitude;
  unsigned fraction = (unsigned)tmp;
  double diff = tmp - fraction;

  if (diff > 0.5) {
    ++fraction;
    if (fraction >= magnitude) {
      fraction = 0;
      ++number;
    }
  } else if (diff == 0.5 && ((fraction == 0) || (fraction & 1))) {
    // bankers rounding
    ++fraction;
  }

  // output decimal fraction
  if (fraction != 0) {
    int nodigits = 1;
    char c = 0;
    for (int x = 0; x < precision; ++x) {
      c = fraction % 10;
      if (!(c == 0 && nodigits)) {
        *p++ = c + '0';
        nodigits = 0;
      }
      fraction /= 10;
    }
    *p++ = '.';
  }

  // output number
  do {
    *p++ = (number % 10) + '0';
    number /= 10;
  }
  while (number > 0);

  size_t remaining = output->size - output->pos;
  size_t needed = (p - buffer) + negative;
  if (needed >= remaining) {
    if (realloc_output(output, needed)) return 1;
  }

  if (negative) {
    output->data[output->pos++] = '-';
  }
  do {
    --p;
    output->data[output->pos++] = *p;
  }
  while (p != buffer);
  output->data[output->pos] = 0;

  return 0;
}


int serialize_table(lua_sandbox* lsb, serialization_data* data, size_t parent)
{
  int result = 0;
  lua_checkstack(lsb->lua, 2);
  lua_pushnil(lsb->lua);
  while (result == 0 && lua_next(lsb->lua, -2) != 0) {
    result = serialize_kvp(lsb, data, parent);
    lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                          // the next interation.
  }
  return result;
}


int serialize_data(lua_sandbox* lsb, int index, output_data* output)
{
  output->pos = 0;
  switch (lua_type(lsb->lua, index)) {
  case LUA_TNUMBER:
    if (serialize_double(output, lua_tonumber(lsb->lua, index))) {
      return 1;
    }
    break;
  case LUA_TSTRING:
    // The stack is cleaned up on failure by preserve_global_data
    // but for clarity it is incrementally cleaned up anyway.
    lua_checkstack(lsb->lua, 4);
    lua_getglobal(lsb->lua, LUA_STRLIBNAME);
    lua_getfield(lsb->lua, -1, "format");
    if (!lua_isfunction(lsb->lua, -1)) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "serialize_data cannot access the string format function");
      lua_pop(lsb->lua, 2); // Remove the bogus format function and
                            // string table.
      return 1;
    }
    lua_pushstring(lsb->lua, "%q");
    lua_pushvalue(lsb->lua, index - 3);
    if (lua_pcall(lsb->lua, 2, 1, 0) == 0) {
      if (appendf(output, "%s", lua_tostring(lsb->lua, -1))) {
        lua_pop(lsb->lua, 1); // Remove the string table.
        return 1;
      }
    } else {
      int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                         "serialize_data '%s'",
                         lua_tostring(lsb->lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      lua_pop(lsb->lua, 2); // Remove the error message and the string
                            // table.
      return 1;
    }
    lua_pop(lsb->lua, 2); // Remove the pcall result and the string table.
    break;
  case LUA_TBOOLEAN:
    if (appendf(output, "%s",
                lua_toboolean(lsb->lua, index)
                ? "true" : "false")) {
      return 1;
    }
    break;
  default:
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "serialize_data cannot preserve type '%s'",
             lua_typename(lsb->lua, lua_type(lsb->lua, index)));
    return 1;
  }
  return 0;
}


void* userdata_type(lua_State* lua, int index, const char* tname)
{
  void* ud = lua_touserdata(lua, index);
  if (ud != NULL) {
    if (lua_getmetatable(lua, index)) {
      lua_getfield(lua, LUA_REGISTRYINDEX, tname);
      if (!lua_rawequal(lua, -1, -2)) {
        ud = NULL;
      }
    }
  }
  lua_pop(lua, 2); // metatable and field
  return ud;
}


int serialize_kvp(lua_sandbox* lsb, serialization_data* data, size_t parent)
{
  int kindex = -2, vindex = -1, result = 0;

  if (ignore_value_type(lsb, data, vindex)) {
    return 0;
  }

  if (serialize_data(lsb, kindex, &lsb->output)) {
    return 1;
  }

  size_t pos = data->keys.pos;
  if (appendf(&data->keys, "%s[%s]", data->keys.data + parent,
              lsb->output.data)) {
    return 1;
  }

  if (lua_type(lsb->lua, vindex) == LUA_TTABLE) {
    const void* ptr = lua_topointer(lsb->lua, vindex);
    table_ref* seen = find_table_ref(&data->tables, ptr);
    if (seen == NULL) {
      seen = add_table_ref(&data->tables, ptr, pos);
      if (seen != NULL) {
        data->keys.pos += 1;
        fprintf(data->fh, "%s = {}\n", data->keys.data + pos);
        result = serialize_table(lsb, data, pos);
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "preserve table out of memory");
        return 1;
      }
    } else {
      fprintf(data->fh, "%s = ", data->keys.data + pos);
      data->keys.pos = pos;
      fprintf(data->fh, "%s\n", data->keys.data + seen->name_pos);
    }
  } else if (lua_type(lsb->lua, vindex) == LUA_TUSERDATA) {
    void* ud = NULL;
    if ((ud = userdata_type(lsb->lua, vindex, lsb_circular_buffer))) {
      table_ref* seen = find_table_ref(&data->tables, ud);
      if (seen == NULL) {
        seen = add_table_ref(&data->tables, ud, pos);
        if (seen != NULL) {
          data->keys.pos += 1;
          result = serialize_circular_buffer(lsb->lua,
                                             data->keys.data + pos,
                                             (circular_buffer*)ud,
                                             &lsb->output);
          if (result == 0) {
            fprintf(data->fh, "%s", lsb->output.data);
          }
        } else {
          snprintf(lsb->error_message, LSB_ERROR_SIZE,
                   "serialize_circular_buffer out of memory");
          return 1;
        }
      } else {
        fprintf(data->fh, "%s = ", data->keys.data + pos);
        data->keys.pos = pos;
        fprintf(data->fh, "%s\n", data->keys.data +
                seen->name_pos);
      }
    } else if ((ud = userdata_type(lsb->lua, vindex, lsb_bloom_filter))) {
      table_ref* seen = find_table_ref(&data->tables, ud);
      if (seen == NULL) {
        seen = add_table_ref(&data->tables, ud, pos);
        if (seen != NULL) {
          data->keys.pos += 1;
          result = serialize_bloom_filter(data->keys.data + pos,
                                          (bloom_filter*)ud,
                                          &lsb->output);
          if (result == 0) {
            size_t n = fwrite(lsb->output.data, 1, lsb->output.pos, data->fh);
            if (n != lsb->output.pos) {
              snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "serialize_bloom_filter failed");
              return 1;
            }
          }
        } else {
          snprintf(lsb->error_message, LSB_ERROR_SIZE,
                   "serialize_bloom_filter out of memory");
          return 1;
        }
      } else {
        fprintf(data->fh, "%s = ", data->keys.data + pos);
        data->keys.pos = pos;
        fprintf(data->fh, "%s\n", data->keys.data +
                seen->name_pos);
      }
    } else if ((ud = userdata_type(lsb->lua, vindex, lsb_hyperloglog))) {
      table_ref* seen = find_table_ref(&data->tables, ud);
      if (seen == NULL) {
        seen = add_table_ref(&data->tables, ud, pos);
        if (seen != NULL) {
          data->keys.pos += 1;
          result = serialize_hyperloglog(data->keys.data + pos,
                                         (hyperloglog*)ud,
                                         &lsb->output);
          if (result == 0) {
            size_t n = fwrite(lsb->output.data, 1, lsb->output.pos, data->fh);
            if (n != lsb->output.pos) {
              snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "serialize_hyperloglog failed");
              return 1;
            }
          }
        } else {
          snprintf(lsb->error_message, LSB_ERROR_SIZE,
                   "serialize_hyperloglog out of memory");
          return 1;
        }
      } else {
        fprintf(data->fh, "%s = ", data->keys.data + pos);
        data->keys.pos = pos;
        fprintf(data->fh, "%s\n", data->keys.data +
                seen->name_pos);
      }
    }
  } else {
    fprintf(data->fh, "%s = ", data->keys.data + pos);
    data->keys.pos = pos;
    result = serialize_data(lsb, vindex, &lsb->output);
    if (result == 0) {
      fprintf(data->fh, "%s\n", lsb->output.data);
    }
  }
  return result;
}


int ignore_key(lua_sandbox* lsb, int index)
{
  if (lua_type(lsb->lua, index) == LUA_TSTRING) {
    const char* key = lua_tostring(lsb->lua, index);
    if (key[0] == '_') {
      return 1;
    }
  }
  return 0;
}


table_ref* find_table_ref(table_ref_array* tra, const void* ptr)
{
  for (size_t i = 0; i < tra->pos; ++i) {
    if (ptr == tra->array[i].ptr) {
      return &tra->array[i];
    }
  }
  return NULL;
}


table_ref* add_table_ref(table_ref_array* tra, const void* ptr, size_t name_pos)
{
  if (tra->pos == tra->size) {
    size_t newsize =  tra->size * 2;
    void* p = realloc(tra->array, newsize * sizeof(table_ref));
    if (p != NULL) {
      tra->array = p;
      tra->size = newsize;
    } else {
      return NULL;
    }
  }
  tra->array[tra->pos].ptr = ptr;
  tra->array[tra->pos].name_pos = name_pos;
  return &tra->array[tra->pos++];
}


int ignore_value_type(lua_sandbox* lsb, serialization_data* data, int index)
{
  switch (lua_type(lsb->lua, index)) {
  case LUA_TTABLE:
    if (lua_getmetatable(lsb->lua, index) != 0) {
      lua_pop(lsb->lua, 1); // Remove the metatable.
      return 1;
    }
    if (lua_topointer(lsb->lua, index) == data->globals) {
      return 1;
    }
    break;
  case LUA_TUSERDATA:
    if (!userdata_type(lsb->lua, index, lsb_circular_buffer) &&
        !userdata_type(lsb->lua, index, lsb_bloom_filter) &&
        !userdata_type(lsb->lua, index, lsb_hyperloglog)) {
      return 1;
    }
    break;
  case LUA_TNONE:
  case LUA_TFUNCTION:
  case LUA_TTHREAD:
  case LUA_TLIGHTUSERDATA:
  case LUA_TNIL:
    return 1;
  }
  return 0;
}


int restore_global_data(lua_sandbox* lsb, const char* data_file)
{
  // Clear the sandbox limits during restoration.
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT, 0);
#else
  unsigned limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif
  lua_sethook(lsb->lua, instruction_manager, 0, 0);

  int err = 0;
  if ((err = luaL_dofile(lsb->lua, data_file)) != 0) {
    if (LUA_ERRFILE != err) {
      int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                         "restore_global_data %s",
                         lua_tostring(lsb->lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      sandbox_terminate(lsb);
      return 1;
    }
  }
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT,
         lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT]); // reinstate limit
#else
  lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = limit;
  lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM] =
    lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT];
#endif
  return 0;
}


int serialize_binary(const void* src, size_t len, output_data* output)
{
  const char* uc = (const char*)src;
  for (unsigned i = 0; i < len; ++i) {
    switch (uc[i]) {
    case '\n':
      if (appends(output, "\\n")) return 1;
      break;
    case '\r':
      if (appends(output, "\\r")) return 1;
      break;
    case '"':
      if (appends(output, "\\\"")) return 1;
      break;
    case '\\':
      if (appends(output, "\\\\")) return 1;
      break;
    default:
      if (appendc(output, uc[i])) return 1;
      break;
    }
  }
  return 0;
}
