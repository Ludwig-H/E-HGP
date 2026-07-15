#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <xmmintrin.h>
#endif

namespace {

using morsehgp3d::exact::BarycentricCoordinates;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertificationStage;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterKind;
using morsehgp3d::exact::CircumcenterSupportAnalysis;
using morsehgp3d::exact::CircumcenterSupportStatus;
using morsehgp3d::exact::ConvexHullLocation;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateFilterPolicy;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::SpherePointLocation;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::barycentric_coordinates;
using morsehgp3d::exact::classify_sphere_point;

int failures = 0;

class ScopedFloatingEnvironment {
 public:
  ScopedFloatingEnvironment() noexcept
      : saved_(std::fegetenv(&environment_) == 0) {
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    mxcsr_ = _mm_getcsr();
#endif
  }

  ScopedFloatingEnvironment(const ScopedFloatingEnvironment&) = delete;
  ScopedFloatingEnvironment& operator=(const ScopedFloatingEnvironment&) =
      delete;

  ~ScopedFloatingEnvironment() {
    if (saved_) {
      static_cast<void>(std::fesetenv(&environment_));
    }
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    _mm_setcsr(mxcsr_);
#endif
  }

  [[nodiscard]] bool saved() const noexcept { return saved_; }

 private:
  std::fenv_t environment_{};
  bool saved_;
#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  unsigned int mxcsr_{};
#endif
};

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

ExactRational3 point(long long x, long long y, long long z) {
  return ExactRational3{BigInt{x}, BigInt{y}, BigInt{z}, BigInt{1}};
}

CertifiedPoint3 binary_point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

ExactRational3 rational_point(
    ExactRational x, ExactRational y, ExactRational z) {
  return ExactRational3{std::array<ExactRational, 3>{
      std::move(x), std::move(y), std::move(z)}};
}

void check_barycentric(
    const BarycentricCoordinates& result,
    ConvexHullLocation expected_location,
    const std::array<ExactRational, 4>& expected,
    std::size_t support_size,
    const std::string& message) {
  check(result.support_size() == support_size, message + " has the expected size");
  check(result.location() == expected_location, message + " has the expected location");
  for (std::size_t index = 0; index < support_size; ++index) {
    check(
        result.coordinate(index) == expected[index],
        message + " has exact coordinate " + std::to_string(index));
    check(
        result.sign(index) ==
            morsehgp3d::exact::predicate_sign(expected[index].sign()),
        message + " has exact sign " + std::to_string(index));
  }
}

void check_analysis_witnesses(
    const CircumcenterSupportAnalysis& analysis,
    std::size_t support_size,
    const ExactRational3& center,
    const ExactLevel& level,
    const std::string& message) {
  const auto& result = analysis.circumcenter_result();
  check(
      result.kind() == CircumcenterKind::unique &&
          result.support_size() == support_size && result.center().has_value() &&
          result.squared_level().has_value(),
      message + " has structurally complete circumcenter witnesses");
  if (result.center().has_value()) {
    check(*result.center() == center, message + " has the expected center");
  }
  if (result.squared_level().has_value()) {
    check(*result.squared_level() == level, message + " has the expected level");
  }
}

void check_terminal_certifications(
    const PredicateCounters& counters,
    CertificationStage expected_stage,
    std::uint64_t expected_count,
    std::uint64_t expected_zeros,
    const std::string& message) {
  check(
      counters.certified_decisions() == expected_count &&
          counters.fp64_filtered_certified() ==
              (expected_stage == CertificationStage::fp64_filtered
                   ? expected_count
                   : 0U) &&
          counters.expansion_certified() ==
              (expected_stage == CertificationStage::expansion
                   ? expected_count
                   : 0U) &&
          counters.cpu_multiprecision_certified() ==
              (expected_stage == CertificationStage::cpu_multiprecision
                   ? expected_count
                   : 0U) &&
          counters.exact_zeros() == expected_zeros &&
          counters.remaining_unknown() == 0U,
      message);
}

void check_same_sphere_witness(
    const morsehgp3d::exact::SpherePointClassification& left,
    const morsehgp3d::exact::SpherePointClassification& right,
    const std::string& message) {
  check(
      left.decision().sign() == right.decision().sign() &&
          left.point_squared_distance() == right.point_squared_distance() &&
          left.signed_power() == right.signed_power() &&
          left.location() == right.location(),
      message);
}

void test_singleton_and_pair_barycentrics() {
  PredicateCounters singleton_counters;
  const std::array<ExactRational3, 1> singleton{point(3, -2, 5)};
  const BarycentricCoordinates singleton_barycentric =
      barycentric_coordinates(singleton[0], singleton, &singleton_counters);
  check_barycentric(
      singleton_barycentric,
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}}, ExactRational{}, ExactRational{}, ExactRational{}},
      1U,
      "singleton barycentric witness");
  check(
      singleton_counters.cpu_multiprecision_certified() == 1U &&
          singleton_counters.exact_zeros() == 0U,
      "singleton barycentric certification records one positive decision");
  check_throws<std::invalid_argument>(
      [&singleton] {
        static_cast<void>(barycentric_coordinates(point(3, -2, 6), singleton));
      },
      "a point outside a singleton affine hull is rejected");

  const std::array<ExactRational3, 2> pair{
      point(0, 0, 0), point(2, 0, 0)};
  check_barycentric(
      barycentric_coordinates(point(1, 0, 0), pair),
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}, BigInt{2}},
       ExactRational{BigInt{1}, BigInt{2}},
       ExactRational{},
       ExactRational{}},
      2U,
      "pair midpoint");
  check_barycentric(
      barycentric_coordinates(pair[0], pair),
      ConvexHullLocation::relative_boundary,
      {ExactRational{BigInt{1}}, ExactRational{}, ExactRational{}, ExactRational{}},
      2U,
      "pair endpoint");
  check_barycentric(
      barycentric_coordinates(point(-1, 0, 0), pair),
      ConvexHullLocation::exterior,
      {ExactRational{BigInt{3}, BigInt{2}},
       ExactRational{BigInt{-1}, BigInt{2}},
       ExactRational{},
       ExactRational{}},
      2U,
      "point outside a segment");
  check_throws<std::invalid_argument>(
      [&pair] {
        static_cast<void>(barycentric_coordinates(point(1, 1, 0), pair));
      },
      "a point outside a pair affine hull is not silently projected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(barycentric_coordinates(
            point(0, 0, 0),
            std::array<ExactRational3, 2>{point(0, 0, 0), point(0, 0, 0)}));
      },
      "a dependent pair has no unique barycentric coordinates");
}

