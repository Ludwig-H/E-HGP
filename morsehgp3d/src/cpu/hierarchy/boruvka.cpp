#include "morsehgp3d/hierarchy/boruvka.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t invalid_node_index = static_cast<std::size_t>(-1);

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t size)
      : parents_(size), component_count_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (value >= parents_.size()) {
      throw std::out_of_range("a Boruvka disjoint-set label is out of range");
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

struct TraversalNode {
  std::array<spatial::PointId, 3> lower_point_ids{};
  std::array<spatial::PointId, 3> upper_point_ids{};
  std::size_t left_child{invalid_node_index};
  std::size_t right_child{invalid_node_index};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};

  [[nodiscard]] bool is_leaf() const noexcept {
    return left_child == invalid_node_index;
  }
};

struct TraversalIndexSnapshot {
  std::vector<spatial::PointId> leaves;
  std::vector<TraversalNode> nodes;
  std::size_t root_index{invalid_node_index};
};

struct ComponentTag {
  bool uniform{false};
  spatial::PointId component_label{};
};

struct CandidateEdge {
  ExactEmstEdge edge;
  spatial::PointId source_point_id{};
};

struct NodeQueueEntry {
  exact::ExactLevel lower_bound;
  std::size_t node_index{};
};

struct NearestNodeFirst {
  [[nodiscard]] bool operator()(
      const NodeQueueEntry& left,
      const NodeQueueEntry& right) const {
    if (left.lower_bound != right.lower_bound) {
      return left.lower_bound > right.lower_bound;
    }
    return left.node_index > right.node_index;
  }
};

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(
                  spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("a Boruvka point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] std::size_t checked_point_index(
    spatial::PointId point_id,
    std::size_t point_count) {
  if (point_id >
      static_cast<spatial::PointId>(std::numeric_limits<std::size_t>::max())) {
    throw std::logic_error("a Boruvka PointId does not fit size_t");
  }
  const std::size_t index = static_cast<std::size_t>(point_id);
  if (index >= point_count) {
    throw std::logic_error("a Boruvka PointId is outside the cloud");
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
    throw std::logic_error("a Boruvka edge cannot be a loop");
  }
  const spatial::PointId u = std::min(left, right);
  const spatial::PointId v = std::max(left, right);
  exact::ExactLevel squared_length = exact_squared_distance(cloud, u, v);
  if (squared_length == exact::ExactLevel{}) {
    throw std::invalid_argument(
        "exact Boruvka requires distinct geometric points");
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

[[nodiscard]] bool candidate_less(
    const CandidateEdge& left,
    const CandidateEdge& right) {
  if (edge_less(left.edge, right.edge)) {
    return true;
  }
  if (edge_less(right.edge, left.edge)) {
    return false;
  }
  return left.source_point_id < right.source_point_id;
}

[[nodiscard]] exact::ExactLevel exact_aabb_lower_bound(
    const TraversalNode& node,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId query_id) {
  const exact::ExactRational3& query = cloud.point(query_id).exact();
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational coordinate = query.coordinate(axis);
    const exact::ExactRational lower =
        cloud.point(node.lower_point_ids[axis]).coordinate(axis);
    const exact::ExactRational upper =
        cloud.point(node.upper_point_ids[axis]).coordinate(axis);
    exact::ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] std::size_t theoretical_maximum_round_count(
    std::size_t point_count) {
  if (point_count <= 1U) {
    return 0U;
  }
  return static_cast<std::size_t>(std::bit_width(point_count - 1U));
}

void validate_snapshot(
    const TraversalIndexSnapshot& snapshot,
    std::size_t point_count) {
  if (point_count == 0U || snapshot.leaves.size() != point_count ||
      snapshot.root_index >= snapshot.nodes.size()) {
    throw std::logic_error("a Boruvka LBVH snapshot is incomplete");
  }
  if (point_count > (std::numeric_limits<std::size_t>::max() / 2U) + 1U ||
      snapshot.nodes.size() != point_count * 2U - 1U) {
    throw std::logic_error("a Boruvka LBVH snapshot has a wrong node count");
  }
  std::vector<unsigned char> point_seen(point_count, 0U);
  for (const spatial::PointId point_id : snapshot.leaves) {
    const std::size_t point_index =
        checked_point_index(point_id, point_count);
    if (point_seen[point_index] != 0U) {
      throw std::logic_error("a Boruvka LBVH snapshot repeats a PointId");
    }
    point_seen[point_index] = 1U;
  }

  for (std::size_t node_index = 0U;
       node_index < snapshot.nodes.size();
       ++node_index) {
    const TraversalNode& node = snapshot.nodes[node_index];
    if (node.leaf_begin >= node.leaf_end ||
        node.leaf_end > snapshot.leaves.size()) {
      throw std::logic_error("a Boruvka LBVH node has a wrong leaf range");
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      static_cast<void>(checked_point_index(
          node.lower_point_ids[axis], point_count));
      static_cast<void>(checked_point_index(
          node.upper_point_ids[axis], point_count));
    }
    if (node.is_leaf()) {
      if (node.right_child != invalid_node_index ||
          node.leaf_end - node.leaf_begin != 1U) {
        throw std::logic_error("a Boruvka LBVH leaf is malformed");
      }
      continue;
    }
    if (node.right_child == invalid_node_index ||
        node.left_child >= node_index || node.right_child >= node_index) {
      throw std::logic_error(
          "Boruvka requires postorder LBVH child indices");
    }
    const TraversalNode& left = snapshot.nodes[node.left_child];
    const TraversalNode& right = snapshot.nodes[node.right_child];
    if (node.leaf_begin != left.leaf_begin ||
        left.leaf_end != right.leaf_begin ||
        node.leaf_end != right.leaf_end) {
      throw std::logic_error(
          "a Boruvka LBVH parent range is not its child partition");
    }
  }
  const TraversalNode& root = snapshot.nodes[snapshot.root_index];
  if (root.leaf_begin != 0U || root.leaf_end != point_count) {
    throw std::logic_error("the Boruvka LBVH root does not cover the cloud");
  }
}

[[nodiscard]] std::vector<ComponentTag> build_component_tags(
    const TraversalIndexSnapshot& snapshot,
    const std::vector<spatial::PointId>& frozen_labels,
    std::size_t& uniform_count,
    std::size_t& mixed_count) {
  std::vector<ComponentTag> tags(snapshot.nodes.size());
  uniform_count = 0U;
  mixed_count = 0U;
  for (std::size_t node_index = 0U;
       node_index < snapshot.nodes.size();
       ++node_index) {
    const TraversalNode& node = snapshot.nodes[node_index];
    ComponentTag tag;
    if (node.is_leaf()) {
      const spatial::PointId point_id = snapshot.leaves[node.leaf_begin];
      tag.uniform = true;
      tag.component_label =
          frozen_labels[checked_point_index(point_id, frozen_labels.size())];
    } else {
      const ComponentTag& left = tags[node.left_child];
      const ComponentTag& right = tags[node.right_child];
      if (left.uniform && right.uniform &&
          left.component_label == right.component_label) {
        tag.uniform = true;
        tag.component_label = left.component_label;
      }
    }
    tags[node_index] = tag;
    if (tag.uniform) {
      ++uniform_count;
    } else {
      ++mixed_count;
    }
  }
  if (uniform_count + mixed_count != snapshot.nodes.size()) {
    throw std::logic_error("Boruvka LBVH tags do not cover every node");
  }
  return tags;
}

[[nodiscard]] std::optional<CandidateEdge> exact_outgoing_edge_for_point(
    const TraversalIndexSnapshot& snapshot,
    const spatial::CanonicalPointCloud& cloud,
    const std::vector<ComponentTag>& tags,
    spatial::PointId query_id,
    spatial::PointId query_component,
    K1ExactBoruvkaCounters& counters) {
  ++counters.point_query_count;
  if (tags[snapshot.root_index].uniform &&
      tags[snapshot.root_index].component_label == query_component) {
    return std::nullopt;
  }

  std::priority_queue<
      NodeQueueEntry,
      std::vector<NodeQueueEntry>,
      NearestNodeFirst>
      nodes_to_visit;
  exact::ExactLevel root_bound = exact_aabb_lower_bound(
      snapshot.nodes[snapshot.root_index], cloud, query_id);
  ++counters.exact_aabb_bound_evaluation_count;
  nodes_to_visit.push(
      NodeQueueEntry{std::move(root_bound), snapshot.root_index});

  std::optional<CandidateEdge> best;
  while (!nodes_to_visit.empty()) {
    NodeQueueEntry entry = nodes_to_visit.top();
    nodes_to_visit.pop();
    ++counters.node_visit_count;
    const ComponentTag& tag = tags[entry.node_index];
    if (tag.uniform && tag.component_label == query_component) {
      ++counters.uniform_component_prune_count;
      continue;
    }
    if (best.has_value() &&
        entry.lower_bound > best->edge.squared_length) {
      ++counters.strict_aabb_prune_count;
      continue;
    }

    const TraversalNode& node = snapshot.nodes[entry.node_index];
    if (node.is_leaf()) {
      const spatial::PointId other_id = snapshot.leaves[node.leaf_begin];
      ExactEmstEdge edge = exact_edge(cloud, query_id, other_id);
      ++counters.exact_point_distance_evaluation_count;
      CandidateEdge candidate{std::move(edge), query_id};
      if (!best.has_value() || candidate_less(candidate, *best)) {
        best = std::move(candidate);
      }
      continue;
    }

    ++counters.internal_node_expansion_count;
    for (const std::size_t child : {node.left_child, node.right_child}) {
      const ComponentTag& child_tag = tags[child];
      if (child_tag.uniform &&
          child_tag.component_label == query_component) {
        ++counters.uniform_component_prune_count;
        continue;
      }
      exact::ExactLevel child_bound =
          exact_aabb_lower_bound(snapshot.nodes[child], cloud, query_id);
      ++counters.exact_aabb_bound_evaluation_count;
      nodes_to_visit.push(NodeQueueEntry{std::move(child_bound), child});
    }
  }
  return best;
}

[[nodiscard]] K1ExactBoruvkaResult run_without_verification(
    const TraversalIndexSnapshot& snapshot,
    const spatial::CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  validate_snapshot(snapshot, point_count);

  K1ExactBoruvkaResult result;
  result.point_count = point_count;
  result.counters.point_count = point_count;
  result.counters.lbvh_node_count = snapshot.nodes.size();
  result.counters.theoretical_max_round_count =
      theoretical_maximum_round_count(point_count);
  std::vector<spatial::PointId> component_labels(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    component_labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = point_count;

  while (component_count > 1U) {
    const std::size_t round_index = result.rounds.size();
    if (round_index >= result.counters.theoretical_max_round_count) {
      throw std::logic_error("exact Boruvka exceeded ceil(log2(n)) rounds");
    }
    const std::size_t pre_component_count = component_count;
    const std::vector<spatial::PointId> frozen_labels = component_labels;
    result.counters.frozen_component_label_count += point_count;

    std::size_t uniform_node_count = 0U;
    std::size_t mixed_node_count = 0U;
    const std::vector<ComponentTag> tags = build_component_tags(
        snapshot,
        frozen_labels,
        uniform_node_count,
        mixed_node_count);
    result.counters.uniform_lbvh_node_tag_count += uniform_node_count;
    result.counters.mixed_lbvh_node_tag_count += mixed_node_count;
    if (tags[snapshot.root_index].uniform) {
      throw std::logic_error(
          "a nonterminal Boruvka round has a uniform LBVH root");
    }

    std::vector<std::optional<CandidateEdge>> component_candidates(
        point_count);
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      const spatial::PointId point_id = checked_point_id(point_index);
      std::optional<CandidateEdge> candidate =
          exact_outgoing_edge_for_point(
              snapshot,
              cloud,
              tags,
              point_id,
              frozen_labels[point_index],
              result.counters);
      if (!candidate.has_value()) {
        continue;
      }
      const std::size_t component_index = checked_point_index(
          frozen_labels[point_index], point_count);
      std::optional<CandidateEdge>& component_candidate =
          component_candidates[component_index];
      if (!component_candidate.has_value() ||
          candidate_less(*candidate, *component_candidate)) {
        component_candidate = std::move(candidate);
      }
    }

    K1BoruvkaRound round;
    round.round_index = round_index;
    round.pre_round_component_count = pre_component_count;
    round.uniform_lbvh_node_count = uniform_node_count;
    round.mixed_lbvh_node_count = mixed_node_count;
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      if (frozen_labels[point_index] != checked_point_id(point_index)) {
        continue;
      }
      if (!component_candidates[point_index].has_value()) {
        throw std::logic_error(
            "a nonterminal Boruvka component has no outgoing edge");
      }
      const CandidateEdge& candidate = *component_candidates[point_index];
      const std::size_t source_index = checked_point_index(
          candidate.source_point_id, point_count);
      const std::size_t u_index =
          checked_point_index(candidate.edge.u, point_count);
      const std::size_t v_index =
          checked_point_index(candidate.edge.v, point_count);
      if (frozen_labels[source_index] != checked_point_id(point_index) ||
          frozen_labels[u_index] == frozen_labels[v_index]) {
        throw std::logic_error(
            "a Boruvka component minimum violates frozen labels");
      }
      round.component_minima.push_back(K1BoruvkaComponentMinimum{
          checked_point_id(point_index),
          candidate.source_point_id,
          candidate.edge});
    }
    if (round.component_minima.size() != pre_component_count) {
      throw std::logic_error(
          "a Boruvka round did not emit one minimum per component");
    }
    result.counters.component_minimum_count +=
        round.component_minima.size();

    K1BoruvkaRoundContraction contraction =
        contract_exact_k1_boruvka_round(
            cloud, frozen_labels, round.component_minima);
    round.accepted_edges = std::move(contraction.accepted_edges);
    result.emst_edges.insert(
        result.emst_edges.end(),
        round.accepted_edges.begin(),
        round.accepted_edges.end());
    const std::size_t post_component_count =
        contraction.post_round_component_count;
    round.post_round_component_count = post_component_count;
    result.counters.accepted_edge_count += round.accepted_edges.size();
    result.counters.component_contraction_count +=
        pre_component_count - post_component_count;
    component_labels = std::move(
        contraction.post_round_component_labels);
    component_count = post_component_count;
    result.rounds.push_back(std::move(round));
  }

  std::sort(result.emst_edges.begin(), result.emst_edges.end(), edge_less);
  if (result.emst_edges.size() != point_count - 1U ||
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) !=
          result.emst_edges.end()) {
    throw std::logic_error("the Boruvka witness is not a canonical tree");
  }
  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  for (const ExactEmstEdge& edge : result.emst_edges) {
    total_squared_weight =
        total_squared_weight + edge.squared_length.rational();
    total_hgp_weight = total_hgp_weight + edge.merge_level.rational();
  }
  result.total_squared_weight =
      exact::ExactLevel{std::move(total_squared_weight)};
  result.total_hgp_weight = exact::ExactLevel{std::move(total_hgp_weight)};
  result.counters.round_count = result.rounds.size();
  result.counters.final_component_count = component_count;
  if (result.counters.final_component_count != 1U ||
      result.counters.round_count >
          result.counters.theoretical_max_round_count ||
      result.counters.accepted_edge_count != point_count - 1U ||
      result.counters.component_contraction_count != point_count - 1U) {
    throw std::logic_error("the final Boruvka counters do not close");
  }
  return result;
}

[[nodiscard]] bool same_round_structure(
    const K1ExactBoruvkaResult& observed,
    const K1ExactBoruvkaResult& expected) {
  if (observed.point_count != expected.point_count ||
      observed.rounds.size() != expected.rounds.size() ||
      observed.counters != expected.counters) {
    return false;
  }
  for (std::size_t round_index = 0U;
       round_index < observed.rounds.size();
       ++round_index) {
    const K1BoruvkaRound& left = observed.rounds[round_index];
    const K1BoruvkaRound& right = expected.rounds[round_index];
    if (left.round_index != right.round_index ||
        left.pre_round_component_count != right.pre_round_component_count ||
        left.post_round_component_count != right.post_round_component_count ||
        left.uniform_lbvh_node_count != right.uniform_lbvh_node_count ||
        left.mixed_lbvh_node_count != right.mixed_lbvh_node_count) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool same_component_minima(
    const K1ExactBoruvkaResult& observed,
    const K1ExactBoruvkaResult& expected) {
  if (observed.rounds.size() != expected.rounds.size()) {
    return false;
  }
  for (std::size_t round_index = 0U;
       round_index < observed.rounds.size();
       ++round_index) {
    if (observed.rounds[round_index].component_minima !=
        expected.rounds[round_index].component_minima) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool same_accepted_edges(
    const K1ExactBoruvkaResult& observed,
    const K1ExactBoruvkaResult& expected) {
  if (observed.rounds.size() != expected.rounds.size() ||
      observed.emst_edges != expected.emst_edges) {
    return false;
  }
  for (std::size_t round_index = 0U;
       round_index < observed.rounds.size();
       ++round_index) {
    if (observed.rounds[round_index].accepted_edges !=
        expected.rounds[round_index].accepted_edges) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool replay_canonical_contractions(
    const K1ExactBoruvkaResult& result) {
  if (result.point_count == 0U) {
    return false;
  }
  CanonicalDisjointSet components{result.point_count};
  std::vector<ExactEmstEdge> replayed_edges;
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const K1BoruvkaRound& round = result.rounds[round_index];
    const std::size_t pre_component_count = components.component_count();
    if (round.round_index != round_index ||
        round.pre_round_component_count != pre_component_count ||
        round.component_minima.size() != pre_component_count ||
        round.uniform_lbvh_node_count + round.mixed_lbvh_node_count !=
            result.counters.lbvh_node_count) {
      return false;
    }
    std::vector<spatial::PointId> frozen_labels(result.point_count);
    std::vector<spatial::PointId> component_labels;
    component_labels.reserve(pre_component_count);
    for (std::size_t point_index = 0U;
         point_index < result.point_count;
         ++point_index) {
      const spatial::PointId label =
          checked_point_id(components.find(point_index));
      frozen_labels[point_index] = label;
      if (label == checked_point_id(point_index)) {
        component_labels.push_back(label);
      }
    }
    for (std::size_t minimum_index = 0U;
         minimum_index < round.component_minima.size();
         ++minimum_index) {
      const K1BoruvkaComponentMinimum& minimum =
          round.component_minima[minimum_index];
      if (minimum.component_label != component_labels[minimum_index] ||
          (minimum.source_point_id != minimum.outgoing_edge.u &&
           minimum.source_point_id != minimum.outgoing_edge.v) ||
          minimum.outgoing_edge.u >= minimum.outgoing_edge.v) {
        return false;
      }
      const std::size_t source = checked_point_index(
          minimum.source_point_id, result.point_count);
      const std::size_t u = checked_point_index(
          minimum.outgoing_edge.u, result.point_count);
      const std::size_t v = checked_point_index(
          minimum.outgoing_edge.v, result.point_count);
      if (frozen_labels[source] != minimum.component_label ||
          frozen_labels[u] == frozen_labels[v] ||
          minimum.outgoing_edge.merge_level !=
              merge_level(minimum.outgoing_edge.squared_length)) {
        return false;
      }
    }

    std::vector<ExactEmstEdge> expected_accepted;
    expected_accepted.reserve(round.component_minima.size());
    for (const K1BoruvkaComponentMinimum& minimum :
         round.component_minima) {
      expected_accepted.push_back(minimum.outgoing_edge);
    }
    std::sort(
        expected_accepted.begin(), expected_accepted.end(), edge_less);
    expected_accepted.erase(
        std::unique(expected_accepted.begin(), expected_accepted.end()),
        expected_accepted.end());
    if (round.accepted_edges != expected_accepted) {
      return false;
    }
    for (const ExactEmstEdge& edge : round.accepted_edges) {
      const std::size_t u =
          checked_point_index(edge.u, result.point_count);
      const std::size_t v =
          checked_point_index(edge.v, result.point_count);
      if (frozen_labels[u] == frozen_labels[v] ||
          !components.unite(u, v)) {
        return false;
      }
      replayed_edges.push_back(edge);
    }
    const std::size_t post_component_count = components.component_count();
    if (round.post_round_component_count != post_component_count ||
        post_component_count == 0U ||
        post_component_count >= pre_component_count ||
        post_component_count > pre_component_count / 2U ||
        round.accepted_edges.size() !=
            pre_component_count - post_component_count) {
      return false;
    }
  }
  std::sort(replayed_edges.begin(), replayed_edges.end(), edge_less);
  return components.component_count() == 1U &&
         replayed_edges == result.emst_edges;
}

[[nodiscard]] bool verify_spanning_tree(
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& result) {
  if (result.point_count != cloud.size() || result.point_count == 0U ||
      result.emst_edges.size() != result.point_count - 1U ||
      !std::is_sorted(
          result.emst_edges.begin(), result.emst_edges.end(), edge_less) ||
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) !=
          result.emst_edges.end()) {
    return false;
  }
  CanonicalDisjointSet components{result.point_count};
  for (const ExactEmstEdge& edge : result.emst_edges) {
    const std::size_t u =
        checked_point_index(edge.u, result.point_count);
    const std::size_t v =
        checked_point_index(edge.v, result.point_count);
    if (edge.u >= edge.v || edge != exact_edge(cloud, edge.u, edge.v) ||
        !components.unite(u, v)) {
      return false;
    }
  }
  return components.component_count() == 1U;
}

[[nodiscard]] bool verify_exact_weights(
    const K1ExactBoruvkaResult& result) {
  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  for (const ExactEmstEdge& edge : result.emst_edges) {
    if (edge.merge_level != merge_level(edge.squared_length)) {
      return false;
    }
    total_squared_weight =
        total_squared_weight + edge.squared_length.rational();
    total_hgp_weight = total_hgp_weight + edge.merge_level.rational();
  }
  return result.total_squared_weight ==
             exact::ExactLevel{std::move(total_squared_weight)} &&
         result.total_hgp_weight ==
             exact::ExactLevel{std::move(total_hgp_weight)};
}

}  // namespace

K1BoruvkaRoundContraction contract_exact_k1_boruvka_round(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    std::span<const K1BoruvkaComponentMinimum> component_minima) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U ||
      frozen_component_labels.size() != point_count) {
    throw std::invalid_argument(
        "exact Boruvka contraction needs one frozen label per point");
  }

  std::vector<spatial::PointId> canonical_component_labels;
  canonical_component_labels.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const spatial::PointId label = frozen_component_labels[point_index];
    if (!std::in_range<std::size_t>(label)) {
      throw std::invalid_argument(
          "a frozen Boruvka component label does not fit size_t");
    }
    const std::size_t label_index = static_cast<std::size_t>(label);
    if (label_index > point_index || label_index >= point_count ||
        frozen_component_labels[label_index] != label) {
      throw std::invalid_argument(
          "a frozen Boruvka component label is not its least PointId");
    }
    if (label_index == point_index) {
      canonical_component_labels.push_back(label);
    }
  }
  const std::size_t pre_component_count =
      canonical_component_labels.size();
  if (pre_component_count <= 1U ||
      component_minima.size() != pre_component_count) {
    throw std::invalid_argument(
        "exact Boruvka contraction needs one minimum per nonterminal component");
  }

  CanonicalDisjointSet components{point_count};
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const std::size_t label_index = static_cast<std::size_t>(
        frozen_component_labels[point_index]);
    if (label_index != point_index &&
        !components.unite(label_index, point_index)) {
      throw std::logic_error(
          "a frozen Boruvka partition could not be reconstructed");
    }
  }
  if (components.component_count() != pre_component_count) {
    throw std::logic_error(
        "a frozen Boruvka partition has inconsistent component labels");
  }

  K1BoruvkaRoundContraction contraction;
  contraction.pre_round_component_count = pre_component_count;
  contraction.accepted_edges.reserve(component_minima.size());
  for (std::size_t minimum_index = 0U;
       minimum_index < component_minima.size();
       ++minimum_index) {
    const K1BoruvkaComponentMinimum& minimum =
        component_minima[minimum_index];
    if (minimum.component_label !=
        canonical_component_labels[minimum_index]) {
      throw std::invalid_argument(
          "Boruvka component minima are not ordered by canonical label");
    }
    const ExactEmstEdge& edge = minimum.outgoing_edge;
    if (edge.u >= edge.v ||
        (minimum.source_point_id != edge.u &&
         minimum.source_point_id != edge.v)) {
      throw std::invalid_argument(
          "a Boruvka component minimum has an invalid source or orientation");
    }
    const std::size_t source = checked_point_index(
        minimum.source_point_id, point_count);
    const std::size_t u = checked_point_index(edge.u, point_count);
    const std::size_t v = checked_point_index(edge.v, point_count);
    if (frozen_component_labels[source] != minimum.component_label ||
        frozen_component_labels[u] == frozen_component_labels[v] ||
        edge != exact_edge(cloud, edge.u, edge.v)) {
      throw std::invalid_argument(
          "a Boruvka component minimum violates frozen exact geometry");
    }
    contraction.accepted_edges.push_back(edge);
  }

  std::sort(
      contraction.accepted_edges.begin(),
      contraction.accepted_edges.end(),
      edge_less);
  contraction.accepted_edges.erase(
      std::unique(
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end()),
      contraction.accepted_edges.end());
  for (const ExactEmstEdge& edge : contraction.accepted_edges) {
    const std::size_t u = checked_point_index(edge.u, point_count);
    const std::size_t v = checked_point_index(edge.v, point_count);
    if (frozen_component_labels[u] == frozen_component_labels[v] ||
        !components.unite(u, v)) {
      throw std::logic_error(
          "certified Boruvka minima formed a cycle in one frozen round");
    }
  }

  contraction.post_round_component_count = components.component_count();
  if (contraction.post_round_component_count == 0U ||
      contraction.post_round_component_count >= pre_component_count ||
      contraction.post_round_component_count > pre_component_count / 2U ||
      contraction.accepted_edges.size() !=
          pre_component_count - contraction.post_round_component_count) {
    throw std::logic_error(
        "a certified Boruvka round violated its contraction bound");
  }

  contraction.post_round_component_labels.resize(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    contraction.post_round_component_labels[point_index] =
        checked_point_id(components.find(point_index));
  }
  return contraction;
}

