/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Heka sandbox implementation @file */

#ifndef luasandbox_heka_sandbox_h_
#define luasandbox_heka_sandbox_h_

#include <stdbool.h>
#include <time.h>

#include "../../luasandbox.h"
#include "../error.h"
#include "../util/heka_message.h"

#ifdef _WIN32
#ifdef luasandboxheka_EXPORTS
#define LSB_HEKA_EXPORT __declspec(dllexport)
#else
#define LSB_HEKA_EXPORT __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define LSB_HEKA_EXPORT __attribute__ ((visibility ("default")))
#else
#define LSB_HEKA_EXPORT
#endif
#endif

#define LSB_HEKA_MAX_MESSAGE_SIZE "max_message_size"
#define LSB_HEKA_UPDATE_CHECKPOINT "update_checkpoint"
#define LSB_HEKA_THIS_PTR "lsb_heka_this_ptr"

enum lsb_heka_pm_rv {
  LSB_HEKA_PM_SENT  = 0,
  LSB_HEKA_PM_FAIL  = -1,
  LSB_HEKA_PM_SKIP  = -2,
  LSB_HEKA_PM_RETRY = -3,
  LSB_HEKA_PM_BATCH = -4,
  LSB_HEKA_PM_ASYNC = -5
};

enum lsb_heka_im_rv {
  LSB_HEKA_IM_SUCCESS    = 0,
  LSB_HEKA_IM_ERROR      = 1, // generic error for backward compatibility
  LSB_HEKA_IM_CHECKPOINT = 2,
  LSB_HEKA_IM_LIMIT      = 3,
};

typedef struct lsb_heka_sandbox lsb_heka_sandbox;

typedef struct lsb_heka_stats {
  unsigned long long mem_cur;
  unsigned long long mem_max;
  unsigned long long ins_max;
  unsigned long long out_max;
  unsigned long long im_cnt;
  unsigned long long im_bytes;
  unsigned long long pm_cnt;
  unsigned long long pm_failures;
  double             pm_avg;
  double             pm_sd;
  double             te_avg;
  double             te_sd;
} lsb_heka_stats;

