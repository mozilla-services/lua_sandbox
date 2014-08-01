/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox unit tests @file */

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

  if (lua_gettop(lua) != 1 || lua_type(lua, 1) != LUA_TTABLE) {
    luaL_error(lua, "write_message() takes a table argument");
  }

  if (lsb_output_protobuf(lsb, 1, 0) != 0) {
    luaL_error(lua, "write_message() could not encode protobuf - %s",
               lsb_get_error(lsb));
  }
  written_data = lsb_get_output(lsb, &written_data_len);
  return 0;
}


static char* test_create_error()
{
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules",
                               LSB_MEMORY + 1, 1000, 1024);
  mu_assert(!sb, "lsb_create() memory_limit");

  sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765,
                  LSB_INSTRUCTION + 1, 1024);
  mu_assert(!sb, "lsb_create() instruction_limit");

  sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765, 1000,
                  LSB_OUTPUT + 1);
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
  lua_sandbox* sb = lsb_create(NULL, "lua/simple1.lua", "../../modules", 65765,
                               1000, 1024);
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

  sb = lsb_create(NULL, "lua/lpeg_date_time.lua", NULL, 100000, 1000, 8000);
  mu_assert(sb, "lsb_create() received: NULL");

  // disabled external modules
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  const char* expected = "lua/lpeg_date_time.lua:7: require_library() external modules are disabled";
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

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765,
                               1000, 1024);
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

  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765,
                               1000, 1024);
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
  lua_sandbox* sb = lsb_create(NULL, "lua/simple.lua", "../../modules", 65765,
                               1000, 1024);
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
    "1.2 string nil true false"
    , ""
    , "\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x13\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x3a\x08\x00\x00\x00\x00\x00\x00\xf0\x3f"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x24\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x3a\x18\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x00\x00\x00\x00\x00\x00\x10\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x1a\x0a\x05\x63\x6f\x75\x6e\x74\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x08\x00\x00\x00\x00\x00\x00\x14\x40"
    , "\x10\x80\x94\xeb\xdc\x03\x52\x2b\x0a\x06\x63\x6f\x75\x6e\x74\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\x18\x40\x00\x00\x00\x00\x00\x00\x1c\x40\x00\x00\x00\x00\x00\x00\x20\x40"
#ifdef LUA_JIT
    , "\x10\x80\x94\xeb\xdc\x03\x52\x14\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x3a\x08\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x0b\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x42\x01\x01\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
