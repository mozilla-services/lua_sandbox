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

#include "../luasandbox_impl.h"
#include "luasandbox/lauxlib.h"
#include "luasandbox/lua.h"
#include "luasandbox/test/mu_test.h"
#include "luasandbox/test/sandbox.h"
#include "luasandbox/util/util.h"
#include "luasandbox_output.h"
#include "luasandbox_serialize.h"

char *e = NULL;
static char print_out[2048] = { 0 };


#ifdef _WIN32
#define MODULE_PATH "path = 'modules\\\\?.lua';cpath = 'modules\\\\?.dll'\n"
#else
#define MODULE_PATH "path = 'modules/?.lua';cpath = 'modules/?.so'\n"
#endif

static const char *mozsvc_test_ud = "mozsvc.test_ud";

typedef struct test_ud {
  char name[10];
} test_ud;


static int ud_new(lua_State *lua)
{
  size_t len;
  const char *name = luaL_checklstring(lua, 1, &len);
  test_ud *ud = lua_newuserdata(lua, sizeof(test_ud));
  if (len < 10) {
    strcpy(ud->name, name);
  } else {
    memcpy(ud->name, name, 9);
    ud->name[9] = 0;
  }
  luaL_getmetatable(lua, mozsvc_test_ud);
  lua_setmetatable(lua, -2);
  return 1;
}


static int ud_serialize(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  const char *key = lua_touserdata(lua, -2);
  test_ud *ud = lua_touserdata(lua, -3);
  if (!(ob && key && ud)) {return 1;}
  return lsb_outputf(ob, "if %s == nil then %s = ud.new('%s') end\n", key, key,
                     ud->name) == NULL ? 0 : 1;
}


static int ud_output(lua_State *lua)
{
  lsb_output_buffer *ob = lua_touserdata(lua, -1);
  test_ud *ud = lua_touserdata(lua, -2);
  if (!(ob && ud)) {return 1; }
  return lsb_outputf(ob, "%s", ud->name) == NULL ? 0 : 1;
}


static const struct luaL_reg testudlib_f[] =
{
  { "new", ud_new }, { NULL, NULL }
};


static int luaopen_ud(lua_State *lua)
{
  lua_newtable(lua);
  lsb_add_serialize_function(lua, ud_serialize);
  lsb_add_output_function(lua, ud_output);
  lua_replace(lua, LUA_ENVIRONINDEX);
  luaL_newmetatable(lua, mozsvc_test_ud);
  luaL_register(lua, "ud", testudlib_f);
  return 1;
}


static void add_ud_module(lsb_lua_sandbox *sb)
{
  lua_State *lua = lsb_get_lua(sb);
  luaL_findtable(lua, LUA_REGISTRYINDEX, "_PRELOADED", 1);
  lua_pushstring(lua, "ud");
  lua_pushcfunction(lua, luaopen_ud);
  lua_rawset(lua, -3);
  lua_pop(lua, 1); // remove the preloaded table
}


static const char *test_cfg =
    "userflag = true\n"
    "memory_limit = 0\n"
    "instruction_limit = 0\n"
    "output_limit = 0\n"
    "remove_entries = {\n"
    "[''] = {'collectgarbage','dofile','load','loadfile','loadstring',"
    "'newproxy','print'},\n"
    "os = {'getenv','execute','exit','remove','rename','setlocale','tmpname'}\n"
    "}\n"
    "disable_modules = {io = 1, coroutine = 1}\n"
    "log_level = 7\n"
    MODULE_PATH
;


int file_exists(const char *fn)
{
  FILE *fh;
  fh = fopen(fn, "r");
  if (fh) {
    fclose(fh);
    return 1;
  }
  return 0;
}


