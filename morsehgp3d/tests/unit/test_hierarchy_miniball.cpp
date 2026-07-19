#include "morsehgp3d/hierarchy/miniball.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactFacetMiniballResult;
using morsehgp3d::hierarchy::ExactFacetMiniballScope;
using morsehgp3d::hierarchy::ExactFacetMiniballStatus;
using morsehgp3d::hierarchy::ExactFacetMiniballVerification;
using morsehgp3d::hierarchy::build_exact_facet_miniball;
using morsehgp3d::hierarchy::verify_exact_facet_miniball;
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
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactCenter3 center(
    std::int64_t x,
    std::int64_t y,
    std::int64_t z,
    std::int64_t denominator = 1) {
  return ExactCenter3{
      BigInt{x}, BigInt{y}, BigInt{z}, BigInt{denominator}};
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] bool all_certificates_close(
    const ExactFacetMiniballVerification& verification) {
  return verification.facet_identity_certified &&
         verification.exhaustive_support_enumeration_certified &&
         verification.exact_center_and_radius_certified &&
         verification.enclosing_partition_certified &&
         verification.canonical_support_certified &&
         verification.counters_certified &&
         verification.status_certified &&
         verification.local_scope_certified &&
         verification.fresh_replay_certified &&
         verification.local_exact_facet_miniball_certified;
}

void test_singleton_and_default_statuses() {
  const ExactFacetMiniballResult empty;
  check(
      empty.status == ExactFacetMiniballStatus::not_certified &&
          empty.scope == ExactFacetMiniballScope::unspecified,
      "default facet miniball certifies neither result nor scope");

  const std::array<CertifiedPoint3, 1> input{point(2.0, -3.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 1> facet{0U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  const ExactFacetMiniballVerification verification =
      verify_exact_facet_miniball(cloud, facet, result);

  check(
      result.facet_point_ids == std::vector<PointId>{0U} &&
          result.support_point_ids == std::vector<PointId>{0U} &&
          result.strictly_inside_point_ids.empty() &&
          result.boundary_point_ids == std::vector<PointId>{0U},
      "singleton is its own exact support and boundary");
  check(
      result.center == center(2, -3, 4) &&
          result.squared_radius == ExactLevel{},
      "singleton has its exact point as center and zero radius");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{1U, 0U, 0U, 0U} &&
          result.counters.enumerated_support_count == 1U &&
          result.counters.minimal_support_candidate_count == 1U &&
          result.counters.candidate_point_classification_count == 1U &&
          result.counters.candidate_boundary_classification_count == 1U &&
          result.counters.enclosing_support_count == 1U &&
          result.counters.optimal_support_count == 1U &&
          result.counters.selected_support_size == 1U,
      "singleton exhaustive counters close");
  check(
      result.status ==
              ExactFacetMiniballStatus::exact_facet_miniball_certified &&
          result.scope ==
              ExactFacetMiniballScope::local_facet_miniball_only &&
          all_certificates_close(verification),
      "singleton publishes only its local exact miniball certificate");
  check(
      ExactFacetMiniballResult::maximum_facet_point_count == 10U &&
          ExactFacetMiniballResult::maximum_support_point_count == 4U &&
          ExactFacetMiniballResult::maximum_enumerated_support_count == 385U &&
          std::string_view{ExactFacetMiniballResult::proof_basis} ==
              "exhaustive_exact_supports_up_to_four_facet_miniball_v1",
      "facet miniball exposes its bounded proof basis");
}

void test_triangle_support_reductions() {
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(1.0, 1.0), point(4.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{2U, 0U, 1U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.facet_point_ids == std::vector<PointId>({0U, 1U, 2U}) &&
            result.support_point_ids == std::vector<PointId>({0U, 2U}) &&
            result.strictly_inside_point_ids ==
                std::vector<PointId>({1U}) &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 2U}),
        "obtuse triangle reduces to its exact diameter pair");
    check(
        result.center == center(2, 0, 0) &&
            result.squared_radius == level(4),
        "obtuse triangle diameter has exact center and squared radius");
  }
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(0.0, 2.0), point(2.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{0U, 1U, 2U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({1U, 2U}) &&
            result.strictly_inside_point_ids.empty() &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U}) &&
            result.center == center(1, 1, 0) &&
            result.squared_radius == level(2),
        "right triangle exposes a reduced support and its extra shell point");
    check(
        result.counters.boundary_reduced_support_count >= 1U,
        "right triangle records the nonessential three-point support");
  }
  {
    const std::array<CertifiedPoint3, 3> input{
        point(0.0, 0.0), point(1.0, 3.0), point(4.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 3> facet{0U, 1U, 2U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({0U, 1U, 2U}) &&
            result.center == center(2, 1, 0) &&
            result.squared_radius == level(5) &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U}),
        "acute triangle keeps its three-point exact support");
  }
}

