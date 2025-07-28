#ifndef NETWORK_EPOLLER_H_
#define NETWORK_EPOLLER_H_

#include <stddef.h>
#include <stdint.h>

class Epoller {
 public:
  explicit Epoller();
  ~Epoller();
  Epoller(const Epoller&) = delete;
  Epoller& operator=(const Epoller&) = delete;

  int Create(size_t event_arr_size, bool is_et);
  int Add(int fd, uint32_t events);
  int Modify(int fd, uint32_t events);
  int Del(int fd);
  int Wait(int timeout_ms);

  int GetEvent(struct epoll_event* ev);
  int GetFd();

 private:
  int SetNonBlock(int fd);

 private:
  int epollfd_;
  bool is_et_;
  size_t event_arr_size_;
  struct epoll_event* event_arr_;
  size_t next_arr_size_;
  struct epoll_event* next_arr_;
};

#endif  // NETWORK_EPOLLER_H_
