#include "socket_wrapper.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/un.h>
#include <unistd.h>

SocketWrapper::SocketWrapper(bool manual_mgnt)
    : attr_(0), sockfd_(-1), manual_mgnt_(manual_mgnt) {}

SocketWrapper::SocketWrapper(SocketWrapper&& other)
    : attr_(other.attr_),
      sockfd_(other.sockfd_),
      manual_mgnt_(other.manual_mgnt_) {
  other.attr_ = 0;
  other.sockfd_ = -1;
}

SocketWrapper& SocketWrapper::operator=(SocketWrapper&& other) {
  if (this == &other) return *this;
  Close();

  attr_ = other.attr_;
  sockfd_ = other.sockfd_;
  manual_mgnt_ = other.manual_mgnt_;

  other.attr_ = 0;
  other.sockfd_ = -1;
  return *this;
}

int SocketWrapper::Create(const SocketCfg& sock_cfg) {
  int ret = SocketCreator::Instance().Create(sock_cfg, &attr_);
  if (ret >= 0) {
    sockfd_ = ret;
    return 0;
  }
  return ret;
}

void SocketWrapper::Close() {
  if (sockfd_ != -1) {
    close(sockfd_);
    sockfd_ = -1;
  }
}

int SocketWrapper::Recv(SocketBuffer& buf) {
  int saved_errno;
  int ret = buf.ReadFromSock(sockfd_, attr_ & kIsNonBlock, &saved_errno);
  // TODO: ret
  return ret;
}

int SocketWrapper::Send(SocketBuffer& buf) {
  int saved_errno;
  int ret = buf.WriteToSock(sockfd_, attr_ & kIsNonBlock, &saved_errno);
  // TODO: ret
  return ret;
}
