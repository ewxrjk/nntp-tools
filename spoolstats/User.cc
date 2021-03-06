/*
 * spoolstats - news spool stats
 * Copyright (C) 2014 Richard Kettlewell
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
#include "spoolstats.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

void become(const char *user) {
  struct passwd *pw = getpwnam(user);
  if(!pw)
    fatal(0, "unknown user: %s", user);
  if(initgroups(user, pw->pw_gid) < 0)
    fatal(errno, "initgroups");
  if(setgid(pw->pw_gid) < 0)
    fatal(errno, "setgid");
  if(setuid(pw->pw_uid) < 0)
    fatal(errno, "setuid");
}
