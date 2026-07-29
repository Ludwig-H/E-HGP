#pragma once

#include "morsehgp3d/gpu/ranked_diametral_pair_catalog.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::detail {

enum class Phase15RankedPairCatalogExecutionKind : std::uint64_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class Phase15RankedPairCatalogFailureCode : std::uint64_t {
  none = 0U,
  malformed_input = 1U,
  capacity_exhausted = 2U,
  traversal_failure = 3U,
  exact_predicate_failure = 4U,
};

struct Phase15RankedPairCatalogAdoptedTraversal {
  std::shared_ptr<void> owner;
  std::shared_ptr<const void> source_cloud_identity;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t coordinate_word_capacity{};
  std::size_t morton_point_id_capacity{};
  std::size_t node_capacity{};
  std::size_t point_count{};
  std::size_t node_count{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool cuda_resident{false};
  bool host_fake{false};
};

struct Phase15RankedPairCatalogDeviceOutput {
  std::shared_ptr<void> owner;
  const std::uint64_t* support_u{};
  const std::uint64_t* support_v{};
  const std::uint8_t* closed_rank{};
  const std::uint64_t* rank_offsets{};
  const std::uint64_t* strict_offsets{};
  const std::uint64_t* strict_point_ids{};
  const std::uint64_t* shell_offsets{};
  const std::uint64_t* shell_point_ids{};
  std::size_t catalog_record_capacity{};
  std::size_t payload_point_id_capacity{};
  std::size_t catalog_record_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  std::size_t rank_offset_count{};
};

struct Phase15RankedPairCatalogDeviceReceipt {
  Phase15RankedPairCatalogDeviceOutput output;
  ClosedRankCatalogClosure closed_rank_closure;
  PairSupportRelevantGpClosure pair_support_relevant_gp_closure;
  RankedDiametralPairCatalogBudget fixed_budget;
  std::uint32_t contract_schema_version{
      ranked_diametral_pair_catalog_contract_schema_version};
  std::size_t point_count{};
  std::size_t node_count{};
  std::size_t maximum_level{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t intermediate_candidate_device_to_host_count{};
  std::size_t legacy_host_callback_count{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t buffer_epoch{};
  std::uint64_t status{};
  std::uint64_t stop_reason{};
  std::uint64_t failure_code{};
  std::uint64_t receipt_digest_fnv1a{};
  int cuda_device{-1};
  Phase15RankedPairCatalogExecutionKind execution_kind{
      Phase15RankedPairCatalogExecutionKind::host_fake};
  bool cuda_path_qualified{false};
  bool device_output_layout_validated{false};
  bool device_rank_offsets_validated{false};
  bool device_records_sorted_unique{false};
  bool device_payload_partition_validated{false};
  bool morton_ownership_partition_validated{false};
  bool yao48_filter_policy_validated{false};
  bool candidate_point_id_canonicalization_validated{false};
  bool candidate_deduplication_before_exact_classification_validated{false};
  bool closed_prune_mass_reused_for_relevant_gp{false};
  bool exact_gpu_predicates_implemented{false};
  bool scientific_decision_published{false};
  bool global_relevant_gp_complete_published{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool morton_scientific_authority_claimed{false};
  bool raw_morton_pair_stream_materialized{false};
  bool forbidden_global_pair_matrix_materialized{false};
  bool public_status_claimed{false};
};

[[nodiscard]] constexpr std::uint64_t
phase15_ranked_pair_catalog_digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase15_ranked_pair_catalog_receipt_digest(
    const Phase15RankedPairCatalogDeviceReceipt& receipt) noexcept {
  std::uint64_t digest = UINT64_C(14695981039346656037);
  const auto add = [&digest](std::uint64_t word) {
    digest = phase15_ranked_pair_catalog_digest_word(digest, word);
  };
  add(UINT64_C(0x5031355041495232));
  add(static_cast<std::uint64_t>(receipt.contract_schema_version));
  add(receipt.source_snapshot_epoch);
  add(receipt.buffer_epoch);
  add(static_cast<std::uint64_t>(receipt.point_count));
  add(static_cast<std::uint64_t>(receipt.node_count));
  add(static_cast<std::uint64_t>(receipt.maximum_level));
  add(receipt.status);
  add(receipt.stop_reason);
  add(receipt.failure_code);
  add(static_cast<std::uint64_t>(receipt.launcher_call_count));
  add(static_cast<std::uint64_t>(receipt.kernel_launch_count));
  add(static_cast<std::uint64_t>(receipt.synchronization_count));
  add(static_cast<std::uint64_t>(
      receipt.intermediate_candidate_device_to_host_count));
  add(static_cast<std::uint64_t>(receipt.legacy_host_callback_count));
  add(static_cast<std::uint64_t>(receipt.cuda_device + 1));
  add(static_cast<std::uint64_t>(receipt.execution_kind));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_anchor_tile_size));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_work_item_count));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_yao48_survivor_occurrence_count));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_canonical_candidate_pair_count));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_catalog_record_count));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_payload_point_id_count));
  add(static_cast<std::uint64_t>(
      receipt.fixed_budget.maximum_exact_fallback_count));
  add(static_cast<std::uint64_t>(receipt.output.catalog_record_capacity));
  add(static_cast<std::uint64_t>(receipt.output.payload_point_id_capacity));
  add(static_cast<std::uint64_t>(receipt.output.catalog_record_count));
  add(static_cast<std::uint64_t>(receipt.output.strict_point_id_count));
  add(static_cast<std::uint64_t>(receipt.output.shell_point_id_count));
  add(static_cast<std::uint64_t>(receipt.output.rank_offset_count));
  add(receipt.output.owner ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.support_u != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.support_v != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.closed_rank != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.rank_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.strict_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.strict_point_ids != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.shell_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.shell_point_ids != nullptr ? UINT64_C(1) : UINT64_C(0));

  const ClosedRankCatalogClosure& closed = receipt.closed_rank_closure;
  add(static_cast<std::uint64_t>(closed.yao48_filter_policy));
  add(closed.owned_pair_universe_count);
  add(closed.yao48_owner_pruned_pair_count);
  add(closed.yao48_inverse_pruned_pair_count);
  add(closed.yao48_survivor_occurrence_count);
  add(closed.duplicate_survivor_occurrence_count);
  add(closed.canonical_candidate_pair_count);
  add(closed.above_window_pair_count);
  add(closed.ranked_record_count);
  add(closed.unresolved_owned_pair_count);
  add(closed.unresolved_candidate_pair_count);
  add(closed.unresolved_exact_predicate_count);
  add(closed.closed_rank_catalog_complete ? UINT64_C(1) : UINT64_C(0));

  const PairSupportRelevantGpClosure& gp =
      receipt.pair_support_relevant_gp_closure;
  add(gp.unordered_pair_universe_count);
  add(gp.strict_interior_pruned_pair_count);
  add(gp.shell_scanned_pair_count);
  add(gp.unresolved_pair_count);
  add(gp.exact_violation_count);
  add(gp.canonical_violation_support_u);
  add(gp.canonical_violation_support_v);
  add(gp.canonical_violation_extra_shell);
  add(static_cast<std::uint64_t>(gp.decision));
  add(gp.canonical_violation_present ? UINT64_C(1) : UINT64_C(0));
  add(gp.relevant_gp_complete ? UINT64_C(1) : UINT64_C(0));

  add(receipt.cuda_path_qualified ? UINT64_C(1) : UINT64_C(0));
  add(receipt.device_output_layout_validated ? UINT64_C(1) : UINT64_C(0));
  add(receipt.device_rank_offsets_validated ? UINT64_C(1) : UINT64_C(0));
  add(receipt.device_records_sorted_unique ? UINT64_C(1) : UINT64_C(0));
  add(receipt.device_payload_partition_validated ? UINT64_C(1) : UINT64_C(0));
  add(receipt.morton_ownership_partition_validated
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.yao48_filter_policy_validated ? UINT64_C(1) : UINT64_C(0));
  add(receipt.candidate_point_id_canonicalization_validated
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.candidate_deduplication_before_exact_classification_validated
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.closed_prune_mass_reused_for_relevant_gp
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.exact_gpu_predicates_implemented ? UINT64_C(1) : UINT64_C(0));
  add(receipt.scientific_decision_published ? UINT64_C(1) : UINT64_C(0));
  add(receipt.global_relevant_gp_complete_published
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.hierarchy_reduction_or_attachment_published
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.morton_scientific_authority_claimed
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.raw_morton_pair_stream_materialized
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.forbidden_global_pair_matrix_materialized
          ? UINT64_C(1)
          : UINT64_C(0));
  add(receipt.public_status_claimed ? UINT64_C(1) : UINT64_C(0));
  return digest;
}

