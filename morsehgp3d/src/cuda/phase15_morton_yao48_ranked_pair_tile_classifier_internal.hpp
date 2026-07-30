#pragma once

#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"

#include "phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::detail {

struct Phase15MortonYao48RankedPairTileCatalogPrivateViews {
  std::shared_ptr<void> output_owner;
  std::shared_ptr<void> source_owner_authority;
  std::shared_ptr<const void> source_cloud_identity_authority;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_support_u{};
  const std::uint64_t* device_support_v{};
  const std::uint8_t* device_closed_rank{};
  const std::uint64_t* device_rank_offsets{};
  const std::uint64_t* device_strict_offsets{};
  const std::uint64_t* device_strict_point_ids{};
  const std::uint64_t* device_shell_offsets{};
  const std::uint64_t* device_shell_point_ids{};
  std::size_t point_count{};
  std::size_t catalog_record_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  std::size_t rank_offset_count{};
  std::size_t record_offset_count{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

// Explicit internal capability for a resident downstream stage or a native
// qualification executable.  The public lease deliberately exposes no raw
// device address and therefore cannot create an accidental output D2H API.
class Phase15MortonYao48RankedPairTileCatalogPrivateViewAccess final {
 public:
  [[nodiscard]] static
      Phase15MortonYao48RankedPairTileCatalogPrivateViews inspect(
          const MortonYao48RankedPairTileCatalogLease& lease) noexcept;
};

enum class Phase15MortonYao48RankedPairTileClassifierExecutionKind
    : std::uint64_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class Phase15MortonYao48RankedPairTileClassifierFailureCode
    : std::uint64_t {
  none = 0U,
  capacity_exhausted = 1U,
  malformed_input = 2U,
  exact_predicate_failure = 3U,
  internal_invariant = 4U,
};

struct Phase15MortonYao48RankedPairTileClassifierDeviceOutput {
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
  std::size_t exact_fallback_capacity{};
  std::size_t catalog_record_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  std::size_t rank_offset_count{};
  std::size_t record_offset_count{};
};

struct Phase15MortonYao48RankedPairTileClassifierRequest {
  MortonYao48RankedPairTileClassifierConfig fixed_config{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  std::uint64_t candidate_count{};
  std::uint64_t transaction_certified_pruned_pair_mass{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t frontier_unresolved_pair_mass{};
  std::uint64_t prior_candidate_count{};
  std::uint64_t prior_accepted_record_count{};
  std::uint64_t prior_above_window_count{};
  std::uint64_t prior_unresolved_count{};
  std::uint64_t prior_strict_payload_point_id_count{};
  std::uint64_t prior_shell_payload_point_id_count{};
  std::uint64_t prior_exact_fallback_count{};
  bool same_tile_continuation{false};
  bool tile_closed_after_chunk{false};
  bool frontier_closed_after_chunk{false};
};

struct Phase15MortonYao48RankedPairTileClassifierReceipt {
  Phase15MortonYao48RankedPairTileClassifierDeviceOutput output;
  MortonYao48RankedPairTileClassifierConfig fixed_config{};
  std::uint32_t schema_version{
      morton_yao48_ranked_pair_tile_classifier_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t candidate_buffer_epoch{};
  std::uint64_t catalog_buffer_epoch{};
  std::uint64_t tile_epoch{};
  std::uint64_t chunk_sequence{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t anchor_begin{};
  std::size_t anchor_end{};
  std::uint64_t transaction_candidate_count{};
  std::uint64_t transaction_accepted_record_count{};
  std::uint64_t transaction_above_window_count{};
  std::uint64_t transaction_unresolved_count{};
  std::uint64_t transaction_strict_payload_point_id_count{};
  std::uint64_t transaction_shell_payload_point_id_count{};
  std::uint64_t transaction_exact_fallback_count{};
  std::uint64_t transaction_certified_pruned_pair_mass{};
  std::uint64_t cumulative_candidate_count{};
  std::uint64_t cumulative_accepted_record_count{};
  std::uint64_t cumulative_above_window_count{};
  std::uint64_t cumulative_unresolved_count{};
  std::uint64_t cumulative_strict_payload_point_id_count{};
  std::uint64_t cumulative_shell_payload_point_id_count{};
  std::uint64_t cumulative_exact_fallback_count{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t frontier_unresolved_pair_mass{};
  std::uint64_t required_catalog_record_count{};
  std::uint64_t required_payload_point_id_count{};
  std::uint64_t required_exact_fallback_count{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t candidate_device_to_host_count{};
  std::size_t certified_prune_device_to_host_count{};
  std::size_t legacy_pair_callback_count{};
  std::uint64_t status{};
  std::uint64_t stop_reason{};
  std::uint64_t failure_code{};
  std::uint64_t receipt_digest{};
  int cuda_device{-1};
  Phase15MortonYao48RankedPairTileClassifierExecutionKind execution_kind{
      Phase15MortonYao48RankedPairTileClassifierExecutionKind::host_fake};
  bool same_tile_continuation{false};
  bool tile_closed_after_chunk{false};
  bool frontier_closed_after_chunk{false};
  bool source_identity_authenticated{false};
  bool source_epoch_authenticated{false};
  bool candidate_epoch_authenticated{false};
  bool tile_chunk_sequence_authenticated{false};
  bool candidate_tile_lease_alive_during_launch{false};
  bool fixed_linear_capacities_validated{false};
  bool exact_multiorder_classification_implemented{false};
  bool one_multiorder_launch_used{false};
  bool count_scan_payload_layout_validated{false};
  bool output_records_sorted_unique{false};
  bool output_rollback_validated{false};
  bool output_invalidated{false};
  bool candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};
};

[[nodiscard]] constexpr std::uint64_t
phase15_morton_yao48_ranked_pair_tile_classifier_digest_word(
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
phase15_morton_yao48_ranked_pair_tile_classifier_receipt_digest(
    const Phase15MortonYao48RankedPairTileClassifierReceipt& receipt)
    noexcept {
  std::uint64_t digest = UINT64_C(14695981039346656037);
  const auto add = [&digest](std::uint64_t word) noexcept {
    digest =
        phase15_morton_yao48_ranked_pair_tile_classifier_digest_word(
            digest, word);
  };
  add(UINT64_C(0x4d59343852505431));
  add(static_cast<std::uint64_t>(receipt.schema_version));
  add(receipt.source_snapshot_epoch);
  add(receipt.candidate_buffer_epoch);
  add(receipt.catalog_buffer_epoch);
  add(receipt.tile_epoch);
  add(receipt.chunk_sequence);
  add(static_cast<std::uint64_t>(receipt.point_count));
  add(static_cast<std::uint64_t>(receipt.certified_node_count));
  add(static_cast<std::uint64_t>(receipt.anchor_begin));
  add(static_cast<std::uint64_t>(receipt.anchor_end));
  add(static_cast<std::uint64_t>(receipt.fixed_config.maximum_closed_rank));
  add(static_cast<std::uint64_t>(receipt.fixed_config.catalog_record_capacity));
  add(static_cast<std::uint64_t>(
      receipt.fixed_config.payload_point_id_capacity));
  add(static_cast<std::uint64_t>(receipt.fixed_config.exact_fallback_capacity));
  add(receipt.transaction_candidate_count);
  add(receipt.transaction_accepted_record_count);
  add(receipt.transaction_above_window_count);
  add(receipt.transaction_unresolved_count);
  add(receipt.transaction_strict_payload_point_id_count);
  add(receipt.transaction_shell_payload_point_id_count);
  add(receipt.transaction_exact_fallback_count);
  add(receipt.transaction_certified_pruned_pair_mass);
  add(receipt.cumulative_candidate_count);
  add(receipt.cumulative_accepted_record_count);
  add(receipt.cumulative_above_window_count);
  add(receipt.cumulative_unresolved_count);
  add(receipt.cumulative_strict_payload_point_id_count);
  add(receipt.cumulative_shell_payload_point_id_count);
  add(receipt.cumulative_exact_fallback_count);
  add(receipt.cumulative_certified_pruned_pair_mass);
  add(receipt.unordered_pair_universe_count);
  add(receipt.frontier_unresolved_pair_mass);
  add(receipt.required_catalog_record_count);
  add(receipt.required_payload_point_id_count);
  add(receipt.required_exact_fallback_count);
  add(static_cast<std::uint64_t>(receipt.launcher_call_count));
  add(static_cast<std::uint64_t>(receipt.kernel_launch_count));
  add(static_cast<std::uint64_t>(receipt.synchronization_count));
  add(static_cast<std::uint64_t>(receipt.candidate_device_to_host_count));
  add(static_cast<std::uint64_t>(
      receipt.certified_prune_device_to_host_count));
  add(static_cast<std::uint64_t>(receipt.legacy_pair_callback_count));
  add(receipt.status);
  add(receipt.stop_reason);
  add(receipt.failure_code);
  add(static_cast<std::uint64_t>(receipt.cuda_device + 1));
  add(static_cast<std::uint64_t>(receipt.execution_kind));
  add(static_cast<std::uint64_t>(receipt.output.catalog_record_capacity));
  add(static_cast<std::uint64_t>(receipt.output.payload_point_id_capacity));
  add(static_cast<std::uint64_t>(receipt.output.exact_fallback_capacity));
  add(static_cast<std::uint64_t>(receipt.output.catalog_record_count));
  add(static_cast<std::uint64_t>(receipt.output.strict_point_id_count));
  add(static_cast<std::uint64_t>(receipt.output.shell_point_id_count));
  add(static_cast<std::uint64_t>(receipt.output.rank_offset_count));
  add(static_cast<std::uint64_t>(receipt.output.record_offset_count));
  add(receipt.output.owner ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.support_u != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.support_v != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.closed_rank != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.rank_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.strict_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.strict_point_ids != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.shell_offsets != nullptr ? UINT64_C(1) : UINT64_C(0));
  add(receipt.output.shell_point_ids != nullptr ? UINT64_C(1) : UINT64_C(0));
  for (const bool flag : {
           receipt.same_tile_continuation,
           receipt.tile_closed_after_chunk,
           receipt.frontier_closed_after_chunk,
           receipt.source_identity_authenticated,
           receipt.source_epoch_authenticated,
           receipt.candidate_epoch_authenticated,
           receipt.tile_chunk_sequence_authenticated,
           receipt.candidate_tile_lease_alive_during_launch,
           receipt.fixed_linear_capacities_validated,
           receipt.exact_multiorder_classification_implemented,
           receipt.one_multiorder_launch_used,
           receipt.count_scan_payload_layout_validated,
           receipt.output_records_sorted_unique,
           receipt.output_rollback_validated,
           receipt.output_invalidated,
           receipt.candidate_device_to_host_performed,
           receipt.certified_prune_device_to_host_performed,
           receipt.callback_per_pair_performed,
           receipt.dense_pair_scan_performed,
           receipt.global_pair_matrix_materialized,
           receipt.ordinary_delaunay_materialized,
           receipt.higher_order_delaunay_mosaic_materialized,
           receipt.scientific_decision_published,
           receipt.public_status_claimed}) {
    add(flag ? UINT64_C(1) : UINT64_C(0));
  }
  return digest;
}

class Phase15MortonYao48RankedPairTileClassifierContextState final {
 public:
  explicit Phase15MortonYao48RankedPairTileClassifierContextState(
      MortonYao48RankedPairTileClassifierConfig config)
      : config_(config) {}

  Phase15MortonYao48RankedPairTileClassifierContextState(
      const Phase15MortonYao48RankedPairTileClassifierContextState&) = delete;
  Phase15MortonYao48RankedPairTileClassifierContextState& operator=(
      const Phase15MortonYao48RankedPairTileClassifierContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_launcher_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the resident ranked-pair tile classifier is poisoned by a prior "
          "launcher or receipt validation failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::uint64_t advance_catalog_buffer_epoch() {
    if (catalog_buffer_epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the resident ranked-pair catalog buffer epoch overflowed");
    }
    return ++catalog_buffer_epoch_;
  }

  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_.load(std::memory_order_acquire);
  }

  [[nodiscard]] bool config_matches(
      const MortonYao48RankedPairTileClassifierConfig& config) const
      noexcept {
    return config == config_;
  }

  [[nodiscard]] const std::shared_ptr<void>& launcher_state() const
      noexcept {
    return launcher_state_;
  }

  void replace_launcher_state(std::shared_ptr<void> state) noexcept {
    launcher_state_ = std::move(state);
  }

 private:
  std::mutex mutex_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t catalog_buffer_epoch_{};
  const MortonYao48RankedPairTileClassifierConfig config_{};
  std::shared_ptr<void> launcher_state_;
};

// CUDA owns this definition.  It consumes only the private resident views and
// must return a scalar/control receipt; candidate and prune arrays never cross
// to the host.
[[nodiscard]] Phase15MortonYao48RankedPairTileClassifierReceipt
classify_phase15_morton_yao48_ranked_pair_tile_on_device(
    Phase15MortonYao48RankedPairTileClassifierContextState& context,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews& candidate_views,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request);

}  // namespace morsehgp3d::gpu::detail
