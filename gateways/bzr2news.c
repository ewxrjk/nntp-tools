/*
 * bzr2news - post bzr change logs to a news server
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2007-2009, 2011 Richard Kettlewell
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
#include <sys/types.h>
#include <assert.h>

#include "utils.h"
#include "nntp.h"

/* --- types --------------------------------------------------------------- */

struct logentry {
  int revno;                            /* bzr only */
  char *commitid;                       /* git only */
  time_t timestamp;
  char *branch;
  char *committer;
  char *message;
  char *added;
  char *removed;
  char *modified;
  char *renamed;
  char *diff;
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
  { "diff", no_argument, 0, 'D' },
  { "diffs", no_argument, 0, 'D' },     /* in case of typos */
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
 { 0, 0, 0, 0 }
};

static long maxage = 86400 * 7;         /* = a week */
static const char *newsgroup = 0;
static const char *msgiddomain = "tylerdurden.greenend.org.uk";
static const char *salt = "";
static int preview;
static int diffs;
static const char *progname;

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
  else if(!strcmp(name, "committer")
          || !strcmp(name, "Author"))
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
  else if(!strcmp(name, "Date"))
    l->timestamp = atoll(text);         /* easy l-) */
  else if(!strcmp(name, "commit"))
    l->commitid = xstrdup(text);
  else
    D(("...unknown!"));
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
  free(l->commitid);
  free(l->diff);
  memset(l, 0, sizeof *l);
}

/* --- posting ------------------------------------------------------------- */

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
  char *subject, *newline;
  int subject_len;

  if(l->commitid)
    D(("posting commit %s", l->commitid));
  else
    D(("posting revision %d", l->revno));
  D(("message: %s", l->message));
  /* Knock up a message ID */
  if((gerr = gcry_md_open(&hd, GCRY_MD_SHA1, 0)))
    fatal(0, "error calling gcry_md_open: %s/%s",
	  gcry_strsource(gerr), gcry_strerror(gerr));
  gcry_md_write(hd, "bzr2news", 8);
  gcry_md_write(hd, salt, strlen(salt));
  gcry_md_write(hd, dir, strlen(dir));
  if(l->commitid)
    gcry_md_write(hd, l->commitid, strlen(l->commitid));
  else
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
             "Date: %s\n"
             "Message-ID: %s\n",
             newsgroup,
             l->committer,
             date822,
             msgid) < 0)
    fatal(errno, "error constructing article");
  /* Use the first line of the message in the subject */
  subject = l->message;
  while(isspace((unsigned char)*subject))
    ++subject;
  newline = strchr(subject, '\n');
  if(newline)
    subject_len = newline - subject;
  else
    subject_len = strlen(subject);
  if(subject_len > 58 - (int)strlen(l->branch))
    subject_len = 58 - (int)strlen(l->branch);
  if(l->commitid) {
    if(fprintf(output,
               "X-Git-Commit: %s\n"
               "Subject: [%s] %.8s %.*s\n",
               l->commitid,
               l->branch, l->commitid, subject_len, subject) < 0)
      fatal(errno, "error constructing article");
  } else {
    if(fprintf(output,
               "X-Bzr-Revision: %d\n"
               "Subject: [%s] %d %.*s\n",
               l->revno, l->branch, l->revno, subject_len, subject) < 0)
      fatal(errno, "error constructing article");
  }
  /* Exactly one blank line */
  if(fprintf(output, "\n") < 0)
    fatal(errno, "error constructing article");
  /* The body */
  if(fprintf(output, "%s\n", l->message) < 0)
    fatal(errno, "error constructing article");
  if(diffs && l->diff) {
    char *d = l->diff;
    while(isspace((unsigned char)*d))
      ++d;
    if(*d) {
      if(fprintf(output, "\n* Changes --------------------------------------------------------------\n\n%s",
                 d) < 0)
        fatal(errno, "error constructing article");
    }
  }
  /* That's it */
  if(fclose(output) < 0) fatal(errno, "error constructing article");
  if(preview) {
    if(printf("%s------------------------------------------------------------------------\n\n", article) < 0)
      fatal(errno, "error writing to stdout");
  } else
    post(msgid, article);
  free(msgid);
  free(article);
}

