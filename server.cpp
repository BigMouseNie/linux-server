#include "server.h"

#include <fstream>
#include <json.hpp>
#include <sstream>

#include "business.h"
#include "logger.h"

using json = nlohmann::json;

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
  SocketWrapper serv_sock;
  if (serv_sock.Create(sock_cfg_) < 0) return -1;
  serv_sock.SetManualMgnt(false);

  if (biz_->Start() < 0) return -2;

  auto acc_cb = [this](int conn_fd, struct sockaddr* addr, int addr_len) {
    biz_->SendConnection(conn_fd, addr, addr_len);
  };
  if (acceptor_.Create(acc_cb, sock_cfg_.GetBacklog(),
                       sock_cfg_.GetSockAttr() & kIsNonBlock) < 0) {
    return -3;
  }
  acceptor_.SetListenSock(std::move(serv_sock));
  if (worker_.Init(&Server::Work, this) < 0) return -4;
  running_ = true;
  if (worker_.Run() < 0) return -5;

  LOG_INFO("server start success");
  return 0;
}

int Server::Config() {
  const char* cfg_path =
      "/home/charstar/projects/playback-node/server_config.json";
  std::ifstream file(cfg_path);
  if (!file.is_open()) return -1;
  std::stringstream buffer;
  buffer << file.rdbuf();
  json j = json::parse(buffer.str(), nullptr, /* allow_exceptions = */ false);
  if (j.is_discarded()) return -2;

  std::string domain = j.value("domain", "INET");
  std::string type = j.value("type", "STREAM");
  int port = j.value("port", 8081);
  int backlog = j.value("backlog", 5);
  bool edge_trigger = j.value("edge_trigger", true);

  int sock_attr = 0;
  if (domain == "LOCAL") sock_attr |= kIsLocal;
  if (domain == "INET6") sock_attr |= kIsIPV6;
  if (type == "DGRAM") sock_attr |= kIsUDP;
  if (edge_trigger == true) sock_attr |= kIsNonBlock;

  sock_attr |= kIsListen;
  sock_cfg_.SetSockAttr(sock_attr);
  sock_cfg_.SetPort(port);
  sock_cfg_.SetBacklog(backlog);
  return 0;
}

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
