//-*-C++-*-
/*
 * This file is part of rjk-nntp-tools.
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
#ifndef ARTICLEPROPERTY_H
#define ARTICLEPROPERTY_H

#include <map>
#include <set>
#include <string>
#include <vector>

class Article;

class ArticleProperty {
public:
  struct Value {
    const std::string value;
    long articles;
    std::set<std::string> senders;
    inline Value(const std::string &value_): value(value_), articles(0) {
    }
    Value &operator+=(const Value &that);
    struct ptr_art_compare {
      bool operator()(const Value *a, const Value *b) {
        return a->articles > b->articles;
      }
    };
  private:
    Value();                             // not default-constructable
  };

  ArticleProperty();
  ~ArticleProperty();

  void update(const Article *article,
              const std::string &value);

  void order(std::vector<const Value *> &) const;

  typedef const std::string &summarize_fn(const std::string &);

  void summarize(ArticleProperty &dest,
                 summarize_fn *summarize);

private:
  std::map<std::string,Value> values;
};

#endif /* ARTICLEPROPERTY_H */
