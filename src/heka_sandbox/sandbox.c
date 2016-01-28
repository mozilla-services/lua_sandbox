/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Heka sandbox implementation @file */

#include "luasandbox/heka/sandbox.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/heka/message_matcher.h"
#include "luasandbox.h"
#include "luasandbox_output.h"
#include "message_impl.h"
#include "sandbox_impl.h"
#include "stream_reader_impl.h"
#include "luasandbox/util/protobuf.h"
#include "luasandbox/util/running_stats.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

static const char *pm_func_name = "process_message";
static const char *im_func_name = "inject_message";

static int read_message(lua_State *lua)
{
  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == lsb) {
    return luaL_error(lua, "read_message() invalid lightuserdata");
  }
  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);

  if (!hsb->msg || !hsb->msg->raw.s) {
    lua_pushnil(lua);
    return 1;
  }
  return heka_read_message(lua, hsb->msg);
}


static int inject_message_input(lua_State *lua)
{
  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == lsb) {
    return luaL_error(lua, "%s() invalid lightuserdata", im_func_name);
  }

  const char *scp =  NULL;
  double ncp = NAN;
  int t = lua_type(lua, 2);
  switch (t) {
  case LUA_TNUMBER:
    ncp = lua_tonumber(lua, 2);
    break;
  case LUA_TSTRING:
    scp = lua_tostring(lua, 2);
    break;
  case LUA_TNONE:
  case LUA_TNIL:
    break;
  default:
    return luaL_error(lua, "%s() unsupported checkpoint type: %s", im_func_name,
                      lua_typename(lua, t));
  }

  lsb_const_string output;
  t = lua_type(lua, 1);
  switch (t) {
  case LUA_TUSERDATA:
    {
      heka_stream_reader *hsr = luaL_checkudata(lua, 1,
                                                mozsvc_heka_stream_reader);
      if (hsr->msg.raw.s) {
        output.len = hsr->msg.raw.len;
        output.s = hsr->msg.raw.s;
      } else {
        return luaL_error(lua, "%s() attempted to inject a nil message",
                          im_func_name);
      }
    }
    break;
  case LUA_TSTRING:
    {
      lsb_heka_message m;
      lsb_init_heka_message(&m, 8);
      output.s = lua_tolstring(lua, 1, &output.len);
      bool ok = lsb_decode_heka_message(&m, output.s, output.len, NULL);
      lsb_free_heka_message(&m);
      if (!ok) {
        return luaL_error(lua, "%s() attempted to inject a invalid protobuf "
                          "string", im_func_name);
      }
    }
    break;
  case LUA_TTABLE:
    if (heka_encode_message_table(lsb, 1)) {
      const char *err = lsb_get_error(lsb);
      if (strlen(err) == 0) err = "exceeded output_limit";
      return luaL_error(lua, "%s() failed: %s", im_func_name, err);
    }
    output.len = 0;
    output.s = lsb_get_output(lsb, &output.len);
    break;
  default:
    return luaL_error(lua, "%s() unsupported message type: %s",
                      im_func_name, lua_typename(lua, t));
  }

  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  if (hsb->cb.iim(hsb->parent, output.s, output.len, ncp, scp) != 0) {
    return luaL_error(lua, "%s() failed: rejected by the callback",
                      im_func_name);
  }
  ++hsb->stats.im_cnt;
  hsb->stats.im_bytes += output.len;
  return 0;
}


static int inject_message_analysis(lua_State *lua)
{
  luaL_checktype(lua, 1, LUA_TTABLE);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == lsb) {
    return luaL_error(lua, "%s() invalid lightuserdata", im_func_name);
  }

  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  lua_pushstring(lua, hsb->name);
  lua_setfield(lua, 1, LSB_LOGGER);
  lua_pushstring(lua, hsb->hostname);
  lua_setfield(lua, 1, LSB_HOSTNAME);

  if (heka_encode_message_table(lsb, 1)) {
    return luaL_error(lua, "%s() failed: %s", im_func_name, lsb_get_error(lsb));
  }

  size_t output_len = 0;
  const char *output = lsb_get_output(lsb, &output_len);
  if (hsb->cb.aim(hsb->parent, output, output_len) != 0) {
    return luaL_error(lua, "%s() failed: rejected by the callback",
                      im_func_name);
  }
  ++hsb->stats.im_cnt;
  hsb->stats.im_bytes += output_len;
  return 0;
}


