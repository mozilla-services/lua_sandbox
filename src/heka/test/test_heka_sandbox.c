/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Heka sandbox unit tests @file */

#include "test.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sandbox_impl.h"
#include "luasandbox/heka/sandbox.h"
#include "luasandbox_output.h"

static unsigned long long clockres = 1;

// {Uuid="" Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}
static char pb[] = "\x0a\x10" "abcdefghijklmnop" "\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33";

char *e = NULL;

void dlog(void *context, const char *component, int level, const char *fmt, ...)
{
  (void)context;
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%lld [%d] %s ", (long long)time(NULL), level,
          component ? component : "unnamed");
  vfprintf(stderr, fmt, args);
  fwrite("\n", 1, 1, stderr);
  va_end(args);
}
static lsb_logger logger = { .context = NULL, .cb = dlog };


static int iim(void *parent, const char *pb, size_t pb_len, double cp_numeric,
               const char *cp_string)
{
  if (!pb) {
    return LSB_HEKA_IM_SUCCESS;
  }

  static int cnt = 0;
  struct im_result {
    const char  *pb;
    size_t      pb_len;
    double      cp_numeric;
    const char  *cp_string;
  };

  struct im_result results[] = {
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x22\x03iim\x40\x00\x4a\x02hn", .pb_len = 31, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x01\x22\x03iim\x40\x00\x4a\x02hn", .pb_len = 31, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x02\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x05\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0a\x65\x6e\x76\x76\x65\x72\x73\x69\x6f\x6e\x40\x63\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65", .pb_len = 69, .cp_numeric = 99, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x03\x22\x03iim\x40\x00\x4a\x02hn\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33", .pb_len = 167, .cp_numeric = NAN, .cp_string = "foo.log:123" },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x04", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x05", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x05", .pb_len = 20, .cp_numeric = NAN, .cp_string = NULL },
  };

  if (cnt >= (int)(sizeof results / sizeof results[0])) {
    fprintf(stderr, "tests and results are mis-matched\n");
    return LSB_HEKA_IM_ERROR;
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
    return LSB_HEKA_IM_CHECKPOINT;
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
    return LSB_HEKA_IM_CHECKPOINT;
  }
  cnt++;
  return LSB_HEKA_IM_SUCCESS;
}


static int aim(void *parent, const char *pb, size_t pb_len)
{
  static int offset = 28; // skip Uuid and Timestamp
  static int cnt = 0;
  struct im_result {
    const char  *pb;
    size_t      pb_len;
    double      cp_numeric;
    const char  *cp_string;
  };

  struct im_result results[] = {
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x8e\xa8\xf3\xde\x88\xb5\xb7\x93\x15\x22\x03\x61\x69\x6d\x40\x00\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d", .pb_len = 44, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\xbf\x97\x9c\xcc\xbe\xc2\xb7\x93\x15\x22\x03\x61\x69\x6d\x40\x00\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d\x00\x00\x00\x00\x0a\x10\x00\x00", .pb_len = 44, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\xea\x95\xd4\xfc\x7c\x10\x40\x95\xa8\x17\xcb\x56\x26\x91\x8c\x47\x10\xba\xf6\xd5\xf9\xfd\xce\xb7\x93\x15\x1a\x0e\x69\x6e\x6a\x65\x63\x74\x5f\x70\x61\x79\x6c\x6f\x61\x64\x22\x03\x61\x69\x6d\x32\x07\x66\x6f\x6f\x20\x62\x61\x72\x40\x00\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d\x52\x13\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x74\x79\x70\x65\x22\x03\x74\x78\x74", .pb_len = 90, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\xfd\x49\x92\x77\x02\x37\x4b\xf0\xaf\x86\x6f\x9b\x80\x26\xf4\x35\x10\xaf\xec\x9e\xa4\xd8\xcf\xb7\x93\x15\x1a\x0e\x69\x6e\x6a\x65\x63\x74\x5f\x70\x61\x79\x6c\x6f\x61\x64\x22\x03\x61\x69\x6d\x32\x07\x66\x6f\x6f\x20\x62\x61\x72\x40\x00\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d\x52\x13\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x74\x79\x70\x65\x22\x03\x64\x61\x74\x52\x14\x0a\x0c\x70\x61\x79\x6c\x6f\x61\x64\x5f\x6e\x61\x6d\x65\x22\x04\x74\x65\x73\x74", .pb_len = 112, .cp_numeric = NAN, .cp_string = NULL },
    { .pb = "\x0a\x10\x7c\x32\xd6\x23\x98\xe8\x49\x9e\xa2\xe8\x0d\x78\x84\x8e\x75\xb2\x10\xf7\xf5\xdb\x89\x88\xe4\xb7\x93\x15\x22\x03\x61\x69\x6d\x40\x00\x4a\x07\x66\x6f\x6f\x2e\x63\x6f\x6d", .pb_len = 0, .cp_numeric = NAN, .cp_string = NULL },  }; // intentionally fail on size to to test the custom return value

  if (cnt >= (int)(sizeof results / sizeof results[0])) {
    fprintf(stderr, "tests and results are mis-matched\n");
    return LSB_HEKA_IM_LIMIT;
  }

  if (parent) {
    fprintf(stderr, "test: %d parent set\n", cnt);
  }

  if (pb_len != results[cnt].pb_len) {
    fprintf(stderr, "test: %d pb len expected: %" PRIuSIZE " received: %"
            PRIuSIZE "\n", cnt, results[cnt].pb_len, pb_len);
    cnt++;
    return 99;
  }

  if (memcmp(pb + offset, results[cnt].pb + offset, pb_len - offset)) {
    fprintf(stderr, "test: %d\nexpected: ", cnt);
    for (size_t i = offset; i < results[cnt].pb_len; ++i) {
      fprintf(stderr, "\\x%02hhx", results[cnt].pb[i]);
    }
    fprintf(stderr, "\nreceived: ");
    for (size_t i = offset; i < pb_len; ++i) {
      fprintf(stderr, "\\x%02hhx", pb[i]);
    }
    fprintf(stderr, "\n");
    return 1;
  }
  cnt++;
  return LSB_HEKA_IM_SUCCESS;
}

