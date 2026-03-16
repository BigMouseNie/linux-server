#include "reactor_acc.h"

#include <sys/epoll.h>
#include <sys/socket.h>

ReactorAcc::ReactorAcc(AcceptCallBack cb, int listenfd_num, bool is_et)
    : Acceptor(std::move(cb), is_et) {
  Epoller::EventsCallBack ev_cb = [this](struct epoll_event* evs, size_t size) {
    for (size_t i = 0; i < size; ++i) {
      int listenfd = evs[i].data.fd;
      if (evs[i].events & EPOLLERR) Remove(listenfd);
      int ret = DealConnFromSock(listenfd);
      if (ret < 0) Remove(listenfd);
    }
  };
  epoller_ptr_ =
      std::make_unique<Epoller>(std::move(ev_cb), listenfd_num * 2, is_et);
}

int ReactorAcc::SetListenSock(ServerSocket&& sock) {
  if (sock.IsValid() == false || epoller_ptr_ == nullptr ||
      epoller_ptr_->IsValid() == false ||
      epoller_ptr_->Add(sock.GetFd(), EPOLLIN) < 0)
    return -1;

  listenfd_map_.emplace(sock.GetFd(), std::move(sock));
  return 0;
}

int ReactorAcc::Accept(int timeout_ms, int* saved_errno) {
  return epoller_ptr_->Wait(timeout_ms, saved_errno);
}

void ReactorAcc::Remove(int listenfd) {
  auto it = listenfd_map_.find(listenfd);
  if (it == listenfd_map_.end()) return;
  epoller_ptr_->Del(listenfd);
  listenfd_map_.erase(it);
}
