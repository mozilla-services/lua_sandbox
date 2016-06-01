/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight/Heka message matcher implementation @file */

#include "heka_message_matcher_impl.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/heka_message_matcher.h"
#include "luasandbox/util/string.h"
#include "luasandbox/util/string_matcher.h"


static bool string_test(match_node *mn, lsb_const_string *val)
{
  switch (mn->op) {
  case OP_EQ:
    if (val->len != mn->value_len) return false;
    return strncmp(val->s, mn->value.s, val->len) == 0;
  case OP_NE:
    if (val->len != mn->value_len) return true;
    return strncmp(val->s, mn->value.s, val->len) != 0;
  case OP_LT:
    {
      int cmp = strncmp(val->s, mn->value.s, val->len);
      if (cmp == 0) {
        return val->len < mn->value_len;
      }
      return cmp < 0;
    }
  case OP_LTE:
    return strncmp(val->s, mn->value.s, val->len) <= 0;
  case OP_GT:
    {
      int cmp = strncmp(val->s, mn->value.s, val->len);
      if (cmp == 0) {
        return val->len > mn->value_len;
      }
      return cmp > 0;
    }
  case OP_GTE:
    {
      int cmp = strncmp(val->s, mn->value.s, val->len);
      if (cmp == 0) {
        return val->len == mn->value_len;
      }
      return cmp > 0;
    }
  case OP_RE:
    return lsb_string_match(val->s, val->len, mn->value.s);
  case OP_NRE:
    return !lsb_string_match(val->s, val->len, mn->value.s);
  default:
    break;
  }
  return false;
}


static bool numeric_test(match_node *mn, double val)
{
  switch (mn->op) {
  case OP_EQ:
    return val == mn->value.d;
  case OP_NE:
    return val != mn->value.d;
  case OP_LT:
    return val < mn->value.d;
  case OP_LTE:
    return val <= mn->value.d;
  case OP_GT:
    return val > mn->value.d;
  case OP_GTE:
    return val >= mn->value.d;
  default:
    break;
  }
  return false;
}


static bool eval_node(match_node *mn, lsb_heka_message *m)
{
  switch (mn->op) {
  case OP_TRUE:
    return true;
  case OP_FALSE:
    return false;
  default:
    switch (mn->id) {
    case LSB_PB_TIMESTAMP:
      return numeric_test(mn, (double)m->timestamp);
    case LSB_PB_TYPE:
      return string_test(mn, &m->type);
    case LSB_PB_LOGGER:
      return string_test(mn, &m->logger);
    case LSB_PB_SEVERITY:
      return numeric_test(mn, m->severity);
    case LSB_PB_PAYLOAD:
      return string_test(mn, &m->payload);
    case LSB_PB_ENV_VERSION:
      return string_test(mn, &m->env_version);
    case LSB_PB_PID:
      return numeric_test(mn, m->pid);
    case LSB_PB_HOSTNAME:
      return string_test(mn, &m->hostname);
    case LSB_PB_UUID:
      return string_test(mn, &m->uuid);
    default:
      {
        lsb_read_value val;
        lsb_const_string variable = { .s = mn->variable,
          .len = mn->variable_len };

        switch (mn->value_type) {
        case TYPE_STRING:
          if (lsb_read_heka_field(m, &variable, mn->fi, mn->ai, &val)
              && val.type == LSB_READ_STRING) {
            return string_test(mn, &val.u.s);
          }
          break;
        case TYPE_NUMERIC:
        case TYPE_BOOLEAN:
          if (lsb_read_heka_field(m, &variable, mn->fi, mn->ai, &val)
              && (val.type == LSB_READ_NUMERIC || val.type == LSB_READ_BOOL)) {
            return numeric_test(mn, val.u.d);
          }
          break;
        case TYPE_NIL:
          if (lsb_read_heka_field(m, &variable, mn->fi, mn->ai, &val)) {
            return mn->op == OP_NE;
          }
          return mn->op == OP_EQ;
        }
      }
      break;
    }
    break;
  }
  return false;
}


static bool eval_tree(match_node *root, match_node *mn, lsb_heka_message *m)
{
  bool match;
  if (mn->id == 0 && mn->fi) {
    match = eval_tree(root, root + mn->fi, m);
  } else {
    match = eval_node(mn, m);
  }

  if (match && mn->op == OP_OR) {
    return match; // short circuit
  }

  if (!match && mn->op == OP_AND) {
    return match; // short circuit
  }

  if (mn->id == 0 && mn->ai) {
    match = eval_tree(root, root + mn->ai, m);
  }
  return match;
}


void lsb_destroy_message_matcher(lsb_message_matcher *mm)
{
  if (!mm) return;

  for (int i = 0; i < mm->nodes[0].size; ++i) {
    free(mm->nodes[i].variable);
    switch (mm->nodes[i].value_type) {
    case TYPE_STRING:
      free(mm->nodes[i].value.s);
      break;
    default:
      // no action required
      break;
    }
  }
  free(mm->nodes);
  free(mm);
}


bool lsb_eval_message_matcher(lsb_message_matcher *mm, lsb_heka_message *m)
{
  return eval_tree(mm->nodes, mm->nodes, m);
}
