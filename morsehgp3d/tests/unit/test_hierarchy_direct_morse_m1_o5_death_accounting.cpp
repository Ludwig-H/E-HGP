#include "morsehgp3d/hierarchy/direct_morse_m1_o5_death_accounting.hpp"
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
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::uint64_t authority_id = UINT64_C(0x0F5A11);
int failures = 0;

static_assert(direct_morse_m1_o5_death_accounting_schema_version == 1U);
static_assert(
    ExactDirectMorseM1O5DeathAccountingResult::backend == "reference_cpu");
static_assert(
    ExactDirectMorseM1O5DeathAccountingResult::profile == "hgp_reduced");

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
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

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingBudget
accounting_budget() {
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
      capacity};
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

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingResult build_accounting(
    const Fixture& fixture,
    const ExactDirectMorseM1O5DeathAccountingBudget& budget,
    const ExactDirectMorseForestJournalResult* forest = nullptr) {
  return build_exact_direct_morse_m1_o5_death_accounting(
      fixture.index,
      fixture.cloud,
      fixture.facade,
      fixture.events,
      seed_budget(),
      fixture.seeds,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first,
      forest == nullptr ? fixture.forest : *forest,
      budget);
}

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingBudget exact_budget_from(
    const ExactDirectMorseM1O5DeathAccountingResult& result) {
  return {
      result.counters.event_audit_count,
      result.counters.group_audit_count,
      result.counters.batch_audit_count,
      result.counters.source_family_scan_count,
      result.counters.source_arm_seed_scan_count,
      result.counters.source_binding_scan_count,
      result.counters.source_saddle_scan_count,
      result.counters.source_group_scan_count,
      result.counters.source_batch_scan_count,
      result.counters.point_id_reference_scan_count,
      result.counters.prior_root_reference_scan_count,
      result.counters.exact_level_comparison_count,
      result.counters.peak_logical_scratch_entry_count,
      result.required_logical_output_entry_count};
}

void check_atomic_failure(
    const ExactDirectMorseM1O5DeathAccountingResult& result,
    ExactDirectMorseM1O5DeathAccountingDecision decision,
    const std::string& message) {
  check(
      result.decision == decision && result.certified_atomic_failure() &&
          result.event_audits.empty() && result.group_audits.empty() &&
          result.batch_audits.empty() &&
          result.no_partial_scientific_payload_published_on_failure &&
          !result.m1_o5_combinatorial_accounting_replayed,
      message);
}

