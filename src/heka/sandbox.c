/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Heka sandbox implementation @file */

#include "luasandbox/heka/sandbox.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../luasandbox_defines.h"
#include "../luasandbox_impl.h"
#include "luasandbox.h"
#include "luasandbox/lualib.h"
#include "luasandbox/heka/stream_reader.h"
#include "luasandbox/util/heka_message_matcher.h"
#include "luasandbox/util/protobuf.h"
#include "luasandbox/util/running_stats.h"
#include "luasandbox_output.h"
#include "message_impl.h"
#include "sandbox_impl.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

lsb_err_id LSB_ERR_HEKA_INPUT = "invalid input";

static const char *pm_func_name = "process_message";
static const char *im_func_name = "inject_message";
static const char *lsb_heka_message_matcher = "lsb.heka_message_matcher";

int heka_create_stream_reader(lua_State *lua);
int heka_create_read_message_zc(lua_State *lua);


static int is_running(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);
  lsb_heka_sandbox *hsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!hsb) {
    return luaL_error(lua, "%s() invalid " LSB_HEKA_THIS_PTR, __func__);
  }
  // call inject_message with a NULL message/checkpoint (special case
  // synchronization point)
  if (hsb->cb.iim(hsb->parent, NULL, 0, NAN, NULL) != 0) {
    return luaL_error(lua, "%s() failed: rejected by the callback", __func__);
  }
  lua_pushboolean(lua, lsb_heka_is_running(hsb));
  return 1;
}


static int read_message(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);
  lsb_heka_sandbox *hsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!hsb) {
    return luaL_error(lua, "%s() invalid " LSB_HEKA_THIS_PTR, __func__);
  }
  if (lua_gettop(lua) == 4) {
    luaL_checktype(lua, 4, LUA_TBOOLEAN);
    if (lua_toboolean(lua, 4)) {
      return heka_create_read_message_zc(lua);
    }
    lua_pop(lua, 1); // remove the zc flag
  }
  return heka_read_message(lua, hsb->msg);
}


static lsb_message_matcher* mm_check(lua_State *lua)
{
  lsb_message_matcher **ppmm = luaL_checkudata(lua, 1,
                                               lsb_heka_message_matcher);
  return *ppmm;
}


static int mm_gc(lua_State *lua)
{
  lsb_message_matcher *mm = mm_check(lua);
  lsb_destroy_message_matcher(mm);
  return 0;
}


static int mm_eval(lua_State *lua)
{
  lsb_message_matcher *mm = mm_check(lua);
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);
  lsb_heka_sandbox *hsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!hsb) {
    return luaL_error(lua, "%s() invalid " LSB_HEKA_THIS_PTR, __func__);
  }
  if (!hsb->msg || !hsb->msg->raw.s) {
    return luaL_error(lua, "no active message");
  }
  lua_pushboolean(lua, lsb_eval_message_matcher(mm, hsb->msg));
  return 1;
}


static int mm_create(lua_State *lua)
{
  const char *exp = luaL_checkstring(lua, 1);
  lsb_message_matcher **ppmm = lua_newuserdata(lua, sizeof*ppmm);
  if (luaL_newmetatable(lua, lsb_heka_message_matcher) == 1) {
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");
    lua_pushcfunction(lua, mm_gc);
    lua_setfield(lua, -2, "__gc");
    lua_pushcfunction(lua, mm_eval);
    lua_setfield(lua, -2, "eval");
  }
  lua_setmetatable(lua, -2);

  *ppmm = lsb_create_message_matcher(exp);
  if (!*ppmm) {
    return luaL_error(lua, "invalid message matcher expression");
  }
  return 1;
}


