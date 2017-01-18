/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Unit test macros and global data @file */

#ifndef luasandbox_test_mu_test_h_
#define luasandbox_test_mu_test_h_

#include <stdio.h>

#ifdef _WIN32
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif

#if defined(_MSC_VER)
#define PRIuSIZE "Iu"
#else
#define PRIuSIZE "zu"
#endif

#define MU_ERR_LEN 1024

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

#define mu_assert_rv(rv, fn)                                                   \
do {                                                                           \
  int result = fn;                                                             \
  if (rv != result) {                                                          \
    int cnt = snprintf(mu_err, MU_ERR_LEN, "line: %d %s expected: %d "         \
    " received: %d", __LINE__, #fn, rv, result);                               \
    if (cnt > 0 && cnt < MU_ERR_LEN) {                                         \
      return mu_err;                                                           \
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

int mu_tests_run = 0;
char mu_err[MU_ERR_LEN] = { 0 };

#endif
