/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Data stream output buffer implementation @file */

#include "luasandbox/util/output_buffer.h"

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/util/util.h"

#ifdef _MSC_VER
// To silence the +/-INFINITY warning
#pragma warning( disable : 4756 )
#pragma warning( disable : 4056 )
#endif

lsb_err_value
lsb_init_output_buffer(lsb_output_buffer *b, size_t max_message_size)
{
  if (!b) return LSB_ERR_UTIL_NULL;
  if (max_message_size && max_message_size < LSB_OUTPUT_SIZE) {
    b->size = max_message_size;
  } else {
    b->size = LSB_OUTPUT_SIZE;
  }
  b->maxsize = max_message_size;
  b->pos = 0;
  b->buf = malloc(b->size);
  return b->buf ? NULL : LSB_ERR_UTIL_OOM;
}


void lsb_free_output_buffer(lsb_output_buffer *b)
{
  if (!b) return;
  free(b->buf);
  b->buf = NULL;
  b->size = 0;
  b->pos = 0;
}


lsb_err_value lsb_expand_output_buffer(lsb_output_buffer *b, size_t needed)
{
  if (!b) return LSB_ERR_UTIL_NULL;

  if (needed <= b->size - b->pos) return NULL;

  if (b->maxsize && needed + b->pos > b->maxsize) {
    return LSB_ERR_UTIL_FULL;
  }

  size_t newsize = lsb_lp2(b->pos + needed);
  if (b->maxsize && newsize > b->maxsize) {
    newsize = b->maxsize;
  }

  void *ptr = realloc(b->buf, newsize);
  if (!ptr) {
    return LSB_ERR_UTIL_OOM;
  }

  b->buf = ptr;
  b->size = newsize;
  return NULL;
}


lsb_err_value lsb_outputc(lsb_output_buffer *b, char ch)
{
  if (!b) {
    return LSB_ERR_UTIL_NULL;
  }
  lsb_err_value ret = lsb_expand_output_buffer(b, 2);
  if (ret) return ret;

  b->buf[b->pos++] = ch;
  b->buf[b->pos] = 0;
  return NULL;
}


lsb_err_value lsb_outputf(lsb_output_buffer *b, const char *fmt, ...)
{
  if (!b || !fmt) {
    return LSB_ERR_UTIL_NULL;
  }

  va_list args;
  int remaining = 0;
  char *ptr = NULL, *old_ptr = NULL;
  do {
    ptr = b->buf + b->pos;
    remaining = (int)(b->size - b->pos);
    va_start(args, fmt);
    int needed = vsnprintf(ptr, remaining, fmt, args);
    va_end(args);
    if (needed == -1) {
      // Windows and Unix have different return values for this function
      // -1 on Unix is a format error
      // -1 on Windows means the buffer is too small and the required len
      // is not returned
      needed = remaining;
    }
    if (needed >= remaining) {
      if (b->maxsize && (b->size >= b->maxsize
                         || b->pos + needed >= b->maxsize)) {
        return LSB_ERR_UTIL_FULL;
      }
      size_t newsize = b->size * 2;
      while ((size_t)needed >= newsize - b->pos) {
        newsize *= 2;
      }
      if (b->maxsize && newsize > b->maxsize) {
        newsize = b->maxsize;
      }
      void *p = malloc(newsize);
      if (p != NULL) {
        memcpy(p, b->buf, b->pos);
        old_ptr = b->buf;
        b->buf = p;
        b->size = newsize;
      } else {
        return LSB_ERR_UTIL_OOM;
      }
    } else {
      b->pos += needed;
      break;
    }
  } while (1);

  free(old_ptr);
  return NULL;
}


lsb_err_value lsb_outputs(lsb_output_buffer *b, const char *str, size_t len)
{
  if (!b) {
    return LSB_ERR_UTIL_NULL;
  }
  lsb_err_value ret = lsb_expand_output_buffer(b, len + 1);
  if (ret) return ret;

  memcpy(b->buf + b->pos, str, len);
  b->pos += len;
  b->buf[b->pos] = 0;
  return ret;
}


lsb_err_value lsb_outputd(lsb_output_buffer *b, double d)
{
  if (!b) return LSB_ERR_UTIL_NULL;

  if (isnan(d)) {
    return lsb_outputs(b, "nan", 3);
  }
  if (d == INFINITY) {
    return lsb_outputs(b, "inf", 3);
  }
  if (d == -INFINITY) {
    return lsb_outputs(b, "-inf", 4);
  }
  return lsb_outputfd(b, d);
}


lsb_err_value lsb_outputfd(lsb_output_buffer *b, double d)
{
  if (!b) return LSB_ERR_UTIL_NULL;

  if (d < INT_MIN || d > INT_MAX) {
    return lsb_outputf(b, "%0.17g", d);
  }

  const int precision = 8;
  const unsigned magnitude = 100000000;
  char buffer[20];
  char *p = buffer;
  int negative = 0;

  if (d < 0) {
    negative = 1;
    d = -d;
  }

  int number = (int)d;
  double tmp = (d - number) * magnitude;
  unsigned fraction = (unsigned)tmp;
  double diff = tmp - fraction;

  if (diff > 0.5) {
    ++fraction;
    if (fraction >= magnitude) {
      fraction = 0;
      ++number;
    }
  } else if (diff == 0.5 && ((fraction == 0) || (fraction & 1))) {
    // bankers rounding
    ++fraction;
  }

  // decimal fraction
  if (fraction != 0) {
    int nodigits = 1;
    char c = 0;
    for (int x = 0; x < precision; ++x) {
      c = fraction % 10;
      if (!(c == 0 && nodigits)) {
        *p++ = c + '0';
        nodigits = 0;
      }
      fraction /= 10;
    }
    *p++ = '.';
  }

  // number
  do {
    *p++ = (number % 10) + '0';
    number /= 10;
  } while (number > 0);

  lsb_err_value ret = lsb_expand_output_buffer(b, (p - buffer) + negative + 1);
  if (!ret) {
    if (negative) {
      b->buf[b->pos++] = '-';
    }

    do {
      --p;
      b->buf[b->pos++] = *p;
    } while (p != buffer);

    b->buf[b->pos] = 0;
  }
  return ret;
}
