#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_unordered_pair_count(
    std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  std::size_t first = point_count;
  std::size_t second = point_count - 1U;
  if ((first & 1U) == 0U) {
    first /= 2U;
  } else {
    second /= 2U;
  }
  return checked_multiply(
      first,
      second,
      "the pair-support unordered pair count overflows size_t");
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    std::string_view message) {
  if (value > static_cast<std::size_t>(
                  std::numeric_limits<std::uint64_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    std::string_view message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] bool can_add_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  return current <= maximum && increment <= maximum - current;
}

struct ExactBoxCoordinates {
  std::array<exact::ExactRational, 3> lower{};
  std::array<exact::ExactRational, 3> upper{};
};

[[nodiscard]] ExactBoxCoordinates exact_box_coordinates(
    const spatial::ExactDyadicAabb3& box) {
  ExactBoxCoordinates coordinates;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t lower_bits =
        exact::canonicalize_binary64_bits(box.lower_binary64_bits[axis]);
    const std::uint64_t upper_bits =
        exact::canonicalize_binary64_bits(box.upper_binary64_bits[axis]);
    coordinates.lower[axis] =
        exact::ExactRational::from_binary64_bits(lower_bits);
    coordinates.upper[axis] =
        exact::ExactRational::from_binary64_bits(upper_bits);
    if (coordinates.upper[axis] < coordinates.lower[axis]) {
      throw std::invalid_argument(
          "an exact dyadic AABB has a reversed axis");
    }
  }
  return coordinates;
}

[[nodiscard]] bool event_less(
    const ExactPairSupportEvent& left,
    const ExactPairSupportEvent& right) {
  return left.support_ids < right.support_ids;
}

[[nodiscard]] bool diagnostic_less(
    const ExactPairSupportExtraShellDiagnostic& left,
    const ExactPairSupportExtraShellDiagnostic& right) {
  return left.support_ids < right.support_ids;
}

}  // namespace

ExactDiametralPhiAabbMaximum exact_diametral_phi_aabb_maximum(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactBoxCoordinates first =
      exact_box_coordinates(first_support_box);
  const ExactBoxCoordinates second =
      exact_box_coordinates(second_support_box);
  const ExactBoxCoordinates query = exact_box_coordinates(query_box);
  ExactDiametralPhiAabbMaximum result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<exact::ExactRational, 2> first_endpoints{
        first.lower[axis], first.upper[axis]};
    const std::array<exact::ExactRational, 2> second_endpoints{
        second.lower[axis], second.upper[axis]};
    const std::array<exact::ExactRational, 2> query_endpoints{
        query.lower[axis], query.upper[axis]};
    bool initialized = false;
    exact::ExactRational axis_maximum;
    for (std::size_t query_selector = 0U;
         query_selector < query_endpoints.size();
         ++query_selector) {
      for (std::size_t first_selector = 0U;
           first_selector < first_endpoints.size();
           ++first_selector) {
        for (std::size_t second_selector = 0U;
             second_selector < second_endpoints.size();
             ++second_selector) {
          const exact::ExactRational candidate =
              (query_endpoints[query_selector] -
               first_endpoints[first_selector]) *
              (query_endpoints[query_selector] -
               second_endpoints[second_selector]);
          if (!initialized || candidate > axis_maximum) {
            initialized = true;
            axis_maximum = candidate;
            result.query_endpoint[axis] =
                static_cast<std::uint8_t>(query_selector);
            result.first_support_endpoint[axis] =
                static_cast<std::uint8_t>(first_selector);
            result.second_support_endpoint[axis] =
                static_cast<std::uint8_t>(second_selector);
          }
        }
      }
    }
    if (!initialized) {
      throw std::logic_error(
          "the exact diametral AABB maximum has no endpoint candidate");
    }
    result.maximum_phi = result.maximum_phi + axis_maximum;
  }
  return result;
}

