#include "morsehgp3d/hierarchy/silent_pair_gateway_gamma2_differential.hpp"

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;
using FacetLabel = std::vector<PointId>;
using FacetSet = std::vector<FacetLabel>;
using RootMap = std::map<std::size_t, FacetSet>;

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

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool bounded_binomial(
    std::size_t point_count,
    std::size_t subset_size,
    std::size_t& result) noexcept {
  if (subset_size > point_count) {
    result = 0U;
    return true;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    if (!checked_multiply(
            value,
            point_count - subset_size + factor,
            value)) {
      return false;
    }
    value /= factor;
  }
  result = value;
  return true;
}

[[nodiscard]] bool valid_budget_caps(
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) noexcept {
  return budget.gamma_budget.maximum_enumerated_facet_count <=
             ExactStrictGammaBudget::maximum_supported_facet_count &&
         budget.gamma_budget.maximum_enumerated_coface_count <=
             ExactStrictGammaBudget::maximum_supported_coface_count &&
         budget.gamma_budget.maximum_union_attempt_count <=
             ExactStrictGammaBudget::maximum_supported_union_attempt_count &&
         budget.maximum_activation_level_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_activation_level_count &&
         budget.maximum_total_facet_work_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_total_facet_work_count &&
         budget.maximum_total_coface_work_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_total_coface_work_count &&
         budget.maximum_total_union_work_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_total_union_work_count &&
         budget.maximum_node_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_node_count &&
         budget.maximum_child_reference_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_child_reference_count &&
         budget.maximum_group_root_reference_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_group_root_reference_count &&
         budget.maximum_group_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_group_count &&
         budget.maximum_group_newly_active_facet_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_group_newly_active_facet_count &&
         budget.maximum_group_equal_level_coface_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_group_equal_level_coface_count &&
         budget.maximum_delta_facet_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_delta_facet_count &&
         budget.maximum_delta_point_reference_count <=
             ExactPersistentReducedGammaOrderHistoryBudget::
                 maximum_supported_delta_point_reference_count;
}

[[nodiscard]] bool budget_covers_complete_order_two_history(
    std::size_t point_count,
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) noexcept {
  if (!valid_budget_caps(budget)) {
    return false;
  }
  std::size_t facet_count = 0U;
  std::size_t coface_count = 0U;
  if (!bounded_binomial(point_count, 2U, facet_count) ||
      !bounded_binomial(point_count, 3U, coface_count)) {
    return false;
  }
  std::size_t union_count = 0U;
  std::size_t level_count = 0U;
  std::size_t replay_count = 0U;
  std::size_t total_facet_work = 0U;
  std::size_t total_coface_work = 0U;
  std::size_t total_union_work = 0U;
  std::size_t point_delta_count = 0U;
  if (!checked_multiply(2U, coface_count, union_count) ||
      !checked_add(facet_count, coface_count, level_count) ||
      !checked_add(level_count, 1U, replay_count) ||
      !checked_multiply(
          replay_count, facet_count, total_facet_work) ||
      !checked_multiply(
          replay_count, coface_count, total_coface_work) ||
      !checked_multiply(
          replay_count, union_count, total_union_work) ||
      !checked_multiply(2U, facet_count, point_delta_count)) {
    return false;
  }
  const std::size_t tree_reference_count =
      coface_count == 0U ? 0U : coface_count - 1U;
  return budget.gamma_budget.maximum_enumerated_facet_count >=
             facet_count &&
         budget.gamma_budget.maximum_enumerated_coface_count >=
             coface_count &&
         budget.gamma_budget.maximum_union_attempt_count >= union_count &&
         budget.maximum_activation_level_count >= level_count &&
         budget.maximum_total_facet_work_count >= total_facet_work &&
         budget.maximum_total_coface_work_count >= total_coface_work &&
         budget.maximum_total_union_work_count >= total_union_work &&
         budget.maximum_node_count >= coface_count &&
         budget.maximum_child_reference_count >= tree_reference_count &&
         budget.maximum_group_root_reference_count >=
             tree_reference_count &&
         budget.maximum_group_count >= level_count &&
         budget.maximum_group_newly_active_facet_count >= facet_count &&
         budget.maximum_group_equal_level_coface_count >= coface_count &&
         budget.maximum_delta_facet_count >= facet_count &&
         budget.maximum_delta_point_reference_count >= point_delta_count;
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
        value,
        "a SilentPairGateway Gamma2 differential size does not fit "
        "uint64"));
  }

  void point_id(PointId point_id) { u64(point_id); }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void level(const exact::ExactLevel& value) {
    text(value.numerator_string());
    text(value.denominator_string());
  }

  void canonical_id(const contract::CanonicalId& value) {
    builder_.update(value.bytes());
  }

  template <typename Range>
  void point_ids(const Range& point_ids) {
    size(point_ids.size());
    for (const PointId point_id_value : point_ids) {
      point_id(point_id_value);
    }
  }

  void facets(const FacetSet& facets_value) {
    size(facets_value.size());
    for (const FacetLabel& facet : facets_value) {
      point_ids(facet);
    }
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

template <std::size_t Size>
[[nodiscard]] std::array<PointId, Size> canonical_ids(
    std::array<PointId, Size> ids) {
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[nodiscard]] FacetLabel as_facet(
    const ExactSilentPairGatewayPairIds& ids) {
  return {ids.begin(), ids.end()};
}

[[nodiscard]] FacetLabel as_facet(
    const ExactSilentPairGatewayCofaceIds& ids) {
  return {ids.begin(), ids.end()};
}

[[nodiscard]] bool valid_pair(
    const ExactSilentPairGatewayPairIds& pair,
    std::size_t point_count) noexcept {
  return pair[0] < pair[1] &&
         pair[1] < static_cast<PointId>(point_count);
}

[[nodiscard]] bool contains_facet(
    const FacetSet& facets,
    const FacetLabel& facet) {
  return std::binary_search(facets.begin(), facets.end(), facet);
}

[[nodiscard]] bool coface_contains_pair(
    std::span<const PointId> coface,
    const ExactSilentPairGatewayPairIds& pair) {
  return std::binary_search(coface.begin(), coface.end(), pair[0]) &&
         std::binary_search(coface.begin(), coface.end(), pair[1]);
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational difference =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance =
        squared_distance + difference * difference;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] std::vector<PointId> covered_points(
    const FacetSet& facets) {
  std::set<PointId> points;
  for (const FacetLabel& facet : facets) {
    points.insert(facet.begin(), facet.end());
  }
  return {points.begin(), points.end()};
}

[[nodiscard]] contract::CanonicalId cloud_digest(
    const spatial::CanonicalPointCloud& cloud) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/cloud/v1"};
  writer.size(cloud.size());
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    writer.point_id(static_cast<PointId>(index));
    for (const std::uint64_t bits :
         cloud.point(static_cast<PointId>(index)).canonical_input_bits()) {
      writer.u64(bits);
    }
  }
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId closed_ball_digest(
    const contract::CanonicalId& source_cloud_digest,
    const ExactSilentPairGatewayPairIds& pair,
    const ExactFacetMiniballResult& pair_miniball,
    std::span<const PointId> interiors,
    std::span<const PointId> extra_shell,
    std::span<const PointId> exteriors) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/closed-ball/v1"};
  writer.canonical_id(source_cloud_digest);
  writer.point_ids(pair);
  writer.level(pair_miniball.squared_radius);
  writer.point_ids(pair_miniball.support_point_ids);
  writer.point_ids(interiors);
  writer.point_ids(extra_shell);
  writer.point_ids(exteriors);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId witness_digest(
    const contract::CanonicalId& closed_ball_source,
    PointId witness,
    const exact::ExactLevel& squared_distance) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/interior/v1"};
  writer.canonical_id(closed_ball_source);
  writer.point_id(witness);
  writer.level(squared_distance);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId snapshot_digest(
    const contract::CanonicalId& source_cloud_digest,
    std::size_t prefix_count,
    const exact::ExactLevel& level,
    const RootMap& roots) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/snapshot/v1"};
  writer.canonical_id(source_cloud_digest);
  writer.size(prefix_count);
  writer.level(level);
  writer.size(roots.size());
  for (const auto& [root_id, facets] : roots) {
    writer.size(root_id);
    writer.facets(facets);
  }
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId leg_digest(
    const contract::CanonicalId& source_cloud_digest,
    const ExactSilentPairGatewayGamma2LegReplay& leg,
    const contract::CanonicalId& source_snapshot_digest) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/strict-leg/v1"};
  writer.canonical_id(source_cloud_digest);
  writer.point_id(leg.removed_support_point_id);
  writer.point_ids(leg.strict_facet_point_ids);
  writer.point_ids(leg.bridge_facet_point_ids);
  writer.point_ids(leg.strict_replacement_coface_point_ids);
  writer.level(
      leg.strict_replacement_coface_miniball.squared_radius);
  writer.level(leg.strict_path_maximum_squared_level);
  writer.size(leg.strict_gamma_component_index);
  writer.canonical_id(source_snapshot_digest);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId target_digest(
    const contract::CanonicalId& source_snapshot_digest,
    std::size_t root_id,
    const FacetSet& facets) {
  DigestWriter writer{
      "MorseHGP3D/SilentPairGatewayGamma2Differential/target/v1"};
  writer.canonical_id(source_snapshot_digest);
  writer.size(root_id);
  writer.facets(facets);
  return writer.finalize();
}

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialResult base_result(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair,
    ExactSilentPairGatewayGamma2DifferentialBudget budget) {
  ExactSilentPairGatewayGamma2DifferentialResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.pair_point_ids = canonical_ids(pair);
  result.scope =
      ExactSilentPairGatewayGamma2DifferentialScope::
          bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only;
  result.counters.preflight_count = 1U;
  return result;
}

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialResult reject(
    ExactSilentPairGatewayGamma2DifferentialResult result,
    ExactSilentPairGatewayGamma2DifferentialDecision decision) {
  result.gateway_input.reset();
  result.gateway_result.reset();
  result.compact_gateway_freshly_certified = false;
  result.decision = decision;
  return result;
}