void test_e5_success_and_fresh_verification() {
  const Fixture fixture = e5_fixture();
  check(
      fixture.facade.terminal_catalog_certified() &&
          fixture.events.certified_partial_refinement() &&
          fixture.seeds.certified_partial_refinement() &&
          fixture.forest.certified_conditional_h0_candidate(),
      "the real E5 chain supplies every relative source authority");

  const auto result = build_accounting(fixture, accounting_budget());
  std::size_t expected_peak_scratch = 0U;
  for (const auto& batch : fixture.forest.batches) {
    std::size_t binding_count = 0U;
    for (std::size_t local = 0U; local < batch.saddle_record_count; ++local) {
      binding_count += fixture.forest
                           .saddle_records[batch.saddle_record_offset + local]
                           .arm_binding_count;
    }
    expected_peak_scratch = std::max(
        expected_peak_scratch,
        10U * binding_count + 3U * batch.saddle_record_count +
            2U * batch.atomic_group_count);
  }
  check(
      result.counters.source_binding_scan_count ==
              2U * fixture.forest.arm_root_bindings.size() &&
          result.counters.source_saddle_scan_count ==
              3U * fixture.forest.saddle_records.size() &&
          result.counters.source_batch_scan_count ==
              3U * fixture.forest.batches.size() &&
          result.counters.exact_level_comparison_count ==
              3U * fixture.seeds.families.size() +
                  fixture.forest.saddle_records.size() +
                  fixture.forest.batches.size() &&
          result.counters.peak_logical_scratch_entry_count ==
              expected_peak_scratch,
      "preflight charges every source pass, comparison and live scratch arena");
  std::array<std::size_t, 3U> kind_counts{};
  for (const auto& group : result.group_audits) {
    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
        ++kind_counts[0U];
        check(group.prior_reduced_root_count == 0U &&
                  group.death_count == 0U,
              "a q0 reduced birth kills no prior H0 class");
        break;
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        ++kind_counts[1U];
        check(group.prior_reduced_root_count == 1U &&
                  group.death_count == 0U,
              "a q1 continuation kills no prior H0 class");
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion:
        ++kind_counts[2U];
        check(
            group.death_count == group.prior_reduced_root_count - 1U,
            "a multifusion kills exactly q minus one prior H0 classes");
        break;
    }
  }

  check(
      result.certified_bounded_conditional_m1_o5_accounting() &&
          result.decision ==
              ExactDirectMorseM1O5DeathAccountingDecision::
                  complete_bounded_conditional_m1_o5_death_accounting &&
          !result.event_audits.empty() && !result.group_audits.empty() &&
          !result.batch_audits.empty() && kind_counts[0U] > 0U &&
          kind_counts[1U] > 0U && kind_counts[2U] > 0U,
      "E5 certifies nonempty q0, q1 and q-at-least-two accounting");
  check(
      std::all_of(
          result.event_audits.begin(),
          result.event_audits.end(),
          [](const ExactDirectMorseM1O5EventAudit& event) {
            return event.arm_count == event.support_point_count &&
                event.arm_count >= 2U &&
                event.delta_one == event.arm_count - 1U;
          }),
      "every event audit binds all shell arms to Delta1 equals |U|-1");
  check(
      std::all_of(
          result.batch_audits.begin(),
          result.batch_audits.end(),
          [](const ExactDirectMorseM1O5BatchAudit& batch) {
            return batch.global_death_count ==
                       batch.sum_group_death_count &&
                batch.global_death_count <=
                    batch.local_multiplicity_capacity &&
                batch.touched_distinct_prior_root_count ==
                    batch.global_death_count +
                        batch.positive_root_group_count;
          }),
      "every batch closes both death identities and its local multiplicity bound");
  check(
      result.source_event_journal_freshly_replayed_relative_to_facade &&
          result.source_seed_journal_freshly_replayed_relative_to_facade &&
          result.source_forest_freshly_replayed_relative_to_facade &&
          result.same_prior_root_carriers_unioned_before_saddle_hyperedges &&
          result.every_saddle_hyperedge_including_latent_closed_transitively &&
          result.m1_o5_combinatorial_accounting_replayed &&
          !result.event_local_death_attribution_serialized &&
          !result.durable_carrier_root_or_node_ids_serialized &&
          !result.source_catalog_complete_for_full_pi0 &&
          !result.carrier_faithfulness_complete &&
          !result.silent_gamma_group_completeness &&
          !result.bidirectional_gamma_group_completeness &&
          !result.external_target_authority_replayed &&
          !result.global_morse_obligation_replayed &&
          !result.global_m1_claimed &&
          !result.all_naturality_squares_replayed &&
          !result.vertical_maps_complete && !result.forest_semantics_exact &&
          !result.bounded_exhaustive_gamma_oracle_used &&
          !result.gamma_cells_or_global_cofaces_materialized &&
          !result.higher_order_delaunay_materialized &&
          !result.public_status_claimed && !result.scalable_50k_claimed,
      "the local O5 receipt preserves every global non-claim");

  const auto verification =
      verify_exact_direct_morse_m1_o5_death_accounting(
          fixture.index,
          fixture.cloud,
          fixture.facade,
          fixture.events,
          seed_budget(),
          fixture.seeds,
          forest_budget(),
          forest_config(),
          LbvhTraversalOrder::near_first,
          fixture.forest,
          result.requested_budget,
          result);
  check(
      verification.observed_storage_within_budget &&
          verification.source_event_journal_freshly_replayed &&
          verification.source_seed_journal_freshly_replayed &&
          verification.source_forest_freshly_replayed &&
          verification.expected_result_freshly_reconstructed &&
          verification.observed_recursively_equal &&
          verification.trusted_inputs_accepted &&
          verification.result_certified,
      "the verifier recursively reconstructs the complete E5 O5 receipt");
}

