/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox Heka protobuf deserialization @file */

#include <string.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox.h"

#define UUID_SIZE 16
#define MAX_VARINT_BYTES 10

static unsigned const char*
read_key(unsigned const char* p, int* tag, int* wiretype)
{
  *wiretype = 7 & *p;
  *tag = *p >> 3;
  return ++p;
}


static unsigned const char*
read_length(unsigned const char* p, unsigned const char* e, size_t* vi)
{
  *vi = 0;
  unsigned i, shift = 0;
  for (i = 0; p != e && i < MAX_VARINT_BYTES; i++) {
    *vi |= ((unsigned long long)p[i] & 0x7f) << shift;
    shift += 7;
    if ((p[i] & 0x80) == 0) break;
  }
  if (i == MAX_VARINT_BYTES) {
    return NULL;
  }
  return p + i + 1;
}


static unsigned const char*
read_varint(unsigned const char* p, unsigned const char* e, long long* vi)
{
  *vi = 0;
  unsigned i, shift = 0;
  for (i = 0; p != e && i < MAX_VARINT_BYTES; i++) {
    *vi |= ((unsigned long long)p[i] & 0x7f) << shift;
    shift += 7;
    if ((p[i] & 0x80) == 0) break;
  }
  if (i == MAX_VARINT_BYTES) {
    return NULL;
  }
  return p + i + 1;
}


static unsigned const char*
read_string(lua_State* lua,
            int wiretype,
            unsigned const char* p,
            unsigned const char* e)
{
  if (wiretype != 2) {
    return NULL;
  }

  size_t len = 0;
  p = read_length(p, e, &len);
  if (!p || p + len > e) {
    return NULL;
  }
  lua_pushlstring(lua, (const char*)p, len);
  p += len;
  return p;
}


static unsigned const char*
process_varint(lua_State* lua,
               const char* name,
               int wiretype,
               int stack_index,
               unsigned const char* p,
               unsigned const char* e)
{
  if (wiretype != 0) {
    return NULL;
  }
  long long val = 0;
  p = read_varint(p, e, &val);
  if (!p) {
    return NULL;
  }
  lua_pushnumber(lua, (lua_Number)val);
  lua_setfield(lua, stack_index, name);
  return p;
}


static unsigned const char*
process_fields(lua_State* lua,
               unsigned const char* p,
               unsigned const char* e)
{
  int tag = 0;
  int wiretype = 0;
  int has_name = 0;
  int value_count = 0;
  size_t len = 0;

  p = read_length(p, e, &len);
  if (!p || p + len > e) {
    return NULL;
  }
  e = p + len; // only process to the end of the current field record

  lua_newtable(lua); // Table to be added to the Fields array index 4
  lua_newtable(lua); // Table to hold the value(s) index 5
  do {
    p = read_key(p, &tag, &wiretype);

    switch (tag) {
    case 1:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 4, "name");
        has_name = 1;
      }
      break;

    case 2:
      p = process_varint(lua, "value_type", wiretype, 4, p, e);
      break;

    case 3:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 4, "representation");
      }
      break;

    case 4: // value_string
    case 5: // value_bytes
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_rawseti(lua, 5, ++value_count);
      }
      break;

    case 6: // value_integer
      {
        long long val = 0;
        switch (wiretype) {
        case 0:
          p = read_varint(p, p + len, &val);
          if (!p) break;
          lua_pushnumber(lua, (lua_Number)val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = read_length(p, e, &len);
          if (!p || p + len > e) {
            p = NULL;
            break;
          }
          do {
            p = read_varint(p, p + len, &val);
            if (!p) break;
            lua_pushnumber(lua, (lua_Number)val);
            lua_rawseti(lua, 5, ++value_count);
          }
          while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;

    case 7: // value_double
      {
        double val = 0;
        switch (wiretype) {
        case 1:
          if (p + sizeof(double) > e) {
            p = NULL;
            break;
          }
          memcpy(&val, p, sizeof(double));
          p += sizeof(double);
          lua_pushnumber(lua, val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = read_length(p, e, &len);
          if (!p || p + len > e || len % sizeof(double) != 0) {
            p = NULL;
            break;
          }
          do {
            memcpy(&val, p, sizeof(double));
            p += sizeof(double);
            lua_pushnumber(lua, val);
            lua_rawseti(lua, 5, ++value_count);
          }
          while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;

    case 8: // value_bool
      {
        long long val = 0;
        switch (wiretype) {
        case 0:
          p = read_varint(p, p + len, &val);
          if (!p) break;
          lua_pushboolean(lua, (int)val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = read_length(p, e, &len);
          if (!p || p + len > e) {
            p = NULL;
            break;
          }
          do {
            p = read_varint(p, p + len, &val);
            if (!p) break;
            lua_pushboolean(lua, (int)val);
            lua_rawseti(lua, 5, ++value_count);
          }
          while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;
    default:
      p = NULL; // don't allow unknown tags
      break;
    }
  }
  while (p && p < e);

  lua_setfield(lua, 4, "value");

  return has_name ? p : NULL;
}


int lsb_decode_protobuf(lua_State* lua)
{
  int n = lua_gettop(lua);
  if (n != 1 || lua_type(lua, 1) != LUA_TSTRING) {
    return luaL_argerror(lua, 0, "must have one string argument");
  }

  size_t len;
  const char* pbstr = lua_tolstring(lua, 1, &len);
  if (len < 20) {
    return luaL_error(lua, "invalid message, too short");
  }

  unsigned const char* p = (unsigned const char*)pbstr;
  unsigned const char* lp = p;
  unsigned const char* e = (unsigned const char*)pbstr + len;
  int wiretype = 0;
  int tag = 0;
  int has_uuid = 0;
  int has_timestamp = 0;
  int field_count = 0;

  lua_newtable(lua); // message table index 2
  do {
    p = read_key(p, &tag, &wiretype);

    switch (tag) {
    case 1:
      p = read_string(lua, wiretype, p, e);
      if (p && p - lp == 18) {
        lua_setfield(lua, 2, "Uuid");
        has_uuid = 1;
      } else {
        p = NULL;
      }
      break;

    case 2:
      p = process_varint(lua, "Timestamp", wiretype, 2, p, e);
      if (p) {
        has_timestamp = 1;
      }
      break;

    case 3:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Type");
      }
      break;

    case 4:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Logger");
      }
      break;

    case 5:
      p = process_varint(lua, "Severity", wiretype, 2, p, e);
      break;

    case 6:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Payload");
      }
      break;

    case 7:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "EnvVersion");
      }
      break;

    case 8:
      p = process_varint(lua, "Pid", wiretype, 2, p, e);
      break;

    case 9:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Hostname");
      }
      break;

    case 10:
      if (wiretype != 2) {
        p = NULL;
        break;
      }
      if (field_count == 0) {
        lua_newtable(lua); // Fields table index 3
      }
      p = process_fields(lua, p, e);
      if (p) {
        lua_rawseti(lua, 3, ++field_count);
      }
      break;

    default:
      p = NULL; // don't allow unknown tags
      break;
    }
    if (p) lp = p;
  }
  while (p && p < e);

  if (!p) {
    return luaL_error(lua, "error in tag: %d wiretype: %d offset: %d", tag,
                      wiretype, (const char*)lp - pbstr);
  }

  if (!(has_uuid && has_timestamp)) {
    return luaL_error(lua, "missing required field uuid: %s timestamp: %s",
                      has_uuid ? "found" : "not found",
                      has_timestamp ? "found" : "not found");
  }

  if (field_count) {
    lua_setfield(lua, 2, "Fields");
  }

  return 1;
}
