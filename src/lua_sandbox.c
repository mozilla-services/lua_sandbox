/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandboxed implementation @file */

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
  "dofile", "load", "loadfile", "loadstring", "module", "print", "require", NULL };

static jmp_buf g_jbuf;


lua_sandbox* lsb_create(void* parent,
                        const char* lua_file,
                        const char* require_path,
                        unsigned memory_limit,
                        unsigned instruction_limit,
                        unsigned output_limit)
{
  if (!lua_file) {
    return NULL;
  }

  if (memory_limit > LSB_MEMORY || instruction_limit > LSB_INSTRUCTION
      || output_limit > LSB_OUTPUT) {
    return NULL;
  }

  if (output_limit < OUTPUT_SIZE) {
    output_limit = OUTPUT_SIZE;
  }

#if _WIN32
  if (_putenv("TZ=UTC") != 0) {
    return NULL;
  }
#else
  if (setenv("TZ", "UTC", 1) != 0) {
    return NULL;
  }
#endif

  lua_sandbox* lsb = malloc(sizeof(lua_sandbox));
  memset(lsb->usage, 0, sizeof(lsb->usage));
  if (!lsb) {
    return NULL;
  }
#ifdef LUA_JIT
  lsb->lua = luaL_newstate();
#else
  lsb->lua = lua_newstate(memory_manager, lsb);
#endif

  if (!lsb->lua) {
    free(lsb);
    return NULL;
  }

  lsb->parent = parent;
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = memory_limit;
  lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] = instruction_limit;
  lsb->usage[LSB_UT_OUTPUT][LSB_US_LIMIT] = output_limit;
  lsb->state = LSB_UNKNOWN;
  lsb->error_message[0] = 0;
  lsb->output.pos = 0;
  lsb->output.maxsize = output_limit;
  lsb->output.size = OUTPUT_SIZE;
  lsb->output.data = malloc(lsb->output.size);
  size_t len = strlen(lua_file);
  lsb->lua_file = malloc(len + 1);
  lsb->require_path = NULL;
  if (require_path) {
    len = strlen(require_path);
    lsb->require_path = malloc(len + 1);
  }
  if (!lsb->output.data || !lsb->lua_file
      || (require_path && !lsb->require_path)) {
    free(lsb);
    free(lsb->lua_file);
    free(lsb->require_path);
    lua_close(lsb->lua);
    lsb->lua = NULL;
    return NULL;
  }
  strcpy(lsb->lua_file, lua_file);
  if (lsb->require_path) {
    strcpy(lsb->require_path, require_path);
  }
  srand((unsigned int)time(NULL));
  return lsb;
}


int unprotected_panic(lua_State* lua)
{
  (void)lua;
  longjmp(g_jbuf, 1);
  return 0;
}


int lsb_init(lua_sandbox* lsb, const char* data_file)
{
  if (!lsb) {
    return 0;
  }
#ifndef LUA_JIT
  unsigned mem_limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif

  load_library(lsb->lua, "", luaopen_base, disable_base_functions);
  lua_pop(lsb->lua, 1);

  // Create a simple package cache
  lua_createtable(lsb->lua, 0, 1);
  lua_pushvalue(lsb->lua, -1);
  lua_setglobal(lsb->lua, package_table);
  // Add empty metatable to prevent serialization
  lua_newtable(lsb->lua);
  lua_setmetatable(lsb->lua, -2);
  // add the loaded table
  lua_newtable(lsb->lua);
  lua_setfield(lsb->lua, -2, loaded_table);
  lua_pop(lsb->lua, 1); // remove the package table

  lua_pushlightuserdata(lsb->lua, (void*)lsb);
  lua_pushcclosure(lsb->lua, &require_library, 1);
  lua_setglobal(lsb->lua, "require");

  lua_pushlightuserdata(lsb->lua, (void*)lsb);
  lua_pushcclosure(lsb->lua, &output, 1);
  lua_setglobal(lsb->lua, "output");

  lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
              lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
#ifdef LUA_JIT
  lua_gc(lsb->lua, LUA_GCSETMEMLIMIT,
         (int)lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT]);
#else
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = mem_limit;
#endif
  lua_CFunction pf = lua_atpanic(lsb->lua, unprotected_panic);
  int jump = setjmp(g_jbuf);
  if (jump || luaL_dofile(lsb->lua, lsb->lua_file) != 0) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE, "%s",
                       lua_tostring(lsb->lua, -1));
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    sandbox_terminate(lsb);
    return 2;
  } else {
    lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
    lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] =
      (unsigned)instruction_usage(lsb);
    if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
        > lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
      lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
        lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
    }
    lsb->state = LSB_RUNNING;
    if (data_file != NULL && strlen(data_file) > 0) {
      if (restore_global_data(lsb, data_file)) return 3;
    }
  }
  lua_atpanic(lsb->lua, pf);
  return 0;
}


