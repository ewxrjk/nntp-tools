/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2005-08, 2010-11 Richard Kettlewell
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

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>

#include "utils.h"

/* --- utilities ----------------------------------------------------------- */

/* Allocate memory, call fatal() on error */
void *xmalloc(size_t n) {
  void *ptr;

  if(n == 0)
    n = 1;
  if(!(ptr = malloc(n)))
    fatal(errno, "error calling malloc");
  return ptr;
}

/* Realloc memory, call fatal() on error */
void *xrealloc(void *ptr, size_t n) {
  if(n == 0)
    n = 1;
  if(!(ptr = realloc(ptr, n)))
    fatal(errno, "error calling realloc");
  return ptr;
}

/* Duplicate a string, call fatal() on error */
char *xstrdup(const char *s) {
  return strcpy(xmalloc(strlen(s) + 1), s);
}

/* Duplicate the first N bytes of string S, call fatal() on error */
char *xstrndup(const char *s, size_t n) {
  char *new = xmalloc(n + 1);
  memcpy(new, s, n);
  new[n] = 0;
  return new;
}

/* make FD close-on-exec */
void cloexec(int fd) {
  int flags;

  if((flags = fcntl(fd, F_GETFD)) < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC))
    fatal(errno, "error calling fcntl");
}

/* call fork and set exitfn */
pid_t xfork(void) {
  pid_t pid;

  switch(pid = fork()) {
  case 0: exitfn = _exit; break;
  case -1: fatal(errno, "error calling fork");
  }
  return pid;
}

/* acquire mutex */
void lock(pthread_mutex_t *m) {
  int err;

  D(("acquire %p", (void *)m));
  if((err = pthread_mutex_lock(m)))
    fatal(err, "error calling pthread_mutex_lock");
  D(("acquired %p", (void *)m));
}

/* release mutex */
void unlock(pthread_mutex_t *m) {
  int err;

  D(("release %p", (void *)m));
  if((err = pthread_mutex_unlock(m)))
    fatal(err, "error calling pthread_mutex_unlock");
}

/* Return nonzero if PATH exists, else 0.  On error, calls fatal(). */
int fexists(const char *path) {
  struct stat sb;
  if(stat(path, &sb) < 0) {
    if(errno == ENOENT)
      return 0;
    fatal(errno, "error statting %s", path);
  }
  return 1;
}

/* Return nonzero if PATH is a directory, else 0.  On error, calls fatal(). */
int isdir(const char *path) {
  struct stat sb;
  if(stat(path, &sb) < 0) {
    if(errno == ENOENT)
      return 0;
    fatal(errno, "error statting %s", path);
  }
  return S_ISDIR(sb.st_mode);
}

/* --- convert a W3C date to an RFC822 date -------------------------------- */

/* https://dublincore.org/documents/dces/ said:
 * ------------------------------------------------------------------------
 *  Typically, Date will be associated with the creation or availability
 *  of the resource. Recommended best practice for encoding the date value
 *  is defined in a profile of ISO 8601 [W3CDTF] and includes (among
 *  others) dates of the form YYYY-MM-DD."
 * ------------------------------------------------------------------------
 *
 * https://www.w3.org/TR/NOTE-datetime says:
 * ------------------------------------------------------------------------
 * "The formats are as follows. Exactly the components shown here must be
 * present, with exactly this punctuation. Note that the "T" appears
 * literally in the string, to indicate the beginning of the time
 * element, as specified in ISO 8601.
 *
 *     Year:
 *        YYYY (eg 1997)
 *     Year and month:
 *        YYYY-MM (eg 1997-07)
 *     Complete date:
 *        YYYY-MM-DD (eg 1997-07-16)
 *     Complete date plus hours and minutes:
 *        YYYY-MM-DDThh:mmTZD (eg 1997-07-16T19:20+01:00)
 *     Complete date plus hours, minutes and seconds:
 *        YYYY-MM-DDThh:mm:ssTZD (eg 1997-07-16T19:20:30+01:00)
 *     Complete date plus hours, minutes, seconds and a decimal fraction of a
 *     second
 *        YYYY-MM-DDThh:mm:ss.sTZD (eg 1997-07-16T19:20:30.45+01:00)
 *
 *  where:
 *
 *       YYYY = four-digit year
 *       MM   = two-digit month (01=January, etc.)
 *       DD   = two-digit day of month (01 through 31)
 *       hh   = two digits of hour (00 through 23) (am/pm NOT allowed)
 *       mm   = two digits of minute (00 through 59)
 *       ss   = two digits of second (00 through 59)
 *       s    = one or more digits representing a decimal fraction of a second
 *       TZD  = time zone designator (Z or +hh:mm or -hh:mm)
 *  ------------------------------------------------------------------------
 *
 * (Z meaning UTC.)  All this extra complexity is presumably supposed to have
 * some kind of advantage over 822-style dates.
 */

