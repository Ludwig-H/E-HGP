#include "morsehgp3d/hierarchy/k1_forest.hpp"

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
      throw std::out_of_range("a compact-forest disjoint-set identifier is out of range");
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

struct PendingMerge {
  std::vector<std::size_t> pre_batch_roots;
  K1NodeId node_id{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right != 0U && left > std::numeric_limits<std::size_t>::max() / right) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index >
      static_cast<std::size_t>(spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("the compact k=1 point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] K1NodeId checked_node_id(std::size_t index) {
  if (index > static_cast<std::size_t>(std::numeric_limits<K1NodeId>::max())) {
    throw std::length_error("the compact k=1 node index exceeds K1NodeId");
  }
  return static_cast<K1NodeId>(index);
}

[[nodiscard]] std::size_t checked_node_index(K1NodeId id) {
  if (id > static_cast<K1NodeId>(std::numeric_limits<std::size_t>::max())) {
    throw std::out_of_range("a compact k=1 node identifier does not fit size_t");
  }
  return static_cast<std::size_t>(id);
}

[[nodiscard]] exact::ExactLevel expected_merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
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

[[nodiscard]] std::size_t merge_node_index(
    const K1CompactForest& forest,
    K1NodeId node_id) {
  const std::size_t node_index = checked_node_index(node_id);
  if (node_index < forest.point_count) {
    throw std::invalid_argument("an implicit k=1 leaf is not a merge node");
  }
  const std::size_t index = node_index - forest.point_count;
  if (index >= forest.merge_nodes.size() ||
      forest.merge_nodes[index].id != node_id) {
    throw std::out_of_range("a compact k=1 merge node identifier is invalid");
  }
  return index;
}

[[nodiscard]] spatial::PointId minimum_point_id(
    const K1CompactForest& forest,
    K1NodeId node_id) {
  const std::size_t node_index = checked_node_index(node_id);
  if (node_index < forest.point_count) {
    return checked_point_id(node_index);
  }
  return forest.merge_nodes[merge_node_index(forest, node_id)].minimum_point_id;
}

[[nodiscard]] std::size_t coverage_size(
    const K1CompactForest& forest,
    K1NodeId node_id) {
  const std::size_t node_index = checked_node_index(node_id);
  if (node_index < forest.point_count) {
    return 1U;
  }
  return forest.merge_nodes[merge_node_index(forest, node_id)].coverage_size;
}

[[nodiscard]] std::vector<ExactEmstEdge> canonical_tree_edges(
    std::size_t point_count,
    std::span<const ExactEmstEdge> input_edges) {
  if (point_count == 0U) {
    throw std::invalid_argument("a compact k=1 forest requires at least one point");
  }
  static_cast<void>(checked_point_id(point_count - 1U));
  if (input_edges.size() != point_count - 1U) {
    throw std::invalid_argument(
        "a compact k=1 forest requires exactly point_count - 1 tree edges");
  }

  std::vector<ExactEmstEdge> edges;
  if (input_edges.size() > edges.max_size()) {
    throw std::length_error("the exact spanning tree exceeds vector capacity");
  }
  edges.reserve(input_edges.size());
  for (const ExactEmstEdge& input_edge : input_edges) {
    if (input_edge.u >= point_count || input_edge.v >= point_count) {
      throw std::invalid_argument(
          "an exact spanning-tree endpoint is outside the point domain");
    }
    if (input_edge.u == input_edge.v) {
      throw std::invalid_argument("an exact spanning tree cannot contain a loop");
    }
    if (input_edge.squared_length == exact::ExactLevel{}) {
      throw std::invalid_argument(
          "an exact spanning-tree edge must have positive squared length");
    }
    if (input_edge.merge_level != expected_merge_level(input_edge.squared_length)) {
      throw std::invalid_argument(
          "an exact spanning-tree merge level must equal squared_length / 4");
    }
    ExactEmstEdge edge = input_edge;
    if (edge.v < edge.u) {
      std::swap(edge.u, edge.v);
    }
    edges.push_back(std::move(edge));
  }
  std::sort(edges.begin(), edges.end(), edge_less);

  DisjointSet tree_components{point_count};
  for (const ExactEmstEdge& edge : edges) {
    if (!tree_components.unite(
            static_cast<std::size_t>(edge.u),
            static_cast<std::size_t>(edge.v))) {
      throw std::invalid_argument(
          "the exact spanning-tree edges contain a duplicate or a cycle");
    }
  }
  if (tree_components.component_count() != 1U) {
    throw std::invalid_argument("the exact spanning-tree edges are disconnected");
  }
  return edges;
}

[[nodiscard]] std::size_t compact_storage_entry_count(
    const K1CompactForest& forest) {
  std::size_t count = forest.selected_edges.size();
  count = checked_add(
      count, forest.levels.size(), "the compact storage counter overflows");
  count = checked_add(
      count, forest.merge_nodes.size(), "the compact storage counter overflows");
  count = checked_add(
      count, forest.child_ids.size(), "the compact storage counter overflows");
  return checked_add(
      count,
      forest.equal_level_batches.size(),
      "the compact storage counter overflows");
}

void validate_compact_forest(const K1CompactForest& forest) {
  if (forest.point_count == 0U) {
    throw std::logic_error("a compact k=1 forest is empty");
  }
  static_cast<void>(checked_point_id(forest.point_count - 1U));
  if (forest.selected_edges.size() != forest.point_count - 1U) {
    throw std::logic_error("the compact forest does not store one spanning tree");
  }
  if (forest.levels.size() != forest.equal_level_batches.size()) {
    throw std::logic_error("compact levels and equality batches do not close");
  }
  if (!std::is_sorted(
          forest.selected_edges.begin(), forest.selected_edges.end(), edge_less)) {
    throw std::logic_error("compact selected edges are not canonically sorted");
  }
  if (std::adjacent_find(
          forest.levels.begin(), forest.levels.end(),
          [](const exact::ExactLevel& left, const exact::ExactLevel& right) {
            return !(left < right);
          }) != forest.levels.end()) {
    throw std::logic_error("compact hierarchy levels are not unique and increasing");
  }

  DisjointSet edge_components{forest.point_count};
  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  for (const ExactEmstEdge& edge : forest.selected_edges) {
    if (edge.u >= forest.point_count || edge.v >= forest.point_count ||
        edge.u >= edge.v || edge.squared_length == exact::ExactLevel{} ||
        edge.merge_level != expected_merge_level(edge.squared_length)) {
      throw std::logic_error("a compact selected edge violates its exact contract");
    }
    if (!edge_components.unite(
            static_cast<std::size_t>(edge.u),
            static_cast<std::size_t>(edge.v))) {
      throw std::logic_error("compact selected edges are not a tree");
    }
    total_squared_weight = total_squared_weight + edge.squared_length.rational();
    total_hgp_weight = total_hgp_weight + edge.merge_level.rational();
  }
  if (edge_components.component_count() != 1U ||
      forest.total_squared_weight !=
          exact::ExactLevel{std::move(total_squared_weight)} ||
      forest.total_hgp_weight != exact::ExactLevel{std::move(total_hgp_weight)}) {
    throw std::logic_error("compact tree weights or connectivity do not close");
  }

  const std::size_t total_node_count = checked_add(
      forest.point_count,
      forest.merge_nodes.size(),
      "the compact total node count overflows");
  std::vector<std::size_t> incoming_reference_count(total_node_count, 0U);
  std::size_t expected_child_offset = 0U;
  std::size_t multifusion_count = 0U;
  std::size_t max_merge_arity = 0U;
  for (std::size_t index = 0U; index < forest.merge_nodes.size(); ++index) {
    const K1CompactMergeNode& node = forest.merge_nodes[index];
    const std::size_t expected_node_index = checked_add(
        forest.point_count, index, "the compact node identifier overflows");
    if (node.id != checked_node_id(expected_node_index) ||
        node.level_index >= forest.levels.size() ||
        node.child_offset != expected_child_offset || node.child_count < 2U ||
        node.child_count > forest.child_ids.size() - node.child_offset) {
      throw std::logic_error("a compact merge-node record is not closed");
    }
    const std::size_t child_end = checked_add(
        node.child_offset,
        node.child_count,
        "a compact child range overflows");
    if (child_end > forest.child_ids.size()) {
      throw std::logic_error("a compact child range exceeds the CSR arena");
    }

    spatial::PointId previous_minimum{};
    bool have_previous = false;
    std::size_t reconstructed_coverage_size = 0U;
    for (std::size_t child_index = node.child_offset;
         child_index < child_end;
         ++child_index) {
      const K1NodeId child_id = forest.child_ids[child_index];
      const std::size_t child_node_index = checked_node_index(child_id);
      if (child_node_index >= expected_node_index) {
        throw std::logic_error(
            "a compact merge node must reference a strict pre-batch child");
      }
      if (child_node_index >= total_node_count) {
        throw std::logic_error("a compact child identifier is out of range");
      }
      ++incoming_reference_count[child_node_index];
      const spatial::PointId child_minimum = minimum_point_id(forest, child_id);
      if (have_previous && child_minimum <= previous_minimum) {
        throw std::logic_error(
            "compact children are not ordered by minimum PointId");
      }
      previous_minimum = child_minimum;
      have_previous = true;
      reconstructed_coverage_size = checked_add(
          reconstructed_coverage_size,
          coverage_size(forest, child_id),
          "a compact coverage size overflows");
      if (child_node_index >= forest.point_count) {
        const K1CompactMergeNode& child_node =
            forest.merge_nodes[child_node_index - forest.point_count];
        if (!(forest.levels[child_node.level_index] <
              forest.levels[node.level_index])) {
          throw std::logic_error(
              "a compact child is not strict below its parent batch");
        }
      }
    }
    if (node.minimum_point_id !=
            minimum_point_id(forest, forest.child_ids[node.child_offset]) ||
        node.coverage_size != reconstructed_coverage_size ||
        node.coverage_size < node.child_count ||
        node.coverage_size > forest.point_count) {
      throw std::logic_error("compact merge-node coverage metadata is inconsistent");
    }
    if (node.child_count >= 3U) {
      ++multifusion_count;
    }
    max_merge_arity = std::max(max_merge_arity, node.child_count);
    expected_child_offset = child_end;
  }
  if (expected_child_offset != forest.child_ids.size()) {
    throw std::logic_error("compact child CSR offsets do not cover their arena");
  }

  const std::size_t root_index = checked_node_index(forest.root_node_id);
  if (root_index >= total_node_count || coverage_size(forest, forest.root_node_id) !=
                                                forest.point_count) {
    throw std::logic_error("the compact forest root does not cover every point");
  }
  for (std::size_t node_index = 0U; node_index < total_node_count; ++node_index) {
    const std::size_t expected = node_index == root_index ? 0U : 1U;
    if (incoming_reference_count[node_index] != expected) {
      throw std::logic_error(
          "the compact hierarchy is not a rooted tree with implicit leaves");
    }
  }
  const std::size_t expected_child_reference_count = checked_add(
      forest.point_count,
      forest.merge_nodes.size(),
      "the compact child identity overflows") -
      1U;
  if (forest.child_ids.size() != expected_child_reference_count) {
    throw std::logic_error("the compact child-reference identity does not close");
  }

  std::size_t edge_offset = 0U;
  std::size_t merge_node_offset = 0U;
  std::size_t previous_component_count = forest.point_count;
  for (std::size_t batch_index = 0U;
       batch_index < forest.equal_level_batches.size();
       ++batch_index) {
    const K1CompactEqualLevelBatch& batch =
        forest.equal_level_batches[batch_index];
    if (batch.level_index != batch_index ||
        batch.selected_edge_offset != edge_offset ||
        batch.merge_node_offset != merge_node_offset ||
        batch.selected_edge_count == 0U || batch.merge_node_count == 0U ||
        batch.pre_batch_component_count != previous_component_count ||
        batch.selected_edge_count > forest.selected_edges.size() - edge_offset ||
        batch.merge_node_count > forest.merge_nodes.size() - merge_node_offset) {
      throw std::logic_error("a compact equality-batch record is not closed");
    }
    const std::size_t next_edge_offset = checked_add(
        edge_offset,
        batch.selected_edge_count,
        "a compact batch edge offset overflows");
    const std::size_t next_merge_node_offset = checked_add(
        merge_node_offset,
        batch.merge_node_count,
        "a compact batch node offset overflows");
    if (next_edge_offset > forest.selected_edges.size() ||
        next_merge_node_offset > forest.merge_nodes.size() ||
        batch.pre_batch_component_count < batch.post_batch_component_count ||
        batch.pre_batch_component_count - batch.post_batch_component_count !=
            batch.selected_edge_count) {
      throw std::logic_error("compact batch component counts are inconsistent");
    }
    for (std::size_t index = edge_offset; index < next_edge_offset; ++index) {
      if (forest.selected_edges[index].merge_level != forest.levels[batch_index]) {
        throw std::logic_error("a compact batch contains an edge at another level");
      }
    }
    for (std::size_t index = merge_node_offset;
         index < next_merge_node_offset;
         ++index) {
      if (forest.merge_nodes[index].level_index != batch_index) {
        throw std::logic_error("a compact batch contains a node at another level");
      }
    }
    edge_offset = next_edge_offset;
    merge_node_offset = next_merge_node_offset;
    previous_component_count = batch.post_batch_component_count;
  }
  if (edge_offset != forest.selected_edges.size() ||
      merge_node_offset != forest.merge_nodes.size() ||
      previous_component_count != 1U) {
    throw std::logic_error("compact batch offsets do not close their arenas");
  }

  const std::size_t linear_storage_entry_count =
      compact_storage_entry_count(forest);
  const std::size_t linear_storage_entry_limit = checked_multiply(
      6U,
      forest.point_count - 1U,
      "the compact linear-storage bound overflows");
  if (linear_storage_entry_count > linear_storage_entry_limit) {
    throw std::logic_error("the compact hierarchy exceeds its linear storage bound");
  }

  const K1CompactForestCounters expected_counters{
      forest.point_count,
      forest.selected_edges.size(),
      forest.selected_edges.size(),
      edge_offset,
      forest.levels.size(),
      forest.equal_level_batches.size(),
      forest.merge_nodes.size(),
      merge_node_offset,
      forest.child_ids.size(),
      total_node_count,
      forest.merge_nodes.size(),
      multifusion_count,
      max_merge_arity,
      forest.point_count,
      0U,
      linear_storage_entry_count,
      linear_storage_entry_limit};
  if (forest.counters != expected_counters) {
    throw std::logic_error("compact forest counters are not exactly closed");
  }
}

[[nodiscard]] K1Cut materialize_cut(
    const K1CompactForest& forest,
    const exact::ExactLevel& level,
    K1CutClosure closure) {
  switch (closure) {
    case K1CutClosure::strict:
    case K1CutClosure::closed:
      break;
    default:
      throw std::invalid_argument("the compact k=1 cut closure is invalid");
  }
  if (closure == K1CutClosure::strict && level == exact::ExactLevel{}) {
    return {};
  }

  DisjointSet components{forest.point_count};
  for (const ExactEmstEdge& edge : forest.selected_edges) {
    const bool active = closure == K1CutClosure::strict
                            ? edge.merge_level < level
                            : edge.merge_level <= level;
    if (!active) {
      break;
    }
    if (edge.u >= forest.point_count || edge.v >= forest.point_count) {
      throw std::logic_error("a compact cut edge endpoint is out of range");
    }
    static_cast<void>(components.unite(
        static_cast<std::size_t>(edge.u),
        static_cast<std::size_t>(edge.v)));
  }

  std::map<std::size_t, K1CutComponent> components_by_root;
  for (std::size_t point_index = 0U;
       point_index < forest.point_count;
       ++point_index) {
    components_by_root[components.find(point_index)].push_back(
        checked_point_id(point_index));
  }
  K1Cut result;
  result.reserve(components_by_root.size());
  for (auto& [root, points] : components_by_root) {
    static_cast<void>(root);
    result.push_back(std::move(points));
  }
  std::sort(
      result.begin(), result.end(),
      [](const K1CutComponent& left, const K1CutComponent& right) {
        return left.front() < right.front();
      });
  return result;
}

}  // namespace

std::size_t K1CompactForest::node_count() const noexcept {
  return point_count + merge_nodes.size();
}

bool K1CompactForest::is_leaf(K1NodeId node_id) const {
  const std::size_t node_index = checked_node_index(node_id);
  if (node_index < point_count) {
    return true;
  }
  static_cast<void>(merge_node_index(*this, node_id));
  return false;
}

exact::ExactLevel K1CompactForest::node_level(K1NodeId node_id) const {
  if (is_leaf(node_id)) {
    return exact::ExactLevel{};
  }
  const K1CompactMergeNode& node = merge_nodes[merge_node_index(*this, node_id)];
  if (node.level_index >= levels.size()) {
    throw std::logic_error("a compact node level index is out of range");
  }
  return levels[node.level_index];
}

std::span<const K1NodeId> K1CompactForest::children(K1NodeId node_id) const {
  if (is_leaf(node_id)) {
    return {};
  }
  const K1CompactMergeNode& node = merge_nodes[merge_node_index(*this, node_id)];
  if (node.child_offset > child_ids.size() ||
      node.child_count > child_ids.size() - node.child_offset) {
    throw std::logic_error("a compact node child range is out of bounds");
  }
  return std::span<const K1NodeId>{child_ids}.subspan(
      node.child_offset, node.child_count);
}

K1Cut K1CompactForest::cut(
    const exact::ExactLevel& level,
    K1CutClosure closure) const {
  if (point_count == 0U) {
    throw std::logic_error("a compact k=1 forest must contain at least one point");
  }
  return materialize_cut(*this, level, closure);
}

K1Cut K1CompactForest::cut_strict(const exact::ExactLevel& level) const {
  return cut(level, K1CutClosure::strict);
}

K1Cut K1CompactForest::cut_closed(const exact::ExactLevel& level) const {
  return cut(level, K1CutClosure::closed);
}

K1HierarchyNode K1CompactForest::materialize_node(K1NodeId node_id) const {
  if (is_leaf(node_id)) {
    return K1HierarchyNode{
        node_id,
        exact::ExactLevel{},
        {},
        {static_cast<spatial::PointId>(node_id)}};
  }

  const K1CompactMergeNode& compact_node =
      merge_nodes[merge_node_index(*this, node_id)];
  std::vector<spatial::PointId> point_ids;
  if (compact_node.coverage_size > point_ids.max_size()) {
    throw std::length_error("a materialized compact coverage exceeds vector capacity");
  }
  point_ids.reserve(compact_node.coverage_size);
  std::vector<K1NodeId> stack{node_id};
  while (!stack.empty()) {
    const K1NodeId current = stack.back();
    stack.pop_back();
    if (is_leaf(current)) {
      point_ids.push_back(static_cast<spatial::PointId>(current));
      continue;
    }
    const std::span<const K1NodeId> current_children = children(current);
    for (const K1NodeId child : current_children) {
      if (child >= current) {
        throw std::logic_error(
            "compact materialization encountered a non-descending child");
      }
      stack.push_back(child);
    }
  }
  std::sort(point_ids.begin(), point_ids.end());
  if (point_ids.size() != compact_node.coverage_size ||
      std::adjacent_find(point_ids.begin(), point_ids.end()) != point_ids.end()) {
    throw std::logic_error("a materialized compact coverage is inconsistent");
  }
  const std::span<const K1NodeId> child_span = children(node_id);
  return K1HierarchyNode{
      node_id,
      node_level(node_id),
      std::vector<K1NodeId>{child_span.begin(), child_span.end()},
      std::move(point_ids)};
}

K1Multifusion K1CompactForest::materialize_multifusion(
    K1NodeId node_id) const {
  if (is_leaf(node_id)) {
    throw std::invalid_argument("an implicit leaf cannot materialize a multifusion");
  }
  const K1HierarchyNode node = materialize_node(node_id);
  std::vector<K1CutComponent> child_components;
  child_components.reserve(node.children.size());
  for (const K1NodeId child : node.children) {
    child_components.push_back(materialize_node(child).point_ids);
  }
  std::sort(child_components.begin(), child_components.end());
  return K1Multifusion{node.id, std::move(child_components), node.point_ids};
}

std::vector<K1HierarchyNode> K1CompactForest::materialize_nodes() const {
  std::vector<K1HierarchyNode> result;
  const std::size_t count = node_count();
  if (count > result.max_size()) {
    throw std::length_error("materialized compact nodes exceed vector capacity");
  }
  result.reserve(count);
  for (std::size_t node_index = 0U; node_index < count; ++node_index) {
    result.push_back(materialize_node(checked_node_id(node_index)));
  }
  return result;
}

std::vector<K1Multifusion> K1CompactForest::materialize_multifusions() const {
  std::vector<K1Multifusion> result;
  if (merge_nodes.size() > result.max_size()) {
    throw std::length_error("materialized multifusions exceed vector capacity");
  }
  result.reserve(merge_nodes.size());
  for (const K1CompactMergeNode& node : merge_nodes) {
    result.push_back(materialize_multifusion(node.id));
  }
  return result;
}

K1CompactForest build_compact_k1_forest(
    std::size_t point_count,
    std::span<const ExactEmstEdge> certified_emst_edges) {
  K1CompactForest result;
  result.point_count = point_count;
  result.selected_edges =
      canonical_tree_edges(point_count, certified_emst_edges);

  const std::size_t maximum_merge_node_count = point_count - 1U;
  if (maximum_merge_node_count > result.levels.max_size() ||
      maximum_merge_node_count > result.merge_nodes.max_size() ||
      maximum_merge_node_count > result.equal_level_batches.max_size()) {
    throw std::length_error("the compact hierarchy exceeds arena capacity");
  }
  result.levels.reserve(maximum_merge_node_count);
  result.merge_nodes.reserve(maximum_merge_node_count);
  result.equal_level_batches.reserve(maximum_merge_node_count);
  const std::size_t maximum_child_reference_count = checked_multiply(
      2U,
      point_count - 1U,
      "the compact child arena bound overflows");
  if (maximum_child_reference_count > result.child_ids.max_size()) {
    throw std::length_error("the compact hierarchy child arena exceeds capacity");
  }
  result.child_ids.reserve(maximum_child_reference_count);

  DisjointSet components{point_count};
  std::vector<K1NodeId> component_node_ids(point_count);
  for (std::size_t point_index = 0U; point_index < point_count; ++point_index) {
    component_node_ids[point_index] = checked_node_id(point_index);
  }

  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  std::size_t batch_begin = 0U;
  while (batch_begin < result.selected_edges.size()) {
    std::size_t batch_end = batch_begin + 1U;
    while (batch_end < result.selected_edges.size() &&
           result.selected_edges[batch_end].squared_length ==
               result.selected_edges[batch_begin].squared_length) {
      ++batch_end;
    }

    const std::size_t level_index = result.levels.size();
    result.levels.push_back(result.selected_edges[batch_begin].merge_level);
    K1CompactEqualLevelBatch batch;
    batch.level_index = level_index;
    batch.selected_edge_offset = batch_begin;
    batch.selected_edge_count = batch_end - batch_begin;
    batch.merge_node_offset = result.merge_nodes.size();
    batch.pre_batch_component_count = components.component_count();

    std::vector<std::pair<std::size_t, std::size_t>> quotient_edges;
    quotient_edges.reserve(batch.selected_edge_count);
    std::vector<std::size_t> quotient_roots;
    const std::size_t quotient_root_capacity = checked_multiply(
        2U,
        batch.selected_edge_count,
        "a compact equality batch exceeds quotient-root capacity");
    quotient_roots.reserve(quotient_root_capacity);
    for (std::size_t edge_index = batch_begin;
         edge_index < batch_end;
         ++edge_index) {
      const ExactEmstEdge& edge = result.selected_edges[edge_index];
      const std::size_t u_root =
          components.find(static_cast<std::size_t>(edge.u));
      const std::size_t v_root =
          components.find(static_cast<std::size_t>(edge.v));
      if (u_root == v_root) {
        throw std::logic_error(
            "a spanning-tree edge is redundant before its equality batch");
      }
      const auto roots = std::minmax(u_root, v_root);
      quotient_edges.emplace_back(roots.first, roots.second);
      quotient_roots.push_back(roots.first);
      quotient_roots.push_back(roots.second);
    }
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
          right_position == quotient_roots.end() ||
          *right_position != right_root) {
        throw std::logic_error(
            "a compact quotient edge references an absent pre-batch root");
      }
      if (!batch_components.unite(
              static_cast<std::size_t>(left_position - quotient_roots.begin()),
              static_cast<std::size_t>(right_position - quotient_roots.begin()))) {
        throw std::logic_error("a compact equality-batch quotient contains a cycle");
      }
    }

