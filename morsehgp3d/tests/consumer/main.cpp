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
  using morsehgp3d::exact::PredicateSign;

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
  return 0;
}
