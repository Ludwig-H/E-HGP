#pragma once

// MorseHGP3D v9 (25 septembre 2026) — resident block pool of the lanes
// records (lever LanesInput::pinned_records, chain q34_lanes_pinned).
//
// The lanes call downloads its records (850 k at 08/000000 K5, 4.63 M at
// K10, 128 bytes each; receipts R20/R21) into host memory. Into a fresh
// pageable vector, the driver stages the copy and the host pays the first
// touch of every new page on each call. This pool keeps RESIDENT blocks
// (pinned host memory on the device side, allocated once, grown on demand,
// never shrunk) and hands the caller a LEASE on one of them: the records are
// read in place, and the block returns to the pool when the lease dies.
//
// Contract (judged by tests/gpu/record_pool_gate.cpp on the CPU build):
// - a leased block is never handed out again, never grown and never freed
//   until its lease is destroyed or reset: a second acquisition while a
//   lease is alive takes another free block, or creates one; it never waits
//   (no deadlock when one thread keeps a lease and calls again) and never
//   overwrites a leased block;
// - acquisition picks the smallest free block that holds the request (ties:
//   lowest index); otherwise it grows the largest free block (ties: lowest
//   index), or creates a block when none is free. A growth or a creation by
//   acquire allocates count + count / 8 elements (headroom against
//   frame-to-frame variation), by reserve exactly `count`; both are counted
//   (allocations; growths when a smaller block is replaced);
// - strong exception guarantee: an allocation that throws leaves the pool
//   as before the call (the block keeps its former memory, a block being
//   created is left empty and free) and the exception reaches the caller;
// - acquire(0) returns an EMPTY lease (no block taken); reserve(count)
//   makes some free block hold `count` elements (warm-up), taking nothing;
// - thread safety: the pool's own mutex guards the block table; an
//   allocation runs outside it, on a block marked as taken, so a lease can
//   be returned from any thread at any time. The device pool is also only
//   acquired or grown under the lanes call's resident mutex (lanes_resident,
//   filter_runner.cu), like the other resident buffers.
//
// Plain C++17 (read by nvcc). The memory is injected (BlockMemory): pinned
// memory (cudaHostAlloc, never write-combined: the host reads the records)
// on the device side, plain aligned memory for the host twin, a counting or
// failing memory in the unit gate. The pools of the process are leaked on
// purpose (no cudaFreeHost after the runtime's teardown, no lease outliving
// its pool at exit).

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace mhgp9::gpu {

// Where the blocks come from. allocate throws on failure (the pool is left
// unchanged); release never throws.
class BlockMemory {
 public:
  virtual ~BlockMemory() = default;
  virtual void* allocate(std::size_t bytes) = 0;
  virtual void release(void* pointer) noexcept = 0;
};

// Plain host memory, 64-byte aligned (the host twin's pool).
class AlignedHostMemory final : public BlockMemory {
 public:
  void* allocate(std::size_t bytes) override { return ::operator new(bytes, std::align_val_t{64}); }
  void release(void* pointer) noexcept override { ::operator delete(pointer, std::align_val_t{64}); }
};

struct BlockPoolStats {
  std::uint64_t blocks = 0;        // blocks holding memory
  std::uint64_t bytes = 0;         // their capacity, in bytes
  std::uint64_t leased = 0;        // blocks leased now
  std::uint64_t acquisitions = 0;  // non-empty leases handed out
  std::uint64_t reservations = 0;  // reserve calls with a nonzero count
  std::uint64_t allocations = 0;   // successful allocations (creation or growth)
  std::uint64_t growths = 0;       // allocations that replaced a smaller block
  std::uint64_t second_blocks = 0;  // acquisitions made while another block was leased
  std::uint64_t failures = 0;      // allocations that threw (pool unchanged)
};

template <class T>
class ResidentBlockPool {
  static_assert(std::is_trivially_copyable<T>::value, "pooled records are trivially copyable");

 public:
  // Exclusive, move-only ownership of `size` elements of one block.
  class Lease {
   public:
    Lease() = default;
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    Lease(Lease&& other) noexcept : pool_(other.pool_), index_(other.index_), data_(other.data_), size_(other.size_) {
      other.pool_ = nullptr;
      other.data_ = nullptr;
      other.size_ = 0;
    }
    Lease& operator=(Lease&& other) noexcept {
      if (this != &other) {
        reset();
        pool_ = other.pool_;
        index_ = other.index_;
        data_ = other.data_;
        size_ = other.size_;
        other.pool_ = nullptr;
        other.data_ = nullptr;
        other.size_ = 0;
      }
      return *this;
    }
    ~Lease() { reset(); }
    // Returns the block to its pool (idempotent).
    void reset() noexcept {
      if (pool_ != nullptr) {
#if !defined(MHGP9_RECORD_POOL_MUTANT_NO_RETURN)
        pool_->give_back(index_);
#endif
      }
      pool_ = nullptr;
      data_ = nullptr;
      size_ = 0;
    }
    explicit operator bool() const noexcept { return pool_ != nullptr; }
    T* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }
    std::size_t block() const noexcept { return index_; }

   private:
    friend class ResidentBlockPool;
    Lease(ResidentBlockPool* pool, std::size_t index, T* data, std::size_t size) noexcept
        : pool_(pool), index_(index), data_(data), size_(size) {}
    ResidentBlockPool* pool_ = nullptr;
    std::size_t index_ = 0;
    T* data_ = nullptr;
    std::size_t size_ = 0;
  };

  explicit ResidentBlockPool(BlockMemory& memory) : memory_(&memory) {}
  ResidentBlockPool(const ResidentBlockPool&) = delete;
  ResidentBlockPool& operator=(const ResidentBlockPool&) = delete;
  // Every lease must be gone (the process pools are leaked instead).
  ~ResidentBlockPool() {
    for (auto& block : blocks_)
      if (block.data != nullptr) memory_->release(block.data);
  }

  // A block of at least `count` elements, leased; acquire(0) is empty.
  Lease acquire(std::size_t count) {
    if (count == 0) return Lease{};
    const std::size_t index = take(count, false);
    std::lock_guard<std::mutex> lock(mu_);
    ++stats_.acquisitions;
    return Lease(this, index, blocks_[index].data, count);
  }

  // Some free block holds `count` elements afterwards (warm-up; nothing is
  // leased). A growth or a creation is counted as in acquire.
  void reserve(std::size_t count) {
    if (count == 0) return;
    const std::size_t index = take(count, true);
    give_back(index);
  }

  BlockPoolStats stats() const {
    std::lock_guard<std::mutex> lock(mu_);
    BlockPoolStats out = stats_;
    for (const auto& block : blocks_) {
      if (block.data == nullptr) continue;
      ++out.blocks;
      out.bytes += static_cast<std::uint64_t>(block.capacity) * sizeof(T);
      out.leased += block.leased ? 1U : 0U;
    }
    return out;
  }

  // Elements a growth or a creation by acquire allocates for `count`.
  static std::size_t grown_capacity(std::size_t count) { return count + count / 8; }

 private:
  struct Block {
    T* data = nullptr;
    std::size_t capacity = 0;
    bool leased = false;
  };

  // Marks a free block holding `count` elements as taken and returns its
  // index; grows or creates one outside the mutex when needed.
  std::size_t take(std::size_t count, bool reservation) {
    std::size_t index = 0, previous = 0;
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (reservation) ++stats_.reservations;
      bool other_leased = false, fit = false, free = false;
      std::size_t best = 0, largest = 0;
      for (std::size_t i = 0; i < blocks_.size(); ++i) {
        const Block& block = blocks_[i];
#if defined(MHGP9_RECORD_POOL_MUTANT_TAKE_LEASED)
        const bool leased = false;  // mutant: a leased block looks free
#else
        const bool leased = block.leased;
#endif
        if (leased) {
          other_leased = true;
          continue;
        }
        if (block.capacity >= count && (!fit || block.capacity < blocks_[best].capacity)) {
          best = i;
          fit = true;
        }
        if (!free || block.capacity > blocks_[largest].capacity) largest = i;
        free = true;
      }
      if (!reservation && other_leased) ++stats_.second_blocks;
      if (fit) {
        blocks_[best].leased = true;
        return best;
      }
      if (free) {
        index = largest;
      } else {
        blocks_.push_back(Block{});
        index = blocks_.size() - 1;
      }
      blocks_[index].leased = true;  // taken while its memory is replaced
      previous = blocks_[index].capacity;
    }
    const std::size_t capacity = reservation ? count : grown_capacity(count);
    T* data = nullptr;
    try {
      data = static_cast<T*>(memory_->allocate(capacity * sizeof(T)));
    } catch (...) {
      std::lock_guard<std::mutex> lock(mu_);
      ++stats_.failures;
#if !defined(MHGP9_RECORD_POOL_MUTANT_KEEP_TAKEN_ON_THROW)
      blocks_[index].leased = false;  // the block keeps its former memory
#endif
      throw;
    }
    T* old = nullptr;
    {
      std::lock_guard<std::mutex> lock(mu_);
      old = blocks_[index].data;
      blocks_[index].data = data;
      blocks_[index].capacity = capacity;
      ++stats_.allocations;
      if (previous != 0) ++stats_.growths;
    }
    if (old != nullptr) memory_->release(old);
    return index;
  }

  void give_back(std::size_t index) noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    blocks_[index].leased = false;
  }

  BlockMemory* memory_;
  mutable std::mutex mu_;
  std::vector<Block> blocks_;  // entries are never removed; a block never shrinks
  BlockPoolStats stats_;
};

}  // namespace mhgp9::gpu
