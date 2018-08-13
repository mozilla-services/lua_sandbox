/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief lua_sandbox Heka file stream cat @file */

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "luasandbox/util/heka_message.h"
#include "luasandbox/util/heka_message_matcher.h"
#include "luasandbox/util/input_buffer.h"
#include "luasandbox/util/protobuf.h"
#include "luasandbox/util/util.h"

typedef void (*output_function)(lsb_heka_message *msg);

static void
log_cb(void *context, const char *component, int level, const char *fmt, ...)
{
  (void)context;
  (void)component;
  (void)level;
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fwrite("\n", 1, 1, stderr);
}
static lsb_logger logger = { .context = NULL, .cb = log_cb };


// todo this function is duplicated in util/heka_message.c
// we may want to add it to the API if it is generally useful
static const char*
read_string(int wiretype, const char *p, const char *e, lsb_const_string *s)
{
  if (wiretype != LSB_PB_WT_LENGTH) {
    return NULL;
  }

  long long vi;
  p = lsb_pb_read_varint(p, e, &vi);
  if (!p || vi < 0 || vi > e - p) {
    return NULL;
  }
  s->s = p;
  s->len = (size_t)vi;
  return p + vi;
}


static void output_cs(const char *key, lsb_const_string *cs, bool eol)
{
  if (cs->s) {
    fprintf(stdout, "%s: %.*s", key, (int)cs->len, cs->s);
  } else {
    fprintf(stdout, "%s: <nil>", key);
  }
  if (eol) {
    fprintf(stdout, "\n");
  }
}


static void output_text(lsb_heka_message *msg)
{
  static char tstr[64];
  if (!msg->raw.s) return;

  fprintf(stdout, ":Uuid: %02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx"
          "-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx\n",
          msg->uuid.s[0], msg->uuid.s[1], msg->uuid.s[2], msg->uuid.s[3],
          msg->uuid.s[4], msg->uuid.s[5], msg->uuid.s[6], msg->uuid.s[7],
          msg->uuid.s[8], msg->uuid.s[9], msg->uuid.s[10], msg->uuid.s[11],
          msg->uuid.s[12], msg->uuid.s[13], msg->uuid.s[14], msg->uuid.s[15]);
  fprintf(stdout, ":Timestamp: ");

  time_t t = floor(msg->timestamp / 1e9);
  double frac = msg->timestamp - t * 1e9;
  struct tm *tms = gmtime(&t);
  strftime(tstr, sizeof(tstr) - 1, "%Y-%m-%dT%H:%M:%S", tms);
  fprintf(stdout, "%s.%09lldZ\n", tstr, (long long)frac);
  output_cs(":Type", &msg->type, true);
  output_cs(":Logger", &msg->logger, true);
  fprintf(stdout, ":Severity: %d\n", msg->severity);
  output_cs(":Payload", &msg->payload, true);
  output_cs(":EnvVersion", &msg->env_version, true);
  if (msg->pid == INT_MIN) {
    fprintf(stdout, ":Pid: <nil>\n");
  } else {
    fprintf(stdout, ":Pid: %d\n", msg->pid);
  }
  output_cs(":Hostname", &msg->hostname, true);
  fprintf(stdout, ":Fields:\n");
  for (int i = 0; i < msg->fields_len; ++i) {
    fprintf(stdout, "    | name: %.*s type: %d ", (int)msg->fields[i].name.len,
            msg->fields[i].name.s, msg->fields[i].value_type);
    output_cs("representation", &msg->fields[i].representation, false);
    fprintf(stdout, " value: ");
    const char *p = msg->fields[i].value.s;
    const char *e = msg->fields[i].value.s + msg->fields[i].value.len;
    switch (msg->fields[i].value_type) {
    case LSB_PB_STRING:
      {
        lsb_const_string cs;
        int tag = 0;
        int wiretype = 0;
        while (p && p < e) {
          p = lsb_pb_read_key(p, &tag, &wiretype);
          p = read_string(wiretype, p, e, &cs);
          if (p) {
            fprintf(stdout, "%.*s", (int)cs.len, cs.s);
            if (p < e) fprintf(stdout, "|");
          }
        }
      }
      break;
    case LSB_PB_BYTES:
      {
        lsb_const_string cs;
        int tag = 0;
        int wiretype = 0;
        while (p && p < e) {
          p = lsb_pb_read_key(p, &tag, &wiretype);
          p = read_string(wiretype, p, e, &cs);
          if (p) {
            for (size_t i = 0; i < cs.len; ++i) {
              if (isprint(cs.s[i])) {
                if (cs.s[i] == '\\') {
                  fwrite("\\\\", 2, 1, stdout);
                } else {
                  putchar(cs.s[i]);
                }
              } else {
                fprintf(stdout, "\\x%02hhx", (unsigned char)cs.s[i]);
              }
            }
            if (p < e) fprintf(stdout, "|");
          }
        }
      }
      break;
    case LSB_PB_INTEGER:
      {
        long long ll = 0;
        while (p && p < e) {
          p = lsb_pb_read_varint(p, e, &ll);
          if (p) {
            fprintf(stdout, "%lld", ll);
            if (p < e) fprintf(stdout, "|");
          }
        }
      }
      break;
    case LSB_PB_DOUBLE:
      {
        double d;
        for (int i = 0; p <= (e - sizeof(double)); p += sizeof(double), ++i) {
          memcpy(&d, p, sizeof(double));
          if (i > 0) fprintf(stdout, "|");
          fprintf(stdout, "%.17g", d);
        }
      }
      break;
    case LSB_PB_BOOL:
      {
        long long ll = 0;
        while (p && p < e) {
          p = lsb_pb_read_varint(p, e, &ll);
          if (p) {
            fprintf(stdout, "%s", ll == 0 ? "false" : "true");
            if (p < e) fprintf(stdout, "|");
          }
        }
      }
      break;
    }
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "\n");
  return;
}


