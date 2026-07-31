#include "morsehgp3d/hierarchy/silent_pair_gateway_gamma2_differential.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] std::size_t binomial(
    std::size_t n, std::size_t k) {
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

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialBudget tight_budget(
    std::size_t point_count) {
  constexpr std::size_t order = 2U;
  const std::size_t facet_count = binomial(point_count, order);
  const std::size_t coface_count =
      binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_count = facet_count + coface_count;
  const std::size_t replay_count = level_count + 1U;

  ExactSilentPairGatewayGamma2DifferentialBudget budget;
  auto& history = budget.history_budget;
  history.gamma_budget = {
      facet_count, coface_count, union_count};
  history.maximum_activation_level_count = level_count;
  history.maximum_total_facet_work_count =
      replay_count * facet_count;
  history.maximum_total_coface_work_count =
      replay_count * coface_count;
  history.maximum_total_union_work_count =
      replay_count * union_count;
  history.maximum_node_count = coface_count;
  history.maximum_child_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  history.maximum_group_root_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  history.maximum_group_count = level_count;
  history.maximum_group_newly_active_facet_count = facet_count;
  history.maximum_group_equal_level_coface_count = coface_count;
  history.maximum_delta_facet_count = facet_count;
  history.maximum_delta_point_reference_count =
      order * facet_count;
  return budget;
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  // Source labels are A, B, C, D, E. Canonical ids are D=0, A=1,
  // B=2, C=3 and E=4.
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0)};
  return canonical_cloud(input);
}

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialResult
build_e5() {
  return build_exact_silent_pair_gateway_gamma2_differential(
      e5_cloud(), {1U, 3U}, tight_budget(5U));
}

void require_complete(
    const CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair,
    const ExactSilentPairGatewayGamma2DifferentialBudget& budget,
    const ExactSilentPairGatewayGamma2DifferentialResult& result,
    const std::string& fixture) {
  check(
      result.certified_differential(),
      fixture + ": bounded differential did not certify");
  const auto verification =
      verify_exact_silent_pair_gateway_gamma2_differential(
          cloud, pair, budget, result);
  check(
      verification.certified_differential(),
      fixture + ": fresh rebuild did not certify");
  check(
      verification.fresh_replay_certified &&
          verification.expected_outcome_freshly_rebuilt &&
          verification.no_forbidden_structure_or_public_claim_certified,
      fixture + ": fresh verifier did not close every outer gate");
}

void require_rejected(
    const CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair,
    const ExactSilentPairGatewayGamma2DifferentialBudget& budget,
    ExactSilentPairGatewayGamma2DifferentialDecision expected,
    const std::string& fixture) {
  const auto result =
      build_exact_silent_pair_gateway_gamma2_differential(
          cloud, pair, budget);
  check(
      result.decision == expected,
      fixture + ": wrong fail-closed decision");
  check(
      result.certified_fail_closed_rejection(),
      fixture + ": rejection is not certified fail-closed");
  check(
      !result.gateway_input.has_value() &&
          !result.gateway_result.has_value(),
      fixture + ": rejection leaked a compact gateway");
  check(
      verify_exact_silent_pair_gateway_gamma2_differential(
          cloud, pair, budget, result)
          .certified_fail_closed_rejection(),
      fixture + ": rejected outcome did not fresh-replay");
}

template <typename Mutation>
void require_hostile_observation_rejected(
    const ExactSilentPairGatewayGamma2DifferentialResult& baseline,
    Mutation&& mutation,
    const std::string& fixture) {
  auto hostile = baseline;
  std::forward<Mutation>(mutation)(hostile);
  const auto verification =
      verify_exact_silent_pair_gateway_gamma2_differential(
          e5_cloud(), {1U, 3U}, tight_budget(5U), hostile);
  check(
      !verification.certified_differential() &&
          !verification.fresh_replay_certified &&
          !verification.expected_outcome_freshly_rebuilt,
      fixture + ": hostile observed state passed fresh rebuild");
}

