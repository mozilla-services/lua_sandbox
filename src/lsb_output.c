/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox output buffer implementation @file */

#include "luasandbox_output.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/lauxlib.h"
#include "lsb_private.h"
#include "luasandbox_serialize.h"
#include "lsb_serialize_protobuf.h"

static const char* output_function = "lsb_output";


static void update_output_stats(lua_sandbox* lsb)
{
  lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = lsb->output.pos;
  if (lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM] =
      lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT];
  }
}


void lsb_add_output_function(lua_State* lua, lua_CFunction fp)
{
  lua_pushstring(lua, output_function);
  lua_pushcfunction(lua, fp);
  lua_rawset(lua, -3);
}


lua_CFunction lsb_get_output_function(lua_State* lua, int index)
{
  lua_CFunction fp = NULL;
  lua_getfenv(lua, index);
  lua_pushstring(lua, output_function);
  lua_rawget(lua, -2);
  fp = lua_tocfunction(lua, -1);
  lua_pop(lua, 2); // environment and field
  return fp;
}


int lsb_appendc(lsb_output_data* output, char ch)
{
  size_t needed = 2;
  if (output->size - output->pos < needed) {
    if (lsb_realloc_output(output, needed)) return 1;
  }
  output->data[output->pos++] = ch;
  output->data[output->pos] = 0;
  return 0;
}


int lsb_appendf(lsb_output_data* output, const char* fmt, ...)
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


int lsb_appends(lsb_output_data* output, const char* str, size_t len)
{
  size_t needed = len + 1;
  if (output->size - output->pos < needed) {
    if (lsb_realloc_output(output, needed)) return 1;
  }
  memcpy(output->data + output->pos, str, len);
  output->pos += len;
  output->data[output->pos] = 0;
  return 0;
}


int lsb_realloc_output(lsb_output_data* output, size_t needed)
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


void lsb_output(lua_sandbox* lsb, int start, int end, int append)
{
  if (!append) {
    lsb->output.pos = 0;
  }

  int result = 0;
  for (int i = start; result == 0 && i <= end; ++i) {
    switch (lua_type(lsb->lua, i)) {
    case LUA_TNUMBER:
      if (lsb_output_double(&lsb->output, lua_tonumber(lsb->lua, i))) {
        result = 1;
      }
      break;
    case LUA_TSTRING:
      {
        size_t len;
        const char* s = lua_tolstring(lsb->lua, i, &len);
        if (lsb_appends(&lsb->output, s, len)) {
          result = 1;
        }
      }
      break;
    case LUA_TNIL:
      if (lsb_appends(&lsb->output, "nil", 3)) {
        result = 1;
      }
      break;
    case LUA_TBOOLEAN:
      if (lsb_appendf(&lsb->output, "%s",
                      lua_toboolean(lsb->lua, i)
                      ? "true" : "false")) {
        result = 1;
      }
      break;
    case LUA_TUSERDATA:
      {
        lua_CFunction fp = lsb_get_output_function(lsb->lua, i);
        if (!fp) {
          luaL_argerror(lsb->lua, i, "unknown userdata type");
          return; // never reaches here but the compiler doesn't know it
        }
        lua_pushvalue(lsb->lua, i);
        lua_pushlightuserdata(lsb->lua, &lsb->output);
        result = fp(lsb->lua);
        lua_pop(lsb->lua, 2); // remove the copy of the value and the output
      }
      break;
    default:
      luaL_argerror(lsb->lua, i, "unsupported type");
      break;
    }
  }
  update_output_stats(lsb);
  if (result != 0) {
    if (lsb->error_message[0] == 0) {
      luaL_error(lsb->lua, "output_limit exceeded");
    }
    luaL_error(lsb->lua, lsb->error_message);
  }
}


int lsb_output_protobuf(lua_sandbox* lsb, int index, int append)
{
  if (lua_type(lsb->lua, index) != LUA_TTABLE) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE, "takes a table argument");
    return 1;
  }

  if (!append) {
    lsb->output.pos = 0;
  }

  size_t last_pos = lsb->output.pos;
  if (lsb_serialize_table_as_pb(lsb, index) != 0) {
    lsb->output.pos = last_pos;
    return 1;
  }

  update_output_stats(lsb);
  return 0;
}
