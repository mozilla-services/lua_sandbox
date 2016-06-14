/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** General purpose utility functions @file */

#include "luasandbox/util/util.h"
#include "../luasandbox_defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <limits.h>
#else
#include <sys/time.h>
#endif

#if defined(__MACH__) && defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

lsb_err_id LSB_ERR_UTIL_NULL    = "pointer is NULL";
lsb_err_id LSB_ERR_UTIL_OOM     = "memory allocation failed";
lsb_err_id LSB_ERR_UTIL_FULL    = "buffer full";
lsb_err_id LSB_ERR_UTIL_PRANGE  = "parameter out of range";

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
  FILE *fh = fopen(fn, "rb" CLOSE_ON_EXEC);
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
    (void)mach_timebase_info(&tbi);
    convert = tbi.numer / tbi.denom;
  }
  return mach_absolute_time() * convert;
#elif defined(_WIN32)
  static unsigned long long qpf = ULLONG_MAX;
  static_assert(sizeof(LARGE_INTEGER) == sizeof qpf, "size mismatch");

  unsigned long long t;
  if (qpf == ULLONG_MAX) QueryPerformanceFrequency((LARGE_INTEGER *)&qpf);
  if (qpf) {
    QueryPerformanceCounter((LARGE_INTEGER *)&t);
    return (t / qpf * 1000000000ULL) + ((t % qpf) * 1000000000ULL / qpf);
  } else {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    t = ft.dwHighDateTime;
    t <<= 32;
    t |= ft.dwLowDateTime;
    return t * 100ULL;
  }
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000ULL;
#endif
}


long long lsb_get_timestamp()
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#elif defined(__MACH__) && defined(__APPLE__)
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return mts.tv_sec * 1000000000LL + mts.tv_nsec;
#elif defined(_WIN32)
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  unsigned long long t = ft.dwHighDateTime;
  t <<= 32;
  t |= ft.dwLowDateTime;
  t -= 16444736000000000ULL; // convert from Jan 1 1601 to Jan 1 1970
  return t * 100LL;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000000LL + tv.tv_usec * 1000LL;
#endif
}


bool lsb_set_tz(const char *tz)
{
  if (!tz) {
    tz = "UTC";
  }
#if _WIN32
  char s[32];
  int n = _snprintf(s, sizeof(s), "TZ=%s", tz);
  if (n < 0 || n >= sizeof(s) || _putenv(s) != 0) {
    return false;
  }
#else
  if (setenv("TZ", tz, 1) != 0) {
    return false;
  }
#endif
  return true;
}


#ifdef HAVE_ZLIB
uint32_t lsb_crc32(const char *buf, size_t buf_len)
{
  uint32_t crc = (uint32_t)crc32(0L, Z_NULL, 0);
  crc = (uint32_t)crc32(crc, (const Bytef *)buf, buf_len);
  return crc;
}


char* lsb_ungzip(const char *s, size_t s_len, size_t max_len, size_t *r_len)
{
  if (!s || (max_len && s_len > max_len)) {
    return NULL;
  }
  size_t buf_len = 2 * s_len;
  if (max_len && buf_len > max_len) {
    buf_len = max_len;
  }
  unsigned char *buf = malloc(buf_len);
  if (!buf) {
    return NULL;
  }

  z_stream strm;
  strm.zalloc     = Z_NULL;
  strm.zfree      = Z_NULL;
  strm.opaque     = Z_NULL;
  strm.avail_in   = s_len;
  strm.next_in    = (unsigned char *)s;
  strm.avail_out  = buf_len;
  strm.next_out   = buf;

  int ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    free(buf);
    return NULL;
  }

  do {
    if (ret == Z_BUF_ERROR) {
      if (max_len && buf_len == max_len) {
        ret = Z_MEM_ERROR;
        break;
      }
      buf_len *= 2;
      if (max_len && buf_len > max_len) {
        buf_len = max_len;
      }
      unsigned char *tmp = realloc(buf, buf_len);
      if (!tmp) {
        ret = Z_MEM_ERROR;
        break;
      } else {
        buf = tmp;
        strm.avail_out = buf_len - strm.total_out;
        strm.next_out = buf + strm.total_out;
      }
    }
    ret = inflate(&strm, Z_FINISH);
  } while (ret == Z_BUF_ERROR && strm.avail_in > 0);

  inflateEnd(&strm);
  if (ret != Z_STREAM_END) {
    free(buf);
    return NULL;
  }
  if (r_len) *r_len = strm.total_out;
  return (char *)buf;
}
#endif