void test_adaptive_binary_barycentrics() {
  const std::array<CertifiedPoint3, 1> singleton{
      binary_point(2.0, -1.0, 0.5)};
  PredicateCounters singleton_counters;
  const BarycentricCoordinates singleton_coordinates = barycentric_coordinates(
      singleton[0], singleton, &singleton_counters);
  check_barycentric(
      singleton_coordinates,
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}}, ExactRational{}, ExactRational{}, ExactRational{}},
      1U,
      "adaptive singleton query");
  check(
      singleton_coordinates.certification_stage() ==
          CertificationStage::fp64_filtered,
      "a well-separated singleton barycentric sign uses FP64");
  check_terminal_certifications(
      singleton_counters,
      CertificationStage::fp64_filtered,
      1U,
      0U,
      "a singleton query records one FP64 terminal decision");

  const std::array<CertifiedPoint3, 2> pair{
      binary_point(0.0, 0.0, 0.0), binary_point(2.0, 0.0, 0.0)};
  PredicateCounters pair_counters;
  const BarycentricCoordinates pair_midpoint = barycentric_coordinates(
      binary_point(1.0, 0.0, 0.0), pair, &pair_counters);
  check_barycentric(
      pair_midpoint,
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}, BigInt{2}},
       ExactRational{BigInt{1}, BigInt{2}},
       ExactRational{},
       ExactRational{}},
      2U,
      "adaptive pair midpoint");
  check(
      pair_midpoint.certification_stage() ==
          CertificationStage::fp64_filtered,
      "a well-separated pair query uses FP64 collectively");
  check_terminal_certifications(
      pair_counters,
      CertificationStage::fp64_filtered,
      2U,
      0U,
      "a pair query records exactly two FP64 decisions");

  const std::array<CertifiedPoint3, 3> triangle{
      binary_point(0.0, 0.0, 0.0),
      binary_point(2.0, 0.0, 0.0),
      binary_point(0.0, 2.0, 0.0)};
  const CertifiedPoint3 triangle_query = binary_point(0.5, 0.5, 0.0);
  PredicateCounters triangle_counters;
  const BarycentricCoordinates triangle_coordinates = barycentric_coordinates(
      triangle_query, triangle, &triangle_counters);
  check_barycentric(
      triangle_coordinates,
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}, BigInt{2}},
       ExactRational{BigInt{1}, BigInt{4}},
       ExactRational{BigInt{1}, BigInt{4}},
       ExactRational{}},
      3U,
      "adaptive triangle query");
  check(
      triangle_coordinates.certification_stage() ==
          CertificationStage::fp64_filtered,
      "a well-separated triangle query uses FP64 collectively");
  check_terminal_certifications(
      triangle_counters,
      CertificationStage::fp64_filtered,
      3U,
      0U,
      "a triangle query records exactly three FP64 decisions");

  const std::array<CertifiedPoint3, 4> tetrahedron{
      binary_point(0.0, 0.0, 0.0),
      binary_point(1.0, 0.0, 0.0),
      binary_point(0.0, 1.0, 0.0),
      binary_point(0.0, 0.0, 1.0)};
  PredicateCounters tetrahedron_counters;
  const BarycentricCoordinates tetrahedron_coordinates =
      barycentric_coordinates(
          binary_point(0.25, 0.25, 0.25),
          tetrahedron,
          &tetrahedron_counters);
  check_barycentric(
      tetrahedron_coordinates,
      ConvexHullLocation::relative_interior,
      {ExactRational{BigInt{1}, BigInt{4}},
       ExactRational{BigInt{1}, BigInt{4}},
       ExactRational{BigInt{1}, BigInt{4}},
       ExactRational{BigInt{1}, BigInt{4}}},
      4U,
      "adaptive tetrahedron query");
  check(
      tetrahedron_coordinates.certification_stage() ==
          CertificationStage::fp64_filtered,
      "a well-separated tetrahedron query uses FP64 collectively");
  check_terminal_certifications(
      tetrahedron_counters,
      CertificationStage::fp64_filtered,
      4U,
      0U,
      "a tetrahedron query records exactly four FP64 decisions");

  PredicateCounters zero_counters;
  const BarycentricCoordinates endpoint = barycentric_coordinates(
      pair[0], pair, &zero_counters, PredicateFilterPolicy::allow_adaptive);
  check(
      endpoint.certification_stage() == CertificationStage::expansion &&
          endpoint.sign(0U) == PredicateSign::positive &&
          endpoint.sign(1U) == PredicateSign::zero,
      "an exact zero forces the collective pair witness to expansion");
  check_terminal_certifications(
      zero_counters,
      CertificationStage::expansion,
      2U,
      1U,
      "an endpoint records two expansion decisions including one zero");

  PredicateCounters fp64_only_zero_counters;
  const BarycentricCoordinates fp64_only_endpoint = barycentric_coordinates(
      pair[0],
      pair,
      &fp64_only_zero_counters,
      PredicateFilterPolicy::allow_fp64);
  check(
      fp64_only_endpoint == endpoint &&
          fp64_only_endpoint.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "the legacy FP64-only policy falls back to MP on an exact zero");
  check_terminal_certifications(
      fp64_only_zero_counters,
      CertificationStage::cpu_multiprecision,
      2U,
      1U,
      "the FP64-only zero case records one MP terminal per coordinate");

  PredicateCounters multiprecision_counters;
  const BarycentricCoordinates multiprecision_triangle =
      barycentric_coordinates(
          triangle_query,
          triangle,
          &multiprecision_counters,
          PredicateFilterPolicy::multiprecision_only);
  check(
      multiprecision_triangle == triangle_coordinates &&
          multiprecision_triangle.certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "disabling fast stages preserves the complete triangle witness");
  check_terminal_certifications(
      multiprecision_counters,
      CertificationStage::cpu_multiprecision,
      3U,
      0U,
      "MP-only triangle signs have exactly three terminal decisions");

  PredicateCounters exterior_counters;
  const BarycentricCoordinates exterior = barycentric_coordinates(
      binary_point(-1.0, 0.0, 0.0), pair, &exterior_counters);
  check_barycentric(
      exterior,
      ConvexHullLocation::exterior,
      {ExactRational{BigInt{3}, BigInt{2}},
       ExactRational{BigInt{-1}, BigInt{2}},
       ExactRational{},
       ExactRational{}},
      2U,
      "adaptive exterior pair query");
  check(
      exterior.certification_stage() == CertificationStage::fp64_filtered,
      "positive and negative exterior signs are both certified by FP64");
  check_terminal_certifications(
      exterior_counters,
      CertificationStage::fp64_filtered,
      2U,
      0U,
      "the exterior query records exactly two terminal signs");

  const std::array<ExactRational, 3> original_weights{
      ExactRational{BigInt{1}, BigInt{2}},
      ExactRational{BigInt{1}, BigInt{4}},
      ExactRational{BigInt{1}, BigInt{4}}};
  std::array<std::size_t, 3> permutation{0U, 1U, 2U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<CertifiedPoint3, 3> permuted{
        triangle[permutation[0]],
        triangle[permutation[1]],
        triangle[permutation[2]]};
    PredicateCounters counters;
    const BarycentricCoordinates coordinates =
        barycentric_coordinates(triangle_query, permuted, &counters);
    check(
        coordinates.certification_stage() ==
            CertificationStage::fp64_filtered,
        "triangle support permutation preserves the FP64 collective stage");
    for (std::size_t index = 0; index < permutation.size(); ++index) {
      check(
          coordinates.coordinate(index) == original_weights[permutation[index]],
          "canonical filtering remaps a barycentric coordinate to its original position");
    }
    check_terminal_certifications(
        counters,
        CertificationStage::fp64_filtered,
        3U,
        0U,
        "each permuted triangle records three terminal decisions");
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 6U, "all adaptive triangle remappings were tested");

  check_throws<std::invalid_argument>(
      [&pair] {
        static_cast<void>(barycentric_coordinates(
            binary_point(1.0, 1.0, 0.0), pair));
      },
      "a binary64 query outside the affine hull is rejected");
  check_throws<std::invalid_argument>(
      [] {
        const std::array<CertifiedPoint3, 2> duplicate{
            binary_point(0.0, 0.0, 0.0),
            binary_point(0.0, 0.0, 0.0)};
        static_cast<void>(barycentric_coordinates(duplicate[0], duplicate));
      },
      "a dependent binary64 support has no adaptive barycentric witness");
}

