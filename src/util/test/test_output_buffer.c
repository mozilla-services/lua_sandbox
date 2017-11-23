/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_output_buffer unit tests @file */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/output_buffer.h"
#include "luasandbox/util/heka_message.h"

#ifdef _MSC_VER
// To silence the +/-INFINITY warning
#pragma warning( disable : 4756 )
#pragma warning( disable : 4056 )
#endif

static char* test_stub()
{
  return NULL;
}


static char* test_init_small_buf()
{
  size_t size = 512;
  lsb_output_buffer b;

  lsb_err_value ret = lsb_init_output_buffer(NULL, size);
  mu_assert(ret == LSB_ERR_UTIL_NULL, "received: %s", lsb_err_string(ret));

  mu_assert(!lsb_init_output_buffer(&b, size), "init failed");
  mu_assert(b.size == size, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size, "received: %" PRIuSIZE, b.size);
  lsb_free_output_buffer(&b);
  lsb_free_output_buffer(NULL);
  return NULL;
}


static char* test_init_large_buf()
{
  size_t size = 1024 * 1024;
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, size), "init failed");
  mu_assert(b.size == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size , "received: %" PRIuSIZE, b.size);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_init_zero_buf()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  mu_assert(b.size == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == 0 , "received: %" PRIuSIZE, b.size);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_expand_buf()
{
  size_t size = 1024 * 1024;
  size_t rsize = 1024 * 16;
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, size), "init failed");
  mu_assert(b.size == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.size);

  lsb_err_value ret = lsb_expand_output_buffer(NULL, 0);
  mu_assert(LSB_ERR_UTIL_NULL == ret, "received: %s", lsb_err_string(ret));

  mu_assert(!lsb_expand_output_buffer(&b, 1024 * 9), "expand failed");
  mu_assert(b.size == rsize, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size, "received: %" PRIuSIZE, b.size);

  mu_assert(!lsb_expand_output_buffer(&b, 1024), "expand failed");
  mu_assert(b.size == rsize, "received: %" PRIuSIZE, b.size);
  mu_assert(b.maxsize == size, "received: %" PRIuSIZE, b.size);

  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_expand_failure()
{
  size_t size = 1024;
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, size), "init failed");
  mu_assert(lsb_expand_output_buffer(&b, size + 1), "expand succeeded");
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputc()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  lsb_outputc(&b, 'a');
  mu_assert(strcmp("a", b.buf) == 0, "received: %s", b.buf);
  lsb_outputc(&b, 'b');
  mu_assert(strcmp("ab", b.buf) == 0, "received: %s", b.buf);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputf()
{
  lsb_err_value ret;
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 10), "init failed");
  lsb_outputf(&b, "%s", "foo");
  mu_assert(strcmp("foo", b.buf) == 0, "received: %s", b.buf);
  lsb_outputf(&b, " %s", "bar");
  mu_assert(strcmp("foo bar", b.buf) == 0, "received: %s", b.buf);
  ret = lsb_outputf(&b, " %s", "exceed the buffer");
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received: %s", lsb_err_string(ret));
  ret = lsb_outputf(NULL, "%s", "bar");
  mu_assert(ret == LSB_ERR_UTIL_NULL, "received: %s", lsb_err_string(ret));
  ret = lsb_outputf(&b, NULL, "bar");
  mu_assert(ret == LSB_ERR_UTIL_NULL, "received: %s", lsb_err_string(ret));
  lsb_free_output_buffer(&b);

  size_t len = 2000;
  mu_assert(!lsb_init_output_buffer(&b, len), "init failed");
  for (size_t i = 0; i < len - 1; ++i) {
    ret = lsb_outputf(&b, "%c", 'a');
    mu_assert(!ret, "received: %s", ret);
  }
  ret = lsb_outputf(&b, "%c", 'a');
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received: %s", lsb_err_string(ret));
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputs()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  lsb_outputs(&b, "foo", 3);
  mu_assert(strcmp("foo", b.buf) == 0, "received: %s", b.buf);
  lsb_outputf(&b, " bar", 4);
  mu_assert(strcmp("foo bar", b.buf) == 0, "received: %s", b.buf);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputd()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  lsb_outputd(&b, 10.1);
  mu_assert(strcmp("10.1", b.buf) == 0, "received: %s", b.buf);
  double d = INT_MAX;
  lsb_outputd(&b, d + 1);
  mu_assert(strcmp("10.12147483648", b.buf) == 0, "received: %s", b.buf);
  d = INT_MIN;
  lsb_outputd(&b, d - 1);
  mu_assert(strcmp("10.12147483648-2147483649", b.buf) == 0,
            "received: %s", b.buf);
  lsb_outputd(&b, NAN);
  mu_assert(strcmp("10.12147483648-2147483649nan", b.buf) == 0,
            "received: %s", b.buf);
  lsb_outputd(&b, INFINITY);
  mu_assert(strcmp("10.12147483648-2147483649naninf", b.buf) == 0,
            "received: %s", b.buf);
  lsb_outputd(&b, -INFINITY);
  mu_assert(strcmp("10.12147483648-2147483649naninf-inf", b.buf) == 0,
            "received: %s", b.buf);
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputc_full()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  for (int i = 0; i < LSB_OUTPUT_SIZE; ++i) {
    lsb_outputc(&b, 'a');
  }
  mu_assert(b.size == LSB_OUTPUT_SIZE * 2, "received: %" PRIuSIZE, b.size);
  mu_assert(b.pos == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.pos);
  mu_assert(b.buf[b.pos] == '\0', "missing NUL");
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputf_full()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  for (int i = 0; i < LSB_OUTPUT_SIZE; ++i) {
    lsb_outputf(&b, "%s", "f");
  }
  mu_assert(b.size == LSB_OUTPUT_SIZE * 2, "received: %" PRIuSIZE, b.size);
  mu_assert(b.pos == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.pos);
  mu_assert(b.buf[b.pos] == '\0', "missing NUL");
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputs_full()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  for (int i = 0; i < LSB_OUTPUT_SIZE; ++i) {
    lsb_outputs(&b, "s", 1);
  }
  mu_assert(b.size == LSB_OUTPUT_SIZE * 2, "received: %" PRIuSIZE, b.size);
  mu_assert(b.pos == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.pos);
  mu_assert(b.buf[b.pos] == '\0', "missing NUL");
  lsb_free_output_buffer(&b);
  return NULL;
}


static char* test_outputd_full()
{
  lsb_output_buffer b;
  mu_assert(!lsb_init_output_buffer(&b, 0), "init failed");
  for (int i = 0; i < LSB_OUTPUT_SIZE; ++i) {
    lsb_outputd(&b, 1);
  }
  mu_assert(b.size == LSB_OUTPUT_SIZE * 2, "received: %" PRIuSIZE, b.size);
  mu_assert(b.pos == LSB_OUTPUT_SIZE, "received: %" PRIuSIZE, b.pos);
  mu_assert(b.buf[b.pos] == '\0', "missing NUL");
  lsb_free_output_buffer(&b);
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
  mu_run_test(test_outputc);
  mu_run_test(test_outputf);
  mu_run_test(test_outputs);
  mu_run_test(test_outputd);

  // make sure the terminating NUL is included in the size of the buffer
  mu_run_test(test_outputc_full);
  mu_run_test(test_outputf_full);
  mu_run_test(test_outputs_full);
  mu_run_test(test_outputd_full);
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
