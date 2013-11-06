/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Sandboxed Lua execution @file
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <time.h>
#include "lua_sandbox_private.h"
#include "lua_serialize.h"
#include "lua_serialize_json.h"
#include "lua_serialize_protobuf.h"
#include "lua_circular_buffer.h"

const char* disable_none[] = { NULL };

////////////////////////////////////////////////////////////////////////////////
void load_library(lua_State* lua, const char* table, lua_CFunction f,
                  const char** disable)
{
  lua_pushcfunction(lua, f);
  lua_call(lua, 0, 1);

  if (strlen(table) == 0) { // Handle the special "" base table.
    for (int i = 0; disable[i]; ++i) {
      lua_pushnil(lua);
      lua_setfield(lua, LUA_GLOBALSINDEX, disable[i]);
    }
  } else {
    for (int i = 0; disable[i]; ++i) {
      lua_pushnil(lua);
      lua_setfield(lua, -2, disable[i]);
    }
    // Add an empty metatable to identify core libraries during
    // preservation.
    lua_newtable(lua);
    lua_setmetatable(lua, -2);
  }
}

////////////////////////////////////////////////////////////////////////////////
void instruction_manager(lua_State* lua, lua_Debug* ar)
{
  if (LUA_HOOKCOUNT == ar->event) {
    luaL_error(lua, "instruction_limit exceeded");
  }
}

////////////////////////////////////////////////////////////////////////////////
size_t instruction_usage(lua_sandbox* lsb)
{
  return lua_gethookcount(lsb->m_lua) - lua_gethookcountremaining(lsb->m_lua);
}

////////////////////////////////////////////////////////////////////////////////
void sandbox_terminate(lua_sandbox* lsb)
{
  if (lsb->m_lua) {
    lua_close(lsb->m_lua);
    lsb->m_lua = NULL;
  }
  lsb->m_usage[LSB_UT_MEMORY][LSB_US_CURRENT] = 0;
  lsb->m_state = LSB_TERMINATED;
}

////////////////////////////////////////////////////////////////////////////////
void update_output_stats(lua_sandbox* lsb)
{
  lsb->m_usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = lsb->m_output.m_pos;
  if (lsb->m_usage[LSB_UT_OUTPUT][LSB_US_CURRENT]
      > lsb->m_usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM]) {
    lsb->m_usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM] =
      lsb->m_usage[LSB_UT_OUTPUT][LSB_US_CURRENT];
  }
}

