#include "socket_buffer.h"

#include <errno.h>
#include <unistd.h>

#include "socket_wrapper.h"

const size_t SocketBuffer::kMinBufSize = 256;

int SocketBuffer::ReadFromSock(int sock, bool is_et, int* saved_errno) {
  int res = 0;
  bool err_interr = false;  // 是否信号中断
  do {
    err_interr = false;
    EnsureWritableSize(kMinBufSize);
    int n = read(sock, GetWritePtr(), Writable());
    if (n > 0) {
      res += n;
      Written(n);
    } else if (n == 0) {
      break;
    } else {
      if (errno == EINTR)
        err_interr = true;
      else if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      else {
        *saved_errno = errno;
        res = -1;
        break;
      }
    }
  } while (err_interr || is_et);
  return res;
}

int SocketBuffer::WriteToSock(int sock, bool is_et, int* saved_errno) {
  int res = 0;
  bool err_interr = false;
  do {
    if (Readable() == 0) break;
    err_interr = false;
    int n = write(sock, GetReadPtr(), Readable());
    if (n > 0) {
      res += n;
      ReadOut(n);
    } else {
      if (errno == EINTR)
        err_interr = true;
      else if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      else {
        *saved_errno = errno;
        res = -1;
        break;
      }
    }
  } while (err_interr || is_et);
  return res;
}

int SocketBuffer::ReadFromSock(SocketWrapper& sock, int* saved_errno) {
  if (!sock.IsValid()) return -1;
  return ReadFromSock(sock.GetSocket(), sock.IsNonBlock(), saved_errno);
}

int SocketBuffer::WriteToSock(SocketWrapper& sock, int* saved_errno) {
  if (!sock.IsValid()) return -1;
  return WriteToSock(sock.GetSocket(), sock.IsNonBlock(), saved_errno);
}
