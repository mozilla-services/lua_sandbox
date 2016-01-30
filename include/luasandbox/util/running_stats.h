/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Calculates the running count, sum, mean, variance, and standard deviation
 *  @file */

#ifndef lsb_util_running_stats_h_
#define lsb_util_running_stats_h_

#include "util.h"

typedef struct lsb_running_stats
{
  double count;
  double mean;
  double sum;
} lsb_running_stats;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Zeros out the stats counters
 *
 * @param s Stat structure to zero out
 */
LSB_UTIL_EXPORT void lsb_init_running_stats(lsb_running_stats *s);

/**
 * Value to add to the running stats
 *
 * @param s Stat structure
 * @param d Value to add
 */
LSB_UTIL_EXPORT void lsb_update_running_stats(lsb_running_stats *s, double d);

/**
 * Return the standard deviation of the stats
 *
 * @param s Stat structure
 *
 * @return double Standard deviation of the stats up to this point
 */
LSB_UTIL_EXPORT double lsb_sd_running_stats(lsb_running_stats *s);

#ifdef __cplusplus
}
#endif

#endif
