#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::detail {

// This state deliberately contains no CUDA type or runtime reference. It is
// constructed by predicate_filter.cpp, so the host orchestration can still be
// linked to simulated launchers in a CPU-only build. Real launchers populate
// cuda_resources_ lazily with a type-erased, CUDA-owned resource bundle.
class PredicateFilterContextState final {
 public:
  PredicateFilterContextState() = default;
  ~PredicateFilterContextState() = default;

  PredicateFilterContextState(const PredicateFilterContextState&) = delete;
  PredicateFilterContextState& operator=(
      const PredicateFilterContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 2B predicate filter context is poisoned by a prior GPU failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      mark_poisoned();
      throw;
    }
  }

  void mark_poisoned() noexcept {
    poisoned_.store(true, std::memory_order_release);
  }

  // Access is valid only from an operation passed to with_gpu_section().
  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
};

}  // namespace morsehgp3d::gpu::detail
