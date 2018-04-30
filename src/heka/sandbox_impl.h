/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight Heka sandbox private implementation @file */

#ifndef luasandbox_heka_sandbox_impl_h_
#define luasandbox_heka_sandbox_impl_h_

#include "luasandbox.h"
#include "luasandbox/heka/sandbox.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/running_stats.h"

struct heka_stats {
  unsigned long long im_cnt;
  unsigned long long im_bytes;

  unsigned long long pm_cnt;
  unsigned long long pm_failures;

  lsb_running_stats pm;
  lsb_running_stats te;
};


struct lsb_heka_sandbox {
  void                              *parent;
  lsb_lua_sandbox                   *lsb;
  lsb_heka_message                  *msg;
  char                              *name;
  char                              *hostname;
  union {
    lsb_heka_im_input           iim;
    lsb_heka_im_analysis        aim;
    lsb_heka_update_checkpoint  ucp;
  } cb;
  struct heka_stats                 stats;
  char                              type;
  bool                              restricted_headers;
  int                               pid;
};

#endif