static int aim1(void *parent, const char *pb, size_t pb_len)
{
  if (parent) {
    fprintf(stderr, "parent set\n");
    return 1;
  }
  if (!pb) {
    fprintf(stderr, "pb null set\n");
    return 1;
  }
  size_t expected = 308;
  if (pb_len != expected) {
    fprintf(stderr, "pb_len expected: %" PRIuSIZE " received: %" PRIuSIZE "\n",
            expected, pb_len);

    for (size_t i = 0; i < pb_len; ++i) {
      if (isprint(pb[i])) {
        fprintf(stderr, "%c", pb[i]);
      } else {
        fprintf(stderr, "\\x%02hhx", pb[i]);
      }
    }
    fprintf(stderr, "\n");
    return 1;
  }
  return LSB_HEKA_IM_SUCCESS;
}


static int ucp(void *parent, void *sequence_id)
{
  static int cnt = 0;
  if (parent) return 1;
  void *results[] = { NULL, (void *)99, (void *)99 };

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


static char* test_api_assertion()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");

  lsb_heka_sandbox * isb,*asb,*osb;
  isb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, &logger, iim);
  asb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, &logger, aim);
  osb = lsb_heka_create_output(NULL, "lua/output.lua", NULL, NULL, &logger, ucp);

  mu_assert(isb, "lsb_heka_create_input failed");
  mu_assert_rv(1, lsb_heka_pm_input(NULL, 0, NULL, false));
  mu_assert_rv(1, lsb_heka_timer_event(isb, 0, false));

  mu_assert(isb, "lsb_heka_create_analysis failed");
  mu_assert_rv(1, lsb_heka_pm_analysis(NULL, NULL, false));
  mu_assert_rv(1, lsb_heka_pm_analysis(asb, NULL, false));
  mu_assert_rv(1, lsb_heka_timer_event(NULL, 0, false));
  mu_assert_rv(1, lsb_heka_pm_analysis(isb, &m, false));

  mu_assert(isb, "lsb_heka_create_output failed");
  mu_assert_rv(1, lsb_heka_pm_output(NULL, NULL, NULL, false));
  mu_assert_rv(1, lsb_heka_pm_output(osb, NULL, NULL, false));
  mu_assert_rv(1, lsb_heka_pm_output(asb, &m, NULL, false));

  lsb_heka_destroy_sandbox(isb);
  lsb_heka_destroy_sandbox(asb);
  lsb_heka_destroy_sandbox(osb);
  lsb_free_heka_message(&m);

  mu_assert(strcmp(lsb_heka_get_error(NULL), "") == 0, "not empty");
  mu_assert(lsb_heka_get_lua_file(NULL) == NULL, "not null");
  lsb_heka_stats stats = lsb_heka_get_stats(NULL);
  mu_assert(0 == stats.mem_cur, "received: %llu", stats.mem_cur);
  mu_assert(!lsb_heka_is_running(NULL), "running is true");
  int state = lsb_heka_get_state(NULL);
  mu_assert(LSB_UNKNOWN == state, "received: %d", state);
  return NULL;
}


