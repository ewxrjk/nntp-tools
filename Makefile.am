
#
# Copyright (C) 2005-2008 Richard Kettlewell
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

bin_PROGRAMS=lj2news tla2news bzr2news spoolstats

lj2news_SOURCES=lj2news.c utils.c utils.h nntp.c nntp.h
lj2news_LDADD=$(LIBOBJS) $(LIBEXPAT) $(LIBGCRYPT) $(LIBCURL) $(LIBPTHREAD)

tla2news_SOURCES=tla2news.c utils.c utils.h nntp.c nntp.h
tla2news_LDADD=$(LIBOBJS) $(LIBGCRYPT) $(LIBPTHREAD)

bzr2news_SOURCES=bzr2news.c utils.c utils.h nntp.c nntp.h
bzr2news_LDADD=$(LIBOBJS) $(LIBGCRYPT) $(LIBPTHREAD)

spoolstats_SOURCES=spoolstats.cc utils.c Article.h Article.cc Group.h	\
Group.cc timezones.h timezones.cc

man_MANS=lj2news.1 tla2news.1 bzr2news.1

EXTRA_DIST=$(man_MANS)
ACLOCAL_AMFLAGS=-I aclocal

SUBDIRS=debian

echo-distdir:
	@echo $(distdir)

echo-version:
	@echo $(VERSION)
