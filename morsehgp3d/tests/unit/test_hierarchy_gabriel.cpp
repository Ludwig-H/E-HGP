#include "morsehgp3d/hierarchy/gabriel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::K1ExactAnchorResult;
using morsehgp3d::hierarchy::K1Cut;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1PairCatalogStatus;
using morsehgp3d::hierarchy::K1RankTwoCutEdgeSource;
using morsehgp3d::hierarchy::K1PairSphereClassification;
using morsehgp3d::hierarchy::K1PairSphereRecord;
using morsehgp3d::hierarchy::build_exact_k1_anchor;
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

[[nodiscard]] const K1PairSphereRecord& pair_record(
    const K1ExactAnchorResult& result, PointId u, PointId v) {
  const auto position = std::find_if(
      result.pair_catalog.pairs.begin(),
      result.pair_catalog.pairs.end(),
      [u, v](const K1PairSphereRecord& pair) {
        return pair.u == u && pair.v == v;
      });
  if (position == result.pair_catalog.pairs.end()) {
    throw std::logic_error("the requested canonical pair is absent");
  }
  return *position;
}

[[nodiscard]] bool same_anchor(
    const K1ExactAnchorResult& left,
    const K1ExactAnchorResult& right) {
  return left.emst.point_count == right.emst.point_count &&
         left.emst.complete_graph_edge_count ==
             right.emst.complete_graph_edge_count &&
         left.emst.total_squared_weight == right.emst.total_squared_weight &&
         left.emst.total_hgp_weight == right.emst.total_hgp_weight &&
         left.emst.complete_edges == right.emst.complete_edges &&
         left.emst.emst_edges == right.emst.emst_edges &&
         left.emst.nodes == right.emst.nodes &&
         left.emst.equal_level_batches == right.emst.equal_level_batches &&
         left.emst.root_node_id == right.emst.root_node_id &&
         left.emst.counters == right.emst.counters &&
         left.pair_catalog == right.pair_catalog &&
         left.rank_two_reduction == right.rank_two_reduction &&
         left.certificate == right.certificate;
}

void check_anchor_certificate(
    const K1ExactAnchorResult& result, const std::string& fixture) {
  const auto& certificate = result.certificate;
  check(
      certificate.exact_pair_decisions_complete,
      fixture + " has complete exact pair decisions");
  check(
      certificate.pair_universe_matches_emst,
      fixture + " pair universe matches the independent complete graph");
  check(
      certificate.comparison_level_count >= 1U,
      fixture + " compares the zero replay level");
  check(
      certificate.strict_cuts_match && certificate.closed_cuts_match,
      fixture + " has identical strict and closed cuts on all four paths");
  check(
      certificate.multifusions_match,
      fixture + " has identical frozen multifusions");
  check(
      certificate.selected_tree_edges_match &&
          certificate.selected_tree_squared_weight_matches &&
          certificate.selected_tree_hgp_weight_matches,
      fixture + " has identical deterministic witness tree and exact weights");
  check(
      certificate.selected_tree_hierarchy_matches &&
          certificate.all_selected_witness_edges_are_rank_two,
      fixture + " hierarchy is carried only by rank-two critical pairs");
  check(
      certificate.anchor_equivalence_certified,
      fixture + " closes its local k=1 equivalence certificate");
  check(
      result.rank_two_reduction.nodes == result.emst.nodes &&
          result.rank_two_reduction.root_node_id == result.emst.root_node_id,
      fixture + " has node-for-node EMST and rank-two hierarchies");
  check(
      result.rank_two_reduction.total_selected_squared_weight ==
              result.emst.total_squared_weight &&
          result.rank_two_reduction.total_selected_hgp_weight ==
              result.emst.total_hgp_weight,
      fixture + " preserves the two exact total-weight conventions");
}

