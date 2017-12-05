/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandboxed implementation @file */

#define LUA_LIB
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
#include "luasandbox/util/output_buffer.h"
#include "luasandbox_defines.h"
#include "luasandbox_impl.h"
#include "luasandbox_serialize.h"

lsb_err_id LSB_ERR_INIT       = "already initialized";
lsb_err_id LSB_ERR_LUA        = "lua error"; // use lsb_get_error for details
lsb_err_id LSB_ERR_TERMINATED = "sandbox already terminated";

static jmp_buf g_jbuf;

static const luaL_Reg preload_module_list[] = {
  { LUA_BASELIBNAME, luaopen_base },
  { LUA_COLIBNAME, luaopen_coroutine },
  { LUA_TABLIBNAME, luaopen_table },
  { LUA_IOLIBNAME, luaopen_io },
  { LUA_OSLIBNAME, luaopen_os },
  { LUA_STRLIBNAME, luaopen_string },
  { LUA_MATHLIBNAME, luaopen_math },
  { NULL, NULL }
};

static int libsize(const luaL_Reg *l)
{
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}

static void preload_modules(lua_State *lua)
{
  const luaL_Reg *lib = preload_module_list;
  luaL_findtable(lua, LUA_REGISTRYINDEX, "_PRELOADED",
                 libsize(preload_module_list));
  for (; lib->func; lib++) {
    lua_pushstring(lua, lib->name);
    lua_pushcfunction(lua, lib->func);
    lua_rawset(lua, -3);
  }
  lua_pop(lua, 1); // remove the preloaded table
}


/**
* Implementation of the memory allocator for the Lua state.
*
* See: http://www.lua.org/manual/5.1/manual.html#lua_Alloc
*
* @param ud Pointer to the lsb_lua_sandbox
* @param ptr Pointer to the memory block being allocated/reallocated/freed.
* @param osize The original size of the memory block.
* @param nsize The new size of the memory block.
*
* @return void* A pointer to the memory block.
*/
static void* memory_manager(void *ud, void *ptr, size_t osize, size_t nsize)
{
  lsb_lua_sandbox *lsb = (lsb_lua_sandbox *)ud;

  void *nptr = NULL;
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


static size_t instruction_usage(lsb_lua_sandbox *lsb)
{
  return lua_gethookcount(lsb->lua) - lua_gethookcountremaining(lsb->lua);
}


static void instruction_manager(lua_State *lua, lua_Debug *ar)
{
  if (LUA_HOOKCOUNT == ar->event) {
    luaL_error(lua, "instruction_limit exceeded");
  }
}


static int output(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) return luaL_error(lua, "%s() invalid " LSB_THIS_PTR, __func__);

  int n = lua_gettop(lua);
  if (n == 0) {
    return luaL_argerror(lsb->lua, 0, "must have at least one argument");
  }
  lsb_output_coroutine(lsb, lua, 1, n, 1);
  return 0;
}


static int output_print(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) return luaL_error(lua, "print() invalid " LSB_THIS_PTR);

  lsb->output.buf[0] = 0;
  lsb->output.pos = 0; // clear the buffer

  int n = lua_gettop(lua);
  if (!lsb->logger.cb || n == 0) {
    return 0;
  }

  lua_getglobal(lua, "tostring");
  for (int i = 1; i <= n; ++i) {
    lua_pushvalue(lua, -1);  // tostring
    lua_pushvalue(lua, i);   // value
    lua_call(lua, 1, 1);
    const char *s = lua_tostring(lua, -1);
    if (s == NULL) {
      return luaL_error(lua, LUA_QL("tostring") " must return a string to "
                        LUA_QL("print"));
    }
    if (i > 1) {
      lsb_outputc(&lsb->output, '\t');
    }

    while (*s) {
      if (isprint(*s)) {
        lsb_outputc(&lsb->output, *s);
      } else {
        lsb_outputc(&lsb->output, ' ');
      }
      ++s;
    }
    lua_pop(lua, 1);
  }

  const char *component = NULL;
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  if (lua_type(lua, -1) == LUA_TTABLE) {
    // this makes an assumptions by looking for a Heka sandbox specific cfg
    // variable but will fall back to the lua filename in the generic case
    lua_getfield(lua, -1, "Logger");
    component = lua_tostring(lua, -1);
    if (!component) {
      component = lsb->lua_file;
    }
  }

  lsb->logger.cb(lsb->logger.context, component, 7, "%s", lsb->output.buf);
  lsb->output.pos = 0;
  return 0;
}


