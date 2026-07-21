#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t pair_support_maximum_requested_order = 10U;

// Exact maximum of (x-u).(x-v) over query_box x first_support_box x
// second_support_box.  Endpoint selectors are 0 for lower and 1 for upper;
// they form a compact replay witness while maximum_phi remains authoritative.
struct ExactDiametralPhiAabbMaximum {
  exact::ExactRational maximum_phi;
  std::array<std::uint8_t, 3> query_endpoint{};
  std::array<std::uint8_t, 3> first_support_endpoint{};
  std::array<std::uint8_t, 3> second_support_endpoint{};

  friend bool operator==(
      const ExactDiametralPhiAabbMaximum&,
      const ExactDiametralPhiAabbMaximum&) = default;
};

[[nodiscard]] ExactDiametralPhiAabbMaximum
exact_diametral_phi_aabb_maximum(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box);

enum class ExactPairSupportStreamStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class ExactPairSupportStopReason : std::uint8_t {
  none,
  work_unit_limit,
  frontier_entry_limit,
  auxiliary_frontier_entry_limit,
  emitted_record_limit,
  emitted_point_id_reference_limit,
  global_closed_ball_query_limit,
  point_classification_limit,
};

struct ExactPairSupportStreamBudget {
  // A work unit is either one support-product visit or one witness-node visit.
  // A global closed-ball query is an indivisible exact leaf operation and is
  // accounted separately in the audit.
  std::size_t maximum_work_unit_count{};
  std::size_t maximum_frontier_entry_count{1U};
  // Bounds either one witness search or one sparse closed-ball DFS.  These
  // auxiliary frontiers are ephemeral and never become a global cell arena.
  std::size_t maximum_auxiliary_frontier_entry_count{1U};
  std::size_t maximum_emitted_record_count{};
  // Includes fixed support ids and sparse interior/extra-shell witnesses.
  // Exterior ids and complete degenerate shells are never materialized.
  std::size_t maximum_emitted_point_id_reference_count{};
  std::size_t maximum_global_closed_ball_query_count{};
  // Checked conservatively before a leaf query: one complete global shell
  // classifies point_count sites, even when LBVH bulk decisions avoid an
  // exact point-distance evaluation.
  std::size_t maximum_point_classification_count{};

  friend bool operator==(
      const ExactPairSupportStreamBudget&,
      const ExactPairSupportStreamBudget&) = default;
};

struct ExactPairSupportRequirements {
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};

  friend bool operator==(
      const ExactPairSupportRequirements&,
      const ExactPairSupportRequirements&) = default;
};

// Fixed-width, pointer-free frontier record suitable for later GPU packing.
// Leaf ranges are half-open Morton intervals in the supplied LBVH.
struct ExactPairSupportFrontierEntry {
  std::uint64_t first_node_index{};
  std::uint64_t second_node_index{};
  std::uint64_t first_leaf_begin{};
  std::uint64_t first_leaf_end{};
  std::uint64_t second_leaf_begin{};
  std::uint64_t second_leaf_end{};
  std::uint8_t self_product{};

  friend bool operator==(
      const ExactPairSupportFrontierEntry&,
      const ExactPairSupportFrontierEntry&) = default;
};

static_assert(std::is_standard_layout_v<ExactPairSupportFrontierEntry>);
static_assert(std::is_trivially_copyable_v<ExactPairSupportFrontierEntry>);
static_assert(std::is_standard_layout_v<spatial::ExactDyadicAabb3>);
static_assert(std::is_trivially_copyable_v<spatial::ExactDyadicAabb3>);

struct ExactPairSupportEvent {
  std::array<spatial::PointId, 2> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactPairSupportEvent&,
      const ExactPairSupportEvent&) = default;
};

// A pair with an additional exact shell point is not emitted as a regular
// event.  It remains relevant when interior_count + support_size is inside the
// requested rank window, independently of the full shell cardinality.
struct ExactPairSupportExtraShellDiagnostic {
  std::array<spatial::PointId, 2> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  // A complete exact traversal certifies shell_count.  Only the canonical
  // least extra-shell id is retained, so a 10M-point cosphere cannot force a
  // 10M-entry resident diagnostic.
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactPairSupportExtraShellDiagnostic&,
      const ExactPairSupportExtraShellDiagnostic&) = default;
};

