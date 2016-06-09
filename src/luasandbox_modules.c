/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Additional built in Lua sandbox modules @file */

#include <stdlib.h>

#include "lauxlib.h"
#include "lua.h"
#include "luasandbox/util/util.h"
#include "luasandbox_impl.h"

#ifdef HAVE_ZLIB
#include <zlib.h>


static int zlib_crc32(lua_State *lua)
{
  size_t len;
  const char *buf;

  if (lua_type(lua, 1) == LUA_TSTRING) {
    buf = lua_tolstring(lua, 1, &len);
  } else {
    return luaL_argerror(lua, 1, "must be a string");
  }
  lua_pushnumber(lua, (lua_Number)lsb_crc32(buf, len));
  return 1;
}


static int zlib_ungzip(lua_State *lua)
{
  size_t len;
  const char *buf;

  if (lua_type(lua, 1) == LUA_TSTRING) {
    buf = lua_tolstring(lua, 1, &len);
  } else {
    return luaL_argerror(lua, 1, "must be a string");
  }

  lua_Number mss = lua_tonumber(lua, 2); // unlimited if max is not provided
  int t = lua_type(lua, 2);
  if (mss < 0 || (t != LUA_TNUMBER && t != LUA_TNONE && t != LUA_TNIL)) {
    return luaL_argerror(lua, 2, "must be a positive number or nil");
  }

  char *gbuf = lsb_ungzip(buf, len, (size_t)mss, &len);
  if (gbuf) {
    lua_pushlstring(lua, gbuf, len);
    free(gbuf);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}
#endif


static const struct luaL_reg hashlib_f[] =
{
#ifdef HAVE_ZLIB
  { "crc32", zlib_crc32 },
#endif
  { NULL, NULL }
};


int luaopen_lsb_hash(lua_State *lua)
{
  luaL_register(lua, LSB_HASH_MODULE, hashlib_f);
  return 1;
}


static const struct luaL_reg compressionlib_f[] =
{
#ifdef HAVE_ZLIB
  { "ungzip", zlib_ungzip },
#endif
  { NULL, NULL }
};


int luaopen_lsb_compression(lua_State *lua)
{
  luaL_register(lua, LSB_COMPRESSION_MODULE, compressionlib_f);
  return 1;
}

