#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

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

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
generous_first_incidence_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget generous_gateway_budget() {
  return {
      4096U,
      65536U,
      65536U,
      655360U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_first_incidence_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
generous_gateway_identity_budget() {
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

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget generous_plan_budget() {
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
      generous_gateway_identity_budget(),
  };
}

struct DirectSources {
  ExactPairSupportStreamResult pair_stream;
  ExactHigherSupportStreamResult higher_stream;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal = build_exact_direct_closed_saddle_incidence_journal(
      cloud,
      facade,
      event_journal,
      arm_budget,
      arm_journal,
      incidence_budget);
  return {
      std::move(pair),
      std::move(higher),
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

[[nodiscard]] bool complete_verification(
    const ExactDirectNormalizedH0SourcePlanVerification& verification) {
  return verification.observed_storage_within_budget &&
         verification.source_gateway_journal_freshly_verified &&
         verification.expected_result_freshly_reconstructed &&
         verification.observed_recursively_equal &&
         verification.no_forbidden_global_structure_materialized &&
         verification.unsupported_noop_and_resident_claims_remain_false &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

struct IntegratedSource {
  ExactDirectSparseGatewayCandidateBudget gateway_budget;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectSparseGatewayCandidateScientificIdentityResult gateway_identity;
  ExactDirectNormalizedH0SourcePlanBudget plan_budget;
  ExactDirectNormalizedH0SourcePlanResult plan;
};

[[nodiscard]] IntegratedSource build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    LbvhTraversalOrder traversal_order) {
  const auto gateway_budget = generous_gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      traversal_order);
  check(
      gateway.certified_partial_refinement(),
      "the existing sparse gateway authority is certified");
  const auto plan_budget = generous_plan_budget();
  auto plan = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      traversal_order,
      gateway,
      plan_budget);
  const auto verification = verify_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      traversal_order,
      gateway,
      plan_budget,
      plan);
  check(
      complete_verification(verification),
      "the candidate-complete source plan closes under fresh recursive replay");
  auto gateway_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          gateway, plan_budget.source_gateway_identity_budget);
  check(
      gateway_identity.certified_identity(),
      "the gateway scientific identity is computed once for reconstruction");
  return {
      gateway_budget,
      std::move(gateway),
      std::move(gateway_identity),
      plan_budget,
      std::move(plan),
  };
}

