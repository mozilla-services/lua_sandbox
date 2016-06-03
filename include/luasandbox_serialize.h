/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox serialization @file */

#ifndef luasandbox_serialize_h_
#define luasandbox_serialize_h_

#include <stdio.h>

#include "luasandbox/util/output_buffer.h"
#include "luasandbox_output.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "luasandbox/lua.h"

/**
 * Add a serialization function to the environment table. The environment table
 * must be on the top of the stack. This function will receive the userdata,
 * fully qualified variable name, and lsb_output_buffer struct as pointers on the
 * Lua stack.
 *
 * lsb_output_buffer* output = (output_data*)lua_touserdata(lua, -1);
 * const char *key = (const char*)lua_touserdata(lua, -2);
 * ud_object* ud = (ud_object*)lua_touserdata(lua, -3);
 *
 * @param lua Pointer the Lua state.
 * @param fp Function pointer to the serializer.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_EXPORT void
lsb_add_serialize_function(lua_State *lua, lua_CFunction fp);

/**
 * Serializes a binary data to a Lua string.
 *
 * @param output Pointer the output buffer.
 * @param src Pointer to the binary data.
 * @param len Length in bytes of the data to output.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_EXPORT lsb_err_value
lsb_serialize_binary(lsb_output_buffer *output, const void *src, size_t len);

/**
 * More efficient serialization of a double to a string
 *
 * @param output Pointer the output buffer.
 * @param d Double value to convert to a string.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_EXPORT lsb_err_value
lsb_serialize_double(lsb_output_buffer *output, double d);

#ifdef __cplusplus
}
#endif

#endif
