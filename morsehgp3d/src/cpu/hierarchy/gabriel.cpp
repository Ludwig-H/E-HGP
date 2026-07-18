#include "morsehgp3d/hierarchy/gabriel.hpp"

#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
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

struct PendingMerge {
  std::vector<std::size_t> pre_batch_roots;
  K1NodeId node_id{};
};

struct FusionSignature {
  exact::ExactLevel level{};
  std::vector<K1CutComponent> child_components;
  K1CutComponent merged_component;

  friend bool operator==(const FusionSignature&, const FusionSignature&) =
      default;
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
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
  return checked_multiply(
      left,
      right,
      "the canonical pair count overflows size_t");
}

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (!std::in_range<spatial::PointId>(index) ||
      static_cast<spatial::PointId>(index) >
          spatial::CanonicalPointCloud::max_point_id) {
    throw std::length_error("the point index exceeds the canonical PointId domain");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] K1NodeId checked_node_id(std::size_t index) {
  if (!std::in_range<K1NodeId>(index)) {
    throw std::length_error("the k=1 hierarchy node count exceeds K1NodeId");
  }
  return static_cast<K1NodeId>(index);
}

[[nodiscard]] std::size_t checked_node_index(K1NodeId id, std::size_t size) {
  if (!std::in_range<std::size_t>(id)) {
    throw std::logic_error("a k=1 hierarchy node identifier does not fit size_t");
  }
  const std::size_t index = static_cast<std::size_t>(id);
  if (index >= size) {
    throw std::logic_error("a k=1 hierarchy node identifier is out of range");
  }
  return index;
}

