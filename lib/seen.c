/*
 * This file is part of rjk-nntp-tools.
 * Copyright (C) 2011 Richard Kettlewell
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

#include "seen.h"
#include "utils.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

/* We track seen commits in a binary tree.  We don't attempt to balance it,
 * instead we rely on the commit IDs being uniformly distributed.
 *
 * The left and right subtree positions are store in bigendian format.
 */
struct node {
  int64_t left;                         /* left subtree position or 0 */
  int64_t right;                        /* right subtree position or 0 */
  char commit[48];                      /* commit id + 0 terminator */
};

/* Open tracking file */
static int fd = -1;

static uint64_t native_order(uint64_t n) {
#ifndef WORDS_BIGENDIAN
#if __amd64__ && __GNUC__
  __asm__("bswap %0" : "+q"(n));
  return n;
#else
  return ((uint64_t)htonl(n & 0xFFFFFFFF) << 32) | (htonl(n >> 32));
#endif
#endif
}

static uint64_t file_order(uint64_t n) {
#ifndef WORDS_BIGENDIAN
#if __amd64__ && __GNUC__
  __asm__("bswap %0" : "+q"(n));
  return n;
#else
  return ((uint64_t)ntohl(n & 0xFFFFFFFF) << 32) | (ntohl(n >> 32));
#endif
#endif
}

/* Read a node from the tracking file.  Returns 1 on success and 0 if there is
 * no such node. */
static int get(struct node *n, int64_t pos) {
  D(("get %#"PRIx64, pos));
  ssize_t bytes = pread(fd, n, sizeof *n, pos);
  if(bytes == 0)
    return 0;
  if(bytes < 0)
    fatal(errno, "reading tracking file");
  if(bytes != sizeof *n)
    fatal(0, "truncated tracking file");
  if(memchr(n->commit, 0, sizeof n->commit) == NULL)
    fatal(0, "corrupted tracking file");
  return 1;
}

int seen(const char *commit) {
  D(("seen %s", commit));
  /* Get the root node */
  struct node n[1];
  if(!get(n, 0))
    return 0;                           /* no root -> not seen anything */
  int c;
  while((c = strcmp(commit, n->commit))) {
    int64_t branch = native_order(c < 0 ? n->left : n->right);
    D(("%s %s %d %#"PRIx64, commit, n->commit, c, branch));
    if(!branch)
      return 0;                         /* fallen off the bottom */
    if(!get(n, branch))
      fatal(0, "missing tracking file node");
  }
  /* If we get here we have a match */
  return 1;
}

void remember(const char *commit) {
  D(("remember %s", commit));
  /* Fill out the node to insert */
  struct node newnode[1];
  memset(newnode, 0, sizeof *newnode);
  strcpy(newnode->commit, commit);
  /* Append it to the file */
  int64_t newnodepos = lseek(fd, 0L, SEEK_END);
  D(("newnodepos=%#"PRIx64, newnodepos));
  if(newnodepos < 0)
    fatal(errno, "seeking tracking file");
  if(newnodepos % sizeof newnode != 0)
    fatal(0, "tracking file has unsuitable length");
  ssize_t bytes = write(fd, newnode, sizeof newnode);
  if(bytes < 0)
    fatal(errno, "writing tracking file");
  if((size_t)bytes < sizeof newnode)
    fatal(0, "short write on tracking file");
  /* If we just wrote the root node then we're done */
  if(newnodepos == 0)
    return;
  /* Get the root node */
  int64_t parent = 0;
  struct node n[1];
  for(;;) {
    if(!get(n, parent))
      fatal(0, "missing branch");
    int c = strcmp(commit, n->commit);
    if(c == 0) {
      /* We've already seen this commit.  We shouldn't be called in this
       * situation!  The effect is that we waste a node; otherwise it's
       * harmless.  We could truncate the file back one node to fix this. */
      return;
    }
    int64_t *childp = c < 0 ? &n->left : &n->right;
    int64_t child = native_order(*childp);
    D(("%s %s %d %#"PRIx64, commit, n->commit, c, child));
    if(child) {
      parent = child;
      /* ...and we go round again */
    } else {
      *childp = file_order(newnodepos);
      bytes = pwrite(fd, n, sizeof *n, parent);
      if(bytes < 0)
        fatal(errno, "writing tracking file");
      if((size_t)bytes < sizeof *n)
        fatal(0, "short write on tracking file");
      return;
    }
  }
}

void init_seen(const char *path) {
  D(("init_seen %s", path));
  if(fd != -1) {
    close(fd);
    fd = -1;
  }
  if((fd = open(path, O_RDWR|O_CREAT, 0666)) < 0)
    fatal(errno, "opening %s", path);
}
