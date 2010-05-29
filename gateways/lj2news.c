/*
 * Copyright (C) 2005, 2006, 2008, 2009 Richard Kettlewell
 * Copyright (C) 2008 Colin Watson
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
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <gcrypt.h>
#include <expat.h>
#include <curl/curl.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>
#include <ctype.h>

#include "utils.h"
#include "nntp.h"

#ifdef XML_UNICODE
# error ascii only please
#endif

/* --- globals ------------------------------------------------------------- */

static XML_Parser p;
static int item, subitem;
static struct element {
  struct element *next;
  char *name;
  char *value;
} *elements;
static const char *salt = "";
static CURL *curl;
static char cerrbuf[CURL_ERROR_SIZE];
static int urlpipe[2];
static const char *tag, *newsgroup, *organization, *sigfile;
static const char *msgiddomain = "tylerdurden.greenend.org.uk";
static const char *fromline;
static time_t start_date;

/* --- options ------------------------------------------------------------- */

const struct option options[] = {
  { "salt", required_argument, 0, 'x' },
  { "user-agent", required_argument, 0, 'a' },
  { "start-date", required_argument, 0, 'D' },
  { "tag", required_argument, 0, 't' },
  { "no-tag", no_argument, 0, 'T' },
  { "newsgroup", required_argument, 0, 'n' },
  { "signature", required_argument, 0, 'S' },
  { "organization", required_argument, 0, 'o' },
  { "msggid-domain", required_argument, 0, 'M' },
  { "server",  required_argument, 0, 's' },
  { "port", required_argument, 0,  'p' },
  { "debug", no_argument,  0, 'd' },
  { "from", required_argument, 0, 'f' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0 }
};

/* --- thread to write the <description> section to lynx's input pipe ------ */

struct writedescription_state {
  FILE *fp;
  const char *description;
};

static void *writedescriptionthread(void *arg) {
  int c;
  struct writedescription_state *wds = arg;

  D(("writedescriptionthread starting"));
  while((c = (unsigned char)*wds->description++))
    if(putc(c, wds->fp) < 0)
      fatal(errno, "error writing to pipe to lynx");
  if(fclose(wds->fp) < 0)
    fatal(errno, "error writing to pipe to lynx");
  D(("writedescriptionthread done"));
  return 0;
}

/* --- find a element by name ---------------------------------------------- */

static const char *find_element(const struct element *ehead,
                                const char *name) {
  const struct element *e;

  for(e = ehead; e && strcmp(e->name, name); e = e->next)
    ;
  return e ? e->value : 0;
}

static const char *need_element(const struct element *ehead,
                                const char *name) {
  const char *value;

  if(!(value = find_element(ehead, name)))
    fatal(0, "no <%s> found", name);
  return value;
}

/* --- process a parsed item ----------------------------------------------- */

/* We could do this in a thread, but there doesn't seem much point; XML parsing
 * is likely to be rather almost instant compared to running lynx, posting,
 * etc. */

