/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight/Heka message matcher implementation @file */

#include "heka_sandbox/message_matcher.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/lauxlib.h"
#include "luasandbox/lua.h"
#include "luasandbox/lualib.h"
#include "util/string_matcher.h"

static const char *grammar =
    "local l = require 'lpeg'\n"
    "local dt = require 'date_time'\n"
    "l.locale(l)\n"
    "\n"
    "local sp            = l.space^0\n"
    "local eqne          = l.Cg(l.P'==' + '!=', 'op')\n"
    "local string_match  = l.Cg(l.P'=~' + '!~', 'op')\n"
    "local relational    = l.Cg(eqne + '>=' + '>' + '<=' + '<', 'op')\n"
    "local op_and        = l.C(l.P'&&')\n"
    "local op_or         = l.C(l.P'||')\n"
    "\n"
    "local string_vars   = l.Cg(l.P'" LSB_TYPE
    "' + '" LSB_LOGGER
    "' + '" LSB_HOSTNAME
    "' + '" LSB_ENV_VERSION
    "' + '" LSB_PAYLOAD
    "' + '" LSB_UUID
    "', 'variable')\n"
    "local numeric_vars  = l.Cg(l.P'" LSB_SEVERITY
    "' + '" LSB_PID
    "', 'variable')\n"
    "local boolean       = l.Cg(l.P'TRUE' / function() return true end + l.P'FALSE' / function() return false end, 'value')\n"
    "local null          = l.P'NIL'\n"
    "local index         = l.P'[' * (l.digit^1 / tonumber) * l.P']' + l.Cc'0' / tonumber\n"
    "local fields        = l.P'Fields[' * l.Cg((l.P(1) - ']')^0, 'variable') * ']' * l.Cg(index, 'fi') * l.Cg(index, 'ai')\n"
    "\n"
    "local string_value  = l.Cg(l.P'\"' * l.Cs(((l.P(1) - l.S'\\\\\"') + l.P'\\\\\"' / '\"')^0) * '\"'\n"
    "                    + l.P'\\'' * l.Cs(((l.P(1) - l.S'\\\\\\'') + l.P'\\\\\\'' / '\\'')^0) * '\\'', 'value')\n"
    "local string_test   = l.Ct(string_vars * sp * (relational * sp * string_value + string_match * sp * string_value))\n"
    "\n"
    "local sign          = l.S'+-'^-1\n"
    "local number        = l.P'0' + l.R'19' * l.digit^0\n"
    "local decimal       = (l.P'.'^-1 * l.digit^1)^-1\n"
    "local exponent      = (l.S'eE' * sign * l.digit^1)^-1\n"
    "local numeric_value = l.Cg((sign * number * decimal * exponent) / tonumber, 'value')\n"
    "local numeric_test  = l.Ct(numeric_vars * sp * relational * sp * numeric_value)\n"
    "\n"
    "local ts_value      = l.Cg(dt.rfc3339  / dt.time_to_ns, 'value')\n"
    "local ts_quoted     = l.P'\"' * ts_value * '\"' + l.P\"'\" * ts_value * \"'\"\n"
    "local ts_test       = l.Ct(l.Cg(l.P'Timestamp', 'variable') * sp * relational * sp * (numeric_value + ts_quoted))\n"
    "\n"
    "local field_test    = l.Ct(fields * sp * (relational * sp * (string_value + numeric_value)\n"
    "                    + string_match * sp * string_value\n"
    "                    + eqne * sp * (boolean + null)))\n"
    "\n"
    "local Exp, Term, Test = l.V'Exp', l.V'Term', l.V'Test'\n"
    "g = l.P{\n"
    "Exp,\n"
    "Exp = l.Ct(Term * (op_or * sp * Term)^0);\n"
    "Term = l.Ct(Test * (op_and * sp * Test)^0) * sp;\n"
    "Test = (string_test + ts_test + numeric_test + field_test + l.Ct(l.Cg(boolean, 'op'))) * sp + l.P'(' * Exp * ')' * sp;\n"
    "}\n"
    "\n"
    "local function postfix(t, output, stack, stack_ptr)\n"
    "    local sp = stack_ptr\n"
    "    for j,v in ipairs(t) do\n"
    "        if type(v) == 'string' then\n"
    "            stack_ptr = stack_ptr + 1\n"
    "            stack[stack_ptr] = v\n"
    "        elseif type(v) == 'table' then\n"
    "            if v.op then\n"
    "                output[#output+1] = v\n"
    "            else\n"
    "                postfix(v, output, stack, stack_ptr)\n"
    "            end\n"
    "        end\n"
    "    end\n"
    "    for i=stack_ptr, sp+1, -1 do\n"
    "        output[#output+1] = stack[i]\n"
    "    end\n"
    "    stack_ptr = sp\n"
    "    return output\n"
    "end\n"
    "\n"
    "local grammar = l.Ct(sp * g * -1)\n"
    "\n"
    "function parse(s)\n"
    "    local output = {}\n"
    "    local stack = {}\n"
    "    local stack_ptr = 0\n"
    "    local t = grammar:match(s)\n"
    "    local len = 0\n"
    "    if t then\n"
    "        t = postfix(t, output, stack, stack_ptr)\n"
    "        len = #t\n"
    "    end\n"
    "    return t, len -- node array and length\n"
    "end\n";


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
  OP_OR,
  OP_AND
} match_operation;


