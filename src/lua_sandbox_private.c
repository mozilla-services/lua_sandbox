/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox private implementation @file */

#include <ctype.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lua_bloom_filter.h"
#include "lua_circular_buffer.h"
#include "lua_hyperloglog.h"
#include "lua_sandbox_private.h"
#include "lua_serialize.h"
#include "lua_serialize_protobuf.h"

#ifdef _WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif
#define MAX_PATH 255

const char* disable_none[] = { NULL };
const char* package_table = "package";
const char* loaded_table = "loaded";


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


int appendf(output_data* output, const char* fmt, ...)
{
  va_list args;
  int result = 0;
  int remaining = 0;
  char* ptr = NULL, *old_ptr = NULL;
  do {
    ptr = output->data + output->pos;
    remaining = (int)(output->size - output->pos);
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
      if (output->maxsize
          && (output->size >= output->maxsize
              || output->pos + needed >= output->maxsize)) {
        return 1;
      }
      size_t newsize = output->size * 2;
      while ((size_t)needed >= newsize - output->pos) {
        newsize *= 2;
      }
      if (output->maxsize && newsize > output->maxsize) {
        newsize = output->maxsize;
      }
      void* p = malloc(newsize);
      if (p != NULL) {
        memcpy(p, output->data, output->pos);
        old_ptr = output->data;
        output->data = p;
        output->size = newsize;
      } else {
        return 1; // Out of memory condition.
      }
    } else {
      output->pos += needed;
      break;
    }
  }
  while (1);
  free(old_ptr);
  return result;
}


int realloc_output(output_data* output, size_t needed)
{
  if (output->maxsize && needed + output->pos > output->maxsize) {
    return 1;
  }
  size_t newsize = output->size * 2;
  while (needed >= newsize - output->pos) {
    newsize *= 2;
  }
  if (output->maxsize && newsize > output->maxsize) {
    newsize = output->maxsize;
  }

  void* ptr = realloc(output->data, newsize);
  if (!ptr) return 1;
  output->data = ptr;
  output->size = newsize;
  return 0;
}


int appends(output_data* output, const char* str)
{
  size_t needed = strlen(str) + 1;
  if (output->size - output->pos < needed) {
    if (realloc_output(output, needed)) return 1;
  }
  memcpy(output->data + output->pos, str, needed);
  output->pos += needed - 1;
  return 0;
}


int appendc(output_data* output, char ch)
{
  size_t needed = 2;
  if (output->size - output->pos < needed) {
    if (realloc_output(output, needed)) return 1;
  }
  output->data[output->pos++] = ch;
  output->data[output->pos] = 0;
  return 0;
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
    return luaL_error(lua, "output() must have at least one argument");
  }
  lsb_output(lsb, 1, n, 1);
  return 0;
}

LUALIB_API int (luaopen_cjson)(lua_State* L);
LUALIB_API int (luaopen_lpeg)(lua_State* L);

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
    load_library(lua, name, luaopen_cjson, disable_none);
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, name);
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
    // Add an empty metatable to identify the library during preservation.
    lua_newtable(lua);
    lua_setmetatable(lua, -2);
  }
  lua_pushvalue(lua, -1);
  lua_setfield(lua, pos, name);
  return 1;
}
