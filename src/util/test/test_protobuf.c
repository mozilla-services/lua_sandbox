/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief util protobuf unit tests @file */

#include <stdio.h>
#include <string.h>

#include "luasandbox/error.h"
#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/protobuf.h"

static char* test_stub()
{
  return NULL;
}


static char* test_lsb_pb_read_key()
{
  int tag, wt;
  const char input[] = "\x0b";
  const char *p = lsb_pb_read_key(input, &tag, &wt);
  mu_assert(p = input + 1, "received: %p", (void *)p);
  mu_assert(tag == 1, "received: %d", tag);
  mu_assert(wt == 3, "received: %d", wt);
  p = lsb_pb_read_key(NULL, &tag, &wt);
  mu_assert(!p, "not null");
  p = lsb_pb_read_key(input, NULL, &wt);
  mu_assert(!p, "not null");
  p = lsb_pb_read_key(input, &tag, NULL);
  mu_assert(!p, "not null");
  return NULL;
}


static char* test_lsb_pb_write_key()
{
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, LSB_MAX_VARINT_BYTES);

  lsb_err_value ret = lsb_pb_write_key(&ob, 1, 3);
  mu_assert(!ret, "received %s", ret);
  mu_assert(memcmp(ob.buf, "\x0b", 1) == 0, "received: %02hhx", ob.buf[0]);

  ob.pos = ob.maxsize;
  ret = lsb_pb_write_key(&ob, 1, 3);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));
  lsb_free_output_buffer(&ob);
  return NULL;
}


static char* test_varint()
{
  struct test_data {
    long long v;
    const char *s;
  };

  struct test_data tests[] = {
    { 0, "\x00" },
    { 16, "\x10" },
    { 300, "\xac\x02" },
    { -1,  "\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01" },
    { 5000000000LL, "\x80\xe4\x97\xd0\x12" }
  };

  lsb_err_value ret;
  long long vi;
  const char *p;
  for (unsigned i = 0; i < sizeof tests / sizeof tests[0]; ++i){
    long long v = tests[i].v;
    const char *s = tests[i].s;
    size_t len = strlen(s);
    len = len ? len : 1;
    p = lsb_pb_read_varint(s, s + len, &vi);
    mu_assert(p = s + len, "received: %p", (void *)p);
    mu_assert(v == vi, "expected: %lld received: %lld", v, vi);

    lsb_output_buffer ob;
    lsb_init_output_buffer(&ob, LSB_MAX_VARINT_BYTES);
    ret = lsb_pb_write_varint(&ob, v);
    mu_assert(!ret, "received %s", ret);
    if (strncmp(s, ob.buf, len) != 0) {
      char expected[LSB_MAX_VARINT_BYTES + 1] = { 0 };
      char received[LSB_MAX_VARINT_BYTES + 1] = { 0 };
      for (unsigned i = 0; i < len; ++i) {
        sprintf(expected + i * 2, "%02hhx", s[i]);
        sprintf(received + i * 2, "%02hhx", ob.buf[i]);
      }
      mu_assert(false, "expected: %s received: %s", expected, received);
    }
    lsb_free_output_buffer(&ob);
  }

  const char tl[] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
  mu_assert(!lsb_pb_read_varint(tl, tl + sizeof(tl) - 1, &vi),
            "parsed invalid varint (too long)");

  const char ts[] = "\xff";
  mu_assert(!lsb_pb_read_varint(ts, ts + sizeof(ts) - 1, &vi),
            "parsed invalid varint (too short)");

  mu_assert(!lsb_pb_read_varint(NULL, ts + sizeof(ts) - 1, &vi), "null start");
  mu_assert(!lsb_pb_read_varint(ts, NULL, &vi), "null end");
  mu_assert(!lsb_pb_read_varint(ts, ts + sizeof(ts) - 1, NULL), "null ret");

  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, LSB_MAX_VARINT_BYTES);
  ob.pos = ob.maxsize;
  ret = lsb_pb_write_varint(&ob, 1);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));
  lsb_free_output_buffer(&ob);

  return NULL;
}


