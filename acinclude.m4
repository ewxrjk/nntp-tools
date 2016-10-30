dnl
dnl This file is part of rjk-nntp-tools.
dnl Copyright © 2013 Richard Kettlewell
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl USA
dnl

dnl INN dev libs support
dnl
dnl Trying to use INN dev libs has the following problems:
dnl
dnl 1. You've no idea where they are.  There is no .pc file or anything.
dnl
dnl 2. You need to know whether to define HAVE_SSL to ensure that the
dnl    installed libraries are compatible with the headers.
dnl
dnl 3. They are (sometimes) static libraries so you need to recompile
dnl    after any updates.
dnl
dnl This macro addresses 1 and 2 up to a point.  You're on your own
dnl with 3.

AC_DEFUN([RJK_INN],[
  AC_CACHE_CHECK([for INN libraries],
                 [rjk_cv_innlib], [
    AC_ARG_WITH([inn-libs],
                [AS_HELP_STRING([--with-inn-libs=DIR],
                                [location of INN libraries])],
                [
      rjk_cv_innlib="${withval}"
    ],[
      rjk_cv_innlib=no
      for dir in /usr/lib/news /usr/local/lib/news; do
        if test -e ${dir}/libinn.a; then
          rjk_cv_innlib="${dir}"
        fi
      done
    ])
  ])
  if test "${rjk_cv_innlib}" != no; then
    INNLIB="${rjk_cv_innlib}"
    AC_CACHE_CHECK([whether INN libraries require -DHAVE_SSL],
                   [rjk_cv_innssl], [
      if grep tlscafile ${INNLIB}/libinn.a >/dev/null; then
        rjk_cv_innssl=yes
      else
        rjk_cv_innssl=no
      fi
    ])
    if test $rjk_cv_innssl = yes; then
      AC_DEFINE([INN_HAVE_SSL],[1],[define to 1 if INN was built with SSL support])
    fi
  fi
  AC_SUBST([INNLIB])
])