static void process(const struct element *ehead) {
  const struct element *e;
  gcry_error_t gerr;
  gcry_md_hd_t hd;
  unsigned char *hash;
  char itemid[41];
  int n, w, c, err, nkeywords;
  FILE *fp, *output;
  int lynxinpipe[2], lynxoutpipe[2];
  pid_t lynxpid, r;
  const char *title, *date, *url, *guid, *dcdate;
  char *article = 0, *msgid;
  size_t articlesize = 0;
  pthread_t writedescription_id;
  struct writedescription_state wds[1];
  char buffer[4096], *s;

  /* compute itemid as the hash of the guid and the salt (-x option) */
  guid = need_element(ehead, "guid");
  D(("guid %s", guid));
  if((gerr = gcry_md_open(&hd, GCRY_MD_SHA1, 0)))
    fatal(0, "error calling gcry_md_open: %s/%s",
	  gcry_strsource(gerr), gcry_strerror(gerr));
  gcry_md_write(hd, salt, strlen(salt));
  gcry_md_write(hd, guid, strlen(guid));
  hash = gcry_md_read(hd, 0);
  for(n = 0; n < 20; ++n) sprintf(itemid + 2 * n, "%02x", hash[n]);
  gcry_md_close(hd);
  D(("id %s", itemid));
  if(asprintf(&msgid, "<%s@%s>", itemid, msgiddomain) < 0)
    fatal(errno, "error calling asprintf");
  /* calculate header values */
  if(!(title = find_element(ehead, "title"))) title = "untitled";
  if(!(date = find_element(ehead, "pubDate"))) {
    if((dcdate = find_element(ehead, "dc:date"))) {
      date = w3date_to_822date(dcdate);
    } else
      date = 0;
  }
  if(start_date) {
    if(!date) {
      D(("skipping dateless article"));
      return;
    }
    if(rfc822date_to_time_t(date) < start_date) {
      D(("\"%s\" too early", date));
      return;
    }
  }
  if(!(url = find_element(ehead, "link"))) url = 0;
  /* output the header */
  if(!(output = open_memstream(&article, &articlesize)))
    fatal(errno, "error calling open_memstream");
  if(fprintf(output, "Subject: ") < 0
     || (tag ? fprintf(output, "[%s] ", tag) < 0 : 0)
     || fprintf(output, "%s\n"
             "From: %s\n"
	     "Newsgroups: %s\n"
	     "MIME-Version: 1.0\n"
	     "Content-Type: text/plain; charset=UTF-8\n"
	     "Content-Transfer-Encoding: 8bit\n"
	     "Message-ID: %s\n",
	     title, fromline, newsgroup, msgid) < 0)
    fatal(errno, "error writing article");
  if(organization && fprintf(output, "Organization: %s\n",
			     organization) < 0)
    fatal(errno, "error writing article");
  if(date && fprintf(output, "Date: %s\n", date) < 0)
    fatal(errno, "error writing article");
  if(url && fprintf(output, "URL: %s\n", url) < 0)
    fatal(errno, "error writing article");
  nkeywords = 0;
  for(e = ehead; e; e = e->next)
    if(!strcmp(e->name, "category")) {
      if(!nkeywords) {
	if(fprintf(output, "Keywords: ") < 0)
	  fatal(errno, "error writing article");
      } else {
	if(fprintf(output, ",") < 0)
	  fatal(errno, "error writing article");
      }
      if(fprintf(output, "%s", e->value) < 0)
	fatal(errno, "error writing article");
      ++nkeywords;
    }
  if(nkeywords)
    if(fprintf(output, "\n") < 0)
      fatal(errno, "error writing article");
  /* End of header */
  if(fprintf(output, "\n") < 0)
    fatal(errno, "error writing article");
  /* use lynx to construct the body */
  if(pipe(lynxinpipe) < 0) fatal(errno, "error calling pipe");
  if(pipe(lynxoutpipe) < 0) fatal(errno, "error calling pipe");
  if(!(wds->fp = fdopen(lynxinpipe[1], "w")))
    fatal(errno, "eror calling fdopen");
  cloexec(lynxinpipe[1]);
  cloexec(lynxoutpipe[0]);
  if(!(lynxpid = fork())) {
    if(dup2(lynxinpipe[0], 0) < 0 || dup2(lynxoutpipe[1], 1) < 0)
      fatal(errno, "error calling dup2");
    close(lynxinpipe[0]);
    close(lynxoutpipe[1]);
    execlp("lynx", "lynx", "-force_html", "-dump", "-stdin",
           "-assume_charset=UTF-8", "-display_charset=UTF-8", (char *)0);
    fatal(errno, "error executing lynx");
  }
  close(lynxinpipe[0]);
  close(lynxoutpipe[1]);
  D(("started lynx"));
  /* we feed lynx from a thread made for the purpose */
  wds->description = need_element(ehead, "description");
  if((err = pthread_create(&writedescription_id, 0,
                           writedescriptionthread, wds)))
    fatal(err, "error calling pthread_create");
  /* Catch the output.  It seems that some versions of Lynx send an initial
   * blank line and others do not.  Therefore we eat any initial blank lines
   * Lynx sends and introduce our own single blank line (above). */
  D(("reading lynx output"));
  if(!(fp = fdopen(lynxoutpipe[0], "r")))
    fatal(errno, "error calling fdopen");
  while((s = fgets(buffer, sizeof buffer, fp)) != 0) {
    if(!strchr(buffer, '\n'))
      /* First line is ridiculously long */
      continue;
    /* See if there's any nonblank characters on this line */
    for(s = buffer; *s; ++s)
      if(!isspace((unsigned char)*s))
        break;
    /* If there are then we escape */
    if(*s)
      break;
    /* This is a leading blank line, so we ignore it. */
  }
  /* s can be NULL here if we ran out of input before finding a nonblank
   * line */
  if(s) {
    /* Send the first nonblank line (might hypothetically be an unreasonably
     * long blank line) */
    if(fputs(buffer, output) < 0)
      fatal(errno, "error writing article");
    /* Send everything else */
    while((c = getc(fp)) >= 0) {
      if(putc(c, output) < 0)
        fatal(errno, "error writing article");
    }
  }
  if(ferror(fp)) fatal(errno, "error reading from pipe from lynx");
  fclose(fp);
  /* tidy up */
  D(("tidying up after lynx"));
  if((err = pthread_join(writedescription_id, 0)))
    fatal(err, "error calling pthread_join");
  while((r = waitpid(lynxpid, &w, 0)) < 0 && errno == EINTR)
    ;
  if(r < 0) fatal(errno, "error calling waitpid");
  if(w) fatal(0, "lynx failed with status %#x", w);
  /* append the sigfile if there is any */
  if(sigfile) {
    D(("append sigfile"));
    if(fprintf(output, "-- \n") < 0)
      fatal(errno, "error writing article");
    if(!(fp = fopen(sigfile, "r")))
      fatal(errno, "error opening %s", sigfile);
    while((c = getc(fp)) >= 0)
      if(putc(c, output) < 0)
	fatal(errno, "error writing article");
    if(ferror(fp))
      fatal(errno, "error reading %s", sigfile);
    fclose(fp);
  }
  if(fclose(output) < 0) fatal(errno, "error writing article");
  /* pass the article to the posting thread */
  D(("notify poster"));
  post(msgid, article);
}

