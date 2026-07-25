#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationBudget;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationResult;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationStatus;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::classify_exact_anchored_pair_candidate;
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
    std::forward<Function>(function)();
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

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    std::initializer_list<CertifiedPoint3> points) {
  const std::vector<CertifiedPoint3> storage{points};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{storage});
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

[[nodiscard]] ExactRational phi(
    const CanonicalPointCloud& cloud,
    PointId query_id,
    const std::array<PointId, 2>& support_ids) {
  ExactRational result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result = result +
        (cloud.point(query_id).coordinate(axis) -
         cloud.point(support_ids[0]).coordinate(axis)) *
            (cloud.point(query_id).coordinate(axis) -
             cloud.point(support_ids[1]).coordinate(axis));
  }
  return result;
}

[[nodiscard]] ExactAnchoredPairCandidateClassificationResult
brute_classify(
    const CanonicalPointCloud& cloud,
    std::array<PointId, 2> support_ids,
    std::size_t maximum_closed_rank) {
  if (support_ids[1] < support_ids[0]) {
    std::swap(support_ids[0], support_ids[1]);
  }
  ExactAnchoredPairCandidateClassificationResult result;
  result.support_ids = support_ids;
  result.maximum_closed_rank = maximum_closed_rank;
  std::vector<PointId> interior_ids;
  std::size_t shell_count = 0U;
  std::size_t exterior_count = 0U;
  std::optional<PointId> canonical_extra_shell_witness_id;
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    const int sign = phi(cloud, point_id, support_ids).sign();
    if (sign < 0) {
      interior_ids.push_back(point_id);
    } else if (sign == 0) {
      ++shell_count;
      if (point_id != support_ids[0] && point_id != support_ids[1] &&
          (!canonical_extra_shell_witness_id.has_value() ||
           point_id < *canonical_extra_shell_witness_id)) {
        canonical_extra_shell_witness_id = point_id;
      }
    } else {
      ++exterior_count;
    }
  }
  if (interior_ids.size() > maximum_closed_rank - 2U) {
    result.status =
        ExactAnchoredPairCandidateClassificationStatus::above_rank;
    return result;
  }

  const morsehgp3d::exact::CircumcenterResult sphere =
      morsehgp3d::exact::circumcenter(
          cloud.point(support_ids[0]), cloud.point(support_ids[1]));
  const std::size_t observed_closed_rank =
      interior_ids.size() + shell_count;
  if (shell_count == 2U) {
    result.event = ExactPairSupportEvent{
        support_ids,
        *sphere.center(),
        *sphere.squared_level(),
        std::move(interior_ids),
        observed_closed_rank,
        exterior_count};
  } else {
    result.relevant_extra_shell_diagnostic =
        ExactPairSupportExtraShellDiagnostic{
            support_ids,
            *sphere.center(),
            *sphere.squared_level(),
            std::move(interior_ids),
            shell_count,
            *canonical_extra_shell_witness_id,
            observed_closed_rank - shell_count + 2U,
            observed_closed_rank,
            exterior_count};
  }
  result.status =
      ExactAnchoredPairCandidateClassificationStatus::complete;
  return result;
}

[[nodiscard]] bool same_scientific_result(
    const ExactAnchoredPairCandidateClassificationResult& left,
    const ExactAnchoredPairCandidateClassificationResult& right) {
  return left.status == right.status &&
      left.support_ids == right.support_ids &&
      left.maximum_closed_rank == right.maximum_closed_rank &&
      left.event == right.event &&
      left.relevant_extra_shell_diagnostic ==
          right.relevant_extra_shell_diagnostic;
}

void check_filter_audit(
    const ExactAnchoredPairCandidateClassificationResult& result,
    const std::string& message) {
  const auto& audit = result.audit;
  check(
      audit.node_visit_count ==
          audit.interval_exterior_node_count +
              audit.interval_interior_node_count +
              audit.exact_node_fallback_count,
      message + ": interval and exact paths must partition node visits");
  check(
      audit.exact_minimum_phi_aabb_bound_count ==
          audit.exact_node_fallback_count,
      message + ": every fallback must evaluate one exact minimum");
  check(
      audit.exact_maximum_phi_aabb_bound_count <=
          audit.exact_node_fallback_count,
      message + ": an exact positive minimum may skip the maximum");
}

