#include "epoller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>

Epoller::Epoller() : event_arr_size_(0), is_et_(true) {}

Epoller::~Epoller() {
  if (event_arr_) {
    delete[] event_arr_;
    event_arr_ = nullptr;
  }

  if (epollfd_ > 0) {
    close(epollfd_);
    epollfd_ = -1;
  }
}

int Epoller::Create(EventsCallBack cb, size_t event_arr_size, bool is_et) {
  event_arr_size_ = event_arr_size;
  is_et_ = is_et;
  event_arr_ = new struct epoll_event[event_arr_size_];
  epollfd_ = epoll_create1(0);
  ev_cb_ = std::move(cb);
  return epollfd_ < 0 ? -1 : 0;
}

int Epoller::Add(int fd, uint32_t events, void* ev_data) {
  if (fd < 0) return -1;

  if (is_et_) {
    SetNonBlock(fd);
    events |= EPOLLET;
  }

  struct epoll_event ev = {0};
  sizeof(ev.data);
  if (!ev_data)
    ev.data.fd = fd;
  else
    ev.data.ptr = ev_data;

  ev.events = events;
  int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
  if (ret < 0) {
    return -1;
  }
  return 0;
}

int Epoller::Modify(int fd, uint32_t events) {
  if (fd < 0) return -1;

  if (is_et_) {
    events |= EPOLLET;
  }

  struct epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
}

int Epoller::Del(int fd) {
  if (fd < 0) {
    return -1;
  }

  struct epoll_event ev = {0};
  return epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeout_ms) {
  int ret = epoll_wait(epollfd_, event_arr_, event_arr_size_, timeout_ms);
  if (ret == -1) {
    // 被信号打断，返回0什么也不做
    if (errno == EINTR) {
      return 0;
    }
    return -1;
  }

  ev_cb_(event_arr_, ret);
  return 0;
}

int Epoller::SetNonBlock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