static int do_strtol(const char **ptrp) {
  long n;
  char *end;

  errno = 0;
  n = strtol(*ptrp, &end, 10);
  if(errno)
    fatal(errno, "error converting integer");
  if(end == *ptrp)
    fatal(0, "no integer found");
  *ptrp = end;
  if(n < INT_MIN || n > INT_MAX)
    fatal(0, "integer %ld out of range", n);
  return n;
}

static time_t w3date_to_time_t(const char *w3date) {
  int year, mon = 0, day = 1, hour = 0, min = 0, sec = 0, tzh = 0, tzm = 0;
  const char *ptr = w3date;
  long tz;
  time_t t;
  struct tm bdt;

  year = do_strtol(&ptr);
  if(year < 1900)
    fatal(0, "year too early");
  if(*ptr == '-') {
    ++ptr;
    mon = do_strtol(&ptr);
    if(mon < 1 || mon > 12)
      fatal(0, "invalid month number");
    if(*ptr == '-') {
      ++ptr;
      day = do_strtol(&ptr);
      if(day < 1 || day > 31)
        fatal(0, "invalid day number");
      if(*ptr == 'T') {
        ++ptr;
        hour = do_strtol(&ptr);
        if(hour < 0 || hour > 23)
          fatal(0, "invalid hour number");
        if(*ptr != ':')
          fatal(0, "no minutes field found");
        ++ptr;
        min = do_strtol(&ptr);
        if(min < 0 || min > 59)
          fatal(0, "invalid minute number");
        if(*ptr == ':') {
          ++ptr;
          sec = do_strtol(&ptr);
          /* W3 may be ignorant of leap-seconds but we can choose not to be */
          if(sec < 0 || sec > 61)
            fatal(0, "invalid second number");
          if(*ptr == '.') {
            ++ptr;
            do_strtol(&ptr); /* throw away fractional seconds */
          }
        }
        if(*ptr == '+' || *ptr == '-') {
          tzh = do_strtol(&ptr);
          if(*ptr != ':')
            fatal(0, "no minutes field found in timezone");
          ++ptr;
          tzm = do_strtol(&ptr);
        } else if(*ptr == 'Z')
          ++ptr;
        else
          fatal(0, "invalid timezone");
      }
    }
  }
  if(*ptr)
    fatal(0, "invalid W3 date");
  fprintf(stderr,
          "w3date: %s\n"
          "%d-%d-%d %d:%d:%d %d, %d\n",
          w3date, year, mon, day, hour, min, sec, tzh, tzm);
  /* Timezone offset in seconds */
  tz = tzh > 0 ? tzh * 3600 + tzm * 60 : tzh * 3600 - tzm * 60;
  memset(&bdt, 0, sizeof bdt);
  bdt.tm_year = year - 1900;
  bdt.tm_mon = mon - 1;
  bdt.tm_mday = day;
  bdt.tm_hour = hour;
  bdt.tm_min = min;
  bdt.tm_sec = sec;
  bdt.tm_isdst = 0;
  /* We clobbered TZ in main() so mktime() should give us UTC */
  if((t = mktime(&bdt)) == (time_t)-1)
    fatal(errno, "error calling mktime");
  return t - tz; /* Correct for timezone */
}

const char *time_t_to_822date(time_t when) {
  struct tm utc;
  char tbuf[64];

  if(!gmtime_r(&when, &utc))
    fatal(errno, "error calling gmtime_r");
  strftime(tbuf, sizeof tbuf, "%a, %d %b %Y %H:%M:%S GMT", &utc);
  return xstrdup(tbuf);
}

const char *w3date_to_822date(const char *w3date) {
  return time_t_to_822date(w3date_to_time_t(w3date));
}

