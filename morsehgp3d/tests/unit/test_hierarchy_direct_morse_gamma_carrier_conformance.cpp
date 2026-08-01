#include "morsehgp3d/hierarchy/direct_morse_gamma_carrier_conformance.hpp"
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

constexpr std::uint64_t authority_id = UINT64_C(0xE5CA221);
int failures = 0;

static_assert(
    direct_morse_gamma_carrier_conformance_schema_version == 1U);
static_assert(
    ExactDirectMorseGammaCarrierConformanceResult::backend ==
    "reference_cpu");
static_assert(
    ExactDirectMorseGammaCarrierConformanceResult::profile ==
    "hgp_reduced");

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y, double z = 0.0) {
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

[[nodiscard]] ExactPairSupportStreamBudget pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget seed_budget() {
  return {4096U, 4096U, 4096U, 4096U};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{1024U, 64U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{1024U, 64U}};
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
  budget.locator_budget = {
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
  budget.closure_budget = {512U, 512U, 512U, 1024U, step_budget()};
  budget.quotient_budget = {512U, 512U, 512U, 512U, 4096U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactCriticalCatalogReducedGammaOverlayBudget overlay_budget() {
  ExactCriticalCatalogReducedGammaOverlayBudget budget;
  budget.critical_catalog_budget = {
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
  auto& history = budget.reduced_gamma_history_budget;
  history.gamma_budget = {
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
  history.maximum_activation_level_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_activation_level_count;
  history.maximum_total_facet_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_facet_work_count;
  history.maximum_total_coface_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_coface_work_count;
  history.maximum_total_union_work_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_total_union_work_count;
  history.maximum_node_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_node_count;
  history.maximum_child_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_child_reference_count;
  history.maximum_group_root_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_root_reference_count;
  history.maximum_group_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_count;
  history.maximum_group_newly_active_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_newly_active_facet_count;
  history.maximum_group_equal_level_coface_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_group_equal_level_coface_count;
  history.maximum_delta_facet_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_facet_count;
  history.maximum_delta_point_reference_count =
      ExactPersistentReducedGammaOrderHistoryBudget::
          maximum_supported_delta_point_reference_count;
  budget.maximum_event_projection_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_event_projection_count;
  budget.maximum_group_overlay_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_group_overlay_count;
  budget.maximum_label_slot_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_label_slot_count;
  budget.maximum_history_point_id_scan_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_history_point_id_scan_count;
  budget.maximum_catalog_point_id_scan_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_catalog_point_id_scan_count;
  budget.maximum_group_event_reference_count =
      ExactCriticalCatalogReducedGammaOverlayBudget::
          maximum_supported_group_event_reference_count;
  return budget;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutIndexBudget cut_budget() {
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

[[nodiscard]] ExactDirectMorseGammaCarrierConformanceBudget
conformance_budget() {
  constexpr std::size_t capacity = 1'000'000'000U;
  ExactDirectMorseGammaCarrierConformanceBudget budget;
  budget.overlay_budget = overlay_budget();
  budget.carrier_cut_budget = cut_budget();
  budget.maximum_direct_role_scan_count = capacity;
  budget.maximum_forest_birth_scan_count = capacity;
  budget.maximum_forest_saddle_scan_count = capacity;
  budget.maximum_forest_atomic_group_scan_count = capacity;
  budget.maximum_activation_level_count = capacity;
  budget.maximum_gamma_transition_build_count = capacity;
  budget.maximum_checkpoint_count = capacity;
  budget.maximum_group_audit_count = capacity;
  budget.maximum_carrier_anchor_scan_count = capacity;
  budget.maximum_gamma_component_scan_count = capacity;
  budget.maximum_gamma_facet_scan_count = capacity;
  budget.maximum_group_event_reference_scan_count = capacity;
  budget.maximum_arm_binding_scan_count = capacity;
  budget.maximum_gamma_incidence_join_count = capacity;
  budget.maximum_carrier_cut_entry_lookup_count = capacity;
  budget.maximum_group_component_membership_count = capacity;
  budget.maximum_gamma_label_comparison_count = capacity;
  budget.maximum_logical_scratch_entry_count = capacity;
  budget.maximum_logical_output_entry_count = capacity;
  return budget;
}

struct Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult events;
  ExactDirectSaddleArmSeedJournalResult seeds;
  ExactDirectMorseForestJournalResult forest;
};

[[nodiscard]] Fixture e5_fixture() {
  CanonicalPointCloud cloud = canonical_cloud(std::array{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto events = build_exact_direct_morse_event_journal(cloud, facade);
  auto seeds = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, events, seed_budget());
  auto forest = build_exact_direct_morse_forest_journal(
      index,
      cloud,
      facade,
      events,
      seed_budget(),
      seeds,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first);
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(events),
      std::move(seeds),
      std::move(forest)};
}

[[nodiscard]] ExactDirectMorseGammaCarrierConformanceResult build_result(
    const Fixture& fixture,
    const ExactDirectMorseGammaCarrierConformanceBudget& budget) {
  return build_exact_direct_morse_gamma_carrier_conformance(
      fixture.index,
      fixture.cloud,
      2U,
      fixture.facade,
      fixture.events,
      seed_budget(),
      fixture.seeds,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first,
      fixture.forest,
      budget);
}

void test_e5_complete_conformance() {
  const Fixture fixture = e5_fixture();
  const auto result = build_result(fixture, conformance_budget());
  check(
      result.certified_bounded_conformance() &&
          result.decision ==
              ExactDirectMorseGammaCarrierConformanceDecision::
                  complete_bounded_single_order_direct_gamma_carrier_group_conformance,
      "E5 closes the bounded direct--Gamma conformance receipt");
  check(
      result.counters.activation_level_count == 12U &&
          result.counters.checkpoint_count == 12U &&
          result.counters.group_audit_count == 13U &&
          result.counters.direct_birth_role_count == 6U &&
          result.counters.direct_saddle_role_count == 4U &&
          result.counters.direct_birth_group_count == 6U &&
          result.counters.direct_saddle_group_count == 2U &&
          result.counters.mixed_direct_residual_group_count == 2U &&
          result.counters.silent_residual_continuation_group_count == 3U &&
          result.counters.silent_checkpoint_count == 3U,
      "E5 partitions all 13 Gamma2 groups as 6 birth, 2 saddle, 2 mixed and 3 silent");

  const std::array<ExactLevel, 3U> silent_levels{
      level(17, 2), level(85, 9), level(10)};
  check(
      std::all_of(
          silent_levels.begin(),
          silent_levels.end(),
          [&](const ExactLevel& expected) {
            return std::any_of(
                result.checkpoints.begin(),
                result.checkpoints.end(),
                [&](const ExactDirectMorseGammaCarrierCheckpointAudit&
                        checkpoint) {
                  return checkpoint.squared_level == expected &&
                         checkpoint.silent_gamma_group_count == 1U &&
                         !checkpoint.direct_batch_present &&
                         checkpoint.strict_partition_bijective &&
                         checkpoint.closed_partition_bijective;
                });
          }),
      "E5 retains the three exact silent checkpoints 17/2, 85/9 and 10");
  check(
      result.source_forest_freshly_replayed_relative_to_facade &&
          result.critical_catalog_gamma_overlay_freshly_replayed &&
          result.direct_h0_catalog_roles_bidirectionally_reconciled &&
          result.direct_atomic_groups_bidirectionally_reconciled &&
          result.
              direct_saddle_arms_bidirectionally_reconciled_with_strict_gamma_components &&
          result.bounded_carrier_faithfulness_replayed &&
          result.bounded_silent_gamma_group_completeness_replayed &&
          result.bounded_bidirectional_gamma_group_completeness_replayed &&
          result.bounded_exhaustive_gamma_oracle_used &&
          !result.product_sparse_silent_source_complete &&
          !result.global_morse_obligation_replayed &&
          !result.global_m1_claimed &&
          !result.all_naturality_squares_replayed &&
          !result.vertical_maps_complete && !result.forest_semantics_exact &&
          !result.scalable_50k_claimed &&
          !result.gamma_cells_or_global_cofaces_persisted &&
          !result.higher_order_delaunay_materialized &&
          !result.public_status_claimed,
      "bounded horizontal evidence never promotes the sparse product or public exact status");
  check(
      result.counters.arm_binding_scan_count > 0U &&
          result.counters.gamma_incidence_join_count ==
              result.counters.arm_binding_scan_count &&
          result.counters.carrier_cut_entry_lookup_count ==
              result.counters.arm_binding_scan_count &&
          std::all_of(
              result.group_audits.begin(),
              result.group_audits.end(),
              [](const ExactDirectMorseGammaGroupAudit& audit) {
                return audit.direct_saddle_role_count == 0U ||
                       (audit.direct_arm_strict_component_bijection &&
                        audit.direct_arm_binding_count > 0U &&
                        audit.strict_gamma_component_count ==
                            audit.latent_strict_gamma_component_count +
                                audit.resolved_strict_gamma_component_count &&
                        audit.resolved_strict_gamma_component_count ==
                            audit.gamma_prior_root_count);
              }),
      "every E5 direct saddle arm joins its exact strict Gamma component and carrier kind");

  auto forged = result;
  forged.group_audits.front().classification =
      static_cast<ExactDirectMorseGammaGroupClassification>(0xffU);
  check(
      !forged.certified_bounded_conformance(),
      "the success predicate rejects an unknown group classification");
  forged = result;
  const auto direct_saddle_group = std::find_if(
      forged.group_audits.begin(),
      forged.group_audits.end(),
      [](const ExactDirectMorseGammaGroupAudit& audit) {
        return audit.direct_saddle_role_count != 0U;
      });
  check(
      direct_saddle_group != forged.group_audits.end(),
      "E5 exposes a direct saddle group for arm-join predicate hardening");
  if (direct_saddle_group != forged.group_audits.end()) {
    direct_saddle_group->direct_arm_strict_component_bijection = false;
  }
  check(
      !forged.certified_bounded_conformance(),
      "the success predicate rejects a forged arm--component conformance fact");

  auto one_less = conformance_budget();
  check(
      result.required_group_audit_capacity > 0U,
      "E5 exposes a nonzero group-audit preflight capacity");
  one_less.maximum_group_audit_count =
      result.required_group_audit_capacity - 1U;
  const auto rejected = build_result(fixture, one_less);
  check(
      rejected.decision ==
              ExactDirectMorseGammaCarrierConformanceDecision::
                  no_conformance_budget_exhausted &&
          rejected.certified_atomic_failure() &&
          rejected.checkpoints.empty() && rejected.group_audits.empty() &&
          rejected.counters.overlay_build_count == 0U &&
          rejected.counters.gamma_transition_build_count == 0U &&
          rejected.counters.carrier_cut_build_count == 0U &&
          !rejected.bounded_carrier_faithfulness_replayed &&
          !rejected.bounded_silent_gamma_group_completeness_replayed,
      "one fewer group-audit slot fails before the exhaustive overlay starts");
}

}  // namespace

int main() {
  test_e5_complete_conformance();
  if (failures != 0) {
    std::cerr << failures
              << " direct Morse--Gamma carrier conformance test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse--Gamma carrier conformance tests passed\n";
  return 0;
}
