/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Running stats implementation @file */

#include "luasandbox/util/running_stats.h"

#include <math.h>

void lsb_init_running_stats(lsb_running_stats *s)
{
  s->count = 0.0;
  s->mean = 0.0;
  s->sum = 0.0;
}


void lsb_update_running_stats(lsb_running_stats *s, double d)
{
  if (!isfinite(d)) return;

  double old_mean = s->mean;
  double old_sum = s->sum;

  if (++s->count == 1) {
    s->mean = d;
  } else {
    s->mean = old_mean + (d - old_mean) / s->count;
    s->sum = old_sum + (d - old_mean) * (d - s->mean);
  }
}


double lsb_sd_running_stats(lsb_running_stats *s)
{
  if (s->count < 2) return 0.0;
  return sqrt(s->sum / (s->count - 1));
}

