#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>

#include "epoller.h"
#include "socket_wrap.h"

class EpollerTest : public ::testing::Test {
 protected:
  static constexpr uint16_t kTestPort = 19200;

  void SetUp() override {}
  void TearDown() override {}
};

// 基本构造测试

TEST_F(EpollerTest, Create) {
  int callback_called = 0;
  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) { callback_called++; }, 32,
      true);

  // epollfd 应该有效
  EXPECT_TRUE(epoller.IsValid());
}

TEST_F(EpollerTest, CreateWithLT) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32, false);
  EXPECT_TRUE(epoller.IsValid());
}

// Add 测试

TEST_F(EpollerTest, AddInvalidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);
  int ret = epoller.Add(-1, EPOLLIN);
  EXPECT_EQ(ret, -1);
}

TEST_F(EpollerTest, AddValidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int fds[2];
  int ret = pipe(fds);
  ASSERT_EQ(ret, 0);

  ret = epoller.Add(fds[0], EPOLLIN);
  EXPECT_EQ(ret, 0);

  close(fds[0]);
  close(fds[1]);
}

TEST_F(EpollerTest, AddWithUserData) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int fds[2];
  pipe(fds);

  int user_data = 12345;
  int ret = epoller.Add(fds[0], EPOLLIN, &user_data);
  EXPECT_EQ(ret, 0);

  close(fds[0]);
  close(fds[1]);
}

// Modify 测试

TEST_F(EpollerTest, ModifyInvalidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);
  int ret = epoller.Modify(-1, EPOLLOUT);
  EXPECT_EQ(ret, -1);
}

TEST_F(EpollerTest, ModifyValidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int fds[2];
  pipe(fds);

  epoller.Add(fds[0], EPOLLIN);
  int ret = epoller.Modify(fds[0], EPOLLOUT);
  EXPECT_EQ(ret, 0);

  close(fds[0]);
  close(fds[1]);
}

// Del 测试

TEST_F(EpollerTest, DelInvalidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);
  int ret = epoller.Del(-1);
  EXPECT_EQ(ret, -1);
}

TEST_F(EpollerTest, DelValidFd) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int fds[2];
  pipe(fds);

  epoller.Add(fds[0], EPOLLIN);
  int ret = epoller.Del(fds[0]);
  EXPECT_EQ(ret, 0);

  close(fds[0]);
  close(fds[1]);
}

// Wait 超时测试

TEST_F(EpollerTest, WaitTimeout) {
  std::atomic<int> callback_count{0};
  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) { callback_count++; }, 32);

  // 没有事件，应该超时
  int ret = epoller.Wait(100);  // 100ms 超时
  EXPECT_EQ(ret, 0);            // 超时返回 0 个事件
  EXPECT_EQ(callback_count.load(), 0);
}

// Wait 事件触发测试

TEST_F(EpollerTest, WaitWithEvent) {
  std::atomic<int> event_count{0};
  std::atomic<void*> received_data{nullptr};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) {
        event_count = size;
        if (size > 0) {
          received_data = evs[0].data.ptr;
        }
      },
      32);

  int fds[2];
  pipe(fds);

  int user_data = 999;
  epoller.Add(fds[0], EPOLLIN, &user_data);

  // 写入数据触发事件
  write(fds[1], "hello", 5);

  int ret = epoller.Wait(1000);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(event_count.load(), 1);
  EXPECT_EQ(received_data.load(), &user_data);

  close(fds[0]);
  close(fds[1]);
}

// 多个 fd 测试

TEST_F(EpollerTest, MultipleFds) {
  std::atomic<int> event_count{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) { event_count = size; }, 32);

  int fds1[2], fds2[2];
  pipe(fds1);
  pipe(fds2);

  epoller.Add(fds1[0], EPOLLIN);
  epoller.Add(fds2[0], EPOLLIN);

  // 同时触发两个 fd
  write(fds1[1], "a", 1);
  write(fds2[1], "b", 1);

  int ret = epoller.Wait(1000);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(event_count.load(), 2);

  close(fds1[0]);
  close(fds1[1]);
  close(fds2[0]);
  close(fds2[1]);
}

// EPOLLOUT 测试（socket 可写）

TEST_F(EpollerTest, WaitWriteEvent) {
  std::atomic<int> event_count{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) { event_count = size; }, 32);

  int fds[2];
  pipe(fds);

  epoller.Add(fds[1], EPOLLOUT);  // 写端通常可写

  int ret = epoller.Wait(100);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(event_count.load(), 1);

  close(fds[0]);
  close(fds[1]);
}

// 边缘触发 vs 水平触发测试

