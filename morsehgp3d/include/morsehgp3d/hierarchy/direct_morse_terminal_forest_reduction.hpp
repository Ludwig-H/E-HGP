#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_terminal_forest_reduction_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_mode =
        "resident_bounded_source_plan_live_commit_and_forest_finish";
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_terminal_forest_reduction_proof_basis =
        "certified_terminal_bridge_fresh_14C_plan_shared_locator_14H_"
        "single_use_ticket_reducer_before_cursor_commit_dense_source_batch_"
        "induction_then_conditional_H0_forest_finish_v1";

// All caps remain caller authority.  This seam deliberately does not infer a
// massive capacity from a small resident source or silently raise a rejected
// budget.  The bridge and every object borrowed through it must outlive the
// synchronous reduction call.
struct ExactDirectMorseTerminalForestReductionConfig {
  ExactDirectMorseIndustrialPlanConfig industrial_config{};
  ExactDirectSparseFacetDescentBatchPlanBudget plan_budget{};
  ExactDirectMorseForestBudget forest_budget{};
  ExactDirectSparseFacetDescentBatchExecutionBudget execution_budget{};
  ExactDirectMorseForestConfig forest_config{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};

  friend bool operator==(
      const ExactDirectMorseTerminalForestReductionConfig&,
      const ExactDirectMorseTerminalForestReductionConfig&) = default;
};

enum class ExactDirectMorseTerminalForestReductionDecision : std::uint8_t {
  not_reduced,
  no_reduction_bridge_not_certified,
  no_reduction_source_plan_rejected,
  no_reduction_executor_cursor_out_of_range,
  no_reduction_batch_token_overflow,
  no_reduction_ticket_preparation_rejected,
  no_reduction_live_commit_rejected,
  no_reduction_stream_incomplete,
  no_reduction_forest_finish_rejected,
  complete_conditional_h0_forest,
};

struct ExactDirectMorseTerminalForestReductionTimings {
  // Operational steady-clock observations only.  They have no scientific or
  // source-authority role and are excluded from certified_reduction().
  std::uint64_t plan_wall_nanoseconds{};
  std::uint64_t reducer_setup_wall_nanoseconds{};
  std::uint64_t reducer_stream_wall_nanoseconds{};
  std::uint64_t forest_finish_wall_nanoseconds{};
  std::uint64_t total_wall_nanoseconds{};

  friend bool operator==(
      const ExactDirectMorseTerminalForestReductionTimings&,
      const ExactDirectMorseTerminalForestReductionTimings&) = default;
};

struct ExactDirectMorseTerminalForestReductionAudit {
  static constexpr std::string_view backend =
      direct_morse_terminal_forest_reduction_backend;
  static constexpr std::string_view profile =
      direct_morse_terminal_forest_reduction_profile;
  static constexpr std::string_view mode =
      direct_morse_terminal_forest_reduction_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_terminal_forest_reduction_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_terminal_forest_reduction_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_terminal_forest_reduction_proof_basis;

  std::uint32_t schema_version{
      direct_morse_terminal_forest_reduction_schema_version};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t source_batch_count{};
  std::size_t plan_lane_count{};
  std::size_t prepared_ticket_count{};
  std::size_t committed_batch_count{};
  std::size_t aggregate_resolved_key_count{};
  std::size_t aggregate_arm_join_count{};
  std::size_t maximum_transient_closure_node_count{};
  std::size_t forest_birth_record_count{};
  std::size_t forest_saddle_record_count{};
  std::size_t forest_atomic_group_count{};
  std::size_t forest_node_count{};
  std::size_t forest_final_root_count{};
  std::size_t forest_logical_output_entry_count{};
  ExactDirectSparseFacetDescentBatchPlanDecision plan_decision{
      ExactDirectSparseFacetDescentBatchPlanDecision::not_planned};
  ExactDirectSparseFacetDescentBatchExecutionDecision
      last_batch_execution_decision{
          ExactDirectSparseFacetDescentBatchExecutionDecision::not_executed};
  ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision
      last_preparation_decision{
          ExactDirectSparseFacetDescentBatchTopKProposalPreparationDecision::
              not_prepared};
  ExactDirectMorseForestLiveCommitDecision last_live_commit_decision{
      ExactDirectMorseForestLiveCommitDecision::not_committed};
  ExactDirectMorseForestReducerFoldDecision last_reducer_fold_decision{
      ExactDirectMorseForestReducerFoldDecision::not_folded};
  bool source_bridge_certified{false};
  bool source_manifest_certified{false};
  bool provider_constructor_used{false};
  bool source_plan_complete{false};
  bool executor_and_reducer_shared_one_locator{false};
  bool every_ticket_prepared{false};
  bool every_live_commit_certified{false};
  bool executor_complete{false};
  bool reducer_complete{false};
  bool forest_materialized{false};
  bool forest_conditional_h0_candidate_certified{false};
  bool no_source_window_or_batch_delta_retained{false};
  bool forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};
  bool public_status_claimed{false};
  ExactDirectMorseTerminalForestReductionTimings timings{};
  ExactDirectMorseTerminalForestReductionDecision decision{
      ExactDirectMorseTerminalForestReductionDecision::not_reduced};

  [[nodiscard]] bool certified_reduction() const noexcept;

  friend bool operator==(
      const ExactDirectMorseTerminalForestReductionAudit&,
      const ExactDirectMorseTerminalForestReductionAudit&) = default;
};

struct ExactDirectMorseTerminalForestReductionResult {
  ExactDirectMorseTerminalForestReductionAudit audit{};
  std::optional<ExactDirectMorseForestJournalResult> forest;

  [[nodiscard]] bool certified_reduction() const noexcept;
};

// Closes the existing product seam from a certified terminal-source bridge to
// the resident conditional H0 forest.  It builds no global facet, coface,
// incidence, Gamma, cell or higher-order Delaunay structure.  The returned
// forest keeps the existing conditional status: this function does not prove
// the separate global Morse/carrier-faithfulness obligation and never claims
// public exactness.
[[nodiscard]] ExactDirectMorseTerminalForestReductionResult
reduce_exact_direct_morse_terminal_source_to_forest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactDirectMorseTerminalReducerSourceBridge& source_bridge,
    const ExactDirectMorseTerminalForestReductionConfig& config);

}  // namespace morsehgp3d::hierarchy
