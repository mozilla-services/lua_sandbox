/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** String functions @file */

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "luasandbox/util/string.h"

void lsb_init_const_string(lsb_const_string *s)
{
  s->s = NULL;
  s->len = 0;
}


char* lsb_lua_string_unescape(char *d, const char *s, size_t *dlen)
{
  if (!s || !d || !dlen || *dlen <= strlen(s)) {
    return NULL;
  }

  int x = 0;
  int y = 0;
  while (s[x]) {
    switch (s[x]) {
    case '\\':
      ++x;
      switch (s[x]) {
      case 'a':
        d[y++] = '\a'; break;
      case 'b':
        d[y++] = '\b'; break;
      case 'f':
        d[y++] = '\f'; break;
      case 'n':
        d[y++] = '\n'; break;
      case 'r':
        d[y++] = '\r'; break;
      case 't':
        d[y++] = '\t'; break;
      case 'v':
        d[y++] = '\v'; break;
      default:
        if (!isdigit(s[x])) {
          switch (s[x]) {
          case '"':
          case '\'':
          case '?':
          case '\\':
            break;
          default:
            return NULL;
          }
          d[y++] = s[x];
        } else {  /* \xxx */
          int n = 0;
          int c = 0;
          do {
            c = 10 * c + (s[x++] - '0');
          } while (++n < 3 && isdigit(s[x]));
          if (c > UCHAR_MAX) return NULL;
          d[y++] = (char)c;
          --x;
        }
      }
      ++x;
      break;

    default:
      d[y++] = s[x++];
      break;
    }
  }
  d[y] = 0;
  *dlen = y;
  return d;
}
