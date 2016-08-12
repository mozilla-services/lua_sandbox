/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Heka message string @file */

#ifndef luasandbox_util_string_h_
#define luasandbox_util_string_h_

#include "util.h"

typedef struct lsb_const_string
{
  const char  *s;
  size_t      len;
} lsb_const_string;


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initializes the struct to zero.
 *
 * @param s Pointer to the struct
 *
 */
LSB_UTIL_EXPORT void lsb_init_const_string(lsb_const_string *s);


/**
 * Lua string unescape. The returned string is always NUL terminated, but can
 * contain other NULs in its body.
 *
 * @param d Pointer to the destination array where the content is to be
 *          unescaped.
 * @param s C string to be unescaped
 * @param dlen The length of the destination array (must be 1 byte larger than
 *            the source string (for inclusion of the NUL terminator).  After
 *            successful conversion the final length of the escaped string is
 *            written back to this value as it may not equal strlen(d).
 *
 * @return char* A pointer to d or NULL on error.
 */
LSB_UTIL_EXPORT
char* lsb_lua_string_unescape(char *d, const char *s, size_t *dlen);

#ifdef __cplusplus
}
#endif

#endif
