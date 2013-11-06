/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Lua lua_sandbox for Heka plugins @file
#ifndef lua_sandbox_private_h_
#define lua_sandbox_private_h_

#include <stdio.h>
#include <lua.h>
#include "lua_sandbox.h"

#define OUTPUT_SIZE 64

typedef struct
{
  size_t m_maxsize;
  size_t m_size;
  size_t m_pos;
  char*  m_data;
} output_data;

struct lua_sandbox
{
  lua_State*      m_lua;
  void*           m_parent;
  lsb_state      m_state;
  output_data     m_output;
  char*           m_lua_file;
  unsigned        m_usage[LSB_UT_MAX][LSB_US_MAX];
  char            m_error_message[LSB_ERROR_SIZE];
};

extern const char* disable_none[];

////////////////////////////////////////////////////////////////////////////////
/// Sandbox management functions.
////////////////////////////////////////////////////////////////////////////////
/** 
 * Performs the library load and secures the sandbox environment for use.
 * 
 * @param lua Pointer to the Lua state.
 * @param table Name of the table being loaded.
 * @param f Pointer to the table load function.
 * @param disable Array of function names to disable in the loaded table.
 */
void load_library(lua_State* lua, const char* table, lua_CFunction f,
                  const char** disable);

/** 
 * Lua hook to monitor the instruction usage of the sandbox.
 * 
 * @param lua Pointer to the Lua state.
 * @param ar Pointer to the Lua debug interface.
 */
void instruction_manager(lua_State* lua, lua_Debug* ar);

/** 
 * Extracts the current instruction usage count from the Lua state. 
 * 
 * @param lsb Pointer to the sandbox.
 * 
 * @return size_t The number of instructions used in the last function call
 */
size_t instruction_usage(lua_sandbox* lsb);

/** 
 * Tears down the sandbox on error.
 * 
 * @param lsb Pointer to the sandbox.
 */
void sandbox_terminate(lua_sandbox* lsb);

/** 
 * Helper function to update the output statistics.
 * 
 * @param lsb Pointer to the sandbox.
 */
void update_output_stats(lua_sandbox* lsb);

/** 
 * Append formatted string to the output stream.
 * 
 * @param output Pointer the output collector.
 * @param fmt Printf format specifier.
 * 
 * @return int Zero on success, non-zero if out of memory.
 */
int appendf(output_data* output, const char* fmt, ...);

/** 
 * Resize the output buffer when more space is needed.
 * 
 * @param output Output buffer to resize.
 * @param needed Number of additional bytes needed.
 * 
 * @return int Zero on success, non-zero on failure.
 */
int realloc_output(output_data* output, size_t needed);

/** 
 * Append a fixed string to the output stream.
 * 
 * @param output Pointer the output collector.
 * @param str String to append to the output.
 * 
 * @return int Zero on success, non-zero if out of memory.
 */
int appends(output_data* output, const char* str);

/** 
 * Append a character to the output stream.
 * 
 * @param output Pointer the output collector.
 * @param ch Character to append to the output.
 * 
 * @return int Zero on success, non-zero if out of memory.
 */
int appendc(output_data* output, char ch);

////////////////////////////////////////////////////////////////////////////////
/// Lua to C function interface
////////////////////////////////////////////////////////////////////////////////
/** 
 * Collect sandbox output into an in memory buffer.
 * 
 * @param lua Pointer to the Lua state.
 * 
 * @return int Returns zero values on the stack.
 */
int output(lua_State* lua);

/** 
 * Overridden 'require' used to load optional sandbox libraries in global space.
 * 
 * @param lua Pointer to the Lua state.
 * 
 * @return int Returns zero values on the stack.
 */
int require_library(lua_State* lua);

#endif
