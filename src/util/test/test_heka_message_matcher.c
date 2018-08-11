/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight/Heka message matcher unit tests @file */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/heka_message_matcher.h"

// {"Logger":"GoSpec","Uuid":"xxx","Pid":32157,"Severity":6,"EnvVersion":"0.8","Fields":[{""value":["bar"],"name":"foo","value_type":0},{"value":[64],"name":"number","value_type":2},{"value":["data"],"name":"bytes","value_type":1},{"value":[999,1024],"name":"int","value_type":2},{"value":[99.9],"name":"double","value_type":3},{"value":[true],"name":"bool","value_type":4},{"value":["alternate"],"name":"foo","value_type":0},{"value":["name=test;type=web;"],"name":"Payload","value_type":0},{"representation":"date-time","value":["Mon Jan 02 15:04:05 -0700 2006"],"name":"Timestamp","value_type":0},{"value":[0],"name":"zero","value_type":2},{"value":["43"],"name":"string","value_type":0}],"Payload":"Test Payload with a longer string to attempt to create a difference in pattern match time versus the string literal match time for a unique-item","Timestamp":1.428773426113e+18,"Hostname":"trink-x230","Type":"TEST"}

char pb[] = "\x0a\x10\x23\x00\x81\xdc\x32\x6f\x4e\x3f\x9a\x5a\x93\x86\xa3\x7e\x24\x6f\x10\xe4\x9e\xf1\xff\xc6\xbb\x81\xea\x13\x1a\x04\x54\x45\x53\x54\x22\x06\x47\x6f\x53\x70\x65\x63\x28\x06\x32\x90\x01Test Payload with a longer string to attempt to create a difference in pattern match time versus the string literal match time for a unique-item\x3a\x03\x30\x2e\x38\x40\x9d\xfb\x01\x4a\x0a\x74\x72\x69\x6e\x6b\x2d\x78\x32\x33\x30\x52\x0c\x0a\x03\x66\x6f\x6f\x10\x00\x22\x03\x62\x61\x72\x52\x0d\x0a\x06\x6e\x75\x6d\x62\x65\x72\x10\x02\x32\x01\x40\x52\x0f\x0a\x05\x62\x79\x74\x65\x73\x10\x01\x2a\x04\x64\x61\x74\x61\x52\x0d\x0a\x03\x69\x6e\x74\x10\x02\x32\x04\xe7\x07\x80\x08\x52\x14\x0a\x06\x64\x6f\x75\x62\x6c\x65\x10\x03\x3a\x08\x9a\x99\x99\x99\x99\xf9\x58\x40\x52\x0b\x0a\x04\x62\x6f\x6f\x6c\x10\x04\x42\x01\x01\x52\x12\x0a\x03\x66\x6f\x6f\x10\x00\x22\x09\x61\x6c\x74\x65\x72\x6e\x61\x74\x65\x52\x20\x0a\x07\x50\x61\x79\x6c\x6f\x61\x64\x10\x00\x22\x13\x6e\x61\x6d\x65\x3d\x74\x65\x73\x74\x3b\x74\x79\x70\x65\x3d\x77\x65\x62\x3b\x52\x38\x0a\x09\x54\x69\x6d\x65\x73\x74\x61\x6d\x70\x10\x00\x1a\x09\x64\x61\x74\x65\x2d\x74\x69\x6d\x65\x22\x1e\x4d\x6f\x6e\x20\x4a\x61\x6e\x20\x30\x32\x20\x31\x35\x3a\x30\x34\x3a\x30\x35\x20\x2d\x30\x37\x30\x30\x20\x32\x30\x30\x36\x52\x0b\x0a\x04\x7a\x65\x72\x6f\x10\x02\x32\x01\x00\x52\x0e\x0a\x06\x73\x74\x72\x69\x6e\x67\x10\x00\x22\x02\x34\x33";
size_t pblen = sizeof(pb);

char pbmin[] = "\x0a\x10\x23\x00\x81\xdc\x32\x6f\x4e\x3f\x9a\x5a\x93\x86\xa3\x7e\x24\x6f\x10\x00";
size_t pbminlen = sizeof(pbmin);


static char* test_stub()
{
  return NULL;
}


static char* test_api_assertion()
{
  mu_assert(NULL == lsb_create_message_matcher(NULL), "not null");
  lsb_destroy_message_matcher(NULL);
  return NULL;
}

#define T128 "Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' &&" \
"Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.' && Type =~ '.'"

#define S255 "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789" \
"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789" \
"0123456789012345678901234567890123456789012345678901234"

