#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <thread>
#include <vector>

#include "acceptor.h"
#include "socket_wrap.h"

class AcceptorTest : public ::testing::Test {
 protected:
  static constexpr uint16_t kTestPort = 19100;

  void SetUp() override {}
  void TearDown() override {}
};

// 基本构造测试

TEST_F(AcceptorTest, ConstructorWithLT) {
  int callback_called = 0;
  Acceptor acceptor(
      [&callback_called](int conn_fd, struct sockaddr* addr, int addr_len) {
        callback_called++;
        close(conn_fd);
      },
      false);  // LT 模式
  // 构造成功即可
}

TEST_F(AcceptorTest, ConstructorWithET) {
  Acceptor acceptor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); },
      true);  // ET 模式
}

TEST_F(AcceptorTest, ConstructorDefaultET) {
  Acceptor acceptor(
      [](int conn_fd, struct sockaddr* addr, int addr_len) { close(conn_fd); });
  // 默认 ET 模式
}

// 单连接测试 (LT 模式)

TEST_F(AcceptorTest, AcceptSingleConnectionLT) {
  ServerSocket server(kTestPort, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  Acceptor acceptor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      false);  // LT 模式

  // 客户端连接
  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort);
    usleep(100000);
  });

  usleep(100000);

  // 处理连接
  int ret = acceptor.DealConnFromSock(server.GetFd());
  EXPECT_EQ(ret, 0);

  client_thread.join();

  EXPECT_EQ(accepted_count.load(), 1);
  EXPECT_EQ(client_fds.size(), 1);

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
}

// 多连接测试 (LT 模式)

TEST_F(AcceptorTest, AcceptMultipleConnectionsLT) {
  ServerSocket server(kTestPort + 1, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  Acceptor acceptor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      false);

  constexpr int kNumClients = 3;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < kNumClients; ++i) {
    client_threads.emplace_back([&]() {
      usleep(50000);
      ClientSocket client;
      client.Connect("127.0.0.1", kTestPort + 1);
      usleep(100000);
    });
  }

  usleep(100000);

  // LT 模式每次只处理一个连接
  for (int i = 0; i < kNumClients; ++i) {
    acceptor.DealConnFromSock(server.GetFd());
    usleep(10000);
  }

  for (auto& t : client_threads) {
    t.join();
  }

  EXPECT_EQ(accepted_count.load(), kNumClients);

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
}

// 多连接测试 (ET 模式)

TEST_F(AcceptorTest, AcceptMultipleConnectionsET) {
  ServerSocket server(kTestPort + 2, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  Acceptor acceptor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      true);  // ET 模式

  constexpr int kNumClients = 5;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < kNumClients; ++i) {
    client_threads.emplace_back([&]() {
      usleep(50000);
      ClientSocket client;
      client.Connect("127.0.0.1", kTestPort + 2);
      usleep(100000);
    });
  }

  usleep(200000);

  // ET 模式一次处理所有待处理的连接
  int ret = acceptor.DealConnFromSock(server.GetFd());
  EXPECT_EQ(ret, 0);

  for (auto& t : client_threads) {
    t.join();
  }

  EXPECT_EQ(accepted_count.load(), kNumClients);

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
}

// 回调参数测试

TEST_F(AcceptorTest, CallbackParameters) {
  ServerSocket server(kTestPort + 3, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  int captured_fd = -1;
  struct sockaddr_storage captured_addr;
  int captured_len = -1;

  Acceptor acceptor(
      [&](int conn_fd, struct sockaddr* addr, int addr_len) {
        captured_fd = conn_fd;
        if (addr) {
          memcpy(&captured_addr, addr, addr_len);
        }
        captured_len = addr_len;
      },
      false);

  std::thread client_thread([&]() {
    usleep(50000);
    ClientSocket client;
    client.Connect("127.0.0.1", kTestPort + 3);
    usleep(100000);
  });

  usleep(100000);
  acceptor.DealConnFromSock(server.GetFd());

  client_thread.join();

  EXPECT_GE(captured_fd, 0);
  EXPECT_GT(captured_len, 0);

  // 验证地址结构
  struct sockaddr_in* addr_in = (struct sockaddr_in*)&captured_addr;
  EXPECT_EQ(addr_in->sin_family, AF_INET);
  EXPECT_NE(addr_in->sin_port, 0);

  close(captured_fd);
}

// 无连接时测试

TEST_F(AcceptorTest, NoConnection) {
  ServerSocket server(kTestPort + 4, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  int callback_called = 0;

  Acceptor acceptor(
      [&callback_called](int conn_fd, struct sockaddr* addr, int addr_len) {
        callback_called++;
        close(conn_fd);
      },
      true);

  // 没有客户端连接，应该返回 0
  int ret = acceptor.DealConnFromSock(server.GetFd());
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(callback_called, 0);
}

// 错误 socket 测试

TEST_F(AcceptorTest, InvalidSocket) {
  int callback_called = 0;

  Acceptor acceptor(
      [&callback_called](int conn_fd, struct sockaddr* addr, int addr_len) {
        callback_called++;
      },
      true);

  // 无效的 socket fd
  int ret = acceptor.DealConnFromSock(-1);
  EXPECT_EQ(ret, -1);
  EXPECT_EQ(callback_called, 0);
}

// Unix Domain Socket 测试

TEST_F(AcceptorTest, AcceptUnixSocket) {
  const char* socket_path = "/tmp/test_acceptor.sock";
  unlink(socket_path);

  ServerSocket server(socket_path, true);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accepted_count{0};
  std::vector<int> client_fds;

  Acceptor acceptor(
      [&accepted_count, &client_fds](int conn_fd, struct sockaddr* addr,
                                     int addr_len) {
        accepted_count++;
        client_fds.push_back(conn_fd);
      },
      true);

  std::thread client_thread([&]() {
    usleep(50000);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    usleep(100000);
    close(fd);
  });

  usleep(100000);
  int ret = acceptor.DealConnFromSock(server.GetFd());
  EXPECT_EQ(ret, 0);

  client_thread.join();

  EXPECT_EQ(accepted_count.load(), 1);

  // 清理
  for (int fd : client_fds) {
    close(fd);
  }
  unlink(socket_path);
}

// 并发测试

TEST_F(AcceptorTest, ConcurrentAccept) {
  ServerSocket server(kTestPort + 5, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::atomic<int> accepted_count{0};

  Acceptor acceptor(
      [&accepted_count](int conn_fd, struct sockaddr* addr, int addr_len) {
        accepted_count++;
        usleep(10000);  // 模拟处理时间
        close(conn_fd);
      },
      true);

  constexpr int kNumClients = 10;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < kNumClients; ++i) {
    client_threads.emplace_back([&]() {
      usleep(rand() % 100000);
      ClientSocket client;
      client.Connect("127.0.0.1", kTestPort + 5);
      usleep(50000);
    });
  }

  // 多次调用 DealConnFromSock
  for (int i = 0; i < kNumClients; ++i) {
    usleep(20000);
    acceptor.DealConnFromSock(server.GetFd());
  }

  for (auto& t : client_threads) {
    t.join();
  }

  EXPECT_EQ(accepted_count.load(), kNumClients);
}