/*
 * tla2news - post tla change logs to a news server
 * Copyright (C) 2006 Richard Kettlewell
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
#include <pthread.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <gcrypt.h>

#include "utils.h"
#include "nntp.h"

/* --- types --------------------------------------------------------------- */

struct line {
  struct line *next;
  char *line;
};

struct state {
  const char *subject;
  const char *keywords;
  const char *from;
  time_t date;
};

struct header {
  const char *name;
  void (*handler)(struct state *s,
                  const struct header *h,
                  const char *line);
  size_t offset;
};

/* --- forward declarations ------------------------------------------------ */

static void post_patch(const char *archive,
                       const char *category,
                       const char *version,
                       const char *patch);
static void handle_gather(struct state *s,
                          const struct header *h,
                          const char *line);
static void handle_stddate(struct state *s,
                           const struct header *h,
                           const char *line);
static time_t parse_tladate(const char *s);

/* --- variables ----------------------------------------------------------- */

static const struct header headers[] = {
  { "Creator", handle_gather, offsetof(struct state, from) },
  { "Keywords", handle_gather, offsetof(struct state, keywords) },
  { "Standard-Date", handle_stddate, 0 },
  { "Summary", handle_gather, offsetof(struct state, subject) },
};

#define NHEADERS (int)(sizeof headers / sizeof *headers)

static const char *newsgroup = 0;
static const char *msgiddomain = "tylerdurden.greenend.org.uk";
static const char *salt = "";

/* --- options ------------------------------------------------------------- */

