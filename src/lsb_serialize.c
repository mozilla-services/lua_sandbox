/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Sandbox serialization implementation @file */

#include "luasandbox_serialize.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox/lualib.h"
#include "lsb_private.h"

#ifdef _MSC_VER
#pragma warning( disable : 4056 )
#endif

typedef struct
{
  const void* ptr;
  size_t name_pos;
} table_ref;

typedef struct
{
  size_t size;
  size_t pos;
  table_ref* array;
} table_ref_array;

typedef struct
{
  FILE* fh;
  lsb_output_data keys;
  table_ref_array tables;
  const void* globals;
} serialization_data;

static const char* preservation_version = "_PRESERVATION_VERSION";
static const char* serialize_function = "lsb_serialize";

/**
 * Serializes a Lua table structure.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return int Zero on success, non-zero on failure.
 */
static int
serialize_table(lua_sandbox* lsb, serialization_data* data, size_t parent);

/**
 * Serializes a Lua data value.
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the data resides.
 * @param output Pointer the output collector.
 *
 * @return int
 */
static int serialize_data(lua_sandbox* lsb, int index, lsb_output_data* output);

/**
 * Serializes a table key value pair.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return int Zero on success, non-zero on failure.
 */
static int
serialize_kvp(lua_sandbox* lsb, serialization_data* data, size_t parent);

/**
 * Returns the serialization function if the userdata implemented one.
 *
 * @param lua Lua State.
 * @param index Position on the stack where the userdata pointer resides.
 *
 * @return lua_CFunction NULL if not found
 */
static lua_CFunction get_serialize_function(lua_State* lua, int index)
{
  lua_CFunction fp = NULL;
  lua_getfenv(lua, index);
  lua_pushstring(lua, serialize_function);
  lua_rawget(lua, -2);
  fp = lua_tocfunction(lua, -1);
  lua_pop(lua, 2); // environment and field
  return fp;
}

/**
 * Helper function to determine what data should not be serialized.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param index Lua stack index where the data resides.
 * @param fp Function pointer to be set for userdata serialization
 *
 * @return int
 */