static int inject_message_input(lua_State *lua)
{
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) {
    return luaL_error(lua, "%s() invalid " LSB_THIS_PTR, im_func_name);
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
                                                LSB_HEKA_STREAM_READER);
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
    if (heka_encode_message_table(lsb, lua, 1)) {
      const char *err = lsb_get_error(lsb);
      if (strlen(err) == 0) err = "exceeded output_limit";
      return luaL_error(lua, "%s() failed: %s", im_func_name, err);
    }
    output.len = 0;
    output.s = lsb_get_output(lsb, &output.len);
    break;
  case LUA_TNIL:
    if (lua_isnoneornil(lua, 2)) {
      return luaL_error(lua, "%s() message cannot be nil without a checkpoint"
                        " update", im_func_name);
    }
    output.len = 0;
    output.s = NULL;
    break;
  default:
    return luaL_error(lua, "%s() unsupported message type: %s",
                      im_func_name, lua_typename(lua, t));
  }

  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  int rv = hsb->cb.iim(hsb->parent, output.s, output.len, ncp, scp);
  switch (rv) {
  case LSB_HEKA_IM_SUCCESS:
    break;
  case LSB_HEKA_IM_CHECKPOINT:
    return luaL_error(lua, "%s() failed: checkpoint update",
                      im_func_name);
  case LSB_HEKA_IM_ERROR:
    // fall through
  default:
    return luaL_error(lua, "%s() failed: rejected by the callback rv: %d",
                      im_func_name, rv);
  }
  if (output.s) {
    ++hsb->stats.im_cnt;
    hsb->stats.im_bytes += output.len;
  }
  return 0;
}


static int inject_message_analysis(lua_State *lua)
{
  luaL_checktype(lua, 1, LUA_TTABLE);
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) return luaL_error(lua, "%s() invalid " LSB_THIS_PTR, im_func_name);

  if (heka_encode_message_table(lsb, lua, 1)) {
    return luaL_error(lua, "%s() failed: %s", im_func_name, lsb_get_error(lsb));
  }

  size_t output_len = 0;
  const char *output = lsb_get_output(lsb, &output_len);
  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  int rv = hsb->cb.aim(hsb->parent, output, output_len);
  switch (rv) {
  case LSB_HEKA_IM_SUCCESS:
    break;
  case LSB_HEKA_IM_LIMIT:
    return luaL_error(lua, "%s() failed: injection limit exceeded",
                      im_func_name);
  case LSB_HEKA_IM_ERROR:
    // fall through
  default:
    return luaL_error(lua, "%s() failed: rejected by the callback rv: %d",
                      im_func_name, rv);
  }
  ++hsb->stats.im_cnt;
  hsb->stats.im_bytes += output_len;
  return 0;
}


static int inject_payload(lua_State *lua)
{
  static const char *default_type = "txt";

  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_THIS_PTR);
  lsb_lua_sandbox *lsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!lsb) return luaL_error(lua, "%s() invalid " LSB_THIS_PTR, __func__);

  int n = lua_gettop(lua);

  if (n > 0) {
    if (lua_type(lua, 1) != LUA_TSTRING) {
      return luaL_error(lua, "%s() payload_type argument must be a string",
                        __func__);
    }
  }

  if (n > 1) {
    if (lua_type(lua, 2) != LUA_TSTRING) {
      return luaL_error(lua, "%s() payload_name argument must be a string",
                        __func__);
    }
  }

  if (n > 2) {
    lsb_output_coroutine(lsb, lua, 3, n, 1);
    lua_pop(lua, n - 2);
  }
  size_t len = 0;
  const char *output = lsb_get_output(lsb, &len);
  if (!len) return 0;

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
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);
  lsb_heka_sandbox *hsb = lua_touserdata(lua, -1);
  lua_pop(lua, 1); // remove this ptr
  if (!hsb) {
    return luaL_error(lua, "%s() invalid " LSB_HEKA_THIS_PTR, __func__);
  }

  int result = 0;
  int n = lua_gettop(lua);
  switch (n) {
  case 2: // async case
    luaL_checktype(lua, 2, LUA_TNUMBER);
    hsb->stats.pm_failures += (unsigned long long)lua_tonumber(lua, 2);
    // fall thru
  case 1:
    luaL_checktype(lua, 1, LUA_TLIGHTUSERDATA);
    result = hsb->cb.ucp(hsb->parent, lua_touserdata(lua, 1));
    break;
  case 0: // batch case
    result = hsb->cb.ucp(hsb->parent, NULL);
    break;
  default:
    return luaL_error(lua, "%s() invalid number of args: %d", __func__, n);
  }
  if (result) {
    return luaL_error(lua, "%s() failed: rejected by the callback", __func__);
  }
  return result;
}


