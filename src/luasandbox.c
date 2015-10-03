/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandboxed implementation @file */

#include "luasandbox.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox/lua.h"
#include "luasandbox/lualib.h"
#include "luasandbox_output.h"
#include "lsb_private.h"
#include "luasandbox_serialize.h"
#include "lsb_serialize_protobuf.h"

static const char* standard_config = "{"
  "memory_limit = %u,"
  "instruction_limit = %u,"
  "output_limit = %u,"
  "path = [[%s]],"
  "cpath = [[%s]],"
  "remove_entries = {"
  "[''] = {'collectgarbage','coroutine','dofile','load','loadfile'"
  ",'loadstring','newproxy','print'},"
  "os = {'getenv','execute','exit','remove','rename','setlocale','tmpname'}"
  "},"
  "disable_modules = {io = 1}"
  "}";

static jmp_buf g_jbuf;

static const luaL_Reg preload_module_list[] = {
  { "", luaopen_base },
  { LUA_TABLIBNAME, luaopen_table },
  { LUA_IOLIBNAME, luaopen_io },
  { LUA_OSLIBNAME, luaopen_os },
  { LUA_STRLIBNAME, luaopen_string },
  { LUA_MATHLIBNAME, luaopen_math },
  { NULL, NULL }
};

static int libsize(const luaL_Reg* l)
{
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}

static void preload_modules(lua_State* lua)
{
  const luaL_Reg* lib = preload_module_list;
  luaL_findtable(lua, LUA_REGISTRYINDEX, "_PRELOADED",
                 libsize(preload_module_list));
  for (; lib->func; lib++) {
    lua_pushstring(lua, lib->name);
    lua_pushcfunction(lua, lib->func);
    lua_rawset(lua, -3);
  }
  lua_pop(lua, 1); // remove the preloaded table
}


#ifndef LUA_JIT
/**
* Implementation of the memory allocator for the Lua state.
*
* See: http://www.lua.org/manual/5.1/manual.html#lua_Alloc
*
* @param ud Pointer to the lua_sandbox
* @param ptr Pointer to the memory block being allocated/reallocated/freed.
* @param osize The original size of the memory block.
* @param nsize The new size of the memory block.
*
* @return void* A pointer to the memory block.
*/
static void* memory_manager(void* ud, void* ptr, size_t osize, size_t nsize)
{
  lua_sandbox* lsb = (lua_sandbox*)ud;

  void* nptr = NULL;
  if (nsize == 0) {
    free(ptr);
    lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] -= osize;
  } else {
    size_t new_state_memory =
      lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] + nsize - osize;
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


static size_t instruction_usage(lua_sandbox* lsb)
{
  return lua_gethookcount(lsb->lua) - lua_gethookcountremaining(lsb->lua);
}


static void instruction_manager(lua_State* lua, lua_Debug* ar)
{
  if (LUA_HOOKCOUNT == ar->event) {
    luaL_error(lua, "instruction_limit exceeded");
  }
}


static int output(lua_State* lua)
{
  void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    return luaL_error(lua, "output() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  int n = lua_gettop(lua);
  if (n == 0) {
    return luaL_argerror(lsb->lua, 0, "must have at least one argument");
  }
  lsb_output(lsb, 1, n, 1);
  return 0;
}


static int unprotected_panic(lua_State* lua)
{
  (void)lua;
  longjmp(g_jbuf, 1);
  return 0;
}


static size_t get_usage_config(lua_State* lua, int idx, const char* item)
{
  lua_getfield(lua, idx, item);
  size_t u = (size_t)lua_tointeger(lua, -1); // defaults to zero
  lua_pop(lua, 1);
  return u;
}


static int expand_path(const char* path, int n, const char* fmt, char* opath)
{
  // not an exhaustive check, just making sure the Lua markers are accounted for
  for (int i = 0; (path[i]); ++i) {
    if (path[i] == '\'' || path[i] == ';' || path[i] == '?'
        || !isprint(path[i])) {
      return 1;
    }
  }
  int result = snprintf(opath, n, fmt, path);
  if (result < 0 || result > n - 1) {
    return 1;
  }
  return 0;
}


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

  char config[1024 * 2];
  char lpath[260] = { 0 };
  char cpath[260] = { 0 };

  if (require_path) {
#if defined(_WIN32)
    if (expand_path(require_path, sizeof(lpath), "%s\\?.lua", lpath)) {
      return NULL;
    }
    if (expand_path(require_path, sizeof(cpath), "%s\\?.dll", cpath)) {
      return NULL;
    }
#else
    if (expand_path(require_path, sizeof(lpath), "%s/?.lua", lpath)) {
      return NULL;
    }
    if (expand_path(require_path, sizeof(cpath), "%s/?.so", cpath)) {
      return NULL;
    }
#endif
  }

  int result = snprintf(config, sizeof(config), standard_config, memory_limit,
                        instruction_limit, output_limit, lpath, cpath);

  if (result < 0 || result > (int)sizeof(config) - 1) {
    return NULL;
  }

  return lsb_create_custom(parent, lua_file, config);
}


