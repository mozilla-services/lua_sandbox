/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Error handling and logging  @file */

#ifndef luasandbox_error_h_
#define luasandbox_error_h_

// See Identify your Errors better with char[]
// http://accu.org/index.php/journals/2184
typedef const char lsb_err_id[];
typedef const char *lsb_err_value;
#define lsb_err_string(s) s ? s : "<no error>"

typedef void (*lsb_logger_cb)(void *context,
                              const char *component,
                              int level,
                              const char *fmt,
                              ...);

typedef struct lsb_logger {
  void          *context;
  lsb_logger_cb cb;
} lsb_logger;

#endif
