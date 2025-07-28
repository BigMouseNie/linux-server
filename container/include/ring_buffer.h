#ifndef CONTAINER_RINGBUFFER_H_
#define CONTAINER_RINGBUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <cassert>

class RingBuffer {
 public:
  RingBuffer();
  ~RingBuffer();

  int Read(char* dest, size_t dest_size);
  int Write(const char* src, size_t src_size);
  int Read(RingBuffer& dest);
  int Write(RingBuffer& src);
  void Clear();

  inline size_t Readable() { return readable_; }
  inline size_t Writable() { return size_ - readable_; }
  inline size_t Size() { return size_; }

 private:
  void ReadOut(size_t len);
  void Written(size_t len);

  void EnsureWritableSize(size_t size);
  void Compact();
  void Expand(size_t size);

 private:
  class CompactRate {
   public:
    CompactRate() : compact_cnt(1), total_cnt(kScale) {}
    ~CompactRate() = default;
    CompactRate(const CompactRate&) = delete;
    CompactRate& operator=(const CompactRate&) = delete;

    bool UpdateStatsAndCheck(bool is_compact) {
      assert(compact_cnt > 0 && total_cnt > 0);
      ++total_cnt;
      if (total_cnt > kScale) {
        Reset();
        return true;
      }

      if (!is_compact) {
        return true;
      }

      ++compact_cnt;
      return (total_cnt / compact_cnt) > kMinAllowedRate;
    }

    void Reset() {
      compact_cnt = 1;
      total_cnt = kScale;
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

  static const size_t kExpandFactor;
};

#endif  // CONTAINER_RINGBUFFER_H_
