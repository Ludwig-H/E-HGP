#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"
#include "morsehgp3d/hierarchy/direct_morse_event_vertical_propagation_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_gamma_carrier_conformance.hpp"
#include "morsehgp3d/hierarchy/direct_morse_k2_k1_target_authority.hpp"
#include "morsehgp3d/hierarchy/direct_morse_m1_o5_death_accounting.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_pipeline.hpp"
#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_durable_run_archive.hpp"
#if MORSEHGP3D_RUNNER_HAS_DEVICE_TILED_HIGHER
#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"
#endif
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

constexpr std::uint64_t locator_authority_id =
    UINT64_C(0x4D485047503344);

// The device headers exist only in a CUDA build, so the option layer states
// the two sealed bridge constants literally; the device branch static_asserts
// them against the real ones.
inline constexpr std::size_t runner_default_higher_tile_target = 16U;
inline constexpr std::size_t runner_maximum_higher_tile_target = 1024U;

struct Options {
  std::string family{"uniform_latin"};
  std::string mode{"resident_timed"};
  std::size_t point_count{};
  std::size_t maximum_order{10U};
  std::size_t support_work_budget{20'000U};
  std::size_t support_record_budget{4'096U};
  std::size_t higher_chunk_limit{1U};
  std::size_t downstream_record_budget{1'000'000U};
  std::size_t descent_work_budget{65'536U};
  std::uint64_t chunk_byte_budget{UINT64_C(1) << 30U};
  std::uint64_t operational_deadline_ms{};
  std::string archive_directory{};
  std::string higher_backend{"host_fixed_chunk"};
  std::string higher_verification_basis{"fresh_cpu_replay"};
  std::size_t higher_tile_target{runner_default_higher_tile_target};
  bool point_count_supplied{false};
  // SPECIFICATION_MORSEHGP3D.md 1.1: the exact industrial version carries no
  // configured budget at all.  `bounded` keeps the fail-fast diagnostic caps
  // above; `unbudgeted_industrial` replaces every one of the six axes with
  // the representational ceiling and is the ONLY profile a product
  // measurement may be taken on.
  std::string budget_profile{"bounded"};
  bool support_work_budget_supplied{false};
  bool support_record_budget_supplied{false};
  bool higher_chunk_limit_supplied{false};
  bool downstream_record_budget_supplied{false};
  bool descent_work_budget_supplied{false};
  bool chunk_byte_budget_supplied{false};
};

[[nodiscard]] bool unbudgeted_industrial(const Options& options) noexcept {
  return options.budget_profile == "unbudgeted_industrial";
}

[[nodiscard]] constexpr std::size_t representational_ceiling() noexcept {
  return std::numeric_limits<std::size_t>::max();
}

struct Timings {
  double generation_ms{};
  double canonicalization_ms{};
  double lbvh_ms{};
  double pair_support_ms{};
  double higher_support_ms{};
  double terminal_facade_ms{};
  double event_journal_ms{};
  double saddle_seed_journal_ms{};
  double batch_plan_ms{};
  double reducer_setup_ms{};
  double reducer_stream_ms{};
  double forest_finish_ms{};
  double archive_finalize_ms{};
  double archive_reload_ms{};
  double event_rank_tower_link_journal_ms{};
  double event_vertical_propagation_journal_ms{};
  double event_vertical_propagation_fresh_verification_ms{};
  double conditional_adjacent_event_vertical_propagation_ms{};
  double vertical_target_pipeline_ms{};
  double vertical_journal_ms{};
  double k2_to_k1_oracle_source_history_ms{};
  double k2_to_k1_oracle_k1_ms{};
  double k2_to_k1_oracle_hierarchy_ms{};
  double k2_to_k1_target_authority_build_ms{};
  double k2_to_k1_target_authority_verify_ms{};
  double k2_to_k1_target_authority_ms{};
  double m1_o5_death_accounting_build_ms{};
  double m1_o5_death_accounting_verify_ms{};
  double m1_o5_death_accounting_ms{};
  double direct_morse_gamma_carrier_conformance_ms{};
  double vertical_target_replay_diagnostic_ms{};
  double total_ms{};
};

struct Report {
  Options options;
  Timings timings;
  std::string terminal_stage{"not_started"};
  std::string stop_category{"none"};
  std::string stop_detail{"none"};
  bool pipeline_complete{false};
  bool resident_conditional_pipeline_complete{false};
  bool budget_exhausted{false};
  bool scientific_result_materialized{false};
  bool conditional_h0_candidate_certified{false};
  bool architecture_audit_complete{true};
  bool no_forbidden_global_structure_materialized{true};
  bool complete_hierarchy_attempt_requested{false};
  bool bounded_k2_to_k1_target_authority_qualification_requested{false};
  bool bounded_m1_o5_death_accounting_qualification_requested{false};
  bool bounded_direct_gamma_carrier_conformance_qualification_requested{
      false};
  bool conditional_adjacent_event_vertical_propagation_qualification_requested{
      false};
  bool configurable_pair_total_caps_disabled{false};
  bool downstream_static_confidence_caps_enabled{true};
  bool operational_deadline_reached{false};
  std::size_t effective_higher_chunk_limit{};

  std::string pair_status{"not_run"};
  std::string pair_stop_reason{"none"};
  std::string pair_source_kind{"not_run"};
  std::string pair_authority_kind{"not_run"};
  std::size_t pair_maximum_closed_rank{};
  ExactMortonGroupedAnchoredPairScheduleConfig pair_schedule_config{};
  ExactSparseAnchoredPairSessionAdvanceBudget pair_advance_budget{};
  ExactSparseAnchoredPairSessionTotalCapacity pair_total_capacity{};
  std::size_t pair_advance_calls{};
  std::size_t pair_schedule_advances{};
  std::size_t pair_orientation_checks{};
  std::size_t pair_reverse_or_self_orientation_skips{};
  std::size_t pair_prepared_groups{};
  std::size_t pair_completed_groups{};
  std::size_t pair_grouped_traversal_node_visits{};
  std::size_t pair_witness_subtree_node_visits{};
  std::size_t pair_grouped_common_node_visits{};
  std::size_t pair_anchor_subgroup_node_visits{};
  std::size_t pair_singleton_node_visits{};
  std::size_t pair_grouped_witness_slots{};
  std::size_t pair_grouped_inherited_witness_reuses{};
  std::size_t pair_grouped_traversal_exact_predicates{};
  std::size_t pair_fp64_filtered_negative_predicates{};
  std::size_t pair_fp64_filtered_positive_predicates{};
  std::size_t pair_exact_fallback_predicates{};
  std::size_t pair_floating_witness_order_preparations{};
  std::size_t pair_floating_witness_score_evaluations{};
  std::size_t pair_floating_witness_nonfinite_scores{};
  bool pair_floating_witness_order_requested{false};
  bool pair_floating_witness_order_effective_for_every_prepared_traversal{
      true};
  bool pair_fp64_filter_partition_certified{false};
  std::size_t pair_witness_subtree_exact_predicates{};
  std::size_t pair_witness_subtree_receipts{};
  std::size_t pair_witness_subtree_successes{};
  std::size_t pair_witness_subtree_fail_opens{};
  std::size_t pair_grouped_common_exact_predicates{};
  std::size_t pair_anchor_subgroup_exact_predicates{};
  std::size_t pair_singleton_exact_predicates{};
  std::size_t pair_grouped_strict_witness_discoveries{};
  std::size_t pair_grouped_diagonal_node_descents{};
  std::size_t pair_grouped_common_frontiers{};
  std::size_t pair_delegated_frontier_anchors{};
  std::size_t pair_prepared_anchor_subgroup_probes{};
  std::size_t pair_anchor_subgroup_witness_pool_entries{};
  std::size_t pair_query_facing_fallback_witness_pool_entries{};
  std::size_t pair_anchor_subgroup_splits{};
  std::size_t pair_query_subtree_splits{};
  std::size_t pair_anchor_subgroup_certified_prunes{};
  std::size_t pair_anchor_subgroup_certified_anchors{};
  std::size_t pair_prepared_singleton_fallbacks{};
  std::size_t pair_completed_singleton_fallbacks{};
  std::size_t pair_singleton_witness_pool_entries{};
  std::size_t pair_singleton_certified_prunes{};
  std::size_t pair_maximum_pending_anchor_subgroups{};
  std::size_t pair_triangular_block_pair_visits{};
  std::size_t pair_triangular_diagonal_splits{};
  std::size_t pair_triangular_oversized_anchor_splits{};
  std::size_t pair_triangular_consumer_query_splits{};
  std::size_t pair_triangular_self_pairs{};
  std::size_t pair_triangular_cross_blocks{};
  std::size_t pair_triangular_certified_cross_blocks{};
  std::size_t pair_triangular_certified_unordered_pairs{};
  std::size_t pair_triangular_opened_singleton_cross_blocks{};
  std::size_t pair_triangular_opened_unordered_pairs{};
  std::size_t pair_triangular_maximum_pending_block_pairs{};
  bool pair_triangular_partition_complete{false};
  bool pair_no_dynamic_dual_tree_or_pair_arena{true};
  std::size_t pair_authenticated_prunes{};
  std::size_t pair_authenticated_pruned_directed_pairs{};
  std::size_t pair_directed_pair_universe{};
  std::size_t pair_admitted_candidates{};
  std::size_t pair_classification_advances{};
  std::size_t pair_classification_node_visits{};
  std::size_t pair_classification_terminals{};
  std::size_t pair_above_rank{};
  std::size_t pair_output_records{};
  std::size_t pair_output_point_id_references{};
  std::size_t pair_local_budget_exhaustions{};
  std::size_t pair_total_capacity_exhaustions{};
  std::size_t pair_maximum_live_candidates{};
  std::size_t pair_accepted_events{};
  std::size_t pair_extra_shell_diagnostics{};
  bool pair_directed_coverage_certified{false};
  bool pair_orientation_partition_certified{false};
  bool pair_classification_partition_certified{false};
  bool pair_output_partition_certified{false};
  bool pair_records_certified{false};

  std::string higher_status{"not_run"};
  std::string higher_stop_reason{"none"};
  std::size_t higher_work_units{};
  std::size_t higher_product_visits{};
  std::size_t higher_closed_ball_queries{};
  std::size_t higher_accepted_events{};
  std::size_t higher_extra_shell_diagnostics{};
  std::size_t higher_prune_certificates{};
  std::size_t higher_chunk_count{};
  std::string higher_authority_kind{"not_run"};
  // Copied from the sealed certificate, never from the requested option, so
  // a run can never report a stronger basis than the one it actually used.
  std::string higher_verification_basis{"not_run"};
  std::size_t higher_requested_tile_target{};
  // R2-c progress ventilation of the device-tiled assembly.  Published on
  // every outcome, including a censored one, so that a run which does not
  // reach the terminal still says exactly how far it got.  The three
  // masses are canonical decimal exact::BigInt strings: their ratio is the
  // exact fraction of C(n,3)+C(n,4) decided before the stop.
  std::string higher_assembly_censure{"none"};
  std::string higher_total_support_mass{"0"};
  std::string higher_resolved_support_mass{"0"};
  std::string higher_remaining_frontier_support_mass{"0"};
  std::size_t higher_minimal_leaf_count{};
  std::size_t higher_above_rank_leaf_count{};
  std::size_t higher_leaf_support_analyses{};
  std::size_t higher_point_classifications{};
  std::size_t higher_closed_ball_node_visits{};
  std::size_t higher_committed_transactions{};
  std::size_t higher_committed_tile_transactions{};
  std::size_t higher_committed_expansion_transitions{};
  std::size_t higher_committed_tile_slots{};
  std::size_t higher_frontier_entries{};

  bool terminal_catalog_certified{false};
  std::size_t canonical_point_count{};
  std::size_t effective_maximum_order{};
  std::size_t terminal_event_count{};
  std::size_t terminal_extra_shell_diagnostic_count{};
  std::size_t event_batch_count{};
  std::size_t event_role_count{};
  std::size_t saddle_family_count{};
  std::size_t arm_seed_count{};
  std::size_t industrial_chunk_count{};
  std::size_t plan_lane_count{};
  std::size_t committed_batch_count{};
  std::size_t prepared_ticket_count{};
  unsigned industrial_plan_decision{};
  unsigned plan_decision{};
  unsigned batch_execution_decision{};
  unsigned preparation_decision{};
  unsigned reducer_fold_decision{};
  unsigned live_commit_decision{};

  std::size_t forest_birth_count{};
  std::size_t forest_materialized_birth_count{};
  std::size_t forest_saddle_count{};
  std::size_t forest_atomic_group_count{};
  std::size_t forest_node_count{};
  std::size_t forest_materialized_node_count{};
  std::size_t forest_final_root_count{};
  std::size_t forest_logical_output_entry_count{};
  std::size_t forest_aggregate_closure_node_count{};
  std::size_t forest_aggregate_closure_step_call_count{};

  bool durable_archive_requested{false};
  bool durable_archive_published{false};
  bool durable_identity_recertified{false};
  std::size_t durable_segment_count{};
  std::size_t durable_reloaded_segment_count{};
  std::string durable_archive_digest_hex{"unavailable"};

  bool conditional_adjacent_event_vertical_propagation_attempted{false};
  bool conditional_adjacent_event_vertical_propagation_certified{false};
  std::optional<ExactDirectMorseEventRankTowerLinkJournalResult>
      event_rank_tower_link_journal;
  std::optional<ExactDirectMorseEventVerticalPropagationJournalResult>
      event_vertical_propagation_journal;
  ExactDirectMorseEventVerticalPropagationFreshForestVerification
      event_vertical_propagation_fresh_verification{};

  bool vertical_target_pipeline_attempted{false};
  bool vertical_target_pipeline_certified{false};
  unsigned vertical_target_pipeline_decision{};
  ExactDirectMorseVerticalTargetProposalPipelineCounters
      vertical_target_pipeline_counters{};
  std::size_t vertical_target_required_session_count{};
  std::size_t vertical_target_required_group_count{};
  std::size_t vertical_target_required_proposal_count{};
  std::vector<std::size_t> vertical_target_source_group_count_by_order;
  std::optional<std::size_t> vertical_target_rejected_source_group_index;
  unsigned vertical_target_rejected_closure_adapter_decision{};

  bool vertical_journal_attempted{false};
  bool vertical_journal_certified{false};
  unsigned vertical_journal_decision{};
  std::size_t vertical_journal_adjacent_family_count{};
  std::size_t vertical_journal_label_resolution_count{};
  std::size_t vertical_journal_group_check_count{};
  std::size_t vertical_journal_checkpoint_count{};
  std::size_t vertical_journal_logical_output_entry_count{};
  ExactDirectMorseVerticalCounters vertical_journal_counters{};
  bool vertical_journal_source_forest_shape_replayed{false};
  bool vertical_journal_conditional_on_fresh_source_forest_replay{false};
  bool vertical_journal_external_target_authority_replayed{false};
  bool vertical_journal_global_morse_obligation_replayed{false};
  bool vertical_journal_all_naturality_squares_replayed{false};
  bool vertical_journal_vertical_maps_complete{false};
  bool vertical_journal_gamma_or_global_cofaces_materialized{false};
  bool vertical_journal_higher_order_delaunay_materialized{false};
  bool vertical_journal_public_status_claimed{false};

  bool k2_to_k1_target_authority_required{false};
  bool k2_to_k1_target_authority_attempted{false};
  bool k2_to_k1_target_authority_certified{false};
  unsigned k2_to_k1_target_authority_decision{};
  unsigned k2_to_k1_target_authority_scope{};
  std::size_t k2_to_k1_oracle_source_batch_count{};
  std::size_t k2_to_k1_oracle_source_group_count{};
  std::size_t k2_to_k1_oracle_source_node_count{};
  std::size_t k2_to_k1_oracle_k1_node_count{};
  std::size_t k2_to_k1_oracle_external_checkpoint_count{};
  std::string k2_to_k1_direct_cloud_digest;
  std::string k2_to_k1_external_cloud_digest;
  std::string k2_to_k1_external_hierarchy_digest;
  std::size_t k2_to_k1_required_logical_output_entry_count{};
  ExactDirectMorseK2K1TargetAuthorityCounters
      k2_to_k1_target_authority_counters{};
  std::optional<std::size_t>
      k2_to_k1_rejected_label_resolution_index;
  std::optional<std::size_t> k2_to_k1_rejected_atomic_group_index;
  std::optional<std::size_t> k2_to_k1_rejected_source_binding_index;
  bool k2_to_k1_direct_pipeline_freshly_replayed{false};
  bool k2_to_k1_direct_vertical_journal_freshly_replayed{false};
  bool k2_to_k1_external_hierarchy_freshly_replayed{false};
  bool k2_to_k1_canonical_point_namespace_identity_certified{false};
  bool k2_to_k1_all_observed_labels_present{false};
  bool k2_to_k1_all_observed_labels_resolved{false};
  bool k2_to_k1_every_direct_target_coverage_reconstructed{false};
  bool k2_to_k1_every_external_target_coverage_reconstructed{false};
  bool k2_to_k1_every_observed_target_coverage_equal{false};
  bool k2_to_k1_observed_label_target_authority_replayed{false};
  bool k2_to_k1_bidirectional_gamma_group_completeness_replayed{false};
  bool k2_to_k1_silent_gamma_checkpoint_completeness_replayed{false};
  bool k2_to_k1_external_target_authority_replayed{false};
  bool k2_to_k1_global_morse_obligation_replayed{false};
  bool k2_to_k1_all_naturality_squares_replayed{false};
  bool k2_to_k1_vertical_maps_complete{false};
  bool k2_to_k1_global_m1_claimed{false};
  bool k2_to_k1_bounded_exhaustive_gamma_oracle_used{false};
  bool k2_to_k1_gamma_cells_or_global_cofaces_persisted{false};
  bool k2_to_k1_higher_order_delaunay_materialized{false};
  bool k2_to_k1_public_status_claimed{false};
  bool k2_to_k1_no_partial_scientific_payload_on_failure{false};
  ExactDirectMorseK2K1TargetAuthorityVerification
      k2_to_k1_target_authority_verification{};

  bool m1_o5_death_accounting_required{false};
  bool m1_o5_death_accounting_attempted{false};
  bool m1_o5_death_accounting_certified{false};
  ExactDirectMorseM1O5DeathAccountingResult m1_o5_death_accounting{};
  ExactDirectMorseM1O5DeathAccountingVerification
      m1_o5_death_accounting_verification{};

  bool direct_morse_gamma_carrier_conformance_required{false};
  bool direct_morse_gamma_carrier_conformance_attempted{false};
  bool direct_morse_gamma_carrier_conformance_certified{false};
  ExactDirectMorseGammaCarrierConformanceResult
      direct_morse_gamma_carrier_conformance{};

  bool vertical_target_replay_diagnostic_attempted{false};
  bool vertical_target_replay_diagnostic_callback_invoked{false};
  bool vertical_target_replay_diagnostic_advance_certified{false};
  std::optional<std::size_t> vertical_target_diagnostic_source_group_index;
  std::optional<std::size_t> vertical_target_diagnostic_source_batch_index;
  std::size_t vertical_target_diagnostic_source_order{};
  std::size_t vertical_target_diagnostic_target_order{};
  std::string vertical_target_diagnostic_level_numerator{"0"};
  std::string vertical_target_diagnostic_level_denominator{"1"};
  std::uint64_t vertical_target_diagnostic_query_token{};
  unsigned vertical_target_diagnostic_plan_decision{};
  unsigned vertical_target_diagnostic_session_initialization_decision{};
  unsigned vertical_target_diagnostic_advance_decision{};
  unsigned vertical_target_diagnostic_closure_adapter_decision{};
  unsigned vertical_target_diagnostic_inner_closure_decision{};
  unsigned vertical_target_diagnostic_inner_closure_disposition{};
  std::vector<std::size_t>
      vertical_target_diagnostic_representative_binding_indices;
  std::vector<ExactDirectSparseFacetKey>
      vertical_target_diagnostic_source_keys;
  std::vector<std::size_t>
      vertical_target_diagnostic_target_facet_indices;
  std::vector<ExactDirectSparseFacetKey>
      vertical_target_diagnostic_target_keys;
  ExactDirectSparseFacetDescentClosureCounters
      vertical_target_diagnostic_closure_counters{};
  std::optional<ExactDirectSparseFacetDescentContradictionWitness>
      vertical_target_diagnostic_contradiction_witness;
};

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>{duration}.count();
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view description) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{description});
  }
  return left + right;
}

[[nodiscard]] bool three_sum_matches(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t expected) noexcept {
  return second <= std::numeric_limits<std::size_t>::max() - first &&
      third <= std::numeric_limits<std::size_t>::max() - first - second &&
      first + second + third == expected;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view description) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{description});
  }
  return left * right;
}

[[nodiscard]] bool complete_resident_diagnostic(
    const Options& options) noexcept {
  return options.mode == "complete_resident_diagnostic";
}

[[nodiscard]] bool durable_archive_recertification(
    const Options& options) noexcept {
  return options.mode == "durable_archive_recertification";
}

[[nodiscard]] bool bounded_k2_to_k1_target_authority_qualification(
    const Options& options) noexcept {
  return options.mode ==
      "bounded_k2_k1_target_authority_qualification";
}

[[nodiscard]] bool bounded_m1_o5_death_accounting_qualification(
    const Options& options) noexcept {
  return options.mode ==
      "bounded_m1_o5_death_accounting_qualification";
}

[[nodiscard]] bool bounded_direct_gamma_carrier_conformance_qualification(
    const Options& options) noexcept {
  return options.mode ==
      "bounded_direct_gamma_carrier_conformance_qualification";
}

[[nodiscard]] bool
conditional_adjacent_event_vertical_propagation_qualification(
    const Options& options) noexcept {
  return options.mode ==
      "conditional_adjacent_event_vertical_propagation_qualification";
}

[[nodiscard]] Report make_report(const Options& options) {
  Report report;
  report.options = options;
  report.complete_hierarchy_attempt_requested =
      complete_resident_diagnostic(options);
  report.bounded_k2_to_k1_target_authority_qualification_requested =
      bounded_k2_to_k1_target_authority_qualification(options);
  report.bounded_m1_o5_death_accounting_qualification_requested =
      bounded_m1_o5_death_accounting_qualification(options);
  report
      .bounded_direct_gamma_carrier_conformance_qualification_requested =
      bounded_direct_gamma_carrier_conformance_qualification(options);
  report
      .conditional_adjacent_event_vertical_propagation_qualification_requested =
      conditional_adjacent_event_vertical_propagation_qualification(options);
  report.k2_to_k1_target_authority_required =
      bounded_k2_to_k1_target_authority_qualification(options);
  report.m1_o5_death_accounting_required =
      bounded_m1_o5_death_accounting_qualification(options);
  report.direct_morse_gamma_carrier_conformance_required =
      bounded_direct_gamma_carrier_conformance_qualification(options);
  // The core result defaults this caller-relative premise to true.  A mode
  // that did not execute O5 must instead publish a completely dormant receipt.
  report.m1_o5_death_accounting
      .conditional_on_caller_fresh_phase9_facade_replay = false;
  report.configurable_pair_total_caps_disabled =
      complete_resident_diagnostic(options);
  report.higher_requested_tile_target = options.higher_tile_target;
  report.effective_higher_chunk_limit =
      complete_resident_diagnostic(options)
          ? std::numeric_limits<std::size_t>::max()
          : options.higher_chunk_limit;
  return report;
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    std::string_view option) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::invalid_argument(
        std::string{option} + " expects an unsigned integer");
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(
    std::string_view text,
    std::string_view option) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::invalid_argument(
        std::string{option} + " expects an unsigned integer");
  }
  return value;
}

void print_usage(std::ostream& output) {
  output
      << "usage: morsehgp3d_direct_morse_product_runner "
         "--point-count N [options]\n"
      << "  --mode resident_timed|complete_resident_diagnostic|"
         "bounded_k2_k1_target_authority_qualification|"
         "bounded_m1_o5_death_accounting_qualification|"
         "bounded_direct_gamma_carrier_conformance_qualification|"
         "conditional_adjacent_event_vertical_propagation_qualification|"
         "durable_archive_recertification\n"
      << "  --family uniform_latin|eight_clusters\n"
      << "  --maximum-order K (alias: --K; 1 <= K <= 10)\n"
      << "  --support-work-budget N (cap for each P8l work axis)\n"
      << "  --support-record-budget N (P8l output-record cap)\n"
      << "  --higher-chunk-limit N\n"
      << "  --downstream-record-budget N\n"
      << "  --descent-work-budget N\n"
      << "  --chunk-byte-budget N\n"
      << "  --operational-deadline-ms N (required for complete diagnostic)\n"
      << "  --archive-directory DIR (required for "
         "durable_archive_recertification)\n"
      << "  --higher-backend host_fixed_chunk|device_tiled_session\n"
      << "  --higher-verification-basis fresh_cpu_replay|tile_certified"
         " (tile_certified requires the device backend)\n"
      << "  --higher-tile-target N (frontier entries pre-expanded before a"
         " device tile; 1 <= N <= 1024)\n"
      << "  --budget-profile bounded|unbudgeted_industrial (unbudgeted "
         "rejects every explicit --*-budget option and is the only profile "
         "a product measurement may be taken on)\n"
      << "Default caps are fail-fast diagnostics, not a 50k "
         "qualification envelope.\n";
}

void parse_options(int argc, char** argv, Options& options) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--help" || option == "-h") {
      print_usage(std::cout);
      std::exit(0);
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{option} + " requires one value");
    }
    const std::string_view value{argv[++index]};
    if (option == "--point-count") {
      options.point_count = parse_size(value, option);
      options.point_count_supplied = true;
    } else if (option == "--family") {
      options.family = value;
    } else if (option == "--mode") {
      options.mode = value;
    } else if (option == "--maximum-order" || option == "--K") {
      options.maximum_order = parse_size(value, option);
    } else if (option == "--support-work-budget") {
      options.support_work_budget = parse_size(value, option);
      options.support_work_budget_supplied = true;
    } else if (option == "--support-record-budget") {
      options.support_record_budget = parse_size(value, option);
      options.support_record_budget_supplied = true;
    } else if (option == "--higher-chunk-limit") {
      options.higher_chunk_limit = parse_size(value, option);
      options.higher_chunk_limit_supplied = true;
    } else if (option == "--downstream-record-budget") {
      options.downstream_record_budget = parse_size(value, option);
      options.downstream_record_budget_supplied = true;
    } else if (option == "--descent-work-budget") {
      options.descent_work_budget = parse_size(value, option);
      options.descent_work_budget_supplied = true;
    } else if (option == "--chunk-byte-budget") {
      options.chunk_byte_budget = parse_u64(value, option);
      options.chunk_byte_budget_supplied = true;
    } else if (option == "--budget-profile") {
      options.budget_profile = value;
    } else if (option == "--archive-directory") {
      options.archive_directory = value;
    } else if (option == "--higher-backend") {
      options.higher_backend = value;
    } else if (option == "--higher-verification-basis") {
      options.higher_verification_basis = value;
    } else if (option == "--higher-tile-target") {
      options.higher_tile_target = parse_size(value, option);
    } else if (option == "--operational-deadline-ms") {
      options.operational_deadline_ms = parse_u64(value, option);
    } else {
      throw std::invalid_argument(
          "unknown option: " + std::string{option});
    }
  }
  if (!options.point_count_supplied || options.point_count == 0U) {
    throw std::invalid_argument(
        "--point-count must be supplied and strictly positive");
  }
  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  if (options.mode != "resident_timed" &&
      options.mode != "complete_resident_diagnostic" &&
      options.mode !=
          "bounded_k2_k1_target_authority_qualification" &&
      options.mode !=
          "bounded_m1_o5_death_accounting_qualification" &&
      options.mode !=
          "bounded_direct_gamma_carrier_conformance_qualification" &&
      options.mode !=
          "conditional_adjacent_event_vertical_propagation_qualification" &&
      options.mode != "durable_archive_recertification") {
    throw std::invalid_argument(
        "--mode must be resident_timed, complete_resident_diagnostic, or "
        "bounded_k2_k1_target_authority_qualification, or "
        "bounded_m1_o5_death_accounting_qualification, or "
        "bounded_direct_gamma_carrier_conformance_qualification, or "
        "conditional_adjacent_event_vertical_propagation_qualification, or "
        "durable_archive_recertification");
  }
  if (options.mode == "durable_archive_recertification" &&
      options.archive_directory.empty()) {
    throw std::invalid_argument(
        "durable_archive_recertification requires --archive-directory");
  }
  if (options.mode != "durable_archive_recertification" &&
      !options.archive_directory.empty()) {
    throw std::invalid_argument(
        "--archive-directory is only meaningful for "
        "durable_archive_recertification");
  }
  if (options.higher_backend != "host_fixed_chunk" &&
      options.higher_backend != "device_tiled_session") {
    throw std::invalid_argument(
        "--higher-backend must be host_fixed_chunk or "
        "device_tiled_session");
  }
  if (options.higher_verification_basis != "fresh_cpu_replay" &&
      options.higher_verification_basis != "tile_certified") {
    throw std::invalid_argument(
        "--higher-verification-basis must be fresh_cpu_replay or "
        "tile_certified");
  }
  if (options.higher_verification_basis == "tile_certified" &&
      options.higher_backend != "device_tiled_session") {
    throw std::invalid_argument(
        "--higher-verification-basis tile_certified requires "
        "--higher-backend device_tiled_session");
  }
  if (options.higher_tile_target == 0U ||
      options.higher_tile_target > runner_maximum_higher_tile_target) {
    throw std::invalid_argument(
        "--higher-tile-target must be in [1, 1024]");
  }
  if (options.maximum_order == 0U ||
      options.maximum_order >
          higher_support_maximum_requested_order) {
    throw std::invalid_argument(
        "--maximum-order must be in [1, 10]");
  }
  if (options.budget_profile != "bounded" &&
      options.budget_profile != "unbudgeted_industrial") {
    throw std::invalid_argument(
        "--budget-profile must be bounded or unbudgeted_industrial");
  }
  if (unbudgeted_industrial(options)) {
    // No half-uncapping.  A run that carried one explicit cap while
    // declaring the unbudgeted profile could not honestly claim that no
    // configured budget bounded it, so the combination is a caller error.
    if (options.support_work_budget_supplied ||
        options.support_record_budget_supplied ||
        options.higher_chunk_limit_supplied ||
        options.downstream_record_budget_supplied ||
        options.descent_work_budget_supplied ||
        options.chunk_byte_budget_supplied) {
      throw std::invalid_argument(
          "--budget-profile unbudgeted_industrial rejects every explicit "
          "budget option; the exact industrial version carries none");
    }
    // The six axes become the representational ceiling.  Every downstream
    // capacity is a copy, a minimum or a small-factor product of these, so
    // the three products that would wrap are guarded at their factory.
    options.support_work_budget = representational_ceiling();
    options.support_record_budget = representational_ceiling();
    options.higher_chunk_limit = representational_ceiling();
    options.downstream_record_budget = representational_ceiling();
    options.descent_work_budget = representational_ceiling();
    options.chunk_byte_budget =
        std::numeric_limits<std::uint64_t>::max();
  }
  if (options.support_work_budget == 0U ||
      options.support_record_budget == 0U ||
      options.higher_chunk_limit == 0U ||
      options.downstream_record_budget == 0U ||
      options.descent_work_budget == 0U ||
      options.chunk_byte_budget == 0U) {
    throw std::invalid_argument(
        "all operational budgets must be strictly positive");
  }
  if (complete_resident_diagnostic(options) &&
      options.operational_deadline_ms == 0U) {
    throw std::invalid_argument(
        "complete_resident_diagnostic requires a positive operational deadline");
  }
  if (options.operational_deadline_ms >
      static_cast<std::uint64_t>(
          std::numeric_limits<std::chrono::milliseconds::rep>::max())) {
    throw std::invalid_argument(
        "--operational-deadline-ms exceeds the steady-clock duration range");
  }
}

[[nodiscard]] std::string_view pair_stop_reason_text(
    ExactSparseAnchoredPairSessionStopReason reason) {
  switch (reason) {
    case ExactSparseAnchoredPairSessionStopReason::none:
      return "none";
    case ExactSparseAnchoredPairSessionStopReason::schedule_advance_limit:
      return "schedule_advance_limit";
    case ExactSparseAnchoredPairSessionStopReason::orientation_check_limit:
      return "orientation_check_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        grouped_traversal_node_visit_limit:
      return "grouped_traversal_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        grouped_traversal_exact_predicate_limit:
      return "grouped_traversal_exact_predicate_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        classification_node_visit_limit:
      return "classification_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        total_schedule_advance_capacity:
      return "total_schedule_advance_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_orientation_check_capacity:
      return "total_orientation_check_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_grouped_traversal_node_visit_capacity:
      return "total_grouped_traversal_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_grouped_traversal_exact_predicate_capacity:
      return "total_grouped_traversal_exact_predicate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_admitted_candidate_capacity:
      return "total_admitted_candidate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_classification_node_visit_capacity:
      return "total_classification_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_output_record_capacity:
      return "total_output_record_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_output_point_id_reference_capacity:
      return "total_output_point_id_reference_capacity";
  }
  return "invalid";
}

[[nodiscard]] bool industrial_plan_budget_failure(
    ExactDirectMorseIndustrialPlanDecision decision) {
  return decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_resident_requirements_not_met ||
         decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_atomic_batch_exceeds_chunk_budget ||
         decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_chunk_count_budget_exhausted;
}

