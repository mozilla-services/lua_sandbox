/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Generic Lua sandbox for dynamic data analysis  @file */

#ifndef luasandbox_h_
#define luasandbox_h_

#include "luasandbox/error.h"

#ifdef _WIN32
#ifdef luasandbox_EXPORTS
#define LSB_EXPORT __declspec(dllexport)
#else
#define LSB_EXPORT __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define LSB_EXPORT __attribute__ ((visibility ("default")))
#else
#define LSB_EXPORT
#endif
#endif

#define LSB_ERROR_SIZE 256

#define LSB_SHUTTING_DOWN     "shutting down"
#define LSB_CONFIG_TABLE      "lsb_config"
#define LSB_THIS_PTR          "lsb_this_ptr"
#define LSB_MEMORY_LIMIT      "memory_limit"
#define LSB_INSTRUCTION_LIMIT "instruction_limit"
#define LSB_INPUT_LIMIT       "input_limit"
#define LSB_OUTPUT_LIMIT      "output_limit"
#define LSB_LOG_LEVEL         "log_level"
#define LSB_LUA_PATH          "path"
#define LSB_LUA_CPATH         "cpath"
#define LSB_NIL_ERROR         "<nil error message>"

typedef enum {
  LSB_UNKNOWN     = 0,
  LSB_RUNNING     = 1,
  LSB_TERMINATED  = 2,
  LSB_STOP        = 3
} lsb_state;

typedef enum {
  LSB_US_LIMIT    = 0,
  LSB_US_CURRENT  = 1,
  LSB_US_MAXIMUM  = 2,

  LSB_US_MAX
} lsb_usage_stat;

typedef enum {
  LSB_UT_MEMORY       = 0,
  LSB_UT_INSTRUCTION  = 1,
  LSB_UT_OUTPUT       = 2,

  LSB_UT_MAX
} lsb_usage_type;

typedef struct lsb_lua_sandbox lsb_lua_sandbox;

