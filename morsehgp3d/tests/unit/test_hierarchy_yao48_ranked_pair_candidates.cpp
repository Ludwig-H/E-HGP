#include "morsehgp3d/hierarchy/yao48_ranked_pair_candidates.hpp"

#include "morsehgp3d/exact/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactYao48RankCutoffReceipt;
using morsehgp3d::hierarchy::ExactYao48RankedPairCandidateResult;
using morsehgp3d::hierarchy::build_exact_yao48_ranked_pair_candidate_reference;
using morsehgp3d::hierarchy::classify_exact_yao48_cone;
using morsehgp3d::hierarchy::exact_yao48_cone_count;
using morsehgp3d::hierarchy::
    exact_yao48_directional_rank_cutoff_certifies_above;
using morsehgp3d::hierarchy::
    exact_yao48_ranked_pair_candidate_reference_max_point_count;
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

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel squared_distance(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second) {
  return ExactLevel{morsehgp3d::exact::squared_distance(
      cloud.point(first), cloud.point(second))};
}

[[nodiscard]] std::size_t closed_rank(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second) {
  std::size_t rank = 0U;
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId witness = static_cast<PointId>(point_index);
    ExactRational phi;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      phi = phi +
          (cloud.point(witness).coordinate(axis) -
           cloud.point(first).coordinate(axis)) *
              (cloud.point(witness).coordinate(axis) -
               cloud.point(second).coordinate(axis));
    }
    if (phi.sign() <= 0) {
      ++rank;
    }
  }
  return rank;
}

[[nodiscard]] PointId canonical_id_from_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a source point has no canonical PointId");
}

[[nodiscard]] bool point_in_closed_diametral_ball(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second,
    PointId witness) {
  ExactRational phi;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    phi = phi +
        (cloud.point(witness).coordinate(axis) -
         cloud.point(first).coordinate(axis)) *
            (cloud.point(witness).coordinate(axis) -
             cloud.point(second).coordinate(axis));
  }
  return phi.sign() <= 0;
}

[[nodiscard]] std::set<std::array<PointId, 2>> candidate_keys(
    const ExactYao48RankedPairCandidateResult& result) {
  std::set<std::array<PointId, 2>> keys;
  for (const auto& candidate : result.candidates) {
    keys.insert(candidate.support_ids);
  }
  return keys;
}

void check_failing_direction_receipt(
    const CanonicalPointCloud& cloud,
    const ExactYao48RankedPairCandidateResult& result,
    PointId anchor,
    PointId target,
    std::size_t cone_index,
    const std::string& fixture) {
  const ExactYao48RankCutoffReceipt& receipt =
      result.cutoff_receipts[
          static_cast<std::size_t>(anchor) * exact_yao48_cone_count +
          cone_index];
  check(
      receipt.kth_squared_distance.has_value(),
      fixture + " rejected direction has a finite Kth cutoff");
  if (!receipt.kth_squared_distance.has_value()) {
    return;
  }
  check(
      exact_yao48_directional_rank_cutoff_certifies_above(
          cloud.point(anchor),
          cloud.point(target),
          *receipt.kth_squared_distance),
      fixture + " rejected direction satisfies the exact projection cutoff");
  check(
      receipt.retained_nearest_point_count == result.requested_order,
      fixture + " rejection retains exactly K witness ids");
  for (std::size_t index = 0U;
       index < receipt.retained_nearest_point_count;
       ++index) {
    const PointId witness = receipt.nearest_point_ids[index];
    check(
        witness != anchor && witness != target,
        fixture + " cutoff witness is distinct from both supports");
    check(
        classify_exact_yao48_cone(
            cloud.point(anchor), cloud.point(witness))
                .cone_index == cone_index,
        fixture + " cutoff witness belongs to the directed chamber");
    check(
        point_in_closed_diametral_ball(
            cloud, anchor, target, witness),
        fixture + " cutoff witness is in the closed diametral ball");
  }
}

