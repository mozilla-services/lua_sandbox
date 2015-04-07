/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua sandbox Heka protobuf serialization implementation @file */

#ifndef lsb_serialize_protobuf_h_
#define lsb_serialize_protobuf_h_

#include "luasandbox.h"

/**
 * Serialize a specific Lua table structure as Protobuf
 *
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index of the table.
 *
 * @return int Zero on success, non-zero on failure.
 */
int lsb_serialize_table_as_pb(lua_sandbox* lsb, int index);

#endif
