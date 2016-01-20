/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lsb_input_buffer unit tests @file */

#include <string.h>

#include "../../test/mu_test.h"
#include "util/string_matcher.h"

static char* test_stub()
{
  return NULL;
}


static char* test_success()
{
  char *tests[] = {
    " "  , "."
    , "foobar", "foo"
    , "foobar", "bar$"
    , "a"     , "%a"
    , "1"     , "%d"
    , "a"     , "%l"
    , ","     , "%p"
    , "\t"    , "%s"
    , "A"     , "%u"
    , "a"     , "%w"
    , "f"     , "%x"
    , "%"     , "%%"
    , "a"     , "[a-c]"
    , "d"     , "[^a-c]"
    , "abc"   , "^%a+$"
    , "("     , "("
    , "|foo|" , "%b||"
    , "?"     , "%?"
    , "+"     , "%+"
    , "-"     , "%-"
    , "*"     , "%*"
    , "%"     , "%%"
    , "."     , "%."
    , "^"     , "%^"
    , "$"     , "%$"
    , "["     , "%["
    , "]"     , "%]"
    , "]"     , "]"
    , "("    , "%("
    , ")"    , "%)"
    , ")"    , ")"
    , NULL
  };

  for (int i = 0; tests[i]; i += 2) {
    mu_assert(lsb_string_match(tests[i], strlen(tests[i]), tests[i + 1]),
              "no match test: %d string: %s pattern: %s", i / 2, tests[i],
              tests[i + 1]);
  }
  mu_assert(lsb_string_match("\0", 1, "%z"), "NULL match");
  return NULL;
}


static char* test_failure()
{
  char *tests[] = {
    ""  , "."
    , "_foobar", "^foo"
    , "foobar_", "bar$"
    , "1"     , "%a"
    , "a"     , "%d"
    , "A"     , "%l"
    , "a"     , "%p"
    , "."     , "%s"
    , "a"     , "%u"
    , "."     , "%w"
    , "p"     , "%x"
    , "a"     , "%%"
    , "d"     , "[a-c]"
    , "a"     , "[^a-c]"
    , "a."    , "^%a+$"
    , "foo()" , "bar()"
    , "|foo"  , "%b||"
    , "aa"    , "(%a)%1*"
    , NULL
  };

  for (int i = 0; tests[i]; i += 2) {
    mu_assert(!lsb_string_match(tests[i], strlen(tests[i]), tests[i + 1]),
              "match test: %d string: %s pattern: %s", i / 2, tests[i],
              tests[i + 1]);
  }
  return NULL;
}


static char* test_invalid()
{
  char *tests[] = {
    ""    , "[f"
    , ""  , "%b|"
    , ""  , "?"
    , ""  , "+"
    , ""  , "-"
    , ""  , "*"
    , ""  , "%"
    , NULL
  };

  for (int i = 0; tests[i]; i += 2) {
    mu_assert(!lsb_string_match(tests[i], strlen(tests[i]), tests[i + 1]),
              "invalid test: %d string: %s pattern: %s", i / 2, tests[i],
              tests[i + 1]);
  }
  return NULL;
}


static char* all_tests()
{
  mu_run_test(test_stub);
  mu_run_test(test_success);
  mu_run_test(test_failure);
  mu_run_test(test_invalid);
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