static char* test_create_input_sandbox()
{
  static const char *lua_file = "lua/input.lua";
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, lua_file, NULL, NULL, &logger, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  const char *lfn = lsb_heka_get_lua_file(hsb);
  mu_assert(strcmp(lua_file, lfn) == 0, "expected %s received %s", lua_file,
            lfn);
  e = lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_input(NULL, "notfourd.lua", NULL, NULL, NULL,
                              iim);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");

  hsb = lsb_heka_create_input(NULL, NULL, NULL, NULL, NULL, iim);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");

  hsb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, NULL, NULL);
  mu_assert(!hsb, "lsb_heka_create_input succeeded");
  e = lsb_heka_destroy_sandbox(hsb); // test NULL

  hsb = lsb_heka_create_input(NULL, lua_file, NULL, NULL, &logger, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  lsb_heka_terminate_sandbox(hsb, "boom");
  const char *err =  lsb_heka_get_error(hsb);
  mu_assert(strcmp("boom", err) == 0, "received: %s", err);
  e = lsb_heka_destroy_sandbox(hsb);
  mu_assert(!e, "received %s", e);
  return NULL;
}


static char* test_create_analysis_sandbox()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, &logger, aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  e = lsb_heka_destroy_sandbox(hsb);

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
  hsb = lsb_heka_create_output(NULL, "lua/output.lua", NULL, NULL, &logger, ucp);
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
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, &logger, aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(0 < stats.mem_cur, "received %llu", stats.mem_cur);
  mu_assert(0 < stats.mem_max, "received %llu", stats.mem_max);
  mu_assert(0 == stats.out_max, "received %llu", stats.out_max);
  mu_assert(0 < stats.ins_max, "received %llu", stats.ins_max);
  mu_assert(0 == stats.pm_cnt, "received %llu", stats.pm_cnt);
  mu_assert(0 == stats.pm_failures, "received %llu", stats.pm_failures);
  mu_assert(0 == stats.pm_avg, "received %g", stats.pm_avg);
  mu_assert(0 == stats.pm_sd, "received %g", stats.pm_sd);
  mu_assert(0 == stats.te_avg, "received %g", stats.te_avg);
  mu_assert(0 == stats.te_sd, "received %g", stats.te_sd);
  mu_assert(true == lsb_heka_is_running(hsb), "not running");
  int state = lsb_heka_get_state(hsb);
  mu_assert(LSB_RUNNING == state, "received: %d", state);

  mu_assert(0 == lsb_heka_timer_event(hsb, 0, false), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(0 == lsb_heka_timer_event(hsb, 1, true), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(1 == lsb_heka_timer_event(hsb, 2, false), "err: %s",
            lsb_heka_get_error(hsb));
  mu_assert(false == lsb_heka_is_running(hsb), "not running");
  state = lsb_heka_get_state(hsb);
  mu_assert(LSB_TERMINATED == state, "received: %d", state);

  stats = lsb_heka_get_stats(hsb);
  mu_assert(0 == stats.im_cnt, "received %llu", stats.im_cnt);
  mu_assert(0 == stats.im_bytes, "received %llu", stats.im_bytes);
  mu_assert(0 == stats.pm_cnt, "received %llu", stats.pm_cnt);
  mu_assert(0 == stats.pm_failures, "received %llu", stats.pm_failures);
  mu_assert(0 == stats.pm_avg, "received %g", stats.pm_avg);
  mu_assert(0 == stats.pm_sd, "received %g", stats.pm_sd);
  if (clockres <= 100) {
    mu_assert(0 < stats.te_avg, "received %g res %llu", stats.te_avg, clockres);
    mu_assert(0 < stats.te_sd, "received %g", stats.te_sd);
  }

  e = lsb_heka_destroy_sandbox(hsb);

  hsb = lsb_heka_create_analysis(NULL, "lua/pm_no_return.lua", NULL, NULL, NULL, aim);
  mu_assert(hsb, "lsb_heka_create_analysis succeeded");
  mu_assert_rv(1, lsb_heka_timer_event(hsb, 0, false));
  const char *err = lsb_heka_get_error(hsb);
  mu_assert(strcmp("timer_event() function was not found", err) == 0,
            "received: %s", err);
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_clean_stop_input()
{
  static const char *state_file = "stop.data";
  remove(state_file);
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/input.lua", state_file, NULL, &logger, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");

  mu_assert(true == lsb_heka_is_running(hsb), "running");
  int rv = lsb_heka_pm_input(hsb, 8, NULL, true);
  const char *err = lsb_heka_get_error(hsb);
  mu_assert(rv == 0, "error: %s expected: %d received: %d", err, 0, rv);
  mu_assert(true == lsb_heka_is_running(hsb), "running");

  lsb_heka_stop_sandbox_clean(hsb);
  rv = lsb_heka_pm_input(hsb, 9, NULL, true);
  err = lsb_heka_get_error(hsb);
  mu_assert(rv == 0, "error: %s expected: %d received: %d", err, 0, rv);
  mu_assert(false == lsb_heka_is_running(hsb), "not running");

  e = lsb_heka_destroy_sandbox(hsb);
  mu_assert(!e, "received %s", e);
  return NULL;
}


static char* test_stop_input()
{
  static const char *state_file = "stop.data";
  remove(state_file);
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/input.lua", state_file, NULL, &logger, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  lsb_heka_stop_sandbox(hsb);
  e = lsb_heka_destroy_sandbox(hsb);
  mu_assert(!e, "received %s", e);
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

  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, &logger, iim);
  mu_assert(hsb, "lsb_heka_create_input failed");
  for (unsigned i = 0; i < sizeof results / sizeof results[0];++i){
    int rv = lsb_heka_pm_input(hsb, results[i].ncp, results[i].scp, true);
    const char *err = lsb_heka_get_error(hsb);
    mu_assert(strcmp(results[i].err, err) == 0, "expected: %s received: %s",
              results[i].err, err);
    mu_assert(results[i].rv == rv, "test: %u expected: %d received: %d", i,
              results[i].rv,
              rv);
  }
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(5 == stats.pm_cnt, "expected %llu", stats.pm_cnt);
  mu_assert(1 == stats.pm_failures, "expected %llu", stats.pm_failures);
  mu_assert(0 < stats.mem_cur, "expected %llu", stats.mem_cur);
  if (clockres <= 100) {
    mu_assert(0 < stats.pm_avg, "received %g res %llu", stats.pm_avg, clockres);
    mu_assert(0 < stats.pm_sd, "received %g", stats.pm_sd);
  }
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}

