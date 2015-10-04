/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox unit tests @file */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "luasandbox.h"
#include "luasandbox/lauxlib.h"
#include "luasandbox/lua.h"
#include "luasandbox_output.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#define PRIuSIZE "Iu"
#else
#define PRIuSIZE "zu"
#endif

#define mu_assert(cond, ...)                                                   \
do {                                                                           \
  if (!(cond)) {                                                               \
    int cnt = snprintf(mu_err, MU_ERR_LEN, "line: %d (%s) ", __LINE__, #cond); \
    if (cnt > 0 && cnt < MU_ERR_LEN) {                                         \
      cnt = snprintf(mu_err+cnt, MU_ERR_LEN-cnt, __VA_ARGS__);                 \
      if (cnt > 0 && cnt < MU_ERR_LEN) {                                       \
        return mu_err;                                                         \
      }                                                                        \
    }                                                                          \
    mu_err[MU_ERR_LEN - 1] = 0;                                                \
    return mu_err;                                                             \
  }                                                                            \
} while (0)

#define mu_run_test(test)                                                      \
do {                                                                           \
  char *message = test();                                                      \
  mu_tests_run++;                                                              \
  if (message)                                                                 \
    return message;                                                            \
} while (0)

#define MU_ERR_LEN 1024
int mu_tests_run = 0;
char mu_err[MU_ERR_LEN] = { 0 };
char* e = NULL;
const char* written_data = NULL;
size_t written_data_len = 0;


int file_exists(const char* fn)
{
  FILE* fh;
  fh = fopen(fn, "r");
  if (fh) {
    fclose(fh);
    return 1;
  }
  return 0;
}


char* read_file(const char* fn)
{
  char* str = NULL;
  FILE* fh;
  fh = fopen(fn, "rb");
  mu_assert(fh != NULL, "fopen() failed");

  int result = fseek(fh, 0, SEEK_END);
  mu_assert(result == 0, "fseek(SEEK_END) failed %d", result);

  long pos = ftell(fh);
  mu_assert(pos != -1, "ftell() failed %s", strerror(errno));
  rewind(fh);

  str = malloc(pos + 1);
  mu_assert(str != NULL, "malloc failed");

  size_t b = fread(str, 1, pos, fh);
  mu_assert((long)b == pos, "fread() failed");
  str[pos] = 0;

  fclose(fh);
  return str;
}


int process(lua_sandbox* lsb, double ts)
{
  static const char* func_name = "process";
  lua_State* lua = lsb_get_lua(lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(lsb, func_name)) return 1;

  lua_pushnumber(lua, ts);
  if (lua_pcall(lua, 1, 2, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    int len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                       lua_tostring(lua, -1));
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  if (!lua_isnumber(lua, 1)) {
    char err[LSB_ERROR_SIZE];
    int len = snprintf(err, LSB_ERROR_SIZE,
                       "%s() must return a numeric error code", func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  int status = (int)lua_tointeger(lua, 1);
  switch (lua_type(lua, 2)) {
  case LUA_TNIL:
    lsb_set_error(lsb, NULL);
    break;
  case LUA_TSTRING:
    lsb_set_error(lsb, lua_tostring(lua, 2));
    break;
  default:
    {
      char err[LSB_ERROR_SIZE];
      int len = snprintf(err, LSB_ERROR_SIZE,
                         "%s() must return a nil or string error message",
                         func_name);
      if (len >= LSB_ERROR_SIZE || len < 0) {
        err[LSB_ERROR_SIZE - 1] = 0;
      }
      lsb_terminate(lsb, err);
      return 1;
    }
    break;
  }
  lua_pop(lua, 2);

  lsb_pcall_teardown(lsb);

  return status;
}


int report(lua_sandbox* lsb, double tc)
{
  static const char* func_name = "report";
  lua_State* lua = lsb_get_lua(lsb);
  if (!lua) return 1;

  if (lsb_pcall_setup(lsb, func_name)) return 1;

  lua_pushnumber(lua, tc);
  if (lua_pcall(lua, 1, 0, 0) != 0) {
    char err[LSB_ERROR_SIZE];
    int len = snprintf(err, LSB_ERROR_SIZE, "%s() %s", func_name,
                       lua_tostring(lua, -1));
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  lsb_pcall_teardown(lsb);
  lua_gc(lua, LUA_GCCOLLECT, 0);

  return 0;
}


int write_output(lua_State* lua)
{
  void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    luaL_error(lua, "write_output() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  int n = lua_gettop(lua);
  lsb_output(lsb, 1, n, 1);
  written_data = lsb_get_output(lsb, &written_data_len);
  return 0;
}


int write_message(lua_State* lua)
{
  void* luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    luaL_error(lua, "write_message() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  if (lsb_output_protobuf(lsb, 1, 0) != 0) {
    luaL_error(lua, "write_message() could not encode protobuf - %s",
               lsb_get_error(lsb));
  }
  written_data = lsb_get_output(lsb, &written_data_len);
  return 0;
}


static char* test_create_error()
{
  lua_sandbox* sb = lsb_create(NULL, NULL, "modules", 0, 0, 0);
  mu_assert(!sb, "lsb_create() null lua_file");

  return NULL;
}


static char* test_init_error()
{
  // null sandbox
  int result = lsb_init(NULL, NULL);
  mu_assert(result == 0, "lsb_init() null sandbox ptr");

  // load error
  lua_sandbox* sb = lsb_create(NULL, "lua/simple1.lua", "modules", 0,
                               0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_TERMINATED, "lsb_get_state() received: %d", s);
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // out of memory
  sb = lsb_create(NULL, "lua/simple.lua", "modules", 6000, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s\n", e);

  sb = lsb_create(NULL, "lua/lpeg_date_time.lua", NULL, 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  // disabled external modules
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  const char* expected = "lua/lpeg_date_time.lua:7: module 'date_time' not found:";
  mu_assert(strcmp(lsb_get_error(sb), expected) == 0,
            "lsb_get_error() received: %s", lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_destroy_error()
{
  const char* expected = "lsb_preserve_global_data could not open: "
    "invaliddir/simple.preserve";
  e = lsb_destroy(NULL, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, "invaliddir/simple.preserve");
  mu_assert(e, "lsb_destroy() received NULL");
  mu_assert(strcmp(e, expected) == 0,
            "lsb_destroy() received: %s", e);
  free(e);
  e = NULL;

  return NULL;
}


static char* test_usage_error()
{
  size_t u = lsb_usage(NULL, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u == 0, "NULL sandbox memory usage received: %" PRIuSIZE, u);

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  u = lsb_usage(NULL, LSB_UT_MAX + 1, LSB_US_CURRENT);
  mu_assert(u == 0, "Invalid usage type received: %" PRIuSIZE, u);

  u = lsb_usage(NULL, LSB_UT_MEMORY, LSB_US_MAX + 1);
  mu_assert(u == 0, "Invalid usage stat received: %" PRIuSIZE, u);

  mu_assert(sb, "lsb_create() received: NULL");
  lsb_terminate(sb, "forced termination");
  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u == 0, "Terminated memory usage received: %" PRIuSIZE, u);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_misc()
{
  lsb_state s = lsb_get_state(NULL);
  mu_assert(s == LSB_UNKNOWN, "lsb_get_state() received: %d", s);

  const char* le = lsb_get_error(NULL);
  mu_assert(strlen(le) == 0, "lsb_get_error() received: %s", le);

  return NULL;
}


static char* test_simple()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "modules",
                               65765, 1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  size_t u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u > 0, "Current memory usage received: %" PRIuSIZE, u);
  printf("cur_mem %" PRIuSIZE "\n", u);

  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_MAXIMUM);
  mu_assert(u > 0, "Maximum memory usage received: %" PRIuSIZE, u);
  printf("max_mem %" PRIuSIZE "\n", u);

  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_LIMIT);
  mu_assert(u == 65765, "Memory limit received: %" PRIuSIZE, u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_CURRENT);
  mu_assert(u == 7, "Current instructions received: %" PRIuSIZE, u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_MAXIMUM);
  mu_assert(u == 7, "Maximum instructions received: %" PRIuSIZE, u);
  printf("max_ins %" PRIuSIZE "\n", u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_LIMIT);
  mu_assert(u == 1000, "Instruction limit received: %" PRIuSIZE, u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_CURRENT);
  mu_assert(u == 0, "Current output received: %" PRIuSIZE, u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_MAXIMUM);
  mu_assert(u == 0, "Maximum output received: %" PRIuSIZE, u);
  printf("max_out %" PRIuSIZE "\n", u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_LIMIT);
  mu_assert(u == 1024, "Output limit received: %" PRIuSIZE, u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_LIMIT);
  mu_assert(u == 1024, "Output limit received: %" PRIuSIZE, u);

  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_RUNNING, "lsb_get_state() received: %d", s);

  e = lsb_destroy(sb, "simple.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_simple_error()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 1);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(strcmp("ok", lsb_get_error(sb)) == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(strcmp("", lsb_get_error(sb)) == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 2);
  mu_assert(result == 1, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output()
{
  const char* outputs[] = {
    "1.2 string nil true false"
    , ""
    , "\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x12\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x24\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x3a\x18\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x00\x00\x00\x00\x00\x00\x10\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x19\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x39\x00\x00\x00\x00\x00\x00\x14\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x2b\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\x18\x40\x00\x00\x00\x00\x00\x00\x1c\x40\x00\x00\x00\x00\x00\x00\x20\x40"
#ifdef LUA_JIT
    , "\x10\x80\x94\xeb\xdc\x03\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
#else
    , "\x10\x80\x94\xeb\xdc\x03\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
#endif
    , "\x10\x80\x94\xeb\xdc\x03\x52\x8d\x01\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x82\x01\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39"
    , "\x10\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01\x28\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01\x40\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x14\x0a\x05\x63\x6f\x75\x6e\x74\x10\x02\x1a\x07\x69\x6e\x74\x65\x67\x65\x72\x30\x01"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x16\x0a\x05\x63\x6f\x75\x6e\x74\x10\x02\x1a\x07\x69\x6e\x74\x65\x67\x65\x72\x32\x02\x01\x02"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x14\x0a\x05\x63\x6f\x75\x6e\x74\x10\x02\x1a\x07\x69\x6e\x74\x65\x67\x65\x72\x30\x01"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x14\x0a\x05\x63\x6f\x75\x6e\x74\x10\x02\x1a\x07\x69\x6e\x74\x65\x67\x65\x72\x30\x01\x52\x14\x0a\x05\x63\x6f\x75\x6e\x74\x10\x02\x1a\x07\x69\x6e\x74\x65\x67\x65\x72\x30\x02"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x1a\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x1a\x06\x64\x6f\x75\x62\x6c\x65\x39\x00\x00\x00\x00\x00\x00\xf0\x3f"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x23\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x1a\x06\x64\x6f\x75\x62\x6c\x65\x3a\x10\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\xf0\x3f"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x17\x0a\x05\x6e\x61\x6d\x65\x73\x10\x01\x1a\x04\x6c\x69\x73\x74\x2a\x02\x73\x31\x2a\x02\x73\x32"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x15\x0a\x05\x6e\x61\x6d\x65\x73\x1a\x04\x6c\x69\x73\x74\x22\x02\x73\x31\x22\x02\x73\x32"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x11\x0a\x05\x6e\x61\x6d\x65\x73\x10\x01\x2a\x02\x73\x31\x2a\x02\x73\x32"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x9f\x60\x0a\x03\x68\x6c\x6c\x10\x01\x1a\x03\x68\x6c\x6c\x2a\x90\x60\x48\x59\x4c\x4c"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x93\x01\x0a\x02\x63\x62\x10\x01\x1a\x04\x63\x62\x75\x66\x2a\x84\x01{\"time\":0,\"rows\":2,\"columns\":1,\"seconds_per_row\":60,\"column_info\":[{\"name\":\"Column_1\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\nnan\n"
    , NULL
  };

  enum {output_size = 63 * 1024};

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "modules", 0, 0
                               , output_size);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  lsb_add_function(sb, &write_message, "write_message");

  for (int x = 0; outputs[x]; ++x) {
    result = process(sb, x);
    mu_assert(!result, "process() test: %d failed: %d %s", x, result,
              lsb_get_error(sb));
    if (outputs[x][0]) {
      if (outputs[x][0] == 0x10) {
        size_t header = 18;
        if (x == 19) {
          mu_assert(written_data_len == 12346, "test: %d received: %" PRIuSIZE, x, written_data_len);
          written_data_len = header + 28; // just compare the protobuf before the hyperloglog
        }
        if (memcmp(outputs[x], written_data + header, written_data_len - header)
            != 0) {
          char hex_data[output_size * 3 + 1];
          size_t z = 0;
          for (size_t y = header; y < written_data_len; ++y, z += 3) {
            snprintf(hex_data + z, output_size - z, "%02x ",
                     (unsigned char)written_data[y]);
          }
          hex_data[z] = 0;
          mu_assert(0, "test: %d received: %s", x, hex_data);
        }
      } else {
        mu_assert(strcmp(outputs[x], written_data) == 0,
                  "test: %d received: %s", x, written_data);
      }
    }
  }

  e = lsb_destroy(sb, "circular_buffer.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output_errors()
{
  const char* tests[] =
  {
    "process() lua/output_errors.lua:10: bad argument #1 to 'output' (unsupported type)"
    , "process() lua/output_errors.lua:16: output_limit exceeded"
    , "process() lua/output_errors.lua:19: write_message() could not encode protobuf - array has mixed types"
    , "process() lua/output_errors.lua:22: write_message() could not encode protobuf - unsupported type: nil"
    , "process() lua/output_errors.lua:24: bad argument #1 to 'write_output' (unknown userdata type)"
    , "process() lua/output_errors.lua:27: output_limit exceeded"
    , "process() lua/output_errors.lua:30: write_message() could not encode protobuf - unsupported array type: table"
    , "process() lua/output_errors.lua:36: strbuf output_limit exceeded"
    , "process() lua/output_errors.lua:38: write_message() could not encode protobuf - takes a table argument"
    , "process() lua/output_errors.lua:41: write_message() could not encode protobuf - invalid string value_type: 2"
    , "process() lua/output_errors.lua:44: write_message() could not encode protobuf - invalid string value_type: 3"
    , "process() lua/output_errors.lua:47: write_message() could not encode protobuf - invalid string value_type: 4"
    , "process() lua/output_errors.lua:50: write_message() could not encode protobuf - invalid numeric value_type: 0"
    , "process() lua/output_errors.lua:53: write_message() could not encode protobuf - invalid numeric value_type: 1"
    , "process() lua/output_errors.lua:56: write_message() could not encode protobuf - invalid numeric value_type: 4"
    , "process() lua/output_errors.lua:59: write_message() could not encode protobuf - invalid boolean value_type: 0"
    , "process() lua/output_errors.lua:62: write_message() could not encode protobuf - invalid boolean value_type: 1"
    , "process() lua/output_errors.lua:65: write_message() could not encode protobuf - invalid boolean value_type: 2"
    , "process() lua/output_errors.lua:68: write_message() could not encode protobuf - invalid boolean value_type: 3"
    , "process() lua/output_errors.lua:72: write_message() could not encode protobuf - user data object does not implement lsb_output"
    , "process() lua/output_errors.lua:76: write_message() could not encode protobuf - user data object does not implement lsb_output"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/output_errors.lua", "modules",
                                 0, 0, 128);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    lsb_add_function(sb, &write_output, "write_output");
    lsb_add_function(sb, &write_message, "write_message");

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char* le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_cbuf_errors()
{
  const char* tests[] =
  {
    "process() not enough memory"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_errors.lua",
                                 "modules", 32767, 0, 0);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char* le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_cbuf_core()
{
  lua_sandbox* sb = lsb_create(NULL, "../../ep_base/Source/lua_circular_buffer/test.lua",
                               "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, "");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cbuf()
{
  const char* outputs[] = {
    "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\nnan\tnan\tnan\n"
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
    , "{\"time\":2,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n3\t1\t3\nnan\tnan\tnan\n1\t1\t1\n"
    , "{\"time\":8,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\n1\t1\t1\n"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING, "error %s",
            lsb_get_error(sb));
  mu_assert(strcmp(outputs[0], written_data) == 0, "received: %s",
            written_data);

  process(sb, 0);
  process(sb, 1e9);
  process(sb, 1e9);
  process(sb, 2e9);
  process(sb, 2e9);
  process(sb, 2e9);
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[1], written_data) == 0, "received: %s",
            written_data);

  process(sb, 4e9);
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[2], written_data) == 0, "received: %s",
            written_data);

  process(sb, 10e9);
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[3], written_data) == 0, "received: %s",
            written_data);

  e = lsb_destroy(sb, "circular_buffer.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cbuf_delta()
{
  const char* outputs[] = {
    "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
#ifdef LUA_JIT
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n0\t1\t1\t1\n1\t2\t1\t2\n2\t3\t1\t3\n"
#else
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t2\t1\t2\n2\t3\t1\t3\n0\t1\t1\t1\n"
#endif
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
    , ""
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n2\tnan\tnan\tnan\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t2\t5\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t3\t8\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\tnan\t9\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\tnan\t10\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t1\tnan\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t1\tnan\n"
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t-2\t0\n"
    , ""
    , "{\"time\":0,\"rows\":2,\"columns\":2,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Sum_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Min\",\"unit\":\"count\",\"aggregation\":\"min\"}]}\n0\t3\t4\n"
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n0\tinf\t-inf\tinf\n"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_delta.lua",
                               "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  process(sb, 0);
  process(sb, 1e9);
  process(sb, 1e9);
  process(sb, 2e9);
  process(sb, 2e9);
  process(sb, 2e9);
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[0], written_data) == 0, "received: %s",
            written_data);

  result = report(sb, 1);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[1], written_data) == 0, "received: %s",
            written_data);

  for (int i = 2; outputs[i] != NULL; ++i) {
    result = report(sb, i - 2);
    mu_assert(result == 0, "report() received: %d error: %s", result,
              lsb_get_error(sb));
    mu_assert(strcmp(outputs[i], written_data) == 0, "test: %d received: %s", i,
              written_data);
  }

  e = lsb_destroy(sb, "circular_buffer_delta.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cjson()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/cjson.lua", "modules", 0, 0, 64);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cjson_unlimited()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/cjson_unlimited.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(written_data_len == 103001, "received %d bytes", (int)written_data_len);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_errors()
{
  const char* tests[] = {
#ifdef _WIN32
    "process() lua/errors.lua:9: module 'unknown' not found:\n\tno file 'lua\\unknown.lua'\n\tno file 'lua\\unknown.dll'"
#else
    "process() lua/errors.lua:9: module 'unknown' not found:\n\tno file 'lua/unknown.lua'\n\tno file 'lua/unknown.so'"
#endif
    , "process() lua/errors.lua:11: bad argument #0 to 'output' (must have at least one argument)"
    , "process() not enough memory"
    , "process() instruction_limit exceeded"
    , "process() lua/errors.lua:20: attempt to perform arithmetic on global 'x' (a nil value)"
    , "process() must return a numeric error code"
    , "process() must return a numeric error code"
    , "process() lua/errors.lua:27: output_limit exceeded"
#ifdef _WIN32
    , "process() lua\\bad_module.lua:1: attempt to perform arithmetic on global 'nilvalue' (a nil value)"
#else
    , "process() lua/bad_module.lua:1: attempt to perform arithmetic on global 'nilvalue' (a nil value)"
#endif
    , "process() invalid module name '../invalid'"
    , "process() lua/errors.lua:34: module 'pathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpathoverflowpa"
#ifdef _WIN32
    , "process() lua/errors.lua:36: module 'foo.bar' not found:\n\tno file 'lua\\foo\\bar.lua'\n\tno file 'lua\\foo\\bar.dll'\n\tno file 'lua\\foo.dll'"
#else
    , "process() lua/errors.lua:36: module 'foo.bar' not found:\n\tno file 'lua/foo/bar.lua'\n\tno file 'lua/foo/bar.so'\n\tno file 'lua/foo.so'"
#endif
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/errors.lua", "lua", 32767, 1000,
                                 128);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char* le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_lpeg()
{
  const char* tests[] = {
    "lua/lpeg.lua"
    , "lua/lpeg_clf.lua"
    , "lua/lpeg_cbufd.lua"
    , "lua/lpeg_date_time.lua"
    , "lua/lpeg_ip_address.lua"
    , "lua/lpeg_mysql.lua"
    , "lua/lpeg_postfix.lua"
    , "lua/lpeg_syslog.lua"
    , "lua/lpeg_syslog_message.lua"
    , "lua/lpeg_sfl4j.lua"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, tests[i], "modules", 0, 0, 0);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, 0);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));

    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }


  return NULL;
}


static char* test_util()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/util_test.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_serialize()
{
  const char* output_file = "serialize.preserve";
  lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "modules",
                               64000, 1000, 64000);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

#ifdef LUA_JIT
  char* expected = read_file("output/serialize.data");
#else
  char* expected = read_file("output/serialize.lua51.data");
#endif
  char* actual = read_file(output_file);
  mu_assert(strcmp(expected, actual) == 0, "serialization mismatch");
  free(expected);
  free(actual);

  return NULL;
}


static char* test_restore()
{
  const char* output_file = "restore.preserve";

  lua_sandbox* sb = lsb_create(NULL, "lua/restore.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("101", written_data) == 0, "test: initial load received: %s",
            written_data);
  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/restore.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("102", written_data) == 0, "test: reload received: %s",
            written_data);
  result = report(sb, 2); // change the preservation version
  mu_assert(result == 0, "report() received: %d", result);
  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data with a version change
  sb = lsb_create(NULL, "lua/restore.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("101", written_data) == 0,
            "test: reload with version change received: %s", written_data);
  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_serialize_failure()
{
  const char* output_file = "serialize_failure.preserve";
  const char* expected = "serialize_data cannot preserve type 'function'";

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/serialize_failure.lua", "modules",
                               32767, 1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, output_file);
  mu_assert(e, "lsb_destroy() received: no error");
  mu_assert(strcmp(e, expected) == 0, "lsb_destroy() received: %s", e);
  free(e);
  e = NULL;
  mu_assert(file_exists(output_file) == 0, "output file was not cleaned up");

  return NULL;
}


static char* test_bloom_filter_core()
{
  lua_sandbox* sb = lsb_create(NULL, "../../ep_base/Source/lua_bloom_filter/test.lua",
                               "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, "");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_bloom_filter()
{
  const char* output_file = "bloom_filter.preserve";
  const char* tests[] = {
    "1"
    , "2"
    , "3"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/bloom_filter.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  int i = 0;
  for (; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
    result = report(sb, 0);
    mu_assert(result == 0, "report() received: %d", result);
    mu_assert(strcmp(tests[i], written_data) == 0, "test: %d received: %s", i,
              written_data);
  }

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(tests[i - 1], written_data) == 0, "test: %d received: %s", i,
            written_data); // count should remain the same

  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/bloom_filter.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  report(sb, 0);
  mu_assert(strcmp("3", written_data) == 0, "test: count received: %s",
            written_data);

  for (int i = 0; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
  }
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(tests[i - 1], written_data) == 0, "test: %d received: %s", i,
            written_data); // count should remain the same

  // test clear
  report(sb, 99);
  process(sb, 0);
  report(sb, 0);
  mu_assert(strcmp("1", written_data) == 0, "test: clear received: %s",
            written_data);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_hyperloglog_core()
{
  lua_sandbox* sb = lsb_create(NULL, "../../ep_base/Source/lua_hyperloglog/test.lua",
                               "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, "");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_hyperloglog()
{
  const char* output_file = "hyperloglog.preserve";

  lua_sandbox* sb = lsb_create(NULL, "lua/hyperloglog.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");


  for (int i = 0; i < 100000; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
  }

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp("100070", written_data) == 0, "test: initial received: %s",
            written_data); // count should remain the same

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp("100070", written_data) == 0, "test: cache received: %s",
            written_data); // count should remain the same

  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/hyperloglog.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp("100070", written_data) == 0, "test: reload received: %s",
            written_data); // count should remain the same

  for (int i = 0; i < 100000; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
  }
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp("100070", written_data) == 0,
            "test: data replay received: %s", written_data);
  // count should remain the same

  // test clear
  report(sb, 99);
  report(sb, 0);
  mu_assert(strcmp("0", written_data) == 0, "test: clear received: %s",
            written_data);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_struct()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/struct.lua", "modules",
                               65765, 1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result, lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_sandbox_config()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/sandbox_config.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_decode_message()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/decode_message.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_add_function(sb, &lsb_decode_protobuf, "decode_message");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cuckoo_filter()
{
  const char* output_file = "cuckoo_filter.preserve";
  const char* tests[] = {
    "1"
    , "2"
    , "3"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/cuckoo_filter.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  int i = 0;
  for (; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
    result = report(sb, 0);
    mu_assert(result == 0, "report() received: %d", result);
    mu_assert(strcmp(tests[i], written_data) == 0, "test: %d received: %s", i,
              written_data);
  }

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(tests[i - 1], written_data) == 0, "test: %d received: %s", i,
            written_data); // count should remain the same

  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/cuckoo_filter.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  report(sb, 0);
  mu_assert(strcmp("3", written_data) == 0, "test: count received: %s",
            written_data);

  for (int i = 0; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));
  }
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(tests[i - 1], written_data) == 0, "test: %d received: %s", i,
            written_data); // count should remain the same

  // test deletion
  report(sb, 98);
  mu_assert(strcmp("2", written_data) == 0, "test: delete received: %s",
            written_data);
  // tst clear
  report(sb, 99);
  mu_assert(strcmp("0", written_data) == 0, "test: clear received: %s",
            written_data);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_sax()
{
  const char* output_file = "sax.preserve";
  const char* word = "CDC CDC ABEGH ABEGH";

  lua_sandbox* sb = lsb_create(NULL, "lua/sax.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d %s", result, lsb_get_error(sb));
  mu_assert(strcmp("### CDC ##### #####", written_data) == 0, "received: %s", written_data);

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(word, written_data) == 0, "received: %s", written_data);
  e = lsb_destroy(sb, output_file);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/sax.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  report(sb, 0);
  mu_assert(strcmp(word, written_data) == 0, "received: %s", written_data);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* benchmark_counter()
{
  int iter = 10000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/counter.lua", "modules", 32000,
                               10, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 0);
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_counter() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_serialize()
{
  int iter = 1000;
  const char* output_file = "serialize.preserve";

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "modules",
                                 64000, 1000, 1024);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    e = lsb_destroy(sb, output_file);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_serialize() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_deserialize()
{
  int iter = 1000;

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "modules",
                                 0, 0, 0);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, "output/serialize.data");
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_deserialize() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_lpeg_decoder()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/decoder.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  lsb_add_function(sb, &write_message, "write_message");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 0), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lpeg_decoder() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_lua_types_output()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "modules",
                               100000, 1000, 1024 * 63);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 0), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lua_types_output() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_cbuf_output()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 1), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cbuf_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_message_output()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "modules", 0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_message, "write_message");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 7), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_message_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_cbuf_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/circular_buffer_add.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  double ts = 0;
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 0), "%s", lsb_get_error(sb));
    ts += 1e9;
  }
  t = clock() - t;
  mu_assert(lsb_get_state(sb) == LSB_RUNNING, "benchmark_cbuf_add() failed %s",
            lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cbuf_add() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_bloom_filter_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/bloom_filter_benchmark.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, x), "%s", lsb_get_error(sb)); // test add speed
  }
  t = clock() - t;
  report(sb, 0);
  mu_assert(strcmp("999970", written_data) == 0, "received: %s", written_data);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING,
            "benchmark_bloom_filter_add() failed %s", lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_bloom_filter_add() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_hyperloglog_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/hyperloglog.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, x), "%s", lsb_get_error(sb)); // test add speed
  }
  t = clock() - t;
  report(sb, 0);
  mu_assert(strcmp("1006268", written_data) == 0, "received: %s", written_data);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING,
            "benchmark_hyperloglog_add() failed %s", lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_hyperloglog_add() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_decode_message()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/decode_message_benchmark.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  lsb_add_function(sb, &lsb_decode_protobuf, "decode_message");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, x), "%s", lsb_get_error(sb)); // test add speed
  }
  t = clock() - t;
  report(sb, 0);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING,
            "benchmark_decode_message() failed %s", lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_decode_message() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_cuckoo_filter_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/cuckoo_filter_benchmark.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, x), "%s", lsb_get_error(sb)); // test add speed
  }
  t = clock() - t;
  report(sb, 0);
  mu_assert(strcmp("999985", written_data) == 0, "received: %s", written_data);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING,
            "benchmark_cuckoo_filter_add() failed %s", lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cuckoo_filter_add() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_sax_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/sax_benchmark.lua", "modules",
                               0, 0, 0);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, x), "%s", lsb_get_error(sb)); // test add speed
  }
  t = clock() - t;
  report(sb, 0);
  mu_assert(strcmp("ABEGH", written_data) == 0, "received: %s", written_data);
  mu_assert(lsb_get_state(sb) == LSB_RUNNING,
            "benchmark_sax_add() failed %s", lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_sax_add() %g seconds\n", ((float)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}



static char* all_tests()
{
  mu_run_test(test_create_error);
  mu_run_test(test_init_error);
  mu_run_test(test_destroy_error);
  mu_run_test(test_usage_error);
  mu_run_test(test_misc);
  mu_run_test(test_simple);
  mu_run_test(test_simple_error);
  mu_run_test(test_output);
  mu_run_test(test_output_errors);
  mu_run_test(test_cbuf_errors);
  mu_run_test(test_cbuf_core);
  mu_run_test(test_cbuf);
  mu_run_test(test_cbuf_delta);
  mu_run_test(test_cjson);
  mu_run_test(test_cjson_unlimited);
  mu_run_test(test_errors);
  mu_run_test(test_lpeg);
  mu_run_test(test_util);
  mu_run_test(test_serialize);
  mu_run_test(test_restore);
  mu_run_test(test_serialize_failure);
  mu_run_test(test_bloom_filter_core);
  mu_run_test(test_bloom_filter);
  mu_run_test(test_hyperloglog_core);
  mu_run_test(test_hyperloglog);
  mu_run_test(test_struct);
  mu_run_test(test_sandbox_config);
  mu_run_test(test_decode_message);
  mu_run_test(test_cuckoo_filter);
  mu_run_test(test_sax);

  mu_run_test(benchmark_counter);
  mu_run_test(benchmark_serialize);
  mu_run_test(benchmark_deserialize);
  mu_run_test(benchmark_lpeg_decoder);
  mu_run_test(benchmark_lua_types_output);
  mu_run_test(benchmark_cbuf_output);
  mu_run_test(benchmark_message_output);
  mu_run_test(benchmark_cbuf_add);
  mu_run_test(benchmark_bloom_filter_add);
  mu_run_test(benchmark_hyperloglog_add);
  mu_run_test(benchmark_decode_message);
  mu_run_test(benchmark_cuckoo_filter_add);
  mu_run_test(benchmark_sax_add);

  return NULL;
}


int main()
{
  char* result = all_tests();
  if (result) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", mu_tests_run);
  free(e);

  return result != 0;
}