void test_e5_complete_differential() {
  const CanonicalPointCloud cloud = e5_cloud();
  const auto budget = tight_budget(5U);
  const auto result =
      build_exact_silent_pair_gateway_gamma2_differential(
          cloud, {1U, 3U}, budget);
  require_complete(
      cloud, {1U, 3U}, budget, result, "E5 silent pair");

  check(
      result.decision ==
          ExactSilentPairGatewayGamma2DifferentialDecision::
              complete_exact_regular_silent_pair_gateway_gamma2_differential &&
          result.scope ==
              ExactSilentPairGatewayGamma2DifferentialScope::
                  bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only,
      "E5 did not retain its exact decision and bounded scope");
  check(
      result.pair_point_ids ==
              ExactSilentPairGatewayPairIds{1U, 3U} &&
          result.pair_miniball.has_value() &&
          result.pair_miniball->squared_radius == level(33, 2) &&
          result.pair_miniball->support_point_ids ==
              std::vector<PointId>({1U, 3U}),
      "E5 pair lost its canonical diametral support or exact level");
  check(
      result.strict_interior_point_ids ==
              std::vector<PointId>({0U, 4U}) &&
          result.extra_shell_point_ids.empty() &&
          result.canonical_strict_interior_witness_point_ids ==
              std::array<PointId, 2U>{0U, 4U},
      "E5 closed-ball partition or canonical witnesses changed");
  check(
      result.exhaustive_pair_equal_level_coface_point_ids ==
          std::vector<ExactSilentPairGatewayCofaceIds>(
              {{0U, 1U, 3U}, {1U, 3U, 4U}}),
      "E5 compact witnesses do not reconcile the exhaustive pair cofaces");
  check(
      result.strict_leg_replays.size() == 2U &&
          result.strict_gamma.has_value() &&
          result.reduced_gamma_batch.has_value() &&
          result.persistent_gamma_history.has_value() &&
          result.target_strict_gamma_component_index.has_value() &&
          result.target_prior_root_node_id.has_value(),
      "E5 omitted a strict leg, Gamma view, history or pre-batch root");
  check(
      result.all_pair_interior_cofaces_exhaustively_reconciled &&
          result.both_strict_legs_reach_same_component &&
          result.target_is_frozen_nontrivial_pre_batch_root &&
          result.equal_level_effect_is_pair_only_zero_points_continuation &&
          result.pair_history_creates_no_node &&
          result.pair_persists_through_every_later_gamma2_batch &&
          result.compact_gateway_freshly_certified,
      "E5 did not close the gateway-to-history differential facts");
  check(
      result.gateway_result.has_value() &&
          result.gateway_result->gateway.has_value() &&
          result.gateway_result->gateway->coverage_delta
                  .added_facet_point_ids[0] ==
              ExactSilentPairGatewayPairIds{1U, 3U} &&
          result.gateway_result->gateway->coverage_delta
              .added_point_ids.empty() &&
          !result.gateway_result->gateway->autonomous_node_created,
      "E5 gateway is not exactly pair/zero-points/no-node");
  check(
      !result.later_root_trace.empty() &&
          result.later_root_trace.front().squared_level ==
              level(33, 2) &&
          result.later_root_trace.back().root_facet_count == 10U &&
          result.later_root_trace.back().root_covered_point_count == 5U,
      "E5 pair was not traced through the complete later history");
  check(
      result.no_global_product_delaunay_or_public_claim &&
          !result.gateway_result->gateway
               ->gamma2_completeness_claimed &&
          !result.gateway_result->gateway
               ->public_hierarchy_exactness_claimed,
      "E5 differential escaped into a product or public exactness claim");
}

void test_pair_permutation_is_canonical() {
  const CanonicalPointCloud cloud = e5_cloud();
  const auto budget = tight_budget(5U);
  const auto canonical =
      build_exact_silent_pair_gateway_gamma2_differential(
          cloud, {1U, 3U}, budget);
  const auto permuted =
      build_exact_silent_pair_gateway_gamma2_differential(
          cloud, {3U, 1U}, budget);
  require_complete(
      cloud, {3U, 1U}, budget, permuted, "E5 permuted pair");
  check(
      permuted == canonical &&
          permuted.pair_point_ids ==
              ExactSilentPairGatewayPairIds{1U, 3U},
      "pair permutation changed the canonical bounded differential");
}

void test_fresh_geometry_rejections() {
  {
    // Pair {-2,2}: two strict interiors and the additional shell point
    // (0,2,0). Canonical pair ids are {0,4}.
    const std::array<CertifiedPoint3, 5U> input{
        point(-2.0), point(2.0), point(0.0),
        point(0.0, 1.0), point(0.0, 2.0)};
    const auto cloud = canonical_cloud(input);
    require_rejected(
        cloud,
        {0U, 4U},
        tight_budget(5U),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_extra_shell_not_excluded,
        "fresh extra-shell pair");
  }
  {
    // Pair {-2,2}: only canonical id 1 lies strictly inside; id 3 is
    // exterior. Canonical pair ids are {0,2}.
    const std::array<CertifiedPoint3, 4U> input{
        point(-2.0), point(2.0), point(0.0), point(4.0)};
    const auto cloud = canonical_cloud(input);
    require_rejected(
        cloud,
        {0U, 2U},
        tight_budget(4U),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_fewer_than_two_strict_interiors,
        "fresh one-interior pair");
  }
}

