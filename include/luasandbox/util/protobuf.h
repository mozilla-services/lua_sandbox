/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Generic protobuf utility functions @file */

#ifndef luasandbox_util_protobuf_h_
#define luasandbox_util_protobuf_h_

#include <stddef.h>
#include <stdbool.h>

#include "output_buffer.h"
#include "util.h"

#define LSB_MAX_VARINT_BYTES  10

typedef enum {
  LSB_PB_WT_VARINT  = 0,
  LSB_PB_WT_FIXED64 = 1,
  LSB_PB_WT_LENGTH  = 2,
  LSB_PB_WT_SGROUP  = 3,
  LSB_PB_WT_EGROUP  = 4,
  LSB_PB_WT_FIXED32 = 5
} lsb_pb_wire_types;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extract the tag and wiretype from a protobuf key
 *
 * @param p Key
 * @param tag Tag Id
 * @param wiretype Wiretype Id
 *
 * @return LSB_EXPORT const char*
 */
LSB_UTIL_EXPORT
const char* lsb_pb_read_key(const char *p, int *tag, int *wiretype);

/**
 * Writes a field key (tag id/wire type) to the output buffer.
 *
 * @param ob Pointer to the output data buffer.
 * @param tag Field identifier.
 * @param wiretype Field wire type.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_pb_write_key(lsb_output_buffer *ob, unsigned char tag,
                 unsigned char wiretype);

/**
 * Reads the varint into the provided variable
 *
 * @param p Start of buffer
 * @param e End of buffer
 * @param vi Varint value
 *
 * @return const char* Position in the buffer after the varint
 */
LSB_UTIL_EXPORT
const char* lsb_pb_read_varint(const char *p, const char *e, long long *vi);


/**
 * Outputs the varint to an existing buffer
 *
 * @param buf Pointer to buffer with at least LSB_MAX_VARINT_BYTES available,
 * @param i Number to be encoded.
 *
 * @return int Number of bytes written
 */
LSB_UTIL_EXPORT int lsb_pb_output_varint(char *buf, unsigned long long i);

/**
 * Writes a varint encoded number to the output buffer.
 *
 * @param ob Pointer to the output data buffer.
 * @param i Number to be encoded.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_pb_write_varint(lsb_output_buffer *ob, unsigned long long i);

/**
 * Writes a bool to the output buffer.
 *
 * @param ob Pointer to the output data buffer.
 * @param i Number to be encoded.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value lsb_pb_write_bool(lsb_output_buffer *ob, int i);

/**
 * Writes a double to the output buffer.
 *
 * @param ob Pointer to the output data buffer.
 * @param i Double to be encoded.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_pb_write_double(lsb_output_buffer *ob, double i);

/**
 * Writes a string to the output buffer.
 *
 * @param ob Pointer to the output data buffer.
 * @param tag Field identifier.
 * @param s  String to output.
 * @param len Length of s.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_pb_write_string(lsb_output_buffer *ob, char tag, const char *s, size_t len);

/**
 * Updates the field length in the output buffer once the size is known, this
 * allows for single pass encoding.
 *
 * @param ob  Pointer to the output data buffer.
 * @param len_pos Position in the output buffer where the length should be
 *                written.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_pb_update_field_length(lsb_output_buffer *ob, size_t len_pos);

#ifdef __cplusplus
}
#endif

#endif
