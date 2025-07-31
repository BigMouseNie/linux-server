#ifndef NETWORK_SOCKETNBUFFER_H_
#define NETWORK_SOCKETNBUFFER_H_

#include "ring_buffer.h"

class SocketBuffer : public RingBuffer {
 public:
  SocketBuffer() = default;
  SocketBuffer(size_t size) { EnsureWritableSize(size); }
  ~SocketBuffer() = default;

  int ReadFromSock(int sock, bool is_et, int* saved_errno);
  int WriteToSock(int sock, bool is_et, int* saved_errno);

 private:
  static const size_t kMinBufSize;
};

#endif  // NETWORK_SOCKETNBUFFER_H_
