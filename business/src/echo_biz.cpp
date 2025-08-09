#include "echo_biz.h"

#include <thread>

#include "http_parser_wrap.h"
#include "logger.h"

int EchoBizClnt::Recv() {
  int save_errno;
  int ret = recv_buf_.ReadFromSock(sock_, is_et_, &save_errno);
  if (ret == 0) {
    LOG_INFO("client(%d) actively close", sock_);
    return 0;
  } else if (ret < 0) {
    LOG_WARN("client(%d) read socket err(%d:%s)", sock_, save_errno,
             strerror(errno));
    return -1;
  }
  return ret;
}

int EchoBizClnt::Send() {
  int save_errno;
  int ret = send_buf_.WriteToSock(sock_, is_et_, &save_errno);
  if (ret == 0) {
    LOG_WARN("client(%d) write socket bytes is zero");
    return 0;
  } else if (ret < 0) {
    LOG_WARN("client(%d) write socket err(%d:%s)", sock_, save_errno,
             strerror(errno));
    return -1;
  }
  return ret;
}

int EchoBizClnt::Process() {
  // pass
  return 0;
}

void EchoBizClnt::Close() {
  if (sock_ != -1) {
    close(sock_);
    sock_ = -1;
  }
}

EchoBiz::EchoBiz() : is_et_(true) {
  thread_num_ = std::thread::hardware_concurrency();
  event_arr_size_ = thread_num_ * 2;
}

int EchoBiz::Start() {
  if (proc_.Init(&EchoBiz::EventLoop, this) < 0) return -1;
  if (proc_.Run() < 0) return -2;
  return 0;
}

int EchoBiz::SendConnection(int fd, void* info, int info_len) {
  return proc_.WriteFdToPipe(fd, info, info_len);
}

int EchoBiz::RecvConnection(int& fd, void* info, int* info_len) {
  return proc_.ReadFdFromPipe(fd, info, info_len);
}

void EchoBiz::EventLoop() {
  auto ev_cb_ = [this](struct epoll_event* evs, size_t size) {
    for (int i = 0; i < size; ++i) {
      EchoBizClnt* clnt = static_cast<EchoBizClnt*>(evs[i].data.ptr);
      if (evs[i].events & EPOLLIN) {
        thread_pool_.AddTask(&EchoBiz::DealRead, this, clnt);
      } else if (evs[i].events & EPOLLOUT) {
        thread_pool_.AddTask(&EchoBiz::DealWrite, this, clnt);
      }
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
  int ret = accpetor_.Init(&EchoBiz::Accept, this);
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

void EchoBiz::Accept() {
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
    EchoBizClnt* clnt = AddClient(fd);
    epoller_.Add(fd, EPOLLIN, clnt);
  }
}

EchoBizClnt* EchoBiz::AddClient(int sock) {
  EchoBizClnt* clnt_ptr = new EchoBizClnt;
  clnt_ptr->SetSocket(sock, is_et_);
  std::shared_ptr<EchoBizClnt> clnt(clnt_ptr);
  std::lock_guard<std::mutex> lock(clnts_mtx_);
  clients[sock] = clnt;
  return clnt_ptr;
}

void EchoBiz::RemoveClient(int sock) {
  std::lock_guard<std::mutex> lock(clnts_mtx_);
  auto it = clients.find(sock);
  if (it == clients.end()) return;
  clients.erase(it);
}

void EchoBiz::DealRead(EchoBizClnt* clnt) {
  if (clnt->Recv() <= 0) {
    RemoveClient(clnt->GetSocket());
    return;
  }
  OnProcess(clnt);
}

void EchoBiz::DealWrite(EchoBizClnt* clnt) {
  if (clnt->Send() <= 0) {
    RemoveClient(clnt->GetSocket());
    return;
  }
  OnProcess(clnt);
}

void EchoBiz::OnProcess(EchoBizClnt* clnt) {
  if (clnt->Process() < 0) {
    epoller_.Modify(clnt->GetSocket(), EPOLLIN);
  } else {
    epoller_.Modify(clnt->GetSocket(), EPOLLOUT);
  }
}