char* lsb_destroy(lua_sandbox* lsb, const char* data_file)
{
  char* err = NULL;
  if (!lsb) {
    return err;
  }

  if (lsb->lua && data_file && strlen(data_file) > 0) {
    if (preserve_global_data(lsb, data_file) != 0) {
      size_t len = strlen(lsb->error_message);
      err = malloc(len + 1);
      if (err != NULL) {
        strcpy(err, lsb->error_message);
      }
    }
  }
  sandbox_terminate(lsb);
  free(lsb->output.data);
  free(lsb->lua_file);
  free(lsb->require_path);
  free(lsb);
  return err;
}


unsigned lsb_usage(lua_sandbox* lsb, lsb_usage_type utype,
                   lsb_usage_stat ustat)
{
  if (!lsb || utype >= LSB_UT_MAX || ustat >= LSB_US_MAX) {
    return 0;
  }
#ifdef LUA_JIT
  if (lsb->lua && utype == LSB_UT_MEMORY) {
    switch (ustat) {
    case LSB_US_CURRENT:
      return lua_gc(lsb->lua, LUA_GCCOUNT, 0) * 1024
             + lua_gc(lsb->lua, LUA_GCCOUNTB, 0);
    case LSB_US_MAXIMUM:
      return lua_gc(lsb->lua, LUA_GCMAXCOUNT, 0) * 1024
             + lua_gc(lsb->lua, LUA_GCMAXCOUNTB, 0);
    default:
      break;
    }
  }
#endif
  return lsb->usage[utype][ustat];
}


const char* lsb_get_error(lua_sandbox* lsb)
{
  if (lsb) {
    return lsb->error_message;
  }
  return "";
}


lua_State* lsb_get_lua(lua_sandbox* lsb)
{
  return lsb->lua;
}


void* lsb_get_parent(lua_sandbox* lsb)
{
  return lsb->parent;
}


lsb_state lsb_get_state(lua_sandbox* lsb)
{
  if (lsb) {
    return lsb->state;
  }
  return LSB_UNKNOWN;
}


void lsb_add_function(lua_sandbox* lsb, lua_CFunction func,
                      const char* func_name)
{
  lua_pushlightuserdata(lsb->lua, (void*)lsb);
  lua_pushcclosure(lsb->lua, func, 1);
  lua_setglobal(lsb->lua, func_name);
}


const char* lsb_get_output(lua_sandbox* lsb, size_t* len)
{
  if (len) {
    *len = lsb->output.pos;
  }
  if (lsb->output.pos == 0) return "";

  lsb->output.pos = 0;
  return lsb->output.data;
}


const char* lsb_output_userdata(lua_sandbox* lsb, int index, int append)
{
  if (!append) {
    lsb->output.pos = 0;
  }

  void* ud = userdata_type(lsb->lua, index, lsb_circular_buffer);
  if (ud) {
    circular_buffer* cb = (circular_buffer*)ud;
    size_t last_pos = lsb->output.pos;
    if (output_circular_buffer(lsb->lua, cb, &lsb->output)) {
      lsb->output.pos = last_pos;
      snprintf(lsb->error_message, LSB_ERROR_SIZE, "output_limit exceeded");
      return NULL;
    }
    update_output_stats(lsb);
    return get_output_format(cb);
  }

  snprintf(lsb->error_message, LSB_ERROR_SIZE, "unknown userdata type");
  return NULL;
}


int lsb_output_protobuf(lua_sandbox* lsb, int index, int append)
{
  if (!append) {
    lsb->output.pos = 0;
  }

  size_t last_pos = lsb->output.pos;
  if (serialize_table_as_pb(lsb, index) != 0) {
    lsb->output.pos = last_pos;
    return 1;
  }

  update_output_stats(lsb);
  return 0;
}


int lsb_pcall_setup(lua_sandbox* lsb, const char* func_name)
{

  lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
              lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  lua_getglobal(lsb->lua, func_name);
  if (!lua_isfunction(lsb->lua, -1)) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE,
                       "%s() function was not found",
                       func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return 1;
  }
  return 0;
}


void lsb_pcall_teardown(lua_sandbox* lsb)
{
  lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] =
    (unsigned)instruction_usage(lsb);
  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
      lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
  }
}


void lsb_terminate(lua_sandbox* lsb, const char* err)
{
  strncpy(lsb->error_message, err, LSB_ERROR_SIZE);
  lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
  sandbox_terminate(lsb);
}