    std::map<std::size_t, std::vector<std::size_t>> roots_by_component;
    for (std::size_t local_index = 0U;
         local_index < quotient_roots.size();
         ++local_index) {
      roots_by_component[batch_components.find(local_index)].push_back(
          quotient_roots[local_index]);
    }

    std::vector<PendingMerge> pending_merges;
    pending_merges.reserve(roots_by_component.size());
    for (auto& [batch_root, pre_batch_roots] : roots_by_component) {
      static_cast<void>(batch_root);
      std::vector<K1NodeId> children;
      children.reserve(pre_batch_roots.size());
      for (const std::size_t root : pre_batch_roots) {
        children.push_back(component_node_ids[root]);
      }
      std::sort(
          children.begin(), children.end(),
          [&result](K1NodeId left, K1NodeId right) {
            const spatial::PointId left_minimum = minimum_point_id(result, left);
            const spatial::PointId right_minimum = minimum_point_id(result, right);
            if (left_minimum != right_minimum) {
              return left_minimum < right_minimum;
            }
            return left < right;
          });
      if (children.size() < 2U ||
          std::adjacent_find(children.begin(), children.end()) != children.end()) {
        throw std::logic_error(
            "a compact multifusion does not contain distinct frozen children");
      }

      std::size_t node_coverage_size = 0U;
      for (const K1NodeId child : children) {
        node_coverage_size = checked_add(
            node_coverage_size,
            coverage_size(result, child),
            "a compact merge-node coverage size overflows");
      }
      const std::size_t node_index = checked_add(
          point_count,
          result.merge_nodes.size(),
          "the compact merge-node identifier overflows");
      const K1NodeId node_id = checked_node_id(node_index);
      const std::size_t child_offset = result.child_ids.size();
      result.child_ids.insert(
          result.child_ids.end(), children.begin(), children.end());
      result.merge_nodes.push_back(K1CompactMergeNode{
          node_id,
          level_index,
          child_offset,
          children.size(),
          minimum_point_id(result, children.front()),
          node_coverage_size});
      pending_merges.push_back(
          PendingMerge{std::move(pre_batch_roots), node_id});
    }
    batch.merge_node_count = result.merge_nodes.size() - batch.merge_node_offset;

