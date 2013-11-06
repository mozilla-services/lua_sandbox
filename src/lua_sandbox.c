/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Sandboxed Lua execution @file
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lua_sandbox_private.h"
#include "lua_serialize.h"
#include "lua_serialize_protobuf.h"
#include "lua_circular_buffer.h"

static const char* disable_base_functions[] = { "collectgarbage", "coroutine",
  "dofile", "getfenv", "getmetatable", "load", "loadfile", "loadstring",
  "module", "print", "rawequal", "require", "setfenv", NULL };

static jmp_buf g_jbuf;

////////////////////////////////////////////////////////////////////////////////
lua_sandbox* lsb_create(void* parent,
                        const char* lua_file,
                        unsigned memory_limit,
                        unsigned instruction_limit,
                        unsigned output_limit)
{
  if (!lua_file) return NULL;

  if (memory_limit > LSB_MEMORY || instruction_limit > LSB_INSTRUCTION
      || output_limit > LSB_OUTPUT) {
    return NULL;
  }

  if (output_limit < OUTPUT_SIZE) {
    output_limit = OUTPUT_SIZE;
  }

  lua_sandbox* lsb = malloc(sizeof(lua_sandbox));
  if (!lsb) return NULL;

  lsb->m_lua = luaL_newstate();
  if (!lsb->m_lua) {
    free(lsb);
    return NULL;
  }

  lsb->m_parent = parent;
  memset(lsb->m_usage, 0, sizeof(lsb->m_usage));
  lsb->m_usage[LSB_UT_MEMORY][LSB_US_LIMIT] = memory_limit;
  lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] = instruction_limit;
  lsb->m_usage[LSB_UT_OUTPUT][LSB_US_LIMIT] = output_limit;
  lsb->m_state = LSB_UNKNOWN;
  lsb->m_error_message[0] = 0;
  lsb->m_output.m_pos = 0;
  lsb->m_output.m_maxsize = output_limit;
  lsb->m_output.m_size = OUTPUT_SIZE;
  lsb->m_output.m_data = malloc(lsb->m_output.m_size);
  size_t len = strlen(lua_file);
  lsb->m_lua_file = malloc(len + 1);
  if (!lsb->m_output.m_data || !lsb->m_lua_file) {
    free(lsb);
    lua_close(lsb->m_lua);
    lsb->m_lua = NULL;
    return NULL;
  }
  strcpy(lsb->m_lua_file, lua_file);
  srand(time(NULL));
  return lsb;
}

