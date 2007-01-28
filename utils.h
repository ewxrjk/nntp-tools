/*
 * Copyright (C) 2006 Richard Kettlewell
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

#ifndef UTILS_H
#define UTILS_H

/* Utility functions */

void error(int errno_value, const char *msg, ...);
void fatal(int errno_value, const char *msg, ...);
void *xmalloc(size_t n);
void *xrealloc(void *ptr, size_t n) ;
char *xstrdup(const char *s);
void cloexec(int fd);
pid_t xfork(void);
void lock(pthread_mutex_t *m);
void unlock(pthread_mutex_t *m);
const char *w3date_to_822date(const char *w3date);

/* Debugging */

void do_debug(const char *s, ...);
extern int debug;                       /* debug output */
#define D(x) do { if(debug) do_debug x ; } while(0)

#endif /* UTILS_H */


/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
