#
# This file is part of rjk-nntp-tools.
# Copyright (C) 2010, 2011, 13, 15 Richard Kettlewell
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
#

noinst_PROGRAMS=seen-t
noinst_LIBRARIES=libmisc.a libmiscpp.a
libmisc_a_SOURCES=getdelim.c getline.c nntp.c open_memstream.c utils.c	\
nntp.h utils.h capture.c io.h io.c recode.c seen.c seen.h error.h error.c
libmiscpp_a_SOURCES=cpputils.h split.cc Timezones.h Timezones.cc	\
HTML.h Escape.cc Quote.cc Header.cc case.cc parse_date.cc		\
parse_csv.cc compact_kilo.cc round_kilo.cc thead.cc read_file.cc	\
write_file.cc listdir.h

TESTS=seen-t

seen_t_SOURCES=seen-t.c
seen_t_LDADD=libmisc.a
