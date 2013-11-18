/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Lua sandbox JSON serialization implementation @file

#include "lua_serialize_json.h"


int serialize_table_as_json(lua_sandbox* lsb,
                            serialization_data* data,
                            int isHash)
{
  int result = 0;
  lua_checkstack(lsb->lua, 2);
  lua_pushnil(lsb->lua);
  int had_output = 0;
  size_t start = 0;
  while (result == 0 && lua_next(lsb->lua, -2) != 0) {
    if (had_output) {
      if (appendc(&lsb->output, ',')) return 1;
    }
    start = lsb->output.pos;
    result = serialize_kvp_as_json(lsb, data, isHash);
    lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                          // the next interation.
    if (start != lsb->output.pos) {
      had_output = 1;
    } else {
      had_output = 0;
    }
  }
  if (start != 0 && had_output == 0) { // remove the trailing comma
    size_t reset_pos = start - 1;
    if (lsb->output.data[reset_pos] == ',') {
      lsb->output.data[reset_pos] = 0;
      lsb->output.pos = reset_pos;
    }
  }
  return result;
}


int serialize_data_as_json(lua_sandbox* lsb, int index, output_data* output)
{
  const char* s;
  size_t len = 0;
  size_t start_pos = output->pos;
  size_t escaped_len = 0;
  switch (lua_type(lsb->lua, index)) {
  case LUA_TNUMBER:
    if (serialize_double(output, lua_tonumber(lsb->lua, index))) {
      return 1;
    }
    break;
  case LUA_TSTRING:
    s = lua_tolstring(lsb->lua, index, &len);
    escaped_len = len + 3; // account for the quotes and terminator
    for (size_t i = 0; i < len; ++i) {
      // buffer needs at least enough room for quotes, terminator, and an
      // escaped character
      if (output->pos + 5 > output->size) {
        size_t needed = escaped_len - (output->pos - start_pos);
        if (realloc_output(output, needed)) return 1;
      }
      if (i == 0) {
        output->data[output->pos++] = '"';
      }
      switch (s[i]) {
      case '"':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = '"';
        ++escaped_len;
        break;
      case '\\':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = '\\';
        ++escaped_len;
        break;
      case '/':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = '/';
        ++escaped_len;
        break;
      case '\b':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = 'b';
        ++escaped_len;
        break;
      case '\f':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = 'f';
        ++escaped_len;
        break;
      case '\n':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = 'n';
        ++escaped_len;
        break;
      case '\r':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = 'r';
        ++escaped_len;
        break;
      case '\t':
        output->data[output->pos++] = '\\';
        output->data[output->pos++] = 't';
        ++escaped_len;
        break;
      default:
        output->data[output->pos++] = s[i];
      }
    }
    output->data[output->pos++] = '"';
    output->data[output->pos] = 0;
    break;
  case LUA_TBOOLEAN:
    if (appendf(output, "%s", lua_toboolean(lsb->lua, index)
                ? "true" : "false")) {
      return 1;
    }
  case LUA_TNIL:
    break;
  default:
    snprintf(lsb->error_message, LSB_ERROR_SIZE,
             "serialize_data_as_json cannot preserve type '%s'",
             lua_typename(lsb->lua, lua_type(lsb->lua, index)));
    return 1;
  }
  return 0;
}


int serialize_kvp_as_json(lua_sandbox* lsb,
                          serialization_data* data,
                          int isHash)
{
  static const char array_start = '[', array_end = ']';
  static const char hash_start = '{', hash_end = '}';
  int kindex = -2, vindex = -1, result = 0;

  if (ignore_value_type_json(lsb, vindex)) return 0;
  if (ignore_key(lsb, kindex)) return 0;
  if (isHash) {
    if (serialize_data_as_json(lsb, kindex, &lsb->output)) return 1;
    if (appendc(&lsb->output, ':')) return 1;
  }

  if (lua_type(lsb->lua, vindex) == LUA_TTABLE) {
    const void* ptr = lua_topointer(lsb->lua, vindex);
    table_ref* seen = find_table_ref(&data->tables, ptr);
    if (seen == NULL) {
      seen = add_table_ref(&data->tables, ptr, 0);
      if (seen != NULL) {
        char start, end;
        lua_rawgeti(lsb->lua, vindex, 1);
        int hash = lua_isnil(lsb->lua, -1);
        lua_pop(lsb->lua, 1); // remove the test value
        if (hash) {
          start = hash_start;
          end = hash_end;
        } else {
          start = array_start;
          end = array_end;
        }
        if (appendc(&lsb->output, start)) return 1;
        if (serialize_table_as_json(lsb, data, hash)) return 1;
        if (appendc(&lsb->output, end)) return 1;
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "serialize table out of memory");
        return 1;
      }
    } else {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "table contains an internal or circular reference");
      return 1;
    }
  } else {
    result = serialize_data_as_json(lsb, vindex, &lsb->output);
  }
  return result;
}


int ignore_value_type_json(lua_sandbox* lsb, int index)
{
  switch (lua_type(lsb->lua, index)) {
  case LUA_TTABLE:
    if (lua_getmetatable(lsb->lua, index) != 0) {
      lua_pop(lsb->lua, 1); // Remove the metatable.
      return 1;
    }
    break;
  case LUA_TUSERDATA:
  case LUA_TNONE:
  case LUA_TFUNCTION:
  case LUA_TTHREAD:
  case LUA_TLIGHTUSERDATA:
  case LUA_TNIL:
    return 1;
  }
  return 0;
}
