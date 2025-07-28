#include "socket_wrapper.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

SocketWrapper::SocketWrapper(bool manual_mgnt)
    : sockfd_(-1), manual_mgnt_(manual_mgnt) {}

SocketWrapper::SocketWrapper(SocketWrapper&& other)
    : sock_attr_(other.sock_attr_),
      sockfd_(other.sockfd_),
      manual_mgnt_(other.manual_mgnt_) {
  memcpy(&addr_, &(other.addr_), sizeof(addr_));
  other.sock_attr_ = 0;
  other.sockfd_ = -1;
  memset(&(other.addr_), 0, sizeof(other.addr_));
}

SocketWrapper& SocketWrapper::operator=(SocketWrapper&& other) {
  if (this == &other) {
    return *this;
  }

  sock_attr_ = other.sock_attr_;
  sockfd_ = other.sockfd_;
  manual_mgnt_ = other.manual_mgnt_;
  memcpy(&addr_, &(other.addr_), sizeof(addr_));

  other.sock_attr_ = 0;
  other.sockfd_ = -1;
  memset(&(other.addr_), 0, sizeof(other.addr_));
}

SocketWrapper::~SocketWrapper() {
  if (!manual_mgnt_) {
    Close();
  }
}

int SocketWrapper::Create(const SocketCfg& sock_cfg) {
  int ret = CreateSocket(sock_cfg);
  if (ret < 0) {
    return -1;
  }
  sockfd_ = ret;

  ret = CreateAddress(sock_cfg);
  if (ret < 0) {
    Close();
    return -2;
  }

  ret = Bind(sock_cfg);
  if (ret < 0) {
    Close();
    return -3;
  }

  /*
  ret = Listen(sock_cfg);
  if (ret < 0) {
    Close();
    return -4;
  }
  */

  sock_attr_ = sock_cfg.sock_attr;
  sock_attr_ |= kIsCreate;  // 创建成功
  return 0;
}

int SocketWrapper::Recv() { return 0; }

int SocketWrapper::Send() { return 0; }

void SocketWrapper::Close() {
  if (sockfd_ != -1) {
    close(sockfd_);
    sockfd_ = -1;
  }
}

int SocketWrapper::CreateSocket(const SocketCfg& sock_cfg) {
  int domain = PF_INET;
  if (sock_cfg.sock_attr & kIsLocal) {
    domain = PF_LOCAL;
  } else if (sock_cfg.sock_attr & kIsIPV6) {
    domain = PF_INET6;
  }
  int type = SOCK_STREAM;
  if (sock_cfg.sock_attr & kIsUDP) {
    SOCK_DGRAM;
  }
  int ret = socket(domain, type, 0);
  if (ret < 0) {
    return -1;
  }

  if (sock_cfg.sock_attr & kIsNonBlock) {
    ret = SetNonBlock();
  }
  if (ret < 0) {
    Close();
    return -2;
  }
  return 0;
}

int SocketWrapper::CreateAddress(const SocketCfg& sock_cfg) {
  memset(&addr_, 0, sizeof(addr_));
  // local socket
  if (sock_cfg.sock_attr & kIsLocal) {
    struct sockaddr_un* un_addr = (struct sockaddr_un*)(&addr_);
    if (sock_cfg.sock_attr & kIsListen) {
      unlink(sock_cfg.address);
    }
    un_addr->sun_family = AF_LOCAL;
    memcpy(un_addr->sun_path, sock_cfg.address, sock_cfg.address_size - 1);
    return 0;
  }

  // IPV6
  if (sock_cfg.sock_attr & kIsIPV6) {
    struct sockaddr_in6* in6_addr = (struct sockaddr_in6*)(&addr_);
    in6_addr->sin6_family = AF_INET6;
    in6_addr->sin6_port = htons(sock_cfg.port);
    if (sock_cfg.sock_attr & kIsListen) {
      in6_addr->sin6_addr = in6addr_any;
    } else {
      if (inet_pton(AF_INET6, sock_cfg.address, &(in6_addr->sin6_addr)) < 0) {
        return -1;
      }
    }
    return 0;
  }

  // IPV4
  struct sockaddr_in* in_addr = (struct sockaddr_in*)(&addr_);
  in_addr->sin_family = AF_INET;
  in_addr->sin_port = htons(sock_cfg.port);
  if (sock_cfg.sock_attr & kIsListen) {
    in_addr->sin_addr.s_addr = INADDR_ANY;
  } else {
    in_addr->sin_addr.s_addr = inet_addr(sock_cfg.address);
  }
}

int SocketWrapper::Bind(const SocketCfg& sock_cfg) {
  if (sock_cfg.sock_attr & kIsListen) {
    if (bind(sockfd_, &addr_, sizeof(addr_)) < 0) {
      return -1;
    }
  }
  return 0;
}

/*
int SocketWrapper::Listen(const SocketCfg& sock_cfg) {
  if (sock_cfg.sock_attr & kIsListen) {
    if (listen(sockfd_, sock_cfg.backlog) < 0) {
      return -1;
    }
  }
  return 0;
}
*/

int SocketWrapper::SetNonBlock() {
  return fcntl(sockfd_, F_SETFL, fcntl(sockfd_, F_GETFL, 0) | O_NONBLOCK);
}
