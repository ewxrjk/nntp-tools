#
# This file is part of rjk-nntp-tools.
# Copyright © 2013 Richard Kettlewell
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
bin_PROGRAMS=find-unhistorical
find_unhistorical_SOURCES=unhistorical.c inn-includes.h
find_unhistorical_LDADD=${INNLIBS} ../lib/libmisc.a
find_unhistorical_LDFLAGS=-L ${INNLIBDIR} -Wl,-rpath,${INNLIBDIR}
AM_CPPFLAGS=-I${top_srcdir}/lib
man_MANS=find-unhistorical.1
EXTRA_DIST=$(man_MANS)