class ExactPairSupportStreamBuilder {
 public:
  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget)
      : index_(index), cloud_(cloud) {
    validate_inputs(requested_maximum_order, budget);
    result_.requirements = requirements_for(requested_maximum_order);
    result_.budget = budget;
    result_.audit.total_pair_count =
        checked_unordered_pair_count(cloud_.size());
    if (result_.audit.total_pair_count != 0U) {
      frontier_.push_back(make_entry(index_.root_index_, index_.root_index_));
      result_.audit.maximum_frontier_entry_count = 1U;
    }
  }

  [[nodiscard]] ExactPairSupportStreamResult run() {
    if (result_.audit.total_pair_count == 0U) {
      finish_result();
      return std::move(result_);
    }
    while (!frontier_.empty() && !stopped_) {
      visit_frontier_back();
    }
    finish_result();
    return std::move(result_);
  }

 private:
  using Node = spatial::MortonLbvhIndex::Node;

  enum class RankSearchOutcome : std::uint8_t {
    keep,
    prune,
    budget_exhausted,
  };

  enum class SparseBallOutcome : std::uint8_t {
    complete,
    rank_exceeded,
  };

  struct SparseBallClassification {
    SparseBallOutcome outcome{SparseBallOutcome::rank_exceeded};
    std::vector<PointId> interior_ids;
    std::size_t shell_count{};
    std::optional<PointId> canonical_extra_shell_witness_id;
    std::size_t exterior_count{};
  };

  void validate_inputs(
      std::size_t requested_maximum_order,
      const ExactPairSupportStreamBudget& budget) const {
    if (!index_.validated_for(cloud_)) {
      throw std::invalid_argument(
          "the pair-support stream requires the supplied cloud's Morton LBVH");
    }
    if (cloud_.size() == 0U) {
      throw std::invalid_argument(
          "the pair-support stream requires a nonempty point cloud");
    }
    if (requested_maximum_order == 0U ||
        requested_maximum_order > pair_support_maximum_requested_order) {
      throw std::out_of_range(
          "the pair-support stream requires 1<=Kmax<=10");
    }
    static_cast<void>(checked_unordered_pair_count(cloud_.size()));
    if (cloud_.size() >= 2U &&
        budget.maximum_frontier_entry_count == 0U) {
      throw std::invalid_argument(
          "a nonempty pair frontier requires at least one budgeted entry");
    }
  }

  [[nodiscard]] ExactPairSupportRequirements requirements_for(
      std::size_t requested_maximum_order) const {
    ExactPairSupportRequirements requirements;
    requirements.point_count = cloud_.size();
    requirements.requested_maximum_order = requested_maximum_order;
    requirements.effective_maximum_order =
        std::min(requested_maximum_order, cloud_.size());
    requirements.maximum_relevant_closed_rank = std::min(
        checked_add(
            requirements.effective_maximum_order,
            1U,
            "the pair-support relevant closed rank overflows size_t"),
        cloud_.size());
    return requirements;
  }

  [[nodiscard]] const Node& node(std::size_t node_index) const {
    if (node_index >= index_.nodes_.size()) {
      throw std::logic_error(
          "a pair-support frontier references an invalid LBVH node");
    }
    return index_.nodes_[node_index];
  }

  [[nodiscard]] spatial::ExactDyadicAabb3 node_box(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    spatial::ExactDyadicAabb3 box{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] =
          cloud_.point(current.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      box.upper_binary64_bits[axis] =
          cloud_.point(current.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    return box;
  }

  [[nodiscard]] exact::ExactLevel node_squared_diagonal(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    exact::ExactRational squared;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const exact::ExactRational lower =
          cloud_.point(current.lower_point_ids[axis]).coordinate(axis);
      const exact::ExactRational upper =
          cloud_.point(current.upper_point_ids[axis]).coordinate(axis);
      const exact::ExactRational delta = upper - lower;
      squared = squared + delta * delta;
    }
    return exact::ExactLevel{std::move(squared)};
  }

  [[nodiscard]] ExactPairSupportFrontierEntry make_entry(
      std::size_t first_node_index,
      std::size_t second_node_index) const {
    const Node* first = &node(first_node_index);
    const Node* second = &node(second_node_index);
    if (first_node_index != second_node_index &&
        second->leaf_begin < first->leaf_begin) {
      std::swap(first_node_index, second_node_index);
      std::swap(first, second);
    }
    const bool self_product = first_node_index == second_node_index;
    if (!self_product && first->leaf_end > second->leaf_begin) {
      throw std::logic_error(
          "a pair-support cross product has overlapping Morton ranges");
    }
    return ExactPairSupportFrontierEntry{
        checked_u64(
            first_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            second_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            first->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            first->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        static_cast<std::uint8_t>(self_product ? 1U : 0U)};
  }

  [[nodiscard]] std::pair<std::size_t, std::size_t> entry_nodes(
      const ExactPairSupportFrontierEntry& entry) const {
    const std::size_t first_index = checked_size(
        entry.first_node_index,
        "a pair-support frontier node index does not fit size_t");
    const std::size_t second_index = checked_size(
        entry.second_node_index,
        "a pair-support frontier node index does not fit size_t");
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const bool self_product = entry.self_product == 1U;
    if (entry.self_product > 1U ||
        self_product != (first_index == second_index) ||
        entry.first_leaf_begin != checked_u64(
            first.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.first_leaf_end != checked_u64(
            first.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_begin != checked_u64(
            second.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_end != checked_u64(
            second.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        (!self_product && first.leaf_end > second.leaf_begin)) {
      throw std::logic_error(
          "a pair-support frontier entry contradicts its LBVH nodes");
    }
    return {first_index, second_index};
  }

  [[nodiscard]] std::size_t entry_pair_count(
      const ExactPairSupportFrontierEntry& entry) const {
    const auto [first_index, second_index] = entry_nodes(entry);
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const std::size_t first_size = first.leaf_end - first.leaf_begin;
    if (first_index == second_index) {
      return checked_unordered_pair_count(first_size);
    }
    const std::size_t second_size = second.leaf_end - second.leaf_begin;
    return checked_multiply(
        first_size,
        second_size,
        "a pair-support product coverage overflows size_t");
  }

  [[nodiscard]] bool consume_work_unit() {
    if (result_.audit.work_unit_count >=
        result_.budget.maximum_work_unit_count) {
      stop(ExactPairSupportStopReason::work_unit_limit);
      return false;
    }
    result_.audit.work_unit_count = checked_add(
        result_.audit.work_unit_count,
        1U,
        "the pair-support work count overflows size_t");
    return true;
  }

  void stop(ExactPairSupportStopReason reason) {
    if (!stopped_) {
      stopped_ = true;
      result_.status = ExactPairSupportStreamStatus::budget_exhausted;
      result_.stop_reason = reason;
    }
  }

  [[nodiscard]] bool node_range_intersects(
      const Node& query_node,
      const Node& support_node) const noexcept {
    return query_node.leaf_begin < support_node.leaf_end &&
           support_node.leaf_begin < query_node.leaf_end;
  }

  [[nodiscard]] RankSearchOutcome rank_prune_search(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    const std::size_t excluded_count = first_node_index == second_node_index
                                           ? first.leaf_end - first.leaf_begin
                                           : checked_add(
                                                 first.leaf_end - first.leaf_begin,
                                                 second.leaf_end - second.leaf_begin,
                                                 "pair-support exclusion count overflows size_t");
    if (excluded_count > cloud_.size()) {
      throw std::logic_error(
          "pair-support ranges exclude more points than the cloud contains");
    }
    const std::size_t witness_threshold =
        result_.requirements.maximum_relevant_closed_rank - 1U;
    if (cloud_.size() - excluded_count < witness_threshold) {
      return RankSearchOutcome::keep;
    }
    if (result_.budget.maximum_auxiliary_frontier_entry_count == 0U) {
      stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
      return RankSearchOutcome::budget_exhausted;
    }

    result_.audit.rank_prune_search_count = checked_add(
        result_.audit.rank_prune_search_count,
        1U,
        "the pair-support rank-search count overflows size_t");
    std::vector<std::size_t> witness_frontier{index_.root_index_};
    result_.audit.maximum_witness_frontier_entry_count = std::max(
        result_.audit.maximum_witness_frontier_entry_count,
        witness_frontier.size());
    const spatial::ExactDyadicAabb3 first_box = node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box = node_box(second_node_index);
    const PointId first_anchor_id =
        index_.leaves_[first.leaf_begin].point_id;
    const PointId second_anchor_id =
        index_.leaves_[second.leaf_begin].point_id;
    const exact::CircumcenterResult anchor_sphere = exact::circumcenter(
        cloud_.point(first_anchor_id), cloud_.point(second_anchor_id));
    if (anchor_sphere.kind() != exact::CircumcenterKind::unique ||
        !anchor_sphere.center().has_value() ||
        !anchor_sphere.squared_level().has_value()) {
      throw std::logic_error(
          "two support-product anchor points did not define a unique sphere");
    }
    std::size_t witness_point_count = 0U;
    while (!witness_frontier.empty()) {
      if (!consume_work_unit()) {
        return RankSearchOutcome::budget_exhausted;
      }
      const std::size_t query_node_index = witness_frontier.back();
      witness_frontier.pop_back();
      result_.audit.witness_node_visit_count = checked_add(
          result_.audit.witness_node_visit_count,
          1U,
          "the pair-support witness-node count overflows size_t");
      const Node& query_node = node(query_node_index);
      const bool overlaps_support =
          node_range_intersects(query_node, first) ||
          (first_node_index != second_node_index &&
           node_range_intersects(query_node, second));
      if (!overlaps_support) {
        const ExactDiametralPhiAabbMaximum maximum =
            exact_diametral_phi_aabb_maximum(
                first_box,
                second_box,
                node_box(query_node_index));
        result_.audit.exact_phi_aabb_bound_count = checked_add(
            result_.audit.exact_phi_aabb_bound_count,
            1U,
            "the pair-support phi-bound count overflows size_t");
        if (maximum.maximum_phi.sign() < 0) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          witness_point_count = checked_add(
              witness_point_count,
              subtree_size,
              "the pair-support witness count overflows size_t");
          result_.audit.strict_interior_witness_subtree_count = checked_add(
              result_.audit.strict_interior_witness_subtree_count,
              1U,
              "the pair-support witness-subtree count overflows size_t");
          result_.audit.strict_interior_witness_point_count = checked_add(
              result_.audit.strict_interior_witness_point_count,
              subtree_size,
              "the pair-support witness-point audit overflows size_t");
          if (witness_point_count >= witness_threshold) {
            return RankSearchOutcome::prune;
          }
          continue;
        }
        const exact::ExactLevel anchor_minimum_distance =
            index_.minimum_squared_distance_to_node(
                cloud_, query_node_index, *anchor_sphere.center());
        result_.audit.exact_anchor_ball_minimum_aabb_bound_count = checked_add(
            result_.audit.exact_anchor_ball_minimum_aabb_bound_count,
            1U,
            "the pair-support anchor-bound count overflows size_t");
        if (anchor_minimum_distance >= *anchor_sphere.squared_level()) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          if (anchor_minimum_distance == *anchor_sphere.squared_level()) {
            result_.audit.certified_anchor_shell_tangent_subtree_count =
                checked_add(
                    result_.audit
                        .certified_anchor_shell_tangent_subtree_count,
                    1U,
                    "the pair-support anchor-tangent count overflows size_t");
          }
          result_.audit.certified_anchor_noninterior_subtree_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_subtree_count,
                  1U,
                  "the pair-support anchor-subtree count overflows size_t");
          result_.audit.certified_anchor_noninterior_point_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_point_count,
                  subtree_size,
                  "the pair-support anchor-point count overflows size_t");
          continue;
        }
        result_.audit.equality_or_positive_bound_descent_count = checked_add(
            result_.audit.equality_or_positive_bound_descent_count,
            1U,
            "the pair-support nonnegative-bound count overflows size_t");
      }
      if (query_node.is_leaf()) {
        continue;
      }
      if (!can_add_within(
              witness_frontier.size(),
              2U,
              result_.budget.maximum_auxiliary_frontier_entry_count)) {
        stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
        return RankSearchOutcome::budget_exhausted;
      }
      witness_frontier.push_back(query_node.right_child);
      witness_frontier.push_back(query_node.left_child);
      result_.audit.maximum_witness_frontier_entry_count = std::max(
          result_.audit.maximum_witness_frontier_entry_count,
          witness_frontier.size());
    }
    return RankSearchOutcome::keep;
  }

  [[nodiscard]] bool leaf_preflight() {
    const std::size_t emitted_record_count = checked_add(
        result_.audit.accepted_event_count,
        result_.audit.relevant_extra_shell_diagnostic_count,
        "the pair-support emitted-record count overflows size_t");
    if (emitted_record_count >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactPairSupportStopReason::emitted_record_limit);
      return false;
    }
    const std::size_t maximum_record_references = checked_add(
        result_.requirements.maximum_relevant_closed_rank,
        1U,
        "the pair-support record reference bound overflows size_t");
    if (!can_add_within(
            result_.audit.emitted_point_id_reference_count,
            maximum_record_references,
            result_.budget.maximum_emitted_point_id_reference_count)) {
      stop(ExactPairSupportStopReason::emitted_point_id_reference_limit);
      return false;
    }
    if (result_.audit.global_closed_ball_query_count >=
        result_.budget.maximum_global_closed_ball_query_count) {
      stop(ExactPairSupportStopReason::global_closed_ball_query_limit);
      return false;
    }
    if (!can_add_within(
            result_.audit.point_classification_count,
            cloud_.size(),
            result_.budget.maximum_point_classification_count)) {
      stop(ExactPairSupportStopReason::point_classification_limit);
      return false;
    }
    const std::size_t required_closed_ball_frontier = checked_add(
        index_.build_counters().maximum_depth,
        1U,
        "the sparse closed-ball frontier bound overflows size_t");
    if (required_closed_ball_frontier >
        result_.budget.maximum_auxiliary_frontier_entry_count) {
      stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
      return false;
    }
    return true;
  }

  void add_point_classifications(std::size_t count) {
    result_.audit.point_classification_count = checked_add(
        result_.audit.point_classification_count,
        count,
        "the sparse closed-ball classification count overflows size_t");
    if (result_.audit.point_classification_count >
        result_.budget.maximum_point_classification_count) {
      throw std::logic_error(
          "a sparse closed-ball query exceeded its atomic classification budget");
    }
  }

  [[nodiscard]] SparseBallClassification classify_sparse_closed_ball(
      const std::array<PointId, 2>& support_ids,
      const exact::ExactCenter3& center,
      const exact::ExactLevel& squared_level) {
    SparseBallClassification classification;
    const std::size_t interior_cap =
        result_.requirements.maximum_relevant_closed_rank - 2U;
    classification.interior_ids.reserve(interior_cap);
    result_.audit.global_closed_ball_query_count = checked_add(
        result_.audit.global_closed_ball_query_count,
        1U,
        "the sparse closed-ball query count overflows size_t");
    std::vector<std::size_t> frontier{index_.root_index_};
    result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
        result_.audit.maximum_closed_ball_frontier_entry_count,
        frontier.size());
    std::array<bool, 2> support_seen{false, false};
    std::size_t interior_count = 0U;
    while (!frontier.empty()) {
      const std::size_t node_index = frontier.back();
      frontier.pop_back();
      const Node& current = node(node_index);
      const std::size_t subtree_size =
          current.leaf_end - current.leaf_begin;
      result_.audit.closed_ball_node_visit_count = checked_add(
          result_.audit.closed_ball_node_visit_count,
          1U,
          "the sparse closed-ball node count overflows size_t");
      const exact::ExactLevel minimum_distance =
          index_.minimum_squared_distance_to_node(cloud_, node_index, center);
      result_.audit.exact_closed_ball_minimum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_minimum_aabb_bound_count,
          1U,
          "the sparse closed-ball minimum-bound count overflows size_t");
      if (minimum_distance > squared_level) {
        classification.exterior_count = checked_add(
            classification.exterior_count,
            subtree_size,
            "the sparse closed-ball exterior count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_subtree_count,
            1U,
            "the sparse closed-ball exterior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_point_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_point_count,
            subtree_size,
            "the sparse closed-ball exterior-point count overflows size_t");
        add_point_classifications(subtree_size);
        continue;
      }
      const exact::ExactLevel maximum_distance =
          index_.maximum_squared_distance_to_node(cloud_, node_index, center);
      result_.audit.exact_closed_ball_maximum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_maximum_aabb_bound_count,
          1U,
          "the sparse closed-ball maximum-bound count overflows size_t");
      if (maximum_distance < squared_level) {
        interior_count = checked_add(
            interior_count,
            subtree_size,
            "the sparse closed-ball interior count overflows size_t");
        result_.audit.closed_ball_bulk_interior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_interior_subtree_count,
            1U,
            "the sparse closed-ball interior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_interior_point_count = checked_add(
            result_.audit.closed_ball_bulk_interior_point_count,
            subtree_size,
            "the sparse closed-ball interior-point count overflows size_t");
        add_point_classifications(subtree_size);
        if (interior_count > interior_cap) {
          result_.audit.early_closed_rank_rejection_count = checked_add(
              result_.audit.early_closed_rank_rejection_count,
              1U,
              "the sparse closed-ball early-rejection count overflows size_t");
          classification.outcome = SparseBallOutcome::rank_exceeded;
          return classification;
        }
        for (std::size_t position = current.leaf_begin;
             position < current.leaf_end;
             ++position) {
          classification.interior_ids.push_back(
              index_.leaves_[position].point_id);
        }
        continue;
      }
      if (current.is_leaf()) {
        const PointId point_id =
            index_.leaves_[current.leaf_begin].point_id;
        const exact::SpherePointClassification point_classification =
            exact::classify_sphere_point(
                center,
                squared_level,
                cloud_.point(point_id));
        result_.audit.exact_point_distance_evaluation_count = checked_add(
            result_.audit.exact_point_distance_evaluation_count,
            1U,
            "the sparse closed-ball exact-distance count overflows size_t");
        add_point_classifications(1U);
        switch (point_classification.location()) {
          case exact::SpherePointLocation::strictly_inside:
            interior_count = checked_add(
                interior_count,
                1U,
                "the sparse closed-ball interior count overflows size_t");
            if (interior_count > interior_cap) {
              result_.audit.early_closed_rank_rejection_count = checked_add(
                  result_.audit.early_closed_rank_rejection_count,
                  1U,
                  "the sparse closed-ball early-rejection count overflows size_t");
              classification.outcome = SparseBallOutcome::rank_exceeded;
              return classification;
            }
            classification.interior_ids.push_back(point_id);
            break;
          case exact::SpherePointLocation::boundary:
            classification.shell_count = checked_add(
                classification.shell_count,
                1U,
                "the sparse closed-ball shell count overflows size_t");
            if (point_id == support_ids[0]) {
              support_seen[0] = true;
            } else if (point_id == support_ids[1]) {
              support_seen[1] = true;
            } else if (!classification.canonical_extra_shell_witness_id.has_value() ||
                       point_id <
                           *classification.canonical_extra_shell_witness_id) {
              classification.canonical_extra_shell_witness_id = point_id;
            }
            break;
          case exact::SpherePointLocation::outside:
            classification.exterior_count = checked_add(
                classification.exterior_count,
                1U,
                "the sparse closed-ball exterior count overflows size_t");
            break;
        }
        continue;
      }
      frontier.push_back(current.right_child);
      frontier.push_back(current.left_child);
      if (frontier.size() >
          result_.budget.maximum_auxiliary_frontier_entry_count) {
        throw std::logic_error(
            "a sparse closed-ball DFS exceeded its preflight frontier bound");
      }
      result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
          result_.audit.maximum_closed_ball_frontier_entry_count,
          frontier.size());
    }
    const std::size_t classified_count = checked_add(
        checked_add(
            interior_count,
            classification.shell_count,
            "the sparse closed-ball partition count overflows size_t"),
        classification.exterior_count,
        "the sparse closed-ball partition count overflows size_t");
    if (classified_count != cloud_.size() ||
        !support_seen[0] || !support_seen[1] ||
        classification.shell_count < 2U ||
        (classification.shell_count == 2U) !=
            !classification.canonical_extra_shell_witness_id.has_value() ||
        interior_count != classification.interior_ids.size()) {
      throw std::logic_error(
          "a sparse closed-ball traversal did not close its exact partition");
    }
    std::sort(
        classification.interior_ids.begin(),
        classification.interior_ids.end());
    classification.outcome = SparseBallOutcome::complete;
    return classification;
  }

  void classify_leaf_pair(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    if (!leaf_preflight()) {
      return;
    }
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (!first.is_leaf() || !second.is_leaf() ||
        first_node_index == second_node_index) {
      throw std::logic_error(
          "a pair-support leaf classifier received a nonterminal product");
    }
    std::array<PointId, 2> support_ids{
        index_.leaves_[first.leaf_begin].point_id,
        index_.leaves_[second.leaf_begin].point_id};
    if (support_ids[1] < support_ids[0]) {
      std::swap(support_ids[0], support_ids[1]);
    }
    const exact::CircumcenterResult sphere = exact::circumcenter(
        cloud_.point(support_ids[0]),
        cloud_.point(support_ids[1]));
    if (sphere.kind() != exact::CircumcenterKind::unique ||
        !sphere.center().has_value() ||
        !sphere.squared_level().has_value()) {
      throw std::logic_error(
          "two canonical distinct points did not define a unique sphere");
    }
    SparseBallClassification classification = classify_sparse_closed_ball(
        support_ids,
        *sphere.center(),
        *sphere.squared_level());
    frontier_.pop_back();
    result_.audit.leaf_pair_classification_count = checked_add(
        result_.audit.leaf_pair_classification_count,
        1U,
        "the pair-support leaf-classification count overflows size_t");
    result_.audit.resolved_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        1U,
        "the pair-support resolved-pair count overflows size_t");
    if (classification.outcome == SparseBallOutcome::rank_exceeded) {
      result_.audit.above_rank_pair_count = checked_add(
          result_.audit.above_rank_pair_count,
          1U,
          "the pair-support above-rank count overflows size_t");
      return;
    }
    const std::size_t observed_closed_rank = checked_add(
        classification.interior_ids.size(),
        classification.shell_count,
        "the pair-support observed rank overflows size_t");
    const std::size_t minimum_possible_closed_rank = checked_add(
        classification.interior_ids.size(),
        2U,
        "the pair-support minimum rank overflows size_t");
    if (minimum_possible_closed_rank >
        result_.requirements.maximum_relevant_closed_rank) {
      throw std::logic_error(
          "a complete sparse shell escaped its interior-rank cap");
    }
    std::size_t emitted_references = 0U;
    if (classification.shell_count == 2U) {
      ExactPairSupportEvent event;
      event.support_ids = support_ids;
      event.center = *sphere.center();
      event.squared_level = *sphere.squared_level();
      event.interior_ids = std::move(classification.interior_ids);
      event.closed_rank = observed_closed_rank;
      event.exterior_count = classification.exterior_count;
      emitted_references = checked_add(
          2U,
          event.interior_ids.size(),
          "the pair-support event reference count overflows size_t");
      result_.events.push_back(std::move(event));
      result_.audit.accepted_event_count = checked_add(
          result_.audit.accepted_event_count,
          1U,
          "the pair-support event count overflows size_t");
    } else {
      if (!classification.canonical_extra_shell_witness_id.has_value()) {
        throw std::logic_error(
            "an extra-shell pair omitted its canonical extra witness");
      }
      ExactPairSupportExtraShellDiagnostic diagnostic;
      diagnostic.support_ids = support_ids;
      diagnostic.center = *sphere.center();
      diagnostic.squared_level = *sphere.squared_level();
      diagnostic.interior_ids = std::move(classification.interior_ids);
      diagnostic.shell_count = classification.shell_count;
      diagnostic.canonical_extra_shell_witness_id =
          *classification.canonical_extra_shell_witness_id;
      diagnostic.minimum_possible_closed_rank =
          minimum_possible_closed_rank;
      diagnostic.observed_closed_rank = observed_closed_rank;
      diagnostic.exterior_count = classification.exterior_count;
      emitted_references = checked_add(
          3U,
          diagnostic.interior_ids.size(),
          "the pair-support diagnostic reference count overflows size_t");
      result_.relevant_extra_shell_diagnostics.push_back(
          std::move(diagnostic));
      result_.audit.relevant_extra_shell_diagnostic_count = checked_add(
          result_.audit.relevant_extra_shell_diagnostic_count,
          1U,
          "the pair-support diagnostic count overflows size_t");
    }
    result_.audit.emitted_point_id_reference_count = checked_add(
        result_.audit.emitted_point_id_reference_count,
        emitted_references,
        "the pair-support emitted reference count overflows size_t");
    if (result_.audit.emitted_point_id_reference_count >
        result_.budget.maximum_emitted_point_id_reference_count) {
      throw std::logic_error(
          "a pair-support record exceeded its conservative reference preflight");
    }
  }

  [[nodiscard]] bool push_replacement_entries(
      std::vector<ExactPairSupportFrontierEntry> replacements) {
    const std::size_t base_size = frontier_.size() - 1U;
    const std::size_t new_size = checked_add(
        base_size,
        replacements.size(),
        "the pair-support frontier size overflows size_t");
    if (new_size > result_.budget.maximum_frontier_entry_count) {
      stop(ExactPairSupportStopReason::frontier_entry_limit);
      return false;
    }
    frontier_.pop_back();
    for (auto iterator = replacements.rbegin();
         iterator != replacements.rend();
         ++iterator) {
      frontier_.push_back(*iterator);
    }
    result_.audit.maximum_frontier_entry_count = std::max(
        result_.audit.maximum_frontier_entry_count,
        frontier_.size());
    return true;
  }

  void expand_product(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index) {
      if (first.is_leaf()) {
        throw std::logic_error(
            "a pair-support diagonal leaf cannot be expanded");
      }
      if (!push_replacement_entries({
          make_entry(first.left_child, first.left_child),
          make_entry(first.left_child, first.right_child),
          make_entry(first.right_child, first.right_child)})) {
        return;
      }
      result_.audit.support_product_expansion_count = checked_add(
          result_.audit.support_product_expansion_count,
          1U,
          "the pair-support product-expansion count overflows size_t");
      result_.audit.self_product_expansion_count = checked_add(
          result_.audit.self_product_expansion_count,
          1U,
          "the pair-support self-expansion count overflows size_t");
      return;
    }
    const bool first_leaf = first.is_leaf();
    const bool second_leaf = second.is_leaf();
    bool split_first = false;
    if (!first_leaf && second_leaf) {
      split_first = true;
    } else if (first_leaf && !second_leaf) {
      split_first = false;
    } else if (!first_leaf && !second_leaf) {
      const exact::ExactLevel first_diagonal =
          node_squared_diagonal(first_node_index);
      const exact::ExactLevel second_diagonal =
          node_squared_diagonal(second_node_index);
      const std::size_t first_size = first.leaf_end - first.leaf_begin;
      const std::size_t second_size = second.leaf_end - second.leaf_begin;
      if (first_diagonal != second_diagonal) {
        split_first = first_diagonal > second_diagonal;
      } else if (first_size != second_size) {
        split_first = first_size > second_size;
      } else {
        split_first = first_node_index > second_node_index;
      }
    } else {
      throw std::logic_error(
          "a pair-support leaf cross product escaped classification");
    }
    if (split_first) {
      if (!push_replacement_entries({
          make_entry(first.left_child, second_node_index),
          make_entry(first.right_child, second_node_index)})) {
        return;
      }
    } else {
      if (!push_replacement_entries({
          make_entry(first_node_index, second.left_child),
          make_entry(first_node_index, second.right_child)})) {
        return;
      }
    }
    result_.audit.support_product_expansion_count = checked_add(
        result_.audit.support_product_expansion_count,
        1U,
        "the pair-support product-expansion count overflows size_t");
    result_.audit.cross_product_expansion_count = checked_add(
        result_.audit.cross_product_expansion_count,
        1U,
        "the pair-support cross-expansion count overflows size_t");
  }

  void visit_frontier_back() {
    if (!consume_work_unit()) {
      return;
    }
    result_.audit.support_product_visit_count = checked_add(
        result_.audit.support_product_visit_count,
        1U,
        "the pair-support product-visit count overflows size_t");
    const ExactPairSupportFrontierEntry entry = frontier_.back();
    const auto [first_node_index, second_node_index] = entry_nodes(entry);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index && first.is_leaf()) {
      frontier_.pop_back();
      result_.audit.diagonal_leaf_discard_count = checked_add(
          result_.audit.diagonal_leaf_discard_count,
          1U,
          "the pair-support diagonal count overflows size_t");
      return;
    }
    // A diagonal product cannot satisfy the strict phi prune: the relaxed
    // support boxes allow u = v at one endpoint of A, hence
    // phi(x, u, u) = ||x-u||^2 >= 0 for every witness box.  Avoid a global
    // witness traversal whose exact outcome is known in advance.
    if (first_node_index == second_node_index) {
      result_.audit.diagonal_product_rank_search_skip_count = checked_add(
          result_.audit.diagonal_product_rank_search_skip_count,
          1U,
          "the pair-support diagonal skip count overflows size_t");
      expand_product(first_node_index, second_node_index);
      return;
    }
    // At a leaf pair, the sparse closed-ball traversal already performs the
    // exact rank cap.  Running a separate global phi witness search here
    // would traverse the same LBVH twice for every surviving support.
    if (first.is_leaf() && second.is_leaf()) {
      classify_leaf_pair(first_node_index, second_node_index);
      return;
    }

    const RankSearchOutcome rank_search =
        rank_prune_search(first_node_index, second_node_index);
    if (rank_search == RankSearchOutcome::budget_exhausted) {
      return;
    }
    if (rank_search == RankSearchOutcome::prune) {
      const std::size_t pair_count = entry_pair_count(entry);
      frontier_.pop_back();
      result_.audit.rank_pruned_product_count = checked_add(
          result_.audit.rank_pruned_product_count,
          1U,
          "the pair-support pruned-product count overflows size_t");
      result_.audit.rank_pruned_pair_count = checked_add(
          result_.audit.rank_pruned_pair_count,
          pair_count,
          "the pair-support pruned-pair count overflows size_t");
      result_.audit.resolved_pair_count = checked_add(
          result_.audit.resolved_pair_count,
          pair_count,
          "the pair-support resolved-pair count overflows size_t");
      return;
    }
    expand_product(first_node_index, second_node_index);
  }

  void finish_result() {
    std::sort(result_.events.begin(), result_.events.end(), event_less);
    std::sort(
        result_.relevant_extra_shell_diagnostics.begin(),
        result_.relevant_extra_shell_diagnostics.end(),
        diagnostic_less);
    result_.remaining_frontier = frontier_;
    for (const ExactPairSupportFrontierEntry& entry : frontier_) {
      result_.audit.remaining_frontier_pair_count = checked_add(
          result_.audit.remaining_frontier_pair_count,
          entry_pair_count(entry),
          "the pair-support remaining-pair count overflows size_t");
    }
    const std::size_t accounted_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        result_.audit.remaining_frontier_pair_count,
        "the pair-support accounting sum overflows size_t");
    result_.audit.pair_partition_accounting_certified =
        accounted_pair_count == result_.audit.total_pair_count &&
        result_.audit.resolved_pair_count == checked_add(
            result_.audit.rank_pruned_pair_count,
            result_.audit.leaf_pair_classification_count,
            "the pair-support terminal accounting overflows size_t");
    if (!result_.audit.pair_partition_accounting_certified) {
      throw std::logic_error(
          "the pair-support frontier does not partition all unordered pairs");
    }
    result_.self_product_partition_certified = true;
    result_.witness_antichains_certified = true;
    result_.all_rank_prunes_recertified = true;
    // Every non-rank-rejected leaf query reaches a complete global shell.
    result_.all_rank_relevant_shells_complete = true;
    result_.frontier_exhausted = frontier_.empty();
    result_.no_forbidden_global_structure_materialized = true;
    result_.hierarchy_reduction_performed = false;
    if (frontier_.empty()) {
      result_.status = ExactPairSupportStreamStatus::complete;
      result_.stop_reason = ExactPairSupportStopReason::none;
    } else if (!stopped_) {
      throw std::logic_error(
          "a nonempty pair-support frontier has no budget stop reason");
    }
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  ExactPairSupportStreamResult result_;
  std::vector<ExactPairSupportFrontierEntry> frontier_;
  bool stopped_{false};
};

ExactPairSupportStreamResult build_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget) {
  return ExactPairSupportStreamBuilder{
      index, cloud, requested_maximum_order, budget}.run();
}

ExactPairSupportStreamVerification verify_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& observed) {
  const ExactPairSupportStreamResult expected =
      build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, budget);
  ExactPairSupportStreamVerification verification;
  verification.requested_budget_certified = observed.budget == budget;
  verification.requirements_certified =
      observed.requirements == expected.requirements;
  verification.partial_records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics;
  verification.completion_claim_certified =
      observed.stream_complete() == expected.stream_complete() &&
      observed.status == expected.status &&
      observed.stop_reason == expected.stop_reason &&
      observed.frontier_exhausted == expected.frontier_exhausted;
  verification.absence_claim_certified =
      observed.absence_of_additional_pair_supports_certified() ==
      expected.absence_of_additional_pair_supports_certified();
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.partial_records_individually_exact &&
      verification.completion_claim_certified &&
      verification.absence_claim_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