void check_catalog_invariants(
    const K1ExactAnchorResult& result, const std::string& fixture) {
  const std::size_t point_count = result.pair_catalog.point_count;
  const std::size_t pair_count = point_count * (point_count - 1U) / 2U;
  const auto& counters = result.pair_catalog.counters;
  check(
      result.pair_catalog.pairs.size() == pair_count &&
          result.pair_catalog.all_pair_edges.size() == pair_count,
      fixture + " exhausts every canonical pair");
  check(
      counters.point_count == point_count && counters.pair_count == pair_count &&
          counters.support_analysis_count == pair_count &&
          counters.support_predicate_decision_count == 2U * pair_count &&
          counters.closed_ball_query_count == pair_count &&
          counters.exact_point_distance_evaluation_count ==
              pair_count * point_count,
      fixture + " closes its exact cubic work counters");
  check(
      counters.rank_two_critical_count +
              counters.extra_shell_degeneracy_count +
              counters.interior_blocked_count ==
          pair_count,
      fixture + " gives every pair exactly one classification");
  check(
      counters.gabriel_diagnostic_count ==
          counters.rank_two_critical_count +
              counters.extra_shell_degeneracy_count,
      fixture + " distinguishes Gabriel diagnostics from rank-two events");

  for (std::size_t index = 0U;
       index < result.pair_catalog.pairs.size();
       ++index) {
    const K1PairSphereRecord& pair = result.pair_catalog.pairs[index];
    check(
        pair.pair_index == index && pair.u < pair.v,
        fixture + " pair has its canonical lexicographic identity");
    check(
        pair.squared_length == ExactLevel{
                                   pair.level.numerator() * BigInt{4},
                                   pair.level.denominator()},
        fixture + " pair has exact d-squared over four level");
    check(
        pair.closed_rank == pair.interior_ids.size() + pair.shell_ids.size() &&
            pair.closed_rank + pair.exterior_count == point_count,
        fixture + " pair closes interior, shell and exterior cardinalities");
    check(
        std::is_sorted(pair.interior_ids.begin(), pair.interior_ids.end()) &&
            std::is_sorted(pair.shell_ids.begin(), pair.shell_ids.end()) &&
            std::binary_search(pair.shell_ids.begin(), pair.shell_ids.end(), pair.u) &&
            std::binary_search(pair.shell_ids.begin(), pair.shell_ids.end(), pair.v),
        fixture + " pair exposes a complete canonical shell");
    if (pair.classification == K1PairSphereClassification::rank_two_critical) {
      check(
          pair.interior_ids.empty() &&
              pair.shell_ids == std::vector<PointId>{pair.u, pair.v} &&
              pair.closed_rank == 2U,
          fixture + " rank-two event has exactly its support on the shell");
    } else if (
        pair.classification ==
        K1PairSphereClassification::extra_shell_degeneracy) {
      check(
          pair.interior_ids.empty() && pair.shell_ids.size() > 2U,
          fixture + " extra-shell diagnostic is not a rank-two event");
    } else {
      check(
          !pair.interior_ids.empty(),
          fixture + " non-Gabriel pair has a strict interior witness");
    }
  }
}

void test_singleton_anchor() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "singleton anchor");
  check_catalog_invariants(result, "singleton anchor");
  check(
      result.pair_catalog.pairs.empty() &&
          result.rank_two_reduction.rank_two_edges.empty() &&
          result.rank_two_reduction.nodes.size() == 1U,
      "singleton anchor contains only its level-zero leaf");
  check(
      result.pair_catalog.catalog_status == K1PairCatalogStatus::supported &&
          result.locally_supported(),
      "singleton anchor is locally supported without a pair degeneracy");
}

void test_blocked_pair_and_rank_two_chain() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(8.0, 0.0, 0.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "collinear rank-two chain");
  check_catalog_invariants(result, "collinear rank-two chain");
  const K1PairSphereRecord& blocked = pair_record(result, PointId{0}, PointId{2});
  check(
      blocked.classification == K1PairSphereClassification::interior_blocked &&
          blocked.squared_length == level(64) && blocked.level == level(16) &&
          blocked.interior_ids == std::vector<PointId>{PointId{1}} &&
          blocked.shell_ids ==
              std::vector<PointId>{PointId{0}, PointId{2}},
      "long collinear pair is rejected by its exact interior witness");
  check(
      result.pair_catalog.rank_two_edges.size() == 2U &&
          result.pair_catalog.gabriel_diagnostic_edges.size() == 2U &&
          result.rank_two_reduction.equal_level_batches.size() == 2U &&
          result.rank_two_reduction.total_selected_squared_weight == level(40),
      "two rank-two pairs reproduce the exact EMST chain");
  check(
      result.pair_catalog.catalog_status == K1PairCatalogStatus::supported &&
          result.locally_supported(),
      "interior-blocked non-events do not create a degeneracy status");
}

