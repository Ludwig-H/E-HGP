#include "morsehgp3d/hierarchy/silent_pair_gateway.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
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

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] CanonicalId id(std::uint8_t seed) {
  std::array<std::uint8_t, CanonicalId::byte_count> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        seed + static_cast<std::uint8_t>(index));
  }
  return CanonicalId{bytes};
}

[[nodiscard]] ExactSilentPairGatewayStrictLegDecision leg(
    PointId removed,
    ExactSilentPairGatewayPairIds strict_facet,
    ExactSilentPairGatewayPairIds bridge,
    ExactSilentPairGatewayCofaceIds coface,
    ExactLevel coface_level,
    ExactLevel path_level,
    std::size_t target,
    const CanonicalId& snapshot,
    const CanonicalId& source_id) {
  ExactSilentPairGatewayStrictLegDecision result;
  result.removed_support_point_id = removed;
  result.strict_facet_point_ids = strict_facet;
  result.bridge_facet_point_ids = bridge;
  result.strict_replacement_coface_point_ids = coface;
  result.strict_replacement_coface_squared_level =
      std::move(coface_level);
  result.strict_path_maximum_squared_level = std::move(path_level);
  result.target_component_handle = target;
  result.pre_batch_snapshot_digest = snapshot;
  result.source_decision_id = source_id;
  result.exact_strict_replacement_coface_certified = true;
  result.exact_path_to_pre_batch_target_certified = true;
  return result;
}

[[nodiscard]] ExactSilentPairGatewayInput valid_input() {
  // Realizable one-dimensional regular fixture embedded in R3: support
  // {-5,5}, canonical interiors {0,1}, and two exterior points.  Removing
  // -5 gives the strict coface {0,1,5} at 25/4; removing 5 gives
  // {-5,0,1} at 9.  Both share {0,1} and therefore reach the same root
  // strictly before the pair level 25.
  ExactSilentPairGatewayInput input;
  input.point_count = 6U;
  input.pair_point_ids = {1U, 4U};
  input.squared_level = level(25);
  input.closed_ball.strict_interior_count = 2U;
  input.closed_ball.extra_shell_count = 0U;
  input.closed_ball.exterior_count = 2U;
  input.closed_ball.source_decision_id = id(1U);
  input.closed_ball.exact_diametral_pair_level_certified = true;
  input.closed_ball.unique_essential_diametral_support_certified = true;
  input.closed_ball.complete_closed_ball_partition_certified = true;
  input.closed_ball.canonical_two_witness_selection_certified = true;
  input.strict_interior_witnesses = {
      ExactSilentPairGatewayStrictInteriorDecision{
          2U, level(0), id(2U), true},
      ExactSilentPairGatewayStrictInteriorDecision{
          3U, level(1), id(3U), true}};
  input.pre_batch_target.order = 2U;
  input.pre_batch_target.batch_squared_level = level(25);
  input.pre_batch_target.component_handle = 7U;
  input.pre_batch_target.pre_batch_component_count = 9U;
  input.pre_batch_target.pre_batch_prefix_count = 4U;
  input.pre_batch_target.pre_batch_snapshot_digest = id(4U);
  input.pre_batch_target.source_decision_id = id(5U);
  input.pre_batch_target.target_frozen_before_equal_level_batch = true;
  input.pre_batch_target.target_is_nontrivial_reduced_root = true;
  input.strict_legs = {
      leg(
          1U,
          {4U, 2U},
          {2U, 3U},
          {4U, 2U, 3U},
          level(25, 4),
          level(9),
          7U,
          input.pre_batch_target.pre_batch_snapshot_digest,
          id(6U)),
      leg(
          4U,
          {1U, 2U},
          {2U, 3U},
          {1U, 2U, 3U},
          level(9),
          level(9),
          7U,
          input.pre_batch_target.pre_batch_snapshot_digest,
          id(7U))};
  return input;
}