[[nodiscard]] exact::ExactLevel scaled_level(
    const exact::ExactLevel& level,
    const exact::BigInt& factor) {
  if (factor <= 0) {
    throw std::invalid_argument("an exact level scale must be positive");
  }
  return exact::ExactLevel{
      level.numerator() * factor,
      level.denominator()};
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

[[nodiscard]] bool edge_is_in_sorted(
    std::span<const ExactEmstEdge> edges,
    const ExactEmstEdge& edge) {
  const auto position = std::lower_bound(
      edges.begin(), edges.end(), edge, edge_less);
  return position != edges.end() && *position == edge;
}

[[nodiscard]] bool strictly_increasing_ids(
    std::span<const spatial::PointId> ids) {
  return std::adjacent_find(
             ids.begin(), ids.end(),
             [](spatial::PointId left, spatial::PointId right) {
               return left >= right;
             }) == ids.end();
}

void require_ids_in_cloud(
    std::span<const spatial::PointId> ids,
    std::size_t point_count,
    const char* message) {
  if (!strictly_increasing_ids(ids)) {
    throw std::logic_error(message);
  }
  for (const spatial::PointId id : ids) {
    if (!std::in_range<std::size_t>(id) ||
        static_cast<std::size_t>(id) >= point_count) {
      throw std::logic_error(message);
    }
  }
}

[[nodiscard]] K1PairSphereClassification expected_classification(
    const K1PairSphereRecord& pair) {
  if (!pair.interior_ids.empty()) {
    return K1PairSphereClassification::interior_blocked;
  }
  if (pair.shell_ids.size() == 2U) {
    return K1PairSphereClassification::rank_two_critical;
  }
  return K1PairSphereClassification::extra_shell_degeneracy;
}

void require_pair_record(
    const K1PairSphereRecord& pair,
    std::size_t expected_index,
    spatial::PointId expected_u,
    spatial::PointId expected_v,
    std::size_t point_count) {
  if (pair.pair_index != expected_index || pair.u != expected_u ||
      pair.v != expected_v || pair.u >= pair.v ||
      !std::in_range<std::size_t>(pair.v) ||
      static_cast<std::size_t>(pair.v) >= point_count) {
    throw std::logic_error(
        "a pair-sphere record is outside its canonical pair enumeration");
  }
  if (pair.decision_status != K1PairDecisionStatus::exact) {
    throw std::logic_error("a returned pair-sphere decision is not exact");
  }
  if (pair.squared_length == exact::ExactLevel{} ||
      scaled_level(pair.level, exact::BigInt{4}) != pair.squared_length) {
    throw std::logic_error(
        "a pair-sphere level is not one quarter of its squared length");
  }
  require_ids_in_cloud(
      pair.interior_ids,
      point_count,
      "a pair-sphere interior is not a canonical PointId set");
  require_ids_in_cloud(
      pair.shell_ids,
      point_count,
      "a pair-sphere shell is not a canonical PointId set");
  if (!std::binary_search(pair.shell_ids.begin(), pair.shell_ids.end(), pair.u) ||
      !std::binary_search(pair.shell_ids.begin(), pair.shell_ids.end(), pair.v)) {
    throw std::logic_error(
        "a diametric pair-sphere shell omitted a support endpoint");
  }
  for (const spatial::PointId id : pair.interior_ids) {
    if (std::binary_search(pair.shell_ids.begin(), pair.shell_ids.end(), id)) {
      throw std::logic_error(
          "a pair-sphere point cannot be both interior and on the shell");
    }
  }
  const std::size_t closed_rank = checked_add(
      pair.interior_ids.size(),
      pair.shell_ids.size(),
      "a pair-sphere closed rank overflows size_t");
  if (pair.closed_rank != closed_rank || closed_rank > point_count ||
      pair.exterior_count != point_count - closed_rank) {
    throw std::logic_error(
        "a pair-sphere interior, shell and exterior count do not close");
  }
  if (pair.classification != expected_classification(pair)) {
    throw std::logic_error(
        "a pair-sphere classification contradicts its exact partition");
  }
}

void require_catalog(const K1PairSphereCatalog& catalog) {
  if (catalog.point_count == 0U ||
      catalog.decision_status != K1PairDecisionStatus::exact) {
    throw std::invalid_argument(
        "a rank-two reduction requires a nonempty exact pair catalogue");
  }
  const std::size_t pair_count = checked_pair_count(catalog.point_count);
  if (catalog.pairs.size() != pair_count) {
    throw std::logic_error(
        "the pair catalogue does not contain every canonical pair");
  }

  std::vector<ExactEmstEdge> all_edges;
  std::vector<ExactEmstEdge> rank_two_edges;
  std::vector<ExactEmstEdge> gabriel_edges;
  std::vector<std::size_t> rank_two_indices;
  std::vector<std::size_t> gabriel_indices;
  all_edges.reserve(pair_count);
  rank_two_edges.reserve(catalog.rank_two_edges.size());
  gabriel_edges.reserve(catalog.gabriel_diagnostic_edges.size());
  rank_two_indices.reserve(catalog.rank_two_pair_indices.size());
  gabriel_indices.reserve(catalog.gabriel_diagnostic_pair_indices.size());

  std::size_t index = 0U;
  std::size_t rank_two_count = 0U;
  std::size_t extra_shell_count = 0U;
  std::size_t interior_blocked_count = 0U;
  for (std::size_t left_index = 0U;
       left_index < catalog.point_count;
       ++left_index) {
    const spatial::PointId u = checked_point_id(left_index);
    for (std::size_t right_index = left_index + 1U;
         right_index < catalog.point_count;
         ++right_index) {
      const spatial::PointId v = checked_point_id(right_index);
      const K1PairSphereRecord& pair = catalog.pairs[index];
      require_pair_record(pair, index, u, v, catalog.point_count);
      ExactEmstEdge edge = pair.edge();
      all_edges.push_back(edge);
      switch (pair.classification) {
        case K1PairSphereClassification::rank_two_critical:
          rank_two_indices.push_back(index);
          gabriel_indices.push_back(index);
          rank_two_edges.push_back(edge);
          gabriel_edges.push_back(std::move(edge));
          ++rank_two_count;
          break;
        case K1PairSphereClassification::extra_shell_degeneracy:
          gabriel_indices.push_back(index);
          gabriel_edges.push_back(std::move(edge));
          ++extra_shell_count;
          break;
        case K1PairSphereClassification::interior_blocked:
          ++interior_blocked_count;
          break;
        default:
          throw std::logic_error("a pair-sphere classification is invalid");
      }
      ++index;
    }
  }
  std::sort(all_edges.begin(), all_edges.end(), edge_less);
  std::sort(rank_two_edges.begin(), rank_two_edges.end(), edge_less);
  std::sort(gabriel_edges.begin(), gabriel_edges.end(), edge_less);
  if (catalog.all_pair_edges != all_edges ||
      catalog.rank_two_edges != rank_two_edges ||
      catalog.gabriel_diagnostic_edges != gabriel_edges ||
      catalog.rank_two_pair_indices != rank_two_indices ||
      catalog.gabriel_diagnostic_pair_indices != gabriel_indices) {
    throw std::logic_error(
        "pair catalogue indices or edge projections are inconsistent");
  }

  const std::size_t gabriel_count = checked_add(
      rank_two_count,
      extra_shell_count,
      "the Gabriel diagnostic count overflows size_t");
  const std::size_t classified_count = checked_add(
      gabriel_count,
      interior_blocked_count,
      "the pair classification count overflows size_t");
  const std::size_t predicate_count = checked_multiply(
      pair_count,
      2U,
      "the pair support predicate count overflows size_t");
  const std::size_t distance_count = checked_multiply(
      pair_count,
      catalog.point_count,
      "the pair closed-ball distance count overflows size_t");
  if (classified_count != pair_count ||
      catalog.counters.point_count != catalog.point_count ||
      catalog.counters.pair_count != pair_count ||
      catalog.counters.support_analysis_count != pair_count ||
      catalog.counters.support_predicate_decision_count != predicate_count ||
      catalog.counters.closed_ball_query_count != pair_count ||
      catalog.counters.exact_point_distance_evaluation_count != distance_count ||
      catalog.counters.rank_two_critical_count != rank_two_count ||
      catalog.counters.extra_shell_degeneracy_count != extra_shell_count ||
      catalog.counters.interior_blocked_count != interior_blocked_count ||
      catalog.counters.gabriel_diagnostic_count != gabriel_count) {
    throw std::logic_error("the pair catalogue counters do not close");
  }
  const K1PairCatalogStatus expected_status =
      extra_shell_count == 0U
          ? K1PairCatalogStatus::supported
          : K1PairCatalogStatus::unsupported_degeneracy;
  if (catalog.catalog_status != expected_status ||
      !catalog.exact_decisions_complete()) {
    throw std::logic_error(
        "the pair catalogue status contradicts its exact decisions");
  }
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
  if (children.size() < 2U || point_ids.size() < 2U ||
      std::adjacent_find(point_ids.begin(), point_ids.end()) != point_ids.end()) {
    throw std::logic_error("a rank-two multifusion has invalid pre-batch children");
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

void require_edge(
    const ExactEmstEdge& edge,
    std::size_t point_count) {
  if (edge.u >= edge.v || !std::in_range<std::size_t>(edge.v) ||
      static_cast<std::size_t>(edge.v) >= point_count ||
      edge.squared_length == exact::ExactLevel{} ||
      scaled_level(edge.merge_level, exact::BigInt{4}) !=
          edge.squared_length) {
    throw std::logic_error("an exact rank-two edge is invalid");
  }
}

[[nodiscard]] K1Cut replay_cut(
    std::size_t point_count,
    std::span<const ExactEmstEdge> edges,
    const exact::ExactLevel& level,
    K1CutClosure closure) {
  if (point_count == 0U) {
    throw std::logic_error("a k=1 reduction must contain at least one point");
  }
  switch (closure) {
    case K1CutClosure::strict:
    case K1CutClosure::closed:
      break;
    default:
      throw std::invalid_argument("the k=1 cut closure is invalid");
  }
  if (closure == K1CutClosure::strict && level == exact::ExactLevel{}) {
    return {};
  }
  DisjointSet components{point_count};
  for (const ExactEmstEdge& edge : edges) {
    require_edge(edge, point_count);
    const bool active = closure == K1CutClosure::strict
                            ? edge.merge_level < level
                            : edge.merge_level <= level;
    if (active) {
      static_cast<void>(components.unite(
          static_cast<std::size_t>(edge.u),
          static_cast<std::size_t>(edge.v)));
    }
  }

  std::map<std::size_t, K1CutComponent> by_root;
  for (std::size_t index = 0U; index < point_count; ++index) {
    by_root[components.find(index)].push_back(checked_point_id(index));
  }
  K1Cut result;
  result.reserve(by_root.size());
  for (auto& [root, component] : by_root) {
    static_cast<void>(root);
    result.push_back(std::move(component));
  }
  std::sort(result.begin(), result.end());
  return result;
}

[[nodiscard]] std::vector<FusionSignature> emst_fusion_signatures(
    const K1EmstResult& emst) {
  std::vector<FusionSignature> signatures;
  for (const K1EqualLevelBatch& batch : emst.equal_level_batches) {
    for (const K1Multifusion& fusion : batch.multifusions) {
      signatures.push_back(FusionSignature{
          batch.level, fusion.child_components, fusion.merged_component});
    }
  }
  return signatures;
}

[[nodiscard]] std::vector<FusionSignature> rank_two_fusion_signatures(
    const K1RankTwoReductionResult& reduction) {
  std::vector<FusionSignature> signatures;
  for (const K1RankTwoEqualLevelBatch& batch : reduction.equal_level_batches) {
    for (const K1Multifusion& fusion : batch.multifusions) {
      signatures.push_back(FusionSignature{
          batch.level, fusion.child_components, fusion.merged_component});
    }
  }
  return signatures;
}

[[nodiscard]] std::vector<exact::ExactLevel> comparison_levels(
    const K1EmstResult& emst,
    const K1PairSphereCatalog& catalog,
    const K1RankTwoReductionResult& reduction) {
  std::vector<exact::ExactLevel> levels;
  const std::size_t first_count = checked_add(
      emst.complete_edges.size(),
      catalog.all_pair_edges.size(),
      "the k=1 comparison level count overflows size_t");
  const std::size_t second_count = checked_add(
      reduction.rank_two_edges.size(),
      reduction.gabriel_diagnostic_edges.size(),
      "the k=1 comparison level count overflows size_t");
  levels.reserve(checked_add(
      checked_add(
          first_count,
          second_count,
          "the k=1 comparison level count overflows size_t"),
      1U,
      "the k=1 comparison level count overflows size_t"));
  levels.emplace_back();
  for (const ExactEmstEdge& edge : emst.complete_edges) {
    levels.push_back(edge.merge_level);
  }
  for (const ExactEmstEdge& edge : catalog.all_pair_edges) {
    levels.push_back(edge.merge_level);
  }
  for (const ExactEmstEdge& edge : reduction.rank_two_edges) {
    levels.push_back(edge.merge_level);
  }
  for (const ExactEmstEdge& edge : reduction.gabriel_diagnostic_edges) {
    levels.push_back(edge.merge_level);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
  return levels;
}

[[nodiscard]] K1ExactAnchorCertificate certify_anchor(
    const K1EmstResult& emst,
    const K1PairSphereCatalog& catalog,
    const K1RankTwoReductionResult& reduction) {
  K1ExactAnchorCertificate certificate;
  certificate.exact_pair_decisions_complete =
      catalog.exact_decisions_complete();
  certificate.pair_universe_matches_emst =
      catalog.all_pair_edges == emst.complete_edges;

  const std::vector<exact::ExactLevel> levels =
      comparison_levels(emst, catalog, reduction);
  certificate.comparison_level_count = levels.size();
  certificate.strict_cuts_match = true;
  certificate.closed_cuts_match = true;
  for (const exact::ExactLevel& level : levels) {
    for (const K1CutClosure closure : {
             K1CutClosure::strict,
             K1CutClosure::closed}) {
      const K1Cut reference = emst.cut(
          level, closure, K1CutEdgeSource::complete_graph);
      const bool matches =
          emst.cut(level, closure, K1CutEdgeSource::selected_emst) == reference &&
          reduction.cut(
              level,
              closure,
              K1RankTwoCutEdgeSource::rank_two_graph) == reference &&
          reduction.cut(
              level,
              closure,
              K1RankTwoCutEdgeSource::selected_witness_tree) == reference &&
          reduction.cut(
              level,
              closure,
              K1RankTwoCutEdgeSource::gabriel_diagnostic_graph) == reference;
      if (closure == K1CutClosure::strict) {
        certificate.strict_cuts_match =
            certificate.strict_cuts_match && matches;
      } else {
        certificate.closed_cuts_match =
            certificate.closed_cuts_match && matches;
      }
    }
  }

  certificate.multifusions_match =
      emst_fusion_signatures(emst) ==
      rank_two_fusion_signatures(reduction);
  certificate.selected_tree_edges_match =
      emst.emst_edges == reduction.selected_witness_edges;
  certificate.selected_tree_squared_weight_matches =
      emst.total_squared_weight == reduction.total_selected_squared_weight;
  certificate.selected_tree_hgp_weight_matches =
      emst.total_hgp_weight == reduction.total_selected_hgp_weight;
  certificate.selected_tree_hierarchy_matches =
      emst.point_count == reduction.point_count &&
      emst.nodes == reduction.nodes &&
      emst.root_node_id == reduction.root_node_id;
  certificate.all_selected_witness_edges_are_rank_two =
      std::all_of(
          reduction.selected_witness_edges.begin(),
          reduction.selected_witness_edges.end(),
          [&reduction](const ExactEmstEdge& edge) {
            return edge_is_in_sorted(reduction.rank_two_edges, edge);
          });
  certificate.anchor_equivalence_certified =
      certificate.exact_pair_decisions_complete &&
      certificate.pair_universe_matches_emst &&
      certificate.strict_cuts_match &&
      certificate.closed_cuts_match &&
      certificate.multifusions_match &&
      certificate.selected_tree_squared_weight_matches &&
      certificate.selected_tree_hgp_weight_matches &&
      certificate.selected_tree_hierarchy_matches &&
      certificate.all_selected_witness_edges_are_rank_two;
  return certificate;
}

}  // namespace

std::string_view to_string(K1PairDecisionStatus status) {
  switch (status) {
    case K1PairDecisionStatus::exact:
      return "exact";
  }
  throw std::invalid_argument("the pair decision status is invalid");
}

std::string_view to_string(K1PairCatalogStatus status) {
  switch (status) {
    case K1PairCatalogStatus::supported:
      return "supported";
    case K1PairCatalogStatus::unsupported_degeneracy:
      return "unsupported_degeneracy";
  }
  throw std::invalid_argument("the pair catalogue status is invalid");
}

std::string_view to_string(K1PairSphereClassification classification) {
  switch (classification) {
    case K1PairSphereClassification::rank_two_critical:
      return "rank_two_critical";
    case K1PairSphereClassification::extra_shell_degeneracy:
      return "extra_shell_degeneracy";
    case K1PairSphereClassification::interior_blocked:
      return "interior_blocked";
  }
  throw std::invalid_argument("the pair-sphere classification is invalid");
}

ExactEmstEdge K1PairSphereRecord::edge() const {
  return ExactEmstEdge{u, v, squared_length, level};
}

K1PairSphereCatalog build_exact_k1_pair_sphere_catalog(
    const spatial::CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "the exact pair-sphere catalogue requires a nonempty canonical cloud");
  }
  const std::size_t pair_count = checked_pair_count(point_count);
  K1PairSphereCatalog catalog;
  catalog.point_count = point_count;
  if (pair_count > catalog.pairs.max_size()) {
    throw std::length_error("the exact pair-sphere catalogue exceeds vector capacity");
  }
  catalog.pairs.reserve(pair_count);
  catalog.all_pair_edges.reserve(pair_count);
  catalog.rank_two_pair_indices.reserve(pair_count);
  catalog.gabriel_diagnostic_pair_indices.reserve(pair_count);
  catalog.rank_two_edges.reserve(pair_count);
  catalog.gabriel_diagnostic_edges.reserve(pair_count);

  exact::PredicateCounters predicate_counters;
  std::size_t exact_distance_evaluations = 0U;
  for (std::size_t left_index = 0U;
       left_index < point_count;
       ++left_index) {
    const spatial::PointId u = checked_point_id(left_index);
    for (std::size_t right_index = left_index + 1U;
         right_index < point_count;
         ++right_index) {
      const spatial::PointId v = checked_point_id(right_index);
      const std::array<exact::CertifiedPoint3, 2> support{
          cloud.point(u), cloud.point(v)};
      const exact::CircumcenterSupportAnalysis analysis =
          exact::analyze_circumcenter_support(
              support,
              &predicate_counters,
              exact::PredicateFilterPolicy::multiprecision_only);
      if (analysis.status() != exact::CircumcenterSupportStatus::minimal ||
          !analysis.reduced_support_size().has_value() ||
          *analysis.reduced_support_size() != 2U ||
          !analysis.reduced_support_contains(0U) ||
          !analysis.reduced_support_contains(1U) ||
          !analysis.barycentric().has_value() ||
          analysis.barycentric()->location() !=
              exact::ConvexHullLocation::relative_interior ||
          analysis.barycentric()->sign(0U) != exact::PredicateSign::positive ||
          analysis.barycentric()->sign(1U) != exact::PredicateSign::positive) {
        throw std::logic_error(
            "a distinct canonical pair did not yield a minimal two-point support");
      }
      const exact::CircumcenterResult& sphere =
          analysis.circumcenter_result();
      if (sphere.kind() != exact::CircumcenterKind::unique ||
          sphere.support_size() != 2U || sphere.affine_dimension() != 1U ||
          !sphere.center().has_value() || !sphere.squared_level().has_value()) {
        throw std::logic_error(
            "a distinct canonical pair omitted its exact diametric sphere");
      }
      exact::ExactLevel squared_length{
          exact::squared_distance(cloud.point(u), cloud.point(v))};
      if (squared_length == exact::ExactLevel{} ||
          scaled_level(*sphere.squared_level(), exact::BigInt{4}) !=
              squared_length) {
        throw std::logic_error(
            "an exact pair circumradius is not one quarter of its squared length");
      }

      const spatial::ClosedBallPartition partition =
          spatial::brute_force_closed_ball(
              cloud, *sphere.center(), *sphere.squared_level());
      if (!partition.partition_complete() || !partition.validated_for(cloud) ||
          partition.squared_radius() != *sphere.squared_level() ||
          partition.evaluation_count() != point_count ||
          partition.query_counters().method !=
              spatial::SpatialQueryMethod::brute_force ||
          partition.distance_evaluation_count() != point_count) {
        throw std::logic_error(
            "an exact pair closed-ball query returned an incomplete partition");
      }
      exact_distance_evaluations = checked_add(
          exact_distance_evaluations,
          partition.distance_evaluation_count(),
          "the pair closed-ball distance count overflows size_t");

      K1PairSphereRecord pair;
      pair.pair_index = catalog.pairs.size();
      pair.u = u;
      pair.v = v;
      pair.center = *sphere.center();
      pair.squared_length = squared_length;
      pair.level = *sphere.squared_level();
      pair.interior_ids.assign(
          partition.interior_ids().begin(), partition.interior_ids().end());
      pair.shell_ids.assign(
          partition.shell_ids().begin(), partition.shell_ids().end());
      pair.exterior_count = partition.exterior_ids().size();
      pair.closed_rank = partition.closed_rank();
      pair.classification = expected_classification(pair);
      require_pair_record(
          pair, pair.pair_index, u, v, point_count);

      const ExactEmstEdge edge = pair.edge();
      catalog.all_pair_edges.push_back(edge);
      switch (pair.classification) {
        case K1PairSphereClassification::rank_two_critical:
          catalog.rank_two_pair_indices.push_back(pair.pair_index);
          catalog.gabriel_diagnostic_pair_indices.push_back(pair.pair_index);
          catalog.rank_two_edges.push_back(edge);
          catalog.gabriel_diagnostic_edges.push_back(edge);
          ++catalog.counters.rank_two_critical_count;
          ++catalog.counters.gabriel_diagnostic_count;
          break;
        case K1PairSphereClassification::extra_shell_degeneracy:
          catalog.gabriel_diagnostic_pair_indices.push_back(pair.pair_index);
          catalog.gabriel_diagnostic_edges.push_back(edge);
          ++catalog.counters.extra_shell_degeneracy_count;
          ++catalog.counters.gabriel_diagnostic_count;
          break;
        case K1PairSphereClassification::interior_blocked:
          ++catalog.counters.interior_blocked_count;
          break;
      }
      catalog.pairs.push_back(std::move(pair));
    }
  }
  std::sort(catalog.all_pair_edges.begin(), catalog.all_pair_edges.end(), edge_less);
  std::sort(catalog.rank_two_edges.begin(), catalog.rank_two_edges.end(), edge_less);
  std::sort(
      catalog.gabriel_diagnostic_edges.begin(),
      catalog.gabriel_diagnostic_edges.end(),
      edge_less);

  if (!std::in_range<std::size_t>(predicate_counters.certified_decisions())) {
    throw std::length_error("the pair support predicate count exceeds size_t");
  }
  catalog.counters.point_count = point_count;
  catalog.counters.pair_count = pair_count;
  catalog.counters.support_analysis_count = pair_count;
  catalog.counters.support_predicate_decision_count =
      static_cast<std::size_t>(predicate_counters.certified_decisions());
  catalog.counters.closed_ball_query_count = pair_count;
  catalog.counters.exact_point_distance_evaluation_count =
      exact_distance_evaluations;
  if (predicate_counters.fp32_proposals() != 0U ||
      predicate_counters.fp64_filtered_certified() != 0U ||
      predicate_counters.expansion_certified() != 0U ||
      predicate_counters.remaining_unknown() != 0U ||
      predicate_counters.exact_zeros() != 0U) {
    throw std::logic_error(
        "the exact pair support analysis used an unexpected predicate authority");
  }
  catalog.catalog_status =
      catalog.counters.extra_shell_degeneracy_count == 0U
          ? K1PairCatalogStatus::supported
          : K1PairCatalogStatus::unsupported_degeneracy;
  require_catalog(catalog);
  return catalog;
}

K1Cut K1RankTwoReductionResult::cut(
    const exact::ExactLevel& level,
    K1CutClosure closure,
    K1RankTwoCutEdgeSource edge_source) const {
  const std::vector<ExactEmstEdge>* edges = nullptr;
  switch (edge_source) {
    case K1RankTwoCutEdgeSource::rank_two_graph:
      edges = &rank_two_edges;
      break;
    case K1RankTwoCutEdgeSource::selected_witness_tree:
      edges = &selected_witness_edges;
      break;
    case K1RankTwoCutEdgeSource::gabriel_diagnostic_graph:
      edges = &gabriel_diagnostic_edges;
      break;
    default:
      throw std::invalid_argument("the rank-two cut edge source is invalid");
  }
  return replay_cut(point_count, *edges, level, closure);
}

K1RankTwoReductionResult build_exact_k1_rank_two_reduction(
    const K1PairSphereCatalog& catalog) {
  require_catalog(catalog);
  K1RankTwoReductionResult result;
  result.point_count = catalog.point_count;
  result.rank_two_edges = catalog.rank_two_edges;
  result.gabriel_diagnostic_edges = catalog.gabriel_diagnostic_edges;
  result.selected_witness_edges.reserve(result.point_count - 1U);
  if (result.point_count > result.nodes.max_size()) {
    throw std::length_error("the rank-two hierarchy leaves exceed vector capacity");
  }
  result.nodes.reserve(result.point_count);
  for (std::size_t index = 0U; index < result.point_count; ++index) {
    result.nodes.push_back(leaf_node(checked_point_id(index)));
  }

  DisjointSet components{result.point_count};
  std::vector<K1NodeId> component_node_ids(result.point_count);
  for (std::size_t index = 0U; index < result.point_count; ++index) {
    component_node_ids[index] = checked_node_id(index);
  }
  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;

  std::size_t batch_begin = 0U;
  while (batch_begin < result.rank_two_edges.size()) {
    std::size_t batch_end = batch_begin + 1U;
    while (batch_end < result.rank_two_edges.size() &&
           result.rank_two_edges[batch_end].merge_level ==
               result.rank_two_edges[batch_begin].merge_level) {
      ++batch_end;
    }
    K1RankTwoEqualLevelBatch batch;
    batch.squared_length = result.rank_two_edges[batch_begin].squared_length;
    batch.level = result.rank_two_edges[batch_begin].merge_level;
    batch.rank_two_edges.assign(
        result.rank_two_edges.begin() +
            static_cast<std::vector<ExactEmstEdge>::difference_type>(batch_begin),
        result.rank_two_edges.begin() +
            static_cast<std::vector<ExactEmstEdge>::difference_type>(batch_end));
    batch.pre_batch_component_count = components.component_count();

    std::vector<std::pair<std::size_t, std::size_t>> quotient_edges;
    quotient_edges.reserve(batch.rank_two_edges.size());
    std::vector<std::size_t> quotient_roots;
    if (batch.rank_two_edges.size() > quotient_roots.max_size() / 2U) {
      throw std::length_error("a rank-two equality batch exceeds root capacity");
    }
    quotient_roots.reserve(batch.rank_two_edges.size() * 2U);
    for (const ExactEmstEdge& edge : batch.rank_two_edges) {
      require_edge(edge, result.point_count);
      if (edge.squared_length != batch.squared_length ||
          edge.merge_level != batch.level) {
        throw std::logic_error(
            "a rank-two equality batch contains unequal exact levels");
      }
      const std::size_t u_root =
          components.find(static_cast<std::size_t>(edge.u));
      const std::size_t v_root =
          components.find(static_cast<std::size_t>(edge.v));
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
          right_position == quotient_roots.end() ||
          *right_position != right_root) {
        throw std::logic_error(
            "a rank-two quotient edge references an absent pre-batch root");
      }
      static_cast<void>(batch_components.unite(
          static_cast<std::size_t>(left_position - quotient_roots.begin()),
          static_cast<std::size_t>(right_position - quotient_roots.begin())));
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
      K1HierarchyNode node = merge_node(
          batch.level, pre_batch_roots, component_node_ids, result.nodes);
      const K1NodeId node_id = node.id;
      batch.multifusions.push_back(multifusion_from_node(node, result.nodes));
      result.nodes.push_back(std::move(node));
      batch.merge_node_ids.push_back(node_id);
      pending_merges.push_back(PendingMerge{
          std::move(pre_batch_roots), node_id});
    }

    for (const ExactEmstEdge& edge : batch.rank_two_edges) {
      if (!components.unite(
              static_cast<std::size_t>(edge.u),
              static_cast<std::size_t>(edge.v))) {
        continue;
      }
      total_squared_weight =
          total_squared_weight + edge.squared_length.rational();
      total_hgp_weight =
          total_hgp_weight + edge.merge_level.rational();
      result.selected_witness_edges.push_back(edge);
      batch.selected_witness_edges.push_back(edge);
    }
    for (const PendingMerge& pending : pending_merges) {
      const std::size_t final_root =
          components.find(pending.pre_batch_roots.front());
      for (const std::size_t pre_batch_root : pending.pre_batch_roots) {
        if (components.find(pre_batch_root) != final_root) {
          throw std::logic_error(
              "a rank-two equality-batch multifusion was not contracted");
        }
      }
      component_node_ids[final_root] = pending.node_id;
    }
    batch.post_batch_component_count = components.component_count();
    if (batch.pre_batch_component_count < batch.post_batch_component_count ||
        batch.pre_batch_component_count - batch.post_batch_component_count !=
            batch.selected_witness_edges.size()) {
      throw std::logic_error(
          "a rank-two Kruskal batch has inconsistent component counts");
    }
    result.equal_level_batches.push_back(std::move(batch));
    batch_begin = batch_end;
  }

  if (components.component_count() != 1U ||
      result.selected_witness_edges.size() != result.point_count - 1U) {
    throw std::logic_error(
        "the exact rank-two graph did not yield a spanning witness tree");
  }
  if (!std::all_of(
          result.selected_witness_edges.begin(),
          result.selected_witness_edges.end(),
          [&result](const ExactEmstEdge& edge) {
            return edge_is_in_sorted(result.rank_two_edges, edge);
          })) {
    throw std::logic_error("a selected witness edge is not rank-two critical");
  }
  result.total_selected_squared_weight =
      exact::ExactLevel{std::move(total_squared_weight)};
  result.total_selected_hgp_weight =
      exact::ExactLevel{std::move(total_hgp_weight)};
  result.root_node_id = component_node_ids[components.find(0U)];

  result.counters.point_count = result.point_count;
  result.counters.rank_two_edge_count = result.rank_two_edges.size();
  result.counters.gabriel_diagnostic_edge_count =
      result.gabriel_diagnostic_edges.size();
  result.counters.distinct_rank_two_level_count =
      result.equal_level_batches.size();
  result.counters.selected_witness_edge_count =
      result.selected_witness_edges.size();
  result.counters.redundant_rank_two_edge_count =
      result.rank_two_edges.size() - result.selected_witness_edges.size();
  result.counters.replay_level_count =
      checked_add(
          result.equal_level_batches.size(),
          1U,
          "the rank-two replay level count overflows size_t");
  for (const K1RankTwoEqualLevelBatch& batch : result.equal_level_batches) {
    if (batch.rank_two_edges.size() > 1U) {
      ++result.counters.equal_level_batch_count;
    }
    result.counters.max_equal_level_batch_size = std::max(
        result.counters.max_equal_level_batch_size,
        batch.rank_two_edges.size());
    if (!batch.multifusions.empty()) {
      ++result.counters.merge_batch_count;
    }
    result.counters.merge_event_count = checked_add(
        result.counters.merge_event_count,
        batch.multifusions.size(),
        "the rank-two merge event count overflows size_t");
    for (const K1Multifusion& fusion : batch.multifusions) {
      result.counters.max_merge_arity = std::max(
          result.counters.max_merge_arity, fusion.arity());
      if (fusion.arity() >= 3U) {
        ++result.counters.multifusion_count;
      }
    }
  }
  return result;
}

K1ExactAnchorResult build_exact_k1_anchor(
    const spatial::CanonicalPointCloud& cloud) {
  K1EmstResult emst = build_exact_complete_graph_emst(cloud);
  K1PairSphereCatalog catalog =
      build_exact_k1_pair_sphere_catalog(cloud);
  K1RankTwoReductionResult reduction =
      build_exact_k1_rank_two_reduction(catalog);
  K1ExactAnchorCertificate certificate =
      certify_anchor(emst, catalog, reduction);
  return K1ExactAnchorResult{
      std::move(emst),
      std::move(catalog),
      std::move(reduction),
      std::move(certificate)};
}

}  // namespace morsehgp3d::hierarchy
