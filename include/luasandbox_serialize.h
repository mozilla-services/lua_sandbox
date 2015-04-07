/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox serialization @file */

#ifndef lsb_serialize_h_
#define lsb_serialize_h_

#include <stdio.h>

#include "luasandbox/lua.h"
#include "luasandbox_output.h"

/**
 * Serialize all user global data to disk.
 *
 * @param lsb Pointer to the sandbox.
 * @param data_file Filename where the data will be written (create/overwrite)
 *
 * @return int Zero on success, non-zero on failure.
 */
int lsb_preserve_global_data(lua_sandbox* lsb, const char* data_file);

/**
 * Restores previously serialized data from disk.
 *
 * @param lsb Pointer to the sandbox.
 * @param data_file Filename from where the data will be read.
 *
 * @return int Zero on success, non-zero on failure.
 */
int lsb_restore_global_data(lua_sandbox* lsb, const char* data_file);

/**
 * Add a serialization function to the environment table. The environment table
 * must be on the top of the stack. This function will receive the userdata,
 * fully qualified variable name, and lsb_output_data struct as pointers on the
 * Lua stack.
 *
 * lsb_output_data* output = (output_data*)lua_touserdata(lua, -1);
 * const char *key = (const char*)lua_touserdata(lua, -2);
 * ud_object* ud = (ud_object*)lua_touserdata(lua, -3);
 *
 * @param lua Pointer the Lua state.
 * @param fp Function pointer to the serializer.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT void
lsb_add_serialize_function(lua_State *lua, lua_CFunction fp);

/**
 * Serializes a binary data to a Lua string.
 *
 * @param src Pointer to the binary data.
 * @param len Length in bytes of the data to output.
 * @param output Pointer the output collector.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT int
lsb_serialize_binary(const void *src, size_t len, lsb_output_data* output);

/**
 * More efficient serialization of a double to a string
 *
 * @param output Pointer the output collector.
 * @param d Double value to convert to a string.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT int lsb_serialize_double(lsb_output_data* output, double d);

#endif
