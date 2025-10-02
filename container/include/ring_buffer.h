#ifndef CONTAINER_RINGBUFFER_H_
#define CONTAINER_RINGBUFFER_H_

#include <stddef.h>
#include <stdint.h>

class RingBuffer {
 public:
  RingBuffer();
  virtual ~RingBuffer();

  int Peek(char* dest, size_t dest_size) const;
  int Read(char* dest, size_t dest_size);
  int Write(const char* src, size_t src_size);
  int Read(RingBuffer* dest);
  int Write(RingBuffer* src);

  size_t Readable() const { return readable_; }
  size_t Writable() const { return writeable_; }
  size_t UnusedSize() const { return size_ - readable_; }
  size_t Size() const { return size_; }

  const char* GetReadPtr() const { return read_ptr_; }
  char* GetRawReadPtr() { return read_ptr_; }
  char* GetWritePtr() { return write_ptr_; }
  void ReadOut(size_t len) {
    read_ptr_ += len;
    readable_ -= len;
  }
  void Written(size_t len) {
    write_ptr_ += len;
    writeable_ -= len;
    readable_ += len;
  }
  void Clear() {
    read_ptr_ = data_;
    write_ptr_ = data_;
    readable_ = 0;
    writeable_ = size_;
  }

  int Resize(size_t size);
  int EnsureWritableSize(size_t size);

 public:
  static const size_t kExpandFactor;

 private:
  void Compact();

 private:
  class CompactRate {
   public:
    CompactRate() : compact_cnt(0), total_cnt(0) {}
    ~CompactRate() = default;
    CompactRate(const CompactRate&) = delete;
    CompactRate& operator=(const CompactRate&) = delete;

    bool UpdateStatsAndCheck(bool is_compact) {
      ++total_cnt;
      if (is_compact) ++compact_cnt;
      if (total_cnt == kScale) {
        bool res = true;
        if (compact_cnt != 0) {
          res = total_cnt / compact_cnt >= 5; 
        }
        Reset();
        return res;
      }
      return true;
    }

    void Reset() {
      compact_cnt = 0;
      total_cnt = 0;
    }

   private:
    int compact_cnt;
    int total_cnt;
    static const int kScale;
    static const int kMinAllowedRate;
  };
  CompactRate compact_rate_;
  char* data_;
  char* read_ptr_;
  char* write_ptr_;
  size_t readable_;
  size_t writeable_;
  size_t size_;
};

#endif  // CONTAINER_RINGBUFFER_H_
