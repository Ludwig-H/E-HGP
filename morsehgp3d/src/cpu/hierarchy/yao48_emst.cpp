#include "morsehgp3d/hierarchy/yao48_emst.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::array<std::uint8_t, 3>, 6> axis_permutations{{
    {{0U, 1U, 2U}},
    {{0U, 2U, 1U}},
    {{1U, 0U, 2U}},
    {{1U, 2U, 0U}},
    {{2U, 0U, 1U}},
    {{2U, 1U, 0U}},
}};

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t size)
      : parents_(size), component_count_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (value >= parents_.size()) {
      throw std::out_of_range("a Yao48 disjoint-set value is out of range");
    }
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t parent = parents_[value];
      parents_[value] = root;
      value = parent;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    const std::size_t root = std::min(left, right);
    parents_[std::max(left, right)] = root;
    --component_count_;
    return true;
  }

  [[nodiscard]] std::size_t component_count() const noexcept {
    return component_count_;
  }

 private:
  std::vector<std::size_t> parents_;
  std::size_t component_count_{};
};

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_pair_count(std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  std::size_t left = point_count;
  std::size_t right = point_count - 1U;
  if ((left & std::size_t{1}) == 0U) {
    left /= 2U;
  } else {
    right /= 2U;
  }
  return checked_product(
      left, right, "the exact Yao48 unordered-pair count overflows size_t");
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(
                  spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("an exact Yao48 point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] std::size_t checked_point_index(
    spatial::PointId point_id,
    std::size_t point_count) {
  if (point_id >
      static_cast<spatial::PointId>(std::numeric_limits<std::size_t>::max())) {
    throw std::logic_error("an exact Yao48 PointId does not fit size_t");
  }
  const std::size_t index = static_cast<std::size_t>(point_id);
  if (index >= point_count) {
    throw std::logic_error("an exact Yao48 PointId is outside the cloud");
  }
  return index;
}

[[nodiscard]] exact::ExactLevel merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left_id,
    spatial::PointId right_id) {
  const exact::ExactRational3& left = cloud.point(left_id).exact();
  const exact::ExactRational3& right = cloud.point(right_id).exact();
  const exact::BigInt common_denominator =
      left.denominator() * right.denominator();
  exact::BigInt squared_numerator = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::BigInt difference =
        left.numerator(axis) * right.denominator() -
        right.numerator(axis) * left.denominator();
    squared_numerator += difference * difference;
  }
  return exact::ExactLevel{
      std::move(squared_numerator),
      common_denominator * common_denominator};
}

[[nodiscard]] ExactEmstEdge exact_edge(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left,
    spatial::PointId right) {
  if (left == right) {
    throw std::logic_error("an exact Yao48 edge cannot be a loop");
  }
  const spatial::PointId u = std::min(left, right);
  const spatial::PointId v = std::max(left, right);
  exact::ExactLevel squared_length = exact_squared_distance(cloud, u, v);
  if (squared_length == exact::ExactLevel{}) {
    throw std::invalid_argument(
        "exact Yao48 requires distinct geometric points");
  }
  exact::ExactLevel level = merge_level(squared_length);
  return ExactEmstEdge{u, v, std::move(squared_length), std::move(level)};
}

[[nodiscard]] bool edge_less(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] std::size_t permutation_index(
    const std::array<std::uint8_t, 3>& axes) {
  const auto found =
      std::find(axis_permutations.begin(), axis_permutations.end(), axes);
  if (found == axis_permutations.end()) {
    throw std::logic_error("an exact Yao48 axis order is not a permutation");
  }
  return static_cast<std::size_t>(
      std::distance(axis_permutations.begin(), found));
}

[[nodiscard]] bool same_edge_endpoints(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) noexcept {
  return left.u == right.u && left.v == right.v;
}

// This is intentionally a second implementation of the complete reference
// transcript.  Keeping it separate from the producer helpers makes mutations
// of stored cone keys, edges, ordering, weights or counters observable instead
// of merely re-asserting producer-local invariants.
namespace verification_replay {

constexpr std::array<std::array<std::uint8_t, 3>, 6>
    replay_axis_permutations{{
        {{0U, 1U, 2U}},
        {{0U, 2U, 1U}},
        {{1U, 0U, 2U}},
        {{1U, 2U, 0U}},
        {{2U, 0U, 1U}},
        {{2U, 1U, 0U}},
    }};

class ReplayDisjointSet {
 public:
  explicit ReplayDisjointSet(std::size_t size)
      : parents_(size), component_count_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t root(std::size_t value) {
    std::size_t representative = value;
    while (parents_[representative] != representative) {
      representative = parents_[representative];
    }
    while (parents_[value] != value) {
      const std::size_t parent = parents_[value];
      parents_[value] = representative;
      value = parent;
    }
    return representative;
  }