void test_exhaustive_rank_cutoff_differential() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(-7.0, 2.0, 5.0),
      point(-4.0, -6.0, 1.0),
      point(-3.0, 7.0, -5.0),
      point(-1.0, -2.0, -8.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 9.0, 3.0),
      point(2.0, -7.0, 6.0),
      point(4.0, 3.0, -6.0),
      point(6.0, -1.0, -3.0),
      point(8.0, 5.0, 2.0),
      point(9.0, -4.0, 7.0),
  });

  for (std::size_t order = 1U; order <= 10U; ++order) {
    const ExactYao48RankedPairCandidateResult result =
        build_exact_yao48_ranked_pair_candidate_reference(cloud, order);
    const ExactYao48RankedPairCandidateResult replay =
        build_exact_yao48_ranked_pair_candidate_reference(cloud, order);
    check(
        result == replay,
        "Yao48 ranked-pair oracle is deterministic at order " +
            std::to_string(order));
    check(
        result.locally_structurally_valid(),
        "Yao48 ranked-pair local contract closes at order " +
            std::to_string(order));
    const std::set<std::array<PointId, 2>> candidates =
        candidate_keys(result);

    for (std::size_t first_index = 0U;
         first_index < cloud.size();
         ++first_index) {
      const PointId first = static_cast<PointId>(first_index);
      for (std::size_t second_index = first_index + 1U;
           second_index < cloud.size();
           ++second_index) {
        const PointId second = static_cast<PointId>(second_index);
        const std::array<PointId, 2> support{{first, second}};
        const std::size_t rank = closed_rank(cloud, first, second);
        const bool admitted = candidates.contains(support);
        if (rank <= order + 1U) {
          check(
              admitted,
              "every rank-at-most-K+1 pair survives both Yao48 cutoffs");
        }
        if (admitted) {
          continue;
        }
        check(
            rank >= order + 2U,
            "every Yao48 cutoff rejection is above the requested rank");
        const std::size_t first_cone =
            classify_exact_yao48_cone(
                cloud.point(first), cloud.point(second))
                .cone_index;
        const std::size_t second_cone =
            classify_exact_yao48_cone(
                cloud.point(second), cloud.point(first))
                .cone_index;
        const ExactLevel pair_distance =
            squared_distance(cloud, first, second);
        const ExactYao48RankCutoffReceipt& first_receipt =
            result.cutoff_receipts[
                first_index * exact_yao48_cone_count + first_cone];
        const ExactYao48RankCutoffReceipt& second_receipt =
            result.cutoff_receipts[
                second_index * exact_yao48_cone_count + second_cone];
        const bool first_failed =
            first_receipt.kth_squared_distance.has_value() &&
            exact_yao48_directional_rank_cutoff_certifies_above(
                cloud.point(first),
                cloud.point(second),
                *first_receipt.kth_squared_distance);
        const bool second_failed =
            second_receipt.kth_squared_distance.has_value() &&
            exact_yao48_directional_rank_cutoff_certifies_above(
                cloud.point(second),
                cloud.point(first),
                *second_receipt.kth_squared_distance);
        check(
            first_failed || second_failed,
            "every omitted pair has at least one directed cutoff receipt");
        if (first_failed) {
          check_failing_direction_receipt(
              cloud,
              result,
              first,
              second,
              first_cone,
              "first endpoint");
        }
        if (second_failed) {
          check_failing_direction_receipt(
              cloud,
              result,
              second,
              first,
              second_cone,
              "second endpoint");
        }
      }
    }
  }
}

