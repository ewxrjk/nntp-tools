/*
 * Copyright (C) 2005, 2006, 2010 Richard Kettlewell
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

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>

#include "utils.h"
#include "nntp.h"

/* --- NNTP poster thread -------------------------------------------------- */

/* A queued article to post.  Jobs are kept in a linked list and processed in
 * order. */
static struct postjob {
  struct postjob *next;
  char *msgid;
  char *article;
} *postjobs;

/* Set when the last postjob is queued */
static int postdone;

/* Lock protecting postjobs */
static pthread_mutex_t postlock = PTHREAD_MUTEX_INITIALIZER;

/* condvar signaled when postjobs or postdone are modified */
static pthread_cond_t postcond = PTHREAD_COND_INITIALIZER;

/* ID of the posting thread */
static pthread_t postthread_id;

/* Count of unwanted articles */
static int unwanted;

/* Protocol family override */
static int pf;

/* Server hostname/port */
static const char *server, *port;

/* Internal state of the posting thread */
struct postthreadstate {
  FILE *fpin, *fpout;                   /* input from and output to NNRPD */
  char *line;                           /* latest response line */
  size_t n;                             /* buffer size for getline(3) */
};

/* Fetch an NNTP response.  Returns the 3-digit status as an int. */
static int pts_response(struct postthreadstate *pts) {
  char *s;

  if(getline(&pts->line, &pts->n, pts->fpin) == -1) {
    if(ferror(pts->fpin))
      fatal(errno, "error reading from news server");
    else
      fatal(0, "unexpected EOF reading from news server");
  }
  s = strrchr(pts->line, '\r');
  if(!s || s[1] != '\n' || s[2] != 0)
    fatal(0, "line read from news server does not end CRLF");
  *s = 0;
  D(("nntp: %s", pts->line));
  if(!(*pts->line >= '0' && *pts->line <= '9')
     || !(pts->line[1] >= '0' && pts->line[1] <= '9')
     || !(pts->line[2] >= '0' && pts->line[2] <= '9')
     || (pts->line[3] >= '0' && pts->line[3] <= '9'))
    fatal(0, "line read from news server does not start with response code");
  return pts->line[0] * 100 + pts->line[1] * 10 + pts->line[2] - 111 * '0';
}

/* Check the startup banner and terminate if it is unsuitable */
static void pts_banner(struct postthreadstate *pts) {
  switch(pts_response(pts)) {
  case 200: break;
  case 201: fatal(0, "news server does not support posting: %s", pts->line);
  default: fatal(0, "unknown response from news server: %s", pts->line);
  }
}

/* Write to the NNRPD, printf-style.  The trailing CRLF must be included in
 * FMT. */
static void pts_write(struct postthreadstate *pts, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  if(vfprintf(pts->fpout, fmt, ap) < 0
     || fflush(pts->fpout) < 0)
    fatal(errno, "error writing to news server");
  va_end(ap);
  if(debug) {
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
  }
}

/* Write a byte to the NNRPD */
static void pts_putc(struct postthreadstate *pts, int c) {
  if(putc(c, pts->fpout) < 0)
    fatal(errno, "error writing to news server");
  if(debug)
    putc(c, stderr);
}

/* Do NNTP authentication.  Only AUTHINFO GENERIC is supported currently. */
static void pts_auth(struct postthreadstate *pts) {
  const char *nntpauth, *nntp_auth_fds;
  int cookiefd = -1;
  FILE *cookiefp;
  pid_t helperpid, r;
  int w;
  char *new_nntp_auth_fds;

  D(("pts_auth"));
  if(!(nntpauth = getenv("NNTPAUTH"))) fatal(0, "NNTPAUTH not set");
  nntpauth = xstrdup(nntpauth);
  if((nntp_auth_fds = getenv("NNTP_AUTH_FDS")))
    sscanf(nntp_auth_fds, "%*u.%*u.%d", &cookiefd);
  if(cookiefd == -1) {
    if(!(cookiefp = tmpfile())) fatal(errno, "error calling tmpfile");
    cookiefd = fileno(cookiefp);
  }
  pts_write(pts, "AUTHINFO GENERIC %s\r\n", nntpauth);
  D(("starting helper"));
  if(!(helperpid = xfork())) {
    if(asprintf(&new_nntp_auth_fds, "NNTP_AUTH_FDS=%d.%d.%d",
		fileno(pts->fpin), fileno(pts->fpout), cookiefd) < 0)
      fatal(errno, "error calling asprintf");
    if(putenv(new_nntp_auth_fds))
      fatal(errno, "error calling putenv");
    execlp("sh", "sh", "-c", nntpauth, (char *)0);
    fatal(errno, "error invoking sh");
  }
  D(("waiting for helper"));
  while((r = waitpid(helperpid, &w, 0)) < 0 && errno == EINTR)
    ;
  if(r < 0) fatal(errno, "error calling waitpid");
  if(w) fatal(0, "NNTP AUTHINFO GENERIC helper exited with status %#x", w);
  D(("helper finished"));
}

/* post ARTICLE */
static void pts_post(struct postthreadstate *pts, const char *article) {
  int r, sol = 1, c;
  const char *s;

  pts_write(pts, "POST\r\n");
  if((r = pts_response(pts)) == 480) {
    pts_auth(pts);
    pts_post(pts, article);
    return;
  }
  if(r != 340) fatal(0, "cannot post: %s", pts->line);
  s = article;
  while((c = (unsigned char)*s++)) {
    switch(c) {
    case '.':
      if(sol) pts_putc(pts, '.');
      /* fall thru */
    default:
      pts_putc(pts, c);
      sol = 0;
      break;
    case '\n':
      pts_write(pts, "\r\n");
      sol = 1;
      break;
    }
  }
  if(!sol) pts_write(pts, "\r\n");
  pts_write(pts, ".\r\n");
  if(fflush(pts->fpout) < 0) fatal(errno, "error writing to news server");
  switch(pts_response(pts)) {
  case 240: break;
  case 441:
    /* 441 XYZ is an INN convention - or at least happenstance - but there
     * doesn't seem to be a standard way to convey this.  nntp-merge screws it
     * up by prefixing its own report, but the string from INN is still
     * there. */
    if(!strstr(pts->line, "441 437")
       || !strstr(pts->line, "441 435"))
      ++unwanted;
    else
      error(0, "article rejected: %s", pts->line);
    break;
  case 480:
    pts_auth(pts);
    pts_post(pts, article);
    return;
  }
}