  [[nodiscard]] bool join(std::size_t left, std::size_t right) {
    left = root(left);
    right = root(right);
    if (left == right) {
      return false;
    }
    parents_[right] = left;
    --component_count_;
    return true;
  }

  [[nodiscard]] std::size_t component_count() const noexcept {
    return component_count_;
  }

 private:
  std::vector<std::size_t> parents_;
  std::size_t component_count_{};
};

[[nodiscard]] std::size_t replay_pair_count(std::size_t point_count) noexcept {
  return point_count < 2U
             ? 0U
             : point_count * (point_count - 1U) / 2U;
}

[[nodiscard]] spatial::PointId replay_point_id(std::size_t index) noexcept {
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] exact::ExactLevel replay_merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
}

[[nodiscard]] ExactEmstEdge replay_edge(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId first,
    spatial::PointId second) {
  const spatial::PointId u = std::min(first, second);
  const spatial::PointId v = std::max(first, second);
  const exact::ExactRational3& left = cloud.point(u).exact();
  const exact::ExactRational3& right = cloud.point(v).exact();
  const exact::BigInt denominator =
      left.denominator() * right.denominator();
  exact::BigInt numerator = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::BigInt delta =
        left.numerator(axis) * right.denominator() -
        right.numerator(axis) * left.denominator();
    numerator += delta * delta;
  }
  exact::ExactLevel squared_length{
      std::move(numerator), denominator * denominator};
  return ExactEmstEdge{
      u, v, squared_length, replay_merge_level(squared_length)};
}

[[nodiscard]] bool replay_edge_less(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  return std::pair{left.u, left.v} < std::pair{right.u, right.v};
}

[[nodiscard]] bool replay_same_endpoints(
    const ExactEmstEdge& left,
    const ExactEmstEdge& right) noexcept {
  return left.u == right.u && left.v == right.v;
}

[[nodiscard]] std::size_t replay_cone_index(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target) {
  std::array<exact::ExactRational, 3> magnitudes{};
  std::array<std::uint8_t, 3> order{{0U, 1U, 2U}};
  std::uint8_t negative_mask = 0U;
  for (std::size_t axis = 0U; axis < order.size(); ++axis) {
    exact::ExactRational delta =
        target.coordinate(axis) - source.coordinate(axis);
    if (delta.sign() < 0) {
      const std::uint8_t bit = static_cast<std::uint8_t>(
          std::uint32_t{1U} << static_cast<std::uint32_t>(axis));
      negative_mask = static_cast<std::uint8_t>(negative_mask | bit);
      delta = -delta;
    }
    magnitudes[axis] = std::move(delta);
  }
  std::sort(
      order.begin(),
      order.end(),
      [&magnitudes](std::uint8_t left, std::uint8_t right) {
        const std::size_t left_axis = static_cast<std::size_t>(left);
        const std::size_t right_axis = static_cast<std::size_t>(right);
        return magnitudes[left_axis] == magnitudes[right_axis]
                   ? left < right
                   : magnitudes[left_axis] > magnitudes[right_axis];
      });
  std::size_t permutation = 0U;
  while (replay_axis_permutations[permutation] != order) {
    ++permutation;
  }
  return static_cast<std::size_t>(negative_mask) *
             replay_axis_permutations.size() +
         permutation;
}