static char* test_pm_error()
{
  struct pm_result {
    double      ncp;
    const char  *scp;
    int         rv;
    const char  *err;
  };

  struct pm_result results[] = {
    { .ncp = 3, .scp = NULL, .rv = 1,  .err = "process_message() lua/input.lua:39: boom" },
    { .ncp = 4, .scp = NULL, .rv = 1,  .err = "process_message() must return a nil or string error message" },
    { .ncp = 5, .scp = NULL, .rv = 1,  .err = "process_message() must return a numeric status code" },
    { .ncp = 6, .scp = NULL, .rv = 1,  .err = "process_message() lua/input.lua:45: aaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    }, // >max error message
    { .ncp = 7, .scp = NULL, .rv = 1,  .err = "process_message() <nil error message>" },
  };

  lsb_heka_sandbox *hsb;
  for (unsigned i = 0; i < sizeof results / sizeof results[0];++i){
    hsb = lsb_heka_create_input(NULL, "lua/input.lua", NULL, NULL, &logger, iim);
    mu_assert(hsb, "lsb_heka_create_input failed");
    int rv = lsb_heka_pm_input(hsb, results[i].ncp, results[i].scp, true);
    const char *err = lsb_heka_get_error(hsb);
    mu_assert(strcmp(results[i].err, err) == 0, "expected: %s received: %s",
              results[i].err, err);
    mu_assert(results[i].rv == rv, "test: %u expected: %d received: %d", i,
              results[i].rv,
              rv);
    e = lsb_heka_destroy_sandbox(hsb);
  }
  return NULL;
}


static char* test_pm_analysis()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/analysis.lua", NULL, NULL, &logger, aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  mu_assert_rv(0, lsb_heka_pm_analysis(hsb, &m, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(1 == stats.pm_cnt, "expected %llu", stats.pm_cnt);
  mu_assert(0 == stats.pm_failures, "expected %llu", stats.pm_failures);
  mu_assert(0 == stats.pm_avg, "received %g", stats.pm_avg);
  mu_assert(0 == stats.pm_sd, "received %g", stats.pm_sd);
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_pm_no_return()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/pm_no_return.lua", NULL, NULL, &logger, aim);
  mu_assert_rv(1, lsb_heka_pm_analysis(hsb, &m, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "process_message() must return a numeric status code";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_pm_output()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof(pb) - 1, &logger), "failed");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/output.lua", NULL,
                               "path = [[" TEST_LUA_PATH "]]\n"
                               "cpath = [[" TEST_LUA_CPATH "]]\n", &logger, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");

  mu_assert_rv(0, lsb_heka_pm_output(hsb, &m, NULL, false));
  const char *err = lsb_heka_get_error(hsb);
  const char *eerr = "";
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(0 == stats.pm_failures, "expected %llu", stats.pm_failures);

  mu_assert_rv(-5, lsb_heka_pm_output(hsb, &m, (void *)99, false));
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);

  stats = lsb_heka_get_stats(hsb);
  mu_assert(2 == stats.pm_cnt, "expected %llu", stats.pm_cnt);
  mu_assert(7 == stats.pm_failures, "expected %llu", stats.pm_failures);

  mu_assert_rv(-3, lsb_heka_pm_output(hsb, &m, (void *)100, false));
  mu_assert(strcmp(eerr, err) == 0, "expected: %s received: %s", eerr, err);
  stats = lsb_heka_get_stats(hsb);
  mu_assert(2 == stats.pm_cnt, "expected %llu", stats.pm_cnt);
  mu_assert(7 == stats.pm_failures, "expected %llu", stats.pm_failures);

  mu_assert_rv(0, lsb_heka_timer_event(hsb, 0, false));

  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_im_input()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_input(NULL, "lua/iim.lua", NULL,
                              "path = [[" TEST_LUA_PATH "]]\n"
                              "cpath = [[" TEST_LUA_CPATH "]]\n"
                              "Hostname = 'hn'\n"
                              "Logger = 'iim'\n",
                              &logger, iim);
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(6 == stats.im_cnt, "received %llu", stats.im_cnt);
  mu_assert(338 == stats.im_bytes, "received %llu", stats.im_bytes);
  mu_assert(hsb, "lsb_heka_create_input failed");
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_im_analysis()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/aim.lua", NULL,
                                 "Hostname = 'foo.com';Logger = 'aim'",
                                 &logger, aim);
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(4 == stats.im_cnt, "expected %llu", stats.im_cnt);
  mu_assert(290 == stats.im_bytes, "expected %llu", stats.im_bytes);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_encode_message()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/encode_message.lua", NULL,
                               "path = [[" TEST_LUA_PATH "]]\n"
                               "cpath = [[" TEST_LUA_CPATH "]]\n"
                               "Hostname = 'sh'\n"
                               "Logger = 'sl'\n"
                               "Pid = 0\n",
                               &logger, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");
  lsb_heka_stats stats = lsb_heka_get_stats(hsb);
  mu_assert(164 == stats.out_max, "received %llu", stats.out_max);
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_decode_message()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof(pb) - 1, &logger), "failed");

  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/decode_message.lua", NULL, NULL, &logger, ucp);
  mu_assert(hsb, "lsb_heka_create_output failed");
  int rv = lsb_heka_pm_output(hsb, &m, NULL, false);
  mu_assert(0 == rv, "expected: %d received: %d %s", 0, rv,
            lsb_heka_get_error(hsb));
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_read_message()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof(pb) - 1, &logger), "failed");

  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/read_message.lua", NULL, NULL, &logger, aim);
  mu_assert(hsb, "lsb_heka_create_analysis failed");
  int rv = lsb_heka_pm_analysis(hsb, &m, false);
  mu_assert(0 == rv, "expected: %d received: %d %s", 0, rv,
            lsb_heka_get_error(hsb));
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static int rm_zc(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 1, n, "incorrect number of arguments");
  luaL_checktype(lua, 1, LUA_TUSERDATA);
  lua_CFunction fp = lsb_get_zero_copy_function(lua, 1);
  if (!fp) {
    return luaL_argerror(lua, 1, "no zero copy support");
  }
  int results = fp(lua);
  int start = n + 1;
  int end = start + results;
  int cnt = 0;
  for (int i = start; i < end; ++i) {
    switch (lua_type(lua, i)) {
    case LUA_TSTRING:
      lua_pushvalue(lua, i);
      ++cnt;
      break;
    case LUA_TLIGHTUSERDATA:
      {
        const char *s = lua_touserdata(lua, i++);
        size_t len = (size_t)lua_tointeger(lua, i);
        if (s && len > 0) {
          lua_pushlstring(lua, s, len);
          ++cnt;
        }
      }
      break;
    default:
      return luaL_error(lua, "invalid zero copy return");
    }
  }
  if (cnt) {
    lua_concat(lua, cnt);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}


