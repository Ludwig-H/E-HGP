#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::K1CompactEqualLevelBatch;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1CompactMergeNode;
using morsehgp3d::hierarchy::K1Cut;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1CutEdgeSource;
using morsehgp3d::hierarchy::K1EmstResult;
using morsehgp3d::hierarchy::K1HierarchyNode;
using morsehgp3d::hierarchy::K1Multifusion;
using morsehgp3d::hierarchy::K1NodeId;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactLevel quarter(const ExactLevel& value) {
  return ExactLevel{
      value.numerator(), value.denominator() * BigInt{4}};
}

[[nodiscard]] ExactEmstEdge edge(
    PointId u, PointId v, std::int64_t squared_length) {
  const ExactLevel length = level(squared_length);
  return ExactEmstEdge{u, v, length, quarter(length)};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::vector<K1Multifusion> reference_multifusions(
    const K1EmstResult& reference) {
  std::vector<K1Multifusion> result;
  for (const auto& batch : reference.equal_level_batches) {
    result.insert(
        result.end(), batch.multifusions.begin(), batch.multifusions.end());
  }
  return result;
}

[[nodiscard]] bool same_compact_storage(
    const K1CompactForest& left, const K1CompactForest& right) {
  return left.point_count == right.point_count && left.levels == right.levels &&
         left.selected_edges == right.selected_edges &&
         left.merge_nodes == right.merge_nodes &&
         left.child_ids == right.child_ids &&
         left.equal_level_batches == right.equal_level_batches &&
         left.root_node_id == right.root_node_id &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters;
}

void check_compact_internal_invariants(
    const K1CompactForest& forest, const std::string& fixture) {
  const std::size_t point_count = forest.point_count;
  const std::size_t edge_count = point_count - 1U;
  check(point_count > 0U, fixture + " is nonempty");
  check(
      forest.selected_edges.size() == edge_count,
      fixture + " stores exactly n-1 selected edges");
  check(
      forest.node_count() == point_count + forest.merge_nodes.size(),
      fixture + " has implicit leaves and only explicit merge nodes");
  check(
      forest.root_node_id < forest.node_count(),
      fixture + " root node is in range");
  check(
      forest.total_hgp_weight == quarter(forest.total_squared_weight),
      fixture + " total HGP weight is exactly one quarter of tree weight");

  check(
      std::is_sorted(forest.levels.begin(), forest.levels.end()) &&
          std::adjacent_find(forest.levels.begin(), forest.levels.end()) ==
              forest.levels.end(),
      fixture + " factorizes strictly increasing exact levels");

  for (std::size_t point_index = 0U; point_index < point_count;
       ++point_index) {
    const K1NodeId leaf_id = static_cast<K1NodeId>(point_index);
    check(forest.is_leaf(leaf_id), fixture + " keeps leaves implicit");
    check(
        forest.node_level(leaf_id) == level(0),
        fixture + " gives every implicit leaf level zero");
    check(
        forest.children(leaf_id).empty(),
        fixture + " gives every implicit leaf no children");
    check(
        forest.materialize_node(leaf_id) ==
            K1HierarchyNode{
                leaf_id,
                level(0),
                {},
                {static_cast<PointId>(point_index)}},
        fixture + " materializes each implicit singleton exactly");
  }

  std::size_t child_offset = 0U;
  std::size_t computed_multifusion_count = 0U;
  std::size_t computed_max_arity = 0U;
  for (std::size_t merge_index = 0U;
       merge_index < forest.merge_nodes.size(); ++merge_index) {
    const K1CompactMergeNode& merge = forest.merge_nodes[merge_index];
    const K1NodeId expected_id =
        static_cast<K1NodeId>(point_count + merge_index);
    check(
        merge.id == expected_id && !forest.is_leaf(merge.id),
        fixture + " assigns contiguous internal node identifiers");
    check(
        merge.level_index < forest.levels.size(),
        fixture + " merge level index is in range");
    check(
        merge.child_offset == child_offset && merge.child_count >= 2U,
        fixture + " merge children form contiguous nontrivial CSR slices");
    check(
        forest.node_level(merge.id) == forest.levels[merge.level_index],
        fixture + " resolves each internal node level through the level arena");

    const std::span<const K1NodeId> children = forest.children(merge.id);
    check(
        children.size() == merge.child_count,
        fixture + " resolves the complete CSR child slice");
    for (const K1NodeId child_id : children) {
      check(
          child_id < merge.id &&
              forest.node_level(child_id) < forest.node_level(merge.id),
          fixture + " keeps every child in the strict pre-batch hierarchy");
    }

    const K1HierarchyNode materialized = forest.materialize_node(merge.id);
    check(
        materialized.id == merge.id &&
            materialized.level == forest.levels[merge.level_index] &&
            materialized.children ==
                std::vector<K1NodeId>{children.begin(), children.end()} &&
            materialized.point_ids.size() == merge.coverage_size &&
            materialized.point_ids.front() == merge.minimum_point_id,
        fixture + " reconstructs metadata and coverage only on demand");
    check(
        forest.materialize_multifusion(merge.id).node_id == merge.id,
        fixture + " materializes the multifusion of every internal node");

    child_offset += merge.child_count;
    computed_max_arity = std::max(computed_max_arity, merge.child_count);
    if (merge.child_count >= 3U) {
      ++computed_multifusion_count;
    }
  }
  check(
      child_offset == forest.child_ids.size(),
      fixture + " merge nodes partition the CSR child arena");

  std::size_t selected_edge_offset = 0U;
  std::size_t merge_node_offset = 0U;
  std::size_t previous_post_batch_count = point_count;
  for (std::size_t batch_index = 0U;
       batch_index < forest.equal_level_batches.size(); ++batch_index) {
    const K1CompactEqualLevelBatch& batch =
        forest.equal_level_batches[batch_index];
    check(
        batch.level_index == batch_index &&
            batch.level_index < forest.levels.size(),
        fixture + " gives each equality batch one canonical level slot");
    check(
        batch.selected_edge_offset == selected_edge_offset &&
            batch.merge_node_offset == merge_node_offset,
        fixture + " equality batches partition both compact arenas");
    check(
        batch.pre_batch_component_count == previous_post_batch_count &&
            batch.post_batch_component_count + batch.selected_edge_count ==
                batch.pre_batch_component_count,
        fixture + " batch loss equals its selected tree-edge count");

    for (std::size_t edge_index = 0U;
         edge_index < batch.selected_edge_count; ++edge_index) {
      const ExactEmstEdge& selected =
          forest.selected_edges[batch.selected_edge_offset + edge_index];
      check(
          selected.u < selected.v &&
              selected.merge_level == forest.levels[batch.level_index] &&
              selected.merge_level == quarter(selected.squared_length),
          fixture + " stores canonical edges with the exact quarter level");
    }
    for (std::size_t local_merge_index = 0U;
         local_merge_index < batch.merge_node_count; ++local_merge_index) {
      check(
          forest.merge_nodes[batch.merge_node_offset + local_merge_index]
                  .level_index == batch.level_index,
          fixture + " assigns every batch merge to the batch level");
    }

    selected_edge_offset += batch.selected_edge_count;
    merge_node_offset += batch.merge_node_count;
    previous_post_batch_count = batch.post_batch_component_count;
  }
  check(
      selected_edge_offset == forest.selected_edges.size() &&
          merge_node_offset == forest.merge_nodes.size(),
      fixture + " batches consume both compact arenas exactly once");
  check(
      previous_post_batch_count == 1U,
      fixture + " final closed batch has one component");

  const std::size_t expected_storage_entries =
      forest.levels.size() + forest.selected_edges.size() +
      forest.merge_nodes.size() + forest.child_ids.size() +
      forest.equal_level_batches.size();
  const auto& counters = forest.counters;
  check(
      counters.point_count == point_count &&
          counters.input_edge_count == edge_count &&
          counters.selected_edge_count == forest.selected_edges.size() &&
          counters.batched_selected_edge_count == forest.selected_edges.size(),
      fixture + " closes point and selected-edge counters");
  check(
      counters.distinct_level_count == forest.levels.size() &&
          counters.equal_level_batch_count ==
              forest.equal_level_batches.size(),
      fixture + " closes level and batch counters");
  check(
      counters.merge_node_count == forest.merge_nodes.size() &&
          counters.batched_merge_node_count == forest.merge_nodes.size() &&
          counters.child_reference_count == forest.child_ids.size() &&
          counters.total_node_count == forest.node_count() &&
          counters.merge_event_count == forest.merge_nodes.size(),
      fixture + " closes node and CSR counters");
  check(
      counters.multifusion_count == computed_multifusion_count &&
          counters.max_merge_arity == computed_max_arity &&
          counters.root_coverage_size == point_count,
      fixture + " closes arity and root-coverage counters");
  check(
      counters.stored_coverage_point_id_count == 0U,
      fixture + " stores no materialized coverage point identifiers");
  check(
      counters.linear_storage_entry_count == expected_storage_entries &&
          counters.linear_storage_entry_count <=
              counters.linear_storage_entry_limit &&
          counters.linear_storage_entry_limit == 6U * edge_count,
      fixture + " certifies the six-times-(n-1) linear storage bound");
}

void check_against_complete_graph_reference(
    const K1CompactForest& forest,
    const K1EmstResult& reference,
    const std::string& fixture,
    bool require_same_selected_edges = true) {
  check_compact_internal_invariants(forest, fixture);
  check(
      forest.point_count == reference.point_count &&
          forest.root_node_id == reference.root_node_id,
      fixture + " agrees with the complete-graph reference root");
  check(
      forest.total_squared_weight == reference.total_squared_weight &&
          forest.total_hgp_weight == reference.total_hgp_weight,
      fixture + " agrees with both exact reference weights");
  if (require_same_selected_edges) {
    check(
        forest.selected_edges == reference.emst_edges,
        fixture + " preserves the deterministic reference witness tree");
  }
  check(
      forest.materialize_nodes() == reference.nodes,
      fixture + " materializes exactly the complete-graph hierarchy nodes");
  check(
      forest.materialize_multifusions() == reference_multifusions(reference),
      fixture + " materializes exactly the complete-graph multifusions");

  check(
      forest.cut_strict(level(0)).empty() &&
          forest.cut_closed(level(0)) ==
              reference.cut(
                  level(0),
                  K1CutClosure::closed,
                  K1CutEdgeSource::selected_emst),
      fixture + " preserves the strict and closed singleton births");
  for (const auto& batch : reference.equal_level_batches) {
    check(
        forest.cut_strict(batch.level) ==
            reference.cut(
                batch.level,
                K1CutClosure::strict,
                K1CutEdgeSource::selected_emst) &&
            forest.cut(batch.level, K1CutClosure::strict) ==
                forest.cut_strict(batch.level),
        fixture + " agrees with the reference strict cut at every level");
    check(
        forest.cut_closed(batch.level) ==
            reference.cut(
                batch.level,
                K1CutClosure::closed,
                K1CutEdgeSource::selected_emst) &&
            forest.cut(batch.level, K1CutClosure::closed) ==
                forest.cut_closed(batch.level),
        fixture + " agrees with the reference closed cut at every level");
  }
}

void test_singleton_implicit_leaf() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(input));
  const K1CompactForest forest = build_compact_k1_forest(reference);

  check_against_complete_graph_reference(forest, reference, "singleton");
  check(
      forest.levels.empty() && forest.selected_edges.empty() &&
          forest.merge_nodes.empty() && forest.child_ids.empty() &&
          forest.equal_level_batches.empty() &&
          forest.root_node_id == K1NodeId{0},
      "singleton compact forest stores only its scalar root identity");
  check(
      forest.materialize_multifusions().empty(),
      "singleton has no materialized multifusion");
}

