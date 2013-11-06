/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Sandboxed Lua execution @file

#include "lua_serialize_json.h"

////////////////////////////////////////////////////////////////////////////////
int serialize_table_as_json(lua_sandbox* lsb,
                            serialization_data* data,
                            int isHash)
{
  int result = 0;
  lua_checkstack(lsb->m_lua, 2);
  lua_pushnil(lsb->m_lua);
  int had_output = 0;
  size_t start = 0;
  while (result == 0 && lua_next(lsb->m_lua, -2) != 0) {
    if (had_output) {
      if (appendc(&lsb->m_output, ',')) return 1;
    }
    start = lsb->m_output.m_pos;
    result = serialize_kvp_as_json(lsb, data, isHash);
    lua_pop(lsb->m_lua, 1); // Remove the value leaving the key on top for
                            // the next interation.
    if (start != lsb->m_output.m_pos) {
      had_output = 1;
    } else {
      had_output = 0;
    }
  }
  if (start != 0 && had_output == 0) { // remove the trailing comma
    size_t reset_pos = start - 1;
    if (lsb->m_output.m_data[reset_pos] == ',') {
      lsb->m_output.m_data[reset_pos] = 0;
      lsb->m_output.m_pos = reset_pos;
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int serialize_data_as_json(lua_sandbox* lsb, int index, output_data* output)
{
  const char* s;
  size_t len = 0;
  size_t start_pos = output->m_pos;
  size_t escaped_len = 0;
  switch (lua_type(lsb->m_lua, index)) {
  case LUA_TNUMBER:
    if (serialize_double(output, lua_tonumber(lsb->m_lua, index))) {
      return 1;
    }
    break;
  case LUA_TSTRING:
    s = lua_tolstring(lsb->m_lua, index, &len);
    escaped_len = len + 3; // account for the quotes and terminator
    for (size_t i = 0; i < len; ++i) {
      // buffer needs at least enough room for quotes, terminator, and an
      // escaped character
      if (output->m_pos + 5 > output->m_size) {
        size_t needed = escaped_len - (output->m_pos - start_pos);
        if (realloc_output(output, needed)) return 1;
      }
      if (i == 0) {
        output->m_data[output->m_pos++] = '"';
      }
      switch (s[i]) {
      case '"':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = '"';
        ++escaped_len;
        break;
      case '\\':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = '\\';
        ++escaped_len;
        break;
      case '/':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = '/';
        ++escaped_len;
        break;
      case '\b':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = 'b';
        ++escaped_len;
        break;
      case '\f':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = 'f';
        ++escaped_len;
        break;
      case '\n':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = 'n';
        ++escaped_len;
        break;
      case '\r':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = 'r';
        ++escaped_len;
        break;
      case '\t':
        output->m_data[output->m_pos++] = '\\';
        output->m_data[output->m_pos++] = 't';
        ++escaped_len;
        break;
      default:
        output->m_data[output->m_pos++] = s[i];
      }
    }
    output->m_data[output->m_pos++] = '"';
    output->m_data[output->m_pos] = 0;
    break;
  case LUA_TBOOLEAN:
    if (appendf(output, "%s", lua_toboolean(lsb->m_lua, index) 
                ? "true" : "false")) {
      return 1;
    }
    break;
  default:
    snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
             "serialize_data_as_json cannot preserve type '%s'",
             lua_typename(lsb->m_lua, lua_type(lsb->m_lua, index)));
    return 1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
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
    if (serialize_data_as_json(lsb, kindex, &lsb->m_output)) return 1;
    if (appendc(&lsb->m_output, ':')) return 1;
  }

  if (lua_type(lsb->m_lua, vindex) == LUA_TTABLE) {
    const void* ptr = lua_topointer(lsb->m_lua, vindex);
    table_ref* seen = find_table_ref(&data->m_tables, ptr);
    if (seen == NULL) {
      seen = add_table_ref(&data->m_tables, ptr, 0);
      if (seen != NULL) {
        char start, end;
        lua_rawgeti(lsb->m_lua, vindex, 1);
        int hash = lua_isnil(lsb->m_lua, -1);
        lua_pop(lsb->m_lua, 1); // remove the test value
        if (hash) {
          start = hash_start;
          end = hash_end;
        } else {
          start = array_start;
          end = array_end;
        }
        if (appendc(&lsb->m_output, start)) return 1;
        if (serialize_table_as_json(lsb, data, hash)) return 1;
        if (appendc(&lsb->m_output, end)) return 1;
      } else {
        snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
                 "serialize table out of memory");
        return 1;
      }
    } else {
      snprintf(lsb->m_error_message, LSB_ERROR_SIZE,
               "table contains an internal or circular reference");
      return 1;
    }
  } else {
    result = serialize_data_as_json(lsb, vindex, &lsb->m_output);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
int ignore_value_type_json(lua_sandbox* lsb, int index)
{
  switch (lua_type(lsb->m_lua, index)) {
  case LUA_TTABLE:
    if (lua_getmetatable(lsb->m_lua, index) != 0) {
      lua_pop(lsb->m_lua, 1); // Remove the metatable.
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