static int inject_payload(lua_State *lua)
{
  static const char *default_type = "txt";

  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (!lsb) {
    return luaL_error(lua, "%s invalid lightuserdata", __FUNCTION__);
  }

  int n = lua_gettop(lua);
  if (n > 2) {
    lsb_output(lsb, 3, n, 1);
    lua_pop(lua, n - 2);
  }
  size_t len = 0;
  const char *output = lsb_get_output(lsb, &len);
  if (!len) return 0;

  if (n > 0) {
    if (lua_type(lua, 1) != LUA_TSTRING) {
      return luaL_error(lua, "%s() payload_type argument must be a string",
                        __FUNCTION__);
    }
  }

  if (n > 1) {
    if (lua_type(lua, 2) != LUA_TSTRING) {
      return luaL_error(lua, "%s() payload_name argument must be a string",
                        __FUNCTION__);
    }
  }

  // build up a heka message table
  lua_createtable(lua, 0, 2); // message
  lua_createtable(lua, 0, 2); // Fields
  if (n > 0) {
    lua_pushvalue(lua, 1);
  } else {
    lua_pushstring(lua, default_type);
  }
  lua_setfield(lua, -2, "payload_type");

  if (n > 1) {
    lua_pushvalue(lua, 2);
    lua_setfield(lua, -2, "payload_name");
  }
  lua_setfield(lua, -2, LSB_FIELDS);
  lua_pushstring(lua, "inject_payload");
  lua_setfield(lua, -2, LSB_TYPE);
  lua_pushlstring(lua, output, len);
  lua_setfield(lua, -2, LSB_PAYLOAD);
  if (lua_gettop(lua) > 1) lua_replace(lua, 1);

  inject_message_analysis(lua);
  return 0;
}


static int update_checkpoint(lua_State *lua)
{
  static const char *func_name = "update_checkpoint";

  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (!lsb) {
    return luaL_error(lua, "%s() invalid upvalueindex", func_name);
  }
  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  int result = 0;
  int n = lua_gettop(lua);
  switch (n) {
  case 2: // async case
    luaL_checktype(lua, 2, LUA_TNUMBER);
    hsb->stats.pm_failures += lua_tonumber(lua, 2);
    // fall thru
  case 1:
    luaL_checktype(lua, 1, LUA_TLIGHTUSERDATA);
    result = hsb->cb.ucp(hsb->parent, lua_touserdata(lua, 1));
    break;
  case 0: // batch case
    result = hsb->cb.ucp(hsb->parent, NULL);
    break;
  default:
    return luaL_error(lua, "%s() invalid number of args: %d", func_name, n);
  }
  if (result) {
    return luaL_error(lua, "%s() failed: rejected by the callback", func_name);
  }
  return result;
}