void require_complete(
    const ExactSilentPairGatewayInput& input,
    const ExactSilentPairGatewayResult& result,
    const std::string& fixture) {
  check(result.certified_gateway(), fixture + ": local acceptance failed");
  const ExactSilentPairGatewayVerification verification =
      verify_exact_silent_pair_gateway(input, result);
  check(
      verification.certified_gateway(),
      fixture + ": fresh relative replay failed");
  check(
      result.gateway.has_value(),
      fixture + ": no compact gateway was published");
}

void require_rejected(
    const ExactSilentPairGatewayInput& input,
    ExactSilentPairGatewayDecision expected_decision,
    const std::string& fixture) {
  const ExactSilentPairGatewayResult result =
      build_exact_silent_pair_gateway(input);
  check(
      result.decision == expected_decision,
      fixture + ": wrong rejection decision");
  check(
      result.certified_fail_closed_rejection(),
      fixture + ": rejection was not atomic and fail-closed");
  check(
      !result.gateway.has_value(),
      fixture + ": a rejected input leaked a partial gateway");
  check(
      verify_exact_silent_pair_gateway(input, result)
          .certified_fail_closed_rejection(),
      fixture + ": rejected outcome did not fresh-replay");
}

void test_complete_gateway_contract() {
  const ExactSilentPairGatewayInput input = valid_input();
  const ExactSilentPairGatewayResult result =
      build_exact_silent_pair_gateway(input);
  require_complete(input, result, "complete regular pair");
  if (!result.gateway.has_value()) {
    return;
  }
  const ExactSilentPairGatewayRecord& gateway = *result.gateway;
  check(
      gateway.pair_point_ids ==
          ExactSilentPairGatewayPairIds{1U, 4U} &&
          gateway.strict_interior_witness_point_ids ==
              std::array<PointId, 2U>{2U, 3U} &&
          gateway.squared_level == level(25),
      "complete gateway lost its canonical pair, witnesses or exact level");
  check(
      gateway.strict_legs[0].removed_support_point_id == 1U &&
          gateway.strict_legs[0].strict_replacement_coface_point_ids ==
              ExactSilentPairGatewayCofaceIds{2U, 3U, 4U} &&
          gateway.strict_legs[1].removed_support_point_id == 4U &&
          gateway.strict_legs[1].strict_replacement_coface_point_ids ==
              ExactSilentPairGatewayCofaceIds{1U, 2U, 3U},
      "strict bridge cofaces were not canonical");
  check(
      gateway.target_component_handle == 7U &&
          gateway.pre_batch_component_count == 9U &&
          gateway.pre_batch_prefix_count == 4U &&
          gateway.pre_batch_snapshot_digest ==
              input.pre_batch_target.pre_batch_snapshot_digest,
      "pre-batch target authority was not retained");
  check(
      gateway.coverage_delta.added_facet_count == 1U &&
          gateway.coverage_delta.added_facet_point_ids[0] ==
              gateway.pair_point_ids &&
          gateway.coverage_delta.added_point_count == 0U &&
          gateway.coverage_delta.added_point_ids.empty() &&
          gateway.coverage_delta.exact_pair_facet_only,
      "coverage_delta is not exactly {pair}/zero points");
  check(
      !gateway.autonomous_node_created &&
          !gateway.root_birth_published &&
          !gateway.merge_node_published &&
          !gateway.gateway_attach_published &&
          !gateway.gamma2_prune_authority_claimed &&
          !gateway.gamma2_completeness_claimed &&
          !gateway.public_hierarchy_exactness_claimed &&
          !gateway.public_status_claimed,
      "local silent incidence escaped into a node, prune or public claim");
  check(
      result.audit.no_cloud_or_gamma_rescan_performed &&
          result.audit.no_global_facet_coface_or_cell_catalog_materialized &&
          result.audit.no_delaunay_or_higher_order_mosaic_materialized &&
          result.audit.partial_refinement_only,
      "isolated gateway violated its architecture-only scope");
  check(
      gateway.canonical_digest.to_lower_hex() ==
          "581de50087ee1a126667940c3d0a3879fcd780b27741080b2f8f1434409593b9",
      "canonical gateway digest changed");
}

