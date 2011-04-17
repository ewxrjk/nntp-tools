/*
 * This file is part of rjk-nntp-tools
 * Copyright (C) 2007-2009, 2011 Richard Kettlewell
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

#include "utils.h"
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

/* Execute CMD via the shell and return its standard output as a string.
 * Terminates on error (including if the command exits nonzero or receives a
 * fatal signal). */
char *capture(const char *cmd) {
  char *result = 0;
  size_t result_len = 0;
  FILE *dfp, *mfp;
  int c, w;

  D(("popen %s", cmd));
  if(!(dfp = popen(cmd, "r")))
    fatal(errno, "error calling popen");
  if(!(mfp = open_memstream(&result, &result_len)))
    fatal(errno, "error calling open_memstream");
  while((c = fgetc(dfp)) != EOF)
    fputc(c, mfp);
  /* Check for error */
  w = pclose(dfp);
  /* -1 = could not wait for child */
  if(w < 0)
    fatal(errno, "error calling pclose: %s", strerror(errno));
  /* 0 = no change, 1/2 = changes, other = error */
  if(!WIFEXITED(w) || (WIFEXITED(w) && WEXITSTATUS(w) > 2))
    fatal(0, "command '%s' failed: wstat=%#x", cmd, w);
  /* Update diffsize */ 
  if(fflush(mfp) < 0)
    fatal(errno, "error calling fflush on memory stream");
  /* Make sure there's a final newline if nonempty */
  if(c != '\n' && result_len)
    fputc('\n', mfp);
  if(fclose(mfp) < 0)
    fatal(errno, "error calling fflush on memory stream");
  return result;
}