static void set_restrictions(lua_State *lua, lsb_heka_sandbox *hsb)
{
  static const char *io[] = {
    NULL, "", "dofile", "load", "loadfile", "loadstring", "newproxy",
    NULL, "os", "exit", "setlocale",
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

  lua_pushlightuserdata(lua, hsb);
  lua_setfield(lua, LUA_REGISTRYINDEX, LSB_HEKA_THIS_PTR);

  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  switch (hsb->type) {
  case 'i':
  case 'o':
    list_size = sizeof(io) / sizeof(*io);
    list = io;
    break;
  default:
    list_size = sizeof(analysis) / sizeof(*analysis);
    list = analysis;
    lua_newtable(lua);
    lua_pushboolean(lua, true);
    lua_setfield(lua, -2, LUA_IOLIBNAME);
    lua_pushboolean(lua, true);
    lua_setfield(lua, -2, LUA_COLIBNAME);
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

  lua_getfield(lua, 1, LSB_PID);
  hsb->pid = (int)lua_tointeger(lua, -1);
  lua_pop(lua, 1);

  lua_getfield(lua, 1, "restricted_headers");
  if (lua_type(lua, -1) == LUA_TBOOLEAN) {
    hsb->restricted_headers = lua_toboolean(lua, -1);
  }
  lua_pop(lua, 1); // remove the restricted_headers boolean

  lua_pop(lua, 1); // remove the lsb_config table
}


lsb_heka_sandbox* lsb_heka_create_input(void *parent,
                                        const char *lua_file,
                                        const char *state_file,
                                        const char *lsb_cfg,
                                        lsb_logger *logger,
                                        lsb_heka_im_input im)
{
  if (!lua_file) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "lua_file must be specified");
    }
    return NULL;
  }

  if (!im) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "inject_message callback must "
                 "be specified");
    }
    return NULL;
  }

  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "memory allocation failed");
    }
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

  lsb_add_function(hsb->lsb, heka_decode_message, "decode_message");
  lsb_add_function(hsb->lsb, inject_message_input, "inject_message");
// inject_payload is intentionally excluded from input plugins
// you can construct whatever you need with inject_message
  lsb_add_function(hsb->lsb, heka_create_stream_reader, "create_stream_reader");
  lsb_add_function(hsb->lsb, is_running, "is_running");

  if (lsb_init(hsb->lsb, state_file)) {
    if (logger && logger->cb) {
      logger->cb(logger->context, hsb->name, 3, "%s", lsb_get_error(hsb->lsb));
    }
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
    const char *em = lua_tostring(lua, -1);
    if (hsb->type == 'i' && em && strcmp(em, LSB_SHUTTING_DOWN) == 0) {
      return 0;
    }
    size_t len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", pm_func_name,
                          em ? em : LSB_NIL_ERROR);
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  if (profile) {
    end = lsb_get_time();
    lsb_update_running_stats(&hsb->stats.pm, (double)(end - start));
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
                      double cp_numeric,
                      const char *cp_string,
                      bool profile)
{
  if (!hsb || hsb->type != 'i') {
    return 1;
  }

  lsb_err_value ret = lsb_pcall_setup(hsb->lsb, pm_func_name);
  if (ret) {
    if (ret != LSB_ERR_TERMINATED) {
      char err[LSB_ERROR_SIZE];
      snprintf(err, LSB_ERROR_SIZE, "%s() function was not found",
               pm_func_name);
      lsb_terminate(hsb->lsb, err);
    }
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
  return process_message(hsb, NULL, lua, 1, profile);
}


lsb_heka_sandbox* lsb_heka_create_analysis(void *parent,
                                           const char *lua_file,
                                           const char *state_file,
                                           const char *lsb_cfg,
                                           lsb_logger *logger,
                                           lsb_heka_im_analysis im)
{
  if (!lua_file) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "lua_file must be specified");
    }
    return NULL;
  }

  if (!im) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "inject_message callback must "
                 "be specified");
    }
    return NULL;
  }


  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "memory allocation failed");
    }
    return NULL;
  }

  hsb->type = 'a';
  hsb->parent = parent;
  hsb->msg = NULL;
  hsb->cb.aim = im;
  hsb->name = NULL;
  hsb->hostname = NULL;
  hsb->restricted_headers = true;

  hsb->lsb = lsb_create(hsb, lua_file, lsb_cfg, logger);
  if (!hsb->lsb) {
    free(hsb);
    return NULL;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  set_restrictions(lua, hsb);

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
    if (logger && logger->cb) {
      logger->cb(logger->context, hsb->name, 3, "%s", lsb_get_error(hsb->lsb));
    }
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
  if (!hsb || !msg || hsb->type != 'a') return 1;

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


// IO write zero copy replacement
static int pushresult(lua_State *lua, int i, const char *filename)
{
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    lua_pushboolean(lua, 1);
    return 1;
  } else {
    lua_pushnil(lua);
    if (filename) lua_pushfstring(lua, "%s: %s", filename, strerror(en));
    else lua_pushfstring(lua, "%s", strerror(en));
    lua_pushinteger(lua, en);
    return 3;
  }
}


