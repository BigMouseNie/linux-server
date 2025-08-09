#ifndef RUN_RUNBASE_H_
#define RUN_RUNBASE_H_

#include <functional>

enum class RunState : int {
  kInvalid,
  kInitialized,
  kRunning,
  kStopped,
  kPaused,
};

class RunBase {
 public:
  RunBase() : state_(RunState::kInvalid) {}
  virtual ~RunBase() = default;

  template <typename Func, typename... Args>
  int Init(Func&& func, Args&&... args) {
    if (state_ != RunState::kInitialized && state_ != RunState::kInvalid) {
      return -1;
    }
    entry_ = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    state_ = RunState::kInitialized;
    return 0;
  }

  virtual int Run(void* arg = nullptr) {
    if (state_ != RunState::kInitialized) {
      return -1;
    }
    state_ = RunState::kRunning;
    entry_();
    state_ = RunState::kStopped;
    return 0;
  }

  virtual int Stop(void* arg = nullptr) { return -1; }

  virtual int PauseOrUnpaused(void* arg = nullptr) { return -1; }

 protected:
  RunBase(const RunBase&) = delete;
  RunBase& operator=(const RunBase&) = delete;

 protected:
  std::function<void()> entry_;
  RunState state_;
};

#endif  // RUN_RUNBASE_H_