static void output_heka(lsb_heka_message *msg)
{
  static char header[LSB_MIN_HDR_SIZE];
  size_t hlen = lsb_write_heka_header(header, msg->raw.len);
  if (fwrite(header, hlen, 1, stdout) != 1) {
    log_cb(NULL, NULL, 0, "error outputting header");
    exit(1);
  }
  if (fwrite(msg->raw.s, msg->raw.len, 1, stdout) != 1) {
    log_cb(NULL, NULL, 0, "error outputting message");
    exit(1);
  }
  return;
}


static size_t read_file(FILE *fh, lsb_input_buffer *ib)
{
  size_t need;
  if (ib->msglen) {
    need = ib->msglen + (size_t)ib->buf[ib->scanpos + 1] + LSB_HDR_FRAME_SIZE
        - (ib->readpos - ib->scanpos);
  } else {
    need = ib->scanpos + ib->size - ib->readpos;
  }

  if (lsb_expand_input_buffer(ib, need)) {
    log_cb(NULL, NULL, 0, "buffer reallocation failed");
    exit(EXIT_FAILURE);
  }
  size_t nread = fread(ib->buf + ib->readpos,
                       1,
                       ib->size - ib->readpos,
                       fh);
  ib->readpos += nread;
  return nread;
}


static int find_header(const char *cur, int clen, const char *prev, int num)
{
  static int cnt = 0;
  for (int i = clen - 1; i >= 0; --i) {
    if (cur[i] == 0x1e) {
      unsigned char hlen;
      if (i + 1 < clen) {
        hlen = (unsigned char)cur[i + 1];
      } else {
        hlen = (unsigned char)prev[0];
      }

      char flag;
      int hend = i + 2;
      if (hend < clen) {
        flag = cur[hend];
      } else {
        flag = prev[hend - clen];
      }

      if (flag != 0x08) {
        continue;
      }

      hend += hlen;
      if (hend < clen) {
        flag = cur[hend];
      } else {
        flag = prev[hend - clen];
      }

      if (flag == 0x1f) {
        if (++cnt == num) {
          return i;
        }
      }
    }
  }
  return -1;
}


