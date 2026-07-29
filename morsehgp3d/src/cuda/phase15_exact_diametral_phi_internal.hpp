#pragma once

#include "morsehgp3d/gpu/exact_diametral_phi.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

enum class ExactDiametralPhiExecutionKind : std::uint8_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class ExactDiametralPhiDeviceStage : std::uint8_t {
  fp64_interval = 0U,
  fixed_limb_dyadic = 1U,
  requires_cpu_fallback = 2U,
  invalid = 3U,
};

enum class ExactDiametralPhiDeviceSign : std::int8_t {
  negative = -1,
  zero = 0,
  positive = 1,
  unresolved = 2,
  invalid = 3,
};

struct ExactDiametralPhiDeviceRecord {
  std::uint64_t query_index{};
  ExactDiametralPhiDeviceSign sign{ExactDiametralPhiDeviceSign::invalid};
  ExactDiametralPhiDeviceStage stage{ExactDiametralPhiDeviceStage::invalid};
  std::uint8_t reserved[6]{};
};
static_assert(std::is_standard_layout_v<ExactDiametralPhiDeviceRecord>);
static_assert(std::is_trivially_copyable_v<ExactDiametralPhiDeviceRecord>);
static_assert(sizeof(ExactDiametralPhiDeviceRecord) == 16U);

struct ExactDiametralPhiDeviceBatch {
  std::vector<ExactDiametralPhiDeviceRecord> records;
  std::size_t maximum_query_count{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  int cuda_device{-1};
  ExactDiametralPhiExecutionKind execution_kind{
      ExactDiametralPhiExecutionKind::host_fake};
  bool exact_fixed_limb_implemented{false};
  bool epsilon_used{false};
};

class ExactDiametralPhiContextState final {
 public:
  ExactDiametralPhiContextState() = default;
  ~ExactDiametralPhiContextState() = default;

  ExactDiametralPhiContextState(const ExactDiametralPhiContextState&) = delete;
  ExactDiametralPhiContextState& operator=(
      const ExactDiametralPhiContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    waiting_call_count_.fetch_add(1U, std::memory_order_acq_rel);
    try {
      mutex_.lock();
    } catch (...) {
      waiting_call_count_.fetch_sub(1U, std::memory_order_acq_rel);
      throw;
    }
    waiting_call_count_.fetch_sub(1U, std::memory_order_acq_rel);
    std::lock_guard<std::mutex> lock{mutex_, std::adopt_lock};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the exact diametral-Phi context is poisoned by a prior failure");
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

  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

  // A nonzero value means another call has entered the transaction protocol
  // and is waiting for this context's single publication lock. This internal
  // observation makes the concurrent poison regression deterministic.
  [[nodiscard]] std::size_t waiting_call_count() const noexcept {
    return waiting_call_count_.load(std::memory_order_acquire);
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::atomic<std::size_t> waiting_call_count_{0U};
};

[[nodiscard]] ExactDiametralPhiDeviceBatch
decide_exact_diametral_phi_on_gpu(
    ExactDiametralPhiContextState& context,
    std::span<const ExactDiametralPhiInput> inputs,
    std::size_t maximum_query_count);

}  // namespace morsehgp3d::gpu::detail
