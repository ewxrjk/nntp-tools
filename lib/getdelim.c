/*
 * This file is part of rjk-nntp-tools
 * Copyright (C) 2005 Richard Kettlewell
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

#if ! HAVE_GETDELIM

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"

/* portable reimplementation of GNU getdelim */

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream) {
  size_t size = 0, space = *n, newspace;
  char *line = *lineptr, *newline;
  int c, save_errno;
  
  while((c = getc(stream)) != EOF) {
    if(size + 2 > space) {
      newspace = space ? 2 * space : 32;
      if(!(newline = realloc(line, newspace))) {
	save_errno = errno;
	ungetc(c, stream);
	errno = save_errno;
	return -1;
      }
      *lineptr = line = newline;
      *n = space = newspace;
    }
    line[size++] = c;
    if(c == delimiter) break;
  }
  if(size && !ferror(stream)) {
    line[size] = 0;
    return 0;
  }
  return -1;
}

#endif