////////////////////////////////////////////////////////////////////////////////
int unprotected_panic(lua_State* lua)
{
  (void)lua;
  longjmp(g_jbuf, 1);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int lsb_init(lua_sandbox* lsb, const char* data_file)
{
  if (!lsb) return 0;

  load_library(lsb->m_lua, "", luaopen_base, disable_base_functions);
  lua_pop(lsb->m_lua, 1);

  lua_pushcfunction(lsb->m_lua, &require_library);
  lua_setglobal(lsb->m_lua, "require");

  lua_pushlightuserdata(lsb->m_lua, (void*)lsb);
  lua_pushcclosure(lsb->m_lua, &output, 1);
  lua_setglobal(lsb->m_lua, "output");

  lua_sethook(lsb->m_lua, instruction_manager, LUA_MASKCOUNT,
              lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);

  lua_gc(lsb->m_lua, LUA_GCSETMEMLIMIT,
         (int)lsb->m_usage[LSB_UT_MEMORY][LSB_US_LIMIT]);
  int jump = setjmp(g_jbuf);
  lua_CFunction pf = lua_atpanic(lsb->m_lua, unprotected_panic);
  if (jump || luaL_dofile(lsb->m_lua, lsb->m_lua_file) != 0) {
    int len = snprintf(lsb->m_error_message, LSB_ERROR_SIZE, "%s",
                       lua_tostring(lsb->m_lua, -1));
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    sandbox_terminate(lsb);
    return 2;
  } else {
    lua_gc(lsb->m_lua, LUA_GCCOLLECT, 0);
    lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] =
      instruction_usage(lsb);
    if (lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
        > lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
      lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
        lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
    }
    lsb->m_state = LSB_RUNNING;
    if (data_file != NULL && strlen(data_file) > 0) {
      if (restore_global_data(lsb, data_file)) return 3;
    }
  }
  lua_atpanic(lsb->m_lua, pf);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
char* lsb_destroy(lua_sandbox* lsb, const char* data_file)
{
  char* err = NULL;
  if (!lsb) return err;

  if (lsb->m_lua && data_file && strlen(data_file) > 0) {
    if (preserve_global_data(lsb, data_file) != 0) {
      size_t len = strlen(lsb->m_error_message);
      err = malloc(len + 1);
      if (err != NULL) {
        strcpy(err, lsb->m_error_message);
      }
    }
  }
  sandbox_terminate(lsb);
  free(lsb->m_output.m_data);
  free(lsb->m_lua_file);
  free(lsb);
  return err;
}

////////////////////////////////////////////////////////////////////////////////
unsigned lsb_usage(lua_sandbox* lsb, lsb_usage_type utype,
                   lsb_usage_stat ustat)
{
  if (!lsb || utype >= LSB_UT_MAX || ustat >= LSB_US_MAX) {
    return 0;
  }
  if (utype == LSB_UT_MEMORY) {
    switch (ustat) {
    case LSB_US_CURRENT:
      return lua_gc(lsb->m_lua, LUA_GCCOUNT, 0) * 1024
             + lua_gc(lsb->m_lua, LUA_GCCOUNTB, 0);
    case LSB_US_MAXIMUM:
      return lua_gc(lsb->m_lua, LUA_GCMAXCOUNT, 0) * 1024
             + lua_gc(lsb->m_lua, LUA_GCMAXCOUNTB, 0);
    default:
      break;
    }
  }
  return lsb->m_usage[utype][ustat];
}

////////////////////////////////////////////////////////////////////////////////
const char* lsb_get_error(lua_sandbox* lsb)
{
  if (lsb) {
    return lsb->m_error_message;
  }
  return "";
}

////////////////////////////////////////////////////////////////////////////////
lua_State* lsb_get_lua(lua_sandbox* lsb)
{
  return lsb->m_lua;
}

////////////////////////////////////////////////////////////////////////////////
void* lsb_get_parent(lua_sandbox* lsb)
{
  return lsb->m_parent;
}

////////////////////////////////////////////////////////////////////////////////
lsb_state lsb_get_state(lua_sandbox* lsb)
{
  if (lsb) {
    return lsb->m_state;
  }
  return LSB_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////
void lsb_add_function(lua_sandbox* lsb, lua_CFunction func,
                      const char* func_name)
{
  lua_pushlightuserdata(lsb->m_lua, (void*)lsb);
  lua_pushcclosure(lsb->m_lua, func, 1);
  lua_setglobal(lsb->m_lua, func_name);
}

////////////////////////////////////////////////////////////////////////////////
const char* lsb_get_output(lua_sandbox* lsb, size_t* len)
{
  if (len) {
    *len = lsb->m_output.m_pos;
  }
  if (lsb->m_output.m_pos == 0) return "";

  lsb->m_output.m_pos = 0;
  return lsb->m_output.m_data;
}

////////////////////////////////////////////////////////////////////////////////
const char* lsb_output_userdata(lua_sandbox* lsb, int index, int append)
{
  if (!append) lsb->m_output.m_pos = 0;

  void* ud = lua_touserdata(lsb->m_lua, index);
  if (heka_circular_buffer == userdata_type(lsb->m_lua, ud, index)) {
    circular_buffer* cb = (circular_buffer*)ud;
    size_t last_pos = lsb->m_output.m_pos;
    if (output_circular_buffer(lsb->m_lua, cb, &lsb->m_output)) {
      lsb->m_output.m_pos = last_pos;
      return NULL;
    }
    return get_output_format(cb);
  } else {
    return NULL;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
int lsb_output_protobuf(lua_sandbox* lsb, int index, int append)
{
  if (!append) lsb->m_output.m_pos = 0;

  size_t last_pos = lsb->m_output.m_pos;
  if (serialize_table_as_pb(lsb, index) != 0) {
    lsb->m_output.m_pos = last_pos;
    return 1;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int lsb_pcall_setup(lua_sandbox* lsb, const char* func_name)
{

  lua_sethook(lsb->m_lua, instruction_manager, LUA_MASKCOUNT,
              lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  lua_getglobal(lsb->m_lua, func_name);
  if (!lua_isfunction(lsb->m_lua, -1)) {
    int len = snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                       "%s() function was not found",
                       func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return 1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
void lsb_pcall_teardown(lua_sandbox* lsb)
{
  lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] = instruction_usage(lsb);
  if (lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
      > lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
    lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
      lsb->m_usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
  }
}

////////////////////////////////////////////////////////////////////////////////
void lsb_terminate(lua_sandbox* lsb, const char* err)
{
  strncpy(lsb->m_error_message, err, LSB_ERROR_SIZE);
  lsb->m_error_message[LSB_ERROR_SIZE - 1] = 0;
  sandbox_terminate(lsb);
}