/* STAT article.  Return the NNTP response code (223 if it exists) */
static int pts_stat(struct postthreadstate *pts, const char *msgid) {
  int r;

  pts_write(pts, "STAT %s\r\n", msgid);
  switch(r = pts_response(pts)) {
  case 223:
  case 423:
  case 430:
    break;
  case 480:
    pts_auth(pts);
    return pts_stat(pts, msgid);
  default:
    error(0, "STAT %s failed: %s", msgid, pts->line);
    break;
  }
  return r;
}

/* Main thread entry point */
static void *postthread(void attribute((unused)) *arg) {
  int fd = -1, fd2, err;
  struct addrinfo hints, *res = NULL, *ans;
  struct postthreadstate pts[1];

  D(("postthread"));
  pts->line = 0;
  pts->n = 0;
  /* look up the server */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = pf;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_CANONNAME;
  if((err = getaddrinfo(server, port, &hints, &res)))
    fatal(0, "error resolving node %s service %s: %s", server, port,
          gai_strerror(err));
  /* wait for some work to arrive */
  lock(&postlock);
  while(postjobs || !postdone) {
    D(("postthread loop"));
    if(postjobs) {
      D(("jobs found"));
      if(fd == -1) {
        D(("connect to news swerver"));
        /* unlock the queue while we're connecting */
        unlock(&postlock);
	/* connect to it */
	for(ans = res; ans && fd == -1; ans = ans->ai_next) {
	  if((fd = socket(ans->ai_family, ans->ai_socktype,
			  ans->ai_protocol)) < 0)
	    fatal(errno, "error calling socket");
	  if(connect(fd, ans->ai_addr, ans->ai_addrlen) < 0) {
	    error(errno, "error connecting to %s", ans->ai_canonname);
            close(fd);
            fd = -1;
          }
	}
	if(fd == -1)
	  fatal(0, "cannot connect to node %s service %s", server, port);
	if((fd2 = dup(fd)) < 0) fatal(errno, "error calling dup");
	if(!(pts->fpin = fdopen(fd, "r"))
	   || !(pts->fpout = fdopen(fd2, "w")))
	  fatal(errno, "error calling fdopen");
	/* initial banner */
	pts_banner(pts);
	/* make sure we are talking to an NNRP server */
	pts_write(pts, "MODE READER\r\n");
	pts_banner(pts);
        lock(&postlock);
      }
      /* do any work that is now available */
      while(postjobs) {
        struct postjob *pj = postjobs;
        /* TODO actually we could probably unlock here */
        D(("posting..."));
        if(pts_stat(pts, pj->msgid) != 223)
          pts_post(pts, pj->article);
	postjobs = postjobs->next;
        free(pj->msgid);
        free(pj->article);
        free(pj);
      }
      /* postdone might have been set during the period we released the lock
       * above.  If so, then postjobs must now be empty, because of the loop we
       * just went through.  So the correct reaction is to exit the loop. */
      if(postdone)
        break;
    }
    D(("postthread cond wait"));
    if((err = pthread_cond_wait(&postcond, &postlock)))
      fatal(err, "error calling pthread_cond_wait");
  }
  D(("postthread done"));
  unlock(&postlock);
  if(fd != -1) {
    fclose(pts->fpin);
    fclose(pts->fpout);
  }
  if(res)
    freeaddrinfo(res);
  free(pts->line);
  return 0;
}

/* Create the posting thread. */
void create_postthread(int pf_, const char *server_, const char *port_) {
  int err;

  D(("create_postthread"));
  pf = pf_;
  if(!(server = server_)) {
    if((server = getenv("NNTPSERVER")))
      server = xstrdup(server);
    else
      server = "news";
  }
  if(!(port = port_))
    port = "nntp";
  if((err = pthread_create(&postthread_id, 0, postthread, 0)))
    fatal(err, "error calling pthread_create");
}

/* Wait for the posting thread to complete */
void join_postthread(void) {
  int err;

  D(("join_postthread"));
  lock(&postlock);
  D(("main taken post lock"));
  postdone = 1;
  if((err = pthread_cond_signal(&postcond)))
    fatal(err, "error calling pthread_cond_signal");
  D(("main signalled poster"));
  unlock(&postlock);
  if((err = pthread_join(postthread_id, 0)))
    fatal(err, "error calling pthread_join");
  D(("%d articles unwanted", unwanted));
}

/* Queue an article for posting */
void post(const char *msgid, const char *article) {
  struct postjob *pj = xmalloc(sizeof *pj), **pjj;
  int err;

  D(("post %s", msgid));
  lock(&postlock);
  D(("processor taken post lock"));
  pjj = &postjobs;
  while(*pjj)
    pjj = &(*pjj)->next;
  pj->next = NULL;
  pj->msgid = xstrdup(msgid);
  pj->article = xstrdup(article);
  *pjj = pj;
  if((err = pthread_cond_signal(&postcond)))
    fatal(err, "error calling pthread_cond_signal");
  D(("processor signalled poster"));
  unlock(&postlock);
}