void test_directional_projection_strictly_tightens_uniform_radius() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(1.125, 0.5, 0.5),
  });
  const PointId anchor = canonical_id_from_source(cloud, 0U);
  const PointId witness = canonical_id_from_source(cloud, 1U);
  const PointId target = canonical_id_from_source(cloud, 2U);
  const ExactLevel witness_distance = squared_distance(cloud, anchor, witness);
  const ExactLevel target_distance = squared_distance(cloud, anchor, target);
  check(
      target_distance < ExactLevel{
          morsehgp3d::exact::BigInt{3} * witness_distance.numerator(),
          witness_distance.denominator()},
      "the directional fixture lies strictly inside the uniform sqrt(3) radius");
  check(
      exact_yao48_directional_rank_cutoff_certifies_above(
          cloud.point(anchor), cloud.point(target), witness_distance),
      "the three exact extreme-ray projections certify the tighter rejection");

  const ExactYao48RankedPairCandidateResult result =
      build_exact_yao48_ranked_pair_candidate_reference(cloud, 1U);
  check(
      !candidate_keys(result).contains(
          std::array<PointId, 2>{std::min(anchor, target),
                                 std::max(anchor, target)}),
      "the bounded oracle applies the directional refinement");
  check(
      result.counters.directional_refinement_pruned_pair_count >= 1U,
      "the audit distinguishes projection-only prunes from uniform radial prunes");
}

void test_fixed_prefix_counterexample_survives_adaptive_cutoff() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(-120.0, 121.0, -104.0),
      point(-195.0, 41.0, 21.0),
      point(132.0, -117.0, 94.0),
      point(-202.0, 38.0, 14.0),
      point(194.0, -106.0, -66.0),
      point(-132.0, 116.0, -106.0),
      point(67.0, -123.0, 166.0),
      point(-194.0, 107.0, 46.0),
      point(-218.0, -13.0, 35.0),
  });
  const PointId a = canonical_id_from_source(cloud, 0U);
  const PointId b = canonical_id_from_source(cloud, 1U);
  const PointId c = canonical_id_from_source(cloud, 2U);
  const ExactYao48RankedPairCandidateResult result =
      build_exact_yao48_ranked_pair_candidate_reference(cloud, 2U);
  const auto keys = candidate_keys(result);
  const auto contains_side = [&keys](PointId first, PointId second) {
    return keys.contains(std::array<PointId, 2>{
        std::min(first, second), std::max(first, second)});
  };
  check(
      contains_side(a, b) && contains_side(a, c) && contains_side(b, c),
      "a failed fixed top-m prefix does not falsify the adaptive Yao48 cutoff");
}

void test_closed_equality_is_pruned() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
  });
  const ExactYao48RankedPairCandidateResult result =
      build_exact_yao48_ranked_pair_candidate_reference(cloud, 1U);
  const std::set<std::array<PointId, 2>> candidates =
      candidate_keys(result);
  check(
      !candidates.contains(std::array<PointId, 2>{{0U, 2U}}),
      "distance-squared equality 3*d_1^2 is excluded");
  check(
      closed_rank(cloud, 0U, 2U) == 3U,
      "the equality witness lies on the closed diametral shell");
}

void test_target_inside_top_k_is_not_counted_as_a_witness() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
  });
  const PointId anchor = canonical_id_from_source(cloud, 0U);
  const PointId target = canonical_id_from_source(cloud, 1U);
  const std::size_t cone_index =
      classify_exact_yao48_cone(
          cloud.point(anchor), cloud.point(target))
          .cone_index;
  const ExactYao48RankedPairCandidateResult result =
      build_exact_yao48_ranked_pair_candidate_reference(cloud, 1U);
  const ExactYao48RankCutoffReceipt& receipt =
      result.cutoff_receipts[
          static_cast<std::size_t>(anchor) * exact_yao48_cone_count +
          cone_index];
  check(
      receipt.retained_nearest_point_count == 1U &&
          receipt.nearest_point_ids[0] == target &&
          receipt.kth_squared_distance.has_value(),
      "the two-point fixture makes the target its own chamber top-1 entry");
  check(
      receipt.kth_squared_distance.has_value() &&
          !exact_yao48_directional_rank_cutoff_certifies_above(
              cloud.point(anchor),
              cloud.point(target),
              *receipt.kth_squared_distance),
      "a target inside the top-K cannot certify itself as a non-support witness");
  check(
      candidate_keys(result).contains(
          std::array<PointId, 2>{std::min(anchor, target),
                                 std::max(anchor, target)}) &&
          closed_rank(cloud, anchor, target) == 2U,
      "the exact rank-K+1 pair survives when the target occupies the top-K prefix");
}

