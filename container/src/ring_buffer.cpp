#include "ring_buffer.h"

#include <memory.h>
#include <stdlib.h>

const size_t RingBuffer::kExpandFactor = 2;
const int RingBuffer::CompactRate::kScale = 10;
const int RingBuffer::CompactRate::kMinAllowedRate = 5;

RingBuffer::RingBuffer()
    : data_(nullptr),
      read_ptr_(nullptr),
      write_ptr_(nullptr),
      readable_(0),
      writeable_(0),
      size_(0) {}

RingBuffer::~RingBuffer() {
  if (data_) {
    free(data_);
    data_ = nullptr;
    read_ptr_ = 0;
    write_ptr_ = 0;
  }
}

int RingBuffer::Peek(char* dest, size_t dest_size) const {
  if (!dest) return 0;
  size_t read_len = readable_ > dest_size ? dest_size : readable_;
  memcpy(dest, read_ptr_, read_len);
  return read_len;
}

int RingBuffer::Read(char* dest, size_t dest_size) {
  size_t read_len = Peek(dest, dest_size);
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

int RingBuffer::Resize(size_t size) {
  if (size == size_) return 0;
  compact_rate_.Reset();

  if (size == 0) {
    free(data_);
    data_ = nullptr;
    size_ = size;
    Clear();
    return 0;
  }

  if (size_ == 0) {
    data_ = (char*)malloc(sizeof(char) * size);
    if (!data_) return -1;
    size_ = size;
    Clear();
    return 0;
  }

  Compact();
  void* tmp = realloc(data_, size);
  if (!tmp) return -2;
  data_ = (char*)tmp;
  read_ptr_ = data_;
  readable_ = readable_ > size ? size : readable_;
  write_ptr_ = read_ptr_ + readable_;
  writeable_ = size - readable_;
  size_ = size;
  return 0;
}

int RingBuffer::EnsureWritableSize(size_t size) {
  if (writeable_ >= size) {
    if (compact_rate_.UpdateStatsAndCheck(false)) {
      return 0;
    }
  }
  else if (UnusedSize() >= size) {
    if (compact_rate_.UpdateStatsAndCheck(true)) {
      Compact();
      return 0;
    }
  }

  return Resize((size + size_) * kExpandFactor);  // expand
}

void RingBuffer::Compact() {
  if (data_ == read_ptr_) return;
  memcpy(data_, read_ptr_, readable_);
  read_ptr_ = data_;
  write_ptr_ = read_ptr_ + readable_;
  writeable_ = size_ - readable_;
}