typedef enum {
  TYPE_NIL,
  TYPE_STRING,
  TYPE_NUMERIC,
  TYPE_BOOLEAN,
  TYPE_STRING_MATCH
} match_type;


typedef struct match_node {
  struct match_node *left;
  struct match_node *right;

  char *variable;
  size_t variable_len;

  union {
    char *s;
    double  d;
  } value;
  size_t      value_len;
  match_type  value_type;

  match_operation op;
  int             id;
  int             fi;
  int             ai;
} match_node;


struct lsb_message_match_builder {
  lua_State *parser;
};


struct lsb_message_matcher {
  int nodes_size;
  match_node nodes[];
};


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
      return numeric_test(mn, m->timestamp);
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
        case TYPE_STRING_MATCH:
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


static bool eval_tree(match_node *mn, lsb_heka_message *m)
{
  bool match;
  if (mn->left) {
    match = eval_tree(mn->left, m);
  } else {
    match = eval_node(mn, m);
  }

  if (match && mn->op == OP_OR) {
    return match; // short circuit
  }

  if (!match && mn->op == OP_AND) {
    return match; // short circuit
  }

  if (mn->right) {
    match = eval_tree(mn->right, m);
  }
  return match;
}


static void load_op_node(lua_State *L, match_node *mn)
{
  const char *op = lua_tostring(L, -1);
  if (strcmp(op, "||") == 0) {
    mn->op = OP_OR;
  } else if (strcmp(op, "&&") == 0) {
    mn->op = OP_AND;
  } else {
    fprintf(stderr, "message_matcher unknown op: %s", op);
    exit(EXIT_FAILURE);
  }
}