void test_increasing_chain_has_linear_persistent_storage() {
  constexpr std::size_t point_count = 32U;
  std::vector<CertifiedPoint3> input;
  input.reserve(point_count);
  double x = 0.0;
  input.push_back(point(x, 0.0, 0.0));
  for (std::size_t index = 1U; index < point_count; ++index) {
    x += static_cast<double>(2U * index);
    input.push_back(point(x, 0.0, 0.0));
  }

  const K1EmstResult reference = build_exact_complete_graph_emst(
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{input}));
  const K1CompactForest forest = build_compact_k1_forest(reference);
  check_against_complete_graph_reference(
      forest, reference, "strictly increasing chain");

  check(
      forest.levels.size() == point_count - 1U &&
          forest.equal_level_batches.size() == point_count - 1U &&
          forest.merge_nodes.size() == point_count - 1U &&
          forest.child_ids.size() == 2U * (point_count - 1U),
      "increasing chain uses one binary merge per exact selected level");
  check(
      forest.counters.max_merge_arity == 2U &&
          forest.counters.multifusion_count == 0U,
      "increasing chain remains a sequence of strict binary events");

  const std::size_t materialized_reference_coverage_entries =
      std::accumulate(
          reference.nodes.begin(),
          reference.nodes.end(),
          std::size_t{0},
          [](std::size_t total, const K1HierarchyNode& node) {
            return total + node.point_ids.size();
          });
  check(
      materialized_reference_coverage_entries >
              point_count * point_count / 2U &&
          forest.counters.stored_coverage_point_id_count == 0U &&
          forest.counters.linear_storage_entry_count <=
              6U * (point_count - 1U),
      "chain replaces quadratic persistent coverages by certified O(n) storage");
}

