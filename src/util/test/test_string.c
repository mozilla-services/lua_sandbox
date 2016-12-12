/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_input_buffer unit tests @file */

#include <stdlib.h>
#include <string.h>

#include "luasandbox/test/mu_test.h"
#include "luasandbox/util/string.h"

static char* test_stub()
{
  return NULL;
}

typedef struct {
  char *s;
  char *r;
  size_t len;
} testcase;


static char* test_success()
{
  testcase tests[] = {
    {"TRUE"         ,"TRUE"   ,4},
    {"\\a"          ,"\a"     ,1},
    {"\\b"          ,"\b"     ,1},
    {"\\f"          ,"\f"     ,1},
    {"\\n"          ,"\n"     ,1},
    {"\\r"          ,"\r"     ,1},
    {"\\t"          ,"\t"     ,1},
    {"\\v"          ,"\v"     ,1},
    {"\\\""         ,"\""     ,1},
    {"\\'"          ,"'"      ,1},
    {"\\\\"         ,"\\"     ,1},
    {"\\?"          ,"?"      ,1},
    {"\\1"          ,"\1"     ,1},
    {"\\33"         ,"!"      ,1},
    {"\\109"        ,"m"      ,1},
    {"+\\99\\1009+" ,"+cd9+"  ,5},
    {"1\\000M"      ,"1\x00M" ,3},
  };

  for (unsigned i = 0; i < sizeof tests / sizeof(testcase); ++i){
    testcase *tc = &tests[i];
    size_t len = strlen(tc->s) + 1;
    char *d = malloc(len);
    char *us = lsb_lua_string_unescape(d, tc->s, &len);
    mu_assert(us, "test: %d valid string: %s", i, tc->s);
    mu_assert(len == tc->len, "test: %d string: %s expected len: %" PRIuSIZE
              " received: %" PRIuSIZE, i, tc->s, tc->len, len);
    mu_assert(memcmp(us, tc->r, len) == 0, "test: %d memcmp string: %s", i,
              tc->s);
    free(d);
  }
  return NULL;
}


static char* test_failure()
{
  testcase tests[] = {
    {"\\p"    ,NULL ,2}, // invalid escape char
    {"\\999"  ,NULL ,4}, // invalid char value
    {"foo"    ,NULL ,2}, // destination too small
    {NULL     ,NULL ,2}, // NULL source
  };

  for (unsigned i = 0; i < sizeof tests / sizeof(testcase); ++i){
    testcase *tc = &tests[i];
    size_t len = tc->len + 1;
    char *d = malloc(len);
    mu_assert(!lsb_lua_string_unescape(d, tc->s, &len),
              "test: %d invalid string: %s", i, tc->s);
    free(d);
  }
  mu_assert(!lsb_lua_string_unescape(NULL, tests[0].s, &tests[0].len),
            "NULL destination");

  char d[3];
  mu_assert(!lsb_lua_string_unescape(d, tests[0].s, NULL), "NULL length");
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_success);
  mu_run_test(test_failure);
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
