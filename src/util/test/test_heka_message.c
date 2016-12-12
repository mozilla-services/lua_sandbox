/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief heka_message unit tests @file */

#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#include <stdio.h>

#include "luasandbox/error.h"
#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/heka_message.h"

#define TEST_UUID "\x0a\x10\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
#define TEST_NS   "\x10\x01"

// {Uuid="" Timestamp = 1e9, Type="type", Logger="logger", Payload="payload", EnvVersion="env_version", Hostname="hostname", Severity=9, Fields = {number=1,numbers={value={1,2,3}, representation="count"},string="string",strings={"s1","s2","s3"}, bool=true, bools={true,false,false}}}
static char pb[] = "\x0a\x10\x73\x1e\x36\x84\xec\x25\x42\x76\xa4\x01\x79\x6f\x17\xdd\x20\x63\x10\x80\x94\xeb\xdc\x03\x1a\x04\x74\x79\x70\x65\x22\x06\x6c\x6f\x67\x67\x65\x72\x28\x09\x32\x07\x70\x61\x79\x6c\x6f\x61\x64\x3a\x0b\x65\x6e\x76\x5f\x76\x65\x72\x73\x69\x6f\x6e\x4a\x08\x68\x6f\x73\x74\x6e\x61\x6d\x65\x52\x13\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x03\x39\x00\x00\x00\x00\x00\x00\xf0\x3f\x52\x2c\x0a\x07\x6e\x75\x6d\x62\x65\x72\x73\x10\x03\x1a\x05\x63\x6f\x75\x6e\x74\x3a\x18\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x08\x40\x52\x0e\x0a\x05\x62\x6f\x6f\x6c\x73\x10\x04\x42\x03\x01\x00\x00\x52\x0a\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x40\x01\x52\x10\x0a\x06\x73\x74\x72\x69\x6e\x67\x22\x06\x73\x74\x72\x69\x6e\x67\x52\x15\x0a\x07\x73\x74\x72\x69\x6e\x67\x73\x22\x02\x73\x31\x22\x02\x73\x32\x22\x02\x73\x33";


struct log_message {
  const char  *component;
  int         severity;
  char        msg[1024];
};

static struct log_message lm = { .component = NULL, .severity = 0, .msg = { 0 } };

void log_cb(void *context, const char *component, int severity, const char *fmt,
            ...)
{
  (void)context;
  lm.component = component;
  lm.severity = severity;
  va_list args;
  va_start(args, fmt);
  vsnprintf(lm.msg, sizeof lm.msg, fmt, args);
  va_end(args);
}
static lsb_logger logger = { .context = NULL, .cb = log_cb };

static char* test_stub()
{
  return NULL;
}


static char* test_init()
{
  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 10), "failed");
  lsb_free_heka_message(&m);
  lsb_free_heka_message(NULL);
  return NULL;
}


