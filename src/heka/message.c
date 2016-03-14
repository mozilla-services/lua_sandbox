/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox Heka protobuf serialization/deserialization @file */

#include "message_impl.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../luasandbox_impl.h" // todo the API should change so this doesn't
// need access to the impl

#include "luasandbox.h"
#include "luasandbox/lauxlib.h"
#include "luasandbox_output.h"
#include "sandbox_impl.h"
#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/output_buffer.h"
#include "luasandbox/util/protobuf.h"

/**
 * Adds missing headers specified in the configuration to the message output.
 *
 * @param lua Pointer the Lua state.
 * @param idx Lua stack index of the message table.
 */
static void set_missing_headers(lua_State *lua, int idx, lsb_heka_sandbox *hsb)
{
  lua_getfield(lua, idx, LSB_LOGGER);
  int t = lua_type(lua, -1);
  lua_pop(lua, 1);
  if (t == LUA_TNIL && hsb->name) {
    lua_pushstring(lua, hsb->name);
    lua_setfield(lua, idx, LSB_LOGGER);
  }

  lua_getfield(lua, idx, LSB_HOSTNAME);
  t = lua_type(lua, -1);
  lua_pop(lua, 1);
  if (t == LUA_TNIL && hsb->hostname) {
    lua_pushstring(lua, hsb->hostname);
    lua_setfield(lua, 1, LSB_HOSTNAME);
  }
}


static const char* read_string(lua_State *lua,
                               int wiretype,
                               const char *p,
                               const char *e)
{
  if (wiretype != LSB_PB_WT_LENGTH) {
    return NULL;
  }

  long long len = 0;
  p = lsb_pb_read_varint(p, e, &len);
  if (!p || len < 0 || p + len > e) {
    return NULL;
  }
  lua_pushlstring(lua, p, (size_t)len);
  p += len;
  return p;
}


static const char* process_varint(lua_State *lua,
                                  const char *name,
                                  int wiretype,
                                  int stack_index,
                                  const char *p,
                                  const char *e)
{
  if (wiretype != LSB_PB_WT_VARINT) {
    return NULL;
  }
  long long val = 0;
  p = lsb_pb_read_varint(p, e, &val);
  if (!p) {
    return NULL;
  }
  lua_pushnumber(lua, (lua_Number)val);
  lua_setfield(lua, stack_index, name);
  return p;
}


