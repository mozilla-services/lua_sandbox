/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox module management @file */

#ifndef lsb_modules_h_
#define lsb_modules_h_

#include <lua.h>

#include "lua_bloom_filter.h"
#include "lua_circular_buffer.h"
#include "lua_hyperloglog.h"

extern const char* lsb_disable_none[];
extern const char* lsb_package_table;
extern const char* lsb_loaded_table;

/**
 * Performs the library load and secures the sandbox environment for use.
 *
 * @param lua Pointer to the Lua state.
 * @param table Name of the table being loaded.
 * @param f Pointer to the table load function.
 * @param disable Array of function names to disable in the loaded table.
 */
void lsb_load_library(lua_State* lua, const char* table, lua_CFunction f,
                  const char** disable);

/**
 * Overridden 'require' used to load optional sandbox libraries in global space.
 *
 * @param lua Pointer to the Lua state.
 *
 * @return int Returns 1 value on the stack (for the standard modules a table
 *         for the LPEG grammars, userdata).
 */
int lsb_require_library(lua_State* lua);

/* declarations for modules without headers */
LUALIB_API int luaopen_cjson(lua_State* L);
int set_encode_max_buffer(lua_State* L, int index, unsigned maxsize);

LUALIB_API int luaopen_lpeg(lua_State* L);
LUALIB_API int luaopen_struct(lua_State* L);

#endif
