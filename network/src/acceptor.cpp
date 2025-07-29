#include "acceptor.h"

#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Acceptor::Acceptor() : is_et_(false) {}

Acceptor::~Acceptor() {
  for (auto it = listenfd_lst_.begin(); it != listenfd_lst_.end(); ++it) {
    close(*it);
  }

  if (epoller_) {
    delete epoller_;
    epoller_ = nullptr;
  }
}

int Acceptor::Create(bool is_et) {
  is_et_ = is_et;
  return 0;
}

int Acceptor::Create(int listenfd_num, bool is_et) {
  epoller_ = new Epoller;
  if (epoller_->Create(listenfd_num * 2, is_et) < 0) {
    return -1;
  }

  is_et_ = is_et;
  return 0;
}

int Acceptor::SetListenFd(int listenfd) {
  if (epoller_->Add(listenfd, EPOLLIN) < 0) {
    return -1;
  }

  listenfd_lst_.push_back(listenfd);
  return 0;
}

int Acceptor::SetListenFd(int* listenfd_arr, size_t size) {
  for (int i = 0; i < size; ++i) {
    if (epoller_->Add(listenfd_arr[i], EPOLLIN) < 0) {
      return -1;
    }
    listenfd_lst_.push_back(listenfd_arr[i]);
  }

  return 0;
}

int Acceptor::SetListenFd(SocketWrapper&& sock) {
  return SetListenFd(sock.GetSocket());
}

int Acceptor::Accept(int timeout_ms) {
  int ret = epoller_->Wait(timeout_ms);
  if (ret < 0) return -1;
  if (ret == 0) return 0;

  int cnt = 0;
  while (true) {
    int listenfd = epoller_->GetFd();
    if (listenfd < 0) break;
    ret = DealConnFromSock(listenfd);
    if (ret < 0) return -2;
    cnt += ret;
  }
  return cnt;
}

int Acceptor::DealConnFromSock(int sock) {
  int cnt = 0;
  struct sockaddr_storage addr;
  socklen_t addr_len;
  do {
    addr_len = sizeof(addr);
    int conn_fd = accept(sock, (sockaddr*)&addr, &addr_len);
    if (conn_fd >= 0) {
      accept_cb_(conn_fd, (sockaddr*)&addr, addr_len);
      ++cnt;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {  // error log
        return -1;
      }
    }
  } while (is_et_);
  return cnt;
}
