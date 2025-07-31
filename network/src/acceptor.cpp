#include "acceptor.h"

#include <errno.h>
#include <sys/socket.h>

int Acceptor::Create(AcceptCallBack cb, bool is_et) {
  is_et_ = is_et;
  accept_cb_ = std::move(cb);
  return 0;
}

int Acceptor::DealConnFromSock(int sock) {
  struct sockaddr_storage addr;
  socklen_t addr_len;
  do {
    addr_len = sizeof(addr);
    int conn_fd = accept(sock, (sockaddr*)&addr, &addr_len);
    if (conn_fd >= 0) {
      accept_cb_(conn_fd, (sockaddr*)&addr, addr_len);
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {  // error log
        return -1;
      }
    }
  } while (is_et_);
  return 0;
}
