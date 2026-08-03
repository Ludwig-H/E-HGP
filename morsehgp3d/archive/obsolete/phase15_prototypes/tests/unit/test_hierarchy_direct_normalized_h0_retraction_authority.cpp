// Archived Phase 15 prototype test; excluded from the active test suite.

#include "morsehgp3d/hierarchy/direct_normalized_h0_retraction_authority.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
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
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
first_incidence_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget gateway_budget() {
  return {
      4096U,
      65536U,
      65536U,
      655360U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      first_incidence_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
gateway_identity_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget plan_budget() {
  return {
      4096U,
      65536U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      gateway_identity_budget(),
  };
}

struct DirectSources {
  ExactPairSupportStreamResult pair_stream;
  ExactHigherSupportStreamResult higher_stream;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal{
      pair_budget(), higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal.pair);
  auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm = arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm);
  const auto incidence = incidence_budget();
  auto incidence_journal = build_exact_direct_closed_saddle_incidence_journal(
      cloud,
      facade,
      event_journal,
      arm,
      arm_journal,
      incidence);
  return {
      std::move(pair),
      std::move(higher),
      std::move(facade),
      std::move(event_journal),
      arm,
      std::move(arm_journal),
      incidence,
      std::move(incidence_journal),
  };
}

struct IntegratedSources {
  ExactDirectSparseGatewayCandidateBudget gateway_cap;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectNormalizedH0SourcePlanBudget plan_cap;
  ExactDirectNormalizedH0SourcePlanResult plan;
  ExactDirectRankWindowSaturatedH0Authority rank;
};

[[nodiscard]] IntegratedSources integrated_sources(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& direct) {
  const auto gateway_cap = gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm,
      direct.arm_journal,
      direct.incidence,
      direct.incidence_journal,
      gateway_cap,
      LbvhTraversalOrder::near_first);
  const auto plan_cap = plan_budget();
  auto plan = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm,
      direct.arm_journal,
      direct.incidence,
      direct.incidence_journal,
      gateway_cap,
      LbvhTraversalOrder::near_first,
      gateway,
      plan_cap);
  auto rank =
      build_exact_direct_rank_window_saturated_h0_authority(direct.facade);
  return {
      gateway_cap,
      std::move(gateway),
      plan_cap,
      std::move(plan),
      std::move(rank),
  };
}

[[nodiscard]] ExactDirectNormalizedH0RetractionAuthority build_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& direct,
    const IntegratedSources& integrated) {
  return build_exact_direct_normalized_h0_retraction_authority(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm,
      direct.arm_journal,
      direct.incidence,
      direct.incidence_journal,
      integrated.gateway_cap,
      LbvhTraversalOrder::near_first,
      integrated.gateway,
      integrated.plan_cap,
      integrated.plan,
      integrated.rank);
}

[[nodiscard]] ExactDirectNormalizedH0RetractionVerification verify_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& direct,
    const IntegratedSources& integrated,
    const ExactDirectNormalizedH0RetractionAuthority& authority) {
  return verify_exact_direct_normalized_h0_retraction_authority(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm,
      direct.arm_journal,
      direct.incidence,
      direct.incidence_journal,
      integrated.gateway_cap,
      LbvhTraversalOrder::near_first,
      integrated.gateway,
      integrated.plan_cap,
      integrated.plan,
      integrated.rank,
      authority);
}

void test_success_and_recursive_mutation_rejection() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources direct = direct_sources(cloud, 2U);
  const IntegratedSources integrated =
      integrated_sources(index, cloud, direct);
  const auto authority = build_authority(index, cloud, direct, integrated);
  const auto verification =
      verify_authority(index, cloud, direct, integrated, authority);

  check(
      integrated.plan.certified_complete_candidate_source_plan() &&
          integrated.rank.certified() && authority.structurally_consistent() &&
          verification.result_certified &&
          authority.normalized_horizontal_h0_equivalence_certified &&
          authority.incidence_complete_reduction_proved &&
          authority.every_higher_order_first_incidence_family_complete &&
          authority.every_first_incidence_cominimizer_retained &&
          authority.exact_level_batches_complete_atomic_quotient_inputs &&
          authority.corollary_4_1_rank_relevant_branch_certified &&
          authority.corollary_4_2_above_window_branch_certified,
      "fresh direct-plus-M(F) and rank-window replay certify the normalized higher-order horizontal H0 source");
  check(
      !integrated.plan.incidence_complete_reduction_proved &&
          !authority.order_one_boruvka_seam_certified &&
          !authority.geometric_global_regularity_claimed &&
          !authority.atomic_quotient_or_forest_fold_executed &&
          !authority.verticality_certified && !authority.naturality_certified &&
          !authority.contract_v2_identity_compatible &&
          !authority.campaign_v3_product_source_claimed &&
          !authority.public_status_claimed && !authority.performance_claimed,
      "the theorem-level receipt does not mutate the source plan or promote excluded scopes");

  auto forged = authority;
  forged.geometric_global_regularity_claimed = true;
  const auto forged_verification =
      verify_authority(index, cloud, direct, integrated, forged);
  check(
      !forged.structurally_consistent() &&
          !forged_verification.observed_recursively_equal &&
          !forged_verification.unsupported_claims_remain_false &&
          !forged_verification.result_certified,
      "recursive replay rejects a forged global-regularity capability");
}

