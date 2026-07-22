#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t higher_support_maximum_requested_order = 10U;
inline constexpr std::string_view higher_support_stream_proof_basis =
    "exact_grouped_multiplicity_lbvh_support_partition_"
    "universal_gram_cramer_rank_receipts_sparse_closed_ball_v1";

enum class ExactHigherSupportStreamStatus : std::uint8_t {
  complete,
  budget_exhausted,
};

enum class ExactHigherSupportStopReason : std::uint8_t {
  none,
  work_unit_limit,
  frontier_entry_limit,
  auxiliary_frontier_entry_limit,
  emitted_record_limit,
  emitted_point_id_reference_limit,
  prune_receipt_limit,
  global_closed_ball_query_limit,
  point_classification_limit,
};

// Product visits and rank-witness node visits consume work units.  A leaf
// closed-ball query is an indivisible exact operation and has separate caps.
// No capacity is proportional to C(n,3)+C(n,4).
struct ExactHigherSupportStreamBudget {
  std::size_t maximum_work_unit_count{};
  // This capacity must hold the complete initial root set: zero entries for
  // n<3, one for n=3, and two for n>=4.  A smaller value is an invalid
  // configuration because no residual frontier could represent the universe.
  std::size_t maximum_frontier_entry_count{2U};
  std::size_t maximum_auxiliary_frontier_entry_count{1U};
  // Includes regular events, extra-shell diagnostics and prune certificates.
  std::size_t maximum_emitted_record_count{};
  // Includes support ids, retained strict-interior ids and an optional
  // canonical extra-shell witness.  Prune receipts have their own cap.
  std::size_t maximum_emitted_point_id_reference_count{};
  std::size_t maximum_prune_receipt_count{};
  std::size_t maximum_global_closed_ball_query_count{};
  // Conservatively reserves point_count classifications before each atomic
  // sparse closed-ball query, although exact LBVH bounds may classify a whole
  // subtree at once and an above-rank query may terminate early.
  std::size_t maximum_point_classification_count{};

  friend bool operator==(
      const ExactHigherSupportStreamBudget&,
      const ExactHigherSupportStreamBudget&) = default;
};

struct ExactHigherSupportRequirements {
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_relevant_closed_rank{};

  friend bool operator==(
      const ExactHigherSupportRequirements&,
      const ExactHigherSupportRequirements&) = default;
};

// A group means: choose exactly multiplicity leaves from the immutable Morton
// interval of node_index.  Distinct groups in one product have disjoint
// intervals.  The semantic coverage of an entry is therefore the product of
// binomial(range_size, multiplicity), evaluated with exact::BigInt.
struct ExactHigherSupportNodeGroup {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::uint8_t multiplicity{};

  friend bool operator==(
      const ExactHigherSupportNodeGroup&,
      const ExactHigherSupportNodeGroup&) = default;
};

struct ExactHigherSupportFrontierEntry {
  std::uint8_t support_size{};
  std::uint8_t group_count{};
  std::array<ExactHigherSupportNodeGroup, 4> groups{};

  friend bool operator==(
      const ExactHigherSupportFrontierEntry&,
      const ExactHigherSupportFrontierEntry&) = default;
};

// Repeated Morton intervals authenticate node indices without exposing the
// private LBVH node representation.
struct ExactHigherSupportNodeReceipt {
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};

  friend bool operator==(
      const ExactHigherSupportNodeReceipt&,
      const ExactHigherSupportNodeReceipt&) = default;
};

static_assert(std::is_standard_layout_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeGroup>);
static_assert(std::is_standard_layout_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportFrontierEntry>);
static_assert(std::is_standard_layout_v<ExactHigherSupportNodeReceipt>);
static_assert(std::is_trivially_copyable_v<ExactHigherSupportNodeReceipt>);

enum class ExactHigherSupportPruneReason : std::uint8_t {
  no_well_centered_support,
  strict_interior_rank_bound,
};

struct ExactHigherSupportRankReceipt {
  ExactHigherSupportNodeReceipt query_node{};
  std::size_t certified_point_count{};
  // Replaying this bound must recover a strictly negative upper endpoint.
  ExactHigherSupportProductAabbAnalysis query_product_analysis{};

  friend bool operator==(
      const ExactHigherSupportRankReceipt&,
      const ExactHigherSupportRankReceipt&) = default;
};

// Every pruned product remains externally replayable.  A well-centring prune
// is justified by support_product_analysis alone.  A rank prune additionally
// carries a disjoint LBVH receipt antichain whose exact point-count sum reaches
// required_strict_interior_point_count.
struct ExactHigherSupportPruneCertificate {
  ExactHigherSupportFrontierEntry product{};
  ExactHigherSupportPruneReason reason{
      ExactHigherSupportPruneReason::no_well_centered_support};
  exact::BigInt pruned_support_count{0};
  ExactHigherSupportProductAabbAnalysis support_product_analysis{};
  std::size_t required_strict_interior_point_count{};
  exact::BigInt certified_strict_interior_point_count{0};
  std::vector<ExactHigherSupportRankReceipt> rank_receipts;

  friend bool operator==(
      const ExactHigherSupportPruneCertificate&,
      const ExactHigherSupportPruneCertificate&) = default;
};