static int read_config(lua_State *lua)
{
  luaL_checkstring(lua, 1);
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "too many arguments");
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  if (lua_type(lua, -1) == LUA_TTABLE) {
    lua_getfield(lua, -1, lua_tostring(lua, 1));
  } else {
    lua_pushnil(lua);
  }
  return 1;
}


static int unprotected_panic(lua_State *lua)
{
  (void)lua;
  longjmp(g_jbuf, 1);
  return 0;
}


static size_t get_size(lua_State *lua, int idx, const char *item)
{
  lua_getfield(lua, idx, item);
  size_t size = (size_t)lua_tonumber(lua, -1);
  lua_pop(lua, 1);
  return size;
}


static int check_string(lua_State *L, int idx, const char *name,
                        const char *dflt)
{
  lua_getfield(L, idx, name);
  int t = lua_type(L, -1);
  switch (t) {
  case LUA_TSTRING:
    break;
  case LUA_TNIL:
    if (dflt) {
      lua_pushstring(L, dflt); // add the default to the config
      lua_setglobal(L, name);
    }
    break;
  default:
    lua_pushfstring(L, "%s must be set to a string", name);
    return 1;
  }
  return 0;
}


static int check_unsigned(lua_State *L, int idx, const char *name, unsigned val)
{
  lua_getfield(L, idx, name);
  double d;
  switch (lua_type(L, -1)) {
  case LUA_TNUMBER:
    d = lua_tonumber(L, -1);
    if (d < 0 || d > UINT_MAX) {
      lua_pushfstring(L, "%s must be an unsigned int", name);
      return 1;
    }
    break;
  case LUA_TNIL: // add the default to the config
    lua_pushnumber(L, (lua_Number)val);
    lua_setglobal(L, name);
    break; // use the default
  default:
    lua_pushfstring(L, "%s must be set to a number", name);
    return 1;
  }
  lua_pop(L, 1);
  return 0;
}


static int check_size(lua_State *L, int idx, const char *name, size_t val)
{
  lua_getfield(L, idx, name);
  double d;
  switch (lua_type(L, -1)) {
  case LUA_TNUMBER:
    d = lua_tonumber(L, -1);
    if (d < 0 || d > SIZE_MAX) {
      lua_pushfstring(L, "%s must be a size_t", name);
      return 1;
    }
    break;
  case LUA_TNIL: // add the default to the config
    lua_pushnumber(L, (lua_Number)val);
    lua_setglobal(L, name);
    break; // use the default
  default:
    lua_pushfstring(L, "%s must be set to a number", name);
    return 1;
  }
  lua_pop(L, 1);
  return 0;
}


static lua_State* load_sandbox_config(const char *cfg, lsb_logger *logger)
{
  lua_State *L = luaL_newstate();
  if (!L) {
    if (logger->cb) logger->cb(logger->context, __func__, 3,
                               "lua_State creation failed");
    return NULL;
  }

  if (!cfg) cfg = ""; // use the default settings

  int ret = luaL_dostring(L, cfg);
  if (ret) goto cleanup;

  ret = check_size(L, LUA_GLOBALSINDEX, LSB_INPUT_LIMIT, 64 * 1024);
  if (ret) goto cleanup;

  ret = check_size(L, LUA_GLOBALSINDEX, LSB_OUTPUT_LIMIT, 64 * 1024);
  if (ret) goto cleanup;

  ret = check_size(L, LUA_GLOBALSINDEX, LSB_MEMORY_LIMIT, 8 * 1024 * 1024);
  if (ret) goto cleanup;

  ret = check_size(L, LUA_GLOBALSINDEX, LSB_INSTRUCTION_LIMIT, 1000000);
  if (ret) goto cleanup;

  ret = check_unsigned(L, LUA_GLOBALSINDEX, LSB_LOG_LEVEL, 3);
  if (ret) goto cleanup;

  ret = check_string(L, LUA_GLOBALSINDEX, LSB_LUA_PATH, NULL);
  if (ret) goto cleanup;

  ret = check_string(L, LUA_GLOBALSINDEX, LSB_LUA_CPATH, NULL);
  if (ret) goto cleanup;

cleanup:
  if (ret) {
    if (logger->cb) {
      logger->cb(logger->context, __func__, 3, "config error: %s",
                 lua_tostring(L, -1));
    }
    lua_close(L);
    return NULL;
  }
  return L;
}


