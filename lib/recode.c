/*
 * This file is part of rjk-nntp-tools.
 * Copyright © 2011 Richard Kettlewell
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
#include "utils.h"
#include <iconv.h>
#include <string.h>
#include <errno.h>

char *recode(const char *input,
             const char *from,
             const char *to) {
  iconv_t cd;
  char *inptr, *outptr, outbuffer[256], *buffer = NULL;
  size_t inleft, outleft, len = 0, rc, outbytes;

  if(!strcmp(from, to))
    return xstrdup(input);
  if((cd = iconv_open(to, from)) == (iconv_t)-1)
    fatal(errno, "iconv_open %s -> %s", from, to);
  inptr = (char *)input;
  inleft = strlen(input);
  while(inleft > 0) {
    /* Convert some bytes */
    outptr = outbuffer;
    outleft = sizeof outbuffer;
    rc = iconv(cd, &inptr, &inleft, &outptr, &outleft);
    /* Accumulate output */
    outbytes = (sizeof outbuffer) - outleft;
    buffer = xrealloc(buffer, len + outbytes);
    memcpy(buffer + len, outbuffer, outbytes);
    len += outbytes;
    if(rc == (size_t)-1) {
      switch(errno) {
      case EILSEQ:
      case EINVAL:
        /* Invalid input */
        buffer = xrealloc(buffer, len + 1);
        buffer[len++] = '?';
        ++inptr;
        --inleft;
        break;
      case E2BIG:
        /* Output buffer full */
        break;
      default:
        fatal(errno, "iconv");
      }
    } else
      break;
  }
  buffer = xrealloc(buffer, len + 1);
  buffer[len] = 0;
  iconv_close(cd);
  return buffer;
}
