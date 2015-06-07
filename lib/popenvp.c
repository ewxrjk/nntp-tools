/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2015 Richard Kettlewell
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
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

FILE *popenvp(const char *type, pid_t *pidp,
              const char *file, char *const argv[]) {
  int type_read = 0, type_write = 0, type_cloexec = 0;
  int p[2];
  pid_t pid;
  FILE *fp;

  while(*type) {
    switch(*type++) {
    case 'r': type_read = 1; break;
    case 'w': type_write = 1; break;
    case 'e': type_cloexec = 1; break;
    default: errno = EINVAL; return 0;
    }
  }
  if(type_read + type_write != 1) {
    errno = EINVAL;
    return 0;
  }
  if(pipe2(p, type_cloexec ? O_CLOEXEC : 0) < 0)
    return 0;
  switch(pid = fork()) {
  case 0:
    if(dup2(p[type_read], type_read) < 0)
      _exit(-1);
    if(close(p[0]) < 0 || close(p[1]) < 0)
      _exit(-1);
    if(execvp(file, argv) < 0)
      _exit(-1);
    _exit(-1);
  case -1:
    return 0;
  }
  if(close(p[type_read]) < 0)
    return NULL;
  if(!(fp = fdopen(p[type_write], type_read ? "r" : "w")))
    return NULL;
  if(pidp)
    *pidp = pid;
  return fp;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
