#ifndef PLAYBACKNODE_SERVER_H_
#define PLAYBACKNODE_SERVER_H_

#include <memory>

#include "epoller.h"
#include "reactor_acc.h"
#include "socket_wrapper.h"

class Business;
class Server {
 public:
  static Server& Instance();
  int Start();
  int Close();

 private:
  Server() : running_(false) {};
  ~Server();
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;
  void Config();
  void Work();

 private:
  SocketCfg sock_cfg_;
  SocketWrapper serv_sock_;
  ReactorAcc acceptor_;
  bool running_;
  ThreadWrapper worker_;
  std::unique_ptr<Business> biz_;
};

#endif  // PLAYBACKNODE_SERVER_H_