class Phase15RankedDiametralPairCatalogContextState final {
 public:
  explicit Phase15RankedDiametralPairCatalogContextState(
      RankedDiametralPairCatalogBudget fixed_budget)
      : fixed_budget_(fixed_budget) {}

  Phase15RankedDiametralPairCatalogContextState(
      const Phase15RankedDiametralPairCatalogContextState&) = delete;
  Phase15RankedDiametralPairCatalogContextState& operator=(
      const Phase15RankedDiametralPairCatalogContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_launcher_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 15 ranked-pair catalog context is poisoned by a prior "
          "launcher or receipt validation failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 15 ranked-pair catalog epoch overflowed");
    }
    return ++epoch_;
  }

  [[nodiscard]] bool fixed_budget_matches(
      const RankedDiametralPairCatalogBudget& budget) const noexcept {
    return budget == fixed_budget_;
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
  const RankedDiametralPairCatalogBudget fixed_budget_{};
};

[[nodiscard]] Phase15RankedPairCatalogDeviceReceipt
build_phase15_ranked_diametral_pair_catalog_on_device(
    Phase15RankedDiametralPairCatalogContextState& context,
    const Phase15RankedPairCatalogAdoptedTraversal& traversal,
    std::size_t maximum_level,
    const RankedDiametralPairCatalogBudget& fixed_budget);

}  // namespace morsehgp3d::gpu::detail