static void copy_table(lua_State *sb, lua_State *cfg, lsb_logger *logger)
{
  lua_newtable(sb);
  lua_pushnil(cfg);
  while (lua_next(cfg, -2) != 0) {
    int kt = lua_type(cfg, -2);
    int vt = lua_type(cfg, -1);
    switch (kt) {
    case LUA_TNUMBER:
    case LUA_TSTRING:
      switch (vt) {
      case LUA_TSTRING:
        {
          size_t len;
          const char *tmp = lua_tolstring(cfg, -1, &len);
          if (tmp) {
            lua_pushlstring(sb, tmp, len);
            if (kt == LUA_TSTRING) {
              lua_setfield(sb, -2, lua_tostring(cfg, -2));
            } else {
              lua_rawseti(sb, -2, (int)lua_tointeger(cfg, -2));
            }
          }
        }
        break;
      case LUA_TNUMBER:
        lua_pushnumber(sb, lua_tonumber(cfg, -1));
        if (kt == LUA_TSTRING) {
          lua_setfield(sb, -2, lua_tostring(cfg, -2));
        } else {
          lua_rawseti(sb, -2, (int)lua_tointeger(cfg, -2));
        }
        break;
      case LUA_TBOOLEAN:
        lua_pushboolean(sb, lua_toboolean(cfg, -1));
        if (kt == LUA_TSTRING) {
          lua_setfield(sb, -2, lua_tostring(cfg, -2));
        } else {
          lua_rawseti(sb, -2, (int)lua_tointeger(cfg, -2));
        }
        break;
      case LUA_TTABLE:
        copy_table(sb, cfg, logger);
        break;
      default:
        if (logger->cb) {
          logger->cb(logger->context, __func__, 4,
                     "skipping config value type: %s", lua_typename(cfg, vt));
        }
        break;
      }
      break;
    default:
      if (logger->cb) {
        logger->cb(logger->context, __func__, 4, "skipping config key type: %s",
                   lua_typename(cfg, kt));
      }
      break;
    }
    lua_pop(cfg, 1);
  }

  switch (lua_type(cfg, -2)) {
  case LUA_TSTRING:
    lua_setfield(sb, -2, lua_tostring(cfg, -2));
    break;
  case LUA_TNUMBER:
    lua_rawseti(sb, -2, (int)lua_tointeger(cfg, -2));
    break;
  }
}


static void set_random_seed()
{
  bool seeded = false;
#ifdef _WIN32
  // todo use CryptGenRandom to seed srand
#else
  FILE *fh = fopen("/dev/urandom", "r" CLOSE_ON_EXEC);
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
}


