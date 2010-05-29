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
