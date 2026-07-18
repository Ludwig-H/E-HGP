#include "morsehgp3d/hierarchy/emst.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::K1Cut;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1CutEdgeSource;
using morsehgp3d::hierarchy::K1EmstResult;
using morsehgp3d::hierarchy::K1EqualLevelBatch;
using morsehgp3d::hierarchy::K1HierarchyNode;
using morsehgp3d::hierarchy::K1NodeId;
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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactLevel level(std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] bool edge_less(
    const ExactEmstEdge& left, const ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] bool same_scientific_result(
    const K1EmstResult& left, const K1EmstResult& right) {
  return left.point_count == right.point_count &&
         left.complete_graph_edge_count == right.complete_graph_edge_count &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.complete_edges == right.complete_edges &&
         left.emst_edges == right.emst_edges && left.nodes == right.nodes &&
         left.equal_level_batches == right.equal_level_batches &&
         left.root_node_id == right.root_node_id &&
         left.counters == right.counters;
}

void check_result_invariants(
    const K1EmstResult& result, const std::string& fixture) {
  const std::size_t point_count = result.point_count;
  check(point_count > 0U, fixture + " is nonempty");
  check(
      result.complete_graph_edge_count ==
          point_count * (point_count - 1U) / 2U,
      fixture + " has the complete edge count");
  check(
      result.complete_edges.size() == result.complete_graph_edge_count,
      fixture + " materializes every complete-graph edge");
  check(
      result.emst_edges.size() == point_count - 1U,
      fixture + " selects n-1 EMST edges");
  check(
      std::is_sorted(
          result.complete_edges.begin(), result.complete_edges.end(), edge_less),
      fixture + " orders complete edges by exact weight and endpoints");
  check(
      std::is_sorted(
          result.emst_edges.begin(), result.emst_edges.end(), edge_less),
      fixture + " orders selected edges by exact weight and endpoints");

  std::size_t candidate_edge_count = 0U;
  std::size_t merge_node_count = 0U;
  std::size_t previous_post_count = point_count;
  for (std::size_t batch_index = 0U;
       batch_index < result.equal_level_batches.size();
       ++batch_index) {
    const K1EqualLevelBatch& batch = result.equal_level_batches[batch_index];
    candidate_edge_count += batch.candidate_edge_count;
    merge_node_count += batch.merge_node_ids.size();
    check(
        batch.complete_edges.size() == batch.candidate_edge_count,
        fixture + " batch materializes every candidate edge");
    check(
        batch.pre_batch_component_count == previous_post_count,
        fixture + " batch starts from the preceding closed state");
    check(
        batch.post_batch_component_count + batch.selected_edges.size() ==
            batch.pre_batch_component_count,
        fixture + " batch component loss equals selected edge count");
    check(
        batch.multifusions.size() == batch.merge_node_ids.size(),
        fixture + " batch exposes each semantic multifusion");
    check(
        batch.level == ExactLevel{
                           batch.squared_length.numerator(),
                           batch.squared_length.denominator() * BigInt{4}},
        fixture + " batch applies the exact one-quarter factor");
    if (batch_index > 0U) {
      check(
          result.equal_level_batches[batch_index - 1U].squared_length <
              batch.squared_length,
          fixture + " has strictly ordered equality batches");
    }
    for (const ExactEmstEdge& edge : batch.complete_edges) {
      check(
          edge.u < edge.v && edge.squared_length == batch.squared_length &&
              edge.merge_level == batch.level,
          fixture + " batch edge has canonical endpoints and exact levels");
    }
    for (const auto& multifusion : batch.multifusions) {
      check(
          multifusion.arity() >= 2U,
          fixture + " multifusion has at least two frozen children");
      std::vector<PointId> merged;
      for (const auto& child : multifusion.child_components) {
        merged.insert(merged.end(), child.begin(), child.end());
      }
      std::sort(merged.begin(), merged.end());
      check(
          merged == multifusion.merged_component,
          fixture + " multifusion is the disjoint union of its children");
      check(
          multifusion.node_id < result.nodes.size() &&
              result.nodes[static_cast<std::size_t>(multifusion.node_id)].level ==
                  batch.level,
          fixture + " multifusion points to its equal-level hierarchy node");
    }
    previous_post_count = batch.post_batch_component_count;
  }
  check(
      candidate_edge_count == result.complete_graph_edge_count,
      fixture + " batches partition the complete graph");
  check(
      result.nodes.size() == point_count + merge_node_count,
      fixture + " creates only leaves and semantic merge nodes");
  check(previous_post_count == 1U, fixture + " ends in one component");
  check(
      result.root_node_id < result.nodes.size(),
      fixture + " root identifier is in range");
  if (result.root_node_id < result.nodes.size()) {
    std::vector<PointId> expected_root(point_count);
    std::iota(expected_root.begin(), expected_root.end(), PointId{0});
    check(
        result.nodes[static_cast<std::size_t>(result.root_node_id)].point_ids ==
            expected_root,
        fixture + " root covers every canonical point exactly once");
  }
  for (const K1EqualLevelBatch& batch : result.equal_level_batches) {
    check(
        result.cut(
            batch.level,
            K1CutClosure::strict,
            K1CutEdgeSource::complete_graph) ==
            result.cut(
                batch.level,
                K1CutClosure::strict,
                K1CutEdgeSource::selected_emst),
        fixture + " strict complete-graph and EMST cuts agree");
    check(
        result.cut(
            batch.level,
            K1CutClosure::closed,
            K1CutEdgeSource::complete_graph) ==
            result.cut(
                batch.level,
                K1CutClosure::closed,
                K1CutEdgeSource::selected_emst),
        fixture + " closed complete-graph and EMST cuts agree");
  }
}