[[nodiscard]] bool pair_miniball_is_unique_diametral(
    const ExactFacetMiniballResult& miniball,
    const ExactSilentPairGatewayPairIds& pair) {
  return miniball.status ==
             ExactFacetMiniballStatus::exact_facet_miniball_certified &&
         miniball.scope ==
             ExactFacetMiniballScope::local_facet_miniball_only &&
         miniball.facet_point_ids == as_facet(pair) &&
         miniball.support_point_ids == as_facet(pair) &&
         miniball.strictly_inside_point_ids.empty() &&
         miniball.boundary_point_ids == as_facet(pair) &&
         miniball.squared_radius.numerator() > 0 &&
         miniball.counters.optimal_support_count == 1U &&
         miniball.counters.selected_support_size == 2U;
}

[[nodiscard]] bool canonical_facet_set(FacetSet& facets) {
  for (FacetLabel& facet : facets) {
    if (!std::is_sorted(facet.begin(), facet.end()) ||
        std::adjacent_find(facet.begin(), facet.end()) !=
            facet.end()) {
      return false;
    }
  }
  std::sort(facets.begin(), facets.end());
  return std::adjacent_find(facets.begin(), facets.end()) ==
         facets.end();
}

struct HistoryReplay {
  bool valid{false};
  std::vector<RootMap> roots_before_batches;
  std::vector<RootMap> roots_after_batches;
};

