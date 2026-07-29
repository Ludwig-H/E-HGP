#pragma once

#include "morsehgp3d/gpu/morton_yao48_radial_subtree_filter.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

enum class Phase15MortonYao48RadialExecutionKind : std::uint8_t {
  host_fake,
  cuda,
};

enum class Phase15MortonYao48RadialProposal : std::uint8_t {
  descend,
  prune,
};

struct Phase15MortonYao48RadialDeviceInput {
  std::uint64_t replay_id{};
  std::array<std::uint64_t, 3U> anchor_bits{};
  std::array<std::uint64_t, 3U> lower_bits{};
  std::array<std::uint64_t, 3U> upper_bits{};
  std::array<std::uint64_t,
             morton_yao48_radial_subtree_filter_cone_count>
      bank_radius_upper_bits{};
  std::uint64_t full_bank_mask{};
  std::uint8_t device_eligible{};
  std::uint8_t reserved[7]{};
};
static_assert(
    std::is_standard_layout_v<Phase15MortonYao48RadialDeviceInput> &&
    std::is_trivially_copyable_v<Phase15MortonYao48RadialDeviceInput> &&
    sizeof(Phase15MortonYao48RadialDeviceInput) ==
        60U * sizeof(std::uint64_t));

struct Phase15MortonYao48RadialDeviceRecord {
  std::uint64_t replay_id{};
  Phase15MortonYao48RadialProposal proposal{
      Phase15MortonYao48RadialProposal::descend};
  std::uint8_t directed_bounds_valid{};
  std::uint8_t reserved[6]{};
};
static_assert(
    std::is_standard_layout_v<Phase15MortonYao48RadialDeviceRecord> &&
    std::is_trivially_copyable_v<Phase15MortonYao48RadialDeviceRecord> &&
    sizeof(Phase15MortonYao48RadialDeviceRecord) == 16U);

struct Phase15MortonYao48RadialDeviceBatch {
  std::vector<Phase15MortonYao48RadialDeviceRecord> records;
  std::size_t maximum_query_count{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  int cuda_device{-1};
  Phase15MortonYao48RadialExecutionKind execution_kind{
      Phase15MortonYao48RadialExecutionKind::host_fake};
  bool directed_round_down_aabb_implemented{};
  bool directed_round_up_three_radius_implemented{};
  bool equality_proposes_prune{};
};

class Phase15MortonYao48RadialSubtreeFilterState final {
 public:
  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Morton/Yao48 radial-subtree filter is poisoned by a prior "
          "launcher or validation failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> poisoned_{false};
  std::shared_ptr<void> cuda_resources_;
};

[[nodiscard]] Phase15MortonYao48RadialDeviceBatch
propose_phase15_morton_yao48_radial_subtrees_on_gpu(
    Phase15MortonYao48RadialSubtreeFilterState& state,
    std::span<const Phase15MortonYao48RadialDeviceInput> inputs,
    std::size_t maximum_query_count);

}  // namespace morsehgp3d::gpu::detail
