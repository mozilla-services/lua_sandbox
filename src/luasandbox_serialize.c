/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Sandbox serialization implementation @file */

#define LUA_LIB
#include "luasandbox_serialize.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox/lualib.h"
#include "luasandbox_defines.h"
#include "luasandbox_impl.h"

#ifdef _MSC_VER
#pragma warning( disable : 4056 )
#endif

typedef struct
{
  const void *ptr;
  size_t name_pos;
} table_ref;

typedef struct
{
  size_t size;
  size_t pos;
  table_ref *array;
} table_ref_array;

typedef struct
{
  FILE *fh;
  lsb_output_buffer keys;
  table_ref_array tables;
  const void *globals;
} serialization_data;

static const char *preservation_version = "_PRESERVATION_VERSION";
static const char *serialize_function = "lsb_serialize";

/**
 * Serializes a Lua table structure.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
serialize_table(lsb_lua_sandbox *lsb, serialization_data *data, size_t parent);

/**
 * Serializes a Lua data value.
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the data resides.
 * @param ob Pointer the output buffer.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value serialize_data(lsb_lua_sandbox *lsb,
                                    int index,
                                    lsb_output_buffer *ob);

/**
 * Serializes a table key value pair.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
serialize_kvp(lsb_lua_sandbox *lsb, serialization_data *data, size_t parent);

/**
 * Returns the serialization function if the userdata implemented one.
 *
 * @param lua Lua State.
 * @param index Position on the stack where the userdata pointer resides.
 *
 * @return lua_CFunction NULL if not found
 */
static lua_CFunction get_serialize_function(lua_State *lua, int index)
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
ignore_value_type(lsb_lua_sandbox *lsb, serialization_data *data, int index,
                  lua_CFunction *fp)
{
  switch (lua_type(lsb->lua, index)) {
  case LUA_TSTRING:
  case LUA_TNUMBER:
  case LUA_TBOOLEAN:
    return 0;
  case LUA_TTABLE:
    if (lua_getmetatable(lsb->lua, index) != 0) {
      lua_pop(lsb->lua, 1); // Remove the metatable.
      return 1;
    }
    if (lua_topointer(lsb->lua, index) == data->globals) {
      return 1;
    }
    return 0;
  case LUA_TUSERDATA:
    *fp = get_serialize_function(lsb->lua, index);
    return !*fp ? 1 : 0;
  default:
    break;
  }
  return 1;
}


/**
 * Looks for a table to see if it has already been processed.
 *
 * @param tra Pointer to the table references.
 * @param ptr Pointer value of the table.
 *
 * @return table_ref* NULL if not found.
 */