[[nodiscard]] HistoryReplay replay_history_roots(
    const ExactPersistentReducedGammaOrderHistory& history) {
  HistoryReplay replay;
  replay.roots_before_batches.reserve(history.batch_metadata.size());
  replay.roots_after_batches.reserve(history.batch_metadata.size());
  RootMap active_roots;
  for (std::size_t batch_index = 0U;
       batch_index < history.batch_metadata.size();
       ++batch_index) {
    const ExactPersistentReducedGammaBatchMetadata& metadata =
        history.batch_metadata[batch_index];
    if (metadata.batch_index != batch_index ||
        metadata.first_group_record_index >
            history.group_records.size() ||
        metadata.group_record_count >
            history.group_records.size() -
                metadata.first_group_record_index ||
        metadata.active_root_count_before != active_roots.size()) {
      return replay;
    }
    const RootMap snapshot = active_roots;
    replay.roots_before_batches.push_back(snapshot);
    std::set<std::size_t> consumed_roots;
    RootMap pending_roots;
    for (std::size_t offset = 0U;
         offset < metadata.group_record_count;
         ++offset) {
      const std::size_t record_index =
          metadata.first_group_record_index + offset;
      const ExactPersistentReducedGammaHistoryGroupRecord& record =
          history.group_records[record_index];
      if (record.group_record_index != record_index ||
          record.batch_index != batch_index ||
          record.batch_group_index != offset ||
          record.squared_level != metadata.squared_level) {
        return replay;
      }
      if (record.kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
        if (!record.prior_root_node_ids.empty() ||
            record.resulting_root_node_id.has_value() ||
            record.created_node_id.has_value() ||
            record.coverage_delta.has_value()) {
          return replay;
        }
        continue;
      }
      if (!record.resulting_root_node_id.has_value() ||
          !record.coverage_delta.has_value()) {
        return replay;
      }
      FacetSet resulting_facets;
      FacetSet prior_facets;
      for (const std::size_t prior_root :
           record.prior_root_node_ids) {
        const auto found = snapshot.find(prior_root);
        if (found == snapshot.end() ||
            !consumed_roots.insert(prior_root).second) {
          return replay;
        }
        resulting_facets.insert(
            resulting_facets.end(),
            found->second.begin(),
            found->second.end());
        prior_facets.insert(
            prior_facets.end(),
            found->second.begin(),
            found->second.end());
      }
      resulting_facets.insert(
          resulting_facets.end(),
          record.coverage_delta->added_facet_point_ids.begin(),
          record.coverage_delta->added_facet_point_ids.end());
      if (!canonical_facet_set(prior_facets) ||
          !canonical_facet_set(resulting_facets)) {
        return replay;
      }
      const std::vector<PointId> prior_points =
          covered_points(prior_facets);
      const std::vector<PointId> resulting_points =
          covered_points(resulting_facets);
      std::vector<PointId> expected_added_points;
      std::set_difference(
          resulting_points.begin(),
          resulting_points.end(),
          prior_points.begin(),
          prior_points.end(),
          std::back_inserter(expected_added_points));
      if (record.coverage_delta->added_point_ids !=
              expected_added_points ||
          !pending_roots
               .emplace(
                   *record.resulting_root_node_id,
                   std::move(resulting_facets))
               .second) {
        return replay;
      }
    }
    for (const std::size_t consumed_root : consumed_roots) {
      active_roots.erase(consumed_root);
    }
    for (auto& [root_id, facets] : pending_roots) {
      if (!active_roots.emplace(root_id, std::move(facets)).second) {
        return replay;
      }
    }
    if (metadata.active_root_count_after != active_roots.size()) {
      return replay;
    }
    replay.roots_after_batches.push_back(active_roots);
  }

  if (active_roots.size() != history.final_active_roots.size()) {
    return replay;
  }
  for (const ExactPersistentReducedGammaActiveRoot& root :
       history.final_active_roots) {
    const auto found = active_roots.find(root.root_node_id);
    if (found == active_roots.end() ||
        found->second != root.facet_point_ids ||
        covered_points(found->second) != root.covered_point_ids) {
      return replay;
    }
  }
  replay.valid = true;
  return replay;
}

[[nodiscard]] std::optional<std::size_t> unique_root_with_facets(
    const RootMap& roots,
    const FacetSet& facets) {
  std::optional<std::size_t> result;
  for (const auto& [root_id, root_facets] : roots) {
    if (root_facets == facets) {
      if (result.has_value()) {
        return std::nullopt;
      }
      result = root_id;
    }
  }
  return result;
}

[[nodiscard]] std::optional<std::size_t> unique_root_containing_pair(
    const RootMap& roots,
    const FacetLabel& pair) {
  std::optional<std::size_t> result;
  for (const auto& [root_id, facets] : roots) {
    if (contains_facet(facets, pair)) {
      if (result.has_value()) {
        return std::nullopt;
      }
      result = root_id;
    }
  }
  return result;
}

[[nodiscard]] bool exact_pair_only_delta(
    const ExactReducedGammaCoverageDelta& delta,
    const FacetLabel& pair) {
  return delta.added_facet_point_ids == FacetSet{pair} &&
         delta.added_point_ids.empty() && !delta.fully_redundant;
}

[[nodiscard]] bool exact_pair_only_delta(
    const ExactSilentPairGatewayCoverageDelta& delta,
    const ExactSilentPairGatewayPairIds& pair) noexcept {
  return delta.added_facet_count == 1U &&
         delta.added_facet_point_ids[0] == pair &&
         delta.added_point_count == 0U &&
         delta.added_point_ids.empty() &&
         delta.exact_pair_facet_only;
}

[[nodiscard]] std::optional<std::size_t> component_index_by_facets(
    const std::vector<ExactStrictGammaComponentWitness>& components,
    const FacetSet& target) {
  std::optional<std::size_t> result;
  for (std::size_t index = 0U; index < components.size(); ++index) {
    if (components[index].facet_point_ids == target) {
      if (result.has_value()) {
        return std::nullopt;
      }
      result = index;
    }
  }
  return result;
}

[[nodiscard]] bool no_forbidden_claim(
    const ExactSilentPairGatewayGamma2DifferentialResult& result) {
  if (!result.no_global_product_delaunay_or_public_claim) {
    return false;
  }
  if (!result.gateway_result.has_value()) {
    return true;
  }
  if (!result.gateway_result->gateway.has_value()) {
    return !result.gateway_result->certified_gateway();
  }
  const ExactSilentPairGatewayRecord& gateway =
      *result.gateway_result->gateway;
  return !gateway.autonomous_node_created &&
         !gateway.root_birth_published &&
         !gateway.merge_node_published &&
         !gateway.gateway_attach_published &&
         !gateway.gamma2_prune_authority_claimed &&
         !gateway.gamma2_completeness_claimed &&
         !gateway.public_hierarchy_exactness_claimed &&
         !gateway.public_status_claimed;
}

