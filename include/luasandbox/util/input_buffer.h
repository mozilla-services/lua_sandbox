/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Data stream input buffer @file */

#ifndef luasandbox_util_input_buffer_h_
#define luasandbox_util_input_buffer_h_

#include <stdbool.h>
#include <stddef.h>

#include "util.h"

typedef struct lsb_input_buffer
{
  char    *buf;
  size_t  size;
  size_t  maxsize;
  size_t  readpos;
  size_t  scanpos;
  size_t  msglen;
} lsb_input_buffer;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the provided input buffer
 *
 * @param b Input buffer
 * @param max_message_size The maximum message size the buffer will handle
 *                 before erroring (the internal buffer will contain extra space
 *                 for the header)
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_init_input_buffer(lsb_input_buffer *b, size_t max_message_size);

/**
 * Frees the memory internally allocated by the buffer and resets the state
 *
 * @param b Input buffer
 */
LSB_UTIL_EXPORT void lsb_free_input_buffer(lsb_input_buffer *b);

/**
 * Expands the input buffer (if necessary) to accomadate the requested number of
 * bytes. The expansion happens in power of two increments up to the maxsize.
 * The buffer never shrinks in size.
 *
 * @param b Input buffer
 * @param len The length of the data being added to the buffer
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_UTIL_EXPORT lsb_err_value
lsb_expand_input_buffer(lsb_input_buffer *b, size_t len);

#ifdef __cplusplus
}
#endif

#endif