void test_tetrahedron_supports() {
  {
    const std::array<CertifiedPoint3, 4> input{
        point(-1.0, -1.0, 1.0),
        point(-1.0, 1.0, -1.0),
        point(1.0, -1.0, -1.0),
        point(1.0, 1.0, 1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids ==
                std::vector<PointId>({0U, 1U, 2U, 3U}) &&
            result.center == center(0, 0, 0) &&
            result.squared_radius == level(3) &&
            result.strictly_inside_point_ids.empty() &&
            result.boundary_point_ids ==
                std::vector<PointId>({0U, 1U, 2U, 3U}),
        "regular tetrahedron requires its four-point exact support");
    check(
        all_certificates_close(
            verify_exact_facet_miniball(cloud, facet, result)),
        "regular tetrahedron closes the fresh exhaustive replay");
  }
  {
    const std::array<CertifiedPoint3, 4> input{
        point(0.0, 0.0, 0.0),
        point(0.0, 0.0, 2.0),
        point(0.0, 2.0, 0.0),
        point(2.0, 0.0, 0.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
    const ExactFacetMiniballResult result =
        build_exact_facet_miniball(cloud, facet);
    check(
        result.support_point_ids == std::vector<PointId>({1U, 2U, 3U}) &&
            result.center == center(2, 2, 2, 3) &&
            result.squared_radius == level(8, 3) &&
            result.strictly_inside_point_ids ==
                std::vector<PointId>({0U}) &&
            result.boundary_point_ids ==
                std::vector<PointId>({1U, 2U, 3U}),
        "non-well-centred tetrahedron reduces to its opposite face");
    check(
        result.counters.exterior_circumcenter_support_count >= 1U,
        "non-well-centred tetrahedron records its rejected four-point center");
  }
}

void test_square_exposes_multiple_optimal_supports() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 4> facet{3U, 1U, 0U, 2U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  check(
      result.facet_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          result.support_point_ids == std::vector<PointId>({0U, 3U}) &&
          result.center == center(0, 0, 0) &&
          result.squared_radius == level(2) &&
          result.boundary_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}),
      "square chooses the smallest lexicographic diagonal without hiding shell points");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{4U, 6U, 4U, 1U} &&
          result.counters.enumerated_support_count == 15U &&
          result.counters.optimal_support_count == 2U &&
          result.counters.selected_support_size == 2U,
      "square exposes both optimal exact supports and one canonical choice");
}

void test_smaller_exterior_circumsupport_is_rejected() {
  const std::array<CertifiedPoint3, 6> input{
      point(-1.0, 0.0, -2.0),
      point(-1.0, 0.0, 2.0),
      point(1.0, -2.0, 0.0),
      point(1.0, 2.0, 0.0),
      point(2.0, -1.0, 0.0),
      point(2.0, 1.0, 0.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 6> facet{0U, 1U, 2U, 3U, 4U, 5U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);

  check(
      result.support_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U}) &&
          result.center == center(0, 0, 0) &&
          result.squared_radius == level(5) &&
          result.strictly_inside_point_ids.empty() &&
          result.boundary_point_ids ==
              std::vector<PointId>({0U, 1U, 2U, 3U, 4U, 5U}),
      "positive four-point support wins over a smaller exterior triple");
  check(
      result.counters.exterior_circumcenter_support_count >= 1U &&
          all_certificates_close(
              verify_exact_facet_miniball(cloud, facet, result)),
      "exterior circumsupport regression closes its exact replay");
}

void test_ten_point_bound_is_exactly_385_supports() {
  const std::array<CertifiedPoint3, 10> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 10> facet{
      9U, 0U, 8U, 1U, 7U, 2U, 6U, 3U, 5U, 4U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);
  check(
      result.support_point_ids == std::vector<PointId>({0U, 9U}) &&
          result.center == center(9, 0, 0, 2) &&
          result.squared_radius == level(81, 4) &&
          result.strictly_inside_point_ids ==
              std::vector<PointId>({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U}) &&
          result.boundary_point_ids == std::vector<PointId>({0U, 9U}),
      "ten collinear points select their two extreme endpoints");
  check(
      result.counters.enumerated_support_count_by_size ==
              std::array<std::size_t, 4>{10U, 45U, 120U, 210U} &&
          result.counters.enumerated_support_count == 385U &&
          result.counters.affinely_dependent_support_count == 330U &&
          result.counters.minimal_support_candidate_count == 55U &&
          result.counters.candidate_point_classification_count == 550U &&
          result.counters.candidate_strictly_inside_classification_count ==
              120U &&
          result.counters.candidate_boundary_classification_count == 100U &&
          result.counters.candidate_outside_classification_count == 330U &&
          result.counters.enclosing_support_count == 1U &&
          result.counters.optimal_support_count == 1U,
      "ten-point exhaustive enumeration closes the 385-support work bound");
  check(
      all_certificates_close(
          verify_exact_facet_miniball(cloud, facet, result)),
      "ten-point bounded oracle closes its fresh replay");
}

