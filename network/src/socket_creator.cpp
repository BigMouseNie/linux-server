#include "socket_creator.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/un.h>
#include <unistd.h>

int SocketCreator::Create(const SocketCfg& sock_cfg, int* attr) {
  static SocketCreator* kIntance;

  int ret = kIntance->CreateSocket(sock_cfg);
  if (ret < 0) return -1;
  int fd = ret;

  struct sockaddr_storage addr;
  ret = kIntance->CreateAddress(fd, &addr, sock_cfg);
  if (ret < 0) {
    kIntance->Close(fd);
    return -2;
  }

  ret = kIntance->Bind(fd, &addr, sock_cfg);
  if (ret < 0) {
    kIntance->Close(fd);
    return -3;
  }

  ret = kIntance->Listen(fd, sock_cfg);
  if (ret < 0) {
    kIntance->Close(fd);
    return -4;
  }

  if (attr) {
    (*attr) = sock_cfg.sock_attr;
    (*attr) |= kIsCreate;
  }
  return fd;
}

int SocketCreator::CreateSocket(const SocketCfg& sock_cfg) {
  int domain = PF_INET;
  if (sock_cfg.sock_attr & kIsLocal) {
    domain = PF_LOCAL;
  } else if (sock_cfg.sock_attr & kIsIPV6) {
    domain = PF_INET6;
  }
  int type = SOCK_STREAM;
  if (sock_cfg.sock_attr & kIsUDP) {
    type = SOCK_DGRAM;
  }
  int ret = socket(domain, type, 0);
  if (ret < 0) return -1;
  int fd = ret;

  if (sock_cfg.sock_attr & kIsNonBlock) {
    ret = SetNonBlock(fd);
  }
  if (ret < 0) {
    Close(fd);
    return -2;
  }
  return 0;
}

int SocketCreator::CreateAddress(int fd, struct sockaddr_storage* addr,
                                 const SocketCfg& sock_cfg) {
  memset(addr, 0, sizeof(*addr));
  // local socket
  if (sock_cfg.sock_attr & kIsLocal) {
    struct sockaddr_un* un_addr = (struct sockaddr_un*)(addr);
    if (sock_cfg.sock_attr & kIsListen) unlink(sock_cfg.address);
    un_addr->sun_family = AF_LOCAL;
    memcpy(un_addr->sun_path, sock_cfg.address, sock_cfg.address_size - 1);
    return 0;
  }

  // IPV6
  if (sock_cfg.sock_attr & kIsIPV6) {
    struct sockaddr_in6* in6_addr = (struct sockaddr_in6*)(addr);
    in6_addr->sin6_family = AF_INET6;
    in6_addr->sin6_port = htons(sock_cfg.port);
    if (sock_cfg.sock_attr & kIsListen) {
      in6_addr->sin6_addr = in6addr_any;
    } else {
      if (inet_pton(AF_INET6, sock_cfg.address, &(in6_addr->sin6_addr)) < 0)
        return -1;
    }
    return 0;
  }

  // IPV4
  struct sockaddr_in* in_addr = (struct sockaddr_in*)(addr);
  in_addr->sin_family = AF_INET;
  in_addr->sin_port = htons(sock_cfg.port);
  if (sock_cfg.sock_attr & kIsListen) {
    in_addr->sin_addr.s_addr = INADDR_ANY;
  } else {
    in_addr->sin_addr.s_addr = inet_addr(sock_cfg.address);
  }
}

int SocketCreator::Bind(int fd, struct sockaddr_storage* addr,
                        const SocketCfg& sock_cfg) {
  if (sock_cfg.sock_attr & kIsListen) {
    if (bind(fd, (struct sockaddr*)addr, sizeof(*addr)) < 0) return -1;
  }
  return 0;
}

int SocketCreator::Listen(int fd, const SocketCfg& sock_cfg) {
  if (sock_cfg.sock_attr & kIsListen) {
    if (listen(fd, sock_cfg.backlog) < 0) return -1;
  }
  return 0;
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
