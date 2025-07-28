#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <string>

#include "base/singleton.hpp"
#include "epoller.h"
#include "ring_buffer.h"
#include "socket_wrapper.h"

class Process;
int ClientDealProc(Process* proc) {
  // test read
  // int fd = -1;
  // if (0 != proc->ReadFromPipe(fd)) {
  //   printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
  //   strerror(errno)); return -1;
  // }

  // printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
  // printf("<%s> : <%d> : fd(%d)\n", __FUNCTION__, __LINE__, fd);
  // char buf[10];
  // lseek(fd, 0, SEEK_SET);
  // read(fd, buf, sizeof(buf));
  // std::cout << "sub proc recv : " << buf << std::endl;
  // return 0;
}

int LoggerProc(Process* proc) { return 0; }

int main() {
  // Process::Daemonize();
  // Process client_proc;
  // client_proc.SetEntryFunction(ClientDealProc, &client_proc);
  // client_proc.Create();

  // // test write
  // int fd =
  //     open("./test.txt", O_RDWR | O_CREAT | O_APPEND);  // 读写 | 创建 | 追加
  // printf("<%s> : <%d> : pid(%d)\n", __FUNCTION__, __LINE__, getpid());
  // printf("<%s> : <%d> : fd(%d)\n", __FUNCTION__, __LINE__, fd);
  // write(fd, "player", sizeof("player"));
  // client_proc.WriteToPipe(fd);
  // close(fd);
  // return 0;

  // Epoller epoller;
  // epoller.Create(5, true);
  // int ret = epoller.Wait(3000);
  // std::cout << "ret val : " << ret << std::endl;
  // printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__,
  //       strerror(errno));

  // char addr[30] = {"/tmp/log_socket.log"};
  // SocketWrapper socket_wrapper;
  // SocketWrapper::SocketCfg socket_cfg;
  // socket_cfg.domain = AF_INET;
  // socket_cfg.type = SOCK_STREAM;
  // socket_cfg.is_listen = false;
  // socket_cfg.is_local = true;
  // socket_cfg.port = 8081;
  // socket_cfg.address = addr;
  // socket_cfg.address_size = strlen(addr) + 1;
  // int ret = socket_wrapper.Create(socket_cfg);
  // printf("<%s> : <%d> : res<%d> : err(%s)\n", __FUNCTION__, __LINE__,
  //        ret, strerror(errno));

  // RingBuffer rbuf1;
  // RingBuffer rbuf2;
  // char sbuf1[256];
  // char sbuf2[256];
  // std::string str("123456789");
  // std::cout << "str.size() = " << str.size() << std::endl;
  // rbuf1.Write(str.c_str(), str.size());
  // rbuf1.Write("\0", 1);

  // printf("class<%s> : Readable(%d) : Writeable(%d) : Size(%d)\n",
  //     "rbuf1", rbuf1.Readable(), rbuf1.Writable(), rbuf1.Size());

  // printf("class<%s> : Readable(%d) : Writeable(%d) : Size(%d)\n",
  //     "rbuf2", rbuf2.Readable(), rbuf2.Writable(), rbuf2.Size());

  // rbuf1.Read(rbuf2);
  // // rbuf2.Write(rbuf1);

  // printf("class<%s> : Readable(%d) : Writeable(%d) : Size(%d)\n",
  //     "rbuf1", rbuf1.Readable(), rbuf1.Writable(), rbuf1.Size());

  // printf("class<%s> : Readable(%d) : Writeable(%d) : Size(%d)\n",
  //     "rbuf2", rbuf2.Readable(), rbuf2.Writable(), rbuf2.Size());

  // rbuf2.Read(sbuf1, 3);

  // printf("class<%s> : Readable(%d) : Writeable(%d) : Size(%d)\n",
  //     "rbuf2", rbuf2.Readable(), rbuf2.Writable(), rbuf2.Size());

  // std::cout << sbuf1 << std::endl;

  return 0;
}
