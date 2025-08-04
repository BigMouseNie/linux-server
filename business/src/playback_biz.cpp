#include "playback_biz.h"

#include <thread>

#include "logger.h"

PlaybackBiz::PlaybackBiz() : is_et_(true) {
  thread_num_ = std::thread::hardware_concurrency();
  event_arr_size_ = thread_num_ * 2;
}

int PlaybackBiz::Start() {
  if (proc_.Init(&PlaybackBiz::EventLoop, this) < 0) return -1;
  if (proc_.Run() < 0) return -2;
  return 0;
}

int PlaybackBiz::SendConnection(int fd, void* info, int info_len) {
  return proc_.WriteFdToPipe(fd, info, info_len);
}

int PlaybackBiz::RecvConnection(int& fd, void* info, int* info_len) {
  return proc_.ReadFdFromPipe(fd, info, info_len);
}

void PlaybackBiz::EventLoop() {
  auto ev_cb_ = [this](struct epoll_event* evs, size_t size) {
    for (int i = 0; i < size; ++i) {
      int fd = evs[i].data.fd;
      // business, to thread pool read and deal
    }
  };
  if (epoller_.Create(ev_cb_, event_arr_size_, is_et_) < 0) {
    LOG_ERROR("epoller create failed");
    return;
  }

  if (thread_pool_.Start(thread_num_)) {
    LOG_ERROR("thread pool start failed");
    return;
  }

  running_ = true;
  int ret = accpetor_.Init(&PlaybackBiz::Accept, this);
  if (accpetor_.Run() < 0) {
    LOG_ERROR("accept thread run failed");
    return;
  }

  while (running_) {
    if (epoller_.Wait(100) < 0) {
      LOG_ERROR("epoller wait error");
      running_ = false;
    }
  }
}

void PlaybackBiz::Accept() {
  while (running_) {
    int fd;
    if (RecvConnection(fd) < 0) {
      LOG_ERROR("recv connection failed");
      running_ = false;
      break;
    }
    if (fd < 0) {
      running_ = false;
      break;
    }
    epoller_.Add(fd, EPOLLIN);
  }
}