/* --- XML parser callbacks ------------------------------------------------ */

/* called at the start of an element */
static void XMLCALL start_element(void attribute((unused)) *userData,
				  const XML_Char *name,
				  const XML_Char attribute((unused))  **atts) {
  struct element *e;

  if(!strcmp(name, "item")) {
    ++item;
    elements = 0;
  } else if(item) {
    /* we'll record all the subelements of <item> */
    e = xmalloc(sizeof *e);
    e->next = elements;
    e->name = xstrdup(name);
    e->value = xstrdup("");
    elements = e;
    ++subitem;
  }
}

/* called at the end of an element */
static void XMLCALL end_element(void attribute((unused)) *userData,
				const XML_Char *name) {
  if(!strcmp(name, "item")) {
    --item;
    process(elements);
  } else if(item)
    --subitem;
}

/* called with character data */
static void XMLCALL character_data(void attribute((unused)) *userData,
				   const XML_Char *s,
				   int len) {
  size_t l;

  if(subitem) {
    l = strlen(elements->value);
    elements->value = xrealloc(elements->value, l + len + 1);
    memcpy(elements->value + l, s, len);
    elements->value[l + len] = 0;
  }
}

/* --- main RSS parser ----------------------------------------------------- */

/* parse some bytes */
static void parseabit(const char *path, char *buffer, size_t n, int eof) {
  if(XML_Parse(p, buffer, n, eof) == XML_STATUS_ERROR)
    fatal(0, "%s:%d:%d: parse error: %s",
	  path, XML_GetCurrentLineNumber(p),
	  XML_GetCurrentColumnNumber(p),
	  XML_ErrorString(XML_GetErrorCode(p)));
}

/* parse FP, using PATH in diagnostics */
static void parse(const char *path, FILE *fp) {
  char *line = 0;
  size_t linesize = 0;

  D(("parse %s", path));
  XML_ParserReset(p,  0);
  XML_SetElementHandler(p, start_element, end_element);
  XML_SetCharacterDataHandler(p, character_data);
  item = 0;
  while(getline(&line, &linesize, fp) != -1)
    parseabit(path, line, strlen(line), 0);
  if(!feof(fp)) fatal(errno, "error reading %s", path);
  parseabit(path, 0, 0, 1);
  free(line);
}

/* --- HTTP fetcher thread ------------------------------------------------- */

/* write bytes to the pipe back to the parser */
static size_t writedata(void *buffer,
			size_t n,
			size_t count,
			void attribute((unused)) *u) {
  int bytes;
  size_t total = n * count, written = 0;

  while(written < total) {
    bytes = write(urlpipe[1], (char *)buffer + written, total - written);
    if(bytes < 0) {
      if(errno != EINTR) fatal(errno, "error writing to internal pipe");
    } else
      written += bytes;
  }
  return written;
}

/* thread entry point - ARG is the URL to fetch */
static void *curlthread(void *arg) {
  const char *url = arg;
  CURLcode cerr;

  D(("curlthread %s", url));
  if((cerr = curl_easy_setopt(curl, CURLOPT_URL, url)))
    fatal(0, "curl_easy_setopt CURLOPT_URL: %s",
	  curl_easy_strerror(cerr));
  if((cerr = curl_easy_perform(curl)))
    fatal(0, "curl_easy_perform %s: %s: %s",
	  url, curl_easy_strerror(cerr), cerrbuf);
  D(("curlthread finishing"));
  close(urlpipe[1]);
  return 0;
}