void test_triangle_support_analysis() {
  const std::array<ExactRational3, 3> acute{
      point(0, 0, 0), point(4, 0, 0), point(1, 3, 0)};
  PredicateCounters acute_counters;
  const CircumcenterSupportAnalysis acute_analysis =
      analyze_circumcenter_support(acute, &acute_counters);
  check_analysis_witnesses(
      acute_analysis,
      3U,
      point(2, 1, 0),
      ExactLevel{BigInt{5}},
      "acute triangle");
  check(
      acute_analysis.status() == CircumcenterSupportStatus::minimal &&
          acute_analysis.reduced_support_size() == 3U,
      "an acute triangle is already a minimal support");
  check(acute_analysis.barycentric().has_value(), "acute triangle has barycentrics");
  if (acute_analysis.barycentric().has_value()) {
    check_barycentric(
        *acute_analysis.barycentric(),
        ConvexHullLocation::relative_interior,
        {ExactRational{BigInt{1}, BigInt{4}},
         ExactRational{BigInt{5}, BigInt{12}},
         ExactRational{BigInt{1}, BigInt{3}},
         ExactRational{}},
        3U,
        "acute triangle circumcenter");
  }
  check(
      acute_counters.cpu_multiprecision_certified() == 3U &&
          acute_counters.exact_zeros() == 0U,
      "acute triangle records its three positive barycentric signs");

  const std::array<ExactRational3, 3> right{
      point(0, 0, 0), point(2, 0, 0), point(0, 2, 0)};
  PredicateCounters right_counters;
  const CircumcenterSupportAnalysis right_analysis =
      analyze_circumcenter_support(right, &right_counters);
  check_analysis_witnesses(
      right_analysis,
      3U,
      point(1, 1, 0),
      ExactLevel{BigInt{2}},
      "right triangle");
  check(
      right_analysis.status() == CircumcenterSupportStatus::boundary_reduced &&
          right_analysis.reduced_support_size() == 2U &&
          !right_analysis.reduced_support_contains(0U) &&
          right_analysis.reduced_support_contains(1U) &&
          right_analysis.reduced_support_contains(2U),
      "a right triangle reduces exactly to its hypotenuse");
  check(
      right_counters.cpu_multiprecision_certified() == 3U &&
          right_counters.exact_zeros() == 1U,
      "right triangle records one exact zero barycentric sign");

  const std::array<ExactRational3, 3> obtuse{
      point(0, 0, 0), point(4, 0, 0), point(1, 1, 0)};
  const CircumcenterSupportAnalysis obtuse_analysis =
      analyze_circumcenter_support(obtuse);
  check_analysis_witnesses(
      obtuse_analysis,
      3U,
      point(2, -1, 0),
      ExactLevel{BigInt{5}},
      "obtuse triangle");
  check(
      obtuse_analysis.status() ==
              CircumcenterSupportStatus::exterior_circumcenter &&
          !obtuse_analysis.reduced_support_mask().has_value(),
      "an obtuse triangle does not invent a miniball reduction");
  if (obtuse_analysis.barycentric().has_value()) {
    check_barycentric(
        *obtuse_analysis.barycentric(),
        ConvexHullLocation::exterior,
        {ExactRational{BigInt{5}, BigInt{4}},
         ExactRational{BigInt{3}, BigInt{4}},
         ExactRational{BigInt{-1}},
         ExactRational{}},
        3U,
        "obtuse triangle circumcenter");
  }

  PredicateCounters dependent_counters;
  const CircumcenterSupportAnalysis dependent = analyze_circumcenter_support(
      std::array<ExactRational3, 3>{
          point(0, 0, 0), point(1, 0, 0), point(2, 0, 0)},
      &dependent_counters);
  check(
      dependent.status() == CircumcenterSupportStatus::affinely_dependent &&
          dependent.circumcenter_result().affine_dimension() == 1U &&
          !dependent.barycentric().has_value() &&
          !dependent.reduced_support_mask().has_value(),
      "a collinear triangle exposes no invented support witnesses");
  check(
      dependent_counters.certified_decisions() == 0U,
      "a dependent support emits no nonexistent barycentric decisions");
}