[[nodiscard]] ExactSilentPairGatewayGamma2DifferentialResult
compute_exact_silent_pair_gateway_gamma2_differential(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair_point_ids,
    ExactSilentPairGatewayGamma2DifferentialBudget budget) {
  ExactSilentPairGatewayGamma2DifferentialResult result =
      base_result(cloud, pair_point_ids, budget);
  const ExactSilentPairGatewayPairIds pair = result.pair_point_ids;
  if (cloud.size() <
          ExactSilentPairGatewayGamma2DifferentialResult::
              minimum_supported_point_count ||
      cloud.size() >
          ExactSilentPairGatewayGamma2DifferentialResult::
              maximum_supported_point_count ||
      !valid_pair(pair, cloud.size())) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_point_count_or_pair);
  }
  if (!budget_covers_complete_order_two_history(
          cloud.size(), budget.history_budget)) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            no_differential_preflight_budget_insufficient);
  }
  result.candidate_space_preflight_certified = true;
  result.canonical_cloud_digest = cloud_digest(cloud);

  const FacetLabel pair_facet = as_facet(pair);
  result.pair_miniball =
      build_exact_facet_miniball(cloud, pair_facet);
  ++result.counters.pair_miniball_build_count;
  // Every exact subordinate builder performs its own fresh verifier before
  // returning. Count that owned replay without immediately repeating it a
  // third time inside this differential.
  ++result.counters.pair_miniball_verification_count;
  result.pair_miniball_freshly_certified =
      result.pair_miniball->status ==
      ExactFacetMiniballStatus::exact_facet_miniball_certified;
  if (!result.pair_miniball_freshly_certified ||
      !pair_miniball_is_unique_diametral(*result.pair_miniball, pair)) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_nonunique_essential_diametral_support);
  }
  result.unique_essential_diametral_support_certified = true;

  const spatial::ClosedBallPartition partition =
      spatial::brute_force_closed_ball(
          cloud,
          result.pair_miniball->center,
          result.pair_miniball->squared_radius);
  ++result.counters.closed_ball_query_count;
  result.counters.closed_ball_distance_evaluation_count =
      partition.distance_evaluation_count();
  result.strict_interior_point_ids.assign(
      partition.interior_ids().begin(),
      partition.interior_ids().end());
  result.exterior_point_ids.assign(
      partition.exterior_ids().begin(),
      partition.exterior_ids().end());
  for (const PointId shell_id : partition.shell_ids()) {
    if (!std::binary_search(pair.begin(), pair.end(), shell_id)) {
      result.extra_shell_point_ids.push_back(shell_id);
    }
  }
  std::size_t classified_count = 0U;
  result.complete_closed_ball_partition_certified =
      partition.validated_for(cloud) && partition.partition_complete() &&
      partition.squared_radius() ==
          result.pair_miniball->squared_radius &&
      checked_add(
          result.strict_interior_point_ids.size(),
          partition.shell_ids().size(),
          classified_count) &&
      checked_add(
          classified_count,
          result.exterior_point_ids.size(),
          classified_count) &&
      classified_count == cloud.size() &&
      partition.shell_ids().size() ==
          pair.size() + result.extra_shell_point_ids.size() &&
      std::binary_search(
          partition.shell_ids().begin(),
          partition.shell_ids().end(),
          pair[0]) &&
      std::binary_search(
          partition.shell_ids().begin(),
          partition.shell_ids().end(),
          pair[1]);
  if (!result.complete_closed_ball_partition_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_nonunique_essential_diametral_support);
  }
  if (!result.extra_shell_point_ids.empty()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_extra_shell_not_excluded);
  }
  result.no_extra_shell_certified = true;
  if (result.strict_interior_point_ids.size() < 2U) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_fewer_than_two_strict_interiors);
  }
  result.canonical_strict_interior_witness_point_ids = {
      result.strict_interior_point_ids[0],
      result.strict_interior_point_ids[1]};
  result.canonical_two_strict_interiors_certified = true;

  const std::array<PointId, 2U>& witnesses =
      result.canonical_strict_interior_witness_point_ids;
  const ExactSilentPairGatewayPairIds bridge =
      canonical_ids(
          ExactSilentPairGatewayPairIds{witnesses[0], witnesses[1]});
  std::vector<FacetLabel> gamma_sources;
  gamma_sources.reserve(3U);
  result.strict_leg_replays.reserve(2U);
  for (std::size_t leg_index = 0U; leg_index < pair.size();
       ++leg_index) {
    const PointId removed = pair[leg_index];
    const PointId retained = pair[1U - leg_index];
    ExactSilentPairGatewayGamma2LegReplay leg;
    leg.removed_support_point_id = removed;
    leg.strict_facet_point_ids =
        canonical_ids(
            ExactSilentPairGatewayPairIds{retained, witnesses[0]});
    leg.bridge_facet_point_ids = bridge;
    leg.strict_replacement_coface_point_ids =
        canonical_ids(ExactSilentPairGatewayCofaceIds{
            retained, witnesses[0], witnesses[1]});
    const FacetLabel replacement =
        as_facet(leg.strict_replacement_coface_point_ids);
    leg.strict_replacement_coface_miniball =
        build_exact_facet_miniball(cloud, replacement);
    ++result.counters.replacement_coface_miniball_build_count;
    ++result.counters.replacement_coface_miniball_verification_count;
    if (leg.strict_replacement_coface_miniball.status !=
            ExactFacetMiniballStatus::
                exact_facet_miniball_certified ||
        leg.strict_replacement_coface_miniball.squared_radius
                .numerator() <= 0 ||
        !(leg.strict_replacement_coface_miniball.squared_radius <
          result.pair_miniball->squared_radius)) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayGamma2DifferentialDecision::
              rejected_zero_or_nonstrict_replacement_coface);
    }
    leg.strict_path_facet_point_ids = {
        as_facet(leg.strict_facet_point_ids),
        as_facet(leg.bridge_facet_point_ids)};
    leg.strict_path_coface_point_ids = {replacement};
    leg.strict_path_maximum_squared_level =
        leg.strict_replacement_coface_miniball.squared_radius;
    result.counters.strict_path_facet_visit_count += 2U;
    ++result.counters.strict_path_coface_visit_count;
    gamma_sources.push_back(as_facet(leg.strict_facet_point_ids));
    result.strict_leg_replays.push_back(std::move(leg));
  }
  gamma_sources.push_back(as_facet(bridge));
  result.two_strict_replacement_cofaces_freshly_certified = true;

  result.strict_gamma =
      build_exact_strict_gamma_source_classification(
          cloud,
          2U,
          result.pair_miniball->squared_radius,
          gamma_sources,
          budget.history_budget.gamma_budget);
  ++result.counters.strict_gamma_build_count;
  ++result.counters.strict_gamma_verification_count;
  result.strict_gamma_sources_freshly_certified =
      result.strict_gamma->decision ==
          ExactStrictGammaDecision::
              complete_all_sources_active_and_classified &&
      result.strict_gamma->complete_source_classification_certified &&
      result.strict_gamma->all_sources_active_and_classified &&
      result.strict_gamma->source_classifications.size() ==
          gamma_sources.size();
  if (!result.strict_gamma_sources_freshly_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_incomplete_strict_gamma_sources);
  }

  std::optional<std::size_t> target_component;
  for (const ExactStrictGammaSourceClassification& classification :
       result.strict_gamma->source_classifications) {
    if (!classification.active_strictly_below_cut ||
        !classification.component_index.has_value()) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayGamma2DifferentialDecision::
              rejected_incomplete_strict_gamma_sources);
    }
    if (!target_component.has_value()) {
      target_component = classification.component_index;
    } else if (target_component != classification.component_index) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayGamma2DifferentialDecision::
              rejected_divergent_pre_batch_targets);
    }
  }
  if (!target_component.has_value() ||
      *target_component >= result.strict_gamma->components.size()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_divergent_pre_batch_targets);
  }
  const FacetSet& target_component_facets =
      result.strict_gamma->components[*target_component].facet_point_ids;
  if (target_component_facets.size() <= 1U) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_trivial_pre_batch_target);
  }
  result.target_strict_gamma_component_index = target_component;
  result.both_strict_legs_reach_same_component = true;
  for (ExactSilentPairGatewayGamma2LegReplay& leg :
       result.strict_leg_replays) {
    leg.strict_gamma_component_index = *target_component;
  }

  result.reduced_gamma_batch = build_exact_reduced_gamma_batch(
      cloud,
      2U,
      result.pair_miniball->squared_radius,
      budget.history_budget.gamma_budget);
  ++result.counters.reduced_gamma_batch_build_count;
  ++result.counters.reduced_gamma_batch_verification_count;
  result.reduced_equal_level_batch_freshly_certified =
      result.reduced_gamma_batch->decision ==
          ExactReducedGammaBatchDecision::
              complete_exhaustive_reduced_gamma_batch &&
      result.reduced_gamma_batch->equal_level_batch_semantics_certified;
  if (!result.reduced_equal_level_batch_freshly_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_equal_level_gamma2_effect);
  }

  std::vector<ExactSilentPairGatewayCofaceIds> expected_pair_cofaces;
  expected_pair_cofaces.reserve(result.strict_interior_point_ids.size());
  for (const PointId interior : result.strict_interior_point_ids) {
    expected_pair_cofaces.push_back(
        canonical_ids(ExactSilentPairGatewayCofaceIds{
            pair[0], pair[1], interior}));
  }
  std::sort(
      expected_pair_cofaces.begin(), expected_pair_cofaces.end());
  for (const ExactStrictGammaCofaceWitness& coface :
       result.reduced_gamma_batch->gamma_transition
           .equal_level_cofaces) {
    if (coface.coface_point_ids.size() == 3U &&
        coface_contains_pair(coface.coface_point_ids, pair)) {
      result.exhaustive_pair_equal_level_coface_point_ids.push_back(
          ExactSilentPairGatewayCofaceIds{
              coface.coface_point_ids[0],
              coface.coface_point_ids[1],
              coface.coface_point_ids[2]});
    }
  }
  std::sort(
      result.exhaustive_pair_equal_level_coface_point_ids.begin(),
      result.exhaustive_pair_equal_level_coface_point_ids.end());
  result.counters.exhaustive_pair_equal_level_coface_count =
      result.exhaustive_pair_equal_level_coface_point_ids.size();
  result.all_pair_interior_cofaces_exhaustively_reconciled =
      result.exhaustive_pair_equal_level_coface_point_ids ==
      expected_pair_cofaces;
  if (!result.all_pair_interior_cofaces_exhaustively_reconciled) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_equal_level_gamma2_effect);
  }

  const auto target_batch_component = component_index_by_facets(
      result.reduced_gamma_batch->gamma_transition.strict_gamma
          .components,
      target_component_facets);
  if (!target_batch_component.has_value()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_divergent_pre_batch_targets);
  }
  const auto target_classification = std::find_if(
      result.reduced_gamma_batch->strict_component_classifications.begin(),
      result.reduced_gamma_batch->strict_component_classifications.end(),
      [&](const ExactReducedGammaStrictComponentClassification&
              classification) {
        return classification.strict_component_index ==
               *target_batch_component;
      });
  if (target_classification ==
          result.reduced_gamma_batch->strict_component_classifications
              .end() ||
      !target_classification->carries_prior_reduced_root ||
      target_classification->kind !=
          ExactReducedGammaStrictComponentKind::
              prior_nontrivial_reduced_root) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_trivial_pre_batch_target);
  }

  std::optional<std::size_t> pair_group_index;
  for (std::size_t group_index = 0U;
       group_index < result.reduced_gamma_batch->groups.size();
       ++group_index) {
    const ExactReducedGammaBatchGroup& group =
        result.reduced_gamma_batch->groups[group_index];
    if (contains_facet(group.newly_active_facet_point_ids, pair_facet)) {
      if (pair_group_index.has_value()) {
        return reject(
            std::move(result),
            ExactSilentPairGatewayGamma2DifferentialDecision::
                rejected_equal_level_gamma2_effect);
      }
      pair_group_index = group_index;
    }
  }
  if (!pair_group_index.has_value()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_equal_level_gamma2_effect);
  }
  const ExactReducedGammaBatchGroup& pair_group =
      result.reduced_gamma_batch->groups[*pair_group_index];
  if (pair_group.kind !=
          ExactReducedGammaBatchGroupKind::continuation ||
      pair_group.prior_reduced_root_strict_component_indices !=
          std::vector<std::size_t>{*target_batch_component} ||
      !pair_group.coverage_delta.has_value() ||
      !exact_pair_only_delta(*pair_group.coverage_delta, pair_facet) ||
      pair_group.newly_active_facet_point_ids != FacetSet{pair_facet}) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_equal_level_gamma2_effect);
  }
  result.equal_level_effect_is_pair_only_zero_points_continuation = true;

  result.persistent_gamma_history =
      build_exact_persistent_reduced_gamma_order_history(
          cloud, 2U, budget.history_budget);
  ++result.counters.persistent_gamma_history_build_count;
  ++result.counters.persistent_gamma_history_verification_count;
  result.persistent_gamma_history_freshly_certified =
      result.persistent_gamma_history->decision ==
          ExactPersistentReducedGammaOrderHistoryDecision::
              complete_persistent_reduced_gamma_history &&
      result.persistent_gamma_history
          ->persistent_reduced_gamma_history_certified;
  if (!result.persistent_gamma_history_freshly_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }

  std::optional<std::size_t> pair_batch_index;
  for (std::size_t batch_index = 0U;
       batch_index <
       result.persistent_gamma_history->batch_metadata.size();
       ++batch_index) {
    if (result.persistent_gamma_history
            ->batch_metadata[batch_index]
            .squared_level ==
        result.pair_miniball->squared_radius) {
      if (pair_batch_index.has_value()) {
        return reject(
            std::move(result),
            ExactSilentPairGatewayGamma2DifferentialDecision::
                rejected_persistent_gamma2_history);
      }
      pair_batch_index = batch_index;
    }
  }
  if (!pair_batch_index.has_value() || *pair_batch_index == 0U) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }
  const HistoryReplay replay =
      replay_history_roots(*result.persistent_gamma_history);
  if (!replay.valid ||
      replay.roots_before_batches.size() <= *pair_batch_index ||
      replay.roots_after_batches.size() <= *pair_batch_index) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }
  const RootMap& pre_batch_roots =
      replay.roots_before_batches[*pair_batch_index];
  result.counters.pre_batch_root_count = pre_batch_roots.size();
  const std::optional<std::size_t> target_root =
      unique_root_with_facets(pre_batch_roots, target_component_facets);
  if (!target_root.has_value()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_divergent_pre_batch_targets);
  }
  const auto target_position = pre_batch_roots.find(*target_root);
  result.target_pre_batch_component_handle =
      static_cast<std::size_t>(
          std::distance(pre_batch_roots.begin(), target_position));
  result.target_prior_root_node_id = target_root;
  result.target_pre_batch_root_facet_point_ids =
      target_position->second;
  result.pre_batch_prefix_count = *pair_batch_index;
  result.pre_batch_snapshot_digest = snapshot_digest(
      result.canonical_cloud_digest,
      result.pre_batch_prefix_count,
      result.pair_miniball->squared_radius,
      pre_batch_roots);
  result.target_is_frozen_nontrivial_pre_batch_root =
      pre_batch_roots.size() > 0U &&
      target_position->second.size() > 1U;
  if (!result.target_is_frozen_nontrivial_pre_batch_root) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_trivial_pre_batch_target);
  }

  const ExactPersistentReducedGammaBatchMetadata& pair_metadata =
      result.persistent_gamma_history
          ->batch_metadata[*pair_batch_index];
  std::optional<std::size_t> pair_record_index;
  for (std::size_t offset = 0U;
       offset < pair_metadata.group_record_count;
       ++offset) {
    const std::size_t record_index =
        pair_metadata.first_group_record_index + offset;
    const ExactPersistentReducedGammaHistoryGroupRecord& record =
        result.persistent_gamma_history->group_records[record_index];
    if (record.batch_group_index == *pair_group_index) {
      if (pair_record_index.has_value()) {
        return reject(
            std::move(result),
            ExactSilentPairGatewayGamma2DifferentialDecision::
                rejected_persistent_gamma2_history);
      }
      pair_record_index = record_index;
    }
  }
  if (!pair_record_index.has_value()) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }
  const ExactPersistentReducedGammaHistoryGroupRecord& pair_record =
      result.persistent_gamma_history
          ->group_records[*pair_record_index];
  if (pair_record.kind !=
          ExactReducedGammaBatchGroupKind::continuation ||
      pair_record.prior_root_node_ids !=
          std::vector<std::size_t>{*target_root} ||
      pair_record.resulting_root_node_id != target_root ||
      pair_record.created_node_id.has_value() ||
      !pair_record.coverage_delta.has_value() ||
      !exact_pair_only_delta(
          *pair_record.coverage_delta, pair_facet) ||
      pair_record.newly_active_facet_point_ids !=
          FacetSet{pair_facet}) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }
  result.pair_history_batch_index = pair_batch_index;
  result.pair_history_group_record_index = pair_record_index;
  result.pair_history_creates_no_node = true;

  for (std::size_t batch_index = *pair_batch_index;
       batch_index < replay.roots_after_batches.size();
       ++batch_index) {
    const RootMap& roots = replay.roots_after_batches[batch_index];
    const std::optional<std::size_t> root =
        unique_root_containing_pair(roots, pair_facet);
    if (!root.has_value()) {
      return reject(
          std::move(result),
          ExactSilentPairGatewayGamma2DifferentialDecision::
              rejected_persistent_gamma2_history);
    }
    const FacetSet& facets = roots.at(*root);
    result.later_root_trace.push_back(
        ExactSilentPairGatewayGamma2LaterRootWitness{
            batch_index,
            result.persistent_gamma_history
                ->batch_metadata[batch_index]
                .squared_level,
            *root,
            facets.size(),
            covered_points(facets).size()});
  }
  result.counters.later_history_batch_count =
      result.later_root_trace.size();
  result.pair_persists_through_every_later_gamma2_batch =
      !result.later_root_trace.empty();

  const contract::CanonicalId closed_source = closed_ball_digest(
      result.canonical_cloud_digest,
      pair,
      *result.pair_miniball,
      result.strict_interior_point_ids,
      result.extra_shell_point_ids,
      result.exterior_point_ids);
  ExactSilentPairGatewayInput gateway_input;
  gateway_input.point_count = cloud.size();
  gateway_input.pair_point_ids = pair;
  gateway_input.squared_level =
      result.pair_miniball->squared_radius;
  gateway_input.closed_ball.strict_interior_count =
      result.strict_interior_point_ids.size();
  gateway_input.closed_ball.extra_shell_count =
      result.extra_shell_point_ids.size();
  gateway_input.closed_ball.exterior_count =
      result.exterior_point_ids.size();
  gateway_input.closed_ball.source_decision_id = closed_source;
  gateway_input.closed_ball.exact_diametral_pair_level_certified = true;
  gateway_input.closed_ball
      .unique_essential_diametral_support_certified = true;
  gateway_input.closed_ball
      .complete_closed_ball_partition_certified = true;
  gateway_input.closed_ball
      .canonical_two_witness_selection_certified = true;
  for (std::size_t witness_index = 0U;
       witness_index < witnesses.size();
       ++witness_index) {
    const PointId witness = witnesses[witness_index];
    const exact::ExactLevel squared_distance =
        exact_squared_distance(
            result.pair_miniball->center,
            cloud.point(witness).exact());
    gateway_input.strict_interior_witnesses[witness_index] = {
        witness,
        squared_distance,
        witness_digest(closed_source, witness, squared_distance),
        true};
  }
  for (std::size_t leg_index = 0U;
       leg_index < result.strict_leg_replays.size();
       ++leg_index) {
    const ExactSilentPairGatewayGamma2LegReplay& source =
        result.strict_leg_replays[leg_index];
    gateway_input.strict_legs[leg_index] = {
        source.removed_support_point_id,
        source.strict_facet_point_ids,
        source.bridge_facet_point_ids,
        source.strict_replacement_coface_point_ids,
        source.strict_replacement_coface_miniball.squared_radius,
        source.strict_path_maximum_squared_level,
        *result.target_pre_batch_component_handle,
        result.pre_batch_snapshot_digest,
        leg_digest(
            result.canonical_cloud_digest,
            source,
            result.pre_batch_snapshot_digest),
        true,
        true};
  }
  gateway_input.pre_batch_target = {
      2U,
      result.pair_miniball->squared_radius,
      *result.target_pre_batch_component_handle,
      pre_batch_roots.size(),
      result.pre_batch_prefix_count,
      result.pre_batch_snapshot_digest,
      target_digest(
          result.pre_batch_snapshot_digest,
          *target_root,
          target_component_facets),
      true,
      true};
  result.gateway_input = gateway_input;
  result.gateway_result =
      build_exact_silent_pair_gateway(gateway_input);
  ++result.counters.gateway_build_count;
  ++result.counters.gateway_verification_count;
  result.compact_gateway_freshly_certified =
      result.gateway_result->certified_gateway() &&
      result.gateway_result->gateway.has_value() &&
      exact_pair_only_delta(
          result.gateway_result->gateway->coverage_delta, pair) &&
      !result.gateway_result->gateway->autonomous_node_created;
  if (!result.compact_gateway_freshly_certified) {
    return reject(
        std::move(result),
        ExactSilentPairGatewayGamma2DifferentialDecision::
            rejected_persistent_gamma2_history);
  }

  result.decision =
      ExactSilentPairGatewayGamma2DifferentialDecision::
          complete_exact_regular_silent_pair_gateway_gamma2_differential;
  return result;
}