static char* test_true_matcher()
{
  char *tests[] = {
    "TRUE"
    , "Timestamp > 1.428773420000e+18"
    , "Timestamp < 1.428773426999e18"
    , "Timestamp == 1428773426113040228"
    , "Timestamp > '2015-04-11T17:30:26Z'" // 1428773426000000000
    , "Timestamp > '2015-04-11T17:30:26.112Z'"
    , "Timestamp < '2015-04-11T17:30:26.114Z'"
    , "(Severity == 7 || Logger == 'GoSpec') && Type == 'TEST'"
    , "EnvVersion == \"0.8\""
    , "EnvVersion == '0.8'"
    , "EnvVersion != '0.9'"
    , "EnvVersion > '0.7'"
    , "EnvVersion >= '0.8'"
    , "EnvVersion < '0.9'"
    , "EnvVersion <= '0.8'"
    , "Hostname != ''"
    , "Logger == 'GoSpec'"
    , "Pid != 0"
    , "Pid != NIL"
    , "Severity != 5"
    , "Severity < 7"
    , "Severity <= 7"
    , "Severity <= 6"
    , "Severity == 6"
    , "Severity > 5"
    , "Severity >= 5"
    , "Severity >= 6"
    , "Timestamp > 0"
    , "Type != 'test'"
    , "Type == 'TEST' && Severity == 6"
    , "Type == 'test' && Severity == 7 || Logger == 'GoSpec'"
    , "Type == 'TEST'"
    , "Type == 'foo' || Type == 'bar' || Type == 'TEST'"
    , "Fields[foo] == 'bar'"
    , "Fields[foo][0] == 'bar'"
    , "Fields[foo][0][0] == 'bar'"
    , "Fields[foo][1] == 'alternate'"
    , "Fields[foo][1][0] == 'alternate'"
    , "Fields[foo] <= 'barx'"
    , "Fields[foo] < 'barx'"
    , "Fields[foo] >= 'bar'"
    , "Uuid > '#'"
    , "Uuid >= '#'"
    , "Fields[foo] > 'baq'"
    , "Fields[foo] != 'bara'"
    , "Fields[bytes] == 'data'"
    , "Fields[int] == 999"
    , "Fields[int][0][1] == 1024"
    , "Fields[double] == 99.9"
    , "Fields[bool] == TRUE"
    , "Fields[bool] != FALSE"
    , "Fields[int] != NIL"
    , "Fields[int][0][1] != NIL"
    , "Fields[int][0][2] == NIL"
    , "Fields[missing] == NIL"
    , "Type =~ 'TEST'"
    , "Type !~ 'bogus'"
    , "Type =~ 'TEST' && Payload =~ 'Payload'"
    , "Fields[foo][1] =~ 'alt'"
    , "Fields[Payload] =~ 'name=%w+'"
    , "Type =~ 'ST'"
    , "Type =~ '^TE'"
    , "Type =~ 'ST$'"
    , "Type !~ '^te'"
    , "Type !~ 'st$'"
    , "Fields[foo][255] == NIL"
    , "Fields[foo][0][255] == NIL"
    , T128
    , "Payload =~ 'unique-item'%"
    , "Fields[Timestamp] =~ ' -0700'%"
    , "Type != NIL"
    , "Hostname != NIL"
    , "EnvVersion != NIL"
    , "Pid != NIL"
    , "Logger != NIL"
    , "Payload != NIL"
    , "Pid == 32157"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 16);
  mu_assert(lsb_decode_heka_message(&m, pb, pblen - 1, NULL), "decode failed");
  for (int i = 0; tests[i]; ++i) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "failed to create the matcher %s", tests[i]);
    mu_assert(lsb_eval_message_matcher(mm, &m), "%s", tests[i]);
    lsb_destroy_message_matcher(mm);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_nil_header_true_matcher()
{
  char *tests[] = {
    "Type == NIL"
    , "Type != ''"
    , "Type < ''"
    , "Type <= ''"
    , "Type !~ '.'"
    , "Type !~ 'foo'"
    , "Hostname == NIL"
    , "EnvVersion == NIL"
    , "Pid == NIL"
    , "Logger == NIL"
    , "Payload == NIL"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 1);
  mu_assert(lsb_decode_heka_message(&m, pbmin, pbminlen - 1, NULL), "decode failed");
  for (int i = 0; tests[i]; ++i) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "failed to create the matcher %s", tests[i]);
    mu_assert(lsb_eval_message_matcher(mm, &m), "%s", tests[i]);
    lsb_destroy_message_matcher(mm);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_nil_header_false_matcher()
{
  char *tests[] = {
    "Type != NIL"
    , "Type == '%b()'"
    , "Type == ''"
    , "Type > ''"
    , "Type >= ''"
    , "Type =~ '.'"
    , "Type =~ 'foo'"
    , "Hostname != NIL"
    , "Hostname == ''"
    , "EnvVersion != NIL"
    , "EnvVersion == ''"
    , "Pid != NIL"
    , "Logger != NIL"
    , "Logger == ''"
    , "Payload != NIL"
    , "Payload == ''"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 1);
  mu_assert(lsb_decode_heka_message(&m, pbmin, pbminlen - 1, NULL), "decode failed");
  for (int i = 0; tests[i]; ++i) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "failed to create the matcher %s", tests[i]);
    mu_assert(lsb_eval_message_matcher(mm, &m) == false, "%s", tests[i]);
    lsb_destroy_message_matcher(mm);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_false_matcher()
{
  char *tests[] = {
    "FALSE"
    , "Timestamp == 1e9"
    , "Timestamp > '2015-04-11T17:30:27Z'"
    , "Type == 'test'&&(Severity==7||Payload=='Test Payload')"
    , "EnvVersion == '0.9'"
    , "EnvVersion != '0.8'"
    , "EnvVersion > '0.9'"
    , "EnvVersion >= '0.9'"
    , "EnvVersion < '0.8'"
    , "EnvVersion <= '0.7'"
    , "Severity == 5"
    , "Severity != 6"
    , "Severity < 6"
    , "Severity <= 5"
    , "Severity > 6"
    , "Severity >= 7"
    , "Fields[foo] == 'ba'"
    , "Fields[foo][1] == 'bar'"
    , "Fields[foo][0][1] == 'bar'"
    , "Fields[bool] == FALSE"
    , "Fields[bool] != TRUE"
    , "Fields[foo] > 'bara'"
    , "Fields[foo] >= 'bara'"
    , "Fields[foo] == 'bara'"
    , "Type =~ 'Test'"
    , "Type !~ 'TEST'"
    , "Payload =~ '^Payload'"
    , "Type == \"te'st\""
    , "Type == 'te\"st'"
    , "Fields[int] =~ '999'"
    , "Fields[zero] == \"0\""
    , "Fields[string] == 43"
    , "Fields[int] == NIL"
    , "Fields[int][0][1] == NIL"
    , "Fields[missing] != NIL"
    , "Type =~ '^te'"
    , "Type =~ 'st$'"
    , "Type !~ '^TE'"
    , "Type !~ 'ST$'"
    , "Logger =~ '.' && Type =~ '^anything'"
    , "Type == '" S255 "'"
    , "Payload =~ 'not.found'%"
    , "Fields[foo][1] =~ 'not.found'%"
    , "Type == NIL"
    , "Hostname == NIL"
    , "EnvVersion == NIL"
    , "Pid == NIL"
    , "Logger == NIL"
    , "Payload == NIL"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 8);
  mu_assert(lsb_decode_heka_message(&m, pb, pblen - 1, NULL), "decode failed");
  for (int i = 0; tests[i]; ++i) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "failed to create the matcher %s", tests[i]);
    mu_assert(lsb_eval_message_matcher(mm, &m) == false, "%s", tests[i]);
    lsb_destroy_message_matcher(mm);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* test_malformed_matcher()
{
  char *tests[] = {
    ""
    , "bogus"
    , "Type = 'test'"                                               // invalid operator
    , "Pid == 'test='"                                              // Pid is not a string
    , "Type == 'test' && (Severity==7 || Payload == 'Test Payload'" // missing paren
    , "Invalid == 'bogus'"                                          // unknown variable name
    , "Fields[]"                                                    // empty name key
    , "Fields[test][]"                                              // empty field index
    , "Fields[test][a]"                                             // non numeric field index
    , "Fields[test][0][]"                                           // empty array index
    , "Fields[test][0][a]"                                          // non numeric array index
    , "Fields[test][0][0][]"                                        // extra index dimension
    , "Fields[test][xxxx"                                           // unmatched bracket
    , "Pid =~ '6'"                                                  // string match not allowed on numeric
    , "Pid !~ '6'"                                                  // string match not allowed on numeric
    , "Type =~ 'test"                                               // unmatched quote
    , "Type != 'test\""                                             // mis matched quote types
    , "Pid =~ 6"                                                    // incorrect type for the operator
    , "NIL"                                                         // invalid use of constant
    , "Type         > NIL"                                          // existence check only works with equals and not equals
    , "Pid > NIL"
    , "Fields[test] > NIL"
    , "Uuid == NIL"                                                 // required header, cannot be nil
    , "Severity == NIL"                                             // defaulted header, cannot be nil
    , "TRUE FALSE"                                                  // missing operator
    , "Timestamp == '20150411T173026'"                              // non rfc3339 timestamp
    , T128 " && Type =~ '.'"                                        // too many tests
    , "Type == '" S255 "5'"                                         // string too long
    , "Fields[test][256] == 1"                                      // field index out of bounds
    , "Fields[test][0][256] == 1"                                   // array index out of bounds
    , "Payload =~ 'foo'i"                                           // invalid string match pattern modifier
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 8);
  mu_assert(lsb_decode_heka_message(&m, pb, pblen - 1, NULL), "decode failed");
  for (int i = 0; tests[i]; ++i) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm == NULL, "created malformed matcher");
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* benchmark_matcher_create()
{
  int iter = 100000;
  const char *exp = "Type == 'TEST' && Severity == 6";

  clock_t t = clock();
  for (int x = 0; x < iter; ++x) {
    lsb_message_matcher *mm = lsb_create_message_matcher(exp);
    mu_assert(mm, "lsb_create_message_matcher failed");
    lsb_destroy_message_matcher(mm);
  }
  t = clock() - t;
  printf("benchmark_matcher_create: %g\n", ((double)t) / CLOCKS_PER_SEC
         / iter);
  return NULL;
}


