/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox private functions @file */

#ifndef lsb_private_h_
#define lsb_private_h_

#include "luasandbox.h"
#include "luasandbox/lua.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

#define LSB_OUTPUT_SIZE 1024

struct lsb_output_data
{
  size_t maxsize;
  size_t size;
  size_t pos;
  char*  data;
};

struct lua_sandbox {
  lua_State*      lua;
  void*           parent;
  lsb_state       state;
  lsb_output_data output;
  char*           lua_file;
  size_t          usage[LSB_UT_MAX][LSB_US_MAX];
  char            error_message[LSB_ERROR_SIZE];
};

/**
 * Utility function to retrieve a user data output function
 *
 * @param lua
 * @param index
 *
 * @return lua_CFunction
 */
lua_CFunction lsb_get_output_function(lua_State* lua, int index);

#endif
