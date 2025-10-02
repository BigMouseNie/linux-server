#include <gtest/gtest.h>

#include <string>

#include "ring_buffer.h"

class RingBufferTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override { buf.Resize(0); }

  RingBuffer buf;
};

TEST_F(RingBufferTest, InitialState) {
  EXPECT_TRUE(buf.GetReadPtr() == nullptr);
  EXPECT_TRUE(buf.GetWritePtr() == nullptr);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_EQ(buf.Writable(), 0);
  EXPECT_EQ(buf.Size(), 0);
}

TEST_F(RingBufferTest, WriteRaw) {
  std::string str("abcdefg");
  int len = buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(len, str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_EQ(buf.Readable() + buf.Writable(), buf.Size());
  EXPECT_TRUE(buf.GetRawReadPtr() + buf.Readable() == buf.GetWritePtr());
  EXPECT_STREQ(buf.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, WriteRingBuffer01) {
  std::string str("abcdefg");
  int len = buf.Write(nullptr);
  EXPECT_EQ(len, 0);
  EXPECT_EQ(buf.Readable(), 0);
}

TEST_F(RingBufferTest, WriteRingBuffer02) {
  RingBuffer tmp;
  std::string str("abcdefg");
  tmp.Write(str.c_str(), str.size() + 1);
  int len = buf.Write(&tmp);
  EXPECT_EQ(tmp.Readable(), 0);
  EXPECT_EQ(len, str.size() + 1);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_STREQ(buf.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, PeekRaw01) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  char tmp[64];
  int len = buf.Peek(tmp, 64);
  EXPECT_EQ(len, str.size() + 1);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_STREQ(buf.GetReadPtr(), tmp);
}

TEST_F(RingBufferTest, PeekRaw02) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  char tmp[64];
  int len = buf.Peek(tmp, 3);
  EXPECT_EQ(len, 3);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  tmp[3] = '\0';
  EXPECT_STREQ(tmp, "abc");
}

TEST_F(RingBufferTest, ReadRaw01) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  char tmp[64];
  int len = buf.Read(tmp, 64);
  EXPECT_EQ(len, str.size() + 1);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_STREQ(tmp, str.c_str());
}

TEST_F(RingBufferTest, ReadRaw02) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  char tmp[64];
  int len = buf.Read(tmp, 3);
  EXPECT_EQ(len, 3);
  EXPECT_EQ(buf.Readable(), str.size() + 1 - 3);
  tmp[3] = '\0';
  EXPECT_STREQ(tmp, "abc");
  EXPECT_STREQ(buf.GetReadPtr(), "defg");
}

TEST_F(RingBufferTest, ReadRingBuffer01) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  int len = buf.Read(nullptr);
  EXPECT_EQ(len, 0);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_STREQ(buf.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, ReadRingBuffer02) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  RingBuffer tmp;
  int len = buf.Read(&tmp);
  EXPECT_EQ(len, str.size() + 1);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_EQ(tmp.Readable(), str.size() + 1);
  EXPECT_STREQ(tmp.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, Clear01) {
  buf.Clear();
  EXPECT_TRUE(buf.GetReadPtr() == buf.GetWritePtr());
  EXPECT_EQ(buf.Size(), buf.Writable());
  EXPECT_EQ(buf.Readable(), 0);
}

TEST_F(RingBufferTest, Clear02) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  buf.Clear();
  EXPECT_TRUE(buf.GetReadPtr() == buf.GetWritePtr());
  EXPECT_EQ(buf.Size(), buf.Writable());
  EXPECT_EQ(buf.Readable(), 0);
}

TEST_F(RingBufferTest, Resize01) {
  int res = buf.Resize(0);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), 0);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_EQ(buf.Writable(), 0);
  EXPECT_TRUE(buf.GetWritePtr() == nullptr);
  EXPECT_TRUE(buf.GetReadPtr() == nullptr);
}