static table_ref* find_table_ref(table_ref_array *tra, const void *ptr)
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
add_table_ref(table_ref_array *tra, const void *ptr, size_t name_pos)
{
  if (tra->pos == tra->size) {
    size_t newsize = tra->size * 2;
    void *p = realloc(tra->array, newsize * sizeof(table_ref));
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
static int get_preservation_version(lua_State *lua)
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


static lsb_err_value
serialize_table(lsb_lua_sandbox *lsb, serialization_data *data, size_t parent)
{
  lsb_err_value ret = NULL;
  lua_checkstack(lsb->lua, 2);
  lua_pushnil(lsb->lua);
  while (!ret && lua_next(lsb->lua, -2) != 0) {
    ret = serialize_kvp(lsb, data, parent);
    lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                          // the next interation.
  }
  return ret;
}


static lsb_err_value
serialize_data(lsb_lua_sandbox *lsb, int index, lsb_output_buffer *ob)
{
  lsb_err_value ret = NULL;
  ob->pos = 0; // clear the buffer
  switch (lua_type(lsb->lua, index)) {
  case LUA_TNUMBER:
    ret = lsb_serialize_double(ob, lua_tonumber(lsb->lua, index));
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
      return LSB_ERR_LUA;
    }
    lua_pushstring(lsb->lua, "%q");
    lua_pushvalue(lsb->lua, index - 3);
    if (lua_pcall(lsb->lua, 2, 1, 0) == 0) {
      const char* em = lua_tostring(lsb->lua, -1);
      ret = lsb_outputf(ob, "%s", em ? em : LSB_NIL_ERROR);
      if (ret) {
        lua_pop(lsb->lua, 1); // Remove the string table.
        return ret;
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
      return LSB_ERR_LUA;
    }
    lua_pop(lsb->lua, 2); // Remove the pcall result and the string table.
    break;
  case LUA_TBOOLEAN:
    ret = lsb_outputf(ob, "%s", lua_toboolean(lsb->lua, index) ? "true" :
                                                                 "false");
    break;
  default:
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "serialize_data cannot preserve type '%s'",
             lua_typename(lsb->lua, lua_type(lsb->lua, index)));
    ret = LSB_ERR_LUA;
  }
  return ret;
}


static lsb_err_value
serialize_kvp(lsb_lua_sandbox *lsb, serialization_data *data, size_t parent)
{
  lsb_err_value ret = NULL;
  lua_CFunction fp = NULL;
  int kindex = -2, vindex = -1;
  if (ignore_value_type(lsb, data, vindex, &fp)) {
    return ret;
  }
  ret = serialize_data(lsb, kindex, &lsb->output);
  if (ret) {
    return ret;
  }

  size_t pos = data->keys.pos;
  ret = lsb_outputf(&data->keys, "%s[%s]", data->keys.buf + parent,
                    lsb->output.buf);
  if (ret) return ret;

  if (lua_type(lsb->lua, vindex) == LUA_TTABLE) {
    const void *ptr = lua_topointer(lsb->lua, vindex);
    table_ref *seen = find_table_ref(&data->tables, ptr);
    if (seen == NULL) {
      seen = add_table_ref(&data->tables, ptr, pos);
      if (seen != NULL) {
        data->keys.pos += 1;
        if (lua_tabletype(lsb->lua, vindex) == LUA_TTARRAY
            && lua_objlen(lsb->lua, vindex) == 0) {
            fprintf(data->fh, "%s = {nil}\n", data->keys.buf + pos);
        } else {
          fprintf(data->fh, "%s = {}\n", data->keys.buf + pos);
          ret = serialize_table(lsb, data, pos);
        }
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "lsb_serialize preserve table out of memory");
        return LSB_ERR_UTIL_OOM;
      }
    } else {
      fprintf(data->fh, "%s = ", data->keys.buf + pos);
      data->keys.pos = pos;
      fprintf(data->fh, "%s\n", data->keys.buf + seen->name_pos);
    }
  } else if (lua_type(lsb->lua, vindex) == LUA_TUSERDATA) {
    void *ud = lua_touserdata(lsb->lua, vindex);
    table_ref *seen = find_table_ref(&data->tables, ud);
    if (seen == NULL) {
      seen = add_table_ref(&data->tables, ud, pos);
      if (seen != NULL) {
        data->keys.pos += 1;
        lua_pushlightuserdata(lsb->lua, data->keys.buf + pos);
        lua_pushlightuserdata(lsb->lua, &lsb->output);
        lsb->output.pos = 0;
        int result = fp(lsb->lua);
        lua_pop(lsb->lua, 2); // remove the key and the output
        if (!result) {
          size_t n = fwrite(lsb->output.buf, 1, lsb->output.pos, data->fh);
          if (n != lsb->output.pos) {
            snprintf(lsb->error_message, LSB_ERROR_SIZE,
                     "lsb_serialize failed %s", data->keys.buf + pos);
            return LSB_ERR_LUA;
          }
        }
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "lsb_serialize out of memory %s", data->keys.buf + pos);
        return LSB_ERR_UTIL_OOM;
      }
    } else {
      fprintf(data->fh, "%s = ", data->keys.buf + pos);
      data->keys.pos = pos;
      fprintf(data->fh, "%s\n", data->keys.buf +
              seen->name_pos);
    }
  } else {
    fprintf(data->fh, "%s = ", data->keys.buf + pos);
    data->keys.pos = pos;
    ret = serialize_data(lsb, vindex, &lsb->output);
    if (!ret) {
      fprintf(data->fh, "%s\n", lsb->output.buf);
    }
  }
  return ret;
}


lsb_err_value preserve_global_data(lsb_lua_sandbox *lsb)
{

  if (!lsb->lua || !lsb->state_file || lsb->state == LSB_TERMINATED) {
    return NULL;
  }
  lua_sethook(lsb->lua, NULL, 0, 0);

  // make sure the string library is loaded before we start
  lua_getglobal(lsb->lua, LUA_STRLIBNAME);
  if (!lua_istable(lsb->lua, -1)) {
    lua_getglobal(lsb->lua, "require");
    if (!lua_iscfunction(lsb->lua, -1)) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "preserve_global_data 'require' function not found");
      return LSB_ERR_LUA;
    }
    lua_pushstring(lsb->lua, LUA_STRLIBNAME);
    if (lua_pcall(lsb->lua, 1, 1, 0)) {
      int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                         "preserve_global_data failed loading 'string'");
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      return LSB_ERR_LUA;
    }
  }
  lua_pop(lsb->lua, 1);

  lua_pushvalue(lsb->lua, LUA_GLOBALSINDEX);

  FILE *fh = fopen(lsb->state_file, "wb" CLOSE_ON_EXEC);
  if (fh == NULL) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "preserve_global_data could not open: %s",
                       lsb->state_file);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return LSB_ERR_LUA;;
  }

  lsb_err_value ret = NULL;
  serialization_data data;
  data.fh = fh;