    for (std::size_t edge_index = batch_begin;
         edge_index < batch_end;
         ++edge_index) {
      const ExactEmstEdge& edge = result.selected_edges[edge_index];
      if (!components.unite(
              static_cast<std::size_t>(edge.u),
              static_cast<std::size_t>(edge.v))) {
        throw std::logic_error(
            "a spanning-tree equality batch did not reduce the component count");
      }
      total_squared_weight = total_squared_weight + edge.squared_length.rational();
      total_hgp_weight = total_hgp_weight + edge.merge_level.rational();
    }
    for (const PendingMerge& pending : pending_merges) {
      const std::size_t final_root =
          components.find(pending.pre_batch_roots.front());
      for (const std::size_t pre_batch_root : pending.pre_batch_roots) {
        if (components.find(pre_batch_root) != final_root) {
          throw std::logic_error(
              "a compact equality-batch multifusion was not fully contracted");
        }
      }
      component_node_ids[final_root] = pending.node_id;
    }

    batch.post_batch_component_count = components.component_count();
    if (batch.pre_batch_component_count - batch.post_batch_component_count !=
        batch.selected_edge_count) {
      throw std::logic_error(
          "a compact equality batch has inconsistent component counts");
    }
    result.equal_level_batches.push_back(batch);
    batch_begin = batch_end;
  }

  if (components.component_count() != 1U) {
    throw std::logic_error("the compact exact tree did not yield one root");
  }
  const std::size_t final_root = components.find(0U);
  result.root_node_id = component_node_ids[final_root];
  result.total_squared_weight = exact::ExactLevel{std::move(total_squared_weight)};
  result.total_hgp_weight = exact::ExactLevel{std::move(total_hgp_weight)};

  const std::size_t total_node_count = checked_add(
      point_count,
      result.merge_nodes.size(),
      "the compact total node count overflows");
  std::size_t multifusion_count = 0U;
  std::size_t max_merge_arity = 0U;
  for (const K1CompactMergeNode& node : result.merge_nodes) {
    if (node.child_count >= 3U) {
      ++multifusion_count;
    }
    max_merge_arity = std::max(max_merge_arity, node.child_count);
  }
  const std::size_t linear_storage_entry_count =
      compact_storage_entry_count(result);
  const std::size_t linear_storage_entry_limit = checked_multiply(
      6U,
      point_count - 1U,
      "the compact linear-storage bound overflows");
  result.counters = K1CompactForestCounters{
      point_count,
      certified_emst_edges.size(),
      result.selected_edges.size(),
      result.selected_edges.size(),
      result.levels.size(),
      result.equal_level_batches.size(),
      result.merge_nodes.size(),
      result.merge_nodes.size(),
      result.child_ids.size(),
      total_node_count,
      result.merge_nodes.size(),
      multifusion_count,
      max_merge_arity,
      coverage_size(result, result.root_node_id),
      0U,
      linear_storage_entry_count,
      linear_storage_entry_limit};

  validate_compact_forest(result);
  return result;
}

K1CompactForest build_compact_k1_forest(const K1EmstResult& emst) {
  return build_compact_k1_forest(emst.point_count, emst.emst_edges);
}

}  // namespace morsehgp3d::hierarchy
