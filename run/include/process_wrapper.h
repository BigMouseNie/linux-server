#ifndef RUN_PROCESSWRAPPER_H_
#define RUN_PROCESSWRAPPER_H_

#include <functional>

class ProcessWrapper {
 public:
  ProcessWrapper() = default;
  ~ProcessWrapper() = default;
  ProcessWrapper(const ProcessWrapper&) = delete;
  ProcessWrapper& operator=(const ProcessWrapper&) = delete;

  template <typename Func, typename... Args>
  int SetEntryFunction(Func&& func, Args&&... args) {
    entry_ = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    return 0;
  }

  int Create();
  int ReadFdFromPipe(int& fd);
  int WriteFdToPipe(int fd);
  static int Daemonize();

 private:
  std::function<void()> entry_;
  pid_t pid_;
  int pipe_[2];
};

#endif  // RUN_PROCESSWRAPPER_H_
