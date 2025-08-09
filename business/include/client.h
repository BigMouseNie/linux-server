#ifndef BUSSINESS_CLIENT_H_
#define BUSSINESS_CLIENT_H_

class Client {
 public:
  Client() = default;
  virtual ~Client() = default;
  virtual void SetSocket(int sock, bool is_et) = 0;
  virtual int Process() = 0;
  virtual void Close() = 0;
};

#endif  // BUSSINESS_CLIENT_H_
