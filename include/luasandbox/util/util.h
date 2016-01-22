/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Shared types and structures @file */

#ifndef luasandbox_util_util_h_
#define luasandbox_util_util_h_

#include <time.h>

#include "luasandbox.h"
#include "output_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hacker's Delight - Henry S. Warren, Jr. page 48
 *
 * @param x
 *
 * @return size_t Least power of 2 greater than or equal to x
 */
LSB_EXPORT size_t lsb_lp2(unsigned long long x);


/**
 * Computes the duration between two timespec structures.
 *
 * @param s Start time
 * @param e End time
 *
 * @return double Time difference in fractional seconds
 *
 */
LSB_EXPORT double lsb_timespec_delta(const struct timespec* s,
                                     const struct timespec* e);

#ifdef __cplusplus
}
#endif

#endif