void test_extra_shell_is_exact_but_unsupported() {
  const std::array<CertifiedPoint3, 3> input{
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 0.0, 0.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "right-triangle extra shell");
  check_catalog_invariants(result, "right-triangle extra shell");
  const K1PairSphereRecord& diameter = pair_record(result, PointId{0}, PointId{2});
  check(
      diameter.classification ==
              K1PairSphereClassification::extra_shell_degeneracy &&
          diameter.interior_ids.empty() &&
          diameter.shell_ids ==
              std::vector<PointId>{PointId{0}, PointId{1}, PointId{2}} &&
          diameter.closed_rank == 3U && diameter.is_gabriel_diagnostic() &&
          !diameter.is_rank_two_critical(),
      "right-triangle diameter is Gabriel but not a rank-two event");
  check(
      result.pair_catalog.catalog_status ==
              K1PairCatalogStatus::unsupported_degeneracy &&
          !result.locally_supported(),
      "extra shell blocks local public-status eligibility without hiding decisions");
  check(
      result.pair_catalog.rank_two_edges.size() == 2U &&
          result.pair_catalog.gabriel_diagnostic_edges.size() == 3U &&
          result.rank_two_reduction.nodes == result.emst.nodes,
      "redundant extra-shell Gabriel edge does not change the hierarchy");
}

