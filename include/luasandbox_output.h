/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sandbox output generation/retrieval functions @file */

#ifndef luasandbox_output_h_
#define luasandbox_output_h_

#include <stdio.h>

#include "luasandbox.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "luasandbox/lua.h"

/**
 * Add a output function to the environment table. The environment table must be
 * on the top of the stack. This function will receive the userdata and
 * lsb_output_buffer struct as pointers on the Lua stack.
 *
 * lsb_output_buffer* output = (output_data*)lua_touserdata(lua, -1);
 * ud_object* ud = (ud_object*)lua_touserdata(lua, -2);
 *
 * @param lua Pointer the Lua state.
 * @param fp Function pointer to the outputter.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT void lsb_add_output_function(lua_State *lua, lua_CFunction fp);

/**
 * Utility function to retrieve a user data output function
 *
 * @param lua
 * @param index
 *
 * @return lua_CFunction
 */
LSB_EXPORT lua_CFunction lsb_get_output_function(lua_State *lua, int index);

/**
 * Add a zero copy function to the environment table. The environment table must
 * be on the top of the stack. This function will receive the userdata as a
 * pointer on the Lua stack.
 *
 * ud_object* ud = (ud_object*)lua_touserdata(lua, -1);
 *
 * @param lua Pointer the Lua state.
 * @param fp Function pointer to the zero copy function.
 *
 * @return int Number of segments (pointer and length for each)
 */
LSB_EXPORT void lsb_add_zero_copy_function(lua_State *lua, lua_CFunction fp);


/**
 * Utility function to retrieve a user data zero copy function
 *
 * @param lua
 * @param index
 *
 * @return lua_CFunction
 */
LSB_EXPORT lua_CFunction lsb_get_zero_copy_function(lua_State *lua, int index);

/**
 * Write an array of variables on the Lua stack to the output buffer.
 *
 * @param lsb Pointer to the sandbox.
 * @param start Lua stack index of first variable.
 * @param end Lua stack index of the last variable.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 *
 */
LSB_EXPORT void
lsb_output(lsb_lua_sandbox *lsb, int start, int end, int append);

/**
 * Write an array of variables on the Lua stack to the output buffer. After
 * adding support for coroutines we need an extra variable to specify the
 * correct Lua state.
 *
 * @param lsb Pointer to the sandbox.
 * @param lua Pointer the Lua state
 * @param start Lua stack index of first variable.
 * @param end Lua stack index of the last variable.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 *
 */
LSB_EXPORT void
lsb_output_coroutine(lsb_lua_sandbox *lsb, lua_State *lua, int start,
                     int end, int append);

/**
 * Retrieve the data in the output buffer and reset the buffer. The returned
 * output string will remain valid until additional sandbox output is performed.
 * The output should be copied if the application needs to hold onto it.
 *
 * @param lsb Pointer to the sandbox.
 * @param len If len is not NULL, it will be set to the length of the string.
 *
 * @return const char* Pointer to the output buffer.
 */
LSB_EXPORT const char* lsb_get_output(lsb_lua_sandbox *lsb, size_t *len);

#ifdef __cplusplus
}
#endif

#endif
