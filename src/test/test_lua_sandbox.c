/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Lua sandbox unit tests @file

#include "lua_sandbox.h"

#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define snprintf _snprintf
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
  if (lua_pcall(lua, 1, 1, 0) != 0) {
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
                       "%s() must return a single numeric value", func_name);
    if (len >= LSB_ERROR_SIZE || len < 0) {
      err[LSB_ERROR_SIZE - 1] = 0;
    }
    lsb_terminate(lsb, err);
    return 1;
  }

  int status = (int)lua_tointeger(lua, 1);
  lua_pop(lua, 1);

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
    luaL_error(lua, "write() invalid lightuserdata");
  }
  lua_sandbox* lsb = (lua_sandbox*)luserdata;

  switch (lua_gettop(lua)) {
  case 0:
    break;
  case 1:
    switch (lua_type(lua, 1)) {
    case LUA_TTABLE:
      if (lsb_output_protobuf(lsb, 1, 0) != 0) {
        luaL_error(lua, "write() cound not encode protobuf - %s",
                   lsb_get_error(lsb));
      }
      break;
    case LUA_TUSERDATA:
      lsb_output_userdata(lsb, 1, 0);
      break;
    default:
      luaL_typerror(lua, 1, "table, or circular_buffer");
      break;
    }
    break;
  default:
    luaL_error(lua, "write() takes a maximum of 1 argument");
    break;
  }
  written_data = lsb_get_output(lsb, &written_data_len);
  return 0;
}


static char* test_create_error()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", LSB_MEMORY + 1,
                               1000, 1024);
  mu_assert(!sb, "lsb_create() memory_limit");

  sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, LSB_INSTRUCTION + 1,
                  1024);
  mu_assert(!sb, "lsb_create() instruction_limit");

  sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, 1000, LSB_OUTPUT + 1);
  mu_assert(!sb, "lsb_create() output_limit");

  sb = lsb_create(NULL, NULL, "../../modules", 65765, 1000, 1024);
  mu_assert(!sb, "lsb_create() null lua_file");

  return NULL;
}


