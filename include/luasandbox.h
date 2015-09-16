/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Generic Lua sandbox for dynamic data analysis  @file */

#ifndef lsb_h_
#define lsb_h_

#include "luasandbox/lua.h"

#ifdef _WIN32
#ifdef luasandbox_EXPORTS
#define  LSB_EXPORT __declspec(dllexport)
#else
#define  LSB_EXPORT __declspec(dllimport)
#endif
#else
#define LSB_EXPORT
#endif

#define LSB_ERROR_SIZE 256

typedef enum {
  LSB_UNKNOWN = 0,
  LSB_RUNNING = 1,
  LSB_TERMINATED = 2
} lsb_state;

typedef enum {
  LSB_US_LIMIT = 0,
  LSB_US_CURRENT = 1,
  LSB_US_MAXIMUM = 2,

  LSB_US_MAX
} lsb_usage_stat;

typedef enum {
  LSB_UT_MEMORY = 0,
  LSB_UT_INSTRUCTION = 1,
  LSB_UT_OUTPUT = 2,

  LSB_UT_MAX
} lsb_usage_type;

typedef struct lua_sandbox lua_sandbox;
typedef struct lsb_output_data lsb_output_data;

/**
 * Allocates and initializes the structure around the Lua sandbox.
 *
 * @param parent Pointer to associate the owner to this sandbox.
 * @param lua_file Filename of the Lua script to run in this sandbox.
 * @param require_path Location of the common sandbox modules
 * @param memory_limit Sets the sandbox memory limit (bytes).
 * @param instruction_limit Sets the sandbox Lua instruction limit (count).
 * This limit is per call to process_message or timer_event
 * @param output_limit Sets the single message payload limit (bytes). This
 * limit applies to the in memory output buffer.  The buffer is reset back
 * to zero when inject_message is called.
 * @return lua_sandbox Sandbox pointer or NULL on failure.
 */
LSB_EXPORT lua_sandbox* lsb_create(void* parent,
                                   const char* lua_file,
                                   const char* require_path,
                                   unsigned memory_limit,
                                   unsigned instruction_limit,
                                   unsigned output_limit);

/**
 * Allocates and initializes the structure around the Lua sandbox allowing
 * full specification of the sandbox configuration using a Lua configuration
 * string.
 *
 * {
 * memory_limit = 1024*1024*1,
 * instruction_limit = 10000,
 * output_limit = 64*1024,
 * path = '/modules/?.lua',
 * cpath = '/modules/?.so',
 * remove_entries = {
 *    [''] = {'collectgarbage','coroutine','dofile','load','loadfile'"
 *           ",'loadstring','newproxy','print'},
 *    os = {'getenv','execute','exit','remove','rename','setlocale','tmpname'}
 * },
 * disable_modules = {io = 1}
 * }
 *
 * @param parent Pointer to associate the owner to this sandbox.
 * @param lua_file Filename of the Lua script to run in this sandbox.
 * @param config Lua structure defining the full sandbox restrictions.
 * @return lua_sandbox Sandbox pointer or NULL on failure.
 */
LSB_EXPORT lua_sandbox* lsb_create_custom(void* parent,
                                          const char* lua_file,
                                          const char* config);

/**
 * Initializes the Lua sandbox and loads/runs the Lua script that was specified
 * in lua_create_sandbox.
 *
 * @param lsb Pointer to the sandbox.
 * @param state_file Filename where the global data is read. Use a NULL or empty
 *                   string for no data restoration.  The global
 *                   _PRESERVATION_VERSION variable will be examined during
 *                   restoration; if the previous version does not match the
 *                   current version the restoration will be aborted and the
 *                   sandbox will start cleanly. _PRESERVATION_VERSION should be
 *                   incremented any time an incompatible change is made to the
 *                   global data schema. If no version is set the check will
 *                   always succeed and a version of zero is assigned.
 *
 * @return int Zero on success, non-zero on failure.
 */
LSB_EXPORT int lsb_init(lua_sandbox* lsb, const char* state_file);