void test_tetrahedron_support_analysis() {
  const std::array<ExactRational3, 4> regular{
      point(1, 1, 1),
      point(1, -1, -1),
      point(-1, 1, -1),
      point(-1, -1, 1)};
  const CircumcenterSupportAnalysis regular_analysis =
      analyze_circumcenter_support(regular);
  check_analysis_witnesses(
      regular_analysis,
      4U,
      point(0, 0, 0),
      ExactLevel{BigInt{3}},
      "regular tetrahedron");
  check(
      regular_analysis.status() == CircumcenterSupportStatus::minimal &&
          regular_analysis.reduced_support_size() == 4U,
      "a regular tetrahedron is a minimal support");

  const std::array<ExactRational3, 4> boundary_edge{
      point(1, 0, 0),
      point(-1, 0, 0),
      point(0, 1, 0),
      point(0, 0, 1)};
  const CircumcenterSupportAnalysis edge_analysis =
      analyze_circumcenter_support(boundary_edge);
  check_analysis_witnesses(
      edge_analysis,
      4U,
      point(0, 0, 0),
      ExactLevel{BigInt{1}},
      "edge-boundary tetrahedron");
  check(
      edge_analysis.status() == CircumcenterSupportStatus::boundary_reduced &&
          edge_analysis.reduced_support_size() == 2U &&
          edge_analysis.reduced_support_contains(0U) &&
          edge_analysis.reduced_support_contains(1U) &&
          !edge_analysis.reduced_support_contains(2U) &&
          !edge_analysis.reduced_support_contains(3U),
      "a tetrahedral boundary center reduces to its positive edge");

  const std::array<ExactRational3, 4> boundary_face{
      point(5, 0, 0),
      point(-3, 4, 0),
      point(-3, -4, 0),
      point(0, 0, 5)};
  const CircumcenterSupportAnalysis face_analysis =
      analyze_circumcenter_support(boundary_face);
  check_analysis_witnesses(
      face_analysis,
      4U,
      point(0, 0, 0),
      ExactLevel{BigInt{25}},
      "face-boundary tetrahedron");
  check(
      face_analysis.status() == CircumcenterSupportStatus::boundary_reduced &&
          face_analysis.reduced_support_size() == 3U &&
          face_analysis.reduced_support_contains(0U) &&
          face_analysis.reduced_support_contains(1U) &&
          face_analysis.reduced_support_contains(2U) &&
          !face_analysis.reduced_support_contains(3U),
      "a tetrahedral boundary center reduces to its positive face");

  const std::array<ExactRational3, 4> standard{
      point(0, 0, 0),
      point(1, 0, 0),
      point(0, 1, 0),
      point(0, 0, 1)};
  const CircumcenterSupportAnalysis standard_analysis =
      analyze_circumcenter_support(standard);
  check(
      standard_analysis.status() ==
              CircumcenterSupportStatus::exterior_circumcenter &&
          !standard_analysis.reduced_support_mask().has_value(),
      "a non-well-centered tetrahedron is not silently reduced");

  PredicateCounters exterior_zero_counters;
  const CircumcenterSupportAnalysis exterior_with_zero =
      analyze_circumcenter_support(
          std::array<ExactRational3, 4>{
              point(0, 0, 0),
              point(4, 0, 0),
              point(1, 1, 0),
              point(3, -1, 2)},
          &exterior_zero_counters);
  check_analysis_witnesses(
      exterior_with_zero,
      4U,
      point(2, -1, 0),
      ExactLevel{BigInt{5}},
      "exterior tetrahedron with a zero barycentric coefficient");
  check(
      exterior_with_zero.status() ==
              CircumcenterSupportStatus::exterior_circumcenter &&
          !exterior_with_zero.reduced_support_mask().has_value(),
      "a negative coefficient takes priority over a simultaneous exact zero");
  check(
      exterior_zero_counters.cpu_multiprecision_certified() == 4U &&
          exterior_zero_counters.exact_zeros() == 1U,
      "the mixed exterior witness records its exact zero without reducing");
  if (exterior_with_zero.barycentric().has_value()) {
    check_barycentric(
        *exterior_with_zero.barycentric(),
        ConvexHullLocation::exterior,
        {ExactRational{BigInt{5}, BigInt{4}},
         ExactRational{BigInt{3}, BigInt{4}},
         ExactRational{BigInt{-1}},
         ExactRational{}},
        4U,
        "mixed-sign tetrahedron circumcenter");
  }

  const CircumcenterSupportAnalysis coplanar = analyze_circumcenter_support(
      std::array<ExactRational3, 4>{
          point(0, 0, 0),
          point(1, 0, 0),
          point(0, 1, 0),
          point(1, 1, 0)});
  check(
      coplanar.status() == CircumcenterSupportStatus::affinely_dependent &&
          coplanar.circumcenter_result().affine_dimension() == 2U,
      "a coplanar quadruplet remains an explicit dimensional degeneracy");
}