void test_verifier_rejects_every_result_layer() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const std::array<PointId, 4> facet{0U, 1U, 2U, 3U};
  const ExactFacetMiniballResult result =
      build_exact_facet_miniball(cloud, facet);

  ExactFacetMiniballResult bad_facet = result;
  bad_facet.facet_point_ids.pop_back();
  const auto facet_check =
      verify_exact_facet_miniball(cloud, facet, bad_facet);
  check(
      !facet_check.facet_identity_certified &&
          !facet_check.fresh_replay_certified &&
          !facet_check.local_exact_facet_miniball_certified,
      "facet identity falsification fails closed");

  ExactFacetMiniballResult bad_support = result;
  bad_support.support_point_ids = {1U, 2U};
  const auto support_check =
      verify_exact_facet_miniball(cloud, facet, bad_support);
  check(
      !support_check.canonical_support_certified &&
          !support_check.local_exact_facet_miniball_certified,
      "canonical support falsification fails closed");

  ExactFacetMiniballResult bad_center = result;
  bad_center.center = center(1, 0, 0);
  const auto center_check =
      verify_exact_facet_miniball(cloud, facet, bad_center);
  check(
      !center_check.exact_center_and_radius_certified &&
          !center_check.local_exact_facet_miniball_certified,
      "exact center falsification fails closed");

  ExactFacetMiniballResult bad_radius = result;
  bad_radius.squared_radius = level(3);
  const auto radius_check =
      verify_exact_facet_miniball(cloud, facet, bad_radius);
  check(
      !radius_check.exact_center_and_radius_certified &&
          !radius_check.local_exact_facet_miniball_certified,
      "exact radius falsification fails closed");

  ExactFacetMiniballResult bad_partition = result;
  bad_partition.strictly_inside_point_ids.push_back(0U);
  const auto partition_check =
      verify_exact_facet_miniball(cloud, facet, bad_partition);
  check(
      !partition_check.enclosing_partition_certified &&
          !partition_check.local_exact_facet_miniball_certified,
      "inside-boundary partition falsification fails closed");

  ExactFacetMiniballResult bad_counters = result;
  ++bad_counters.counters.enumerated_support_count;
  const auto counter_check =
      verify_exact_facet_miniball(cloud, facet, bad_counters);
  check(
      !counter_check.exhaustive_support_enumeration_certified &&
          !counter_check.counters_certified &&
          !counter_check.local_exact_facet_miniball_certified,
      "enumeration counter falsification fails closed");

  ExactFacetMiniballResult bad_status = result;
  bad_status.status = ExactFacetMiniballStatus::not_certified;
  const auto status_check =
      verify_exact_facet_miniball(cloud, facet, bad_status);
  check(
      !status_check.status_certified &&
          !status_check.local_exact_facet_miniball_certified,
      "local result status is treated as untrusted");

  ExactFacetMiniballResult bad_scope = result;
  bad_scope.scope = ExactFacetMiniballScope::unspecified;
  const auto scope_check =
      verify_exact_facet_miniball(cloud, facet, bad_scope);
  check(
      !scope_check.local_scope_certified &&
          !scope_check.local_exact_facet_miniball_certified,
      "local result scope is treated as untrusted");
}

void test_invalid_facets_are_rejected() {
  const std::array<CertifiedPoint3, 11> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  check_throws<std::invalid_argument>(
      [&cloud]() {
        static_cast<void>(build_exact_facet_miniball(cloud, {}));
      },
      "empty facet is rejected");
  const std::array<PointId, 2> duplicate{0U, 0U};
  check_throws<std::invalid_argument>(
      [&cloud, &duplicate]() {
        static_cast<void>(build_exact_facet_miniball(cloud, duplicate));
      },
      "duplicate PointIds are rejected");
  const std::array<PointId, 11> too_large{
      0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U};
  check_throws<std::invalid_argument>(
      [&cloud, &too_large]() {
        static_cast<void>(build_exact_facet_miniball(cloud, too_large));
      },
      "facet larger than ten points is rejected before enumeration");
  const std::array<PointId, 1> outside{11U};
  check_throws<std::out_of_range>(
      [&cloud, &outside]() {
        static_cast<void>(build_exact_facet_miniball(cloud, outside));
      },
      "PointId outside the cloud is rejected");
}

}  // namespace

int main() {
  test_singleton_and_default_statuses();
  test_triangle_support_reductions();
  test_tetrahedron_supports();
  test_square_exposes_multiple_optimal_supports();
  test_smaller_exterior_circumsupport_is_rejected();
  test_ten_point_bound_is_exactly_385_supports();
  test_verifier_rejects_every_result_layer();
  test_invalid_facets_are_rejected();

  if (failures != 0) {
    std::cerr << failures << " exact facet-miniball test(s) failed\n";
    return 1;
  }
  std::cout << "Exact facet-miniball tests passed\n";
  return 0;
}
