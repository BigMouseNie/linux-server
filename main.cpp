#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <iostream>

class Process {
 public:
  Process() = default;
  ~Process() = default;
  Process(const Process&) = delete;
  Process& operator=(const Process&) = delete;

  template <typename Func, typename... Args>
  int SetEntryFunction(Func&& func, Args&&... args) {
    entry_ = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    return 0;
  }

  int Create() {
    if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, 0, pipe_)) {
      printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
             strerror(errno));
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
      printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
      entry_();
      exit(0);
    }

    // main
    close(pipe_[0]);
    pipe_[0] = 0;                 // close
    shutdown(pipe_[1], SHUT_RD);  // shutdown read
    return pid_;
  }

  // read fd
  int ReadFromPipe(int& fd) {
    struct msghdr msg;
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
      printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
             strerror(errno));
      return -1;
    }
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET &&
        cmsg->cmsg_type == SCM_RIGHTS) {
      memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
      return 0;
    }
    printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
    return -1;
  }

  // send fd
  int WriteToPipe(int fd) {
    struct msghdr msg = {};
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
      printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
             strerror(errno));
      return -1;
    }
    return 0;
  }

  static int Daemonize() {
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

    // 忽略 SIGCHLD，防止子进程变僵尸
    signal(SIGCHLD, SIG_IGN);

    // 修改工作目录
    chdir("/");

    // 重设文件权限掩码
    umask(0);

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

 private:
  std::function<void()> entry_;
  pid_t pid_;
  int pipe_[2];
};

int ClientDealProc(Process* proc) {
  // test read
  int fd = -1;
  if (0 != proc->ReadFromPipe(fd)) {
    printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
    return -1;
  }

  printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
  printf("<%s> : <%d> : fd(%d)\n", __FUNCTION__, __LINE__, fd);
  char buf[10];
  lseek(fd, 0, SEEK_SET);
  read(fd, buf, sizeof(buf));
  std::cout << "sub proc recv : " << buf << std::endl;
  return 0;
}

int LoggerProc(Process* proc) { return 0; }

int main() {
  // Process::Daemonize();
  Process client_proc;
  client_proc.SetEntryFunction(ClientDealProc, &client_proc);
  client_proc.Create();

  // test write
  int fd =
      open("./test.txt", O_RDWR | O_CREAT | O_APPEND);  // 读写 | 创建 | 追加
  printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
  printf("<%s> : <%d> : fd(%d)\n", __FUNCTION__, __LINE__, fd);
  write(fd, "player", sizeof("player"));
  client_proc.WriteToPipe(fd);
  close(fd);
  return 0;
}