static int
ignore_value_type(lua_sandbox* lsb, serialization_data* data, int index,
                  lua_CFunction* fp)
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
    *fp = get_serialize_function(lsb->lua, index);
    if (!*fp) {
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


/**
 * Looks for a table to see if it has already been processed.
 *
 * @param tra Pointer to the table references.
 * @param ptr Pointer value of the table.
 *
 * @return table_ref* NULL if not found.
 */
static table_ref* find_table_ref(table_ref_array* tra, const void* ptr)
{
  for (size_t i = 0; i < tra->pos; ++i) {
    if (ptr == tra->array[i].ptr) {
      return &tra->array[i];
    }
  }
  return NULL;
}


/**
 * Adds a table to the processed array.
 *
 * @param tra Pointer to the table references.
 * @param ptr Pointer value of the table.
 * @param name_pos Index pointing to name in the table array.
 *
 * @return table_ref* Pointer to the table reference or NULL if out of memory.
 */
static table_ref*
add_table_ref(table_ref_array* tra, const void* ptr, size_t name_pos)
{
  if (tra->pos == tra->size) {
    size_t newsize = tra->size * 2;
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


/**
 * Extracts the current preservation from the sandbox.
 *
 * @param lua Lua state
 *
 * @return int Version numebr
 */
static int get_preservation_version(lua_State* lua)
{
  int ver = 0;
  lua_getglobal(lua, preservation_version);
  int t = lua_type(lua, -1);
  if (t == LUA_TNUMBER) {
    ver = (int)lua_tointeger(lua, -1);
  }
  lua_pop(lua, 1); // remove the version from the stack

  if (t != LUA_TNIL) { // remove the version from the data preservation
    lua_pushnil(lua);
    lua_setglobal(lua, preservation_version);
  }
  return ver;
}


static int
serialize_table(lua_sandbox* lsb, serialization_data* data, size_t parent)
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


static int
serialize_data(lua_sandbox* lsb, int index, lsb_output_data* output)
{
  output->pos = 0;
  switch (lua_type(lsb->lua, index)) {
  case LUA_TNUMBER:
    if (lsb_serialize_double(output, lua_tonumber(lsb->lua, index))) {
      return 1;
    }
    break;
  case LUA_TSTRING:
    // The stack is cleaned up on failure by lsb_preserve_global_data
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
      if (lsb_appendf(output, "%s", lua_tostring(lsb->lua, -1))) {
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
    if (lsb_appendf(output, "%s",
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


static int
serialize_kvp(lua_sandbox* lsb, serialization_data* data, size_t parent)
{
  int kindex = -2, vindex = -1, result = 0;
  lua_CFunction fp = NULL;
  if (ignore_value_type(lsb, data, vindex, &fp)) {
    return 0;
  }
  if (serialize_data(lsb, kindex, &lsb->output)) {
    return 1;
  }

  size_t pos = data->keys.pos;
  if (lsb_appendf(&data->keys, "%s[%s]", data->keys.data + parent,
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
    void* ud = lua_touserdata(lsb->lua, vindex);
    table_ref* seen = find_table_ref(&data->tables, ud);
    if (seen == NULL) {
      seen = add_table_ref(&data->tables, ud, pos);
      if (seen != NULL) {
        data->keys.pos += 1;
        lua_pushlightuserdata(lsb->lua, data->keys.data + pos);
        lua_pushlightuserdata(lsb->lua, &lsb->output);
        lsb->output.pos = 0;
        result = fp(lsb->lua);
        lua_pop(lsb->lua, 2); // remove the key and the output
        if (result == 0) {
          size_t n = fwrite(lsb->output.data, 1, lsb->output.pos, data->fh);
          if (n != lsb->output.pos) {
            snprintf(lsb->error_message, LSB_ERROR_SIZE,
                     "lsb_serialize failed %s", data->keys.data + pos);
            return 1;
          }
        }
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "lsb_serialize out of memory %s", data->keys.data + pos);
        return 1;
      }
    } else {
      fprintf(data->fh, "%s = ", data->keys.data + pos);
      data->keys.pos = pos;
      fprintf(data->fh, "%s\n", data->keys.data +
              seen->name_pos);
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


int lsb_preserve_global_data(lua_sandbox* lsb, const char* data_file)
{
  // make sure the string library is loaded before we start
  lua_getglobal(lsb->lua, LUA_STRLIBNAME);
  if (!lua_istable(lsb->lua, -1)) {
    lua_getglobal(lsb->lua, "require");
    if (!lua_iscfunction(lsb->lua, -1)) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "lsb_preserve_global_data 'require' not found");
      return 1;
    }
    lua_pushstring(lsb->lua, LUA_STRLIBNAME);
    lua_call(lsb->lua, 1, 1);
  }
  lua_pop(lsb->lua, 1);

  lua_pushvalue(lsb->lua, LUA_GLOBALSINDEX);

  FILE* fh = fopen(data_file, "wb");
  if (fh == NULL) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "lsb_preserve_global_data could not open: %s",
                       data_file);
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
//  size_t limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif
  lua_sethook(lsb->lua, NULL, 0, 0);
//  size_t cur_output_size = lsb->output.size;
//  size_t max_output_size = lsb->output.maxsize;
  lsb->output.maxsize = 0;
// end clear

  data.keys.size = LSB_OUTPUT_SIZE;
  data.keys.pos = 0;
  data.keys.data = malloc(data.keys.size);
  data.tables.size = 64;
  data.tables.pos = 0;
  data.tables.array = malloc(data.tables.size * sizeof(table_ref));
  if (data.tables.array == NULL || data.keys.data == NULL) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "lsb_preserve_global_data out of memory");
    result = 1;
  } else {
    fprintf(data.fh, "if %s and %s ~= %d then return end\n",
            preservation_version,
            preservation_version,
            get_preservation_version(lsb->lua));
    lsb_appends(&data.keys, "_G", 2);
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
// Note: serialization uses the output buffer, inprogress output can be
// destroyed if the user was collecting output between calls.
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


static int file_exists(const char* fn)
{
  FILE* fh = fopen(fn, "r");
  if (fh) {
    fclose(fh);
    return 1;
  }
  return 0;
}


int lsb_restore_global_data(lua_sandbox* lsb, const char* data_file)
{
  if (!file_exists(data_file)) {
    return 0;
  }

  // Clear the sandbox limits during restoration.
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT, 0);
#else
  size_t limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif
  lua_sethook(lsb->lua, NULL, 0, 0);

  int err = 0;
  if ((err = luaL_dofile(lsb->lua, data_file)) != 0) {
    if (LUA_ERRFILE != err) {
      int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                         "lsb_restore_global_data %s",
                         lua_tostring(lsb->lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      lsb_terminate(lsb, NULL);
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


void lsb_add_serialize_function(lua_State* lua, lua_CFunction fp)
{
  lua_pushstring(lua, serialize_function);
  lua_pushcfunction(lua, fp);
  lua_rawset(lua, -3);
}


int lsb_serialize_binary(const void* src, size_t len, lsb_output_data* output)
{
  const char* uc = (const char*)src;
  for (unsigned i = 0; i < len; ++i) {
    switch (uc[i]) {
    case '\n':
      if (lsb_appends(output, "\\n", 2)) return 1;
      break;
    case '\r':
      if (lsb_appends(output, "\\r", 2)) return 1;
      break;
    case '"':
      if (lsb_appends(output, "\\\"", 2)) return 1;
      break;
    case '\\':
      if (lsb_appends(output, "\\\\", 2)) return 1;
      break;
    default:
      if (lsb_appendc(output, uc[i])) return 1;
      break;
    }
  }
  return 0;
}

static int fast_double(lsb_output_data* output, double d)
{
  if (d > INT_MAX) {
    return lsb_appendf(output, "%0.17g", d);
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
    if (lsb_realloc_output(output, needed)) return 1;
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


int lsb_serialize_double(lsb_output_data* output, double d)
{
  if (isnan(d)) {
    return lsb_appends(output, "0/0", 3);
  }
  if (d == INFINITY) {
    return lsb_appends(output, "1/0", 3);
  }
  if (d == -INFINITY) {
    return lsb_appends(output, "-1/0", 4);
  }
  return fast_double(output, d);
}


int lsb_output_double(lsb_output_data* output, double d)
{
  if (isnan(d)) {
    return lsb_appends(output, "nan", 3);
  }
  if (d == INFINITY) {
    return lsb_appends(output, "inf", 3);
  }
  if (d == -INFINITY) {
    return lsb_appends(output, "-inf", 4);
  }
  return fast_double(output, d);
}
