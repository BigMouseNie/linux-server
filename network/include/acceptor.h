#ifndef NETWORK_ACCEPTOR_H_
#define NETWORK_ACCEPTOR_H_

#include <functional>
#include <list>

#include "epoller.h"
#include "socket_wrapper.h"

class Acceptor {
 public:
  using AcceptCallBack = std::function<void(int conn_fd, struct sockaddr* addr,
                                            socklen_t addr_len)>;

  Acceptor();
  ~Acceptor();

  // 不创建Epoller,适合和其他非fd共用一个Epoller
  int Create(bool is_et);

  // 创建Epoller,适合主从reactor架构，Acceptor运行在单独一个线程
  int Create(int listenfd_num, bool is_et);
  int SetListenFd(int listenfd);
  int SetListenFd(int* listenfd_arr, size_t size);
  int SetListenFd(SocketWrapper&& sock);
  int Accept(int timeout_ms);
  // 以上函数搭配创建Epoller的Create使用(后续建议分开)

  void SetAcceptCallback(AcceptCallBack cb) { accept_cb_ = std::move(cb); }
  int DealConnFromSock(int sock);

 private:
  Epoller* epoller_;
  AcceptCallBack accept_cb_;
  bool is_et_;
  std::list<int> listenfd_lst_;
};

#endif  // NETWORK_ACCEPTOR_H_
