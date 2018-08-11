/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight/Heka message matcher implementation @file */

#include "heka_message_matcher_impl.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/heka_message_matcher.h"
#include "luasandbox/util/string.h"
#include "luasandbox/util/string_matcher.h"


static bool string_test(match_node *mn, lsb_const_string *val)
{
  const char *mn_val = mn->data + mn->var_len;
  switch (mn->op) {
  case OP_EQ:
    if (val->len != mn->val_len || !val->s) return false;
    return strncmp(val->s, mn_val, val->len) == 0;
  case OP_NE:
    if (val->len != mn->val_len || !val->s) return true;
    return strncmp(val->s, mn_val, val->len) != 0;
  case OP_LT:
    {
      if (!val->s) return true;
      int cmp = strncmp(val->s, mn_val, val->len);
      return cmp == 0 ? val->len < mn->val_len : cmp < 0;
    }
  case OP_LTE:
    return val->s ? strncmp(val->s, mn_val, val->len) <= 0 : true;
  case OP_GT:
    {
      if (!val->s) return false;
      int cmp = strncmp(val->s, mn_val, val->len);
      return cmp == 0 ? val->len > mn->val_len : cmp > 0;
    }
  case OP_GTE:
    {
      if (!val->s) return false;
      int cmp = strncmp(val->s, mn_val, val->len);
      return cmp == 0 ? val->len >= mn->val_len : cmp > 0;
    }
  case OP_RE:
    if (mn->val_mod == PATTERN_MOD_ESC) {
      return lsb_string_find(val->s, val->len, mn_val, mn->val_len);
    } else {
      return lsb_string_match(val->s, val->len, mn_val);
    }
  case OP_NRE:
    if (mn->val_mod == PATTERN_MOD_ESC) {
      return !lsb_string_find(val->s, val->len, mn_val, mn->val_len);
    } else {
      return !lsb_string_match(val->s, val->len, mn_val);
    }
  default:
    break;
  }
  return false;
}


static bool numeric_test(match_node *mn, double val)
{
  double d = 0;
  memcpy(&d, mn->data + mn->var_len, sizeof(double));
  switch (mn->op) {
  case OP_EQ:
    return val == d;
  case OP_NE:
    return val != d;
  case OP_LT:
    return val < d;
  case OP_LTE:
    return val <= d;
  case OP_GT:
    return val > d;
  case OP_GTE:
    return val >= d;
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
    switch (mn->field_id) {
    case LSB_PB_TIMESTAMP:
      return numeric_test(mn, (double)m->timestamp);
    case LSB_PB_TYPE:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->type.s == NULL;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return string_test(mn, &m->type);
    case LSB_PB_LOGGER:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->logger.s == NULL;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return string_test(mn, &m->logger);
    case LSB_PB_SEVERITY:
      return numeric_test(mn, m->severity);
    case LSB_PB_PAYLOAD:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->payload.s == NULL;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return string_test(mn, &m->payload);
    case LSB_PB_ENV_VERSION:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->env_version.s == NULL;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return string_test(mn, &m->env_version);
    case LSB_PB_PID:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->pid == INT_MIN;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return numeric_test(mn, m->pid);
    case LSB_PB_HOSTNAME:
      if (mn->val_type == TYPE_NIL) {
        bool is_nil = m->hostname.s == NULL;
        return mn->op == OP_EQ  ? is_nil : !is_nil;
      }
      return string_test(mn, &m->hostname);
    case LSB_PB_UUID:
      return string_test(mn, &m->uuid);
    default:
      {
        lsb_read_value val;
        lsb_const_string variable = { .s = mn->data, .len = mn->var_len };

        if (!lsb_read_heka_field(m, &variable, mn->u.idx.f, mn->u.idx.a,
                                 &val)) {
          if (mn->val_type == TYPE_NIL) {
            return mn->op == OP_EQ;
          }
          return false;
        }

        switch (mn->val_type) {
        case TYPE_STRING:
          if (val.type == LSB_READ_STRING) {
            return string_test(mn, &val.u.s);
          }
          break;
        case TYPE_NUMERIC:
          if (val.type == LSB_READ_NUMERIC) {
            return numeric_test(mn, val.u.d);
          }
          break;
        case TYPE_TRUE:
          if (val.type == LSB_READ_BOOL || val.type == LSB_READ_NUMERIC) {
            return mn->op == OP_EQ ? val.u.d == true : val.u.d != true;
          }
          break;
        case TYPE_FALSE:
          if (val.type == LSB_READ_BOOL || val.type == LSB_READ_NUMERIC) {
            return mn->op == OP_EQ ? val.u.d == false: val.u.d != false;
          }
          break;
        case TYPE_NIL:
          return mn->op == OP_NE;
        }
      }
      break;
    }
    break;
  }
  return false;
}


void lsb_destroy_message_matcher(lsb_message_matcher *mm)
{
  if (!mm) return;
  free(mm->nodes);
  free(mm);
}


bool lsb_eval_message_matcher(lsb_message_matcher *mm, lsb_heka_message *m)
{
  bool match = false;
  if (!mm) return match;

  match_node *s = mm->nodes;
  match_node *e = mm->nodes + (mm->bytes / sizeof(match_node));
  for (match_node *p = mm->nodes; p < e;) {
    switch (p->op) {
    case OP_OR:
      if (match) {
        p = s + p->u.off; // short circuit
        continue;
      }
      break;
    case OP_AND:
      if (!match) {
        p = s + p->u.off; // short circuit
        continue;
      }
      break;
    default:
      match = eval_node(p, m);
      break;
    }
    p += p->units;
  }
  return match;
}