K1ExactBoruvkaResult build_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "exact Boruvka requires a ready LBVH for the same point namespace");
  }
  TraversalIndexSnapshot snapshot;
  snapshot.leaves.reserve(index.leaves_.size());
  for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
    snapshot.leaves.push_back(leaf.point_id);
  }
  snapshot.nodes.reserve(index.nodes_.size());
  for (const auto& node : index.nodes_) {
    snapshot.nodes.push_back(TraversalNode{
        node.lower_point_ids,
        node.upper_point_ids,
        node.left_child,
        node.right_child,
        node.leaf_begin,
        node.leaf_end});
  }
  snapshot.root_index = index.root_index_;

  K1ExactBoruvkaResult result = run_without_verification(snapshot, cloud);
  const K1BoruvkaVerification verification =
      verify_exact_lbvh_boruvka(index, cloud, result);
  if (!verification.emst_witness_certified) {
    throw std::logic_error("the exact Boruvka witness failed its local replay");
  }
  result.emst_witness_certified = true;
  return result;
}

K1BoruvkaVerification verify_exact_lbvh_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& result) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "Boruvka verification requires a ready LBVH for the same point namespace");
  }
  TraversalIndexSnapshot snapshot;
  snapshot.leaves.reserve(index.leaves_.size());
  for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
    snapshot.leaves.push_back(leaf.point_id);
  }
  snapshot.nodes.reserve(index.nodes_.size());
  for (const auto& node : index.nodes_) {
    snapshot.nodes.push_back(TraversalNode{
        node.lower_point_ids,
        node.upper_point_ids,
        node.left_child,
        node.right_child,
        node.leaf_begin,
        node.leaf_end});
  }
  snapshot.root_index = index.root_index_;

  const K1ExactBoruvkaResult expected =
      run_without_verification(snapshot, cloud);
  K1BoruvkaVerification verification;
  verification.index_identity_certified = true;
  verification.replayed_round_count = expected.rounds.size();
  verification.replayed_component_minimum_count =
      expected.counters.component_minimum_count;
  verification.round_count_bound_certified =
      result.rounds.size() <= theoretical_maximum_round_count(cloud.size()) &&
      result.counters.round_count == result.rounds.size() &&
      result.counters.theoretical_max_round_count ==
          theoretical_maximum_round_count(cloud.size());
  verification.round_replay_certified =
      same_round_structure(result, expected);
  verification.component_minima_certified =
      same_component_minima(result, expected);
  verification.accepted_edges_certified =
      same_accepted_edges(result, expected);
  try {
    verification.canonical_contractions_certified =
        replay_canonical_contractions(result);
    verification.spanning_tree_certified =
        verify_spanning_tree(cloud, result);
    verification.exact_weights_certified = verify_exact_weights(result);
  } catch (const std::exception&) {
    verification.canonical_contractions_certified = false;
    verification.spanning_tree_certified = false;
    verification.exact_weights_certified = false;
  }
  verification.emst_witness_certified =
      verification.index_identity_certified &&
      verification.round_count_bound_certified &&
      verification.round_replay_certified &&
      verification.component_minima_certified &&
      verification.accepted_edges_certified &&
      verification.canonical_contractions_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