void test_small_differential() {
  const CanonicalPointCloud cloud = cloud_from({
      point(-4.0, 0.0, 0.0),
      point(-1.0, 2.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(1.0, -2.0, 1.0),
      point(2.0, 1.0, -1.0),
      point(5.0, 0.0, 2.0),
  });
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  const ExactAnchoredPairCandidateClassificationBudget unlimited{
      std::numeric_limits<std::size_t>::max()};
  for (std::size_t first = 0U; first < cloud.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < cloud.size();
         ++second) {
      for (std::size_t maximum_closed_rank = 2U;
           maximum_closed_rank <= 6U;
           ++maximum_closed_rank) {
        const std::array<PointId, 2> support_ids{
            static_cast<PointId>(second),
            static_cast<PointId>(first)};
        const auto observed = classify_exact_anchored_pair_candidate(
            lbvh,
            cloud,
            support_ids,
            maximum_closed_rank,
            unlimited);
        const auto expected = brute_classify(
            cloud, support_ids, maximum_closed_rank);
        check(
            same_scientific_result(observed, expected),
            "small exact LBVH classification must match brute force");
        check(
            !observed.audit.center_and_level_constructed ||
                observed.complete(),
            "center construction must be reserved for complete output");
        check(
            observed.status !=
                    ExactAnchoredPairCandidateClassificationStatus::
                        budget_exhausted,
            "an unlimited small classification must not exhaust");
        check_filter_audit(observed, "small differential audit");
      }
    }
  }
}

void test_complete_cosphere_shell() {
  const CanonicalPointCloud cloud = cloud_from({
      point(-1.0, 0.0),
      point(1.0, 0.0),
      point(0.0, 1.0),
      point(0.0, -1.0),
      point(0.0, 0.0),
      point(2.0, 0.0),
  });
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  const PointId first_support_id = canonical_id_from_source(cloud, 0U);
  const PointId second_support_id = canonical_id_from_source(cloud, 1U);
  const PointId first_extra_shell_id =
      canonical_id_from_source(cloud, 2U);
  const PointId second_extra_shell_id =
      canonical_id_from_source(cloud, 3U);
  const PointId interior_id = canonical_id_from_source(cloud, 4U);
  const auto result = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {first_support_id, second_support_id},
      3U,
      {std::numeric_limits<std::size_t>::max()});
  check(result.complete(), "a relevant cospherical pair must complete");
  check(!result.event.has_value(), "an extra shell is not a regular event");
  check(
      result.relevant_extra_shell_diagnostic.has_value(),
      "a complete cosphere must produce its sparse diagnostic");
  if (result.relevant_extra_shell_diagnostic.has_value()) {
    const auto& diagnostic =
        *result.relevant_extra_shell_diagnostic;
    check(diagnostic.shell_count == 4U, "the whole equality shell is counted");
    check(
        diagnostic.canonical_extra_shell_witness_id ==
            std::min(first_extra_shell_id, second_extra_shell_id),
        "only the canonical least extra-shell witness is retained");
    check(
        diagnostic.interior_ids == std::vector<PointId>{interior_id},
        "the sparse cosphere diagnostic retains its bounded interior");
    check(diagnostic.exterior_count == 1U, "the exterior is counted only");
    check(
        diagnostic.minimum_possible_closed_rank == 3U &&
            diagnostic.observed_closed_rank == 5U,
        "the cosphere exposes minimum and observed closed ranks");
  }
  check(
      result.audit.classified_point_count == cloud.size() &&
          result.audit.complete_partition_certified,
      "the cosphere equality decision requires a complete partition");
  check(
      result.audit.exact_node_fallback_count > 0U,
      "a cospherical equality must remain on the exact fail-open path");
  check_filter_audit(result, "cospherical equality audit");
}

void test_interval_filter_strict_decisions() {
  const CanonicalPointCloud cloud = cloud_from({
      point(-1.0),
      point(1.0),
      point(0.0),
      point(10.0),
  });
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  const PointId first_support_id = canonical_id_from_source(cloud, 0U);
  const PointId second_support_id = canonical_id_from_source(cloud, 1U);
  const auto result = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {first_support_id, second_support_id},
      3U,
      {std::numeric_limits<std::size_t>::max()});
  check(
      result.complete() && result.event.has_value(),
      "strict interval decisions must preserve the exact pair event");
  check(
      result.audit.fp64_interval_filter_enabled,
      "strict classifier tests require the guarded FP64 environment");
  check(
      result.audit.interval_interior_node_count > 0U,
      "a strict negative interval must classify an interior subtree");
  check(
      result.audit.interval_exterior_node_count > 0U,
      "a strict positive interval must classify an exterior subtree");
  check(
      result.audit.exact_node_fallback_count > 0U,
      "support-shell equality must still force exact fallback");
  check_filter_audit(result, "strict interval decision audit");
}

