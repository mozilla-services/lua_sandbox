/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_input_buffer unit tests @file */

#include <stdio.h>
#include <string.h>

#include "luasandbox/error.h"
#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/input_buffer.h"

static char* test_stub()
{
  return NULL;
}

static char* test_init_small_buf()
{
  size_t size = 100;
  lsb_input_buffer b;

  lsb_err_value ret = lsb_init_input_buffer(NULL, size);
  mu_assert(ret == LSB_ERR_UTIL_NULL, "received: %s", lsb_err_string(ret));

  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  mu_assert(b.size == size + LSB_MAX_HDR_SIZE, "received: %" PRIuSIZE,
            b.size);
  mu_assert(b.maxsize == size + LSB_MAX_HDR_SIZE, "received: %" PRIuSIZE,
            b.size);
  lsb_free_input_buffer(&b);
  lsb_free_input_buffer(NULL);
  return NULL;
}


static char* test_init_large_buf()
{
  size_t size = 1024 * 1024;
  lsb_input_buffer b;
  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  mu_assert(b.size == BUFSIZ, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size + LSB_MAX_HDR_SIZE, "received: %" PRIuSIZE,
            b.size);
  lsb_free_input_buffer(&b);
  return NULL;
}


static char* test_init_zero_buf()
{
  lsb_input_buffer b;
  lsb_err_value ret = lsb_init_input_buffer(&b, 0);
  mu_assert(ret == LSB_ERR_UTIL_PRANGE, "received: %s", lsb_err_string(ret));
  lsb_free_input_buffer(&b);
  return NULL;
}


static char* test_expand_buf()
{
  size_t size = 1024 * 1024;
  size_t rsize = 1024 * 16;
  lsb_input_buffer b;
  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  mu_assert(b.size == BUFSIZ, "received: %" PRIuSIZE, b.size);

  lsb_err_value ret = lsb_expand_input_buffer(NULL, 0);
  mu_assert(LSB_ERR_UTIL_NULL == ret, "received: %s", lsb_err_string(ret));

  mu_assert(!lsb_expand_input_buffer(&b, 1024 * 9), "expand failed");
  mu_assert(b.size == rsize, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size + LSB_MAX_HDR_SIZE, "received: %" PRIuSIZE,
            b.size);

  mu_assert(!lsb_expand_input_buffer(&b, 1024), "expand failed");
  mu_assert(b.size == rsize, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size + LSB_MAX_HDR_SIZE, "received: %" PRIuSIZE,
            b.size);

  lsb_free_input_buffer(&b);
  return NULL;
}


static char* test_expand_failure()
{
  size_t size = 1024;
  lsb_input_buffer b;
  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  lsb_err_value ret = lsb_expand_input_buffer(&b, size  + LSB_MAX_HDR_SIZE + 1);
  mu_assert(LSB_ERR_UTIL_FULL == ret, "received: %s", lsb_err_string(ret));
  lsb_free_input_buffer(&b);
  return NULL;
}


static char* test_shift_empty()
{
  size_t size = 16;
  lsb_input_buffer b;
  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  b.readpos = 10;
  b.scanpos = 10;
  mu_assert(!lsb_expand_input_buffer(&b, 1), "expand failed");
  mu_assert(b.scanpos == 0, "received: %" PRIuSIZE, b.scanpos);
  mu_assert(b.readpos == 0, "received: %" PRIuSIZE, b.readpos);
  lsb_free_input_buffer(&b);
  return NULL;
}


static char* test_shift_partial()
{
  size_t size = 16;
  lsb_input_buffer b;
  mu_assert(!lsb_init_input_buffer(&b, size), "init failed");
  memset(b.buf, '#', size);
  b.readpos = 10;
  b.scanpos = 9;
  b.buf[b.scanpos] = 'A';
  mu_assert(!lsb_expand_input_buffer(&b, 1), "expand failed");
  mu_assert(b.scanpos == 0, "received: %" PRIuSIZE, b.scanpos);
  mu_assert(b.readpos == 1, "received: %" PRIuSIZE, b.readpos);
  mu_assert(b.buf[0] == 'A', "received: %c", b.buf[0]);
  lsb_free_input_buffer(&b);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_init_small_buf);
  mu_run_test(test_init_large_buf);
  mu_run_test(test_init_zero_buf);
  mu_run_test(test_expand_buf);
  mu_run_test(test_expand_failure);
  mu_run_test(test_shift_empty);
  mu_run_test(test_shift_partial);
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