void test_budget_source_mutation_result_mutation_and_n15_rejections() {
  const Fixture fixture = e5_fixture();
  const auto generous = build_accounting(fixture, accounting_budget());
  auto exact = exact_budget_from(generous);
  const auto exact_result = build_accounting(fixture, exact);
  check(
      exact_result.certified_bounded_conditional_m1_o5_accounting() &&
          exact_result.counters == generous.counters,
      "the exact observed accounting caps still certify E5");

  check(
      exact.maximum_group_audit_count > 0U,
      "E5 exposes a nonzero group-audit cap");
  --exact.maximum_group_audit_count;
  const auto group_limited_failure = build_accounting(fixture, exact);
  check_atomic_failure(
      group_limited_failure,
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_budget_exhausted,
      "one fewer group audit fails closed without scientific payload");

  auto forged_failure = group_limited_failure;
  forged_failure.schema_version += 1U;
  check(
      !forged_failure.certified_outcome(),
      "an atomic failure with a foreign schema is not certified");
  forged_failure = group_limited_failure;
  forged_failure.decision =
      static_cast<ExactDirectMorseM1O5DeathAccountingDecision>(255U);
  check(
      !forged_failure.certified_outcome(),
      "an unknown failure decision is not certified");
  forged_failure = group_limited_failure;
  forged_failure.budget_preflight_certified = true;
  check(
      !forged_failure.certified_outcome(),
      "an atomic failure cannot retain a positive preflight fact");
  forged_failure = group_limited_failure;
  forged_failure.counters.event_audit_count = 1U;
  check(
      !forged_failure.certified_outcome(),
      "an atomic failure cannot retain nonzero scientific counters");
  forged_failure = group_limited_failure;
  forged_failure.event_audit_digest = generous.event_audit_digest;
  check(
      !forged_failure.certified_outcome(),
      "an atomic failure cannot retain a scientific digest");

  auto exact_level_limited = exact_budget_from(generous);
  check(
      exact_level_limited.maximum_exact_level_comparison_count > 0U,
      "E5 exposes a nonzero exact-level comparison cap");
  --exact_level_limited.maximum_exact_level_comparison_count;
  check_atomic_failure(
      build_accounting(fixture, exact_level_limited),
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_budget_exhausted,
      "one fewer exact-level comparison fails during preflight");

  auto binding_pass_limited = exact_budget_from(generous);
  check(
      binding_pass_limited.maximum_source_binding_scan_count > 0U,
      "E5 exposes a nonzero source-binding pass cap");
  --binding_pass_limited.maximum_source_binding_scan_count;
  check_atomic_failure(
      build_accounting(fixture, binding_pass_limited),
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_budget_exhausted,
      "one fewer source-binding visit fails during preflight");

  auto scratch_limited = exact_budget_from(generous);
  check(
      scratch_limited.maximum_logical_scratch_entry_count > 0U,
      "E5 exposes a nonzero logical-scratch cap");
  --scratch_limited.maximum_logical_scratch_entry_count;
  check_atomic_failure(
      build_accounting(fixture, scratch_limited),
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_budget_exhausted,
      "one fewer live scratch entry fails during preflight");

  auto forged_forest = fixture.forest;
  check(
      !forged_forest.atomic_groups.empty(),
      "E5 exposes a forest group to mutate");
  if (!forged_forest.atomic_groups.empty()) {
    ++forged_forest.atomic_groups.back().prior_reduced_root_count;
  }
  check_atomic_failure(
      build_accounting(fixture, accounting_budget(), &forged_forest),
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_source_forest_rejected,
      "a forged source q_R is rejected by fresh forest replay");

  auto forged_result = generous;
  check(
      !forged_result.batch_audits.empty(),
      "E5 exposes a batch death total to mutate");
  if (!forged_result.batch_audits.empty()) {
    ++forged_result.batch_audits.back().global_death_count;
  }
  const auto forged_verification =
      verify_exact_direct_morse_m1_o5_death_accounting(
          fixture.index,
          fixture.cloud,
          fixture.facade,
          fixture.events,
          seed_budget(),
          fixture.seeds,
          forest_budget(),
          forest_config(),
          LbvhTraversalOrder::near_first,
          fixture.forest,
          generous.requested_budget,
          forged_result);
  check(
      forged_verification.expected_result_freshly_reconstructed &&
          !forged_verification.observed_recursively_equal &&
          !forged_verification.result_certified,
      "fresh verification rejects one mutated batch death total");

  std::vector<CertifiedPoint3> raw_n15;
  raw_n15.reserve(15U);
  for (std::size_t index = 0U; index < 15U; ++index) {
    const double value = static_cast<double>(index);
    raw_n15.push_back(point(value, value * value, value * value * value));
  }
  const CanonicalPointCloud cloud_n15 =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{raw_n15});
  const MortonLbvhIndex index_n15 = MortonLbvhIndex::build(cloud_n15);
  const auto n15 = build_exact_direct_morse_m1_o5_death_accounting(
      index_n15,
      cloud_n15,
      fixture.facade,
      fixture.events,
      seed_budget(),
      fixture.seeds,
      forest_budget(),
      forest_config(),
      LbvhTraversalOrder::near_first,
      fixture.forest,
      accounting_budget());
  check_atomic_failure(
      n15,
      ExactDirectMorseM1O5DeathAccountingDecision::
          no_accounting_point_count_or_order_rejected,
      "n=15 fails before any source or exhaustive-oracle replay");
}

}  // namespace

int main() {
  test_e5_success_and_fresh_verification();
  test_budget_source_mutation_result_mutation_and_n15_rejections();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse M1/O5 death-accounting test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse M1/O5 death-accounting tests passed\n";
  return 0;
}
