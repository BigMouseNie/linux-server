#include "epoller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory.h>

int Epoller::Add(int fd, uint32_t events, void* ev_data) {
  if (!IsValid() || fd < 0) return -1;

  if (is_et_) {
    SetNonBlock(fd);
    events |= EPOLLET;
  }

  struct epoll_event ev = {0};
  if (!ev_data)
    ev.data.fd = fd;
  else
    ev.data.ptr = ev_data;

  ev.events = events;
  int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
  if (ret < 0) return -1;
  return 0;
}

int Epoller::Modify(int fd, uint32_t events, void* ev_data) {
  if (!IsValid() || fd < 0) return -1;

  if (is_et_) events |= EPOLLET;

  struct epoll_event ev = {0};
  if (ev_data)
    ev.data.ptr = ev_data;
  else
    ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
}

int Epoller::Del(int fd) {
  if (!IsValid() || fd < 0) return -1;

  struct epoll_event ev = {0};
  return epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeout_ms, int* saved_errno) {
  if (!IsValid()) return -1;
  int ret = 0;
  do {
    ret = epoll_wait(epollfd_, event_arr_, event_arr_size_, timeout_ms);
  } while (ret == -1 && errno == EINTR);

  if (ret == -1) {
    if (saved_errno) *saved_errno = errno;
    return -1;
  }

  if (ret == 0) return 0;

  ev_cb_(event_arr_, ret);
  return ret;
}

int Epoller::SetNonBlock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
