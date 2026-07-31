#include "morsehgp3d/hierarchy/silent_pair_gateway.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) { text(domain); }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) { byte(value ? std::uint8_t{1U} : 0U); }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(
        value, "a SilentPairGateway size does not fit uint64"));
  }

  void point_id(PointId value) { u64(value); }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void level(const exact::ExactLevel& value) { text(value.canonical_key()); }

  void canonical_id(const contract::CanonicalId& value) {
    builder_.update(value.bytes());
  }

  template <std::size_t Size>
  void point_ids(const std::array<PointId, Size>& values) {
    size(values.size());
    for (const PointId value : values) {
      point_id(value);
    }
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

template <std::size_t Size>
[[nodiscard]] std::array<PointId, Size> canonical_point_ids(
    std::array<PointId, Size> point_ids) noexcept {
  std::sort(point_ids.begin(), point_ids.end());
  return point_ids;
}

[[nodiscard]] bool valid_point_id(
    PointId point_id,
    std::size_t point_count) noexcept {
  if constexpr (sizeof(std::size_t) > sizeof(PointId)) {
    if (point_count >
        static_cast<std::size_t>(std::numeric_limits<PointId>::max())) {
      return false;
    }
  }
  return point_id < static_cast<PointId>(point_count);
}

template <std::size_t Size>
[[nodiscard]] bool strictly_increasing_valid_ids(
    const std::array<PointId, Size>& point_ids,
    std::size_t point_count) noexcept {
  for (std::size_t index = 0U; index < Size; ++index) {
    if (!valid_point_id(point_ids[index], point_count) ||
        (index != 0U && point_ids[index - 1U] >= point_ids[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactSilentPairGatewayResult base_result() {
  ExactSilentPairGatewayResult result;
  result.scope =
      ExactSilentPairGatewayScope::
          regular_q3_pair_silent_incidence_relative_to_supplied_certified_decisions_only;
  result.audit.supplied_decisions_only = true;
  result.audit.no_cloud_or_gamma_rescan_performed = true;
  result.audit.no_global_facet_coface_or_cell_catalog_materialized = true;
  result.audit.no_delaunay_or_higher_order_mosaic_materialized = true;
  result.audit.no_partial_gateway_published = true;
  result.audit.gamma2_prune_or_completeness_claimed = false;
  result.audit.public_hierarchy_or_status_claimed = false;
  result.audit.partial_refinement_only = true;
  return result;
}

[[nodiscard]] ExactSilentPairGatewayResult reject(
    ExactSilentPairGatewayResult result,
    ExactSilentPairGatewayDecision decision) {
  result.gateway.reset();
  result.decision = decision;
  return result;
}

[[nodiscard]] ExactSilentPairGatewayPairIds expected_strict_facet(
    const ExactSilentPairGatewayPairIds& pair,
    const std::array<PointId, 2U>& witnesses,
    PointId removed_support_point_id) {
  const PointId retained_support =
      removed_support_point_id == pair[0] ? pair[1] : pair[0];
  return canonical_point_ids(
      ExactSilentPairGatewayPairIds{retained_support, witnesses[0]});
}

[[nodiscard]] ExactSilentPairGatewayCofaceIds expected_strict_coface(
    const ExactSilentPairGatewayPairIds& pair,
    const std::array<PointId, 2U>& witnesses,
    PointId removed_support_point_id) {
  const PointId retained_support =
      removed_support_point_id == pair[0] ? pair[1] : pair[0];
  return canonical_point_ids(
      ExactSilentPairGatewayCofaceIds{
          retained_support, witnesses[0], witnesses[1]});
}

void write_strict_leg(
    DigestWriter& writer,
    const ExactSilentPairGatewayStrictLegWitness& leg) {
  writer.point_id(leg.removed_support_point_id);
  writer.point_ids(leg.strict_facet_point_ids);
  writer.point_ids(leg.bridge_facet_point_ids);
  writer.point_ids(leg.strict_replacement_coface_point_ids);
  writer.level(leg.strict_replacement_coface_squared_level);
  writer.level(leg.strict_path_maximum_squared_level);
  writer.canonical_id(leg.source_decision_id);
}

[[nodiscard]] contract::CanonicalId canonical_gateway_digest(
    const ExactSilentPairGatewayRecord& gateway) {
  DigestWriter writer{"MorseHGP3D/SilentPairGateway/canonical/v1"};
  writer.size(gateway.order);
  writer.point_ids(gateway.pair_point_ids);
  writer.level(gateway.squared_level);
  writer.point_ids(gateway.strict_interior_witness_point_ids);
  for (const exact::ExactLevel& distance :
       gateway.strict_interior_witness_squared_distances) {
    writer.level(distance);
  }
  for (const contract::CanonicalId& source_id :
       gateway.strict_interior_source_decision_ids) {
    writer.canonical_id(source_id);
  }
  writer.size(gateway.certified_strict_interior_count);
  writer.size(gateway.certified_extra_shell_count);
  writer.size(gateway.certified_exterior_count);
  writer.canonical_id(gateway.closed_ball_source_decision_id);
  for (const ExactSilentPairGatewayStrictLegWitness& leg :
       gateway.strict_legs) {
    write_strict_leg(writer, leg);
  }
  writer.size(gateway.target_component_handle);
  writer.size(gateway.pre_batch_component_count);
  writer.size(gateway.pre_batch_prefix_count);
  writer.canonical_id(gateway.pre_batch_snapshot_digest);
  writer.canonical_id(gateway.pre_batch_target_source_decision_id);
  writer.size(gateway.coverage_delta.added_facet_count);
  for (const ExactSilentPairGatewayPairIds& facet :
       gateway.coverage_delta.added_facet_point_ids) {
    writer.point_ids(facet);
  }
  writer.size(gateway.coverage_delta.added_point_count);
  writer.boolean(gateway.coverage_delta.exact_pair_facet_only);
  writer.boolean(gateway.autonomous_node_created);
  writer.boolean(gateway.root_birth_published);
  writer.boolean(gateway.merge_node_published);
  writer.boolean(gateway.gateway_attach_published);
  writer.boolean(gateway.gamma2_prune_authority_claimed);
  writer.boolean(gateway.gamma2_completeness_claimed);
  writer.boolean(gateway.public_hierarchy_exactness_claimed);
  writer.boolean(gateway.public_status_claimed);
  return writer.finalize();
}

[[nodiscard]] bool gateway_negative_claims_valid(
    const ExactSilentPairGatewayRecord& gateway) noexcept {
  return !gateway.autonomous_node_created &&
         !gateway.root_birth_published &&
         !gateway.merge_node_published &&
         !gateway.gateway_attach_published &&
         !gateway.gamma2_prune_authority_claimed &&
         !gateway.gamma2_completeness_claimed &&
         !gateway.public_hierarchy_exactness_claimed &&
         !gateway.public_status_claimed;
}

[[nodiscard]] bool coverage_delta_valid(
    const ExactSilentPairGatewayRecord& gateway) noexcept {
  return gateway.coverage_delta.added_facet_count == 1U &&
         gateway.coverage_delta.added_facet_point_ids[0] ==
             gateway.pair_point_ids &&
         gateway.coverage_delta.added_point_count == 0U &&
         gateway.coverage_delta.added_point_ids.empty() &&
         gateway.coverage_delta.exact_pair_facet_only;
}

[[nodiscard]] bool gateway_structurally_valid(
    const ExactSilentPairGatewayRecord& gateway) {
  std::size_t point_count = 2U;
  if (!checked_add(
          point_count,
          gateway.certified_strict_interior_count,
          point_count) ||
      !checked_add(
          point_count,
          gateway.certified_extra_shell_count,
          point_count) ||
      !checked_add(
          point_count,
          gateway.certified_exterior_count,
          point_count) ||
      point_count >
          static_cast<std::size_t>(
              spatial::CanonicalPointCloud::max_point_count) ||
      gateway.order != silent_pair_gateway_order ||
      gateway.squared_level.numerator() <= 0 ||
      gateway.certified_strict_interior_count < 2U ||
      gateway.certified_extra_shell_count != 0U ||
      !strictly_increasing_valid_ids(
          gateway.pair_point_ids, point_count) ||
      !strictly_increasing_valid_ids(
          gateway.strict_interior_witness_point_ids, point_count) ||
      std::binary_search(
          gateway.pair_point_ids.begin(),
          gateway.pair_point_ids.end(),
          gateway.strict_interior_witness_point_ids[0]) ||
      std::binary_search(
          gateway.pair_point_ids.begin(),
          gateway.pair_point_ids.end(),
          gateway.strict_interior_witness_point_ids[1]) ||
      gateway.pre_batch_component_count == 0U ||
      gateway.target_component_handle >=
          gateway.pre_batch_component_count ||
      gateway.pre_batch_prefix_count == 0U ||
      !coverage_delta_valid(gateway) ||
      !gateway_negative_claims_valid(gateway)) {
    return false;
  }
  for (const exact::ExactLevel& distance :
       gateway.strict_interior_witness_squared_distances) {
    if (!(distance < gateway.squared_level)) {
      return false;
    }
  }
  for (std::size_t leg_index = 0U;
       leg_index < gateway.strict_legs.size();
       ++leg_index) {
    const ExactSilentPairGatewayStrictLegWitness& leg =
        gateway.strict_legs[leg_index];
    if (leg.removed_support_point_id !=
            gateway.pair_point_ids[leg_index] ||
        leg.strict_facet_point_ids !=
            expected_strict_facet(
                gateway.pair_point_ids,
                gateway.strict_interior_witness_point_ids,
                leg.removed_support_point_id) ||
        leg.bridge_facet_point_ids !=
            gateway.strict_interior_witness_point_ids ||
        leg.strict_replacement_coface_point_ids !=
            expected_strict_coface(
                gateway.pair_point_ids,
                gateway.strict_interior_witness_point_ids,
                leg.removed_support_point_id) ||
        leg.strict_replacement_coface_squared_level.numerator() <= 0 ||
        !(leg.strict_replacement_coface_squared_level <
          gateway.squared_level) ||
        !(leg.strict_path_maximum_squared_level <
          gateway.squared_level) ||
        leg.strict_path_maximum_squared_level <
            leg.strict_replacement_coface_squared_level) {
      return false;
    }
  }
  return gateway.canonical_digest == canonical_gateway_digest(gateway);
}

[[nodiscard]] bool base_audit_valid(
    const ExactSilentPairGatewayAudit& audit) noexcept {
  return audit.supplied_decisions_only &&
         audit.no_cloud_or_gamma_rescan_performed &&
         audit.no_global_facet_coface_or_cell_catalog_materialized &&
         audit.no_delaunay_or_higher_order_mosaic_materialized &&
         audit.no_partial_gateway_published &&
         !audit.gamma2_prune_or_completeness_claimed &&
         !audit.public_hierarchy_or_status_claimed &&
         audit.partial_refinement_only;
}

}  // namespace

bool ExactSilentPairGatewayResult::certified_gateway() const noexcept {
  if (schema_version != silent_pair_gateway_schema_version ||
      decision !=
          ExactSilentPairGatewayDecision::
              complete_certified_regular_q3_silent_pair_gateway ||
      scope !=
          ExactSilentPairGatewayScope::
              regular_q3_pair_silent_incidence_relative_to_supplied_certified_decisions_only ||
      !gateway.has_value() || !base_audit_valid(audit)) {
    return false;
  }
  return audit.pair_and_witness_permutations_canonicalized &&
         audit.pair_level_and_unique_essential_support_certified &&
         audit.complete_closed_ball_count_accounting_certified &&
         audit.no_extra_shell_certified &&
         audit.two_canonical_strict_interior_witnesses_certified &&
         audit.two_strict_bridge_cofaces_certified &&
         audit.both_strict_paths_target_same_frozen_pre_batch_root &&
         audit.coverage_delta_is_pair_facet_and_zero_points &&
         audit.no_autonomous_node_or_forest_action &&
         gateway_structurally_valid(*gateway);
}

bool ExactSilentPairGatewayResult::certified_fail_closed_rejection()
    const noexcept {
  return schema_version == silent_pair_gateway_schema_version &&
         decision != ExactSilentPairGatewayDecision::not_certified &&
         decision !=
             ExactSilentPairGatewayDecision::
                 complete_certified_regular_q3_silent_pair_gateway &&
         scope ==
             ExactSilentPairGatewayScope::
                 regular_q3_pair_silent_incidence_relative_to_supplied_certified_decisions_only &&
         !gateway.has_value() && base_audit_valid(audit);
}

ExactSilentPairGatewayResult build_exact_silent_pair_gateway(
    const ExactSilentPairGatewayInput& input) {
  ExactSilentPairGatewayResult result = base_result();
  if (input.schema_version != silent_pair_gateway_schema_version ||
      input.point_count < 4U ||
      input.point_count >
          static_cast<std::size_t>(spatial::CanonicalPointCloud::max_point_count)) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::rejected_schema_or_point_count);
  }

  const ExactSilentPairGatewayPairIds pair =
      canonical_point_ids(input.pair_point_ids);
  if (!strictly_increasing_valid_ids(pair, input.point_count) ||
      input.squared_level.numerator() <= 0) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::rejected_pair_or_level);
  }

  auto witnesses = input.strict_interior_witnesses;
  std::sort(
      witnesses.begin(),
      witnesses.end(),
      [](const ExactSilentPairGatewayStrictInteriorDecision& left,
         const ExactSilentPairGatewayStrictInteriorDecision& right) {
        return left.point_id < right.point_id;
      });
  const std::array<PointId, 2U> witness_ids{
      witnesses[0].point_id, witnesses[1].point_id};
  if (!strictly_increasing_valid_ids(witness_ids, input.point_count) ||
      std::binary_search(pair.begin(), pair.end(), witness_ids[0]) ||
      std::binary_search(pair.begin(), pair.end(), witness_ids[1])) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::
            rejected_strict_interior_witnesses);
  }
  result.audit.pair_and_witness_permutations_canonicalized = true;

  const auto& closed_ball = input.closed_ball;
  if (!closed_ball.exact_diametral_pair_level_certified ||
      !closed_ball.unique_essential_diametral_support_certified ||
      !closed_ball.complete_closed_ball_partition_certified ||
      !closed_ball.canonical_two_witness_selection_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_certificate);
  }
  result.audit.pair_level_and_unique_essential_support_certified = true;

  std::size_t classified_count = 2U;
  if (!checked_add(
          classified_count,
          closed_ball.strict_interior_count,
          classified_count) ||
      !checked_add(
          classified_count,
          closed_ball.extra_shell_count,
          classified_count) ||
      !checked_add(
          classified_count,
          closed_ball.exterior_count,
          classified_count) ||
      classified_count != input.point_count ||
      closed_ball.strict_interior_count < 2U) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::
            rejected_closed_ball_count_accounting);
  }
  result.audit.complete_closed_ball_count_accounting_certified = true;
  if (closed_ball.extra_shell_count != 0U) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::
            rejected_extra_shell_not_excluded);
  }
  result.audit.no_extra_shell_certified = true;

  for (const ExactSilentPairGatewayStrictInteriorDecision& witness :
       witnesses) {
    if (!witness.exact_strict_interior_certified ||
        !(witness.squared_distance_to_diametral_center <
          input.squared_level)) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayDecision::
              rejected_strict_interior_witnesses);
    }
  }
  result.audit.two_canonical_strict_interior_witnesses_certified = true;

  const auto& target = input.pre_batch_target;
  if (target.order != silent_pair_gateway_order ||
      target.batch_squared_level != input.squared_level ||
      target.pre_batch_component_count == 0U ||
      target.component_handle >= target.pre_batch_component_count ||
      target.pre_batch_prefix_count == 0U ||
      !target.target_frozen_before_equal_level_batch ||
      !target.target_is_nontrivial_reduced_root) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayDecision::rejected_pre_batch_target);
  }

  auto legs = input.strict_legs;
  std::sort(
      legs.begin(),
      legs.end(),
      [](const ExactSilentPairGatewayStrictLegDecision& left,
         const ExactSilentPairGatewayStrictLegDecision& right) {
        return left.removed_support_point_id <
               right.removed_support_point_id;
      });
  const ExactSilentPairGatewayPairIds bridge =
      canonical_point_ids(witness_ids);
  for (std::size_t leg_index = 0U; leg_index < legs.size(); ++leg_index) {
    const ExactSilentPairGatewayStrictLegDecision& leg = legs[leg_index];
    if (leg.removed_support_point_id != pair[leg_index] ||
        canonical_point_ids(leg.strict_facet_point_ids) !=
            expected_strict_facet(
                pair, witness_ids, leg.removed_support_point_id) ||
        canonical_point_ids(leg.bridge_facet_point_ids) != bridge ||
        canonical_point_ids(leg.strict_replacement_coface_point_ids) !=
            expected_strict_coface(
                pair, witness_ids, leg.removed_support_point_id) ||
        !leg.exact_strict_replacement_coface_certified ||
        !leg.exact_path_to_pre_batch_target_certified) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayDecision::rejected_strict_leg_shape);
    }
    if (leg.strict_replacement_coface_squared_level.numerator() <= 0 ||
        !(leg.strict_replacement_coface_squared_level <
          input.squared_level) ||
        !(leg.strict_path_maximum_squared_level <
          input.squared_level) ||
        leg.strict_path_maximum_squared_level <
            leg.strict_replacement_coface_squared_level) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayDecision::rejected_strict_leg_level);
    }
    if (leg.target_component_handle != target.component_handle ||
        leg.pre_batch_snapshot_digest !=
            target.pre_batch_snapshot_digest) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayDecision::rejected_strict_leg_target);
    }
  }
  result.audit.two_strict_bridge_cofaces_certified = true;
  result.audit.both_strict_paths_target_same_frozen_pre_batch_root = true;

  ExactSilentPairGatewayRecord gateway;
  gateway.pair_point_ids = pair;
  gateway.squared_level = input.squared_level;
  gateway.strict_interior_witness_point_ids = witness_ids;
  for (std::size_t witness_index = 0U;
       witness_index < witnesses.size();
       ++witness_index) {
    gateway.strict_interior_witness_squared_distances[witness_index] =
        witnesses[witness_index].squared_distance_to_diametral_center;
    gateway.strict_interior_source_decision_ids[witness_index] =
        witnesses[witness_index].source_decision_id;
  }
  gateway.certified_strict_interior_count =
      closed_ball.strict_interior_count;
  gateway.certified_extra_shell_count = closed_ball.extra_shell_count;
  gateway.certified_exterior_count = closed_ball.exterior_count;
  gateway.closed_ball_source_decision_id =
      closed_ball.source_decision_id;
  for (std::size_t leg_index = 0U; leg_index < legs.size(); ++leg_index) {
    const ExactSilentPairGatewayStrictLegDecision& source = legs[leg_index];
    ExactSilentPairGatewayStrictLegWitness& destination =
        gateway.strict_legs[leg_index];
    destination.removed_support_point_id =
        source.removed_support_point_id;
    destination.strict_facet_point_ids =
        canonical_point_ids(source.strict_facet_point_ids);
    destination.bridge_facet_point_ids =
        canonical_point_ids(source.bridge_facet_point_ids);
    destination.strict_replacement_coface_point_ids =
        canonical_point_ids(source.strict_replacement_coface_point_ids);
    destination.strict_replacement_coface_squared_level =
        source.strict_replacement_coface_squared_level;
    destination.strict_path_maximum_squared_level =
        source.strict_path_maximum_squared_level;
    destination.source_decision_id = source.source_decision_id;
  }
  gateway.target_component_handle = target.component_handle;
  gateway.pre_batch_component_count = target.pre_batch_component_count;
  gateway.pre_batch_prefix_count = target.pre_batch_prefix_count;
  gateway.pre_batch_snapshot_digest = target.pre_batch_snapshot_digest;
  gateway.pre_batch_target_source_decision_id =
      target.source_decision_id;
  gateway.coverage_delta.added_facet_point_ids[0] = pair;
  gateway.coverage_delta.added_facet_count = 1U;
  gateway.coverage_delta.added_point_count = 0U;
  gateway.coverage_delta.exact_pair_facet_only = true;
  gateway.canonical_digest = canonical_gateway_digest(gateway);

  result.gateway = std::move(gateway);
  result.audit.coverage_delta_is_pair_facet_and_zero_points = true;
  result.audit.no_autonomous_node_or_forest_action = true;
  result.decision =
      ExactSilentPairGatewayDecision::
          complete_certified_regular_q3_silent_pair_gateway;
  return result;
}