static void load_expression_node(lua_State *L, match_node *mn)
{
  size_t len;
  const char *tmp;
  lua_getfield(L, -1, "op");
  tmp = lua_tolstring(L, -1, &len);
  if (strcmp(tmp, "==") == 0) {
    mn->op = OP_EQ;
  } else if (strcmp(tmp, "!=") == 0) {
    mn->op = OP_NE;
  } else if (strcmp(tmp, ">=") == 0) {
    mn->op = OP_GTE;
  } else if (strcmp(tmp, ">") == 0) {
    mn->op = OP_GT;
  } else if (strcmp(tmp, "<=") == 0) {
    mn->op = OP_LTE;
  } else if (strcmp(tmp, "<") == 0) {
    mn->op = OP_LT;
  } else if (strcmp(tmp, "=~") == 0) {
    mn->value_type = TYPE_STRING_MATCH;
    mn->op = OP_RE;
  } else if (strcmp(tmp, "!~") == 0) {
    mn->value_type = TYPE_STRING_MATCH;
    mn->op = OP_NRE;
  } else if (strcmp(tmp, "TRUE") == 0) {
    mn->op = OP_TRUE;
    lua_pop(L, 1);
    return; // no other args
  } else if (strcmp(tmp, "FALSE") == 0) {
    mn->op = OP_FALSE; // no other args
    lua_pop(L, 1);
    return;
  } else {
    fprintf(stderr, "message_matcher invalid op: %s", tmp);
    exit(EXIT_FAILURE);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "fi");
  if (lua_type(L, -1) == LUA_TNUMBER) {
    mn->id = LSB_PB_FIELDS;
  }
  mn->fi = lua_tointeger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "ai");
  mn->ai = lua_tointeger(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, -1, "variable");
  tmp = lua_tolstring(L, -1, &len);
  if (mn->id == LSB_PB_FIELDS) {
    mn->variable = malloc(len + 1);
    mn->variable_len = len;
    memcpy(mn->variable, tmp, len + 1);
  } else {
    if (strcmp(tmp, LSB_TIMESTAMP) == 0) {
      mn->id = LSB_PB_TIMESTAMP;
    } else if (strcmp(tmp, LSB_TYPE) == 0) {
      mn->id = LSB_PB_TYPE;
    } else if (strcmp(tmp, LSB_LOGGER) == 0) {
      mn->id = LSB_PB_LOGGER;
    } else if (strcmp(tmp, LSB_SEVERITY) == 0) {
      mn->id = LSB_PB_SEVERITY;
    } else if (strcmp(tmp, LSB_PAYLOAD) == 0) {
      mn->id = LSB_PB_PAYLOAD;
    } else if (strcmp(tmp, LSB_ENV_VERSION) == 0) {
      mn->id = LSB_PB_ENV_VERSION;
    } else if (strcmp(tmp, LSB_PID) == 0) {
      mn->id = LSB_PB_PID;
    } else if (strcmp(tmp, LSB_HOSTNAME) == 0) {
      mn->id = LSB_PB_HOSTNAME;
    } else if (strcmp(tmp, LSB_UUID) == 0) {
      mn->id = LSB_PB_UUID;
    } else {
      fprintf(stderr, "broken message_matcher grammar: invalid header");
      exit(EXIT_FAILURE);
    }
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "value");
  switch (lua_type(L, -1)) {
  case LUA_TSTRING:
    tmp = lua_tolstring(L, -1, &len);
    mn->value_type = TYPE_STRING;
    mn->value_len = len;
    mn->value.s = malloc(len + 1);
    memcpy(mn->value.s, tmp, len + 1);
    break;
  case LUA_TNUMBER:
    mn->value_type = TYPE_NUMERIC;
    mn->value.d = lua_tonumber(L, -1);
    break;
  case LUA_TBOOLEAN:
    mn->value_type = TYPE_BOOLEAN;
    mn->value.d = lua_toboolean(L, -1);
    break;
  case LUA_TNIL:
    mn->value_type = TYPE_NIL;
    break;
  default:
    fprintf(stderr, "message_matcher invalid value");
    exit(EXIT_FAILURE);
  }
  lua_pop(L, 1);
}