void test_singleton_birth_and_zero_cut() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const K1EmstResult result =
      build_exact_complete_graph_emst(canonical_cloud(input));

  check_result_invariants(result, "singleton");
  check(
      result.complete_edges.empty() && result.emst_edges.empty() &&
          result.equal_level_batches.empty(),
      "singleton has no edge or equality batch");
  check(
      result.nodes == std::vector<K1HierarchyNode>{
                          K1HierarchyNode{K1NodeId{0}, level(0), {}, {PointId{0}}}},
      "singleton injects one canonical rank-one minimum at level zero");
  check(
      result.total_squared_weight == level(0) &&
          result.total_hgp_weight == level(0),
      "singleton has zero exact weight");
  check(
      result.cut(level(0), K1CutClosure::strict).empty(),
      "strict zero cut precedes the singleton birth");
  check(
      result.cut(level(0), K1CutClosure::closed) == K1Cut{{PointId{0}}},
      "closed zero cut contains the singleton birth");
  check(
      result.cut(level(1), K1CutClosure::strict) == K1Cut{{PointId{0}}},
      "positive strict cut contains the singleton birth");
}

void test_exact_quarter_level() {
  const std::array<CertifiedPoint3, 2> input{
      point(0.0, 0.0, 0.0), point(1.0, 1.0, 1.0)};
  const K1EmstResult result =
      build_exact_complete_graph_emst(canonical_cloud(input));

  check_result_invariants(result, "quarter-factor pair");
  check(
      result.emst_edges == std::vector<ExactEmstEdge>{
                               ExactEmstEdge{
                                   PointId{0}, PointId{1}, level(3), level(3, 4)}},
      "squared length three maps exactly to HGP level three quarters");
  check(
      result.total_squared_weight == level(3) &&
          result.total_hgp_weight == level(3, 4),
      "pair exposes exact total geometric and HGP weights");
  check(
      result.equal_level_batches.size() == 1U &&
          result.equal_level_batches[0].pre_batch_component_count == 2U &&
          result.equal_level_batches[0].post_batch_component_count == 1U,
      "pair contracts its unique equality batch atomically");
  check(
      result.cut(level(3, 4), K1CutClosure::strict) ==
          K1Cut{{PointId{0}}, {PointId{1}}} &&
          result.cut(level(3, 4), K1CutClosure::closed) ==
              K1Cut{{PointId{0}, PointId{1}}},
      "pair distinguishes strict and closed cuts at the exact merge level");
}

