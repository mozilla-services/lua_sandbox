
/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox modules implementation @file */

#include "lsb_modules.h"

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <string.h>

#include "lsb_private.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifdef _WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

const char* lsb_disable_none[] = { NULL };
const char* lsb_package_table = "package";
const char* lsb_loaded_table = "loaded";

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


void lsb_load_library(lua_State* lua, const char* table, lua_CFunction f,
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


int lsb_require_library(lua_State* lua)
{
  const char* name = luaL_checkstring(lua, 1);
  lua_getglobal(lua, lsb_package_table);
  if (!lua_istable(lua, -1)) {
    return luaL_error(lua, "%s table is missing", lsb_package_table);
  }
  lua_getfield(lua, -1, lsb_loaded_table);
  if (!lua_istable(lua, -1)) {
    return luaL_error(lua, "%s.%s table is missing", lsb_package_table,
                      lsb_loaded_table);
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
    lsb_load_library(lua, name, luaopen_string, lsb_disable_none);
  } else if (strcmp(name, LUA_MATHLIBNAME) == 0) {
    lsb_load_library(lua, name, luaopen_math, lsb_disable_none);
  } else if (strcmp(name, LUA_TABLIBNAME) == 0) {
    lsb_load_library(lua, name, luaopen_table, lsb_disable_none);
  } else if (strcmp(name, LUA_OSLIBNAME) == 0) {
    const char* disable[] = { "execute", "exit", "remove", "rename",
      "setlocale", "tmpname", NULL };
    lsb_load_library(lua, name, luaopen_os, disable);
  } else if (strcmp(name, mozsvc_circular_buffer_table) == 0) {
    lsb_load_library(lua, name, luaopen_circular_buffer, lsb_disable_none);
  } else if (strcmp(name, mozsvc_bloom_filter_table) == 0) {
    lsb_load_library(lua, name, luaopen_bloom_filter, lsb_disable_none);
  } else if (strcmp(name, mozsvc_hyperloglog_table) == 0) {
    lsb_load_library(lua, name, luaopen_hyperloglog, lsb_disable_none);
  } else if (strcmp(name, "lpeg") == 0) {
    lsb_load_library(lua, name, luaopen_lpeg, lsb_disable_none);
  } else if (strcmp(name, "cjson") == 0) {
    void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
    if (NULL == luserdata) {
      return luaL_error(lua, "require_library() invalid lightuserdata");
    }
    lua_sandbox* lsb = (lua_sandbox*)luserdata;

    const char* disable[] = { "new", "encode_keep_buffer", NULL };
    lsb_load_library(lua, name, luaopen_cjson, disable);
    if (set_encode_max_buffer(lua, -1, (unsigned)lsb->output.maxsize)) {
      return luaL_error(lua, "cjson encode buffer could not be configured");
    }
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, name);
  } else if (strcmp(name, "struct") == 0) {
    lsb_load_library(lua, name, luaopen_struct, lsb_disable_none);
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
