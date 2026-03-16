#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>

#include "reactor_acc.h"
#include "socket_wrap.h"

class ReactorAccTest : public ::testing::Test {
 protected:
  static constexpr uint16_t kTestPort = 19300;

  void SetUp() override {}
  void TearDown() override {}
};

// 基本构造测试

TEST_F(ReactorAccTest, Constructor) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10, true);

  EXPECT_TRUE(reactor.IsValid());
}

TEST_F(ReactorAccTest, ConstructorWithLT) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10, false);

  EXPECT_TRUE(reactor.IsValid());
}

// SetListenSock 测试

TEST_F(ReactorAccTest, SetListenSockValid) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10);

  ServerSocket server(kTestPort, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  int ret = reactor.SetListenSock(std::move(server));
  EXPECT_EQ(ret, 0);
}

// TEST_F(ReactorAccTest, SetListenSockInvalidSocket) {
//   ReactorAcc reactor(
//       [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd);
//       }, 10);

//   ServerSocket invalid_server;  // 无效的服务器 socket
//   int ret = reactor.SetListenSock(std::move(invalid_server));
//   EXPECT_EQ(ret, -1);
// }

TEST_F(ReactorAccTest, SetListenSockMultiple) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10);

  ServerSocket server1(kTestPort, false, true, 10);
  ServerSocket server2(kTestPort + 1, false, true, 10);
  ASSERT_TRUE(server1.IsValid());
  ASSERT_TRUE(server2.IsValid());

  int ret1 = reactor.SetListenSock(std::move(server1));
  int ret2 = reactor.SetListenSock(std::move(server2));

  EXPECT_EQ(ret1, 0);
  EXPECT_EQ(ret2, 0);
}

// Accept 超时测试

TEST_F(ReactorAccTest, AcceptTimeout) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10);

  ServerSocket server(kTestPort + 2, false, true, 10);
  reactor.SetListenSock(std::move(server));

  int ret = reactor.Accept(100);  // 100ms 超时
  EXPECT_EQ(ret, 0);              // 没有连接，返回 0
}

// 单连接测试

TEST_F(ReactorAccTest, AcceptSingleConnection) {
  std::atomic<int> accepted_count{0};

  ReactorAcc reactor(
      [&accepted_count](int conn_fd, struct sockaddr* addr, int addr_len) {
        accepted_count++;
        close(conn_fd);
      },
      10);

  ServerSocket server(kTestPort + 3, false, true, 10);
  reactor.SetListenSock(std::move(server));

  // 客户端连接
  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 3);
    usleep(50000);
  });

  usleep(100000);
  int ret = reactor.Accept(1000);
  EXPECT_GE(ret, 0);

  client_thread.join();
  EXPECT_EQ(accepted_count.load(), 1);
}

// 多连接测试

TEST_F(ReactorAccTest, AcceptMultipleConnections) {
  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  ReactorAcc reactor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      10,
      true);  // ET 模式

  ServerSocket server(kTestPort + 4, false, true, 10);
  reactor.SetListenSock(std::move(server));

  constexpr int kNumClients = 5;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < kNumClients; ++i) {
    client_threads.emplace_back([&]() {
      usleep(50000);
      ClientSocket client;
      client.Connect("127.0.0.1", kTestPort + 4);
      usleep(50000);
    });
  }

  usleep(150000);
  int ret = reactor.Accept(1000);
  EXPECT_GE(ret, 0);

  for (auto& t : client_threads) {
    t.join();
  }

  EXPECT_EQ(accepted_count.load(), kNumClients);

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
}

// 多个监听端口测试

TEST_F(ReactorAccTest, MultipleListenPorts) {
  std::atomic<int> port1_count{0};
  std::atomic<int> port2_count{0};

  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10);

  ServerSocket server1(kTestPort + 5, false, true, 10);
  ServerSocket server2(kTestPort + 6, false, true, 10);
  reactor.SetListenSock(std::move(server1));
  reactor.SetListenSock(std::move(server2));

  // 客户端连接两个端口
  std::thread client1([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 5);
    usleep(50000);
  });

  std::thread client2([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 6);
    usleep(50000);
  });

  usleep(100000);
  reactor.Accept(1000);
  usleep(50000);
  reactor.Accept(1000);

  client1.join();
  client2.join();
}

// LT 模式测试

TEST_F(ReactorAccTest, LTMode) {
  std::atomic<int> accept_count{0};

  ReactorAcc reactor(
      [&accept_count](int conn_fd, struct sockaddr* addr, int addr_len) {
        accept_count++;
        close(conn_fd);
      },
      10,
      false);  // LT 模式

  ServerSocket server(kTestPort + 7, false, false, 10);  // 阻塞 socket
  reactor.SetListenSock(std::move(server));

  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 7);
    usleep(100000);
  });

  usleep(100000);
  int ret = reactor.Accept(1000);
  EXPECT_GE(ret, 0);

  client_thread.join();
  EXPECT_EQ(accept_count.load(), 1);
}

// saved_errno 测试

TEST_F(ReactorAccTest, AcceptWithSavedErrno) {
  ReactorAcc reactor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      10);

  ServerSocket server(kTestPort + 8, false, true, 10);
  reactor.SetListenSock(std::move(server));

  int saved_errno = 0;
  int ret = reactor.Accept(100, &saved_errno);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(saved_errno, 0);
}

// Unix Domain Socket 测试

TEST_F(ReactorAccTest, UnixDomainSocket) {
  const char* socket_path = "/tmp/test_reactor_acc.sock";
  unlink(socket_path);

  std::atomic<int> accepted_count{0};

  ReactorAcc reactor(
      [&accepted_count](int conn_fd, struct sockaddr* addr, int addr_len) {
        accepted_count++;
        close(conn_fd);
      },
      10);

  ServerSocket server(socket_path, true);
  ASSERT_TRUE(server.IsValid());
  reactor.SetListenSock(std::move(server));

  std::thread client_thread([&]() {
    usleep(50000);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    usleep(50000);
    close(fd);
  });

  usleep(100000);
  int ret = reactor.Accept(1000);
  EXPECT_GE(ret, 0);

  client_thread.join();
  EXPECT_EQ(accepted_count.load(), 1);

  unlink(socket_path);
}

// 连接后通信测试

TEST_F(ReactorAccTest, ConnectionCommunication) {
  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  ReactorAcc reactor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      10);

  ServerSocket server(kTestPort + 9, false, true, 10);
  reactor.SetListenSock(std::move(server));

  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 9);

    // 发送数据
    send(client.GetFd(), "hello", 5, 0);

    usleep(100000);
  });

  usleep(100000);
  reactor.Accept(1000);

  client_thread.join();

  // 验证收到了连接
  EXPECT_EQ(accepted_count.load(), 1);
  EXPECT_EQ(client_fds.size(), 1);

  // 如果有客户端 fd，可以读取数据验证
  if (!client_fds.empty()) {
    char buf[10] = {0};
    recv(client_fds[0], buf, 5, 0);
    EXPECT_STREQ(buf, "hello");
  }

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
}
