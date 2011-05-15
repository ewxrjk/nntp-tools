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
#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdarg.h>

/* Timeout-aware IO handle */
typedef struct IO_data IO;

/* Create an IO handle from a file descriptor */
IO *io_create(int fd);

/* Set the IO timeout.  0 means no timeout (and is the default).  Timeouts
 * apply to top-level operations, not low-level reads and writes.  Returns 0 on
 * success or an errno value on error.
 *
 * If the timeout is nonzero then the FD will be put into nonblocking mode.
 */
int io_set_timeout(IO *io, int seconds);

/* CLose IO handle.  Returns 0 on success or an errno value on error. */
int io_close(IO *io);

/* Read a line.  *LINE points to a realloc-able buffer of size *LINESIZE; both
 * are updated.  NULL, 0 is acceptable.  The returned line is 0-terminated and
 * includes its final \n if it is not a final, partial line.  Returns 0 on
 * success (including a partial line at EOF), -1 at EOF (if there is no partial
 * line) and an errno value on any other errror. */
int io_getline(IO *io, char **line, size_t *linesize);

/* Flush pending writes.  Returns 0 on success or an errno value on error. */
int io_flush(IO *io);

/* Write LEN bytes starting at PTR.  If WROTE is not a null pointer, the number
 * of bytes actually written is stored there.  Returns 0 on success (in which
 * case *WROTE=LEN) or an errno value on error. */
int io_write(IO *io, const void *ptr, size_t len, size_t *wrote);

/* printf(3)-style formatting.  Returns 0 on success (in which case *WROTE=LEN)
 * or an errno value on error.  NB unlike printf() does not return a character
 * count. */
int io_printf(IO *io, const char *format, ...);
int io_vprintf(IO *io, const char *format, va_list ap);

/* Retrieve the file descriptor originally passed to io_create(). */
int io_fileno(IO *io);

#endif /* IO_H */

