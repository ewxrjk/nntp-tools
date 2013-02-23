/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2013 Richard Kettlewell
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

#ifndef ERROR_H
#define ERROR_H


#ifdef __cplusplus
extern "C" {
#endif

/* Error-reporting functions */

void error(int errno_value, const char *msg, ...);
void fatal(int errno_value, const char *msg, ...) attribute((noreturn));

extern void (*exitfn)(int) attribute((noreturn));

extern int errors;

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
