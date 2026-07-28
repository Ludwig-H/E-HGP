#include "morsehgp3d/hierarchy/yao48_emst.hpp"

#include "morsehgp3d/hierarchy/boruvka.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::ExactYao48EmstResult;
using morsehgp3d::hierarchy::build_exact_complete_graph_emst;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::hierarchy::build_exact_yao48_emst_reference;
using morsehgp3d::hierarchy::classify_exact_yao48_cone;
using morsehgp3d::hierarchy::exact_yao48_cone_count;
using morsehgp3d::hierarchy::exact_yao48_emst_reference_max_point_count;
using morsehgp3d::hierarchy::verify_exact_yao48_emst_reference;
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

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
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

void check_against_complete_graph(
    const CanonicalPointCloud& cloud,
    const ExactYao48EmstResult& yao,
    const std::string& fixture) {
  const auto complete = build_exact_complete_graph_emst(cloud);
  check(
      yao.emst_edges.size() == cloud.size() - 1U &&
          yao.counters.final_component_count == 1U,
      fixture + " emits a spanning tree");
  check(
      yao.total_squared_weight == complete.total_squared_weight &&
          yao.total_hgp_weight == complete.total_hgp_weight,
      fixture + " has the exact complete-graph MST weight");
  check(
      yao.emst_edges == complete.emst_edges,
      fixture + " retains the canonical complete-graph Kruskal tree");
  check(
      verify_exact_yao48_emst_reference(cloud, yao).transcript_certified(),
      fixture + " passes the independent full-transcript replay");
}

void test_all_signed_ordered_cones_and_boundaries() {
  constexpr std::array<std::array<std::uint8_t, 3>, 6> permutations{{
      {{0U, 1U, 2U}},
      {{0U, 2U, 1U}},
      {{1U, 0U, 2U}},
      {{1U, 2U, 0U}},
      {{2U, 0U, 1U}},
      {{2U, 1U, 0U}},
  }};
  constexpr std::array<double, 3> magnitudes{{4.0, 2.0, 1.0}};
  const CertifiedPoint3 origin = point(0.0, 0.0, 0.0);
  std::array<bool, exact_yao48_cone_count> seen{};

  for (std::uint8_t mask = 0U; mask < 8U; ++mask) {
    for (std::size_t order_index = 0U;
         order_index < permutations.size();
         ++order_index) {
      std::array<double, 3> coordinates{};
      for (std::size_t rank = 0U; rank < 3U; ++rank) {
        const std::size_t axis =
            static_cast<std::size_t>(permutations[order_index][rank]);
        const std::uint8_t bit = static_cast<std::uint8_t>(
            std::uint32_t{1U} << static_cast<std::uint32_t>(axis));
        coordinates[axis] =
            (mask & bit) == 0U ? magnitudes[rank] : -magnitudes[rank];
      }
      const auto key = classify_exact_yao48_cone(
          origin,
          point(coordinates[0], coordinates[1], coordinates[2]));
      const std::size_t expected_index =
          static_cast<std::size_t>(mask) * permutations.size() + order_index;
      check(
          key.negative_axis_mask == mask &&
              key.axes_by_descending_absolute_delta ==
                  permutations[order_index] &&
              key.cone_index == expected_index,
          "signed strict chamber receives its unique canonical index " +
              std::to_string(expected_index));
      check(
          !seen[key.cone_index],
          "signed strict chambers do not overlap at index " +
              std::to_string(key.cone_index));
      seen[key.cone_index] = true;
    }
  }
  check(
      std::all_of(seen.begin(), seen.end(), [](bool value) { return value; }),
      "the strict representatives cover all 48 Yao chambers");

  const auto equal_absolute_boundary =
      classify_exact_yao48_cone(origin, point(-1.0, 1.0, 1.0));
  check(
      equal_absolute_boundary.negative_axis_mask == 1U &&
          equal_absolute_boundary.axes_by_descending_absolute_delta ==
              std::array<std::uint8_t, 3>{{0U, 1U, 2U}} &&
          equal_absolute_boundary.cone_index == 6U,
      "equal absolute deltas use increasing axis as the half-open boundary");

  const auto zero_boundary =
      classify_exact_yao48_cone(origin, point(0.0, -1.0, 0.0));
  check(
      zero_boundary.negative_axis_mask == 2U &&
          zero_boundary.axes_by_descending_absolute_delta ==
              std::array<std::uint8_t, 3>{{1U, 0U, 2U}} &&
          zero_boundary.cone_index == 14U,
      "zero deltas use positive signs and increasing-axis tie breaking");

  const auto opposite_corner = classify_exact_yao48_cone(
      point(1.0, 1.0, 1.0), origin);
  check(
      opposite_corner.negative_axis_mask == 7U &&
          opposite_corner.axes_by_descending_absolute_delta ==
              std::array<std::uint8_t, 3>{{0U, 1U, 2U}} &&
          opposite_corner.cone_index == 42U,
      "the opposite direction receives its distinct signed chamber");

  // For the three extreme rays of x>=y>=z>=0, 4*(a.b)^2 >
  // |a|^2*|b|^2.  Hence every pair has angle strictly below pi/3;
  // the smallest margin is the pair (1,0,0), (1,1,1): 4 > 3.
  constexpr std::array<std::array<std::int64_t, 3>, 3> extreme_rays{{
      {{1, 0, 0}},
      {{1, 1, 0}},
      {{1, 1, 1}},
  }};
  for (std::size_t right = 1U; right < extreme_rays.size(); ++right) {
    for (std::size_t left = 0U; left < right; ++left) {
      std::int64_t dot = 0;
      std::int64_t left_norm = 0;
      std::int64_t right_norm = 0;
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        dot += extreme_rays[left][axis] * extreme_rays[right][axis];
        left_norm += extreme_rays[left][axis] * extreme_rays[left][axis];
        right_norm += extreme_rays[right][axis] * extreme_rays[right][axis];
      }
      check(
          4 * dot * dot > left_norm * right_norm,
          "canonical cone extreme rays have angle strictly below pi/3");
    }
  }

  check_throws<std::invalid_argument>(
      [&origin]() {
        static_cast<void>(classify_exact_yao48_cone(origin, origin));
      },
      "the zero direction has no Yao chamber");
}

