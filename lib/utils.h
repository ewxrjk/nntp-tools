/*
 * Copyright (C) 2006, 2007, 2010 Richard Kettlewell
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

#include <config.h>

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Utility functions */

void error(int errno_value, const char *msg, ...);
void fatal(int errno_value, const char *msg, ...) attribute((noreturn));
void *xmalloc(size_t n);
void *xrealloc(void *ptr, size_t n) ;
char *xstrdup(const char *s);
void cloexec(int fd);
pid_t xfork(void);
void lock(pthread_mutex_t *m);
void unlock(pthread_mutex_t *m);
const char *w3date_to_822date(const char *w3date);
time_t bzrdate_to_time_t(const char *bzrdate);
const char *time_t_to_822date(time_t when);
time_t rfc822date_to_time_t(const char *rfc822date);
int fexists(const char *path);
char *capture(const char *cmd);

/* Debugging */

void do_debug(const char *s, ...);
extern int debug;                       /* debug output */
#define D(x) do { if(debug) do_debug x ; } while(0)

#if ! HAVE_OPEN_MEMSTREAM
FILE *open_memstream(char **ptr, size_t *sizeloc);
#endif

#if ! HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

#if ! HAVE_GETDELIM
ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);
#endif

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */

/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
