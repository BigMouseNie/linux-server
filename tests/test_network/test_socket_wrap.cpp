#include <arpa/inet.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "socket_wrap.h"

using namespace std::chrono_literals;

class SocketWrapTest : public ::testing::Test {
 protected:
  static constexpr uint16_t kTestPort = 18999;
  static constexpr const char* kTestPath = "/tmp/test_socket_wrap.sock";

  void SetUp() override {
    // 清理可能的残留文件
    unlink(kTestPath);
  }

  void TearDown() override {
    // 清理测试文件
    unlink(kTestPath);
  }
};

// SocketWrap tests
TEST_F(SocketWrapTest, DefaultConstructor) {
  SocketWrap sock;
  EXPECT_EQ(sock.GetFd(), -1);
  EXPECT_FALSE(sock.IsValid());
}

TEST_F(SocketWrapTest, ConstructorWithFd) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd, 0);

  SocketWrap sock(fd);
  EXPECT_EQ(sock.GetFd(), fd);
  EXPECT_TRUE(sock.IsValid());
}

TEST_F(SocketWrapTest, MoveConstructor) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd, 0);

  SocketWrap sock1(fd);
  EXPECT_EQ(sock1.GetFd(), fd);

  SocketWrap sock2(std::move(sock1));
  EXPECT_EQ(sock2.GetFd(), fd);
  EXPECT_EQ(sock1.GetFd(), -1);
  EXPECT_FALSE(sock1.IsValid());
}

TEST_F(SocketWrapTest, MoveAssignment) {
  int fd1 = socket(AF_INET, SOCK_STREAM, 0);
  int fd2 = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd1, 0);
  ASSERT_GE(fd2, 0);

  SocketWrap sock1(fd1);
  SocketWrap sock2(fd2);

  sock2 = std::move(sock1);
  EXPECT_EQ(sock2.GetFd(), fd1);
  EXPECT_EQ(sock1.GetFd(), -1);
  close(fd2);  // sock2 原来的 fd 需要手动关闭
}

TEST_F(SocketWrapTest, Release) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd, 0);

  SocketWrap sock(fd);
  int released_fd = sock.Release();
  EXPECT_EQ(released_fd, fd);
  EXPECT_EQ(sock.GetFd(), -1);
  EXPECT_FALSE(sock.IsValid());

  // 手动关闭释放的 fd
  close(released_fd);
}

TEST_F(SocketWrapTest, ManualMgmtNoClose) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd, 0);

  SocketWrap sock(fd);
  sock.SetManualMgmt(true);  // 手动管理，不自动关闭

  // 析构时不会自动关闭
  sock.~SocketWrap();

  // 检查 fd 是否有效
  int result = fcntl(fd, F_GETFL, 0);
  EXPECT_GE(result, 0) << "fd should still be valid";

  close(fd);
}

TEST_F(SocketWrapTest, AutoCloseOnDestroy) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(fd, 0);

  {
    SocketWrap sock(fd);
    sock.SetManualMgmt(false);  // 自动管理
  }                             // sock 离开作用域，应该自动关闭

  // 检查 fd 是否已关闭
  int result = fcntl(fd, F_GETFL, 0);
  EXPECT_LT(result, 0) << "fd should be closed";
  EXPECT_EQ(errno, EBADF);
}

// ServerSocket tests
TEST_F(SocketWrapTest, ServerSocketTCP) {
  ServerSocket server(kTestPort, false, false, 10);
  EXPECT_TRUE(server.IsValid());
  EXPECT_GE(server.GetFd(), 0);
}

TEST_F(SocketWrapTest, ServerSocketTCPNonBlock) {
  ServerSocket server(kTestPort, false, true, 10);
  EXPECT_TRUE(server.IsValid());

  int flags = fcntl(server.GetFd(), F_GETFL, 0);
  EXPECT_TRUE(flags & O_NONBLOCK);
  EXPECT_TRUE(server.IsNonBlock());
}

TEST_F(SocketWrapTest, ServerSocketUnix) {
  ServerSocket server(kTestPath, false);
  EXPECT_TRUE(server.IsValid());
  EXPECT_GE(server.GetFd(), 0);

  // 检查 socket 文件是否存在
  struct stat st;
  EXPECT_EQ(stat(kTestPath, &st), 0);
}

TEST_F(SocketWrapTest, ServerSocketDestructorCleanup) {
  {
    ServerSocket server(kTestPath, false);
    EXPECT_TRUE(server.IsValid());
  }  // server 析构，应该 unlink socket 文件

  // 检查 socket 文件是否被删除
  struct stat st;
  EXPECT_NE(stat(kTestPath, &st), 0);
}

TEST_F(SocketWrapTest, ServerSocketAccept) {
  ServerSocket server(kTestPort, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  // 在另一个线程连接
  std::thread client_thread([this]() {
    std::this_thread::sleep_for(100ms);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(kTestPort);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    std::this_thread::sleep_for(100ms);
    close(fd);
  });

  // 接受连接
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);

  client_thread.join();

  EXPECT_GE(client_fd, 0);
  if (client_fd >= 0) {
    close(client_fd);
  }
}