void test_distance_tie_excluded_by_point_id_does_not_prune() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0),
      point(7.0, 1.0, 0.0),
      point(5.0, 5.0, 0.0),
  });
  const PointId anchor = canonical_id_from_source(cloud, 0U);
  const PointId first_tied = canonical_id_from_source(cloud, 1U);
  const PointId second_tied = canonical_id_from_source(cloud, 2U);
  const std::size_t cone_index =
      classify_exact_yao48_cone(
          cloud.point(anchor), cloud.point(first_tied))
          .cone_index;
  check(
      classify_exact_yao48_cone(
          cloud.point(anchor), cloud.point(second_tied))
              .cone_index == cone_index &&
          squared_distance(cloud, anchor, first_tied) ==
              squared_distance(cloud, anchor, second_tied),
      "the tie fixture places two equidistant targets in one half-open chamber");

  const ExactYao48RankedPairCandidateResult result =
      build_exact_yao48_ranked_pair_candidate_reference(cloud, 1U);
  const ExactYao48RankCutoffReceipt& receipt =
      result.cutoff_receipts[
          static_cast<std::size_t>(anchor) * exact_yao48_cone_count +
          cone_index];
  const PointId excluded_target =
      receipt.nearest_point_ids[0] == first_tied ? second_tied : first_tied;
  check(
      receipt.retained_nearest_point_count == 1U &&
          receipt.kth_squared_distance.has_value() &&
          !exact_yao48_directional_rank_cutoff_certifies_above(
              cloud.point(anchor),
              cloud.point(excluded_target),
              *receipt.kth_squared_distance),
      "a target excluded from top-K only by the PointId tie-break cannot be pruned");
  check(
      candidate_keys(result).contains(std::array<PointId, 2>{
          std::min(anchor, excluded_target),
          std::max(anchor, excluded_target)}) &&
          closed_rank(cloud, anchor, excluded_target) == 2U,
      "the tied exact rank-K+1 pair remains in the candidate superset");
}

void test_directional_cutoff_respects_signed_permutations() {
  constexpr std::array<std::array<std::size_t, 3>, 6> permutations{{
      {{0U, 1U, 2U}},
      {{0U, 2U, 1U}},
      {{1U, 0U, 2U}},
      {{1U, 2U, 0U}},
      {{2U, 0U, 1U}},
      {{2U, 1U, 0U}},
  }};
  constexpr std::array<double, 3> tightening_target{{1.125, 0.5, 0.5}};
  constexpr std::array<double, 3> top_k_boundary{{1.0, 0.0, 0.0}};
  const CertifiedPoint3 anchor = point(0.0, 0.0, 0.0);
  const ExactLevel cutoff{morsehgp3d::exact::BigInt{1}};
  for (const auto& permutation : permutations) {
    for (std::size_t negative_mask = 0U;
         negative_mask < 8U;
         ++negative_mask) {
      const auto transform = [&](const std::array<double, 3>& source) {
        std::array<double, 3> result{};
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
          const double sign =
              (negative_mask & (std::size_t{1} << axis)) == 0U
              ? 1.0
              : -1.0;
          result[axis] = sign * source[permutation[axis]];
        }
        return point(result[0], result[1], result[2]);
      };
      check(
          exact_yao48_directional_rank_cutoff_certifies_above(
              anchor, transform(tightening_target), cutoff),
          "the directional refinement is invariant over all 48 signed permutations");
      check(
          !exact_yao48_directional_rank_cutoff_certifies_above(
              anchor, transform(top_k_boundary), cutoff),
          "a top-K boundary ray survives over all 48 signed permutations");
    }
  }
}