void print(void *context, const char *component, int level, const char *fmt, ...)
{
  (void)context;
  va_list args;
  int n = snprintf(print_out, sizeof print_out, "%d %s ", level,
                   component ? component : "unnamed");
  va_start(args, fmt);
  n = vsnprintf(print_out + n, sizeof print_out - n, fmt, args);
  va_end(args);
}
static lsb_logger printer = { .context = NULL, .cb = print };


static char* test_api_assertion()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/counter.lua", "", NULL);
  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  lsb_stop_sandbox(NULL);
  mu_assert(lsb_destroy(NULL) == NULL, "not null");
  mu_assert(lsb_usage(NULL, 0, 0) == 0, "not 0");
  mu_assert(lsb_usage(sb, LSB_UT_MAX, 0) == 0, "not 0");
  mu_assert(lsb_usage(sb, 0, LSB_US_MAX) == 0, "not 0");
  mu_assert(strcmp(lsb_get_error(NULL), "") == 0, "not empty");
  lsb_set_error(NULL, "foo");
  mu_assert(lsb_get_lua(NULL) == NULL, "not null");
  mu_assert(lsb_get_lua_file(NULL) == NULL, "not null");
  mu_assert(lsb_get_parent(NULL) == NULL, "not null");
  mu_assert(lsb_get_logger(NULL) == NULL, "not null");
  mu_assert(lsb_get_state(NULL) == LSB_UNKNOWN, "not unknown");
  lsb_add_function(NULL, lsb_test_write_output, "foo");
  lsb_add_function(sb, NULL, "foo");
  lsb_add_function(sb, lsb_test_write_output, NULL);
  mu_assert(lsb_pcall_setup(NULL, "foo") == LSB_ERR_UTIL_NULL, "not null");
  mu_assert(lsb_pcall_setup(sb, NULL) == LSB_ERR_UTIL_NULL, "not null");
  lsb_add_function(NULL, NULL, NULL);
  lsb_pcall_teardown(NULL);
  lsb_terminate(NULL, NULL);
  lsb_terminate(sb, NULL);
  lsb_add_function(sb, lsb_test_write_output, "write_output");

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_create()
{
  static char *cfg = "function foo() return 0 end\nt = {[true] = 1}\n";
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/counter.lua", cfg, &lsb_test_logger);
  mu_assert(sb, "lsb_create() failed");
  lsb_destroy(sb);
  sb = lsb_create(NULL, "lua/counter.lua", cfg, NULL);
  mu_assert(sb, "lsb_create() failed");
  lsb_destroy(sb);
  sb = lsb_create(NULL, "lua/counter.lua", "memory_limit = 3e9", NULL);
  mu_assert(sb, "lsb_create() failed");
  lsb_destroy(sb);
  return NULL;
}


static char* test_create_error()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, NULL, NULL, NULL);
  mu_assert(!sb, "lsb_create() null lua_file");

  sb = lsb_create(NULL, "lua/counter.lua", "input_limit = 'aaa'", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "output_limit = 'aaa'", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "memory_limit = 'aaa'", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "memory_limit = -1", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "memory_limit = 1.85e19", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "instruction_limit = 'aaa'", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "path = 1", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "cpath = 1", NULL);
  mu_assert(!sb, "lsb_create() invalid config");

  sb = lsb_create(NULL, "lua/counter.lua", "test = {", &lsb_test_logger);
  mu_assert(!sb, "lsb_create() invalid config");

  return NULL;
}