[[nodiscard]] bool batch_execution_budget_failure(
    ExactDirectSparseFacetDescentBatchExecutionDecision decision) {
  return decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_batch_budget_exhausted ||
         decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_shared_closure_budget_exhausted;
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptResult
empty_proposal_transcript(
    std::size_t source_batch_index,
    const morsehgp3d::exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::array<ExactDirectSparseFacetTopKProposalRecord, 0U> records{};
  return build_exact_direct_sparse_facet_top_k_proposal_transcript(
      {source_batch_index,
       closed_batch_squared_level,
       locator.snapshot_stamp()},
      records,
      {0U, 0U, 0U, 0U, 0U});
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget make_seed_budget(
    const CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& facade,
    std::size_t downstream_cap) {
  const std::size_t event_count = facade.events.size();
  const std::size_t replay_bound = checked_add(
      checked_multiply(
          3U, cloud.size(), "seed replay bound overflow"),
      checked_multiply(
          5U, event_count, "seed replay bound overflow"),
      "seed replay bound overflow");
  std::size_t role_count = cloud.size();
  std::size_t family_count = 0U;
  std::size_t arm_count = 0U;
  for (const ExactDirectSupportEvent& event : facade.events) {
    if (event.birth_order.has_value()) {
      role_count =
          checked_add(role_count, 1U, "role count overflow");
    }
    if (event.saddle_order.has_value()) {
      role_count =
          checked_add(role_count, 1U, "role count overflow");
      family_count =
          checked_add(family_count, 1U, "family count overflow");
      arm_count = checked_add(
          arm_count,
          static_cast<std::size_t>(event.support_size),
          "arm count overflow");
    }
  }
  if (replay_bound > downstream_cap || role_count > downstream_cap ||
      family_count > downstream_cap || arm_count > downstream_cap) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_seed_journal");
  }
  return {replay_bound, role_count, family_count, arm_count};
}

[[nodiscard]] std::size_t make_support_frontier_capacity(
    const Options& options) {
  if (unbudgeted_industrial(options)) {
    return representational_ceiling();
  }
  return checked_add(
      checked_multiply(
          8U,
          options.support_work_budget,
          "support frontier capacity overflow"),
      2U,
      "support frontier capacity overflow");
}

[[nodiscard]] std::size_t make_higher_point_reference_capacity(
    const Options& options) {
  if (unbudgeted_industrial(options)) {
    return representational_ceiling();
  }
  return checked_multiply(
      options.support_record_budget,
      checked_add(
          options.maximum_order,
          4U,
          "support point-reference factor overflow"),
      "support point-reference capacity overflow");
}

[[nodiscard]] ExactMortonGroupedAnchoredPairScheduleConfig
make_sparse_pair_schedule_config() {
  return {32U, 64U, true, false, true, false, true};
}

[[nodiscard]] std::size_t make_sparse_pair_maximum_closed_rank(
    const Options& options) {
  return checked_add(
      options.maximum_order,
      1U,
      "sparse pair maximum closed rank overflow");
}

[[nodiscard]] std::size_t make_sparse_pair_point_reference_capacity(
    const Options& options) {
  if (unbudgeted_industrial(options)) {
    return representational_ceiling();
  }
  return checked_multiply(
      options.support_record_budget,
      checked_add(
          options.maximum_order,
          2U,
          "sparse pair point-reference factor overflow"),
      "sparse pair point-reference capacity overflow");
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget
make_sparse_pair_advance_budget(const Options& options) {
  return {
      {options.support_work_budget,
       options.support_work_budget,
       options.support_work_budget,
       options.support_work_budget},
      {options.support_work_budget},
      1U,
      checked_add(
          options.maximum_order,
          2U,
          "sparse pair per-advance reference capacity overflow"),
  };
}

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity
make_sparse_pair_total_capacity(const Options& options) {
  if (complete_resident_diagnostic(options)) {
    // The work axes do not yet have proved finite formulae tighter than the
    // representation.  This value disables the caller's diagnostic stop; it
    // is deliberately reported as a representational ceiling, never as a
    // mathematical completion envelope.
    const std::size_t representational_ceiling =
        std::numeric_limits<std::size_t>::max();
    return {
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
    };
  }
  return {
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_record_budget,
      make_sparse_pair_point_reference_capacity(options),
  };
}

[[nodiscard]] ExactHigherSupportStreamBudget make_higher_budget(
    const Options& options) {
  if (complete_resident_diagnostic(options) ||
      unbudgeted_industrial(options)) {
    // R2-e.  This mode is the one that must run without artificial caps,
    // and it already disables them on the pair axes -- but the higher
    // budget still derived every axis from --support-work-budget, and two
    // of those derivations are products: 8*W+2 for the frontier and W*n
    // for the point classifications.  At n = 50 000 any W above
    // 2^64 / 50 000 therefore made the runner throw an overflow BEFORE
    // reaching the pipeline, so the uncapped 50k campaign was
    // unreachable from the product binary.  Like the pair one, this value
    // disables the caller's diagnostic stop; it is deliberately reported
    // as a representational ceiling, never as a mathematical completion
    // envelope, and it certifies nothing.
    const std::size_t representational_ceiling =
        std::numeric_limits<std::size_t>::max();
    return {
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
        representational_ceiling,
    };
  }
  const std::size_t frontier_capacity =
      make_support_frontier_capacity(options);
  const std::size_t higher_point_classification_capacity =
      checked_multiply(
          options.support_work_budget,
          options.point_count,
          "higher-support point-classification capacity overflow");
  return {
      options.support_work_budget,
      frontier_capacity,
      frontier_capacity,
      options.support_record_budget,
      make_higher_point_reference_capacity(options),
      options.support_work_budget,
      options.support_work_budget,
      higher_point_classification_capacity,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
make_closure_budget(
    const Options& options,
    std::size_t locator_table_slot_count,
    std::size_t component_count) {
  const std::size_t closure_capacity = std::min(
      {options.downstream_record_budget,
       options.descent_work_budget,
       direct_sparse_facet_descent_closure_maximum_node_count});
  const std::size_t memo_slot_count = checked_add(
      checked_multiply(
          2U, closure_capacity, "closure memo capacity overflow"),
      1U,
      "closure memo capacity overflow");
  const std::size_t probe_slot_limit =
      std::min(locator_table_slot_count, options.descent_work_budget);
  const std::size_t parent_hop_limit =
      std::min(component_count, options.descent_work_budget);
  const ExactDirectSparsePositiveFacetProbeBudget probe_budget{
      probe_slot_limit, parent_hop_limit};
  const std::size_t neighbor_capacity =
      std::min(options.maximum_order, options.descent_work_budget);
  const std::size_t shell_capacity = std::min(
      options.downstream_record_budget,
      options.descent_work_budget);
  const ExactLbvhTopKBudget top_k_budget{
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      neighbor_capacity,
      shell_capacity};
  return {
      closure_capacity,
      closure_capacity,
      closure_capacity,
      memo_slot_count,
      {probe_budget, top_k_budget, probe_budget},
  };
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig
make_industrial_config(const Options& options) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy =
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k;
  config.memory_model = {
      64U,
      16U,
      16U,
      8U,
      16U,
      16U,
      16U,
      4U,
      16U,
      8U,
      16U,
      2U,
  };
  const std::uint64_t cap =
      static_cast<std::uint64_t>(
          options.downstream_record_budget);
  config.chunk_budget = {
      options.chunk_byte_budget,
      cap,
      cap,
      cap,
      cap,
      static_cast<std::uint64_t>(
          std::min(
              options.downstream_record_budget,
              options.descent_work_budget)),
  };
  return config;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget
make_plan_budget(
    const Options& options,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  const std::size_t lane_cap = checked_multiply(
      3U,
      event_journal.batches.size(),
      "batch-plan lane capacity overflow");
  return {
      1U,
      event_journal.batches.size(),
      seed_journal.families.size(),
      seed_journal.arm_seeds.size(),
      lane_cap,
      seed_journal.arm_seeds.size(),
      options.descent_work_budget,
      options.descent_work_budget,
  };
}

[[nodiscard]] ExactDirectMorseForestBudget make_forest_budget(
    const Options& options,
    const CanonicalPointCloud& cloud,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  const std::size_t direct_birth_count =
      static_cast<std::size_t>(std::count_if(
          event_journal.materialized_direct_role_records.begin(),
          event_journal.materialized_direct_role_records.end(),
          [](const ExactDirectMorseH0RoleRecord& role) {
            return role.role == ExactDirectMorseH0Role::birth;
          }));
  const std::size_t birth_count = checked_add(
      cloud.size(), direct_birth_count, "forest birth count overflow");
  const std::size_t family_count = seed_journal.families.size();
  const std::size_t arm_count = seed_journal.arm_seeds.size();
  const std::size_t batch_count = event_journal.batches.size();
  const std::size_t component_count = checked_add(
      birth_count, arm_count, "locator component capacity overflow");
  const std::size_t committed_binding_count = checked_add(
      birth_count, arm_count, "locator binding capacity overflow");
  const std::size_t table_slot_count = checked_add(
      checked_multiply(
          2U,
          committed_binding_count,
          "locator table capacity overflow"),
      1U,
      "locator table capacity overflow");
  const std::size_t maximum_batch_mutation_count = checked_add(
      birth_count, arm_count, "locator batch capacity overflow");
  const std::size_t batch_scratch_slot_count = checked_add(
      checked_multiply(
          2U,
          maximum_batch_mutation_count,
          "locator scratch capacity overflow"),
      1U,
      "locator scratch capacity overflow");
  const std::size_t key_point_count = checked_add(
      cloud.size(),
      checked_multiply(
          options.maximum_order,
          checked_add(
              direct_birth_count,
              arm_count,
              "locator key population overflow"),
          "locator key-point capacity overflow"),
      "locator key-point capacity overflow");
  const std::size_t node_count = checked_add(
      birth_count, family_count, "forest node capacity overflow");
  std::size_t logical_output_count = checked_multiply(
      13U, birth_count, "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      checked_multiply(
          12U, arm_count, "forest logical-output capacity overflow"),
      "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      checked_multiply(
          3U, family_count, "forest logical-output capacity overflow"),
      "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      batch_count,
      "forest logical-output capacity overflow");
  const std::size_t maximum_required_population =
      std::max(
          {birth_count,
           component_count,
           committed_binding_count,
           key_point_count,
           node_count,
           logical_output_count});
  if (maximum_required_population >
      options.downstream_record_budget) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_forest");
  }
  const ExactDirectSparseFacetDescentClosureBudget closure_budget =
      make_closure_budget(
          options, table_slot_count, component_count);
  const std::size_t closure_capacity =
      closure_budget.maximum_node_count;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count =
      event_journal.role_record_count;
  budget.maximum_source_batch_scan_count = batch_count;
  budget.maximum_source_family_scan_count = family_count;
  budget.maximum_source_arm_seed_scan_count = arm_count;
  budget.maximum_birth_record_count = birth_count;
  budget.maximum_arm_root_binding_count = arm_count;
  budget.maximum_saddle_record_count = family_count;
  budget.maximum_atomic_group_count = family_count;
  budget.maximum_child_reference_count = arm_count;
  budget.maximum_batch_record_count = batch_count;
  budget.maximum_node_count = node_count;
  budget.maximum_final_root_count =
      event_journal.effective_maximum_order;
  budget.maximum_batch_distinct_arm_count = arm_count;
  budget.maximum_logical_output_entry_count = logical_output_count;
  budget.maximum_aggregate_closure_node_count =
      options.downstream_record_budget;
  budget.maximum_aggregate_closure_step_call_count =
      options.downstream_record_budget;
  budget.locator_budget = {
      component_count,
      committed_binding_count,
      key_point_count,
      arm_count,
      checked_add(batch_count, 1U, "locator batch count overflow"),
      closure_capacity,
      maximum_batch_mutation_count,
      maximum_batch_mutation_count,
      checked_multiply(
          options.maximum_order,
          maximum_batch_mutation_count,
          "locator batch key-point capacity overflow"),
      table_slot_count,
      batch_scratch_slot_count,
  };
  budget.closure_budget = closure_budget;
  budget.quotient_budget = {
      family_count,
      arm_count,
      arm_count,
      arm_count,
      options.downstream_record_budget};
  return budget;
}

[[nodiscard]] ExactDirectMorseEventRankTowerLinkBudget
make_event_rank_tower_link_budget(
    const Options& options,
    const ExactDirectMorseForestJournalResult& forest) {
  const std::size_t logical_birth_count = checked_add(
      forest.implicit_order_one_prefix_count,
      forest.birth_records.size(),
      "event-rank link logical-birth capacity overflow");
  const std::size_t logical_node_count = checked_add(
      forest.implicit_order_one_prefix_count,
      forest.nodes.size(),
      "event-rank link logical-node capacity overflow");
  const std::size_t logical_output_count = checked_add(
      forest.birth_records.size(),
      forest.arm_root_bindings.size(),
      "event-rank link logical-output capacity overflow");
  const std::size_t maximum_required_population = std::max(
      {logical_birth_count,
       logical_node_count,
       forest.saddle_records.size(),
       forest.arm_root_bindings.size(),
       forest.atomic_groups.size(),
       forest.batches.size(),
       logical_output_count});
  if (maximum_required_population > options.downstream_record_budget) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_event_rank_tower_links");
  }
  return {
      logical_birth_count,
      forest.saddle_records.size(),
      forest.arm_root_bindings.size(),
      forest.atomic_groups.size(),
      forest.batches.size(),
      logical_node_count,
      forest.birth_records.size(),
      forest.arm_root_bindings.size(),
      logical_output_count};
}

[[nodiscard]] ExactDirectMorseEventVerticalPropagationBudget
make_event_vertical_propagation_budget(
    const Options& options,
    const ExactDirectMorseForestJournalResult& forest,
    const ExactDirectMorseEventRankTowerLinkJournalResult& links) {
  const std::size_t logical_node_count = checked_add(
      forest.implicit_order_one_prefix_count,
      forest.nodes.size(),
      "event-vertical logical-node capacity overflow");
  const std::size_t group_input_count = checked_multiply(
      2U,
      forest.arm_root_bindings.size(),
      "event-vertical group-input capacity overflow");
  std::size_t logical_output_count = checked_add(
      links.links.size(),
      forest.atomic_groups.size(),
      "event-vertical logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      group_input_count,
      "event-vertical logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      forest.final_roots.size(),
      "event-vertical logical-output capacity overflow");
  const std::size_t maximum_required_population = std::max(
      {links.links.size(),
       forest.atomic_groups.size(),
       forest.arm_root_bindings.size(),
       logical_node_count,
       forest.child_node_ids.size(),
       forest.final_roots.size(),
       group_input_count,
       logical_output_count});
  if (maximum_required_population > options.downstream_record_budget) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_event_vertical_propagation");
  }
  return {
      links.links.size(),
      forest.atomic_groups.size(),
      forest.arm_root_bindings.size(),
      logical_node_count,
      forest.child_node_ids.size(),
      forest.final_roots.size(),
      links.links.size(),
      forest.atomic_groups.size(),
      group_input_count,
      forest.final_roots.size(),
      forest.child_node_ids.size(),
      options.downstream_record_budget,
      logical_output_count};
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionBudget
make_execution_budget(
    const Options& options,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  return {
      3U,
      seed_journal.families.size(),
      seed_journal.arm_seeds.size(),
      checked_multiply(
          options.maximum_order,
          seed_journal.arm_seeds.size(),
          "execution key-reference capacity overflow"),
      seed_journal.arm_seeds.size(),
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget
vertical_target_descent_step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{65537U, 16384U},
      ExactLbvhTopKBudget{
          1000000U,
          1000000U,
          1000000U,
          1000000U,
          1000000U,
          16U,
          16384U},
      ExactDirectSparsePositiveFacetProbeBudget{65537U, 16384U},
  };
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalPipelineBudget
make_vertical_target_pipeline_budget(
    const ExactDirectMorseForestJournalResult& forest) {
  constexpr std::size_t capacity = 16384U;
  constexpr std::size_t group_capacity = 4096U;
  constexpr std::size_t large_capacity = 1000000U;
  ExactDirectMorseVerticalTargetProposalPipelineBudget budget;

  auto& carrier = budget.session_budget.carrier_state_budget;
  carrier.maximum_forest_birth_record_scan_count = capacity;
  carrier.maximum_forest_node_scan_count = capacity;
  carrier.maximum_forest_batch_scan_count = capacity;
  carrier.maximum_forest_atomic_group_scan_count = capacity;
  carrier.maximum_forest_saddle_scan_count = capacity;
  carrier.maximum_forest_arm_binding_scan_count = capacity;
  carrier.maximum_forest_child_reference_scan_count = capacity;
  carrier.maximum_forest_final_root_scan_count = capacity;
  carrier.maximum_component_state_count = capacity;
  carrier.maximum_node_marker_state_count = capacity;
  carrier.maximum_index_entry_count = capacity;
  carrier.maximum_group_carrier_scratch_count = capacity;
  carrier.maximum_group_prior_root_scratch_count = capacity;
  carrier.maximum_parent_hop_count = large_capacity;
  carrier.maximum_exact_level_comparison_count = large_capacity;
  carrier.maximum_single_exact_level_integer_bit_count = capacity;
  carrier.maximum_logical_output_entry_count = 65536U;

  budget.session_budget.locator_budget =
      forest.requested_budget.locator_budget;
  budget.session_budget.maximum_replayed_global_batch_count = capacity;
  budget.session_budget.maximum_replayed_locator_union_count = capacity;
  budget.session_budget.maximum_replayed_locator_binding_count = capacity;
  budget.session_budget.maximum_batch_group_plan_count = capacity;
  budget.session_budget.maximum_batch_group_carrier_reference_count =
      capacity;
  budget.session_budget.maximum_batch_group_prior_root_reference_count =
      capacity;
  budget.session_budget.maximum_batch_locator_union_scratch_count =
      capacity;
  budget.session_budget.maximum_batch_locator_binding_scratch_count =
      capacity;

  auto& plan = budget.facet_plan_budget;
  plan.maximum_group_saddle_scan_count = capacity;
  plan.maximum_group_arm_binding_scan_count = capacity;
  plan.maximum_binding_sort_scratch_count = group_capacity;
  plan.maximum_representative_binding_count = group_capacity;
  plan.maximum_projected_target_facet_reference_count = 65536U;
  plan.maximum_distinct_target_facet_count = group_capacity;
  plan.maximum_sort_comparison_count = large_capacity;
  plan.maximum_target_facet_lookup_comparison_count = large_capacity;
  plan.maximum_retained_key_point_reference_count = 131072U;
  plan.maximum_logical_output_entry_count = 131072U;

  auto& closure = budget.closure_budget;
  closure.maximum_canonical_key_scan_count = group_capacity;
  closure.maximum_terminal_summary_count = group_capacity;
  closure.maximum_terminal_key_point_reference_count = 131072U;
  closure.maximum_carrier_cut_lookup_count = group_capacity;
  closure.maximum_logical_output_entry_count = 65536U;
  closure.closure_budget = {
      group_capacity,
      65536U,
      65536U,
      131073U,
      vertical_target_descent_step_budget(),
  };

  auto& proposal = budget.proposal_budget;
  proposal.maximum_source_saddle_revalidation_count = capacity;
  proposal.maximum_source_binding_revalidation_count = capacity;
  proposal.maximum_source_key_lookup_comparison_count = large_capacity;
  proposal.maximum_closure_summary_scan_count = group_capacity;
  proposal.maximum_positive_terminal_probe_count = group_capacity;
  proposal.maximum_positive_terminal_probe_slot_visit_count =
      large_capacity;
  proposal.maximum_positive_terminal_probe_parent_hop_count =
      large_capacity;
  proposal.positive_terminal_probe_budget = {65537U, capacity};
  proposal.maximum_carrier_entry_revalidation_count = group_capacity;
  proposal.maximum_target_node_lookup_count = group_capacity;
  proposal.maximum_exact_level_comparison_count = large_capacity;
  proposal.maximum_single_exact_level_integer_bit_count = capacity;
  proposal.maximum_proposal_count = group_capacity;
  proposal.maximum_logical_output_entry_count = 65536U;

  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_atomic_group_scan_count = capacity;
  budget.maximum_referenced_target_order_count = 9U;
  budget.maximum_target_order_lookup_count = large_capacity;
  budget.maximum_preflight_facet_plan_count = capacity;
  budget.maximum_executed_facet_plan_count = capacity;
  budget.maximum_replay_advance_count = capacity;
  budget.maximum_closure_build_count = capacity;
  budget.maximum_proposal_adapter_count = capacity;
  budget.maximum_aggregate_representative_count = capacity;
  budget.maximum_aggregate_projected_target_facet_reference_count =
      163840U;
  budget.maximum_aggregate_distinct_target_facet_count = 163840U;
  budget.maximum_aggregate_retained_key_point_reference_count = 2000000U;
  budget.maximum_aggregate_plan_logical_output_entry_count = large_capacity;
  budget.maximum_aggregate_closure_terminal_summary_count = 163840U;
  budget.maximum_aggregate_proposal_count = capacity;
  budget.maximum_session_audit_count = 9U;
  budget.maximum_group_audit_count = capacity;
  budget.maximum_logical_output_entry_count = 65536U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalBudget
make_vertical_journal_budget() {
  constexpr std::size_t capacity = 16384U;
  return {
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
  };
}

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t n, std::size_t k) noexcept {
  if (k > n) {
    return 0U;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    value = value * (n - k + index) / index;
  }
  return value;
}

[[nodiscard]] std::size_t checked_power_of_two(
    std::size_t exponent,
    std::string_view description) {
  std::size_t value = 1U;
  for (std::size_t index = 0U; index < exponent; ++index) {
    value = checked_multiply(value, 2U, description);
  }
  return value;
}

[[nodiscard]] ExactDirectMorseGammaCarrierConformanceBudget
make_bounded_direct_gamma_carrier_conformance_budget(
    std::size_t point_count,
    std::size_t order) {
  const std::size_t facet_count = bounded_binomial(point_count, order);
  const std::size_t coface_count =
      bounded_binomial(point_count, order + 1U);
  const std::size_t activation_count = checked_add(
      facet_count,
      coface_count,
      "bounded direct--Gamma activation capacity overflow");
  const std::size_t exhaustive_run_count = checked_add(
      activation_count,
      1U,
      "bounded direct--Gamma exhaustive-run capacity overflow");
  const std::size_t strict_gamma_union_count = checked_multiply(
      order,
      coface_count,
      "bounded direct--Gamma strict-Gamma union capacity overflow");

  std::size_t critical_candidate_count = 0U;
  for (std::size_t support_size = 1U;
       support_size <= std::min(std::size_t{4U}, point_count);
       ++support_size) {
    critical_candidate_count = checked_add(
        critical_candidate_count,
        bounded_binomial(point_count, support_size),
        "bounded direct--Gamma critical-candidate capacity overflow");
  }
  const std::size_t critical_point_classification_count = checked_multiply(
      critical_candidate_count,
      point_count,
      "bounded direct--Gamma critical-classification capacity overflow");

  std::size_t selected_event_support_count = 0U;
  const std::size_t maximum_selected_support_size =
      std::min({std::size_t{4U}, order + 1U, point_count});
  for (std::size_t support_size = 2U;
       support_size <= maximum_selected_support_size;
       ++support_size) {
    selected_event_support_count = checked_add(
        selected_event_support_count,
        bounded_binomial(point_count, support_size),
        "bounded direct--Gamma selected-event capacity overflow");
  }
  const std::size_t event_projection_count =
      std::min(activation_count, selected_event_support_count);
  const std::size_t history_point_id_scan_count = checked_add(
      checked_multiply(
          order,
          facet_count,
          "bounded direct--Gamma history-facet PointId capacity overflow"),
      checked_multiply(
          order + 1U,
          coface_count,
          "bounded direct--Gamma history-coface PointId capacity overflow"),
      "bounded direct--Gamma history PointId capacity overflow");
  const std::size_t catalog_point_id_scan_count = checked_multiply(
      order + 1U,
      event_projection_count,
      "bounded direct--Gamma catalog PointId capacity overflow");

  ExactDirectMorseGammaCarrierConformanceBudget budget;
  auto& overlay = budget.overlay_budget;
  overlay.critical_catalog_budget = {
      critical_candidate_count, critical_point_classification_count};
  auto& history = overlay.reduced_gamma_history_budget;
  history.gamma_budget = {
      facet_count, coface_count, strict_gamma_union_count};
  history.maximum_activation_level_count = activation_count;
  history.maximum_total_facet_work_count = checked_multiply(
      exhaustive_run_count,
      facet_count,
      "bounded direct--Gamma total facet-work capacity overflow");
  history.maximum_total_coface_work_count = checked_multiply(
      exhaustive_run_count,
      coface_count,
      "bounded direct--Gamma total coface-work capacity overflow");
  history.maximum_total_union_work_count = checked_multiply(
      exhaustive_run_count,
      strict_gamma_union_count,
      "bounded direct--Gamma total union-work capacity overflow");
  history.maximum_node_count = coface_count;
  history.maximum_child_reference_count = coface_count - 1U;
  history.maximum_group_root_reference_count = coface_count - 1U;
  history.maximum_group_count = activation_count;
  history.maximum_group_newly_active_facet_count = facet_count;
  history.maximum_group_equal_level_coface_count = coface_count;
  history.maximum_delta_facet_count = facet_count;
  history.maximum_delta_point_reference_count = checked_multiply(
      order,
      facet_count,
      "bounded direct--Gamma delta PointId capacity overflow");
  overlay.maximum_event_projection_count = event_projection_count;
  overlay.maximum_group_overlay_count = activation_count;
  overlay.maximum_label_slot_count = activation_count;
  overlay.maximum_history_point_id_scan_count =
      history_point_id_scan_count;
  overlay.maximum_catalog_point_id_scan_count =
      catalog_point_id_scan_count;
  overlay.maximum_group_event_reference_count = event_projection_count;

  const std::size_t subset_count = checked_power_of_two(
      point_count,
      "bounded direct--Gamma power-set capacity overflow");
  const std::size_t half_subset_count = subset_count / 2U;
  const std::size_t forest_reference_count = checked_multiply(
      point_count,
      half_subset_count,
      "bounded direct--Gamma multiorder forest-reference capacity overflow");
  const std::size_t group_scratch_count = checked_multiply(
      6U,
      facet_count,
      "bounded direct--Gamma carrier scratch capacity overflow");
  const std::size_t parent_hop_count = checked_multiply(
      2U,
      checked_multiply(
          forest_reference_count,
          facet_count,
          "bounded direct--Gamma parent-hop capacity overflow"),
      "bounded direct--Gamma parent-hop capacity overflow");
  const int binary64_exponent_span_signed =
      std::numeric_limits<double>::max_exponent -
      std::numeric_limits<double>::min_exponent;
  const std::size_t binary64_exact_coordinate_span = checked_add(
      static_cast<std::size_t>(binary64_exponent_span_signed),
      static_cast<std::size_t>(std::numeric_limits<double>::digits),
      "bounded direct--Gamma exact-coordinate bit capacity overflow");
  const std::size_t exact_level_integer_bit_count = checked_multiply(
      8U,
      binary64_exact_coordinate_span,
      "bounded direct--Gamma exact-level bit capacity overflow");

  auto& carrier = budget.carrier_cut_budget;
  carrier.maximum_forest_birth_record_scan_count = subset_count;
  carrier.maximum_forest_node_scan_count = subset_count;
  carrier.maximum_forest_batch_scan_count = subset_count;
  carrier.maximum_forest_atomic_group_scan_count = subset_count;
  carrier.maximum_forest_saddle_scan_count = subset_count;
  carrier.maximum_forest_arm_binding_scan_count = forest_reference_count;
  carrier.maximum_forest_child_reference_scan_count = forest_reference_count;
  carrier.maximum_forest_final_root_scan_count = subset_count;
  carrier.maximum_component_state_count = subset_count;
  carrier.maximum_node_marker_state_count = subset_count;
  carrier.maximum_index_entry_count = facet_count;
  carrier.maximum_group_carrier_scratch_count = group_scratch_count;
  carrier.maximum_group_prior_root_scratch_count = group_scratch_count;
  carrier.maximum_parent_hop_count = parent_hop_count;
  carrier.maximum_exact_level_comparison_count = forest_reference_count;
  carrier.maximum_single_exact_level_integer_bit_count =
      exact_level_integer_bit_count;
  carrier.maximum_logical_output_entry_count = facet_count;

  const std::size_t carrier_anchor_count = checked_multiply(
      checked_multiply(
          2U,
          activation_count,
          "bounded direct--Gamma carrier-anchor capacity overflow"),
      facet_count,
      "bounded direct--Gamma carrier-anchor capacity overflow");
  const std::size_t gamma_lookup_count = checked_multiply(
      carrier_anchor_count,
      facet_count,
      "bounded direct--Gamma component/facet lookup capacity overflow");
  const std::size_t group_event_reference_count = checked_add(
      facet_count,
      checked_multiply(
          2U,
          coface_count,
          "bounded direct--Gamma group-event capacity overflow"),
      "bounded direct--Gamma group-event capacity overflow");
  const std::size_t coface_membership_count = checked_multiply(
      order + 1U,
      coface_count,
      "bounded direct--Gamma coface-membership capacity overflow");
  const std::size_t group_component_membership_count = checked_multiply(
      checked_add(
          coface_membership_count,
          activation_count,
          "bounded direct--Gamma component-membership capacity overflow"),
      facet_count,
      "bounded direct--Gamma component-membership capacity overflow");
  const std::size_t gamma_label_comparison_count = checked_multiply(
      coface_membership_count,
      checked_add(
          coface_count,
          order + 1U,
          "bounded direct--Gamma label-comparison capacity overflow"),
      "bounded direct--Gamma label-comparison capacity overflow");
  budget.maximum_direct_role_scan_count = activation_count;
  budget.maximum_forest_birth_scan_count = subset_count;
  budget.maximum_forest_saddle_scan_count = subset_count;
  budget.maximum_forest_atomic_group_scan_count = subset_count;
  budget.maximum_activation_level_count = activation_count;
  budget.maximum_gamma_transition_build_count = activation_count;
  budget.maximum_checkpoint_count = activation_count;
  budget.maximum_group_audit_count = activation_count;
  budget.maximum_carrier_anchor_scan_count = carrier_anchor_count;
  budget.maximum_gamma_component_scan_count = gamma_lookup_count;
  budget.maximum_gamma_facet_scan_count = gamma_lookup_count;
  budget.maximum_group_event_reference_scan_count =
      group_event_reference_count;
  budget.maximum_arm_binding_scan_count = coface_membership_count;
  budget.maximum_gamma_incidence_join_count = coface_membership_count;
  budget.maximum_carrier_cut_entry_lookup_count =
      coface_membership_count;
  budget.maximum_group_component_membership_count =
      group_component_membership_count;
  budget.maximum_gamma_label_comparison_count =
      gamma_label_comparison_count;
  budget.maximum_logical_scratch_entry_count = group_scratch_count;
  budget.maximum_logical_output_entry_count = checked_add(
      checked_multiply(
          2U,
          activation_count,
          "bounded direct--Gamma logical-output capacity overflow"),
      1U,
      "bounded direct--Gamma logical-output capacity overflow");
  return budget;
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget
make_bounded_gamma2_history_budget(std::size_t point_count) {
  constexpr std::size_t order = 2U;
  const std::size_t facet_count = bounded_binomial(point_count, order);
  const std::size_t coface_count =
      bounded_binomial(point_count, order + 1U);
  const std::size_t union_count = checked_multiply(
      order, coface_count, "bounded Gamma2 union capacity overflow");
  const std::size_t activation_level_count = checked_add(
      facet_count,
      coface_count,
      "bounded Gamma2 activation-level capacity overflow");
  const std::size_t replay_count = checked_add(
      activation_level_count,
      1U,
      "bounded Gamma2 replay capacity overflow");
  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {facet_count, coface_count, union_count};
  budget.maximum_activation_level_count = activation_level_count;
  budget.maximum_total_facet_work_count = checked_multiply(
      replay_count,
      facet_count,
      "bounded Gamma2 facet-work capacity overflow");
  budget.maximum_total_coface_work_count = checked_multiply(
      replay_count,
      coface_count,
      "bounded Gamma2 coface-work capacity overflow");
  budget.maximum_total_union_work_count = checked_multiply(
      replay_count,
      union_count,
      "bounded Gamma2 union-work capacity overflow");
  budget.maximum_node_count = coface_count;
  budget.maximum_child_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_root_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  budget.maximum_group_count = activation_level_count;
  budget.maximum_group_newly_active_facet_count = facet_count;
  budget.maximum_group_equal_level_coface_count = coface_count;
  budget.maximum_delta_facet_count = facet_count;
  budget.maximum_delta_point_reference_count = checked_multiply(
      order,
      facet_count,
      "bounded Gamma2 delta-point capacity overflow");
  return budget;
}

[[nodiscard]] ExactK2K1HierarchyBudget
make_bounded_k2_k1_hierarchy_budget() noexcept {
  ExactK2K1HierarchyBudget budget;
  budget.maximum_source_batch_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_batch_count;
  budget.maximum_source_group_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_group_count;
  budget.maximum_source_node_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_node_count;
  budget.maximum_source_child_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_child_reference_count;
  budget.maximum_source_root_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_root_reference_count;
  budget.maximum_checkpoint_count =
      ExactK2K1HierarchyBudget::maximum_supported_checkpoint_count;
  budget.maximum_source_facet_replay_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_source_facet_replay_count;
  budget.maximum_vertical_endpoint_lookup_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_vertical_endpoint_lookup_count;
  budget.maximum_k1_parent_hop_count =
      ExactK2K1HierarchyBudget::maximum_supported_k1_parent_hop_count;
  budget.maximum_target_coverage_point_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_target_coverage_point_reference_count;
  budget.maximum_digest_logical_entry_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_digest_logical_entry_count;
  budget.maximum_digest_exact_text_byte_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_digest_exact_text_byte_count;
  return budget;
}

[[nodiscard]] ExactReducedGammaCutBudget
make_bounded_closed_gamma2_cut_budget() noexcept {
  return {
      ExactReducedGammaCutBudget::maximum_supported_batch_count,
      ExactReducedGammaCutBudget::maximum_supported_group_record_count,
      ExactReducedGammaCutBudget::maximum_supported_node_record_count,
      ExactReducedGammaCutBudget::
          maximum_supported_prior_root_reference_count,
      ExactReducedGammaCutBudget::maximum_supported_child_reference_count,
      ExactReducedGammaCutBudget::
          maximum_supported_newly_active_facet_count,
      ExactReducedGammaCutBudget::
          maximum_supported_equal_level_coface_count,
      ExactReducedGammaCutBudget::maximum_supported_delta_facet_count,
      ExactReducedGammaCutBudget::
          maximum_supported_delta_point_reference_count,
      ExactReducedGammaCutBudget::maximum_supported_active_root_count,
      ExactReducedGammaCutBudget::
          maximum_supported_output_facet_reference_count,
      ExactReducedGammaCutBudget::
          maximum_supported_output_point_reference_count,
      ExactReducedGammaCutBudget::
          maximum_supported_facet_replay_work_count,
      ExactReducedGammaCutBudget::
          maximum_supported_point_id_replay_work_count,
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_facet_check_count,
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_point_id_work_count,
  };
}

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityBudget
make_bounded_k2_k1_target_authority_budget() noexcept {
  // This cap is deliberately fixed and independent of the observed receipt.
  // The n<=14 gate and the nested oracle hard caps bound all actual work far
  // below it; exceeding it remains an atomic fail-closed outcome.
  constexpr std::size_t work_cap = 1'000'000'000U;
  return {
      make_bounded_closed_gamma2_cut_budget(),
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
      work_cap,
  };
}

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingBudget
make_bounded_m1_o5_death_accounting_budget() noexcept {
  // Fixed independently of the observed receipt.  Together with the n<=14
  // mode gate, this bounds both persistent audits and transient quotient work.
  constexpr std::size_t capacity = 65536U;
  return {
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
  };
}

[[nodiscard]] std::string_view m1_o5_death_accounting_stop_detail(
    ExactDirectMorseM1O5DeathAccountingDecision decision) noexcept {
  switch (decision) {
    case ExactDirectMorseM1O5DeathAccountingDecision::not_certified:
      return "m1_o5_death_accounting_not_certified";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_capacity_overflow:
      return "m1_o5_death_accounting_capacity_overflow";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_allocation_failed:
      return "m1_o5_death_accounting_allocation_failed";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_budget_exhausted:
      return "m1_o5_death_accounting_budget_exhausted";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_point_count_or_order_rejected:
      return "m1_o5_death_accounting_point_count_or_order_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_source_event_journal_rejected:
      return "m1_o5_death_accounting_source_event_journal_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_source_seed_journal_rejected:
      return "m1_o5_death_accounting_source_seed_journal_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_source_forest_rejected:
      return "m1_o5_death_accounting_source_forest_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_source_join_inconsistent:
      return "m1_o5_death_accounting_source_join_inconsistent";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_arm_bijection_rejected:
      return "m1_o5_death_accounting_arm_bijection_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_point_id_or_level_join_rejected:
      return "m1_o5_death_accounting_point_id_or_level_join_rejected";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_carrier_root_conflict:
      return "m1_o5_death_accounting_carrier_root_conflict";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_quotient_group_mismatch:
      return "m1_o5_death_accounting_quotient_group_mismatch";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_death_identity_mismatch:
      return "m1_o5_death_accounting_death_identity_mismatch";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        no_accounting_local_multiplicity_bound_failed:
      return "m1_o5_death_accounting_local_multiplicity_bound_failed";
    case ExactDirectMorseM1O5DeathAccountingDecision::
        complete_bounded_conditional_m1_o5_death_accounting:
      return "none";
  }
  return "m1_o5_death_accounting_unknown_decision";
}

[[nodiscard]] std::string_view
direct_morse_gamma_carrier_conformance_stop_detail(
    ExactDirectMorseGammaCarrierConformanceDecision decision) noexcept {
  switch (decision) {
    case ExactDirectMorseGammaCarrierConformanceDecision::not_certified:
      return "direct_morse_gamma_carrier_conformance_not_certified";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_capacity_overflow:
      return "direct_morse_gamma_carrier_conformance_capacity_overflow";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_budget_exhausted:
      return "direct_morse_gamma_carrier_conformance_budget_exhausted";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_allocation_failed:
      return "direct_morse_gamma_carrier_conformance_allocation_failed";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_point_count_or_order_rejected:
      return "direct_morse_gamma_carrier_conformance_point_count_or_order_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_direct_forest_replay_rejected:
      return "direct_morse_gamma_carrier_conformance_direct_forest_replay_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_overlay_rejected:
      return "direct_morse_gamma_carrier_conformance_overlay_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_direct_role_overlay_mismatch:
      return "direct_morse_gamma_carrier_conformance_direct_role_overlay_mismatch";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_direct_group_overlay_mismatch:
      return "direct_morse_gamma_carrier_conformance_direct_group_overlay_mismatch";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_gamma_residual_group_rejected:
      return "direct_morse_gamma_carrier_conformance_gamma_residual_group_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_gamma_transition_rejected:
      return "direct_morse_gamma_carrier_conformance_gamma_transition_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_carrier_cut_replay_rejected:
      return "direct_morse_gamma_carrier_conformance_carrier_cut_replay_rejected";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        no_conformance_carrier_partition_mismatch:
      return "direct_morse_gamma_carrier_conformance_carrier_partition_mismatch";
    case ExactDirectMorseGammaCarrierConformanceDecision::
        complete_bounded_single_order_direct_gamma_carrier_group_conformance:
      return "none";
  }
  return "direct_morse_gamma_carrier_conformance_unknown_decision";
}

[[nodiscard]] std::string_view k2_to_k1_target_authority_stop_detail(
    ExactDirectMorseK2K1TargetAuthorityDecision decision) noexcept {
  switch (decision) {
    case ExactDirectMorseK2K1TargetAuthorityDecision::not_certified:
      return "k2_to_k1_target_authority_not_certified";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_capacity_overflow:
      return "k2_to_k1_target_authority_capacity_overflow";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_budget_exhausted:
      return "k2_to_k1_target_authority_budget_exhausted";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_allocation_failed:
      return "k2_to_k1_target_authority_allocation_failed";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_point_count_or_order_rejected:
      return "k2_to_k1_target_authority_point_count_or_order_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_direct_pipeline_rejected:
      return "k2_to_k1_target_authority_direct_pipeline_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_direct_journal_rejected:
      return "k2_to_k1_target_authority_direct_journal_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_external_hierarchy_rejected:
      return "k2_to_k1_target_authority_external_hierarchy_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_label_partition_rejected:
      return "k2_to_k1_target_authority_label_partition_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_observed_k2_k1_label_missing:
      return "k2_to_k1_target_authority_observed_label_missing";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_observed_k2_k1_label_unresolved:
      return "k2_to_k1_target_authority_observed_label_unresolved";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_source_binding_rejected:
      return "k2_to_k1_target_authority_source_binding_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_gamma_cut_rejected:
      return "k2_to_k1_target_authority_gamma_cut_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_gamma_component_rejected:
      return "k2_to_k1_target_authority_gamma_component_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_external_checkpoint_rejected:
      return "k2_to_k1_target_authority_external_checkpoint_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_direct_target_rejected:
      return "k2_to_k1_target_authority_direct_target_rejected";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        no_authority_target_coverage_mismatch:
      return "k2_to_k1_target_authority_target_coverage_mismatch";
    case ExactDirectMorseK2K1TargetAuthorityDecision::
        complete_observed_resolved_k2_k1_label_target_authority_replay:
      return "none";
  }
  return "k2_to_k1_target_authority_unknown_decision";
}

[[nodiscard]] std::string_view vertical_target_pipeline_stop_detail(
    ExactDirectMorseVerticalTargetProposalPipelineDecision decision)
    noexcept {
  switch (decision) {
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        not_certified:
      return "vertical_target_pipeline_not_certified";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_source_forest_rejected:
      return "vertical_target_pipeline_source_forest_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_point_namespace_rejected:
      return "vertical_target_pipeline_point_namespace_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_source_shape_rejected:
      return "vertical_target_pipeline_source_shape_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_capacity_overflow:
      return "vertical_target_pipeline_capacity_overflow";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_budget_exhausted:
      return "vertical_target_pipeline_budget_exhausted";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_allocation_failed:
      return "vertical_target_pipeline_allocation_failed";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_token_overflow:
      return "vertical_target_pipeline_token_overflow";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_facet_plan_rejected:
      return "vertical_target_pipeline_facet_plan_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_session_initialization_rejected:
      return "vertical_target_pipeline_session_initialization_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_replay_advance_rejected:
      return "vertical_target_pipeline_replay_advance_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_closure_rejected:
      return "vertical_target_pipeline_closure_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        no_pipeline_proposal_adapter_rejected:
      return "vertical_target_pipeline_proposal_adapter_rejected";
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        complete_empty_adjacent_group_set:
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        complete_all_resolved_multiorder_target_proposals:
    case ExactDirectMorseVerticalTargetProposalPipelineDecision::
        complete_with_unresolved_multiorder_target_proposals:
      return "none";
  }
  return "vertical_target_pipeline_unknown_decision";
}

[[nodiscard]] std::string_view vertical_journal_stop_detail(
    ExactDirectMorseVerticalDecision decision) noexcept {
  switch (decision) {
    case ExactDirectMorseVerticalDecision::not_certified:
      return "vertical_journal_not_certified";
    case ExactDirectMorseVerticalDecision::no_vertical_capacity_overflow:
      return "vertical_journal_capacity_overflow";
    case ExactDirectMorseVerticalDecision::no_vertical_budget_exhausted:
      return "vertical_journal_budget_exhausted";
    case ExactDirectMorseVerticalDecision::no_vertical_allocation_failed:
      return "vertical_journal_allocation_failed";
    case ExactDirectMorseVerticalDecision::no_vertical_source_forest_rejected:
      return "vertical_journal_source_forest_rejected";
    case ExactDirectMorseVerticalDecision::no_vertical_forest_shape_rejected:
      return "vertical_journal_forest_shape_rejected";
    case ExactDirectMorseVerticalDecision::
        no_vertical_proposal_partition_rejected:
      return "vertical_journal_proposal_partition_rejected";
    case ExactDirectMorseVerticalDecision::no_vertical_target_rejected:
      return "vertical_journal_target_rejected";
    case ExactDirectMorseVerticalDecision::
        no_vertical_relative_target_conflict:
      return "vertical_journal_relative_target_conflict";
    case ExactDirectMorseVerticalDecision::
        complete_conditional_partial_vertical_journal:
    case ExactDirectMorseVerticalDecision::
        complete_conditional_total_relative_vertical_journal:
      return "none";
  }
  return "vertical_journal_unknown_decision";
}

void diagnose_rejected_vertical_target_group(
    const ExactDirectMorseForestJournalResult& forest,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactDirectMorseVerticalTargetProposalPipelineBudget& budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult& pipeline,
    Report& report) {
  if (!pipeline.rejected_source_atomic_group_index.has_value()) {
    return;
  }
  const Clock::time_point begin = Clock::now();
  report.vertical_target_replay_diagnostic_attempted = true;
  const std::size_t group_index =
      *pipeline.rejected_source_atomic_group_index;
  report.vertical_target_diagnostic_source_group_index = group_index;
  if (group_index >= forest.atomic_groups.size()) {
    report.timings.vertical_target_replay_diagnostic_ms =
        milliseconds(Clock::now() - begin);
    return;
  }
  const auto& group = forest.atomic_groups[group_index];
  report.vertical_target_diagnostic_source_batch_index = group.batch_index;
  if (group.batch_index >= forest.batches.size()) {
    report.timings.vertical_target_replay_diagnostic_ms =
        milliseconds(Clock::now() - begin);
    return;
  }
  const auto& batch = forest.batches[group.batch_index];
  if (batch.order < 2U ||
      group_index > std::numeric_limits<std::uint64_t>::max() / 3U - 1U) {
    report.timings.vertical_target_replay_diagnostic_ms =
        milliseconds(Clock::now() - begin);
    return;
  }
  report.vertical_target_diagnostic_source_order = batch.order;
  report.vertical_target_diagnostic_target_order = batch.order - 1U;
  report.vertical_target_diagnostic_level_numerator =
      batch.squared_level.numerator_string();
  report.vertical_target_diagnostic_level_denominator =
      batch.squared_level.denominator_string();
  report.vertical_target_diagnostic_query_token =
      3U * (static_cast<std::uint64_t>(group_index) + 1U);

  const auto plan = build_exact_direct_morse_vertical_target_facet_plan(
      forest, group_index, budget.facet_plan_budget);
  report.vertical_target_diagnostic_plan_decision =
      static_cast<unsigned>(plan.decision);
  for (const auto& representative : plan.representatives) {
    report.vertical_target_diagnostic_representative_binding_indices
        .push_back(
            representative.representative_arm_root_binding_index);
    report.vertical_target_diagnostic_source_keys.push_back(
        representative.source_strict_arm_key);
  }
  report.vertical_target_diagnostic_target_facet_indices =
      plan.target_facet_indices;
  report.vertical_target_diagnostic_target_keys =
      plan.canonical_distinct_target_facet_keys;
  if (!plan.certified_group_local_target_facet_plan()) {
    report.timings.vertical_target_replay_diagnostic_ms =
        milliseconds(Clock::now() - begin);
    return;
  }

  auto initialized =
      build_exact_direct_morse_forest_carrier_cut_replay_session(
          forest,
          batch.order - 1U,
          budget.session_budget);
  report.vertical_target_diagnostic_session_initialization_decision =
      static_cast<unsigned>(initialized.decision);
  if (!initialized.certified_ready_session() || !initialized.session) {
    report.timings.vertical_target_replay_diagnostic_ms =
        milliseconds(Clock::now() - begin);
    return;
  }

  const auto advanced = initialized.session->advance_to_closed_cut(
      batch.squared_level,
      [&](const ExactDirectMorseForestCarrierCutReplayView& view) {
        report.vertical_target_replay_diagnostic_callback_invoked = true;
        const ExactDirectSparseFacetWitness query_witness{
            pipeline.external_target_authority_id,
            report.vertical_target_diagnostic_query_token};
        const auto closure =
            view.build_closure_summary_from_canonical_distinct_keys(
                index,
                cloud,
                plan.canonical_distinct_target_facet_keys,
                query_witness,
                budget.closure_budget,
                forest.config.closure_config,
                forest.traversal_order);
        report.vertical_target_diagnostic_closure_adapter_decision =
            static_cast<unsigned>(closure.decision);
        report.vertical_target_diagnostic_inner_closure_decision =
            static_cast<unsigned>(closure.closure_decision);
        report.vertical_target_diagnostic_inner_closure_disposition =
            static_cast<unsigned>(closure.closure_disposition);
        report.vertical_target_diagnostic_closure_counters =
            closure.closure_counters;
        report.vertical_target_diagnostic_contradiction_witness =
            closure.contradiction_witness;
      });
  report.vertical_target_diagnostic_advance_decision =
      static_cast<unsigned>(advanced.decision);
  report.vertical_target_replay_diagnostic_advance_certified =
      advanced.certified_forest_relative_closed_cut();
  report.timings.vertical_target_replay_diagnostic_ms =
      milliseconds(Clock::now() - begin);
}

[[nodiscard]] std::string json_escape(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char character : text) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(character);
        break;
    }
  }
  return escaped;
}

