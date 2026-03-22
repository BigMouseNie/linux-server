#include "process_wrap.h"

#include <unistd.h>

int ProcessWrap::ForkAndRun() {
  pid_ = fork();
  if (pid_ == -1) {
    return -1;
  }
  if (pid_ == 0) {
    entry_();
    exit(0);
  }
  return 0;
}
