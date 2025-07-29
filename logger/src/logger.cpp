#include "logger.h"

#include <sys/epoll.h>

#include <memory>

#include "string.h"

int Logger::Create(SocketWrapper&& sock_serv, char* save_path, bool is_et) {
  if (!sock_serv.isValid()) {
    return -1;
  }
  sock_serv_ = std::move(sock_serv);
  sock_serv_.SetManualMgnt(true);

  strcpy(path_, save_path);

  int ret = worker_.Init(&Logger::Worker, this);
  if (ret != 0) return -2;
  ret = worker_.Run();
  if (ret != 0) return -3;
  acceptor_.Create(is_et);
  return 0;
}

int Logger::Worker() {
  int ret = epoller_.Add(sock_serv_.GetSocket(), EPOLLET | EPOLLIN | EPOLLERR);
  if (ret != 0) return -1;

  while (true) {
    struct epoll_event ev;
    ret = epoller_.Wait(100);
    if (ret < 0) break;
    while (epoller_.GetEvent(&ev) != 0) {
      int fd = ev.data.fd;
      if (fd == sock_serv_.GetSocket()) {  // accept

      } else {  // read
      }
    }
  }
}
