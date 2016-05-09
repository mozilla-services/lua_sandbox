/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Generic protobuf utility functions @file */

#include "luasandbox/util/protobuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* lsb_pb_read_key(const char *p, int *tag, int *wiretype)
{
  if (!p || !tag || !wiretype) return NULL;

  *wiretype = 7 & (unsigned char)*p;
  *tag = (unsigned char)*p >> 3;
  return ++p;
}


lsb_err_value lsb_pb_write_key(lsb_output_buffer *ob, unsigned char tag,
                               unsigned char wiretype)
{
  lsb_err_value ret = lsb_expand_output_buffer(ob, 1);
  if (!ret) {
    ob->buf[ob->pos++] = wiretype | (tag << 3);
  }
  return ret;
}


const char* lsb_pb_read_varint(const char *p, const char *e, long long *vi)
{
  if (!p || !e || !vi) {
    return NULL;
  }

  *vi = 0;
  int i, shift = 0;
  for (i = 0; p != e && i < LSB_MAX_VARINT_BYTES; ++i, ++p) {
    *vi |= ((unsigned long long)*p & 0x7f) << shift;
    shift += 7;
    if ((*p & 0x80) == 0) break;
  }
  if (i == LSB_MAX_VARINT_BYTES || p == e) {
    return NULL;
  }
  return ++p;
}


int lsb_pb_output_varint(char *buf, unsigned long long i)
{
  int pos = 0;
  if (!buf) return pos;

  if (i == 0) {
    buf[pos++] = 0;
    return pos;
  }

  while (i) {
    buf[pos++] = (i & 0x7F) | 0x80;
    i >>= 7;
  }
  buf[pos - 1] &= 0x7F; // end the varint
  return pos;
}


lsb_err_value lsb_pb_write_varint(lsb_output_buffer *ob, unsigned long long i)
{
  lsb_err_value ret = lsb_expand_output_buffer(ob, LSB_MAX_VARINT_BYTES);
  if (!ret) {
    ob->pos += lsb_pb_output_varint(ob->buf + ob->pos, i);
  }
  return ret;
}


lsb_err_value lsb_pb_write_bool(lsb_output_buffer *ob, int i)
{
  lsb_err_value ret = lsb_expand_output_buffer(ob, 1);
  if (!ret) {
    if (i) {
      ob->buf[ob->pos++] = 1;
    } else {
      ob->buf[ob->pos++] = 0;
    }
  }
  return ret;
}


lsb_err_value lsb_pb_write_double(lsb_output_buffer *ob, double i)
{
  static const size_t needed = sizeof(double);

  lsb_err_value ret = lsb_expand_output_buffer(ob, needed);
  if (!ret) {
    // todo add big endian support if necessary
    memcpy(&ob->buf[ob->pos], &i, needed);
    ob->pos += needed;
  }
  return ret;
}


lsb_err_value
lsb_pb_write_string(lsb_output_buffer *ob, char tag, const char *s, size_t len)
{
  lsb_err_value ret = lsb_pb_write_key(ob, tag, LSB_PB_WT_LENGTH);
  if (!ret) ret = lsb_pb_write_varint(ob, len);
  if (!ret) ret = lsb_expand_output_buffer(ob, len);
  if (!ret) {
    memcpy(&ob->buf[ob->pos], s, len);
    ob->pos += len;
  }
  return ret;
}


lsb_err_value lsb_pb_update_field_length(lsb_output_buffer *ob, size_t len_pos)
{
  if (len_pos >= ob->pos) {
    return LSB_ERR_UTIL_PRANGE;
  }

  size_t len = ob->pos - len_pos - 1;
  if (len < 128) {
    ob->buf[len_pos] = (char)len;
    return NULL;
  }
  size_t l = len, cnt = 0;
  while (l) {
    l >>= 7;
    ++cnt;  // compute the number of bytes needed for the varint length
  }
  size_t needed = cnt - 1;
  lsb_err_value ret = lsb_expand_output_buffer(ob, needed);
  if (!ret) {
    ob->pos += needed;
    memmove(&ob->buf[len_pos + cnt], &ob->buf[len_pos + 1], len);
    lsb_pb_output_varint(ob->buf + len_pos, len);
  }
  return ret;
}
