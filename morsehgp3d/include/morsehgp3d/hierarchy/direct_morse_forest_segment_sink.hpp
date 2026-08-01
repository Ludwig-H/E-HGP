#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_forest_output_segment_schema_version =
    2U;
inline constexpr std::string_view direct_morse_forest_output_segment_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_output_segment_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_output_segment_mode =
    "segmented_committed_forest_batch_output";
inline constexpr std::string_view
    direct_morse_forest_output_segment_deployment_status = "architecture_only";
inline constexpr std::string_view
    direct_morse_forest_output_segment_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_forest_output_segment_refinement_status =
        "conditional_h0_candidate";

// Per-batch physical caps.  They bound the reducer-owned output working set;
// the global scientific counts remain governed by ExactDirectMorseForestBudget.
// There is deliberately no byte cap here: 15I defines no wire codec and exact
// integer limbs remain part of the in-memory payload.
struct ExactDirectMorseForestSegmentLimits {
  std::size_t maximum_birth_record_count{};
  std::size_t maximum_arm_root_binding_count{};
  std::size_t maximum_saddle_record_count{};
  std::size_t maximum_atomic_group_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_node_count{};

  friend bool operator==(
      const ExactDirectMorseForestSegmentLimits&,
      const ExactDirectMorseForestSegmentLimits&) = default;
};

// Dense logical offsets after all acknowledged segments.  The singleton
// prefix is counted logically but is never materialized in a segment.
struct ExactDirectMorseForestSegmentCursor {
  std::size_t segment_count{};
  std::size_t implicit_order_one_prefix_count{};
  std::size_t birth_record_count{};
  std::size_t arm_root_binding_count{};
  std::size_t saddle_record_count{};
  std::size_t atomic_group_count{};
  std::size_t child_reference_count{};
  std::size_t batch_record_count{};
  std::size_t node_count{};
  std::size_t logical_output_entry_count{};
  contract::CanonicalId chain_digest{};

  friend bool operator==(
      const ExactDirectMorseForestSegmentCursor&,
      const ExactDirectMorseForestSegmentCursor&) = default;
};

// Exactly one successfully committed source batch.  All record indices,
// offsets and node identifiers stay in the resident journal's global logical
// coordinate system; consumers must never rebase them per segment.
struct ExactDirectMorseForestBatchSegment {
  std::uint32_t schema_version{
      direct_morse_forest_output_segment_schema_version};
  ExactDirectMorseForestSegmentCursor begin_cursor{};
  ExactDirectMorseForestSegmentCursor end_cursor{};
  contract::CanonicalId payload_digest{};
  ExactDirectMorseForestBatch batch{};
  std::vector<ExactDirectMorseForestBirthRecord> birth_records;
  std::vector<ExactDirectMorseForestArmRootBinding> arm_root_bindings;
  std::vector<ExactDirectMorseForestSaddleRecord> saddle_records;
  std::vector<ExactDirectMorseForestAtomicGroup> atomic_groups;
  std::vector<ExactDirectMorseForestNodeId> child_node_ids;
  std::vector<ExactDirectMorseForestNode> nodes;
  bool canonical_singleton_prefix_implicit{false};
  bool derived_cache_only{true};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_structure() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestBatchSegment&,
      const ExactDirectMorseForestBatchSegment&) = default;
};

// A synchronous, non-owning sink.  append must catch its own exceptions and
// return false on failure.  A false result leaves the reducer's pending
// segment intact for an identical retry.
struct ExactDirectMorseForestSegmentSinkView {
  using AppendFunction = bool (*)(
      void*, const ExactDirectMorseForestBatchSegment&) noexcept;

  void* state{};
  AppendFunction append{};

  [[nodiscard]] bool valid() const noexcept {
    return state != nullptr && append != nullptr;
  }

  [[nodiscard]] bool try_append(
      const ExactDirectMorseForestBatchSegment& segment) const noexcept {
    return valid() && append(state, segment);
  }
};

enum class ExactDirectMorseForestSegmentDrainDecision : std::uint8_t {
  not_drained,
  no_pending_segment,
  no_invalid_sink,
  no_sink_rejected,
  complete_segment_acknowledged,
};

struct ExactDirectMorseForestSegmentDrainResult {
  ExactDirectMorseForestSegmentCursor pre_drain_cursor{};
  ExactDirectMorseForestSegmentCursor post_drain_cursor{};
  bool pending_segment_retained{false};
  bool reducer_output_history_released{false};
  ExactDirectMorseForestSegmentDrainDecision decision{
      ExactDirectMorseForestSegmentDrainDecision::not_drained};

  [[nodiscard]] bool certified_acknowledged() const noexcept;
  [[nodiscard]] bool certified_retryable_rejection() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestSegmentDrainResult&,
      const ExactDirectMorseForestSegmentDrainResult&) = default;
};

// Terminal O(K) projection.  It is derived from the live locator/carrier
// authorities; batch segments themselves never become scientific input.
struct ExactDirectMorseForestFinalSeal {
  std::uint32_t schema_version{
      direct_morse_forest_output_segment_schema_version};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  ExactDirectMorseForestSegmentCursor final_cursor{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp final_locator_stamp{};
  ExactDirectMorseForestCounters counters{};
  std::vector<ExactDirectMorseForestFinalRoot> final_roots;
  std::size_t logical_output_entry_count{};
  bool all_source_batches_committed{false};
  bool every_segment_acknowledged{false};
  bool derived_cache_only{true};
  bool conditional_exact_h0_only{true};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_conditional_h0_candidate() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestFinalSeal&,
      const ExactDirectMorseForestFinalSeal&) = default;
};

}  // namespace morsehgp3d::hierarchy