#else
    , "\x10\x80\x94\xeb\xdc\x03\x52\x14\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x3a\x08\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0b\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x42\x01\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33"
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
  lsb_add_function(sb, &write_output, "write_output");
  lsb_add_function(sb, &write_message, "write_message");

  for (int x = 0; outputs[x]; ++x) {
    result = process(sb, x);
    mu_assert(!result, "process() test: %d failed: %d %s", x, result,
              lsb_get_error(sb));
    if (outputs[x][0]) {
      if (outputs[x][0] == 0x10) {
        size_t header = 18;
        if (memcmp(outputs[x], written_data + header, written_data_len - header)
            != 0) {
          char hex_data[LSB_OUTPUT + 1];
          size_t z = 0;
          for (size_t y = header; y < written_data_len; ++y, z += 3) {
            snprintf(hex_data + z, LSB_OUTPUT - z, "%02x ",
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
    "process() lua/output_errors.lua:10: bad argument #1 to 'output' (unsuported type)"
    , "process() lua/output_errors.lua:16: output_limit exceeded"
    , "process() lua/output_errors.lua:19: write_message() could not encode protobuf - array has mixed types"
    , "process() lua/output_errors.lua:22: write_message() could not encode protobuf - unsupported type: nil"
    , "process() lua/output_errors.lua:24: bad argument #1 to 'write_output' (unknown userdata type)"
    , "process() lua/output_errors.lua:27: output_limit exceeded"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/output_errors.lua", "../../modules",
                                 256000, 1000, 128);
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
    , "process() lua/circular_buffer_errors.lua:77: fromstring() too few values: 0, expected 2"
    , "process() lua/circular_buffer_errors.lua:80: fromstring() too few values: 0, expected 2"
    , "process() lua/circular_buffer_errors.lua:83: fromstring() too many values, more than: 2"
    , "process() lua/circular_buffer_errors.lua:86: bad argument #-1 to 'mannwhitneyu' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:89: bad argument #1 to 'mannwhitneyu' (number expected, got nil)"
    , "process() lua/circular_buffer_errors.lua:92: bad argument #1 to 'mannwhitneyu' (column out of range)"
    , "process() lua/circular_buffer_errors.lua:95: bad argument #3 to 'mannwhitneyu' (ranges must not overlap)"
    , "process() lua/circular_buffer_errors.lua:98: bad argument #3 to 'mannwhitneyu' (end_1 must be >= start_1)"
    , "process() lua/circular_buffer_errors.lua:101: bad argument #5 to 'mannwhitneyu' (end_2 must be >= start_2)"
    , "process() lua/circular_buffer_errors.lua:104: bad argument #-1 to 'mannwhitneyu' (too many arguments)"
    , "process() lua/circular_buffer_errors.lua:107: bad argument #6 to 'mannwhitneyu' (use_continuity must be a boolean)"
    , "process() lua/circular_buffer_errors.lua:110: bad argument #-1 to 'get_header' (incorrect number of arguments)"
    , "process() lua/circular_buffer_errors.lua:113: bad argument #1 to 'get_header' (column out of range)"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_errors.lua",
                                 "../../modules", 32767, 1000, 128);
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
    "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\nnan\tnan\tnan\n"
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
    , "{\"time\":2,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n3\t1\t3\nnan\tnan\tnan\n1\t1\t1\n"
    , "{\"time\":8,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\n1\t1\t1\n"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer.lua", "../../modules",
                               64000, 2000, 32767);
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

  for (int i = 1; i < 18; ++i) {
    result = report(sb, i);
    mu_assert(result == 0, "report() test: %d received: %d error: %s", i,
              result, lsb_get_error(sb));
  }

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
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/circular_buffer_delta.lua",
                               "../../modules", 32767, 1000, 32767);
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

  lua_sandbox* sb = lsb_create(NULL, "lua/cjson.lua", "../../modules", 128000,
                               10000, 64512);
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
    , "process() lua/errors.lua:11: bad argument #0 to 'output' (must have at least one argument)"
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
  const char* tests[] = {
    "lua/lpeg.lua"
    , "lua/lpeg_clf.lua"
    , "lua/lpeg_cbufd.lua"
    , "lua/lpeg_date_time.lua"
    , "lua/lpeg_ip_address.lua"
    , "lua/lpeg_mysql.lua"
    , "lua/lpeg_syslog.lua"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, tests[i], "../../modules", 8000000,
                                 1000000, 0);
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
  lua_sandbox* sb = lsb_create(NULL, "lua/util_test.lua", "../../modules",
                               8000000, 1000000, 0);
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
  lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules",
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

  lua_sandbox* sb = lsb_create(NULL, "lua/restore.lua", "../../modules",
                               8000000, 1000000, 63 * 1024);
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
  sb = lsb_create(NULL, "lua/restore.lua", "../../modules", 8000000, 1000000,
                  63 * 1024);
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
  sb = lsb_create(NULL, "lua/restore.lua", "../../modules", 8000000, 1000000,
                  63 * 1024);
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
                               "lua/serialize_failure.lua", "../../modules",
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


static char* test_serialize_noglobal()
{
  const char* output_file = "serialize_noglobal.preserve";
  const char* expected = "preserve_global_data cannot access the global table";

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/serialize_noglobal.lua", "../../modules",
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


static char* test_bloom_filter_errors()
{
  const char* tests[] =
  {
    "process() lua/bloom_filter_errors.lua:9: bad argument #0 to 'new' (incorrect number of arguments)"
    , "process() lua/bloom_filter_errors.lua:11: bad argument #1 to 'new' (number expected, got nil)"
    , "process() lua/bloom_filter_errors.lua:13: bad argument #1 to 'new' (items must be > 1)"
    , "process() lua/bloom_filter_errors.lua:15: bad argument #2 to 'new' (number expected, got nil)"
    , "process() lua/bloom_filter_errors.lua:17: bad argument #2 to 'new' (probability must be between 0 and 1)"
    , "process() lua/bloom_filter_errors.lua:19: bad argument #2 to 'new' (probability must be between 0 and 1)"
    , "process() lua/bloom_filter_errors.lua:22: bad argument #-1 to 'add' (incorrect number of arguments)"
    , "process() lua/bloom_filter_errors.lua:25: bad argument #1 to 'add' (must be a string or number)"
    , "process() lua/bloom_filter_errors.lua:28: bad argument #-1 to 'query' (incorrect number of arguments)"
    , "process() lua/bloom_filter_errors.lua:31: bad argument #1 to 'query' (must be a string or number)"
    , "process() lua/bloom_filter_errors.lua:34: bad argument #-1 to 'clear' (incorrect number of arguments)"
    , "process() lua/bloom_filter_errors.lua:37: bad argument #1 to 'fromstring' (string expected, got table)"
    , "process() lua/bloom_filter_errors.lua:40: fromstring() bytes found: 23, expected 24"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL, "lua/bloom_filter_errors.lua",
                                 "../../modules", 32767, 1000, 128);
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


static char* test_bloom_filter()
{
  const char* output_file = "bloom_filter.preserve";
  const char* tests[] = {
    "1"
    , "2"
    , "3"
    , NULL
  };

  lua_sandbox* sb = lsb_create(NULL, "lua/bloom_filter.lua", "../../modules",
                               8000000, 1000000, 63 * 1024);
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
  sb = lsb_create(NULL, "lua/bloom_filter.lua", "../../modules", 8000000,
                  1000000, 63 * 1024);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

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


static char* test_hyperloglog_errors()
{
  const char* tests[] =
  {
    "process() lua/hyperloglog_errors.lua:9: bad argument #0 to 'new' (incorrect number of arguments)"
    , "process() lua/hyperloglog_errors.lua:12: bad argument #1 to 'add' (must be a string or number)"
    , "process() lua/hyperloglog_errors.lua:15: bad argument #-1 to 'add' (incorrect number of arguments)"
    , "process() lua/hyperloglog_errors.lua:18: bad argument #-1 to 'count' (incorrect number of arguments)"
    , "process() lua/hyperloglog_errors.lua:21: bad argument #-1 to 'clear' (incorrect number of arguments)"
    , "process() lua/hyperloglog_errors.lua:24: bad argument #1 to 'fromstring' (string expected, got table)"
    , "process() lua/hyperloglog_errors.lua:27: fromstring() bytes found: 23, expected 12304"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lua_sandbox* sb = lsb_create(NULL,
                                 "lua/hyperloglog_errors.lua", "../../modules",
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


static char* test_hyperloglog()
{
  const char* output_file = "hyperloglog.preserve";

  lua_sandbox* sb = lsb_create(NULL, "lua/hyperloglog.lua", "../../modules",
                               8000000, 1000000, 63 * 1024);
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
  sb = lsb_create(NULL, "lua/hyperloglog.lua", "../../modules", 8000000,
                  1000000, 63 * 1024);
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


static char* benchmark_counter()
{
  int iter = 10000000;

  lua_sandbox* sb = lsb_create(NULL, "lua/counter.lua", "../../modules", 32000,
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
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules",
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
    lua_sandbox* sb = lsb_create(NULL, "lua/serialize.lua", "../../modules",
                                 64000, 1000, 1024);
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

  lua_sandbox* sb = lsb_create(NULL, "lua/decoder.lua", "../../modules", 8
                               * 1024 * 1024, 1000000,
                               1024 * 63);
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
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules",
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

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000,
                               1000, 1024 * 63);
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
  printf("benchmark_cbuf_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_message_output()
{
  int iter = 10000;

  lua_sandbox* sb = lsb_create(NULL, "lua/output.lua", "../../modules", 100000,
                               1000,
                               1024 * 63);
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
  printf("benchmark_message_output() %g seconds\n", ((float)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_cbuf_add()
{
  int iter = 1000000;

  lua_sandbox* sb = lsb_create(NULL,
                               "lua/circular_buffer_add.lua", "../../modules",
                               8000000, 1000, 1024 * 63);
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
                               "lua/bloom_filter_benchmark.lua", "../../modules",
                               1024 * 1024 * 8, 1000, 1024 * 63);
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
  mu_assert(strcmp("1000000", written_data) == 0, "received: %s", written_data);
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

  lua_sandbox* sb = lsb_create(NULL, "lua/hyperloglog.lua", "../../modules",
                               1024 * 1024 * 8, 1000, 1024 * 63);
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


static char* all_tests()
{

  mu_run_test(test_create_error);
  mu_run_test(test_init_error);
  mu_run_test(test_destroy_error);
  mu_run_test(test_usage_error);
  mu_run_test(test_misc);
  mu_run_test(test_simple);
  mu_run_test(test_output);
  mu_run_test(test_output_errors);
  mu_run_test(test_cbuf_errors);
  mu_run_test(test_cbuf);
  mu_run_test(test_cbuf_delta);
  mu_run_test(test_cjson);
  mu_run_test(test_errors);
  mu_run_test(test_lpeg);
  mu_run_test(test_util);
  mu_run_test(test_serialize);
  mu_run_test(test_restore);
  mu_run_test(test_serialize_failure);
  mu_run_test(test_serialize_noglobal);
  mu_run_test(test_bloom_filter_errors);
  mu_run_test(test_bloom_filter);
  mu_run_test(test_hyperloglog_errors);
  mu_run_test(test_hyperloglog);

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