static char* test_read_config()
{
  const char *cfg = "memory_limit = 65765\n"
      "instruction_limit = 1000\n"
      "output_limit = 1024\n"
      "array = {'foo', 99}\n"
      "hash  = {foo = 'bar', hash1 = {subfoo = 'subbar'}}\n"
      MODULE_PATH;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/read_config.lua", cfg, NULL);
  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_init_error()
{
  // null sandbox
  lsb_err_value ret = lsb_init(NULL, NULL);
  mu_assert(ret == LSB_ERR_UTIL_NULL, "lsb_init() null sandbox ptr");

  // load error
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple1.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  ret = lsb_init(sb, NULL);
  mu_assert(ret == LSB_ERR_LUA, "lsb_init() received: %s", lsb_err_string(ret));
  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_TERMINATED, "lsb_get_state() received: %d", s);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // out of memory
  sb = lsb_create(NULL, "lua/simple.lua", "memory_limit = 6000", NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  ret = lsb_init(sb, NULL);
  mu_assert(ret == LSB_ERR_LUA, "lsb_init() received: %s", lsb_err_string(ret));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s\n", e);

  sb = lsb_create(NULL, "lua/no_external_modules.lua", NULL, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  // disabled external modules
  ret = lsb_init(sb, NULL);
  mu_assert(ret == LSB_ERR_LUA, "lsb_init() received: %s", lsb_err_string(ret));
  const char *expected = "no 'path' configuration was specified for the "
      "sandbox; external modules have been disabled";
  mu_assert(strcmp(lsb_get_error(sb), expected) == 0,
            "lsb_get_error() received: %s", lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_destroy_error()
{
  const char *expected = "preserve_global_data could not open: "
      "invaliddir/simple.preserve";
  e = lsb_destroy(NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  lsb_err_value ret = lsb_init(sb, "invaliddir/simple.preserve");
  mu_assert(!ret, "lsb_init() received: %s", ret);
  e = lsb_destroy(sb);
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

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  u = lsb_usage(NULL, LSB_UT_MAX + 1, LSB_US_CURRENT);
  mu_assert(u == 0, "Invalid usage type received: %" PRIuSIZE, u);

  u = lsb_usage(NULL, LSB_UT_MEMORY, LSB_US_MAX + 1);
  mu_assert(u == 0, "Invalid usage stat received: %" PRIuSIZE, u);

  mu_assert(sb, "lsb_create() received: NULL");
  lsb_terminate(sb, "forced termination");
  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_TERMINATED, "lsb_get_state() received: %d", s);
  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u > 0, "Terminated memory usage received: 0");

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_stop()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/counter.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_stop_sandbox(sb);
  lua_getglobal(lsb_get_lua(sb), "process");
  lua_pushnumber(lsb_get_lua(sb), 0);
  mu_assert_rv(2, lua_pcall(lsb_get_lua(sb), 1, 2, 0));
  const char *msg = lua_tostring(lsb_get_lua(sb), -1);
  mu_assert(strcmp(LSB_SHUTTING_DOWN, msg) == 0, "received: %s", msg);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_simple()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua",
                                   "memory_limit = 65765;"
                                   "instruction_limit = 1000;"
                                   "output_limit = 1024;", NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, "simple.preserve");
  mu_assert(!ret, "lsb_init() received: %s", ret);

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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}



static char* test_simple_error()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  int result = lsb_test_process(sb, 1);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(strcmp("ok", lsb_get_error(sb)) == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(strcmp("", lsb_get_error(sb)) == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  result = lsb_test_process(sb, 2);
  mu_assert(result == 1, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output()
{
  const char *outputs[] = {
    "1.2 string nil true false",
    "foo",
    NULL
  };

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  add_ud_module(sb);

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  for (int x = 0; outputs[x]; ++x) {
    int result = lsb_test_process(sb, x);
    mu_assert(!result, "process() test: %d failed: %d %s", x, result,
              lsb_get_error(sb));
    if (outputs[x][0]) {
      mu_assert(strcmp(outputs[x], lsb_test_output) == 0,
                "test: %d received: %s", x, lsb_test_output);
    }
  }

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output_errors()
{
  const char *tests[] =
  {
    "process() lua/output_errors.lua:10: bad argument #1 to 'output' (unsupported type)"
    , "process() lua/output_errors.lua:16: output_limit exceeded"
    , "process() lua/output_errors.lua:18: bad argument #1 to 'write_output' (unknown userdata type)"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output_errors.lua",
                                     MODULE_PATH "output_limit = 128", NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    lsb_err_value ret = lsb_init(sb, NULL);
    mu_assert(!ret, "lsb_init() received: %s", ret);
    lsb_add_function(sb, &lsb_test_write_output, "write_output");

    int result = lsb_test_process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char *le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_errors()
{
  const char *tests[] = {
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
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/errors.lua",
                                     "memory_limit = 32767;"
                                     "instruction_limit = 1000;"
                                     "output_limit = 128;"
#ifdef _WIN32
                                     "path = 'lua\\\\?.lua'"
                                     "cpath = 'lua\\\\?.dll'",
#else
                                     "path = 'lua/?.lua'"
                                     "cpath = 'lua/?.so'",
#endif
                                     NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    lsb_err_value ret = lsb_init(sb, NULL);
    mu_assert(!ret, "lsb_init() received: %s", ret);

    int result = lsb_test_process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char *le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_serialize()
{
  const char *output_file = "serialize.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/serialize.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  add_ud_module(sb);

  lsb_err_value ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  char *expected = lsb_read_file("output/serialize.lua51.data");
  char *actual = lsb_read_file(output_file);
  mu_assert(strcmp(expected, actual) == 0, "serialization mismatch");
  free(expected);
  free(actual);

  return NULL;
}


static char* test_restore()
{
  const char *output_file = "restore.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/restore.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  lsb_err_value ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_add_function(sb, &lsb_test_write_output, "write_output");
  int result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("101", lsb_test_output) == 0, "test: initial load received: %s",
            lsb_test_output);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/restore.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_add_function(sb, &lsb_test_write_output, "write_output");
  result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("102", lsb_test_output) == 0, "test: reload received: %s",
            lsb_test_output);
  result = lsb_test_report(sb, 2); // change the preservation version
  mu_assert(result == 0, "report() received: %d", result);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data with a version change
  sb = lsb_create(NULL, "lua/restore.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_add_function(sb, &lsb_test_write_output, "write_output");
  result = lsb_test_process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("101", lsb_test_output) == 0,
            "test: reload with version change received: %s", lsb_test_output);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_serialize_failure()
{
  const char *output_file = "serialize_failure.preserve";
  const char *expected = "serialize_data cannot preserve type 'function'";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "lua/serialize_failure.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, output_file);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  e = lsb_destroy(sb);
  mu_assert(e, "lsb_destroy() received: no error");
  mu_assert(strcmp(e, expected) == 0, "lsb_destroy() received: %s", e);
  free(e);
  e = NULL;
  mu_assert(file_exists(output_file) == 0, "output file was not cleaned up");

  return NULL;
}


static char* test_sandbox_config()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/sandbox_config.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_print()
{
  const char *tests[] =
  {
    ""
    , "7 lua/print.lua foo \t10\ttrue"
    , "7 lua/print.lua f \t10\ttrue"
    , ""
    , NULL
  };
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/print.lua", "log_level = 7;", &printer);
  mu_assert(sb, "lsb_create() received: NULL");
  mu_assert(lsb_get_logger(sb) != NULL, "no logger");
  mu_assert(lsb_get_logger(sb)->cb == printer.cb, "incorrect logger callback");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  for (int i = 0; tests[i]; ++i) {
    print_out[0] = 0;
    int result = lsb_test_process(sb, i);
    mu_assert(result == 0, "test: %d received: %d error: %s", i, result, lsb_get_error(sb));
    mu_assert(strcmp(tests[i], print_out) == 0, "test: %d expected: %s received: %s", i, tests[i], print_out);
  }

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_print_disabled()
{
  const char *tests[] =
  {
    ""
    , ""
    , NULL
  };
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/print.lua", "log_level = 6;", &printer);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  for (int i = 0; tests[i]; ++i) {
    print_out[0] = 0;
    int result = lsb_test_process(sb, i);
    mu_assert(result == 0, "test: %d received: %d error: %s", i, result, lsb_get_error(sb));
    mu_assert(strcmp(tests[i], print_out) == 0, "test: %d expected: %s received: %s", i, tests[i], print_out);
  }

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_print_lsb_test_logger()
{
  const char *tests[] =
  {
    ""
    , "7 test.print foo \t10\ttrue"
    , NULL
  };
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/print.lua", "log_level = 7;Logger = 'test.print';", &printer);
  mu_assert(sb, "lsb_create() received: NULL");

  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);

  for (int i = 0; tests[i]; ++i) {
    print_out[0] = 0;
    int result = lsb_test_process(sb, i);
    mu_assert(result == 0, "test: %d received: %d error: %s", i, result, lsb_get_error(sb));
    mu_assert(strcmp(tests[i], print_out) == 0, "test: %d expected: %s received: %s", i, tests[i], print_out);
  }

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  return NULL;
}


static char* test_serialize_binary()
{
  size_t size = 512;
  lsb_output_buffer b;
  lsb_err_value ret = lsb_init_output_buffer(&b, size);
  mu_assert(ret == NULL, "received: %s", lsb_err_string(ret));
  lsb_serialize_binary(&b, "a\r\n\\\"", 5);
  mu_assert(b.pos == 9, "received %d", (int)b.pos);
  mu_assert(memcmp(b.buf, "a\\r\\n\\\\\\\"", 5) == 0, "received %.*s", 9,
            b.buf);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* benchmark_counter()
{
  int iter = 10000000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/counter.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_test_process(sb, 0);
  }
  t = clock() - t;
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_counter() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_serialize()
{
  int iter = 1000;
  const char *output_file = "serialize.preserve";

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    remove(output_file);
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/serialize.lua", test_cfg, NULL);
    mu_assert(sb, "lsb_create() received: NULL");
    add_ud_module(sb);

    lsb_err_value ret = lsb_init(sb, output_file);
    mu_assert(!ret, "lsb_init() received: %s %s", ret, lsb_get_error(sb));
    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_serialize() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_deserialize()
{
  int iter = 1000;

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/serialize.lua", test_cfg, NULL);
    mu_assert(sb, "lsb_create() received: NULL");
    add_ud_module(sb);

    lsb_err_value ret = lsb_init(sb, "output/serialize.data");
    mu_assert(!ret, "lsb_init() received: %s", ret);
    free(sb->state_file);
    sb->state_file = NULL; // poke the internals to prevent serialization
    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_deserialize() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_lua_types_output()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  lsb_err_value ret = lsb_init(sb, NULL);
  mu_assert(!ret, "lsb_init() received: %s", ret);
  lsb_add_function(sb, &lsb_test_write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == lsb_test_process(sb, 0), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lua_types_output() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_api_assertion);
  mu_run_test(test_create);
  mu_run_test(test_create_error);
  mu_run_test(test_read_config);
  mu_run_test(test_init_error);
  mu_run_test(test_destroy_error);
  mu_run_test(test_usage_error);
  mu_run_test(test_stop);
  mu_run_test(test_simple);
  mu_run_test(test_simple_error);
  mu_run_test(test_output);
  mu_run_test(test_output_errors);
  mu_run_test(test_errors);
  mu_run_test(test_serialize);
  mu_run_test(test_restore);
  mu_run_test(test_serialize_failure);
  mu_run_test(test_sandbox_config);
  mu_run_test(test_print);
  mu_run_test(test_print_disabled);
  mu_run_test(test_print_lsb_test_logger);
  mu_run_test(test_serialize_binary);

  mu_run_test(benchmark_counter);
  mu_run_test(benchmark_serialize);
  mu_run_test(benchmark_deserialize);
  mu_run_test(benchmark_lua_types_output);

  return NULL;
}


int main()
{
  char *result = all_tests();
  if (result) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", mu_tests_run);
  free(e);

  return result != NULL;
}
