/*
 * This file is part of spoolstats.
 * Copyright (C) 2010 Richard Kettlewell
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
#ifndef BUCKET_H
#define BUCKET_H

class Article;

class Bucket {
public:
  int articles;                 // count of articles
  intmax_t bytes;               // count of bytes

  inline Bucket(): articles(0),
                   bytes(0) {
  }

  virtual ~Bucket();

  // Supply an article to this bucket
  inline void visit(const Article *a) {
    ++articles;
    bytes += a->get_size();
  }
};

#endif /* BUCKET_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