void test_disjoint_merges_in_one_batch() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(5.0, 0.0, 0.0)};
  const K1EmstResult result =
      build_exact_complete_graph_emst(canonical_cloud(input));

  check_result_invariants(result, "disjoint equal merges");
  check(
      result.total_squared_weight == level(11) &&
          result.total_hgp_weight == level(11, 4),
      "disjoint fixture has exact EMST weight eleven");
  check(
      result.equal_level_batches.size() == 4U,
      "disjoint fixture retains redundant complete-graph levels");
  const K1EqualLevelBatch& first = result.equal_level_batches[0];
  check(
      first.squared_length == level(1) && first.candidate_edge_count == 2U &&
          first.selected_edges.size() == 2U &&
          first.multifusions.size() == 2U &&
          first.pre_batch_component_count == 4U &&
          first.post_batch_component_count == 2U,
      "one equality batch records two disjoint binary fusions");
  check(
      first.multifusions[0].child_components ==
              K1Cut{{PointId{0}}, {PointId{1}}} &&
          first.multifusions[1].child_components ==
              K1Cut{{PointId{2}}, {PointId{3}}},
      "disjoint same-level events remain distinct semantic fusions");
  check(
      result.equal_level_batches[2].selected_edges.empty() &&
          result.equal_level_batches[3].selected_edges.empty(),
      "post-connectivity complete-graph batches remain auditable and redundant");
}

void test_equal_level_multifusion_is_not_binarized() {
  const std::array<CertifiedPoint3, 6> input{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0),
      point(4.0, 0.0, 0.0),
      point(5.0, 0.0, 0.0)};
  const K1EmstResult result =
      build_exact_complete_graph_emst(canonical_cloud(input));

  check_result_invariants(result, "six-way multifusion");
  const K1EqualLevelBatch& first = result.equal_level_batches[0];
  check(
      first.squared_length == level(1) && first.candidate_edge_count == 5U &&
          first.selected_edges.size() == 5U && first.multifusions.size() == 1U &&
          first.multifusions[0].arity() == 6U,
      "adjacent equal edges produce one six-way frozen multifusion");
  check(
      result.nodes.size() == 7U && result.root_node_id == K1NodeId{6} &&
          result.nodes[6].children ==
              std::vector<K1NodeId>{0U, 1U, 2U, 3U, 4U, 5U},
      "six-way event is not serialized as a binary chain");
  check(
      result.total_squared_weight == level(5) &&
          result.total_hgp_weight == level(5, 4),
      "six-way fixture preserves the exact total weight");
}

void test_permutation_invariance_with_ties() {
  const std::array<CertifiedPoint3, 4> source{
      point(-1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(1.0, 1.0, 0.0)};
  const K1EmstResult reference =
      build_exact_complete_graph_emst(canonical_cloud(source));
  check_result_invariants(reference, "square permutation reference");
  check(
      reference.equal_level_batches.size() == 2U &&
          reference.equal_level_batches[0].candidate_edge_count == 4U &&
          reference.equal_level_batches[0].multifusions.size() == 1U &&
          reference.equal_level_batches[0].multifusions[0].arity() == 4U &&
          reference.total_squared_weight == level(12),
      "square records one four-way fusion and exact tied weight");

  std::array<std::size_t, 4> permutation{0U, 1U, 2U, 3U};
  std::size_t permutation_count = 0U;
  do {
    std::array<CertifiedPoint3, 4> permuted{
        source[permutation[0]],
        source[permutation[1]],
        source[permutation[2]],
        source[permutation[3]]};
    const K1EmstResult actual =
        build_exact_complete_graph_emst(canonical_cloud(permuted));
    check(
        same_scientific_result(actual, reference),
        "square hierarchy is invariant under input permutation " +
            std::to_string(permutation_count));
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 24U, "all square input permutations were checked");
}

}  // namespace

int main() {
  test_singleton_birth_and_zero_cut();
  test_exact_quarter_level();
  test_disjoint_merges_in_one_batch();
  test_equal_level_multifusion_is_not_binarized();
  test_permutation_invariance_with_ties();

  if (failures != 0) {
    std::cerr << failures << " hierarchy EMST test(s) failed\n";
    return 1;
  }
  return 0;
}