[[nodiscard]] bool geometric_projection_equal(
    const ExactSilentPairGatewayGamma2DifferentialResult& left,
    const ExactSilentPairGatewayGamma2DifferentialResult& right) {
  return left.canonical_cloud_digest == right.canonical_cloud_digest &&
         left.pair_miniball == right.pair_miniball &&
         left.strict_interior_point_ids ==
             right.strict_interior_point_ids &&
         left.extra_shell_point_ids == right.extra_shell_point_ids &&
         left.exterior_point_ids == right.exterior_point_ids &&
         left.canonical_strict_interior_witness_point_ids ==
             right.canonical_strict_interior_witness_point_ids &&
         left.strict_leg_replays == right.strict_leg_replays &&
         left.pair_miniball_freshly_certified ==
             right.pair_miniball_freshly_certified &&
         left.unique_essential_diametral_support_certified ==
             right.unique_essential_diametral_support_certified &&
         left.complete_closed_ball_partition_certified ==
             right.complete_closed_ball_partition_certified &&
         left.no_extra_shell_certified ==
             right.no_extra_shell_certified &&
         left.canonical_two_strict_interiors_certified ==
             right.canonical_two_strict_interiors_certified &&
         left.two_strict_replacement_cofaces_freshly_certified ==
             right.two_strict_replacement_cofaces_freshly_certified;
}

}  // namespace