TEST_F(RingBufferTest, Resize02) {
  int res = buf.Resize(256);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), 256);
  EXPECT_EQ(buf.Readable(), 0);
  EXPECT_EQ(buf.Writable(), 256);
  EXPECT_TRUE(buf.GetWritePtr() == buf.GetReadPtr());
}

TEST_F(RingBufferTest, Resize03) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  int newSize = buf.Size() + 256;
  int res = buf.Resize(newSize);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), newSize);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_EQ(buf.Writable(), newSize - (str.size() + 1));
  EXPECT_TRUE(buf.GetWritePtr() == buf.GetReadPtr() + str.size() + 1);
  EXPECT_STREQ(buf.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, Resize04) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  int newSize = buf.Size();
  int res = buf.Resize(buf.Size());
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), newSize);
  EXPECT_EQ(buf.Readable(), str.size() + 1);
  EXPECT_EQ(buf.Writable(), newSize - (str.size() + 1));
  EXPECT_TRUE(buf.GetWritePtr() == buf.GetReadPtr() + str.size() + 1);
  EXPECT_STREQ(buf.GetReadPtr(), str.c_str());
}

TEST_F(RingBufferTest, Resize05) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  char tmp[64];
  buf.Read(tmp, 3);
  int newSize = buf.Size() + 256;
  int res = buf.Resize(newSize);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), newSize);
  EXPECT_EQ(buf.Readable(), str.size() + 1 - 3);
  EXPECT_EQ(buf.Writable(), newSize - (str.size() + 1 - 3));
  EXPECT_TRUE(buf.GetWritePtr() == buf.GetReadPtr() + str.size() + 1 - 3);
  EXPECT_STREQ(buf.GetReadPtr(), "defg");
}

TEST_F(RingBufferTest, Resize06) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  char tmp[64];
  buf.Read(tmp, 3);
  int newSize = buf.Size();
  const char* read_ptr = buf.GetReadPtr();
  const char* write_ptr = buf.GetWritePtr();
  int res = buf.Resize(newSize);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(buf.Size(), newSize);
  EXPECT_EQ(buf.Readable(), str.size() + 1 - 3);
  EXPECT_EQ(buf.Writable(), newSize - (str.size() + 1));
  EXPECT_TRUE(buf.GetWritePtr() == buf.GetReadPtr() + str.size() + 1 - 3);
  EXPECT_STREQ(buf.GetReadPtr(), "defg");
  EXPECT_TRUE(read_ptr == buf.GetReadPtr());
  EXPECT_TRUE(write_ptr == buf.GetWritePtr());
}

TEST_F(RingBufferTest, Resize07) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  int res = buf.Resize(3);
  EXPECT_EQ(buf.Size(), 3);
  buf.Write("\0", 1);
  EXPECT_STREQ(buf.GetReadPtr(), "abc");
}

TEST_F(RingBufferTest, Resize08) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  buf.ReadOut(3);
  EXPECT_EQ(buf.Size(), (str.size() + 1) * RingBuffer::kExpandFactor);
  int res = buf.Resize(5);
  EXPECT_EQ(buf.Size(), 5);
  buf.Write("\0", 1);
  EXPECT_STREQ(buf.GetReadPtr(), "defg");
}

TEST_F(RingBufferTest, CompactRate) {
  std::string str("abcdefg");
  buf.Write(str.c_str(), str.size() + 1);
  int size = buf.Size();
  buf.ReadOut(1);
  buf.EnsureWritableSize(size - 7);
  buf.ReadOut(1);
  buf.EnsureWritableSize(size - 6);
  buf.ReadOut(1);
  buf.EnsureWritableSize(size - 5);
  for (int i = 0; i < 7; ++i) {
    buf.EnsureWritableSize(1);
  }
  EXPECT_STREQ(buf.GetReadPtr(), "defg");
  EXPECT_TRUE(buf.Size() > size);
}
