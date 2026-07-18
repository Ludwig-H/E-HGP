#include "morsehgp3d/hierarchy/emst.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t size)
      : parents_(size), component_count_(size) {
    std::iota(parents_.begin(), parents_.end(), std::size_t{0});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    if (value >= parents_.size()) {
      throw std::out_of_range("a disjoint-set identifier is out of range");
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
    const std::size_t child = std::max(left, right);
    parents_[child] = root;
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

struct CompleteGraphEdge {
  spatial::PointId u{};
  spatial::PointId v{};
  exact::ExactLevel squared_length{};
};

struct PendingMerge {
  std::vector<std::size_t> pre_batch_roots;
  K1NodeId node_id{};
};

[[nodiscard]] std::size_t checked_complete_graph_edge_count(
    std::size_t point_count) {
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
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error("the complete Euclidean graph edge count overflows size_t");
  }
  return left * right;
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("the point index exceeds the canonical PointId domain");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] K1NodeId checked_node_id(std::size_t index) {
  if (index > static_cast<std::size_t>(std::numeric_limits<K1NodeId>::max())) {
    throw std::length_error("the k=1 hierarchy node count exceeds K1NodeId");
  }
  return static_cast<K1NodeId>(index);
}

[[nodiscard]] std::size_t checked_node_index(K1NodeId id, std::size_t size) {
  if (id > static_cast<K1NodeId>(std::numeric_limits<std::size_t>::max())) {
    throw std::logic_error("a k=1 hierarchy node identifier does not fit size_t");
  }
  const std::size_t index = static_cast<std::size_t>(id);
  if (index >= size) {
    throw std::logic_error("a k=1 hierarchy node identifier is out of range");
  }
  return index;
}

[[nodiscard]] exact::ExactLevel exact_squared_length(
    const exact::CertifiedPoint3& left,
    const exact::CertifiedPoint3& right) {
  exact::ExactRational squared_length;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational delta =
        left.coordinate(axis) - right.coordinate(axis);
    squared_length = squared_length + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_length)};
}

[[nodiscard]] exact::ExactLevel merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
}

[[nodiscard]] ExactEmstEdge public_edge(const CompleteGraphEdge& edge) {
  return ExactEmstEdge{
      edge.u, edge.v, edge.squared_length, merge_level(edge.squared_length)};
}

[[nodiscard]] bool complete_edge_less(
    const CompleteGraphEdge& left,
    const CompleteGraphEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] std::vector<CompleteGraphEdge> complete_graph_edges(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t edge_count) {
  std::vector<CompleteGraphEdge> edges;
  if (edge_count > edges.max_size()) {
    throw std::length_error("the complete Euclidean graph exceeds vector capacity");
  }
  edges.reserve(edge_count);
  for (std::size_t right_index = 1U; right_index < cloud.size(); ++right_index) {
    const spatial::PointId v = checked_point_id(right_index);
    for (std::size_t left_index = 0U; left_index < right_index; ++left_index) {
      const spatial::PointId u = checked_point_id(left_index);
      exact::ExactLevel squared_length =
          exact_squared_length(cloud.point(u), cloud.point(v));
      if (squared_length == exact::ExactLevel{}) {
        throw std::invalid_argument(
            "the exact EMST requires distinct geometric points");
      }
      edges.push_back(CompleteGraphEdge{u, v, std::move(squared_length)});
    }
  }
  if (edges.size() != edge_count) {
    throw std::logic_error("the complete Euclidean graph edge count is inconsistent");
  }
  std::sort(edges.begin(), edges.end(), complete_edge_less);
  return edges;
}

[[nodiscard]] K1HierarchyNode leaf_node(spatial::PointId point_id) {
  return K1HierarchyNode{
      static_cast<K1NodeId>(point_id),
      exact::ExactLevel{},
      {},
      {point_id}};
}