static void set_restrictions(lua_State *lua, lsb_heka_sandbox *hsb)
{
  static const char *io[] = {
    NULL, "", "dofile", "load", "loadfile", "loadstring", "newproxy", "print",
    NULL, "os", "getenv", "exit", "setlocale",
    NULL, "string", "dump"
  };

  static const char *analysis[] = {
    NULL, "", "collectgarbage", "dofile", "load", "loadfile", "loadstring",
    "newproxy", "print",
    NULL, "os", "getenv", "execute", "exit", "remove", "rename", "setlocale",
    "tmpname",
    NULL, "string", "dump"
  };

  const char **list;
  size_t list_size;

  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  switch (hsb->type) {
  case 'i':
  case 'o':
    list_size = sizeof io / sizeof*io;
    list = io;
    break;
  default:
    list_size = sizeof analysis / sizeof*analysis;
    list = analysis;
    lua_newtable(lua);
    lua_pushboolean(lua, true);
    lua_setfield(lua, -2, "io");
    lua_pushboolean(lua, true);
    lua_setfield(lua, -2, "coroutine");
    lua_setfield(lua, 1, "disable_modules");
    break;
  }

  lua_newtable(lua);
  for (unsigned i = 0, j = 1; i < list_size; ++i) {
    if (list[i]) {
      lua_pushstring(lua, list[i]);
      lua_rawseti(lua, -2, j++);
    } else {
      if (lua_gettop(lua) == 3) lua_pop(lua, 1); // remove the previous list
      lua_newtable(lua);
      lua_pushvalue(lua, -1);
      lua_setfield(lua, -3, list[++i]);
      j = 1;
    }
  }
  lua_pop(lua, 1); // remove the last list
  lua_setfield(lua, 1, "remove_entries");

  lua_getfield(lua, 1, LSB_HEKA_MAX_MESSAGE_SIZE);
  if (lua_type(lua, -1) != LUA_TNUMBER || lua_tointeger(lua, -1) <= 0) {
    lua_pushnumber(lua, 64 * 1024);
    lua_setfield(lua, 1, LSB_HEKA_MAX_MESSAGE_SIZE);
  }
  lua_pop(lua, 1); // remove max_message_size

  lua_getfield(lua, 1, LSB_LOGGER);
  size_t len;
  const char *tmp = lua_tolstring(lua, -1, &len);
  if (tmp) {
    hsb->name = malloc(len + 1);
    if (hsb->name) strcpy(hsb->name, tmp);
  }
  lua_pop(lua, 1); // remove the Logger

  lua_getfield(lua, 1, LSB_HOSTNAME);
  tmp = lua_tolstring(lua, -1, &len);
  if (tmp) {
    hsb->hostname = malloc(len + 1);
    if (hsb->hostname) strcpy(hsb->hostname, tmp);
  } else {
    char hostname[256] = { 0 };
    if (gethostname(hostname, sizeof hostname)) {
      hostname[sizeof hostname - 1] = 0;
    }
    len = strlen(hostname);
    hsb->hostname = malloc(len + 1);
    if (hsb->hostname) strcpy(hsb->hostname, hostname);
  }
  lua_pop(lua, 1); // remove the Hostname

  lua_pop(lua, 1); // remove the lsb_config table
}


lsb_heka_sandbox* lsb_heka_create_input(void *parent,
                                        const char *lua_file,
                                        const char *state_file,
                                        const char *lsb_cfg,
                                        lsb_logger logger,
                                        lsb_heka_inject_message_input im)
{
  if (!lua_file) {
    if (logger) logger(__FUNCTION__, 3, "lua_file must be specified");
    return NULL;
  }

  if (!im) {
    if (logger) logger(__FUNCTION__, 3, "inject_message callback must be "
                       "specified");
    return NULL;
  }

  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger) logger(__FUNCTION__, 3, "memory allocation failed");
    return NULL;
  }

  hsb->type = 'i';
  hsb->parent = parent;
  hsb->msg = NULL;
  hsb->cb.iim = im;
  hsb->name = NULL;
  hsb->hostname = NULL;

  hsb->lsb = lsb_create(hsb, lua_file, lsb_cfg, logger);
  if (!hsb->lsb) {
    free(hsb);
    return NULL;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  set_restrictions(lua, hsb);

// todo link print to the logger or a no-op
  lsb_add_function(hsb->lsb, heka_decode_message, "decode_message");
  lsb_add_function(hsb->lsb, read_message, "read_message");
  lsb_add_function(hsb->lsb, inject_message_input, "inject_message");
// inject_payload is intentionally excluded from input plugins
// you can construct whatever you need with inject_message

// preload the Heka stream reader module
  luaL_findtable(lua, LUA_REGISTRYINDEX, "_PRELOADED", 1);
  lua_pushstring(lua, mozsvc_heka_stream_reader_table);
  lua_pushcfunction(lua, luaopen_heka_stream_reader);
  lua_rawset(lua, -3);
  lua_pop(lua, 1); // remove the preloaded table

  if (lsb_init(hsb->lsb, state_file)) {
    if (logger) logger(hsb->name, 3, lsb_get_error(hsb->lsb));
    lsb_destroy(hsb->lsb);
    free(hsb->hostname);
    free(hsb->name);
    free(hsb);
    return NULL;
  }

// remove output function
  lua_pushnil(lua);
  lua_setglobal(lua, "output");

  return hsb;
}


