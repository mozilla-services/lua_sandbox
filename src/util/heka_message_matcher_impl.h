/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight/Heka message matcher implementation header @file */

#ifndef luasandbox_util_heka_message_matcher_impl_h_
#define luasandbox_util_heka_message_matcher_impl_h_

#include <stddef.h>

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
  TYPE_BOOLEAN,
} match_type;


typedef struct match_node {
  unsigned char id;
  unsigned char op;
  unsigned char size;
  unsigned char value_type;
  unsigned char value_len;
  unsigned char variable_len;
  unsigned char fi; // left node index for logical op
  unsigned char ai; // right node index for logical op
  char          *variable;

  union {
    char    *s;
    double  d;
  } value;
} match_node;


struct lsb_message_matcher {
  match_node *nodes;
};

#endif