#ifdef __cplusplus
extern "C"
{
#endif

#include "luasandbox/lua.h"

LSB_EXPORT extern lsb_err_id LSB_ERR_INIT;
LSB_EXPORT extern lsb_err_id LSB_ERR_LUA;
LSB_EXPORT extern lsb_err_id LSB_ERR_TERMINATED;

/**
 * Allocates and initializes the structure around the Lua sandbox allowing
 * full specification of the sandbox configuration using a Lua configuration
 * string.
 * memory_limit = 1024*1024*1
 * instruction_limit = 10000
 * output_limit = 64*1024
 * path = '/modules/?.lua'
 * cpath = '/modules/?.so'
 * remove_entries = {
 * [''] =
 * {'collectgarbage','coroutine','dofile','load','loadfile','loadstring',
 * 'newproxy','print'},
 * os = {'getenv','execute','exit','remove','rename','setlocale','tmpname'}
 * }
 * disable_modules = {io = 1}
 *
 *
 * @param parent Pointer to associate the owner to this sandbox.
 * @param lua_file Filename of the Lua script to run in this sandbox.
 * @param cfg Lua structure defining the full sandbox restrictions (may contain
 *            optional host configuration options, everything is available to
 *            the sandbox through the read_config API.
 * @param logger Struct for error reporting/debug printing (NULL to disable)
 * @return lsb_lua_sandbox Sandbox pointer or NULL on failure.
 */
LSB_EXPORT lsb_lua_sandbox*
lsb_create(void *parent, const char *lua_file, const char *cfg,
           lsb_logger *logger);

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
 * @return lsb_err_value NULL on success error message on failure
 *
 */
LSB_EXPORT lsb_err_value
lsb_init(lsb_lua_sandbox *lsb, const char *state_file);

/**
 * Changes the sandbox state to LSB_STOP to allow for a clean exit. This call is
 * not thread safe.
 *
 * @param lsb sandbox to clean stop
 *
 * @return
 *
 */
LSB_EXPORT void lsb_stop_sandbox_clean(lsb_lua_sandbox *lsb);

/**
 * Aborts the running sandbox from a different thread of execution. A "shutting
 * down" Lua error message is generated.
 *
 * @param lsb sandbox to abort
 *
 * @return
 *
 */
LSB_EXPORT void lsb_stop_sandbox(lsb_lua_sandbox *lsb);

/**
 * Frees the memory associated with the sandbox.
 *
 * @param lsb        Sandbox pointer to discard.
 *
 * @return NULL on success, pointer to an error message on failure that MUST BE
 * FREED by the caller.
 */
LSB_EXPORT char* lsb_destroy(lsb_lua_sandbox *lsb);

/**
 * Retrieve the sandbox usage statistics.
 *
 * @param lsb Pointer to the sandbox.
 * @param utype Type of statistic to retrieve i.e. memory.
 * @param ustat Type of statistic to retrieve i.e. current.
 *
 * @return size_t Count or number of bytes depending on the statistic.
 */
LSB_EXPORT size_t lsb_usage(lsb_lua_sandbox *lsb,
                            lsb_usage_type utype,
                            lsb_usage_stat ustat);
/**
 * Retrieve the current sandbox status.
 *
 * @param lsb    Pointer to the sandbox.
 *
 * @return lsb_state code
 */
LSB_EXPORT lsb_state lsb_get_state(lsb_lua_sandbox *lsb);

/**
 * Return the last error in human readable form.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return const char* error message
 */
LSB_EXPORT const char* lsb_get_error(lsb_lua_sandbox *lsb);

/**
 * Sets the last error string.
 *
 * @param lsb Pointer to the sandbox.
 * @param err Error message.
 *
 * @return const char* error message
 */
LSB_EXPORT void lsb_set_error(lsb_lua_sandbox *lsb, const char *err);

/**
 * Access the Lua pointer.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return lua_State* The lua_State pointer.
 */
LSB_EXPORT lua_State* lsb_get_lua(lsb_lua_sandbox *lsb);

/**
 * Returns the filename of the Lua source.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return const char* filename.
 */
LSB_EXPORT const char* lsb_get_lua_file(lsb_lua_sandbox *lsb);

/**
 * Access the parent pointer stored in the sandbox.
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return void* The parent pointer passed to init.
 */
LSB_EXPORT void* lsb_get_parent(lsb_lua_sandbox *lsb);

/**
 * Access the logger struct stored in the sandbox. The logger callback is only
 * available to modules in debug mode (same as print).
 *
 * @param lsb Pointer to the sandbox.
 *
 * @return lsb_logger Pointer to the logger struct
 *
 */
LSB_EXPORT const lsb_logger* lsb_get_logger(lsb_lua_sandbox *lsb);

/**
 * Create a CFunction for use by the Sandbox. The Lua sandbox pointer is pushed
 * to upvalue index 1.
 *
 * @param lsb Pointer to the sandbox.
 * @param func Lua CFunction pointer.
 * @param func_name Function name exposed to the Lua sandbox.
 */
LSB_EXPORT void lsb_add_function(lsb_lua_sandbox *lsb,
                                 lua_CFunction func,
                                 const char *func_name);

/**
 * Helper function to load the Lua function and set the instruction limits
 *
 * @param lsb Pointer to the sandbox.
 * @param func_name Name of the function to load
 *
 * @return lsb_err_value NULL on success error message on failure
 */
LSB_EXPORT lsb_err_value
lsb_pcall_setup(lsb_lua_sandbox *lsb, const char *func_name);

/**
 * Helper function to update the statistics after the call
 *
 * @param lsb Pointer to the sandbox.
 */
LSB_EXPORT void lsb_pcall_teardown(lsb_lua_sandbox *lsb);

/**
 * Change the sandbox state to LSB_TERMINATED due to a fatal error.
 *
 * @param lsb Pointer to the sandbox.
 * @param err Reason for termination
 */
LSB_EXPORT void lsb_terminate(lsb_lua_sandbox *lsb, const char *err);

#ifdef __cplusplus
}
#endif

#endif