[[nodiscard]] K1HierarchyNode merge_node(
    const exact::ExactLevel& level,
    std::span<const std::size_t> pre_batch_roots,
    std::span<const K1NodeId> component_node_ids,
    const std::vector<K1HierarchyNode>& existing_nodes) {
  std::vector<K1NodeId> children;
  children.reserve(pre_batch_roots.size());
  for (const std::size_t root : pre_batch_roots) {
    if (root >= component_node_ids.size()) {
      throw std::logic_error("a pre-batch root is outside the point table");
    }
    children.push_back(component_node_ids[root]);
  }
  std::sort(
      children.begin(), children.end(),
      [&existing_nodes](K1NodeId left, K1NodeId right) {
        const K1HierarchyNode& left_node =
            existing_nodes[checked_node_index(left, existing_nodes.size())];
        const K1HierarchyNode& right_node =
            existing_nodes[checked_node_index(right, existing_nodes.size())];
        if (left_node.point_ids != right_node.point_ids) {
          return left_node.point_ids < right_node.point_ids;
        }
        return left < right;
      });
  if (std::adjacent_find(children.begin(), children.end()) != children.end()) {
    throw std::logic_error("a multifusion contains a duplicate pre-batch child");
  }

  std::vector<spatial::PointId> point_ids;
  for (const K1NodeId child : children) {
    const K1HierarchyNode& child_node =
        existing_nodes[checked_node_index(child, existing_nodes.size())];
    point_ids.insert(
        point_ids.end(), child_node.point_ids.begin(), child_node.point_ids.end());
  }
  std::sort(point_ids.begin(), point_ids.end());
  if (std::adjacent_find(point_ids.begin(), point_ids.end()) != point_ids.end()) {
    throw std::logic_error("pre-batch components of a multifusion overlap");
  }
  if (children.size() < 2U || point_ids.size() < 2U) {
    throw std::logic_error("a k=1 merge node must join at least two components");
  }

  return K1HierarchyNode{
      checked_node_id(existing_nodes.size()),
      level,
      std::move(children),
      std::move(point_ids)};
}

[[nodiscard]] K1Multifusion multifusion_from_node(
    const K1HierarchyNode& node,
    const std::vector<K1HierarchyNode>& existing_nodes) {
  if (node.is_leaf()) {
    throw std::logic_error("a leaf cannot describe a k=1 multifusion");
  }
  std::vector<K1CutComponent> child_components;
  child_components.reserve(node.children.size());
  for (const K1NodeId child : node.children) {
    child_components.push_back(
        existing_nodes[checked_node_index(child, existing_nodes.size())].point_ids);
  }
  std::sort(child_components.begin(), child_components.end());
  return K1Multifusion{node.id, std::move(child_components), node.point_ids};
}

}  // namespace

K1Cut K1EmstResult::cut(
    const exact::ExactLevel& level,
    K1CutClosure closure,
    K1CutEdgeSource edge_source) const {
  if (point_count == 0U) {
    throw std::logic_error("a k=1 EMST result must contain at least one point");
  }
  switch (closure) {
    case K1CutClosure::strict:
    case K1CutClosure::closed:
      break;
    default:
      throw std::invalid_argument("the k=1 cut closure is invalid");
  }
  const std::vector<ExactEmstEdge>* replay_edges = nullptr;
  switch (edge_source) {
    case K1CutEdgeSource::complete_graph:
      replay_edges = &complete_edges;
      break;
    case K1CutEdgeSource::selected_emst:
      replay_edges = &emst_edges;
      break;
    default:
      throw std::invalid_argument("the k=1 cut edge source is invalid");
  }
  if (closure == K1CutClosure::strict && level == exact::ExactLevel{}) {
    return {};
  }
  if (point_count - 1U >
      static_cast<std::size_t>(spatial::CanonicalPointCloud::max_point_id)) {
    throw std::logic_error("the k=1 result point count exceeds PointId");
  }
  DisjointSet components{point_count};
  for (const ExactEmstEdge& edge : *replay_edges) {
    const bool active = closure == K1CutClosure::strict
                            ? edge.merge_level < level
                            : edge.merge_level <= level;
    if (!active) {
      continue;
    }
    if (edge.u >= point_count || edge.v >= point_count) {
      throw std::logic_error("a k=1 EMST edge endpoint is out of range");
    }
    static_cast<void>(components.unite(
        static_cast<std::size_t>(edge.u),
        static_cast<std::size_t>(edge.v)));
  }

  std::map<std::size_t, K1CutComponent> components_by_root;
  for (std::size_t index = 0U; index < point_count; ++index) {
    components_by_root[components.find(index)].push_back(checked_point_id(index));
  }

  K1Cut result;
  result.reserve(components_by_root.size());
  for (auto& [root, point_ids] : components_by_root) {
    static_cast<void>(root);
    result.push_back(std::move(point_ids));
  }
  std::sort(
      result.begin(), result.end(),
      [](const K1CutComponent& left, const K1CutComponent& right) {
        return left.front() < right.front();
      });
  return result;
}