time_t rfc822date_to_time_t(const char *rfc822date) {
  struct tm bdt;
  time_t t;

  /* strptime doesn't handle timezones usefully, so must use GMT */
  if(!strptime(rfc822date, "%a, %d %b %Y %H:%M:%S", &bdt))
    fatal(0, "RFC822 date not understood: %s", rfc822date);
  if((t = mktime(&bdt)) == (time_t)-1)
    fatal(errno, "error calling mktime");
  return t;
}

/* --- convert a bzr date to a time_t -------------------------------------- */

/* bzr dates look like 'Sun 2007-01-28 13:19:43 +0000' */
time_t bzrdate_to_time_t(const char *bzrdate) {
  int year, mon, day, hour, min, sec, tz, tzs;
  const char *ptr = bzrdate;
  struct tm bdt;
  time_t t;

  if(!isalpha((unsigned char)ptr[0]) || !isalpha((unsigned char)ptr[1])
     || !isalpha((unsigned char)ptr[2]) || ptr[3] != ' ')
    fatal(0, "unexpected date format: %s", bzrdate);
  ptr += 4;
  year = do_strtol(&ptr);
  if(year < 1900)
    fatal(0, "year too early: %s", bzrdate);
  if(*ptr++ != '-')
    fatal(0, "expected '-' after year: %s", bzrdate);
  mon = do_strtol(&ptr);
  if(mon < 1 || mon > 12)
    fatal(0, "month out of range: %s", bzrdate);
  if(*ptr++ != '-')
    fatal(0, "expected '-' after month: %s", bzrdate);
  day = do_strtol(&ptr);
  if(day < 1 || day > 31)
    fatal(0, "day out of range: %s", bzrdate);
  if(*ptr++ != ' ')
    fatal(0, "expected ' ' after month: %s", bzrdate);
  hour = do_strtol(&ptr);
  if(hour < 0 || hour > 23)
    fatal(0, "hour out of range: %s", bzrdate);
  if(*ptr++ != ':')
    fatal(0, "expected ':' after day: %s", bzrdate);
  min = do_strtol(&ptr);
  if(min < 0 || min > 59)
    fatal(0, "minute out of range: %s", bzrdate);
  if(*ptr++ != ':')
    fatal(0, "expected ':' after day: %s", bzrdate);
  sec = do_strtol(&ptr);
  if(min < 0 || min > 61)
    fatal(0, "second out of range: %s", bzrdate);
  if(*ptr++ != ' ')
    fatal(0, "expected ' ' after day: %s", bzrdate);
  switch(*ptr++) {
  case '+': tzs = 60; break;
  case '-': tzs = -60; break;
  default: fatal(0, "expected sign at start of timezone: %s", bzrdate);
  }
  tz = do_strtol(&ptr);
  if(tz < 0 || tz / 100 > 23 || tz % 100 > 59)
    fatal(0, "timezone out of range: %s", bzrdate);
  tz = tzs * (tz % 100 + 60 * (tz / 100)); /* convert to seconds */
  while(isspace((unsigned char)*ptr))
    ++ptr;
  if(*ptr)
    fatal(0, "junk at end of timestamp: %s", bzrdate);
  memset(&bdt, 0, sizeof bdt);
  bdt.tm_year = year - 1900;
  bdt.tm_mon = mon - 1;
  bdt.tm_mday = day;
  bdt.tm_hour = hour;
  bdt.tm_min = min;
  bdt.tm_sec = sec;
  bdt.tm_isdst = 0;
  /* We clobbered TZ in main() so mktime() should give us UTC */
  if((t = mktime(&bdt)) == (time_t)-1)
    fatal(errno, "error calling mktime");
  return t - tz; /* Correct for timezone */
}

/* --- debugging support --------------------------------------------------- */

int debug;

void do_debug(const char *s, ...) {
  va_list ap;
  pthread_t me = pthread_self();
  char idbuf[2 * sizeof me + 1];
  size_t n;

  for(n = 0; n < sizeof me; ++n)
    sprintf(idbuf + 2 * n, "%02x", ((unsigned char *)&me)[n]);
  fprintf(stderr, "%s: ", idbuf);
  va_start(ap, s);
  vfprintf(stderr, s, ap);
  va_end(ap);
  fputc('\n', stderr);
}

#define D(x)                                                                   \
  do {                                                                         \
    if(debug)                                                                  \
      do_debug x;                                                              \
  } while(0)