void test_adaptive_support_analysis_and_provenance() {
  const std::array<CertifiedPoint3, 1> singleton{
      binary_point(3.0, -2.0, 1.0)};
  const std::array<CertifiedPoint3, 2> pair{
      binary_point(-1.0, 0.0, 0.0),
      binary_point(1.0, 0.0, 0.0)};
  const std::array<CertifiedPoint3, 3> acute_triangle{
      binary_point(0.0, 0.0, 0.0),
      binary_point(4.0, 0.0, 0.0),
      binary_point(1.0, 3.0, 0.0)};
  const std::array<CertifiedPoint3, 4> regular_tetrahedron{
      binary_point(1.0, 1.0, 1.0),
      binary_point(1.0, -1.0, -1.0),
      binary_point(-1.0, 1.0, -1.0),
      binary_point(-1.0, -1.0, 1.0)};

  PredicateCounters singleton_counters;
  const auto singleton_analysis = analyze_circumcenter_support(
      singleton, &singleton_counters);
  PredicateCounters pair_counters;
  const auto pair_analysis =
      analyze_circumcenter_support(pair, &pair_counters);
  PredicateCounters triangle_counters;
  const auto triangle_analysis = analyze_circumcenter_support(
      acute_triangle, &triangle_counters);
  PredicateCounters tetrahedron_counters;
  const auto tetrahedron_analysis = analyze_circumcenter_support(
      regular_tetrahedron, &tetrahedron_counters);

  const std::array<const CircumcenterSupportAnalysis*, 4> analyses{
      &singleton_analysis,
      &pair_analysis,
      &triangle_analysis,
      &tetrahedron_analysis};
  for (std::size_t index = 0; index < analyses.size(); ++index) {
    const CircumcenterSupportAnalysis& analysis = *analyses[index];
    check(
        analysis.status() == CircumcenterSupportStatus::minimal &&
            analysis.barycentric().has_value() &&
            analysis.barycentric()->certification_stage() ==
                CertificationStage::fp64_filtered &&
            analysis.binary64_support().size() == index + 1U,
        "well-separated binary64 support analysis retains FP64 provenance");
  }
  check_terminal_certifications(
      singleton_counters,
      CertificationStage::fp64_filtered,
      1U,
      0U,
      "singleton support analysis records one FP64 terminal");
  check_terminal_certifications(
      pair_counters,
      CertificationStage::fp64_filtered,
      2U,
      0U,
      "pair support analysis records two FP64 terminals");
  check_terminal_certifications(
      triangle_counters,
      CertificationStage::fp64_filtered,
      3U,
      0U,
      "triangle support analysis records three FP64 terminals");
  check_terminal_certifications(
      tetrahedron_counters,
      CertificationStage::fp64_filtered,
      4U,
      0U,
      "tetrahedron support analysis records four FP64 terminals");

  const std::array<CertifiedPoint3, 3> right_triangle{
      binary_point(0.0, 0.0, 0.0),
      binary_point(2.0, 0.0, 0.0),
      binary_point(0.0, 2.0, 0.0)};
  PredicateCounters boundary_counters;
  const auto boundary_analysis = analyze_circumcenter_support(
      right_triangle,
      &boundary_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      boundary_analysis.status() ==
              CircumcenterSupportStatus::boundary_reduced &&
          boundary_analysis.barycentric().has_value() &&
          boundary_analysis.barycentric()->certification_stage() ==
              CertificationStage::expansion &&
          boundary_analysis.binary64_support().size() == 3U &&
          boundary_analysis.reduced_support_size() == 2U,
      "a binary right triangle keeps full source provenance and an expansion zero");
  check_terminal_certifications(
      boundary_counters,
      CertificationStage::expansion,
      3U,
      1U,
      "right-triangle analysis records one collective expansion pass");

  PredicateCounters multiprecision_counters;
  const auto multiprecision_analysis = analyze_circumcenter_support(
      acute_triangle,
      &multiprecision_counters,
      PredicateFilterPolicy::multiprecision_only);
  check(
      multiprecision_analysis == triangle_analysis &&
          multiprecision_analysis.barycentric().has_value() &&
          multiprecision_analysis.barycentric()->certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "support analysis keeps identical exact witnesses when fast stages are disabled");
  check_terminal_certifications(
      multiprecision_counters,
      CertificationStage::cpu_multiprecision,
      3U,
      0U,
      "MP-only support analysis records one terminal per coordinate");

  const std::array<ExactRational3, 3> rational_acute{
      point(0, 0, 0), point(4, 0, 0), point(1, 3, 0)};
  const auto rational_analysis = analyze_circumcenter_support(rational_acute);
  check(
      rational_analysis == triangle_analysis &&
          rational_analysis.binary64_support().empty() &&
          triangle_analysis.binary64_support().size() == 3U,
      "scientific analysis equality ignores provenance while rational input has none");

  const std::array<CertifiedPoint3, 3> dependent_support{
      binary_point(0.0, 0.0, 0.0),
      binary_point(1.0, 0.0, 0.0),
      binary_point(2.0, 0.0, 0.0)};
  PredicateCounters dependent_counters;
  const auto dependent = analyze_circumcenter_support(
      dependent_support, &dependent_counters);
  check(
      dependent.status() == CircumcenterSupportStatus::affinely_dependent &&
          !dependent.barycentric().has_value() &&
          dependent.binary64_support().size() == 3U,
      "a dependent binary64 analysis is rejected scientifically but retains replay provenance");
  check(
      dependent_counters.certified_decisions() == 0U,
      "a dependent adaptive analysis records no invented sign decision");
  const auto rational_dependent = analyze_circumcenter_support(
      std::array<ExactRational3, 3>{
          point(0, 0, 0), point(1, 0, 0), point(2, 0, 0)});
  check(
      rational_dependent.binary64_support().empty(),
      "a dependent rational analysis does not forge binary64 provenance");
}