void test_two_disjoint_fusions_share_one_batch() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(8.0, 0.0, 0.0),
      point(10.0, 0.0, 0.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(input));
  const K1CompactForest forest = build_compact_k1_forest(reference);
  check_against_complete_graph_reference(
      forest, reference, "two disjoint equal-level fusions");

  check(
      forest.levels == std::vector<ExactLevel>{level(1), level(9)} &&
          forest.equal_level_batches.size() == 2U,
      "disjoint fixture factorizes only the two selected tree levels");
  if (!forest.equal_level_batches.empty()) {
    const K1CompactEqualLevelBatch& first = forest.equal_level_batches.front();
    check(
        first.selected_edge_count == 2U && first.merge_node_count == 2U &&
            first.pre_batch_component_count == 4U &&
            first.post_batch_component_count == 2U,
        "one frozen batch keeps two disjoint binary fusions distinct");
  }
}

void test_six_way_multifusion_is_not_binarized() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(6.0, 0.0, 0.0),
      point(8.0, 0.0, 0.0),
      point(10.0, 0.0, 0.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(input));
  const K1CompactForest forest = build_compact_k1_forest(reference);
  check_against_complete_graph_reference(
      forest, reference, "six-way compact multifusion");

  check(
      forest.levels == std::vector<ExactLevel>{level(1)} &&
          forest.merge_nodes.size() == 1U &&
          forest.merge_nodes.front().child_count == 6U &&
          forest.child_ids ==
              std::vector<K1NodeId>{0U, 1U, 2U, 3U, 4U, 5U} &&
          forest.counters.multifusion_count == 1U &&
          forest.counters.max_merge_arity == 6U,
      "five equal tree edges produce one compact arity-six node");
}

