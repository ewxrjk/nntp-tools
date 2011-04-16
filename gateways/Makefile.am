#
# Copyright (C) 2005-2008, 2010-11 Richard Kettlewell
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

bin_PROGRAMS=lj2news bzr2news

lj2news_SOURCES=lj2news.c
lj2news_LDADD=../lib/libmisc.a $(LIBEXPAT) $(LIBGCRYPT) $(LIBCURL) $(LIBPTHREAD)

bzr2news_SOURCES=bzr2news.c
bzr2news_LDADD=../lib/libmisc.a $(LIBGCRYPT) $(LIBPTHREAD)

AM_CPPFLAGS=-I${top_srcdir}/lib

man_MANS=lj2news.1 bzr2news.1 git2news.1

EXTRA_DIST=$(man_MANS) setup-lj2news

install-exec-hook:
	rm -f $(DESTDIR)$(bindir)/git2news$(EXEEXT)
	ln $(DESTDIR)$(bindir)/bzr2news$(EXEEXT) \
                $(DESTDIR)$(bindir)/git2news$(EXEEXT)
