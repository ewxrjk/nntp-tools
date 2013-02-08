# Process this file with autoconf to produce a configure script.
#
# This file is part of rjk-nntp-tools.
# Copyright (C) 2005-2011 Richard Kettlewell
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

AC_INIT([rjk-nntp-tools], [0.23], [richard+newstools@sfere.greenend.org.uk])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([1.10])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([gateways/lj2news.c])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_SET_MAKE
AC_PROG_RANLIB

missing_libraries=""
missing_headers=""
missing_functions=""

AC_DEFINE(_GNU_SOURCE, 1, [required for e.g. strsignal])

AC_CACHE_CHECK([for fink],[rjk_cv_fink],[
  if test -d /sw; then
    rjk_cv_fink=/sw
  else
    rjk_cv_fink=no
  fi
])

if test "$rjk_cv_fink" != no; then
  CPPFLAGS="${CPPFLAGS} -isystem ${rjk_cv_fink}/include"
  LDFLAGS="${LDFLAGS} -L${rjk_cv_fink}/lib"
fi

case `uname -s` in
FreeBSD )
  CPPFLAGS="${CPPFLAGS} -isystem /usr/local/include"
  LDFLAGS="${LDFLAGS} -L/usr/local/lib"
  ;;
esac  

# Checks for libraries.
# We save up a list of missing libraries that we can't do without
# and report them all at once.
AC_CHECK_LIB(gcrypt, gcry_md_open,
             [AC_SUBST(LIBGCRYPT,[-lgcrypt])],
            [missing_libraries="$missing_libraries libgcrypt"])
AC_CHECK_LIB(expat, XML_Parse,
             [AC_SUBST(LIBEXPAT,[-lexpat])],
            [missing_libraries="$missing_libraries libexpat"])
AC_CHECK_LIB(pthread, pthread_create,
             [AC_SUBST(LIBPTHREAD,[-lpthread])],
            [missing_libraries="$missing_libraries libpthread"])
LIBCURL_CHECK_CONFIG([yes],,,[missing_libraries="$missing_libraries libcurl"])
AC_CACHE_CHECK([for Cairomm CFLAGS],[rjk_cv_cairomm_cflags],[
  rjk_cv_cairomm_cflags=`pkg-config --silence-errors --cflags cairomm-1.0`
])
AC_CACHE_CHECK([for Cairomm LIBS],[rjk_cv_cairomm_libs],[
  rjk_cv_cairomm_libs=`pkg-config --silence-errors --libs cairomm-1.0`
])
AC_CHECK_LIB([iconv],[iconv_open],[],
             [AC_CHECK_LIB([iconv],[libiconv_open])])
if test "$rjk_cv_cairomm_libs" = ""; then
  SPOOLSTATS=""
else
  SPOOLSTATS="graph spoolstats"
fi
AC_SUBST([SPOOLSTATS])
AC_SUBST([CAIROMM_CFLAGS],[$rjk_cv_cairomm_cflags])
AC_SUBST([CAIROMM_LIBS],[$rjk_cv_cairomm_libs])

if test ! -z "$missing_libraries"; then
  AC_MSG_ERROR([missing libraries:$missing_libraries])
fi

# Checks for header files.
# Compilation will fail if any of these headers are missing, so we
# check for them here and fail early.
# We don't bother checking very standard stuff
AC_CHECK_HEADERS([gcrypt.h expat.h pthread.h curl/curl.h],[:],[
  missing_headers="$missing_headers $ac_header"
])

if test ! -z "$missing_headers"; then
  AC_MSG_ERROR([missing headers:$missing_headers])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_C_INLINE
AC_C_BIGENDIAN

# Checks for functions
AC_CHECK_FUNCS([funopen open_memstream getline getdelim])

if test "x$GCC" = xyes; then
  # a reasonable default set of warnings
  CC="${CC} -Wall -W -Wpointer-arith -Wbad-function-cast \
	-Wwrite-strings -Wmissing-prototypes \
	-Wmissing-declarations -Wnested-externs -Wshadow -Wredundant-decls"

  AC_CACHE_CHECK([whether -Werror is usable],[rjk_cv_werror],[
    old_CC="${CC}"
    CC="${CC} -Werror"
    AC_TRY_COMPILE([#include <unistd.h>
		    #include <stdio.h>
		    #include <getopt.h>],
                   [],
 		   [rjk_cv_werror=yes],
 		   [rjk_cv_werror=no])
    CC="${old_CC}"
  ])
  if test $rjk_cv_werror = yes; then
    CC="${CC} -Werror"
  fi    
fi

if test "x$GXX" = xyes; then
  CXX="${CXX} -Wall -W -Wformat=2 -Wshadow -Wpointer-arith \
-Wmissing-noreturn -Wredundant-decls"
  CXX="${CXX} -Werror"
fi

AH_BOTTOM([#ifdef __GNUC__
# define attribute(x) __attribute__(x)
#else
# define attribute(x)
#endif])

AC_CONFIG_FILES([Makefile
                 debian/Makefile
                 gateways/Makefile
                 lib/Makefile
                 graph/Makefile
                 spoolstats/Makefile])
AC_OUTPUT
