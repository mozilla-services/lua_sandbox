/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua bloom_filter filter @file */

#ifndef lua_bloom_filter_h_
#define lua_bloom_filter_h_

#include <lua.h>
#include "lua_sandbox_private.h"

extern const char* lsb_bloom_filter;
extern const char* lsb_bloom_filter_table;
typedef struct bloom_filter bloom_filter;

/**
 * Serialize bloom_filter data
 *
 * @param key Lua variable name.
 * @param b  Bloom userdata object.
 * @param output Output stream where the data is written.
 * @return Zero on success
 *
 */
int serialize_bloom_filter(const char* key, bloom_filter* bf,
                           output_data* output);


/**
 * Bloom library loader
 *
 * @param lua Lua state.
 *
 * @return 1 on success
 *
 */
int luaopen_bloom_filter(lua_State* lua);


#endif
