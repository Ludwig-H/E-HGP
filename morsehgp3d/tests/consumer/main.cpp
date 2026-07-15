#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/level_order.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::CertifiedPoint3;
  using morsehgp3d::exact::CircumcenterKind;
  using morsehgp3d::exact::CircumcenterSupportStatus;
  using morsehgp3d::exact::CanonicalSupportIds;
  using morsehgp3d::exact::ExactLabelMoments;
  using morsehgp3d::exact::ExactLevel;
  using morsehgp3d::exact::ExactPlane3;
  using morsehgp3d::exact::ExactRational3;
  using morsehgp3d::exact::CertificationStage;
  using morsehgp3d::exact::PredicateFilterPolicy;
  using morsehgp3d::exact::PredicateCounters;
  using morsehgp3d::exact::PredicateSign;
  using morsehgp3d::exact::SpherePointLocation;
  using morsehgp3d::exact::ThreePlaneIntersectionKind;

  if (!morsehgp3d::exact::fp64_filter_environment_supported()) {
    std::cerr << "installed target did not preserve the strict FP64 environment\n";
    return 1;
  }

  const ExactLevel level{BigInt{2}, BigInt{8}};
  if (level.canonical_key() != "1/4") {
    std::cerr << "installed ExactLevel did not preserve canonical semantics\n";
    return 1;
  }
  PredicateCounters level_counters;
  const auto equal_level_order = morsehgp3d::exact::compare_exact_levels(
      ExactLevel{BigInt{1}, BigInt{2}},
      ExactLevel{BigInt{2}, BigInt{4}},
      &level_counters);
  if (equal_level_order.decision.sign() != PredicateSign::zero ||
      equal_level_order.cross_product_difference != 0 ||
      level_counters.cpu_multiprecision_certified() != 1U ||
      level_counters.exact_zeros() != 1U) {
    std::cerr << "installed exact-level ordering changed semantics\n";
    return 1;
  }

  const std::array<CertifiedPoint3, 2> points{
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0)};
  const std::array<std::uint32_t, 1> q_ids{0U};
  const std::array<std::uint32_t, 1> r_ids{1U};
  const ExactLabelMoments q =
      ExactLabelMoments::from_canonical_ids(q_ids, points);
  const ExactLabelMoments r =
      ExactLabelMoments::from_canonical_ids(r_ids, points);
  const auto side = morsehgp3d::exact::decide_power_bisector_side(
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0), r, q);
  if (side.sign() != PredicateSign::positive) {
    std::cerr << "installed power-bisector predicate changed exact semantics\n";
    return 1;
  }

  const auto exact_only_distance =
      morsehgp3d::exact::decide_squared_distance_order(
          CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
          points[0],
          points[1],
          nullptr,
          PredicateFilterPolicy::multiprecision_only);
  if (exact_only_distance.sign() != PredicateSign::negative ||
      exact_only_distance.certification_stage() !=
          CertificationStage::cpu_multiprecision) {
    std::cerr << "installed exact-only predicate path changed semantics\n";
    return 1;
  }
  const auto filtered_distance =
      morsehgp3d::exact::decide_squared_distance_order(
          CertifiedPoint3::from_binary64(0.0, 0.0, 0.0), points[0], points[1]);
  if (filtered_distance.sign() != PredicateSign::negative ||
      filtered_distance.certification_stage() !=
          CertificationStage::fp64_filtered) {
    std::cerr << "installed filtered predicate path changed semantics\n";
    return 1;
  }

  const ExactPlane3 x_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{1}, BigInt{0}, BigInt{0}, BigInt{0}});
  const ExactPlane3 y_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{0}, BigInt{1}, BigInt{0}, BigInt{0}});
  const ExactPlane3 z_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{0}, BigInt{0}, BigInt{1}, BigInt{0}});
  const auto intersection =
      morsehgp3d::exact::intersect_three_planes(x_zero, y_zero, z_zero);
  const auto fourth = morsehgp3d::exact::decide_fourth_plane_incidence(
      x_zero,
      y_zero,
      z_zero,
      ExactPlane3::from_integer_coefficients(
          {BigInt{1}, BigInt{1}, BigInt{1}, BigInt{0}}));
  if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
      !intersection.point().has_value() ||
      intersection.affine_dimension() != 0U ||
      fourth.sign() != PredicateSign::zero) {
    std::cerr << "installed affine exact kernel changed semantics\n";
    return 1;
  }

  const auto center = morsehgp3d::exact::circumcenter(
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 1.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 0.0, 1.0));
  if (center.kind() != CircumcenterKind::unique ||
      center.support_size() != 4U || center.affine_dimension() != 3U ||
      !center.center().has_value() || !center.squared_level().has_value() ||
      *center.center() !=
          ExactRational3{BigInt{1}, BigInt{1}, BigInt{1}, BigInt{2}} ||
      *center.squared_level() != ExactLevel{BigInt{3}, BigInt{4}}) {
    std::cerr << "installed exact center construction changed semantics\n";
    return 1;
  }

  const std::array<CertifiedPoint3, 3> right_triangle{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 1.0, 0.0)};
  const auto support =
      morsehgp3d::exact::analyze_circumcenter_support(right_triangle);
  if (support.status() != CircumcenterSupportStatus::boundary_reduced ||
      support.reduced_support_size() != 2U ||
      !support.reduced_support_contains(0U) ||
      support.reduced_support_contains(1U) ||
      !support.reduced_support_contains(2U)) {
    std::cerr << "installed exact support reduction changed semantics\n";
    return 1;
  }
  const std::array<std::uint64_t, 3> source_support_ids{0U, 1U, 2U};
  const auto reduced_emission =
      morsehgp3d::exact::support_level_emission_from_analysis(
          source_support_ids, support);
  const std::array<std::uint64_t, 2> reduced_support_ids{0U, 2U};
  const auto direct_emission = morsehgp3d::exact::SupportLevelEmission::create(
      reduced_emission.squared_level(),
      CanonicalSupportIds::from_ids(reduced_support_ids),
      CanonicalSupportIds::from_ids(reduced_support_ids));
  const std::vector<morsehgp3d::exact::SupportLevelEmission> emissions{
      reduced_emission, direct_emission, direct_emission};
  const auto batches = morsehgp3d::exact::canonical_level_batches(emissions);
  if (batches.batches.size() != 1U ||
      batches.batches[0].supports.size() != 1U ||
      batches.batches[0].supports[0].source_provenance.size() != 2U ||
      batches.emission_count != 3U || batches.unique_emission_count != 2U ||
      batches.duplicate_emission_count != 1U) {
    std::cerr << "installed canonical level batching changed semantics\n";
    return 1;
  }
  const auto sphere_side = morsehgp3d::exact::classify_sphere_point(
      support.circumcenter_result(), right_triangle[1]);
  if (sphere_side.location() != SpherePointLocation::boundary ||
      sphere_side.decision().sign() != PredicateSign::zero) {
    std::cerr << "installed exact sphere classification changed semantics\n";
    return 1;
  }
  return 0;
}