TEST_F(EpollerTest, EdgeTriggered) {
  std::atomic<int> callback_count{0};
  std::atomic<int> total_events{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) {
        callback_count++;
        total_events += size;
      },
      32, true);  // ET 模式

  int fds[2];
  pipe(fds);

  epoller.Add(fds[0], EPOLLIN);

  // 写入两批数据
  write(fds[1], "hello", 5);
  write(fds[1], "world", 5);

  // ET 模式：第一次 Wait 会返回，第二次可能不返回（除非有新数据）
  int ret1 = epoller.Wait(100);
  EXPECT_EQ(ret1, 1);

  // 读取数据
  char buf[20] = {0};
  read(fds[0], buf, 10);

  // 数据已读完，再次 Wait 应该超时
  int ret2 = epoller.Wait(100);
  EXPECT_EQ(ret2, 0);

  close(fds[0]);
  close(fds[1]);
}

TEST_F(EpollerTest, LevelTriggered) {
  std::atomic<int> callback_count{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) { callback_count++; }, 32,
      false);  // LT 模式

  int fds[2];
  pipe(fds);

  epoller.Add(fds[0], EPOLLIN);

  write(fds[1], "hello", 5);

  // LT 模式：只要数据存在，每次 Wait 都会返回
  int ret1 = epoller.Wait(100);
  EXPECT_EQ(ret1, 1);

  // 数据还在缓冲区，再次 Wait 还是会返回
  int ret2 = epoller.Wait(100);
  EXPECT_EQ(ret2, 1);

  close(fds[0]);
  close(fds[1]);
}

// 网络连接测试

TEST_F(EpollerTest, AcceptConnection) {
  ServerSocket server(kTestPort, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accept_count{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) {
        for (size_t i = 0; i < size; ++i) {
          if (evs[i].events & EPOLLIN) {
            int* server_fd = static_cast<int*>(evs[i].data.ptr);
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd =
                accept(*server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd >= 0) {
              accept_count++;
              close(client_fd);
            }
          }
        }
      },
      32, true);

  int server_fd = server.GetFd();
  epoller.Add(server_fd, EPOLLIN, &server_fd);

  // 客户端连接
  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort);
    usleep(50000);
  });

  usleep(100000);
  int ret = epoller.Wait(1000);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(accept_count.load(), 1);

  client_thread.join();
}

TEST_F(EpollerTest, ReadFromConnection) {
  ServerSocket server(kTestPort + 1, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> bytes_received{0};

  Epoller epoller(
      [&](struct epoll_event* evs, size_t size) {
        for (size_t i = 0; i < size; ++i) {
          if (evs[i].events & EPOLLIN) {
            int* client_fd = static_cast<int*>(evs[i].data.ptr);
            char buf[128];
            int n = recv(*client_fd, buf, sizeof(buf), 0);
            if (n > 0) {
              bytes_received += n;
            }
          }
        }
      },
      32, true);

  // 接受连接
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int listen_fd = server.GetFd();

  std::thread server_thread([&]() {
    usleep(50000);
    int client_fd =
        accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      epoller.Add(client_fd, EPOLLIN, new int(client_fd));
    }
  });

  std::thread client_thread([&]() {
    usleep(100000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 1);
    send(client.GetFd(), "hello", 5, 0);
    usleep(50000);
  });

  usleep(150000);
  int ret = epoller.Wait(1000);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(bytes_received.load(), 5);

  server_thread.join();
  client_thread.join();
}

// 错误处理测试

TEST_F(EpollerTest, WaitWithError) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int fds[2];
  pipe(fds);
  epoller.Add(fds[0], EPOLLIN);

  // 关闭写端，读端会收到 EPOLLHUP
  close(fds[1]);

  std::atomic<uint32_t> received_events{0};
  Epoller epoller2(
      [&](struct epoll_event* evs, size_t size) {
        if (size > 0) {
          received_events = evs[0].events;
        }
      },
      32);

  int fds2[2];
  pipe(fds2);
  epoller2.Add(fds2[0], EPOLLIN);
  close(fds2[1]);

  int ret = epoller2.Wait(1000);
  EXPECT_EQ(ret, 1);
  EXPECT_TRUE(received_events.load() & EPOLLHUP);

  close(fds[0]);
  close(fds2[0]);
}

// saved_errno 测试

TEST_F(EpollerTest, WaitWithSavedErrno) {
  Epoller epoller([](struct epoll_event* evs, size_t size) {}, 32);

  int saved_errno = 0;
  int ret = epoller.Wait(100, &saved_errno);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(saved_errno, 0);
}