#ifdef __cplusplus
extern "C"
{
#endif

#include "../lauxlib.h"
#include "../lua.h"

LSB_HEKA_EXPORT extern lsb_err_id LSB_ERR_HEKA_INPUT;

/**
 * inject_message callback function provided by the host. Only one (or neither)
 * of the checkpoint values will be set in a call.  Numeric checkpoints can have
 * a measurable performance benefit but are not always applicable so both
 * options are provided to support various types of input plugins. This function
 * can be called with a NULL pb pointer by the 'is_running' API or if only a
 * checkpoint update is being performed; it should be treated as a no-op and
 * used as a synchronization point to collect statistics and communicate
 * shutdown status.
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param pb Pointer to a Heka protobuf encoded message being injected.
 * @param pb_en Length of s
 * @param cp_numeric Numeric based checkpoint (can be NAN)
 * @param cp_string String based checkpoint (can be NULL)
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_im_input)(void *parent,
                                 const char *pb,
                                 size_t pb_len,
                                 double cp_numeric,
                                 const char *cp_string);

/**
 * inject_message callback function provided by the host.
 *
 * @param parent Opaque pointer the host object owning this sandbox.
 * @param pb Pointer to a Heka protobuf encoded message being injected.
 * @param len Length of pb_len
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_im_analysis)(void *parent,
                                    const char *pb,
                                    size_t pb_len);

/**
 * update_checkpoint callback function provided by the host.
 *
 * @param parent Opaque pointer the host object owning this sandbox.
 * @param sequence_id Opaque pointer to the host message sequencing (passed into
 *               process_message).
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_update_checkpoint)(void *parent, void *sequence_id);

/**
 * Create a sandbox supporting the Heka Input Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified path to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Struct for error reporting/debug printing (NULL to disable)
 * @param im inject_message callback
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_HEKA_EXPORT
lsb_heka_sandbox* lsb_heka_create_input(void *parent,
                                        const char *lua_file,
                                        const char *state_file,
                                        const char *lsb_cfg,
                                        lsb_logger *logger,
                                        lsb_heka_im_input im);

/**
 * Host access to the input sandbox process_message API.  If a numeric
 * checkpoint is set the string checkpoint is ignored.
 *
 * @param hsb Heka input sandbox
 * @param cp_numeric NAN if no numeric checkpoint
 * @param cp_string NULL if no string checkpoint
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  <0 non-fatal error (status message is logged)
 *
 */
LSB_HEKA_EXPORT
int lsb_heka_pm_input(lsb_heka_sandbox *hsb,
                      double cp_numeric,
                      const char *cp_string,
                      bool profile);

/**
 * Create a sandbox supporting the Heka Analysis Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified filename to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Struct for error reporting/debug printing (NULL to disable)
 * @param im inject_message callback
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_HEKA_EXPORT
lsb_heka_sandbox* lsb_heka_create_analysis(void *parent,
                                           const char *lua_file,
                                           const char *state_file,
                                           const char *lsb_cfg,
                                           lsb_logger *logger,
                                           lsb_heka_im_analysis im);

/**
 * Host access to the analysis sandbox process_message API
 *
 * @param hsb Heka analysis sandbox
 * @param msg Heka message to process
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  <0 non-fatal error (status message is logged)
 *
 */
LSB_HEKA_EXPORT
int lsb_heka_pm_analysis(lsb_heka_sandbox *hsb,
                         lsb_heka_message *msg,
                         bool profile);

/**
 * Create a sandbox supporting the Heka Output Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified path to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Struct for error reporting/debug printing (NULL to disable)
 * @param ucp checkpoint_updated callback when using batch or async output
 *
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_HEKA_EXPORT
lsb_heka_sandbox* lsb_heka_create_output(void *parent,
                                         const char *lua_file,
                                         const char *state_file,
                                         const char *lsb_cfg,
                                         lsb_logger *logger,
                                         lsb_heka_update_checkpoint ucp);

/**
 * Host access to the output sandbox process_message API
 *
 * @param hsb Heka output sandbox
 * @param msg Heka message to process
 * @param sequence_id Opaque pointer to the message sequence id (only used for
 *                    async output plugin otherwise it should be NULL)
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  -1 non-fatal_error (status message is logged)
 *  -2 skip
 *  -3 retry
 *  -4 batching
 *  -5 async output
 *
 */
LSB_HEKA_EXPORT
int lsb_heka_pm_output(lsb_heka_sandbox *hsb,
                       lsb_heka_message *msg,
                       void *sequence_id,
                       bool profile);
/**
 * Requests a long running input sandbox to stop. This call is not thread safe.
 *
 * @param hsb Heka sandbox to cleanly stop
 *
 * @return
 *
 */
LSB_HEKA_EXPORT void
lsb_heka_stop_sandbox_clean(lsb_heka_sandbox *hsb);

/**
 * Aborts the running sandbox from a different thread of execution. A "shutting
 * down" termination message is generated. Used to abort long runnning sandboxes
 * such as an input sandbox.
 *
 * @param hsb Heka sandbox to forcibly stop
 *
 * @return
 *
 */
LSB_HEKA_EXPORT void
lsb_heka_stop_sandbox(lsb_heka_sandbox *hsb);

/**
 * Terminates the sandbox as if it had a fatal error (not thread safe).
 *
 * @param hsb Heka sandbox to terminate
 * @param err Reason for termination
 */
LSB_HEKA_EXPORT void
lsb_heka_terminate_sandbox(lsb_heka_sandbox *hsb, const char *err);

/**
 * Frees all memory associated with the sandbox; hsb cannont be used after this
 * point and the host should set it to NULL.
 *
 * @param hsb Heka sandbox to free
 *
 * @return NULL on success, pointer to an error message on failure that MUST BE
 * FREED by the caller.
 *
 */
LSB_HEKA_EXPORT char*
lsb_heka_destroy_sandbox(lsb_heka_sandbox *hsb);

/**
 * Host access to the timer_event API
 *
 * @param hsb Heka sandbox
 * @param t Clock time of the timer_event execution
 * @param shutdown Flag indicating the Host is shutting down allowing the
 *                 sandbox to do any desired finialization)
 *
 * @return int 0 on success
 */
LSB_HEKA_EXPORT
int lsb_heka_timer_event(lsb_heka_sandbox *hsb, time_t t, bool shutdown);

/**
 * Return the last error in human readable form.
 *
 * @param hsb Heka sandbox
 *
 * @return const char* error message
 */
LSB_HEKA_EXPORT const char* lsb_heka_get_error(lsb_heka_sandbox *hsb);

/**
 * Returns the filename of the Lua source.
 *
 * @param hsb Heka sandbox
 *
 * @return const char* filename.
 */
LSB_HEKA_EXPORT const char* lsb_heka_get_lua_file(lsb_heka_sandbox *hsb);

/**
 * Retrieve the sandbox profiling/monitoring statistics.  This call accesses
 * internal data and is not thread safe.
 *
 * @param hsb Heka sandbox
 *
 * @return lsb_heka_stats A copy of the stats structure
 */
LSB_HEKA_EXPORT lsb_heka_stats lsb_heka_get_stats(lsb_heka_sandbox *hsb);

/**
 * Convenience function to test if a sandbox is running.
 *
 * @param hsb Heka sandbox
 *
 * @return True if the sandbox has not been terminated
 */
LSB_HEKA_EXPORT bool lsb_heka_is_running(lsb_heka_sandbox *hsb);

/**
 * Queries the state of the sandbox.
 *
 * @param hsb
 *
 * @return lsb_state
 *
 */
LSB_HEKA_EXPORT lsb_state lsb_heka_get_state(lsb_heka_sandbox *hsb);

/**
 * Retrieve the currently active sandbox message.  This call returns a handle to
 * internal data and is not thread safe.
 * *
 * @param hsb Heka sandbox
 *
 * @return const lsb_heka_message* NULL if there is no active message
 */
LSB_HEKA_EXPORT const lsb_heka_message*
lsb_heka_get_message(lsb_heka_sandbox *hsb);

/**
 * Retrieve the sandbox type.
 * *
 * @param hsb Heka sandbox
 *
 * @return char Heka sandbox type identifer
 */
LSB_HEKA_EXPORT char lsb_heka_get_type(lsb_heka_sandbox *hsb);

#ifdef __cplusplus
}
#endif

#endif