static char* test_read_message_zc()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof(pb) - 1, &logger), "failed");

  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/read_message_zc.lua", NULL, "Hostname = 'ubuntu'", &logger, aim1);
  mu_assert(hsb, "lsb_heka_create_analysis failed");

  lsb_add_function(hsb->lsb, rm_zc, "read_message_zc");
  int rv = lsb_heka_pm_analysis(hsb, &m, false);
  mu_assert(0 == rv, "expected: %d received: %d %s", 0, rv,
            lsb_heka_get_error(hsb));
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_get_message()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/read_message.lua", NULL, NULL, &logger, aim);
  mu_assert(lsb_heka_get_message(NULL) == NULL, "non NULL");
  mu_assert(lsb_heka_get_message(hsb) == NULL, "non NULL");
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* test_get_type()
{
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_analysis(NULL, "lua/read_message.lua", NULL, NULL, &logger, aim);
  char t = lsb_heka_get_type(NULL);
  mu_assert(t == '\0', "received %c", t);
  t = lsb_heka_get_type(hsb);
  mu_assert(t == 'a', "received %c", t);
  e = lsb_heka_destroy_sandbox(hsb);
  return NULL;
}


static char* benchmark_decode_message()
{
  int iter = 100000;

  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed to init message");
  lsb_heka_sandbox *hsb;
  hsb = lsb_heka_create_output(NULL, "lua/decode_message_benchmark.lua", NULL,
                               NULL, &logger, ucp);
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    mu_assert(0 == lsb_heka_pm_output(hsb, &m, NULL, false), "%s",
              lsb_heka_get_error(hsb));
  }
  t = clock() - t;
  printf("benchmark_decode_message() %g seconds\n", ((double)t)
         / CLOCKS_PER_SEC / iter);

  mu_assert(hsb, "lsb_heka_create_output failed");
  e = lsb_heka_destroy_sandbox(hsb);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* all_tests()
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec ts;
  clock_getres(CLOCK_MONOTONIC, &ts);
  clockres = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif

  mu_run_test(test_api_assertion);
  mu_run_test(test_create_input_sandbox);
  mu_run_test(test_create_analysis_sandbox);
  mu_run_test(test_create_output_sandbox);
  mu_run_test(test_timer_event);
  mu_run_test(test_clean_stop_input);
  mu_run_test(test_stop_input);
  mu_run_test(test_pm_input);
  mu_run_test(test_pm_error);
  mu_run_test(test_pm_analysis);
  mu_run_test(test_pm_no_return);
  mu_run_test(test_pm_output);
  mu_run_test(test_im_input);
  mu_run_test(test_im_analysis);
  mu_run_test(test_encode_message);
  mu_run_test(test_decode_message);
  mu_run_test(test_read_message);
  mu_run_test(test_read_message_zc);
  mu_run_test(test_get_message);
  mu_run_test(test_get_type);

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