/* --- bzr support --------------------------------------------------------- */

static int is_bzr_divider(const char *line) {
  if(line[0] == '-')
    return 1;
  if(!strcmp(line, "Use --include-merges or -n0 to see merged revisions.\n"))
    return 1;
  return 0;
}

/* read one log entry; return 0 on success, -ve on error. */
static int read_bzr_log(FILE *fp, struct logentry *l, time_t now) {
  size_t nline = 0, ntext = 0;
  ssize_t rc;
  char *line = 0, *text = 0, *name = 0;
  const char *s;

  memset(l, 0, sizeof *l);
  /* skip dividers */
  while((rc = getline(&line, &nline, fp)) >= 0 && is_bzr_divider(line))
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
  /* Collecting the diff is expensive so only do so if we were asked for it and
   * it's within the age limit */
  if(diffs && (!maxage || now - l->timestamp <= maxage)) {
    char *cmd;

    if(asprintf(&cmd, "bzr diff -c%d", l->revno) < 0)
      fatal(errno, "error calling asprintf");
    l->diff = capture(cmd);
    free(cmd);
  }
  /* l->added might be 0 and that's ok with us */
  rc = 0;
error:
  free(line);
  free(text);
  free(name);
  if(rc)
    free_log(l);
  if(rc)
    D(("read_bzr_log failed"));
  return rc;
}

