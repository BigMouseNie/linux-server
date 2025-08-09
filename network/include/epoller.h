#ifndef NETWORK_EPOLLER_H_
#define NETWORK_EPOLLER_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/epoll.h>

#include <functional>

class Epoller {
 public:
  using EventsCallBack =
      std::function<void(struct epoll_event* evs, size_t size)>;
  Epoller();
  ~Epoller();
  Epoller(const Epoller&) = delete;
  Epoller& operator=(const Epoller&) = delete;

  int Create(EventsCallBack cb, size_t event_arr_size, bool is_et);
  int Add(int fd, uint32_t events, void* ev_data = nullptr);
  int Modify(int fd, uint32_t events);
  int Del(int fd);
  int Wait(int timeout_ms);

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
