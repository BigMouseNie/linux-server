#include "server.h"

#include "business.h"
#include "logger.h"

Server::Server() {}

Server::~Server() { Close(); }

Server& Server::Instance() {
  static Server kServer;
  static bool kConfigured = false;
  if (!kConfigured) {
    kServer.Config();
    kConfigured = true;
  }
  return kServer;
}

int Server::Start() {
  if (serv_sock_.Create(sock_cfg_) < 0) return -1;
  serv_sock_.SetManualMgnt(false);

  if (biz_->Start() < 0) return -2;

  auto acc_cb = [this](int conn_fd, struct sockaddr* addr, int addr_len) {
    biz_->SendConnection(conn_fd, addr, addr_len);
  };
  if (acceptor_.Create(acc_cb, sock_cfg_.GetBacklog(),
                       sock_cfg_.GetSockAttr() & kIsNonBlock) < 0) {
    return -3;
  }
  if (worker_.Init(&Server::Work, this) < 0) return -4;
  running_ = true;
  if (worker_.Run() < 0) return -5;

  LOG_INFO("server start success");
  return 0;
}

void Server::Config() {}

int Server::Close() {
  running_ = false;
  worker_.Stop();
}

void Server::Work() {
  while (running_) {
    if (acceptor_.Accept(100) < 0) {
      LOG_ERROR("server accept error");
      break;
    }
  }
}