void test_many_deterministic_clouds_preserve_every_low_rank_pair() {
  constexpr std::size_t point_count = 13U;
  constexpr std::size_t cloud_count = 16U;
  for (std::size_t seed = 0U; seed < cloud_count; ++seed) {
    std::vector<CertifiedPoint3> source_points;
    source_points.reserve(point_count);
    for (std::size_t index = 0U; index < point_count; ++index) {
      const auto signed_index = static_cast<long long>(index);
      const auto signed_seed = static_cast<long long>(seed);
      const double x = static_cast<double>(4LL * signed_index - signed_seed);
      const double y = static_cast<double>(
          (signed_index * signed_index + 3LL * signed_index +
           7LL * signed_seed) %
              29LL -
          14LL);
      const double z = static_cast<double>(
          (signed_index * signed_index * signed_index +
           5LL * signed_index + 11LL * signed_seed) %
              31LL -
          15LL);
      source_points.push_back(point(x, y, z));
    }
    const CanonicalPointCloud cloud = canonical_cloud(source_points);
    check(
        cloud.size() == point_count,
        "the deterministic Yao48 differential cloud is duplicate-free");
    for (std::size_t order = 1U; order <= 10U; ++order) {
      const ExactYao48RankedPairCandidateResult result =
          build_exact_yao48_ranked_pair_candidate_reference(cloud, order);
      const auto candidates = candidate_keys(result);
      check(
          result.counters.underfull_cone_count != 0U,
          "the deterministic differential exercises underfull chambers");
      for (std::size_t first_index = 0U;
           first_index < cloud.size();
           ++first_index) {
        const PointId first = static_cast<PointId>(first_index);
        for (std::size_t second_index = first_index + 1U;
             second_index < cloud.size();
             ++second_index) {
          const PointId second = static_cast<PointId>(second_index);
          const bool admitted = candidates.contains(
              std::array<PointId, 2>{first, second});
          const std::size_t rank = closed_rank(cloud, first, second);
          check(
              admitted || rank >= order + 2U,
              "a deterministic Yao48 cutoff never removes a rank-at-most-K+1 pair");
        }
      }
    }
  }
}

void test_reference_guards() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0), point(1.0, 0.0, 0.0)});
  check_throws<std::out_of_range>(
      [&] {
        (void)build_exact_yao48_ranked_pair_candidate_reference(cloud, 0U);
      },
      "ranked-pair order zero is rejected");
  check_throws<std::out_of_range>(
      [&] {
        (void)build_exact_yao48_ranked_pair_candidate_reference(cloud, 2U);
      },
      "ranked-pair order K>=n is rejected");

  std::vector<CertifiedPoint3> oversized;
  oversized.reserve(
      exact_yao48_ranked_pair_candidate_reference_max_point_count + 1U);
  for (std::size_t index = 0U;
       index <= exact_yao48_ranked_pair_candidate_reference_max_point_count;
       ++index) {
    oversized.push_back(point(static_cast<double>(index), 0.0, 0.0));
  }
  const CanonicalPointCloud oversized_cloud = canonical_cloud(oversized);
  check_throws<std::length_error>(
      [&] {
        (void)build_exact_yao48_ranked_pair_candidate_reference(
            oversized_cloud, 1U);
      },
      "ranked-pair quadratic reference cap is enforced");
}

}  // namespace

int main() {
  test_exhaustive_rank_cutoff_differential();
  test_closed_equality_is_pruned();
  test_target_inside_top_k_is_not_counted_as_a_witness();
  test_distance_tie_excluded_by_point_id_does_not_prune();
  test_directional_cutoff_respects_signed_permutations();
  test_many_deterministic_clouds_preserve_every_low_rank_pair();
  test_directional_projection_strictly_tightens_uniform_radius();
  test_fixed_prefix_counterexample_survives_adaptive_cutoff();
  test_reference_guards();

  if (failures != 0) {
    std::cerr << failures
              << " exact Yao48 ranked-pair candidate test(s) failed\n";
    return 1;
  }
  return 0;
}
