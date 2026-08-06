#include "morsehgp3d/hierarchy/higher_support_closed_ball.hpp"

#include "morsehgp3d/exact/homogeneous_sphere.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

// The per-axis extreme coordinates of one node's exact bounding box, borrowed
// from the cloud in homogeneous form.
[[nodiscard]] std::array<exact::ExactAxisCoordinate, 3> node_axis_coordinates(
    const spatial::CanonicalPointCloud& cloud,
    const std::array<PointId, 3>& point_ids) {
  return {
      exact::exact_axis_coordinate(cloud.point(point_ids[0]).exact(), 0U),
      exact::exact_axis_coordinate(cloud.point(point_ids[1]).exact(), 1U),
      exact::exact_axis_coordinate(cloud.point(point_ids[2]).exact(), 2U)};
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

void increment(std::size_t& value, const char* message) {
  value = checked_add(value, 1U, message);
}

}  // namespace

std::size_t
ExactHigherSupportIndexedClosedBallQuery::proved_traversal_frontier_bound(
    const spatial::MortonLbvhIndex& index) {
  return checked_add(
      index.build_counters().maximum_depth,
      1U,
      "the closed-ball traversal frontier bound overflows size_t");
}

ExactHigherSupportClosedBallClassification
ExactHigherSupportIndexedClosedBallQuery::classify(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const std::array<spatial::PointId, 4>& support_ids,
    std::size_t support_size,
    const exact::ExactCenter3& center,
    const exact::ExactLevel& squared_level,
    std::size_t interior_cap,
    std::size_t maximum_traversal_frontier_entry_count) {
  if (!index.validated_for(cloud)) {
    throw std::logic_error(
        "a closed-ball query received an index that is not validated for "
        "its cloud");
  }
  if (support_size == 0U || support_size > 4U) {
    throw std::logic_error("a closed-ball query received an invalid support");
  }
  const auto node = [&index](std::size_t node_index)
      -> const spatial::MortonLbvhIndex::Node& {
    if (node_index >= index.nodes_.size()) {
      throw std::logic_error("a closed-ball query addressed an unknown node");
    }
    return index.nodes_[node_index];
  };

  const exact::ExactHomogeneousSphere3 sphere{center, squared_level};

  ExactHigherSupportClosedBallClassification classification;
  ExactHigherSupportClosedBallCounters& counters = classification.counters;
  classification.interior_ids.reserve(interior_cap);
  std::vector<std::size_t> frontier{index.root_index_};
  counters.maximum_traversal_frontier_entry_count = std::max(
      counters.maximum_traversal_frontier_entry_count,
      frontier.size());
  std::array<bool, 4> support_seen{};
  std::size_t interior_count = 0U;
  while (!frontier.empty()) {
    const std::size_t node_index = frontier.back();
    frontier.pop_back();
    const spatial::MortonLbvhIndex::Node& current = node(node_index);
    const std::size_t subtree_size = current.leaf_end - current.leaf_begin;
    increment(
        counters.node_visit_count,
        "the closed-ball node count overflows size_t");
    const std::array<exact::ExactAxisCoordinate, 3> lower =
        node_axis_coordinates(cloud, current.lower_point_ids);
    const std::array<exact::ExactAxisCoordinate, 3> upper =
        node_axis_coordinates(cloud, current.upper_point_ids);
    if (sphere.box_minimum_squared_distance_exceeds_level(lower, upper)) {
      classification.exterior_count = checked_add(
          classification.exterior_count,
          subtree_size,
          "the closed-ball exterior count overflows size_t");
      increment(
          counters.bulk_exterior_subtree_count,
          "the closed-ball exterior-subtree count overflows size_t");
      counters.point_classification_count = checked_add(
          counters.point_classification_count,
          subtree_size,
          "the closed-ball classification count overflows size_t");
      continue;
    }
    if (sphere.box_maximum_squared_distance_is_below_level(lower, upper)) {
      interior_count = checked_add(
          interior_count,
          subtree_size,
          "the closed-ball interior count overflows size_t");
      increment(
          counters.bulk_interior_subtree_count,
          "the closed-ball interior-subtree count overflows size_t");
      counters.point_classification_count = checked_add(
          counters.point_classification_count,
          subtree_size,
          "the closed-ball classification count overflows size_t");
      if (interior_count > interior_cap) {
        classification.outcome =
            ExactHigherSupportClosedBallOutcome::rank_exceeded;
        return classification;
      }
      for (std::size_t position = current.leaf_begin;
           position < current.leaf_end;
           ++position) {
        classification.interior_ids.push_back(
            index.leaves_[position].point_id);
      }
      continue;
    }
    if (current.is_leaf()) {
      const PointId point_id = index.leaves_[current.leaf_begin].point_id;
      const exact::SpherePointLocation point_location =
          sphere.classify_point(cloud.point(point_id).exact());
      increment(
          counters.exact_point_distance_evaluation_count,
          "the closed-ball exact-distance count overflows size_t");
      increment(
          counters.point_classification_count,
          "the closed-ball classification count overflows size_t");
      switch (point_location) {
        case exact::SpherePointLocation::strictly_inside:
          increment(
              interior_count,
              "the closed-ball interior count overflows size_t");
          if (interior_count > interior_cap) {
            classification.outcome =
                ExactHigherSupportClosedBallOutcome::rank_exceeded;
            return classification;
          }
          classification.interior_ids.push_back(point_id);
          break;
        case exact::SpherePointLocation::boundary: {
          increment(
              classification.shell_count,
              "the closed-ball shell count overflows size_t");
          bool is_support = false;
          for (std::size_t support_index = 0U;
               support_index < support_size;
               ++support_index) {
            if (point_id == support_ids[support_index]) {
              support_seen[support_index] = true;
              is_support = true;
              break;
            }
          }
          if (!is_support &&
              (!classification.canonical_extra_shell_witness_id.has_value() ||
               point_id <
                   *classification.canonical_extra_shell_witness_id)) {
            classification.canonical_extra_shell_witness_id = point_id;
          }
          break;
        }
        case exact::SpherePointLocation::outside:
          increment(
              classification.exterior_count,
              "the closed-ball exterior count overflows size_t");
          break;
      }
      continue;
    }
    frontier.push_back(current.right_child);
    frontier.push_back(current.left_child);
    if (frontier.size() > maximum_traversal_frontier_entry_count) {
      throw std::logic_error(
          "a closed-ball depth-first traversal exceeded its proved bound");
    }
    counters.maximum_traversal_frontier_entry_count = std::max(
        counters.maximum_traversal_frontier_entry_count,
        frontier.size());
  }
  // A complete traversal certifies its own partition: every cloud point is
  // classified exactly once, the support itself is on the shell, and the
  // extra-shell witness exists exactly when the shell is larger than the
  // support.
  const std::size_t classified_count = checked_add(
      checked_add(
          interior_count,
          classification.shell_count,
          "the closed-ball partition count overflows size_t"),
      classification.exterior_count,
      "the closed-ball partition count overflows size_t");
  bool every_support_seen = true;
  for (std::size_t index_in_support = 0U;
       index_in_support < support_size;
       ++index_in_support) {
    every_support_seen = every_support_seen && support_seen[index_in_support];
  }
  if (classified_count != cloud.size() || !every_support_seen ||
      classification.shell_count < support_size ||
      (classification.shell_count == support_size) !=
          !classification.canonical_extra_shell_witness_id.has_value() ||
      interior_count != classification.interior_ids.size()) {
    throw std::logic_error(
        "a closed-ball traversal did not close its exact partition");
  }
  std::sort(
      classification.interior_ids.begin(),
      classification.interior_ids.end());
  classification.outcome = ExactHigherSupportClosedBallOutcome::complete;
  return classification;
}

}  // namespace morsehgp3d::hierarchy
