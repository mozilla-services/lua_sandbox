/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox private implementation @file */

#include "lua_bloom_filter.h"
#include "lua_circular_buffer.h"
#include "lua_hyperloglog.h"
#include "lua_sandbox_private.h"
#include "lua_serialize.h"
#include "lua_serialize_protobuf.h"

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

const char* disable_none[] = { NULL };
const char* package_table = "package";
const char* loaded_table = "loaded";


// If necessary, add an empty metatable to flag the table as non-data
// during preservation.
static void add_empty_metatable(lua_State* lua)
{
  if (lua_getmetatable(lua, -1) == 0) {
    lua_newtable(lua);
    lua_setmetatable(lua, -2);
  } else {
    lua_pop(lua, 1);
  }
}


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
    add_empty_metatable(lua);
  }
}

#ifndef LUA_JIT
void* memory_manager(void* ud, void* ptr, size_t osize, size_t nsize)
{
  lua_sandbox* lsb = (lua_sandbox*)ud;

  void* nptr = NULL;
  if (nsize == 0) {
    free(ptr);
    lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] -= (unsigned)osize;
  } else {
    unsigned new_state_memory =
      (unsigned)(lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] + nsize - osize);
    if (0 == lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT]
        || new_state_memory
        <= lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT]) {
      nptr = realloc(ptr, nsize);
      if (nptr != NULL) {
        lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] =
          new_state_memory;
        if (lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT]
            > lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM]) {
          lsb->usage[LSB_UT_MEMORY][LSB_US_MAXIMUM] =
            lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT];
        }
      }
    }
  }
  return nptr;
}
#endif

void instruction_manager(lua_State* lua, lua_Debug* ar)
{
  if (LUA_HOOKCOUNT == ar->event) {
    luaL_error(lua, "instruction_limit exceeded");
  }
}


size_t instruction_usage(lua_sandbox* lsb)
{
  return lua_gethookcount(lsb->lua) - lua_gethookcountremaining(lsb->lua);
}


void sandbox_terminate(lua_sandbox* lsb)
{
  if (lsb->lua) {
    lua_close(lsb->lua);
    lsb->lua = NULL;
  }
  lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] = 0;
  lsb->state = LSB_TERMINATED;
}


void update_output_stats(lua_sandbox* lsb)
{
  lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = (unsigned)lsb->output.pos;
  if (lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM] =
      lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT];
  }
}


int output(lua_State* lua)
{
  void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    return luaL_error(lua, "output() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  int n = lua_gettop(lua);
  if (n == 0) {
    return luaL_argerror(lsb->lua, 0, "must have at least one argument");
  }
  lsb_output(lsb, 1, n, 1);
  return 0;
}

int require_library(lua_State* lua)
{
  const char* name = luaL_checkstring(lua, 1);
  lua_getglobal(lua, package_table);
  if (!lua_istable(lua, -1)) {
    return luaL_error(lua, "%s table is missing", package_table);
  }
  lua_getfield(lua, -1, loaded_table);
  if (!lua_istable(lua, -1)) {
    return luaL_error(lua, "%s.%s table is missing", package_table, loaded_table);
  }
  lua_getfield(lua, -1, name);
  if (!lua_isnil(lua, -1)) {
    return 1; // returned the cache copy
  }
  lua_pop(lua, 1); // remove the nil
  int pos = lua_gettop(lua);
  lua_pushboolean(lua, 1);
  lua_setfield(lua, pos, name); // mark it as loaded to prevent a dependency loop

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
  } else if (strcmp(name, lsb_circular_buffer_table) == 0) {
    load_library(lua, name, luaopen_circular_buffer, disable_none);
  } else if (strcmp(name, lsb_bloom_filter_table) == 0) {
    load_library(lua, name, luaopen_bloom_filter, disable_none);
  } else if (strcmp(name, lsb_hyperloglog_table) == 0) {
    load_library(lua, name, luaopen_hyperloglog, disable_none);
  } else if (strcmp(name, "lpeg") == 0) {
    load_library(lua, name, luaopen_lpeg, disable_none);
  } else if (strcmp(name, "cjson") == 0) {
    void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
    if (NULL == luserdata) {
      return luaL_error(lua, "require_library() invalid lightuserdata");
    }
    lua_sandbox* lsb = (lua_sandbox*)luserdata;

    const char* disable[] = { "new", "encode_keep_buffer", NULL };
    load_library(lua, name, luaopen_cjson, disable);
    if (set_encode_max_buffer(lua, -1, (unsigned)lsb->output.maxsize)) {
      return luaL_error(lua, "cjson encode buffer could not be configured");
    }
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, name);
  } else if (strcmp(name, "struct") == 0) {
    load_library(lua, name, luaopen_struct, disable_none);
  } else {
    void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
    if (NULL == luserdata) {
      return luaL_error(lua, "require_library() invalid lightuserdata");
    }
    lua_sandbox* lsb = (lua_sandbox*)luserdata;

    if (!lsb->require_path) {
      return luaL_error(lua, "require_library() external modules are disabled");
    }

    int i = 0;
    while (name[i]) {
      if (!isalnum(name[i]) && name[i] != '_') {
        return luaL_error(lua, "invalid module name '%s'", name);
      }
      ++i;
    }
    char fn[MAX_PATH];
    i = snprintf(fn, MAX_PATH, "%s%c%s.lua", lsb->require_path, PATH_DELIMITER,
                 name);
    if (i < 0 || i >= MAX_PATH) {
      return luaL_error(lua, "require_path exceeded %d", MAX_PATH);
    }

    if (luaL_dofile(lua, fn) != 0) {
      return luaL_error(lua, "%s", lua_tostring(lua, -1));
    }
    add_empty_metatable(lua);
  }
  lua_pushvalue(lua, -1);
  lua_setfield(lua, pos, name);
  return 1;
}