void test_square_input_and_tree_permutations() {
  const std::array<CertifiedPoint3, 4> source{
      point(-1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(1.0, 1.0, 0.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(source));
  const K1CompactForest expected = build_compact_k1_forest(reference);
  check_against_complete_graph_reference(
      expected, reference, "square permutation reference");

  std::array<std::size_t, 4> point_permutation{0U, 1U, 2U, 3U};
  std::size_t point_permutation_count = 0U;
  do {
    const std::array<CertifiedPoint3, 4> permuted{
        source[point_permutation[0]],
        source[point_permutation[1]],
        source[point_permutation[2]],
        source[point_permutation[3]]};
    const K1CompactForest actual = build_compact_k1_forest(
        build_exact_complete_graph_emst(canonical_cloud(permuted)));
    check(
        same_compact_storage(actual, expected),
        "square compact storage is invariant under point permutation " +
            std::to_string(point_permutation_count));
    ++point_permutation_count;
  } while (std::next_permutation(
      point_permutation.begin(), point_permutation.end()));
  check(
      point_permutation_count == 24U,
      "all square point permutations were checked");

  std::vector<ExactEmstEdge> witness = reference.emst_edges;
  std::array<std::size_t, 3> edge_permutation{0U, 1U, 2U};
  std::size_t edge_permutation_count = 0U;
  do {
    std::vector<ExactEmstEdge> permuted;
    permuted.reserve(witness.size());
    for (const std::size_t source_index : edge_permutation) {
      ExactEmstEdge selected = witness[source_index];
      if (((edge_permutation_count + source_index) & 1U) != 0U) {
        std::swap(selected.u, selected.v);
      }
      permuted.push_back(std::move(selected));
    }
    const K1CompactForest actual = build_compact_k1_forest(
        4U, std::span<const ExactEmstEdge>{permuted});
    check(
        same_compact_storage(actual, expected),
        "square compact storage ignores tree order and orientation " +
            std::to_string(edge_permutation_count));
    ++edge_permutation_count;
  } while (std::next_permutation(
      edge_permutation.begin(), edge_permutation.end()));
  check(
      edge_permutation_count == 6U,
      "all selected square-tree edge permutations were checked");
}

void test_distinct_tied_emst_witnesses_have_one_semantic_forest() {
  const std::array<CertifiedPoint3, 5> input{
      point(-1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(5.0, 0.0, 0.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(input));
  const std::vector<ExactEmstEdge> first_witness{
      edge(PointId{0}, PointId{1}, 4),
      edge(PointId{0}, PointId{2}, 4),
      edge(PointId{1}, PointId{3}, 4),
      edge(PointId{2}, PointId{4}, 17)};
  const std::vector<ExactEmstEdge> second_witness{
      edge(PointId{0}, PointId{1}, 4),
      edge(PointId{0}, PointId{2}, 4),
      edge(PointId{2}, PointId{3}, 4),
      edge(PointId{2}, PointId{4}, 17)};
  const K1CompactForest first = build_compact_k1_forest(
      5U, std::span<const ExactEmstEdge>{first_witness});
  const K1CompactForest second = build_compact_k1_forest(
      5U, std::span<const ExactEmstEdge>{second_witness});

  check_against_complete_graph_reference(
      first, reference, "first tied square witness", false);
  check_against_complete_graph_reference(
      second, reference, "second tied square witness", false);
  check(
      first.selected_edges != second.selected_edges,
      "square-plus-tail fixture supplies two different EMST witnesses");
  check(
      first.materialize_nodes() == second.materialize_nodes() &&
          first.materialize_multifusions() ==
              second.materialize_multifusions(),
      "different tied witnesses materialize identical nodes and multifusions");
  check(
      first.cut_strict(level(1)) == second.cut_strict(level(1)) &&
          first.cut_closed(level(1)) == second.cut_closed(level(1)) &&
          first.cut_strict(level(17, 4)) ==
              second.cut_strict(level(17, 4)) &&
          first.cut_closed(level(17, 4)) ==
              second.cut_closed(level(17, 4)) &&
          first.total_squared_weight == second.total_squared_weight &&
          first.total_hgp_weight == second.total_hgp_weight,
      "different tied witnesses preserve a later fusion, cuts and exact weights");
}

void test_invalid_spanning_trees_fail_closed() {
  const std::vector<ExactEmstEdge> empty;
  check_throws<std::invalid_argument>(
      [&empty]() {
        static_cast<void>(build_compact_k1_forest(
            0U, std::span<const ExactEmstEdge>{empty}));
      },
      "zero-point compact forest is rejected");

  const std::vector<ExactEmstEdge> disconnected_count{
      edge(PointId{0}, PointId{1}, 4)};
  check_throws<std::invalid_argument>(
      [&disconnected_count]() {
        static_cast<void>(build_compact_k1_forest(
            3U, std::span<const ExactEmstEdge>{disconnected_count}));
      },
      "a disconnected n-2 edge forest is rejected");

  const std::vector<ExactEmstEdge> cyclic_and_disconnected{
      edge(PointId{0}, PointId{1}, 4),
      edge(PointId{1}, PointId{2}, 4),
      edge(PointId{0}, PointId{2}, 16)};
  check_throws<std::invalid_argument>(
      [&cyclic_and_disconnected]() {
        static_cast<void>(build_compact_k1_forest(
            4U, std::span<const ExactEmstEdge>{cyclic_and_disconnected}));
      },
      "an n-1 edge graph with a cycle and isolated vertex is rejected");

  const std::vector<ExactEmstEdge> duplicate{
      edge(PointId{0}, PointId{1}, 4),
      edge(PointId{1}, PointId{0}, 4)};
  check_throws<std::invalid_argument>(
      [&duplicate]() {
        static_cast<void>(build_compact_k1_forest(
            3U, std::span<const ExactEmstEdge>{duplicate}));
      },
      "a duplicate unoriented tree edge is rejected");

  const std::vector<ExactEmstEdge> endpoint_out_of_range{
      edge(PointId{0}, PointId{2}, 4)};
  check_throws<std::invalid_argument>(
      [&endpoint_out_of_range]() {
        static_cast<void>(build_compact_k1_forest(
            2U, std::span<const ExactEmstEdge>{endpoint_out_of_range}));
      },
      "a tree endpoint outside the point table is rejected");

  const std::vector<ExactEmstEdge> loop{
      edge(PointId{0}, PointId{0}, 4)};
  check_throws<std::invalid_argument>(
      [&loop]() {
        static_cast<void>(build_compact_k1_forest(
            2U, std::span<const ExactEmstEdge>{loop}));
      },
      "a tree self-loop is rejected");

  const std::vector<ExactEmstEdge> zero_length{
      ExactEmstEdge{PointId{0}, PointId{1}, level(0), level(0)}};
  check_throws<std::invalid_argument>(
      [&zero_length]() {
        static_cast<void>(build_compact_k1_forest(
            2U, std::span<const ExactEmstEdge>{zero_length}));
      },
      "a zero-length tree edge is rejected");

  const std::vector<ExactEmstEdge> inconsistent_level{
      ExactEmstEdge{PointId{0}, PointId{1}, level(4), level(2)}};
  check_throws<std::invalid_argument>(
      [&inconsistent_level]() {
        static_cast<void>(build_compact_k1_forest(
            2U, std::span<const ExactEmstEdge>{inconsistent_level}));
      },
      "a merge level other than squared length divided by four is rejected");
}

}  // namespace

int main() {
  test_singleton_implicit_leaf();
  test_increasing_chain_has_linear_persistent_storage();
  test_two_disjoint_fusions_share_one_batch();
  test_six_way_multifusion_is_not_binarized();
  test_square_input_and_tree_permutations();
  test_distinct_tied_emst_witnesses_have_one_semantic_forest();
  test_invalid_spanning_trees_fail_closed();

  if (failures != 0) {
    std::cerr << failures << " compact k=1 forest test(s) failed\n";
    return 1;
  }
  return 0;
}