[[nodiscard]] std::vector<PointId> reconstructed_ids(
    const IntegratedSource& source,
    std::size_t coface_index) {
  const auto key = reconstruct_exact_direct_normalized_h0_source_coface(
      source.gateway,
      source.gateway_identity,
      source.plan,
      coface_index);
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

using ScientificCoface =
    std::tuple<std::vector<PointId>, ExactLevel,
               ExactDirectNormalizedH0SourceCofaceKind>;

[[nodiscard]] std::vector<ScientificCoface> scientific_cofaces(
    const IntegratedSource& source) {
  std::vector<ScientificCoface> projection;
  projection.reserve(source.plan.cofaces.size());
  for (const auto& coface : source.plan.cofaces) {
    projection.emplace_back(
        reconstructed_ids(source, coface.coface_index),
        coface.squared_level,
        coface.kind);
  }
  return projection;
}

void test_e5_complete_compressed_source_and_explicit_blocker() {
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
  const auto near = build_and_verify(
      index, cloud, direct, LbvhTraversalOrder::near_first);
  const auto far = build_and_verify(
      index, cloud, direct, LbvhTraversalOrder::far_first);

  check(
      near.plan.certified_complete_candidate_source_plan() &&
          !near.gateway.facet_tokens.empty() &&
          !near.plan.cofaces.empty() &&
          near.plan.required_source_gateway_token_scan_count ==
              near.gateway.facet_tokens.size() &&
          near.plan.gateway_candidate_references.size() ==
              near.plan.required_higher_order_gateway_candidate_count &&
          near.plan.excluded_order_one_gateway_candidate_count +
                  near.plan.required_higher_order_gateway_candidate_count ==
              near.gateway.gateway_candidates.size() &&
          near.plan.required_batch_coface_reference_count ==
              near.plan.cofaces.size(),
      "E5 reuses every complete gateway, maps every co-minimizer and emits one normalized reference per coface");
  check(
      std::all_of(
          near.plan.gateway_candidate_references.begin(),
          near.plan.gateway_candidate_references.end(),
          [&near](const auto& reference) {
            const auto& candidate = near.gateway.gateway_candidates[
                reference.source_gateway_candidate_index];
            const auto& token =
                near.gateway.facet_tokens[candidate.facet_token_index];
            const std::size_t coface_index = reference.source_coface_index;
            const auto ids = reconstructed_ids(near, coface_index);
            return token.source_facet_key.point_count >= 2U &&
                   std::includes(
                       ids.begin(),
                       ids.end(),
                       token.source_facet_key.point_ids.begin(),
                       token.source_facet_key.point_ids.begin() +
                           static_cast<std::ptrdiff_t>(
                               token.source_facet_key.point_count)) &&
                   near.plan.cofaces[coface_index].squared_level ==
                       token.first_incidence_squared_level;
          }),
      "every aligned gateway mapping preserves its exact level and incident core facet");
  check(
      scientific_cofaces(near) == scientific_cofaces(far) &&
          near.plan.batches == far.plan.batches,
      "near-first and far-first traversal preserve the compressed source and normalized batches");
  check(
      near.plan.gateway_core_keys_and_audits_reused_without_copy &&
          near.plan.no_successive_incidence_star_materialized &&
          near.plan.no_global_star_facet_or_coface_catalog_materialized &&
          near.plan.no_gamma_or_higher_order_delaunay_materialized &&
          !near.plan.global_regularity_authority_certified &&
          !near.plan.omitted_late_cofaces_qr1_noop_certified &&
          !near.plan.normalized_noop_level_receipts_emitted &&
          !near.plan.incidence_complete_reduction_proved &&
          !near.plan.resident_atomic_fold_performed &&
          !near.plan.contract_v2_identity_compatible &&
          !near.plan.campaign_v3_single_gateway_per_core_facet_satisfied &&
          !near.plan.campaign_v3_product_source_claimed &&
          !near.plan.public_status_claimed,
      "the integration records the global-regularity and resident-fold blocker instead of fabricating a no-op certificate");
}

void test_budget_failure_is_atomic_and_mutation_fails_replay() {
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
  auto integrated = build_and_verify(
      index, cloud, direct, LbvhTraversalOrder::near_first);

  auto short_budget = integrated.plan_budget;
  short_budget.maximum_source_gateway_token_scan_count = 0U;
  const auto rejected = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm_budget,
      direct.arm_journal,
      direct.incidence_budget,
      direct.incidence_journal,
      integrated.gateway_budget,
      LbvhTraversalOrder::near_first,
      integrated.gateway,
      short_budget);
  check(
      rejected.decision ==
              ExactDirectNormalizedH0SourcePlanDecision::
                  no_source_budget_exhausted &&
          rejected.gateway_candidate_references.empty() &&
          rejected.cofaces.empty() &&
          rejected.direct_birth_references.empty() &&
          rejected.batch_coface_references.empty() &&
          rejected.batches.empty() &&
          rejected.logical_storage_entry_count == 0U,
      "a gateway scan cap failure publishes no scientific prefix");

  auto late_budget = integrated.plan_budget;
  late_budget.maximum_batch_count = 0U;
  const auto late_rejected = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm_budget,
      direct.arm_journal,
      direct.incidence_budget,
      direct.incidence_journal,
      integrated.gateway_budget,
      LbvhTraversalOrder::near_first,
      integrated.gateway,
      late_budget);
  check(
      late_rejected.decision ==
              ExactDirectNormalizedH0SourcePlanDecision::
                  no_source_budget_exhausted &&
          late_rejected.gateway_candidate_references.empty() &&
          late_rejected.cofaces.empty() &&
          late_rejected.direct_birth_references.empty() &&
          late_rejected.batch_coface_references.empty() &&
          late_rejected.batches.empty(),
      "a late normalized-batch cap rejects atomically before publication");

  auto mutated_gateway = integrated.gateway;
  mutated_gateway.gateway_candidates.front()
      .added_point_in_selected_positive_support =
      !mutated_gateway.gateway_candidates.front()
           .added_point_in_selected_positive_support;
  const auto mutated_gateway_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          mutated_gateway,
          integrated.plan_budget.source_gateway_identity_budget);
  bool mutated_gateway_rejected = false;
  try {
    static_cast<void>(
        reconstruct_exact_direct_normalized_h0_source_coface(
            mutated_gateway,
            mutated_gateway_identity,
            integrated.plan,
            0U));
  } catch (const std::invalid_argument&) {
    mutated_gateway_rejected = true;
  }
  check(
      mutated_gateway_rejected,
      "the one-time scientific identity receipt rejects a same-shape mutated gateway payload");

  integrated.plan.omitted_late_cofaces_qr1_noop_certified = true;
  const auto verification = verify_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      direct.facade,
      direct.event_journal,
      direct.arm_budget,
      direct.arm_journal,
      direct.incidence_budget,
      direct.incidence_journal,
      integrated.gateway_budget,
      LbvhTraversalOrder::near_first,
      integrated.gateway,
      integrated.plan_budget,
      integrated.plan);
  check(
      !verification.observed_recursively_equal &&
          !verification.unsupported_noop_and_resident_claims_remain_false &&
          !verification.result_certified,
      "fresh replay rejects a forged omitted-noop claim");
}

}  // namespace

int main() {
  test_e5_complete_compressed_source_and_explicit_blocker();
  test_budget_failure_is_atomic_and_mutation_fails_replay();
  if (failures != 0) {
    std::cerr << failures
              << " direct normalized-H0 source-plan test(s) failed\n";
    return 1;
  }
  std::cout << "all direct normalized-H0 source-plan tests passed\n";
  return 0;
}
