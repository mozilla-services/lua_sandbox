/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_input_buffer unit tests @file */

#include <math.h>

#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/running_stats.h"

#ifdef _MSC_VER
// To silence the +/-INFINITY warning
#pragma warning( disable : 4756 )
#pragma warning( disable : 4056 )
#endif

static char* test_stub()
{
  return NULL;
}


static char* test_init()
{
  lsb_running_stats stats;
  lsb_init_running_stats(&stats);
  mu_assert(stats.count == 0, "received: %g", stats.count);
  mu_assert(stats.mean  == 0, "received: %g", stats.mean);
  mu_assert(stats.sum == 0, "received: %g", stats.sum);
  return NULL;
}


static char* test_calculation()
{
  lsb_running_stats stats;
  lsb_init_running_stats(&stats);
  double sd = lsb_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  lsb_update_running_stats(&stats, 1.0);
  lsb_update_running_stats(&stats, 2.0);
  lsb_update_running_stats(&stats, 3.0);
  mu_assert(stats.count == 3, "received: %g", stats.count);
  mu_assert(stats.mean  == 2, "received: %g", stats.mean);
  sd = lsb_sd_running_stats(&stats);
  mu_assert(sd == 1.0, "received: %g", sd);
  return NULL;
}


static char* test_nan_inf()
{
  lsb_running_stats stats;
  lsb_init_running_stats(&stats);
  double sd = lsb_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  lsb_update_running_stats(&stats, INFINITY);
  lsb_update_running_stats(&stats, NAN);
  lsb_update_running_stats(&stats, -INFINITY);
  mu_assert(stats.count == 0, "received: %g", stats.count);
  sd = lsb_sd_running_stats(&stats);
  mu_assert(sd == 0, "received: %g", sd);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_init);
  mu_run_test(test_calculation);
  mu_run_test(test_nan_inf);
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
  return result != NULL;
}
