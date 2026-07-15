#pragma once

#include "morsehgp3d/exact/fp64_interval.hpp"
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

[[nodiscard]] inline PredicateDecision filtered_decision(
    PredicateSign sign, PredicateCounters* counters) {
  const PredicateDecision decision{sign, CertificationStage::fp64_filtered};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

inline void require_filter_policy(PredicateFilterPolicy policy) {
  if (policy != PredicateFilterPolicy::allow_fp64 &&
      policy != PredicateFilterPolicy::multiprecision_only) {
    throw std::invalid_argument("predicate filter policy is invalid");
  }
}

[[nodiscard]] inline PredicateDecision certify_materialized_sign(
    const FilterResult& filtered,
    PredicateSign exact_sign,
    PredicateFilterPolicy policy,
    PredicateCounters* counters) {
  require_filter_policy(policy);
  if (policy == PredicateFilterPolicy::allow_fp64 &&
      filtered.state() == FilterState::certified) {
    if (!filtered.sign().has_value() || *filtered.sign() != exact_sign) {
      throw std::runtime_error(
          "fp64 filter contradicted its exact diagnostic witness");
    }
    return filtered_decision(*filtered.sign(), counters);
  }
  return multiprecision_decision(static_cast<int>(exact_sign), counters);
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

[[nodiscard]] inline FilterResult filter_squared_distance_order(
    const CertifiedPoint3& witness,
    const CertifiedPoint3& left,
    const CertifiedPoint3& right) {
  detail::Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }

  detail::Binary64Interval left_squared =
      detail::point_binary64_interval(0.0);
  detail::Binary64Interval right_squared =
      detail::point_binary64_interval(0.0);
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const detail::Binary64Interval witness_coordinate =
        detail::point_binary64_interval(witness.binary64_coordinate(axis));
    const detail::Binary64Interval left_delta =
        detail::subtract_binary64_intervals(
            witness_coordinate,
            detail::point_binary64_interval(left.binary64_coordinate(axis)));
    const detail::Binary64Interval right_delta =
        detail::subtract_binary64_intervals(
            witness_coordinate,
            detail::point_binary64_interval(right.binary64_coordinate(axis)));
    left_squared = detail::add_binary64_intervals(
        left_squared, detail::square_binary64_interval(left_delta));
    right_squared = detail::add_binary64_intervals(
        right_squared, detail::square_binary64_interval(right_delta));
  }
  const detail::Binary64Interval difference =
      detail::subtract_binary64_intervals(left_squared, right_squared);
  const FilterResult result = detail::sign_of_binary64_interval(difference);
  if (!environment.restore()) {
    return FilterResult::uncertain();
  }
  return result;
}

[[nodiscard]] inline FilterResult filter_orientation_3d(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d) {
  detail::Fp64EnvironmentGuard environment;
  if (!environment.supported()) {
    static_cast<void>(environment.restore());
    return FilterResult::uncertain();
  }

  std::array<detail::Binary64Interval, 3> u{};
  std::array<detail::Binary64Interval, 3> v{};
  std::array<detail::Binary64Interval, 3> w{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const detail::Binary64Interval origin =
        detail::point_binary64_interval(a.binary64_coordinate(axis));
    u[axis] = detail::subtract_binary64_intervals(
        detail::point_binary64_interval(b.binary64_coordinate(axis)), origin);
    v[axis] = detail::subtract_binary64_intervals(
        detail::point_binary64_interval(c.binary64_coordinate(axis)), origin);
    w[axis] = detail::subtract_binary64_intervals(
        detail::point_binary64_interval(d.binary64_coordinate(axis)), origin);
  }

  const auto product_difference = [](
      const detail::Binary64Interval& left_first,
      const detail::Binary64Interval& left_second,
      const detail::Binary64Interval& right_first,
      const detail::Binary64Interval& right_second) {
    return detail::subtract_binary64_intervals(
        detail::multiply_binary64_intervals(left_first, left_second),
        detail::multiply_binary64_intervals(right_first, right_second));
  };
  const detail::Binary64Interval first_minor =
      product_difference(v[1], w[2], v[2], w[1]);
  const detail::Binary64Interval second_minor =
      product_difference(v[0], w[2], v[2], w[0]);
  const detail::Binary64Interval third_minor =
      product_difference(v[0], w[1], v[1], w[0]);
  const detail::Binary64Interval first_term =
      detail::multiply_binary64_intervals(u[0], first_minor);
  const detail::Binary64Interval second_term =
      detail::multiply_binary64_intervals(u[1], second_minor);
  const detail::Binary64Interval third_term =
      detail::multiply_binary64_intervals(u[2], third_minor);
  const detail::Binary64Interval determinant = detail::add_binary64_intervals(
      detail::subtract_binary64_intervals(first_term, second_term), third_term);
  const FilterResult result = detail::sign_of_binary64_interval(determinant);
  if (!environment.restore()) {
    return FilterResult::uncertain();
  }
  return result;
}

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
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_fp64) {
  detail::require_filter_policy(filter_policy);
  if (filter_policy == PredicateFilterPolicy::allow_fp64) {
    const FilterResult filtered =
        filter_squared_distance_order(witness, left, right);
    if (filtered.state() == FilterState::certified) {
      return detail::filtered_decision(*filtered.sign(), counters);
    }
  }
  const ExactRational difference =
      squared_distance(witness, left) - squared_distance(witness, right);
  return detail::multiprecision_decision(difference.sign(), counters);
}

// Returns the sign of ||witness-left||^2 - ||witness-right||^2.
[[nodiscard]] inline DistanceComparisonResult compare_squared_distances(
    const CertifiedPoint3& witness,
    const CertifiedPoint3& left,
    const CertifiedPoint3& right,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_fp64) {
  detail::require_filter_policy(filter_policy);
  const FilterResult filtered =
      filter_policy == PredicateFilterPolicy::allow_fp64
          ? filter_squared_distance_order(witness, left, right)
          : FilterResult::uncertain();
  ExactLevel left_level{squared_distance(witness, left)};
  ExactLevel right_level{squared_distance(witness, right)};
  const PredicateSign exact_sign = predicate_sign(
      (left_level < right_level) ? -1 : ((right_level < left_level) ? 1 : 0));
  const PredicateDecision decision = detail::certify_materialized_sign(
      filtered, exact_sign, filter_policy, counters);
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
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_fp64) {
  detail::require_filter_policy(filter_policy);
  if (filter_policy == PredicateFilterPolicy::allow_fp64) {
    const FilterResult filtered = filter_orientation_3d(a, b, c, d);
    if (filtered.state() == FilterState::certified) {
      return detail::filtered_decision(*filtered.sign(), counters);
    }
  }
  return detail::multiprecision_decision(
      orientation_3d_determinant(a, b, c, d).sign(), counters);
}

[[nodiscard]] inline Orientation3DResult orientation_3d(
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    const CertifiedPoint3& d,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_fp64) {
  detail::require_filter_policy(filter_policy);
  const FilterResult filtered =
      filter_policy == PredicateFilterPolicy::allow_fp64
          ? filter_orientation_3d(a, b, c, d)
          : FilterResult::uncertain();
  ExactRational determinant = orientation_3d_determinant(a, b, c, d);
  const PredicateDecision decision = detail::certify_materialized_sign(
      filtered, predicate_sign(determinant.sign()), filter_policy, counters);
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