void test_permutations_and_ulp_boundaries() {
  const std::array<ExactRational3, 3> right{
      point(0, 0, 0), point(2, 0, 0), point(0, 2, 0)};
  std::array<std::size_t, 3> permutation{0U, 1U, 2U};
  std::size_t permutation_count = 0U;
  do {
    const std::array<ExactRational3, 3> permuted{
        right[permutation[0]], right[permutation[1]], right[permutation[2]]};
    const CircumcenterSupportAnalysis analysis =
        analyze_circumcenter_support(permuted);
    check(
        analysis.status() == CircumcenterSupportStatus::boundary_reduced &&
            analysis.reduced_support_size() == 2U,
        "right triangle permutation preserves boundary reduction " +
            std::to_string(permutation_count));
    for (std::size_t index = 0; index < permutation.size(); ++index) {
      const bool expected = permutation[index] != 0U;
      check(
          analysis.reduced_support_contains(index) == expected,
          "right triangle reduction follows its permuted support indices");
    }
    ++permutation_count;
  } while (std::next_permutation(permutation.begin(), permutation.end()));
  check(permutation_count == 6U, "all triangle support permutations were tested");

  const std::array<ExactRational3, 4> face_boundary{
      point(5, 0, 0),
      point(-3, 4, 0),
      point(-3, -4, 0),
      point(0, 0, 5)};
  std::array<std::size_t, 4> tetrahedron_permutation{0U, 1U, 2U, 3U};
  std::size_t tetrahedron_permutation_count = 0U;
  do {
    const std::array<ExactRational3, 4> permuted{
        face_boundary[tetrahedron_permutation[0]],
        face_boundary[tetrahedron_permutation[1]],
        face_boundary[tetrahedron_permutation[2]],
        face_boundary[tetrahedron_permutation[3]]};
    const CircumcenterSupportAnalysis analysis =
        analyze_circumcenter_support(permuted);
    check_analysis_witnesses(
        analysis,
        4U,
        point(0, 0, 0),
        ExactLevel{BigInt{25}},
        "face-boundary tetrahedron permutation " +
            std::to_string(tetrahedron_permutation_count));
    check(
        analysis.status() == CircumcenterSupportStatus::boundary_reduced &&
            analysis.reduced_support_size() == 3U,
        "tetrahedron permutation preserves face reduction " +
            std::to_string(tetrahedron_permutation_count));
    for (std::size_t index = 0; index < tetrahedron_permutation.size();
         ++index) {
      const bool expected = tetrahedron_permutation[index] != 3U;
      check(
          analysis.reduced_support_contains(index) == expected,
          "tetrahedron reduction follows its permuted support indices");
    }
    ++tetrahedron_permutation_count;
  } while (std::next_permutation(
      tetrahedron_permutation.begin(), tetrahedron_permutation.end()));
  check(
      tetrahedron_permutation_count == 24U,
      "all tetrahedron support permutations were tested");

  const double below_one = std::nextafter(1.0, 0.0);
  const double above_one = std::nextafter(1.0, 2.0);
  const auto analyze_height = [](double height) {
    return analyze_circumcenter_support(std::array<CertifiedPoint3, 3>{
        binary_point(0.0, 0.0, 0.0),
        binary_point(2.0, 0.0, 0.0),
        binary_point(1.0, height, 0.0)});
  };
  check(
      analyze_height(below_one).status() ==
          CircumcenterSupportStatus::exterior_circumcenter,
      "one ULP below a right angle is decided as obtuse exactly");
  check(
      analyze_height(1.0).status() ==
          CircumcenterSupportStatus::boundary_reduced,
      "the exact right-angle boundary is preserved");
  check(
      analyze_height(above_one).status() == CircumcenterSupportStatus::minimal,
      "one ULP above a right angle is decided as acute exactly");
}

void test_sphere_point_classification() {
  const ExactRational3 origin = point(0, 0, 0);
  PredicateCounters counters;
  const auto interior =
      classify_sphere_point(origin, ExactLevel{BigInt{1}}, origin, &counters);
  check(
      interior.location() == SpherePointLocation::strictly_inside &&
          interior.decision().sign() == PredicateSign::negative &&
          interior.point_squared_distance() == ExactLevel{} &&
          interior.signed_power() == ExactRational{BigInt{-1}},
      "the sphere center is strictly interior to a positive-radius sphere");
  const auto boundary = classify_sphere_point(
      origin, ExactLevel{BigInt{1}}, point(1, 0, 0), &counters);
  check(
      boundary.location() == SpherePointLocation::boundary &&
          boundary.decision().sign() == PredicateSign::zero &&
          boundary.signed_power().is_zero(),
      "a support point is exactly on its sphere");
  const auto exterior = classify_sphere_point(
      origin, ExactLevel{BigInt{1}}, point(2, 0, 0), &counters);
  check(
      exterior.location() == SpherePointLocation::outside &&
          exterior.decision().sign() == PredicateSign::positive &&
          exterior.signed_power() == ExactRational{BigInt{3}},
      "a point outside a sphere has positive signed power");
  check(
      counters.cpu_multiprecision_certified() == 3U &&
          counters.exact_zeros() == 1U,
      "sphere classifications record exactly one decision per point");

  const auto zero_boundary =
      classify_sphere_point(origin, ExactLevel{}, origin);
  const auto zero_exterior =
      classify_sphere_point(origin, ExactLevel{}, point(0, 0, 1));
  check(
      zero_boundary.location() == SpherePointLocation::boundary &&
          zero_exterior.location() == SpherePointLocation::outside,
      "a zero-radius sphere has only its center on the boundary");

  const ExactRational3 rational_center = rational_point(
      ExactRational{BigInt{7}, BigInt{6}},
      ExactRational{BigInt{-1}, BigInt{6}},
      ExactRational{});
  const auto rational_boundary = classify_sphere_point(
      rational_center, ExactLevel{BigInt{25}, BigInt{18}}, origin);
  check(
      rational_boundary.location() == SpherePointLocation::boundary,
      "a non-dyadic rational center classifies its support point exactly");

  const double inward = std::nextafter(1.0, 0.0);
  const double outward = std::nextafter(1.0, 2.0);
  check(
      classify_sphere_point(
          origin, ExactLevel{BigInt{1}}, binary_point(inward, 0.0, 0.0))
              .location() == SpherePointLocation::strictly_inside,
      "one ULP inside a sphere remains strictly interior");
  check(
      classify_sphere_point(
          origin, ExactLevel{BigInt{1}}, binary_point(outward, 0.0, 0.0))
              .location() == SpherePointLocation::outside,
      "one ULP outside a sphere remains strictly exterior");

  check_throws<std::invalid_argument>(
      [] {
        const auto dependent = morsehgp3d::exact::circumcenter(
            point(0, 0, 0), point(0, 0, 0));
        static_cast<void>(classify_sphere_point(dependent, point(0, 0, 0)));
      },
      "a dependent support cannot be used as an arbitrary sphere");
}