static int zc_write(lua_State *lua, FILE *f, int arg)
{
  int n =  lua_gettop(lua);
  int nargs = n - 1;
  int status = 1;
  for (; nargs--; arg++) {
    switch (lua_type(lua, arg)) {
    case LUA_TNUMBER:
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, LUA_NUMBER_FMT, lua_tonumber(lua, arg)) > 0;
      break;
    case LUA_TSTRING:
      {
        size_t l;
        const char *s = luaL_checklstring(lua, arg, &l);
        status = status && (fwrite(s, sizeof(char), l, f) == l);
      }
      break;
    case LUA_TUSERDATA:
      {
        lua_CFunction fp = lsb_get_zero_copy_function(lua, arg);
        if (!fp) {
          return luaL_argerror(lua, arg, "no zero copy support");
        }
        lua_pushvalue(lua, arg);
        int results = fp(lua);
        int start = n + 2;
        int end = start + results;
        size_t len;
        const char *s;
        for (int i = start; i < end; ++i) {
          switch (lua_type(lua, i)) {
          case LUA_TSTRING:
            s = lua_tolstring(lua, i, &len);
            if (s && len > 0) {
              status = status && (fwrite(s, sizeof(char), len, f) == len);
            }
            break;
          case LUA_TLIGHTUSERDATA:
            s = lua_touserdata(lua, i++);
            len = (size_t)lua_tointeger(lua, i);
            if (s && len > 0) {
              status = status && (fwrite(s, sizeof(char), len, f) == len);
            }
            break;
          default:
            return luaL_error(lua, "invalid zero copy return");
          }
        }
        lua_pop(lua, results + 1); // remove the returns values and
                                   // the copy of the userdata
      }
      break;
    default:
      return luaL_typerror(lua, arg, "number, string or userdata");
    }
  }
  return pushresult(lua, status, NULL);
}

static FILE* getiofile(lua_State *lua, int findex)
{
  static const char *const fnames[] = { "input", "output" };
  FILE *f;
  lua_rawgeti(lua, LUA_ENVIRONINDEX, findex);
  f = *(FILE **)lua_touserdata(lua, -1);
  if (f == NULL) luaL_error(lua, "standard %s file is closed",
                            fnames[findex - 1]);
  return f;
}


static int zc_io_write(lua_State *lua)
{
  return zc_write(lua, getiofile(lua, 2), 1);
}


static FILE* tofile(lua_State *lua)
{
  FILE **f = (FILE **)luaL_checkudata(lua, 1, LUA_FILEHANDLE);
  if (*f == NULL) luaL_error(lua, "attempt to use a closed file");
  return *f;
}


static int zc_f_write(lua_State *lua)
{
  return zc_write(lua, tofile(lua), 2);
}


static int zc_luaopen_io(lua_State *lua)
{
  luaopen_io(lua);
  lua_pushcclosure(lua, zc_io_write, 0);
  lua_setfield(lua, -2, "write");

  luaL_getmetatable(lua, LUA_FILEHANDLE);
  lua_pushcclosure(lua, zc_f_write, 0);
  lua_setfield(lua, -2, "write");
  lua_pop(lua, 1); // remove the metatable
  return 1;
}
// End IO write zero copy replacement