// ClientSocket tests
TEST_F(SocketWrapTest, ClientSocketCreate) {
  ClientSocket client(false, false);
  EXPECT_TRUE(client.IsValid());
  EXPECT_GE(client.GetFd(), 0);
}

TEST_F(SocketWrapTest, ClientSocketNonBlock) {
  ClientSocket client(false, true);
  EXPECT_TRUE(client.IsValid());

  int flags = fcntl(client.GetFd(), F_GETFL, 0);
  EXPECT_TRUE(flags & O_NONBLOCK);
  EXPECT_TRUE(client.IsNonBlock());
}

TEST_F(SocketWrapTest, ClientSocketConnect) {
  // 启动服务端
  ServerSocket server(kTestPort, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread accept_thread([this, &server]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      char buf[128] = {0};
      recv(client_fd, buf, sizeof(buf), 0);
      send(client_fd, "world", 5, 0);
      close(client_fd);
    }
  });

  // 客户端连接
  std::this_thread::sleep_for(100ms);
  ClientSocket client(false, false);
  int ret = client.Connect("127.0.0.1", kTestPort);
  EXPECT_EQ(ret, 0);

  // 发送数据
  send(client.GetFd(), "hello", 5, 0);

  // 接收数据
  char buf[128] = {0};
  recv(client.GetFd(), buf, sizeof(buf), 0);
  EXPECT_STREQ(buf, "world");

  accept_thread.join();
}

TEST_F(SocketWrapTest, ClientSocketConnectInConstructor) {
  // 启动服务端
  ServerSocket server(kTestPort, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread accept_thread([this, &server]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      char buf[128] = {0};
      recv(client_fd, buf, sizeof(buf), 0);
      close(client_fd);
    }
  });

  // 客户端构造时连接
  std::this_thread::sleep_for(100ms);
  ClientSocket client("127.0.0.1", kTestPort, false, true);
  EXPECT_TRUE(client.IsValid());

  // 发送数据
  send(client.GetFd(), "test", 4, 0);

  accept_thread.join();
}

// Unix Domain Socket tests
TEST_F(SocketWrapTest, UnixSocketCommunication) {
  ServerSocket server(kTestPath, false);
  ASSERT_TRUE(server.IsValid());

  std::thread client_thread([this]() {
    std::this_thread::sleep_for(100ms);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, kTestPath, sizeof(addr.sun_path) - 1);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    send(fd, "unix_msg", 8, 0);
    std::this_thread::sleep_for(100ms);
    close(fd);
  });

  int client_fd = server.Accept();
  EXPECT_GE(client_fd, 0);

  if (client_fd >= 0) {
    char buf[128] = {0};
    recv(client_fd, buf, sizeof(buf), 0);
    EXPECT_STREQ(buf, "unix_msg");
    close(client_fd);
  }

  client_thread.join();
}

TEST_F(SocketWrapTest, ClientSocketUnixConnect) {
  ServerSocket server(kTestPath, false);
  ASSERT_TRUE(server.IsValid());

  std::thread accept_thread([this, &server]() {
    int client_fd = server.Accept();
    if (client_fd >= 0) {
      char buf[128] = {0};
      recv(client_fd, buf, sizeof(buf), 0);
      send(client_fd, "reply", 5, 0);
      close(client_fd);
    }
  });

  std::this_thread::sleep_for(100ms);
  ClientSocket client(kTestPath, false);
  ASSERT_TRUE(client.IsValid());

  send(client.GetFd(), "hello", 5, 0);

  char buf[128] = {0};
  recv(client.GetFd(), buf, sizeof(buf), 0);
  EXPECT_STREQ(buf, "reply");

  accept_thread.join();
}

// Multiple clients test
TEST_F(SocketWrapTest, MultipleClients) {
  ServerSocket server(kTestPort, false, true, 10);
  ASSERT_TRUE(server.IsValid());

  constexpr int kNumClients = 5;
  std::atomic<int> connected_count{0};

  std::thread accept_thread([this, &server, &connected_count]() {
    for (int i = 0; i < kNumClients; ++i) {
      int client_fd = server.Accept();
      if (client_fd >= 0) {
        connected_count++;
        close(client_fd);
      }
    }
  });

  std::vector<std::thread> clients;
  for (int i = 0; i < kNumClients; ++i) {
    clients.emplace_back([this]() {
      std::this_thread::sleep_for(100ms);
      int fd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      addr.sin_port = htons(kTestPort);
      connect(fd, (struct sockaddr*)&addr, sizeof(addr));
      std::this_thread::sleep_for(100ms);
      close(fd);
    });
  }

  for (auto& t : clients) {
    t.join();
  }
  accept_thread.join();

  EXPECT_EQ(connected_count, kNumClients);
}