void emit_facet_key_json(const ExactDirectSparseFacetKey& key) {
  std::cout << '[';
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << key.point_ids[index];
  }
  std::cout << ']';
}

void emit_facet_key_vector_json(
    std::span<const ExactDirectSparseFacetKey> keys) {
  std::cout << '[';
  for (std::size_t index = 0U; index < keys.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    emit_facet_key_json(keys[index]);
  }
  std::cout << ']';
}

void emit_size_vector_json(std::span<const std::size_t> values) {
  std::cout << '[';
  for (std::size_t index = 0U; index < values.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << values[index];
  }
  std::cout << ']';
}

void emit_m1_o5_event_audits_json(
    std::span<const ExactDirectMorseM1O5EventAudit> audits) {
  std::cout << '[';
  for (std::size_t index = 0U; index < audits.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& audit = audits[index];
    std::cout
        << "{\"event_audit_index\":" << audit.event_audit_index
        << ",\"order\":" << audit.order
        << ",\"squared_level\":{\"numerator\":\""
        << json_escape(audit.squared_level.numerator_string())
        << "\",\"denominator\":\""
        << json_escape(audit.squared_level.denominator_string())
        << "\"},\"source_event_arm_identity_digest\":\""
        << audit.source_event_arm_identity_digest.to_lower_hex()
        << "\",\"canonical_support_point_ids\":[";
    for (std::size_t point_index = 0U;
         point_index < audit.support_point_count;
         ++point_index) {
      if (point_index != 0U) {
        std::cout << ',';
      }
      std::cout << audit.canonical_support_point_ids[point_index];
    }
    std::cout
        << "],\"support_point_count\":" << audit.support_point_count
        << ",\"arm_count\":" << audit.arm_count
        << ",\"delta_one\":" << audit.delta_one << '}';
  }
  std::cout << ']';
}

void emit_m1_o5_group_audits_json(
    std::span<const ExactDirectMorseM1O5GroupAudit> audits) {
  std::cout << '[';
  for (std::size_t index = 0U; index < audits.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& audit = audits[index];
    std::cout
        << "{\"group_audit_index\":" << audit.group_audit_index
        << ",\"batch_audit_index\":" << audit.batch_audit_index
        << ",\"order\":" << audit.order
        << ",\"squared_level\":{\"numerator\":\""
        << json_escape(audit.squared_level.numerator_string())
        << "\",\"denominator\":\""
        << json_escape(audit.squared_level.denominator_string())
        << "\"},\"canonical_group_digest\":\""
        << audit.canonical_group_digest.to_lower_hex()
        << "\",\"saddle_count\":" << audit.saddle_count
        << ",\"arm_count\":" << audit.arm_count
        << ",\"distinct_carrier_count\":"
        << audit.distinct_carrier_count
        << ",\"prior_reduced_root_count\":"
        << audit.prior_reduced_root_count
        << ",\"death_count\":" << audit.death_count
        << ",\"kind\":" << static_cast<unsigned>(audit.kind) << '}';
  }
  std::cout << ']';
}

void emit_m1_o5_batch_audits_json(
    std::span<const ExactDirectMorseM1O5BatchAudit> audits) {
  std::cout << '[';
  for (std::size_t index = 0U; index < audits.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& audit = audits[index];
    std::cout
        << "{\"batch_audit_index\":" << audit.batch_audit_index
        << ",\"order\":" << audit.order
        << ",\"squared_level\":{\"numerator\":\""
        << json_escape(audit.squared_level.numerator_string())
        << "\",\"denominator\":\""
        << json_escape(audit.squared_level.denominator_string())
        << "\"},\"event_count\":" << audit.event_count
        << ",\"group_count\":" << audit.group_count
        << ",\"local_multiplicity_capacity\":"
        << audit.local_multiplicity_capacity
        << ",\"touched_distinct_prior_root_count\":"
        << audit.touched_distinct_prior_root_count
        << ",\"positive_root_group_count\":"
        << audit.positive_root_group_count
        << ",\"global_death_count\":" << audit.global_death_count
        << ",\"sum_group_death_count\":"
        << audit.sum_group_death_count << '}';
  }
  std::cout << ']';
}

void emit_direct_morse_gamma_carrier_checkpoints_json(
    std::span<const ExactDirectMorseGammaCarrierCheckpointAudit>
        checkpoints) {
  std::cout << '[';
  for (std::size_t index = 0U; index < checkpoints.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& checkpoint = checkpoints[index];
    std::cout
        << "{\"activation_level_index\":"
        << checkpoint.activation_level_index
        << ",\"squared_level\":{\"numerator\":\""
        << json_escape(checkpoint.squared_level.numerator_string())
        << "\",\"denominator\":\""
        << json_escape(checkpoint.squared_level.denominator_string())
        << "\"},\"direct_batch_present\":"
        << (checkpoint.direct_batch_present ? "true" : "false")
        << ",\"strict_direct_carrier_count\":"
        << checkpoint.strict_direct_carrier_count
        << ",\"strict_gamma_component_count\":"
        << checkpoint.strict_gamma_component_count
        << ",\"strict_birth_anchor_count\":"
        << checkpoint.strict_birth_anchor_count
        << ",\"closed_direct_carrier_count\":"
        << checkpoint.closed_direct_carrier_count
        << ",\"closed_gamma_component_count\":"
        << checkpoint.closed_gamma_component_count
        << ",\"closed_birth_anchor_count\":"
        << checkpoint.closed_birth_anchor_count
        << ",\"gamma_group_count\":"
        << checkpoint.gamma_group_count
        << ",\"silent_gamma_group_count\":"
        << checkpoint.silent_gamma_group_count
        << ",\"strict_partition_bijective\":"
        << (checkpoint.strict_partition_bijective ? "true" : "false")
        << ",\"closed_partition_bijective\":"
        << (checkpoint.closed_partition_bijective ? "true" : "false")
        << '}';
  }
  std::cout << ']';
}

void emit_direct_morse_gamma_group_audits_json(
    std::span<const ExactDirectMorseGammaGroupAudit> audits) {
  std::cout << '[';
  for (std::size_t index = 0U; index < audits.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& audit = audits[index];
    std::cout
        << "{\"group_audit_index\":" << index
        << ",\"squared_level\":{\"numerator\":\""
        << json_escape(audit.squared_level.numerator_string())
        << "\",\"denominator\":\""
        << json_escape(audit.squared_level.denominator_string())
        << "\"},\"gamma_kind\":"
        << static_cast<unsigned>(audit.gamma_kind)
        << ",\"classification\":"
        << static_cast<unsigned>(audit.classification)
        << ",\"direct_birth_role_count\":"
        << audit.direct_birth_role_count
        << ",\"direct_saddle_role_count\":"
        << audit.direct_saddle_role_count
        << ",\"residual_newly_active_facet_count\":"
        << audit.residual_newly_active_facet_count
        << ",\"residual_equal_level_coface_count\":"
        << audit.residual_equal_level_coface_count
        << ",\"gamma_prior_root_count\":"
        << audit.gamma_prior_root_count
        << ",\"direct_prior_root_count\":";
    if (audit.direct_prior_root_count.has_value()) {
      std::cout << *audit.direct_prior_root_count;
    } else {
      std::cout << "null";
    }
    std::cout
        << ",\"coverage_delta_present\":"
        << (audit.coverage_delta_present ? "true" : "false")
        << ",\"delta_facet_count\":" << audit.delta_facet_count
        << ",\"delta_point_count\":" << audit.delta_point_count
        << ",\"direct_arm_binding_count\":"
        << audit.direct_arm_binding_count
        << ",\"strict_gamma_component_count\":"
        << audit.strict_gamma_component_count
        << ",\"latent_strict_gamma_component_count\":"
        << audit.latent_strict_gamma_component_count
        << ",\"resolved_strict_gamma_component_count\":"
        << audit.resolved_strict_gamma_component_count
        << ",\"fully_redundant_delta\":"
        << (audit.fully_redundant_delta ? "true" : "false")
        << ",\"direct_atomic_group_present\":"
        << (audit.direct_atomic_group_present ? "true" : "false")
        << ",\"direct_arm_strict_component_bijection\":"
        << (audit.direct_arm_strict_component_bijection
                ? "true"
                : "false")
        << ",\"silent_one_root_zero_point_continuation\":"
        << (audit.silent_one_root_zero_point_continuation
                ? "true"
                : "false")
        << '}';
  }
  std::cout << ']';
}

