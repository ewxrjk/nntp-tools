# rjk-nntp-tools

This package contains tools do with Usenet news.  It depends on cairo,
libpthread, libcurl, libexpat and libgcrypt.  It runs on Linux but
should not be too hard to get going on other UNIX platforms.


## Installation

    ./configure
    make
    make install

(as ever see `INSTALL` for more info.)


## `lj2news`

`lj2news` reads the RSS feed provided by Livejournal (or Dreamwidth),
formats it to plain text, and posts it to a newsgroup.
See `man lj2news` for documentation.

I suggest that you start by posting to a test newsgroup and setting
the `-x` option to something (any nonempty string will do).  Adjust the
command line until you are happy.  For each test (that posts anything)
you will need a fresh `-x` value.

When you are happy remove the `-x` option and set the `-n` option to point
to the newsgroup you really want to post to.

You can run the command manually or put it in a cronjob to run
e.g. once a day.

Be careful with cronjobs, sometimes their environment differs from the
interactive environment.  For instance you may need to explicitly set
`NNTPAUTH` and `NNTPSERVER` (though you can use the `-s` option instead of
the latter.)

For instance, I have the following in a script, which I execute from a
cron job:

    #!/bin/bash
    set -e
    export NNTPAUTH="md5cookie1way richard"
    lj2news \
      -a "http://www.greenend.org.uk/rjk/;rjk+ljua@greenend.org.uk" \
      -t ewx \
      -f "Richard Kettlewell <rjk@greenend.org.uk>" \
      -n chiark.journals \
      -s chiark-tunnel.greenend.org.uk \
      -S $HOME/.lj2news-sig.txt \
      http://www.livejournal.com/users/ewx/rss

Alternatively, use `setup-lj2news` which will ask you about your journal
and figure out the required command.


## `git2news` and `bzr2news`

`git2news` and `bzr2news` post Git and Bazaar changelog entries to a
newsgroup.  See `man git2news` or `man bzr2news` for documentation.

## `spoolstats`

`spoolstats` reads a news spool (directly) and generates an HTML report
giving numbers articles per day, posters, etc.  See `man spoolstats`
for documentation.  You can find example output at [www.greenend.org.uk/rjk/spoolstats](https://www.greenend.org.uk/rjk/spoolstats/).

## find-unhistorical

`find-unhistorical` reads an INN news spool and reports any articles
that aren't in the history file.  Since INN doesn't know about these
articles they are "lost" and will never be deleted in the normal
process of expiry.

This program is only built if the INN libraries can be located.

# Reporting Bugs

Please [report bugs via github](https://github.com/ewxrjk/nntp-tools/issues).

# Copyright

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

* The bulk of the code is copyright © 2005-2015 Richard Kettlewell and
subject to GPL 2 or later.  See `COPYING`.
* Portions copyright © 1997-date Stuart Langridge and subject to the
X11 licence.  See `COPYING.sorttable`.