struct ExactPairSupportStreamAudit {
  std::size_t total_pair_count{};
  std::size_t work_unit_count{};
  std::size_t support_product_visit_count{};
  std::size_t support_product_expansion_count{};
  std::size_t self_product_expansion_count{};
  std::size_t cross_product_expansion_count{};
  std::size_t diagonal_leaf_discard_count{};
  std::size_t diagonal_product_rank_search_skip_count{};
  std::size_t rank_prune_search_count{};
  std::size_t witness_node_visit_count{};
  std::size_t exact_phi_aabb_bound_count{};
  std::size_t exact_anchor_ball_minimum_aabb_bound_count{};
  std::size_t certified_anchor_noninterior_subtree_count{};
  std::size_t certified_anchor_noninterior_point_count{};
  std::size_t certified_anchor_shell_tangent_subtree_count{};
  std::size_t equality_or_positive_bound_descent_count{};
  std::size_t strict_interior_witness_subtree_count{};
  std::size_t strict_interior_witness_point_count{};
  std::size_t rank_pruned_product_count{};
  std::size_t rank_pruned_pair_count{};
  std::size_t leaf_pair_classification_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t point_classification_count{};
  std::size_t closed_ball_node_visit_count{};
  std::size_t exact_closed_ball_minimum_aabb_bound_count{};
  std::size_t exact_closed_ball_maximum_aabb_bound_count{};
  std::size_t closed_ball_bulk_interior_subtree_count{};
  std::size_t closed_ball_bulk_interior_point_count{};
  std::size_t closed_ball_bulk_exterior_subtree_count{};
  std::size_t closed_ball_bulk_exterior_point_count{};
  std::size_t early_closed_rank_rejection_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t above_rank_pair_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_witness_frontier_entry_count{};
  std::size_t maximum_closed_ball_frontier_entry_count{};
  std::size_t remaining_frontier_pair_count{};
  std::size_t resolved_pair_count{};
  bool pair_partition_accounting_certified{false};

  friend bool operator==(
      const ExactPairSupportStreamAudit&,
      const ExactPairSupportStreamAudit&) = default;
};

struct ExactPairSupportStreamResult {
  ExactPairSupportRequirements requirements{};
  ExactPairSupportStreamBudget budget{};
  ExactPairSupportStreamStatus status{
      ExactPairSupportStreamStatus::budget_exhausted};
  ExactPairSupportStopReason stop_reason{
      ExactPairSupportStopReason::work_unit_limit};
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  std::vector<ExactPairSupportFrontierEntry> remaining_frontier;
  ExactPairSupportStreamAudit audit{};
  bool self_product_partition_certified{false};
  bool witness_antichains_certified{false};
  bool all_rank_prunes_recertified{false};
  // Rank-rejected queries may stop as soon as the strict-interior cap is
  // exceeded.  Every query that can still emit a rank-relevant record visits
  // its complete shell.
  bool all_rank_relevant_shells_complete{false};
  bool frontier_exhausted{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  [[nodiscard]] bool stream_complete() const noexcept {
    return status == ExactPairSupportStreamStatus::complete &&
           stop_reason == ExactPairSupportStopReason::none &&
           frontier_exhausted && remaining_frontier.empty() &&
           self_product_partition_certified &&
           witness_antichains_certified &&
           all_rank_prunes_recertified &&
           all_rank_relevant_shells_complete &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed &&
           audit.pair_partition_accounting_certified &&
           audit.remaining_frontier_pair_count == 0U &&
           audit.resolved_pair_count == audit.total_pair_count;
  }

  [[nodiscard]] bool absence_of_additional_pair_supports_certified()
      const noexcept {
    return stream_complete();
  }

  friend bool operator==(
      const ExactPairSupportStreamResult&,
      const ExactPairSupportStreamResult&) = default;
};

[[nodiscard]] ExactPairSupportStreamResult build_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget);

struct ExactPairSupportStreamVerification {
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool partial_records_individually_exact{false};
  bool completion_claim_certified{false};
  bool absence_claim_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

[[nodiscard]] ExactPairSupportStreamVerification verify_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& observed);

}  // namespace morsehgp3d::hierarchy