/* --- main ---------------------------------------------------------------- */

int main(int argc, char **argv) {
  int n, err;
  FILE *fp;
  CURLcode cerr;
  pthread_t curlthread_id;
  const char *useragent = PACKAGE_NAME"/"PACKAGE_VERSION;
  const char *server = 0;
  const char *port = 0;
  int pf = PF_UNSPEC;

  /* Force timezone to GMT */
  setenv("TZ", "UTC", 1);
  /* Tag default to login name */
  if((tag = getenv("LOGNAME")))
    tag = xstrdup(tag);
  while((n = getopt_long(argc, argv, "x:a:D:t:n:S:o:M:s:p:46df:VT",
                         options, 0)) >= 0) {
    switch(n) {
    case 'x':
      salt = optarg;
      break;
    case 'a':
      useragent = optarg;
      break;
    case 'D':
      start_date = rfc822date_to_time_t(optarg);
      break;
    case 't':
      tag = optarg;
      break;
    case 'T':
      tag = 0;
      break;
    case 'n':
      newsgroup = optarg;
      break;
    case 'S':
      if(*optarg) sigfile = optarg;
      break;
    case 'o':
      if(*optarg) organization = optarg;
      break;
    case 'M':
      if(*optarg) msgiddomain = optarg;
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
    case 'f':
      if(*optarg) fromline = optarg;
      break;
    case 'V':
      printf("lj2news from rjk-nntp-tools version " VERSION "\n");
      exit(0);
    case 'h':
      printf("Usage:\n\
  lj2news -a USER-AGENT -f FROM -n GROUP [OPTIONS] URL\n\
\n\
Mandatory options:\n\
  -a, --user-agent USER-AGENT        HTTP user agent string\n\
  -f, --from FROM                    From line\n\
  -n, --newgroup GROUP               Newsgroup\n\
Optional options:\n\
  -t, --tag TAG                      Subject line tag (default $LOGNAME)\n\
  -T, --no-tag                       Suppress subject tag\n\
  -o, --organization ORGANIZATION    Organization line\n\
  -S, --signature PATH               Signature file\n\
  -s, --server HOSTNAME              NNTP server (default $NNTPSERVER)\n\
  -D, --start-date DATE              Ignore articles before DATE\n\
                (DATE takes the format 'Mon, 28 Apr 2008 00:00:00 GMT'.)\n\
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
  /* check command line */
  if(!newsgroup)
    fatal(0, "no -n option");
  if(!fromline)
    fatal(0, "no -f option");
  /* init curl */
  if(!(curl = curl_easy_init())) fatal(0, "curl_easy_init failed");
  if((cerr = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, cerrbuf)))
    fatal(0, "curl_easy_setopt CURLOPT_ERRORBUFFER: %s",
	  curl_easy_strerror(cerr));
  if((cerr = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writedata)))
    fatal(0, "curl_easy_setopt CURLOPT_WRITEFUNCTION: %s",
	  curl_easy_strerror(cerr));
  if((cerr = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L)))
    fatal(0, "curl_easy_setopt CURLOPT_NOSIGNAL: %s",
	  curl_easy_strerror(cerr));
  if((cerr = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)))
    fatal(0, "curl_easy_setopt CURLOPT_FOLLOWLOCATION: %s",
	  curl_easy_strerror(cerr));
  if((cerr = curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent)))
    fatal(0, "curl_easy_setopt CURLOPT_USERAGENT: %s",
	  curl_easy_strerror(cerr));
  /* nnrp posting will happen from a thread */
  create_postthread(pf, server, port);
  /* init expat */
  p = XML_ParserCreate(0);
  /* process URLs as requested */
  for(n = optind; n < argc; ++n) {
    if(pipe(urlpipe) < 0) fatal(errno, "error calling pipe");
    cloexec(urlpipe[0]);
    cloexec(urlpipe[1]);
    if((err = pthread_create(&curlthread_id, 0, curlthread, argv[n])))
      fatal(err, "error calling pthread_create");
    if(!(fp = fdopen(urlpipe[0], "r"))) fatal(errno, "error calling fdopen");
    parse(argv[n], fp);
    fclose(fp);
    if((err = pthread_join(curlthread_id, 0))) 
      fatal(err, "error calling pthread_join");
  }
  D(("main done"));
  /* clean up */
  join_postthread();
  XML_ParserFree(p);
  curl_easy_cleanup(curl);
  return 0;
}

/* --- elvis has left the building ----------------------------------------- */

/*
Local Variables:
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