lsb_heka_sandbox* lsb_heka_create_output(void *parent,
                                         const char *lua_file,
                                         const char *state_file,
                                         const char *lsb_cfg,
                                         lsb_logger *logger,
                                         lsb_heka_update_checkpoint ucp)
{
  if (!lua_file) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "lua_file must be specified");
    }
    return NULL;
  }

  if (!ucp) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, LSB_HEKA_UPDATE_CHECKPOINT
                 " callback must be specified");
    }
    return NULL;
  }


  lsb_heka_sandbox *hsb = calloc(1, sizeof(lsb_heka_sandbox));
  if (!hsb) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 3, "memory allocation failed");
    }
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

  lsb_add_function(hsb->lsb, read_message, "read_message");
  lsb_add_function(hsb->lsb, heka_decode_message, "decode_message");
  lsb_add_function(hsb->lsb, heka_encode_message, "encode_message");
  lsb_add_function(hsb->lsb, update_checkpoint, LSB_HEKA_UPDATE_CHECKPOINT);
  lsb_add_function(hsb->lsb, mm_create, "create_message_matcher");

  // start io.write override with zero copy functionality
  lua_getfield(lua, LUA_REGISTRYINDEX, "_PRELOADED");
  lua_pushstring(lua, LUA_IOLIBNAME);
  lua_pushcfunction(lua, zc_luaopen_io);
  lua_rawset(lua, -3);
  lua_pop(lua, 1);
  // end io.write override

  if (lsb_init(hsb->lsb, state_file)) {
    if (logger && logger->cb) {
      logger->cb(logger->context, hsb->name, 3, "%s", lsb_get_error(hsb->lsb));
    }
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


void lsb_heka_stop_sandbox_clean(lsb_heka_sandbox *hsb)
{
  lsb_stop_sandbox_clean(hsb->lsb);
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
  if (!hsb || !msg || hsb->type != 'o') return 1;

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

  if (!hsb || (hsb->type != 'o' && hsb->type != 'a')) {
    return 1;
  }

  lua_State *lua = lsb_get_lua(hsb->lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(hsb->lsb, func_name)) {
    char err[LSB_ERROR_SIZE];
    snprintf(err, LSB_ERROR_SIZE, "%s() function was not found", func_name);
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  lua_pushnumber(lua, t * 1e9);
  lua_pushboolean(lua, shutdown);

  unsigned long long start, end;
  start = lsb_get_time();
  if (lua_pcall(lua, 2, 0, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    const char *em = lua_tostring(lua, -1);
    size_t len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                          em ? em : LSB_NIL_ERROR);
    if (len >= LSB_ERROR_SIZE) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(hsb->lsb, err);
    return 1;
  }
  end = lsb_get_time();
  lsb_update_running_stats(&hsb->stats.te, (double)(end - start));
  lsb_pcall_teardown(hsb->lsb);
  lua_gc(lua, LUA_GCCOLLECT, 0);
  return 0;
}


const char* lsb_heka_get_error(lsb_heka_sandbox *hsb)
{
  return hsb ? lsb_get_error(hsb->lsb) : "";
}


const char* lsb_heka_get_lua_file(lsb_heka_sandbox *hsb)
{
  return hsb ? lsb_get_lua_file(hsb->lsb) : NULL;
}


lsb_heka_stats lsb_heka_get_stats(lsb_heka_sandbox *hsb)
{
  if (!hsb) return (struct lsb_heka_stats){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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


lsb_state lsb_heka_get_state(lsb_heka_sandbox *hsb)
{
  if (!hsb) return LSB_UNKNOWN;
  return lsb_get_state(hsb->lsb);
}


const lsb_heka_message* lsb_heka_get_message(lsb_heka_sandbox *hsb)
{
  if (!hsb) return NULL;
  return hsb->msg;
}


char lsb_heka_get_type(lsb_heka_sandbox *hsb)
{
  if (!hsb) return '\0';
  return hsb->type;
}
