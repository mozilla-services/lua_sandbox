/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Sandbox serialization implementation @file

#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include "lua_serialize.h"
#include "lua_circular_buffer.h"

////////////////////////////////////////////////////////////////////////////////
int preserve_global_data(lua_sandbox* lsb, const char* data_file)
{
  static const char* G = "_G";
  lua_getglobal(lsb->m_lua, G);
  if (!lua_istable(lsb->m_lua, -1)) {
    snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
             "preserve_global_data cannot access the global table");
    return 1;
  }

  FILE* fh = fopen(data_file, "wb");
  if (fh == NULL) {
    int len = snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                       "preserve_global_data could not open: %s", data_file);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return 1;
  }

  int result = 0;
  serialization_data data;
  data.m_fh = fh;
  data.m_keys.m_maxsize = 0;
  lsb->m_output.m_maxsize = 0; // clear output limit
  data.m_keys.m_size = OUTPUT_SIZE;
  data.m_keys.m_pos = 0;
  data.m_keys.m_data = malloc(data.m_keys.m_size);
  data.m_tables.m_size = 64;
  data.m_tables.m_pos = 0;
  data.m_tables.m_array = malloc(data.m_tables.m_size * sizeof(table_ref));
  if (data.m_tables.m_array == NULL || data.m_keys.m_data == NULL) {
    snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
             "preserve_global_data out of memory");
    result = 1;
  } else {
    appendf(&data.m_keys, "%s", G);
    data.m_keys.m_pos += 1;
    data.m_globals = lua_topointer(lsb->m_lua, -1);
    lua_checkstack(lsb->m_lua, 2);
    lua_pushnil(lsb->m_lua);
    while (result == 0 && lua_next(lsb->m_lua, -2) != 0) {
      result = serialize_kvp(lsb, &data, 0);
      lua_pop(lsb->m_lua, 1);
    }
    lua_pop(lsb->m_lua, lua_gettop(lsb->m_lua));
    // Wipe the entire Lua stack.  Since incremental cleanup on failure
    // was added the stack should only contain table _G.
  }
  free(data.m_tables.m_array);
  free(data.m_keys.m_data);
  fclose(fh);
  if (result != 0) {
    remove(data_file);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int serialize_double(output_data* output, double d)
{
  if (d > INT_MAX) {
    return appendf(output, "%0.9g", d);
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

  size_t remaining = output->m_size - output->m_pos;
  size_t needed = (p - buffer) + negative;
  if (needed >= remaining) {
    if (realloc_output(output, needed)) return 1;
  }

  if (negative) {
    output->m_data[output->m_pos++] = '-';
  }
  do {
    --p;
    output->m_data[output->m_pos++] = *p;
  }
  while (p != buffer);
  output->m_data[output->m_pos] = 0;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int serialize_table(lua_sandbox* lsb, serialization_data* data, size_t parent)
{
  int result = 0;
  lua_checkstack(lsb->m_lua, 2);
  lua_pushnil(lsb->m_lua);
  while (result == 0 && lua_next(lsb->m_lua, -2) != 0) {
    result = serialize_kvp(lsb, data, parent);
    lua_pop(lsb->m_lua, 1); // Remove the value leaving the key on top for
                            // the next interation.
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int serialize_data(lua_sandbox* lsb, int index, output_data* output)
{
  output->m_pos = 0;
  switch (lua_type(lsb->m_lua, index)) {
  case LUA_TNUMBER:
    if (serialize_double(output, lua_tonumber(lsb->m_lua, index))) {
      return 1;
    }
    break;
  case LUA_TSTRING:
    // The stack is cleaned up on failure by preserve_global_data
    // but for clarity it is incrementally cleaned up anyway.
    lua_checkstack(lsb->m_lua, 4);
    lua_getglobal(lsb->m_lua, LUA_STRLIBNAME);
    if (!lua_istable(lsb->m_lua, -1)) {
      lua_pop(lsb->m_lua, 1); // Remove nil string table.
      load_library(lsb->m_lua, LUA_STRLIBNAME, luaopen_string, disable_none);
    }
    lua_getfield(lsb->m_lua, -1, "format");
    if (!lua_isfunction(lsb->m_lua, -1)) {
      snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
               "serialize_data cannot access the string format function");
      lua_pop(lsb->m_lua, 2); // Remove the bogus format function and
                              // string table.
      return 1;
    }
    lua_pushstring(lsb->m_lua, "%q");
    lua_pushvalue(lsb->m_lua, index - 3);
    if (lua_pcall(lsb->m_lua, 2, 1, 0) == 0) {
      if (appendf(output, "%s", lua_tostring(lsb->m_lua, -1))) {
        lua_pop(lsb->m_lua, 1); // Remove the string table.
        return 1;
      }
    } else {
      int len = snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                         "serialize_data '%s'",
                         lua_tostring(lsb->m_lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      lua_pop(lsb->m_lua, 2); // Remove the error message and the string
                              // table.
      return 1;
    }
    lua_pop(lsb->m_lua, 2); // Remove the pcall result and the string table.
    break;
  case LUA_TBOOLEAN:
    if (appendf(output, "%s",
                lua_toboolean(lsb->m_lua, index)
                ? "true" : "false")) {
      return 1;
    }
    break;
  default:
    snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
             "serialize_data cannot preserve type '%s'",
             lua_typename(lsb->m_lua, lua_type(lsb->m_lua, index)));
    return 1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
const char* userdata_type(lua_State* lua, void* ud, int index)
{
  const char* table = NULL;
  if (ud == NULL) return table;

  if (lua_getmetatable(lua, index)) {
    lua_getfield(lua, LUA_REGISTRYINDEX, heka_circular_buffer);
    if (lua_rawequal(lua, -1, -2)) {
      table = heka_circular_buffer;
    }
  }
  lua_pop(lua, 2); // metatable and field
  return table;
}

////////////////////////////////////////////////////////////////////////////////
int serialize_kvp(lua_sandbox* lsb, serialization_data* data, size_t parent)
{
  int kindex = -2, vindex = -1, result = 0;

  if (ignore_value_type(lsb, data, vindex)) return 0;
  if (serialize_data(lsb, kindex, &lsb->m_output)) return 1;

  size_t pos = data->m_keys.m_pos;
  if (appendf(&data->m_keys, "%s[%s]", data->m_keys.m_data + parent,
              lsb->m_output.m_data)) {
    return 1;
  }

  if (lua_type(lsb->m_lua, vindex) == LUA_TTABLE) {
    const void* ptr = lua_topointer(lsb->m_lua, vindex);
    table_ref* seen = find_table_ref(&data->m_tables, ptr);
    if (seen == NULL) {
      seen = add_table_ref(&data->m_tables, ptr, pos);
      if (seen != NULL) {
        data->m_keys.m_pos += 1;
        fprintf(data->m_fh, "%s = {}\n", data->m_keys.m_data + pos);
        result = serialize_table(lsb, data, pos);
      } else {
        snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                 "preserve table out of memory");
        return 1;
      }
    } else {
      fprintf(data->m_fh, "%s = ", data->m_keys.m_data + pos);
      data->m_keys.m_pos = pos;
      fprintf(data->m_fh, "%s\n", data->m_keys.m_data + seen->m_name_pos);
    }
  } else if (lua_type(lsb->m_lua, vindex) == LUA_TUSERDATA) {
    void* ud = lua_touserdata(lsb->m_lua, vindex);
    if (heka_circular_buffer == userdata_type(lsb->m_lua, ud, vindex)) {
      table_ref* seen = find_table_ref(&data->m_tables, ud);
      if (seen == NULL) {
        seen = add_table_ref(&data->m_tables, ud, pos);
        if (seen != NULL) {
          data->m_keys.m_pos += 1;
          result = serialize_circular_buffer(lsb->m_lua,
                                             data->m_keys.m_data + pos,
                                             (circular_buffer*)ud,
                                             &lsb->m_output);
          if (result == 0) {
            fprintf(data->m_fh, "%s", lsb->m_output.m_data);
          }
        } else {
          snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                   "preserve table out of memory");
          return 1;
        }
      } else {
        fprintf(data->m_fh, "%s = ", data->m_keys.m_data + pos);
        data->m_keys.m_pos = pos;
        fprintf(data->m_fh, "%s\n", data->m_keys.m_data +
                seen->m_name_pos);
      }
    }
  } else {
    fprintf(data->m_fh, "%s = ", data->m_keys.m_data + pos);
    data->m_keys.m_pos = pos;
    result = serialize_data(lsb, vindex, &lsb->m_output);
    if (result == 0) {
      fprintf(data->m_fh, "%s\n", lsb->m_output.m_data);
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int ignore_key(lua_sandbox* lsb, int index)
{
  if (lua_type(lsb->m_lua, index) == LUA_TSTRING) {
    const char* key = lua_tostring(lsb->m_lua, index);
    if (key[0] == '_') {
      return 1;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
table_ref* find_table_ref(table_ref_array* tra, const void* ptr)
{
  for (size_t i = 0; i < tra->m_pos; ++i) {
    if (ptr == tra->m_array[i].m_ptr) {
      return &tra->m_array[i];
    }
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
table_ref* add_table_ref(table_ref_array* tra, const void* ptr, size_t name_pos)
{
  if (tra->m_pos == tra->m_size) {
    size_t newsize =  tra->m_size * 2;
    void* p = realloc(tra->m_array, newsize * sizeof(table_ref));
    if (p != NULL) {
      tra->m_array = p;
      tra->m_size = newsize;
    } else {
      return NULL;
    }
  }
  tra->m_array[tra->m_pos].m_ptr = ptr;
  tra->m_array[tra->m_pos].m_name_pos = name_pos;
  return &tra->m_array[tra->m_pos++];
}

////////////////////////////////////////////////////////////////////////////////
int ignore_value_type(lua_sandbox* lsb, serialization_data* data, int index)
{
  void* ud = NULL;
  switch (lua_type(lsb->m_lua, index)) {
  case LUA_TTABLE:
    if (lua_getmetatable(lsb->m_lua, index) != 0) {
      lua_pop(lsb->m_lua, 1); // Remove the metatable.
      return 1;
    }
    if (lua_topointer(lsb->m_lua, index) == data->m_globals) {
      return 1;
    }
    break;
  case LUA_TUSERDATA:
    ud = lua_touserdata(lsb->m_lua, index);
    if ((heka_circular_buffer != userdata_type(lsb->m_lua, ud, index))) {
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

////////////////////////////////////////////////////////////////////////////////
int restore_global_data(lua_sandbox* lsb, const char* data_file)
{
  // Clear the sandbox limits during restoration.
  lua_gc(lsb->m_lua, LUA_GCSETMEMLIMIT, 0);
  lua_sethook(lsb->m_lua, instruction_manager, 0, 0);

  int err = 0;
  if ((err = luaL_dofile(lsb->m_lua, data_file)) != 0) {
    if (LUA_ERRFILE != err) {
      int len = snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                         "restore_global_data %s",
                         lua_tostring(lsb->m_lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      sandbox_terminate(lsb);
      return 1;
    }
  }
  lua_gc(lsb->m_lua, LUA_GCSETMEMLIMIT,
         lsb->m_usage[LSB_UT_MEMORY][LSB_US_LIMIT]); // reinstate limit
  return 0;
}
