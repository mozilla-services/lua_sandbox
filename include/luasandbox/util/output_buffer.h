/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Data stream output buffer @file */

#ifndef luasandbox_util_output_buffer_h_
#define luasandbox_util_output_buffer_h_

#include <stdbool.h>

#include "util.h"

#define LSB_OUTPUT_SIZE 1024

typedef struct lsb_output_buffer {
  char          *buf;
  size_t        maxsize;
  size_t        size;
  size_t        pos;
} lsb_output_buffer;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize the provided input buffer
 *
 * @param b Output buffer
 * @param max_message_size The maximum message size the buffer will handle
 *                 before erroring
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_init_output_buffer(lsb_output_buffer *b, size_t max_message_size);

/**
 * Frees the memory internally allocated by the buffer and resets the state
 *
 * @param b Output buffer
 */
LSB_UTIL_EXPORT void lsb_free_output_buffer(lsb_output_buffer *b);

/**
 * Resize the output buffer when more space is needed.
 *
 * @param b Output buffer to resize.
 * @param needed Number of additional bytes needed.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value lsb_expand_output_buffer(lsb_output_buffer *b,
                                                       size_t needed);

/**
 * Append a character to the output buffer.
 *
 * @param b Pointer the b buffer.
 * @param ch Character to append to the b.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value lsb_outputc(lsb_output_buffer *b, char ch);

/**
 * Append a formatted string to the output buffer.
 *
 * @param b Pointer the b buffer.
 * @param fmt Printf format specifier.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_outputf(lsb_output_buffer *b, const char *fmt, ...);

/**
 * Append a fixed string to the output buffer.
 *
 * @param b Pointer the b buffer.
 * @param str String to append to the b.
 * @param len Length of the string to append
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_outputs(lsb_output_buffer *b, const char *str, size_t len);

/**
 * More efficient output of a double to a string. NaN/Inf check and then calls
 * lsb_outputfd.
 *
 * @param b Pointer the output buffer.
 * @param d Double value to convert to a string.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value lsb_outputd(lsb_output_buffer *b, double d);


/**
 * More efficient output of a double to a string; no NaN or Inf outputs.
 *
 * @param b Pointer the output buffer.
 * @param d Double value to convert to a string.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value lsb_outputfd(lsb_output_buffer *b, double d);

#ifdef __cplusplus
}
#endif

#endif
