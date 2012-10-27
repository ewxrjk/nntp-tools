/*
 * This file is part of rjk-nntp-tools.
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

#ifndef NNTP_H
#define NNTP_H

void create_postthread(int pf, const char *server, const char *port,
                       int timeout);
/* Must be called before first post() */

void join_postthread(void);
/* Must be called after last post() */

void post(const char *msgid, const char *article);
/* Post an article with a given message ID */

#endif /* NNTP_H */