// Clear the sandbox limits during preservation.
//  size_t limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
//  size_t cur_output_size = lsb->output.size;
//  size_t max_output_size = lsb->output.maxsize;
  lsb->output.maxsize = 0;
// end clear

  data.tables.size = 64;
  data.tables.pos = 0;
  data.tables.array = malloc(data.tables.size * sizeof(table_ref));
  if (data.tables.array == NULL || lsb_init_output_buffer(&data.keys, 0)) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "preserve_global_data out of memory");
    ret = LSB_ERR_UTIL_OOM;
  } else {
    fprintf(data.fh, "if %s and %s ~= %d then return end\n",
            preservation_version,
            preservation_version,
            get_preservation_version(lsb->lua));
    ret = lsb_outputs(&data.keys, "_G", 2);
    if (!ret) {
      data.keys.pos += 1; // preserve the NUL in this use case
      data.globals = lua_topointer(lsb->lua, -1);
      lua_checkstack(lsb->lua, 2);
      lua_pushnil(lsb->lua);
      while (!ret && lua_next(lsb->lua, -2) != 0) {
        ret = serialize_kvp(lsb, &data, 0);
        lua_pop(lsb->lua, 1);
      }
    }
    lua_pop(lsb->lua, lua_gettop(lsb->lua));
    // Wipe the entire Lua stack.  Since incremental cleanup on failure
    // was added the stack should only contain table _G.
  }
  free(data.tables.array);
  lsb_free_output_buffer(&data.keys);
  fclose(fh);
  if (ret) remove(lsb->state_file);

// Uncomment if we start preserving state when not destroying the sandbox
// Note: serialization uses the output buffer, inprogress output can be
// destroyed if the user was collecting output between calls.
/*
// Restore the sandbox limits after preservation
  lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = limit;
  lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM] =
    lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT];
  lsb->output.maxsize = max_output_size;
  lsb_clear_output_buffer(lsb->output);
  if (lsb->output.size > cur_output_size) {
    void* ptr = realloc(lsb->output.data, cur_output_size);
    if (!ptr) return 1;
    lsb->output.data = ptr;
    lsb->output.size = cur_output_size;
  }
// end restore
*/
  return ret;
}


static int file_exists(const char *fn)
{
  FILE *fh = fopen(fn, "r" CLOSE_ON_EXEC);
  if (fh) {
    fclose(fh);
    return 1;
  }
  return 0;
}


lsb_err_value restore_global_data(lsb_lua_sandbox *lsb)
{
  if (!lsb) {
    return LSB_ERR_UTIL_NULL;
  }
  if (!lsb->state_file || !file_exists(lsb->state_file)) {
    return NULL;
  }

  // Clear the sandbox limits during restoration.
  size_t limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
  lua_sethook(lsb->lua, NULL, 0, 0);

  int err = luaL_dofile(lsb->lua, lsb->state_file);
  if (err) {
    if (LUA_ERRFILE != err) {
      int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                         "restore_global_data %s",
                         lua_tostring(lsb->lua, -1));
      if (len >= LSB_ERROR_SIZE || len < 0) {
        lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
      }
      lsb_terminate(lsb, NULL);
      return LSB_ERR_LUA;
    }
  }
  lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = limit;
  lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM] =
      lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT];
  return NULL;
}


void lsb_add_serialize_function(lua_State *lua, lua_CFunction fp)
{
  lua_pushstring(lua, serialize_function);
  lua_pushcfunction(lua, fp);
  lua_rawset(lua, -3);
}


lsb_err_value lsb_serialize_binary(lsb_output_buffer *ob, const void *src, size_t len)
{
  lsb_err_value ret = NULL;
  const char *uc = (const char *)src;
  for (unsigned i = 0; !ret && i < len; ++i) {
    switch (uc[i]) {
    case '\n':
      ret = lsb_outputs(ob, "\\n", 2);
      break;
    case '\r':
      ret = lsb_outputs(ob, "\\r", 2);
      break;
    case '"':
      ret = lsb_outputs(ob, "\\\"", 2);
      break;
    case '\\':
      ret = lsb_outputs(ob, "\\\\", 2);
      break;
    default:
      ret = lsb_outputc(ob, uc[i]);
      break;
    }
  }
  return ret;
}


lsb_err_value lsb_serialize_double(lsb_output_buffer *ob, double d)
{
  if (isnan(d)) {
    return lsb_outputs(ob, "0/0", 3);
  }
  if (d == INFINITY) {
    return lsb_outputs(ob, "1/0", 3);
  }
  if (d == -INFINITY) {
    return lsb_outputs(ob, "-1/0", 4);
  }
  return lsb_outputfd(ob, d);
}