[[nodiscard]] ExactYao48EmstResult replay_transcript(
    const spatial::CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  const std::size_t point_cone_count =
      point_count * exact_yao48_cone_count;
  std::vector<std::optional<ExactEmstEdge>> nearest(point_cone_count);

  ExactYao48EmstResult replay;
  replay.point_count = point_count;
  replay.counters.point_count = point_count;
  replay.counters.unordered_pair_count = replay_pair_count(point_count);
  for (std::size_t right_index = 1U;
       right_index < point_count;
       ++right_index) {
    const spatial::PointId right = replay_point_id(right_index);
    for (std::size_t left_index = 0U;
         left_index < right_index;
         ++left_index) {
      const spatial::PointId left = replay_point_id(left_index);
      const ExactEmstEdge candidate = replay_edge(cloud, left, right);
      ++replay.counters.exact_squared_distance_evaluation_count;
      const std::array<std::pair<std::size_t, std::size_t>, 2> slots{{
          {left_index,
           replay_cone_index(cloud.point(left), cloud.point(right))},
          {right_index,
           replay_cone_index(cloud.point(right), cloud.point(left))},
      }};
      replay.counters.directed_cone_classification_count += slots.size();
      for (const auto& [source_index, cone_index] : slots) {
        std::optional<ExactEmstEdge>& incumbent =
            nearest[source_index * exact_yao48_cone_count + cone_index];
        if (!incumbent.has_value() ||
            replay_edge_less(candidate, *incumbent)) {
          incumbent = candidate;
        }
      }
    }
  }

  replay.directed_candidates.reserve(point_cone_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    for (std::size_t cone_index = 0U;
         cone_index < exact_yao48_cone_count;
         ++cone_index) {
      const std::optional<ExactEmstEdge>& candidate =
          nearest[point_index * exact_yao48_cone_count + cone_index];
      if (candidate.has_value()) {
        replay.directed_candidates.push_back(ExactYao48DirectedCandidate{
            replay_point_id(point_index), cone_index, *candidate});
      }
    }
  }
  replay.counters.directed_candidate_count =
      replay.directed_candidates.size();
  replay.counters.empty_point_cone_count =
      point_cone_count - replay.directed_candidates.size();

  replay.unique_candidate_edges.reserve(replay.directed_candidates.size());
  for (const ExactYao48DirectedCandidate& candidate :
       replay.directed_candidates) {
    replay.unique_candidate_edges.push_back(candidate.edge);
  }
  std::sort(
      replay.unique_candidate_edges.begin(),
      replay.unique_candidate_edges.end(),
      replay_edge_less);
  replay.unique_candidate_edges.erase(
      std::unique(
          replay.unique_candidate_edges.begin(),
          replay.unique_candidate_edges.end(),
          replay_same_endpoints),
      replay.unique_candidate_edges.end());
  replay.counters.unique_candidate_edge_count =
      replay.unique_candidate_edges.size();

  ReplayDisjointSet components{point_count};
  replay.emst_edges.reserve(point_count - 1U);
  for (const ExactEmstEdge& candidate : replay.unique_candidate_edges) {
    if (!components.join(
            static_cast<std::size_t>(candidate.u),
            static_cast<std::size_t>(candidate.v))) {
      continue;
    }
    replay.emst_edges.push_back(candidate);
    replay.total_squared_weight = exact::ExactLevel{
        replay.total_squared_weight.rational() +
        candidate.squared_length.rational()};
    if (replay.emst_edges.size() == point_count - 1U) {
      break;
    }
  }
  replay.total_hgp_weight = replay_merge_level(replay.total_squared_weight);
  replay.counters.selected_edge_count = replay.emst_edges.size();
  replay.counters.redundant_candidate_edge_count =
      replay.unique_candidate_edges.size() - replay.emst_edges.size();
  replay.counters.final_component_count = components.component_count();
  return replay;
}

}  // namespace verification_replay

}  // namespace

ExactYao48ConeKey classify_exact_yao48_cone(
    const exact::CertifiedPoint3& source,
    const exact::CertifiedPoint3& target) {
  std::array<exact::ExactRational, 3> absolute_deltas{};
  std::array<std::uint8_t, 3> axes{{0U, 1U, 2U}};
  std::uint8_t negative_axis_mask = 0U;
  bool nonzero = false;
  for (std::size_t axis = 0U; axis < axes.size(); ++axis) {
    exact::ExactRational delta =
        target.coordinate(axis) - source.coordinate(axis);
    const int sign = delta.sign();
    if (sign != 0) {
      nonzero = true;
    }
    if (sign < 0) {
      negative_axis_mask = static_cast<std::uint8_t>(
          negative_axis_mask |
          static_cast<std::uint8_t>(std::uint8_t{1U} << axis));
      delta = -delta;
    }
    absolute_deltas[axis] = std::move(delta);
  }
  if (!nonzero) {
    throw std::invalid_argument(
        "an exact Yao48 cone requires two distinct points");
  }

  std::sort(
      axes.begin(),
      axes.end(),
      [&absolute_deltas](std::uint8_t left, std::uint8_t right) {
        const std::size_t left_axis = static_cast<std::size_t>(left);
        const std::size_t right_axis = static_cast<std::size_t>(right);
        if (absolute_deltas[left_axis] != absolute_deltas[right_axis]) {
          return absolute_deltas[left_axis] > absolute_deltas[right_axis];
        }
        return left < right;
      });
  const std::size_t order_index = permutation_index(axes);
  const std::size_t cone_index =
      static_cast<std::size_t>(negative_axis_mask) *
          axis_permutations.size() +
      order_index;
  if (cone_index >= exact_yao48_cone_count) {
    throw std::logic_error("an exact Yao48 cone index exceeds 47");
  }
  return ExactYao48ConeKey{negative_axis_mask, axes, cone_index};
}

