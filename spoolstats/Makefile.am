#
# This file is part of rjk-nntp-tools.
# Copyright (C) 2010, 11, 14 Richard Kettlewell
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

bin_PROGRAMS=spoolstats

spoolstats_SOURCES=spoolstats.cc spoolstats.h Article.h Article.cc	\
Group.h Group.cc Bucket.h Bucket.cc SenderCountingBucket.h		\
SenderCountingBucket.cc AllGroups.h AllGroups.cc Hierarchy.h		\
Hierarchy.cc Conf.h Conf.cc css.c sorttable.c ArticleProperty.cc	\
ArticleProperty.h User.cc User.h

AM_CXXFLAGS=${CAIROMM_CFLAGS}
AM_CPPFLAGS=-I${top_srcdir}/lib -I${top_srcdir}/graph
LDADD=../lib/libmiscpp.a ../lib/libmisc.a ../graph/libgraph.a
LIBS=${CAIROMM_LIBS}

man_MANS=spoolstats.1

html: spoolstats.1.html

spoolstats.1.html: spoolstats.1
	${top_srcdir}/scripts/htmlman $^

EXTRA_DIST=$(man_MANS) sorttable.js spoolstats.css spoolstats.cron	\
spoolstats.default

css.c: spoolstats.css
	xxd -i $^ > $@.new
	mv $@.new $@
	clang-format -i $@

sorttable.c: sorttable.js
	xxd -i $^ > $@.new
	mv $@.new $@
	clang-format -i $@