static const char* process_fields(lua_State *lua, const char *p, const char *e)
{
  int tag = 0;
  int wiretype = 0;
  int has_name = 0;
  int value_count = 0;
  long long len = 0;

  p = lsb_pb_read_varint(p, e, &len);
  if (!p || len < 0 || p + len > e) {
    return NULL;
  }
  e = p + len; // only process to the end of the current field record

  lua_newtable(lua); // Table to be added to the Fields array index 4
  lua_newtable(lua); // Table to hold the value(s) index 5
  do {
    p = lsb_pb_read_key(p, &tag, &wiretype);

    switch (tag) {
    case 1:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 4, "name");
        has_name = 1;
      }
      break;

    case 2:
      p = process_varint(lua, "value_type", wiretype, 4, p, e);
      break;

    case 3:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 4, "representation");
      }
      break;

    case 4: // value_string
    case 5: // value_bytes
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_rawseti(lua, 5, ++value_count);
      }
      break;

    case 6: // value_integer
      {
        long long val = 0;
        switch (wiretype) {
        case 0:
          p = lsb_pb_read_varint(p, p + len, &val);
          if (!p) break;
          lua_pushnumber(lua, (lua_Number)val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = lsb_pb_read_varint(p, e, &len);
          if (!p || len < 0 || p + len > e) {
            p = NULL;
            break;
          }
          do {
            p = lsb_pb_read_varint(p, p + len, &val);
            if (!p) break;
            lua_pushnumber(lua, (lua_Number)val);
            lua_rawseti(lua, 5, ++value_count);
          } while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;

    case 7: // value_double
      {
        double val = 0;
        switch (wiretype) {
        case 1:
          if (p + sizeof(double) > e) {
            p = NULL;
            break;
          }
          memcpy(&val, p, sizeof(double));
          p += sizeof(double);
          lua_pushnumber(lua, val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = lsb_pb_read_varint(p, e, &len);
          if (!p || len < 0 || p + len > e || len % sizeof(double) != 0) {
            p = NULL;
            break;
          }
          do {
            memcpy(&val, p, sizeof(double));
            p += sizeof(double);
            lua_pushnumber(lua, val);
            lua_rawseti(lua, 5, ++value_count);
          } while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;

    case 8: // value_bool
      {
        long long val = 0;
        switch (wiretype) {
        case 0:
          p = lsb_pb_read_varint(p, p + len, &val);
          if (!p) break;
          lua_pushboolean(lua, (int)val);
          lua_rawseti(lua, 5, ++value_count);
          break;
        case 2:
          p = lsb_pb_read_varint(p, e, &len);
          if (!p || len < 0 || p + len > e) {
            p = NULL;
            break;
          }
          do {
            p = lsb_pb_read_varint(p, p + len, &val);
            if (!p) break;
            lua_pushboolean(lua, (int)val);
            lua_rawseti(lua, 5, ++value_count);
          } while (p < e);
          break;
        default:
          p = NULL;
          break;
        }
      }
      break;
    default:
      p = NULL; // don't allow unknown tags
      break;
    }
  } while (p && p < e);

  lua_setfield(lua, 4, "value");

  return has_name ? p : NULL;
}


/**
 * Retrieve the string value for a Lua table entry (the table should be on top
 * of the stack).  If the entry is not found or not a string nothing is encoded.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 * @param tag Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_string(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, char tag,
              const char *name, int index)
{
  lsb_err_value ret = NULL;
  lua_getfield(lsb->lua, index, name);
  if (lua_isstring(lsb->lua, -1)) {
    size_t len;
    const char *s = lua_tolstring(lsb->lua, -1, &len);
    ret = lsb_pb_write_string(ob, tag, s, len);
  }
  lua_pop(lsb->lua, 1);
  return ret;
}


/**
 * Retrieve the numeric value for a Lua table entry (the table should be on top
 * of the stack).  If the entry is not found or not a number nothing is encoded,
 * otherwise the number is varint encoded.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 * @param tag Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_int(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, char tag,
           const char *name, int index)
{
  lsb_err_value ret = NULL;
  lua_getfield(lsb->lua, index, name);
  if (lua_isnumber(lsb->lua, -1)) {
    unsigned long long i = (unsigned long long)lua_tonumber(lsb->lua, -1);
    ret = lsb_pb_write_key(ob, tag, LSB_PB_WT_VARINT);
    if (!ret) ret = lsb_pb_write_varint(ob, i);
  }
  lua_pop(lsb->lua, 1);
  return ret;
}


/**
 * Encodes the field value.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 * @param first Boolean indicator used to add addition protobuf data in the
 *              correct order.
 * @param representation String representation of the field i.e., "ms"
 * @param value_type Protobuf value type
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_field_value(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, int first,
                   const char *representation, int value_type);


/**
 * Encodes a field that has an array of values.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 * @param t Lua type of the array values.
 * @param representation String representation of the field i.e., "ms"
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_field_array(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, int t,
                   const char *representation, int value_type)
{
  lsb_err_value ret = NULL;
  int first = (int)lua_objlen(lsb->lua, -1);
  int multiple = first > 1 ? first : 0;
  size_t len_pos = 0;
  lua_checkstack(lsb->lua, 2);
  lua_pushnil(lsb->lua);
  while (!ret && lua_next(lsb->lua, -2) != 0) {
    if (lua_type(lsb->lua, -1) != t) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE, "array has mixed types");
      return LSB_ERR_HEKA_INPUT;
    }
    ret = encode_field_value(lsb, ob, first, representation, value_type);
    if (first) {
      len_pos = ob->pos;
      first = 0;
    }
    lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                          // the next interation.
  }
  if (!ret && multiple && value_type == LSB_PB_INTEGER) {
    // fix up the varint packed length
    size_t i = len_pos - 2;
    int y = 0;
    // find the length byte
    while (ob->buf[i] != 0 && y < LSB_MAX_VARINT_BYTES) {
      --i;
      ++y;
    }
    if (y == LSB_MAX_VARINT_BYTES) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "unable set the length of the packed integer array");
      return LSB_ERR_LUA;
    }
    ret = lsb_pb_update_field_length(ob, i);
  }
  return ret;
}


/**
 * Encodes a field that contains metadata in addition to its value.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_field_object(lsb_lua_sandbox *lsb, lsb_output_buffer *ob)
{
  const char *representation = NULL;
  lua_getfield(lsb->lua, -1, "representation");
  if (lua_isstring(lsb->lua, -1)) {
    representation = lua_tostring(lsb->lua, -1);
  }

  int value_type = -1;
  lua_getfield(lsb->lua, -2, "value_type");
  if (lua_isnumber(lsb->lua, -1)) {
    value_type = (int)lua_tointeger(lsb->lua, -1);
  }

  lua_getfield(lsb->lua, -3, "value");
  lsb_err_value ret = encode_field_value(lsb, ob, 1, representation, value_type);
  lua_pop(lsb->lua, 3); // remove representation, value_type and  value
  return ret;
}


static lsb_err_value
encode_field_value(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, int first,
                   const char *representation, int value_type)
{
  lsb_err_value ret = NULL;
  size_t len;
  const char *s;

  int t = lua_type(lsb->lua, -1);
  switch (t) {
  case LUA_TSTRING:
    switch (value_type) {
    case -1: // not specified defaults to string
      value_type = 0;
    case 0:
    case 1:
      break;
    default:
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "invalid string value_type: %d", value_type);
      return LSB_ERR_HEKA_INPUT;
    }
    if (first) { // this uglyness keeps the protobuf fields in order without
                 // additional lookups
      if (value_type == LSB_PB_BYTES) {
        ret = lsb_pb_write_key(ob, LSB_PB_VALUE_TYPE, LSB_PB_WT_VARINT);
        if (!ret) ret = lsb_pb_write_varint(ob, value_type);
        if (ret) return ret;
      }
      if (representation) {
        ret = lsb_pb_write_string(ob, LSB_PB_REPRESENTATION, representation,
                                  strlen(representation));
        if (ret) return ret;
      }
    }
    s = lua_tolstring(lsb->lua, -1, &len);
    if (value_type == LSB_PB_BYTES) {
      ret = lsb_pb_write_string(ob, LSB_PB_VALUE_BYTES, s, len);
      if (ret) return ret;
    } else {
      ret = lsb_pb_write_string(ob, LSB_PB_VALUE_STRING, s, len);
      if (ret) return ret;
    }
    break;
  case LUA_TNUMBER:
    switch (value_type) {
    case -1: // not specified defaults to double
      value_type = 3;
    case 2:
    case 3:
      break;
    default:
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "invalid numeric value_type: %d", value_type);
      return LSB_ERR_HEKA_INPUT;
    }
    if (first) {
      ret = lsb_pb_write_key(ob, LSB_PB_VALUE_TYPE, LSB_PB_WT_VARINT);
      if (!ret) ret = lsb_pb_write_varint(ob, value_type);
      if (ret) return ret;

      if (representation) {
        ret = lsb_pb_write_string(ob, LSB_PB_REPRESENTATION, representation,
                                  strlen(representation));
        if (ret) return ret;
      }
      if (1 == first) {
        if (value_type == LSB_PB_INTEGER) {
          ret = lsb_pb_write_key(ob, LSB_PB_VALUE_INTEGER, LSB_PB_WT_VARINT);
        } else {
          ret = lsb_pb_write_key(ob, LSB_PB_VALUE_DOUBLE, LSB_PB_WT_FIXED64);
        }
        if (ret) return ret;
      } else { // pack array
        if (value_type == LSB_PB_INTEGER) {
          ret = lsb_pb_write_key(ob, LSB_PB_VALUE_INTEGER, LSB_PB_WT_LENGTH);
          if (!ret) ret = lsb_pb_write_varint(ob, 0); // length tbd later
        } else {
          ret = lsb_pb_write_key(ob, LSB_PB_VALUE_DOUBLE, LSB_PB_WT_LENGTH);
          if (!ret) ret = lsb_pb_write_varint(ob, first * sizeof(double));
        }
        if (ret) return ret;
      }
    }
    if (value_type == LSB_PB_INTEGER) {
      ret = lsb_pb_write_varint(ob, lua_tointeger(lsb->lua, -1));
    } else {
      ret = lsb_pb_write_double(ob, lua_tonumber(lsb->lua, -1));
    }
    if (ret) return ret;
    break;

  case LUA_TBOOLEAN:
    if (value_type != -1 && value_type != LSB_PB_BOOL) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "invalid boolean value_type: %d", value_type);
      return LSB_ERR_HEKA_INPUT;
    }
    if (first) {
      ret = lsb_pb_write_key(ob, LSB_PB_VALUE_TYPE, LSB_PB_WT_VARINT);
      if (!ret) ret = lsb_pb_write_varint(ob, LSB_PB_BOOL);
      if (ret) return ret;

      if (representation) {
        ret = lsb_pb_write_string(ob, LSB_PB_REPRESENTATION, representation,
                                  strlen(representation));
        if (ret) return ret;
      }
      if (1 == first) {
        ret = lsb_pb_write_key(ob, LSB_PB_VALUE_BOOL, LSB_PB_WT_VARINT);
      } else {
        ret = lsb_pb_write_key(ob, LSB_PB_VALUE_BOOL, LSB_PB_WT_LENGTH);
        if (!ret) ret = lsb_pb_write_varint(ob, first);
      }
      if (ret) return ret;
    }
    ret = lsb_pb_write_bool(ob, lua_toboolean(lsb->lua, -1));
    break;

  case LUA_TTABLE:
    {
      lua_rawgeti(lsb->lua, -1, 1);
      int t = lua_type(lsb->lua, -1);
      lua_pop(lsb->lua, 1); // remove the array test value
      switch (t) {
      case LUA_TNIL:
        ret = encode_field_object(lsb, ob);
        break;
      case LUA_TNUMBER:
      case LUA_TSTRING:
      case LUA_TBOOLEAN:
        ret = encode_field_array(lsb, ob, t, representation, value_type);
        break;
      default:
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "unsupported array type: %s", lua_typename(lsb->lua, t));
        return LSB_ERR_LUA;
      }
    }
    break;

  case LUA_TLIGHTUSERDATA:
    lua_getfield(lsb->lua, -4, "userdata");
    if (lua_type(lsb->lua, -1) != LUA_TUSERDATA) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "a lightuserdata output must also specify a userdata value");
      return LSB_ERR_LUA;
    }
    // fall thru

  case LUA_TUSERDATA:
    {
      lua_CFunction fp = lsb_get_output_function(lsb->lua, -1);
      size_t len_pos = 0;
      if (!fp) {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "userdata object does not implement lsb_output");
        return LSB_ERR_LUA;
      }
      if (first) {
        ret = lsb_pb_write_key(ob, LSB_PB_VALUE_TYPE, LSB_PB_WT_VARINT);
        if (ret) return ret;

        // encode userdata as a byte array
        ret = lsb_pb_write_varint(ob, LSB_PB_BYTES);
        if (ret) return ret;

        if (representation) {
          ret = lsb_pb_write_string(ob, LSB_PB_REPRESENTATION, representation,
                                    strlen(representation));
          if (ret) return ret;
        }
      }

      ret = lsb_pb_write_key(ob, LSB_PB_VALUE_BYTES, LSB_PB_WT_LENGTH);
      if (ret) return ret;

      len_pos = ob->pos;
      ret = lsb_pb_write_varint(ob, 0);  // length tbd later
      if (ret) return ret;

      lua_pushlightuserdata(lsb->lua, ob);
      int result = fp(lsb->lua);
      lua_pop(lsb->lua, 1); // remove output function
      if (result) {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "userdata output callback failed: %d", result);
        return LSB_ERR_LUA;
      }
      ret = lsb_pb_update_field_length(ob, len_pos);
    }
    if (t == LUA_TLIGHTUSERDATA) lua_pop(lsb->lua, 1); // remove the userdata
    break;

  default:
    snprintf(lsb->error_message, LSB_ERROR_SIZE, "unsupported type: %s",
             lua_typename(lsb->lua, t));
    return LSB_ERR_LUA;
  }
  return ret;
}


/**
 * Iterates over the specified Lua table encoding the contents as user defined
 * message fields.
 *
 * @param lsb  Pointer to the sandbox.
 * @param ob  Pointer to the output data buffer.
 * @param tag Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
static lsb_err_value
encode_fields(lsb_lua_sandbox *lsb, lsb_output_buffer *ob, char tag,
              const char *name, int index)
{
  lsb_err_value ret = NULL;
  lua_getfield(lsb->lua, index, name);
  if (!lua_istable(lsb->lua, -1)) {
    return ret;
  }

  lua_rawgeti(lsb->lua, -1, 1); // test for the array notation
  size_t len_pos, len;
  if (lua_istable(lsb->lua, -1)) {
    int i = 1;
    do {
      ret = lsb_pb_write_key(ob, tag, LSB_PB_WT_LENGTH);
      if (ret) return ret;

      len_pos = ob->pos;
      ret = lsb_pb_write_varint(ob, 0);  // length tbd later
      if (ret) return ret;

      lua_getfield(lsb->lua, -1, "name");
      if (lua_isstring(lsb->lua, -1)) {
        const char *s = lua_tolstring(lsb->lua, -1, &len);
        ret = lsb_pb_write_string(ob, LSB_PB_NAME, s, len);
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "field name must be a string");
        ret = LSB_ERR_HEKA_INPUT;
      }
      lua_pop(lsb->lua, 1); // remove the name
      if (ret) return ret;

      ret = encode_field_object(lsb, ob);
      if (!ret) ret = lsb_pb_update_field_length(ob, len_pos);
      if (ret) return ret;

      lua_pop(lsb->lua, 1); // remove the current field object
      lua_rawgeti(lsb->lua, -1, ++i); // grab the next field object
    } while (!ret && !lua_isnil(lsb->lua, -1));
  } else {
    lua_pop(lsb->lua, 1); // remove the array test value
    lua_checkstack(lsb->lua, 2);
    lua_pushnil(lsb->lua);
    while (lua_next(lsb->lua, -2) != 0) {
      ret = lsb_pb_write_key(ob, tag, LSB_PB_WT_LENGTH);
      if (ret) return ret;

      len_pos = ob->pos;
      ret = lsb_pb_write_varint(ob, 0);  // length tbd later
      if (ret) return ret;

      if (lua_isstring(lsb->lua, -2)) {
        const char *s = lua_tolstring(lsb->lua, -2, &len);
        ret = lsb_pb_write_string(ob, LSB_PB_NAME, s, len);
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "field name must be a string");
        ret = LSB_ERR_HEKA_INPUT;
      }
      if (ret) return ret;

      ret = encode_field_value(lsb, ob, 1, NULL, -1);
      if (!ret) ret = lsb_pb_update_field_length(ob, len_pos);
      if (ret) return ret;

      lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                            // the next interation.
    }
  }
  lua_pop(lsb->lua, 1); // remove the fields table
  return ret;
}


int heka_decode_message(lua_State *lua)
{
  int n = lua_gettop(lua);
  if (n != 1 || lua_type(lua, 1) != LUA_TSTRING) {
    return luaL_argerror(lua, 0, "must have one string argument");
  }

  size_t len;
  const char *pbstr = lua_tolstring(lua, 1, &len);
  if (len < 20) {
    return luaL_error(lua, "invalid message, too short");
  }

  const char *p = pbstr;
  const char *lp = p;
  const char *e = pbstr + len;
  int wiretype = 0;
  int tag = 0;
  int has_uuid = 0;
  int has_timestamp = 0;
  int field_count = 0;

  lua_newtable(lua); // message table index 2
  do {
    p = lsb_pb_read_key(p, &tag, &wiretype);

    switch (tag) {
    case 1:
      p = read_string(lua, wiretype, p, e);
      if (p && p - lp == 18) {
        lua_setfield(lua, 2, "Uuid");
        has_uuid = 1;
      } else {
        p = NULL;
      }
      break;

    case 2:
      p = process_varint(lua, "Timestamp", wiretype, 2, p, e);
      if (p) {
        has_timestamp = 1;
      }
      break;

    case 3:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Type");
      }
      break;

    case 4:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Logger");
      }
      break;

    case 5:
      p = process_varint(lua, "Severity", wiretype, 2, p, e);
      break;

    case 6:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Payload");
      }
      break;

    case 7:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "EnvVersion");
      }
      break;

    case 8:
      p = process_varint(lua, "Pid", wiretype, 2, p, e);
      break;

    case 9:
      p = read_string(lua, wiretype, p, e);
      if (p) {
        lua_setfield(lua, 2, "Hostname");
      }
      break;

    case 10:
      if (wiretype != 2) {
        p = NULL;
        break;
      }
      if (field_count == 0) {
        lua_newtable(lua); // Fields table index 3
      }
      p = process_fields(lua, p, e);
      if (p) {
        lua_rawseti(lua, 3, ++field_count);
      }
      break;

    default:
      p = NULL; // don't allow unknown tags
      break;
    }
    if (p) lp = p;
  } while (p && p < e);

  if (!p) {
    return luaL_error(lua, "error in tag: %d wiretype: %d offset: %d", tag,
                      wiretype, (const char *)lp - pbstr);
  }

  if (!has_uuid || !has_timestamp) {
    return luaL_error(lua, "missing required field uuid: %s timestamp: %s",
                      has_uuid ? "found" : "not found",
                      has_timestamp ? "found" : "not found");
  }

  if (field_count) {
    lua_setfield(lua, 2, "Fields");
  }

  return 1;
}


int heka_encode_message(lua_State *lua)
{
  int n = lua_gettop(lua);
  bool framed = false;

  switch (n) {
  case 2:
    luaL_checktype(lua, 2, LUA_TBOOLEAN);
    framed = lua_toboolean(lua, 2);
    // fall thru
  case 1:
    luaL_checktype(lua, 1, LUA_TTABLE);
    break;
  default:
    return luaL_argerror(lua, n, "incorrect number of arguments");
  }

  lsb_lua_sandbox *lsb = lua_touserdata(lua, lua_upvalueindex(1));
  if (!lsb) {
    return luaL_error(lua, "encode_message() invalid upvalueindex");
  }

  lsb_heka_sandbox *hsb = lsb_get_parent(lsb);
  set_missing_headers(lua, 1, hsb);

  lsb->output.pos = 0;
  lsb_err_value ret = heka_encode_message_table(lsb, 1);
  if (ret) {
    const char *err = lsb_get_error(lsb);
    if (strlen(err) == 0) err = ret;
    return luaL_error(lua, "encode_message() failed: %s", err);
  }

  size_t len = 0;
  const char *output = lsb_get_output(lsb, &len);
  lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = len;

  if (framed) {
    char header[14] = "\x1e\x00\x08"; // up to 10 varint bytes and a \x1f
    int hlen = lsb_pb_output_varint(header + 3, len) + 1;
    lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT] = len + hlen + LSB_HDR_FRAME_SIZE;
    header[1] = (char)hlen;
    header[hlen + 2] = '\x1f';
    luaL_Buffer b;
    luaL_buffinit(lua, &b);
    luaL_addlstring(&b, header, hlen + LSB_HDR_FRAME_SIZE);
    luaL_addlstring(&b, output, len);
    luaL_pushresult(&b);
  } else {
    lua_pushlstring(lua, output, len);
  }

  if (lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT]
      > lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM]) {
    lsb->usage[LSB_UT_OUTPUT][LSB_US_MAXIMUM] =
        lsb->usage[LSB_UT_OUTPUT][LSB_US_CURRENT];
  }
  return 1;
}


lsb_err_value heka_encode_message_table(lsb_lua_sandbox *lsb, int idx)
{
  lsb_err_value ret = NULL;
  lsb_output_buffer *ob = &lsb->output;
  ob->pos = 0;

  // use existing or create a type 4 uuid
  lua_getfield(lsb->lua, idx, LSB_UUID);
  size_t len;
  const char *uuid = lua_tolstring(lsb->lua, -1, &len);
  ret = lsb_write_heka_uuid(ob, uuid, len);
  lua_pop(lsb->lua, 1); // remove uuid
  if (ret) return ret;

  // use existing or create a timestamp
  lua_getfield(lsb->lua, idx, LSB_TIMESTAMP);
  long long ts;
  if (lua_isnumber(lsb->lua, -1)) {
    ts = (long long)lua_tonumber(lsb->lua, -1);
  } else {
    ts = time(NULL) * 1000000000LL;
  }
  lua_pop(lsb->lua, 1); // remove timestamp

  ret = lsb_pb_write_key(ob, LSB_PB_TIMESTAMP, LSB_PB_WT_VARINT);
  if (!ret) ret = lsb_pb_write_varint(ob, ts);
  if (!ret) ret = encode_string(lsb, ob, LSB_PB_TYPE, LSB_TYPE, idx);
  if (!ret) ret = encode_string(lsb, ob, LSB_PB_LOGGER, LSB_LOGGER, idx);
  if (!ret) ret = encode_int(lsb, ob, LSB_PB_SEVERITY, LSB_SEVERITY, idx);
  if (!ret) ret = encode_string(lsb, ob, LSB_PB_PAYLOAD, LSB_PAYLOAD, idx);
  if (!ret) ret = encode_string(lsb, ob, LSB_PB_ENV_VERSION, LSB_ENV_VERSION,
                                idx);
  if (!ret) ret = encode_int(lsb, ob, LSB_PB_PID, LSB_PID, idx);
  if (!ret) ret = encode_string(lsb, ob, LSB_PB_HOSTNAME, LSB_HOSTNAME, idx);
  if (!ret) ret = encode_fields(lsb, ob, LSB_PB_FIELDS, LSB_FIELDS, idx);
  if (!ret) ret = lsb_expand_output_buffer(ob, 1);
  ob->buf[ob->pos] = 0; // prevent possible overrun if treated as a string
  return ret;
}


int heka_read_message(lua_State *lua, lsb_heka_message *m)
{
  int n = lua_gettop(lua);
  if (n < 1 || n > 3) {
    return luaL_error(lua, "read_message() incorrect number of arguments");
  }
  size_t field_len;
  const char *field = luaL_checklstring(lua, 1, &field_len);
  int fi = (int)luaL_optinteger(lua, 2, 0);
  luaL_argcheck(lua, fi >= 0, 2, "field index must be >= 0");
  int ai = (int)luaL_optinteger(lua, 3, 0);
  luaL_argcheck(lua, ai >= 0, 3, "array index must be >= 0");

  if (strcmp(field, LSB_UUID) == 0) {
    if (m->uuid.s) {
      lua_pushlstring(lua, m->uuid.s, m->uuid.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, LSB_TIMESTAMP) == 0) {
    lua_pushnumber(lua, (lua_Number)m->timestamp);
  } else if (strcmp(field, LSB_TYPE) == 0) {
    if (m->type.s) {
      lua_pushlstring(lua, m->type.s, m->type.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, LSB_LOGGER) == 0) {
    if (m->logger.s) {
      lua_pushlstring(lua, m->logger.s, m->logger.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, LSB_SEVERITY) == 0) {
    lua_pushinteger(lua, m->severity);
  } else if (strcmp(field, LSB_PAYLOAD) == 0) {
    if (m->payload.s) {
      lua_pushlstring(lua, m->payload.s, m->payload.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, LSB_ENV_VERSION) == 0) {
    if (m->env_version.s) {
      lua_pushlstring(lua, m->env_version.s, m->env_version.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, LSB_PID) == 0) {
    lua_pushinteger(lua, m->pid);
  } else if (strcmp(field, LSB_HOSTNAME) == 0) {
    if (m->hostname.s) {
      lua_pushlstring(lua, m->hostname.s, m->hostname.len);
    } else {
      lua_pushnil(lua);
    }
  } else if (strcmp(field, "raw") == 0) {
    lua_pushlstring(lua, m->raw.s, m->raw.len);
  } else if (strcmp(field, "framed") == 0) {
    {
      char header[14] = "\x1e\x00\x08"; // up to 10 varint bytes and a \x1f
      int hlen = lsb_pb_output_varint(header + 3, m->raw.len) + 1;
      header[1] = (char)hlen;
      header[hlen + 2] = '\x1f';
      luaL_Buffer b;
      luaL_buffinit(lua, &b);
      luaL_addlstring(&b, header, hlen + 3);
      luaL_addlstring(&b, m->raw.s, m->raw.len);
      luaL_pushresult(&b);
    }
  } else if (strcmp(field, "size") == 0) {
    lua_pushnumber(lua, (lua_Number)m->raw.len);
  } else {
    if (field_len >= 8
        && memcmp(field, LSB_FIELDS "[", 7) == 0
        && field[field_len - 1] == ']') {
      lsb_read_value v;
      lsb_const_string f = { .s = field + 7, .len = field_len - 8 };
      lsb_read_heka_field(m, &f, fi, ai, &v);
      switch (v.type) {
      case LSB_READ_STRING:
        lua_pushlstring(lua, v.u.s.s, v.u.s.len);
        break;
      case LSB_READ_NUMERIC:
        lua_pushnumber(lua, v.u.d);
        break;
      case LSB_READ_BOOL:
        lua_pushboolean(lua, v.u.d ? 1 : 0);
        break;
      default:
        lua_pushnil(lua);
        break;
      }
    } else {
      lua_pushnil(lua);
    }
  }
  return 1;
}