void test_singleton_and_quadratic_guard() {
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactYao48EmstResult result =
      build_exact_yao48_emst_reference(cloud);
  check(
      result.locally_structurally_valid() &&
          verify_exact_yao48_emst_reference(cloud, result)
              .transcript_certified(),
      "singleton closes structural validation and independent replay");
  check(
      result.directed_candidates.empty() &&
          result.unique_candidate_edges.empty() && result.emst_edges.empty(),
      "singleton emits no candidate or tree edge");
  check(
      result.counters.unordered_pair_count == 0U &&
          result.counters.exact_squared_distance_evaluation_count == 0U &&
          result.counters.directed_cone_classification_count == 0U &&
          result.counters.empty_point_cone_count == exact_yao48_cone_count &&
          result.counters.final_component_count == 1U,
      "singleton counters close exactly");

  std::vector<CertifiedPoint3> oversized;
  oversized.reserve(exact_yao48_emst_reference_max_point_count + 1U);
  for (std::size_t index = 0U;
       index <= exact_yao48_emst_reference_max_point_count;
       ++index) {
    oversized.push_back(point(static_cast<double>(index), 0.0, 0.0));
  }
  const CanonicalPointCloud oversized_cloud = canonical_cloud(oversized);
  check_throws<std::length_error>(
      [&oversized_cloud]() {
        static_cast<void>(build_exact_yao48_emst_reference(oversized_cloud));
      },
      "quadratic Yao reference rejects clouds above its explicit oracle limit");
}

void test_equal_distance_candidate_uses_canonical_edge_key() {
  // The last two points have the same squared distance 14225 from the origin
  // and lie in the same strict chamber x>y>z>0.  Canonical point ids are
  // origin=0, (100,56,33)=1 and (100,63,16)=2.
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0, 0.0),
      point(100.0, 56.0, 33.0),
      point(100.0, 63.0, 16.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactYao48EmstResult yao = build_exact_yao48_emst_reference(cloud);
  const auto candidate = std::find_if(
      yao.directed_candidates.begin(),
      yao.directed_candidates.end(),
      [](const auto& value) {
        return value.source_point_id == PointId{0} &&
               value.cone_index == 0U;
      });
  check(
      candidate != yao.directed_candidates.end() &&
          candidate->edge.u == PointId{0} &&
          candidate->edge.v == PointId{1},
      "equal-distance same-cone candidates use canonical endpoint tie break");
  check_against_complete_graph(cloud, yao, "same-cone equal-distance fixture");
}