static char* test_init_error()
{
  // null sandbox
  int result = lsb_init(NULL, NULL);
  mu_assert(result == 0, "lsb_init() null sandbox ptr");

  // load error
  lua_sandbox* sb = lsb_create(NULL, "lua/simple1.lua", "../../modules", 65765, 1000,
                               1024);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_TERMINATED, "lsb_get_state() received: %d", s);
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // out of memory
  sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 6000, 1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s\n", e);

  sb = lsb_create(NULL, "lua/lpeg_grammar.lua", NULL, 100000, 1000,
                               8000);
  mu_assert(sb, "lsb_create() received: NULL");

  // disabled external modules
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  const char *expected = "lua/lpeg_grammar.lua:9: require_library() external modules are disabled";
  mu_assert(strcmp(lsb_get_error(sb), expected) == 0,
            "lsb_get_error() received: %s", lsb_get_error(sb));

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_destroy_error()
{
  const char* expected = "preserve_global_data could not open: "
    "invaliddir/simple.preserve";
  e = lsb_destroy(NULL, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, 1000,
                               1024);
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
  unsigned u = lsb_usage(NULL, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u == 0, "NULL sandbox memory usage received: %u", u);

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, 1000,
                               1024);
  mu_assert(sb, "lsb_create() received: NULL");

  u = lsb_usage(NULL, LSB_UT_MAX + 1, LSB_US_CURRENT);
  mu_assert(u == 0, "Invalid usage type received: %u", u);

  u = lsb_usage(NULL, LSB_UT_MEMORY, LSB_US_MAX + 1);
  mu_assert(u == 0, "Invalid usage stat received: %u", u);

  mu_assert(sb, "lsb_create() received: NULL");
  lsb_terminate(sb, "forced termination");
  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u == 0, "Terminated memory usage received: %u", u);

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
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, 1000,
                               1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  unsigned u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u > 0, "Current memory usage received: %u", u);
  printf("cur_mem %u\n", u);

  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_MAXIMUM);
  mu_assert(u > 0, "Maximum memory usage received: %u", u);
  printf("max_mem %u\n", u);

  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_LIMIT);
  mu_assert(u == 65765, "Memory limit received: %u", u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_CURRENT);
  mu_assert(u == 7, "Current instructions received: %u", u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_MAXIMUM);
  mu_assert(u == 7, "Maximum instructions received: %u", u);
  printf("max_ins %u\n", u);

  u = lsb_usage(sb, LSB_UT_INSTRUCTION, LSB_US_LIMIT);
  mu_assert(u == 1000, "Instruction limit received: %u", u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_CURRENT);
  mu_assert(u == 0, "Current output received: %u", u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_MAXIMUM);
  mu_assert(u == 0, "Maximum output received: %u", u);
  printf("max_out %u\n", u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_LIMIT);
  mu_assert(u == 1024, "Output limit received: %u", u);

  u = lsb_usage(sb, LSB_UT_OUTPUT, LSB_US_LIMIT);
  mu_assert(u == 1024, "Output limit received: %u", u);

  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_RUNNING, "lsb_get_state() received: %d", s);

  e = lsb_destroy(sb, "simple.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output()
{
  const char* outputs[] = {
    "{\"value\":1}\n1.2 string nil true false"
    , ""
#ifdef LUA_JIT
    , "{\"Timestamp\":0,\"Value\":0,\"StatisticValues\":[{\"SampleCount\":0,\"Sum\":0,\"Maximum\":0,\"Minimum\":0},{\"SampleCount\":0,\"Sum\":0,\"Maximum\":0,\"Minimum\":0}],\"Unit\":\"s\",\"MetricName\":\"example\",\"Dimensions\":[{\"Name\":\"d1\",\"Value\":\"v1\"},{\"Name\":\"d2\",\"Value\":\"v2\"}]}\n"
#else
    , "{\"StatisticValues\":[{\"Minimum\":0,\"SampleCount\":0,\"Sum\":0,\"Maximum\":0},{\"Minimum\":0,\"SampleCount\":0,\"Sum\":0,\"Maximum\":0}],\"Dimensions\":[{\"Name\":\"d1\",\"Value\":\"v1\"},{\"Name\":\"d2\",\"Value\":\"v2\"}],\"MetricName\":\"example\",\"Timestamp\":0,\"Value\":0,\"Unit\":\"s\"}\n"
#endif
    , "{\"a\":{\"y\":2,\"x\":1}}\n"
    , "[1,2,3]\n"
    , "{\"x\":1}\n"
    , "[1,2,3]\n"
    , "{}\n"
    , "{\"special\\tcharacters\":\"\\\"\\t\\r\\n\\b\\f\\\\\\/\"}\n"
    , "\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x12\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x25\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x39\x00\x00\x00\x00\x00\x00\x00\x40\x39\x00\x00\x00\x00\x00\x00\x08\x40\x39\x00\x00\x00\x00\x00\x00\x10\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x19\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x39\x00\x00\x00\x00\x00\x00\x14\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x2c\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x39\x00\x00\x00\x00\x00\x00\x18\x40\x39\x00\x00\x00\x00\x00\x00\x1c\x40\x39\x00\x00\x00\x00\x00\x00\x20\x40"
#ifdef LUA_JIT
    , "\x10\x80\x94\xeb\xdc\x03\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x0f\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x40\x01\x40\x00\x40\x00\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x2d\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x39\x00\x00\x00\x00\x00\x00\x00\x40\x39\x00\x00\x00\x00\x00\x00\x08\x40\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
#else
    , "\x10\x80\x94\xeb\xdc\x03\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2d\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x39\x00\x00\x00\x00\x00\x00\x00\x40\x39\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0f\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x40\x01\x40\x00\x40\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
#endif
    , "\x10\x80\x94\xeb\xdc\x03\x52\x8d\x01\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x82\x01\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000,
                               1000, 63 * 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  for (int x = 0; outputs[x]; ++x) {
    result = process(sb, x);
    mu_assert(!result, "process() test: %d failed: %d %s", x, result, lsb_get_error(sb));
    if (outputs[x][0]) {
      if (outputs[x][0] == 0x10) {
        size_t header = 18;
        if (memcmp(outputs[x], written_data + header, written_data_len - header) != 0) {
          char hex_data[LSB_OUTPUT + 1];
          size_t z = 0;
          for (size_t y = header; y < written_data_len; ++y, z += 3) {
            snprintf(hex_data + z, LSB_OUTPUT - z, "%02x ", (unsigned char)written_data[y]);
          }
          hex_data[z] = 0;
          mu_assert(0, "test: %d received: %s", x, hex_data);
        }
      } else {
        mu_assert(strcmp(outputs[x], written_data) == 0, "test: %d received: %s",
                  x, written_data);
      }
    }
  }

  e = lsb_destroy(sb, "circular_buffer.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cbuf_errors()
{
  const char* tests[] =
  {
    "process() lua/circular_buffer_errors.lua:9: bad argument #0 to 'new' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:11: bad argument #1 to 'new' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:13: bad argument #1 to 'new' (rows must be > 1)"
    , "process() lua/circular_buffer_errors.lua:15: bad argument #2 to 'new' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:17: bad argument #2 to 'new' (columns must be > 0)"
    , "process() lua/circular_buffer_errors.lua:19: bad argument #3 to 'new' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:21: bad argument #3 to 'new' (seconds_per_row is out of range)"
    , "process() lua/circular_buffer_errors.lua:23: bad argument #3 to 'new' (seconds_per_row is out of range)"
    , "process() not enough memory"
    , "process() lua/circular_buffer_errors.lua:28: bad argument #2 to 'set' (column out of range)"
    , "process() lua/circular_buffer_errors.lua:31: bad argument #2 to 'set' (column out of range)"
    , "process() lua/circular_buffer_errors.lua:34: bad argument #2 to 'set' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:37: bad argument #1 to 'set' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:41: bad argument #1 to 'get' (lsb.circular_buffer expected, got number)"
    , "process() lua/circular_buffer_errors.lua:44: bad argument #3 to 'set' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:47: bad argument #-1 to 'set' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:50: bad argument #-1 to 'add' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:53: bad argument #-1 to 'get' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:56: bad argument #-1 to 'compute' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:59: bad argument #1 to 'compute' (invalid option 'func')"
    , "process() lua/circular_buffer_errors.lua:62: bad argument #2 to 'compute' (column out of range)"
    , "process() lua/circular_buffer_errors.lua:65: bad argument #4 to 'compute' (end must be >= start)"
    , "process() lua/circular_buffer_errors.lua:68: bad argument #1 to 'format' (invalid option 'invalid')"
    , "process() lua/circular_buffer_errors.lua:71: bad argument #-1 to 'format' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:74: bad argument #-1 to 'format' (incorrect number of arguments)"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_errors.lua", "../../modules",
                                 32767, 1000, 128);
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


static char* test_cbuf()
{
  const char* outputs[] = {
    "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n0\t0\t0\n0\t0\t0\n0\t0\t0\n"
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
    , "{\"time\":2,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n3\t1\t3\n0\t0\t0\n1\t1\t1\n"
    , "{\"time\":8,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n0\t0\t0\n0\t0\t0\n1\t1\t1\n"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer.lua", "../../modules", 32767,
                               1000, 32767);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  result = report(sb, 0);
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

  result = report(sb, 1);
  mu_assert(result == 0, "report() received: %d", result);

  result = report(sb, 3);
  mu_assert(result == 0, "report() received: %d", result);

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
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_delta.lua", "../../modules",
                               32767, 1000, 32767);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

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

  result = report(sb, 0);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[2], written_data) == 0, "received: %s",
            written_data);

  result = report(sb, 1);
  mu_assert(result == 0, "report() received: %d", result);
  mu_assert(strcmp(outputs[3], written_data) == 0, "received: %s",
            written_data);

  e = lsb_destroy(sb, "circular_buffer_delta.preserve");
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cjson()
{

  lua_sandbox* sb = lsb_create(NULL, "lua/cjson.lua", "../../modules", 32767, 1000,
                               32767);
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


static char* test_errors()
{
  const char* tests[] = {
#ifdef _WIN32
    "process() lua/errors.lua:9: cannot open lua\\unknown.lua: No such file or directory"
#else
    "process() lua/errors.lua:9: cannot open lua/unknown.lua: No such file or directory"
#endif
    , "process() lua/errors.lua:11: output() must have at least one argument"
    , "process() not enough memory"
    , "process() instruction_limit exceeded"
    , "process() lua/errors.lua:20: attempt to perform arithmetic on global 'x' (a nil value)"
    , "process() must return a single numeric value"
    , "process() must return a single numeric value"
    , "process() lua/errors.lua:27: output_limit exceeded"
#ifdef _WIN32
    , "process() lua/errors.lua:30: lua\\bad_module.lua:1: attempt to perform arithmetic on global 'nilvalue' (a nil value)"
#else
    , "process() lua/errors.lua:30: lua/bad_module.lua:1: attempt to perform arithmetic on global 'nilvalue' (a nil value)"
#endif
    , "process() lua/errors.lua:32: invalid module name '../invalid'"
    , "process() lua/errors.lua:34: require_path exceeded 255"
    , "process() lua/errors.lua:37: package table is missing"
    , "process() lua/errors.lua:40: package.loaded table is missing"
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
  const char* expected = "[\"1\",\"string with spaces\","
    "\"quoted string, with comma and \\\"quoted\\\" text\"]\n";

  lua_sandbox* sb = lsb_create(NULL, "lua/lpeg_csv.lua", "../../modules", 100000, 1000,
                               8000);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp(expected, written_data) == 0, "received: %s", written_data);

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


#ifdef LUA_JIT
static char* test_lpeg_grammar()
{
  const char* tests[] = {
    "{\"offset_min\":0,\"offset_sign\":\"-\",\"offset_hour\":7,\"sec\":\"59\",\"min\":\"23\",\"day\":\"05\",\"sec_frac\":0.217,\"hour\":\"23\",\"month\":\"05\",\"year\":\"1999\"}\n"
    ,"7 6 5 4 4 3 3 2 1 0 0"
#ifdef _WIN32
    ,"9.25971839e+017"
    #else
    ,"9.25971839e+17"
#endif
    ,"{\"day\":\"12\",\"min\":\"20\",\"sec\":\"50\",\"sec_frac\":0.52,\"year\":\"1985\",\"month\":\"04\",\"hour\":\"23\"}\n"
    ,"{\"offset_min\":0,\"offset_sign\":\"-\",\"offset_hour\":8,\"sec\":\"57\",\"min\":\"39\",\"day\":\"19\",\"hour\":\"16\",\"month\":\"12\",\"year\":\"1996\"}\n"
    ,"{\"day\":\"31\",\"sec\":\"60\",\"min\":\"59\",\"year\":\"1990\",\"month\":\"12\",\"hour\":\"23\"}\n"
    ,"{\"offset_min\":0,\"offset_sign\":\"-\",\"offset_hour\":8,\"sec\":\"60\",\"min\":\"59\",\"day\":\"31\",\"hour\":\"15\",\"month\":\"12\",\"year\":\"1990\"}\n"
    ,"{\"offset_min\":20,\"offset_sign\":\"+\",\"offset_hour\":0,\"sec\":\"27\",\"min\":\"00\",\"day\":\"01\",\"sec_frac\":0.87,\"hour\":\"12\",\"month\":\"01\",\"year\":\"1937\"}\n"
    , NULL
    };

  lua_sandbox* sb = lsb_create(NULL, "lua/lpeg_grammar.lua", "../../modules", 100000, 1000,
                               8000);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  for (int i = 0; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result, lsb_get_error(sb));
    mu_assert(strcmp(tests[i], written_data) == 0, "test: %d received: %s", i, written_data);
  }

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}

#else

static char* test_lpeg_grammar()
{
  const char* tests[] = {
    "{\"offset_sign\":\"-\",\"offset_min\":0,\"hour\":\"23\",\"min\":\"23\",\"day\":\"05\",\"month\":\"05\",\"offset_hour\":7,\"sec\":\"59\",\"year\":\"1999\",\"sec_frac\":0.217}\n"
    ,"7 6 5 4 4 3 3 2 1 0 0"
#ifdef _WIN32
    ,"9.25971839e+017"
    #else
    ,"9.25971839e+17"
#endif
    ,"{\"min\":\"20\",\"year\":\"1985\",\"month\":\"04\",\"sec_frac\":0.52,\"sec\":\"50\",\"hour\":\"23\",\"day\":\"12\"}\n"
    ,"{\"offset_sign\":\"-\",\"offset_min\":0,\"hour\":\"16\",\"min\":\"39\",\"day\":\"19\",\"month\":\"12\",\"sec\":\"57\",\"year\":\"1996\",\"offset_hour\":8}\n"
    ,"{\"min\":\"59\",\"year\":\"1990\",\"month\":\"12\",\"sec\":\"60\",\"hour\":\"23\",\"day\":\"31\"}\n"
    ,"{\"offset_sign\":\"-\",\"offset_min\":0,\"hour\":\"15\",\"min\":\"59\",\"day\":\"31\",\"month\":\"12\",\"sec\":\"60\",\"year\":\"1990\",\"offset_hour\":8}\n"
    ,"{\"offset_sign\":\"+\",\"offset_min\":20,\"hour\":\"12\",\"min\":\"00\",\"day\":\"01\",\"month\":\"01\",\"offset_hour\":0,\"sec\":\"27\",\"year\":\"1937\",\"sec_frac\":0.87}\n"
    , NULL
    };

  lua_sandbox* sb = lsb_create(NULL, "lua/lpeg_grammar.lua", "../../modules", 100000, 1000, 8000);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  for (int i = 0; tests[i]; ++i) {
    result = process(sb, i);
    mu_assert(result == 0, "process() received: %d %s", result, lsb_get_error(sb));
    mu_assert(strcmp(tests[i], written_data) == 0, "test: %d received: %s", i, written_data);
  }

  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}
#endif


static char* test_serialize()
{
  const char* output_file = "serialize.preserve";
  lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules", 64000, 1000, 64000);
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


static char* test_serialize_failure()
{
  const char* output_file = "serialize_failure.preserve";
  const char* expected = "serialize_data cannot preserve type 'function'";

  lua_sandbox* sb = lsb_create(NULL, "lua/serialize_failure.lua", "../../modules", 32767,
                               1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, output_file);
  mu_assert(e, "lsb_destroy() received: no error");
  mu_assert(strcmp(e, expected) == 0, "lsb_destroy() received: %s", e);
  free(e);
  mu_assert(file_exists(output_file) == 0, "output file was not cleaned up");

  return NULL;
}


static char* test_serialize_noglobal()
{
  const char* output_file = "serialize_noglobal.preserve";
  const char* expected = "preserve_global_data cannot access the global table";

  lua_sandbox* sb = lsb_create(NULL, "lua/serialize_noglobal.lua", "../../modules",
                               32767, 1000, 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb, output_file);
  mu_assert(e, "lsb_destroy() received: no error");
  mu_assert(strcmp(e, expected) == 0, "lsb_destroy() received: %s", e);
  free(e);
  mu_assert(file_exists(output_file) == 0, "output file was not cleaned up");

  return NULL;
}


static char* benchmark_counter()
{
  int iter = 10000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/counter.lua", "../../modules", 32000, 10, 0);
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
  printf("benchmark_counter() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_serialize()
{
  int iter = 1000;
  const char* output_file = "serialize.preserve";

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules", 32767, 1000,
                                 1024);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    e = lsb_destroy(sb, output_file);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_serialize() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_deserialize()
{
  int iter = 1000;

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules", 32767, 1000,
                                 1024);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, "output/serialize.data");
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    e = lsb_destroy(sb, NULL);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_deserialize() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_lpeg_decoder()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/decoder.lua", "../../modules", 8 * 1024 * 1024, 1000000,
                               1024 * 63);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 0);
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lpeg_decoder() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_lua_types_output()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000, 1000,
                               1024 * 63);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 0);
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lua_types_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_cbuf_output()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000, 1000,
                               1024 * 63);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 1);
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cbuf_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_table_output()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000, 1000,
                               1024 * 63);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 2);
  }
  t = clock() - t;
  e = lsb_destroy(sb, NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_table_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC / iter);

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
  mu_run_test(test_output);
  mu_run_test(test_cbuf_errors);
  mu_run_test(test_cbuf);
  mu_run_test(test_cbuf_delta);
  mu_run_test(test_cjson);
  mu_run_test(test_errors);
  mu_run_test(test_lpeg);
  mu_run_test(test_lpeg_grammar);
  mu_run_test(test_serialize);
  mu_run_test(test_serialize_failure);
  mu_run_test(test_serialize_noglobal);

  mu_run_test(benchmark_counter);
  mu_run_test(benchmark_serialize);
  mu_run_test(benchmark_deserialize);
  mu_run_test(benchmark_lpeg_decoder);
  mu_run_test(benchmark_lua_types_output);
  mu_run_test(benchmark_cbuf_output);
  mu_run_test(benchmark_table_output);
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
