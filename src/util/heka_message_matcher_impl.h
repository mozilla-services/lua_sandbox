/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight/Heka message matcher implementation header @file */

#ifndef luasandbox_util_heka_message_matcher_impl_h_
#define luasandbox_util_heka_message_matcher_impl_h_

#include <stddef.h>
#include <stdint.h>

typedef enum {
  OP_EQ,
  OP_NE,
  OP_GTE,
  OP_GT,
  OP_LTE,
  OP_LT,
  OP_RE,
  OP_NRE,
  OP_TRUE,
  OP_FALSE,
  OP_OPEN,
  OP_OR,
  OP_AND
} match_operation;


typedef enum {
  TYPE_NIL,
  TYPE_STRING,
  TYPE_NUMERIC,
  TYPE_FALSE,
  TYPE_TRUE,
} match_type;


typedef enum {
  PATTERN_MOD_NONE,
  PATTERN_MOD_ESC,
} match_pattern_mod;


struct indices {
  uint8_t f;
  uint8_t a;
};


typedef struct match_node {
  uint8_t op;
  uint8_t units;
  uint8_t var_len;
  uint8_t val_len;
  uint8_t field_id;
  uint8_t val_type : 4;
  uint8_t val_mod  : 4;
  union {
    struct indices idx;
    uint16_t       off;
  } u;
  char data[]; // inlined field variable and value data (when necessary)
} match_node;


struct lsb_message_matcher {
  size_t bytes;
  match_node *nodes;
};

#endif
