/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox Heka protobuf serialization @file */

#include "lsb_serialize_protobuf.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "luasandbox_output.h"
#include "lsb_private.h"

#define UUID_SIZE 16
#define MAX_VARINT_BYTES 10

/**
 * Writes a varint encoded number to the output buffer.
 *
 * @param d Pointer to the output data buffer.
 * @param i Number to be encoded.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int pb_write_varint(lsb_output_data* d, unsigned long long i)
{
  size_t needed = MAX_VARINT_BYTES;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }

  if (i == 0) {
    d->data[d->pos++] = 0;
    return 0;
  }

  while (i) {
    d->data[d->pos++] = (i & 0x7F) | 0x80;
    i >>= 7;
  }
  d->data[d->pos - 1] &= 0x7F; // end the varint
  return 0;
}


/**
 * Writes a double to the output buffer.
 *
 * @param d Pointer to the output data buffer.
 * @param i Double to be encoded.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int pb_write_double(lsb_output_data* d, double i)
{
  size_t needed = sizeof(double);
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }

  // todo add big endian support if necessary
  memcpy(&d->data[d->pos], &i, needed);
  d->pos += needed;
  return 0;
}


/**
 * Writes a bool to the output buffer.
 *
 * @param d Pointer to the output data buffer.
 * @param i Number to be encoded.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int pb_write_bool(lsb_output_data* d, int i)
{
  size_t needed = 1;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }

  if (i) {
    d->data[d->pos++] = 1;
  } else {
    d->data[d->pos++] = 0;
  }
  return 0;
}


/**
 * Writes a field tag (tag id/wire type) to the output buffer.
 *
 * @param d  Pointer to the output data buffer.
 * @param id Field identifier.
 * @param wire_type Field wire type.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int pb_write_tag(lsb_output_data* d, unsigned char id,
                        unsigned char wire_type)
{
  size_t needed = 1;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }

  d->data[d->pos++] = wire_type | (id << 3);
  return 0;
}


/**
 * Updates the field length in the output buffer once the size is known, this
 * allows for single pass encoding.
 *
 * @param d  Pointer to the output data buffer.
 * @param len_pos Position in the output buffer where the length should be
 *                written.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int update_field_length(lsb_output_data* d, size_t len_pos)
{
  size_t len = d->pos - len_pos - 1;
  if (len < 128) {
    d->data[len_pos] = (char)len;
    return 0;
  }
  size_t l = len, cnt = 0;
  while (l) {
    l >>= 7;
    ++cnt;  // compute the number of bytes needed for the varint length
  }
  size_t needed = cnt - 1;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }
  size_t end_pos = d->pos + needed;
  memmove(&d->data[len_pos + cnt], &d->data[len_pos + 1], len);
  d->pos = len_pos;
  if (pb_write_varint(d, len)) return 1;
  d->pos = end_pos;
  return 0;
}


/**
 * Writes a string to the output buffer.
 *
 * @param d  Pointer to the output data buffer.
 * @param id Field identifier.
 * @param s  String to output.
 * @param len Length of s.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
pb_write_string(lsb_output_data* d, char id, const char* s, size_t len)
{
  if (pb_write_tag(d, id, 2)) {
    return 1;
  }
  if (pb_write_varint(d, len)) {
    return 1;
  }

  size_t needed = len;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }
  memcpy(&d->data[d->pos], s, len);
  d->pos += len;
  return 0;
}


/**
 * Retrieve the string value for a Lua table entry (the table should be on top
 * of the stack).  If the entry is not found or not a string nothing is encoded.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 * @param id Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
encode_string(lua_sandbox* lsb, lsb_output_data* d, char id, const char* name,
              int index)
{
  int result = 0;
  lua_getfield(lsb->lua, index, name);
  if (lua_isstring(lsb->lua, -1)) {
    size_t len;
    const char* s = lua_tolstring(lsb->lua, -1, &len);
    result = pb_write_string(d, id, s, len);
  }
  lua_pop(lsb->lua, 1);
  return result;
}


/**
 * Retrieve the numeric value for a Lua table entry (the table should be on top
 * of the stack).  If the entry is not found or not a number nothing is encoded,
 * otherwise the number is varint encoded.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 * @param id Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
encode_int(lua_sandbox* lsb, lsb_output_data* d, char id, const char* name,
           int index)
{
  int result = 0;
  lua_getfield(lsb->lua, index, name);
  if (lua_isnumber(lsb->lua, -1)) {
    unsigned long long i = (unsigned long long)lua_tonumber(lsb->lua, -1);
    if (!(result = pb_write_tag(d, id, 0))) {
      result = pb_write_varint(d, i);
    }
  }
  lua_pop(lsb->lua, 1);
  return result;
}


/**
 * Encodes the field value.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 * @param first Boolean indicator used to add addition protobuf data in the
 *              correct order.
 * @param representation String representation of the field i.e., "ms"
 * @param value_type Protobuf value type
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
encode_field_value(lua_sandbox* lsb, lsb_output_data* d, int first,
                   const char* representation, int value_type);


/**
 * Encodes a field that has an array of values.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 * @param ltype Lua type of the array values.
 * @param representation String representation of the field i.e., "ms"
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
encode_field_array(lua_sandbox* lsb, lsb_output_data* d, int t,
                   const char* representation, int value_type)
{
  int result = 0, first = (int)lua_objlen(lsb->lua, -1);
  int multiple = first > 1 ? first : 0;
  size_t len_pos = 0;
  lua_checkstack(lsb->lua, 2);
  lua_pushnil(lsb->lua);
  while (result == 0 && lua_next(lsb->lua, -2) != 0) {
    if (lua_type(lsb->lua, -1) != t) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE, "array has mixed types");
      return 1;
    }
    result = encode_field_value(lsb, d, first, representation, value_type);
    if (first) {
      len_pos = d->pos;
      first = 0;
    }
    lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                          // the next interation.
  }
  if (multiple && value_type == 2) { // fix up the varint packed length
    size_t i = len_pos - 2;
    int y = 0;
    while (d->data[i] != 0 && y < MAX_VARINT_BYTES) { // find the length byte
      --i;
      ++y;
    }
    if (y == MAX_VARINT_BYTES || update_field_length(d, i)) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "unable set the length of the packed integer array");
      return 1;
    }
  }
  return result;
}


/**
 * Encodes a field that contains metadata in addition to its value.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int encode_field_object(lua_sandbox* lsb, lsb_output_data* d)
{
  int result = 0;
  int value_type = -1;
  const char* representation = NULL;
  lua_getfield(lsb->lua, -1, "representation");
  if (lua_isstring(lsb->lua, -1)) {
    representation = lua_tostring(lsb->lua, -1);
  }
  lua_getfield(lsb->lua, -2, "value_type");
  if (lua_isnumber(lsb->lua, -1)) {
    value_type = (int)lua_tointeger(lsb->lua, -1);
  }
  lua_getfield(lsb->lua, -3, "value");
  result = encode_field_value(lsb, d, 1, representation, value_type);
  lua_pop(lsb->lua, 3); // remove representation, value_type and  value
  return result;
}


static int
encode_field_value(lua_sandbox* lsb, lsb_output_data* d, int first,
                   const char* representation, int value_type)
{
  int result = 1;
  size_t len;
  const char* s;

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
      return 1;
    }
    if (first) { // this uglyness keeps the protobuf fields in order without
                 // additional lookups
      if (value_type == 1) {
        if (pb_write_tag(d, 2, 0)) return 1;
        if (pb_write_varint(d, value_type)) return 1;
      }
      if (representation) {
        if (pb_write_string(d, 3, representation, strlen(representation))) {
          return 1;
        }
      }
    }
    s = lua_tolstring(lsb->lua, -1, &len);
    if (value_type == 1) {
      result = pb_write_string(d, 5, s, len);
    } else {
      result = pb_write_string(d, 4, s, len);
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
      return 1;
    }
    if (first) {
      if (pb_write_tag(d, 2, 0)) return 1;
      if (pb_write_varint(d, value_type)) return 1;
      if (representation) {
        if (pb_write_string(d, 3, representation,
                            strlen(representation))) {
          return 1;
        }
      }
      if (1 == first) {
        if (value_type == 2) {
          if (pb_write_tag(d, 6, 0)) return 1;
        } else {
          if (pb_write_tag(d, 7, 1)) return 1;
        }
      } else { // pack array
        if (value_type == 2) {
          if (pb_write_tag(d, 6, 2)) return 1;
          if (pb_write_varint(d, 0)) return 1; // length tbd later
        } else {
          if (pb_write_tag(d, 7, 2)) return 1;
          if (pb_write_varint(d, first * sizeof(double))) return 1;
        }
      }
    }
    if (value_type == 2) {
      result = pb_write_varint(d, lua_tointeger(lsb->lua, -1));
    } else {
      result = pb_write_double(d, lua_tonumber(lsb->lua, -1));
    }
    break;

  case LUA_TBOOLEAN:
    if (value_type != -1 && value_type != 4) {
      snprintf(lsb->error_message, LSB_ERROR_SIZE,
               "invalid boolean value_type: %d", value_type);
      return 1;
    }
    if (first) {
      if (pb_write_tag(d, 2, 0)) return 1;
      if (pb_write_varint(d, 4)) return 1;
      if (representation) {
        if (pb_write_string(d, 3, representation,
                            strlen(representation))) {
          return 1;
        }
      }
      if (1 == first) {
        if (pb_write_tag(d, 8, 0)) return 1;
      } else {
        if (pb_write_tag(d, 8, 2)) return 1;
        if (pb_write_varint(d, first)) return 1;
      }
    }
    result = pb_write_bool(d, lua_toboolean(lsb->lua, -1));
    break;

  case LUA_TTABLE:
    {
      lua_rawgeti(lsb->lua, -1, 1);
      int t = lua_type(lsb->lua, -1);
      lua_pop(lsb->lua, 1); // remove the array test value
      switch (t) {
      case LUA_TNIL:
        result = encode_field_object(lsb, d);
        break;
      case LUA_TNUMBER:
      case LUA_TSTRING:
      case LUA_TBOOLEAN:
        result = encode_field_array(lsb, d, t, representation, value_type);
        break;
      default:
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "unsupported array type: %s", lua_typename(lsb->lua, t));
        result = 1;
        break;
      }
    }
    break;

  case LUA_TUSERDATA:
    {
      lua_CFunction fp = lsb_get_output_function(lsb->lua, -1);
      size_t len_pos = 0;
      if (!fp) {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "user data object does not implement lsb_output");
        return 1;
      }
      if (first) {
        if (pb_write_tag(d, 2, 0)) return 1;
        if (pb_write_varint(d, 1)) return 1; // encode userdata as a byte array
        if (representation) {
          if (pb_write_string(d, 3, representation, strlen(representation))) {
            return 1;
          }
        }
      }

      if (pb_write_tag(d, 5, 2)) return 1;
      len_pos = d->pos;
      if (pb_write_varint(d, 0)) return 1;  // length tbd later
      lua_pushlightuserdata(lsb->lua, d);
      if (fp(lsb->lua)) return 1;
      lua_pop(lsb->lua, 1); // remove output function
      result = update_field_length(d, len_pos);
    }
    break;

  default:
    snprintf(lsb->error_message, LSB_ERROR_SIZE, "unsupported type: %s",
             lua_typename(lsb->lua, t));
    result = 1;
    break;
  }
  return result;
}


/**
 * Iterates over the specified Lua table encoding the contents as user defined
 * message fields.
 *
 * @param lsb  Pointer to the sandbox.
 * @param d  Pointer to the output data buffer.
 * @param id Field identifier.
 * @param name Key used for the Lua table entry lookup.
 * @param index Lua stack index of the table.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
static int
encode_fields(lua_sandbox* lsb, lsb_output_data* d, char id, const char* name,
              int index)
{
  int result = 0;
  lua_getfield(lsb->lua, index, name);
  if (!lua_istable(lsb->lua, -1)) {
    return result;
  }

  lua_rawgeti(lsb->lua, -1, 1); // test for the array notation
  size_t len_pos, len;
  if (lua_istable(lsb->lua, -1)) {
    int i = 1;
    do {
      if (pb_write_tag(d, id, 2)) return 1;
      len_pos = d->pos;
      if (pb_write_varint(d, 0)) return 1;  // length tbd later
      lua_getfield(lsb->lua, -1, "name");
      if (lua_isstring(lsb->lua, -1)) {
        const char* s = lua_tolstring(lsb->lua, -1, &len);
        if (pb_write_string(d, 1, s, len)) return 1;
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "field name must be a string");
        return 1;
      }
      lua_pop(lsb->lua, 1); // remove the name

      if (encode_field_object(lsb, d)) return 1;
      if (update_field_length(d, len_pos)) return 1;

      lua_pop(lsb->lua, 1); // remove the current field object
      lua_rawgeti(lsb->lua, -1, ++i); // grab the next field object
    }
    while (!lua_isnil(lsb->lua, -1));
  } else {
    lua_pop(lsb->lua, 1); // remove the array test value
    lua_checkstack(lsb->lua, 2);
    lua_pushnil(lsb->lua);
    while (result == 0 && lua_next(lsb->lua, -2) != 0) {
      if (pb_write_tag(d, id, 2)) return 1;
      len_pos = d->pos;
      if (pb_write_varint(d, 0)) return 1;  // length tbd later
      if (lua_isstring(lsb->lua, -2)) {
        const char* s = lua_tolstring(lsb->lua, -2, &len);
        if (pb_write_string(d, 1, s, len)) return 1;
      } else {
        snprintf(lsb->error_message, LSB_ERROR_SIZE,
                 "field name must be a string");
        return 1;
      }
      if (encode_field_value(lsb, d, 1, NULL, -1)) return 1;
      if (update_field_length(d, len_pos)) return 1;
      lua_pop(lsb->lua, 1); // Remove the value leaving the key on top for
                            // the next interation.
    }
  }
  lua_pop(lsb->lua, 1); // remove the fields table
  return result;
}


int lsb_serialize_table_as_pb(lua_sandbox* lsb, int index)
{
  lsb_output_data* d = &lsb->output;
  d->pos = 0;
  size_t needed = 18;
  if (needed > d->size) {
    if (lsb_realloc_output(d, needed)) return 1;
  }

  // use existing or create a type 4 uuid
  lua_getfield(lsb->lua, index, "Uuid");
  size_t len;
  const char* uuid = lua_tolstring(lsb->lua, -1, &len);

  d->data[d->pos++] = 2 | (1 << 3);
  d->data[d->pos++] = UUID_SIZE;
  if (uuid && len == UUID_SIZE) {
    memcpy(d->data + d->pos, uuid, UUID_SIZE);
    d->pos += UUID_SIZE;
  } else {
    for (int x = 0; x < UUID_SIZE; ++x) {
      d->data[d->pos++] = rand() % 256;
    }
    d->data[8] = (d->data[8] & 0x0F) | 0x40;
    d->data[10] = (d->data[10] & 0x0F) | 0xA0;
  }
  lua_pop(lsb->lua, 1); // remove uuid

  // use existing or create a timestamp
  lua_getfield(lsb->lua, index, "Timestamp");
  unsigned long long ts;
  if (lua_isnumber(lsb->lua, -1)) {
    ts = (unsigned long long)lua_tonumber(lsb->lua, -1);
  } else {
    ts = (unsigned long long)(time(NULL) * 1e9);
  }
  lua_pop(lsb->lua, 1); // remove timestamp

  if (pb_write_tag(d, 2, 0)) return 1;
  if (pb_write_varint(d, ts)) return 1;

  if (encode_string(lsb, d, 3, "Type", index)) return 1;
  if (encode_string(lsb, d, 4, "Logger", index)) return 1;
  if (encode_int(lsb, d, 5, "Severity", index)) return 1;
  if (encode_string(lsb, d, 6, "Payload", index)) return 1;
  if (encode_string(lsb, d, 7, "EnvVersion", index)) return 1;
  if (encode_int(lsb, d, 8, "Pid", index)) return 1;
  if (encode_string(lsb, d, 9, "Hostname", index)) return 1;
  if (encode_fields(lsb, d, 10, "Fields", index)) return 1;
  // if we go above 15 pb_write_tag will need to start varint encoding
  needed = 1;
  if (needed > d->size - d->pos) {
    if (lsb_realloc_output(d, needed)) return 1;
  }
  d->data[d->pos] = 0; // NULL terminate incase someone tries to treat this
                       // as a string

  return 0;
}