void test_permutation_invariance() {
  const ExactSilentPairGatewayInput canonical = valid_input();
  const ExactSilentPairGatewayResult expected =
      build_exact_silent_pair_gateway(canonical);
  require_complete(canonical, expected, "canonical permutation baseline");

  ExactSilentPairGatewayInput permuted = canonical;
  std::swap(
      permuted.pair_point_ids[0], permuted.pair_point_ids[1]);
  std::swap(
      permuted.strict_interior_witnesses[0],
      permuted.strict_interior_witnesses[1]);
  std::swap(permuted.strict_legs[0], permuted.strict_legs[1]);
  for (ExactSilentPairGatewayStrictLegDecision& strict_leg :
       permuted.strict_legs) {
    std::reverse(
        strict_leg.strict_facet_point_ids.begin(),
        strict_leg.strict_facet_point_ids.end());
    std::reverse(
        strict_leg.bridge_facet_point_ids.begin(),
        strict_leg.bridge_facet_point_ids.end());
    std::reverse(
        strict_leg.strict_replacement_coface_point_ids.begin(),
        strict_leg.strict_replacement_coface_point_ids.end());
  }
  const ExactSilentPairGatewayResult observed =
      build_exact_silent_pair_gateway(permuted);
  require_complete(permuted, observed, "fully permuted input");
  check(
      observed == expected,
      "pair, witness, leg or local-facet permutations changed the gateway");
}

void test_fail_closed_decision_matrix() {
  {
    auto input = valid_input();
    input.schema_version = 2U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_schema_or_point_count,
        "schema");
  }
  {
    auto input = valid_input();
    input.point_count = 3U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_schema_or_point_count,
        "point count");
  }
  {
    auto input = valid_input();
    input.pair_point_ids = {1U, 1U};
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_pair_or_level,
        "duplicate support");
  }
  {
    auto input = valid_input();
    input.squared_level = level(0);
    input.pre_batch_target.batch_squared_level = level(0);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_pair_or_level,
        "zero diametral level");
  }
  {
    auto input = valid_input();
    input.closed_ball.unique_essential_diametral_support_certified = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_certificate,
        "non-essential support");
  }
  {
    auto input = valid_input();
    input.closed_ball.complete_closed_ball_partition_certified = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_certificate,
        "incomplete ball partition");
  }
  {
    auto input = valid_input();
    input.closed_ball.canonical_two_witness_selection_certified = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_certificate,
        "noncanonical witness selection");
  }
  {
    auto input = valid_input();
    input.closed_ball.exterior_count = 1U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_count_accounting,
        "closed-ball accounting");
  }
  {
    auto input = valid_input();
    input.closed_ball.strict_interior_count =
        std::numeric_limits<std::size_t>::max();
    input.closed_ball.exterior_count = 0U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_count_accounting,
        "closed-ball overflow");
  }
  {
    auto input = valid_input();
    input.closed_ball.extra_shell_count = 1U;
    input.closed_ball.exterior_count = 1U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_extra_shell_not_excluded,
        "extra shell");
  }
  {
    auto input = valid_input();
    input.strict_interior_witnesses[1].point_id = 2U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_strict_interior_witnesses,
        "duplicate strict witness");
  }
  {
    auto input = valid_input();
    input.strict_interior_witnesses[0]
        .exact_strict_interior_certified = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_strict_interior_witnesses,
        "uncertified strict witness");
  }
  {
    auto input = valid_input();
    input.strict_interior_witnesses[0]
        .squared_distance_to_diametral_center = level(25);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::
            rejected_strict_interior_witnesses,
        "shell witness");
  }
  {
    auto input = valid_input();
    input.pre_batch_target.order = 3U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_pre_batch_target,
        "target order");
  }
  {
    auto input = valid_input();
    input.pre_batch_target.target_frozen_before_equal_level_batch = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_pre_batch_target,
        "unfrozen target");
  }
  {
    auto input = valid_input();
    input.pre_batch_target.component_handle = 9U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_pre_batch_target,
        "out-of-range target");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].strict_facet_point_ids = {1U, 2U};
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_shape,
        "wrong strict facet");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].bridge_facet_point_ids = {2U, 5U};
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_shape,
        "wrong bridge facet");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].exact_strict_replacement_coface_certified = false;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_shape,
        "uncertified replacement coface");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].strict_replacement_coface_squared_level =
        level(0);
    input.strict_legs[0].strict_path_maximum_squared_level = level(0);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_level,
        "zero level for a strict three-point coface");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].strict_replacement_coface_squared_level =
        level(25);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_level,
        "equal-level replacement coface");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].strict_path_maximum_squared_level = level(6);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_level,
        "path maximum below its coface");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].target_component_handle = 6U;
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_target,
        "divergent root target");
  }
  {
    auto input = valid_input();
    input.strict_legs[0].pre_batch_snapshot_digest = id(99U);
    require_rejected(
        input,
        ExactSilentPairGatewayDecision::rejected_strict_leg_target,
        "divergent snapshot target");
  }
}