lsb_lua_sandbox* lsb_create(void *parent,
                            const char *lua_file,
                            const char *cfg,
                            lsb_logger *logger)
{
  if (!lua_file) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "lua_file must be specified");
    }
    return NULL;
  }

  if (!lsb_set_tz(NULL)) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "fail to set the TZ to UTC");
    }
    return NULL;
  }

  set_random_seed();

  lsb_lua_sandbox *lsb = calloc(1, sizeof(*lsb));
  if (!lsb) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "memory allocation failed");
    }
    return NULL;
  }

  lsb->lua = lua_newstate(memory_manager, lsb);
  if (logger) {
    lsb->logger = *logger;
  }

  if (!lsb->lua) {
    if (lsb->logger.cb) {
      lsb->logger.cb(lsb->logger.context, __func__, 3, "lua state creation "
                     "failed");
    }
    free(lsb);
    return NULL;
  }


  // add the config to the lsb_config registry table
  lua_State *lua_cfg = load_sandbox_config(cfg, &lsb->logger);
  if (!lua_cfg) {
    lua_close(lsb->lua);
    free(lsb);
    return NULL;
  }
  lua_pushnil(lua_cfg);
  lua_pushvalue(lua_cfg, LUA_GLOBALSINDEX);
  copy_table(lsb->lua, lua_cfg, &lsb->logger);
  lua_pop(lua_cfg, 2);
  lua_close(lua_cfg);
  size_t ml = get_size(lsb->lua, -1, LSB_MEMORY_LIMIT);
  size_t il = get_size(lsb->lua, -1, LSB_INSTRUCTION_LIMIT);
  size_t ol = get_size(lsb->lua, -1, LSB_OUTPUT_LIMIT);
  size_t log_level = get_size(lsb->lua, -1, LSB_LOG_LEVEL);
  lua_setfield(lsb->lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  lua_pushlightuserdata(lsb->lua, lsb);
  lua_setfield(lsb->lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lua_pushcfunction(lsb->lua, &read_config);
  lua_setglobal(lsb->lua, "read_config");

  lua_pushcfunction(lsb->lua, &output);
  lua_setglobal(lsb->lua, "output");
  preload_modules(lsb->lua);

  lsb->parent = parent;
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = ml;
  lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] = il;
  lsb->usage[LSB_UT_OUTPUT][LSB_US_LIMIT] = ol;
  lsb->state = LSB_UNKNOWN;
  lsb->error_message[0] = 0;
  lsb->lua_file = malloc(strlen(lua_file) + 1);
  lsb->state_file = NULL;
  if (log_level != 7) {
    lsb->logger.cb = NULL; // only give the sandbox access to the logger (print)
                           // when debugging
  }

  if (!lsb->lua_file || lsb_init_output_buffer(&lsb->output, ol)) {
    if (lsb->logger.cb) {
      lsb->logger.cb(lsb->logger.context, __func__, 3, "memory allocation "
                     "failed");
    }
    lsb_free_output_buffer(&lsb->output);
    free(lsb->lua_file);
    lua_close(lsb->lua);
    lsb->lua = NULL;
    free(lsb);
    return NULL;
  }
  strcpy(lsb->lua_file, lua_file);
  return lsb;
}


lsb_err_value lsb_init(lsb_lua_sandbox *lsb, const char *state_file)
{
  if (!lsb) {
    return LSB_ERR_UTIL_NULL;
  }

  if (lsb->state != LSB_UNKNOWN) {
    lsb_terminate(lsb, LSB_ERR_INIT);
    return LSB_ERR_INIT;
  }

  if (state_file && strlen(state_file) > 0) {
    lsb->state_file = malloc(strlen(state_file) + 1);
    if (!lsb->state_file) {
      lsb_terminate(lsb, LSB_ERR_UTIL_OOM);
      return LSB_ERR_UTIL_OOM;
    }
    strcpy(lsb->state_file, state_file);
  }

  size_t mem_limit = lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT];
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = 0;

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
             "lsb_init() 'require' not found");
    lsb_terminate(lsb, NULL);
    return LSB_ERR_LUA;
  }
  lua_pushstring(lsb->lua, LUA_BASELIBNAME);
  if (lua_pcall(lsb->lua, 1, 0, 0)) {
    const char *em = lua_tostring(lsb->lua, -1);
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "lsb_init %s", em ? em : LSB_NIL_ERROR);
    lsb_terminate(lsb, NULL);
    return LSB_ERR_LUA;
  }
  lsb_add_function(lsb, output_print, "print");

  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] != 0) {
    lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
                (int)lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  } else {
    lua_sethook(lsb->lua, NULL, 0, 0);
  }
  lsb->usage[LSB_UT_MEMORY][LSB_US_LIMIT] = mem_limit;
  lua_CFunction pf = lua_atpanic(lsb->lua, unprotected_panic);
  int jump = setjmp(g_jbuf);
  if (jump || luaL_dofile(lsb->lua, lsb->lua_file) != 0) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE, "%s",
                       lua_tostring(lsb->lua, -1));
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, NULL);
    return LSB_ERR_LUA;
  } else {
    lua_gc(lsb->lua, LUA_GCCOLLECT, 0);
    lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] = instruction_usage(lsb);
    if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
        > lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
      lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
          lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
    }
    lsb->state = LSB_RUNNING;
    if (lsb->state_file) {
      lsb_err_value ret = restore_global_data(lsb);
      if (ret) return ret;
    }
  }
  lua_atpanic(lsb->lua, pf);
  return NULL;
}


static void stop_hook(lua_State *lua, lua_Debug *ar)
{
  (void)ar;  /* unused arg. */
  lua_sethook(lua, NULL, 0, 0);
  luaL_error(lua, LSB_SHUTTING_DOWN);
}


