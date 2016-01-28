/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_util unit tests @file */

#include <stdio.h>

#include "../../test/mu_test.h"
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


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_lsb_lp2);

  mu_run_test(benchmark_lsb_get_time);
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
