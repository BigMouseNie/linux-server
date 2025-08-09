#ifndef BUSINESS_BUSINESS_H_
#define BUSINESS_BUSINESS_H_

#include "process_wrapper.h"

class Business {
 public:
  Business() = default;
  ~Business() = default;
  virtual int Start() = 0;
  virtual int SendConnection(int fd, void* info, int info_len) = 0;

 protected:
  virtual int RecvConnection(int& fd, void* info, int* info_len) = 0;
};

#endif  // BUSINESS_BUSINESS_H_
