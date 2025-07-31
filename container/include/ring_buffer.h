#ifndef CONTAINER_RINGBUFFER_H_
#define CONTAINER_RINGBUFFER_H_

#include <stddef.h>
#include <stdint.h>

class RingBuffer {
 public:
  RingBuffer();
  virtual ~RingBuffer();

  int Read(char* dest, size_t dest_size);
  int Write(const char* src, size_t src_size);
  int Read(RingBuffer* dest);
  int Write(RingBuffer* src);
  void Clear();
  int Resize(size_t size);

  size_t Readable() { return readable_; }
  size_t Writable() { return writeable_; }
  size_t UnusedSize() { return size_ - readable_; }
  size_t Size() { return size_; }
  void Expand(size_t size) { return Expand(size, false); }
  void EnsureWritableSize(size_t size);
  const char* GetReadPtr() const { return read_ptr_; }

  char* GetWritePtr() { return write_ptr_; }
  void ReadOut(size_t len);
  void Written(size_t len);

 private:
  void Compact();
  void Expand(size_t size, bool fixed);
  void Shrink(size_t size);

 private:
  class CompactRate {
   public:
    CompactRate() : compact_cnt(1), total_cnt(kScale) {}
    ~CompactRate() = default;
    CompactRate(const CompactRate&) = delete;
    CompactRate& operator=(const CompactRate&) = delete;

    bool UpdateStatsAndCheck(bool is_compact) {
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
