/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_util unit tests @file */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "luasandbox/error.h"
#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/util.h"

static char* test_stub()
{
  return NULL;
}


static char* test_lsb_lp2()
{
  size_t lp2;

  lp2 = lsb_lp2(0);
  mu_assert(lp2 == 0, "received: %" PRIuSIZE, lp2);

  lp2 = lsb_lp2(1);
  mu_assert(lp2 == 1, "received: %" PRIuSIZE, lp2);

  lp2 = lsb_lp2(1000);
  mu_assert(lp2 == 1024, "received: %" PRIuSIZE, lp2);
  return NULL;
}


static char* test_lsb_read_file()
{
  char *s = lsb_read_file("Makefile");
  mu_assert(s, "read file failed");
  free(s);

  s = lsb_read_file("_foo_bar_");
  free(s); // if it succeeded don't leak
  mu_assert(!s, "read file succeeded");
  return NULL;
}


static char* test_lsb_set_tz()
{
  mu_assert(lsb_set_tz(NULL), "set_tz failed");
  mu_assert(lsb_set_tz("America/Los_Angeles"), "set_tz failed");
  static const char too_long[] = "01234567890123456789012345678901";
#ifdef _WIN32
  mu_assert(!lsb_set_tz(too_long), "set_tz succeeded");
#else
  mu_assert(lsb_set_tz(too_long), "set_tz failed");
#endif
  return NULL;
}


static char* benchmark_lsb_get_time()
{
  int iter = 1000000;
  unsigned long long start, end;
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_get_time();
  }
  t = clock() - t;
  printf("benchmark_lsb_get_time(%d) - clock %g seconds\n", iter,
         (double)t / CLOCKS_PER_SEC / iter);

  start = lsb_get_time();
  for (int x = 0; x < iter; ++x) {
    lsb_get_time();
  }
  end = lsb_get_time();
  printf("benchmark_lsb_get_time(%d) - self %g seconds\n", iter,
         (double)(end - start) / iter / 1e9);

  start = lsb_get_time();
  lsb_get_time();
  end = lsb_get_time();
  printf("benchmark_lsb_get_time(1) %llu\n", end - start);
  return NULL;
}


static char* benchmark_lsb_get_timestamp()
{
  int iter = 1000000;
  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_get_timestamp();
  }
  t = clock() - t;
  printf("benchmark_lsb_get_timestamp(%d) - clock %g seconds\n", iter,
         (double)t / CLOCKS_PER_SEC / iter);
  long long delta = lsb_get_timestamp() / 1000000000LL - time(NULL);
  mu_assert(delta > 0 ? delta <= 1 : delta >= -1, "delta %lld", delta);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_lsb_lp2);
  mu_run_test(test_lsb_read_file);
  mu_run_test(test_lsb_set_tz);

  mu_run_test(benchmark_lsb_get_time);
  mu_run_test(benchmark_lsb_get_timestamp);
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
  return result != NULL;
}
