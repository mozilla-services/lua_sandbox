/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox output buffer implementation @file */

#define LUA_LIB
#include "luasandbox_output.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox_impl.h"
#include "luasandbox_serialize.h"

static const char *output_function = "lsb_output";
static const char *zero_copy_function = "lsb_zero_copy";

void lsb_add_output_function(lua_State *lua, lua_CFunction fp)
{
  lua_pushstring(lua, output_function);
  lua_pushcfunction(lua, fp);
  lua_rawset(lua, -3);
}


lua_CFunction lsb_get_output_function(lua_State *lua, int index)
{
  lua_CFunction fp = NULL;
  lua_getfenv(lua, index);
  lua_pushstring(lua, output_function);
  lua_rawget(lua, -2);
  fp = lua_tocfunction(lua, -1);
  lua_pop(lua, 2); // environment and field
  return fp;
}


void lsb_add_zero_copy_function(lua_State *lua, lua_CFunction fp)
{
  lua_pushstring(lua, zero_copy_function);
  lua_pushcfunction(lua, fp);
  lua_rawset(lua, -3);
}


lua_CFunction lsb_get_zero_copy_function(lua_State *lua, int index)
{
  lua_CFunction fp = NULL;
  lua_getfenv(lua, index);
  lua_pushstring(lua, zero_copy_function);
  lua_rawget(lua, -2);
  fp = lua_tocfunction(lua, -1);
  lua_pop(lua, 2); // environment and field
  return fp;
}


void lsb_output(lsb_lua_sandbox *lsb, int start, int end, int append)
{
  lsb_output_coroutine(lsb, lsb->lua, start, end, append);
}


void lsb_output_coroutine(lsb_lua_sandbox *lsb, lua_State *lua, int start,
                          int end, int append)
{
  if (!append) {
    lsb->output.pos = 0;
  }

  int result = 0;
  for (int i = start; result == 0 && i <= end; ++i) {
    switch (lua_type(lua, i)) {
    case LUA_TNUMBER:
      if (lsb_outputd(&lsb->output, lua_tonumber(lua, i))) {
        result = 1;
      }
      break;
    case LUA_TSTRING:
      {
        size_t len;
        const char *s = lua_tolstring(lua, i, &len);
        if (lsb_outputs(&lsb->output, s, len)) {
          result = 1;
        }
      }
      break;
    case LUA_TNIL:
      if (lsb_outputs(&lsb->output, "nil", 3)) {
        result = 1;
      }
      break;
    case LUA_TBOOLEAN:
      if (lsb_outputf(&lsb->output, "%s", lua_toboolean(lua, i)
                      ? "true" : "false")) {
        result = 1;
      }
      break;
    case LUA_TUSERDATA:
      {
        lua_CFunction fp = lsb_get_output_function(lua, i);
        if (!fp) {
          luaL_argerror(lua, i, "unknown userdata type");
          return; // never reaches here but the compiler doesn't know it
        }
        lua_pushvalue(lua, i);
        lua_pushlightuserdata(lua, &lsb->output);
        result = fp(lua);
        lua_pop(lua, 2); // remove the copy of the value and the output
      }
      break;
    default:
      luaL_argerror(lua, i, "unsupported type");
      break;
    }
  }
  lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = lsb->output.pos;
  if (lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM] =
        lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT];
  }

  if (result != 0) {
    if (lsb->error_message[0] == 0) {
      luaL_error(lua, "output_limit exceeded");
    }
    luaL_error(lua, lsb->error_message);
  }
}


const char* lsb_get_output(lsb_lua_sandbox *lsb, size_t *len)
{
  if (len) *len = lsb->output.pos;
  if (lsb->output.pos == 0) return "";
  lsb->output.pos = 0; // internally reset the buffer for the next write
  return lsb->output.buf;
}
