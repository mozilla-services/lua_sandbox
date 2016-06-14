/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Shared types and structures @file */

#ifndef luasandbox_util_util_h_
#define luasandbox_util_util_h_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../error.h"

#ifdef _WIN32
#ifdef luasandboxutil_EXPORTS
#define LSB_UTIL_EXPORT __declspec(dllexport)
#else
#define LSB_UTIL_EXPORT __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define LSB_UTIL_EXPORT __attribute__ ((visibility ("default")))
#else
#define LSB_UTIL_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

LSB_UTIL_EXPORT extern lsb_err_id LSB_ERR_UTIL_NULL;
LSB_UTIL_EXPORT extern lsb_err_id LSB_ERR_UTIL_OOM;
LSB_UTIL_EXPORT extern lsb_err_id LSB_ERR_UTIL_FULL;
LSB_UTIL_EXPORT extern lsb_err_id LSB_ERR_UTIL_PRANGE;

/**
 * Hacker's Delight - Henry S. Warren, Jr. page 48
 *
 * @param x
 *
 * @return size_t Least power of 2 greater than or equal to x
 */
LSB_UTIL_EXPORT size_t lsb_lp2(unsigned long long x);

/**
 * Read a file into a string
 *
 * @param fn Filename to read
 *
 * @return char* NULL on failure otherwise a pointer to the file contents (must
 *         be freed by the caller).
 */
LSB_UTIL_EXPORT char* lsb_read_file(const char *fn);

/**
 * Retrieves the highest resolution timer available converted to nanoseconds
 *
 * @return unsigned long long
 */
LSB_UTIL_EXPORT unsigned long long lsb_get_time();


/**
 * Retrieves the highest resolution time since Jan 1, 1970 converted to
 * nanoseconds
 *
 * @return unsigned long long (time_ns)
 */
LSB_UTIL_EXPORT long long lsb_get_timestamp();

/**
 * Sets the timezone environment variable for the time conversion functions
 *
 * @param tz Timezone string (if NULL uses UTC)
 *
 * @return bool True if the environment variable is successfully set
 */
LSB_UTIL_EXPORT bool lsb_set_tz(const char *tz);

/**
 * ZLIB CRC-32 checksum
 *
 * @param buf - array of bytes
 * @param buf_len - length of the array
 *
 * @return LSB_UTIL_EXPORT unsigned checksum
 */
LSB_UTIL_EXPORT uint32_t lsb_crc32(const char* buf, size_t buf_len);

/**
 * ZLIB Inflate the provided string into a new string
 *
 * @param s String to ungzip
 * @param s_len Length of the string to ungzip
 * @param max_len Maximum length the buffer is allowed to grow to (0 for
 *                unlimited)
 * @param r_len (optional) Length of the returned string
 * @return char* Returned string (MUST be freed by the caller), NULL on failure
 */
LSB_UTIL_EXPORT char* lsb_ungzip(const char *s, size_t s_len, size_t max_len,
                                  size_t *r_len);

#ifdef __cplusplus
}
#endif

#endif