K1EmstResult build_exact_complete_graph_emst(
    const spatial::CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "the exact complete-graph EMST requires a nonempty canonical point cloud");
  }
  const std::size_t edge_count =
      checked_complete_graph_edge_count(point_count);
  const std::vector<CompleteGraphEdge> edges =
      complete_graph_edges(cloud, edge_count);

  K1EmstResult result;
  result.point_count = point_count;
  result.complete_graph_edge_count = edge_count;
  if (edge_count > result.complete_edges.max_size()) {
    throw std::length_error("the public complete graph exceeds vector capacity");
  }
  result.complete_edges.reserve(edge_count);
  for (const CompleteGraphEdge& edge : edges) {
    result.complete_edges.push_back(public_edge(edge));
  }
  result.emst_edges.reserve(point_count - 1U);
  if (point_count > result.nodes.max_size()) {
    throw std::length_error("the k=1 leaves exceed hierarchy node capacity");
  }
  result.nodes.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    result.nodes.push_back(leaf_node(checked_point_id(index)));
  }

  DisjointSet components{point_count};
  std::vector<K1NodeId> component_node_ids(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    component_node_ids[index] = checked_node_id(index);
  }

  exact::ExactRational total_squared_weight;
  exact::ExactRational total_merge_weight;
  std::size_t batch_begin = 0U;
  while (batch_begin < edges.size()) {
    std::size_t batch_end = batch_begin + 1U;
    while (batch_end < edges.size() &&
           edges[batch_end].squared_length == edges[batch_begin].squared_length) {
      ++batch_end;
    }

    K1EqualLevelBatch batch;
    batch.squared_length = edges[batch_begin].squared_length;
    batch.level = merge_level(batch.squared_length);
    batch.candidate_edge_count = batch_end - batch_begin;
    batch.complete_edges.reserve(batch.candidate_edge_count);
    for (std::size_t edge_index = batch_begin; edge_index < batch_end; ++edge_index) {
      batch.complete_edges.push_back(result.complete_edges[edge_index]);
    }
    batch.pre_batch_component_count = components.component_count();

    std::vector<std::pair<std::size_t, std::size_t>> quotient_edges;
    quotient_edges.reserve(batch.candidate_edge_count);
    std::vector<std::size_t> quotient_roots;
    if (batch.candidate_edge_count > quotient_roots.max_size() / 2U) {
      throw std::length_error("an equality batch exceeds quotient-root capacity");
    }
    quotient_roots.reserve(batch.candidate_edge_count * 2U);
    for (std::size_t edge_index = batch_begin; edge_index < batch_end; ++edge_index) {
      const CompleteGraphEdge& edge = edges[edge_index];
      const std::size_t u_root = components.find(static_cast<std::size_t>(edge.u));
      const std::size_t v_root = components.find(static_cast<std::size_t>(edge.v));
      if (u_root == v_root) {
        continue;
      }
      const auto roots = std::minmax(u_root, v_root);
      quotient_edges.emplace_back(roots.first, roots.second);
      quotient_roots.push_back(roots.first);
      quotient_roots.push_back(roots.second);
    }
    std::sort(quotient_edges.begin(), quotient_edges.end());
    quotient_edges.erase(
        std::unique(quotient_edges.begin(), quotient_edges.end()),
        quotient_edges.end());
    std::sort(quotient_roots.begin(), quotient_roots.end());
    quotient_roots.erase(
        std::unique(quotient_roots.begin(), quotient_roots.end()),
        quotient_roots.end());

    DisjointSet batch_components{quotient_roots.size()};
    for (const auto& [left_root, right_root] : quotient_edges) {
      const auto left_position = std::lower_bound(
          quotient_roots.begin(), quotient_roots.end(), left_root);
      const auto right_position = std::lower_bound(
          quotient_roots.begin(), quotient_roots.end(), right_root);
      if (left_position == quotient_roots.end() || *left_position != left_root ||
          right_position == quotient_roots.end() || *right_position != right_root) {
        throw std::logic_error("a quotient edge references an absent pre-batch root");
      }
      static_cast<void>(batch_components.unite(
          static_cast<std::size_t>(left_position - quotient_roots.begin()),
          static_cast<std::size_t>(right_position - quotient_roots.begin())));
    }

    std::map<std::size_t, std::vector<std::size_t>> roots_by_batch_component;
    for (std::size_t local_index = 0U;
         local_index < quotient_roots.size();
         ++local_index) {
      roots_by_batch_component[batch_components.find(local_index)].push_back(
          quotient_roots[local_index]);
    }

    std::vector<PendingMerge> pending_merges;
    pending_merges.reserve(roots_by_batch_component.size());
    for (auto& [batch_root, pre_batch_roots] : roots_by_batch_component) {
      static_cast<void>(batch_root);
      K1HierarchyNode node = merge_node(
          batch.level, pre_batch_roots, component_node_ids, result.nodes);
      const K1NodeId node_id = node.id;
      batch.multifusions.push_back(multifusion_from_node(node, result.nodes));
      result.nodes.push_back(std::move(node));
      batch.merge_node_ids.push_back(node_id);
      pending_merges.push_back(PendingMerge{std::move(pre_batch_roots), node_id});
    }

    for (std::size_t edge_index = batch_begin; edge_index < batch_end; ++edge_index) {
      const CompleteGraphEdge& edge = edges[edge_index];
      if (!components.unite(
              static_cast<std::size_t>(edge.u),
              static_cast<std::size_t>(edge.v))) {
        continue;
      }
      ExactEmstEdge selected_edge = result.complete_edges[edge_index];
      total_squared_weight =
          total_squared_weight + selected_edge.squared_length.rational();
      total_merge_weight =
          total_merge_weight + selected_edge.merge_level.rational();
      result.emst_edges.push_back(selected_edge);
      batch.selected_edges.push_back(std::move(selected_edge));
    }

    for (const PendingMerge& pending : pending_merges) {
      const std::size_t final_root =
          components.find(pending.pre_batch_roots.front());
      for (const std::size_t pre_batch_root : pending.pre_batch_roots) {
        if (components.find(pre_batch_root) != final_root) {
          throw std::logic_error("an equality-batch multifusion was not fully contracted");
        }
      }
      component_node_ids[final_root] = pending.node_id;
    }

    batch.post_batch_component_count = components.component_count();
    if (batch.pre_batch_component_count - batch.post_batch_component_count !=
        batch.selected_edges.size()) {
      throw std::logic_error("a Kruskal equality batch has inconsistent component counts");
    }
    result.equal_level_batches.push_back(std::move(batch));
    batch_begin = batch_end;
  }

  if (components.component_count() != 1U ||
      result.emst_edges.size() != point_count - 1U) {
    throw std::logic_error("the complete Euclidean graph did not yield a spanning tree");
  }
  result.total_squared_weight = exact::ExactLevel{std::move(total_squared_weight)};
  result.total_hgp_weight = exact::ExactLevel{std::move(total_merge_weight)};
  const std::size_t final_root = components.find(0U);
  result.root_node_id = component_node_ids[final_root];

  result.counters.point_count = point_count;
  result.counters.distance_evaluations = edge_count;
  result.counters.complete_edge_count = edge_count;
  result.counters.distinct_edge_weight_count = result.equal_level_batches.size();
  result.counters.selected_edge_count = result.emst_edges.size();
  result.counters.redundant_edge_count = edge_count - result.emst_edges.size();
  result.counters.replay_level_count = result.equal_level_batches.size() + 1U;
  for (const K1EqualLevelBatch& batch : result.equal_level_batches) {
    if (batch.candidate_edge_count > 1U) {
      ++result.counters.equal_weight_batch_count;
    }
    result.counters.max_equal_weight_batch_size = std::max(
        result.counters.max_equal_weight_batch_size,
        batch.candidate_edge_count);
    if (!batch.multifusions.empty()) {
      ++result.counters.merge_batch_count;
    }
    result.counters.merge_event_count += batch.multifusions.size();
    for (const K1Multifusion& multifusion : batch.multifusions) {
      result.counters.max_merge_arity = std::max(
          result.counters.max_merge_arity,
          multifusion.arity());
      if (multifusion.arity() >= 3U) {
        ++result.counters.multifusion_count;
      }
    }
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