void test_centered_interval_resolves_natural_dependency() {
  {
    morsehgp3d::exact::detail::Fp64EnvironmentGuard environment;
    check(
        environment.supported(),
        "the centered-interval fixture requires guarded FP64");
    const std::array<double, 3> first{-10.0, 0.0, 0.0};
    const std::array<double, 3> second{10.0, 0.0, 0.0};
    const std::array<double, 3> lower{1.0, 10.0, 0.0};
    const std::array<double, 3> upper{2.0, 10.0, 0.0};
    morsehgp3d::exact::detail::Binary64Interval natural =
        morsehgp3d::exact::detail::point_binary64_interval(0.0);
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const morsehgp3d::exact::detail::Binary64Interval query{
          lower[axis], upper[axis], true};
      natural = morsehgp3d::exact::detail::add_binary64_intervals(
          natural,
          morsehgp3d::exact::detail::multiply_binary64_intervals(
              morsehgp3d::exact::detail::subtract_binary64_intervals(
                  query,
                  morsehgp3d::exact::detail::point_binary64_interval(
                      first[axis])),
              morsehgp3d::exact::detail::subtract_binary64_intervals(
                  query,
                  morsehgp3d::exact::detail::point_binary64_interval(
                      second[axis]))));
    }
    check(
        !morsehgp3d::exact::detail::sign_of_binary64_interval(natural)
             .sign()
             .has_value(),
        "the natural product extension must contain zero on the fixture");
    check(
        environment.restore(),
        "the natural-extension fixture must restore the FP environment");
  }

  const CanonicalPointCloud cloud = cloud_from({
      point(-10.0, 0.0, 0.0),
      point(10.0, 0.0, 0.0),
      point(1.0, 10.0, 0.0),
      point(2.0, 10.0, 0.0),
  });
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  const PointId first_support_id = canonical_id_from_source(cloud, 0U);
  const PointId second_support_id = canonical_id_from_source(cloud, 1U);
  const auto result = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {first_support_id, second_support_id},
      2U,
      {std::numeric_limits<std::size_t>::max()});
  check(
      result.complete() && result.event.has_value() &&
          result.event->exterior_count == 2U,
      "the centered form must preserve the exact rank-two event");
  check(
      result.audit.interval_exterior_node_count == 1U &&
          result.audit.bulk_exterior_subtree_count == 1U &&
          result.audit.bulk_exterior_point_count == 2U,
      "the centered form must certify the two-point exterior AABB at once");
  check_filter_audit(result, "centered dependency fixture audit");
}

void test_node_budget_and_early_rank_rejection() {
  const CanonicalPointCloud cloud = cloud_from({
      point(-10.0),
      point(10.0),
      point(-4.0),
      point(-2.0),
      point(0.0),
      point(2.0),
      point(4.0),
      point(20.0),
  });
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  const PointId first_support_id = canonical_id_from_source(cloud, 0U);
  const PointId second_support_id = canonical_id_from_source(cloud, 1U);
  const auto exhausted = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {first_support_id, second_support_id},
      3U,
      {0U});
  check(
      exhausted.status ==
          ExactAnchoredPairCandidateClassificationStatus::budget_exhausted &&
          exhausted.audit.node_visit_count == 0U,
      "a zero node budget must exhaust before its first physical visit");
  check(
      !exhausted.event.has_value() &&
          !exhausted.relevant_extra_shell_diagnostic.has_value() &&
          !exhausted.audit.center_and_level_constructed,
      "budget exhaustion must expose no scientific prefix or sphere");
  check_filter_audit(exhausted, "zero-budget audit");

  const auto one_visit = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {first_support_id, second_support_id},
      3U,
      {1U});
  check(
      one_visit.status ==
              ExactAnchoredPairCandidateClassificationStatus::
                  budget_exhausted &&
          one_visit.audit.node_visit_count == 1U,
      "the visit cap must charge exactly one physical root visit");
  check(
      !one_visit.event.has_value() &&
          !one_visit.relevant_extra_shell_diagnostic.has_value(),
      "a partial traversal must not publish a scientific prefix");
  check_filter_audit(one_visit, "one-visit budget audit");

  const auto rejected = classify_exact_anchored_pair_candidate(
      lbvh,
      cloud,
      {second_support_id, first_support_id},
      3U,
      {std::numeric_limits<std::size_t>::max()});
  check(rejected.above_rank(), "two strict interiors must reject rank three");
  check(
      rejected.audit.early_above_rank_certificate,
      "above-rank must carry its strict-interior certificate audit");
  check(
      !rejected.event.has_value() &&
          !rejected.relevant_extra_shell_diagnostic.has_value() &&
          !rejected.audit.center_and_level_constructed,
      "above-rank must not construct a sphere or output record");
  check_filter_audit(rejected, "early rank audit");
}