/* scan a bzr archive's logs, posting new changes */
static void process_bzr_archive(const char *dir, const char *branch, int first) {
  FILE *fp;
  struct logentry l;
  int rc;
  time_t now;
  char *cmd;

  if(branch)
    fatal(0, "separate branches not supported for bzr");
  time(&now);
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
  while(!read_bzr_log(fp, &l, now)) {
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
}

/* --- git support --------------------------------------------------------- */

static char *stem_git_name(const char *dir) {
  const char *slash = strrchr(dir, '/');
  char *stem = xstrdup(slash ? slash + 1 : dir), *ext = strrchr(stem, '.');
  if(ext && !strcmp(ext, ".git"))
    *ext = 0;
  return stem;
}

static void complete_git_commit(const char *dir, 
                                char *message, 
                                struct logentry *l) {
  time_t now;
  time(&now);
  if(!maxage || now - l->timestamp <= maxage) {
    size_t message_len;
    char *stem = stem_git_name(dir);
    header("branch nick", stem, l);
    free(stem);
    header("message", message, l);
    message = 0;
    message_len = 0;
    if(l->commitid) {
      if(diffs) {
        /* Collect the diff */
        char *diff_cmd;
            
        if(asprintf(&diff_cmd, "git show --format=format: %s", l->commitid) < 0)
          fatal(errno, "error calling asprintf");
        l->diff = capture(diff_cmd);
        free(diff_cmd);
      }
      D(("got commit %s", l->commitid));
      post_log(dir, l);
    }
  } else
    D(("too old"));
  free_log(l);
}

static void process_git_archive(const char *dir, const char *branch, int first) {
  char *cmd;
  struct logentry l;
  FILE *fp;
  ssize_t rc;
  char *line = 0;
  size_t linelen = 0;
  int state = 0;
  char *message = 0;
  size_t message_len = 0;

  memset(&l, 0, sizeof l);
  if(first >= 0)
    fatal(0, "--first option is not supported for git archives");/*TODO*/
  if(asprintf(&cmd, "git log --reverse --date=raw %s",
              branch ? branch : "") < 0)
    fatal(errno, "asprintf");
  if(!(fp = popen(cmd, "r")))
    fatal(errno, "cannot popen git log");
  free(cmd);
  while((rc = getline(&line, &linelen, fp)) >= 0) {
    /* Strip off trailing newline (and any other whitespace) */
    while(rc > 0 && isspace((unsigned char)line[rc - 1] ))
      --rc;
    line[rc] = 0;
    switch(state) {
    case 0:                             /* initial state */
      if(!*line)                        /* leading blank line!?! */
        continue;
      if(isspace((unsigned char)*line)) /* leading comment text!?! */
        continue;
      state = 1;                        /* fall through to read a header */

    case 1:                             /* reading header */
    collect_header:
      /* headers are at the start of the line */
      if(*line && !isspace((unsigned char)*line)) {
        size_t n = strcspn(line, ": ");
        if(line[n])
          line[n++] = 0;                /* delimit the header name */
        while(isspace((unsigned char)line[n]))
          ++n;
        header(line, &line[n], &l);
        break;                          /* done with this line */
      }
      /* else fall through into reading comment */
      state = 2;

    case 2:                             /* reading comment */
      /* non-space at the start is the start of the next commit */
      if(*line && !isspace((unsigned char)*line)) {
        complete_git_commit(dir, message, &l);
        free(message);
        message = NULL;
        message_len = 0;
        state = 1;
        goto collect_header;
      }
      /* accumulate the message for this commit */
      if(message || *line) {
        append(&message, &message_len, line);
        append(&message, &message_len, "\n");
      }
      break;
    }
  }
  if(ferror(fp)) fatal(errno, "error reading from git log pipe");
  if(l.commitid)
    complete_git_commit(dir, message, &l);
  free_log(&l);
  free(line);
  free(message);
  if((rc = pclose(fp)) < 0) fatal(errno, "error closing git log pipe");
  else if(rc) fatal(0, "git log exited with wstat %#x", rc);
}

/* --- main program -------------------------------------------------------- */

static void process_archive(const char *dir, int first) {
  int olddir;
  const char *colon;
  const char *branch = NULL;

  /* Switch to target directory */
  if((olddir = open(".", O_RDONLY, 0)) < 0)
    fatal(errno, "cannot open .");
  if((colon = strrchr(dir, ':'))) {
    /* Separate out base and branch */
    char *base = xstrndup(dir, colon - dir);
    branch = colon + 1;
    if(chdir(base) < 0) {
      /* Base directory does not exist, maybe it was really a directory with a
       * colon in */
      if(chdir(dir) < 0) fatal(errno, "cannot cd %s", dir);
      branch = NULL;
    } else {
      dir = base;
    }
  } else
    if(chdir(dir) < 0) fatal(errno, "cannot cd %s", dir);
  /* Try to guess what kind of revision control system we have.  First we look
   * at files, failing that we choose a default based on the name we were
   * invoked as, if even that doesn't work we default to bzr. */
  if(fexists(".bzr"))              process_bzr_archive(dir, branch, first);
  else if(fexists(".git"))         process_git_archive(dir, branch, first);
  else if(fexists("HEAD"))         process_git_archive(dir, branch, first);
  else if(strstr(progname, "bzr")) process_bzr_archive(dir, branch, first);
  else if(strstr(progname, "git")) process_git_archive(dir, branch, first);
  else                             process_bzr_archive(dir, branch, first);
  /* Switch back */
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
  
  progname = argv[0];
  /* Force timezone to GMT */
  setenv("TZ", "UTC", 1);
  while((n = getopt_long(argc, argv, "n:s:p:46dVa:M:x:hPf:D",
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
    case 'D':
      diffs = 1;
      break;
    case 'V':
      printf("bzr2news/git2news from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    case 'h':
      printf("Usage:\n\
  bzr2news -n GROUP [OPTIONS] DIRECTORY[:BRANCH]...\n\
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
  -D, --diff                         Show diffs\n\
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