void emit_report(const Report& report) {
  const auto boolean = [](bool value) {
    return value ? "true" : "false";
  };
  const bool right_censored =
      report.complete_hierarchy_attempt_requested &&
      !report.pipeline_complete &&
      (report.operational_deadline_reached || report.budget_exhausted);
  const std::string_view diagnostic_100ms_outcome =
      report.pipeline_complete
          ? (report.timings.total_ms < 100.0
                 ? "complete_below_threshold"
                 : "miss")
          : (report.timings.total_ms >= 100.0
                 ? "miss"
                 : "unassessed");
  const ExactDirectMorseEventRankTowerLinkJournalResult empty_event_links;
  const auto& event_links = report.event_rank_tower_link_journal.has_value()
      ? *report.event_rank_tower_link_journal
      : empty_event_links;
  const ExactDirectMorseEventVerticalPropagationJournalResult
      empty_event_propagation;
  const auto& event_propagation =
      report.event_vertical_propagation_journal.has_value()
      ? *report.event_vertical_propagation_journal
      : empty_event_propagation;
  const auto& event_strong_verification =
      report.event_vertical_propagation_fresh_verification;
  std::cout
      << "{\n"
      << "  \"schema\":\"morsehgp3d.direct-morse-product-run.v7\",\n"
      << "  \"phase\":\""
      << (report
                  .conditional_adjacent_event_vertical_propagation_qualification_requested
              ? "15_conditional_adjacent_event_vertical_propagation"
              : "15_bounded_direct_gamma_carrier_group_conformance")
      << "\",\n"
      << "  \"backend\":\"reference_cpu\",\n"
      << "  \"profile\":\"hgp_reduced\",\n"
      << "  \"mode\":\"" << json_escape(report.options.mode)
      << "\",\n"
      << "  \"public_status\":\"not_claimed\",\n"
      << "  \"family\":\"" << json_escape(report.options.family)
      << "\",\n"
      << "  \"point_count\":" << report.options.point_count << ",\n"
      << "  \"canonical_point_count\":"
      << report.canonical_point_count << ",\n"
      << "  \"requested_maximum_order\":"
      << report.options.maximum_order << ",\n"
      << "  \"effective_maximum_order\":"
      << report.effective_maximum_order << ",\n"
      << "  \"terminal_stage\":\""
      << json_escape(report.terminal_stage) << "\",\n"
      << "  \"stop_category\":\""
      << json_escape(report.stop_category) << "\",\n"
      << "  \"stop_detail\":\""
      << json_escape(report.stop_detail) << "\",\n"
      << "  \"pipeline_complete\":"
      << boolean(report.pipeline_complete) << ",\n"
      << "  \"resident_conditional_pipeline_complete\":"
      << boolean(report.resident_conditional_pipeline_complete) << ",\n"
      << "  \"budget_exhausted\":"
      << boolean(report.budget_exhausted) << ",\n"
      << "  \"scientific_result_materialized\":"
      << boolean(report.scientific_result_materialized) << ",\n"
      << "  \"conditional_h0_candidate_certified\":"
      << boolean(report.conditional_h0_candidate_certified) << ",\n"
      << "  \"global_morse_obligation_replayed\":false,\n"
      << "  \"k2_to_k1_observed_label_target_authority_replayed\":"
      << boolean(
             report.k2_to_k1_observed_label_target_authority_replayed)
      << ",\n"
      << "  \"bidirectional_gamma_group_completeness_replayed\":false,\n"
      << "  \"silent_gamma_checkpoint_completeness_replayed\":false,\n"
      << "  \"external_target_authority_replayed\":false,\n"
      << "  \"all_naturality_squares_replayed\":false,\n"
      << "  \"vertical_maps_complete\":false,\n"
      << "  \"global_m1_claimed\":false,\n"
      << "  \"product_sparse_silent_source_complete\":false,\n"
      << "  \"forest_semantics_exact\":false,\n"
      << "  \"product_architecture_claimed\":false,\n"
      << "  \"scalable_50k_claimed\":false,\n"
      << "  \"warm_e2e_protocol_executed\":false,\n"
      << "  \"warm_e2e_slo_claimed\":false,\n"
      << "  \"warm_e2e_slo_outcome\":\"not_executed\",\n"
      << "  \"p95_ms\":null,\n"
      << "  \"qualification_claimed\":false,\n"
      << "  \"complete_hierarchy_attempt_requested\":"
      << boolean(report.complete_hierarchy_attempt_requested) << ",\n"
      << "  \"bounded_k2_to_k1_target_authority_qualification_requested\":"
      << boolean(
             report
                 .bounded_k2_to_k1_target_authority_qualification_requested)
      << ",\n"
      << "  \"bounded_m1_o5_death_accounting_qualification_requested\":"
      << boolean(
             report
                 .bounded_m1_o5_death_accounting_qualification_requested)
      << ",\n"
      << "  \"bounded_direct_gamma_carrier_conformance_qualification_requested\":"
      << boolean(
             report
                 .bounded_direct_gamma_carrier_conformance_qualification_requested)
      << ",\n"
      << "  \"conditional_adjacent_event_vertical_propagation_qualification_requested\":"
      << boolean(
             report
                 .conditional_adjacent_event_vertical_propagation_qualification_requested)
      << ",\n"
      << "  \"attempt_kind\":\""
      << (report
                  .conditional_adjacent_event_vertical_propagation_qualification_requested
              ? "fail_closed_conditional_adjacent_event_vertical_propagation_qualification"
              : (report
                  .bounded_direct_gamma_carrier_conformance_qualification_requested
              ? "fail_closed_bounded_direct_gamma_carrier_conformance_qualification"
              : (report.bounded_m1_o5_death_accounting_qualification_requested
                     ? "fail_closed_bounded_m1_o5_death_accounting_qualification"
                     : (report.bounded_k2_to_k1_target_authority_qualification_requested
                            ? "fail_closed_bounded_k2_to_k1_target_authority_qualification"
                            : (report.complete_hierarchy_attempt_requested
                                   ? "right_censorable_full_pipeline_diagnostic"
                                   : "fail_fast_capacity_diagnostic")))))
      << "\",\n"
      << "  \"configured_pair_total_caps_disabled\":"
      << boolean(report.configurable_pair_total_caps_disabled) << ",\n"
      << "  \"pair_work_capacity_policy\":\""
      << (report.configurable_pair_total_caps_disabled
              ? "representational_ceiling_not_completion_envelope"
              : "caller_diagnostic_caps")
      << "\",\n"
      << "  \"support_budgets_are_resumable_chunk_quanta\":true,\n"
      << "  \"downstream_static_confidence_caps_enabled\":"
      << boolean(report.downstream_static_confidence_caps_enabled) << ",\n"
      << "  \"operational_deadline_ms\":"
      << report.options.operational_deadline_ms << ",\n"
      << "  \"operational_deadline_reached\":"
      << boolean(report.operational_deadline_reached) << ",\n"
      << "  \"operational_deadline_semantics\":"
         "\"cooperative_stage_pair_advance_and_higher_chunk_checkpoints\",\n"
      << "  \"completion_latency_ms\":";
  if (report.pipeline_complete) {
    std::cout << std::fixed << std::setprecision(3)
              << report.timings.total_ms;
  } else {
    std::cout << "null";
  }
  std::cout
      << ",\n"
      << "  \"diagnostic_100ms_outcome\":\""
      << diagnostic_100ms_outcome << "\",\n"
      << "  \"censoring\":{\"kind\":\""
      << (right_censored ? "right" : "none")
      << "\",\"cause\":\""
      << (report.operational_deadline_reached
              ? "operational_deadline"
              : (report.budget_exhausted ? "capacity" : "none"))
      << "\",\"boundary_ms\":"
      << report.options.operational_deadline_ms
      << ",\"terminal_stage\":\""
      << json_escape(report.terminal_stage)
      << "\",\"lower_bound_ms\":";
  if (right_censored) {
    std::cout << std::fixed << std::setprecision(3)
              << report.timings.total_ms;
  } else {
    std::cout << "null";
  }
  std::cout
      << "},\n"
      << "  \"timing_scope\":\""
      << (report
                  .conditional_adjacent_event_vertical_propagation_qualification_requested
              ? "attempted_single_process_cpu_generation_to_certified_forest_"
                "then_conditional_adjacent_event_links_propagation_and_"
                "fresh_upstream_forest_replay"
              : (report
                  .bounded_direct_gamma_carrier_conformance_qualification_requested
              ? "attempted_single_process_cpu_generation_to_certified_forest_"
                "and_bounded_fresh_single_order_direct_gamma_carrier_group_"
                "conformance"
              : (report.bounded_m1_o5_death_accounting_qualification_requested
                     ? "attempted_single_process_cpu_generation_to_certified_forest_"
                       "and_bounded_fresh_m1_o5_death_accounting"
                     : (report.bounded_k2_to_k1_target_authority_qualification_requested
                            ? "attempted_single_process_cpu_generation_to_conditional_"
                              "vertical_journal_and_bounded_fresh_gamma2_emst_k1_"
                              "observed_label_target_authority"
                            : "attempted_single_process_cpu_generation_to_materialized_"
                              "forest_and_forest_relative_vertical_target_pipeline_and_"
                              "conditional_vertical_journal"))))
      << "\",\n"
      << "  \"architecture_audit_complete\":"
      << boolean(report.architecture_audit_complete) << ",\n"
      << "  \"no_forbidden_global_structure_materialized\":"
      << boolean(
             report.no_forbidden_global_structure_materialized)
      << ",\n"
      << "  \"no_forbidden_product_path_global_structure_materialized\":"
      << boolean(report.no_forbidden_global_structure_materialized)
      << ",\n"
      << "  \"architecture_audit_scope\":"
         "\"nonbounded_product_path_excluding_explicit_bounded_oracle\",\n"
      << "  \"bounded_oracle_gamma_materialized_transiently\":"
      << boolean(
             report.k2_to_k1_bounded_exhaustive_gamma_oracle_used ||
             report.direct_morse_gamma_carrier_conformance
                 .bounded_exhaustive_gamma_oracle_used ||
             report.direct_morse_gamma_carrier_conformance.counters
                     .overlay_build_count != 0U)
      << ",\n"
      << "  \"bounded_oracle_global_structure_persisted\":false,\n"
      << "  \"higher_order_delaunay_materialized\":false,\n"
      << "  \"budgets\":{\"support_work\":"
      << report.options.support_work_budget
      << ",\"support_records\":"
      << report.options.support_record_budget
      << ",\"higher_chunks\":"
      << report.options.higher_chunk_limit
      << ",\"effective_higher_chunks\":"
      << report.effective_higher_chunk_limit
      << ",\"downstream_records\":"
      << report.options.downstream_record_budget
      << ",\"descent_work\":"
      << report.options.descent_work_budget
      << ",\"chunk_bytes\":"
      << report.options.chunk_byte_budget
      // SPECIFICATION_MORSEHGP3D.md 1.1.  `profile` is the requested
      // selector, but `all_axes_at_representational_ceiling` is recomputed
      // from the budgets actually INSTALLED above, so a run can never
      // declare a weaker bound than the one it ran under -- nor a stronger
      // one.  `sealed_structural_bounds` names what still bounds the run
      // although it is not a budget: an unenumerated structural ceiling
      // would make the unbudgeted claim false.
      << ",\"profile\":\"" << report.options.budget_profile
      << "\",\"all_axes_at_representational_ceiling\":"
      << boolean(
             report.options.support_work_budget ==
                 std::numeric_limits<std::size_t>::max() &&
             report.options.support_record_budget ==
                 std::numeric_limits<std::size_t>::max() &&
             report.options.higher_chunk_limit ==
                 std::numeric_limits<std::size_t>::max() &&
             report.options.downstream_record_budget ==
                 std::numeric_limits<std::size_t>::max() &&
             report.options.descent_work_budget ==
                 std::numeric_limits<std::size_t>::max() &&
             report.options.chunk_byte_budget ==
                 std::numeric_limits<std::uint64_t>::max())
      << ",\"no_configured_budget_stop\":"
      << boolean(!report.budget_exhausted)
      << ",\"sealed_structural_bounds\":{"
      << "\"facet_descent_closure_maximum_nodes\":"
      << direct_sparse_facet_descent_closure_maximum_node_count
      << ",\"higher_support_maximum_requested_order\":"
      << higher_support_maximum_requested_order
      << ",\"interactive_resident_maximum_point_count\":"
      << direct_morse_interactive_resident_maximum_point_count
      << ",\"device_tile_slot_capacity\":"
      << runner_maximum_higher_tile_target << "}},\n"
      << "  \"timings_ms\":{\"generation\":" << std::fixed
      << std::setprecision(3) << report.timings.generation_ms
      << ",\"canonicalization\":"
      << report.timings.canonicalization_ms
      << ",\"lbvh\":" << report.timings.lbvh_ms
      << ",\"pair_support\":"
      << report.timings.pair_support_ms
      << ",\"higher_support\":"
      << report.timings.higher_support_ms
      << ",\"terminal_facade\":"
      << report.timings.terminal_facade_ms
      << ",\"event_journal\":"
      << report.timings.event_journal_ms
      << ",\"saddle_seed_journal\":"
      << report.timings.saddle_seed_journal_ms
      << ",\"batch_plan\":"
      << report.timings.batch_plan_ms
      << ",\"reducer_setup\":"
      << report.timings.reducer_setup_ms
      << ",\"reducer_stream\":"
      << report.timings.reducer_stream_ms
      << ",\"forest_finish\":"
      << report.timings.forest_finish_ms
      << ",\"event_rank_tower_link_journal\":"
      << report.timings.event_rank_tower_link_journal_ms
      << ",\"event_vertical_propagation_journal\":"
      << report.timings.event_vertical_propagation_journal_ms
      << ",\"event_vertical_propagation_fresh_verification\":"
      << report.timings.event_vertical_propagation_fresh_verification_ms
      << ",\"conditional_adjacent_event_vertical_propagation\":"
      << report.timings.conditional_adjacent_event_vertical_propagation_ms
      << ",\"vertical_target_pipeline\":"
      << report.timings.vertical_target_pipeline_ms
      << ",\"vertical_journal\":"
      << report.timings.vertical_journal_ms
      << ",\"k2_to_k1_oracle_source_history\":"
      << report.timings.k2_to_k1_oracle_source_history_ms
      << ",\"k2_to_k1_oracle_k1\":"
      << report.timings.k2_to_k1_oracle_k1_ms
      << ",\"k2_to_k1_oracle_hierarchy\":"
      << report.timings.k2_to_k1_oracle_hierarchy_ms
      << ",\"k2_to_k1_target_authority_build\":"
      << report.timings.k2_to_k1_target_authority_build_ms
      << ",\"k2_to_k1_target_authority_verify\":"
      << report.timings.k2_to_k1_target_authority_verify_ms
      << ",\"k2_to_k1_target_authority\":"
      << report.timings.k2_to_k1_target_authority_ms
      << ",\"m1_o5_death_accounting_build\":"
      << report.timings.m1_o5_death_accounting_build_ms
      << ",\"m1_o5_death_accounting_verify\":"
      << report.timings.m1_o5_death_accounting_verify_ms
      << ",\"m1_o5_death_accounting\":"
      << report.timings.m1_o5_death_accounting_ms
      << ",\"direct_morse_gamma_carrier_conformance\":"
      << report.timings.direct_morse_gamma_carrier_conformance_ms
      << ",\"vertical_target_replay_diagnostic\":"
      << report.timings.vertical_target_replay_diagnostic_ms
      << ",\"total\":" << report.timings.total_ms << "},\n"
      << "  \"pair_support\":{\"source_kind\":\""
      << report.pair_source_kind << "\",\"authority_kind\":\""
      << report.pair_authority_kind
      << "\",\"p7b_replay_performed\":false,\"status\":\""
      << report.pair_status << "\",\"stop_reason\":\""
      << report.pair_stop_reason << "\",\"maximum_closed_rank\":"
      << report.pair_maximum_closed_rank
      << ",\"schedule_config\":{\"maximum_anchors_per_group\":"
      << report.pair_schedule_config.maximum_anchor_count_per_group
      << ",\"proposed_witness_pool_size\":"
      << report.pair_schedule_config.proposed_witness_pool_size
      << ",\"triangular_block_pair_schedule\":"
      << (report.pair_schedule_config.use_triangular_block_pair_schedule
              ? "true"
              : "false")
      << ",\"symmetric_inconclusive_cross_block_splitting\":"
      << (report.pair_schedule_config
                  .use_symmetric_inconclusive_cross_block_splitting
              ? "true"
              : "false")
      << ",\"prioritize_cross_blocks\":"
      << (report.pair_schedule_config.prioritize_cross_blocks
              ? "true"
              : "false")
      << ",\"witness_subtree_first_for_triangular_blocks\":"
      << (report.pair_schedule_config
                  .use_witness_subtree_first_for_triangular_blocks
              ? "true"
              : "false")
      << ",\"floating_witness_order_for_triangular_blocks\":"
      << (report.pair_schedule_config
                  .use_floating_witness_order_for_triangular_blocks
              ? "true"
              : "false")
      << "},\"advance_budget\":{\"schedule_advances\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_schedule_advance_count
      << ",\"orientation_checks\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_orientation_check_count
      << ",\"grouped_node_visits\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_node_visit_count
      << ",\"grouped_exact_predicates\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"grouped_logical_signs\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"classification_node_visits\":"
      << report.pair_advance_budget.classifier.maximum_node_visit_count
      << ",\"emitted_records\":"
      << report.pair_advance_budget.maximum_emitted_record_count
      << ",\"emitted_point_id_references\":"
      << report.pair_advance_budget
             .maximum_emitted_point_id_reference_count
      << "},\"total_capacity\":{\"schedule_advances\":"
      << report.pair_total_capacity.maximum_schedule_advance_count
      << ",\"orientation_checks\":"
      << report.pair_total_capacity.maximum_orientation_check_count
      << ",\"grouped_node_visits\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_node_visit_count
      << ",\"grouped_exact_predicates\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"grouped_logical_signs\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"admitted_candidates\":"
      << report.pair_total_capacity.maximum_admitted_candidate_count
      << ",\"classification_node_visits\":"
      << report.pair_total_capacity
             .maximum_classification_node_visit_count
      << ",\"output_records\":"
      << report.pair_total_capacity.maximum_output_record_count
      << ",\"output_point_id_references\":"
      << report.pair_total_capacity
             .maximum_output_point_id_reference_count
      << "},\"audit\":{\"advance_calls\":"
      << report.pair_advance_calls << ",\"schedule_advances\":"
      << report.pair_schedule_advances << ",\"orientation_checks\":"
      << report.pair_orientation_checks
      << ",\"reverse_or_self_orientation_skips\":"
      << report.pair_reverse_or_self_orientation_skips
      << ",\"prepared_groups\":"
      << report.pair_prepared_groups
      << ",\"completed_groups\":"
      << report.pair_completed_groups
      << ",\"grouped_node_visits\":"
      << report.pair_grouped_traversal_node_visits
      << ",\"witness_subtree_node_visits\":"
      << report.pair_witness_subtree_node_visits
      << ",\"grouped_common_node_visits\":"
      << report.pair_grouped_common_node_visits
      << ",\"anchor_subgroup_node_visits\":"
      << report.pair_anchor_subgroup_node_visits
      << ",\"singleton_node_visits\":"
      << report.pair_singleton_node_visits
      << ",\"grouped_witness_slots\":"
      << report.pair_grouped_witness_slots
      << ",\"grouped_inherited_witness_reuses\":"
      << report.pair_grouped_inherited_witness_reuses
      << ",\"grouped_exact_predicates\":"
      << report.pair_grouped_traversal_exact_predicates
      << ",\"grouped_logical_signs\":"
      << report.pair_grouped_traversal_exact_predicates
      << ",\"fp64_filtered_negative_predicates\":"
      << report.pair_fp64_filtered_negative_predicates
      << ",\"fp64_filtered_positive_predicates\":"
      << report.pair_fp64_filtered_positive_predicates
      << ",\"exact_fallback_predicates\":"
      << report.pair_exact_fallback_predicates
      << ",\"floating_witness_order_preparations\":"
      << report.pair_floating_witness_order_preparations
      << ",\"floating_witness_score_evaluations\":"
      << report.pair_floating_witness_score_evaluations
      << ",\"floating_witness_nonfinite_scores\":"
      << report.pair_floating_witness_nonfinite_scores
      << ",\"floating_witness_order_requested\":"
      << (report.pair_floating_witness_order_requested ? "true" : "false")
      << ",\"floating_witness_order_effective_for_every_prepared_traversal\":"
      << (report
                  .pair_floating_witness_order_effective_for_every_prepared_traversal
              ? "true"
              : "false")
      << ",\"fp64_filter_partition_certified\":"
      << (report.pair_fp64_filter_partition_certified ? "true" : "false")
      << ",\"witness_subtree_exact_predicates\":"
      << report.pair_witness_subtree_exact_predicates
      << ",\"witness_subtree_receipts\":"
      << report.pair_witness_subtree_receipts
      << ",\"witness_subtree_successes\":"
      << report.pair_witness_subtree_successes
      << ",\"witness_subtree_fail_opens\":"
      << report.pair_witness_subtree_fail_opens
      << ",\"grouped_common_exact_predicates\":"
      << report.pair_grouped_common_exact_predicates
      << ",\"anchor_subgroup_exact_predicates\":"
      << report.pair_anchor_subgroup_exact_predicates
      << ",\"singleton_exact_predicates\":"
      << report.pair_singleton_exact_predicates
      << ",\"grouped_strict_witness_discoveries\":"
      << report.pair_grouped_strict_witness_discoveries
      << ",\"grouped_diagonal_node_descents\":"
      << report.pair_grouped_diagonal_node_descents
      << ",\"grouped_common_frontiers\":"
      << report.pair_grouped_common_frontiers
      << ",\"delegated_frontier_anchors\":"
      << report.pair_delegated_frontier_anchors
      << ",\"prepared_anchor_subgroup_probes\":"
      << report.pair_prepared_anchor_subgroup_probes
      << ",\"anchor_subgroup_witness_pool_entries\":"
      << report.pair_anchor_subgroup_witness_pool_entries
      << ",\"query_facing_fallback_witness_pool_entries\":"
      << report.pair_query_facing_fallback_witness_pool_entries
      << ",\"anchor_subgroup_splits\":"
      << report.pair_anchor_subgroup_splits
      << ",\"query_subtree_splits\":"
      << report.pair_query_subtree_splits
      << ",\"anchor_subgroup_certified_prunes\":"
      << report.pair_anchor_subgroup_certified_prunes
      << ",\"anchor_subgroup_certified_anchors\":"
      << report.pair_anchor_subgroup_certified_anchors
      << ",\"prepared_singleton_fallbacks\":"
      << report.pair_prepared_singleton_fallbacks
      << ",\"completed_singleton_fallbacks\":"
      << report.pair_completed_singleton_fallbacks
      << ",\"singleton_witness_pool_entries\":"
      << report.pair_singleton_witness_pool_entries
      << ",\"singleton_certified_prunes\":"
      << report.pair_singleton_certified_prunes
      << ",\"maximum_pending_anchor_subgroups\":"
      << report.pair_maximum_pending_anchor_subgroups
      << ",\"triangular_block_pair_visits\":"
      << report.pair_triangular_block_pair_visits
      << ",\"triangular_diagonal_splits\":"
      << report.pair_triangular_diagonal_splits
      << ",\"triangular_oversized_anchor_splits\":"
      << report.pair_triangular_oversized_anchor_splits
      << ",\"triangular_consumer_query_splits\":"
      << report.pair_triangular_consumer_query_splits
      << ",\"triangular_self_pairs\":"
      << report.pair_triangular_self_pairs
      << ",\"triangular_cross_blocks\":"
      << report.pair_triangular_cross_blocks
      << ",\"triangular_certified_cross_blocks\":"
      << report.pair_triangular_certified_cross_blocks
      << ",\"triangular_certified_unordered_pairs\":"
      << report.pair_triangular_certified_unordered_pairs
      << ",\"triangular_opened_singleton_cross_blocks\":"
      << report.pair_triangular_opened_singleton_cross_blocks
      << ",\"triangular_opened_unordered_pairs\":"
      << report.pair_triangular_opened_unordered_pairs
      << ",\"triangular_maximum_pending_block_pairs\":"
      << report.pair_triangular_maximum_pending_block_pairs
      << ",\"triangular_partition_complete\":"
      << (report.pair_triangular_partition_complete ? "true" : "false")
      << ",\"no_dynamic_dual_tree_or_pair_arena\":"
      << (report.pair_no_dynamic_dual_tree_or_pair_arena ? "true" : "false")
      << ",\"authenticated_prunes\":"
      << report.pair_authenticated_prunes
      << ",\"authenticated_pruned_directed_pairs\":"
      << report.pair_authenticated_pruned_directed_pairs
      << ",\"directed_pair_universe\":"
      << report.pair_directed_pair_universe
      << ",\"admitted_candidates\":"
      << report.pair_admitted_candidates
      << ",\"classification_advances\":"
      << report.pair_classification_advances
      << ",\"classification_node_visits\":"
      << report.pair_classification_node_visits
      << ",\"classification_terminals\":"
      << report.pair_classification_terminals
      << ",\"above_rank\":" << report.pair_above_rank
      << ",\"output_records\":" << report.pair_output_records
      << ",\"output_point_id_references\":"
      << report.pair_output_point_id_references
      << ",\"local_budget_exhaustions\":"
      << report.pair_local_budget_exhaustions
      << ",\"total_capacity_exhaustions\":"
      << report.pair_total_capacity_exhaustions
      << ",\"maximum_live_candidates\":"
      << report.pair_maximum_live_candidates
      << ",\"accepted_events\":"
      << report.pair_accepted_events
      << ",\"extra_shell_diagnostics\":"
      << report.pair_extra_shell_diagnostics
      << ",\"directed_coverage_certified\":"
      << boolean(report.pair_directed_coverage_certified)
      << ",\"orientation_partition_certified\":"
      << boolean(report.pair_orientation_partition_certified)
      << ",\"classification_partition_certified\":"
      << boolean(report.pair_classification_partition_certified)
      << ",\"output_partition_certified\":"
      << boolean(report.pair_output_partition_certified)
      << ",\"records_certified\":"
      << boolean(report.pair_records_certified) << "}},\n"
      << "  \"higher_support\":{\"status\":\""
      << report.higher_status << "\",\"stop_reason\":\""
      << report.higher_stop_reason << "\",\"work_units\":"
      << report.higher_work_units << ",\"product_visits\":"
      << report.higher_product_visits
      << ",\"closed_ball_queries\":"
      << report.higher_closed_ball_queries
      << ",\"accepted_events\":"
      << report.higher_accepted_events
      << ",\"extra_shell_diagnostics\":"
      << report.higher_extra_shell_diagnostics
      << ",\"prune_certificates\":"
      << report.higher_prune_certificates
      << ",\"chunks\":" << report.higher_chunk_count
      << ",\"authority_kind\":\""
      << report.higher_authority_kind
      << "\",\"verification_basis\":\""
      << report.higher_verification_basis
      << "\",\"requested_tile_target\":"
      << report.higher_requested_tile_target
      << ",\"full_geometry_replay_avoided\":"
      << boolean(
             report.higher_authority_kind ==
             "sealed_anchored_fixed_chunk_run")
      << ",\"assembly_censure\":\""
      << report.higher_assembly_censure
      << "\",\"progress\":{\"total_support_mass\":\""
      << report.higher_total_support_mass
      << "\",\"resolved_support_mass\":\""
      << report.higher_resolved_support_mass
      << "\",\"remaining_frontier_support_mass\":\""
      << report.higher_remaining_frontier_support_mass
      << "\",\"minimal_leaves\":" << report.higher_minimal_leaf_count
      << ",\"above_rank_leaves\":" << report.higher_above_rank_leaf_count
      << ",\"leaf_support_analyses\":"
      << report.higher_leaf_support_analyses
      << ",\"point_classifications\":"
      << report.higher_point_classifications
      << ",\"closed_ball_node_visits\":"
      << report.higher_closed_ball_node_visits
      << ",\"committed_transactions\":"
      << report.higher_committed_transactions
      << ",\"committed_tile_transactions\":"
      << report.higher_committed_tile_transactions
      << ",\"committed_expansion_transitions\":"
      << report.higher_committed_expansion_transitions
      << ",\"committed_tile_slots\":"
      << report.higher_committed_tile_slots
      << ",\"frontier_entries\":" << report.higher_frontier_entries
      << "}},\n"
      << "  \"pipeline_counts\":{\"terminal_catalog_certified\":"
      << boolean(report.terminal_catalog_certified)
      << ",\"terminal_events\":"
      << report.terminal_event_count
      << ",\"terminal_extra_shell_diagnostics\":"
      << report.terminal_extra_shell_diagnostic_count
      << ",\"event_batches\":" << report.event_batch_count
      << ",\"event_roles\":" << report.event_role_count
      << ",\"saddle_families\":" << report.saddle_family_count
      << ",\"arm_seeds\":" << report.arm_seed_count
      << ",\"industrial_chunks\":"
      << report.industrial_chunk_count
      << ",\"plan_lanes\":" << report.plan_lane_count
      << ",\"prepared_tickets\":"
      << report.prepared_ticket_count
      << ",\"committed_batches\":"
      << report.committed_batch_count << "},\n"
      << "  \"decisions\":{\"industrial_plan\":"
      << report.industrial_plan_decision
      << ",\"batch_plan\":" << report.plan_decision
      << ",\"last_batch_execution\":"
      << report.batch_execution_decision
      << ",\"last_preparation\":"
      << report.preparation_decision
      << ",\"last_reducer_fold\":"
      << report.reducer_fold_decision
      << ",\"last_live_commit\":"
      << report.live_commit_decision << "},\n"
      << "  \"forest\":{\"birth_records\":"
      << report.forest_birth_count
      << ",\"materialized_birth_records\":"
      << report.forest_materialized_birth_count
      << ",\"saddles\":" << report.forest_saddle_count
      << ",\"atomic_groups\":"
      << report.forest_atomic_group_count
      << ",\"nodes\":" << report.forest_node_count
      << ",\"materialized_nodes\":"
      << report.forest_materialized_node_count
      << ",\"final_roots\":" << report.forest_final_root_count
      << ",\"logical_output_entries\":"
      << report.forest_logical_output_entry_count
      << ",\"aggregate_closure_nodes\":"
      << report.forest_aggregate_closure_node_count
      << ",\"aggregate_closure_step_calls\":"
      << report.forest_aggregate_closure_step_call_count << "},\n"
      << "  \"durable_archive\":{\"requested\":"
      << boolean(report.durable_archive_requested)
      << ",\"published\":"
      << boolean(report.durable_archive_published)
      << ",\"identity_recertified\":"
      << boolean(report.durable_identity_recertified)
      << ",\"segments\":" << report.durable_segment_count
      << ",\"reloaded_segments\":"
      << report.durable_reloaded_segment_count
      << ",\"archive_finalize_ms\":"
      << report.timings.archive_finalize_ms
      << ",\"archive_reload_ms\":"
      << report.timings.archive_reload_ms
      << ",\"archive_digest\":\""
      << report.durable_archive_digest_hex << "\"},\n"
      << "  \"conditional_adjacent_event_vertical_propagation\":{\"requested\":"
      << boolean(
             report
                 .conditional_adjacent_event_vertical_propagation_qualification_requested)
      << ",\"attempted\":"
      << boolean(
             report
                 .conditional_adjacent_event_vertical_propagation_attempted)
      << ",\"certified_conditional_candidate\":"
      << boolean(
             report
                 .conditional_adjacent_event_vertical_propagation_certified)
      << ",\"link_journal\":{\"schema_version\":"
      << event_links.schema_version
      << ",\"backend\":\"" << event_links.backend
      << "\",\"profile\":\"" << event_links.profile
      << "\",\"mode\":\"" << event_links.mode
      << "\",\"refinement_status\":\""
      << event_links.refinement_status
      << "\",\"public_status\":\"" << event_links.public_status
      << "\",\"proof_basis\":\""
      << json_escape(event_links.proof_basis)
      << "\",\"decision\":"
      << static_cast<unsigned>(event_links.decision)
      << ",\"scope\":" << static_cast<unsigned>(event_links.scope)
      << ",\"links\":" << event_links.links.size()
      << ",\"arm_terminals\":" << event_links.arm_terminals.size()
      << ",\"logical_output_entries\":"
      << event_links.logical_output_entry_count
      << ",\"counters\":{\"forest_birth_record_scans\":"
      << event_links.counters.forest_birth_record_scan_count
      << ",\"forest_saddle_record_scans\":"
      << event_links.counters.forest_saddle_record_scan_count
      << ",\"forest_arm_binding_scans\":"
      << event_links.counters.forest_arm_binding_scan_count
      << ",\"forest_atomic_group_scans\":"
      << event_links.counters.forest_atomic_group_scan_count
      << ",\"forest_batch_scans\":"
      << event_links.counters.forest_batch_scan_count
      << ",\"forest_node_scans\":"
      << event_links.counters.forest_node_scan_count
      << ",\"higher_rank_births\":"
      << event_links.counters.higher_rank_birth_count
      << ",\"terminal_maximum_rank_births\":"
      << event_links.counters.terminal_maximum_rank_birth_count
      << ",\"link_count\":" << event_links.counters.link_count
      << ",\"arm_terminal_references\":"
      << event_links.counters.arm_terminal_reference_count
      << "},\"facts\":{\"budget_preflight_certified\":"
      << boolean(event_links.budget_preflight_certified)
      << ",\"source_conditional_forest_certificate_replayed\":"
      << boolean(
             event_links.source_conditional_forest_certificate_replayed)
      << ",\"source_forest_structure_replayed\":"
      << boolean(event_links.source_forest_structure_replayed)
      << ",\"every_rank_at_least_two_birth_linked_exactly_once\":"
      << boolean(
             event_links.every_rank_at_least_two_birth_linked_exactly_once)
      << ",\"same_source_event_projection_replayed\":"
      << boolean(event_links.same_source_event_projection_replayed)
      << ",\"adjacent_order_role_replayed\":"
      << boolean(event_links.adjacent_order_role_replayed)
      << ",\"exact_level_identity_replayed\":"
      << boolean(event_links.exact_level_identity_replayed)
      << ",\"strict_arm_terminals_replayed\":"
      << boolean(
             event_links
                 .every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth)
      << ",\"strict_pre_batch_roots_replayed\":"
      << boolean(event_links.strict_pre_batch_roots_replayed)
      << ",\"atomic_post_batch_target_replayed\":"
      << boolean(event_links.atomic_post_batch_target_replayed)
      << ",\"terminal_maximum_rank_link_required\":"
      << boolean(event_links.terminal_maximum_rank_link_required)
      << ",\"all_present_terminal_maximum_rank_births_included\":"
      << boolean(
             event_links.all_present_terminal_maximum_rank_births_included)
      << ",\"lower_rank_links_are_composition_only\":"
      << boolean(event_links.lower_rank_links_are_composition_only)
      << ",\"output_csr_and_linear_in_source_events_and_arms\":"
      << boolean(
             event_links.output_csr_and_linear_in_source_events_and_arms)
      << "},\"nonclaims\":{\"gamma_cells_or_global_cofaces_materialized\":"
      << boolean(event_links.gamma_cells_or_global_cofaces_materialized)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(event_links.higher_order_delaunay_materialized)
      << ",\"forbidden_global_structure_materialized\":"
      << boolean(event_links.forbidden_global_structure_materialized)
      << ",\"public_status_claimed\":"
      << boolean(event_links.public_status_claimed) << "}},"
      << "\"propagation_journal\":{\"schema_version\":"
      << event_propagation.schema_version
      << ",\"backend\":\"" << event_propagation.backend
      << "\",\"profile\":\"" << event_propagation.profile
      << "\",\"mode\":\"" << event_propagation.mode
      << "\",\"refinement_status\":\""
      << event_propagation.refinement_status
      << "\",\"public_status\":\""
      << event_propagation.public_status
      << "\",\"proof_basis\":\""
      << json_escape(event_propagation.proof_basis)
      << "\",\"decision\":"
      << static_cast<unsigned>(event_propagation.decision)
      << ",\"scope\":" << static_cast<unsigned>(event_propagation.scope)
      << ",\"carrier_anchors\":"
      << event_propagation.carrier_anchors.size()
      << ",\"group_anchors\":"
      << event_propagation.group_anchors.size()
      << ",\"group_input_references\":"
      << event_propagation.group_input_references.size()
      << ",\"final_root_anchors\":"
      << event_propagation.final_root_anchors.size()
      << ",\"logical_output_entries\":"
      << event_propagation.logical_output_entry_count
      << ",\"counters\":{\"source_link_scans\":"
      << event_propagation.counters.source_link_scan_count
      << ",\"source_group_scans\":"
      << event_propagation.counters.source_group_scan_count
      << ",\"source_arm_binding_scans\":"
      << event_propagation.counters.source_arm_binding_scan_count
      << ",\"source_node_scans\":"
      << event_propagation.counters.source_node_scan_count
      << ",\"source_child_reference_scans\":"
      << event_propagation.counters.source_child_reference_scan_count
      << ",\"source_final_root_scans\":"
      << event_propagation.counters.source_final_root_scan_count
      << ",\"carrier_anchors\":"
      << event_propagation.counters.carrier_anchor_count
      << ",\"unconsumed_latent_carrier_anchors\":"
      << event_propagation.counters.unconsumed_latent_carrier_anchor_count
      << ",\"terminal_maximum_rank_latent_carrier_anchors\":"
      << event_propagation.counters
             .terminal_maximum_rank_latent_carrier_anchor_count
      << ",\"group_anchors\":"
      << event_propagation.counters.group_anchor_count
      << ",\"group_input_references\":"
      << event_propagation.counters.group_input_reference_count
      << ",\"final_root_anchors\":"
      << event_propagation.counters.final_root_anchor_count
      << ",\"parent_activations\":"
      << event_propagation.counters.parent_activation_count
      << ",\"parent_find_steps\":"
      << event_propagation.counters.parent_find_step_count
      << ",\"path_compression_writes\":"
      << event_propagation.counters.path_compression_write_count
      << "},\"facts\":{\"budget_preflight_certified\":"
      << boolean(event_propagation.budget_preflight_certified)
      << ",\"source_conditional_forest_compact_contract_checked\":"
      << boolean(
             event_propagation
                 .source_conditional_forest_compact_contract_checked)
      << ",\"source_link_journal_forest_relative_freshly_rebuilt\":"
      << boolean(
             event_propagation
                 .source_link_journal_forest_relative_freshly_rebuilt)
      << ",\"source_parent_forest_reconstructed\":"
      << boolean(event_propagation.source_parent_forest_reconstructed)
      << ",\"one_anchor_per_adjacent_event_link\":"
      << boolean(event_propagation.one_anchor_per_adjacent_event_link)
      << ",\"every_order_at_least_two_source_group_input_propagated\":"
      << boolean(
             event_propagation
                 .every_order_at_least_two_source_group_input_propagated)
      << ",\"every_order_at_least_two_source_group_anchor_converged\":"
      << boolean(
             event_propagation
                 .every_order_at_least_two_source_group_anchor_converged)
      << ",\"every_reduced_final_root_advanced_to_lower_final_root\":"
      << boolean(
             event_propagation
                 .every_reduced_final_root_advanced_to_lower_final_root)
      << ",\"every_nonterminal_higher_order_carrier_reaches_source_group\":"
      << boolean(
             event_propagation
                 .every_nonterminal_higher_order_carrier_reaches_source_group)
      << ",\"terminal_maximum_rank_latent_carrier_required\":"
      << boolean(
             event_propagation
                 .terminal_maximum_rank_latent_carrier_required)
      << ",\"terminal_maximum_rank_latent_carrier_retained\":"
      << boolean(
             event_propagation
                 .terminal_maximum_rank_latent_carrier_retained)
      << ",\"lower_rank_maps_are_composition_only\":"
      << boolean(event_propagation.lower_rank_maps_are_composition_only)
      << ",\"output_storage_flat_and_linear_in_sparse_forest\":"
      << boolean(
             event_propagation
                 .output_storage_flat_and_linear_in_sparse_forest)
      << "},\"premises_and_nonclaims\":{\"conditional_on_caller_fresh_upstream_forest_and_link_replay\":"
      << boolean(
             event_propagation
                 .conditional_on_caller_fresh_upstream_forest_and_link_replay)
      << ",\"conditional_on_direct_forest_o3_o4\":"
      << boolean(event_propagation.conditional_on_direct_forest_o3_o4)
      << ",\"gamma_totality_replayed\":"
      << boolean(event_propagation.gamma_totality_replayed)
      << ",\"m1_reconstruction_claimed\":"
      << boolean(event_propagation.m1_reconstruction_claimed)
      << ",\"gamma_cells_or_global_cofaces_materialized\":"
      << boolean(
             event_propagation.gamma_cells_or_global_cofaces_materialized)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(event_propagation.higher_order_delaunay_materialized)
      << ",\"forbidden_global_structure_materialized\":"
      << boolean(event_propagation.forbidden_global_structure_materialized)
      << ",\"public_status_claimed\":"
      << boolean(event_propagation.public_status_claimed) << "}},"
      << "\"fresh_forest_verification\":{\"source_link\":{\"trusted_forest_inputs_certified\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .trusted_forest_inputs_certified)
      << ",\"source_forest_storage_within_budget\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .source_forest_storage_within_budget)
      << ",\"source_forest_freshly_reconstructed\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .source_forest_freshly_reconstructed)
      << ",\"source_forest_recursively_equal\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .source_forest_recursively_equal)
      << ",\"conditional_on_caller_fresh_phase9_facade_replay\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .conditional_on_caller_fresh_phase9_facade_replay)
      << ",\"source_forest_certified\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .source_forest_certified)
      << ",\"observed_link_storage_within_budget\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .observed_link_storage_within_budget)
      << ",\"expected_link_journal_freshly_rebuilt\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .expected_link_journal_freshly_rebuilt)
      << ",\"observed_link_structure_certified\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .observed_link_structure_certified)
      << ",\"observed_link_recursively_equal\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .observed_link_recursively_equal)
      << ",\"result_certified\":"
      << boolean(
             event_strong_verification.source_link_fresh_forest_verification
                 .result_certified)
      << "},\"propagation\":{\"source_forest_compact_contract_checked\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .source_forest_compact_contract_checked)
      << ",\"source_link_journal_forest_relative_freshly_verified\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .source_link_journal_forest_relative_freshly_verified)
      << ",\"conditional_on_caller_fresh_upstream_forest_and_link_replay\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .conditional_on_caller_fresh_upstream_forest_and_link_replay)
      << ",\"observed_storage_within_budget\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .observed_storage_within_budget)
      << ",\"expected_journal_freshly_rebuilt\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .expected_journal_freshly_rebuilt)
      << ",\"observed_structure_certified\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .observed_structure_certified)
      << ",\"observed_recursively_equal\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .observed_recursively_equal)
      << ",\"result_certified\":"
      << boolean(
             event_strong_verification.propagation_verification
                 .result_certified)
      << "},\"conditional_on_caller_fresh_phase9_facade_replay\":"
      << boolean(
             event_strong_verification
                 .conditional_on_caller_fresh_phase9_facade_replay)
      << ",\"result_certified\":"
      << boolean(event_strong_verification.result_certified)
      << "},\"nonclaims\":{\"global_morse_obligation_replayed\":false,"
         "\"gamma_totality_replayed\":false,"
         "\"all_naturality_squares_replayed\":false,"
         "\"vertical_maps_complete\":false,"
         "\"global_m1_claimed\":false,"
         "\"forest_semantics_exact\":false,"
         "\"product_architecture_claimed\":false,"
         "\"scalable_50k_claimed\":false,"
         "\"public_status_claimed\":false}},\n"
      << "  \"vertical_target_pipeline\":{\"attempted\":"
      << boolean(report.vertical_target_pipeline_attempted)
      << ",\"certified\":"
      << boolean(report.vertical_target_pipeline_certified)
      << ",\"decision\":"
      << report.vertical_target_pipeline_decision
      << ",\"required_sessions\":"
      << report.vertical_target_required_session_count
      << ",\"required_groups\":"
      << report.vertical_target_required_group_count
      << ",\"required_proposals\":"
      << report.vertical_target_required_proposal_count
      << ",\"source_group_count_by_order\":";
  emit_size_vector_json(report.vertical_target_source_group_count_by_order);
  std::cout
      << ",\"rejected_source_atomic_group_index\":";
  if (report.vertical_target_rejected_source_group_index.has_value()) {
    std::cout << *report.vertical_target_rejected_source_group_index;
  } else {
    std::cout << "null";
  }
  const auto& vertical_counters =
      report.vertical_target_pipeline_counters;
  std::cout
      << ",\"rejected_closure_adapter_decision\":"
      << report.vertical_target_rejected_closure_adapter_decision
      << ",\"counters\":{\"source_batches_scanned\":"
      << vertical_counters.source_batch_scan_count
      << ",\"source_groups_scanned\":"
      << vertical_counters.source_atomic_group_scan_count
      << ",\"target_order_lookups\":"
      << vertical_counters.target_order_lookup_count
      << ",\"referenced_target_orders\":"
      << vertical_counters.referenced_target_order_count
      << ",\"initialized_sessions\":"
      << vertical_counters.initialized_session_count
      << ",\"preflight_plans\":"
      << vertical_counters.preflight_facet_plan_count
      << ",\"executed_plans\":"
      << vertical_counters.executed_facet_plan_count
      << ",\"replay_advances\":"
      << vertical_counters.replay_advance_count
      << ",\"closure_builds\":"
      << vertical_counters.closure_build_count
      << ",\"proposal_adapters\":"
      << vertical_counters.proposal_adapter_count
      << ",\"representatives\":"
      << vertical_counters.representative_count
      << ",\"projected_target_facets\":"
      << vertical_counters.projected_target_facet_reference_count
      << ",\"distinct_target_facets\":"
      << vertical_counters.distinct_target_facet_count
      << ",\"closure_terminal_summaries\":"
      << vertical_counters.closure_terminal_summary_count
      << ",\"unresolved_proposals\":"
      << vertical_counters.unresolved_proposal_count
      << ",\"resolved_proposals\":"
      << vertical_counters.resolved_proposal_count
      << "},\"forest_relative_only\":true,"
         "\"external_target_authority_replayed\":false,"
         "\"vertical_maps_complete\":false,"
         "\"public_status_claimed\":false},\n"
      << "  \"vertical_journal\":{\"attempted\":"
      << boolean(report.vertical_journal_attempted)
      << ",\"certified_conditional_candidate\":"
      << boolean(report.vertical_journal_certified)
      << ",\"decision\":"
      << report.vertical_journal_decision
      << ",\"adjacent_families\":"
      << report.vertical_journal_adjacent_family_count
      << ",\"label_resolutions\":"
      << report.vertical_journal_label_resolution_count
      << ",\"group_checks\":"
      << report.vertical_journal_group_check_count
      << ",\"checkpoints\":"
      << report.vertical_journal_checkpoint_count
      << ",\"logical_output_entries\":"
      << report.vertical_journal_logical_output_entry_count;
  const auto& vertical_journal_counters =
      report.vertical_journal_counters;
  std::cout
      << ",\"counters\":{\"expected_labels\":"
      << vertical_journal_counters.expected_label_count
      << ",\"missing_labels\":"
      << vertical_journal_counters.missing_label_count
      << ",\"unresolved_labels\":"
      << vertical_journal_counters.unresolved_label_count
      << ",\"resolved_labels\":"
      << vertical_journal_counters.resolved_label_count
      << ",\"complete_groups\":"
      << vertical_journal_counters.complete_group_count
      << ",\"partial_groups\":"
      << vertical_journal_counters.partial_group_count
      << ",\"expected_elementary_group_squares\":"
      << vertical_journal_counters.expected_elementary_group_square_count
      << ",\"checked_elementary_group_squares\":"
      << vertical_journal_counters.checked_elementary_group_square_count
      << ",\"unresolved_elementary_group_squares\":"
      << vertical_journal_counters.unresolved_elementary_group_square_count
      << ",\"late_checkpoints\":"
      << vertical_journal_counters.late_checkpoint_count
      << "},\"source_forest_shape_replayed\":"
      << boolean(report.vertical_journal_source_forest_shape_replayed)
      << ",\"conditional_on_caller_fresh_source_forest_replay\":"
      << boolean(
             report
                 .vertical_journal_conditional_on_fresh_source_forest_replay)
      << ",\"external_target_authority_replayed\":"
      << boolean(
             report.vertical_journal_external_target_authority_replayed)
      << ",\"global_morse_obligation_replayed\":"
      << boolean(report.vertical_journal_global_morse_obligation_replayed)
      << ",\"all_naturality_squares_replayed\":"
      << boolean(report.vertical_journal_all_naturality_squares_replayed)
      << ",\"vertical_maps_complete\":"
      << boolean(report.vertical_journal_vertical_maps_complete)
      << ",\"gamma_cells_or_global_cofaces_materialized\":"
      << boolean(
             report.vertical_journal_gamma_or_global_cofaces_materialized)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(
             report.vertical_journal_higher_order_delaunay_materialized)
      << ",\"public_status_claimed\":"
      << boolean(report.vertical_journal_public_status_claimed)
      << "},\n"
      << "  \"k2_to_k1_target_authority\":{\"required\":"
      << boolean(report.k2_to_k1_target_authority_required)
      << ",\"attempted\":"
      << boolean(report.k2_to_k1_target_authority_attempted)
      << ",\"certified_observed_label_target_authority\":"
      << boolean(report.k2_to_k1_target_authority_certified)
      << ",\"decision\":"
      << report.k2_to_k1_target_authority_decision
      << ",\"scope\":"
      << report.k2_to_k1_target_authority_scope
      << ",\"backend\":\"reference_cpu\""
         ",\"profile\":\"hgp_reduced\""
         ",\"mode\":\"bounded_n14_k2_to_k1_observed_label_external_"
         "target_authority_conformance\""
         ",\"public_status\":\"not_claimed\""
         ",\"product_nonbounded_modes_keep_oracle_dormant\":true"
      << ",\"oracle\":{\"source_batches\":"
      << report.k2_to_k1_oracle_source_batch_count
      << ",\"source_groups\":"
      << report.k2_to_k1_oracle_source_group_count
      << ",\"source_nodes\":"
      << report.k2_to_k1_oracle_source_node_count
      << ",\"k1_nodes\":"
      << report.k2_to_k1_oracle_k1_node_count
      << ",\"external_checkpoints\":"
      << report.k2_to_k1_oracle_external_checkpoint_count
      << "},\"digests\":{\"direct_cloud\":\""
      << json_escape(report.k2_to_k1_direct_cloud_digest)
      << "\",\"external_cloud\":\""
      << json_escape(report.k2_to_k1_external_cloud_digest)
      << "\",\"external_hierarchy\":\""
      << json_escape(report.k2_to_k1_external_hierarchy_digest)
      << "\"},\"required_logical_output_entries\":"
      << report.k2_to_k1_required_logical_output_entry_count;
  const auto& k2_to_k1_counters =
      report.k2_to_k1_target_authority_counters;
  std::cout
      << ",\"counters\":{\"forest_node_scans\":"
      << k2_to_k1_counters.forest_node_scan_count
      << ",\"forest_child_reference_scans\":"
      << k2_to_k1_counters.forest_child_reference_scan_count
      << ",\"source_batch_scans\":"
      << k2_to_k1_counters.source_batch_scan_count
      << ",\"source_binding_scans\":"
      << k2_to_k1_counters.source_binding_scan_count
      << ",\"label_resolution_scans\":"
      << k2_to_k1_counters.label_resolution_scan_count
      << ",\"group_check_scans\":"
      << k2_to_k1_counters.group_check_scan_count
      << ",\"observed_k2_k1_labels\":"
      << k2_to_k1_counters.observed_k2_k1_label_count
      << ",\"resolved_k2_k1_labels\":"
      << k2_to_k1_counters.resolved_k2_k1_label_count
      << ",\"gamma_cut_builds\":"
      << k2_to_k1_counters.gamma_cut_build_count
      << ",\"gamma_active_root_scans\":"
      << k2_to_k1_counters.gamma_active_root_scan_count
      << ",\"gamma_facet_scans\":"
      << k2_to_k1_counters.gamma_facet_scan_count
      << ",\"external_checkpoint_scans\":"
      << k2_to_k1_counters.external_checkpoint_scan_count
      << ",\"direct_target_point_references\":"
      << k2_to_k1_counters.direct_target_point_reference_count
      << ",\"external_target_point_references\":"
      << k2_to_k1_counters.external_target_point_reference_count
      << ",\"certified_target_coverage_equalities\":"
      << k2_to_k1_counters.certified_target_coverage_equality_count
      << "},\"rejected_label_resolution_index\":";
  if (report.k2_to_k1_rejected_label_resolution_index.has_value()) {
    std::cout << *report.k2_to_k1_rejected_label_resolution_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"rejected_atomic_group_index\":";
  if (report.k2_to_k1_rejected_atomic_group_index.has_value()) {
    std::cout << *report.k2_to_k1_rejected_atomic_group_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"rejected_source_binding_index\":";
  if (report.k2_to_k1_rejected_source_binding_index.has_value()) {
    std::cout << *report.k2_to_k1_rejected_source_binding_index;
  } else {
    std::cout << "null";
  }
  const auto& k2_to_k1_verification =
      report.k2_to_k1_target_authority_verification;
  std::cout
      << ",\"direct_pipeline_freshly_replayed\":"
      << boolean(report.k2_to_k1_direct_pipeline_freshly_replayed)
      << ",\"direct_vertical_journal_freshly_replayed\":"
      << boolean(
             report.k2_to_k1_direct_vertical_journal_freshly_replayed)
      << ",\"external_k2_k1_hierarchy_freshly_replayed\":"
      << boolean(report.k2_to_k1_external_hierarchy_freshly_replayed)
      << ",\"canonical_point_namespace_identity_certified\":"
      << boolean(
             report.k2_to_k1_canonical_point_namespace_identity_certified)
      << ",\"all_observed_k2_k1_labels_present\":"
      << boolean(report.k2_to_k1_all_observed_labels_present)
      << ",\"all_observed_k2_k1_labels_resolved\":"
      << boolean(report.k2_to_k1_all_observed_labels_resolved)
      << ",\"every_direct_target_coverage_reconstructed_transiently\":"
      << boolean(
             report.k2_to_k1_every_direct_target_coverage_reconstructed)
      << ",\"every_external_target_coverage_reconstructed_transiently\":"
      << boolean(
             report.k2_to_k1_every_external_target_coverage_reconstructed)
      << ",\"every_observed_k2_k1_target_coverage_equal\":"
      << boolean(report.k2_to_k1_every_observed_target_coverage_equal)
      << ",\"k2_to_k1_observed_label_target_authority_replayed\":"
      << boolean(
             report.k2_to_k1_observed_label_target_authority_replayed)
      << ",\"verification\":{\"trusted_inputs_accepted\":"
      << boolean(k2_to_k1_verification.trusted_inputs_accepted)
      << ",\"direct_pipeline_freshly_replayed\":"
      << boolean(k2_to_k1_verification.direct_pipeline_freshly_replayed)
      << ",\"direct_vertical_journal_freshly_replayed\":"
      << boolean(
             k2_to_k1_verification
                 .direct_vertical_journal_freshly_replayed)
      << ",\"external_k2_k1_hierarchy_freshly_replayed\":"
      << boolean(
             k2_to_k1_verification
                 .external_k2_k1_hierarchy_freshly_replayed)
      << ",\"expected_result_freshly_reconstructed\":"
      << boolean(
             k2_to_k1_verification.expected_result_freshly_reconstructed)
      << ",\"observed_recursively_equal\":"
      << boolean(k2_to_k1_verification.observed_recursively_equal)
      << ",\"observed_storage_within_budget\":"
      << boolean(k2_to_k1_verification.observed_storage_within_budget)
      << ",\"result_certified\":"
      << boolean(k2_to_k1_verification.result_certified)
      << "},\"bidirectional_gamma_group_completeness_replayed\":"
      << boolean(
             report
                 .k2_to_k1_bidirectional_gamma_group_completeness_replayed)
      << ",\"silent_gamma_checkpoint_completeness_replayed\":"
      << boolean(
             report
                 .k2_to_k1_silent_gamma_checkpoint_completeness_replayed)
      << ",\"external_target_authority_replayed\":"
      << boolean(report.k2_to_k1_external_target_authority_replayed)
      << ",\"global_morse_obligation_replayed\":"
      << boolean(report.k2_to_k1_global_morse_obligation_replayed)
      << ",\"all_naturality_squares_replayed\":"
      << boolean(report.k2_to_k1_all_naturality_squares_replayed)
      << ",\"vertical_maps_complete\":"
      << boolean(report.k2_to_k1_vertical_maps_complete)
      << ",\"global_m1_claimed\":"
      << boolean(report.k2_to_k1_global_m1_claimed)
      << ",\"bounded_exhaustive_gamma_oracle_used\":"
      << boolean(report.k2_to_k1_bounded_exhaustive_gamma_oracle_used)
      << ",\"bounded_oracle_gamma_materialized_transiently\":"
      << boolean(report.k2_to_k1_bounded_exhaustive_gamma_oracle_used)
      << ",\"gamma_cells_or_global_cofaces_persisted\":"
      << boolean(
             report.k2_to_k1_gamma_cells_or_global_cofaces_persisted)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(report.k2_to_k1_higher_order_delaunay_materialized)
      << ",\"public_status_claimed\":"
      << boolean(report.k2_to_k1_public_status_claimed)
      << ",\"no_partial_scientific_payload_published_on_failure\":"
      << boolean(report.k2_to_k1_no_partial_scientific_payload_on_failure)
      << "},\n";

  const auto& m1_o5 = report.m1_o5_death_accounting;
  const auto& m1_o5_budget = m1_o5.requested_budget;
  const auto& m1_o5_counters = m1_o5.counters;
  const auto& m1_o5_verification =
      report.m1_o5_death_accounting_verification;
  std::cout
      << "  \"m1_o5_death_accounting\":{\"required\":"
      << boolean(report.m1_o5_death_accounting_required)
      << ",\"attempted\":"
      << boolean(report.m1_o5_death_accounting_attempted)
      << ",\"certified_bounded_conditional_m1_o5_accounting\":"
      << boolean(report.m1_o5_death_accounting_certified)
      << ",\"schema_version\":" << m1_o5.schema_version
      << ",\"backend\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::backend
      << "\",\"profile\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::profile
      << "\",\"mode\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::mode
      << "\",\"deployment_status\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::deployment_status
      << "\",\"public_status\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::public_status
      << "\",\"proof_basis\":\""
      << ExactDirectMorseM1O5DeathAccountingResult::proof_basis
      << "\",\"product_non_o5_modes_keep_accounting_dormant\":true"
      << ",\"decision\":" << static_cast<unsigned>(m1_o5.decision)
      << ",\"scope\":" << static_cast<unsigned>(m1_o5.scope)
      << ",\"requested_budget\":{"
      << "\"event_audits\":" << m1_o5_budget.maximum_event_audit_count
      << ",\"group_audits\":" << m1_o5_budget.maximum_group_audit_count
      << ",\"batch_audits\":" << m1_o5_budget.maximum_batch_audit_count
      << ",\"source_family_scans\":"
      << m1_o5_budget.maximum_source_family_scan_count
      << ",\"source_arm_seed_scans\":"
      << m1_o5_budget.maximum_source_arm_seed_scan_count
      << ",\"source_binding_scans\":"
      << m1_o5_budget.maximum_source_binding_scan_count
      << ",\"source_saddle_scans\":"
      << m1_o5_budget.maximum_source_saddle_scan_count
      << ",\"source_group_scans\":"
      << m1_o5_budget.maximum_source_group_scan_count
      << ",\"source_batch_scans\":"
      << m1_o5_budget.maximum_source_batch_scan_count
      << ",\"point_id_reference_scans\":"
      << m1_o5_budget.maximum_point_id_reference_scan_count
      << ",\"prior_root_reference_scans\":"
      << m1_o5_budget.maximum_prior_root_reference_scan_count
      << ",\"exact_level_comparisons\":"
      << m1_o5_budget.maximum_exact_level_comparison_count
      << ",\"logical_scratch_entries\":"
      << m1_o5_budget.maximum_logical_scratch_entry_count
      << ",\"logical_output_entries\":"
      << m1_o5_budget.maximum_logical_output_entry_count
      << "},\"receipt\":{\"point_count\":" << m1_o5.point_count
      << ",\"effective_maximum_order\":"
      << m1_o5.effective_maximum_order
      << ",\"source_canonical_cloud_digest\":\""
      << m1_o5.source_canonical_cloud_digest.to_lower_hex()
      << "\",\"event_audit_digest\":\""
      << m1_o5.event_audit_digest.to_lower_hex()
      << "\",\"group_audit_digest\":\""
      << m1_o5.group_audit_digest.to_lower_hex()
      << "\",\"batch_audit_digest\":\""
      << m1_o5.batch_audit_digest.to_lower_hex()
      << "\",\"required_logical_output_entries\":"
      << m1_o5.required_logical_output_entry_count
      << ",\"event_audits\":";
  emit_m1_o5_event_audits_json(m1_o5.event_audits);
  std::cout << ",\"group_audits\":";
  emit_m1_o5_group_audits_json(m1_o5.group_audits);
  std::cout << ",\"batch_audits\":";
  emit_m1_o5_batch_audits_json(m1_o5.batch_audits);
  std::cout
      << "},\"counters\":{\"event_audits\":"
      << m1_o5_counters.event_audit_count
      << ",\"group_audits\":" << m1_o5_counters.group_audit_count
      << ",\"batch_audits\":" << m1_o5_counters.batch_audit_count
      << ",\"source_family_scans\":"
      << m1_o5_counters.source_family_scan_count
      << ",\"source_arm_seed_scans\":"
      << m1_o5_counters.source_arm_seed_scan_count
      << ",\"source_binding_scans\":"
      << m1_o5_counters.source_binding_scan_count
      << ",\"source_saddle_scans\":"
      << m1_o5_counters.source_saddle_scan_count
      << ",\"source_group_scans\":"
      << m1_o5_counters.source_group_scan_count
      << ",\"source_batch_scans\":"
      << m1_o5_counters.source_batch_scan_count
      << ",\"point_id_reference_scans\":"
      << m1_o5_counters.point_id_reference_scan_count
      << ",\"prior_root_reference_scans\":"
      << m1_o5_counters.prior_root_reference_scan_count
      << ",\"exact_level_comparisons\":"
      << m1_o5_counters.exact_level_comparison_count
      << ",\"maximum_batch_carriers\":"
      << m1_o5_counters.maximum_batch_carrier_count
      << ",\"maximum_batch_saddles\":"
      << m1_o5_counters.maximum_batch_saddle_count
      << ",\"peak_logical_scratch_entries\":"
      << m1_o5_counters.peak_logical_scratch_entry_count
      << ",\"total_local_multiplicity_capacity\":"
      << m1_o5_counters.total_local_multiplicity_capacity
      << ",\"total_touched_distinct_prior_roots\":"
      << m1_o5_counters.total_touched_distinct_prior_root_count
      << ",\"total_positive_root_groups\":"
      << m1_o5_counters.total_positive_root_group_count
      << ",\"total_global_deaths\":"
      << m1_o5_counters.total_global_death_count
      << "},\"rejected_source_family_index\":";
  if (m1_o5.rejected_source_family_index.has_value()) {
    std::cout << *m1_o5.rejected_source_family_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"rejected_source_binding_index\":";
  if (m1_o5.rejected_source_binding_index.has_value()) {
    std::cout << *m1_o5.rejected_source_binding_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"rejected_atomic_group_index\":";
  if (m1_o5.rejected_atomic_group_index.has_value()) {
    std::cout << *m1_o5.rejected_atomic_group_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"rejected_source_batch_index\":";
  if (m1_o5.rejected_source_batch_index.has_value()) {
    std::cout << *m1_o5.rejected_source_batch_index;
  } else {
    std::cout << "null";
  }
  std::cout
      << ",\"budget_preflight_certified\":"
      << boolean(m1_o5.budget_preflight_certified)
      << ",\"source_event_journal_freshly_replayed_relative_to_facade\":"
      << boolean(
             m1_o5.source_event_journal_freshly_replayed_relative_to_facade)
      << ",\"source_seed_journal_freshly_replayed_relative_to_facade\":"
      << boolean(
             m1_o5.source_seed_journal_freshly_replayed_relative_to_facade)
      << ",\"source_forest_freshly_replayed_relative_to_facade\":"
      << boolean(m1_o5.source_forest_freshly_replayed_relative_to_facade)
      << ",\"conditional_on_caller_fresh_phase9_facade_replay\":"
      << boolean(m1_o5.conditional_on_caller_fresh_phase9_facade_replay)
      << ",\"every_index_one_event_has_complete_shell_arms\":"
      << boolean(m1_o5.every_index_one_event_has_complete_shell_arms)
      << ",\"every_arm_seed_point_id_and_exact_level_joined\":"
      << boolean(m1_o5.every_arm_seed_point_id_and_exact_level_joined)
      << ",\"same_prior_root_carriers_unioned_before_saddle_hyperedges\":"
      << boolean(
             m1_o5.same_prior_root_carriers_unioned_before_saddle_hyperedges)
      << ",\"every_saddle_hyperedge_including_latent_closed_transitively\":"
      << boolean(
             m1_o5.every_saddle_hyperedge_including_latent_closed_transitively)
      << ",\"simultaneous_carrier_quotient_reconstructed\":"
      << boolean(m1_o5.simultaneous_carrier_quotient_reconstructed)
      << ",\"quotient_partition_matches_forest_atomic_groups\":"
      << boolean(m1_o5.quotient_partition_matches_forest_atomic_groups)
      << ",\"group_root_counts_kinds_and_children_replayed\":"
      << boolean(m1_o5.group_root_counts_kinds_and_children_replayed)
      << ",\"group_death_counts_replayed\":"
      << boolean(m1_o5.group_death_counts_replayed)
      << ",\"batch_global_death_identity_replayed\":"
      << boolean(m1_o5.batch_global_death_identity_replayed)
      << ",\"local_index_one_multiplicity_bounds_replayed\":"
      << boolean(m1_o5.local_index_one_multiplicity_bounds_replayed)
      << ",\"m1_o5_combinatorial_accounting_replayed\":"
      << boolean(m1_o5.m1_o5_combinatorial_accounting_replayed)
      << ",\"verification\":{\"observed_storage_within_budget\":"
      << boolean(m1_o5_verification.observed_storage_within_budget)
      << ",\"source_event_journal_freshly_replayed\":"
      << boolean(m1_o5_verification.source_event_journal_freshly_replayed)
      << ",\"source_seed_journal_freshly_replayed\":"
      << boolean(m1_o5_verification.source_seed_journal_freshly_replayed)
      << ",\"source_forest_freshly_replayed\":"
      << boolean(m1_o5_verification.source_forest_freshly_replayed)
      << ",\"expected_result_freshly_reconstructed\":"
      << boolean(m1_o5_verification.expected_result_freshly_reconstructed)
      << ",\"observed_recursively_equal\":"
      << boolean(m1_o5_verification.observed_recursively_equal)
      << ",\"trusted_inputs_accepted\":"
      << boolean(m1_o5_verification.trusted_inputs_accepted)
      << ",\"result_certified\":"
      << boolean(m1_o5_verification.result_certified)
      << "},\"event_local_death_attribution_serialized\":"
      << boolean(m1_o5.event_local_death_attribution_serialized)
      << ",\"durable_carrier_root_or_node_ids_serialized\":"
      << boolean(m1_o5.durable_carrier_root_or_node_ids_serialized)
      << ",\"source_catalog_complete_for_full_pi0\":"
      << boolean(m1_o5.source_catalog_complete_for_full_pi0)
      << ",\"carrier_faithfulness_complete\":"
      << boolean(m1_o5.carrier_faithfulness_complete)
      << ",\"silent_gamma_group_completeness\":"
      << boolean(m1_o5.silent_gamma_group_completeness)
      << ",\"bidirectional_gamma_group_completeness\":"
      << boolean(m1_o5.bidirectional_gamma_group_completeness)
      << ",\"external_target_authority_replayed\":"
      << boolean(m1_o5.external_target_authority_replayed)
      << ",\"global_morse_obligation_replayed\":"
      << boolean(m1_o5.global_morse_obligation_replayed)
      << ",\"global_m1_claimed\":" << boolean(m1_o5.global_m1_claimed)
      << ",\"all_naturality_squares_replayed\":"
      << boolean(m1_o5.all_naturality_squares_replayed)
      << ",\"vertical_maps_complete\":"
      << boolean(m1_o5.vertical_maps_complete)
      << ",\"forest_semantics_exact\":"
      << boolean(m1_o5.forest_semantics_exact)
      << ",\"bounded_exhaustive_gamma_oracle_used\":"
      << boolean(m1_o5.bounded_exhaustive_gamma_oracle_used)
      << ",\"gamma_cells_or_global_cofaces_materialized\":"
      << boolean(m1_o5.gamma_cells_or_global_cofaces_materialized)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(m1_o5.higher_order_delaunay_materialized)
      << ",\"public_status_claimed\":"
      << boolean(m1_o5.public_status_claimed)
      << ",\"scalable_50k_claimed\":"
      << boolean(m1_o5.scalable_50k_claimed)
      << ",\"no_partial_scientific_payload_published_on_failure\":"
      << boolean(m1_o5.no_partial_scientific_payload_published_on_failure)
      << "},\n";

  const auto& conformance =
      report.direct_morse_gamma_carrier_conformance;
  const auto& conformance_budget = conformance.requested_budget;
  const auto& overlay_budget = conformance_budget.overlay_budget;
  const auto& history_budget =
      overlay_budget.reduced_gamma_history_budget;
  const auto& gamma_budget = history_budget.gamma_budget;
  const auto& carrier_cut_budget =
      conformance_budget.carrier_cut_budget;
  const auto& conformance_counters = conformance.counters;
  std::cout
      << "  \"direct_morse_gamma_carrier_conformance\":{\"required\":"
      << boolean(report.direct_morse_gamma_carrier_conformance_required)
      << ",\"attempted\":"
      << boolean(report.direct_morse_gamma_carrier_conformance_attempted)
      << ",\"certified_bounded_single_order_conformance\":"
      << boolean(report.direct_morse_gamma_carrier_conformance_certified)
      << ",\"schema_version\":" << conformance.schema_version
      << ",\"backend\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::backend
      << "\",\"profile\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::profile
      << "\",\"mode\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::mode
      << "\",\"deployment_status\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::deployment_status
      << "\",\"public_status\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::public_status
      << "\",\"proof_basis\":\""
      << ExactDirectMorseGammaCarrierConformanceResult::proof_basis
      << "\",\"decision\":"
      << static_cast<unsigned>(conformance.decision)
      << ",\"scope\":" << static_cast<unsigned>(conformance.scope)
      << ",\"builder_includes_fresh_verifier\":"
      << boolean(report.direct_morse_gamma_carrier_conformance_attempted)
      << ",\"external_verifier_replay_performed\":false"
      << ",\"timing_includes_embedded_fresh_verifier\":"
      << boolean(report.direct_morse_gamma_carrier_conformance_attempted)
      << ",\"requested_budget\":{\"overlay\":{"
         "\"critical_catalog\":{\"candidates\":"
      << overlay_budget.critical_catalog_budget.maximum_candidate_count
      << ",\"point_classifications\":"
      << overlay_budget.critical_catalog_budget
             .maximum_point_classification_count
      << "},\"reduced_gamma_history\":{\"gamma\":{"
         "\"facets\":"
      << gamma_budget.maximum_enumerated_facet_count
      << ",\"cofaces\":"
      << gamma_budget.maximum_enumerated_coface_count
      << ",\"union_attempts\":"
      << gamma_budget.maximum_union_attempt_count
      << "},\"activation_levels\":"
      << history_budget.maximum_activation_level_count
      << ",\"total_facet_work\":"
      << history_budget.maximum_total_facet_work_count
      << ",\"total_coface_work\":"
      << history_budget.maximum_total_coface_work_count
      << ",\"total_union_work\":"
      << history_budget.maximum_total_union_work_count
      << ",\"nodes\":" << history_budget.maximum_node_count
      << ",\"child_references\":"
      << history_budget.maximum_child_reference_count
      << ",\"group_root_references\":"
      << history_budget.maximum_group_root_reference_count
      << ",\"groups\":" << history_budget.maximum_group_count
      << ",\"group_newly_active_facets\":"
      << history_budget.maximum_group_newly_active_facet_count
      << ",\"group_equal_level_cofaces\":"
      << history_budget.maximum_group_equal_level_coface_count
      << ",\"delta_facets\":"
      << history_budget.maximum_delta_facet_count
      << ",\"delta_point_references\":"
      << history_budget.maximum_delta_point_reference_count
      << "},\"event_projections\":"
      << overlay_budget.maximum_event_projection_count
      << ",\"group_overlays\":"
      << overlay_budget.maximum_group_overlay_count
      << ",\"label_slots\":"
      << overlay_budget.maximum_label_slot_count
      << ",\"history_point_id_scans\":"
      << overlay_budget.maximum_history_point_id_scan_count
      << ",\"catalog_point_id_scans\":"
      << overlay_budget.maximum_catalog_point_id_scan_count
      << ",\"group_event_references\":"
      << overlay_budget.maximum_group_event_reference_count
      << "},\"carrier_cut\":{\"forest_birth_record_scans\":"
      << carrier_cut_budget.maximum_forest_birth_record_scan_count
      << ",\"forest_node_scans\":"
      << carrier_cut_budget.maximum_forest_node_scan_count
      << ",\"forest_batch_scans\":"
      << carrier_cut_budget.maximum_forest_batch_scan_count
      << ",\"forest_atomic_group_scans\":"
      << carrier_cut_budget.maximum_forest_atomic_group_scan_count
      << ",\"forest_saddle_scans\":"
      << carrier_cut_budget.maximum_forest_saddle_scan_count
      << ",\"forest_arm_binding_scans\":"
      << carrier_cut_budget.maximum_forest_arm_binding_scan_count
      << ",\"forest_child_reference_scans\":"
      << carrier_cut_budget.maximum_forest_child_reference_scan_count
      << ",\"forest_final_root_scans\":"
      << carrier_cut_budget.maximum_forest_final_root_scan_count
      << ",\"component_states\":"
      << carrier_cut_budget.maximum_component_state_count
      << ",\"node_marker_states\":"
      << carrier_cut_budget.maximum_node_marker_state_count
      << ",\"index_entries\":"
      << carrier_cut_budget.maximum_index_entry_count
      << ",\"group_carrier_scratch\":"
      << carrier_cut_budget.maximum_group_carrier_scratch_count
      << ",\"group_prior_root_scratch\":"
      << carrier_cut_budget.maximum_group_prior_root_scratch_count
      << ",\"parent_hops\":"
      << carrier_cut_budget.maximum_parent_hop_count
      << ",\"exact_level_comparisons\":"
      << carrier_cut_budget.maximum_exact_level_comparison_count
      << ",\"single_exact_level_integer_bits\":"
      << carrier_cut_budget.maximum_single_exact_level_integer_bit_count
      << ",\"logical_output_entries\":"
      << carrier_cut_budget.maximum_logical_output_entry_count
      << "},\"direct_role_scans\":"
      << conformance_budget.maximum_direct_role_scan_count
      << ",\"forest_birth_scans\":"
      << conformance_budget.maximum_forest_birth_scan_count
      << ",\"forest_saddle_scans\":"
      << conformance_budget.maximum_forest_saddle_scan_count
      << ",\"forest_atomic_group_scans\":"
      << conformance_budget.maximum_forest_atomic_group_scan_count
      << ",\"activation_levels\":"
      << conformance_budget.maximum_activation_level_count
      << ",\"gamma_transition_builds\":"
      << conformance_budget.maximum_gamma_transition_build_count
      << ",\"checkpoints\":"
      << conformance_budget.maximum_checkpoint_count
      << ",\"group_audits\":"
      << conformance_budget.maximum_group_audit_count
      << ",\"carrier_anchor_scans\":"
      << conformance_budget.maximum_carrier_anchor_scan_count
      << ",\"gamma_component_scans\":"
      << conformance_budget.maximum_gamma_component_scan_count
      << ",\"gamma_facet_scans\":"
      << conformance_budget.maximum_gamma_facet_scan_count
      << ",\"group_event_reference_scans\":"
      << conformance_budget.maximum_group_event_reference_scan_count
      << ",\"arm_binding_scans\":"
      << conformance_budget.maximum_arm_binding_scan_count
      << ",\"gamma_incidence_joins\":"
      << conformance_budget.maximum_gamma_incidence_join_count
      << ",\"carrier_cut_entry_lookups\":"
      << conformance_budget.maximum_carrier_cut_entry_lookup_count
      << ",\"group_component_memberships\":"
      << conformance_budget.maximum_group_component_membership_count
      << ",\"gamma_label_comparisons\":"
      << conformance_budget.maximum_gamma_label_comparison_count
      << ",\"logical_scratch_entries\":"
      << conformance_budget.maximum_logical_scratch_entry_count
      << ",\"logical_output_entries\":"
      << conformance_budget.maximum_logical_output_entry_count
      << "},\"receipt\":{\"point_count\":" << conformance.point_count
      << ",\"order\":" << conformance.order
      << ",\"exhaustive_facet_count\":"
      << conformance.exhaustive_facet_count
      << ",\"exhaustive_coface_count\":"
      << conformance.exhaustive_coface_count
      << ",\"required_direct_role_scan_capacity\":"
      << conformance.required_direct_role_scan_capacity
      << ",\"required_forest_birth_scan_capacity\":"
      << conformance.required_forest_birth_scan_capacity
      << ",\"required_forest_saddle_scan_capacity\":"
      << conformance.required_forest_saddle_scan_capacity
      << ",\"required_forest_atomic_group_scan_capacity\":"
      << conformance.required_forest_atomic_group_scan_capacity
      << ",\"required_activation_level_capacity\":"
      << conformance.required_activation_level_capacity
      << ",\"required_carrier_anchor_scan_capacity\":"
      << conformance.required_carrier_anchor_scan_capacity
      << ",\"required_gamma_component_scan_capacity\":"
      << conformance.required_gamma_component_scan_capacity
      << ",\"required_gamma_facet_scan_capacity\":"
      << conformance.required_gamma_facet_scan_capacity
      << ",\"required_group_event_reference_scan_capacity\":"
      << conformance.required_group_event_reference_scan_capacity
      << ",\"required_arm_binding_scan_capacity\":"
      << conformance.required_arm_binding_scan_capacity
      << ",\"required_gamma_incidence_join_capacity\":"
      << conformance.required_gamma_incidence_join_capacity
      << ",\"required_carrier_cut_entry_lookup_capacity\":"
      << conformance.required_carrier_cut_entry_lookup_capacity
      << ",\"required_group_component_membership_capacity\":"
      << conformance.required_group_component_membership_capacity
      << ",\"required_gamma_label_comparison_capacity\":"
      << conformance.required_gamma_label_comparison_capacity
      << ",\"required_logical_scratch_entries\":"
      << conformance.required_logical_scratch_entry_count
      << ",\"required_checkpoint_capacity\":"
      << conformance.required_checkpoint_capacity
      << ",\"required_group_audit_capacity\":"
      << conformance.required_group_audit_capacity
      << ",\"required_logical_output_entries\":"
      << conformance.required_logical_output_entry_count
      << ",\"checkpoints\":";
  emit_direct_morse_gamma_carrier_checkpoints_json(
      conformance.checkpoints);
  std::cout << ",\"group_audits\":";
  emit_direct_morse_gamma_group_audits_json(conformance.group_audits);
  std::cout
      << "},\"counters\":{\"preflights\":"
      << conformance_counters.preflight_count
      << ",\"direct_forest_verifications\":"
      << conformance_counters.direct_forest_verification_count
      << ",\"overlay_builds\":"
      << conformance_counters.overlay_build_count
      << ",\"direct_role_scans\":"
      << conformance_counters.direct_role_scan_count
      << ",\"direct_birth_roles\":"
      << conformance_counters.direct_birth_role_count
      << ",\"direct_saddle_roles\":"
      << conformance_counters.direct_saddle_role_count
      << ",\"forest_birth_scans\":"
      << conformance_counters.forest_birth_scan_count
      << ",\"forest_saddle_scans\":"
      << conformance_counters.forest_saddle_scan_count
      << ",\"forest_atomic_group_scans\":"
      << conformance_counters.forest_atomic_group_scan_count
      << ",\"group_event_reference_scans\":"
      << conformance_counters.group_event_reference_scan_count
      << ",\"activation_levels\":"
      << conformance_counters.activation_level_count
      << ",\"gamma_transition_builds\":"
      << conformance_counters.gamma_transition_build_count
      << ",\"carrier_cut_builds\":"
      << conformance_counters.carrier_cut_build_count
      << ",\"checkpoints\":"
      << conformance_counters.checkpoint_count
      << ",\"strict_partition_bijections\":"
      << conformance_counters.strict_partition_bijection_count
      << ",\"closed_partition_bijections\":"
      << conformance_counters.closed_partition_bijection_count
      << ",\"carrier_anchor_scans\":"
      << conformance_counters.carrier_anchor_scan_count
      << ",\"gamma_component_scans\":"
      << conformance_counters.gamma_component_scan_count
      << ",\"gamma_facet_scans\":"
      << conformance_counters.gamma_facet_scan_count
      << ",\"group_audits\":"
      << conformance_counters.group_audit_count
      << ",\"arm_binding_scans\":"
      << conformance_counters.arm_binding_scan_count
      << ",\"gamma_incidence_joins\":"
      << conformance_counters.gamma_incidence_join_count
      << ",\"carrier_cut_entry_lookups\":"
      << conformance_counters.carrier_cut_entry_lookup_count
      << ",\"group_component_memberships\":"
      << conformance_counters.group_component_membership_count
      << ",\"gamma_label_comparisons\":"
      << conformance_counters.gamma_label_comparison_count
      << ",\"maximum_logical_scratch_entries\":"
      << conformance_counters.maximum_logical_scratch_entry_count
      << ",\"direct_birth_groups\":"
      << conformance_counters.direct_birth_group_count
      << ",\"direct_saddle_groups\":"
      << conformance_counters.direct_saddle_group_count
      << ",\"mixed_direct_residual_groups\":"
      << conformance_counters.mixed_direct_residual_group_count
      << ",\"silent_residual_continuation_groups\":"
      << conformance_counters.silent_residual_continuation_group_count
      << ",\"silent_checkpoints\":"
      << conformance_counters.silent_checkpoint_count
      << "},\"scalar_preflight_certified\":"
      << boolean(conformance.scalar_preflight_certified)
      << ",\"source_forest_freshly_replayed_relative_to_facade\":"
      << boolean(
             conformance.source_forest_freshly_replayed_relative_to_facade)
      << ",\"critical_catalog_gamma_overlay_freshly_replayed\":"
      << boolean(
             conformance.critical_catalog_gamma_overlay_freshly_replayed)
      << ",\"direct_h0_catalog_roles_bidirectionally_reconciled\":"
      << boolean(
             conformance.direct_h0_catalog_roles_bidirectionally_reconciled)
      << ",\"direct_atomic_groups_bidirectionally_reconciled\":"
      << boolean(conformance.direct_atomic_groups_bidirectionally_reconciled)
      << ",\"direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components\":"
      << boolean(
             conformance
                 .direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components)
      << ",\"carrier_partition_bijective_at_every_strict_checkpoint\":"
      << boolean(
             conformance
                 .carrier_partition_bijective_at_every_strict_checkpoint)
      << ",\"carrier_partition_bijective_at_every_closed_checkpoint\":"
      << boolean(
             conformance
                 .carrier_partition_bijective_at_every_closed_checkpoint)
      << ",\"every_gamma_group_classified_once\":"
      << boolean(conformance.every_gamma_group_classified_once)
      << ",\"every_provenance_free_gamma_group_is_silent_continuation\":"
      << boolean(
             conformance
                 .every_provenance_free_gamma_group_is_silent_continuation)
      << ",\"every_silent_continuation_preserves_one_root_and_adds_zero_points\":"
      << boolean(
             conformance
                 .every_silent_continuation_preserves_one_root_and_adds_zero_points)
      << ",\"bounded_carrier_faithfulness_replayed\":"
      << boolean(conformance.bounded_carrier_faithfulness_replayed)
      << ",\"bounded_silent_gamma_group_completeness_replayed\":"
      << boolean(
             conformance.bounded_silent_gamma_group_completeness_replayed)
      << ",\"bounded_bidirectional_gamma_group_completeness_replayed\":"
      << boolean(
             conformance
                 .bounded_bidirectional_gamma_group_completeness_replayed)
      << ",\"bounded_exhaustive_gamma_oracle_used\":"
      << boolean(conformance.bounded_exhaustive_gamma_oracle_used)
      << ",\"product_sparse_silent_source_complete\":"
      << boolean(conformance.product_sparse_silent_source_complete)
      << ",\"global_morse_obligation_replayed\":"
      << boolean(conformance.global_morse_obligation_replayed)
      << ",\"global_m1_claimed\":"
      << boolean(conformance.global_m1_claimed)
      << ",\"all_naturality_squares_replayed\":"
      << boolean(conformance.all_naturality_squares_replayed)
      << ",\"vertical_maps_complete\":"
      << boolean(conformance.vertical_maps_complete)
      << ",\"forest_semantics_exact\":"
      << boolean(conformance.forest_semantics_exact)
      << ",\"scalable_50k_claimed\":"
      << boolean(conformance.scalable_50k_claimed)
      << ",\"gamma_cells_or_global_cofaces_persisted\":"
      << boolean(conformance.gamma_cells_or_global_cofaces_persisted)
      << ",\"higher_order_delaunay_materialized\":"
      << boolean(conformance.higher_order_delaunay_materialized)
      << ",\"public_status_claimed\":"
      << boolean(conformance.public_status_claimed)
      << ",\"no_partial_scientific_payload_published_on_failure\":"
      << boolean(
             conformance
                 .no_partial_scientific_payload_published_on_failure)
      << "},\n"
      << "  \"vertical_target_replay_diagnostic\":{\"attempted\":"
      << boolean(report.vertical_target_replay_diagnostic_attempted)
      << ",\"callback_invoked\":"
      << boolean(report.vertical_target_replay_diagnostic_callback_invoked)
      << ",\"advance_certified\":"
      << boolean(
             report.vertical_target_replay_diagnostic_advance_certified)
      << ",\"source_atomic_group_index\":";
  if (report.vertical_target_diagnostic_source_group_index.has_value()) {
    std::cout << *report.vertical_target_diagnostic_source_group_index;
  } else {
    std::cout << "null";
  }
  std::cout << ",\"source_batch_index\":";
  if (report.vertical_target_diagnostic_source_batch_index.has_value()) {
    std::cout << *report.vertical_target_diagnostic_source_batch_index;
  } else {
    std::cout << "null";
  }
  std::cout
      << ",\"source_order\":"
      << report.vertical_target_diagnostic_source_order
      << ",\"target_order\":"
      << report.vertical_target_diagnostic_target_order
      << ",\"closed_squared_level\":{\"numerator\":\""
      << json_escape(report.vertical_target_diagnostic_level_numerator)
      << "\",\"denominator\":\""
      << json_escape(report.vertical_target_diagnostic_level_denominator)
      << "\"},\"query_token\":"
      << report.vertical_target_diagnostic_query_token
      << ",\"plan_decision\":"
      << report.vertical_target_diagnostic_plan_decision
      << ",\"session_initialization_decision\":"
      << report.vertical_target_diagnostic_session_initialization_decision
      << ",\"advance_decision\":"
      << report.vertical_target_diagnostic_advance_decision
      << ",\"closure_adapter_decision\":"
      << report.vertical_target_diagnostic_closure_adapter_decision
      << ",\"inner_closure_decision\":"
      << report.vertical_target_diagnostic_inner_closure_decision
      << ",\"inner_closure_disposition\":"
      << report.vertical_target_diagnostic_inner_closure_disposition
      << ",\"representative_binding_indices\":";
  emit_size_vector_json(
      report.vertical_target_diagnostic_representative_binding_indices);
  std::cout << ",\"source_keys\":";
  emit_facet_key_vector_json(report.vertical_target_diagnostic_source_keys);
  std::cout << ",\"target_facet_indices\":";
  emit_size_vector_json(report.vertical_target_diagnostic_target_facet_indices);
  std::cout << ",\"canonical_distinct_target_keys\":";
  emit_facet_key_vector_json(report.vertical_target_diagnostic_target_keys);
  const auto& closure_counters =
      report.vertical_target_diagnostic_closure_counters;
  std::cout
      << ",\"closure_counters\":{\"input_seed_references\":"
      << closure_counters.input_seed_reference_count
      << ",\"processed_seed_references\":"
      << closure_counters.processed_seed_reference_count
      << ",\"distinct_seed_keys\":"
      << closure_counters.distinct_seed_key_count
      << ",\"duplicate_seed_key_references\":"
      << closure_counters.duplicate_seed_key_reference_count
      << ",\"interned_nodes\":"
      << closure_counters.interned_node_count
      << ",\"evaluated_step_sources\":"
      << closure_counters.evaluated_step_source_count
      << ",\"strict_edges\":"
      << closure_counters.strict_edge_count
      << ",\"terminal_nodes\":"
      << closure_counters.terminal_node_count
      << ",\"relative_positive_terminals\":"
      << closure_counters.relative_positive_terminal_count
      << ",\"unresolved_terminals\":"
      << closure_counters.unresolved_terminal_count
      << ",\"budget_terminals\":"
      << closure_counters.budget_terminal_count
      << ",\"source_positive_hits\":"
      << closure_counters.source_positive_hit_count
      << ",\"successor_positive_hits\":"
      << closure_counters.successor_positive_hit_count
      << ",\"memoized_seed_reuses\":"
      << closure_counters.memoized_seed_reuse_count
      << ",\"memoized_suffix_reuses\":"
      << closure_counters.memoized_suffix_reuse_count
      << ",\"diagnostic_strict_witness_without_edge\":"
      << closure_counters.diagnostic_strict_witness_without_edge_count
      << ",\"memo_slot_visits\":"
      << closure_counters.memo_slot_visit_count
      << ",\"memo_full_key_comparisons\":"
      << closure_counters.memo_full_key_comparison_count
      << ",\"equal_fingerprint_distinct_keys\":"
      << closure_counters.equal_fingerprint_distinct_key_count
      << ",\"locator_snapshot_checks\":"
      << closure_counters.locator_snapshot_check_count
      << ",\"distinct_cached_miniballs\":"
      << closure_counters.distinct_cached_miniball_count
      << ",\"source_miniball_builds\":"
      << closure_counters.source_miniball_build_count
      << ",\"source_miniball_reuses\":"
      << closure_counters.source_miniball_reuse_count
      << ",\"successor_miniball_builds\":"
      << closure_counters.successor_miniball_build_count
      << ",\"successor_miniball_reuses\":"
      << closure_counters.successor_miniball_reuse_count
      << "},\"contradiction_witness\":";
  if (report.vertical_target_diagnostic_contradiction_witness.has_value()) {
    const auto& witness =
        *report.vertical_target_diagnostic_contradiction_witness;
    std::cout << "{\"source_key\":";
    emit_facet_key_json(witness.source_facet_key);
    std::cout << ",\"target_key\":";
    emit_facet_key_json(witness.target_facet_key);
    std::cout
        << ",\"local_step_decision\":"
        << static_cast<unsigned>(witness.local_step_decision)
        << ",\"visiting_cycle_detected\":"
        << boolean(witness.visiting_cycle_detected)
        << ",\"shared_target_geometry_mismatch\":"
        << boolean(witness.shared_target_geometry_mismatch) << '}';
  } else {
    std::cout << "null";
  }
  std::cout
      << ",\"global_structure_materialized\":false,"
         "\"public_status_claimed\":false}\n"
      << "}\n";
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  return smoke_clouds::eight_clusters_points(options.point_count);
}

[[nodiscard]] bool deadline_due(
    const Options& options,
    Clock::time_point deadline) noexcept {
  return complete_resident_diagnostic(options) &&
      Clock::now() >= deadline;
}

[[nodiscard]] int emit_operational_deadline(
    Report& report,
    std::string_view stage,
    Clock::time_point total_start) {
  report.operational_deadline_reached = true;
  report.terminal_stage = std::string{stage};
  report.stop_category = "operational_deadline";
  report.stop_detail =
      "full_pipeline_diagnostic_right_censored_at_cooperative_boundary";
  report.timings.total_ms = milliseconds(Clock::now() - total_start);
  emit_report(report);
  return 6;
}

// R2-c: the higher-stage progress ventilation, published on EVERY outcome
// and by BOTH backends.  A censored or rejected run is exactly the case
// where these numbers matter most -- how much of C(n,3)+C(n,4) was decided,
// how many minimal terminals were drained, how much closed-ball work the
// host paid -- and before this the runner published none of them.  Filling
// it from the host fixed-chunk session too is what makes the two
// verification bases comparable inside a single report.  Nothing here is a
// scientific claim: a censored run still carries its censure and mints no
// certificate.
void publish_higher_progress_audit(
    Report& report,
    const ExactHigherSupportStreamAudit& progress,
    std::size_t frontier_entry_count) {
  report.higher_total_support_mass =
      morsehgp3d::exact::canonical_integer_string(
          progress.total_support_count);
  report.higher_resolved_support_mass =
      morsehgp3d::exact::canonical_integer_string(
          progress.resolved_support_count);
  report.higher_remaining_frontier_support_mass =
      morsehgp3d::exact::canonical_integer_string(
          progress.remaining_frontier_support_count);
  report.higher_minimal_leaf_count = progress.minimal_leaf_count;
  report.higher_above_rank_leaf_count = progress.above_rank_leaf_count;
  report.higher_leaf_support_analyses =
      progress.leaf_support_analysis_count;
  report.higher_point_classifications = progress.point_classification_count;
  report.higher_closed_ball_node_visits =
      progress.closed_ball_node_visit_count;
  report.higher_frontier_entries = frontier_entry_count;
}

#if MORSEHGP3D_RUNNER_HAS_DEVICE_TILED_HIGHER
void publish_device_tiled_assembly_progress(
    Report& report,
    const morsehgp3d::gpu::HigherSupportDeviceTiledStreamAssembly&
        assembly) {
  report.higher_assembly_censure = std::string{
      morsehgp3d::gpu::higher_support_device_tiled_assembly_censure_text(
          assembly.censure)};
  publish_higher_progress_audit(
      report,
      assembly.progress_audit,
      assembly.progress_frontier_entry_count);
  // The transaction split is the bridge's own accounting, and it is the
  // number R2 needs: expansion transitions are pure host commits, tile
  // transactions are the only ones that reach the device.
  report.higher_committed_transactions =
      assembly.bridge_audit.committed_transaction_count;
  report.higher_committed_tile_transactions =
      assembly.bridge_audit.committed_tile_transaction_count;
  report.higher_committed_expansion_transitions =
      assembly.bridge_audit.committed_expansion_transition_count;
  report.higher_committed_tile_slots =
      assembly.bridge_audit.committed_tile_slot_count;
}
#endif

[[nodiscard]] morsehgp3d::contract::CanonicalId durable_digest_text(
    std::string_view text) {
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

bool append_segment_to_durable_archive(
    void* state,
    const ExactDirectMorseForestBatchSegment& segment) noexcept {
  auto* writer =
      static_cast<ExactDirectMorseForestRunArchiveWriter*>(state);
  return writer->append_segment(segment) ==
         ExactDirectMorseForestRunArchiveDecision::
             complete_atomic_run_archive;
}

struct DurableReloadProbe {
  std::size_t segment_count{};
  bool structure_certified{true};
};

bool count_reloaded_durable_segment(
    void* state,
    const ExactDirectMorseForestBatchSegment& segment) noexcept {
  auto* probe = static_cast<DurableReloadProbe*>(state);
  probe->structure_certified =
      probe->structure_certified && segment.certified_structure();
  ++probe->segment_count;
  return true;
}

[[nodiscard]] int run(const Options& options) {
  Report report = make_report(options);
  const Clock::time_point total_start = Clock::now();
  const Clock::time_point operational_deadline =
      total_start + std::chrono::milliseconds{
                        static_cast<std::chrono::milliseconds::rep>(
                            options.operational_deadline_ms)};
  if (conditional_adjacent_event_vertical_propagation_qualification(
          options) &&
      (options.point_count < 2U ||
       options.maximum_order > options.point_count - 2U)) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "conditional_adjacent_event_vertical_propagation_requires_point_count_at_least_maximum_order_plus_2";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (options.maximum_order >= options.point_count) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "maximum_order_must_be_strictly_less_than_point_count";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (conditional_adjacent_event_vertical_propagation_qualification(
          options) &&
      options.maximum_order < 2U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "conditional_adjacent_event_vertical_propagation_requires_maximum_order_at_least_2";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_k2_to_k1_target_authority_qualification(options) &&
      options.point_count >
          ExactK2K1HierarchyBudget::maximum_supported_point_count) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_k2_k1_target_authority_point_count_exceeds_14";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_k2_to_k1_target_authority_qualification(options) &&
      options.maximum_order < 2U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_k2_k1_target_authority_requires_maximum_order_at_least_2";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_m1_o5_death_accounting_qualification(options) &&
      options.point_count > 14U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_m1_o5_death_accounting_point_count_exceeds_14";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_m1_o5_death_accounting_qualification(options) &&
      options.maximum_order < 2U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_m1_o5_death_accounting_requires_maximum_order_at_least_2";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_direct_gamma_carrier_conformance_qualification(options) &&
      options.point_count > 14U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_direct_gamma_carrier_conformance_point_count_exceeds_14";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (bounded_direct_gamma_carrier_conformance_qualification(options) &&
      options.maximum_order < 2U) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "bounded_direct_gamma_carrier_conformance_requires_maximum_order_at_least_2";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (options.point_count >
      direct_morse_interactive_resident_maximum_point_count) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "resident_timed_point_count_exceeds_50000";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }

  const Clock::time_point generation_start = Clock::now();
  const std::vector<CertifiedPoint3> input = generate_points(options);
  const Clock::time_point generation_end = Clock::now();
  report.timings.generation_ms =
      milliseconds(generation_end - generation_start);
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "point_generation", total_start);
  }

  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{input});
  report.canonical_point_count = cloud.size();
  const Clock::time_point canonicalization_end = Clock::now();
  report.timings.canonicalization_ms =
      milliseconds(canonicalization_end - generation_end);
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "canonicalization", total_start);
  }

  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const Clock::time_point lbvh_end = Clock::now();
  report.timings.lbvh_ms =
      milliseconds(lbvh_end - canonicalization_end);
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "lbvh", total_start);
  }

  const ExactMortonGroupedAnchoredPairScheduleConfig pair_schedule_config =
      make_sparse_pair_schedule_config();
  const std::size_t pair_maximum_closed_rank =
      make_sparse_pair_maximum_closed_rank(options);
  const ExactSparseAnchoredPairSessionAdvanceBudget pair_advance_budget =
      make_sparse_pair_advance_budget(options);
  const ExactSparseAnchoredPairSessionTotalCapacity pair_total_capacity =
      make_sparse_pair_total_capacity(options);
  const ExactHigherSupportStreamBudget higher_budget =
      make_higher_budget(options);
  const std::size_t effective_higher_chunk_limit =
      complete_resident_diagnostic(options)
          ? std::numeric_limits<std::size_t>::max()
          : options.higher_chunk_limit;
  report.effective_higher_chunk_limit = effective_higher_chunk_limit;
  report.pair_maximum_closed_rank = pair_maximum_closed_rank;
  report.pair_schedule_config = pair_schedule_config;
  report.pair_advance_budget = pair_advance_budget;
  report.pair_total_capacity = pair_total_capacity;
  report.pair_source_kind = "sparse_anchored_session";
  report.pair_authority_kind = "unsealed_sparse_anchored_session";
  report.effective_maximum_order = options.maximum_order;

  ExactSparseAnchoredPairSession pair_session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          pair_maximum_closed_rank,
          pair_schedule_config,
          pair_total_capacity);
  ExactSparseAnchoredPairSessionStopReason pair_terminal_stop_reason =
      ExactSparseAnchoredPairSessionStopReason::none;
  while (!pair_session.complete() &&
         !pair_session.total_capacity_exhausted() &&
         !pair_session.poisoned()) {
    if (complete_resident_diagnostic(options) &&
        Clock::now() >= operational_deadline) {
      report.operational_deadline_reached = true;
      break;
    }
    const ExactSparseAnchoredPairSessionStep step = pair_session.advance(
        index, cloud, pair_advance_budget);
    pair_terminal_stop_reason = step.stop_reason();
  }
  const Clock::time_point pair_end = Clock::now();
  report.timings.pair_support_ms =
      milliseconds(pair_end - lbvh_end);
  report.pair_stop_reason =
      report.operational_deadline_reached
          ? "operational_deadline"
          : pair_stop_reason_text(pair_terminal_stop_reason);
  const ExactSparseAnchoredPairSessionAudit pair_audit =
      pair_session.audit();
  const ExactMortonGroupedAnchoredPairCandidateAudit pair_candidate_audit =
      pair_session.candidate_audit();
  const ExactMortonGroupedAnchoredPairScheduleAudit pair_schedule_audit =
      pair_session.schedule_audit();
  report.pair_advance_calls = pair_audit.advance_call_count;
  report.pair_schedule_advances =
      pair_candidate_audit.schedule_advance_count;
  report.pair_orientation_checks =
      pair_candidate_audit.orientation_check_count;
  report.pair_reverse_or_self_orientation_skips =
      pair_candidate_audit.reverse_or_self_orientation_skip_count;
  report.pair_prepared_groups = pair_schedule_audit.prepared_group_count;
  report.pair_completed_groups = pair_schedule_audit.completed_group_count;
  report.pair_grouped_traversal_node_visits =
      pair_candidate_audit.grouped_traversal_node_visit_count;
  report.pair_witness_subtree_node_visits =
      pair_schedule_audit.witness_subtree_node_visit_count;
  report.pair_grouped_common_node_visits =
      pair_schedule_audit.common_traversal_node_visit_count;
  report.pair_anchor_subgroup_node_visits =
      pair_schedule_audit.anchor_subgroup_node_visit_count;
  report.pair_singleton_node_visits =
      pair_schedule_audit.singleton_node_visit_count;
  report.pair_grouped_witness_slots =
      pair_schedule_audit.witness_slot_scan_count;
  report.pair_grouped_inherited_witness_reuses =
      pair_schedule_audit.inherited_witness_reuse_count;
  report.pair_grouped_traversal_exact_predicates =
      pair_candidate_audit.grouped_traversal_exact_predicate_count;
  report.pair_fp64_filtered_negative_predicates =
      pair_schedule_audit.fp64_filtered_negative_predicate_count;
  report.pair_fp64_filtered_positive_predicates =
      pair_schedule_audit.fp64_filtered_positive_predicate_count;
  report.pair_exact_fallback_predicates =
      pair_schedule_audit.exact_fallback_predicate_count;
  report.pair_floating_witness_order_preparations =
      pair_schedule_audit.floating_witness_order_preparation_count;
  report.pair_floating_witness_score_evaluations =
      pair_schedule_audit.floating_witness_score_evaluation_count;
  report.pair_floating_witness_nonfinite_scores =
      pair_schedule_audit.floating_witness_nonfinite_score_count;
  report.pair_floating_witness_order_requested =
      pair_schedule_audit.floating_witness_order_requested;
  report.pair_floating_witness_order_effective_for_every_prepared_traversal =
      pair_schedule_audit
          .floating_witness_order_effective_for_every_prepared_traversal;
  report.pair_fp64_filter_partition_certified = three_sum_matches(
      report.pair_fp64_filtered_negative_predicates,
      report.pair_fp64_filtered_positive_predicates,
      report.pair_exact_fallback_predicates,
      report.pair_grouped_traversal_exact_predicates);
  report.pair_witness_subtree_exact_predicates =
      pair_schedule_audit.witness_subtree_exact_predicate_count;
  report.pair_witness_subtree_receipts =
      pair_schedule_audit.witness_subtree_receipt_count;
  report.pair_witness_subtree_successes =
      pair_schedule_audit.witness_subtree_success_count;
  report.pair_witness_subtree_fail_opens =
      pair_schedule_audit.witness_subtree_fail_open_count;
  report.pair_grouped_common_exact_predicates =
      pair_schedule_audit.common_exact_predicate_count;
  report.pair_anchor_subgroup_exact_predicates =
      pair_schedule_audit.anchor_subgroup_exact_predicate_count;
  report.pair_singleton_exact_predicates =
      pair_schedule_audit.singleton_exact_predicate_count;
  report.pair_grouped_strict_witness_discoveries =
      pair_schedule_audit.strict_witness_discovery_count;
  report.pair_grouped_diagonal_node_descents =
      pair_schedule_audit.diagonal_node_descent_count;
  report.pair_grouped_common_frontiers =
      pair_schedule_audit.common_frontier_count;
  report.pair_delegated_frontier_anchors =
      pair_schedule_audit.delegated_frontier_anchor_count;
  report.pair_prepared_anchor_subgroup_probes =
      pair_schedule_audit.prepared_anchor_subgroup_probe_count;
  report.pair_anchor_subgroup_witness_pool_entries =
      pair_schedule_audit.proposed_anchor_subgroup_witness_pool_entry_count;
  report.pair_query_facing_fallback_witness_pool_entries =
      pair_schedule_audit.query_facing_fallback_witness_pool_entry_count;
  report.pair_anchor_subgroup_splits =
      pair_schedule_audit.anchor_subgroup_split_count;
  report.pair_query_subtree_splits =
      pair_schedule_audit.query_subtree_split_count;
  report.pair_anchor_subgroup_certified_prunes =
      pair_schedule_audit.anchor_subgroup_certified_prune_count;
  report.pair_anchor_subgroup_certified_anchors =
      pair_schedule_audit.anchor_subgroup_certified_anchor_count;
  report.pair_prepared_singleton_fallbacks =
      pair_schedule_audit.prepared_singleton_fallback_count;
  report.pair_completed_singleton_fallbacks =
      pair_schedule_audit.completed_singleton_fallback_count;
  report.pair_singleton_witness_pool_entries =
      pair_schedule_audit.proposed_singleton_witness_pool_entry_count;
  report.pair_singleton_certified_prunes =
      pair_schedule_audit.singleton_certified_prune_count;
  report.pair_maximum_pending_anchor_subgroups =
      pair_schedule_audit.maximum_pending_anchor_subgroup_count;
  report.pair_triangular_block_pair_visits =
      pair_schedule_audit.triangular_block_pair_visit_count;
  report.pair_triangular_diagonal_splits =
      pair_schedule_audit.triangular_diagonal_split_count;
  report.pair_triangular_oversized_anchor_splits =
      pair_schedule_audit.triangular_oversized_anchor_split_count;
  report.pair_triangular_consumer_query_splits =
      pair_schedule_audit.triangular_consumer_query_split_count;
  report.pair_triangular_self_pairs =
      pair_schedule_audit.triangular_self_pair_count;
  report.pair_triangular_cross_blocks =
      pair_schedule_audit.triangular_cross_block_count;
  report.pair_triangular_certified_cross_blocks =
      pair_schedule_audit.triangular_certified_cross_block_count;
  report.pair_triangular_certified_unordered_pairs =
      pair_schedule_audit.triangular_certified_unordered_pair_count;
  report.pair_triangular_opened_singleton_cross_blocks =
      pair_schedule_audit.triangular_opened_singleton_cross_block_count;
  report.pair_triangular_opened_unordered_pairs =
      pair_schedule_audit.triangular_opened_unordered_pair_count;
  report.pair_triangular_maximum_pending_block_pairs =
      pair_schedule_audit.triangular_maximum_pending_block_pair_count;
  report.pair_triangular_partition_complete =
      pair_schedule_audit.triangular_partition_complete;
  report.pair_no_dynamic_dual_tree_or_pair_arena =
      pair_schedule_audit.no_dynamic_dual_tree_or_pair_arena_materialized;
  report.pair_authenticated_prunes =
      pair_audit.authenticated_prune_count;
  report.pair_authenticated_pruned_directed_pairs =
      pair_audit.authenticated_pruned_directed_pair_count;
  report.pair_directed_pair_universe =
      pair_audit.directed_pair_universe_size;
  report.pair_admitted_candidates =
      pair_audit.admitted_candidate_count;
  report.pair_classification_advances =
      pair_audit.classification_advance_count;
  report.pair_classification_node_visits =
      pair_audit.classification_node_visit_count;
  report.pair_classification_terminals =
      pair_audit.classification_terminal_count;
  report.pair_above_rank = pair_audit.above_rank_count;
  report.pair_output_records = pair_audit.emitted_record_count;
  report.pair_output_point_id_references =
      pair_audit.emitted_point_id_reference_count;
  report.pair_local_budget_exhaustions =
      pair_audit.budget_exhaustion_count;
  report.pair_total_capacity_exhaustions =
      pair_audit.total_capacity_exhaustion_count;
  report.pair_maximum_live_candidates =
      pair_audit.maximum_live_candidate_count;
  report.pair_accepted_events = pair_audit.accepted_event_count;
  report.pair_extra_shell_diagnostics =
      pair_audit.relevant_extra_shell_diagnostic_count;
  report.pair_directed_coverage_certified =
      pair_audit.directed_coverage_certified;
  report.pair_orientation_partition_certified =
      pair_audit.orientation_partition_certified;
  report.pair_classification_partition_certified =
      pair_audit.candidate_classification_partition_certified;
  report.pair_output_partition_certified =
      pair_audit.output_partition_certified;
  report.pair_records_certified =
      pair_audit.retained_records_certified;
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      pair_audit.no_forbidden_global_structure_materialized &&
      pair_candidate_audit.no_dynamic_candidate_or_output_arena_materialized &&
      pair_schedule_audit
          .no_global_anchor_pair_or_output_arena_materialized;
  if (!pair_session.complete()) {
    if (report.operational_deadline_reached) {
      report.pair_status = "operational_deadline";
      report.terminal_stage = "sparse_pair_session";
      report.stop_detail = "complete_attempt_operational_deadline_reached";
      report.stop_category = "operational_deadline";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 6;
    }
    const bool capacity_exhausted =
        pair_session.total_capacity_exhausted();
    report.pair_status = capacity_exhausted
        ? "total_capacity_exhausted"
        : "not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_detail = capacity_exhausted
        ? report.pair_stop_reason
        : "sparse_pair_session_not_terminal";
    report.budget_exhausted = capacity_exhausted;
    report.stop_category = capacity_exhausted
        ? "budget_exhausted"
        : "certification_failure";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return capacity_exhausted ? 2 : 3;
  }
  const bool triangular_pair_schedule =
      pair_schedule_config.use_triangular_block_pair_schedule;
  const bool pair_coverage_identity = triangular_pair_schedule
      ? checked_add(
            checked_add(
                pair_audit.authenticated_pruned_directed_pair_count,
                checked_multiply(
                    2U,
                    pair_audit.admitted_candidate_count,
                    "sparse triangular pair classification mass overflow"),
                "sparse triangular pair coverage overflow"),
            cloud.size(),
            "sparse triangular pair self mass overflow") ==
          pair_audit.directed_pair_universe_size
      : checked_add(
            pair_audit.authenticated_pruned_directed_pair_count,
            pair_candidate_audit.orientation_check_count,
            "sparse pair directed partition overflow") ==
          pair_audit.directed_pair_universe_size;
  const bool pair_orientation_identity = triangular_pair_schedule
      ? pair_audit.admitted_candidate_count ==
              pair_candidate_audit.orientation_check_count &&
          pair_candidate_audit.reverse_or_self_orientation_skip_count == 0U
      : checked_add(
            pair_audit.admitted_candidate_count,
            pair_candidate_audit.reverse_or_self_orientation_skip_count,
            "sparse pair orientation partition overflow") ==
          pair_candidate_audit.orientation_check_count;
  const bool pair_session_certified =
      !pair_session.poisoned() &&
      !pair_session.total_capacity_exhausted() &&
      pair_session.validated_for(index, cloud) &&
      pair_audit.every_prune_recertified &&
      pair_audit.directed_coverage_certified &&
      pair_audit.orientation_partition_certified &&
      pair_audit.candidate_classification_partition_certified &&
      pair_audit.output_partition_certified &&
      pair_audit.retained_records_certified &&
      pair_audit.no_forbidden_global_structure_materialized &&
      pair_candidate_audit.complete &&
      pair_candidate_audit.no_dynamic_candidate_or_output_arena_materialized &&
      pair_schedule_audit.complete &&
      pair_schedule_audit.morton_anchor_partition_complete &&
      pair_schedule_audit
          .no_global_anchor_pair_or_output_arena_materialized &&
      pair_coverage_identity && pair_orientation_identity &&
      pair_audit.admitted_candidate_count ==
          pair_audit.classification_terminal_count &&
      checked_add(
          pair_audit.above_rank_count,
          pair_audit.emitted_record_count,
          "sparse pair output partition overflow") ==
          pair_audit.classification_terminal_count &&
      checked_add(
          pair_audit.accepted_event_count,
          pair_audit.relevant_extra_shell_diagnostic_count,
          "sparse pair record partition overflow") ==
          pair_audit.emitted_record_count;
  if (!pair_session_certified) {
    report.pair_status = "session_not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_category = "certification_failure";
    report.stop_detail = "sparse_pair_session_invariants_failed";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  report.pair_status = "complete";
  report.pair_stop_reason = "none";
  ExactSparseAnchoredPairTerminalAuthority pair_authority =
      std::move(pair_session).seal();
  const bool pair_authority_certified =
      pair_authority.sealed_in_process_terminal_authority() &&
      pair_authority.bound_to(
          index,
          cloud,
          pair_maximum_closed_rank,
          pair_schedule_config,
          pair_total_capacity) &&
      pair_authority.maximum_closed_rank() == pair_maximum_closed_rank &&
      pair_authority.schedule_config() == pair_schedule_config &&
      pair_authority.total_capacity() == pair_total_capacity;
  if (!pair_authority_certified) {
    report.pair_status = "authority_not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_category = "certification_failure";
    report.stop_detail = "sparse_pair_authority_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  report.pair_source_kind = "sealed_sparse_anchored_session";
  report.pair_authority_kind = "sealed_in_process_terminal_authority";
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      pair_authority.audit().no_forbidden_global_structure_materialized;
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "sparse_pair_terminal_authority", total_start);
  }

  std::optional<ExactDirectSupportTerminalFacade> facade_storage;
  Clock::time_point higher_stage_end = pair_end;
  if (options.higher_backend == "device_tiled_session") {
#if MORSEHGP3D_RUNNER_HAS_DEVICE_TILED_HIGHER
    morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridgeConfig
        bridge_config;
    bridge_config.tile_certified_commit =
        options.higher_verification_basis == "tile_certified";
    // How much frontier the session pre-expands before dispatching a tile.
    // The device resolves up to slot_tile_capacity roots per launch, so a
    // target far below it leaves the engine starved -- which is exactly the
    // burst profile the c3 session measured.
    static_assert(
        runner_default_higher_tile_target ==
            morsehgp3d::gpu::
                higher_support_device_tiled_session_bridge_default_pre_expansion_target,
        "the runner tile-target default drifted from the sealed bridge one");
    static_assert(
        runner_maximum_higher_tile_target ==
            morsehgp3d::gpu::
                higher_support_device_tiled_frontier_maximum_slot_tile_capacity,
        "the runner tile-target bound drifted from the device slot capacity");
    bridge_config.pre_expansion_target_entry_count =
        options.higher_tile_target;
    // R2-c: the operational deadline now reaches INSIDE the assembly.
    // Before this, it was only tested at the stage boundaries, so a 50k
    // device-tiled run that did not converge returned nothing at all --
    // not even a ventilation.  The guard is cooperative, tested between
    // committed transitions only, and can never certify anything.
    morsehgp3d::gpu::HigherSupportDeviceTiledAssemblyOperationalGuard
        assembly_guard;
    // Armed on exactly the same condition as every other deadline test in
    // this runner: outside complete_resident_diagnostic the deadline field
    // is total_start and would censure the assembly instantly.
    if (complete_resident_diagnostic(options)) {
      assembly_guard.deadline = operational_deadline;
    }
    const auto assembly =
        morsehgp3d::gpu::assemble_exact_higher_support_stream_device_tiled(
            cloud, index, options.maximum_order, higher_budget,
            bridge_config, assembly_guard);
    higher_stage_end = Clock::now();
    report.timings.higher_support_ms =
        milliseconds(higher_stage_end - pair_end);
    publish_device_tiled_assembly_progress(report, assembly);
    if (!assembly.certified_assembled()) {
      if (assembly.censored()) {
        report.higher_status = "censored";
        report.higher_stop_reason =
            std::string{
                morsehgp3d::gpu::
                    higher_support_device_tiled_assembly_censure_text(
                        assembly.censure)};
        report.higher_authority_kind = "anchored_session_chain_certificate";
        return emit_operational_deadline(
            report, "higher_support_device_tiled_assembly", total_start);
      }
      report.higher_status = assembly.assembly_threw
          ? "assembly_threw"
          : (assembly.bridge_poisoned ? "bridge_poisoned"
                                      : "not_assembled");
      report.higher_stop_reason = "device_tiled_assembly_rejected";
      report.higher_authority_kind = "anchored_session_chain_certificate";
      report.terminal_stage = "higher_support";
      report.stop_category = "certification_failure";
      report.stop_detail = "device_tiled_higher_assembly_rejected";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }
    const ExactHigherSupportStreamAudit& device_audit =
        assembly.higher->audit;
    report.higher_work_units = device_audit.work_unit_count;
    report.higher_product_visits =
        device_audit.support_product_visit_count;
    report.higher_closed_ball_queries =
        device_audit.global_closed_ball_query_count;
    report.higher_accepted_events = device_audit.accepted_event_count;
    report.higher_extra_shell_diagnostics =
        device_audit.relevant_extra_shell_diagnostic_count;
    report.higher_prune_certificates =
        device_audit.emitted_prune_certificate_count;
    report.higher_chunk_count =
        assembly.certificate.committed_chunk_count();
    report.higher_status = "complete";
    report.higher_stop_reason = "none";
    report.higher_authority_kind = "anchored_session_chain_certificate";
    report.higher_verification_basis =
        assembly.certificate.verification_basis() ==
            morsehgp3d::hierarchy::ExactHigherSupportVerificationBasis::
                device_search_host_exact_record_classification_bigint_closure
        ? "device_search_host_exact_record_classification_bigint_closure"
        : "fresh_cpu_replay_every_commit";
    facade_storage.emplace(build_exact_direct_support_terminal_facade(
        index,
        cloud,
        options.maximum_order,
        higher_budget,
        std::move(pair_authority),
        *assembly.higher,
        assembly.certificate));
#else
    report.higher_status = "not_compiled";
    report.higher_stop_reason = "device_backend_not_compiled";
    report.higher_authority_kind = "anchored_session_chain_certificate";
    report.terminal_stage = "higher_support";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "device_tiled_higher_backend_requires_a_cuda_build";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
#endif
  } else {
  ExactHigherSupportTerminalSession higher_session{
      index,
      cloud,
      options.maximum_order,
      higher_budget,
      effective_higher_chunk_limit};
  ExactHigherSupportTerminalRunStatus higher_run_status =
      ExactHigherSupportTerminalRunStatus::maximum_chunk_count_reached;
  if (complete_resident_diagnostic(options)) {
    while (true) {
      if (higher_session.trusted_checkpoint().locally_complete()) {
        higher_run_status =
            ExactHigherSupportTerminalRunStatus::terminal;
        break;
      }
      if (deadline_due(options, operational_deadline)) {
        report.operational_deadline_reached = true;
        break;
      }
      const ExactHigherSupportResidentAdvanceStatus advance_status =
          higher_session.advance_one_resident_chunk();
      if (advance_status ==
          ExactHigherSupportResidentAdvanceStatus::
              maximum_chunk_count_reached) {
        break;
      }
    }
  } else {
    higher_run_status = higher_session.run_to_terminal();
  }
  const Clock::time_point higher_end = Clock::now();
  higher_stage_end = higher_end;
  report.timings.higher_support_ms =
      milliseconds(higher_end - pair_end);
  const ExactHigherSupportCheckpoint& higher_trusted =
      higher_session.trusted_checkpoint();
  const ExactHigherSupportStreamAudit& higher_audit =
      higher_trusted.cumulative_audit;
  report.higher_work_units = higher_audit.work_unit_count;
  report.higher_product_visits =
      higher_audit.support_product_visit_count;
  report.higher_closed_ball_queries =
      higher_audit.global_closed_ball_query_count;
  report.higher_accepted_events =
      higher_audit.accepted_event_count;
  report.higher_extra_shell_diagnostics =
      higher_audit.relevant_extra_shell_diagnostic_count;
  report.higher_prune_certificates =
      higher_audit.emitted_prune_certificate_count;
  report.higher_chunk_count = higher_session.chunk_count();
  // Same ventilation as the device path, from the host session's own
  // trusted checkpoint: the two verification bases become comparable in a
  // single report, which is what the R2 sweep reads.
  publish_higher_progress_audit(
      report, higher_audit, higher_trusted.frontier.size());

  if (report.operational_deadline_reached) {
    report.higher_status = "operational_deadline";
    report.higher_stop_reason = "operational_deadline";
    report.higher_authority_kind =
        "unsealed_root_anchored_fixed_chunk_session";
    return emit_operational_deadline(
        report, "higher_support", total_start);
  }

  if (higher_run_status !=
      ExactHigherSupportTerminalRunStatus::terminal) {
    report.higher_status = "budget_exhausted";
    report.higher_stop_reason = "maximum_chunk_count_reached";
    report.higher_authority_kind =
        "unsealed_root_anchored_fixed_chunk_session";
    report.terminal_stage = "higher_support";
    report.stop_detail = "higher_support_chunk_limit_reached";
    report.budget_exhausted = true;
    report.stop_category = "budget_exhausted";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  ExactHigherSupportTerminalAuthority higher_authority =
      std::move(higher_session).seal();
  report.higher_status = "complete";
  report.higher_stop_reason = "none";
  report.higher_authority_kind =
      "sealed_anchored_fixed_chunk_run";
  report.higher_verification_basis = "fresh_cpu_replay_every_commit";
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      higher_authority.no_forbidden_global_structure_materialized();

  facade_storage.emplace(build_exact_direct_support_terminal_facade(
      index,
      cloud,
      options.maximum_order,
      higher_budget,
      std::move(pair_authority),
      std::move(higher_authority)));
  }
  const ExactDirectSupportTerminalFacade& facade = *facade_storage;
  const Clock::time_point facade_end = Clock::now();
  report.timings.terminal_facade_ms =
      milliseconds(facade_end - higher_stage_end);
  report.terminal_catalog_certified =
      facade.terminal_catalog_certified();
  report.terminal_event_count = facade.events.size();
  report.terminal_extra_shell_diagnostic_count =
      facade.relevant_extra_shell_diagnostics.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      facade.certificate.no_forbidden_global_structure_materialized;
  if (!facade.terminal_catalog_certified()) {
    report.terminal_stage = "terminal_facade";
    report.stop_category = "certification_failure";
    report.stop_detail = "terminal_facade_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "terminal_facade", total_start);
  }

  const ExactDirectMorseEventJournalResult event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const Clock::time_point event_end = Clock::now();
  report.timings.event_journal_ms =
      milliseconds(event_end - facade_end);
  report.event_batch_count = event_journal.batches.size();
  report.event_role_count = event_journal.role_record_count;
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      event_journal.no_forbidden_global_structure_materialized;
  if (!event_journal.certified_partial_refinement()) {
    report.terminal_stage = "event_journal";
    report.stop_category = "certification_failure";
    report.stop_detail = "event_journal_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "event_journal", total_start);
  }

  ExactDirectSaddleArmSeedBudget seed_budget;
  try {
    seed_budget = make_seed_budget(
        cloud, facade, options.downstream_record_budget);
  } catch (const std::length_error& error) {
    report.terminal_stage = "saddle_seed_budget";
    report.stop_category = "budget_exhausted";
    report.stop_detail = error.what();
    report.budget_exhausted = true;
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  const ExactDirectSaddleArmSeedJournalResult seed_journal =
      build_exact_direct_saddle_arm_seed_journal(
          cloud, facade, event_journal, seed_budget);
  const Clock::time_point seed_end = Clock::now();
  report.timings.saddle_seed_journal_ms =
      milliseconds(seed_end - event_end);
  report.saddle_family_count = seed_journal.families.size();
  report.arm_seed_count = seed_journal.arm_seeds.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      seed_journal.no_forbidden_global_structure_materialized;
  if (!seed_journal.certified_partial_refinement()) {
    report.terminal_stage = "saddle_seed_journal";
    report.stop_category = "certification_failure";
    report.stop_detail = "saddle_seed_journal_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "saddle_seed_journal", total_start);
  }

  const ExactDirectMorseIndustrialPlanConfig industrial_config =
      make_industrial_config(options);
  const ExactDirectSparseFacetDescentBatchPlanBudget plan_budget =
      make_plan_budget(options, event_journal, seed_journal);
  const ExactDirectSparseFacetDescentBatchPlanResult plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          cloud,
          facade,
          event_journal,
          seed_budget,
          seed_journal,
          industrial_config,
          plan_budget);
  const Clock::time_point plan_end = Clock::now();
  report.timings.batch_plan_ms =
      milliseconds(plan_end - seed_end);
  report.plan_decision =
      static_cast<unsigned>(plan.decision);
  report.industrial_plan_decision =
      static_cast<unsigned>(
          plan.source_industrial_plan.decision);
  report.industrial_chunk_count =
      plan.source_industrial_plan.chunks.size();
  report.plan_lane_count = plan.lanes.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      !plan.forbidden_global_structure_materialized;
  if (!plan.complete_architecture_plan()) {
    report.terminal_stage = "batch_plan";
    report.stop_detail = "batch_plan_not_complete";
    report.budget_exhausted =
        plan.decision ==
            ExactDirectSparseFacetDescentBatchPlanDecision::
                no_plan_budget_exhausted ||
        industrial_plan_budget_failure(
            plan.source_industrial_plan.decision);
    report.stop_category =
        report.budget_exhausted
            ? "budget_exhausted"
            : "certification_failure";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return report.budget_exhausted ? 2 : 3;
  }
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "batch_plan", total_start);
  }

  ExactDirectMorseForestBudget forest_budget;
  try {
    forest_budget = make_forest_budget(
        options, cloud, event_journal, seed_journal);
  } catch (const std::length_error& error) {
    report.terminal_stage = "forest_budget";
    report.stop_category = "budget_exhausted";
    report.stop_detail = error.what();
    report.budget_exhausted = true;
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  ExactDirectMorseForestConfig forest_config;
  forest_config.locator_config.external_authority_id =
      locator_authority_id;
  const ExactDirectSparseFacetDescentBatchExecutionBudget
      execution_budget =
          make_execution_budget(options, seed_journal);

  const bool durable_mode = durable_archive_recertification(options);
  report.durable_archive_requested = durable_mode;
  const ExactDirectMorseForestSegmentLimits durable_segment_limits{
      forest_budget.maximum_birth_record_count,
      forest_budget.maximum_arm_root_binding_count,
      forest_budget.maximum_saddle_record_count,
      forest_budget.maximum_atomic_group_count,
      forest_budget.maximum_child_reference_count,
      forest_budget.maximum_node_count,
  };
  ExactDirectMorseForestSegmentCodecLimits durable_codec_limits;
  durable_codec_limits.segment_limits = durable_segment_limits;
  durable_codec_limits.maximum_final_root_count =
      forest_budget.maximum_final_root_count;
  ExactDirectMorseForestRunArchiveIdentity durable_identity;
  durable_identity.manifest_digest = morsehgp3d::contract::CanonicalId{};
  durable_identity.source_identity_digest = durable_digest_text(
      "morsehgp3d/phase15/durable-campaign/source/" + options.family +
      "/" + std::to_string(options.point_count) + "/" +
      std::to_string(options.maximum_order));
  durable_identity.source_higher_canonical_cloud_digest =
      facade.certificate.higher_canonical_cloud_digest;
  durable_identity.terminal_source_chain_digest = durable_digest_text(
      "morsehgp3d/phase15/durable-campaign/terminal-source-chain/" +
      options.family + "/" + std::to_string(options.point_count) + "/" +
      std::to_string(options.maximum_order));
  durable_identity.point_count = cloud.size();
  durable_identity.effective_maximum_order =
      event_journal.effective_maximum_order;
  const std::string durable_final_name{"morsehgp3d-run.mh3d"};
  std::optional<ExactDirectMorseForestRunArchiveWriter> durable_writer;
  if (durable_mode) {
    auto open_decision =
        ExactDirectMorseForestRunArchiveDecision::not_certified;
    durable_writer.emplace(
        ExactDirectMorseForestRunArchiveWriter::open_new(
            options.archive_directory,
            durable_final_name,
            durable_identity,
            durable_codec_limits,
            open_decision));
    if (!durable_writer->open()) {
      report.terminal_stage = "durable_archive_open";
      report.stop_category = "certification_failure";
      report.stop_detail = "durable_archive_writer_rejected";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }
  }
  auto make_reducer = [&]() {
    if (durable_mode) {
      return ExactDirectMorseForestReducer(
          cloud,
          facade,
          event_journal,
          seed_budget,
          seed_journal,
          forest_budget,
          forest_config,
          durable_segment_limits,
          morsehgp3d::contract::CanonicalId{});
    }
    return ExactDirectMorseForestReducer(
        cloud,
        facade,
        event_journal,
        seed_budget,
        seed_journal,
        forest_budget,
        forest_config);
  };
  ExactDirectMorseForestReducer reducer = make_reducer();
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      index,
      cloud,
      facade,
      event_journal,
      seed_budget,
      seed_journal,
      industrial_config,
      plan_budget,
      plan,
      reducer.strict_locator());

  const Clock::time_point reducer_start = Clock::now();
  report.timings.reducer_setup_ms =
      milliseconds(reducer_start - plan_end);
  while (!executor.complete()) {
    if (deadline_due(options, operational_deadline)) {
      report.timings.reducer_stream_ms =
          milliseconds(Clock::now() - reducer_start);
      return emit_operational_deadline(
          report, "reducer_stream", total_start);
    }
    const std::size_t batch_index =
        executor.next_source_batch_index();
    if (batch_index >= event_journal.batches.size()) {
      report.terminal_stage = "reducer_stream";
      report.stop_category = "certification_failure";
      report.stop_detail = "executor_cursor_out_of_range";
      break;
    }
    const std::uint64_t batch_token =
        checked_multiply(
            batch_index + 1U,
            3U,
            "locator replay token overflow");
    const ExactDirectSparseFacetWitness witness{
        locator_authority_id, batch_token};
    const ExactDirectSparseFacetTopKProposalTranscriptResult
        transcript = empty_proposal_transcript(
            batch_index,
            event_journal.batches[batch_index].squared_level,
            reducer.strict_locator());
    auto ticket =
        executor
            .prepare_next_sealed_with_top_k_proposal_transcript(
                witness,
                execution_budget,
                forest_budget.closure_budget,
                transcript);
    report.preparation_decision = static_cast<unsigned>(
        ticket.preparation().decision);
    report.batch_execution_decision = static_cast<unsigned>(
        ticket.preparation().batch_execution_decision);
    if (!ticket.prepared()) {
      report.terminal_stage = "reducer_preparation";
      report.budget_exhausted = batch_execution_budget_failure(
          ticket.preparation().batch_execution_decision);
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : "certification_failure";
      report.stop_detail = "exact_batch_preparation_rejected";
      break;
    }
    ++report.prepared_ticket_count;
    const ExactDirectMorseForestLiveCommitResult committed =
        reducer.fold_prepared_ticket(executor, std::move(ticket));
    report.live_commit_decision =
        static_cast<unsigned>(committed.decision);
    report.reducer_fold_decision = static_cast<unsigned>(
        committed.reducer_fold.decision);
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !committed.forbidden_global_structure_materialized;
    if (!committed.certified_live_commit()) {
      report.terminal_stage = "reducer_live_commit";
      report.budget_exhausted =
          committed.decision ==
              ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_executor_audit_capacity_exhausted ||
          committed.reducer_fold.decision ==
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_budget_exhausted;
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : "certification_failure";
      report.stop_detail = "live_commit_rejected";
      break;
    }
    ++report.committed_batch_count;
    if (durable_mode) {
      const auto drained = reducer.drain_pending_output_segment(
          {&*durable_writer, append_segment_to_durable_archive});
      if (!drained.certified_acknowledged()) {
        report.terminal_stage = "durable_segment_drain";
        report.stop_category = "certification_failure";
        report.stop_detail = "durable_segment_sink_rejected";
        break;
      }
      ++report.durable_segment_count;
    }
  }
  const Clock::time_point reducer_end = Clock::now();
  report.timings.reducer_stream_ms =
      milliseconds(reducer_end - reducer_start);
  if (!executor.complete() || !reducer.complete()) {
    if (report.terminal_stage == "not_started") {
      report.terminal_stage = "reducer_stream";
      report.stop_category = "certification_failure";
      report.stop_detail = "reducer_stream_incomplete";
    }
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  if (deadline_due(options, operational_deadline)) {
    return emit_operational_deadline(
        report, "reducer_stream", total_start);
  }

  if (durable_mode) {
    const Clock::time_point durable_finish_start = Clock::now();
    std::optional<ExactDirectMorseForestFinalSeal> seal;
    try {
      seal.emplace(reducer.finish_segmented());
    } catch (const std::logic_error& error) {
      report.terminal_stage = "durable_finish_segmented";
      report.stop_category = "certification_failure";
      report.stop_detail = error.what();
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }
    const auto published = durable_writer->finalize(*seal);
    const Clock::time_point durable_publish_end = Clock::now();
    report.timings.archive_finalize_ms =
        milliseconds(durable_publish_end - durable_finish_start);
    report.durable_archive_published = published.certified_published();
    if (!published.certified_published() ||
        published.appended_segment_count !=
            report.durable_segment_count) {
      report.terminal_stage = "durable_archive_finalize";
      report.stop_category = "certification_failure";
      report.stop_detail = "durable_archive_publication_rejected";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }
    report.durable_archive_digest_hex =
        published.archive_digest.to_lower_hex();

    const auto recovered =
        recover_exact_direct_morse_forest_run_archive_directory(
            options.archive_directory,
            durable_final_name,
            durable_identity,
            durable_codec_limits);
    DurableReloadProbe probe;
    const auto reloaded = load_exact_direct_morse_forest_run_archive(
        options.archive_directory,
        durable_final_name,
        durable_identity,
        durable_codec_limits,
        {&probe, count_reloaded_durable_segment});
    const Clock::time_point durable_reload_end = Clock::now();
    report.timings.archive_reload_ms =
        milliseconds(durable_reload_end - durable_publish_end);
    report.durable_reloaded_segment_count = probe.segment_count;
    report.durable_identity_recertified =
        recovered.certified_recovered() &&
        recovered.decision ==
            ExactDirectMorseForestRunArchiveRecoveryDecision::
                valid_final_present &&
        reloaded.certified_loaded() && probe.structure_certified &&
        probe.segment_count == report.durable_segment_count &&
        reloaded.final_seal == *seal &&
        reloaded.archive_digest == published.archive_digest;
    if (!report.durable_identity_recertified) {
      report.terminal_stage = "durable_archive_recertification";
      report.stop_category = "certification_failure";
      report.stop_detail = "durable_reload_identity_rejected";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }

    report.scientific_result_materialized = true;
    report.conditional_h0_candidate_certified =
        seal->conditional_exact_h0_only;
    report.forest_birth_count = seal->counters.birth_record_count;
    report.forest_saddle_count = seal->counters.saddle_record_count;
    report.forest_atomic_group_count =
        seal->counters.atomic_group_count;
    report.forest_node_count = seal->counters.node_count;
    report.forest_final_root_count = seal->counters.final_root_count;
    report.forest_logical_output_entry_count =
        seal->logical_output_entry_count;
    report.pipeline_complete = true;
    report.terminal_stage = "durable_archive_recertification";
    report.stop_category = "none";
    report.stop_detail = "none";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 0;
  }

  const Clock::time_point finish_start = Clock::now();
  std::optional<ExactDirectMorseForestJournalResult> forest;
  try {
    forest.emplace(reducer.finish());
  } catch (const std::logic_error& error) {
    const Clock::time_point finish_failure = Clock::now();
    report.timings.forest_finish_ms =
        milliseconds(finish_failure - finish_start);
    report.timings.total_ms =
        milliseconds(finish_failure - total_start);
    report.terminal_stage = "forest_finish";
    report.stop_category =
        std::string_view{error.what()} ==
                "a reducer final carrier partition is incomplete"
            ? "carrier_incomplete"
            : "certification_failure";
    report.stop_detail = error.what();
    emit_report(report);
    return 3;
  }
  const Clock::time_point finish_end = Clock::now();
  report.timings.forest_finish_ms =
      milliseconds(finish_end - finish_start);
  report.scientific_result_materialized = true;
  report.conditional_h0_candidate_certified =
      forest->certified_conditional_h0_candidate();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      !forest->forbidden_global_structure_materialized &&
      !forest->gamma_cells_or_global_cofaces_materialized &&
      !forest->higher_order_delaunay_materialized;
  report.forest_birth_count =
      forest->counters.birth_record_count;
  report.forest_materialized_birth_count =
      forest->birth_records.size();
  report.forest_saddle_count =
      forest->counters.saddle_record_count;
  report.forest_atomic_group_count =
      forest->counters.atomic_group_count;
  report.forest_node_count = forest->counters.node_count;
  report.forest_materialized_node_count = forest->nodes.size();
  report.forest_final_root_count =
      forest->counters.final_root_count;
  report.forest_logical_output_entry_count =
      forest->logical_output_entry_count;
  report.forest_aggregate_closure_node_count =
      forest->counters.aggregate_closure_node_count;
  report.forest_aggregate_closure_step_call_count =
      forest->counters.aggregate_closure_step_call_count;
  if (conditional_adjacent_event_vertical_propagation_qualification(
          options)) {
    report.terminal_stage =
        "conditional_adjacent_event_vertical_propagation";
    report.conditional_adjacent_event_vertical_propagation_attempted = true;
    if (!report.conditional_h0_candidate_certified ||
        !report.no_forbidden_global_structure_materialized) {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "conditional_adjacent_event_vertical_propagation_requires_certified_forest";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }

    ExactDirectMorseEventRankTowerLinkBudget link_budget;
    try {
      link_budget = make_event_rank_tower_link_budget(
          options, *forest);
    } catch (const std::length_error& error) {
      report.stop_category = "budget_exhausted";
      report.stop_detail = error.what();
      report.budget_exhausted = true;
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 2;
    }

    const Clock::time_point event_vertical_begin = Clock::now();
    report.event_rank_tower_link_journal.emplace(
        build_exact_direct_morse_event_rank_tower_link_journal(
            *forest, link_budget));
    const Clock::time_point event_link_end = Clock::now();
    report.timings.event_rank_tower_link_journal_ms =
        milliseconds(event_link_end - event_vertical_begin);

    ExactDirectMorseEventVerticalPropagationBudget propagation_budget;
    try {
      propagation_budget = make_event_vertical_propagation_budget(
          options, *forest, *report.event_rank_tower_link_journal);
    } catch (const std::length_error& error) {
      report.stop_category = "budget_exhausted";
      report.stop_detail = error.what();
      report.budget_exhausted = true;
      report.timings.conditional_adjacent_event_vertical_propagation_ms =
          milliseconds(Clock::now() - event_vertical_begin);
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 2;
    }

    report.event_vertical_propagation_journal.emplace(
        build_exact_direct_morse_event_vertical_propagation_journal(
            *forest,
            *report.event_rank_tower_link_journal,
            link_budget,
            propagation_budget));
    const Clock::time_point event_propagation_end = Clock::now();
    report.timings.event_vertical_propagation_journal_ms =
        milliseconds(event_propagation_end - event_link_end);

    report.event_vertical_propagation_fresh_verification =
        verify_exact_direct_morse_event_vertical_propagation_journal_from_fresh_forest(
            index,
            cloud,
            facade,
            event_journal,
            seed_budget,
            seed_journal,
            forest_budget,
            forest_config,
            morsehgp3d::spatial::LbvhTraversalOrder::near_first,
            *forest,
            link_budget,
            *report.event_rank_tower_link_journal,
            propagation_budget,
            *report.event_vertical_propagation_journal);
    const Clock::time_point event_verification_end = Clock::now();
    report.timings.event_vertical_propagation_fresh_verification_ms =
        milliseconds(event_verification_end - event_propagation_end);
    report.timings.conditional_adjacent_event_vertical_propagation_ms =
        milliseconds(event_verification_end - event_vertical_begin);

    const auto& links = *report.event_rank_tower_link_journal;
    const auto& propagation =
        *report.event_vertical_propagation_journal;
    const auto& strong =
        report.event_vertical_propagation_fresh_verification;
    report.conditional_adjacent_event_vertical_propagation_certified =
        links.certified_conditional_event_rank_tower_links() &&
        links.source_conditional_forest_certificate_replayed &&
        links.source_forest_structure_replayed &&
        links.every_rank_at_least_two_birth_linked_exactly_once &&
        links.same_source_event_projection_replayed &&
        links.adjacent_order_role_replayed &&
        links.exact_level_identity_replayed &&
        links
            .every_saddle_arm_terminal_is_strictly_earlier_lower_order_birth &&
        links.strict_pre_batch_roots_replayed &&
        links.atomic_post_batch_target_replayed &&
        links.all_present_terminal_maximum_rank_births_included &&
        links.lower_rank_links_are_composition_only &&
        links.output_csr_and_linear_in_source_events_and_arms &&
        !links.gamma_cells_or_global_cofaces_materialized &&
        !links.higher_order_delaunay_materialized &&
        !links.forbidden_global_structure_materialized &&
        !links.public_status_claimed &&
        propagation.certified_conditional_event_vertical_propagation() &&
        propagation
            .conditional_on_caller_fresh_upstream_forest_and_link_replay &&
        propagation.conditional_on_direct_forest_o3_o4 &&
        !propagation.gamma_totality_replayed &&
        !propagation.m1_reconstruction_claimed &&
        !propagation.gamma_cells_or_global_cofaces_materialized &&
        !propagation.higher_order_delaunay_materialized &&
        !propagation.forbidden_global_structure_materialized &&
        !propagation.public_status_claimed &&
        strong.source_link_fresh_forest_verification.result_certified &&
        strong.propagation_verification.result_certified &&
        strong.conditional_on_caller_fresh_phase9_facade_replay &&
        strong.result_certified;
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !links.gamma_cells_or_global_cofaces_materialized &&
        !links.higher_order_delaunay_materialized &&
        !links.forbidden_global_structure_materialized &&
        !propagation.gamma_cells_or_global_cofaces_materialized &&
        !propagation.higher_order_delaunay_materialized &&
        !propagation.forbidden_global_structure_materialized;
    report.pipeline_complete =
        report.conditional_adjacent_event_vertical_propagation_certified;
    if (report.pipeline_complete) {
      report.stop_category = "none";
      report.stop_detail = "none";
    } else if (
        links.decision ==
            ExactDirectMorseEventRankTowerLinkDecision::
                no_link_budget_exhausted ||
        propagation.decision ==
            ExactDirectMorseEventVerticalPropagationDecision::
                no_propagation_budget_exhausted) {
      report.budget_exhausted = true;
      report.stop_category = "budget_exhausted";
      report.stop_detail =
          "conditional_adjacent_event_vertical_propagation_budget_exhausted";
    } else if (
        links.decision ==
            ExactDirectMorseEventRankTowerLinkDecision::
                no_link_allocation_failed ||
        links.decision ==
            ExactDirectMorseEventRankTowerLinkDecision::
                no_link_capacity_overflow ||
        propagation.decision ==
            ExactDirectMorseEventVerticalPropagationDecision::
                no_propagation_allocation_failed ||
        propagation.decision ==
            ExactDirectMorseEventVerticalPropagationDecision::
                no_propagation_capacity_overflow) {
      report.stop_category = "operational_failure";
      report.stop_detail =
          "conditional_adjacent_event_vertical_propagation_operational_failure";
    } else if (!links.certified_conditional_event_rank_tower_links()) {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "conditional_adjacent_event_rank_tower_link_journal_rejected";
    } else if (
        !propagation
             .certified_conditional_event_vertical_propagation()) {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "conditional_adjacent_event_vertical_propagation_journal_rejected";
    } else {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "conditional_adjacent_event_vertical_propagation_fresh_verification_rejected";
    }
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    if (report.pipeline_complete) {
      return 0;
    }
    if (report.budget_exhausted) {
      return 2;
    }
    return report.stop_category == "operational_failure" ? 5 : 3;
  }
  if (bounded_direct_gamma_carrier_conformance_qualification(options)) {
    report.terminal_stage = "direct_morse_gamma_carrier_conformance";
    if (!report.conditional_h0_candidate_certified ||
        !report.no_forbidden_global_structure_materialized) {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "direct_morse_gamma_carrier_conformance_requires_certified_forest";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }

    const ExactDirectMorseGammaCarrierConformanceBudget
        conformance_budget =
            make_bounded_direct_gamma_carrier_conformance_budget(
                cloud.size(), options.maximum_order);
    const Clock::time_point conformance_begin = Clock::now();
    report.direct_morse_gamma_carrier_conformance_attempted = true;
    report.direct_morse_gamma_carrier_conformance =
        build_exact_direct_morse_gamma_carrier_conformance(
            index,
            cloud,
            options.maximum_order,
            facade,
            event_journal,
            seed_budget,
            seed_journal,
            forest_budget,
            forest_config,
            morsehgp3d::spatial::LbvhTraversalOrder::near_first,
            *forest,
            conformance_budget);
    report.timings.direct_morse_gamma_carrier_conformance_ms =
        milliseconds(Clock::now() - conformance_begin);

    const auto& conformance =
        report.direct_morse_gamma_carrier_conformance;
    report.direct_morse_gamma_carrier_conformance_certified =
        conformance.certified_bounded_conformance() &&
        conformance.scalar_preflight_certified &&
        conformance.source_forest_freshly_replayed_relative_to_facade &&
        conformance.critical_catalog_gamma_overlay_freshly_replayed &&
        conformance.direct_h0_catalog_roles_bidirectionally_reconciled &&
        conformance.direct_atomic_groups_bidirectionally_reconciled &&
        conformance
            .direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components &&
        conformance
            .carrier_partition_bijective_at_every_strict_checkpoint &&
        conformance
            .carrier_partition_bijective_at_every_closed_checkpoint &&
        conformance.every_gamma_group_classified_once &&
        conformance
            .every_provenance_free_gamma_group_is_silent_continuation &&
        conformance
            .every_silent_continuation_preserves_one_root_and_adds_zero_points &&
        conformance.bounded_carrier_faithfulness_replayed &&
        conformance.bounded_silent_gamma_group_completeness_replayed &&
        conformance
            .bounded_bidirectional_gamma_group_completeness_replayed &&
        conformance.bounded_exhaustive_gamma_oracle_used &&
        !conformance.product_sparse_silent_source_complete &&
        !conformance.global_morse_obligation_replayed &&
        !conformance.global_m1_claimed &&
        !conformance.all_naturality_squares_replayed &&
        !conformance.vertical_maps_complete &&
        !conformance.forest_semantics_exact &&
        !conformance.scalable_50k_claimed &&
        !conformance.gamma_cells_or_global_cofaces_persisted &&
        !conformance.higher_order_delaunay_materialized &&
        !conformance.public_status_claimed &&
        conformance.no_partial_scientific_payload_published_on_failure;
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !conformance.gamma_cells_or_global_cofaces_persisted &&
        !conformance.higher_order_delaunay_materialized &&
        !conformance.public_status_claimed;
    report.pipeline_complete =
        report.direct_morse_gamma_carrier_conformance_certified;
    if (report.pipeline_complete) {
      report.stop_category = "none";
      report.stop_detail = "none";
    } else {
      report.budget_exhausted =
          conformance.decision ==
          ExactDirectMorseGammaCarrierConformanceDecision::
              no_conformance_budget_exhausted;
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : (conformance.decision ==
                         ExactDirectMorseGammaCarrierConformanceDecision::
                             no_conformance_allocation_failed
                     ? "operational_failure"
                     : "certification_failure");
      report.stop_detail =
          direct_morse_gamma_carrier_conformance_stop_detail(
              conformance.decision);
    }
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    if (report.pipeline_complete) {
      return 0;
    }
    if (report.budget_exhausted) {
      return 2;
    }
    return report.stop_category == "operational_failure" ? 5 : 3;
  }
  if (bounded_m1_o5_death_accounting_qualification(options)) {
    report.terminal_stage = "m1_o5_death_accounting";
    if (!report.conditional_h0_candidate_certified ||
        !report.no_forbidden_global_structure_materialized) {
      report.stop_category = "certification_failure";
      report.stop_detail =
          "m1_o5_death_accounting_requires_certified_forest";
      report.timings.total_ms =
          milliseconds(Clock::now() - total_start);
      emit_report(report);
      return 3;
    }

    const ExactDirectMorseM1O5DeathAccountingBudget accounting_budget =
        make_bounded_m1_o5_death_accounting_budget();
    const Clock::time_point accounting_begin = Clock::now();
    report.m1_o5_death_accounting_attempted = true;
    report.m1_o5_death_accounting =
        build_exact_direct_morse_m1_o5_death_accounting(
            index,
            cloud,
            facade,
            event_journal,
            seed_budget,
            seed_journal,
            forest_budget,
            forest_config,
            morsehgp3d::spatial::LbvhTraversalOrder::near_first,
            *forest,
            accounting_budget);
    const Clock::time_point accounting_build_end = Clock::now();
    report.timings.m1_o5_death_accounting_build_ms =
        milliseconds(accounting_build_end - accounting_begin);
    report.m1_o5_death_accounting_verification =
        verify_exact_direct_morse_m1_o5_death_accounting(
            index,
            cloud,
            facade,
            event_journal,
            seed_budget,
            seed_journal,
            forest_budget,
            forest_config,
            morsehgp3d::spatial::LbvhTraversalOrder::near_first,
            *forest,
            accounting_budget,
            report.m1_o5_death_accounting);
    const Clock::time_point accounting_verify_end = Clock::now();
    report.timings.m1_o5_death_accounting_verify_ms =
        milliseconds(accounting_verify_end - accounting_build_end);
    report.timings.m1_o5_death_accounting_ms =
        milliseconds(accounting_verify_end - accounting_begin);

    const auto& accounting = report.m1_o5_death_accounting;
    const auto& verification =
        report.m1_o5_death_accounting_verification;
    const auto digest_nonzero = [](const auto& digest) {
      const std::string text = digest.to_lower_hex();
      return std::any_of(
          text.begin(), text.end(), [](char character) {
            return character != '0';
          });
    };
    const bool event_identities_close = std::all_of(
        accounting.event_audits.begin(),
        accounting.event_audits.end(),
        [](const ExactDirectMorseM1O5EventAudit& event) {
          return event.support_point_count == event.arm_count &&
              event.arm_count >= 2U &&
              event.delta_one == event.arm_count - 1U;
        });
    const bool group_identities_close = std::all_of(
        accounting.group_audits.begin(),
        accounting.group_audits.end(),
        [](const ExactDirectMorseM1O5GroupAudit& group) {
          switch (group.kind) {
            case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
              return group.prior_reduced_root_count == 0U &&
                  group.death_count == 0U;
            case ExactDirectMorseForestAtomicGroupKind::continuation:
              return group.prior_reduced_root_count == 1U &&
                  group.death_count == 0U;
            case ExactDirectMorseForestAtomicGroupKind::multifusion:
              return group.prior_reduced_root_count >= 2U &&
                  group.death_count ==
                      group.prior_reduced_root_count - 1U;
          }
          return false;
        });
    std::size_t total_local_capacity = 0U;
    std::size_t total_touched_roots = 0U;
    std::size_t total_positive_groups = 0U;
    std::size_t total_deaths = 0U;
    bool batch_identities_close = true;
    for (const auto& batch : accounting.batch_audits) {
      batch_identities_close = batch_identities_close &&
          batch.global_death_count == batch.sum_group_death_count &&
          batch.global_death_count <= batch.local_multiplicity_capacity &&
          batch.touched_distinct_prior_root_count ==
              batch.global_death_count + batch.positive_root_group_count;
      total_local_capacity += batch.local_multiplicity_capacity;
      total_touched_roots += batch.touched_distinct_prior_root_count;
      total_positive_groups += batch.positive_root_group_count;
      total_deaths += batch.global_death_count;
    }
    const auto& counters = accounting.counters;
    const bool receipt_counts_close =
        counters.event_audit_count == accounting.event_audits.size() &&
        counters.group_audit_count == accounting.group_audits.size() &&
        counters.batch_audit_count == accounting.batch_audits.size() &&
        counters.total_local_multiplicity_capacity == total_local_capacity &&
        counters.total_touched_distinct_prior_root_count ==
            total_touched_roots &&
        counters.total_positive_root_group_count == total_positive_groups &&
        counters.total_global_death_count == total_deaths;
    const bool fresh_verifier =
        verification.observed_storage_within_budget &&
        verification.source_event_journal_freshly_replayed &&
        verification.source_seed_journal_freshly_replayed &&
        verification.source_forest_freshly_replayed &&
        verification.expected_result_freshly_reconstructed &&
        verification.observed_recursively_equal &&
        verification.trusted_inputs_accepted &&
        verification.result_certified;
    const bool local_positive_facts =
        accounting.budget_preflight_certified &&
        accounting
            .source_event_journal_freshly_replayed_relative_to_facade &&
        accounting
            .source_seed_journal_freshly_replayed_relative_to_facade &&
        accounting.source_forest_freshly_replayed_relative_to_facade &&
        accounting.conditional_on_caller_fresh_phase9_facade_replay &&
        accounting.every_index_one_event_has_complete_shell_arms &&
        accounting.every_arm_seed_point_id_and_exact_level_joined &&
        accounting
            .same_prior_root_carriers_unioned_before_saddle_hyperedges &&
        accounting
            .every_saddle_hyperedge_including_latent_closed_transitively &&
        accounting.simultaneous_carrier_quotient_reconstructed &&
        accounting.quotient_partition_matches_forest_atomic_groups &&
        accounting.group_root_counts_kinds_and_children_replayed &&
        accounting.group_death_counts_replayed &&
        accounting.batch_global_death_identity_replayed &&
        accounting.local_index_one_multiplicity_bounds_replayed &&
        accounting.m1_o5_combinatorial_accounting_replayed;
    const bool global_nonclaims_preserved =
        !accounting.event_local_death_attribution_serialized &&
        !accounting.durable_carrier_root_or_node_ids_serialized &&
        !accounting.source_catalog_complete_for_full_pi0 &&
        !accounting.carrier_faithfulness_complete &&
        !accounting.silent_gamma_group_completeness &&
        !accounting.bidirectional_gamma_group_completeness &&
        !accounting.external_target_authority_replayed &&
        !accounting.global_morse_obligation_replayed &&
        !accounting.global_m1_claimed &&
        !accounting.all_naturality_squares_replayed &&
        !accounting.vertical_maps_complete &&
        !accounting.forest_semantics_exact &&
        !accounting.bounded_exhaustive_gamma_oracle_used &&
        !accounting.gamma_cells_or_global_cofaces_materialized &&
        !accounting.higher_order_delaunay_materialized &&
        !accounting.public_status_claimed &&
        !accounting.scalable_50k_claimed;
    const bool nonempty_receipt =
        !accounting.event_audits.empty() &&
        !accounting.group_audits.empty() &&
        !accounting.batch_audits.empty() &&
        digest_nonzero(accounting.source_canonical_cloud_digest) &&
        digest_nonzero(accounting.event_audit_digest) &&
        digest_nonzero(accounting.group_audit_digest) &&
        digest_nonzero(accounting.batch_audit_digest);
    report.m1_o5_death_accounting_certified =
        accounting.certified_bounded_conditional_m1_o5_accounting() &&
        nonempty_receipt && receipt_counts_close && event_identities_close &&
        group_identities_close && batch_identities_close &&
        local_positive_facts && global_nonclaims_preserved && fresh_verifier;
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !accounting.gamma_cells_or_global_cofaces_materialized &&
        !accounting.higher_order_delaunay_materialized &&
        !accounting.public_status_claimed;
    report.pipeline_complete = report.m1_o5_death_accounting_certified;
    if (report.pipeline_complete) {
      report.stop_category = "none";
      report.stop_detail = "none";
    } else {
      report.budget_exhausted =
          accounting.decision ==
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_budget_exhausted;
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : (accounting.decision ==
                         ExactDirectMorseM1O5DeathAccountingDecision::
                             no_accounting_allocation_failed
                     ? "operational_failure"
                     : "certification_failure");
      report.stop_detail =
          m1_o5_death_accounting_stop_detail(accounting.decision);
    }
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    if (report.pipeline_complete) {
      return 0;
    }
    if (report.budget_exhausted) {
      return 2;
    }
    return report.stop_category == "operational_failure" ? 5 : 3;
  }

  report.vertical_target_source_group_count_by_order.assign(
      forest->effective_maximum_order + 1U, 0U);
  for (const auto& batch : forest->batches) {
    if (batch.order >= 2U &&
        batch.order <
            report.vertical_target_source_group_count_by_order.size()) {
      report.vertical_target_source_group_count_by_order[batch.order] +=
          batch.atomic_group_count;
    }
  }

  const ExactDirectMorseVerticalTargetProposalPipelineBudget
      vertical_target_budget =
          make_vertical_target_pipeline_budget(*forest);
  const Clock::time_point vertical_target_begin = Clock::now();
  report.vertical_target_pipeline_attempted = true;
  const ExactDirectMorseVerticalTargetProposalPipelineResult
      vertical_target_pipeline =
          build_exact_direct_morse_vertical_target_proposal_pipeline(
              *forest, index, cloud, vertical_target_budget);
  const Clock::time_point vertical_target_end = Clock::now();
  report.timings.vertical_target_pipeline_ms =
      milliseconds(vertical_target_end - vertical_target_begin);
  report.vertical_target_pipeline_certified =
      vertical_target_pipeline.certified_multiorder_target_proposals();
  report.vertical_target_pipeline_decision =
      static_cast<unsigned>(vertical_target_pipeline.decision);
  report.vertical_target_pipeline_counters =
      vertical_target_pipeline.counters;
  report.vertical_target_required_session_count =
      vertical_target_pipeline.required_referenced_target_order_count;
  report.vertical_target_required_group_count =
      vertical_target_pipeline.required_group_count;
  report.vertical_target_required_proposal_count =
      vertical_target_pipeline.required_proposal_count;
  report.vertical_target_rejected_source_group_index =
      vertical_target_pipeline.rejected_source_atomic_group_index;
  report.vertical_target_rejected_closure_adapter_decision =
      static_cast<unsigned>(
          vertical_target_pipeline.rejected_closure_decision);
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      !vertical_target_pipeline
           .global_facet_coface_incidence_cell_gamma_or_delaunay_materialized &&
      !vertical_target_pipeline.higher_order_delaunay_materialized &&
      !vertical_target_pipeline.public_status_claimed;
  if (!report.vertical_target_pipeline_certified) {
    diagnose_rejected_vertical_target_group(
        *forest,
        index,
        cloud,
        vertical_target_budget,
        vertical_target_pipeline,
        report);
  }

  const ExactDirectMorseVerticalBudget vertical_journal_budget =
      make_vertical_journal_budget();
  const ExactDirectMorseVerticalConfig vertical_journal_config{
      vertical_target_pipeline.external_target_authority_id};
  std::optional<ExactDirectMorseVerticalJournalResult> vertical_journal;
  if (report.vertical_target_pipeline_certified) {
    const Clock::time_point vertical_journal_begin = Clock::now();
    report.vertical_journal_attempted = true;
    vertical_journal.emplace(build_exact_direct_morse_vertical_journal(
        *forest,
        std::span<const ExactDirectMorseVerticalTargetProposal>{
            vertical_target_pipeline.proposals},
        vertical_journal_budget,
        vertical_journal_config));
    report.timings.vertical_journal_ms =
        milliseconds(Clock::now() - vertical_journal_begin);
    report.vertical_journal_decision =
        static_cast<unsigned>(vertical_journal->decision);
    report.vertical_journal_adjacent_family_count =
        vertical_journal->adjacent_families.size();
    report.vertical_journal_label_resolution_count =
        vertical_journal->label_resolutions.size();
    report.vertical_journal_group_check_count =
        vertical_journal->group_checks.size();
    report.vertical_journal_checkpoint_count =
        vertical_journal->checkpoints.size();
    report.vertical_journal_logical_output_entry_count =
        vertical_journal->logical_output_entry_count;
    report.vertical_journal_counters = vertical_journal->counters;
    report.vertical_journal_source_forest_shape_replayed =
        vertical_journal->source_forest_shape_replayed;
    report.vertical_journal_conditional_on_fresh_source_forest_replay =
        vertical_journal
            ->conditional_on_caller_fresh_source_forest_replay;
    report.vertical_journal_external_target_authority_replayed =
        vertical_journal->external_target_authority_replayed;
    report.vertical_journal_global_morse_obligation_replayed =
        vertical_journal->global_morse_obligation_replayed;
    report.vertical_journal_all_naturality_squares_replayed =
        vertical_journal->all_naturality_squares_replayed;
    report.vertical_journal_vertical_maps_complete =
        vertical_journal->vertical_maps_complete;
    report.vertical_journal_gamma_or_global_cofaces_materialized =
        vertical_journal->gamma_cells_or_global_cofaces_materialized;
    report.vertical_journal_higher_order_delaunay_materialized =
        vertical_journal->higher_order_delaunay_materialized;
    report.vertical_journal_public_status_claimed =
        vertical_journal->public_status_claimed;

    const auto& counters = vertical_journal->counters;
    const bool label_partition_matches_pipeline =
        counters.expected_label_count ==
            vertical_target_pipeline.proposals.size() &&
        counters.missing_label_count == 0U &&
        counters.unresolved_label_count <=
            counters.expected_label_count &&
        counters.resolved_label_count ==
            counters.expected_label_count -
                counters.unresolved_label_count;
    const bool group_partition_matches_pipeline =
        counters.complete_group_count <=
            vertical_target_pipeline.required_group_count &&
        counters.partial_group_count ==
            vertical_target_pipeline.required_group_count -
                counters.complete_group_count;
    report.vertical_journal_certified =
        vertical_journal->certified_conditional_vertical_candidate() &&
        label_partition_matches_pipeline &&
        group_partition_matches_pipeline &&
        vertical_journal->source_forest_shape_replayed &&
        vertical_journal
            ->conditional_on_caller_fresh_source_forest_replay &&
        !vertical_journal->external_target_authority_replayed &&
        !vertical_journal->global_morse_obligation_replayed &&
        !vertical_journal->all_naturality_squares_replayed &&
        !vertical_journal->vertical_maps_complete &&
        !vertical_journal->gamma_cells_or_global_cofaces_materialized &&
        !vertical_journal->higher_order_delaunay_materialized &&
        !vertical_journal->public_status_claimed;
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !vertical_journal->gamma_cells_or_global_cofaces_materialized &&
        !vertical_journal->higher_order_delaunay_materialized &&
        !vertical_journal->public_status_claimed;
  }
  report.resident_conditional_pipeline_complete =
      report.conditional_h0_candidate_certified &&
      report.no_forbidden_global_structure_materialized &&
      report.vertical_target_pipeline_certified &&
      report.vertical_journal_certified;
  std::optional<ExactDirectMorseK2K1TargetAuthorityResult>
      k2_to_k1_target_authority;
  if (report.resident_conditional_pipeline_complete &&
      report.k2_to_k1_target_authority_required &&
      vertical_journal.has_value()) {
    const Clock::time_point authority_begin = Clock::now();
    const ExactPersistentReducedGammaOrderHistoryBudget
        source_history_budget =
            make_bounded_gamma2_history_budget(cloud.size());
    const ExactPersistentReducedGammaOrderHistory external_source_history =
        build_exact_persistent_reduced_gamma_order_history(
            cloud, 2U, source_history_budget);
    const Clock::time_point source_history_end = Clock::now();
    report.timings.k2_to_k1_oracle_source_history_ms =
        milliseconds(source_history_end - authority_begin);
    report.k2_to_k1_oracle_source_batch_count =
        external_source_history.batch_metadata.size();
    report.k2_to_k1_oracle_source_group_count =
        external_source_history.group_records.size();
    report.k2_to_k1_oracle_source_node_count =
        external_source_history.nodes.size();

    const K1CompactForest external_k1_forest =
        build_compact_k1_forest(build_exact_complete_graph_emst(cloud));
    const Clock::time_point k1_end = Clock::now();
    report.timings.k2_to_k1_oracle_k1_ms =
        milliseconds(k1_end - source_history_end);
    report.k2_to_k1_oracle_k1_node_count =
        external_k1_forest.node_count();

    const ExactK2K1HierarchyBudget external_hierarchy_budget =
        make_bounded_k2_k1_hierarchy_budget();
    const ExactK2K1HierarchyResult external_hierarchy =
        build_exact_k2_k1_hierarchy(
            cloud,
            source_history_budget,
            external_source_history,
            external_k1_forest,
            external_hierarchy_budget);
    const Clock::time_point external_hierarchy_end = Clock::now();
    report.timings.k2_to_k1_oracle_hierarchy_ms =
        milliseconds(external_hierarchy_end - k1_end);
    report.k2_to_k1_oracle_external_checkpoint_count =
        external_hierarchy.vertical_checkpoints.size();

    const ExactDirectMorseK2K1TargetAuthorityBudget authority_budget =
        make_bounded_k2_k1_target_authority_budget();
    report.k2_to_k1_target_authority_attempted = true;
    k2_to_k1_target_authority.emplace(
        build_exact_direct_morse_k2_k1_target_authority(
            index,
            cloud,
            *forest,
            vertical_target_budget,
            vertical_target_pipeline,
            vertical_journal_budget,
            vertical_journal_config,
            *vertical_journal,
            source_history_budget,
            external_source_history,
            external_k1_forest,
            external_hierarchy_budget,
            external_hierarchy,
            authority_budget));
    const Clock::time_point authority_build_end = Clock::now();
    report.timings.k2_to_k1_target_authority_build_ms =
        milliseconds(authority_build_end - external_hierarchy_end);
    report.k2_to_k1_target_authority_verification =
        verify_exact_direct_morse_k2_k1_target_authority(
            index,
            cloud,
            *forest,
            vertical_target_budget,
            vertical_target_pipeline,
            vertical_journal_budget,
            vertical_journal_config,
            *vertical_journal,
            source_history_budget,
            external_source_history,
            external_k1_forest,
            external_hierarchy_budget,
            external_hierarchy,
            authority_budget,
            *k2_to_k1_target_authority);
    const Clock::time_point authority_verify_end = Clock::now();
    report.timings.k2_to_k1_target_authority_verify_ms =
        milliseconds(authority_verify_end - authority_build_end);
    report.timings.k2_to_k1_target_authority_ms =
        milliseconds(authority_verify_end - authority_begin);

    const auto& authority = *k2_to_k1_target_authority;
    report.k2_to_k1_target_authority_decision =
        static_cast<unsigned>(authority.decision);
    report.k2_to_k1_target_authority_scope =
        static_cast<unsigned>(authority.scope);
    report.k2_to_k1_direct_cloud_digest =
        authority.direct_canonical_cloud_digest.to_lower_hex();
    report.k2_to_k1_external_cloud_digest =
        authority.external_canonical_cloud_digest.to_lower_hex();
    report.k2_to_k1_external_hierarchy_digest =
        authority.external_hierarchy_projection_digest.to_lower_hex();
    report.k2_to_k1_required_logical_output_entry_count =
        authority.required_logical_output_entry_count;
    report.k2_to_k1_target_authority_counters = authority.counters;
    report.k2_to_k1_rejected_label_resolution_index =
        authority.rejected_label_resolution_index;
    report.k2_to_k1_rejected_atomic_group_index =
        authority.rejected_atomic_group_index;
    report.k2_to_k1_rejected_source_binding_index =
        authority.rejected_source_binding_index;
    report.k2_to_k1_direct_pipeline_freshly_replayed =
        authority.direct_pipeline_freshly_replayed;
    report.k2_to_k1_direct_vertical_journal_freshly_replayed =
        authority.direct_vertical_journal_freshly_replayed;
    report.k2_to_k1_external_hierarchy_freshly_replayed =
        authority.external_k2_k1_hierarchy_freshly_replayed;
    report.k2_to_k1_canonical_point_namespace_identity_certified =
        authority.canonical_point_namespace_identity_certified;
    report.k2_to_k1_all_observed_labels_present =
        authority.all_observed_k2_k1_labels_present;
    report.k2_to_k1_all_observed_labels_resolved =
        authority.all_observed_k2_k1_labels_resolved;
    report.k2_to_k1_every_direct_target_coverage_reconstructed =
        authority.every_direct_target_coverage_reconstructed_transiently;
    report.k2_to_k1_every_external_target_coverage_reconstructed =
        authority.every_external_target_coverage_reconstructed_transiently;
    report.k2_to_k1_every_observed_target_coverage_equal =
        authority.every_observed_k2_k1_target_coverage_equal;
    report.k2_to_k1_observed_label_target_authority_replayed =
        authority.k2_to_k1_observed_label_target_authority_replayed;
    report.k2_to_k1_bidirectional_gamma_group_completeness_replayed =
        authority.bidirectional_gamma_group_completeness_replayed;
    report.k2_to_k1_silent_gamma_checkpoint_completeness_replayed =
        authority.silent_gamma_checkpoint_completeness_replayed;
    report.k2_to_k1_external_target_authority_replayed =
        authority.external_target_authority_replayed;
    report.k2_to_k1_global_morse_obligation_replayed =
        authority.global_morse_obligation_replayed;
    report.k2_to_k1_all_naturality_squares_replayed =
        authority.all_naturality_squares_replayed;
    report.k2_to_k1_vertical_maps_complete =
        authority.vertical_maps_complete;
    report.k2_to_k1_global_m1_claimed = authority.global_m1_claimed;
    report.k2_to_k1_bounded_exhaustive_gamma_oracle_used =
        authority.bounded_exhaustive_gamma_oracle_used;
    report.k2_to_k1_gamma_cells_or_global_cofaces_persisted =
        authority.gamma_cells_or_global_cofaces_persisted;
    report.k2_to_k1_higher_order_delaunay_materialized =
        authority.higher_order_delaunay_materialized;
    report.k2_to_k1_public_status_claimed =
        authority.public_status_claimed;
    report.k2_to_k1_no_partial_scientific_payload_on_failure =
        authority.no_partial_scientific_payload_published_on_failure;

    const auto& authority_counters = authority.counters;
    const auto& verification =
        report.k2_to_k1_target_authority_verification;
    report.k2_to_k1_target_authority_certified =
        authority.certified_observed_label_target_authority() &&
        verification.result_certified &&
        verification.expected_result_freshly_reconstructed &&
        verification.observed_recursively_equal &&
        authority_counters.observed_k2_k1_label_count > 0U &&
        authority_counters.resolved_k2_k1_label_count ==
            authority_counters.observed_k2_k1_label_count &&
        authority_counters.certified_target_coverage_equality_count ==
            authority_counters.resolved_k2_k1_label_count &&
        authority.k2_to_k1_observed_label_target_authority_replayed &&
        !authority.bidirectional_gamma_group_completeness_replayed &&
        !authority.silent_gamma_checkpoint_completeness_replayed &&
        !authority.external_target_authority_replayed &&
        !authority.global_morse_obligation_replayed &&
        !authority.all_naturality_squares_replayed &&
        !authority.vertical_maps_complete && !authority.global_m1_claimed &&
        authority.bounded_exhaustive_gamma_oracle_used &&
        !authority.gamma_cells_or_global_cofaces_persisted &&
        !authority.higher_order_delaunay_materialized &&
        !authority.public_status_claimed &&
        authority.no_partial_scientific_payload_published_on_failure;
  }
  report.pipeline_complete =
      report.resident_conditional_pipeline_complete &&
      (!report.k2_to_k1_target_authority_required ||
       report.k2_to_k1_target_authority_certified);
  if (report.pipeline_complete) {
    report.terminal_stage =
        report.k2_to_k1_target_authority_required
            ? "k2_to_k1_target_authority"
            : "vertical_journal";
    report.stop_category = "none";
    report.stop_detail = "none";
  } else if (
      !report.no_forbidden_global_structure_materialized) {
    report.terminal_stage = "architecture_audit";
    report.stop_category = "architecture_violation";
    report.stop_detail =
        "forbidden_global_structure_materialized";
  } else if (!report.conditional_h0_candidate_certified) {
    report.terminal_stage = "forest_finish";
    report.stop_category = "certification_failure";
    report.stop_detail =
        "conditional_h0_candidate_not_certified";
  } else if (!report.vertical_target_pipeline_certified) {
    report.terminal_stage = "vertical_target_pipeline";
    report.budget_exhausted =
        vertical_target_pipeline.decision ==
        ExactDirectMorseVerticalTargetProposalPipelineDecision::
            no_pipeline_budget_exhausted;
    report.stop_category =
        report.budget_exhausted
            ? "budget_exhausted"
            : (vertical_target_pipeline.decision ==
                       ExactDirectMorseVerticalTargetProposalPipelineDecision::
                           no_pipeline_allocation_failed
                   ? "operational_failure"
                   : "certification_failure");
    report.stop_detail = vertical_target_pipeline_stop_detail(
        vertical_target_pipeline.decision);
  } else if (!report.vertical_journal_certified) {
    report.terminal_stage = "vertical_journal";
    report.budget_exhausted =
        vertical_journal.has_value() &&
        vertical_journal->decision ==
            ExactDirectMorseVerticalDecision::
                no_vertical_budget_exhausted;
    report.stop_category =
        report.budget_exhausted
            ? "budget_exhausted"
            : (vertical_journal.has_value() &&
                       vertical_journal->decision ==
                           ExactDirectMorseVerticalDecision::
                               no_vertical_allocation_failed
                   ? "operational_failure"
                   : "certification_failure");
    report.stop_detail = vertical_journal.has_value()
        ? vertical_journal_stop_detail(vertical_journal->decision)
        : "vertical_journal_not_attempted";
  } else {
    report.terminal_stage = "k2_to_k1_target_authority";
    report.budget_exhausted =
        k2_to_k1_target_authority.has_value() &&
        k2_to_k1_target_authority->decision ==
            ExactDirectMorseK2K1TargetAuthorityDecision::
                no_authority_budget_exhausted;
    report.stop_category =
        report.budget_exhausted
            ? "budget_exhausted"
            : (k2_to_k1_target_authority.has_value() &&
                       k2_to_k1_target_authority->decision ==
                           ExactDirectMorseK2K1TargetAuthorityDecision::
                               no_authority_allocation_failed
                   ? "operational_failure"
                   : "certification_failure");
    report.stop_detail = k2_to_k1_target_authority.has_value()
        ? k2_to_k1_target_authority_stop_detail(
              k2_to_k1_target_authority->decision)
        : "k2_to_k1_target_authority_not_attempted";
  }
  report.timings.total_ms = milliseconds(Clock::now() - total_start);
  emit_report(report);
  if (report.pipeline_complete) {
    return 0;
  }
  if (report.budget_exhausted) {
    return 2;
  }
  return report.stop_category == "operational_failure" ? 5 : 3;
}

}  // namespace

int main(int argc, char** argv) {
  const Clock::time_point process_start = Clock::now();
  Options options;
  bool options_parsed = false;
  try {
    parse_options(argc, argv, options);
    options_parsed = true;
    return run(options);
  } catch (const std::exception& error) {
    Report report = make_report(options);
    report.terminal_stage =
        options_parsed ? "operational_failure" : "input_parse";
    report.stop_category =
        options_parsed ? "operational_failure" : "invalid_input";
    report.stop_detail = error.what();
    report.architecture_audit_complete = false;
    report.no_forbidden_global_structure_materialized = false;
    report.timings.total_ms =
        milliseconds(Clock::now() - process_start);
    emit_report(report);
    return options_parsed ? 5 : 4;
  } catch (...) {
    Report report = make_report(options);
    report.terminal_stage =
        options_parsed ? "operational_failure" : "input_parse";
    report.stop_category =
        options_parsed ? "operational_failure" : "invalid_input";
    report.stop_detail = "unknown_nonstandard_exception";
    report.architecture_audit_complete = false;
    report.no_forbidden_global_structure_materialized = false;
    report.timings.total_ms =
        milliseconds(Clock::now() - process_start);
    emit_report(report);
    return options_parsed ? 5 : 4;
  }
}
