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

ArticleProperty::ArticleProperty() {}

ArticleProperty::~ArticleProperty() {}

void ArticleProperty::update(const Article *article, const string &value) {
  const string &sender = article->sender();
  map<string, PropertyValue>::iterator it = values.find(value);
  if(it == values.end())
    it = values.insert(pair<string, PropertyValue>(value, PropertyValue(value)))
             .first;
  ++it->second.articles;
  it->second.addSender(sender);
}

void ArticleProperty::summarize(ArticleProperty &dest,
                                summarize_fn *summarizer) {
  for(map<string, PropertyValue>::const_iterator it = values.begin();
      it != values.end(); ++it) {
    const string sname = (*summarizer)(it->first);
    map<string, PropertyValue>::iterator jt = dest.values.find(sname);
    if(jt == dest.values.end())
      jt = dest.values
               .insert(pair<string, PropertyValue>(sname, PropertyValue(sname)))
               .first;
    jt->second += it->second;
  }
}

void ArticleProperty::order(std::vector<const PropertyValue *> &ordered) const {
  for(map<string, PropertyValue>::const_iterator it = values.begin();
      it != values.end(); ++it)
    ordered.push_back(&it->second);
  sort(ordered.begin(), ordered.end(), PropertyValue::ptr_art_compare());
}

ArticleProperty::PropertyValue &ArticleProperty::PropertyValue::
operator+=(const PropertyValue &that) {
  articles += that.articles;
  for(set<string>::const_iterator it = that.senders.begin();
      it != that.senders.end(); ++it)
    senders.insert(*it);
  senderCount += that.senderCount;
  return *this;
}

void ArticleProperty::logs(const string &path) {
  try {
    ofstream os(path.c_str(), ios::trunc);
    os.exceptions(ofstream::badbit | ofstream::failbit);
    for(map<string, PropertyValue>::const_iterator it = values.begin();
        it != values.end(); ++it) {
      const PropertyValue &v = it->second;
      os << csv_quote(v.value) << ',' << v.articles << ',' << v.senderCount
         << '\n';
    }
    os << flush;
  } catch(ios::failure &) {
    fatal(errno, "writing to %s", path.c_str());
  }
}

void ArticleProperty::readLogs(const string &path) {
  vector<vector<Value>> rows;
  read_csv(path, rows);
  for(size_t n = 0; n < rows.size(); ++n) {
    const vector<Value> &row = rows.at(n);
    PropertyValue v(row[0]);
    v.articles = row[1];
    v.senderCount = row[2];
    values.insert(pair<string, PropertyValue>(row[0], v));
  }
}
