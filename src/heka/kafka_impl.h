/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Lua Kafka constants/types @file */

#ifndef luasandbox_heka_kafka_impl_h_
#define luasandbox_heka_kafka_impl_h_

#include "luasandbox/lua.h"

extern const char* mozsvc_heka_kafka_producer_table;
extern const char* mozsvc_heka_kafka_consumer_table;

int luaopen_heka_kafka_producer(lua_State *lua);
int luaopen_heka_kafka_consumer(lua_State *lua);

#endif
