/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Test interface for the generic lua sandbox @file */

#ifndef luasandbox_test_sandbox_h_
#define luasandbox_test_sandbox_h_

#include <stddef.h>

#include "../../luasandbox.h"
#include "../error.h"

#ifdef _WIN32
#ifdef luasandboxtest_EXPORTS
#define LSB_TEST_EXPORT __declspec(dllexport)
#else
#define LSB_TEST_EXPORT __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define LSB_TEST_EXPORT __attribute__ ((visibility ("default")))
#else
#define LSB_TEST_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include "../lua.h"

/**
 * Global variable to store the test output
 *
 */
LSB_TEST_EXPORT extern const char *lsb_test_output;

/**
 * Global variable storing the length of the current test output string
 *
 */
LSB_TEST_EXPORT extern size_t lsb_test_output_len;

/**
 * Generaly purpose stderr logger
 *
 */
LSB_TEST_EXPORT extern lsb_logger lsb_test_logger;

/**
 * Function to emulate processing data
 *
 * @param lsb Pointer to a lua sandbox
 * @param tc Test case number to control how the sandbox state is updated
 *
 * @return LSB_TEST_EXPORT int Status value returned from the sandbox
 */
LSB_TEST_EXPORT int lsb_test_process(lsb_lua_sandbox *lsb, double tc);

/**
 * Function to emulate outputting summary data
 *
 * @param lsb Pointer to a lua sandbox
 * @param tc Test case number to control what data is returned and how the state
 *           is updated
 *
 * @return LSB_TEST_EXPORT int 0 on success, 1 on failure
 */
LSB_TEST_EXPORT int lsb_test_report(lsb_lua_sandbox *lsb, double tc);

/**
 * Callback for collecting output, places a pointer to the results in the global
 * lsb_test_output variable )not thread safe)
 *
 * @param lua Lua state
 *
 * @return LSB_TEST_EXPORT int 0 or lua_error
 */
LSB_TEST_EXPORT int lsb_test_write_output(lua_State *lua);

#ifdef __cplusplus
}
#endif

#endif
