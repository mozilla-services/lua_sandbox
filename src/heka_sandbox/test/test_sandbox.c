/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Heka sandbox unit tests @file */

#include "test.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/heka/sandbox.h"

// {Uuid="" Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}
static char pb[] = "\x0a\x10" "abcdefghijklmnop" "\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33";

char *e = NULL;

void dlog(const char *component, int level, const char *fmt, ...)
{

  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%lld [%d] %s ", (long long)time(NULL), level,
          component ? component : "unnamed");
  vfprintf(stderr, fmt, args);
  fwrite("\n", 1, 1, stderr);
  va_end(args);
}


static int iim(void *parent, const char *pb, size_t pb_len, double cp_numeric,
               const char *cp_string)
{
  static int cnt = 0;
  struct im_result {
    const char  *pb;
    size_t      pb_len;
    double      cp_numeric;
    const char  *cp_string;
  };

  struct im_result results[] = {
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x00", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x01", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x02\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0a\x65\x6e\x76\x76\x65\x72\x73\x69\x6f\x6e\x40\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65", .pb_len = 67, .cp_numeric = 99, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x03\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33", .pb_len = 156, .cp_numeric = NAN, .cp_string = "foo.log:123" },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x04", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x05", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
  };

  if (cnt >= (int)(sizeof results / sizeof results[0])) {
    fprintf(stderr, "tests and results are mis-matched\n");
    return 1;
  }

  if (parent) {
    fprintf(stderr, "test: %d parent set\n", cnt);
  }

  if (pb_len != results[cnt].pb_len) {
    fprintf(stderr, "test: %d pb len expected: %" PRIuSIZE " received: %"
            PRIuSIZE "\n", cnt, results[cnt].pb_len, pb_len);
    return 1;
  }

  if (memcmp(pb, results[cnt].pb, pb_len)) {
    fprintf(stderr, "test: %d\nexpected: ", cnt);
    for (size_t i = 0; i < results[cnt].pb_len; ++i) {
      fprintf(stderr, "\\x%02hhx", results[cnt].pb[i]);
    }
    fprintf(stderr, "\nreceived: ");
    for (size_t i = 0; i < pb_len; ++i) {
      fprintf(stderr, "\\x%02hhx", pb[i]);
    }
    fprintf(stderr, "\n");
    return 1;
  }

  bool ncp_failed = false;
  if (isnan(results[cnt].cp_numeric)) {
    if (!isnan(cp_numeric)) {
      ncp_failed = true;
    }
  } else if (results[cnt].cp_numeric != cp_numeric) {
    ncp_failed = true;
  }
  if (ncp_failed) {
    fprintf(stderr, "test: %d cp_numeric expected: %g received: %g\n", cnt,
            results[cnt].cp_numeric, cp_numeric);
    return 1;
  }

  bool ncs_failed = false;
  if (!results[cnt].cp_string) {
    if (cp_string) {
      ncs_failed = true;
    }
  } else if (!cp_string || strcmp(results[cnt].cp_string, cp_string)) {
    ncs_failed = true;
  }
  if (ncs_failed) {
    fprintf(stderr, "test: %d cp_string expected: %s received: %s\n", cnt,
            results[cnt].cp_string ? results[cnt].cp_string : "NULL",
            cp_string ? cp_string : "NULL");
    return 1;
  }
  cnt++;
  return 0;
}


