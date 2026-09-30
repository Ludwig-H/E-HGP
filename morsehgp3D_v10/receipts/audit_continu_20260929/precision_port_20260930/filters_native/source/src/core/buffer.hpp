// Tampons comptes : tout grand tableau passe par Buffer<T>, reserve dans un MemoryBudget AVANT
// l'allocation (le budget est honnete par construction ; la v4 avait un plafond menteur).
// Pas d'initialisation par defaut ; en build de test (MHGP10_POISON) les octets sont empoisonnes 0xA5.
#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <new>
#include <span>
#include <type_traits>

#include "core/status.hpp"

namespace mhgp10 {

class MemoryBudget {
 public:
  explicit MemoryBudget(u64 limit_bytes = ~u64{0}) : limit_(limit_bytes) {}
  bool reserve(u64 bytes) {
    u64 cur = used_.load(std::memory_order_relaxed);
    for (;;) {
      if (bytes > limit_ || cur > limit_ - bytes) return false;
      if (used_.compare_exchange_weak(cur, cur + bytes, std::memory_order_relaxed)) break;
    }
    u64 peak = peak_.load(std::memory_order_relaxed);
    while (cur + bytes > peak && !peak_.compare_exchange_weak(peak, cur + bytes, std::memory_order_relaxed)) {
    }
    return true;
  }
  void release(u64 bytes) { used_.fetch_sub(bytes, std::memory_order_relaxed); }
  u64 used() const { return used_.load(std::memory_order_relaxed); }
  u64 peak() const { return peak_.load(std::memory_order_relaxed); }
  u64 limit() const { return limit_; }

 private:
  u64 limit_;
  std::atomic<u64> used_{0};
  std::atomic<u64> peak_{0};
};

// Budget par defaut du processus (illimite) pour les usages hors Session (tests, outils).
MemoryBudget& default_budget();

template <class T>
class Buffer {
  static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                "Buffer<T> : types triviaux seulement");

 public:
  Buffer() = default;
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& o) noexcept { swap(o); }
  Buffer& operator=(Buffer&& o) noexcept {
    if (this != &o) {
      reset();
      swap(o);
    }
    return *this;
  }
  ~Buffer() { reset(); }

  // Alloue n elements non initialises ; false si le budget refuse ou si l'allocation echoue.
  [[nodiscard]] bool allocate(u64 n, MemoryBudget& budget = default_budget()) {
    reset();
    if (n == 0) return true;
    const u64 bytes = n * sizeof(T);
    if (bytes / sizeof(T) != n || !budget.reserve(bytes)) return false;
    T* p = static_cast<T*>(::operator new(bytes, std::nothrow));
    if (p == nullptr) {
      budget.release(bytes);
      return false;
    }
#ifdef MHGP10_POISON
    std::memset(static_cast<void*>(p), 0xA5, bytes);
#endif
    data_ = p;
    size_ = n;
    budget_ = &budget;
    return true;
  }
  [[nodiscard]] bool allocate_zero(u64 n, MemoryBudget& budget = default_budget()) {
    if (!allocate(n, budget)) return false;
    if (n) std::memset(static_cast<void*>(data_), 0, n * sizeof(T));
    return true;
  }
  void reset() {
    if (data_ != nullptr) {
      ::operator delete(static_cast<void*>(data_));
      budget_->release(size_ * sizeof(T));
    }
    data_ = nullptr;
    size_ = 0;
    budget_ = nullptr;
  }
  void swap(Buffer& o) noexcept {
    std::swap(data_, o.data_);
    std::swap(size_, o.size_);
    std::swap(budget_, o.budget_);
  }

  u64 size() const { return size_; }
  bool empty() const { return size_ == 0; }
  T* data() { return data_; }
  const T* data() const { return data_; }
  T& operator[](u64 i) { return data_[i]; }
  const T& operator[](u64 i) const { return data_[i]; }
  T* begin() { return data_; }
  T* end() { return data_ + size_; }
  const T* begin() const { return data_; }
  const T* end() const { return data_ + size_; }
  std::span<T> span() { return {data_, static_cast<std::size_t>(size_)}; }
  std::span<const T> span() const { return {data_, static_cast<std::size_t>(size_)}; }

 private:
  T* data_ = nullptr;
  u64 size_ = 0;
  MemoryBudget* budget_ = nullptr;
};

// CSR a decalages u32 : off.size() == rows + 1, off[0] == 0, croissants, off[rows] == val.size().
template <class T>
struct Csr {
  Buffer<u32> off;
  Buffer<T> val;
  u32 rows() const { return off.empty() ? 0 : static_cast<u32>(off.size() - 1); }
  std::span<const T> row(u32 i) const { return {val.data() + off[i], static_cast<std::size_t>(off[i + 1] - off[i])}; }
  bool well_formed() const {
    if (off.empty()) return val.empty();
    if (off[0] != 0) return false;
    for (u64 i = 0; i + 1 < off.size(); ++i)
      if (off[i + 1] < off[i]) return false;
    return off[off.size() - 1] == val.size();
  }
};

}  // namespace mhgp10