bool ExactSilentPairGatewayGamma2DifferentialResult::
    certified_differential() const noexcept {
  return schema_version ==
             silent_pair_gateway_gamma2_differential_schema_version &&
         decision ==
             ExactSilentPairGatewayGamma2DifferentialDecision::
                 complete_exact_regular_silent_pair_gateway_gamma2_differential &&
         scope ==
             ExactSilentPairGatewayGamma2DifferentialScope::
                 bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only &&
         point_count >= minimum_supported_point_count &&
         point_count <= maximum_supported_point_count &&
         valid_pair(pair_point_ids, point_count) &&
         candidate_space_preflight_certified &&
         pair_miniball.has_value() &&
         pair_miniball_freshly_certified &&
         unique_essential_diametral_support_certified &&
         complete_closed_ball_partition_certified &&
         no_extra_shell_certified &&
         canonical_two_strict_interiors_certified &&
         all_pair_interior_cofaces_exhaustively_reconciled &&
         two_strict_replacement_cofaces_freshly_certified &&
         strict_gamma.has_value() &&
         strict_gamma_sources_freshly_certified &&
         both_strict_legs_reach_same_component &&
         target_is_frozen_nontrivial_pre_batch_root &&
         reduced_gamma_batch.has_value() &&
         reduced_equal_level_batch_freshly_certified &&
         equal_level_effect_is_pair_only_zero_points_continuation &&
         persistent_gamma_history.has_value() &&
         persistent_gamma_history_freshly_certified &&
         pair_history_creates_no_node &&
         pair_persists_through_every_later_gamma2_batch &&
         gateway_input.has_value() && gateway_result.has_value() &&
         compact_gateway_freshly_certified &&
         gateway_result->certified_gateway() &&
         no_forbidden_claim(*this);
}