static void move_to_offset(FILE *fh, int num)
{
  char buf[2][BUFSIZ];
  memset(buf, 0, sizeof(buf));
  char *cur = buf[0];
  char *prev = buf[1];
  char *tmp;

  fseek(fh, 0, SEEK_END);
  long len = ftell(fh);
  if (len < 0) {
    log_cb(NULL, NULL, 0, "ftell failed");
  }
  size_t consume = len > BUFSIZ ? BUFSIZ : len;
  size_t pos = len - consume;
  while (consume > 0) {
    if (fseek(fh, pos, SEEK_SET)) {
      log_cb(NULL, NULL, 0, "fseek failed (reading)");
      break;
    }
    if (fread(cur, consume, 1, fh) != 1) {
      log_cb(NULL, NULL, 0, "fread failed");
      break;
    }

    int loc = find_header(cur, consume, prev, num);
    if (loc >= 0) {
      if (fseek(fh, -(consume - loc), SEEK_CUR)) {
        log_cb(NULL, NULL, 0, "fseek failed (find position)");
      }
      return;
    }

    if (pos >= consume) {
      pos -= consume;
    } else {
      consume = pos;
      pos = 0;
    }

    tmp = cur;
    cur = prev;
    prev = tmp;
  }
  fseek(fh, 0, SEEK_SET);
}


int main(int argc, char **argv)
{
  bool argerr     = false;
  bool follow     = false;
  bool use_stdin  = false;
  long num        = -1;
  char *matcher   = "TRUE";
  char *filename  = NULL;

  output_function ofn = output_text;

  int c;
  while ((c = getopt(argc, argv, "tchfn:m:")) != -1) {
    switch (c) {
    case 't':
      ofn = output_text;
      break;
    case 'c':
      ofn = NULL;
      break;
    case 'h':
      ofn = output_heka;
      break;
    case 'f':
      follow = true;
      break;
    case 'n':
      num = strtol(optarg, NULL, 10);
      if (num < 0) argerr = true;
      break;
    case 'm':
      matcher = optarg;
      break;
    default:
      argerr = true;
      break;
    }
  }

  if (argc - optind == 1) {
    filename = argv[optind];
    use_stdin = strcmp("-", filename) == 0;
  } else {
    argerr = true;
  }

  if (argerr) {
    log_cb(NULL, NULL, 0,
           "usage: %s [-t|-c|-h] [-m message_matcher] [-f] [-n #] <FILE>\n"
           "description:\n"
           "  -t output the messages in text format (default)\n"
           "  -c only output the message count\n"
           "  -h output the messages as a Heka protobuf stream\n"
           "  -f output appended data as the file grows\n"
           "  -n output the last # of messages (simple header check so not "
           "100%% accurate)\n"
           "  -m message_matcher expression (default \"TRUE\")\n"
           "  FILE name of the file to cat or '-' for stdin\n"
           "notes:\n"
           "  All output is written to stdout and all log/error messages are "
           "written to stderr.\n",
           argv[0]);
    return EXIT_FAILURE;
  }

  char ms[strlen(matcher) + 1];
  size_t len = sizeof(ms);
  lsb_message_matcher *mm = lsb_create_message_matcher(
      lsb_lua_string_unescape(ms, matcher, &len));
  if (!mm) {
    log_cb(NULL, NULL, 0, "invalid message matcher: %s", matcher);
    return EXIT_FAILURE;
  }

  FILE *fh = stdin;
  if (!use_stdin) {
    fh = fopen(filename, "r");
    if (!fh) {
      log_cb(NULL, NULL, 0, "error opening: %s", filename);
      return EXIT_FAILURE;
    }
    if (num >= 0) {
      move_to_offset(fh, num);
    }
  }

  size_t discarded_bytes;
  size_t bytes_read = 0;
  size_t pcnt = 0;
  size_t mcnt = 0;

  lsb_input_buffer ib;
  lsb_init_input_buffer(&ib, 1024 * 1024 * 1024);
  lsb_heka_message msg;
  lsb_init_heka_message(&msg, 8);

  do {
    if (lsb_find_heka_message(&msg, &ib, true, &discarded_bytes, &logger)) {
      if (lsb_eval_message_matcher(mm, &msg)) {
        if (ofn) {
          ofn(&msg);
        }
        ++mcnt;
      }
      ++pcnt;
    } else {
      bytes_read = read_file(fh, &ib);
    }
    if (bytes_read == 0 && follow && !use_stdin) {
      sleep(1);
    }
  } while (bytes_read > 0 || follow);

  lsb_free_heka_message(&msg);
  lsb_free_input_buffer(&ib);
  lsb_destroy_message_matcher(mm);
  if (!use_stdin) {
    fclose(fh);
  }

  if (ofn) {
    log_cb(NULL, NULL, 0, "Processed: %zu, matched: %zu messages\n", pcnt,
           mcnt);
  } else {
    printf("Processed: %zu, matched: %zu messages\n", pcnt, mcnt);
  }
}
