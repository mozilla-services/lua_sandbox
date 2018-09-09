/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Hindsight Heka message implementation @file */

#include "luasandbox/util/heka_message.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../luasandbox_defines.h"
#include "luasandbox/util/output_buffer.h"
#include "luasandbox/util/protobuf.h"

static size_t decode_header(char *buf,
                            size_t len,
                            size_t max_message_size,
                            lsb_logger *logger)
{
  if (*buf != 0x08) {
    return 0;
  }

  char *p = buf;
  if (p && p < buf + len - 1) {
    long long vi;
    if (lsb_pb_read_varint(p + 1, buf + len, &vi)) {
      if (vi > 0 && vi <= (long long)max_message_size) {
        return (size_t)vi;
      } else {
        if (logger && logger->cb) {
          logger->cb(logger->context, __func__, 4,
                     "maximum (%lld) messages size exceeded: %lld",
                     (long long)max_message_size, vi);
        }
      }
    }
  }
  return 0;
}


static const char*
read_string(int wiretype, const char *p, const char *e, lsb_const_string *s)
{
  if (wiretype != LSB_PB_WT_LENGTH) {
    return NULL;
  }

  long long vi;
  p = lsb_pb_read_varint(p, e, &vi);
  if (!p || vi < 0 || vi > e - p) {
    return NULL;
  }
  s->s = p;
  s->len = (size_t)vi;
  return p + vi;
}


static bool
read_string_value(const char *p, const char *e, int ai, lsb_read_value *val)
{
  int acnt = 0;
  int tag = 0;
  int wiretype = 0;
  while (p && p < e) {
    val->type = LSB_READ_NIL;
    p = lsb_pb_read_key(p, &tag, &wiretype);
    p = read_string(wiretype, p, e, &val->u.s);
    if (p) {
      if (ai == acnt++) {
        val->type = LSB_READ_STRING;
        return true;
      }
    }
  }
  return false;
}


static bool
read_integer_value(const char *p, const char *e, int ai, lsb_read_value *val)
{
  int acnt = 0;
  long long ll = 0;
  while (p && p < e) {
    p = lsb_pb_read_varint(p, e, &ll);
    if (p) {
      if (ai == acnt++) {
        val->type = LSB_READ_NUMERIC;
        val->u.d = (double)ll;
        return true;
      }
    }
  }
  return false;
}


static bool
read_double_value(const char *p, const char *e, int ai, lsb_read_value *val)
{
  if (p + (sizeof(double) * (ai + 1)) > e) {
    return false;
  }
  val->type = LSB_READ_NUMERIC;
  p += sizeof(double) * ai;
  memcpy(&val->u.d, p, sizeof(double));
  return true;
}


static const char*
process_varint(int wiretype, const char *p, const char *e, long long *val)
{
  if (wiretype != 0) {
    return NULL;
  }
  p = lsb_pb_read_varint(p, e, val);
  return p ? p : NULL;
}


static const char*
process_fields(lsb_heka_field *f, const char *p, const char *e)
{
  int tag       = 0;
  int wiretype  = 0;
  long long vi  = 0;

  memset(f, 0, sizeof(lsb_heka_field));
  p = lsb_pb_read_varint(p, e, &vi);
  if (!p || vi < 0 || vi > e - p) {
    return NULL;
  }
  e = p + vi; // only process to the end of the current field record

  do {
    p = lsb_pb_read_key(p, &tag, &wiretype);

    switch (tag) {
    case LSB_PB_NAME:
      p = read_string(wiretype, p, e, &f->name);
      break;

    case LSB_PB_VALUE_TYPE:
      p = process_varint(wiretype, p, e, &vi);
      if (p) {
        f->value_type = (int)vi;
      }
      break;

    case LSB_PB_REPRESENTATION:
      p = read_string(wiretype, p, e, &f->representation);
      break;

      // don't bother with the value(s) until we actually need them
      // since this stream is created by Hindsight
      // - tags are guaranteed to be properly ordered (values at the end)
      // - there won't be repeated tags for packed values
    case LSB_PB_VALUE_STRING:
    case LSB_PB_VALUE_BYTES:
      if (wiretype != 2) {
        p = NULL;
        break;
      }
      f->value.s = p - 1;
      f->value.len = e - f->value.s;
      p = e;
      break;

    case LSB_PB_VALUE_INTEGER:
    case LSB_PB_VALUE_BOOL:
      if (wiretype != 0 && wiretype != 2) {
        p = NULL;
        break;
      }
      // fall thru
    case LSB_PB_VALUE_DOUBLE:
      if (tag == 7 && wiretype != 1 && wiretype != 2) {
        p = NULL;
        break;
      }
      if (wiretype == 2) {
        p = lsb_pb_read_varint(p, e, &vi);
        if (!p || vi < 0 || vi > e - p) {
          p = NULL;
          break;
        }
      }
      f->value.s = p;
      f->value.len = e - f->value.s;
      p = e;
      break;

    default:
      p = NULL; // don't allow unknown tags
      break;
    }
  } while (p && p < e);

  return p && f->name.s ? p : NULL;
}


