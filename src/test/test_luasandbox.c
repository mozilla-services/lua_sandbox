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
#include "luasandbox/util/util.h"

#include "mu_test.h"

char *e = NULL;
const char *written_data = NULL;
size_t written_data_len = 0;


#ifdef _WIN32
#define MODULE_PATH "path = 'modules\\?.lua';cpath = 'modules\\?.dll'\n"
#else
#define MODULE_PATH "path = 'modules/?.lua';cpath = 'modules/?.so'\n"
#endif


static const char *test_cfg =
    "memory_limit = 0\n"
    "instruction_limit = 0\n"
    "output_limit = 0\n"
    "remove_entries = {\n"
    "[''] = {'collectgarbage','coroutine','dofile','load','loadfile'"
    ",'loadstring','newproxy','print'},\n"
    "os = {'getenv','execute','exit','remove','rename','setlocale','tmpname'}\n"
    "}\n"
    "disable_modules = {io = 1}\n"
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


int process(lsb_lua_sandbox *lsb, double ts)
{
  static const char *func_name = "process";
  lua_State *lua = lsb_get_lua(lsb);
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


int report(lsb_lua_sandbox *lsb, double tc)
{
  static const char *func_name = "report";
  lua_State *lua = lsb_get_lua(lsb);
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


int write_output(lua_State *lua)
{
  void *luserdata = lua_touserdata(lua, lua_upvalueindex(1));
  if (NULL == luserdata) {
    luaL_error(lua, "write_output() invalid lightuserdata");
  }
  lsb_lua_sandbox *lsb = (lsb_lua_sandbox *)luserdata;

  int n = lua_gettop(lua);
  lsb_output(lsb, 1, n, 1);
  written_data = lsb_get_output(lsb, &written_data_len);
  return 0;
}


static char* test_create_error()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, NULL, NULL, NULL);
  mu_assert(!sb, "lsb_create() null lua_file");

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
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_init_error()
{
  // null sandbox
  int result = lsb_init(NULL, NULL);
  mu_assert(result == 1, "lsb_init() null sandbox ptr");

  // load error
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple1.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_state s = lsb_get_state(sb);
  mu_assert(s == LSB_TERMINATED, "lsb_get_state() received: %d", s);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // out of memory
  sb = lsb_create(NULL, "lua/simple.lua", "memory_limit = 6000", NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s\n", e);

  sb = lsb_create(NULL, "lua/lpeg_date_time.lua", "path = '';cpath = ''", NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  // disabled external modules
  result = lsb_init(sb, NULL);
  mu_assert(result == 2, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  const char *expected = "lua/lpeg_date_time.lua:7: module 'date_time' not found:";
  mu_assert(strcmp(lsb_get_error(sb), expected) == 0,
            "lsb_get_error() received: %s", lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_destroy_error()
{
  const char *expected = "lsb_preserve_global_data could not open: "
      "invaliddir/simple.preserve";
  e = lsb_destroy(NULL);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, "invaliddir/simple.preserve");
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
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
  u = lsb_usage(sb, LSB_UT_MEMORY, LSB_US_CURRENT);
  mu_assert(u == 0, "Terminated memory usage received: %" PRIuSIZE, u);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}

static char* test_misc()
{
  lsb_state s = lsb_get_state(NULL);
  mu_assert(s == LSB_UNKNOWN, "lsb_get_state() received: %d", s);

  const char *le = lsb_get_error(NULL);
  mu_assert(strlen(le) == 0, "lsb_get_error() received: %s", le);

  return NULL;
}


static char* test_simple()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua",
                                   "memory_limit = 65765;"
                                   "instruction_limit = 1000;"
                                   "output_limit = 1024;", NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, "simple.preserve");
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}



static char* test_simple_error()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/simple.lua", test_cfg, NULL);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_output()
{
  const char *outputs[] = {
    "1.2 string nil true false"
    , ""
    , NULL
  };

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, "circular_buffer.preserve");
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  for (int x = 0; outputs[x]; ++x) {
    result = process(sb, x);
    mu_assert(!result, "process() test: %d failed: %d %s", x, result,
              lsb_get_error(sb));
    if (outputs[x][0]) {
      mu_assert(strcmp(outputs[x], written_data) == 0,
                "test: %d received: %s", x, written_data);
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
    , "process() lua/output_errors.lua:21: output_limit exceeded"
    , "process() lua/output_errors.lua:27: strbuf output_limit exceeded"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output_errors.lua",
                                     MODULE_PATH "output_limit = 128", NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    lsb_add_function(sb, &write_output, "write_output");

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char *le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_cbuf_errors()
{
  const char *tests[] =
  {
    "process() not enough memory"
    , NULL
  };

  for (int i = 0; tests[i]; ++i) {
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/circular_buffer_errors.lua",
                                     MODULE_PATH "memory_limit = 32767", NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char *le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_cbuf_core()
{
  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "../../ep_base/Source/lua_circular_buffer/test.lua",
                                   test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, "");
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cbuf()
{
  const char *output_file = "circular_buffer.preserve";
  const char *outputs[] = {
    "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\nnan\tnan\tnan\n"
    , "{\"time\":0,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n1\t1\t1\n2\t1\t2\n3\t1\t3\n"
    , "{\"time\":2,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\n3\t1\t3\nnan\tnan\tnan\n1\t1\t1\n"
    , "{\"time\":8,\"rows\":3,\"columns\":3,\"seconds_per_row\":1,\"column_info\":[{\"name\":\"Add_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Set_column\",\"unit\":\"count\",\"aggregation\":\"sum\"},{\"name\":\"Get_column\",\"unit\":\"count\",\"aggregation\":\"sum\"}]}\nnan\tnan\tnan\nnan\tnan\tnan\n1\t1\t1\n"
    , NULL
  };

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/circular_buffer.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cbuf_delta()
{
  const char *output_file = "circular_buffer_delta.preserve";
  const char *outputs[] = {
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

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/circular_buffer_delta.lua",
                                   test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cjson()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/cjson.lua",
                                   MODULE_PATH "output_limit = 64", NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cjson_unlimited()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/cjson_unlimited.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  mu_assert(written_data_len == 103001, "received %d bytes", (int)written_data_len);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

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
                                     "path = 'lua\\?.lua'"
                                     "cpath = 'lua\\?.dll'",
#else
                                     "path = 'lua/?.lua'"
                                     "cpath = 'lua/?.so'",
#endif
                                     NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, i);
    mu_assert(result == 1, "test: %d received: %d", i, result);

    const char *le = lsb_get_error(sb);
    mu_assert(le, "test: %d received NULL", i);
    mu_assert(strcmp(tests[i], le) == 0, "test: %d received: %s", i, le);

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }

  return NULL;
}


static char* test_lpeg()
{
  const char *tests[] = {
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
    lsb_lua_sandbox *sb = lsb_create(NULL, tests[i], test_cfg, NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, NULL);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));

    result = process(sb, 0);
    mu_assert(result == 0, "process() received: %d %s", result,
              lsb_get_error(sb));

    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }


  return NULL;
}


static char* test_util()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/util_test.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_serialize()
{
  const char *output_file = "serialize.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/serialize.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

#ifdef LUA_JIT
  char *expected = lsb_read_file("output/serialize.data");
#else
  char *expected = lsb_read_file("output/serialize.lua51.data");
#endif
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
  int result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");
  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result,
            lsb_get_error(sb));
  mu_assert(strcmp("101", written_data) == 0, "test: initial load received: %s",
            written_data);
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/restore.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data with a version change
  sb = lsb_create(NULL, "lua/restore.lua", test_cfg, NULL);
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

  int result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(e, "lsb_destroy() received: no error");
  mu_assert(strcmp(e, expected) == 0, "lsb_destroy() received: %s", e);
  free(e);
  e = NULL;
  mu_assert(file_exists(output_file) == 0, "output file was not cleaned up");

  return NULL;
}


static char* test_bloom_filter_core()
{
  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "../../ep_base/Source/lua_bloom_filter/test.lua",
                                   test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_bloom_filter()
{
  const char *output_file = "bloom_filter.preserve";
  const char *tests[] = {
    "1"
    , "2"
    , "3"
    , NULL
  };

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/bloom_filter.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/bloom_filter.lua", test_cfg, NULL);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_hyperloglog_core()
{
  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "../../ep_base/Source/lua_hyperloglog/test.lua",
                                   test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}

static char* test_hyperloglog()
{
  const char *output_file = "hyperloglog.preserve";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/hyperloglog.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/hyperloglog.lua", test_cfg, NULL);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_struct()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/struct.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  result = process(sb, 0);
  mu_assert(result == 0, "process() received: %d %s", result, lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_sandbox_config()
{
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/sandbox_config.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_cuckoo_filter()
{
  const char *output_file = "cuckoo_filter.preserve";
  const char *tests[] = {
    "1"
    , "2"
    , "3"
    , NULL
  };

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/cuckoo_filter.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/cuckoo_filter.lua", test_cfg, NULL);
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

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* test_sax()
{
  const char *output_file = "sax.preserve";
  const char *word = "CDC CDC ABEGH ABEGH";

  remove(output_file);
  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/sax.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  int result = lsb_init(sb, output_file);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  // re-load to test the preserved data
  sb = lsb_create(NULL, "lua/sax.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");

  result = lsb_init(sb, output_file);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  report(sb, 0);
  mu_assert(strcmp(word, written_data) == 0, "received: %s", written_data);

  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);

  return NULL;
}


static char* benchmark_counter()
{
  int iter = 10000000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/counter.lua", test_cfg, NULL);
  mu_assert(sb, "lsb_create() received: NULL");
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    process(sb, 0);
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

    int result = lsb_init(sb, output_file);
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
    e = lsb_destroy(sb);
    mu_assert(!e, "lsb_destroy() received: %s", e);
  }
  t = clock() - t;
  printf("benchmark_serialize() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_deserialize() // todo need to alter since this now performs the serialization too
{
  int iter = 1000;

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_lua_sandbox *sb = lsb_create(NULL, "lua/serialize.lua", test_cfg, NULL);
    mu_assert(sb, "lsb_create() received: NULL");

    int result = lsb_init(sb, "output/serialize.data");
    mu_assert(result == 0, "lsb_init() received: %d %s", result,
              lsb_get_error(sb));
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
  int result = lsb_init(sb, NULL);
  mu_assert(result == 0, "lsb_init() received: %d %s", result,
            lsb_get_error(sb));
  lsb_add_function(sb, &write_output, "write_output");

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == process(sb, 0), "%s", lsb_get_error(sb));
  }
  t = clock() - t;
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_lua_types_output() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_cbuf_output()
{
  int iter = 10000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/output.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cbuf_output() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_cbuf_add()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "lua/circular_buffer_add.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cbuf_add() %g seconds\n", ((double)t) / CLOCKS_PER_SEC
         / iter);

  return NULL;
}


static char* benchmark_bloom_filter_add()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "lua/bloom_filter_benchmark.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_bloom_filter_add() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_hyperloglog_add()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/hyperloglog.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_hyperloglog_add() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_cuckoo_filter_add()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL,
                                   "lua/cuckoo_filter_benchmark.lua", test_cfg,
                                   NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_cuckoo_filter_add() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* benchmark_sax_add()
{
  int iter = 1000000;

  lsb_lua_sandbox *sb = lsb_create(NULL, "lua/sax_benchmark.lua", test_cfg, NULL);
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
  e = lsb_destroy(sb);
  mu_assert(!e, "lsb_destroy() received: %s", e);
  printf("benchmark_sax_add() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_read_config);
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
  mu_run_test(test_cuckoo_filter);
  mu_run_test(test_sax);

  mu_run_test(benchmark_counter);
  mu_run_test(benchmark_serialize);
  mu_run_test(benchmark_deserialize);
  mu_run_test(benchmark_lua_types_output);
  mu_run_test(benchmark_cbuf_output);
  mu_run_test(benchmark_cbuf_add);
  mu_run_test(benchmark_bloom_filter_add);
  mu_run_test(benchmark_hyperloglog_add);
  mu_run_test(benchmark_cuckoo_filter_add);
  mu_run_test(benchmark_sax_add);

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