////////////////////////////////////////////////////////////////////////////////
int appendf(output_data* output, const char* fmt, ...)
{
  va_list args;
  int result = 0;
  int remaining = 0;
  char* ptr = NULL, *old_ptr = NULL;
  do {
    ptr = output->m_data + output->m_pos;
    remaining = output->m_size - output->m_pos;
    va_start(args, fmt);
    int needed = vsnprintf(ptr, remaining, fmt, args);
    va_end(args);
    if (needed == -1) {
      // Windows and Unix have different return values for this function
      // -1 on Unix is a format error
      // -1 on Windows means the buffer is too small and the required len
      // is not returned
      needed = remaining;
    }
    if (needed >= remaining) {
      if (output->m_maxsize
          && (output->m_size >= output->m_maxsize
              || output->m_pos + needed >= output->m_maxsize)) {
        return 1;
      }
      size_t newsize = output->m_size * 2;
      while ((size_t)needed >= newsize - output->m_pos) {
        newsize *= 2;
      }
      if (output->m_maxsize && newsize > output->m_maxsize) {
        newsize = output->m_maxsize;
      }
      void* p = malloc(newsize);
      if (p != NULL) {
        memcpy(p, output->m_data, output->m_pos);
        old_ptr = output->m_data;
        output->m_data = p;
        output->m_size = newsize;
      } else {
        return 1; // Out of memory condition.
      }
    } else {
      output->m_pos += needed;
      break;
    }
  }
  while (1);
  free(old_ptr);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int realloc_output(output_data* output, size_t needed)
{
  if (output->m_maxsize && needed + output->m_pos > output->m_maxsize) return 1;
  size_t newsize = output->m_size * 2;
  while (needed >= newsize - output->m_pos) {
    newsize *= 2;
  }
  if (output->m_maxsize && newsize > output->m_maxsize) {
    newsize = output->m_maxsize;
  }

  void* ptr = realloc(output->m_data, newsize);
  if (!ptr) return 1;
  output->m_data = ptr;
  output->m_size = newsize;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int appends(output_data* output, const char* str)
{
  size_t needed = strlen(str) + 1;
  if (output->m_size - output->m_pos < needed) {
    if (realloc_output(output, needed)) return 1;
  }
  memcpy(output->m_data + output->m_pos, str, needed);
  output->m_pos += needed - 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int appendc(output_data* output, char ch)
{
  size_t needed = 2;
  if (output->m_size - output->m_pos < needed) {
    if (realloc_output(output, needed)) return 1;
  }
  output->m_data[output->m_pos++] = ch;
  output->m_data[output->m_pos] = 0;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int output(lua_State* lua)
{
  void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    luaL_error(lua, "output() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  int n = lua_gettop(lua);
  if (n == 0) {
    luaL_error(lua, "output() must have at least one argument");
  }

  int result = 0;
  void* ud = NULL;
  for (int i = 1; result == 0 && i <= n; ++i) {
    switch (lua_type(lua, i)) {
    case LUA_TNUMBER:
      if (serialize_double(&lsb->m_output, lua_tonumber(lua, i))) {
        result = 1;
      }
      break;
    case LUA_TSTRING:
      if (appendf(&lsb->m_output, "%s", lua_tostring(lua, i))) {
        result = 1;
      }
      break;
    case LUA_TNIL:
      if (appends(&lsb->m_output, "nil")) {
        result = 1;
      }
      break;
    case LUA_TBOOLEAN:
      if (appendf(&lsb->m_output, "%s",
                  lua_toboolean(lsb->m_lua, i)
                  ? "true" : "false")) {
        result = 1;
      }
      break;
    case LUA_TTABLE: // encode as JSON
      if (!appendc(&lsb->m_output, '{')) {
        serialization_data data;
        data.m_globals = NULL;
        data.m_tables.m_size = 64;
        data.m_tables.m_pos = 0;
        data.m_tables.m_array = malloc(data.m_tables.m_size * sizeof(table_ref));
        if (!data.m_tables.m_array) {
          snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                   "json table serialization out of memory");
          result = 1;
        } else {
          lua_checkstack(lsb->m_lua, 2);
          lua_getfield(lsb->m_lua, i, "_name");
          if (lua_type(lsb->m_lua, -1) != LUA_TSTRING) {
            lua_pop(lsb->m_lua, 1); // remove the failed _name result
            lua_pushstring(lsb->m_lua, "table"); // add default name
          }
          lua_pushvalue(lsb->m_lua, i);
          result = serialize_kvp_as_json(lsb, &data, 1);
          if (result == 0) {
            result = appends(&lsb->m_output, "}\n");
          }
          lua_pop(lsb->m_lua, 2); // remove the name and copy of the table
          free(data.m_tables.m_array);
        }
      } else {
        result = 1;
      }
      break;
    case LUA_TUSERDATA:
      ud = lua_touserdata(lua, i);
      if (heka_circular_buffer == userdata_type(lua, ud, i)) {
        if (output_circular_buffer(lua, (circular_buffer*)ud,
                                   &lsb->m_output)) {
          result = 1;
        }
      }
      break;
    }
  }
  update_output_stats(lsb);
  if (result != 0) {
    if (lsb->m_error_message[0] == 0) {
      luaL_error(lua, "output_limit exceeded");
    }
    luaL_error(lua, lsb->m_error_message);
  }
  return 0;
}

LUALIB_API int (luaopen_cjson_safe)(lua_State* L);
LUALIB_API int (luaopen_lpeg)(lua_State* L);
////////////////////////////////////////////////////////////////////////////////
int require_library(lua_State* lua)
{
  const char* name = luaL_checkstring(lua, 1);
  if (strcmp(name, LUA_STRLIBNAME) == 0) {
    load_library(lua, name, luaopen_string, disable_none);
  } else  if (strcmp(name, LUA_MATHLIBNAME) == 0) {
    load_library(lua, name, luaopen_math, disable_none);
  } else  if (strcmp(name, LUA_TABLIBNAME) == 0) {
    load_library(lua, name, luaopen_table, disable_none);
  } else if (strcmp(name, LUA_OSLIBNAME) == 0) {
    const char* disable[] = { "execute", "exit", "remove", "rename",
      "setlocale",  "tmpname", NULL };
    load_library(lua, name, luaopen_os, disable);
  } else if (strcmp(name, heka_circular_buffer_table) == 0) {
    load_library(lua, name, luaopen_circular_buffer, disable_none);
  } else if (strcmp(name, "lpeg") == 0) {
    load_library(lua, name, luaopen_lpeg, disable_none);
  } else if (strcmp(name, "cjson") == 0) {
    const char* disable[] = { "encode",  "encode_sparse_array",
      "encode_max_depth", "encode_number_precision", "encode_keep_buffer",
      "encode_invalid_numbers", NULL };
    load_library(lua, name, luaopen_cjson_safe, disable);
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, name);
  } else {
    luaL_error(lua, "library '%s' is not available", name);
  }
  return 1;
}
