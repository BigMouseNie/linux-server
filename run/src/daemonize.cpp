#include "daemonize.h"

#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

int Daemonize() {
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

  signal(SIGCHLD, SIG_IGN);
  chdir("/");
  umask(0);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  open("/dev/null", O_RDONLY);
  open("/dev/null", O_WRONLY);
  open("/dev/null", O_RDWR);

  return 0;
}
