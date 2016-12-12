/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua Heka read message zero copy references @file */

#include <stdio.h>
#include <string.h>

#include "../luasandbox_defines.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "luasandbox/heka/sandbox.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox_output.h"
#include "luasandbox_serialize.h"

static const char *metatable_name = "lsb.read_message_zc";

typedef struct read_message_zc
{
  lsb_const_string name;
  int              fi;
  int              ai;
  char             field[];
} read_message_zc;


static const lsb_heka_message* get_heka_message(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);
  lsb_heka_sandbox *hsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!hsb) {
    luaL_error(lua, "invalid " LSB_HEKA_THIS_PTR);
  }
  return lsb_heka_get_message(hsb);
}


static lsb_const_string read_message(lua_State *lua, read_message_zc *zc)
{
  lsb_const_string ret = { NULL, 0 };
  const lsb_heka_message *m = get_heka_message(lua);
  if (!m || !m->raw.s) {
    return ret;
  }

  if (strcmp(zc->name.s, LSB_RAW) == 0) {
    if (m->raw.s) ret = m->raw;
  } else if (strcmp(zc->name.s, LSB_PAYLOAD) == 0) {
    if (m->payload.s) ret = m->payload;
  } else if (strcmp(zc->name.s, LSB_LOGGER) == 0) {
    if (m->logger.s) ret = m->logger;
  } else if (strcmp(zc->name.s, LSB_TYPE) == 0) {
    if (m->type.s) ret = m->type;
  } else if (strcmp(zc->name.s, LSB_ENV_VERSION) == 0) {
    if (m->env_version.s) ret = m->env_version;
  } else if (strcmp(zc->name.s, LSB_HOSTNAME) == 0) {
    if (m->hostname.s) ret = m->hostname;
  } else if (strcmp(zc->name.s, LSB_UUID) == 0) {
    if (m->uuid.s) ret = m->uuid;
  } else if (zc->name.len >= 8
             && memcmp(zc->name.s, LSB_FIELDS "[", 7) == 0
             && zc->name.s[zc->name.len - 1] == ']') {
    lsb_read_value v;
    lsb_const_string f = { zc->name.s + 7, zc->name.len - 8 };
    lsb_read_heka_field(m, &f, zc->fi, zc->ai, &v);
    if (v.type == LSB_READ_STRING) {
      ret = v.u.s;
    } else if (v.type != LSB_READ_NIL) {
      luaL_error(lua, "%s() zc->name.s: '%s' contains an unsupported type",
                 __func__, zc->name.s);
    }
  } else {
    luaL_error(lua, "%s() zc->name.s: '%s' not supported/recognized", __func__,
               zc->name.s);
  }
  return ret;
}


static int zc_output(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  if (!ob) {return 1;}
  read_message_zc *zc = luaL_checkudata(lua, -2, metatable_name);
  lsb_const_string f = read_message(lua, zc);
  if (!f.s) return 0;

  if (strcmp(zc->field, LSB_FRAMED) == 0) {
    char header[LSB_MIN_HDR_SIZE];
    size_t hlen = lsb_write_heka_header(header, f.len);
    if (lsb_outputs(ob, header, hlen)) return 1;
  }
  if (lsb_outputs(ob, f.s, f.len)) return 1;
  return 0;
}


static int zc_return(lua_State *lua)
{
  read_message_zc *zc = luaL_checkudata(lua, -1, metatable_name);
  lua_checkstack(lua, 3);
  int cnt = 2;
  lsb_const_string f = read_message(lua, zc);
  if (strcmp(zc->field, LSB_FRAMED) == 0) {
    char header[LSB_MIN_HDR_SIZE];
    size_t hlen = lsb_write_heka_header(header, f.len);
    lua_pushlstring(lua, header, hlen);
    ++cnt;
  }
  lua_pushlightuserdata(lua, (void *)f.s);
  lua_pushinteger(lua, (int)f.len);
  return cnt;
}


static int zc_tostring(lua_State *lua)
{
  read_message_zc *zc = luaL_checkudata(lua, -1, metatable_name);
  lsb_const_string f = read_message(lua, zc);
  if (f.s) {
    if (strcmp(zc->field, LSB_FRAMED) == 0) {
      char header[LSB_MIN_HDR_SIZE];
      size_t hlen = lsb_write_heka_header(header, f.len);
      lua_pushlstring(lua, header, hlen);
      lua_pushlstring(lua, f.s, f.len);
      lua_concat(lua, 2);
    } else {
      lua_pushlstring(lua, f.s, f.len);
    }
  } else {
    lua_pushnil(lua);
  }
  return 1;
}


static const struct luaL_reg zclib_m[] =
{
  { "__tostring", zc_tostring },
  { NULL, NULL }
};


int heka_create_read_message_zc(lua_State *lua)
{
  size_t len;
  const char *field = luaL_checklstring(lua, 1, &len);
  int fi = luaL_optint(lua, 2, 0);
  luaL_argcheck(lua, fi >= 0, 2, "field index must be >= 0");
  int ai = luaL_optint(lua, 3, 0);
  luaL_argcheck(lua, ai >= 0, 3, "array index must be >= 0");

  if (!(strcmp(field, LSB_UUID) == 0
        || strcmp(field, LSB_TYPE) == 0
        || strcmp(field, LSB_LOGGER) == 0
        || strcmp(field, LSB_PAYLOAD) == 0
        || strcmp(field, LSB_ENV_VERSION) == 0
        || strcmp(field, LSB_HOSTNAME) == 0
        || strcmp(field, LSB_RAW) == 0
        || strcmp(field, LSB_FRAMED) == 0
        || (len >= 8
            && memcmp(field, LSB_FIELDS "[", 7) == 0
            && field[len - 1] == ']'))) {
    luaL_error(lua, "%s() field: '%s' not supported/recognized", __func__,
               field);
  }

  if (luaL_newmetatable(lua, metatable_name) == 1) {
    lua_newtable(lua);
    lsb_add_output_function(lua, zc_output);
    lsb_add_zero_copy_function(lua, zc_return);
    lua_replace(lua, LUA_ENVIRONINDEX);

    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");
    luaL_register(lua, NULL, zclib_m);
  }

  read_message_zc *zc = lua_newuserdata(lua, sizeof(read_message_zc) + len + 1);
  zc->fi = fi;
  zc->ai = ai;
  memcpy(zc->field, field, len + 1);
  if (strcmp(field, LSB_FRAMED) == 0) {
    zc->name.s = LSB_RAW;
    zc->name.len = sizeof(LSB_RAW) - 1;
  } else {
    zc->name.s = zc->field;
    zc->name.len = len;
  }

  lua_pushvalue(lua, -2);
  lua_setmetatable(lua, -2);
  return 1;
}