void test_source_plan_and_rank_authority_fail_open() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources direct = direct_sources(cloud, 2U);
  auto integrated = integrated_sources(index, cloud, direct);

  integrated.plan.unique_batch_per_exact_level_and_order = false;
  const auto no_atomic_input =
      build_authority(index, cloud, direct, integrated);
  check(
      no_atomic_input.decision ==
              ExactDirectNormalizedH0RetractionDecision::
                  no_retraction_source_plan_replay_rejected &&
          !no_atomic_input.source_plan_freshly_replayed &&
          !no_atomic_input.incidence_complete_reduction_proved,
      "a missing atomic-batch source invariant fails closed at fresh source-plan replay");

  integrated = integrated_sources(index, cloud, direct);
  integrated.plan.every_higher_order_source_gateway_candidate_mapped_once =
      false;
  const auto no_complete_mf =
      build_authority(index, cloud, direct, integrated);
  check(
      no_complete_mf.decision ==
              ExactDirectNormalizedH0RetractionDecision::
                  no_retraction_source_plan_replay_rejected &&
          !no_complete_mf.every_higher_order_first_incidence_family_complete &&
          !no_complete_mf.incidence_complete_reduction_proved,
      "an incomplete M(F) mapping fails closed before theorem composition");

  integrated = integrated_sources(index, cloud, direct);
  integrated.rank.geometric_global_regularity_claimed = true;
  const auto no_rank = build_authority(index, cloud, direct, integrated);
  check(
      no_rank.decision ==
              ExactDirectNormalizedH0RetractionDecision::
                  no_retraction_rank_window_authority_rejected &&
          !no_rank.rank_window_authority_freshly_verified &&
          !no_rank.source_plan_freshly_replayed &&
          !no_rank.incidence_complete_reduction_proved,
      "a mutated rank authority is rejected before the allocating source replay");
}

void test_rank_relevant_extra_shell_and_order_one_scope_rejected() {
  const std::array<CertifiedPoint3, 4U> square_points{
      point(-1.0, 0.0),
      point(0.0, -1.0),
      point(0.0, 1.0),
      point(1.0, 0.0),
  };
  const CanonicalPointCloud square_cloud = canonical_cloud(square_points);
  const MortonLbvhIndex square_index = MortonLbvhIndex::build(square_cloud);
  const DirectSources square_direct = direct_sources(square_cloud, 2U);
  const IntegratedSources square_integrated =
      integrated_sources(square_index, square_cloud, square_direct);
  const auto square_authority =
      build_authority(
          square_index, square_cloud, square_direct, square_integrated);
  check(
      square_integrated.rank.decision ==
              ExactDirectRankWindowSaturatedH0Decision::
                  unsupported_rank_relevant_extra_shell_degeneracy &&
          square_authority.decision ==
              ExactDirectNormalizedH0RetractionDecision::
                  no_retraction_unsupported_rank_relevant_extra_shell_degeneracy &&
          !square_authority.source_plan_freshly_replayed &&
          !square_authority.incidence_complete_reduction_proved,
      "an in-window extra shell returns its dedicated fail-open decision before source replay");

  const std::array<CertifiedPoint3, 5U> e5_points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud e5_cloud = canonical_cloud(e5_points);
  const MortonLbvhIndex e5_index = MortonLbvhIndex::build(e5_cloud);
  const DirectSources order_one_direct = direct_sources(e5_cloud, 1U);
  const IntegratedSources order_one_integrated =
      integrated_sources(e5_index, e5_cloud, order_one_direct);
  const auto order_one_authority =
      build_authority(
          e5_index, e5_cloud, order_one_direct, order_one_integrated);
  check(
      order_one_authority.decision ==
              ExactDirectNormalizedH0RetractionDecision::
                  no_retraction_empty_higher_order_scope &&
          order_one_authority.rank_window_authority_freshly_verified &&
          order_one_authority.source_plan_freshly_replayed &&
          !order_one_authority.order_one_boruvka_seam_certified &&
          !order_one_authority.incidence_complete_reduction_proved,
      "K=1 remains an explicit external Boruvka seam instead of a vacuous normalized capability");
}

}  // namespace

int main() {
  test_success_and_recursive_mutation_rejection();
  test_source_plan_and_rank_authority_fail_open();
  test_rank_relevant_extra_shell_and_order_one_scope_rejected();
  if (failures != 0) {
    std::cerr << failures
              << " direct normalized-H0 retraction-authority test(s) failed\n";
    return 1;
  }
  std::cout
      << "all direct normalized-H0 retraction-authority tests passed\n";
  return 0;
}
