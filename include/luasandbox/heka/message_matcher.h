/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight/Heka message matcher @file */

#ifndef luasandbox_heka_message_matcher_h_
#define luasandbox_heka_message_matcher_h_

#include <stdbool.h>

#include "../util/heka_message.h"
#include "sandbox.h"

typedef struct lsb_message_matcher lsb_message_matcher;
typedef struct lsb_message_match_builder lsb_message_match_builder;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a Lua state containing the message matcher PEG.
 *
 * @param lua_path Path to the Lua based modules (date_time)
 * @param lua_cpath Path to the C based modules (lpeg)
 * @return NULL if the message match builder cannot be created
 *
 * @todo Remove the dependency on the sandbox and create a standalone
 * match parser in the luasandboxutil lib
 *
 */
LSB_HEKA_EXPORT lsb_message_match_builder*
lsb_create_message_match_builder(const char *lua_path, const char *lua_cpath);

/**
 * Frees all memory associated with the message matcher builder
 *
 * @param mmb Message matcher builder
 */
LSB_HEKA_EXPORT void
lsb_destroy_message_match_builder(lsb_message_match_builder *mmb);

/**
 * Parsers a message matcher expression and returns the matcher
 *
 * @param mmb Message matcher builder
 * @param exp Expression to parse into a matcher
 *
 * @return lsb_message_matcher*
 */
LSB_HEKA_EXPORT lsb_message_matcher*
lsb_create_message_matcher(const lsb_message_match_builder *mmb,
                           const char *exp);

/**
 * Frees all memory associated with a message matcher instance
 *
 * @param mm Message matcher
 */
LSB_HEKA_EXPORT void lsb_destroy_message_matcher(lsb_message_matcher *mm);

/**
 * Evaluates the message matcher against the provided message
 *
 * @param mm Message matcher
 * @param m Heka message
 *
 * @return bool True if the message is a match
 */
LSB_HEKA_EXPORT bool lsb_eval_message_matcher(lsb_message_matcher *mm,
                                         lsb_heka_message *m);

#ifdef __cplusplus
}
#endif

#endif