lua_sandbox* lsb_create_custom(void* parent,
                               const char* lua_file,
                               const char* config)
{
  if (!lua_file) {
    return NULL;
  }

  bool seeded = false;
#if _WIN32
  if (_putenv("TZ=UTC") != 0) {
    return NULL;
  }
  // todo use CryptGenRandom to seed srand
#else
  if (setenv("TZ", "UTC", 1) != 0) {
    return NULL;
  }

  FILE* fh = fopen("/dev/urandom", "r");
  if (fh) {
    unsigned seed;
    unsigned char advance;
    if (fread(&seed, sizeof(unsigned), 1, fh) == 1 &&
        fread(&advance, sizeof(char), 1, fh) == 1) {
      srand(seed);
      // advance the sequence a random amount
      for (unsigned i = 0; i < advance; ++i) {
        rand();
      }
      seeded = true;
    }
    fclose(fh);
  }
#endif
  if (!seeded) {
    srand((unsigned)time(NULL));
  }

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

  // create config table
  lua_pushfstring(lsb->lua, "return %s", config);
  if (luaL_dostring(lsb->lua, lua_tostring(lsb->lua, -1))
      || lua_type(lsb->lua, -1) != LUA_TTABLE) {
    lua_close(lsb->lua);
    lsb->lua = NULL;
    free(lsb);
    return NULL;
  }
  size_t memory_limit = get_usage_config(lsb->lua, -1, "memory_limit");
  size_t instruction_limit = get_usage_config(lsb->lua, -1, "instruction_limit");
  size_t output_limit = get_usage_config(lsb->lua, -1, "output_limit");
  lua_setfield(lsb->lua, LUA_REGISTRYINDEX, "lsb_config");
  lua_pop(lsb->lua, 1); // remove configuration string

  lsb->parent = parent;
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = memory_limit;
  lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] = instruction_limit;
  lsb->usage[LSB_UT_OUTPUT][LSB_US_LIMIT] = output_limit;
  lsb->state = LSB_UNKNOWN;
  lsb->error_message[0] = 0;
  lsb->output.pos = 0;
  lsb->output.maxsize = output_limit;
  if (output_limit && output_limit < LSB_OUTPUT_SIZE) {
    lsb->output.size = output_limit;
  } else {
    lsb->output.size = LSB_OUTPUT_SIZE;
  }
  lsb->output.data = malloc(lsb->output.size);
  size_t len = strlen(lua_file);
  lsb->lua_file = malloc(len + 1);

  if (!lsb->output.data || !lsb->lua_file) {
    free(lsb->output.data);
    free(lsb->lua_file);
    lua_close(lsb->lua);
    lsb->lua = NULL;
    free(lsb);
    return NULL;
  }
  strcpy(lsb->lua_file, lua_file);
  return lsb;
}


int lsb_init(lua_sandbox* lsb, const char* data_file)
{
  if (!lsb) {
    return 0;
  }
#ifndef LUA_JIT
  size_t mem_limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;
#endif

  preload_modules(lsb->lua);
  // load package module
  lua_pushcfunction(lsb->lua, luaopen_package);
  lua_pushstring(lsb->lua, LUA_LOADLIBNAME);
  lua_call(lsb->lua, 1, 1);
  lua_newtable(lsb->lua);
  lua_setmetatable(lsb->lua, -2);
  lua_pop(lsb->lua, 1);

  // load base module
  lua_getglobal(lsb->lua, "require");
  if (!lua_iscfunction(lsb->lua, -1)) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "lsb_init 'require' not found");
    lsb_terminate(lsb, NULL);
    return 1;
  }
  lua_pushstring(lsb->lua, "");
  if (lua_pcall(lsb->lua, 1, 0, 0)) {
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "lsb_init %s", lua_tostring(lsb->lua, -1));
    lsb_terminate(lsb, NULL);
    return 1;
  }

  lua_pushlightuserdata(lsb->lua, (void*)lsb);
  lua_pushcclosure(lsb->lua, &output, 1);
  lua_setglobal(lsb->lua, "output");

  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] != 0) {
    lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
                (int)lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  } else {
    lua_sethook(lsb->lua, NULL, 0, 0);
  }
#ifdef LUA_JIT
  // todo limit
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
    lsb_terminate(lsb, NULL);
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
      if (lsb_restore_global_data(lsb, data_file)) return 3;
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
    if (lsb_preserve_global_data(lsb, data_file) != 0) {
      size_t len = strlen(lsb->error_message);
      err = malloc(len + 1);
      if (err != NULL) {
        strcpy(err, lsb->error_message);
      }
    }
  }
  lsb_terminate(lsb, NULL);
  free(lsb->output.data);
  free(lsb->lua_file);
  free(lsb);
  return err;
}


size_t lsb_usage(lua_sandbox* lsb, lsb_usage_type utype,
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


void lsb_set_error(lua_sandbox* lsb, const char* err)
{
  if (lsb) {
    if (err) {
      strncpy(lsb->error_message, err, LSB_ERROR_SIZE);
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    } else {
      lsb->error_message[0] = 0;
    }
  }
}


lua_State* lsb_get_lua(lua_sandbox* lsb)
{
  return lsb->lua;
}


const char* lsb_get_lua_file(lua_sandbox* lsb)
{
  return lsb->lua_file;
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


int lsb_pcall_setup(lua_sandbox* lsb, const char* func_name)
{
  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] != 0) {
    lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
                (int)lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  } else {
    lua_sethook(lsb->lua, NULL, 0, 0);
  }
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
  if (err) {
    strncpy(lsb->error_message, err, LSB_ERROR_SIZE);
    lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
  }

  if (lsb->lua) {
    lua_close(lsb->lua);
    lsb->lua = NULL;
  }
  lsb->usage[LSB_UT_MEMORY][LSB_US_CURRENT] = 0;
  lsb->state = LSB_TERMINATED;
}
