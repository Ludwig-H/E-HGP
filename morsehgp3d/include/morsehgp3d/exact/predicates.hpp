#pragma once

#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <stdexcept>
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

struct PowerBisectorWitness {
  ExactRational3 delta_coordinate_sum;
  ExactRational delta_squared_norm_sum;
  ExactRational affine_value;
};

struct PowerBisectorResult {
  PredicateDecision decision;
  PowerBisectorWitness witness;
};

namespace detail {

[[nodiscard]] inline PredicateDecision multiprecision_decision(
    int sign, PredicateCounters* counters) {
  const PredicateDecision decision{
      predicate_sign(sign), CertificationStage::cpu_multiprecision};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

inline void require_power_label_domain(
    const ExactLabelMoments& r, const ExactLabelMoments& q) {
  if (r.cardinality() != q.cardinality()) {
    throw std::invalid_argument(
        "power bisector labels must have the same cardinality");
  }
  if (r.cardinality() == 0U) {
    throw std::invalid_argument("power bisector labels cannot be empty");
  }
  if (r.cardinality() > maximum_power_label_cardinality) {
    throw std::invalid_argument("power bisector labels cannot exceed cardinality ten");
  }
}

}  // namespace detail

[[nodiscard]] inline ExactRational squared_distance(
    const CertifiedPoint3& left, const CertifiedPoint3& right) {
  ExactRational result;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const ExactRational delta = left.coordinate(axis) - right.coordinate(axis);
    result = result + delta * delta;
  }
  return result;
}

// Decision-only entry point. Future filtered stages can return before either
// exact squared distance is materialized.
[[nodiscard]] inline PredicateDecision decide_squared_distance_order(
    const CertifiedPoint3& witness,
    const CertifiedPoint3& left,
    const CertifiedPoint3& right,
    PredicateCounters* counters = nullptr) {
  const ExactRational difference =
      squared_distance(witness, left) - squared_distance(witness, right);
  return detail::multiprecision_decision(difference.sign(), counters);
}

// Returns the sign of ||witness-left||^2 - ||witness-right||^2.
[[nodiscard]] inline DistanceComparisonResult compare_squared_distances(
    const CertifiedPoint3& witness,
    const CertifiedPoint3& left,
    const CertifiedPoint3& right,
    PredicateCounters* counters = nullptr) {
  ExactLevel left_level{squared_distance(witness, left)};
  ExactLevel right_level{squared_distance(witness, right)};
  const PredicateDecision decision = detail::multiprecision_decision(
      (left_level < right_level) ? -1 : ((right_level < left_level) ? 1 : 0), counters);
  DistanceComparisonResult result{
      decision, std::move(left_level), std::move(right_level)};
  return result;
}

// Sign convention: det([b-a, c-a, d-a]); (0, e1, e2, e3) is positive.
[[nodiscard]] inline ExactRational orientation_3d_determinant(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d) {
  std::array<ExactRational, 3> u{};
  std::array<ExactRational, 3> v{};
  std::array<ExactRational, 3> w{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    u[axis] = b.coordinate(axis) - a.coordinate(axis);
    v[axis] = c.coordinate(axis) - a.coordinate(axis);
    w[axis] = d.coordinate(axis) - a.coordinate(axis);
  }

  return
      u[0] * (v[1] * w[2] - v[2] * w[1]) -
      u[1] * (v[0] * w[2] - v[2] * w[0]) +
      u[2] * (v[0] * w[1] - v[1] * w[0]);
}

// Decision-only entry point. The exact determinant remains available through
// orientation_3d() for replay and diagnostic witnesses.
[[nodiscard]] inline PredicateDecision decide_orientation_3d(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d,
    PredicateCounters* counters = nullptr) {
  return detail::multiprecision_decision(
      orientation_3d_determinant(a, b, c, d).sign(), counters);
}

[[nodiscard]] inline Orientation3DResult orientation_3d(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d,
    PredicateCounters* counters = nullptr) {
  ExactRational determinant = orientation_3d_determinant(a, b, c, d);
  const PredicateDecision decision =
      detail::multiprecision_decision(determinant.sign(), counters);
  Orientation3DResult result{decision, std::move(determinant)};
  return result;
}

// Exact value of H_{R,Q}(y) = -2 <y, S_R-S_Q> + (N_R-N_Q).
// R and Q must have the same cardinality so that the quadratic y term cancels.
[[nodiscard]] inline PowerBisectorWitness materialize_power_bisector_witness(
    const ExactRational3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q) {
  detail::require_power_label_domain(r, q);

  std::array<ExactRational, 3> delta_coordinate_sum{};
  ExactRational dot_product;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    delta_coordinate_sum[axis] =
        r.coordinate_sum(axis) - q.coordinate_sum(axis);
    dot_product = dot_product + witness.coordinate(axis) * delta_coordinate_sum[axis];
  }
  ExactRational delta_squared_norm_sum =
      r.squared_norm_sum() - q.squared_norm_sum();
  ExactRational affine_value = ExactRational{BigInt{-2}} * dot_product +
                               delta_squared_norm_sum;
  return PowerBisectorWitness{
      ExactRational3{delta_coordinate_sum},
      std::move(delta_squared_norm_sum),
      std::move(affine_value)};
}

[[nodiscard]] inline ExactRational evaluate_power_bisector(
    const ExactRational3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q) {
  detail::require_power_label_domain(r, q);
  ExactRational dot_product;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    dot_product = dot_product +
                  witness.coordinate(axis) *
                      (r.coordinate_sum(axis) - q.coordinate_sum(axis));
  }
  return ExactRational{BigInt{-2}} * dot_product +
         (r.squared_norm_sum() - q.squared_norm_sum());
}

[[nodiscard]] inline PredicateDecision decide_power_bisector_side(
    const ExactRational3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q,
    PredicateCounters* counters = nullptr) {
  return detail::multiprecision_decision(
      evaluate_power_bisector(witness, r, q).sign(), counters);
}

[[nodiscard]] inline PredicateDecision decide_power_bisector_side(
    const CertifiedPoint3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q,
    PredicateCounters* counters = nullptr) {
  return decide_power_bisector_side(witness.exact(), r, q, counters);
}

[[nodiscard]] inline PowerBisectorResult power_bisector_side(
    const ExactRational3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q,
    PredicateCounters* counters = nullptr) {
  PowerBisectorWitness materialized =
      materialize_power_bisector_witness(witness, r, q);
  const PredicateDecision decision =
      detail::multiprecision_decision(materialized.affine_value.sign(), counters);
  return PowerBisectorResult{decision, std::move(materialized)};
}

[[nodiscard]] inline PowerBisectorResult power_bisector_side(
    const CertifiedPoint3& witness,
    const ExactLabelMoments& r,
    const ExactLabelMoments& q,
    PredicateCounters* counters = nullptr) {
  return power_bisector_side(witness.exact(), r, q, counters);
}

}  // namespace morsehgp3d::exact
