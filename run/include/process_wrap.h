#ifndef RUN_PROCESSWRAP_H_
#define RUN_PROCESSWRAP_H_

#include <functional>
#include <sys/types.h>

class ProcessWrap {
 public:
  template <typename Func, typename... Args>
  explicit ProcessWrap(Func&& func, Args&&... args)
      : pid_(0),
        entry_(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)) {
    ForkAndRun();
  }

  ~ProcessWrap() = default;

  ProcessWrap(const ProcessWrap&) = delete;
  ProcessWrap& operator=(const ProcessWrap&) = delete;

  pid_t Pid() const { return pid_; }

 private:
  int ForkAndRun();
  pid_t pid_;
  std::function<void()> entry_;
};

#endif  // RUN_PROCESSWRAP_H_