void test_invalid_and_degenerate_hostile_pairs() {
  const CanonicalPointCloud cloud = e5_cloud();
  const auto budget = tight_budget(5U);
  require_rejected(
      cloud,
      {1U, 99U},
      budget,
      ExactSilentPairGatewayGamma2DifferentialDecision::
          rejected_point_count_or_pair,
      "out-of-range hostile pair");
  // A duplicate-free cloud has a unique positive diametral support for
  // every distinct pair. A repeated pair is therefore the only direct
  // degenerate/nonpositive hostile pair available to this fresh API.
  require_rejected(
      cloud,
      {1U, 1U},
      budget,
      ExactSilentPairGatewayGamma2DifferentialDecision::
          rejected_point_count_or_pair,
      "degenerate nonunique hostile pair");
}

void test_budget_minus_one_is_atomic() {
  const CanonicalPointCloud cloud = e5_cloud();
  auto insufficient = tight_budget(5U);
  --insufficient.history_budget.gamma_budget
        .maximum_enumerated_facet_count;
  const auto result =
      build_exact_silent_pair_gateway_gamma2_differential(
          cloud, {1U, 3U}, insufficient);
  check(
      result.decision ==
              ExactSilentPairGatewayGamma2DifferentialDecision::
                  no_differential_preflight_budget_insufficient &&
          result.certified_fail_closed_rejection(),
      "budget-1 did not fail atomically at differential preflight");
  check(
      !result.candidate_space_preflight_certified &&
          !result.pair_miniball.has_value() &&
          result.counters.pair_miniball_build_count == 0U &&
          result.counters.closed_ball_query_count == 0U &&
          result.counters.strict_gamma_build_count == 0U &&
          result.counters.persistent_gamma_history_build_count == 0U,
      "budget-1 started geometry or Gamma/history work");
  check(
      verify_exact_silent_pair_gateway_gamma2_differential(
          cloud, {1U, 3U}, insufficient, result)
          .certified_fail_closed_rejection(),
      "budget-1 rejection did not fresh-replay");
}

void test_impossible_natural_branches_are_defensive_mutations() {
  // For a distinct pair in a duplicate-free cloud the positive diametral
  // support is the pair itself. With two strict interior witnesses, each
  // replacement triple has a positive miniball strictly below the pair
  // level, and the two triples share their interior bridge. Consequently
  // nonunique, zero, nonstrict and divergent observations cannot be selected
  // by fresh regular geometry; they remain mandatory verifier defenses.
  const auto baseline = build_e5();
  check(
      baseline.certified_differential(),
      "defensive-mutation baseline did not certify");
  if (!baseline.certified_differential() ||
      baseline.strict_leg_replays.size() != 2U ||
      !baseline.pair_miniball.has_value()) {
    return;
  }

  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.unique_essential_diametral_support_certified = false;
      },
      "defensive nonunique-support mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.strict_leg_replays[0]
            .strict_replacement_coface_miniball.squared_radius =
            level(0);
      },
      "defensive zero-coface mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.strict_leg_replays[0]
            .strict_replacement_coface_miniball.squared_radius =
            result.pair_miniball->squared_radius;
      },
      "defensive nonstrict-coface mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        ++result.strict_leg_replays[0]
              .strict_gamma_component_index;
      },
      "defensive divergent-target mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.target_is_frozen_nontrivial_pre_batch_root = false;
      },
      "defensive trivial-target mutation");
}

void test_fresh_verifier_rejects_every_authority_layer() {
  const auto baseline = build_e5();
  check(
      baseline.certified_differential(),
      "authority-mutation baseline did not certify");
  if (!baseline.certified_differential() ||
      !baseline.persistent_gamma_history.has_value() ||
      !baseline.gateway_result.has_value() ||
      !baseline.gateway_result->gateway.has_value()) {
    return;
  }

  require_hostile_observation_rejected(
      baseline,
      [](auto& result) { result.pair_point_ids = {0U, 3U}; },
      "stored pair mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        ++result.counters.exhaustive_pair_equal_level_coface_count;
      },
      "counter mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.canonical_cloud_digest =
            morsehgp3d::contract::CanonicalId{};
      },
      "cloud digest mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.persistent_gamma_history->group_records.front()
            .newly_active_facet_point_ids.clear();
      },
      "exhaustive history mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        ++result.gateway_result->gateway->coverage_delta
              .added_point_count;
      },
      "compact gateway mutation");
  require_hostile_observation_rejected(
      baseline,
      [](auto& result) {
        result.decision =
            ExactSilentPairGatewayGamma2DifferentialDecision::
                rejected_persistent_gamma2_history;
      },
      "outer decision mutation");
}

}  // namespace

int main() {
  test_e5_complete_differential();
  test_pair_permutation_is_canonical();
  test_fresh_geometry_rejections();
  test_invalid_and_degenerate_hostile_pairs();
  test_budget_minus_one_is_atomic();
  test_impossible_natural_branches_are_defensive_mutations();
  test_fresh_verifier_rejects_every_authority_layer();
  if (failures != 0) {
    std::cerr << failures
              << " silent-pair Gamma2 differential test(s) failed\n";
    return 1;
  }
  std::cout
      << "all silent-pair Gamma2 differential tests passed\n";
  return 0;
}
