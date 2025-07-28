#ifndef NETWORK_ACCEPTOR_H_
#define NETWORK_ACCEPTOR_H_

#include <vector>

#include "epoller.h"
#include "socket_wrapper.h"

class Acceptor {
 public:
  explicit Acceptor();
  ~Acceptor();

  int Create(int listenfd_num, int backlog, bool is_et);
  int SetListenFd(int listenfd);
  int SetListenFd(int* listenfd_arr, size_t size);
  int SetListenFd(SocketWrapper&& sock);

  int Accept(int timeout_ms);
  int GetNewConnection(std::vector<int>& conns,
                       std::vector<struct sockaddr>& addresses,
                       std::vector<socklen_t>& socklens);
  int GetNewConnection(std::vector<int>& conns);

 private:
  int GetNewConnection(std::vector<int>& conns,
                       std::vector<struct sockaddr>& addresses,
                       std::vector<socklen_t>& socklens, bool flag);
  int Listen(int listenfd_);

 private:
  Epoller epoller_;
  bool is_et_;
  int backlog_;
};

#endif  // NETWORK_ACCEPTOR_H_
