/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Lua lua_sandbox for Heka plugins @file
#ifndef lua_sandbox_h_
#define lua_sandbox_h_

#include <lua.h>

typedef enum {
  LSB_UNKNOWN      = 0,
  LSB_RUNNING      = 1,
  LSB_TERMINATED   = 2
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

#define LSB_MEMORY 1024 * 1024 * 8
#define LSB_INSTRUCTION 1000000
#define LSB_OUTPUT 1024 * 63
#define LSB_ERROR_SIZE 256

typedef struct lua_sandbox lua_sandbox;

/**
 * Allocates and initializes the structure around the Lua sandbox.
 * 
 * @param parent Pointer to associate the owner to this sandbox.
 * @param lua_file Filename of the Lua script to run in this sandbox.
 * @param memory_limit Sets the sandbox memory limit (bytes).
 * @param instruction_limit Sets the sandbox Lua instruction limit (count). 
 * This limit is per call to process_message or timer_event 
 * @param output_limit Sets the single message payload limit (bytes). This 
 * limit applies to the in memory output buffer.  The buffer is reset back 
 * to zero when inject_message is called. 
 * 
 * @return lua_sandbox Sandbox pointer or NULL on failure.
 */
lua_sandbox* lsb_create(void* parent,
                        const char* lua_file,
                        unsigned memory_limit,
                        unsigned instruction_limit,
                        unsigned output_limit);

/** 
 * Initializes the Lua sandbox and loads/runs the Lua script that was specified 
 * in lua_create_sandbox.
 * 
 * @param lsb Pointer to the sandbox.
 * @param state_file Filename where the global data is read. Use a NULL or empty
 *                   string no data restoration.
 * 
 * @return int Zero on success, non-zero on failure.
 */
int lsb_init(lua_sandbox* lsb, const char* state_file);

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
char* lsb_destroy(lua_sandbox* lsb, const char* state_file);

/** 
 * Retrieve the sandbox usage statistics.
 * 
 * @param lsb Pointer to the sandbox.
 * @param lsb_usage_type Type of statistic to retrieve i.e. memory.
 * @param lsb_usage_stat Type of statistic to retrieve i.e. current.
 * 
 * @return unsigned Count or number of bytes depending on the statistic.
 */
unsigned 
lsb_usage(lua_sandbox* lsb, lsb_usage_type utype, lsb_usage_stat ustat);
/**
 * Retrieve the current sandbox status.
 * 
 * @param lsb    Pointer to the sandbox.
 * 
 * @return lsb_state code
 */
lsb_state lsb_get_state(lua_sandbox* lsb);

/** 
 * Return the last error in human readable form.
 * 
 * @param lsb Pointer to the sandbox.
 * 
 * @return const char* error message
 */
const char* lsb_get_error(lua_sandbox* lsb);

/**
 * Access the Lua pointer.  
 *  
 * @param lsb Pointer to the sandbox.
 * 
 * @return lua_State* The lua_State pointer.
 */
lua_State* lsb_get_lua(lua_sandbox* lsb);

/**
 * Access the parent pointer stored in the sandbox.  
 *  
 * @param lsb Pointer to the sandbox.
 * 
 * @return void* The parent pointer passed to init.
 */
void* lsb_get_parent(lua_sandbox* lsb);

/**
 * Create a CFunction for use by the Sandbox. The Lua sandbox pointer is pushed 
 * to upvalue index 1. 
 * 
 * @param lsb Pointer to the sandbox.
 * @param func Lua CFunction pointer.
 * @param func_name Function name exposed to the Lua sandbox.
 */
void lsb_add_function(lua_sandbox* lsb, lua_CFunction func,
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
const char* lsb_get_output(lua_sandbox* lsb, size_t* len);

/**
 * Write a userdata structure to the output buffer.
 * 
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index of the userdata.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 * 
 * @return Type of userdata (i.e., "cbuf", "cbufd") or NULL if the data could 
 *         not be converted.
 * 
 */
const char*
lsb_output_userdata(lua_sandbox* lsb, int index, int append);

/**
 * Write a Lua table (in a Heka protobuf structure) to the output buffer.
 * 
 * @param lsb Pointer to the sandbox.
 * @param index Lua stack index of the table.
 * @param append 0 to overwrite the output buffer, 1 to append the output to it
 * 
 * @return int 0 on success
 */
int lsb_output_protobuf(lua_sandbox* lsb, int index, int append);

/**
 * Helper function to load the Lua function and set the instruction limits
 * 
 * @param lsb Pointer to the sandbox.
 * @param func_name Name of the function to load 
 * 
 * @return int 0 on success
 */
int lsb_pcall_setup(lua_sandbox* lsb, const char* func_name);

/**
 * Helper function to updata the statistics after the call
 * 
 * @param lsb Pointer to the sandbox.
 */
void lsb_pcall_teardown(lua_sandbox* lsb);

/**
 * Shutdown the sandbox due to a fatal error.
 * 
 * @param lsb Pointer to the sandbox.
 * @param err Reason for termination
 */
void lsb_terminate(lua_sandbox* lsb, const char* err);


#endif
