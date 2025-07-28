#include "acceptor.h"

#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>

Acceptor::Acceptor() : backlog_(0) {}

Acceptor::~Acceptor() {}

int Acceptor::Create(int listenfd_num, int backlog, bool is_et) {
  if (epoller_.Create(listenfd_num * 2, is_et) < 0) {
    return -1;
  }

  backlog_ = backlog;
  is_et_ = is_et;
  return 0;
}

int Acceptor::SetListenFd(int listenfd) {
  if (Listen(listenfd) < 0) {
    return -1;
  }
  if (epoller_.Add(listenfd, EPOLLIN) < 0) {
    return -2;
  }

  return 0;
}

int Acceptor::SetListenFd(int* listenfd_arr, size_t size) {
  for (int i = 0; i < size; ++i) {
    if (Listen(listenfd_arr[i]) < 0) {
      return -1;
    }
  }

  for (int i = 0; i < size; ++i) {
    if (epoller_.Add(listenfd_arr[i], EPOLLIN) < 0) {
      return -2;
    }
  }

  return 0;
}

int Acceptor::SetListenFd(SocketWrapper&& sock) {
  return SetListenFd(sock.GetSocket());
}

int Acceptor::Accept(int timeout_ms) { return epoller_.Wait(timeout_ms); }

int Acceptor::GetNewConnection(std::vector<int>& conns) {
  std::vector<struct sockaddr> addresses;
  std::vector<socklen_t> socklens;
  return GetNewConnection(conns, addresses, socklens, false);
}

int Acceptor::GetNewConnection(std::vector<int>& conns,
                               std::vector<struct sockaddr>& addresses,
                               std::vector<socklen_t>& socklens) {
  return GetNewConnection(conns, addresses, socklens, true);
}

int Acceptor::GetNewConnection(std::vector<int>& conns,
                               std::vector<struct sockaddr>& addresses,
                               std::vector<socklen_t>& socklens, bool flag) {
  struct sockaddr addr;
  socklen_t len;
  while (true) {
    int listenfd = epoller_.GetFd();
    if (listenfd < 0) {
      return 0;
    }
    do {
      int connfd = accept(listenfd, &addr, &len);
      if (connfd >= 0) {
        conns.push_back(connfd);
        if (flag) {
          addresses.push_back(addr);
          socklens.push_back(len);
        }
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          break;
        } else {
          // error log
          break;
        }
      }
    } while (is_et_);
  }
  return 0;
}

int Acceptor::Listen(int listenfd_) {
  if (listen(listenfd_, backlog_) < 0) {
    return -1;
  }
  return 0;
}
