#! /bin/bash
#
# This file is part of newstools
# Copyright (C) 2005, 2010 Richard Kettlewell
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

set -e

# Find an automake
if [ -z "$AUTOMAKE" ]; then
  for prog in automake automake-1.10 automake-1.9 automake-1.8 automake-1.7; do
    if type $prog >/dev/null 2>&1; then
      AUTOMAKE=$prog
      break
    fi
  done
  if [ -s "$AUTOMAKE" ]; then
    echo "ERROR: no automake found" >&2
    exit 1
  fi
fi
ACLOCAL=${AUTOMAKE/automake/aclocal}

# Reject ancient versions of Automake
amv=`${AUTOMAKE} --version|grep ^automake`
case "$amv" in
*1.4 | *1.4.* | *1.5 | *1.5.* | *1.6 | *1.6.* )
  echo Need Automake 1.7 or better >&2
  exit 1
  ;;
esac

srcdir=$(dirname $0)
here=$(pwd)
cd $srcdir
if [ -d $HOME/usr/share/aclocal ]; then
  ${ACLOCAL} -I $HOME/usr/share/aclocal -I aclocal
else
  ${ACLOCAL} -I aclocal
fi
autoconf
autoheader
${AUTOMAKE} -a || true		# for INSTALL
${AUTOMAKE} --foreign -a
rm -f config.cache
