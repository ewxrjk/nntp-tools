/*
 * bzr2news - post bzr change logs to a news server
 * Copyright (C) 2007, 2008 Richard Kettlewell
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
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "nntp.h"

/* --- types --------------------------------------------------------------- */

struct logentry {
  int revno;
  time_t timestamp;
  char *branch;
  char *committer;
  char *message;
  char *added;
  char *removed;
  char *modified;
  char *renamed;
};

/* --- options ------------------------------------------------------------- */

const struct option options[] = {
  { "age", required_argument, 0,  'a' },
  { "salt", required_argument, 0, 'x' },
  { "newsgroup", required_argument, 0, 'n' },
  { "msggid-domain", required_argument, 0, 'M' },
  { "server",  required_argument, 0, 's' },
  { "port", required_argument, 0,  'p' },
  { "debug", no_argument, 0, 'd' },
  { "preview", no_argument, 0, 'P' },
  { "first", required_argument, 0, 'f' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

static long maxage = 86400 * 7;         /* = a week */
static const char *newsgroup = 0;
static const char *msgiddomain = "tylerdurden.greenend.org.uk";
static const char *salt = "";
static int preview;

/* --- parsing ------------------------------------------------------------- */

static void header(const char *name, char *text, struct logentry *l) {
  char *e = text + strlen(text);
  while(e > text && isspace((unsigned char)e[-1]))
    --e;
  *e = 0;
  D(("header: [%s] text: [%s]", name, text));
  if(!strcmp(name, "message"))
    l->message = xstrdup(text);
  else if(!strcmp(name, "branch nick"))
    l->branch = xstrdup(text);
  else if(!strcmp(name, "committer"))
    l->committer = xstrdup(text);
  else if(!strcmp(name, "added"))
    l->added = xstrdup(text);
  else if(!strcmp(name, "removed"))
    l->removed = xstrdup(text);
  else if(!strcmp(name, "modified"))
    l->modified = xstrdup(text);
  else if(!strcmp(name, "renamed"))
    l->renamed = xstrdup(text);
  else if(!strcmp(name, "revno"))
    l->revno = atoi(text);
  else if(!strcmp(name, "timestamp"))
    l->timestamp = bzrdate_to_time_t(text);
}

static void append(char **ptrp, size_t *lenp, const char *s) {
  const size_t slen = strlen(s);
  *ptrp = xrealloc(*ptrp, *lenp + slen + 1);
  memcpy(*ptrp + *lenp, s, slen);
  *lenp += slen;
  (*ptrp)[*lenp] = 0;
}

static void free_log(struct logentry *l) {
  free(l->branch);
  free(l->committer);
  free(l->message);
  free(l->added);
  memset(l, 0, sizeof *l);
}

/* read one log entry; return 0 on success, -ve on error. */
static int read_log(FILE *fp, struct logentry *l) {
  size_t nline = 0, ntext = 0;
  ssize_t rc;
  char *line = 0, *text = 0, *name = 0;
  const char *s;

  memset(l, 0, sizeof *l);
  /* skip dividers */
  while((rc = getline(&line, &nline, fp)) >= 0 && line[0] == '-')
    ;
  if(rc < 0) goto error;
  do {
    D(("line: [%.*s]", strlen(line)-1, line));
    if(line[0] == ' ') {
      if(!name)
	fatal(0, "continuation of a nonexistent header: %s", line);
      append(&text, &ntext,  line + 2);
    } else {
      if(text) {
	header(name, text, l);
	free(name);
	name = 0;
	free(text);
	text = 0;
	ntext = 0;
      }
      if(!(s = strchr(line, ':')))
	fatal(0, "bogus log line: %s", line);
      name = xmalloc(s - line + 1);
      memcpy(name, line, s - line);
      name[s - line] = 0;
      ++s;
      while(isspace((unsigned char)*s))
	++s;
      append(&text, &ntext, s);
    }
  } while((rc = getline(&line, &nline, fp)) >= 0 && line[0] != '-');
  if(rc < 0 && ferror(fp)) goto error;
  /* EOF is acceptable */
  if(text)
    header(name, text, l);
  if(!l->revno) fatal(0, "no revno found");
  if(!l->timestamp) fatal(0, "no timestamp found");
  if(!l->branch) fatal(0, "no branch found");
  if(!l->message) fatal(0, "no message found");
  /* l->added might be 0 and that's ok with us */
  rc = 0;
error:
  free(line);
  free(text);
  free(name);
  if(rc)
    free_log(l);
  if(rc)
    D(("read_log failed"));
  return rc;
}

/* --- main program -------------------------------------------------------- */

#if 0
/* post a list of files */
static void list(FILE *output, const char *heading, const char *files) {
  if(!files || !*files)
    return;
  if(fprintf(output, "\n%s:\n  ", heading) < 0)
    fatal(errno, "error constructing article");
  while(*files) {
    if(putc(*files, output) < 0)
      fatal(errno, "error constructing article");
    if(*files == '\n')
      if(fputs("  ", output) < 0)
        fatal(errno, "error constructing article");
    ++files;
  }
  if(putc('\n', output) < 0)
    fatal(errno, "error constructing article");
}
#endif

/* post a change */
static void post_log(const char *dir, const struct logentry *l) {
  gcry_error_t gerr;
  gcry_md_hd_t hd;
  unsigned char *hash;
  char itemid[41];
  char *msgid, *article = 0;
  size_t articlesize = 0;
  FILE *output;
  char date822[64];
  struct tm t;
  int n;

  D(("posting revision %d", l->revno));
  D(("message: %s", l->message));
  /* Knock up a message ID */
  if((gerr = gcry_md_open(&hd, GCRY_MD_SHA1, 0)))
    fatal(0, "error calling gcry_md_open: %s/%s",
	  gcry_strsource(gerr), gcry_strerror(gerr));
  gcry_md_write(hd, "bzr2news", 8);
  gcry_md_write(hd, salt, strlen(salt));
  gcry_md_write(hd, dir, strlen(dir));
  gcry_md_write(hd, &l->revno, sizeof l->revno);
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
           gmtime_r(&l->timestamp, &t));
  if(fprintf(output,
             "Newsgroups: %s\n"
             "From: %s\n"
             "Subject: [%s] revision %d\n"
             "Date: %s\n"
             "Message-ID: %s\n",
             newsgroup,
             l->committer,
             l->branch, l->revno,
             date822,
             msgid) < 0)
    fatal(errno, "error constructing article");
  /* Exactly one blank line */
  if(fprintf(output, "\n") < 0)
    fatal(errno, "error constructing article");
  /* The body */
  if(fprintf(output, "%s\n", l->message) < 0)
    fatal(errno, "error constructing article");
  /* That's it */
  if(fclose(output) < 0) fatal(errno, "error constructing article");
  if(preview) {
    if(printf("%s------------------------------------------------------------------------\n\n", article) < 0)
      fatal(errno, "error writing to stdout");
  } else
    post(msgid, article);
}

/* scan an archive's logs, posting new changes */
static void process_archive(const char *dir, int first) {
  FILE *fp;
  struct logentry l;
  int rc;
  time_t now;
  int olddir;
  char *cmd;

  if((olddir = open(".", O_RDONLY, 0)) < 0)
    fatal(errno, "cannot open .");
  time(&now);
  if(chdir(dir) < 0) fatal(errno, "cannot cd %s", dir);
  if(first >= 0) {
    if(asprintf(&cmd, "bzr log --timezone=utc --forward -r %d..",
                first) < 0)
      fatal(errno, "asprintf");
  } else {
    if(asprintf(&cmd, "bzr log --timezone=utc --forward") < 0)
      fatal(errno, "asprintf");
  }
  if(!(fp = popen(cmd, "r")))
    fatal(errno, "cannot popen bzr log");
  while(!read_log(fp, &l)) {
    D(("got revision %d", l.revno));
    if(!maxage || now - l.timestamp <= maxage)
      post_log(dir, &l);
    else
      D(("too old"));
    free_log(&l);
  }
  if(ferror(fp)) fatal(errno, "error reading from bzr log pipe");
  free_log(&l);
  if((rc = pclose(fp)) < 0) fatal(errno, "error closing bzr log pipe");
  else if(rc) fatal(0, "bzr log exited with wstat %#x", rc);
  if(fchdir(olddir) < 0)
    fatal(errno, "cannot fchdir back to original directory");
  close(olddir);
}

int main(int argc, char **argv) {
  int n;
  const char *server = 0;
  const char *port = 0;
  int pf = PF_UNSPEC;
  int first = -1;

  /* Force timezone to GMT */
  setenv("TZ", "UTC", 1);
  while((n = getopt_long(argc, argv, "n:s:p:46dVa:M:x:hPf:",
                         options, 0)) >= 0) {
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
    case 'P':
      preview = 1;
      break;
    case 'f':
      first = atoi(optarg);
      break;
    case 'V':
      printf("bzr2news from newstools version " VERSION "\n");
      exit(0);
    case 'h':
      printf("Usage:\n\
  bzr2news -n GROUP [OPTIONS] DIRECTORY...\n\
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
  -P, --preview                      Preview only, don't post\n\
  -l, --limit N                      Limit to first N revisions\n\
  -d, --debug                        Enable debug output\n");
      exit(0);
    default:
      exit(1);
    }
  }
  if(!newsgroup)
    fatal(0, "no -n option");
  if(!preview)
    create_postthread(pf, server, port);
  for(n = optind; n < argc; ++n)
    process_archive(argv[n], first);
  if(!preview)
    join_postthread();
  return 0;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
