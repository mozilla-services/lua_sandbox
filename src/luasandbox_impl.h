/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox private functions @file */

#ifndef luasandbox_impl_h_
#define luasandbox_impl_h_

#include "luasandbox.h"
#include "luasandbox/lua.h"
#include "luasandbox/util/output_buffer.h"

struct lsb_lua_sandbox {
  lua_State         *lua;
  void              *parent;
  char              *lua_file;
  char              *state_file;
  lsb_logger        logger;
  lsb_state         state;
  lsb_output_buffer output;
  size_t            usage[LSB_UT_MAX][LSB_US_MAX];
  char              error_message[LSB_ERROR_SIZE];
};

/**
 * Serialize all user global data to disk.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
lsb_err_value preserve_global_data(lsb_lua_sandbox *lsb);

/**
 * Restores previously serialized data from disk.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return lsb_err_value NULL on success error message on failure
 */
lsb_err_value restore_global_data(lsb_lua_sandbox *lsb);

#endif