void test_adaptive_support_sphere_classification() {
  const std::array<CertifiedPoint3, 2> support{
      binary_point(-1.0, 0.0, 0.0),
      binary_point(1.0, 0.0, 0.0)};

  PredicateCounters inside_counters;
  const auto inside = classify_sphere_point(
      support,
      binary_point(0.0, 0.0, 0.0),
      &inside_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      inside.location() == SpherePointLocation::strictly_inside &&
          inside.decision().sign() == PredicateSign::negative &&
          inside.decision().certification_stage() ==
              CertificationStage::fp64_filtered &&
          inside.point_squared_distance() == ExactLevel{} &&
          inside.signed_power() == ExactRational{BigInt{-1}},
      "the support polynomial certifies a strict interior point by FP64");
  check_terminal_certifications(
      inside_counters,
      CertificationStage::fp64_filtered,
      1U,
      0U,
      "an interior sphere query records exactly one FP64 terminal");

  PredicateCounters boundary_counters;
  const auto boundary = classify_sphere_point(
      support,
      support[1],
      &boundary_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      boundary.location() == SpherePointLocation::boundary &&
          boundary.decision().sign() == PredicateSign::zero &&
          boundary.decision().certification_stage() ==
              CertificationStage::expansion &&
          boundary.signed_power().is_zero(),
      "an exact sphere boundary is certified by expansion");
  check_terminal_certifications(
      boundary_counters,
      CertificationStage::expansion,
      1U,
      1U,
      "a boundary sphere query records one expansion zero");

  PredicateCounters outside_counters;
  const auto outside = classify_sphere_point(
      support,
      binary_point(2.0, 0.0, 0.0),
      &outside_counters,
      PredicateFilterPolicy::allow_fp64);
  check(
      outside.location() == SpherePointLocation::outside &&
          outside.decision().sign() == PredicateSign::positive &&
          outside.decision().certification_stage() ==
              CertificationStage::fp64_filtered &&
          outside.signed_power() == ExactRational{BigInt{3}},
      "the legacy FP64 policy certifies a strict exterior support query");
  check_terminal_certifications(
      outside_counters,
      CertificationStage::fp64_filtered,
      1U,
      0U,
      "an exterior sphere query records exactly one FP64 terminal");

  PredicateCounters multiprecision_counters;
  const auto multiprecision_outside = classify_sphere_point(
      support,
      binary_point(2.0, 0.0, 0.0),
      &multiprecision_counters,
      PredicateFilterPolicy::multiprecision_only);
  check_same_sphere_witness(
      multiprecision_outside,
      outside,
      "MP-only sphere classification retains the exact FP64 witness");
  check(
      multiprecision_outside.decision().certification_stage() ==
          CertificationStage::cpu_multiprecision,
      "MP-only sphere classification reports its terminal authority");
  check_terminal_certifications(
      multiprecision_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      0U,
      "MP-only sphere classification records exactly one terminal");

  PredicateCounters fp64_only_boundary_counters;
  const auto fp64_only_boundary = classify_sphere_point(
      support,
      support[1],
      &fp64_only_boundary_counters,
      PredicateFilterPolicy::allow_fp64);
  check_same_sphere_witness(
      fp64_only_boundary,
      boundary,
      "the FP64-only boundary fallback preserves the expansion witness");
  check(
      fp64_only_boundary.decision().certification_stage() ==
          CertificationStage::cpu_multiprecision,
      "the FP64-only policy falls back to MP for a sphere equality");
  check_terminal_certifications(
      fp64_only_boundary_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      1U,
      "the FP64-only boundary records one MP zero");

  PredicateCounters rational_counters;
  const auto rational = classify_sphere_point(
      point(0, 0, 0),
      ExactLevel{BigInt{1}},
      point(2, 0, 0),
      &rational_counters);
  check(
      rational.decision().certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          rational.signed_power() == outside.signed_power(),
      "the arbitrary rational sphere API remains MP-only");
  check_terminal_certifications(
      rational_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      0U,
      "the rational sphere API records one MP terminal");

  PredicateCounters legacy_circumcenter_counters;
  const auto legacy_sphere = morsehgp3d::exact::circumcenter(
      point(-1, 0, 0), point(1, 0, 0));
  const auto legacy_classification = classify_sphere_point(
      legacy_sphere,
      binary_point(2.0, 0.0, 0.0),
      &legacy_circumcenter_counters);
  check(
      legacy_classification.decision().certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          legacy_classification.signed_power() == outside.signed_power(),
      "a CircumcenterResult without binary support provenance remains MP-only");
  check_terminal_certifications(
      legacy_circumcenter_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      0U,
      "the legacy circumcenter sphere API records one MP terminal");

  const std::array<CertifiedPoint3, 2> reversed{support[1], support[0]};
  const auto reversed_outside = classify_sphere_point(
      reversed, binary_point(2.0, 0.0, 0.0));
  check_same_sphere_witness(
      reversed_outside,
      outside,
      "support permutations preserve the exact sphere-side witness");
  check(
      reversed_outside.decision().certification_stage() ==
          CertificationStage::fp64_filtered,
      "support permutations preserve the strict FP64 sphere stage");

  check_throws<std::invalid_argument>(
      [] {
        const std::array<CertifiedPoint3, 3> dependent{
            binary_point(0.0, 0.0, 0.0),
            binary_point(1.0, 0.0, 0.0),
            binary_point(2.0, 0.0, 0.0)};
        static_cast<void>(classify_sphere_point(
            dependent, binary_point(0.0, 0.0, 0.0)));
      },
      "sphere classification rejects an affinely dependent binary64 support");
}

