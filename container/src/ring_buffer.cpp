#include "ring_buffer.h"

#include "memory.h"

const size_t RingBuffer::kExpandFactor = 2;
const int RingBuffer::CompactRate::kScale = 10;
const int RingBuffer::CompactRate::kMinAllowedRate = 3;

RingBuffer::RingBuffer()
    : data_(nullptr),
      read_ptr_(nullptr),
      write_ptr_(nullptr),
      readable_(0),
      writeable_(0),
      size_(0) {}

RingBuffer::~RingBuffer() {
  if (data_) {
    delete[] data_;
    data_ = nullptr;
    read_ptr_ = 0;
    write_ptr_ = 0;
  }
}

int RingBuffer::Read(char* dest, size_t dest_size) {
  if (!dest) return 0;
  size_t read_len = readable_ > dest_size ? dest_size : readable_;
  memcpy(dest, read_ptr_, read_len);
  ReadOut(read_len);
  return read_len;
}

int RingBuffer::Write(const char* src, size_t src_size) {
  if (!src) return 0;
  EnsureWritableSize(src_size);
  memcpy(write_ptr_, src, src_size);
  Written(src_size);
  return src_size;
}

int RingBuffer::Read(RingBuffer* dest) {
  if (!dest) return 0;
  size_t read_len = readable_;
  dest->EnsureWritableSize(read_len);
  memcpy(dest->write_ptr_, read_ptr_, read_len);
  ReadOut(read_len);
  dest->Written(read_len);
  return read_len;
}

int RingBuffer::Write(RingBuffer* src) {
  if (!src) return 0;
  return src->Read(this);
}

void RingBuffer::Clear() {
  read_ptr_ = data_;
  write_ptr_ = data_;
  readable_ = 0;
  writeable_ = size_;
}

int RingBuffer::Resize(size_t size) {
  if (size == size_) return 0;
  if (size > size_)
    Expand(size, true);
  else
    Shrink(size);
  return 0;
}

void RingBuffer::ReadOut(size_t len) {
  read_ptr_ += len;
  readable_ -= len;
}

void RingBuffer::Written(size_t len) {
  write_ptr_ += len;
  writeable_ -= len;
  readable_ += len;
}

void RingBuffer::EnsureWritableSize(size_t size) {
  if (writeable_ >= size) {
    compact_rate_.UpdateStatsAndCheck(false);
    return;
  }
  if (UnusedSize() >= size) {
    if (compact_rate_.UpdateStatsAndCheck(true)) {
      Compact();
      return;
    }
  }

  Expand(size, false);
}

void RingBuffer::Compact() {
  memcpy(data_, read_ptr_, readable_);
  read_ptr_ = data_;
  write_ptr_ = read_ptr_ + readable_;
  writeable_ = size_ - readable_;
}

void RingBuffer::Expand(size_t size, bool fixed) {
  size_t new_size;
  if (fixed) {
    new_size = size;
  } else {
    new_size = size_ * kExpandFactor + size;
  }

  char* new_data = new char[new_size];
  size_ = new_size;

  memcpy(new_data, read_ptr_, readable_);
  read_ptr_ = new_data;
  write_ptr_ = read_ptr_ + readable_;
  writeable_ = size_ - readable_;

  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
  data_ = new_data;
  new_data = nullptr;

  compact_rate_.Reset();
}

void RingBuffer::Shrink(size_t size) {
  readable_ = readable_ > size ? size : readable_;
  char* new_data = new char[size];
  size_ = size;

  memcpy(new_data, read_ptr_, readable_);
  read_ptr_ = new_data;
  write_ptr_ = read_ptr_ + readable_;
  writeable_ = size_ - readable_;

  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
  data_ = new_data;
  new_data = nullptr;

  compact_rate_.Reset();
}
