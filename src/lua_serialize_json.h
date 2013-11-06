/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Lua lua_sandbox for Heka plugins @file
#ifndef lua_serialize_json_h_
#define lua_serialize_json_h_

#include "lua_sandbox_private.h"
#include "lua_serialize.h"

/** 
 * Serializes a Lua table structure as JSON.
 * 
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param isHash True if this table is a hash, false if it is an array.
 * 
 * @return int Zero on success, non-zero on failure.
 */
int serialize_table_as_json(lua_sandbox* lsb,
                            serialization_data* data,
                            int isHash);

/** 
 * Serializes a Lua data as JSON.
 * 
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the data resides.
 * @param output Pointer the output collector.
 * 
 * @return int
 */
int serialize_data_as_json(lua_sandbox* lsb, int index, output_data* output);

/** 
 * Serializes a table key value pair as JSON.
 * 
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param isHash True if this kvp is part of a hash, false if it is in an array.
 *  
 * @return int Zero on success, non-zero on failure.
 */
int serialize_kvp_as_json(lua_sandbox* lsb,
                          serialization_data* data,
                          int isHash);

/** 
 * Helper function to determine what data should not be serialized to JSON.
 * 
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the data resides.
 * 
 * @return int
 */
int ignore_value_type_json(lua_sandbox* lsb, int index);

#endif