void test_digest_binds_every_authority_class() {
  const ExactSilentPairGatewayInput baseline_input = valid_input();
  const ExactSilentPairGatewayResult baseline =
      build_exact_silent_pair_gateway(baseline_input);
  require_complete(
      baseline_input, baseline, "digest binding baseline");
  if (!baseline.gateway.has_value()) {
    return;
  }

  {
    auto input = baseline_input;
    input.closed_ball.source_decision_id = id(80U);
    const auto changed = build_exact_silent_pair_gateway(input);
    require_complete(input, changed, "closed-ball authority digest");
    check(
        changed.gateway->canonical_digest !=
            baseline.gateway->canonical_digest,
        "closed-ball authority id is absent from the canonical digest");
  }
  {
    auto input = baseline_input;
    input.strict_interior_witnesses[0]
        .squared_distance_to_diametral_center = level(1, 2);
    const auto changed = build_exact_silent_pair_gateway(input);
    require_complete(input, changed, "strict witness exact digest");
    check(
        changed.gateway->canonical_digest !=
            baseline.gateway->canonical_digest,
        "strict witness distance is absent from the canonical digest");
  }
  {
    auto input = baseline_input;
    input.strict_legs[0].strict_path_maximum_squared_level = level(10);
    const auto changed = build_exact_silent_pair_gateway(input);
    require_complete(input, changed, "strict path exact digest");
    check(
        changed.gateway->canonical_digest !=
            baseline.gateway->canonical_digest,
        "strict path maximum is absent from the canonical digest");
  }
  {
    auto input = baseline_input;
    input.pre_batch_target.pre_batch_snapshot_digest = id(81U);
    for (auto& strict_leg : input.strict_legs) {
      strict_leg.pre_batch_snapshot_digest =
          input.pre_batch_target.pre_batch_snapshot_digest;
    }
    const auto changed = build_exact_silent_pair_gateway(input);
    require_complete(input, changed, "pre-batch snapshot digest");
    check(
        changed.gateway->canonical_digest !=
            baseline.gateway->canonical_digest,
        "pre-batch snapshot is absent from the canonical digest");
  }
}

template <typename Mutation>
void require_hostile_output_rejected(
    const ExactSilentPairGatewayInput& input,
    const ExactSilentPairGatewayResult& baseline,
    Mutation&& mutation,
    const std::string& fixture) {
  ExactSilentPairGatewayResult hostile = baseline;
  mutation(hostile);
  check(
      !verify_exact_silent_pair_gateway(input, hostile)
           .certified_gateway(),
      fixture + ": hostile observed output passed fresh replay");
}

