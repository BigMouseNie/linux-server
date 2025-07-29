#include <iostream>

#include "process_wrapper.h"
#include "thread_wrapper.h"

int test(int x) {
  std::cout << "this val : " << x << std::endl;
  return 0;
}

int main() {
  ProcessWrapper proc;
  proc.Init(test, 6);
  proc.Run();

  ThreadWrapper thrd;
  thrd.Init(test, 9);
  int ret = thrd.Run();

  sleep(1); // wait thread
  return 0;
}