void test_independent_replay_rejects_transcript_mutations() {
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0, 0.0),
      point(100.0, 56.0, 33.0),
      point(100.0, 63.0, 16.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactYao48EmstResult baseline =
      build_exact_yao48_emst_reference(cloud);
  check(
      verify_exact_yao48_emst_reference(cloud, baseline)
          .transcript_certified(),
      "fresh replay accepts the unmodified equal-distance transcript");

  ExactYao48EmstResult candidate_mutation = baseline;
  candidate_mutation.directed_candidates.front().edge.squared_length =
      ExactLevel{};
  const auto candidate_verification = verify_exact_yao48_emst_reference(
      cloud, candidate_mutation);
  check(
      !candidate_verification.directed_candidate_transcript_exact &&
          !candidate_verification.full_transcript_exact &&
          !candidate_verification.transcript_certified(),
      "fresh replay rejects a falsified per-cone candidate edge");

  ExactYao48EmstResult cone_mutation = baseline;
  cone_mutation.directed_candidates.front().cone_index =
      (cone_mutation.directed_candidates.front().cone_index + 1U) %
      exact_yao48_cone_count;
  const auto cone_verification =
      verify_exact_yao48_emst_reference(cloud, cone_mutation);
  check(
      !cone_verification.directed_candidate_transcript_exact &&
          !cone_verification.transcript_certified(),
      "fresh replay rejects a falsified half-open cone key");

  ExactYao48EmstResult weight_mutation = baseline;
  weight_mutation.total_squared_weight = ExactLevel{};
  const auto weight_verification =
      verify_exact_yao48_emst_reference(cloud, weight_mutation);
  check(
      !weight_verification.exact_totals_match &&
          !weight_verification.transcript_certified(),
      "fresh replay rejects a falsified exact aggregate weight");

  ExactYao48EmstResult final_edge_mutation = baseline;
  final_edge_mutation.emst_edges.front().v =
      final_edge_mutation.emst_edges.front().u;
  const auto final_edge_verification = verify_exact_yao48_emst_reference(
      cloud, final_edge_mutation);
  check(
      !final_edge_verification.canonical_kruskal_transcript_exact &&
          !final_edge_verification.transcript_certified(),
      "fresh replay rejects a falsified final Kruskal edge");

  ExactYao48EmstResult counter_mutation = baseline;
  ++counter_mutation.counters.unordered_pair_count;
  const auto counter_verification =
      verify_exact_yao48_emst_reference(cloud, counter_mutation);
  check(
      !counter_verification.counters_match &&
          !counter_verification.transcript_certified(),
      "fresh replay rejects a falsified audit counter");
}