const struct option options[] = {
  { "age", required_argument, 0,  'a' },
  { "salt", required_argument, 0, 'x' },
  { "newsgroup", required_argument, 0, 'n' },
  { "msggid-domain", required_argument, 0, 'M' },
  { "server",  required_argument, 0, 's' },
  { "port", required_argument, 0,  'p' },
  { "debug", no_argument,  0, 'd' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

/* --- main program -------------------------------------------------------- */

int main(int argc, char **argv) {
  int n;
  const char *server = 0;
  const char *port = 0;
  int pf = PF_UNSPEC;
  FILE *browsefp;
  char *line = 0,  *ptr;
  size_t linelen = 0;
  long maxage = 86400 * 7;              /* = a week */
  char *archive = 0, *category = 0, /**branch = 0,*/ *version = 0, *patch = 0;
  time_t now, when;

  while((n = getopt_long(argc, argv, "n:s:p:46dVa:M:x:h", options, 0)) >= 0) {
    switch(n) {
    case 'x':
      salt = optarg;
      break;
    case 'n':
      newsgroup = optarg;
      break;
    case 's':
      server = optarg;
      break;
    case 'p':
      if(*optarg) port = optarg;
      break;
    case '4':
      pf = AF_INET;
      break;
    case '6':
      pf = AF_INET6;
      break;
    case 'd':
      debug = 1;
      break;
    case 'a':
      maxage = atol(optarg);
      break;
    case 'M':
      if(*optarg) msgiddomain = optarg;
      break;
    case 'V':
      printf("tla2news from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    case 'h':
      printf("Usage:\n\
  tla2news -n GROUP [OPTIONS]\n\
\n\
Mandatory options:\n\
  -n, --newgroup GROUP               Newsgroup\n\
Optional options:\n\
  -a, --age SECONDS                  Age limit on commits\n\
  -s, --server HOSTNAME              NNTP server (default $NNTPSERVER)\n\
Rarely used options:\n\
  -p, --port PORT                    Port number (default 119)\n\
  -m, --msggid-domain DOMAIN         Message-ID domain\n\
  -x, --salt SALT                    Salt for ID calculation\n\
  -4, -6                             Use IPv4/IPv6 (latter untested)\n\
  -d, --debug                        Enable debug output\n");
      exit(0);
    default:
      exit(1);
    }
  }
  if(!newsgroup)
    fatal(0, "no -n option");
  /* TODO: non-default archive */
  create_postthread(pf, server, port);
  if(!(browsefp = popen("tla abrowse -D", "r")))
    fatal(errno, "error invoking tla abrowse");
  time(&now);
  while(getline(&line, &linelen, browsefp) != -1) {
    /* strip trailing blanks */
    for(ptr = line + strlen(line);
        ptr > line && isspace((unsigned char)ptr[-1]);
        --ptr)
      ;
    *ptr = 0;
    /* skip initial blanks */
    for(ptr = line; isspace((unsigned char)*ptr); ++ptr)
      ;
    if(!*ptr) continue;                 /* skip completely blank lines */
    switch(ptr - line) {
    case 0:
      archive = xstrdup(ptr);
      break;
    case 2:
      category = xstrdup(ptr);
      break;
    case 4:
      /*branch = xstrdup(ptr);*/
      break;
    case 6:
      version = xstrdup(ptr);
      break;
    case 8:
      patch = xstrdup(ptr);
      break;
    case 10:
      when = parse_tladate(ptr);
      if(when > now) {
        error(0, "%s/%s--%s is in the future", archive, version, patch);
        continue;
      }
      if(now - when > maxage) {
        D(("too old: %s/%s--%s", archive, version, patch));
        continue;                       /* too old */
      }
      post_patch(archive, category, version, patch);
      break;
    default:
      fatal(0, "bad line: [%d] %s", (int)(ptr - line), ptr);
      break;
    }
  }
  if(ferror(browsefp)) fatal(errno, "error reading from tla abrowse");
  if((n = pclose(browsefp)) < 0)
    fatal(errno, "error waiting for tla abrowse");
  else if(n)
    fatal(0, "tla abrowse exited with status %#x", (unsigned)n);
  join_postthread();
  return 0;
}

/* Post a given patch */
static void post_patch(const char *archive,
                       const char *category,
                       const char *version,
                       const char *patch) {
  char *cmd, *ptr;
  FILE *logfp, *output;
  char *line = 0;
  size_t linelen = 0;
  int n;
  struct state s;
  struct line *l, *lines, **linetail = &lines;
  char *msgid, *article = 0;
  size_t articlesize = 0;
  char date822[64];
  struct tm t;
  gcry_error_t gerr;
  gcry_md_hd_t hd;
  unsigned char *hash;
  char itemid[41];

  D(("post_patch %s/%s--%s", archive, version, patch));
  /* Get the log entry */
  if(asprintf(&cmd, "tla cat-archive-log %s/%s--%s",
              archive, version, patch) < 0)
    fatal(errno, "error calling asprintf");
  D(("cmd: %s", cmd));
  if(!(logfp = popen(cmd, "r")))
    fatal(errno, "error invoking tla cat-archive-log");
  while(getline(&line, &linelen, logfp) != -1) {
    if((ptr = strchr(line, '\n')))
      *ptr = 0;                         /* strip newline */
    *linetail = l = xmalloc(sizeof *l);
    l->line = xstrdup(line);
    linetail = &l->next;
  }
  if(ferror(logfp)) fatal(errno, "error reading from tla cat-archive-log");
  if((n = pclose(logfp)) < 0)
    fatal(errno, "error waiting for tla cat-archive-log");
  else if(n)
    fatal(0, "tla cat-archive-log exited with status %#x", (unsigned)n);
  *linetail = 0;                           /* terminate list */
  /* Fish some useful values out of the header */
  memset(&s, 0, sizeof s);
  s.from = s.subject = s.keywords = 0;
  s.date = (time_t)-1;
  for(l = lines; l && l->line[0]; l = l->next) {
    /* Decide whether to skip this header */
    if(!(ptr = strchr(l->line, ':')))
      continue;                         /* Continuation line, ignored */
    for(n = 0; n < NHEADERS; ++n)
      if(ptr - l->line == (int)strlen(headers[n].name)
         && !strncasecmp(headers[n].name, l->line, ptr - l->line))
        break;
    if(n < NHEADERS)
      headers[n].handler(&s, &headers[n], l->line);
  }
  if(!s.subject) fatal(0, "No Summary: field");
  if(s.date == (time_t)-1) fatal(0, "No Standard-Date: field");
  if(!s.from) fatal(0, "No Creator: field");
  /* Knock up a message ID */
  if((gerr = gcry_md_open(&hd, GCRY_MD_SHA1, 0)))
    fatal(0, "error calling gcry_md_open: %s/%s",
	  gcry_strsource(gerr), gcry_strerror(gerr));
  gcry_md_write(hd, salt, strlen(salt));
  gcry_md_write(hd, archive, strlen(archive));
  gcry_md_write(hd, version, strlen(version));
  gcry_md_write(hd, patch, strlen(patch));
  hash = gcry_md_read(hd, 0);
  for(n = 0; n < 20; ++n) sprintf(itemid + 2 * n, "%02x", hash[n]);
  gcry_md_close(hd);
  if(asprintf(&msgid, "<%s@%s>", itemid, msgiddomain) < 0)
    fatal(errno, "error calling asprintf");
  /* Construct the article */
  if(!(output = open_memstream(&article, &articlesize)))
    fatal(errno, "error calling open_memstream");
  /* The header */
  strftime(date822, sizeof date822, "%a, %d %b %Y %H:%M:%S GMT",
           gmtime_r(&s.date, &t));
  if(fprintf(output,
             "Newsgroups: %s\n"
             "From: %s\n"
             "Subject: [%s] %s\n"
             "Date: %s\n"
             "Message-ID: %s\n",
             newsgroup,
             s.from,
             category, s.subject,
             date822,
             msgid) < 0)
    fatal(errno, "error constructing article");
  if(s.keywords
     && *s.keywords
     && fprintf(output,
                "Keywords: %s\n", s.keywords) < 0)
    fatal(errno, "error constructing article");
  /* Exactly one blank line */
  if(fprintf(output, "\n") < 0)
    fatal(errno, "error constructing article");
  for(; l && !l->line[0]; l = l->next)
    ;
  /* The body */
  for(; l; l = l->next)
    if(fprintf(output, "%s\n", l->line) < 0)
      fatal(errno, "error constructing article");
  /* Exactly one blank line */
  if(fprintf(output, "\n") < 0)
    fatal(errno, "error constructing article");
  /* Arch header goes on the end (because less interesting) */
  for(l = lines; l && l->line[0]; l = l->next)
    if(fprintf(output, "%s\n", l->line) < 0)
      fatal(errno, "error constructing article");
  /* That's it */
  if(fclose(output) < 0) fatal(errno, "error constructing article");
  post(msgid, article);
}

/* Callback to remember a single header */
static void handle_gather(struct state *s, const struct header *h,
                          const char *line) {
  char **pp = (char **)((char *)s + h->offset);
  char *ptr;

  for(ptr = strchr(line, ':') + 1; isspace((unsigned char)*ptr); ++ptr)
    ;
  *pp = ptr;
  D(("gathered line %s as '%s'", line, ptr));
}

/* Callback to remember Standard-Date */
static void handle_stddate(struct state *s,
                           const struct header attribute((unused)) *h,
                           const char *line) {
  char *ptr;

  for(ptr = strchr(line, ':') + 1; isspace((unsigned char)*ptr); ++ptr)
    ;
  s->date = parse_tladate(ptr);
}

/* Convert a TLA date into a time_t */
static time_t parse_tladate(const char *s) {
  struct tm t;
  time_t when;

  if(sscanf(s, "%d-%d-%d %d:%d:%d GMT",
            &t.tm_year, &t.tm_mon, &t.tm_mday,
            &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
    fatal(0, "cannot parse date '%s'", s);
  /* struct tm is bizarre */
  t.tm_year -= 1900;
  t.tm_mon -= 1;
  t.tm_isdst = 0;
  when = timegm(&t);
  if(when == (time_t)-1)
    fatal(errno, "error converting date '%s'", s);
  return when;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
