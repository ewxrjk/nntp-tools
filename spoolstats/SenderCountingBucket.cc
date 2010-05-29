#include "spoolstats.h"

using namespace std;

// Visit one article
void SenderCountingBucket::visit(const Article *a) {
  Bucket::visit(a);
  const string &sender = a->sender();
  map<string,int>::iterator it = senders.find(sender);
  if(it == senders.end())
    senders[sender] = 1;
  else
    ++it->second;
}

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