void test_yao_candidates_are_not_a_gabriel_catalogue() {
  // Point (5,14,0) is strictly inside the diametral ball of (0,0,0) and
  // (100,0,0): 45^2 + 14^2 = 2221 < 2500.  It lies in a different Yao
  // chamber from the long edge at the first endpoint, so that non-Gabriel
  // edge remains a Yao candidate.  Only the Kruskal output is an EMST claim.
  const std::array<CertifiedPoint3, 3> input{
      point(0.0, 0.0, 0.0),
      point(5.0, 14.0, 0.0),
      point(100.0, 0.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactYao48EmstResult yao = build_exact_yao48_emst_reference(cloud);
  const bool long_edge_is_a_candidate = std::any_of(
      yao.unique_candidate_edges.begin(),
      yao.unique_candidate_edges.end(),
      [](const ExactEmstEdge& edge) {
        return edge.u == PointId{0} && edge.v == PointId{2};
      });
  check(
      long_edge_is_a_candidate,
      "Yao candidate graph deliberately remains distinct from Gabriel graph");
  check(
      std::none_of(
          yao.emst_edges.begin(),
          yao.emst_edges.end(),
          [](const ExactEmstEdge& edge) {
            return edge.u == PointId{0} && edge.v == PointId{2};
          }),
      "canonical Kruskal removes the explicit non-Gabriel Yao candidate");
  check_against_complete_graph(cloud, yao, "non-Gabriel Yao candidate fixture");
}

void test_small_3d_cloud_against_both_emst_oracles() {
  const std::array<CertifiedPoint3, 9> input{
      point(-7.0, 3.0, 2.0),
      point(-3.0, -5.0, 11.0),
      point(0.0, 0.0, 0.0),
      point(2.0, 9.0, -4.0),
      point(6.0, -2.0, 5.0),
      point(9.0, 7.0, 1.0),
      point(13.0, -6.0, -8.0),
      point(15.0, 4.0, 12.0),
      point(21.0, 1.0, -3.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const ExactYao48EmstResult yao = build_exact_yao48_emst_reference(cloud);
  check_against_complete_graph(cloud, yao, "irregular 3D fixture");

  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto boruvka = build_exact_lbvh_boruvka(index, cloud);
  check(
      yao.emst_edges == boruvka.emst_edges &&
          yao.total_squared_weight == boruvka.total_squared_weight &&
          yao.total_hgp_weight == boruvka.total_hgp_weight,
      "irregular 3D fixture matches the exact LBVH Boruvka oracle");
  check(
      yao.locally_structurally_valid() &&
          yao.directed_candidates.size() <=
              cloud.size() * exact_yao48_cone_count &&
          std::is_sorted(
              yao.unique_candidate_edges.begin(),
              yao.unique_candidate_edges.end(),
              edge_less) &&
          std::is_sorted(
              yao.emst_edges.begin(), yao.emst_edges.end(), edge_less),
      "irregular 3D fixture closes candidate bounds and canonical ordering");
}

void test_boundary_and_tie_subsets_against_canonical_kruskal() {
  // Symmetric integer rays deliberately create distance ties, zero-coordinate
  // boundaries and |dx|=|dy|=|dz| boundaries.  Every five-point subset is a
  // permanent bounded adversarial check of the canonical-tree claim.
  const std::array<CertifiedPoint3, 12> pool{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(1.0, 1.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(-1.0, 0.0, 0.0),
      point(-1.0, -1.0, 0.0),
      point(-1.0, -1.0, -1.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(1.0, -1.0, 1.0)};
  std::array<std::size_t, 5> indices{{0U, 1U, 2U, 3U, 4U}};
  std::size_t subset_count = 0U;
  bool all_spanning = true;
  bool all_minimum_weight = true;
  bool all_canonical = true;
  while (true) {
    std::array<CertifiedPoint3, 5> subset{
        pool[indices[0]],
        pool[indices[1]],
        pool[indices[2]],
        pool[indices[3]],
        pool[indices[4]]};
    const CanonicalPointCloud cloud = canonical_cloud(subset);
    const ExactYao48EmstResult yao = build_exact_yao48_emst_reference(cloud);
    const auto complete = build_exact_complete_graph_emst(cloud);
    all_spanning = all_spanning && yao.emst_edges.size() == 4U &&
                   yao.counters.final_component_count == 1U;
    all_minimum_weight =
        all_minimum_weight &&
        yao.total_squared_weight == complete.total_squared_weight;
    all_canonical = all_canonical && yao.emst_edges == complete.emst_edges;
    ++subset_count;

    bool advanced = false;
    std::size_t position = indices.size();
    while (position > 0U) {
      --position;
      const std::size_t limit = pool.size() - indices.size() + position;
      if (indices[position] < limit) {
        ++indices[position];
        for (std::size_t tail = position + 1U;
             tail < indices.size();
             ++tail) {
          indices[tail] = indices[tail - 1U] + 1U;
        }
        advanced = true;
        break;
      }
    }
    if (!advanced) {
      break;
    }
  }
  check(subset_count == 792U, "all 792 adversarial five-point subsets run");
  check(all_spanning, "every adversarial subset yields a spanning Yao tree");
  check(
      all_minimum_weight,
      "every adversarial subset matches the complete-graph MST weight");
  check(
      all_canonical,
      "every adversarial subset retains the canonical Kruskal edge set");
}

}  // namespace

int main() {
  test_all_signed_ordered_cones_and_boundaries();
  test_singleton_and_quadratic_guard();
  test_equal_distance_candidate_uses_canonical_edge_key();
  test_independent_replay_rejects_transcript_mutations();
  test_yao_candidates_are_not_a_gabriel_catalogue();
  test_small_3d_cloud_against_both_emst_oracles();
  test_boundary_and_tie_subsets_against_canonical_kruskal();

  if (failures != 0) {
    std::cerr << failures << " exact Yao48 EMST test(s) failed\n";
    return 1;
  }
  return 0;
}