static char* benchmark_match_hs()
{
  // see what a single sample looks like for a better comparison with Hindsight
  char *tests[] = {
    "TRUE"
    , "Type == 'TEST' && Severity == 6"
    , "Fields[foo] == 'bar' && Severity == 6"
    , "Fields[number] == 64 && Severity == 6"
    , "Fields[missing] == NIL"
    , "Fields[int] != NIL"
    , "Type =~ '^[Tt]EST' && Severity == 6"
    , "Payload =~ '^Test'"
    , "Payload =~ 'item$'"
    , "Payload =~ 'unique%-item'"
    , "Payload =~ 'unique-item'%"
    , "Payload =~ 'unique'"
    , "Payload =~ 'unique'%"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 8);
  mu_assert(lsb_decode_heka_message(&m, pb, pblen - 1, NULL), "decode failed");

  unsigned long long start, end;
  for (int i = 0; tests[i]; i++) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "lsb_create_message_matcher failed: %s", tests[i]);
    start = lsb_get_time();
    mu_assert(lsb_eval_message_matcher(mm, &m),
              "lsb_eval_message_matcher failed");
    end = lsb_get_time();
    lsb_destroy_message_matcher(mm);
    printf("matcher (lsb_get_time): '%s': %g\n", tests[i],
           (double)(end - start) / 1e9);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* benchmark_match()
{
  int iter = 1000000;
  char *tests[] = {
    "TRUE"
    , "Type == 'TEST' && Severity == 6"
    , "Fields[foo] == 'bar' && Severity == 6"
    , "Fields[number] == 64 && Severity == 6"
    , "Fields[missing] == NIL"
    , "Fields[int] != NIL"
    , "Type =~ '^[Tt]EST' && Severity == 6"
    , "Payload =~ '^Test'"
    , "Payload =~ 'item$'"
    , "Payload =~ 'unique%-item'"
    , "Payload =~ 'unique-item'%"
    , "Payload =~ 'unique'"
    , "Payload =~ 'unique'%"
    , NULL };

  lsb_heka_message m;
  lsb_init_heka_message(&m, 8);
  mu_assert(lsb_decode_heka_message(&m, pb, pblen - 1, NULL), "decode failed");

  for (int i = 0; tests[i]; i++) {
    lsb_message_matcher *mm = lsb_create_message_matcher(tests[i]);
    mu_assert(mm, "lsb_create_message_matcher failed: %s", tests[i]);
    clock_t t = clock();
    for (int x = 0; x < iter; ++x) {
      mu_assert(lsb_eval_message_matcher(mm, &m),
                "lsb_eval_message_matcher failed");
    }
    t = clock() - t;
    lsb_destroy_message_matcher(mm);
    printf("matcher: '%s': %g\n", tests[i], ((double)t) / CLOCKS_PER_SEC
           / iter);
  }
  lsb_free_heka_message(&m);
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_api_assertion);
  mu_run_test(test_true_matcher);
  mu_run_test(test_nil_header_true_matcher);
  mu_run_test(test_false_matcher);
  mu_run_test(test_nil_header_false_matcher);
  mu_run_test(test_malformed_matcher);

  mu_run_test(benchmark_match_hs);
  mu_run_test(benchmark_matcher_create);
  mu_run_test(benchmark_match);
  return NULL;
}


int main()
{
  lsb_set_tz(NULL);
  char *result = all_tests();
  if (result) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", mu_tests_run);
  return result != NULL;
}
