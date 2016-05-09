/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight/Heka message matcher @file */

#ifndef luasandbox_util_heka_message_matcher_h_
#define luasandbox_util_heka_message_matcher_h_

#include <stdbool.h>

#include "heka_message.h"

typedef struct lsb_message_matcher lsb_message_matcher;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Parses a message matcher expression and returns the matcher
 *
 * @param exp Expression to parse into a matcher
 *
 * @return lsb_message_matcher*
 */
LSB_UTIL_EXPORT lsb_message_matcher*
lsb_create_message_matcher(const char *exp);

/**
 * Frees all memory associated with a message matcher instance
 *
 * @param mm Message matcher
 */
LSB_UTIL_EXPORT void lsb_destroy_message_matcher(lsb_message_matcher *mm);

/**
 * Evaluates the message matcher against the provided message
 *
 * @param mm Message matcher
 * @param m Heka message
 *
 * @return bool True if the message is a match
 */
LSB_UTIL_EXPORT bool
lsb_eval_message_matcher(lsb_message_matcher *mm, lsb_heka_message *m);

#ifdef __cplusplus
}
#endif

#endif
