#include "morsehgp3d/hierarchy/direct_morse_k2_k1_target_authority.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::uint64_t authority_id = UINT64_C(0xE5A21);
int failures = 0;

static_assert(direct_morse_k2_k1_target_authority_schema_version == 1U);
static_assert(
    ExactDirectMorseK2K1TargetAuthorityResult::backend == "reference_cpu");
static_assert(
    ExactDirectMorseK2K1TargetAuthorityResult::profile == "hgp_reduced");

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Enum>
[[nodiscard]] unsigned enum_code(Enum value) {
  return static_cast<unsigned>(value);
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::size_t binomial(std::size_t n, std::size_t k) {
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

[[nodiscard]] ExactPairSupportStreamBudget source_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget source_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget source_seed_budget() {
  constexpr std::size_t capacity = 4096U;
  return {capacity, capacity, capacity, capacity};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
      512U,
      512U,
      4096U,
      512U,
      512U,
      512U,
      512U,
      512U,
      4096U,
      1025U,
      1025U};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{1024U, 64U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{1024U, 64U}};
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
descent_closure_budget() {
  return {512U, 512U, 512U, 1024U, step_budget()};
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 64U;
  budget.maximum_logical_output_entry_count = 16384U;
  budget.maximum_aggregate_closure_node_count = 16384U;
  budget.maximum_aggregate_closure_step_call_count = 16384U;
  budget.locator_budget = locator_budget();
  budget.closure_budget = descent_closure_budget();
  budget.quotient_budget = {512U, 512U, 512U, 512U, 4096U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexBudget cut_index_budget() {
  ExactDirectMorseForestCarrierCutIndexBudget budget;
  budget.maximum_forest_birth_record_scan_count = 4096U;
  budget.maximum_forest_node_scan_count = 4096U;
  budget.maximum_forest_batch_scan_count = 4096U;
  budget.maximum_forest_atomic_group_scan_count = 4096U;
  budget.maximum_forest_saddle_scan_count = 4096U;
  budget.maximum_forest_arm_binding_scan_count = 4096U;
  budget.maximum_forest_child_reference_scan_count = 4096U;
  budget.maximum_forest_final_root_scan_count = 4096U;
  budget.maximum_component_state_count = 4096U;
  budget.maximum_node_marker_state_count = 4096U;
  budget.maximum_index_entry_count = 4096U;
  budget.maximum_group_carrier_scratch_count = 4096U;
  budget.maximum_group_prior_root_scratch_count = 4096U;
  budget.maximum_parent_hop_count = 32768U;
  budget.maximum_exact_level_comparison_count = 32768U;
  budget.maximum_single_exact_level_integer_bit_count = 256U;
  budget.maximum_logical_output_entry_count = 16384U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutReplaySessionBudget
replay_session_budget() {
  ExactDirectMorseForestCarrierCutReplaySessionBudget budget;
  budget.carrier_state_budget = cut_index_budget();
  budget.locator_budget = locator_budget();
  budget.maximum_replayed_global_batch_count = 4096U;
  budget.maximum_replayed_locator_union_count = 4096U;
  budget.maximum_replayed_locator_binding_count = 4096U;
  budget.maximum_batch_group_plan_count = 4096U;
  budget.maximum_batch_group_carrier_reference_count = 4096U;
  budget.maximum_batch_group_prior_root_reference_count = 4096U;
  budget.maximum_batch_locator_union_scratch_count = 4096U;
  budget.maximum_batch_locator_binding_scratch_count = 4096U;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutClosureAdapterBudget
closure_adapter_budget() {
  return {512U, 512U, 4096U, 512U, 4096U, descent_closure_budget()};
}

[[nodiscard]] ExactDirectMorseVerticalTargetFacetPlanBudget facet_plan_budget() {
  return {
      512U,
      512U,
      512U,
      512U,
      4096U,
      4096U,
      32768U,
      32768U,
      8192U,
      16384U};
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalAdapterBudget
proposal_adapter_budget() {
  ExactDirectMorseVerticalTargetProposalAdapterBudget budget;
  budget.maximum_source_saddle_revalidation_count = 4096U;
  budget.maximum_source_binding_revalidation_count = 4096U;
  budget.maximum_source_key_lookup_comparison_count = 32768U;
  budget.maximum_closure_summary_scan_count = 4096U;
  budget.maximum_positive_terminal_probe_count = 4096U;
  budget.maximum_positive_terminal_probe_slot_visit_count = 32768U;
  budget.maximum_positive_terminal_probe_parent_hop_count = 32768U;
  budget.positive_terminal_probe_budget = {1024U, 64U};
  budget.maximum_carrier_entry_revalidation_count = 4096U;
  budget.maximum_target_node_lookup_count = 4096U;
  budget.maximum_exact_level_comparison_count = 32768U;
  budget.maximum_single_exact_level_integer_bit_count = 256U;
  budget.maximum_proposal_count = 4096U;
  budget.maximum_logical_output_entry_count = 4096U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalTargetProposalPipelineBudget
pipeline_budget() {
  ExactDirectMorseVerticalTargetProposalPipelineBudget budget;
  budget.session_budget = replay_session_budget();
  budget.facet_plan_budget = facet_plan_budget();
  budget.closure_budget = closure_adapter_budget();
  budget.proposal_budget = proposal_adapter_budget();
  budget.maximum_source_batch_scan_count = 4096U;
  budget.maximum_source_atomic_group_scan_count = 4096U;
  budget.maximum_referenced_target_order_count = 16U;
  budget.maximum_target_order_lookup_count = 4096U;
  budget.maximum_preflight_facet_plan_count = 4096U;
  budget.maximum_executed_facet_plan_count = 4096U;
  budget.maximum_replay_advance_count = 4096U;
  budget.maximum_closure_build_count = 4096U;
  budget.maximum_proposal_adapter_count = 4096U;
  budget.maximum_aggregate_representative_count = 4096U;
  budget.maximum_aggregate_projected_target_facet_reference_count = 16384U;
  budget.maximum_aggregate_distinct_target_facet_count = 16384U;
  budget.maximum_aggregate_retained_key_point_reference_count = 32768U;
  budget.maximum_aggregate_plan_logical_output_entry_count = 32768U;
  budget.maximum_aggregate_closure_terminal_summary_count = 16384U;
  budget.maximum_aggregate_proposal_count = 4096U;
  budget.maximum_session_audit_count = 16U;
  budget.maximum_group_audit_count = 4096U;
  budget.maximum_logical_output_entry_count = 16384U;
  return budget;
}

[[nodiscard]] ExactDirectMorseVerticalBudget vertical_budget() {
  ExactDirectMorseVerticalBudget budget;
  budget.maximum_forest_node_scan_count = 4096U;
  budget.maximum_child_reference_scan_count = 4096U;
  budget.maximum_birth_record_scan_count = 4096U;
  budget.maximum_batch_scan_count = 4096U;
  budget.maximum_atomic_group_scan_count = 4096U;
  budget.maximum_saddle_scan_count = 4096U;
  budget.maximum_arm_binding_scan_count = 4096U;
  budget.maximum_proposal_count = 4096U;
  budget.maximum_label_resolution_count = 4096U;
  budget.maximum_group_check_count = 4096U;
  budget.maximum_checkpoint_count = 4096U;
  budget.maximum_adjacent_family_count = 16U;
  budget.maximum_group_sort_scratch_count = 4096U;
  budget.maximum_group_sort_comparison_count = 32768U;
  budget.maximum_target_parent_hop_count = 32768U;
  budget.maximum_exact_level_comparison_count = 32768U;
  budget.maximum_single_exact_level_integer_bit_count = 256U;
  budget.maximum_logical_output_entry_count = 16384U;
  return budget;
}

[[nodiscard]] ExactPersistentReducedGammaOrderHistoryBudget history_budget(
    std::size_t point_count) {
  constexpr std::size_t order = 2U;
  const std::size_t facet_count = binomial(point_count, order);
  const std::size_t coface_count = binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_count = facet_count + coface_count;
  const std::size_t replay_count = level_count + 1U;

  ExactPersistentReducedGammaOrderHistoryBudget budget;
  budget.gamma_budget = {facet_count, coface_count, union_count};
  budget.maximum_activation_level_count = level_count;
  budget.maximum_total_facet_work_count = replay_count * facet_count;
  budget.maximum_total_coface_work_count = replay_count * coface_count;
  budget.maximum_total_union_work_count = replay_count * union_count;
  budget.maximum_node_count = coface_count;
  budget.maximum_child_reference_count = coface_count - 1U;
  budget.maximum_group_root_reference_count = coface_count - 1U;
  budget.maximum_group_count = level_count;
  budget.maximum_group_newly_active_facet_count = facet_count;
  budget.maximum_group_equal_level_coface_count = coface_count;
  budget.maximum_delta_facet_count = facet_count;
  budget.maximum_delta_point_reference_count = order * facet_count;
  return budget;
}

[[nodiscard]] ExactK2K1HierarchyBudget hierarchy_budget() {
  ExactK2K1HierarchyBudget budget;
  budget.maximum_source_batch_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_batch_count;
  budget.maximum_source_group_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_group_count;
  budget.maximum_source_node_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_node_count;
  budget.maximum_source_child_reference_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_child_reference_count;
  budget.maximum_source_root_reference_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_root_reference_count;
  budget.maximum_checkpoint_count =
      ExactK2K1HierarchyBudget::maximum_supported_checkpoint_count;
  budget.maximum_source_facet_replay_count =
      ExactK2K1HierarchyBudget::maximum_supported_source_facet_replay_count;
  budget.maximum_vertical_endpoint_lookup_count =
      ExactK2K1HierarchyBudget::maximum_supported_vertical_endpoint_lookup_count;
  budget.maximum_k1_parent_hop_count =
      ExactK2K1HierarchyBudget::maximum_supported_k1_parent_hop_count;
  budget.maximum_target_coverage_point_reference_count =
      ExactK2K1HierarchyBudget::
          maximum_supported_target_coverage_point_reference_count;
  budget.maximum_digest_logical_entry_count =
      ExactK2K1HierarchyBudget::maximum_supported_digest_logical_entry_count;
  budget.maximum_digest_exact_text_byte_count =
      ExactK2K1HierarchyBudget::maximum_supported_digest_exact_text_byte_count;
  return budget;
}

[[nodiscard]] ExactReducedGammaCutBudget gamma_cut_budget() {
  ExactReducedGammaCutBudget budget;
  budget.maximum_batch_count =
      ExactReducedGammaCutBudget::maximum_supported_batch_count;
  budget.maximum_group_record_count =
      ExactReducedGammaCutBudget::maximum_supported_group_record_count;
  budget.maximum_node_record_count =
      ExactReducedGammaCutBudget::maximum_supported_node_record_count;
  budget.maximum_prior_root_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_prior_root_reference_count;
  budget.maximum_child_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_child_reference_count;
  budget.maximum_newly_active_facet_count =
      ExactReducedGammaCutBudget::maximum_supported_newly_active_facet_count;
  budget.maximum_equal_level_coface_count =
      ExactReducedGammaCutBudget::maximum_supported_equal_level_coface_count;
  budget.maximum_delta_facet_count =
      ExactReducedGammaCutBudget::maximum_supported_delta_facet_count;
  budget.maximum_delta_point_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_delta_point_reference_count;
  budget.maximum_active_root_count =
      ExactReducedGammaCutBudget::maximum_supported_active_root_count;
  budget.maximum_output_facet_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_output_facet_reference_count;
  budget.maximum_output_point_reference_count =
      ExactReducedGammaCutBudget::maximum_supported_output_point_reference_count;
  budget.maximum_facet_replay_work_count =
      ExactReducedGammaCutBudget::maximum_supported_facet_replay_work_count;
  budget.maximum_point_id_replay_work_count =
      ExactReducedGammaCutBudget::maximum_supported_point_id_replay_work_count;
  budget.maximum_result_incidence_facet_check_count =
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_facet_check_count;
  budget.maximum_result_incidence_point_id_work_count =
      ExactReducedGammaCutBudget::
          maximum_supported_result_incidence_point_id_work_count;
  return budget;
}

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityBudget authority_budget() {
  ExactDirectMorseK2K1TargetAuthorityBudget budget;
  budget.closed_gamma_cut_budget = gamma_cut_budget();
  budget.maximum_forest_node_scan_count = 4096U;
  budget.maximum_forest_child_reference_scan_count = 4096U;
  budget.maximum_source_batch_scan_count = 4096U;
  budget.maximum_source_binding_scan_count = 4096U;
  budget.maximum_label_resolution_scan_count = 4096U;
  budget.maximum_group_check_scan_count = 4096U;
  budget.maximum_gamma_cut_build_count = 4096U;
  budget.maximum_gamma_active_root_scan_count = 4096U;
  budget.maximum_gamma_facet_scan_count = 16384U;
  budget.maximum_external_checkpoint_scan_count = 16384U;
  budget.maximum_direct_target_point_reference_count = 16384U;
  budget.maximum_external_target_point_reference_count = 16384U;
  budget.maximum_logical_output_entry_count = 32768U;
  return budget;
}

struct Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectMorseForestJournalResult forest;
  ExactDirectMorseVerticalTargetProposalPipelineBudget pipeline_limits;
  ExactDirectMorseVerticalTargetProposalPipelineResult pipeline;
  ExactDirectMorseVerticalBudget vertical_limits;
  ExactDirectMorseVerticalConfig vertical_config;
  ExactDirectMorseVerticalJournalResult vertical;
  ExactPersistentReducedGammaOrderHistoryBudget source_limits;
  ExactPersistentReducedGammaOrderHistory source;
  K1CompactForest k1;
  ExactK2K1HierarchyBudget hierarchy_limits;
  ExactK2K1HierarchyResult hierarchy;
};

[[nodiscard]] Fixture e5_fixture() {
  CanonicalPointCloud cloud = canonical_cloud(std::array{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_limits{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_limits.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_limits.higher);
  const auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_limits, pair, higher);
  const auto events = build_exact_direct_morse_event_journal(cloud, facade);
  const auto seeds = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, events, source_seed_budget());
  ExactDirectMorseForestJournalResult forest =
      build_exact_direct_morse_forest_journal(
          index,
          cloud,
          facade,
          events,
          source_seed_budget(),
          seeds,
          forest_budget(),
          forest_config(),
          LbvhTraversalOrder::near_first);

  ExactDirectMorseVerticalTargetProposalPipelineBudget pipeline_limits =
      pipeline_budget();
  ExactDirectMorseVerticalTargetProposalPipelineResult pipeline =
      build_exact_direct_morse_vertical_target_proposal_pipeline(
          forest, index, cloud, pipeline_limits);
  ExactDirectMorseVerticalBudget vertical_limits = vertical_budget();
  const ExactDirectMorseVerticalConfig vertical_config{authority_id};
  ExactDirectMorseVerticalJournalResult vertical =
      build_exact_direct_morse_vertical_journal(
          forest,
          pipeline.proposals,
          vertical_limits,
          vertical_config);

  ExactPersistentReducedGammaOrderHistoryBudget source_limits =
      history_budget(cloud.size());
  ExactPersistentReducedGammaOrderHistory source =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, source_limits);
  K1CompactForest k1 =
      build_compact_k1_forest(build_exact_complete_graph_emst(cloud));
  ExactK2K1HierarchyBudget hierarchy_limits = hierarchy_budget();
  ExactK2K1HierarchyResult hierarchy = build_exact_k2_k1_hierarchy(
      cloud, source_limits, source, k1, hierarchy_limits);

  const bool all_inputs_ready =
      facade.terminal_catalog_certified() &&
      events.certified_partial_refinement() &&
      seeds.certified_partial_refinement() &&
      forest.certified_conditional_h0_candidate() &&
      pipeline.certified_multiorder_target_proposals() &&
      vertical.certified_conditional_vertical_candidate() &&
      source.decision == ExactPersistentReducedGammaOrderHistoryDecision::
                             complete_persistent_reduced_gamma_history &&
      hierarchy.well_formed_success_receipt();
  if (!all_inputs_ready) {
    std::cerr
        << "E5 input diagnostics: facade_cert="
        << facade.terminal_catalog_certified()
        << " facade_decision=" << enum_code(facade.certificate.decision)
        << " events_cert=" << events.certified_partial_refinement()
        << " events_decision=" << enum_code(events.decision)
        << " seeds_cert=" << seeds.certified_partial_refinement()
        << " seeds_decision=" << enum_code(seeds.decision)
        << " forest_cert=" << forest.certified_conditional_h0_candidate()
        << " forest_decision=" << enum_code(forest.decision)
        << " forest_batches=" << forest.batches.size()
        << " forest_groups=" << forest.atomic_groups.size()
        << " pipeline_cert="
        << pipeline.certified_multiorder_target_proposals()
        << " pipeline_decision=" << enum_code(pipeline.decision)
        << " pipeline_proposals=" << pipeline.proposals.size()
        << " pipeline_resolved=" << pipeline.counters.resolved_proposal_count
        << " pipeline_unresolved="
        << pipeline.counters.unresolved_proposal_count
        << " vertical_cert="
        << vertical.certified_conditional_vertical_candidate()
        << " vertical_decision=" << enum_code(vertical.decision)
        << " vertical_expected=" << vertical.counters.expected_label_count
        << " vertical_missing=" << vertical.counters.missing_label_count
        << " vertical_unresolved=" << vertical.counters.unresolved_label_count
        << " vertical_resolved=" << vertical.counters.resolved_label_count
        << " history_decision=" << enum_code(source.decision)
        << " history_groups=" << source.group_records.size()
        << " hierarchy_cert=" << hierarchy.well_formed_success_receipt()
        << " hierarchy_decision=" << enum_code(hierarchy.decision)
        << " hierarchy_checkpoints="
        << hierarchy.vertical_checkpoints.size() << '\n';
  }

  return {
      std::move(cloud),
      std::move(index),
      std::move(forest),
      std::move(pipeline_limits),
      std::move(pipeline),
      std::move(vertical_limits),
      vertical_config,
      std::move(vertical),
      std::move(source_limits),
      std::move(source),
      std::move(k1),
      std::move(hierarchy_limits),
      std::move(hierarchy)};
}

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityResult build_authority(
    const Fixture& fixture,
    const ExactDirectMorseK2K1TargetAuthorityBudget& budget,
    const ExactDirectMorseVerticalTargetProposalPipelineResult* pipeline =
        nullptr,
    const ExactDirectMorseVerticalJournalResult* vertical = nullptr,
    const ExactK2K1HierarchyResult* hierarchy = nullptr) {
  return build_exact_direct_morse_k2_k1_target_authority(
      fixture.index,
      fixture.cloud,
      fixture.forest,
      fixture.pipeline_limits,
      pipeline == nullptr ? fixture.pipeline : *pipeline,
      fixture.vertical_limits,
      fixture.vertical_config,
      vertical == nullptr ? fixture.vertical : *vertical,
      fixture.source_limits,
      fixture.source,
      fixture.k1,
      fixture.hierarchy_limits,
      hierarchy == nullptr ? fixture.hierarchy : *hierarchy,
      budget);
}

[[nodiscard]] ExactDirectMorseK2K1TargetAuthorityBudget exact_budget_from(
    const ExactDirectMorseK2K1TargetAuthorityResult& result) {
  ExactDirectMorseK2K1TargetAuthorityBudget budget = authority_budget();
  budget.maximum_forest_node_scan_count =
      result.counters.forest_node_scan_count;
  budget.maximum_forest_child_reference_scan_count =
      result.counters.forest_child_reference_scan_count;
  budget.maximum_source_batch_scan_count =
      result.counters.source_batch_scan_count;
  budget.maximum_source_binding_scan_count =
      result.counters.source_binding_scan_count;
  budget.maximum_label_resolution_scan_count =
      result.counters.label_resolution_scan_count;
  budget.maximum_group_check_scan_count =
      result.counters.group_check_scan_count;
  budget.maximum_gamma_cut_build_count =
      result.counters.gamma_cut_build_count;
  budget.maximum_gamma_active_root_scan_count =
      result.counters.gamma_active_root_scan_count;
  budget.maximum_gamma_facet_scan_count =
      result.counters.gamma_facet_scan_count;
  budget.maximum_external_checkpoint_scan_count =
      result.counters.external_checkpoint_scan_count;
  budget.maximum_direct_target_point_reference_count =
      result.counters.direct_target_point_reference_count;
  budget.maximum_external_target_point_reference_count =
      result.counters.external_target_point_reference_count;
  budget.maximum_logical_output_entry_count =
      result.required_logical_output_entry_count;
  return budget;
}

void check_atomic_failure(
    const ExactDirectMorseK2K1TargetAuthorityResult& result,
    ExactDirectMorseK2K1TargetAuthorityDecision decision,
    const std::string& message) {
  check(
      result.decision == decision && result.certified_atomic_failure() &&
          result.no_partial_scientific_payload_published_on_failure &&
          !result.k2_to_k1_observed_label_target_authority_replayed &&
          !result.every_observed_k2_k1_target_coverage_equal,
      message);
}

void test_e5_success_and_fresh_verification() {
  const Fixture fixture = e5_fixture();
  if (!fixture.forest.certified_conditional_h0_candidate() ||
      !fixture.pipeline.certified_multiorder_target_proposals() ||
      !fixture.vertical.certified_conditional_vertical_candidate() ||
      !fixture.hierarchy.well_formed_success_receipt()) {
    std::cerr
        << "E5 trusted-input diagnostic: forest_decision="
        << static_cast<unsigned>(fixture.forest.decision)
        << " forest_certified="
        << fixture.forest.certified_conditional_h0_candidate()
        << " pipeline_decision="
        << static_cast<unsigned>(fixture.pipeline.decision)
        << " pipeline_certified="
        << fixture.pipeline.certified_multiorder_target_proposals()
        << " pipeline_groups=" << fixture.pipeline.required_group_count
        << " pipeline_proposals=" << fixture.pipeline.required_proposal_count
        << " vertical_decision="
        << static_cast<unsigned>(fixture.vertical.decision)
        << " vertical_certified="
        << fixture.vertical.certified_conditional_vertical_candidate()
        << " vertical_labels=" << fixture.vertical.label_resolutions.size()
        << " hierarchy_decision="
        << static_cast<unsigned>(fixture.hierarchy.decision)
        << " hierarchy_success="
        << fixture.hierarchy.well_formed_success_receipt()
        << " hierarchy_failure="
        << fixture.hierarchy.well_formed_fail_closed_receipt() << '\n';
  }
  check(
      fixture.forest.certified_conditional_h0_candidate() &&
          fixture.pipeline.certified_multiorder_target_proposals() &&
          fixture.vertical.certified_conditional_vertical_candidate() &&
          fixture.hierarchy.well_formed_success_receipt(),
      "the real E5 builders produce all trusted inputs");

  const auto family = std::find_if(
      fixture.vertical.adjacent_families.begin(),
      fixture.vertical.adjacent_families.end(),
      [](const ExactDirectMorseVerticalAdjacentFamily& candidate) {
        return candidate.source_order == 2U && candidate.target_order == 1U;
      });
  std::size_t resolved_k2_label_count = 0U;
  if (family != fixture.vertical.adjacent_families.end()) {
    const auto first = fixture.vertical.label_resolutions.begin() +
                       static_cast<std::ptrdiff_t>(
                           family->label_resolution_offset);
    const auto last = first + static_cast<std::ptrdiff_t>(
                                  family->label_resolution_count);
    resolved_k2_label_count = static_cast<std::size_t>(std::count_if(
        first,
        last,
        [](const ExactDirectMorseVerticalLabelResolution& label) {
          return label.disposition ==
                     ExactDirectMorseVerticalLabelDisposition::
                         resolved_closed_target_root &&
                 label.closed_target_root_node_id.has_value();
        }));
  }

  const ExactDirectMorseK2K1TargetAuthorityResult result =
      build_authority(fixture, authority_budget());
  check(
      family != fixture.vertical.adjacent_families.end() &&
          family->label_resolution_count > 0U &&
          resolved_k2_label_count == family->label_resolution_count &&
          result.certified_observed_label_target_authority() &&
          result.decision ==
              ExactDirectMorseK2K1TargetAuthorityDecision::
                  complete_observed_resolved_k2_k1_label_target_authority_replay &&
          result.counters.observed_k2_k1_label_count ==
              family->label_resolution_count &&
          result.counters.resolved_k2_k1_label_count ==
              resolved_k2_label_count &&
          result.counters.certified_target_coverage_equality_count ==
              resolved_k2_label_count,
      "every nonempty resolved E5 K2 label is covered by the bounded authority");
  check(
      result.counters.direct_target_point_reference_count > 0U &&
          result.counters.direct_target_point_reference_count ==
              result.counters.external_target_point_reference_count &&
          result.every_direct_target_coverage_reconstructed_transiently &&
          result.every_external_target_coverage_reconstructed_transiently &&
          result.every_observed_k2_k1_target_coverage_equal &&
          result.k2_to_k1_observed_label_target_authority_replayed &&
          result.bounded_exhaustive_gamma_oracle_used &&
          !result.bidirectional_gamma_group_completeness_replayed &&
          !result.silent_gamma_checkpoint_completeness_replayed &&
          !result.global_morse_obligation_replayed &&
          !result.all_naturality_squares_replayed &&
          !result.vertical_maps_complete && !result.global_m1_claimed &&
          !result.gamma_cells_or_global_cofaces_persisted &&
          !result.higher_order_delaunay_materialized &&
          !result.public_status_claimed,
      "coverage equality is transient and never promotes a global claim");

  const bool e5_multifusion_matches = std::any_of(
      fixture.forest.atomic_groups.begin(),
      fixture.forest.atomic_groups.end(),
      [&](const ExactDirectMorseForestAtomicGroup& group) {
        const auto& batch = fixture.forest.batches[group.batch_index];
        return batch.order == 2U && batch.squared_level == level(13, 2) &&
               group.kind ==
                   ExactDirectMorseForestAtomicGroupKind::multifusion &&
               group.prior_reduced_root_count == 2U;
      });
  const bool external_multifusion_matches = std::any_of(
      fixture.hierarchy.vertical_checkpoints.begin(),
      fixture.hierarchy.vertical_checkpoints.end(),
      [](const ExactK2K1VerticalCheckpoint& checkpoint) {
        return checkpoint.squared_level == level(13, 2) &&
               checkpoint.kind ==
                   ExactK2K1VerticalCheckpointKind::multifusion &&
               checkpoint.strict_prior_target_node_ids.size() == 2U;
      });
  check(
      e5_multifusion_matches && external_multifusion_matches,
      "the authority comparison exercises the semantic E5 binary multifusion");

  const auto verification =
      verify_exact_direct_morse_k2_k1_target_authority(
          fixture.index,
          fixture.cloud,
          fixture.forest,
          fixture.pipeline_limits,
          fixture.pipeline,
          fixture.vertical_limits,
          fixture.vertical_config,
          fixture.vertical,
          fixture.source_limits,
          fixture.source,
          fixture.k1,
          fixture.hierarchy_limits,
          fixture.hierarchy,
          result.requested_budget,
          result);
  check(
      verification.trusted_inputs_accepted &&
          verification.direct_pipeline_freshly_replayed &&
          verification.direct_vertical_journal_freshly_replayed &&
          verification.external_k2_k1_hierarchy_freshly_replayed &&
          verification.expected_result_freshly_reconstructed &&
          verification.observed_recursively_equal &&
          verification.observed_storage_within_budget &&
          verification.result_certified,
      "a fresh recursive replay certifies the complete E5 authority receipt");
}

void test_budget_and_mutation_rejections() {
  const Fixture fixture = e5_fixture();
  const ExactDirectMorseK2K1TargetAuthorityResult generous =
      build_authority(fixture, authority_budget());
  ExactDirectMorseK2K1TargetAuthorityBudget exact =
      exact_budget_from(generous);
  const ExactDirectMorseK2K1TargetAuthorityResult exact_result =
      build_authority(fixture, exact);
  check(
      exact_result.certified_observed_label_target_authority() &&
          exact_result.counters == generous.counters,
      "every authority cap accepts its exact observed requirement");

  check(
      exact.maximum_external_target_point_reference_count > 0U,
      "E5 exposes a nonzero representative external coverage budget");
  --exact.maximum_external_target_point_reference_count;
  const ExactDirectMorseK2K1TargetAuthorityResult one_less =
      build_authority(fixture, exact);
  check_atomic_failure(
      one_less,
      ExactDirectMorseK2K1TargetAuthorityDecision::
          no_authority_budget_exhausted,
      "one fewer external target point reference fails closed without payload");

  CanonicalPointCloud foreign_cloud = canonical_cloud(std::array{
      point(98.0, 99.0),
      point(98.0, 101.0),
      point(100.0, 100.0),
      point(103.0, 102.0),
      point(104.0, 99.0)});
  const MortonLbvhIndex foreign_index = MortonLbvhIndex::build(foreign_cloud);
  const auto namespace_rejected =
      build_exact_direct_morse_k2_k1_target_authority(
          foreign_index,
          foreign_cloud,
          fixture.forest,
          fixture.pipeline_limits,
          fixture.pipeline,
          fixture.vertical_limits,
          fixture.vertical_config,
          fixture.vertical,
          fixture.source_limits,
          fixture.source,
          fixture.k1,
          fixture.hierarchy_limits,
          fixture.hierarchy,
          authority_budget());
  check_atomic_failure(
      namespace_rejected,
      ExactDirectMorseK2K1TargetAuthorityDecision::
          no_authority_direct_pipeline_rejected,
      "a same-cardinality foreign canonical namespace is rejected before authority replay");

  ExactK2K1HierarchyResult forged_digest = fixture.hierarchy;
  forged_digest.hierarchy_projection_digest =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          std::string(64U, 'a'));
  const auto digest_rejected = build_authority(
      fixture, authority_budget(), nullptr, nullptr, &forged_digest);
  check_atomic_failure(
      digest_rejected,
      ExactDirectMorseK2K1TargetAuthorityDecision::
          no_authority_external_hierarchy_rejected,
      "a forged observed hierarchy digest is rejected by fresh replay");

  ExactK2K1HierarchyResult forged_coverage = fixture.hierarchy;
  check(
      !forged_coverage.vertical_checkpoints.empty(),
      "E5 exposes an external checkpoint coverage to mutate");
  if (!forged_coverage.vertical_checkpoints.empty()) {
    auto& checkpoint = forged_coverage.vertical_checkpoints.front();
    checkpoint.closed_target_node_id =
        checkpoint.closed_target_node_id == 0U ? 1U : 0U;
  }
  const auto coverage_rejected = build_authority(
      fixture, authority_budget(), nullptr, nullptr, &forged_coverage);
  check_atomic_failure(
      coverage_rejected,
      ExactDirectMorseK2K1TargetAuthorityDecision::
          no_authority_external_hierarchy_rejected,
      "an altered external checkpoint target coverage is rejected atomically");
}

}  // namespace

int main() {
  test_e5_success_and_fresh_verification();
  test_budget_and_mutation_rejections();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse K2-to-K1 target-authority test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse K2-to-K1 target-authority tests passed\n";
  return 0;
}