void test_k10_exact_interior_boundary() {
  const CanonicalPointCloud nine_interior_cloud = cloud_from({
      point(-100.0),
      point(100.0),
      point(-9.0),
      point(-7.0),
      point(-5.0),
      point(-3.0),
      point(-1.0),
      point(1.0),
      point(3.0),
      point(5.0),
      point(7.0),
  });
  const MortonLbvhIndex nine_interior_lbvh =
      MortonLbvhIndex::build(nine_interior_cloud);
  const PointId first_nine_support =
      canonical_id_from_source(nine_interior_cloud, 0U);
  const PointId second_nine_support =
      canonical_id_from_source(nine_interior_cloud, 1U);
  const auto accepted = classify_exact_anchored_pair_candidate(
      nine_interior_lbvh,
      nine_interior_cloud,
      {first_nine_support, second_nine_support},
      11U,
      {std::numeric_limits<std::size_t>::max()});
  check(
      accepted.complete() && accepted.event.has_value(),
      "K=10 must retain a pair with exactly nine strict interiors");
  if (accepted.event.has_value()) {
    check(
        accepted.event->interior_ids.size() == 9U &&
            accepted.event->closed_rank == 11U,
        "the K=10 boundary event must close at rank eleven");
  }
  check_filter_audit(accepted, "K=10 accepted-boundary audit");

  const CanonicalPointCloud ten_interior_cloud = cloud_from({
      point(-100.0),
      point(100.0),
      point(-9.0),
      point(-7.0),
      point(-5.0),
      point(-3.0),
      point(-1.0),
      point(1.0),
      point(3.0),
      point(5.0),
      point(7.0),
      point(9.0),
  });
  const MortonLbvhIndex ten_interior_lbvh =
      MortonLbvhIndex::build(ten_interior_cloud);
  const PointId first_ten_support =
      canonical_id_from_source(ten_interior_cloud, 0U);
  const PointId second_ten_support =
      canonical_id_from_source(ten_interior_cloud, 1U);
  const auto rejected = classify_exact_anchored_pair_candidate(
      ten_interior_lbvh,
      ten_interior_cloud,
      {first_ten_support, second_ten_support},
      11U,
      {std::numeric_limits<std::size_t>::max()});
  check(
      rejected.above_rank() &&
          rejected.audit.early_above_rank_certificate,
      "K=10 must reject a pair as soon as ten strict interiors are certified");
  check(
      !rejected.event.has_value() &&
          !rejected.relevant_extra_shell_diagnostic.has_value() &&
          !rejected.audit.center_and_level_constructed,
      "the tenth K=10 interior must reject before sphere construction");
  check_filter_audit(rejected, "K=10 rejected-boundary audit");
}

void test_contract_rejections() {
  const CanonicalPointCloud cloud = cloud_from({point(0.0), point(1.0)});
  const MortonLbvhIndex lbvh = MortonLbvhIndex::build(cloud);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(classify_exact_anchored_pair_candidate(
            lbvh, cloud, {0U, 0U}, 2U, {1U}));
      },
      "a repeated support id must be rejected");
  check_throws<std::out_of_range>(
      [&] {
        static_cast<void>(classify_exact_anchored_pair_candidate(
            lbvh, cloud, {0U, 1U}, 12U, {1U}));
      },
      "a rank beyond the pair contract must be rejected");
}

}  // namespace

int main() {
  test_small_differential();
  test_complete_cosphere_shell();
  test_interval_filter_strict_decisions();
  test_centered_interval_resolves_natural_dependency();
  test_node_budget_and_early_rank_rejection();
  test_k10_exact_interior_boundary();
  test_contract_rejections();
  if (failures != 0) {
    std::cerr << failures << " anchored pair classifier test(s) failed\n";
    return 1;
  }
  std::cout << "anchored pair candidate classifier tests passed\n";
  return 0;
}