static int aim(void *parent, const char *pb, size_t pb_len)
{
  static int cnt = 0;
  struct im_result {
    const char  *pb;
    size_t      pb_len;
    double      cp_numeric;
    const char  *cp_string;
  };

  struct im_result results[] = {
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x22\x03\x61\x69\x6d\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d", .pb_len = 34, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x1f\x7d\x09\x9e\xf5\x9d\x40\x1d\xa8\xaf\x6a\xff\xc3\x21\xeb\x42\x10\x80\x88\xe4\xaa\xa0\xa9\xbc\x95\x14\x1a\x0e\x69\x6e\x6a\x65\x63\x74\x5f\x70\x61\x79\x6c\x6f\x61\x64\x22\x03\x61\x69\x6d\x32\x07\x66\x6f\x6f\x20\x62\x61\x72\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d\x52\x13\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x74\x79\x70\x65\x22\x03\x74\x78\x74", .pb_len = 88, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x5b\x7d\xee\xa0\x02\xbc\x45\xbb\xaf\xa9\xcc\x2c\xdd\x65\xde\x45\x10\x80\x88\xdc\xad\xcd\xbf\xbc\x95\x14\x1a\x0e\x69\x6e\x6a\x65\x63\x74\x5f\x70\x61\x79\x6c\x6f\x61\x64\x22\x03\x61\x69\x6d\x32\x07\x66\x6f\x6f\x20\x62\x61\x72\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d\x52\x13\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x74\x79\x70\x65\x22\x03\x64\x61\x74\x52\x14\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x6e\x61\x6d\x65\x22\x04\x74\x65\x73\x74", .pb_len = 110, .cp_numeric = NAN, .cp_string = NULL },
  };

  if (cnt >= (int)(sizeof results / sizeof results[0])) {
    fprintf(stderr, "tests and results are mis-matched\n");
    return 1;
  }

  if (parent) {
    fprintf(stderr, "test: %d parent set\n", cnt);
  }

  if (pb_len != results[cnt].pb_len) {
    fprintf(stderr, "test: %d pb len expected: %" PRIuSIZE " received: %"
            PRIuSIZE "\n", cnt, results[cnt].pb_len, pb_len);
    return 1;
  }

  if (cnt == 0) {
    if (memcmp(pb, results[cnt].pb, pb_len)) {
      fprintf(stderr, "test: %d\nexpected: ", cnt);
      for (size_t i = 0; i < results[cnt].pb_len; ++i) {
        fprintf(stderr, "\\x%02hhx", results[cnt].pb[i]);
      }
      fprintf(stderr, "\nreceived: ");
      for (size_t i = 0; i < pb_len; ++i) {
        fprintf(stderr, "\\x%02hhx", pb[i]);
      }
      fprintf(stderr, "\n");
      return 1;
    }
  } else {
    lsb_heka_message m;
    lsb_init_heka_message(&m, 2);
    bool rv = lsb_decode_heka_message(&m, pb, pb_len, dlog);
    lsb_free_heka_message(&m);
    if (!rv) return 1;
  }
  cnt++;
  return 0;
}


static int ucp(void *parent, void *sequence_id)
{
  static int cnt = 0;
  if (parent) return 1;
  void *results[] = { NULL, (void *)99 };

  if (cnt >= (int)(sizeof results / sizeof results[0])) {
    fprintf(stderr, "tests and results are mis-matched\n");
    return 1;
  }

  if (results[cnt] != sequence_id) {
    fprintf(stderr, "expected: %p received: %p\n", results[cnt], sequence_id);
    return 1;
  }
  cnt++;
  return 0;
}


static char* test_create_input_sandbox()
{
  static const char *lua_file = "lua/input.lua";
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, lua_file, NULL, NULL, dlog,
                              iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  const char* lfn = lsb_heka_get_lua_file(hsb);
  mu_assert(strcmp(lua_file, lfn) == 0, "expected %s received %s", lua_file,
            lfn);
  lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_input(NULL, "notfourd.lua", NULL, NULL, NULL,
                              iim);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");

  hsb = lsb_heka_create_input(NULL, NULL, NULL, NULL, NULL, iim);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");

  hsb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, NULL, NULL);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");
  lsb_heka_destroy_sandbox(hsb); // test NULL
  return NULL;
}


static char* test_create_analysis_sandbox()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, dlog,
                                 aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_analysis(NULL, "notfound.lua", NULL, NULL, NULL, aim);
  mu_assert(!hsb, "lsb_heka_create_analysis succeeded");

  hsb = lsb_heka_create_analysis(NULL, NULL, NULL, NULL, NULL, aim);
  mu_assert(!hsb, "lsb_heka_create_analysis succeeded");

  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, NULL,
                                 NULL);
  mu_assert(!hsb, "lsb_heka_create_analysis succeeded");
  return NULL;
}


static char* test_create_output_sandbox()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/output.lua", NULL, NULL, dlog, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");
  lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_output(NULL, "notfound.lua", NULL, NULL, NULL, ucp);
  mu_assert(!hsb, "lsb_heka_create_output succeeded");

  hsb = lsb_heka_create_output(NULL, NULL, NULL, NULL, NULL, ucp);
  mu_assert(!hsb, "lsb_heka_create_output succeeded");

  hsb = lsb_heka_create_output(NULL, "lua/output.lua", NULL, NULL, NULL, NULL);
  mu_assert(!hsb, "lsb_heka_create_output succeeded");
  return NULL;
}