lsb_message_match_builder*
lsb_create_message_match_builder(const char *lua_path, const char *lua_cpath)
{

#if _WIN32
  if (_putenv("TZ=UTC") != 0) {
    return NULL;
  }
#else
  if (setenv("TZ", "UTC", 1) != 0) {
    return NULL;
  }
#endif

  lsb_message_match_builder *mmb = malloc(sizeof *mmb);
  mmb->parser = luaL_newstate();
  if (!mmb->parser) {
    free(mmb);
    return NULL;
  }

  lua_createtable(mmb->parser, 0, 1);
  lua_pushstring(mmb->parser, lua_path);
  lua_setfield(mmb->parser, -2, "path");
  lua_pushstring(mmb->parser, lua_cpath);
  lua_setfield(mmb->parser, -2, "cpath");
  lua_setfield(mmb->parser, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);

  luaL_openlibs(mmb->parser);

  if (luaL_dostring(mmb->parser, grammar)) {
    free(mmb);
    return NULL;
  }
  return mmb;
}


void lsb_destroy_message_match_builder(lsb_message_match_builder *mmb)
{
  if (mmb->parser) {
    lua_close(mmb->parser);
    mmb->parser = NULL;
  }
  free(mmb);
}


lsb_message_matcher*
lsb_create_message_matcher(const lsb_message_match_builder *mmb,
                          const char *exp)
{
  lua_getglobal(mmb->parser, "parse");
  if (!lua_isfunction(mmb->parser, -1)) {
    // todo logger fprintf(stderr, "message_matcher error: %s",
    // lua_tostring(mmb->parser, -1));
    return NULL;
  }
  lua_pushstring(mmb->parser, exp);
  if (lua_pcall(mmb->parser, 1, 2, 0)) {
    // todo logger fprintf(stderr, "message_matcher error: %s",
    // lua_tostring(mmb->parser, -1));
    return NULL;
  }

  if (lua_type(mmb->parser, 1) != LUA_TTABLE) {
    // todo logger fprintf(stderr, "parse failed");
    return NULL;
  }
  int size = lua_tointeger(mmb->parser, 2);
  lsb_message_matcher *mm = calloc(sizeof(lsb_message_matcher) +
                                   (sizeof(match_node) * size), 1);
  if (!mm) {
    // todo logger fprintf(stderr, "calloc failed");
    return NULL;
  }
  mm->nodes_size = size;

  // load in reverse order so the root node will be first
  for (int i = size, j = 0; i > 0; --i, ++j) {
    lua_rawgeti(mmb->parser, 1, i);
    switch (lua_type(mmb->parser, -1)) {
    case LUA_TSTRING:
      load_op_node(mmb->parser, &mm->nodes[j]);
      break;
    case LUA_TTABLE:
      load_expression_node(mmb->parser, &mm->nodes[j]);
      break;
    default:
      free(mm);
      //todo logeer fprintf(stderr,
      //"message_matcher error: invalid table returned");
      return NULL;
    }
    lua_pop(mmb->parser, 1);
  }
  lua_pop(mmb->parser, 2);

  // turn the postfix stack into an executable tree
  match_node **stack = calloc(sizeof(match_node *) * size, 1);
  if (!stack) {
    free(mm);
    // todo logger fprintf(stderr, "message_matcher stack allocation failed");
    return NULL;
  }

  int top = 0;
  for (int i = size - 1; i >= 0; --i) {
    if (mm->nodes[i].op != OP_AND && mm->nodes[i].op != OP_OR) {
      stack[top++] = &mm->nodes[i];
    } else {
      mm->nodes[i].right = stack[--top];
      mm->nodes[i].left = stack[--top];
      stack[top++] = &mm->nodes[i];
    }
  }
  free(stack);
  return mm;
}


void lsb_destroy_message_matcher(lsb_message_matcher *mm)
{
  for (int i = 0; i < mm->nodes_size; ++i) {
    free(mm->nodes[i].variable);
    switch (mm->nodes[i].value_type) {
    case TYPE_STRING:
    case TYPE_STRING_MATCH:
      free(mm->nodes[i].value.s);
      break;
    default:
      // no action required
      break;
    }
  }
  free(mm);
}


bool lsb_eval_message_matcher(lsb_message_matcher *mm, lsb_heka_message *m)
{
  return eval_tree(mm->nodes, m);
}
