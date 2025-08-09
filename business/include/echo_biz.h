#ifndef BUSINESS_ECHOBIZ_H_
#define BUSINESS_ECHOBIZ_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "business.h"
#include "client.h"
#include "epoller.h"
#include "process_wrapper.h"
#include "thread_pool.h"
#include "thread_wrapper.h"
#include "socket_buffer.h"

class EchoBizClnt : Client {
 public:
  EchoBizClnt() : sock_(-1), is_et_(true) {}
  EchoBizClnt(int sock) : sock_(sock) {};
  ~EchoBizClnt() { close(sock_); }
  void SetSocket(int sock, bool is_et) {
    sock_ = sock;
    is_et_ = is_et;
  }
  int GetSocket() { return sock_; }
  int Recv();
  int Send();
  int Process();
  void Close();

 private:
  bool is_et_;
  int sock_;
  SocketBuffer recv_buf_;
  SocketBuffer send_buf_;
};

using Clients = std::unordered_map<int, std::shared_ptr<EchoBizClnt>>;

class EchoBiz : public Business {
 public:
  EchoBiz();
  EchoBiz(bool is_et, int thread_num, int event_arr_size)
      : is_et_(true),
        thread_num_(thread_num),
        event_arr_size_(event_arr_size) {}
  ~EchoBiz() = default;

  virtual int Start();
  virtual int SendConnection(int fd, void* info = nullptr, int info_len = 0);

 private:
  virtual int RecvConnection(int& fd, void* info = nullptr,
                             int* info_len = nullptr);
  void EventLoop();
  void Accept();
  EchoBizClnt* AddClient(int sock);
  void RemoveClient(int sock);
  void DealRead(EchoBizClnt* clnt);
  void DealWrite(EchoBizClnt* clnt);
  void OnProcess(EchoBizClnt* clnt);

 private:
  bool running_;
  bool is_et_;
  int thread_num_;
  int event_arr_size_;
  ProcessWrapper proc_;
  Epoller epoller_;
  ThreadWrapper accpetor_;
  ThreadPool thread_pool_;
  std::mutex clnts_mtx_;
  Clients clients;
};

#endif  // BUSINESS_ECHOBIZ_H_