static char* test_timer_event()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, dlog,
                                 aim);
  mu_assert(0 == lsb_heka_timer_event(hsb, 0, false), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(0 == lsb_heka_timer_event(hsb, 1, true), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(1 == lsb_heka_timer_event(hsb, 2, false), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_analysis(NULL, "lua/input.lua", NULL, NULL, NULL, aim);
  mu_assert(hsb, "lsb_heka_create_analysis succeeded");
  mu_assert_rv(1, lsb_heka_timer_event(hsb, 0, false));
  const char *err = lsb_heka_get_error(hsb);
  mu_assert(strcmp("timer_event() function was not found", err) == 0,
            "received: %s", err);
  lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_pm_input()
{
  struct pm_result {
    double      ncp;
    const char  *scp;
    int         rv;
    const char  *err;
  };

  struct pm_result results[] = {
    { .ncp = 0, .scp = NULL, .rv = -2, .err = "host specific failure" },
    { .ncp = 1, .scp = NULL, .rv = -1, .err = "failed" },
    { .ncp = 2, .scp = NULL, .rv = 0,  .err = "ok" },
    { .ncp = NAN, .scp = "string", .rv = 0,  .err = "string" },
    { .ncp = NAN, .scp = NULL, .rv = 0,  .err = "no cp" },
  };

  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, dlog, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  for (unsigned i = 0; i < sizeof results / sizeof results[0]; ++i){
    int rv = lsb_heka_process_message_input(hsb, &m, results[i].ncp,
                                            results[i].scp, false);
    const char *err = lsb_heka_get_error(hsb);
    mu_assert(strcmp(results[i].err, err) == 0, "expected: %s received: %s",
              results[i].err, err);
    mu_assert(results[i].rv == rv, "test: %u expected: %d received: %d", i,
              results[i].rv,
              rv);
  }
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_pm_analysis()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, dlog,
                                 aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  mu_assert_rv(0, lsb_heka_process_message_analysis(hsb, &m, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_pm_no_return()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/pm_no_return.lua", NULL, NULL, dlog,
                                 aim);
  mu_assert_rv(1, lsb_heka_process_message_analysis(hsb, &m, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "process_message() must return a numeric status code";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_pm_output()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/output.lua", NULL, NULL, dlog, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");

  mu_assert_rv(0, lsb_heka_process_message_output(hsb, &m, NULL, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);

  mu_assert_rv(-5, lsb_heka_process_message_output(hsb, &m, (void *)99, false));
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);

  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_im_input()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/iim.lua", NULL,
                              "path = [[" TEST_LUA_PATH "]]\n"
                              "cpath = [[" TEST_LUA_CPATH "]]\n", dlog, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_im_analysis()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/aim.lua", NULL,
                                 "Hostname = 'foo.com';Logger = 'aim'", dlog,
                                 aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_encode_message()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/encode_message.lua", NULL,
                               "Hostname = 'sh';Logger = 'sl'", dlog, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");
  lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_decode_message()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/decode_message.lua", NULL, NULL,
                               dlog, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_read_message()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof pb - 1, dlog), "failed");

  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/read_message.lua", NULL, NULL, dlog,
                                 aim);
  mu_assert(hsb, "lsb_heka_create_analysist failed");
  int rv = lsb_heka_process_message_analysis(hsb, &m, false);
  mu_assert(0 == rv, "expected: %d received: %d %s", 0, rv,
            lsb_heka_get_error(hsb));
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* benchmark_decode_message()
{
  int iter = 100000;

  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/decode_message_benchmark.lua", NULL,
                               NULL, dlog, ucp);

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == lsb_heka_process_message_output(hsb, &m, NULL, false), "%s",
              lsb_heka_get_error(hsb));
  }
  t = clock() - t;
  printf("benchmark_decode_message() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  mu_assert(hsb, "lsb_heka_create_output failed");
  lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_create_input_sandbox);
  mu_run_test(test_create_analysis_sandbox);
  mu_run_test(test_create_output_sandbox);
  mu_run_test(test_timer_event);
  mu_run_test(test_pm_input);
  mu_run_test(test_pm_analysis);
  mu_run_test(test_pm_no_return);
  mu_run_test(test_pm_output);
  mu_run_test(test_im_input);
  mu_run_test(test_im_analysis);
  mu_run_test(test_encode_message);
  mu_run_test(test_decode_message);
  mu_run_test(test_read_message);

  mu_run_test(benchmark_decode_message);
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

  return result != 0;
}
