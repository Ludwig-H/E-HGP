#pragma once

#include "morsehgp3d/hierarchy/direct_morse_industrial_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_facet_descent_batch_plan_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_mode = "certified";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_descent_batch_plan_proof_basis =
        "fresh_10_1_10_2_industrial_chunks_exact_batch_local_structural_"
        "classes_bounded_operational_tiling_without_geometric_difficulty_"
        "claim_shape_only_fresh_reconstruction_required_v1";

// These caps bound every direct 14C source population and every lane or 14A
// chunk allocated for this result.  The nested fresh journal replays retain
// their own independent budgets and may scan their cloud/facade authorities.
// A work item is one arm-seed provenance before the closure deduplicates
// complete facet keys and shares strict suffixes.
struct ExactDirectSparseFacetDescentBatchPlanBudget {
  std::size_t maximum_source_chunk_count{};
  std::size_t maximum_source_batch_count{};
  std::size_t maximum_source_family_count{};
  std::size_t maximum_source_arm_seed_count{};
  std::size_t maximum_lane_count{};
  std::size_t maximum_initial_seed_launch_count{};
  std::size_t
      maximum_initial_seed_standalone_step_support_examination_count{};
  std::size_t maximum_initial_seed_work_item_count_per_launch{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchPlanBudget&,
      const ExactDirectSparseFacetDescentBatchPlanBudget&) = default;
};

// One lane is a compact selection predicate over one exact source batch.
// Families in [candidate_family_begin_index, candidate_family_end_index)
// belong to the same (order, level) batch.  This lane selects exactly those
// whose arm_seed_count equals source_support_cardinality.  No key, exact
// level, facet or durable arm-index permutation is copied into the plan.
// A future executor must select matches once, stably, into bounded
// chunk-local scratch and join results back by arm_seed_index.
struct ExactDirectSparseFacetDescentBatchLane {
  std::size_t lane_index{};
  std::size_t source_chunk_index{};
  std::size_t source_batch_index{};
  std::size_t candidate_family_begin_index{};
  std::size_t candidate_family_end_index{};
  std::size_t candidate_arm_seed_begin_index{};
  std::size_t candidate_arm_seed_end_index{};
  std::size_t facet_cardinality{};
  std::size_t source_support_cardinality{};
  std::size_t source_interior_cardinality{};
  std::size_t matching_family_count{};
  std::size_t matching_arm_seed_count{};
  std::size_t initial_seed_work_item_count{};
  std::size_t initial_seed_launch_count{};
  std::size_t
      initial_seed_standalone_step_support_examination_count_upper_bound{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchLane&,
      const ExactDirectSparseFacetDescentBatchLane&) = default;
};

struct ExactDirectSparseFacetDescentBatchPlanCounters {
  std::size_t source_chunk_scan_count{};
  std::size_t source_batch_scan_count{};
  std::size_t source_family_scan_count{};
  std::size_t source_arm_seed_reference_count{};
  std::size_t lane_count{};
  std::size_t matching_family_count{};
  std::size_t matching_arm_seed_count{};
  std::size_t initial_seed_launch_count{};
  std::size_t
      initial_seed_standalone_step_support_examination_count_upper_bound{};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchPlanCounters&,
      const ExactDirectSparseFacetDescentBatchPlanCounters&) = default;
};

enum class ExactDirectSparseFacetDescentBatchPlanDecision
    : std::uint8_t {
  not_planned,
  no_plan_invalid_budget,
  no_plan_capacity_overflow,
  no_plan_allocation_failed,
  no_plan_source_industrial_plan_rejected,
  no_plan_source_join_inconsistent,
  no_plan_budget_exhausted,
  complete_architecture_only_descent_batch_plan,
};

enum class ExactDirectSparseFacetDescentBatchPlanScope
    : std::uint8_t {
  unspecified,
  exact_batch_local_structural_provenance_lanes_before_one_shared_closure_only,
};

struct ExactDirectSparseFacetDescentBatchPlanResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_descent_batch_plan_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_descent_batch_plan_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_descent_batch_plan_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_descent_batch_plan_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_descent_batch_plan_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_descent_batch_plan_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_descent_batch_plan_schema_version};
  ExactDirectMorseIndustrialPlanConfig requested_industrial_config{};
  ExactDirectSparseFacetDescentBatchPlanBudget requested_budget{};
  ExactDirectMorseIndustrialPlanResult source_industrial_plan{};
  std::size_t required_lane_count{};
  std::size_t required_initial_seed_launch_count{};
  std::size_t
      required_initial_seed_standalone_step_support_examination_count{};
  std::vector<ExactDirectSparseFacetDescentBatchLane> lanes;
  ExactDirectSparseFacetDescentBatchPlanCounters counters{};

  bool source_population_preflight_certified{false};
  bool source_industrial_plan_freshly_built{false};
  bool source_batches_families_and_arms_joined{false};
  bool budget_preflight_completed{false};
  bool budget_preflight_satisfied{false};
  bool structural_class_is_exact{false};
  bool at_most_three_lanes_per_exact_batch{false};
  bool every_source_family_and_arm_seed_selected_exactly_once{false};
  bool lane_order_is_canonical{false};
  bool initial_seed_work_item_tiling_is_bounded{false};
  bool stable_single_pass_lane_selection_required{false};
  bool lanes_never_cross_chunk_or_exact_batch{false};
  bool common_frozen_pre_batch_locator_snapshot_required{false};
  bool one_shared_closure_and_memo_required_per_exact_batch{false};
  bool scientific_commit_barrier_preserved{false};
  bool lane_order_is_operational_only{false};
  // complete_architecture_plan() checks this compact payload's shape only.
  // The public verifier must rebuild it from 10.1--10.2 before execution or
  // persistence; the plan is not an execution authority by itself.
  bool standalone_shape_check_only{false};
  bool fresh_reconstruction_required_before_execution{false};
  bool complete_shared_closure_work_bounded{false};
  bool actual_lbvh_or_rational_difficulty_claimed{false};
  bool facets_keys_or_durable_arm_permutation_materialized{false};
  bool hierarchy_reduction_or_descent_executed{false};
  bool forbidden_global_structure_materialized{false};
  bool gpu_execution_qualified{false};
  bool sub_second_latency_claimed{false};
  bool ten_million_point_capacity_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseFacetDescentBatchPlanDecision decision{
      ExactDirectSparseFacetDescentBatchPlanDecision::not_planned};
  ExactDirectSparseFacetDescentBatchPlanScope scope{
      ExactDirectSparseFacetDescentBatchPlanScope::unspecified};

  [[nodiscard]] bool complete_architecture_plan() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchPlanResult&,
      const ExactDirectSparseFacetDescentBatchPlanResult&) = default;
};

struct ExactDirectSparseFacetDescentBatchPlanVerification {
  bool observed_storage_within_budget{false};
  bool source_industrial_plan_freshly_rebuilt{false};
  bool structural_lanes_freshly_rebuilt{false};
  bool exact_result_equality_certified{false};
  bool no_geometric_difficulty_or_gpu_qualification_claimed{false};
  bool no_forbidden_global_structure_materialized{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparseFacetDescentBatchPlanVerification&,
      const ExactDirectSparseFacetDescentBatchPlanVerification&) = default;
};

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanResult
build_exact_direct_sparse_facet_descent_batch_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget);

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanVerification
verify_exact_direct_sparse_facet_descent_batch_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget,
    const ExactDirectSparseFacetDescentBatchPlanResult& observed);

}  // namespace morsehgp3d::hierarchy
