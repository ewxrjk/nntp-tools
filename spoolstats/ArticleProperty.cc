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