bool ExactSilentPairGatewayVerification::certified_gateway()
    const noexcept {
  return schema_and_scope_certified &&
         supplied_decision_validation_replayed &&
         canonical_permutation_normalization_replayed &&
         expected_outcome_freshly_rebuilt &&
         canonical_digest_freshly_replayed &&
         coverage_and_target_payload_certified &&
         no_node_prune_completeness_or_public_claim_certified &&
         no_forbidden_global_structure_certified &&
         fresh_replay_certified;
}

bool ExactSilentPairGatewayVerification::
    certified_fail_closed_rejection() const noexcept {
  return schema_and_scope_certified &&
         supplied_decision_validation_replayed &&
         canonical_permutation_normalization_replayed &&
         expected_outcome_freshly_rebuilt &&
         canonical_digest_freshly_replayed &&
         !coverage_and_target_payload_certified &&
         no_node_prune_completeness_or_public_claim_certified &&
         no_forbidden_global_structure_certified &&
         fresh_replay_certified;
}

ExactSilentPairGatewayVerification verify_exact_silent_pair_gateway(
    const ExactSilentPairGatewayInput& input,
    const ExactSilentPairGatewayResult& observed) {
  ExactSilentPairGatewayVerification verification;
  try {
    const ExactSilentPairGatewayResult expected =
        build_exact_silent_pair_gateway(input);
    verification.schema_and_scope_certified =
        observed.schema_version == silent_pair_gateway_schema_version &&
        observed.scope ==
            ExactSilentPairGatewayScope::
                regular_q3_pair_silent_incidence_relative_to_supplied_certified_decisions_only;
    verification.supplied_decision_validation_replayed = true;
    verification.canonical_permutation_normalization_replayed =
        expected.audit.pair_and_witness_permutations_canonicalized ||
        expected.certified_fail_closed_rejection();
    verification.expected_outcome_freshly_rebuilt =
        observed == expected;
    if (expected.gateway.has_value()) {
      verification.canonical_digest_freshly_replayed =
          observed.gateway.has_value() &&
          observed.gateway->canonical_digest ==
              canonical_gateway_digest(*observed.gateway) &&
          observed.gateway->canonical_digest ==
              expected.gateway->canonical_digest;
      verification.coverage_and_target_payload_certified =
          observed.certified_gateway() &&
          coverage_delta_valid(*observed.gateway) &&
          observed.gateway->target_component_handle ==
              input.pre_batch_target.component_handle &&
          observed.gateway->pre_batch_snapshot_digest ==
              input.pre_batch_target.pre_batch_snapshot_digest;
      verification
          .no_node_prune_completeness_or_public_claim_certified =
          observed.certified_gateway() &&
          gateway_negative_claims_valid(*observed.gateway);
    } else {
      verification.canonical_digest_freshly_replayed =
          !observed.gateway.has_value();
      verification.coverage_and_target_payload_certified = false;
      verification
          .no_node_prune_completeness_or_public_claim_certified =
          observed.certified_fail_closed_rejection();
    }
    verification.no_forbidden_global_structure_certified =
        base_audit_valid(observed.audit);
    verification.fresh_replay_certified =
        verification.expected_outcome_freshly_rebuilt;
  } catch (...) {
    return {};
  }
  return verification;
}

}  // namespace morsehgp3d::hierarchy
