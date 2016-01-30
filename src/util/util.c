/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** General purpose utility functions @file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <limits.h>
#else
#include <sys/time.h>
#endif

#if defined(__MACH__) && defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "luasandbox/util/util.h"

size_t lsb_lp2(unsigned long long x)
{
  if (x == 0) return 0;
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  x = x | (x >> 32);
  return (size_t)(x + 1);
}


char* lsb_read_file(const char *fn)
{
  char *str = NULL;
  size_t b;
  FILE *fh = fopen(fn, "rb");
  if (!fh) return str;

  if (fseek(fh, 0, SEEK_END)) goto cleanup;
  long pos = ftell(fh);
  if (pos == -1) goto cleanup;
  rewind(fh);

  str = malloc(pos + 1);
  if (!str) goto cleanup;

  b = fread(str, 1, pos, fh);
  if ((long)b == pos) {
    str[pos] = 0;
  }

cleanup:
  fclose(fh);
  return str;
}


unsigned long long lsb_get_time()
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#elif defined(__MACH__) && defined(__APPLE__)
  static unsigned long long convert = 0;
  if (convert == 0) {
    mach_timebase_info_data_t tbi;
    (void) mach_timebase_info(&tbi);
    convert = tbi.numer / tbi.denom;
  }
  return mach_absolute_time() * convert;
#elif defined(_WIN32)
  static unsigned long long qpf = ULLONG_MAX;
  static_assert(sizeof(LARGE_INTEGER) == sizeof qpf, "size mismatch");

  unsigned long long t;
  if (qpf == ULLONG_MAX) QueryPerformanceFrequency((LARGE_INTEGER*)&qpf);
  if (qpf){
    QueryPerformanceCounter((LARGE_INTEGER*)&t);
    return (t / qpf * 1000000000ULL) + ((t % qpf) * 1000000000ULL / qpf);
  } else {
    GetSystemTimeAsFileTime((FILETIME*)&t);
    return t * 100ULL;
  }
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000ULL;
#endif
}