static char* test_init_failure()
{
  lsb_heka_message m;
  mu_assert(lsb_init_heka_message(&m, 0), "suceeded");
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_decode()
{

#define add_string(str) {.s = str, .len = sizeof str - 1},
  lsb_const_string tests[] = {
    add_string(TEST_UUID TEST_NS) // required fields
    add_string(TEST_UUID TEST_NS "\x1a\x04" "Type")
    add_string(TEST_UUID TEST_NS "\x22\x06" "Logger")
    add_string(TEST_UUID TEST_NS "\x28\x07") // Severity
    add_string(TEST_UUID TEST_NS "\x32\x07" "Payload")
    add_string(TEST_UUID TEST_NS "\x3a\x0a" "EnvVersion")
    add_string(TEST_UUID TEST_NS "\x40\x11") // Pid
    add_string(TEST_UUID TEST_NS "\x4a\x08" "Hostname")
    add_string(TEST_UUID TEST_NS "\x52\x11\x0a\x03" "foo\x10\x00\x1a\x03" "rep\x22\x03" "bar") // string
    add_string(TEST_UUID TEST_NS "\x52\x11\x0a\x03" "foo\x10\x01\x1a\x03" "rep\x2a\x03" "bar") // bytes
    add_string(TEST_UUID TEST_NS "\x52\x0e\x0a\x03" "foo\x10\x02\x1a\x03" "rep\x30\x11")    // integer
    add_string(TEST_UUID TEST_NS "\x52\x15\x0a\x03" "foo\x10\x03\x1a\x03" "rep\x39\x00\x00\x00\x00\x00\x00\x00\x00") // double
    add_string(TEST_UUID TEST_NS "\x52\x0e\x0a\x03" "foo\x10\x04\x1a\x03" "rep\x40\x01") // bool
  };

  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed");
  for (unsigned i = 0; i < sizeof tests / sizeof(lsb_const_string); ++i){
    bool ok = lsb_decode_heka_message(&m, tests[i].s, tests[i].len, &logger);
    mu_assert(ok, "test: %d failed err: %s", i, lm.msg);
  }
  mu_assert(!lsb_decode_heka_message(NULL, NULL, 0, NULL), "succeeded");
  mu_assert(!lsb_decode_heka_message(&m, NULL, 0, NULL), "succeeded");
  mu_assert(!lsb_decode_heka_message(&m, tests[0].s, 0, NULL), "succeeded");
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_decode_failure()
{
  struct decode_failure {
    const char *s;
    const char *e;
  };

  struct decode_failure tests[] = {
    {   TEST_NS, "missing Uuid" } // required test
    , { TEST_UUID, "missing Timestamp" } // require test
    , { "\x0a\x01\xff", "tag:1 wiretype:2 position:0" } // invalid UUID length
    , { "\xf0", "tag:30 wiretype:0 position:0" } // unknown message tag
    , { "\x0b", "tag:1 wiretype:3 position:0" } // uuid invalid wiretype
    , { "\x11", "tag:2 wiretype:1 position:0" } // timestamp invalid wiretype
    , { "\x1b", "tag:3 wiretype:3 position:0" } // type invalid wiretype
    , { "\x23", "tag:4 wiretype:3 position:0" } // logger invalid wiretype
    , { "\x2b", "tag:5 wiretype:3 position:0" } // severity invalid wiretype
    , { "\x33", "tag:6 wiretype:3 position:0" } // payload invalid wiretype
    , { "\x3b", "tag:7 wiretype:3 position:0" } // env_version invalid wiretype
    , { "\x43", "tag:8 wiretype:3 position:0" } // pid invalid wiretype
    , { "\x4b", "tag:9 wiretype:3 position:0" } // hostname invalid wiretype
    , { "\x53", "tag:10 wiretype:3 position:0" } // fields invalid wiretype
    , { "\x52\x10", "tag:10 wiretype:2 position:0" } // invalid field length
    , { "\x52\x01\x0b", "tag:10 wiretype:2 position:0" } // invalid name wiretype
    , { "\x52\x01\x13", "tag:10 wiretype:2 position:0" } // invalid value_type wiretype
    , { "\x52\x01\x1b", "tag:10 wiretype:2 position:0" } // invalid representation wiretype
    , { "\x52\x01\x23", "tag:10 wiretype:2 position:0" } // invalid string wiretype
    , { "\x52\x01\x2b", "tag:10 wiretype:2 position:0" } // invalid bytes wiretype
    , { "\x52\x01\x33", "tag:10 wiretype:2 position:0" } // invalid integer wiretype
    , { "\x52\x01\x3b", "tag:10 wiretype:2 position:0" } // invalid bool wiretype
    , { "\x52\x01\x43", "tag:10 wiretype:2 position:0" } // invalid double wiretype
    , { "\x52\x01\x4b", "tag:10 wiretype:2 position:0" } // unknown field tag
    , { "\x52\xc\x10\x00\x1a\x03" "rep\x22\x03" "bar", "tag:10 wiretype:2 position:0" } // no name
    , { "\x10\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", "tag:2 wiretype:0 position:0" } // invalid varint
    , { "\x0a\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", "tag:1 wiretype:2 position:0" } // invalid varint length
  };

  lsb_heka_message m;
  mu_assert(!lsb_init_heka_message(&m, 10), "failed");
  for (unsigned i = 0; i < sizeof(tests) / sizeof(struct decode_failure); ++i) {
    bool ok = lsb_decode_heka_message(&m, tests[i].s, strlen(tests[i].s), &logger);
    mu_assert(!ok, "test: %u no error generated", i);
    mu_assert(!strcmp(lm.msg, tests[i].e), "test: %u expected: %s received: %s",
              i, tests[i].e, lm.msg);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_find_message()
{
  struct find_message {
    lsb_const_string s;
    bool             b;
    size_t           d;
  };

#define ONE_TWENTY_SIX "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define add_input(str, result, discard) {.s = {.s = str, .len = sizeof str - 1}, .b = result, .d = discard},
  struct find_message tests[] = {
    add_input("\x1e\x02\x08\x14\x1f" TEST_UUID TEST_NS, true, 0) // full message
    add_input("\x1e\x80\x08\x14" ONE_TWENTY_SIX "\x1f" TEST_UUID TEST_NS, true, 0) // large header
    add_input("\x1e", false, 0)
    add_input("\x02", false, 0)
    add_input("\x08", false, 0)
    add_input("\x14", false, 0)
    add_input("\x1f", false, 0)
    add_input(TEST_UUID, false, 0)
    add_input(TEST_NS "\x1e\x02\x08\x14\x1f", true, 0) // completion of an incremental message and the start of another
    add_input(TEST_UUID TEST_NS, true, 0)
    add_input(TEST_UUID TEST_NS, false, 20) // no framing
    add_input("\x1e\x02\x08\x15\x1f" TEST_UUID TEST_NS "\x00", false, 26) // message decode failure
    add_input("\x1e\x02\x08\x00\x1f", false, 5) // header decoder failure
    add_input("\x1e\x02\x08\x14\x01\x1f", false, 6) // invalid header length
    add_input("\x1e\x02\x09\x14\x1f", false, 5) // invalid protobuf header incorrect tag
    add_input("\x1e\x02\x08\x65\x1f", false, 5) // invalid header message too long
  };

  lsb_heka_message m;
  lsb_input_buffer ib;
  size_t db;
  mu_assert(!lsb_init_heka_message(&m, 1), "failed");
  mu_assert(!lsb_init_input_buffer(&ib, 100), "failed");
  for (unsigned i = 0; i < sizeof(tests) / sizeof(struct find_message); ++i) {
    mu_assert(!lsb_expand_input_buffer(&ib, tests[i].s.len), "buffer exhausted");
    memcpy(ib.buf + ib.readpos, tests[i].s.s, tests[i].s.len);
    ib.readpos += tests[i].s.len;
    bool b = lsb_find_heka_message(&m, &ib, true, &db, NULL);
    mu_assert(tests[i].b == b, "test: %u failed", i);
    mu_assert(tests[i].d == db, "test: %u failed expected: %" PRIuSIZE " received: %" PRIuSIZE, i, tests[i].d, db);
  }
  mu_assert(!lsb_find_heka_message(NULL, NULL, true, NULL, NULL), "succeeded");
  mu_assert(!lsb_find_heka_message(&m, NULL, true, NULL, NULL), "succeeded");
  mu_assert(!lsb_find_heka_message(&m, &ib, true, NULL, NULL), "succeeded");
  lsb_free_input_buffer(&ib);
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_read_heka_field()
{
  lsb_heka_message m;
  lsb_init_heka_message(&m, 8);
  mu_assert(lsb_decode_heka_message(&m, pb, sizeof pb - 1, NULL), "decode failed");

  lsb_read_value v;
  lsb_const_string cs;
  cs.s = "string";
  cs.len = 6;
  mu_assert(lsb_read_heka_field(&m, &cs, 0, 0, &v), "standalone");
  mu_assert(v.type == LSB_READ_STRING, "%d", v.type);
  mu_assert(strncmp(v.u.s.s, "string", v.u.s.len) == 0, "invalid value: %.*s",
            (int)v.u.s.len, v.u.s.s);

  cs.s = "strings";
  cs.len = 7;
  mu_assert(lsb_read_heka_field(&m, &cs, 0, 0, &v), "item 0");
  mu_assert(v.type == LSB_READ_STRING, "%d", v.type);
  mu_assert(strncmp(v.u.s.s, "s1", v.u.s.len) == 0, "invalid value: %.*s",
            (int)v.u.s.len, v.u.s.s);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 1, &v), "item 1");
  mu_assert(v.type == LSB_READ_STRING, "%d", v.type);
  mu_assert(strncmp(v.u.s.s, "s2", v.u.s.len) == 0, "invalid value: %.*s",
            (int)v.u.s.len, v.u.s.s);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 2, &v), "item 2");
  mu_assert(v.type == LSB_READ_STRING, "%d", v.type);
  mu_assert(strncmp(v.u.s.s, "s3", v.u.s.len) == 0, "invalid value: %.*s",
            (int)v.u.s.len, v.u.s.s);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 3, &v) == false,
            "no item 3");
  mu_assert(v.type == LSB_READ_NIL, "%d", v.type);

  cs.s = "number";
  cs.len = 6;
  mu_assert(lsb_read_heka_field(&m, &cs, 0, 0, &v), "standalone");
  mu_assert(v.type == LSB_READ_NUMERIC, "%d", v.type);
  mu_assert(v.u.d == 1, "invalid value: %g", v.u.d);

  cs.s = "numbers";
  cs.len = 7;
  mu_assert(lsb_read_heka_field(&m, &cs, 0, 0, &v), "item 0");
  mu_assert(v.type == LSB_READ_NUMERIC, "%d", v.type);
  mu_assert(v.u.d == 1, "invalid value: %g", v.u.d);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 1, &v), "item 1");
  mu_assert(v.type == LSB_READ_NUMERIC, "%d", v.type);
  mu_assert(v.u.d == 2, "invalid value: %g", v.u.d);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 2, &v), "item 2");
  mu_assert(v.type == LSB_READ_NUMERIC, "%d", v.type);
  mu_assert(v.u.d == 3, "invalid value: %g", v.u.d);

  mu_assert(lsb_read_heka_field(&m, &cs, 0, 3, &v) == false,
            "no item 3");
  mu_assert(v.type == LSB_READ_NIL, "%d", v.type);

  cs.s = "bool";
  cs.len = 4;
  mu_assert(lsb_read_heka_field(&m, &cs, 0, 0, &v), "standalone");
  mu_assert(v.type == LSB_READ_BOOL, "%d", v.type);
  mu_assert(v.u.d == 1, "invalid value: %g", v.u.d);

  mu_assert(!lsb_read_heka_field(NULL, NULL, 0, 0, NULL), "succeeded");
  mu_assert(!lsb_read_heka_field(&m, NULL, 0, 0, NULL), "succeeded");
  mu_assert(!lsb_read_heka_field(&m, &cs, 0, 0, NULL), "succeeded");
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_write_heka_uuid()
{
  lsb_err_value ret;
  lsb_output_buffer ob;
  lsb_init_output_buffer(&ob, LSB_UUID_SIZE + 2);

  const char header[2] = "\x0a\x10";
  const char bin_uuid[LSB_UUID_SIZE] = { 0 };
  ret = lsb_write_heka_uuid(&ob, bin_uuid, LSB_UUID_SIZE);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos == LSB_UUID_SIZE + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(memcmp(ob.buf, header, sizeof header) == 0, "invalid header");
  mu_assert(memcmp(ob.buf + 2, bin_uuid, LSB_UUID_SIZE) == 0, "invalid");

  const char str_uuid[] = "00000000-0000-0000-0000-"
      "000000000000";
  ret = lsb_write_heka_uuid(&ob, str_uuid, LSB_UUID_STR_SIZE);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos == LSB_UUID_SIZE + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(memcmp(ob.buf, header, sizeof header) == 0, "invalid header");
  mu_assert(memcmp(ob.buf + 2, bin_uuid, LSB_UUID_SIZE) == 0, "invalid");

  const char err_uuid[] = "00000000+0000-0000-0000-"
      "000000000000";
  ret = lsb_write_heka_uuid(&ob, err_uuid, LSB_UUID_STR_SIZE);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos == LSB_UUID_SIZE + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(memcmp(ob.buf, header, sizeof header) == 0, "invalid header");
  mu_assert(ob.buf[8] & 0x40, "invalid format should create a type 4 uuid");

  ret = lsb_write_heka_uuid(&ob, NULL, 0);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos == LSB_UUID_SIZE + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(ob.buf[8] & 0x40, "null string should create a type 4 uuid");
  lsb_free_output_buffer(&ob);

  ret = lsb_write_heka_uuid(&ob, bin_uuid, 10);
  mu_assert(!ret, "received %s", ret);
  mu_assert(ob.pos == LSB_UUID_SIZE + 2, "received: %" PRIuSIZE, ob.pos);
  mu_assert(ob.buf[8] & 0x40, "unexpected length should create a type 4 uuid");
  lsb_free_output_buffer(&ob);

  lsb_output_buffer sob;
  lsb_init_output_buffer(&sob, LSB_UUID_SIZE);
  ret = lsb_write_heka_uuid(&sob, NULL, 0);
  mu_assert(ret, "received <no error>");
  mu_assert(sob.pos == 0, "received: %" PRIuSIZE, sob.pos);
  lsb_free_output_buffer(&sob);

  ret = lsb_write_heka_uuid(NULL, bin_uuid, LSB_UUID_SIZE);
  mu_assert(ret == LSB_ERR_UTIL_NULL, "received %s", lsb_err_string(ret));

  return NULL;
}

