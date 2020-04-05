/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2005-08, 2010-11 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#include <config.h>
#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* called by fatal() */
void (*exitfn)(int) attribute((noreturn)) = exit;

/* count of errors */
int errors;

/* report a non-fatal error */
void error(int errno_value, const char *msg, ...) {
  va_list ap;

  fprintf(stderr, "ERROR: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  if(errno_value)
    fprintf(stderr, ": %s", strerror(errno_value));
  fputc('\n', stderr);
  ++errors;
}

/* report a fatal error and exit via EXITFN (which might be _exit, in a
 * subprocess) */
void fatal(int errno_value, const char *msg, ...) {
  va_list ap;

  fprintf(stderr, "FATAL: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  if(errno_value)
    fprintf(stderr, ": %s", strerror(errno_value));
  fputc('\n', stderr);
  exitfn(1);
}
