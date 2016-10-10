/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Heka message representation @file */

#ifndef luasandbox_util_heka_message_h_
#define luasandbox_util_heka_message_h_

#include <stdbool.h>
#include <stddef.h>

#include "input_buffer.h"
#include "output_buffer.h"
#include "string.h"
#include "util.h"

#define LSB_UUID_SIZE         16
#define LSB_UUID_STR_SIZE     36
#define LSB_HDR_FRAME_SIZE    3
#define LSB_MIN_HDR_SIZE      14
#define LSB_MAX_HDR_SIZE      (255 + LSB_HDR_FRAME_SIZE)

#define LSB_UUID          "Uuid"
#define LSB_TIMESTAMP     "Timestamp"
#define LSB_TYPE          "Type"
#define LSB_LOGGER        "Logger"
#define LSB_SEVERITY      "Severity"
#define LSB_PAYLOAD       "Payload"
#define LSB_ENV_VERSION   "EnvVersion"
#define LSB_PID           "Pid"
#define LSB_HOSTNAME      "Hostname"
#define LSB_FIELDS        "Fields"
#define LSB_RAW           "raw"
#define LSB_FRAMED        "framed"
#define LSB_SIZE          "size"

typedef enum {
  LSB_PB_UUID         = 1,
  LSB_PB_TIMESTAMP    = 2,
  LSB_PB_TYPE         = 3,
  LSB_PB_LOGGER       = 4,
  LSB_PB_SEVERITY     = 5,
  LSB_PB_PAYLOAD      = 6,
  LSB_PB_ENV_VERSION  = 7,
  LSB_PB_PID          = 8,
  LSB_PB_HOSTNAME     = 9,
  LSB_PB_FIELDS       = 10
} lsb_pb_message;

typedef enum {
  LSB_PB_NAME           = 1,
  LSB_PB_VALUE_TYPE     = 2,
  LSB_PB_REPRESENTATION = 3,
  LSB_PB_VALUE_STRING   = 4,
  LSB_PB_VALUE_BYTES    = 5,
  LSB_PB_VALUE_INTEGER  = 6,
  LSB_PB_VALUE_DOUBLE   = 7,
  LSB_PB_VALUE_BOOL     = 8
} lsb_pb_field;

typedef enum {
  LSB_PB_STRING   = 0,
  LSB_PB_BYTES    = 1,
  LSB_PB_INTEGER  = 2,
  LSB_PB_DOUBLE   = 3,
  LSB_PB_BOOL     = 4
} lsb_pb_value_types;

typedef struct lsb_heka_field
{
  lsb_const_string    name;
  lsb_const_string    representation;
  lsb_const_string    value;
  lsb_pb_value_types  value_type;
} lsb_heka_field;

typedef struct lsb_heka_message
{
  lsb_const_string raw;
  lsb_const_string uuid;
  lsb_const_string type;
  lsb_const_string logger;
  lsb_const_string payload;
  lsb_const_string env_version;
  lsb_const_string hostname;

  lsb_heka_field *fields;

  long long timestamp;
  int       severity;
  int       pid;
  int       fields_len;
  int       fields_size;
} lsb_heka_message;

typedef enum {
  LSB_READ_NIL,
  LSB_READ_NUMERIC,
  LSB_READ_STRING,
  LSB_READ_BOOL
} lsb_read_type;

typedef struct {
  union
  {
    lsb_const_string  s;
    double            d;
  } u;
  lsb_read_type type;
} lsb_read_value;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Zero the structure and allocate memory for at least 'size' fields
 *
 * @param m Heka message structure
 * @param num_fields Preallocated number of fields (must be >0)
 *
 * @return lsb_err_value NULL on success error message on failure
 *
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_init_heka_message(lsb_heka_message *m, int num_fields);

/**
 * Frees the memory allocated for the message fields
 *
 * @param m Heka message structure
 *
 */
LSB_UTIL_EXPORT void lsb_free_heka_message(lsb_heka_message *m);

/**
 * Resets the message headers and fields zeroing the allocated memory
 *
 * @param m Heka message structure
 *
 */
LSB_UTIL_EXPORT void lsb_clear_heka_message(lsb_heka_message *m);

/**
 * Locates a framed Heka message in an input buffer
 *
 * @param m Heka message structure
 * @param ib Input buffer
 * @param decode True if the framed message should be protobuf decoded
 * @param discarded_bytes Used to track stream corruption
 * @param logger Logger structure (can be set to NULL to disable logging)
 *
 * @return bool True on success
 */
LSB_UTIL_EXPORT bool lsb_find_heka_message(lsb_heka_message *m,
                                           lsb_input_buffer *ib,
                                           bool decode,
                                           size_t *discarded_bytes,
                                           lsb_logger *logger);

/**
 * Decodes an array of bytes into a Heka message. The message structure is
 * cleared before decoding.
 *
 * @param m Heka message structure
 * @param buf Protobuf array
 * @param len Length of the protobuf array
 * @param logger Logger structure (can be set to NULL to disable logging)
 *
 * @return bool True on success
 *
 */
LSB_UTIL_EXPORT bool lsb_decode_heka_message(lsb_heka_message *m,
                                             const char *buf,
                                             size_t len,
                                             lsb_logger *logger);

/**
 * Reads a dynamic field from the Heka message
 *
 * @param m Heka meassage structure
 * @param name Field name
 * @param fi Field index
 * @param ai Array index into the field
 * @param val Value structure to be populated by the read
 *
 * @return bool True on success
 */
LSB_UTIL_EXPORT bool lsb_read_heka_field(const lsb_heka_message *m,
                                         lsb_const_string *name,
                                         int fi,
                                         int ai,
                                         lsb_read_value *val);

/**
 * Writes a binary UUID to the output buffer
 *
 * @param ob Pointer to the output data buffer.
 * @param uuid Uuid string to output (can be NULL, binary, human readable UUID)
 * @param len Length of UUID (16 or 36 anything else will cause a new UUID to be
 *            created).
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_write_heka_uuid(lsb_output_buffer *ob, const char *uuid, size_t len);

/**
 * Writes the Heka framing header to the specified buffer.
 *
 * @param buf Buffer to write the header to must be at least LSB_MIN_HDR_SIZE
 *            size.
 * @param len Length of the message to encode into the header
 *
 * @return LSB_UTIL_EXPORT size_t
 */
LSB_UTIL_EXPORT size_t lsb_write_heka_header(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