static char* test_lsb_pb_write_bool()
{
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, 1);
  lsb_err_value ret = lsb_pb_write_bool(&ob, 7);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.buf[0] == 1, "received: %02hhx", ob.buf[0]);
  mu_assert(lsb_pb_write_bool(&ob, 7), "buffer should be full");

  ob.pos = 0;
  ret = lsb_pb_write_bool(&ob, 0);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.buf[0] == 0, "received: %02hhx", ob.buf[0]);

  lsb_free_output_buffer(&ob);
  return NULL;
}


static char* test_lsb_pb_write_double()
{
  double d = 7.13;
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, sizeof d);
  lsb_err_value ret = lsb_pb_write_double(&ob, d);
  mu_assert(!ret, "received %s", ret);
  mu_assert(memcmp(ob.buf, &d, sizeof d) == 0, "received: %g",
            *((double *)ob.buf));
  ret = lsb_pb_write_double(&ob, d);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));
  lsb_free_output_buffer(&ob);
  return NULL;
}


static char* test_lsb_pb_write_string()
{
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, 64);
  const char foo[] = "foo";
  const size_t len = sizeof foo - 1;

  // failure writing the key
  ob.pos = ob.maxsize;
  lsb_err_value ret = lsb_pb_write_string(&ob, 1, foo, len);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));

  // failure writing the len
  ob.pos = ob.maxsize - 1;
  ret = lsb_pb_write_string(&ob, 1, foo, len);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));

  // failure writing the string
  ob.pos = ob.maxsize - 3;
  ret = lsb_pb_write_string(&ob, 1, foo, len);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));

  ob.pos = 0;
  ret = lsb_pb_write_string(&ob, 1, foo, len);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos = len + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(memcmp("\x0a\x03" "foo", ob.buf, 5) == 0, "received: "
            "%02hhx%02hhx%02hhx%02hhx2%hhx", ob.buf[0], ob.buf[1], ob.buf[2],
            ob.buf[3], ob.buf[4]);

  lsb_free_output_buffer(&ob);
  return NULL;
}


static char* test_lsb_pb_update_field_length()
{
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, 1024);
  memset(ob.buf, 0, 1024);
  ob.buf[0] = 'a';
  ob.buf[2] = 'b';
  ob.pos = 3;
  lsb_err_value ret =  lsb_pb_update_field_length(&ob, 1);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.buf[1] == 1, "received: %d", ob.buf[1]);
  mu_assert(ob.buf[2] == 'b', "received: %02hhx", ob.buf[2]);
  mu_assert(ob.pos == 3, "received: %" PRIuSIZE, ob.pos);

  // buffer full
  ob.pos = 1024;
  ret = lsb_pb_update_field_length(&ob, 1);
  mu_assert(ret == LSB_ERR_UTIL_FULL, "received %s", lsb_err_string(ret));

  // position is out of bounds
  ob.pos = 512;
  ret = lsb_pb_update_field_length(&ob, 512);
  mu_assert(ret == LSB_ERR_UTIL_PRANGE, "received %s", lsb_err_string(ret));

  ob.buf[301] = 'x';
  ob.pos = 302;
  ret = lsb_pb_update_field_length(&ob, 1);
  mu_assert(!ret, "received %s", ret);
  mu_assert((unsigned char)ob.buf[1] == 0xac, "received: %02hhx", ob.buf[1]);
  mu_assert(ob.buf[2] == 0x02, "received: %02hhx", ob.buf[2]);
  mu_assert(ob.buf[3] == 'b', "received: %02hhx", ob.buf[3]);
  mu_assert(ob.buf[302] == 'x', "received: %02hhx", ob.buf[302]);
  mu_assert(ob.pos == 303, "received: %" PRIuSIZE, ob.pos);

  lsb_free_output_buffer(&ob);
  return NULL;
}

static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_lsb_pb_read_key);
  mu_run_test(test_lsb_pb_write_key);
  mu_run_test(test_varint);
  mu_run_test(test_lsb_pb_write_bool);
  mu_run_test(test_lsb_pb_write_double);
  mu_run_test(test_lsb_pb_write_string);
  mu_run_test(test_lsb_pb_update_field_length);
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