/**
 * Frees the memory associated with the sandbox.
 *
 * @param lsb        Sandbox pointer to discard.
 * @param state_file Filename where the sandbox global data is saved. Use a
 * NULL or empty string for no preservation.
 *
 * @return NULL on success, pointer to an error message on failure that MUST BE
 * FREED by the caller.
 */
LSB_EXPORT char* lsb_destroy(lua_sandbox* lsb, const char* state_file);

/**
 * Retrieve the sandbox usage statistics.
 *
 * @param lsb Pointer to the sandbox.
 * @param lsb_usage_type Type of statistic to retrieve i.e. memory.
 * @param lsb_usage_stat Type of statistic to retrieve i.e. current.
 *
 * @return size_t Count or number of bytes depending on the statistic.
 */
LSB_EXPORT size_t
lsb_usage(lua_sandbox* lsb, lsb_usage_type utype, lsb_usage_stat ustat);
/**
 * Retrieve the current sandbox status.
 *
 * @param lsb    Pointer to the sandbox.
 *
 * @return lsb_state code
 */
LSB_EXPORT lsb_state lsb_get_state(lua_sandbox* lsb);

/**
 * Return the last error in human readable form.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return const char* error message
 */
LSB_EXPORT const char* lsb_get_error(lua_sandbox* lsb);

/**
 * Sets the last error string.
 *
 * @param lsb Pointer to the sandbox.
 * @param err Error message.
 *
 * @return const char* error message
 */
LSB_EXPORT void lsb_set_error(lua_sandbox* lsb, const char* err);

/**
 * Access the Lua pointer.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return lua_State* The lua_State pointer.
 */
LSB_EXPORT lua_State* lsb_get_lua(lua_sandbox* lsb);

/**
 * Returns the filename of the Lua source.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return const char* filename.
 */
LSB_EXPORT const char* lsb_get_lua_file(lua_sandbox* lsb);

/**
 * Access the parent pointer stored in the sandbox.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return void* The parent pointer passed to init.
 */
LSB_EXPORT void* lsb_get_parent(lua_sandbox* lsb);

/**
 * Create a CFunction for use by the Sandbox. The Lua sandbox pointer is pushed
 * to upvalue index 1.
 *
 * @param lsb Pointer to the sandbox.
 * @param func Lua CFunction pointer.
 * @param func_name Function name exposed to the Lua sandbox.
 */
LSB_EXPORT void lsb_add_function(lua_sandbox* lsb, lua_CFunction func,
                                 const char* func_name);

/**
 * Retrieve the data in the output buffer and reset the buffer. The returned
 * output string will remain valid until additional sandbox output is performed.
 * The output should be copied if the application needs to hold onto it.
 *
 * @param lsb Pointer to the sandbox.
 * @param len If len is not NULL, it will be set to the length of the string.
 *
 * @return const char* Pointer to the output buffer.
 */
LSB_EXPORT const char* lsb_get_output(lua_sandbox* lsb, size_t* len);

/**
 * Helper function to load the Lua function and set the instruction limits
 *
 * @param lsb Pointer to the sandbox.
 * @param func_name Name of the function to load
 *
 * @return int 0 on success
 */
LSB_EXPORT int lsb_pcall_setup(lua_sandbox* lsb, const char* func_name);

/**
 * Helper function to update the statistics after the call
 *
 * @param lsb Pointer to the sandbox.
 */
LSB_EXPORT void lsb_pcall_teardown(lua_sandbox* lsb);

/**
 * Shutdown the sandbox due to a fatal error.
 *
 * @param lsb Pointer to the sandbox.
 * @param err Reason for termination
 */
LSB_EXPORT void lsb_terminate(lua_sandbox* lsb, const char* err);

/**
 * Deserialize a Heka message protobuf string into a Lua table structure. For
 * ease of reuse it is include here (since many of our sandboxes process Heka
 * messages).  If needed it must be manually added to the sandbox.
 *
 * @param lua Pointer the Lua state.
 *
 * @return One on success (table), two on failure (nil, error_message).
 */
LSB_EXPORT int lsb_decode_protobuf(lua_State* lua);

#endif