bool lsb_decode_heka_message(lsb_heka_message *m,
                             const char *buf,
                             size_t len,
                             lsb_logger *logger)
{
  if (!m || !buf || len == 0) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 4, "%s", LSB_ERR_UTIL_NULL);
    }
    return false;
  }

  const char *cp  = buf;       // current position
  const char *lp  = buf;       // last position
  const char *ep  = buf + len; // end position
  int wiretype    = 0;
  int tag         = 0;
  long long val   = 0;
  bool timestamp  = false;

  lsb_clear_heka_message(m);

  do {
    cp = lsb_pb_read_key(cp, &tag, &wiretype);

    switch (tag) {
    case LSB_PB_UUID:
      cp = read_string(wiretype, cp, ep, &m->uuid);
      if (m->uuid.len != LSB_UUID_SIZE) cp = NULL;
      break;

    case LSB_PB_TIMESTAMP:
      cp = process_varint(wiretype, cp, ep, &m->timestamp);
      if (cp) timestamp = true;
      break;

    case LSB_PB_TYPE:
      cp = read_string(wiretype, cp, ep, &m->type);
      break;

    case LSB_PB_LOGGER:
      cp = read_string(wiretype, cp, ep, &m->logger);
      break;

    case LSB_PB_SEVERITY:
      cp = process_varint(wiretype, cp, ep, &val);
      if (cp) m->severity = (int)val;
      break;

    case LSB_PB_PAYLOAD:
      cp = read_string(wiretype, cp, ep, &m->payload);
      break;

    case LSB_PB_ENV_VERSION:
      cp = read_string(wiretype, cp, ep, &m->env_version);
      break;

    case LSB_PB_PID:
      cp = process_varint(wiretype, cp, ep, &val);
      if (cp) m->pid = (int)val;
      break;

    case LSB_PB_HOSTNAME:
      cp = read_string(wiretype, cp, ep, &m->hostname);
      break;

    case LSB_PB_FIELDS:
      if (wiretype != 2) {
        cp = NULL;
        break;
      }
      if (m->fields_len == m->fields_size) {
        int step = 8;
        m->fields_size += step;
        lsb_heka_field *tmp = realloc(m->fields,
                                      m->fields_size * sizeof(lsb_heka_field));
        if (!tmp) {
          if (logger && logger->cb) {
            logger->cb(logger->context, __func__, 0,
                       "fields reallocation failed");
          }
          return false;
        }
        // the new memory will be initialized as needed Issue #231
        m->fields = tmp;
      }
      cp = process_fields(&m->fields[m->fields_len], cp, ep);
      ++m->fields_len;
      break;

    default:
      cp = NULL;
      break;
    }
    if (cp) lp = cp;
  } while (cp && cp < ep);

  if (!cp) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 4, "tag:%d wiretype:%d position:%d",
                 tag, wiretype, lp - buf);
    }
    return false;
  }

  if (!m->uuid.s) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 4, "%s", "missing " LSB_UUID);
    }
    return false;
  }

  if (!timestamp) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 4, "%s", "missing " LSB_TIMESTAMP);
    }
    return false;
  }

  m->raw.s = buf;
  m->raw.len = len;
  return true;
}


