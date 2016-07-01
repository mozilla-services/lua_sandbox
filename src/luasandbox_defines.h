/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Shared compiler defines @file */

#ifndef luasandbox_defines_h_
#define luasandbox_defines_h_

#ifdef _WIN32
#define snprintf _snprintf
#elif __linux
#define CLOSE_ON_EXEC "e"
#endif

#ifdef _MSC_VER
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#endif

#endif