bool ExactYao48EmstResult::locally_structurally_valid() const {
  if (point_count == 0U ||
      point_count > exact_yao48_emst_reference_max_point_count) {
    return false;
  }
  const std::size_t expected_pair_count = checked_pair_count(point_count);
  const std::size_t expected_point_cone_count = checked_product(
      point_count,
      exact_yao48_cone_count,
      "the exact Yao48 local point-cone count overflows size_t");
  const std::size_t expected_classification_count = checked_product(
      expected_pair_count,
      2U,
      "the exact Yao48 local classification count overflows size_t");
  return schema_version == exact_yao48_emst_schema_version &&
         counters.point_count == point_count &&
         counters.unordered_pair_count == expected_pair_count &&
         counters.exact_squared_distance_evaluation_count ==
             expected_pair_count &&
         counters.directed_cone_classification_count ==
             expected_classification_count &&
         counters.directed_candidate_count == directed_candidates.size() &&
         directed_candidates.size() <= expected_point_cone_count &&
         counters.empty_point_cone_count ==
             expected_point_cone_count - directed_candidates.size() &&
         counters.unique_candidate_edge_count ==
             unique_candidate_edges.size() &&
         unique_candidate_edges.size() <= directed_candidates.size() &&
         counters.selected_edge_count == emst_edges.size() &&
         emst_edges.size() <= unique_candidate_edges.size() &&
         counters.redundant_candidate_edge_count ==
             unique_candidate_edges.size() - emst_edges.size() &&
         emst_edges.size() == point_count - 1U &&
         counters.final_component_count == 1U;
}

