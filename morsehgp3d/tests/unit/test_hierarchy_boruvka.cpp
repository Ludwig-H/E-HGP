#include "morsehgp3d/hierarchy/boruvka.hpp"

#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
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
using morsehgp3d::hierarchy::K1BoruvkaVerification;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1Cut;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1CutEdgeSource;
using morsehgp3d::hierarchy::K1EmstResult;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::K1Multifusion;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::hierarchy::verify_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
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
  const ExactLevel exact_squared_length = level(squared_length);
  return ExactEmstEdge{
      u, v, exact_squared_length, quarter(exact_squared_length)};
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

[[nodiscard]] std::size_t theoretical_round_bound(std::size_t point_count) {
  if (point_count <= 1U) {
    return 0U;
  }
  std::size_t bound = 0U;
  std::size_t remaining = point_count - 1U;
  while (remaining != 0U) {
    ++bound;
    remaining >>= 1U;
  }
  return bound;
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

[[nodiscard]] bool same_boruvka_result(
    const K1ExactBoruvkaResult& left,
    const K1ExactBoruvkaResult& right) {
  return left.point_count == right.point_count &&
         left.rounds == right.rounds && left.emst_edges == right.emst_edges &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters &&
         left.emst_witness_certified == right.emst_witness_certified;
}

void check_verification(
    const K1BoruvkaVerification& verification,
    const K1ExactBoruvkaResult& result,
    const std::string& fixture) {
  check(
      verification.index_identity_certified &&
          verification.round_count_bound_certified &&
          verification.round_replay_certified &&
          verification.component_minima_certified &&
          verification.accepted_edges_certified &&
          verification.canonical_contractions_certified &&
          verification.spanning_tree_certified &&
          verification.exact_weights_certified &&
          verification.emst_witness_certified,
      fixture + " independently verifies the complete Boruvka witness");

  std::size_t component_minimum_count = 0U;
  for (const auto& round : result.rounds) {
    component_minimum_count += round.component_minima.size();
  }
  check(
      verification.replayed_round_count == result.rounds.size() &&
          verification.replayed_component_minimum_count ==
              component_minimum_count,
      fixture + " closes the independent replay counters");
}

void check_boruvka_invariants(
    const K1ExactBoruvkaResult& result,
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::string& fixture) {
  const std::size_t point_count = cloud.size();
  const std::size_t expected_round_bound =
      theoretical_round_bound(point_count);
  check(
      result.point_count == point_count && result.point_count > 0U,
      fixture + " preserves the nonempty canonical point count");
  check(
      result.emst_edges.size() == point_count - 1U,
      fixture + " emits exactly n-1 witness edges");
  check(
      std::is_sorted(
          result.emst_edges.begin(), result.emst_edges.end(), edge_less),
      fixture + " orders the final witness by exact weight and endpoints");
  check(
      result.total_hgp_weight == quarter(result.total_squared_weight),
      fixture + " applies the exact one-quarter HGP factor to total weight");
  check(
      result.emst_witness_certified,
      fixture + " exposes a local certified EMST witness");

  std::size_t previous_component_count = point_count;
  std::size_t frozen_component_label_count = 0U;
  std::size_t component_minimum_count = 0U;
  std::size_t accepted_edge_count = 0U;
  std::size_t uniform_lbvh_node_tag_count = 0U;
  std::size_t mixed_lbvh_node_tag_count = 0U;
  std::vector<ExactEmstEdge> accepted_edges;
  accepted_edges.reserve(point_count - 1U);

  for (std::size_t round_index = 0U; round_index < result.rounds.size();
       ++round_index) {
    const auto& round = result.rounds[round_index];
    check(
        round.round_index == round_index &&
            round.pre_round_component_count == previous_component_count,
        fixture + " chains each round from the preceding frozen partition");
    check(
        round.component_minima.size() == round.pre_round_component_count,
        fixture + " emits one exact minimum per frozen component");
    check(
        round.post_round_component_count < round.pre_round_component_count &&
            round.post_round_component_count <=
                round.pre_round_component_count / 2U,
        fixture + " contracts every nonterminal round by the Boruvka bound");
    check(
        round.accepted_edges.size() ==
            round.pre_round_component_count -
                round.post_round_component_count,
        fixture + " makes accepted forest edges equal component contractions");
    check(
        std::is_sorted(
            round.accepted_edges.begin(), round.accepted_edges.end(), edge_less),
        fixture + " orders each accepted round canonically");
    check(
        round.uniform_lbvh_node_count + round.mixed_lbvh_node_count ==
            index.build_counters().node_count,
        fixture + " classifies every LBVH node against the frozen labels");

    for (std::size_t minimum_index = 0U;
         minimum_index < round.component_minima.size(); ++minimum_index) {
      const auto& minimum = round.component_minima[minimum_index];
      if (minimum_index > 0U) {
        check(
            round.component_minima[minimum_index - 1U].component_label <
                minimum.component_label,
            fixture + " orders frozen component minima by canonical label");
      }
      check(
          minimum.outgoing_edge.u < minimum.outgoing_edge.v &&
              (minimum.source_point_id == minimum.outgoing_edge.u ||
               minimum.source_point_id == minimum.outgoing_edge.v) &&
              minimum.outgoing_edge.squared_length > level(0) &&
              minimum.outgoing_edge.merge_level ==
                  quarter(minimum.outgoing_edge.squared_length),
          fixture + " records a canonical positive outgoing edge and issuer");
    }

    for (const ExactEmstEdge& accepted : round.accepted_edges) {
      check(
          accepted.u < accepted.v && accepted.squared_length > level(0) &&
              accepted.merge_level == quarter(accepted.squared_length),
          fixture + " retains canonical accepted edges with exact levels");
      check(
          std::any_of(
              round.component_minima.begin(),
              round.component_minima.end(),
              [&accepted](const auto& minimum) {
                return minimum.outgoing_edge == accepted;
              }),
          fixture + " accepts only an edge proposed by a frozen component");
    }

    frozen_component_label_count += point_count;
    component_minimum_count += round.component_minima.size();
    accepted_edge_count += round.accepted_edges.size();
    uniform_lbvh_node_tag_count += round.uniform_lbvh_node_count;
    mixed_lbvh_node_tag_count += round.mixed_lbvh_node_count;
    accepted_edges.insert(
        accepted_edges.end(),
        round.accepted_edges.begin(),
        round.accepted_edges.end());
    previous_component_count = round.post_round_component_count;
  }

  check(
      previous_component_count == 1U,
      fixture + " finishes with one connected component");
  std::sort(accepted_edges.begin(), accepted_edges.end(), edge_less);
  check(
      accepted_edges == result.emst_edges,
      fixture + " final witness is exactly the union of accepted round edges");

  const auto& counters = result.counters;
  check(
      counters.point_count == point_count &&
          counters.lbvh_node_count == index.build_counters().node_count &&
          counters.round_count == result.rounds.size() &&
          counters.theoretical_max_round_count == expected_round_bound &&
          counters.round_count <= counters.theoretical_max_round_count,
      fixture + " closes point, index and theoretical round counters");
  check(
      counters.frozen_component_label_count ==
              frozen_component_label_count &&
          counters.component_minimum_count == component_minimum_count &&
          counters.accepted_edge_count == accepted_edge_count &&
          counters.uniform_lbvh_node_tag_count ==
              uniform_lbvh_node_tag_count &&
          counters.mixed_lbvh_node_tag_count ==
              mixed_lbvh_node_tag_count &&
          counters.component_contraction_count == point_count - 1U &&
          counters.final_component_count == 1U,
      fixture + " closes minima, accepted-edge and contraction counters");
  check(
      counters.point_query_count == point_count * result.rounds.size(),
      fixture + " performs one exact outgoing query per point and round");
  if (point_count > 1U) {
    check(
        counters.node_visit_count >= counters.point_query_count &&
            counters.exact_point_distance_evaluation_count >=
                counters.point_query_count,
        fixture + " accounts for every query by exact traversal work");
  }

  check_verification(
      verify_exact_lbvh_boruvka(index, cloud, result), result, fixture);
}

void check_against_complete_graph_anchor(
    const K1ExactBoruvkaResult& result,
    const K1EmstResult& reference,
    const std::string& fixture,
    bool require_same_witness) {
  check(
      result.point_count == reference.point_count &&
          result.total_squared_weight == reference.total_squared_weight &&
          result.total_hgp_weight == reference.total_hgp_weight,
      fixture + " agrees with the complete-graph anchor weights");
  if (require_same_witness) {
    check(
        result.emst_edges == reference.emst_edges,
        fixture + " agrees with the unique complete-graph witness");
  }

  const K1CompactForest compact = build_compact_k1_forest(
      result.point_count, std::span<const ExactEmstEdge>{result.emst_edges});
  check(
      compact.total_squared_weight == reference.total_squared_weight &&
          compact.total_hgp_weight == reference.total_hgp_weight &&
          compact.root_node_id == reference.root_node_id,
      fixture + " converts to the same compact root and exact weights");
  check(
      compact.materialize_nodes() == reference.nodes &&
          compact.materialize_multifusions() ==
              reference_multifusions(reference),
      fixture + " converts to the same canonical nodes and multifusions");

  check(
      compact.cut_strict(level(0)).empty() &&
          compact.cut_closed(level(0)) ==
              reference.cut(
                  level(0),
                  K1CutClosure::closed,
                  K1CutEdgeSource::complete_graph),
      fixture + " preserves strict and closed singleton births");
  for (const auto& batch : reference.equal_level_batches) {
    check(
        compact.cut_strict(batch.level) ==
                reference.cut(
                    batch.level,
                    K1CutClosure::strict,
                    K1CutEdgeSource::complete_graph) &&
            compact.cut_closed(batch.level) ==
                reference.cut(
                    batch.level,
                    K1CutClosure::closed,
                    K1CutEdgeSource::complete_graph),
        fixture + " preserves both cuts at every complete-graph level");
  }
}

void test_singleton() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult result =
      build_exact_lbvh_boruvka(index, cloud);
  const K1EmstResult reference = build_exact_complete_graph_emst(cloud);

  check_boruvka_invariants(result, index, cloud, "singleton");
  check_against_complete_graph_anchor(result, reference, "singleton", true);
  check(
      result.rounds.empty() && result.emst_edges.empty() &&
          result.total_squared_weight == level(0) &&
          result.total_hgp_weight == level(0) &&
          result.counters.point_query_count == 0U,
      "singleton needs no Boruvka round, edge or query");
}

