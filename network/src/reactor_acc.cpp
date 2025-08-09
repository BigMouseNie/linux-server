#include "reactor_acc.h"

#include <sys/epoll.h>
#include <sys/socket.h>

int ReactorAcc::Create(AcceptCallBack cb, int listenfd_num, bool is_et) {
  Epoller::EventsCallBack ev_cb = [this](struct epoll_event* evs, size_t size) {
    for (int i = 0; i < size; ++i) {
      int listenfd = evs[i].data.fd;
      if (evs[0].events & EPOLLERR) Remove(listenfd);
      int ret = DealConnFromSock(listenfd);
      if (ret < 0) Remove(listenfd);
    }
  };

  if (epoller_.Create(ev_cb, listenfd_num * 2, is_et) < 0) {
    return -1;
  }
  is_et_ = is_et;
  accept_cb_ = std::move(cb);
  return 0;
}

int ReactorAcc::SetListenSock(SocketWrapper&& sock) {
  if (epoller_.Add(sock.GetSocket(), EPOLLIN) < 0) {
    return -1;
  }
  sock.SetManualMgnt(false);
  listenfd_map_[sock.GetSocket()] = std::move(sock);
  return 0;
}

int ReactorAcc::Accept(int timeout_ms) {
  int ret = epoller_.Wait(timeout_ms);
  if (ret < 0) return -1;
  return 0;
}

void ReactorAcc::Remove(int listenfd) {
  auto it = listenfd_map_.find(listenfd);
  if (it == listenfd_map_.end()) return;
  epoller_.Del(listenfd);
  listenfd_map_.erase(it);
}
