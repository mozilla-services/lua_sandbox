/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Hindsight/Heka message matcher @file */

#ifndef luasandbox_heka_sandbox_message_impl_h_
#define luasandbox_heka_sandbox_message_impl_h_

#include <stdbool.h>

#include "luasandbox.h"
#include "luasandbox/util/heka_message.h"

// these functions are intentionally not exported

/**
 * Deserialize a Heka message protobuf string into a Lua table structure.
 *
 * @param lua Pointer the Lua state.
 *
 * @return int Number of items on the stack (1  table) or throws an error on
 *         failure
 */
int heka_decode_message(lua_State *lua);

/**
 * Serialize a Lua table structure into a Heka message protobuf string.
 *
 * @param lua Pointer the Lua state.
 *
 * @return int Number of items on the stack (1  string) or throws an error on
 *         failure
 */
int heka_encode_message(lua_State *lua);

/**
 * Serialize a Lua table structure into a Heka message protobuf (using the
 * sandbox output buffer, Called indirectly from inject_message so the output
 * buffer can be used without round tripping the resulting data back to the
 * sandbox with heka_encode_message.
 *
 * @param lsb Pointer to the sandbox.
 * @param lua Pointer to the lua_State.
 * @param idx Lua stack index of the message table.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
lsb_err_value
heka_encode_message_table(lsb_lua_sandbox *lsb, lua_State *lua, int idx);


/**
 * Breakout of the common code for the read_message API
 *
 * @param lua Pointer to the lua_State
 * @param m Heka message to extract the data from
 *
 * @return int Number of items on the stack (1 value) or throws an error on
 *         failure
 */
int heka_read_message(lua_State *lua, lsb_heka_message *m);

#endif
