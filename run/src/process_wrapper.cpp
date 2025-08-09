#include "process_wrapper.h"

#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

int ProcessWrapper::Run(void* arg) {
  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, pipe_)) {
    return -1;
  }

  pid_ = fork();
  if (pid_ == -1) {
    return -2;
  }
  if (pid_ == 0) {    // sub process
    close(pipe_[0]);  // close
    pipe_[0] = -1;
    fd_ = pipe_[1];
    entry_();
    exit(0);
  }

  // main
  close(pipe_[1]);  // close
  pipe_[1] = -1;
  fd_ = pipe_[0];
  return pid_;
}

int ProcessWrapper::ReadFdFromPipe(int& fd, void* info, int* info_len) {
  struct msghdr msg = {0};
  if (info && info_len) {
    msg.msg_iovlen = 1;
    msg.msg_iov->iov_base = info;
    msg.msg_iov->iov_len = *info_len;
  }

  char cmsg_buf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);

  if (-1 == recvmsg(fd_, &msg, 0)) {
    return -1;
  }
  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
    return 0;
  }
  return -1;
}

int ProcessWrapper::WriteFdToPipe(int fd, void* info, int info_len) {
  struct msghdr msg = {0};
  if (info && info_len > 0) {
    msg.msg_iovlen = 1;
    msg.msg_iov->iov_base = info;
    msg.msg_iov->iov_len = info_len;
  }

  char cmsg_buf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;

  void* cdata = CMSG_DATA(cmsg);
  *static_cast<int*>(cdata) = fd;

  if (-1 == sendmsg(fd, &msg, 0)) {
    return -1;
  }
  return 0;
}

int ProcessWrapper::Daemonize() {
  pid_t pid = fork();
  if (pid == -1) {
    return -1;
  }
  if (pid > 0) {
    exit(0);
  }

  if (-1 == setsid()) {
    return -2;
  }
  pid = fork();
  if (pid == -1) {
    return -3;
  }
  if (pid > 0) {
    exit(0);
  }

  signal(SIGCHLD, SIG_IGN);  // 忽略 SIGCHLD，防止子进程变僵尸
  chdir("/");                // 修改工作目录
  umask(0);                  // 重设文件权限掩码

  // 关闭标准输入输出
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  // 重定向到 /dev/null
  open("/dev/null", O_RDONLY);  // stdin
  open("/dev/null", O_WRONLY);  // stdout
  open("/dev/null", O_RDWR);    // stderr

  return 0;
}
