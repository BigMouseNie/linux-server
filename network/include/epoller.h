#ifndef NETWORK_EPOLLER_H_
#define NETWORK_EPOLLER_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <functional>

class Epoller {
 public:
  using EventsCallBack =
      std::function<void(struct epoll_event* evs, size_t size)>;

  Epoller(EventsCallBack cb, size_t event_arr_size, bool is_et = true)
      : is_et_(is_et),
        event_arr_size_(event_arr_size),
        event_arr_(new struct epoll_event[event_arr_size_]),
        epollfd_(epoll_create1(0)),
        ev_cb_(std::move(cb)) {}

  ~Epoller() {
    if (event_arr_) delete[] event_arr_;
    if (epollfd_ >= 0) close(epollfd_);
  }

  Epoller(const Epoller&) = delete;
  Epoller& operator=(const Epoller&) = delete;

  bool IsValid() const { return epollfd_ >= 0; }
  int Add(int fd, uint32_t events, void* ev_data = nullptr);
  int Modify(int fd, uint32_t events, void* ev_data = nullptr);
  int Del(int fd);
  int Wait(int timeout_ms, int* saved_errno = nullptr);

 private:
  int SetNonBlock(int fd);

 private:
  int epollfd_;
  bool is_et_;
  size_t event_arr_size_;
  struct epoll_event* event_arr_;
  EventsCallBack ev_cb_;
};

#endif  // NETWORK_EPOLLER_H_