bool lsb_find_heka_message(lsb_heka_message *m,
                           lsb_input_buffer *ib,
                           bool decode,
                           size_t *discarded_bytes,
                           lsb_logger *logger)
{
  if (!m || !ib || !discarded_bytes) {
    if (logger && logger->cb) {
      logger->cb(logger->context, __func__, 4, "%s", LSB_ERR_UTIL_NULL);
    }
    return false;
  }

  *discarded_bytes = 0;
  if (ib->readpos == ib->scanpos) {
    return false; // empty buffer
  }

  char *p = memchr(&ib->buf[ib->scanpos], 0x1e, ib->readpos - ib->scanpos);
  if (p) {
    if (p != ib->buf + ib->scanpos) {
      // partial buffer skipped before locating a possible header
      *discarded_bytes += p - ib->buf - ib->scanpos;
    }
    ib->scanpos = p - ib->buf;

    if (ib->readpos - ib->scanpos < 2) {
      return false; // header length is not buf
    }

    size_t hlen = (unsigned char)ib->buf[ib->scanpos + 1];
    size_t hend = ib->scanpos + hlen + 3;
    if (hend > ib->readpos) {
      return false; // header is not in buf
    }
    if (ib->buf[hend - 1] != 0x1f) {
      // invalid header length
      ++ib->scanpos;
      ++*discarded_bytes;
      size_t db;
      bool b =  lsb_find_heka_message(m, ib, decode, &db, logger);
      *discarded_bytes += db;
      return b;
    }

    if (!ib->msglen) {
      ib->msglen = decode_header(&ib->buf[ib->scanpos + 2], hlen,
                                 ib->maxsize - LSB_MAX_HDR_SIZE, logger);
    }

    if (ib->msglen) {
      size_t mend = hend + ib->msglen;
      if (mend > ib->readpos) {
        return false; // message is not in buf
      }

      if (decode) {
        if (lsb_decode_heka_message(m, &ib->buf[hend], ib->msglen, logger)) {
          ib->scanpos = mend;
          ib->msglen = 0;
          return true;
        } else {
          // message decode failure
          ++ib->scanpos;
          ++*discarded_bytes;
          ib->msglen = 0;
          size_t db;
          bool b =  lsb_find_heka_message(m, ib, decode, &db, logger);
          *discarded_bytes += db;
          return b;
        }
      } else {
        // allow a framed message is non Heka protobuf format
        lsb_clear_heka_message(m);
        m->raw.s = &ib->buf[hend];
        m->raw.len = ib->msglen;
        ib->scanpos = mend;
        ib->msglen = 0;
        return true;
      }
    } else {
      // header decode failure
      ++ib->scanpos;
      ++*discarded_bytes;
      size_t db;
      bool b =  lsb_find_heka_message(m, ib, decode, &db, logger);
      *discarded_bytes += db;
      return b;
    }
  } else {
    // full buffer skipped since no header was located
    *discarded_bytes += ib->readpos - ib->scanpos;
    ib->scanpos = ib->readpos = 0;
  }
  return false;
}


lsb_err_value lsb_init_heka_message(lsb_heka_message *m, int num_fields)
{
  if (!m) return LSB_ERR_UTIL_NULL;
  if (num_fields < 1) return LSB_ERR_UTIL_PRANGE;

  m->fields = malloc(num_fields * sizeof(lsb_heka_field));
  if (!m->fields) return LSB_ERR_UTIL_OOM;

  m->fields_size = num_fields;
  lsb_clear_heka_message(m);
  return NULL;
}


void lsb_clear_heka_message(lsb_heka_message *m)
{
  if (!m) return;

  lsb_init_const_string(&m->raw);
  lsb_init_const_string(&m->uuid);
  lsb_init_const_string(&m->type);
  lsb_init_const_string(&m->logger);
  lsb_init_const_string(&m->payload);
  lsb_init_const_string(&m->env_version);
  lsb_init_const_string(&m->hostname);

  // The fields will be cleared as they are built out anything beyond fields_len
  // should be considered uninitialized Issue #231.
  m->timestamp = 0;
  m->severity = 7;
  m->pid = INT_MIN;
  m->fields_len = 0;
}


void lsb_free_heka_message(lsb_heka_message *m)
{
  if (!m) return;

  lsb_clear_heka_message(m);
  free(m->fields);
  m->fields = NULL;
  m->fields_size = 0;
}


