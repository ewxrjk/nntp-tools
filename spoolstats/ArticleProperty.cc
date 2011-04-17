/*
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
#include "spoolstats.h"

using namespace std;

ArticleProperty::ArticleProperty() {
}

ArticleProperty::~ArticleProperty() {
}

void ArticleProperty::update(const Article *article,
                             const string &value) {
  const string &sender = article->sender();
  map<string,Value>::iterator it = values.find(value);
  if(it == values.end())
    it = values.insert(pair<string,Value>(value,Value(value))).first;
  ++it->second.articles;
  it->second.senders.insert(sender);
}

void ArticleProperty::summarize(ArticleProperty &dest,
                                summarize_fn *summarizer) {
  for(map<string,Value>::const_iterator it = values.begin();
      it != values.end();
      ++it) {
    const string sname = (*summarizer)(it->first);
    map<string,Value>::iterator jt = dest.values.find(sname);
    if(jt == dest.values.end())
      jt = dest.values.insert(pair<string,Value>(sname,Value(sname))).first;
    jt->second += it->second;
  }
}

void ArticleProperty::order(std::vector<const Value *> &ordered) const {
  for(map<string,Value>::const_iterator it = values.begin();
      it != values.end();
      ++it)
    ordered.push_back(&it->second);
  sort(ordered.begin(), ordered.end(), Value::ptr_art_compare());
}

ArticleProperty::Value &ArticleProperty::Value::operator+=(const Value &that) {
  articles += that.articles;
  for(set<string>::const_iterator it = that.senders.begin();
      it != that.senders.end();
      ++it)
    senders.insert(*it);
  return *this;
}
