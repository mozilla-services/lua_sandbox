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


static char* test_lsb_timespec_delta()
{
  double d;
  d = lsb_timespec_delta(
      &(struct timespec){ .tv_sec = 10, .tv_nsec = 600000000L },
      &(struct timespec){ .tv_sec = 11, .tv_nsec = 800000000L });
  mu_assert(d == 1.2, "received: %g", d);

  d = lsb_timespec_delta(
      &(struct timespec){ .tv_sec = 10, .tv_nsec = 600000000L },
      &(struct timespec){ .tv_sec = 12, .tv_nsec = 300000000L });
  mu_assert(d == 1.7, "received: %g", d);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_lsb_lp2);
  mu_run_test(test_lsb_timespec_delta);
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
