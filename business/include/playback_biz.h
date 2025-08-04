#ifndef BUSINESS_PLAYBACKBIZ_H_
#define BUSINESS_PLAYBACKBIZ_H_

#include "business.h"
#include "epoller.h"
#include "process_wrapper.h"
#include "thread_pool.h"
#include "thread_wrapper.h"

class PlaybackBiz : public Business {
 public:
  PlaybackBiz();
  PlaybackBiz(bool is_et, int thread_num, int event_arr_size)
      : is_et_(true),
        thread_num_(thread_num),
        event_arr_size_(event_arr_size) {}
  ~PlaybackBiz() = default;

  virtual int Start();
  virtual int SendConnection(int fd, void* info = nullptr, int info_len = 0);

 private:
  virtual int RecvConnection(int& fd, void* info = nullptr,
                             int* info_len = nullptr);
  void EventLoop();
  void Accept();

 private:
  bool running_;
  bool is_et_;
  int thread_num_;
  int event_arr_size_;
  ProcessWrapper proc_;
  Epoller epoller_;
  ThreadWrapper accpetor_;
  ThreadPool thread_pool_;
};

#endif  // BUSINESS_PLAYBACKBIZ_H_