void test_hostile_observed_output_matrix() {
  const ExactSilentPairGatewayInput input = valid_input();
  const ExactSilentPairGatewayResult baseline =
      build_exact_silent_pair_gateway(input);
  require_complete(input, baseline, "hostile output baseline");
  if (!baseline.gateway.has_value()) {
    return;
  }
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) { result.gateway->pair_point_ids[0] = 0U; },
      "pair mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->strict_interior_witness_point_ids[0] = 0U;
      },
      "witness mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->strict_legs[0]
            .strict_replacement_coface_squared_level = level(8);
      },
      "strict coface mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->target_component_handle = 6U;
      },
      "target mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->coverage_delta.added_point_count = 1U;
      },
      "coverage point mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->autonomous_node_created = true;
      },
      "autonomous node mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->gamma2_prune_authority_claimed = true;
      },
      "Gamma2 prune authority mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->public_status_claimed = true;
      },
      "public status mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.gateway->canonical_digest = id(90U);
      },
      "canonical digest mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.audit.no_cloud_or_gamma_rescan_performed = false;
      },
      "architecture audit mutation");
  require_hostile_output_rejected(
      input,
      baseline,
      [](auto& result) {
        result.decision =
            ExactSilentPairGatewayDecision::rejected_strict_leg_shape;
      },
      "decision mutation");
}

void test_maximum_point_id_boundary_without_cloud_scan() {
  const std::size_t point_count =
      static_cast<std::size_t>(CanonicalPointCloud::max_point_count);
  const PointId maximum_id =
      static_cast<PointId>(point_count - 1U);
  ExactSilentPairGatewayInput input;
  input.point_count = point_count;
  input.pair_point_ids = {maximum_id - 3U, maximum_id - 2U};
  input.squared_level = level(4);
  input.closed_ball.strict_interior_count = 2U;
  input.closed_ball.extra_shell_count = 0U;
  input.closed_ball.exterior_count = point_count - 4U;
  input.closed_ball.source_decision_id = id(100U);
  input.closed_ball.exact_diametral_pair_level_certified = true;
  input.closed_ball.unique_essential_diametral_support_certified = true;
  input.closed_ball.complete_closed_ball_partition_certified = true;
  input.closed_ball.canonical_two_witness_selection_certified = true;
  input.strict_interior_witnesses = {
      ExactSilentPairGatewayStrictInteriorDecision{
          maximum_id - 1U, level(0), id(101U), true},
      ExactSilentPairGatewayStrictInteriorDecision{
          maximum_id, level(1), id(102U), true}};
  input.pre_batch_target.order = 2U;
  input.pre_batch_target.batch_squared_level = level(4);
  input.pre_batch_target.component_handle = 0U;
  input.pre_batch_target.pre_batch_component_count = 1U;
  input.pre_batch_target.pre_batch_prefix_count = 1U;
  input.pre_batch_target.pre_batch_snapshot_digest = id(103U);
  input.pre_batch_target.source_decision_id = id(104U);
  input.pre_batch_target.target_frozen_before_equal_level_batch = true;
  input.pre_batch_target.target_is_nontrivial_reduced_root = true;
  input.strict_legs = {
      leg(
          maximum_id - 3U,
          {maximum_id - 2U, maximum_id - 1U},
          {maximum_id - 1U, maximum_id},
          {maximum_id - 2U, maximum_id - 1U, maximum_id},
          level(1),
          level(2),
          0U,
          input.pre_batch_target.pre_batch_snapshot_digest,
          id(105U)),
      leg(
          maximum_id - 2U,
          {maximum_id - 3U, maximum_id - 1U},
          {maximum_id - 1U, maximum_id},
          {maximum_id - 3U, maximum_id - 1U, maximum_id},
          level(1),
          level(3),
          0U,
          input.pre_batch_target.pre_batch_snapshot_digest,
          id(106U))};
  const auto result = build_exact_silent_pair_gateway(input);
  require_complete(input, result, "maximum PointId boundary");
  check(
      result.audit.no_cloud_or_gamma_rescan_performed,
      "maximum PointId input caused a forbidden cloud scan");
}

}  // namespace

int main() {
  try {
    test_complete_gateway_contract();
    test_permutation_invariance();
    test_fail_closed_decision_matrix();
    test_digest_binds_every_authority_class();
    test_hostile_observed_output_matrix();
    test_maximum_point_id_boundary_without_cloud_scan();
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "UNCAUGHT: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures << " SilentPairGateway test(s) failed\n";
    return 1;
  }
  std::cout << "SilentPairGateway tests passed\n";
  return 0;
}