void test_distinct_level_chain() {
  const std::array<CertifiedPoint3, 4> input{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(11.0, 0.0, 0.0),
      point(13.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult result =
      build_exact_lbvh_boruvka(index, cloud);
  const K1EmstResult reference = build_exact_complete_graph_emst(cloud);
  const std::vector<ExactEmstEdge> expected_edges{
      edge(PointId{0}, PointId{1}, 1),
      edge(PointId{2}, PointId{3}, 4),
      edge(PointId{1}, PointId{2}, 100)};

  check_boruvka_invariants(result, index, cloud, "distinct-level chain");
  check_against_complete_graph_anchor(
      result, reference, "distinct-level chain", true);
  check(
      result.emst_edges == expected_edges,
      "distinct-level chain selects its three exact consecutive edges");
  check(
      result.rounds.size() == 2U &&
          result.rounds[0].pre_round_component_count == 4U &&
          result.rounds[0].accepted_edges ==
              std::vector<ExactEmstEdge>{expected_edges[0], expected_edges[1]} &&
          result.rounds[0].post_round_component_count == 2U &&
          result.rounds[1].pre_round_component_count == 2U &&
          result.rounds[1].accepted_edges ==
              std::vector<ExactEmstEdge>{expected_edges[2]} &&
          result.rounds[1].post_round_component_count == 1U,
      "distinct-level chain freezes two pair contractions before its bridge");
  check(
      result.total_squared_weight == level(105) &&
          result.total_hgp_weight == level(105, 4),
      "distinct-level chain preserves exact total weights");

  K1ExactBoruvkaResult stale_flag = result;
  stale_flag.emst_witness_certified = false;
  const K1BoruvkaVerification stale_flag_verification =
      verify_exact_lbvh_boruvka(index, cloud, stale_flag);
  check(
      stale_flag_verification.emst_witness_certified,
      "verifier recomputes the witness instead of trusting its cached flag");

  K1ExactBoruvkaResult falsified_weight = result;
  falsified_weight.total_squared_weight = level(106);
  const K1BoruvkaVerification falsified_weight_verification =
      verify_exact_lbvh_boruvka(index, cloud, falsified_weight);
  check(
      !falsified_weight_verification.exact_weights_certified &&
          !falsified_weight_verification.emst_witness_certified,
      "verifier rejects a falsified exact total weight and aggregate witness");
}

void test_square_with_tail_ties_and_permutations() {
  const std::array<CertifiedPoint3, 5> source{
      point(-1.0, -1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(5.0, 0.0, 0.0)};
  const CanonicalPointCloud reference_cloud = canonical_cloud(source);
  const MortonLbvhIndex reference_index =
      MortonLbvhIndex::build(reference_cloud);
  const K1ExactBoruvkaResult expected =
      build_exact_lbvh_boruvka(reference_index, reference_cloud);
  const K1EmstResult reference =
      build_exact_complete_graph_emst(reference_cloud);

  check_boruvka_invariants(
      expected, reference_index, reference_cloud, "square with tail");
  check_against_complete_graph_anchor(
      expected, reference, "square with tail", true);

  const K1CompactForest compact = build_compact_k1_forest(
      expected.point_count,
      std::span<const ExactEmstEdge>{expected.emst_edges});
  check(
      compact.levels ==
              std::vector<ExactLevel>{level(1), level(17, 4)} &&
          compact.merge_nodes.size() == 2U &&
          compact.merge_nodes[0].child_count == 4U &&
          compact.merge_nodes[0].coverage_size == 4U &&
          compact.merge_nodes[1].child_count == 2U &&
          compact.merge_nodes[1].coverage_size == 5U,
      "square ties form one four-way multifusion before the tail joins");
  check(
      compact.cut_strict(level(1)) ==
              K1Cut{{PointId{0}}, {PointId{1}}, {PointId{2}}, {PointId{3}},
                    {PointId{4}}} &&
          compact.cut_closed(level(1)) ==
              K1Cut{{PointId{0}, PointId{1}, PointId{2}, PointId{3}},
                    {PointId{4}}} &&
          compact.cut_strict(level(17, 4)) == compact.cut_closed(level(1)) &&
          compact.cut_closed(level(17, 4)) ==
              K1Cut{{PointId{0}, PointId{1}, PointId{2}, PointId{3},
                    PointId{4}}},
      "square with tail preserves both sides of both exact equality levels");

  std::array<std::size_t, 4> permutation{0U, 1U, 2U, 3U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<CertifiedPoint3, 5> permuted{
        source[permutation[0]],
        source[permutation[1]],
        source[permutation[2]],
        source[permutation[3]],
        source[4]};
    const CanonicalPointCloud cloud = canonical_cloud(permuted);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const K1ExactBoruvkaResult actual =
        build_exact_lbvh_boruvka(index, cloud);
    check(
        same_boruvka_result(actual, expected),
        "square with tail is invariant under square permutation " +
            std::to_string(permutation_count));
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(
      permutation_count == 24U,
      "all square-with-tail square permutations were checked");
}

void test_invalid_index_and_cloud_inputs() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(7.0, 0.0, 0.0)};

  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult result =
      build_exact_lbvh_boruvka(index, cloud);
  const CanonicalPointCloud same_geometry_other_identity =
      canonical_cloud(input);
  check_throws<std::invalid_argument>(
      [&index, &same_geometry_other_identity]() {
        static_cast<void>(build_exact_lbvh_boruvka(
            index, same_geometry_other_identity));
      },
      "builder rejects an index from a different cloud identity");
  check_throws<std::invalid_argument>(
      [&index, &same_geometry_other_identity, &result]() {
        static_cast<void>(verify_exact_lbvh_boruvka(
            index, same_geometry_other_identity, result));
      },
      "verifier rejects an index from a different cloud identity");

  CanonicalPointCloud index_cloud = canonical_cloud(input);
  MortonLbvhIndex moved_index_source = MortonLbvhIndex::build(index_cloud);
  const MortonLbvhIndex retained_index = std::move(moved_index_source);
  check(retained_index.ready(), "moved-to index remains ready");
  check_throws<std::invalid_argument>(
      [&moved_index_source, &index_cloud]() {
        static_cast<void>(
            build_exact_lbvh_boruvka(moved_index_source, index_cloud));
      },
      "builder rejects a moved-from LBVH index");

  CanonicalPointCloud moved_cloud_source = canonical_cloud(input);
  const MortonLbvhIndex cloud_index =
      MortonLbvhIndex::build(moved_cloud_source);
  const CanonicalPointCloud retained_cloud = std::move(moved_cloud_source);
  check(retained_cloud.size() == input.size(), "moved-to cloud remains complete");
  check_throws<std::invalid_argument>(
      [&cloud_index, &moved_cloud_source]() {
        static_cast<void>(
            build_exact_lbvh_boruvka(cloud_index, moved_cloud_source));
      },
      "builder rejects a moved-from canonical cloud");
}

}  // namespace

int main() {
  test_singleton();
  test_distinct_level_chain();
  test_square_with_tail_ties_and_permutations();
  test_invalid_index_and_cloud_inputs();

  if (failures != 0) {
    std::cerr << failures << " hierarchy Boruvka test(s) failed\n";
    return 1;
  }
  return 0;
}
