#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <array>
#include <cstdint>
#include <iostream>

int main() {
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::CertifiedPoint3;
  using morsehgp3d::exact::ExactLabelMoments;
  using morsehgp3d::exact::ExactLevel;
  using morsehgp3d::exact::CertificationStage;
  using morsehgp3d::exact::PredicateFilterPolicy;
  using morsehgp3d::exact::PredicateSign;

  if (!morsehgp3d::exact::fp64_filter_environment_supported()) {
    std::cerr << "installed target did not preserve the strict FP64 environment\n";
    return 1;
  }

  const ExactLevel level{BigInt{2}, BigInt{8}};
  if (level.canonical_key() != "1/4") {
    std::cerr << "installed ExactLevel did not preserve canonical semantics\n";
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
  return 0;
}