ExactYao48EmstResult build_exact_yao48_emst_reference(
    const spatial::CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "the exact Yao48 EMST requires a nonempty canonical point cloud");
  }
  if (point_count > exact_yao48_emst_reference_max_point_count) {
    throw std::length_error(
        "the exact Yao48 quadratic reference point limit is 4096");
  }
  const std::size_t unordered_pair_count = checked_pair_count(point_count);
  const std::size_t point_cone_count = checked_product(
      point_count,
      exact_yao48_cone_count,
      "the exact Yao48 point-cone table overflows size_t");
  std::vector<std::optional<ExactEmstEdge>> nearest_by_point_cone;
  if (point_cone_count > nearest_by_point_cone.max_size()) {
    throw std::length_error(
        "the exact Yao48 point-cone table exceeds vector capacity");
  }
  nearest_by_point_cone.resize(point_cone_count);

  ExactYao48EmstResult result;
  result.point_count = point_count;
  result.counters.point_count = point_count;
  result.counters.unordered_pair_count = unordered_pair_count;

  for (std::size_t right_index = 1U;
       right_index < point_count;
       ++right_index) {
    const spatial::PointId right = checked_point_id(right_index);
    for (std::size_t left_index = 0U;
         left_index < right_index;
         ++left_index) {
      const spatial::PointId left = checked_point_id(left_index);
      ExactEmstEdge edge = exact_edge(cloud, left, right);
      ++result.counters.exact_squared_distance_evaluation_count;

      const ExactYao48ConeKey left_cone = classify_exact_yao48_cone(
          cloud.point(left), cloud.point(right));
      const ExactYao48ConeKey right_cone = classify_exact_yao48_cone(
          cloud.point(right), cloud.point(left));
      result.counters.directed_cone_classification_count += 2U;

      for (const auto& [source_index, cone_index] :
           std::array<std::pair<std::size_t, std::size_t>, 2>{{
               {left_index, left_cone.cone_index},
               {right_index, right_cone.cone_index},
           }}) {
        const std::size_t slot =
            source_index * exact_yao48_cone_count + cone_index;
        std::optional<ExactEmstEdge>& incumbent =
            nearest_by_point_cone[slot];
        if (!incumbent.has_value() || edge_less(edge, *incumbent)) {
          incumbent = edge;
        }
      }
    }
  }
  if (result.counters.exact_squared_distance_evaluation_count !=
          unordered_pair_count ||
      result.counters.directed_cone_classification_count !=
          checked_product(
              unordered_pair_count,
              2U,
              "the exact Yao48 directed classification count overflows")) {
    throw std::logic_error("the exact Yao48 pair scan counters do not close");
  }

  if (point_cone_count > result.directed_candidates.max_size()) {
    throw std::length_error(
        "the exact Yao48 directed candidate bound exceeds vector capacity");
  }
  result.directed_candidates.reserve(point_cone_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const spatial::PointId source = checked_point_id(point_index);
    for (std::size_t cone_index = 0U;
         cone_index < exact_yao48_cone_count;
         ++cone_index) {
      const std::optional<ExactEmstEdge>& candidate =
          nearest_by_point_cone[
              point_index * exact_yao48_cone_count + cone_index];
      if (!candidate.has_value()) {
        continue;
      }
      if (candidate->u != source && candidate->v != source) {
        throw std::logic_error(
            "an exact Yao48 outgoing edge omits its source point");
      }
      result.directed_candidates.push_back(
          ExactYao48DirectedCandidate{source, cone_index, *candidate});
    }
  }
  result.counters.directed_candidate_count =
      result.directed_candidates.size();
  result.counters.empty_point_cone_count =
      point_cone_count - result.directed_candidates.size();

  if (result.directed_candidates.size() >
      result.unique_candidate_edges.max_size()) {
    throw std::length_error(
        "the exact Yao48 unique-edge input exceeds vector capacity");
  }
  result.unique_candidate_edges.reserve(result.directed_candidates.size());
  for (const ExactYao48DirectedCandidate& candidate :
       result.directed_candidates) {
    result.unique_candidate_edges.push_back(candidate.edge);
  }
  std::sort(
      result.unique_candidate_edges.begin(),
      result.unique_candidate_edges.end(),
      edge_less);
  result.unique_candidate_edges.erase(
      std::unique(
          result.unique_candidate_edges.begin(),
          result.unique_candidate_edges.end(),
          same_edge_endpoints),
      result.unique_candidate_edges.end());
  result.counters.unique_candidate_edge_count =
      result.unique_candidate_edges.size();

  result.emst_edges.reserve(point_count - 1U);
  CanonicalDisjointSet components{point_count};
  for (const ExactEmstEdge& edge : result.unique_candidate_edges) {
    const std::size_t u = checked_point_index(edge.u, point_count);
    const std::size_t v = checked_point_index(edge.v, point_count);
    if (!components.unite(u, v)) {
      continue;
    }
    result.emst_edges.push_back(edge);
    result.total_squared_weight = exact::ExactLevel{
        result.total_squared_weight.rational() + edge.squared_length.rational()};
    if (result.emst_edges.size() == point_count - 1U) {
      break;
    }
  }
  result.total_hgp_weight = merge_level(result.total_squared_weight);
  result.counters.selected_edge_count = result.emst_edges.size();
  result.counters.redundant_candidate_edge_count =
      result.unique_candidate_edges.size() - result.emst_edges.size();
  result.counters.final_component_count = components.component_count();
  if (result.emst_edges.size() != point_count - 1U ||
      components.component_count() != 1U ||
      !std::is_sorted(
          result.emst_edges.begin(), result.emst_edges.end(), edge_less)) {
    throw std::logic_error(
        "the exact Yao48 candidate graph did not yield a canonical spanning tree");
  }

  if (!result.locally_structurally_valid()) {
    throw std::logic_error(
        "the exact Yao48 local structural validation did not close");
  }
  return result;
}

ExactYao48EmstVerification verify_exact_yao48_emst_reference(
    const spatial::CanonicalPointCloud& cloud,
    const ExactYao48EmstResult& result) {
  ExactYao48EmstVerification verification;
  verification.input_within_reference_bound =
      cloud.size() != 0U &&
      cloud.size() <= exact_yao48_emst_reference_max_point_count;
  verification.result_structurally_valid =
      result.locally_structurally_valid();
  if (!verification.input_within_reference_bound) {
    return verification;
  }

  const ExactYao48EmstResult expected =
      verification_replay::replay_transcript(cloud);
  verification.directed_candidate_transcript_exact =
      result.point_count == expected.point_count &&
      result.directed_candidates == expected.directed_candidates;
  verification.unique_candidate_transcript_exact =
      result.unique_candidate_edges == expected.unique_candidate_edges;
  verification.canonical_kruskal_transcript_exact =
      result.emst_edges == expected.emst_edges;
  verification.exact_totals_match =
      result.total_squared_weight == expected.total_squared_weight &&
      result.total_hgp_weight == expected.total_hgp_weight;
  verification.counters_match = result.counters == expected.counters;
  verification.full_transcript_exact = result == expected;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
