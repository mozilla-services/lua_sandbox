/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Implementation of the test interface for the generic lua sandbox @file */

#include "luasandbox/test/sandbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "luasandbox.h"
#include "luasandbox/lauxlib.h"
#include "luasandbox/lua.h"
#include "luasandbox_output.h"
#include "../luasandbox_defines.h"

const char *lsb_test_output = NULL;
size_t lsb_test_output_len = 0;

static void logger(void *context,
                   const char *component,
                   int level,
                   const char *fmt,
                   ...)
{
  (void)context;
  va_list args;
  fprintf(stderr, "%lld [%d] %s ", (long long)time(NULL), level,
          component ? component : "unnamed");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fwrite("\n", 1, 1, stderr);
}
lsb_logger lsb_test_logger = { .context = NULL, .cb = logger };


int lsb_test_write_output(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) return luaL_error(lua, "%s() invalid " LSB_THIS_PTR, __func__);

  int n = lua_gettop(lua);
  lsb_output(lsb, 1, n, 1);
  lsb_test_output = lsb_get_output(lsb, &lsb_test_output_len);
  return 0;
}


int lsb_test_process(lsb_lua_sandbox *lsb, double tc)
{
  static const char *func_name = "process";
  lua_State *lua = lsb_get_lua(lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(lsb, func_name)) return 1;

  lua_pushnumber(lua, tc);
  if (lua_pcall(lua, 1, 2, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    const char *em = lua_tostring(lua, -1);
    int len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                       em ? em : LSB_NIL_ERROR);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  if (!lua_isnumber(lua, 1)) {
    char err[LSB_ERROR_SIZE];
    int len = snprintf(err, LSB_ERROR_SIZE,
                       "%s() must return a numeric error code", func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  int status = (int)lua_tointeger(lua, 1);
  switch (lua_type(lua, 2)) {
  case LUA_TNIL:
    lsb_set_error(lsb, NULL);
    break;
  case LUA_TSTRING:
    lsb_set_error(lsb, lua_tostring(lua, 2));
    break;
  default:
    {
      char err[LSB_ERROR_SIZE];
      int len = snprintf(err, LSB_ERROR_SIZE,
                         "%s() must return a nil or string error message",
                         func_name);
      if (len >= LSB_ERROR_SIZE || len < 0) {
        err[LSB_ERROR_SIZE - 1] = 0;
      }
      lsb_terminate(lsb, err);
      return 1;
    }
    break;
  }
  lua_pop(lua, 2);

  lsb_pcall_teardown(lsb);

  return status;
}


int lsb_test_report(lsb_lua_sandbox *lsb, double tc)
{
  static const char *func_name = "report";
  lua_State *lua = lsb_get_lua(lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(lsb, func_name)) return 1;

  lua_pushnumber(lua, tc);
  if (lua_pcall(lua, 1, 0, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    const char *em = lua_tostring(lua, -1);
    int len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                       em ? em : LSB_NIL_ERROR);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  lsb_pcall_teardown(lsb);
  lua_gc(lua, LUA_GCCOLLECT, 0);

  return 0;
}