struct ExactHigherSupportEvent {
  std::uint8_t support_size{};
  // Only the first support_size entries are meaningful and are sorted by
  // canonical PointId.  Remaining entries are zero padding.
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactHigherSupportEvent&,
      const ExactHigherSupportEvent&) = default;
};

struct ExactHigherSupportExtraShellDiagnostic {
  std::uint8_t support_size{};
  std::array<spatial::PointId, 4> support_ids{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
  std::vector<spatial::PointId> interior_ids;
  std::size_t shell_count{};
  spatial::PointId canonical_extra_shell_witness_id{};
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const ExactHigherSupportExtraShellDiagnostic&,
      const ExactHigherSupportExtraShellDiagnostic&) = default;
};

struct ExactHigherSupportStreamAudit {
  exact::BigInt total_support_count{0};
  exact::BigInt well_centering_pruned_support_count{0};
  exact::BigInt rank_pruned_support_count{0};
  exact::BigInt leaf_classified_support_count{0};
  exact::BigInt resolved_support_count{0};
  exact::BigInt remaining_frontier_support_count{0};
  std::size_t work_unit_count{};
  std::size_t support_product_visit_count{};
  std::size_t support_product_expansion_count{};
  std::size_t generated_child_product_count{};
  std::size_t exact_product_analysis_count{};
  std::size_t rank_search_count{};
  std::size_t rank_witness_node_visit_count{};
  std::size_t emitted_prune_certificate_count{};
  std::size_t emitted_rank_receipt_count{};
  std::size_t leaf_support_analysis_count{};
  std::size_t affinely_dependent_leaf_count{};
  std::size_t boundary_reduced_leaf_count{};
  std::size_t exterior_circumcenter_leaf_count{};
  std::size_t minimal_leaf_count{};
  std::size_t above_rank_leaf_count{};
  std::size_t global_closed_ball_query_count{};
  std::size_t point_classification_count{};
  std::size_t closed_ball_node_visit_count{};
  std::size_t closed_ball_bulk_interior_subtree_count{};
  std::size_t closed_ball_bulk_exterior_subtree_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_record_count{};
  std::size_t emitted_point_id_reference_count{};
  std::size_t maximum_frontier_entry_count{};
  std::size_t maximum_rank_frontier_entry_count{};
  std::size_t maximum_closed_ball_frontier_entry_count{};
  bool exact_bigint_universe_certified{false};
  bool grouped_partition_accounting_certified{false};

  friend bool operator==(
      const ExactHigherSupportStreamAudit&,
      const ExactHigherSupportStreamAudit&) = default;
};

struct ExactHigherSupportStreamResult {
  ExactHigherSupportRequirements requirements{};
  ExactHigherSupportStreamBudget budget{};
  ExactHigherSupportStreamStatus status{
      ExactHigherSupportStreamStatus::budget_exhausted};
  ExactHigherSupportStopReason stop_reason{
      ExactHigherSupportStopReason::work_unit_limit};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic>
      relevant_extra_shell_diagnostics;
  std::vector<ExactHigherSupportPruneCertificate> prune_certificates;
  std::vector<ExactHigherSupportFrontierEntry> remaining_frontier;
  ExactHigherSupportStreamAudit audit{};
  bool grouped_frontier_partition_certified{false};
  bool all_prunes_replayable{false};
  bool all_rank_relevant_shells_complete{false};
  bool frontier_exhausted{false};
  bool no_forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};

  [[nodiscard]] bool stream_complete() const {
    return status == ExactHigherSupportStreamStatus::complete &&
           stop_reason == ExactHigherSupportStopReason::none &&
           frontier_exhausted && remaining_frontier.empty() &&
           grouped_frontier_partition_certified && all_prunes_replayable &&
           all_rank_relevant_shells_complete &&
           no_forbidden_global_structure_materialized &&
           !hierarchy_reduction_performed &&
           audit.exact_bigint_universe_certified &&
           audit.grouped_partition_accounting_certified &&
           audit.remaining_frontier_support_count == 0 &&
           audit.resolved_support_count == audit.total_support_count;
  }

  [[nodiscard]] bool absence_of_additional_higher_supports_certified() const {
    return stream_complete();
  }

  friend bool operator==(
      const ExactHigherSupportStreamResult&,
      const ExactHigherSupportStreamResult&) = default;
};

// Exact for every size_t point count, including the 10M product target.
[[nodiscard]] exact::BigInt exact_higher_support_candidate_universe_size(
    std::size_t point_count);

[[nodiscard]] ExactHigherSupportStreamResult
build_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget);

struct ExactHigherSupportStreamVerification {
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool exact_bigint_universe_certified{false};
  bool partial_records_individually_exact{false};
  bool prune_certificates_replayed{false};
  bool grouped_frontier_replayed{false};
  bool completion_claim_certified{false};
  bool absence_claim_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};
};

// Rebuilds from the immutable cloud, LBVH, K and trusted budget.  No field of
// observed steers the traversal or any geometric decision.
[[nodiscard]] ExactHigherSupportStreamVerification
verify_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget,
    const ExactHigherSupportStreamResult& observed);

}  // namespace morsehgp3d::hierarchy