void test_one_ulp_classification_around_shell() {
  const double below = std::nextafter(1.0, 0.0);
  const double above = std::nextafter(1.0, std::numeric_limits<double>::infinity());
  const std::array<CertifiedPoint3, 3> inside_input{
      point(-1.0, 0.0, 0.0),
      point(0.0, below, 0.0),
      point(1.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 3> outside_input{
      point(-1.0, 0.0, 0.0),
      point(0.0, above, 0.0),
      point(1.0, 0.0, 0.0)};
  const K1ExactAnchorResult inside =
      build_exact_k1_anchor(canonical_cloud(inside_input));
  const K1ExactAnchorResult outside =
      build_exact_k1_anchor(canonical_cloud(outside_input));

  check_anchor_certificate(inside, "one-ULP inside anchor");
  check_anchor_certificate(outside, "one-ULP outside anchor");
  check(
      pair_record(inside, PointId{0}, PointId{2}).classification ==
          K1PairSphereClassification::interior_blocked,
      "one ULP below the diametric shell is strictly inside");
  check(
      pair_record(outside, PointId{0}, PointId{2}).classification ==
          K1PairSphereClassification::rank_two_critical,
      "one ULP above the diametric shell is strictly outside");
}

void test_two_disjoint_fusions_at_one_exact_level() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(10.0, 0.0, 0.0),
      point(12.0, 0.0, 0.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "two disjoint rank-two fusions");
  check_catalog_invariants(result, "two disjoint rank-two fusions");
  const auto batch = std::find_if(
      result.rank_two_reduction.equal_level_batches.begin(),
      result.rank_two_reduction.equal_level_batches.end(),
      [](const auto& candidate) { return candidate.level == level(1); });
  check(
      batch != result.rank_two_reduction.equal_level_batches.end() &&
          batch->rank_two_edges.size() == 2U &&
          batch->multifusions.size() == 2U &&
          batch->pre_batch_component_count == 4U &&
          batch->post_batch_component_count == 2U,
      "one frozen rank-two batch preserves two disjoint multifusions");

  const K1Cut strict_expected{
      {PointId{0}}, {PointId{1}}, {PointId{2}}, {PointId{3}}};
  const K1Cut closed_expected{
      {PointId{0}, PointId{1}}, {PointId{2}, PointId{3}}};
  check(
      result.rank_two_reduction.cut(
          level(1),
          K1CutClosure::strict,
          K1RankTwoCutEdgeSource::rank_two_graph) == strict_expected &&
          result.rank_two_reduction.cut(
              level(1),
              K1CutClosure::closed,
              K1RankTwoCutEdgeSource::rank_two_graph) == closed_expected,
      "strict and closed cuts differ by the entire equality batch at its level");
}

void test_regular_tetrahedron_rank_two_cycle() {
  const std::array<CertifiedPoint3, 4> input{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "regular tetrahedron");
  check_catalog_invariants(result, "regular tetrahedron");
  check(
      result.pair_catalog.catalog_status == K1PairCatalogStatus::supported &&
          result.pair_catalog.counters.rank_two_critical_count == 6U &&
          result.pair_catalog.counters.extra_shell_degeneracy_count == 0U &&
          result.rank_two_reduction.equal_level_batches.size() == 1U &&
          result.rank_two_reduction.equal_level_batches[0].level == level(2) &&
          result.rank_two_reduction.equal_level_batches[0].multifusions.size() ==
              1U &&
          result.rank_two_reduction.equal_level_batches[0]
                  .multifusions[0]
                  .arity() ==
              4U &&
          result.rank_two_reduction.selected_witness_edges.size() == 3U &&
          result.rank_two_reduction.counters.redundant_rank_two_edge_count == 3U,
      "regular tetrahedron contracts its six-edge 3D rank-two cycle atomically");
}

void test_square_multifusion_and_permutations() {
  const std::array<CertifiedPoint3, 4> source{
      point(-1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(1.0, 1.0, 0.0)};
  const K1ExactAnchorResult reference =
      build_exact_k1_anchor(canonical_cloud(source));
  check_anchor_certificate(reference, "square reference");
  check_catalog_invariants(reference, "square reference");
  check(
      reference.pair_catalog.counters.rank_two_critical_count == 4U &&
          reference.pair_catalog.counters.extra_shell_degeneracy_count == 2U &&
          reference.rank_two_reduction.equal_level_batches.size() == 1U &&
          reference.rank_two_reduction.equal_level_batches[0]
                  .multifusions.size() ==
              1U &&
          reference.rank_two_reduction.equal_level_batches[0]
                  .multifusions[0]
                  .arity() ==
              4U,
      "square sides form one four-way batch and diagonals are explicit degeneracies");

  std::array<std::size_t, 4> permutation{0U, 1U, 2U, 3U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<CertifiedPoint3, 4> permuted{
        source[permutation[0]],
        source[permutation[1]],
        source[permutation[2]],
        source[permutation[3]]};
    const K1ExactAnchorResult actual =
        build_exact_k1_anchor(canonical_cloud(permuted));
    check(
        same_anchor(actual, reference),
        "rank-two square anchor is permutation invariant " +
            std::to_string(permutation_count));
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 24U, "all square permutations were compared");
}

void test_gabriel_pair_beyond_fixed_local_neighbors() {
  const std::array<CertifiedPoint3, 12> input{
      point(-100.0, 0.0, 0.0),
      point(100.0, 0.0, 0.0),
      point(-100.0, 1.0, 1.0),
      point(-100.0, 2.0, 4.0),
      point(-100.0, 3.0, 9.0),
      point(-100.0, 4.0, 16.0),
      point(-100.0, 5.0, 25.0),
      point(100.0, 1.0, 1.0),
      point(100.0, 2.0, 4.0),
      point(100.0, 3.0, 9.0),
      point(100.0, 4.0, 16.0),
      point(100.0, 5.0, 25.0)};
  const K1ExactAnchorResult result = build_exact_k1_anchor(canonical_cloud(input));

  check_anchor_certificate(result, "nonlocal Gabriel fixture");
  check_catalog_invariants(result, "nonlocal Gabriel fixture");
  const K1PairSphereRecord& target = pair_record(result, PointId{0}, PointId{6});
  check(
      target.classification == K1PairSphereClassification::rank_two_critical &&
          target.squared_length == level(40000) && target.level == level(10000) &&
          target.interior_ids.empty() &&
          target.shell_ids ==
              std::vector<PointId>{PointId{0}, PointId{6}},
      "global catalogue retains the Gabriel pair missing from fixed small LNN lists");
  for (const PointId endpoint : {PointId{0}, PointId{6}}) {
    const std::size_t closer_count = static_cast<std::size_t>(std::count_if(
        result.pair_catalog.pairs.begin(),
        result.pair_catalog.pairs.end(),
        [endpoint](const K1PairSphereRecord& pair) {
          return (pair.u == endpoint || pair.v == endpoint) &&
                 pair.squared_length < level(40000);
        }));
    check(
        closer_count == 5U,
        "target endpoint has five strictly closer observations");
  }
}

}  // namespace

int main() {
  test_singleton_anchor();
  test_blocked_pair_and_rank_two_chain();
  test_extra_shell_is_exact_but_unsupported();
  test_one_ulp_classification_around_shell();
  test_two_disjoint_fusions_at_one_exact_level();
  test_regular_tetrahedron_rank_two_cycle();
  test_square_multifusion_and_permutations();
  test_gabriel_pair_beyond_fixed_local_neighbors();

  if (failures != 0) {
    std::cerr << failures << " hierarchy Gabriel test(s) failed\n";
    return 1;
  }
  return 0;
}
