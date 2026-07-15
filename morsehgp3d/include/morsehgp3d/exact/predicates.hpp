#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <utility>

namespace morsehgp3d::exact {

struct DistanceComparisonResult {
  PredicateDecision decision;
  ExactLevel left_squared_distance;
  ExactLevel right_squared_distance;
};

struct Orientation3DResult {
  PredicateDecision decision;
  ExactRational determinant;
};

[[nodiscard]] inline ExactRational squared_distance(
    const CertifiedPoint3& left, const CertifiedPoint3& right) {
  ExactRational result;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const ExactRational delta = left.coordinate(axis) - right.coordinate(axis);
    result = result + delta * delta;
  }
  return result;
}

// Returns the sign of ||witness-left||^2 - ||witness-right||^2.
[[nodiscard]] inline DistanceComparisonResult compare_squared_distances(
    const CertifiedPoint3& witness,
    const CertifiedPoint3& left,
    const CertifiedPoint3& right,
    PredicateCounters* counters = nullptr) {
  ExactLevel left_level{squared_distance(witness, left)};
  ExactLevel right_level{squared_distance(witness, right)};
  const PredicateDecision decision{
      predicate_sign((left_level < right_level) ? -1 : ((right_level < left_level) ? 1 : 0)),
      CertificationStage::cpu_multiprecision};
  DistanceComparisonResult result{
      decision, std::move(left_level), std::move(right_level)};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return result;
}

// Sign convention: det([b-a, c-a, d-a]); (0, e1, e2, e3) is positive.
[[nodiscard]] inline Orientation3DResult orientation_3d(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d,
    PredicateCounters* counters = nullptr) {
  std::array<ExactRational, 3> u{};
  std::array<ExactRational, 3> v{};
  std::array<ExactRational, 3> w{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    u[axis] = b.coordinate(axis) - a.coordinate(axis);
    v[axis] = c.coordinate(axis) - a.coordinate(axis);
    w[axis] = d.coordinate(axis) - a.coordinate(axis);
  }

  ExactRational determinant =
      u[0] * (v[1] * w[2] - v[2] * w[1]) -
      u[1] * (v[0] * w[2] - v[2] * w[0]) +
      u[2] * (v[0] * w[1] - v[1] * w[0]);
  const PredicateDecision decision{
      predicate_sign(determinant.sign()), CertificationStage::cpu_multiprecision};
  Orientation3DResult result{decision, std::move(determinant)};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return result;
}

}  // namespace morsehgp3d::exact