void lsb_stop_sandbox_clean(lsb_lua_sandbox *lsb)
{
  if (!lsb) {
    return;
  }
  lsb->state = LSB_STOP;
}


void lsb_stop_sandbox(lsb_lua_sandbox *lsb)
{
  if (!lsb) {
    return;
  }

  lua_State *lua = lsb_get_lua(lsb);
  if (lua) {
    lua_sethook(lua, stop_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
  }
}


char* lsb_destroy(lsb_lua_sandbox *lsb)
{
  char *err = NULL;
  if (!lsb) {
    return err;
  }

  if (preserve_global_data(lsb)) {
    size_t len = strlen(lsb->error_message);
    err = malloc(len + 1);
    if (err != NULL) {
      strcpy(err, lsb->error_message);
    }
  }

  if (lsb->lua) {
    lua_close(lsb->lua);
    lsb->lua = NULL;
  }

  lsb_free_output_buffer(&lsb->output);
  free(lsb->state_file);
  free(lsb->lua_file);
  free(lsb);
  return err;
}


size_t lsb_usage(lsb_lua_sandbox *lsb, lsb_usage_type utype,
                 lsb_usage_stat ustat)
{
  if (!lsb || utype >= LSB_UT_MAX || ustat >= LSB_US_MAX) {
    return 0;
  }
  return lsb->usage[utype][ustat];
}


const char* lsb_get_error(lsb_lua_sandbox *lsb)
{
  return lsb ? lsb->error_message : "";
}


void lsb_set_error(lsb_lua_sandbox *lsb, const char *err)
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


lua_State* lsb_get_lua(lsb_lua_sandbox *lsb)
{
  return lsb ? lsb->lua : NULL;
}


const char* lsb_get_lua_file(lsb_lua_sandbox *lsb)
{
  return lsb ? lsb->lua_file : NULL;
}


void* lsb_get_parent(lsb_lua_sandbox *lsb)
{
  return lsb ? lsb->parent : NULL;
}


const lsb_logger* lsb_get_logger(lsb_lua_sandbox *lsb)
{
  return lsb ? &lsb->logger : NULL;
}


lsb_state lsb_get_state(lsb_lua_sandbox *lsb)
{
  return lsb ? lsb->state : LSB_UNKNOWN;
}


void lsb_add_function(lsb_lua_sandbox *lsb, lua_CFunction func,
                      const char *func_name)
{
  if (!lsb || !func || !func_name) return;
  if (lsb->state == LSB_TERMINATED) return;

  lua_pushcfunction(lsb->lua, func);
  lua_setglobal(lsb->lua, func_name);
}


lsb_err_value lsb_pcall_setup(lsb_lua_sandbox *lsb, const char *func_name)
{
  if (!lsb || !func_name) return LSB_ERR_UTIL_NULL;
  if (lsb->state == LSB_TERMINATED) return LSB_ERR_TERMINATED;

  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT] != 0) {
    lua_sethook(lsb->lua, instruction_manager, LUA_MASKCOUNT,
                (int)lsb->usage[LSB_UT_INSTRUCTION][LSB_US_LIMIT]);
  } else {
    lua_sethook(lsb->lua, NULL, 0, 0);
  }
  lua_getglobal(lsb->lua, func_name);
  if (!lua_isfunction(lsb->lua, -1)) {
    int len = snprintf(lsb->error_message, LSB_ERROR_SIZE, "%s() not found",
                       func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
    }
    return LSB_ERR_LUA;
  }
  return NULL;
}


void lsb_pcall_teardown(lsb_lua_sandbox *lsb)
{
  if (!lsb) return;

  lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT] =
      (unsigned)instruction_usage(lsb);
  if (lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_INSTRUCTION][LSB_US_MAXIMUM] =
        lsb->usage[LSB_UT_INSTRUCTION][LSB_US_CURRENT];
  }
}


void lsb_terminate(lsb_lua_sandbox *lsb, const char *err)
{
  if (!lsb) return;

  if (err) {
    strncpy(lsb->error_message, err, LSB_ERROR_SIZE);
    lsb->error_message[LSB_ERROR_SIZE - 1] = 0;
  }
  lsb->state = LSB_TERMINATED;
}
