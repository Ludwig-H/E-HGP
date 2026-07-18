#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::uint64_t k1_boruvka_mixed_component =
    std::numeric_limits<std::uint64_t>::max() - UINT64_C(1);
inline constexpr std::uint64_t k1_boruvka_sentinel =
    std::numeric_limits<std::uint64_t>::max();

struct K1BoruvkaNodeInputRecord {
  std::uint64_t lower_bits[3]{};
  std::uint64_t upper_bits[3]{};
  std::uint64_t left_child{k1_boruvka_sentinel};
  std::uint64_t right_child{k1_boruvka_sentinel};
  std::uint64_t escape{k1_boruvka_sentinel};
  std::uint64_t leaf_point_id{k1_boruvka_sentinel};
};
static_assert(
    sizeof(K1BoruvkaNodeInputRecord) == 10U * sizeof(std::uint64_t));
static_assert(std::is_standard_layout_v<K1BoruvkaNodeInputRecord>);
static_assert(std::is_trivially_copyable_v<K1BoruvkaNodeInputRecord>);

struct K1BoruvkaCandidateRecord {
  std::uint64_t source_point_id{k1_boruvka_sentinel};
  std::uint64_t target_point_id{k1_boruvka_sentinel};
};
static_assert(
    sizeof(K1BoruvkaCandidateRecord) == 2U * sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<K1BoruvkaCandidateRecord>);

struct K1BoruvkaCandidateBatch {
  std::vector<std::uint64_t> candidate_offsets;
  std::vector<K1BoruvkaCandidateRecord> records;
  std::size_t output_capacity{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t count_pass_node_visit_count{};
  std::size_t emit_pass_node_visit_count{};
  std::size_t uniform_component_prune_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t invalid_bound_descent_count{};
  std::uint64_t buffer_epoch{};
  bool exact_capacity{false};
  bool no_truncation{false};
};

class K1BoruvkaCandidateContextState final {
 public:
  K1BoruvkaCandidateContextState() = default;
  ~K1BoruvkaCandidateContextState() = default;

  K1BoruvkaCandidateContextState(
      const K1BoruvkaCandidateContextState&) = delete;
  K1BoruvkaCandidateContextState& operator=(
      const K1BoruvkaCandidateContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_healthy_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    require_healthy();
    return std::forward<Operation>(operation)();
  }

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    require_healthy();
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

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 5 K1 Boruvka buffer epoch overflowed");
    }
    return ++epoch_;
  }

 private:
  void require_healthy() const {
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 5 K1 Boruvka candidate context is poisoned by a prior GPU failure");
    }
  }

  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
};

[[nodiscard]] K1BoruvkaCandidateBatch propose_k1_boruvka_candidates_on_gpu(
    K1BoruvkaCandidateContextState& context,
    std::span<const K1BoruvkaNodeInputRecord> nodes,
    std::size_t root_index,
    std::span<const std::uint64_t> coordinate_bits,
    std::size_t point_count,
    std::span<const std::uint64_t> frozen_component_labels,
    std::span<const std::uint64_t> node_component_tags,
    std::span<const std::uint64_t> seed_cutoff_upper_bits);

}  // namespace morsehgp3d::gpu::detail
