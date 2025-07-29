#include "process_wrapper.h"

#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

int ProcessWrapper::Run(void* arg) {
  if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, pipe_)) {
    // printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
    //         strerror(errno));
    return -1;
  }

  pid_ = fork();
  if (pid_ == -1) {
    return -2;
  }
  if (pid_ == 0) {  // sub process
    close(pipe_[1]);
    pipe_[1] = 0;                 // close
    shutdown(pipe_[0], SHUT_WR);  // shutdown write
    // printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
    entry_();
    exit(0);
  }

  // main
  close(pipe_[0]);
  pipe_[0] = 0;                 // close
  shutdown(pipe_[1], SHUT_RD);  // shutdown read
  return pid_;
}

int ProcessWrapper::ReadFdFromPipe(int& fd) {
  struct msghdr msg = {0};
  struct iovec iov[2];
  char temp[2][10];
  iov[0].iov_len = sizeof(temp[0]);
  iov[0].iov_base = temp[0];
  iov[1].iov_len = sizeof(temp[1]);
  iov[1].iov_base = temp[1];
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  char cmsg_buf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);

  if (-1 == recvmsg(pipe_[0], &msg, 0)) {
    // printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
    //         strerror(errno));
    return -1;
  }
  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
    return 0;
  }
  // printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
  return -1;
}

int ProcessWrapper::WriteFdToPipe(int fd) {
  struct msghdr msg = {0};
  struct iovec iov[2];
  char temp[2][10] = {"wang", "shuo"};
  iov[0].iov_base = temp[0];
  iov[0].iov_len = strlen(temp[0]) + 1;
  iov[1].iov_base = temp[1];
  iov[1].iov_len = strlen(temp[1]) + 1;
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;

  char cmsg_buf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;

  void* cdata = CMSG_DATA(cmsg);
  *static_cast<int*>(cdata) = fd;

  if (-1 == sendmsg(pipe_[1], &msg, 0)) {
    // printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
    //         strerror(errno));
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