static int process_message(lsb_heka_sandbox *hsb, lsb_heka_message *msg,
                           lua_State *lua, int nargs, bool profile)
{
  unsigned long long start, end;

  hsb->msg = msg;
  if (profile) {
    start = lsb_get_time();
  }
  if (lua_pcall(lua, nargs, 2, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    size_t len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", pm_func_name,
                          lua_tostring(lua, -1));
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  if (profile) {
    end = lsb_get_time();
    lsb_update_running_stats(&hsb->stats.pm, end - start);
  }
  hsb->msg = NULL;

  if (lua_type(lua, 1) != LUA_TNUMBER) {
    char err[LSB_ERROR_SIZE];
    size_t len = snprintf(err, LSB_ERROR_SIZE,
                          "%s() must return a numeric status code",
                          pm_func_name);
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  }

  int status = (int)lua_tointeger(lua, 1);
  switch (lua_type(lua, 2)) {
  case LUA_TNONE:
  case LUA_TNIL:
    lsb_set_error(hsb->lsb, NULL);
    break;
  case LUA_TSTRING:
    lsb_set_error(hsb->lsb, lua_tostring(lua, 2));
    break;
  default:
    {
      char err[LSB_ERROR_SIZE];
      int len = snprintf(err, LSB_ERROR_SIZE,
                         "%s() must return a nil or string error message",
                         pm_func_name);
      if (len >= LSB_ERROR_SIZE || len < 0) {
        err[LSB_ERROR_SIZE - 1] = 0;
      }
      lsb_terminate(hsb->lsb, err);
      return 1;
    }
    break;
  }
  lua_pop(lua, 2);
  lsb_pcall_teardown(hsb->lsb);

  if (status > 0) {
    char err[LSB_ERROR_SIZE];
    size_t len = snprintf(err, LSB_ERROR_SIZE,
                          "%s() received a termination status code",
                          pm_func_name);
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  } else if (status == LSB_HEKA_PM_FAIL) {
    ++hsb->stats.pm_cnt;
    ++hsb->stats.pm_failures;
  } else  if (hsb->type != 'o' || status != LSB_HEKA_PM_RETRY) {
    ++hsb->stats.pm_cnt;
  }
  return status;
}


int lsb_heka_pm_input(lsb_heka_sandbox *hsb,
                                   lsb_heka_message *msg,
                                   double cp_numeric,
                                   const char *cp_string,
                                   bool profile)
{
  if (!hsb || hsb->type != 'i') return 1;

  if (lsb_pcall_setup(hsb->lsb, pm_func_name)) {
    char err[LSB_ERROR_SIZE];
    snprintf(err, LSB_ERROR_SIZE, "%s() function was not found", pm_func_name);
    lsb_terminate(hsb->lsb, err);
    return 1;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  if (!lua) return 1;

  if (!isnan(cp_numeric)) {
    lua_pushnumber(lua, cp_numeric);
  } else if (cp_string) {
    lua_pushstring(lua, cp_string);
  } else {
    lua_pushnil(lua);
  }
  return process_message(hsb, msg, lua, 1, profile);
}


lsb_heka_sandbox* lsb_heka_create_analysis(void *parent,
                                           const char *lua_file,
                                           const char *state_file,
                                           const char *lsb_cfg,
                                           lsb_logger logger,
                                           lsb_heka_inject_message_analysis im)
{
  if (!lua_file) {
    if (logger) logger(__FUNCTION__, 3, "lua_file must be specified");
    return NULL;
  }

  if (!im) {
    if (logger) logger(__FUNCTION__, 3, "inject_message callback must be "
                       "specified");
    return NULL;
  }


  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger) logger(__FUNCTION__, 3, "memory allocation failed");
    return NULL;
  }

  hsb->type = 'a';
  hsb->parent = parent;
  hsb->msg = NULL;
  hsb->cb.aim = im;
  hsb->name = NULL;
  hsb->hostname = NULL;

  hsb->lsb = lsb_create(hsb, lua_file, lsb_cfg, logger);
  if (!hsb->lsb) {
    free(hsb);
    return NULL;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  set_restrictions(lua, hsb);

// todo link print to the logger or a no-op
  lsb_add_function(hsb->lsb, heka_decode_message, "decode_message");
  lsb_add_function(hsb->lsb, read_message, "read_message");
  lsb_add_function(hsb->lsb, inject_message_analysis, "inject_message");
  lsb_add_function(hsb->lsb, inject_payload, "inject_payload");
// rename output to add_to_payload
  lua_getglobal(lua, "output");
  lua_setglobal(lua, "add_to_payload");
  lua_pushnil(lua);
  lua_setglobal(lua, "output");

  if (lsb_init(hsb->lsb, state_file)) {
    if (logger) logger(hsb->name, 3, lsb_get_error(hsb->lsb));
    lsb_destroy(hsb->lsb);
    free(hsb->hostname);
    free(hsb->name);
    free(hsb);
    return NULL;
  }
  return hsb;
}


int lsb_heka_pm_analysis(lsb_heka_sandbox *hsb,
                                      lsb_heka_message *msg,
                                      bool profile)
{
  if (!hsb || hsb->type != 'a') return 1;

  if (lsb_pcall_setup(hsb->lsb, pm_func_name)) {
    char err[LSB_ERROR_SIZE];
    snprintf(err, LSB_ERROR_SIZE, "%s() function was not found", pm_func_name);
    lsb_terminate(hsb->lsb, err);
    return 1;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  if (!lua) return 1;
  return process_message(hsb, msg, lua, 0, profile);
}


lsb_heka_sandbox* lsb_heka_create_output(void *parent,
                                         const char *lua_file,
                                         const char *state_file,
                                         const char *lsb_cfg,
                                         lsb_logger logger,
                                         lsb_heka_update_checkpoint ucp)
{
  if (!lua_file) {
    if (logger) logger(__FUNCTION__, 3, "lua_file must be specified");
    return NULL;
  }

  if (!ucp) {
    if (logger) logger(__FUNCTION__, 3, "update_checkpoint callback must be "
                       "specified");
    return NULL;
  }


  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger) logger(__FUNCTION__, 3, "memory allocation failed");
    return NULL;
  }

  hsb->type = 'o';
  hsb->parent = parent;
  hsb->msg = NULL;
  hsb->cb.ucp = ucp;
  hsb->name = NULL;
  hsb->hostname = NULL;

  hsb->lsb = lsb_create(hsb, lua_file, lsb_cfg, logger);
  if (!hsb->lsb) {
    free(hsb);
    return NULL;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  set_restrictions(lua, hsb);

// todo link print to the logger or a no-op
  lsb_add_function(hsb->lsb, heka_decode_message, "decode_message");
  lsb_add_function(hsb->lsb, heka_encode_message, "encode_message");
  lsb_add_function(hsb->lsb, update_checkpoint, "update_checkpoint");

  if (lsb_init(hsb->lsb, state_file)) {
    if (logger) logger(hsb->name, 3, lsb_get_error(hsb->lsb));
    lsb_destroy(hsb->lsb);
    free(hsb->hostname);
    free(hsb->name);
    free(hsb);
    return NULL;
  }

// remove output function
  lua_pushnil(lua);
  lua_setglobal(lua, "output");

  return hsb;
}


void lsb_heka_stop_sandbox(lsb_heka_sandbox *hsb)
{
  lsb_stop_sandbox(hsb->lsb);
}


void lsb_heka_terminate_sandbox(lsb_heka_sandbox *hsb, const char *err)
{
  lsb_terminate(hsb->lsb, err);
}


char* lsb_heka_destroy_sandbox(lsb_heka_sandbox *hsb)
{
  if (!hsb) return NULL;
  char *msg = lsb_destroy(hsb->lsb);

  free(hsb->hostname);
  free(hsb->name);
  free(hsb);
  return msg;
}


int lsb_heka_pm_output(lsb_heka_sandbox *hsb,
                                    lsb_heka_message *msg,
                                    void *sequence_id,
                                    bool profile)
{
  if (!hsb || hsb->type != 'o') return 1;

  if (lsb_pcall_setup(hsb->lsb, pm_func_name)) {
    char err[LSB_ERROR_SIZE];
    snprintf(err, LSB_ERROR_SIZE, "%s() function was not found", pm_func_name);
    lsb_terminate(hsb->lsb, err);
    return 1;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  if (!lua) return 1;

  int nargs = 0;
  if (sequence_id) {
    nargs = 1;
    lua_pushlightuserdata(lua, sequence_id);
  }
  return process_message(hsb, msg, lua, nargs, profile);
}


int lsb_heka_timer_event(lsb_heka_sandbox *hsb, time_t t, bool shutdown)
{
  static const char *func_name = "timer_event";

  if (!hsb || (hsb->type != 'o' && hsb->type != 'a')) return 1;

  lua_State *lua = lsb_get_lua(hsb->lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(hsb->lsb, func_name)) {
    char err[LSB_ERROR_SIZE];
    snprintf(err, LSB_ERROR_SIZE, "%s() function was not found", func_name);
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  // todo change if we need more than 1 sec resolution
  lua_pushnumber(lua, t * 1e9);
  lua_pushboolean(lua, shutdown);

  unsigned long long start, end;
  start = lsb_get_time();
  if (lua_pcall(lua, 2, 0, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    size_t len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                          lua_tostring(lua, -1));
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  end = lsb_get_time();
  lsb_update_running_stats(&hsb->stats.te, end - start);
  lsb_pcall_teardown(hsb->lsb);
  lua_gc(lua, LUA_GCCOLLECT, 0);
  return 0;
}


const char* lsb_heka_get_error(lsb_heka_sandbox *hsb)
{
  if (!hsb) return "";
  return lsb_get_error(hsb->lsb);
}


const char* lsb_heka_get_lua_file(lsb_heka_sandbox *hsb)
{
  if (!hsb) return NULL;
  return lsb_get_lua_file(hsb->lsb);
}


lsb_heka_stats lsb_heka_get_stats(lsb_heka_sandbox *hsb)
{
  if (!hsb) return (struct lsb_heka_stats){0,0,0,0,0,0,0,0,0,0,0,0};

  return (struct lsb_heka_stats){
    .mem_cur      = lsb_usage(hsb->lsb, LSB_UT_MEMORY, LSB_US_CURRENT),
    .mem_max      = lsb_usage(hsb->lsb, LSB_UT_MEMORY, LSB_US_MAXIMUM),
    .out_max      = lsb_usage(hsb->lsb, LSB_UT_OUTPUT, LSB_US_MAXIMUM),
    .ins_max      = lsb_usage(hsb->lsb, LSB_UT_INSTRUCTION, LSB_US_MAXIMUM),
    .im_cnt       = hsb->stats.im_cnt,
    .im_bytes     = hsb->stats.im_bytes,
    .pm_cnt       = hsb->stats.pm_cnt,
    .pm_failures  = hsb->stats.pm_failures,
    .pm_avg       = hsb->stats.pm.mean,
    .pm_sd        = lsb_sd_running_stats(&hsb->stats.pm),
    .te_avg       = hsb->stats.te.mean,
    .te_sd        = lsb_sd_running_stats(&hsb->stats.te)
  };
}


bool lsb_heka_is_running(lsb_heka_sandbox *hsb)
{
  if (!hsb) return false;
  if (lsb_get_state(hsb->lsb) == LSB_RUNNING) return true;
  return false;
}
