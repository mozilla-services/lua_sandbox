/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox output buffer functions @file */

#ifndef lsb_output_h_
#define lsb_output_h_

#include <stdio.h>

#include "luasandbox.h"
#include "luasandbox/lua.h"

/**
 * Add a output function to the environment table. The environment table must be
 * on the top of the stack. This function will receive the userdata and
 * lsb_output_data struct as pointers on the Lua stack.
 *
 * lsb_output_data* output = (output_data*)lua_touserdata(lua, -1);
 * ud_object* ud = (ud_object*)lua_touserdata(lua, -2);
 *
 * @param lua Pointer the Lua state.
 * @param fp Function pointer to the outputter.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT void lsb_add_output_function(lua_State* lua, lua_CFunction fp);

/**
 * Append a character to the output stream.
 *
 * @param output Pointer the output collector.
 * @param ch Character to append to the output.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
LSB_EXPORT int lsb_appendc(lsb_output_data* output, char ch);

/**
 * Append formatted string to the output stream.
 *
 * @param output Pointer the output collector.
 * @param fmt Printf format specifier.
 *
 * @return int Zero on success, non-zero if out of memory.
 */
LSB_EXPORT int lsb_appendf(lsb_output_data* output, const char* fmt, ...);

/**
 * Append a fixed string to the output stream.
 *
 * @param output Pointer the output collector.
 * @param str String to append to the output.
 * @param len Length of the string to append
 *
 * @return int Zero on success, non-zero if out of memory.
 */
LSB_EXPORT
int lsb_appends(lsb_output_data* output, const char* str, size_t len);

/**
 * Resize the output buffer when more space is needed.
 *
 * @param output Output buffer to resize.
 * @param needed Number of additional bytes needed.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT int lsb_realloc_output(lsb_output_data* output, size_t needed);

/**
 * Write an array of variables on the Lua stack to the output buffer.
 *
 * @param lsb Pointer to the sandbox.
 * @param start Lua stack index of first variable.
 * @param end Lua stack index of the last variable.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 *
 */
LSB_EXPORT void lsb_output(lua_sandbox* lsb, int start, int end, int append);

/**
 * Write a Lua table (in a Heka protobuf structure) to the output buffer.
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index of the table.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 *
 * @return int 0 on success
 */
LSB_EXPORT int lsb_output_protobuf(lua_sandbox* lsb, int index, int append);

/**
 * More efficient output of a double to a string
 *
 * @param output Pointer the output collector.
 * @param d Double value to convert to a string.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT int lsb_output_double(lsb_output_data* output, double d);
#endif
