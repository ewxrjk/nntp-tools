#ifndef SENDERCOUNTINGBUCKET_H
#define SENDERCOUNTINGBUCKET_H

class Article;

class SenderCountingBucket: public Bucket {
public:
  std::map<std::string,int> senders;    // sender -> article count

  // Visit one article
  void visit(const Article *a);

};


#endif /* SENDERCOUNTINGBUCKET_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