bool lsb_read_heka_field(const lsb_heka_message *m,
                         lsb_const_string *name,
                         int fi,
                         int ai,
                         lsb_read_value *val)
{
  if (!m || !name || !val) {
    return false;
  }

  int fcnt = 0;
  const char *p, *e;
  val->type = LSB_READ_NIL;

  for (int i = 0; i < m->fields_len; ++i) {
    if (name->len == m->fields[i].name.len
        && strncmp(name->s, m->fields[i].name.s, m->fields[i].name.len) == 0) {
      if (fi == fcnt++) {
        p = m->fields[i].value.s;
        e = p + m->fields[i].value.len;
        switch (m->fields[i].value_type) {
        case LSB_PB_STRING:
        case LSB_PB_BYTES:
          return read_string_value(p, e, ai, val);
        case LSB_PB_INTEGER:
          return read_integer_value(p, e, ai, val);
        case LSB_PB_BOOL:
          if (read_integer_value(p, e, ai, val)) {
            val->type = LSB_READ_BOOL;
            return true;
          }
          return false;
        case LSB_PB_DOUBLE:
          return read_double_value(p, e, ai, val);
        default:
          return false;
        }
      }
    }
  }
  return false;
}


lsb_err_value
lsb_write_heka_uuid(lsb_output_buffer *ob, const char *uuid, size_t len)
{
  if (!ob) {
    return LSB_ERR_UTIL_NULL;
  }

  static const size_t needed = 18;
  ob->pos = 0; // writing a uuid will always clear the buffer as it is the
               // start of a new message
  lsb_err_value ret = lsb_expand_output_buffer(ob, needed);
  if (ret) return ret;

  ob->buf[ob->pos++] = 2 | (LSB_PB_UUID << 3); // write key
  ob->buf[ob->pos++] = LSB_UUID_SIZE; // write length
  if (uuid && len == LSB_UUID_SIZE) {
    memcpy(ob->buf + ob->pos, uuid, LSB_UUID_SIZE);
    ob->pos += LSB_UUID_SIZE;
  } else if (uuid && len == LSB_UUID_STR_SIZE) {
    int cnt = sscanf(uuid, "%02hhx%02hhx%02hhx%02hhx"
                     "-%02hhx%02hhx"
                     "-%02hhx%02hhx"
                     "-%02hhx%02hhx"
                     "-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                     (unsigned char *)ob->buf + ob->pos,
                     (unsigned char *)ob->buf + ob->pos + 1,
                     (unsigned char *)ob->buf + ob->pos + 2,
                     (unsigned char *)ob->buf + ob->pos + 3,
                     (unsigned char *)ob->buf + ob->pos + 4,
                     (unsigned char *)ob->buf + ob->pos + 5,
                     (unsigned char *)ob->buf + ob->pos + 6,
                     (unsigned char *)ob->buf + ob->pos + 7,
                     (unsigned char *)ob->buf + ob->pos + 8,
                     (unsigned char *)ob->buf + ob->pos + 9,
                     (unsigned char *)ob->buf + ob->pos + 10,
                     (unsigned char *)ob->buf + ob->pos + 11,
                     (unsigned char *)ob->buf + ob->pos + 12,
                     (unsigned char *)ob->buf + ob->pos + 13,
                     (unsigned char *)ob->buf + ob->pos + 14,
                     (unsigned char *)ob->buf + ob->pos + 15);
    if (cnt == LSB_UUID_SIZE) {
      ob->pos += cnt;
    }
  }

  if (ob->pos == 2) { // only the header has been written
    for (int x = 0; x < LSB_UUID_SIZE; ++x) {
      ob->buf[ob->pos++] = rand() % 256;
    }
    ob->buf[8] = (ob->buf[8] & 0x0F) | 0x40;
    ob->buf[10] = (ob->buf[10] & 0x0F) | 0xA0;
  }
  return NULL;
}


size_t lsb_write_heka_header(char *buf, size_t len)
{
  int hlen = lsb_pb_output_varint(buf + 3, len) + 1;
  buf[hlen + 2] = '\x1f';
  buf[0] = '\x1e';
  buf[1] = (char)hlen;
  buf[2] = '\x08';
  return LSB_HDR_FRAME_SIZE + hlen;
}
