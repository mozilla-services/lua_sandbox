/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** C API for the Lua string pattern matcher @file */

#ifndef luasandbox_util_string_matcher_h_
#define luasandbox_util_string_matcher_h_

#include <stdbool.h>
#include <stddef.h>

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Matches a string using a Lua string match pattern
 *
 * @param s String to match
 * @param len Length of the string
 * @param p Lua match pattern
 *          http://www.lua.org/manual/5.1/manual.html#pdf-string.match
 *
 * @return bool True if the sting matches the pattern
 */
LSB_UTIL_EXPORT bool lsb_string_match(const char *s, size_t len, const char *p);

#ifdef __cplusplus
}
#endif

#endif
