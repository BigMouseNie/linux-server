#include "epoller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/epoll.h>
#include <unistd.h>

Epoller::Epoller()
    : event_arr_size_(0),
      is_et_(false),
      next_arr_(nullptr),
      next_arr_size_(0) {}

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

int Epoller::Create(size_t event_arr_size, bool is_et) {
  event_arr_size_ = event_arr_size;
  is_et_ = is_et;
  event_arr_ = new struct epoll_event[event_arr_size_];
  epollfd_ = epoll_create1(0);
  return epollfd_;
}

int Epoller::Add(int fd, uint32_t events) {
  if (fd < 0) {
    return -1;
  }
  if (is_et_) {
    SetNonBlock(fd);
    events |= EPOLLET;
  }

  struct epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
}

int Epoller::Modify(int fd, uint32_t events) {
  if (fd < 0) {
    return -1;
  }
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
  if (next_arr_size_ != 0) {
    return next_arr_size_;
  }

  int ret = epoll_wait(epollfd_, event_arr_, event_arr_size_, timeout_ms);
  if (ret == -1) {
    // 被信号打断，返回0什么也不做
    if (errno == EINTR) {
      return 0;
    }
    return -1;
  }

  next_arr_ = event_arr_;
  next_arr_size_ = ret;
  return ret;
}

int Epoller::GetEvent(struct epoll_event* ev) {
  if (next_arr_size_ == 0) {
    return -1;
  }

  memcpy(ev, next_arr_, sizeof(next_arr_[0]));
  ++next_arr_;
  --next_arr_size_;
  return 0;
}

int Epoller::GetFd() {
  if (next_arr_size_ == 0) {
    return -1;
  }

  int fd = next_arr_[0].data.fd;
  ++next_arr_;
  --next_arr_size_;
  return fd;
}

int Epoller::SetNonBlock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