bool ExactSilentPairGatewayGamma2DifferentialResult::
    certified_fail_closed_rejection() const noexcept {
  return schema_version ==
             silent_pair_gateway_gamma2_differential_schema_version &&
         decision !=
             ExactSilentPairGatewayGamma2DifferentialDecision::
                 not_certified &&
         decision !=
             ExactSilentPairGatewayGamma2DifferentialDecision::
                 complete_exact_regular_silent_pair_gateway_gamma2_differential &&
         scope ==
             ExactSilentPairGatewayGamma2DifferentialScope::
                 bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only &&
         !gateway_input.has_value() && !gateway_result.has_value() &&
         no_forbidden_claim(*this);
}

ExactSilentPairGatewayGamma2DifferentialResult
build_exact_silent_pair_gateway_gamma2_differential(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair_point_ids,
    ExactSilentPairGatewayGamma2DifferentialBudget budget) {
  ExactSilentPairGatewayGamma2DifferentialResult result =
      compute_exact_silent_pair_gateway_gamma2_differential(
          cloud, pair_point_ids, budget);
  if (!result.certified_differential() &&
      !result.certified_fail_closed_rejection()) {
    throw std::logic_error(
        "the bounded SilentPairGateway Gamma2 differential produced "
        "neither a certified result nor a fail-closed rejection");
  }
  return result;
}

