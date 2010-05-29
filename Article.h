#ifndef ARTICLE_H
#define ARTICLE_H

#include <map>
#include <string>
#include <vector>

class Article {
public:
  static std::map<std::string,Article *> articles; // mid -> article

  Article(const std::string &text, size_t bytes_);
  static int visit(const std::string &text,
                   size_t bytes_,
                   time_t start_time,
                   time_t end_time);

  void get_groups(std::vector<std::string> &groups) const;
  time_t date() const;

  inline const std::string &mid() const {
    return headers.find("message-id")->second;
  }

  inline size_t get_size() const {
    return bytes;
  }

  inline const std::string &sender() const {
    return headers.find("from")->second;
  }

private:
  void parse(const std::string &text);
  static int eol(const std::string &text, std::string::size_type pos);
  static int eoh(const std::string &text, std::string::size_type pos);
  static std::string &lower(std::string &s);
  static std::string &upper(std::string &s);
  static time_t parse_date(const std::string &d);
  static bool parse_date_std(const std::string &d, struct tm &bdt, int &zone);
  static void skip_ws(const std::string &s, std::string::size_type &pos);
  static bool parse_word(const std::string &s, std::string::size_type &pos, std::string &word);
  static bool parse_int(const std::string &s, std::string::size_type &pos, int &n);

  size_t bytes;
  std::map<std::string, std::string> headers;
  mutable time_t cached_date;

  static std::map<std::string, bool> seen;
};

#endif /* ARTICLE_H */


/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