void test_adaptive_support_fenv_fail_closed() {
  ScopedFloatingEnvironment restore_at_exit;
  check(
      restore_at_exit.saved(),
      "the adaptive support FENV test saves the caller environment");
  if (!restore_at_exit.saved()) {
    return;
  }

  const std::array<CertifiedPoint3, 2> support{
      binary_point(-1.0, 0.0, 0.0),
      binary_point(1.0, 0.0, 0.0)};
  check(
      std::fesetround(FE_UPWARD) == 0 &&
          std::feclearexcept(FE_ALL_EXCEPT) == 0 &&
          std::feraiseexcept(FE_INVALID | FE_DIVBYZERO) == 0,
      "the platform accepts the non-nearest support FENV setup");
  const int flags_before = std::fetestexcept(FE_ALL_EXCEPT);
  PredicateCounters support_counters;
  const auto analysis = analyze_circumcenter_support(
      support,
      &support_counters,
      PredicateFilterPolicy::allow_adaptive);
  PredicateCounters sphere_counters;
  const auto sphere = classify_sphere_point(
      support,
      binary_point(2.0, 0.0, 0.0),
      &sphere_counters,
      PredicateFilterPolicy::allow_adaptive);
  check(
      analysis.barycentric().has_value() &&
          analysis.barycentric()->certification_stage() ==
              CertificationStage::cpu_multiprecision &&
          sphere.decision().certification_stage() ==
              CertificationStage::cpu_multiprecision,
      "non-nearest rounding disables adaptive support and sphere fast stages");
  check(
      std::fegetround() == FE_UPWARD &&
          std::fetestexcept(FE_ALL_EXCEPT) == flags_before,
      "adaptive support and sphere APIs preserve caller rounding and flags");
  check_terminal_certifications(
      support_counters,
      CertificationStage::cpu_multiprecision,
      2U,
      0U,
      "non-nearest support analysis records two MP terminals");
  check_terminal_certifications(
      sphere_counters,
      CertificationStage::cpu_multiprecision,
      1U,
      0U,
      "non-nearest sphere classification records one MP terminal");

#if defined(__SSE2__) || defined(_M_X64) || \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  check(
      std::fesetround(FE_TONEAREST) == 0 &&
          std::feclearexcept(FE_ALL_EXCEPT) == 0,
      "the FTZ/DAZ support test selects a clean nearest environment");
  constexpr unsigned int flush_to_zero_mask = 1U << 15U;
  constexpr unsigned int denormals_are_zero_mask = 1U << 6U;
  constexpr unsigned int rounding_control_mask = 3U << 13U;
  constexpr unsigned int exception_status_mask = 0x3fU;
  constexpr unsigned int preserved_mask = flush_to_zero_mask |
                                           denormals_are_zero_mask |
                                           rounding_control_mask |
                                           exception_status_mask;
  const unsigned int original_mxcsr = _mm_getcsr();
  const std::array<unsigned int, 3> altered_modes{
      (original_mxcsr | denormals_are_zero_mask) & ~flush_to_zero_mask,
      (original_mxcsr | flush_to_zero_mask) & ~denormals_are_zero_mask,
      original_mxcsr | flush_to_zero_mask | denormals_are_zero_mask};
  for (const unsigned int altered_mode : altered_modes) {
    _mm_setcsr(altered_mode);
    PredicateCounters altered_support_counters;
    const auto altered_analysis = analyze_circumcenter_support(
        support,
        &altered_support_counters,
        PredicateFilterPolicy::allow_adaptive);
    PredicateCounters altered_sphere_counters;
    const auto altered_sphere = classify_sphere_point(
        support,
        binary_point(2.0, 0.0, 0.0),
        &altered_sphere_counters,
        PredicateFilterPolicy::allow_adaptive);
    check(
        altered_analysis.barycentric().has_value() &&
            altered_analysis.barycentric()->certification_stage() ==
                CertificationStage::cpu_multiprecision &&
            altered_sphere.decision().certification_stage() ==
                CertificationStage::cpu_multiprecision,
        "FTZ or DAZ disables support and sphere fast stages");
    check(
        (_mm_getcsr() & preserved_mask) == (altered_mode & preserved_mask),
        "support and sphere APIs preserve caller FTZ/DAZ and MXCSR status");
    check_terminal_certifications(
        altered_support_counters,
        CertificationStage::cpu_multiprecision,
        2U,
        0U,
        "FTZ/DAZ support analysis records exactly two MP terminals");
    check_terminal_certifications(
        altered_sphere_counters,
        CertificationStage::cpu_multiprecision,
        1U,
        0U,
        "FTZ/DAZ sphere classification records exactly one MP terminal");
  }
  _mm_setcsr(original_mxcsr);
#endif
}

void test_extreme_singletons_and_invariants() {
  const double subnormal_value = std::numeric_limits<double>::denorm_min();
  const double maximum = std::numeric_limits<double>::max();
  const auto subnormal_analysis = analyze_circumcenter_support(
      std::array<CertifiedPoint3, 1>{
          binary_point(subnormal_value, -0.0, 0.0)});
  const auto maximal = analyze_circumcenter_support(
      std::array<CertifiedPoint3, 1>{binary_point(maximum, -maximum, maximum)});
  check(
      subnormal_analysis.status() == CircumcenterSupportStatus::minimal &&
          subnormal_analysis.circumcenter_result().squared_level() ==
              ExactLevel{},
      "minimum-subnormal singleton retains a zero exact level");
  check(
      maximal.status() == CircumcenterSupportStatus::minimal &&
          maximal.circumcenter_result().squared_level() == ExactLevel{},
      "maximum-finite singleton retains a zero exact level");

  check(
      morsehgp3d::exact::to_string(ConvexHullLocation::relative_interior) ==
              "relative_interior" &&
          morsehgp3d::exact::to_string(
              CircumcenterSupportStatus::boundary_reduced) ==
              "boundary_reduced" &&
          morsehgp3d::exact::to_string(SpherePointLocation::boundary) ==
              "boundary",
      "support and sphere classifications have stable diagnostic spellings");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::exact::to_string(
            static_cast<ConvexHullLocation>(99)));
      },
      "an invalid convex-hull location is rejected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::exact::to_string(
            static_cast<CircumcenterSupportStatus>(99)));
      },
      "an invalid support status is rejected");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(morsehgp3d::exact::to_string(
            static_cast<SpherePointLocation>(99)));
      },
      "an invalid sphere-point location is rejected");
  const auto pair = analyze_circumcenter_support(
      std::array<ExactRational3, 2>{point(-1, 0, 0), point(1, 0, 0)});
  check_throws<std::out_of_range>(
      [&pair] { static_cast<void>(pair.reduced_support_contains(2U)); },
      "a reduced-support query rejects an out-of-range index");
}

}  // namespace

int main() {
  test_singleton_and_pair_barycentrics();
  test_adaptive_binary_barycentrics();
  test_triangle_support_analysis();
  test_tetrahedron_support_analysis();
  test_adaptive_support_analysis_and_provenance();
  test_permutations_and_ulp_boundaries();
  test_sphere_point_classification();
  test_adaptive_support_sphere_classification();
  test_adaptive_support_fenv_fail_closed();
  test_extreme_singletons_and_invariants();

  if (failures != 0) {
    std::cerr << failures << " support test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D exact support and sphere tests passed\n";
  return 0;
}