#if SIZE_MAX < 65536
static char* test_write_heka_header()
{
  char header[LSB_MIN_HDR_SIZE];
  size_t hlen;

  for (unsigned i = 0; i < sizeof(size_t); ++i) {
    hlen = lsb_write_heka_header(header, (size_t)1 << (i * 8));
    mu_assert(hlen == 5 + i, "i: %u received %" PRIuSIZE, i, hlen);
  }
  hlen = lsb_write_heka_header(header, (size_t)1 << 15);
  mu_assert(hlen == 7, "received %" PRIuSIZE, hlen);
  return NULL;
}
#elif SIZE_MAX < 4294967296
static char* test_write_heka_header()
{
  char header[LSB_MIN_HDR_SIZE];
  size_t hlen;

  for (unsigned i = 0; i < sizeof(size_t); ++i) {
    hlen = lsb_write_heka_header(header, (size_t)1 << (i * 8));
    mu_assert(hlen == 5 + i, "i: %u received %" PRIuSIZE, i, hlen);
  }
  hlen = lsb_write_heka_header(header, (size_t)1 << 31);
  mu_assert(hlen == 9, "received %" PRIuSIZE, hlen);
  return NULL;
}
#else
static char* test_write_heka_header() // limit to 8 byte test
{
  char header[LSB_MIN_HDR_SIZE];
  size_t hlen;

  for (unsigned i = 0; i < 7; ++i) {
    hlen = lsb_write_heka_header(header, (size_t)1 << (i * 8));
    mu_assert(hlen == 5 + i, "i: %u received %" PRIuSIZE, i, hlen);
  }
  hlen = lsb_write_heka_header(header, (size_t)1 << 56);
  mu_assert(hlen == 13, "received %" PRIuSIZE, hlen);
  hlen = lsb_write_heka_header(header, (size_t)1 << 63);
  mu_assert(hlen == 14, "received %" PRIuSIZE, hlen);
  return NULL;
}
#endif


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_init);
  mu_run_test(test_init_failure);
  mu_run_test(test_decode);
  mu_run_test(test_decode_failure);
  mu_run_test(test_find_message);
  mu_run_test(test_read_heka_field);
  mu_run_test(test_write_heka_uuid);
  mu_run_test(test_write_heka_header);
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