bool ExactSilentPairGatewayGamma2DifferentialVerification::
    certified_differential() const noexcept {
  return schema_scope_and_inputs_certified &&
         requested_budget_certified &&
         fresh_geometric_recertification_replayed &&
         exhaustive_gamma2_batch_replayed &&
         exhaustive_gamma2_history_replayed &&
         compact_gateway_replayed &&
         expected_outcome_freshly_rebuilt &&
         expected_complete_differential &&
         !expected_fail_closed_rejection &&
         no_forbidden_structure_or_public_claim_certified &&
         fresh_replay_certified;
}

bool ExactSilentPairGatewayGamma2DifferentialVerification::
    certified_fail_closed_rejection() const noexcept {
  return schema_scope_and_inputs_certified &&
         requested_budget_certified &&
         fresh_geometric_recertification_replayed &&
         exhaustive_gamma2_batch_replayed &&
         exhaustive_gamma2_history_replayed &&
         compact_gateway_replayed &&
         expected_outcome_freshly_rebuilt &&
         !expected_complete_differential &&
         expected_fail_closed_rejection &&
         no_forbidden_structure_or_public_claim_certified &&
         fresh_replay_certified;
}

ExactSilentPairGatewayGamma2DifferentialVerification
verify_exact_silent_pair_gateway_gamma2_differential(
    const spatial::CanonicalPointCloud& cloud,
    ExactSilentPairGatewayPairIds pair_point_ids,
    ExactSilentPairGatewayGamma2DifferentialBudget budget,
    const ExactSilentPairGatewayGamma2DifferentialResult& result) {
  const ExactSilentPairGatewayGamma2DifferentialResult expected =
      compute_exact_silent_pair_gateway_gamma2_differential(
          cloud, pair_point_ids, budget);
  ExactSilentPairGatewayGamma2DifferentialVerification verification;
  const ExactSilentPairGatewayPairIds pair =
      canonical_ids(pair_point_ids);
  verification.schema_scope_and_inputs_certified =
      result.schema_version ==
          silent_pair_gateway_gamma2_differential_schema_version &&
      result.schema_version == expected.schema_version &&
      result.scope ==
          ExactSilentPairGatewayGamma2DifferentialScope::
              bounded_n14_order2_regular_diametral_pair_gateway_to_exhaustive_gamma2_history_only &&
      result.scope == expected.scope &&
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.pair_point_ids == pair &&
      result.pair_point_ids == expected.pair_point_ids;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.fresh_geometric_recertification_replayed =
      geometric_projection_equal(result, expected);
  verification.exhaustive_gamma2_batch_replayed =
      result.strict_gamma == expected.strict_gamma &&
      result.reduced_gamma_batch == expected.reduced_gamma_batch &&
      result.exhaustive_pair_equal_level_coface_point_ids ==
          expected.exhaustive_pair_equal_level_coface_point_ids &&
      result.target_strict_gamma_component_index ==
          expected.target_strict_gamma_component_index;
  verification.exhaustive_gamma2_history_replayed =
      result.persistent_gamma_history ==
          expected.persistent_gamma_history &&
      result.target_pre_batch_component_handle ==
          expected.target_pre_batch_component_handle &&
      result.target_prior_root_node_id ==
          expected.target_prior_root_node_id &&
      result.target_pre_batch_root_facet_point_ids ==
          expected.target_pre_batch_root_facet_point_ids &&
      result.pre_batch_snapshot_digest ==
          expected.pre_batch_snapshot_digest &&
      result.pre_batch_prefix_count ==
          expected.pre_batch_prefix_count &&
      result.pair_history_batch_index ==
          expected.pair_history_batch_index &&
      result.pair_history_group_record_index ==
          expected.pair_history_group_record_index &&
      result.later_root_trace == expected.later_root_trace;
  verification.compact_gateway_replayed =
      result.gateway_input == expected.gateway_input &&
      result.gateway_result == expected.gateway_result;
  if (expected.certified_differential() &&
      verification.compact_gateway_replayed &&
      result.gateway_input.has_value() &&
      result.gateway_result.has_value()) {
    verification.compact_gateway_replayed =
        verify_exact_silent_pair_gateway(
            *result.gateway_input, *result.gateway_result)
            .certified_gateway();
  }
  verification.expected_outcome_freshly_rebuilt =
      result == expected;
  verification.expected_complete_differential =
      expected.certified_differential();
  verification.expected_fail_closed_rejection =
      expected.certified_fail_closed_rejection();
  verification.no_forbidden_structure_or_public_claim_certified =
      no_forbidden_claim(result) && no_forbidden_claim(expected);
  verification.fresh_replay_certified =
      verification.schema_scope_and_inputs_certified &&
      verification.requested_budget_certified &&
      verification.fresh_geometric_recertification_replayed &&
      verification.exhaustive_gamma2_batch_replayed &&
      verification.exhaustive_gamma2_history_replayed &&
      verification.compact_gateway_replayed &&
      verification.expected_outcome_freshly_rebuilt &&
      verification.no_forbidden_structure_or_public_claim_certified &&
      (verification.expected_complete_differential !=
       verification.expected_fail_closed_rejection);
  return verification;
}

}  // namespace morsehgp3d::hierarchy
