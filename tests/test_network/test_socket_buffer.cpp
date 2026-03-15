#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>

#include "socket_buffer.h"
#include "socket_wrap.h"

class SocketBufferTest : public ::testing::Test {
 protected:
  static constexpr uint16_t kTestPort = 19000;

  void SetUp() override {}
  void TearDown() override {}
};

// 基本功能测试

TEST_F(SocketBufferTest, DefaultConstructor) {
  SocketBuffer buf;
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_EQ(buf.Writable(), 0);
}

TEST_F(SocketBufferTest, ConstructorWithSize) {
  SocketBuffer buf(1024);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_GE(buf.Writable(), 1024);
}

TEST_F(SocketBufferTest, ReadFromInvalidSocket) {
  SocketBuffer buf;
  int saved_errno = 0;
  int ret = buf.ReadFromSock(-1, false, &saved_errno);
  EXPECT_EQ(ret, -1);
}

TEST_F(SocketBufferTest, WriteToInvalidSocket) {
  SocketBuffer buf;
  int saved_errno = 0;
  int ret = buf.WriteToSock(-1, false, &saved_errno);
  EXPECT_EQ(ret, -1);
}

// 阻塞模式测试

TEST_F(SocketBufferTest, ReadFromSockBlocking) {
  // 创建服务端
  ServerSocket server(kTestPort, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    ASSERT_GE(client_fd, 0);

    // 发送测试数据
    send(client_fd, "hello", 5, 0);
    close(client_fd);
  });

  // 等待服务端准备好
  usleep(100000);  // 100ms

  // 客户端连接并读取
  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort), 0);

  SocketBuffer buf;
  int saved_errno = 0;
  int ret = buf.ReadFromSock(client.GetFd(), false, &saved_errno);

  EXPECT_GT(ret, 0);
  EXPECT_EQ(buf.Readable(), 5);

  // 验证读取的数据
  char data[10] = {0};
  buf.Read(data, 5);
  EXPECT_STREQ(data, "hello");

  server_thread.join();
}

TEST_F(SocketBufferTest, WriteToSockBlocking) {
  ServerSocket server(kTestPort + 1, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    ASSERT_GE(client_fd, 0);

    // 接收数据
    char data[10] = {0};
    recv(client_fd, data, 5, 0);
    EXPECT_STREQ(data, "hello");
    close(client_fd);
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 1), 0);

  // 写入数据到 buffer，然后发送
  SocketBuffer buf;
  buf.Write("hello", 5);
  EXPECT_EQ(buf.Readable(), 5);

  int saved_errno = 0;
  int ret = buf.WriteToSock(client.GetFd(), false, &saved_errno);

  EXPECT_EQ(ret, 5);
  EXPECT_EQ(buf.Readable(), 0);

  server_thread.join();
}

// 非阻塞模式测试

TEST_F(SocketBufferTest, ReadFromSockNonBlocking) {
  ServerSocket server(kTestPort + 2, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    usleep(100000);  // 等待客户端连接
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    ASSERT_GE(client_fd, 0);

    // 发送数据
    send(client_fd, "test_data", 9, 0);
    usleep(50000);
    close(client_fd);
  });

  usleep(100000);

  ClientSocket client(false, true);  // 非阻塞
  ASSERT_TRUE(client.IsValid());
  client.Connect("127.0.0.1", kTestPort + 2);

  SocketBuffer buf;
  int saved_errno = 0;

  // 非阻塞读取可能返回 0（无数据）
  int ret = buf.ReadFromSock(client.GetFd(), true, &saved_errno);
  // ET 模式会循环读取，直到 EAGAIN

  // 等待数据到达
  usleep(200000);

  ret = buf.ReadFromSock(client.GetFd(), true, &saved_errno);
  EXPECT_GT(ret, 0);

  server_thread.join();
}

// 使用 SocketWrap 接口测试

TEST_F(SocketBufferTest, ReadFromSockWithSocketWrap) {
  ServerSocket server(kTestPort + 3, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      send(client_fd, "wrap_test", 9, 0);
      close(client_fd);
    }
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 3), 0);

  SocketBuffer buf;
  int saved_errno = 0;
  int ret = buf.ReadFromSock(client, &saved_errno);  // 使用 SocketWrap 引用

  EXPECT_GT(ret, 0);
  EXPECT_EQ(buf.Readable(), 9);

  server_thread.join();
}

TEST_F(SocketBufferTest, WriteToSockWithSocketWrap) {
  ServerSocket server(kTestPort + 4, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      char data[20] = {0};
      recv(client_fd, data, 10, 0);
      EXPECT_STREQ(data, "wrap_write");
      close(client_fd);
    }
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 4), 0);

  SocketBuffer buf;
  buf.Write("wrap_write", 10);

  int saved_errno = 0;
  int ret = buf.WriteToSock(client, &saved_errno);  // 使用 SocketWrap 引用

  EXPECT_EQ(ret, 10);
  EXPECT_EQ(buf.Readable(), 0);

  server_thread.join();
}

// 连接关闭测试

TEST_F(SocketBufferTest, ReadFromSockConnectionClosed) {
  ServerSocket server(kTestPort + 5, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      // 立即关闭连接
      close(client_fd);
    }
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 5), 0);

  SocketBuffer buf;
  int saved_errno = 0;

  usleep(100000);  // 等待服务端关闭

  int ret = buf.ReadFromSock(client.GetFd(), false, &saved_errno);
  EXPECT_EQ(ret, 0);  // 连接关闭返回 0

  server_thread.join();
}

// 大数据量测试

TEST_F(SocketBufferTest, LargeDataTransfer) {
  ServerSocket server(kTestPort + 6, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  constexpr size_t kDataSize = 10000;
  std::string large_data(kDataSize, 'x');

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      // 接收所有数据
      size_t total = 0;
      while (total < kDataSize) {
        char buf[1024];
        int n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        total += n;
      }
      EXPECT_EQ(total, kDataSize);
      close(client_fd);
    }
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 6), 0);

  SocketBuffer buf;
  buf.Write(large_data.c_str(), kDataSize);

  int saved_errno = 0;
  int ret = buf.WriteToSock(client.GetFd(), false, &saved_errno);

  EXPECT_EQ(ret, kDataSize);
  EXPECT_EQ(buf.Readable(), 0);

  server_thread.join();
}

// 双向通信测试

TEST_F(SocketBufferTest, BidirectionalCommunication) {
  ServerSocket server(kTestPort + 7, false, false, 10);
  ASSERT_TRUE(server.IsValid());

  std::thread server_thread([&]() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = server.Accept((struct sockaddr*)&client_addr, &addr_len);
    if (client_fd >= 0) {
      // 接收请求
      char req[10] = {0};
      recv(client_fd, req, 5, 0);
      EXPECT_STREQ(req, "ping");

      // 发送响应
      send(client_fd, "pong", 4, 0);
      close(client_fd);
    }
  });

  usleep(100000);

  ClientSocket client;
  ASSERT_TRUE(client.IsValid());
  ASSERT_EQ(client.Connect("127.0.0.1", kTestPort + 7), 0);

  // 发送请求
  SocketBuffer send_buf;
  send_buf.Write("ping", 5);
  int saved_errno = 0;
  send_buf.WriteToSock(client.GetFd(), false, &saved_errno);

  // 接收响应
  SocketBuffer recv_buf;
  recv_buf.ReadFromSock(client.GetFd(), false, &saved_errno);

  char resp[10] = {0};
  recv_buf.Read(resp, 4);
  EXPECT_STREQ(resp, "pong");

  server_thread.join();
}
