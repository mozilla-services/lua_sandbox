/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox serialization @file */

#ifndef lua_serialize_h_
#define lua_serialize_h_

#include <stdio.h>
#include "lua_sandbox_private.h"

extern const char* not_a_number;

typedef struct
{
  const void*     ptr;
  size_t          name_pos;
} table_ref;

typedef struct
{
  size_t      size;
  size_t      pos;
  table_ref*  array;
} table_ref_array;

typedef struct
{
  FILE*           fh;
  output_data     keys;
  table_ref_array tables;
  const void*     globals;
} serialization_data;

/**
 * Serialize all user global data to disk.
 *
 * @param lsb Pointer to the sandbox.
 * @param data_file Filename where the data will be written (create/overwrite)
 *
 * @return int Zero on success, non-zero on failure.
 */
int preserve_global_data(lua_sandbox* lsb, const char* data_file);

/**
 * More efficient serialization of a double to a string
 *
 * @param output Pointer the output collector.
 * @param d Double value to convert to a string.
 *
 * @return int Zero on success, non-zero on failure.
 */
int serialize_double(output_data* output, double d);

/**
 * Serializes a Lua table structure.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return int Zero on success, non-zero on failure.
 */
int serialize_table(lua_sandbox* lsb, serialization_data* data, size_t parent);

/**
 * Serializes a Lua data value.
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the data resides.
 * @param output Pointer the output collector.
 *
 * @return int
 */
int serialize_data(lua_sandbox* lsb, int index, output_data* output);

/**
 * Determines the name of the userdata type
 *
 * @param lua Lua State
 * @param index Index on the stack where the userdata pointer resides
 * @param tname userdata table name
 *
 * @return const char* NULL if not found
 */
void *userdata_type(lua_State* lua, int index, const char *tname);

/**
 * Serializes a table key value pair.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param parent Index pointing to the parent's name in the table array.
 *
 * @return int Zero on success, non-zero on failure.
 */
int serialize_kvp(lua_sandbox* lsb, serialization_data* data, size_t parent);

/**
 * Checks to see a string key starts with a '_' in which case it will be
 * ignored.
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index where the key resides.
 *
 * @return int True if the key should not be serialized.
 */
int ignore_key(lua_sandbox* lsb, int index);


/**
 * Looks for a table to see if it has already been processed.
 *
 * @param tra Pointer to the table references.
 * @param ptr Pointer value of the table.
 *
 * @return table_ref* NULL if not found.
 */
table_ref* find_table_ref(table_ref_array* tra, const void* ptr);

/**
 * Adds a table to the processed array.
 *
 * @param tra Pointer to the table references.
 * @param ptr Pointer value of the table.
 * @param name_pos Index pointing to name in the table array.
 *
 * @return table_ref* Pointer to the table reference or NULL if out of memory.
 */
table_ref* add_table_ref(table_ref_array* tra, const void* ptr,
                         size_t name_pos);

/**
 * Helper function to determine what data should not be serialized.
 *
 * @param lsb Pointer to the sandbox.
 * @param data Pointer to the serialization state data.
 * @param index Lua stack index where the data resides.
 *
 * @return int
 */
int ignore_value_type(lua_sandbox* lsb, serialization_data* data, int index);

/**
 * Restores previously serialized data from disk.
 *
 * @param lsb Pointer to the sandbox.
 * @param data_file Filename from where the data will be read.
 *
 * @return int Zero on success, non-zero on failure.
 */
int restore_global_data(lua_sandbox* lsb, const char* data_file);

/**
 * Serializes a binary data to a Lua string.
 *
 * @param src Pointer to the binary data.
 * @param len Length in bytes of the data to output.
 * @param output Pointer the output collector.
 *
 * @return int Zero on success, non-zero on failure.
 */
int serialize_binary(const void *src, size_t len, output_data* output);

#endif
