/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight Heka stream reader implementation @file */

#include "stream_reader_impl.h"

#include <stdlib.h>
#include <string.h>

#include "luasandbox/heka/sandbox.h"
#include "message_impl.h"
#include "luasandbox/lauxlib.h"

const char* mozsvc_heka_stream_reader = "mozsvc.heka_stream_reader";
const char *mozsvc_heka_stream_reader_table = "heka_stream_reader";

static int hsr_new(lua_State* lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 1, 0, "incorrect number of arguments");
  size_t len;
  const char* name = luaL_checklstring(lua, 1, &len);
  luaL_argcheck(lua, len < 255, 1, "name is too long");

  size_t nbytes = sizeof(heka_stream_reader);
  heka_stream_reader* hsr = lua_newuserdata(lua, nbytes);

  size_t mms = 0;
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  if (lua_type(lua, -1) == LUA_TTABLE) {
    lua_getfield(lua, -1, LSB_HEKA_MAX_MESSAGE_SIZE);
    mms = (size_t)lua_tointeger(lua, -1);
    lua_pop(lua, 1); // remove limit
  } else {
    free(hsr);
    return luaL_error(lua, LSB_CONFIG_TABLE " is missing");
  }
  lua_pop(lua, 1); // remove config

  if (lsb_init_heka_message(&hsr->msg, 8)) {
    free(hsr);
    return luaL_error(lua, "failed to init the message struct");
  }
  if (lsb_init_input_buffer(&hsr->buf, mms)) {
    lsb_free_heka_message(&hsr->msg);
    free(hsr);
    return luaL_error(lua, "failed to init the input buffer");
  }
  hsr->name = malloc(len + 1);
  if (!hsr->name) {
    lsb_free_input_buffer(&hsr->buf);
    lsb_free_heka_message(&hsr->msg);
    free(hsr);
    return luaL_error(lua, "memory allocation failed");
  }
  strcpy(hsr->name, name);

  luaL_getmetatable(lua, mozsvc_heka_stream_reader);
  lua_setmetatable(lua, -2);
  return 1;
}


static heka_stream_reader* check_hsr(lua_State* lua, int args)
{
  heka_stream_reader* hsr = luaL_checkudata(lua, 1, mozsvc_heka_stream_reader);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return hsr;
}


static int hsr_decode_message(lua_State* lua)
{
  heka_stream_reader* hsr = check_hsr(lua, 2);
  lsb_input_buffer* b = &hsr->buf;
  b->readpos = b->scanpos = b->msglen = 0;

  size_t len;
  if (lua_type(lua, 2) == LUA_TSTRING) {
      const char* s = lua_tolstring(lua, 2, &len);
      if (len > 0) {
        if (lsb_expand_input_buffer(b, len)) {
          return luaL_error(lua, "buffer reallocation failed\tname:%s",
                            hsr->name);
        }
        memcpy(b->buf, s, len);
      } else {
        return luaL_error(lua, "empty protobuf string");
      }
  } else {
    return luaL_error(lua, "buffer must be string");
  }

  if (!lsb_decode_heka_message(&hsr->msg, b->buf, len, NULL)) {
    return luaL_error(lua, "invalid protobuf string");
  }
  return 0;
}


static int hsr_find_message(lua_State* lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n > 1 && n < 4, 0,"incorrect number of arguments");

  heka_stream_reader* hsr = luaL_checkudata(lua, 1, mozsvc_heka_stream_reader);
  lsb_input_buffer* b = &hsr->buf;
  FILE* fh = NULL;

  switch (lua_type(lua, 2)) {
  case LUA_TNIL: // scan the existing buffer
    break;
  case LUA_TSTRING: // add data to the buffer
    {
      size_t len;
      const char* s = lua_tolstring(lua, 2, &len);
      if (len > 0) {
        if (lsb_expand_input_buffer(b, len)) {
          return luaL_error(lua, "buffer reallocation failed\tname:%s",
                            hsr->name);
        }
        memcpy(b->buf + b->readpos, s, len);
        b->readpos += len;
      }
    }
    break;
  case LUA_TUSERDATA: // add data from the provided file handle to the buffer
    fh = *(FILE**)luaL_checkudata(lua, 2, "FILE*");
    if (!fh) luaL_error(lua, "attempt to use a closed file");
    break;
  default:
    return luaL_error(lua, "buffer must be a nil, string, userdata (FILE*)");
  }

  bool decode = true;
  if (n == 3 && lua_type(lua, 3) == LUA_TBOOLEAN) {
    decode = (bool)lua_toboolean(lua, 3);
  }

  size_t pos_r = b->readpos;
  size_t pos_s = b->scanpos;
  size_t discarded = 0;
  bool found = lsb_find_heka_message(&hsr->msg, b, decode, &discarded, NULL);

  if (found) {
    lua_pushboolean(lua, 1); // found
    lua_pushinteger(lua, b->scanpos - pos_s); // consumed
  } else {
    lua_pushboolean(lua, 0); // not found
    if (b->readpos == 0) {
      lua_pushinteger(lua, pos_r - pos_s); // consumed everything in the buf
    } else {
      lua_pushinteger(lua, b->scanpos - pos_s);
    }
  }

  if (fh) { // update bytes read
    if (found) {
      lua_pushinteger(lua, 0);
    } else {
      if (lsb_expand_input_buffer(&hsr->buf, 0)) {
        luaL_error(lua, "%s buffer reallocation failed", hsr->name);
      }
      size_t nread = fread(hsr->buf.buf + hsr->buf.readpos,
                           1,
                           hsr->buf.size - hsr->buf.readpos,
                           fh);
      hsr->buf.readpos += nread;
      lua_pushnumber(lua, nread);
    }
  } else { // update bytes needed
    if (found) {
      if (b->scanpos != b->readpos) {
        lua_pushinteger(lua, 0);
      } else {
        lua_pushinteger(lua, b->size);
      }
    } else {
      if (b->msglen + LSB_MAX_HDR_SIZE > b->size) {
        lua_pushinteger(lua, b->msglen);
      } else {
        lua_pushinteger(lua, b->scanpos + b->size - b->readpos);
      }
    }
  }
  return 3;
}


static int hsr_read_message(lua_State* lua)
{
  int n = lua_gettop(lua);
  if (n < 1 || n > 4) {
    return luaL_error(lua, "read_message() incorrect number of arguments");
  }
  heka_stream_reader* hsr = check_hsr(lua, n);
  lua_remove(lua, 1); // remove the hsr user data
  return heka_read_message(lua, &hsr->msg);
}


static int hsr_gc(lua_State* lua)
{
  heka_stream_reader* hsr = check_hsr(lua, 1);
  free(hsr->name);
  lsb_free_heka_message(&hsr->msg);
  lsb_free_input_buffer(&hsr->buf);
  return 1;
}


static const struct luaL_reg heka_stream_readerlib_f[] =
{
  { "new", hsr_new }
  , { NULL, NULL }
};


static const struct luaL_reg heka_stream_readerlib_m[] =
{
  { "find_message", hsr_find_message }
  , { "decode_message", hsr_decode_message }
  , { "read_message", hsr_read_message }
  , { "__gc", hsr_gc }
  , { NULL, NULL }
};


int luaopen_heka_stream_reader(lua_State* lua)
{
  luaL_newmetatable(lua, mozsvc_heka_stream_reader);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, heka_stream_readerlib_m);
  luaL_register(lua, mozsvc_heka_stream_reader_table, heka_stream_readerlib_f);
  return 1;
}
