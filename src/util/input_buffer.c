/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Data stream input buffer implementation @file */

#include "luasandbox/util/input_buffer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luasandbox/util/util.h"
#include "luasandbox/util/heka_message.h"

int lsb_init_input_buffer(lsb_input_buffer *b, size_t max_message_size)
{
  b->buf = NULL;
  if (max_message_size == 0) return 1;
  max_message_size += LSB_MAX_HDR_SIZE;
  b->size = max_message_size < BUFSIZ ? max_message_size : BUFSIZ;
  b->maxsize = max_message_size;
  b->readpos = 0;
  b->scanpos = 0;
  b->msglen = 0;
  b->buf = malloc(b->size);
  return b->buf ? 0 : 2;
}


void lsb_free_input_buffer(lsb_input_buffer *b)
{
  free(b->buf);
  b->buf = NULL;
  b->size = 0;
  b->readpos = 0;
  b->scanpos = 0;
  b->msglen = 0;
}


int lsb_expand_input_buffer(lsb_input_buffer *b, size_t len)
{
  if (b->scanpos != 0) { // shift the data to the beginning of the buffer
    if (b->scanpos == b->readpos) {
      b->scanpos = b->readpos = 0;
    } else {
      memmove(b->buf, b->buf + b->scanpos, b->readpos - b->scanpos);
      b->readpos = b->readpos - b->scanpos;
      b->scanpos = 0;
    }
  }

  if (b->readpos + len > b->size) {
    size_t newsize = b->readpos + len;
    if (newsize > b->maxsize) return 1;

    newsize = lsb_lp2(newsize);
    if (newsize > b->maxsize) newsize = b->maxsize;;
    char *tmp = realloc(b->buf, newsize);
    if (tmp) {
      b->buf = tmp;
      b->size = newsize;
    } else {
      return 2;
    }
  }
  return 0;
}
