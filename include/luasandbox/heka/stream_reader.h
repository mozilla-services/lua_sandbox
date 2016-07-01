/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight Heka stream reader structures @file */

#ifndef luasandbox_heka_stream_reader_h_
#define luasandbox_heka_stream_reader_h_

#include "../util/heka_message.h"
#include "../util/input_buffer.h"

#define LSB_HEKA_STREAM_READER "lsb.heka_stream_reader"

typedef struct heka_stream_reader
{
  char             *name;
  lsb_heka_message msg;
  lsb_input_buffer buf;
} heka_stream_reader;

#endif
