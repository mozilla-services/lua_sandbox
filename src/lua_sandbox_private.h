/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Lua sandbox private functions @file
#ifndef lua_sandbox_private_h_
#define lua_sandbox_private_h_

#include "lsb_output.h"
#include "lsb.h"

#include <lua.h>
#include <stdio.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif

struct lua_sandbox {
  lua_State*      lua;
  void*           parent;
  lsb_state       state;
  output_data     output;
  char*           lua_file;
  char*           require_path;
  unsigned        usage[LSB_UT_MAX][LSB_US_MAX];
  char            error_message[LSB_ERROR_SIZE];
};

extern const char* disable_none[];
extern const char* package_table;
extern const char* loaded_table;

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
#ifndef LUA_JIT
/**
* Implementation of the memory allocator for the Lua state.
*
* See: http://www.lua.org/manual/5.1/manual.html#lua_Alloc
*
* @param ud Pointer to the lua_sandbox
* @param ptr Pointer to the memory block being allocated/reallocated/freed.
* @param osize The original size of the memory block.
* @param nsize The new size of the memory block.
*
* @return void* A pointer to the memory block.
*/
void* memory_manager(void* ud, void* ptr, size_t osize, size_t nsize);
#endif


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
 * @return int Returns 1 value on the stack (for the standard modules a table
 *         for the LPEG grammars, userdata).
 */
int require_library(lua_State* lua);

LUALIB_API int luaopen_cjson(lua_State* L);
int set_encode_max_buffer(lua_State* L, int index, unsigned maxsize);

LUALIB_API int luaopen_lpeg(lua_State* L);
LUALIB_API int luaopen_struct(lua_State* L);

#endif
