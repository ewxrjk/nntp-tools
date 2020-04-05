/*
 * This file is part of rjk-nntp-tools.
 * Copyright © 2013 Richard Kettlewell
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
#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "inn-includes.h"

static void help(void);
static void version(void);
static int ftw_callback(const char *fpath, const struct stat *sb, int typeflag,
                        struct FTW *ftwbuf);
static void check_article(const char *fpath, const struct stat *sb);
static char *load_article(const char *fpath, const struct stat *sb,
                          size_t *sizep);
static void malformed(const char *fpath);
static void action_list(const char *fpath);
static void action_remove(const char *fpath);

static struct history *history;
static int list_malformed = 1;
static int eol = '\n';
static int verbose = 0;
static void (*action)(const char *) = action_list;

static const struct option options[] = {{"malformed", no_argument, 0, 'm'},
                                        {"no-malformed", no_argument, 0, 'M'},
                                        {"list", no_argument, 0, 'l'},
                                        {"remove", no_argument, 0, 'r'},
                                        {"null", no_argument, 0, '0'},
                                        {"verbose", no_argument, 0, 'v'},
                                        {"help", no_argument, 0, 'h'},
                                        {"version", no_argument, 0, 'V'},
                                        {0, 0, 0, 0}};

int main(int argc, char **argv) {
  int n;
  const char *pathhistory;

  while((n = getopt_long(argc, argv, "hVmMrl0v", options, 0)) >= 0) {
    switch(n) {
    case 'm': list_malformed = 1; break;
    case 'M': list_malformed = 0; break;
    case 'l': action = action_list; break;
    case 'r': action = action_remove; break;
    case '0': eol = 0; break;
    case 'v': ++verbose; break;
    case 'h': help(); return 0;
    case 'V': version(); return 0;
    default: return 1;
    }
  }
  if(!innconf_read(NULL))
    fatal(0, "cannot open inn.conf");
  pathhistory = concatpath(innconf->pathdb, INN_PATH_HISTORY);
  if(!(history = HISopen(pathhistory, innconf->hismethod, HIS_RDONLY)))
    fatal(errno, "opening %s", pathhistory);
  if(nftw(innconf->patharticles, ftw_callback, 128, FTW_PHYS) < 0)
    fatal(errno, "nftw %s failed", innconf->patharticles);
  if(!HISclose(history))
    fatal(errno, "closing %s", pathhistory);
  if(fclose(stdout) < 0)
    fatal(errno, "stdout");
  return !!errors;
}

static void help(void) {
  if(printf(
         "Usage:\n"
         "  find-unhistorical [OPTIONS]\n"
         "Options:\n"
         "  -l, --list          List lost articles (default)\n"
         "  -r, --remove        Remove lost articles\n"
         "  -0, --null          Terminate filenames with \\0 instead of \\n\n"
         "  -m, --malformed     Include malformed articles (default)\n"
         "  -M, --no-malformed  Don't include malformed articles\n"
         "  -v, --verbose       Write directory names to stderr\n"
         "  -h, --help          Display usage message\n"
         "  -V, --version       Display version string\n"
         "\n"
         "Lists or removes files in the news spool that aren't reflected in "
         "the history\n"
         "file.\n")
     < 0)
    fatal(errno, "stdout");
}

static void version(void) {
  if(printf("find-unhistorical from rjk-nntp-tools version " VERSION "\n") < 0)
    fatal(errno, "stdout");
}

static int ftw_callback(const char *fpath, const struct stat *sb, int typeflag,
                        struct FTW attribute((unused)) * ftwbuf) {
  switch(typeflag) {
  case FTW_F: check_article(fpath, sb); break;
  case FTW_D:
    if(verbose)
      fprintf(stderr, "%s\n", fpath);
    break;
  case FTW_SL:
  case FTW_SLN: break;
  case FTW_DNR: error(errno, "cannot read %s", fpath); break;
  case FTW_NS: error(errno, "cannot stat %s", fpath); break;
  default: fatal(0, "unexpected typeflag %d for %s", typeflag, fpath);
  }
  return 0;
}

static void check_article(const char *fpath, const struct stat *sb) {
  size_t size;
  char *article, *mid, *end;

  if(!(article = load_article(fpath, sb, &size)))
    return;
  /* Find the Message-ID header.  If there is no well-formed Message-ID, report
   * the article as malformed. */
  if(!(mid = wire_findheader(article, size, "Message-ID", 1))
     || !(end = wire_endheader(mid, article + size)))
    malformed(fpath);
  else {
    /* Isolate the ID */
    while(end > mid && isspace((unsigned char)end[-1]))
      --end;
    *end = 0;
    if(!HIScheck(history, mid))
      action(fpath);
  }
  free(article);
}

static char *load_article(const char *fpath, const struct stat *sb,
                          size_t *sizep) {
  int fd;
  char *article;
  off_t got, thisread;
  ssize_t n;

  if((fd = open(fpath, O_RDONLY)) < 0) {
    error(errno, "opening %s", fpath);
    return NULL;
  }
  article = xmalloc(sb->st_size + 1);
  got = 0;
  while(got < sb->st_size) {
    thisread = sb->st_size - got;
    if(thisread > SSIZE_MAX)
      thisread = SSIZE_MAX;
    n = read(fd, article + got, (size_t)thisread);
    if(n < 0) {
      if(errno != EINTR && errno != EAGAIN) {
        error(errno, "reading %s", fpath);
        close(fd);
        return NULL;
      }
    } else if(n == 0) {
      error(0, "reading %s: truncated", fpath);
      close(fd);
      return NULL;
    } else
      got += n;
  }
  n = read(fd, article + got, 1);
  if(n < 0) {
    error(errno, "reading %s", fpath);
    close(fd);
    return NULL;
  }
  if(n != 0) {
    error(0, "reading %s: longer than expected", fpath);
    close(fd);
    return NULL;
  }
  close(fd);
  /* Convert to wire format if necessary */
  if(!innconf->wireformat) {
    char *wire = wire_from_native(article, sb->st_size, sizep);
    free(article);
    article = wire;
  } else
    *sizep = sb->st_size;
  return article;
}

static void malformed(const char *fpath) {
  if(list_malformed)
    action(fpath);
}

static void action_list(const char *fpath) {
  if(printf("%s%c", fpath, eol) < 0)
    fatal(errno, "stdout");
}

static void action_remove(const char *fpath) {
  if(unlink(fpath) < 0)
    error(errno, "removing %s", fpath);
}
