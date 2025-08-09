#include "socket_creator.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>

SocketCfg::SocketCfg(const SocketCfg& other)
    : port_(other.port_),
      sock_attr_(other.sock_attr_),
      backlog_(other.backlog_),
      address_size_(other.address_size_) {
  SetAddrOrPath(other.address_);
}

SocketCfg& SocketCfg::operator=(const SocketCfg& other) {
  if (this == &other) return *this;
  port_ = other.port_;
  sock_attr_ = other.sock_attr_;
  backlog_ = other.backlog_;
  address_size_ = other.address_size_;
  SetAddrOrPath(other.address_);
  return *this;
}

SocketCfg::SocketCfg(SocketCfg&& other)
    : port_(other.port_),
      sock_attr_(other.sock_attr_),
      backlog_(other.backlog_),
      address_size_(other.address_size_) {
  address_ = other.address_;
  other.port_ = 0;
  other.sock_attr_ = 0;
  other.backlog_ = 0;
  other.address_size_ = 0;
  other.address_ = nullptr;
}

SocketCfg& SocketCfg::operator=(SocketCfg&& other) {
  if (this == &other) return *this;
  port_ = other.port_;
  sock_attr_ = other.sock_attr_;
  backlog_ = other.backlog_;
  address_size_ = other.address_size_;
  address_ = other.address_;

  other.port_ = 0;
  other.sock_attr_ = 0;
  other.backlog_ = 0;
  other.address_size_ = 0;
  other.address_ = nullptr;
  return *this;
}

int SocketCfg::SetAddrOrPath(const char* addr_or_path) {
  size_t len = strlen(addr_or_path);
  address_ = (char*)realloc(address_, len * sizeof(char) + 1);
  if (!address_) return -1;
  address_size_ = len + 1;
  memcpy(address_, addr_or_path, address_size_);
  return 0;
}

SocketCreator& SocketCreator::Instance() {
  static SocketCreator kSocketCreator;
  return kSocketCreator;
}

int SocketCreator::Create(const SocketCfg& sock_cfg, int* attr,
                          struct sockaddr* addr) {
  int ret = CreateSocket(sock_cfg);
  if (ret < 0) return -1;
  int fd = ret;
  if (!addr) {
    memset(&t_addr_, 0, sizeof(t_addr_));
    addr = (struct sockaddr*)&t_addr_;
  }
  ret = CreateAddress(fd, addr, sock_cfg);
  if (ret < 0) {
    Close(fd);
    return -2;
  }

  if (sock_cfg.sock_attr_ & kIsListen) {
    ret = Bind(fd, addr, sock_cfg);
    if (ret < 0) {
      Close(fd);
      return -3;
    }

    ret = Listen(fd, sock_cfg);
    if (ret < 0) {
      Close(fd);
      return -4;
    }
  } else {
    ret = Connect(fd, addr, sock_cfg);
    if (ret < 0) {
      Close(fd);
      return -5;
    }
  }

  if (attr) {
    (*attr) = sock_cfg.sock_attr_;
    (*attr) |= kIsCreate;
  }
  return fd;
}

int SocketCreator::CreateSocket(const SocketCfg& sock_cfg) {
  int domain = PF_INET;
  if (sock_cfg.sock_attr_ & kIsLocal) {
    domain = PF_LOCAL;
  } else if (sock_cfg.sock_attr_ & kIsIPV6) {
    domain = PF_INET6;
  }
  int type = SOCK_STREAM;
  if (sock_cfg.sock_attr_ & kIsUDP) {
    type = SOCK_DGRAM;
  }
  int ret = socket(domain, type, 0);
  if (ret < 0) return -1;
  int fd = ret;

  if (sock_cfg.sock_attr_ & kIsNonBlock) {
    ret = SetNonBlock(fd);
  }
  if (ret < 0) {
    Close(fd);
    return -2;
  }
  return fd;
}

int SocketCreator::CreateAddress(int fd, struct sockaddr* addr,
                                 const SocketCfg& sock_cfg) {
  if (!sock_cfg.address_ || sock_cfg.address_size_ == 0) return -1;
  memset(addr, 0, sizeof(*addr));
  // local socket
  if (sock_cfg.sock_attr_ & kIsLocal) {
    struct sockaddr_un* un_addr = (struct sockaddr_un*)(addr);
    if (sock_cfg.sock_attr_ & kIsListen) unlink(sock_cfg.address_);
    un_addr->sun_family = AF_LOCAL;
    strncpy(un_addr->sun_path, sock_cfg.address_,
            sizeof(un_addr->sun_path) - 1);
    return 0;
  }

  // IPV6
  if (sock_cfg.sock_attr_ & kIsIPV6) {
    struct sockaddr_in6* in6_addr = (struct sockaddr_in6*)(addr);
    in6_addr->sin6_family = AF_INET6;
    in6_addr->sin6_port = htons(sock_cfg.port_);
    if (sock_cfg.sock_attr_ & kIsListen) {
      in6_addr->sin6_addr = in6addr_any;
    } else {
      if (inet_pton(AF_INET6, sock_cfg.address_, &(in6_addr->sin6_addr)) < 0)
        return -2;
    }
    return 0;
  }

  // IPV4
  struct sockaddr_in* in_addr = (struct sockaddr_in*)(addr);
  in_addr->sin_family = AF_INET;
  in_addr->sin_port = htons(sock_cfg.port_);
  if (sock_cfg.sock_attr_ & kIsListen) {
    in_addr->sin_addr.s_addr = INADDR_ANY;
  } else {
    in_addr->sin_addr.s_addr = inet_addr(sock_cfg.address_);
  }
  return 0;
}

int SocketCreator::Bind(int fd, struct sockaddr* addr,
                        const SocketCfg& sock_cfg) {
  socklen_t addr_len = 0;
  if (sock_cfg.sock_attr_ & kIsLocal) {
    addr_len = sizeof(struct sockaddr_un);
  } else if (sock_cfg.sock_attr_ & kIsIPV6) {
    addr_len = sizeof(struct sockaddr_in6);
  } else {
    addr_len = sizeof(struct sockaddr_in);
  }

  if (sock_cfg.sock_attr_ & kIsListen) {
    int ret = bind(fd, (struct sockaddr*)addr, addr_len);
    if (ret < 0) return -1;
  }
  return 0;
}

int SocketCreator::Listen(int fd, const SocketCfg& sock_cfg) {
  if (sock_cfg.sock_attr_ & kIsListen) {
    if (listen(fd, sock_cfg.backlog_) < 0) return -1;
  }
  return 0;
}

int SocketCreator::Connect(int fd, struct sockaddr* addr,
                           const SocketCfg& sock_cfg) {
  socklen_t addr_len = 0;
  if (sock_cfg.sock_attr_ & kIsLocal) {
    addr_len = sizeof(struct sockaddr_un);
  } else if (sock_cfg.sock_attr_ & kIsIPV6) {
    addr_len = sizeof(struct sockaddr_in6);
  } else {
    addr_len = sizeof(struct sockaddr_in);
  }

  return connect(fd, (struct sockaddr*)addr, addr_len);
}

int SocketCreator::SetNonBlock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void SocketCreator::Close(int fd) {
  if (fd != -1) {
    close(fd);
    fd = -1;
  }
}
