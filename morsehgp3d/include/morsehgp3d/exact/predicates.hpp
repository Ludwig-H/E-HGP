#pragma once

#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/plane.hpp"
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

struct Orientation2DResult {
  PredicateDecision decision;
  ExactRational orientation_value;
};

struct PlaneSideResult {
  PredicateDecision decision;
  ExactRational signed_value;
};

struct FourthPlaneIncidenceResult {
  PredicateDecision decision;
  ExactRational3 intersection;
  ExactRational signed_value;
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

// Sign convention: n dot ((b-a) cross (c-a)), where n is the oriented
// ExactPlane3 normal. Every input point must be exactly incident to the plane.
[[nodiscard]] inline ExactRational orientation_2d_determinant(
    const ExactPlane3& plane,
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c) {
  if (!plane.evaluate(a).is_zero() || !plane.evaluate(b).is_zero() ||
      !plane.evaluate(c).is_zero()) {
    throw std::invalid_argument(
        "orientation_2d points must be exactly incident to the support plane");
  }
  std::array<ExactRational, 3> u{};
  std::array<ExactRational, 3> v{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    u[axis] = b.coordinate(axis) - a.coordinate(axis);
    v[axis] = c.coordinate(axis) - a.coordinate(axis);
  }
  const std::array<ExactRational, 3> cross{
      u[1] * v[2] - u[2] * v[1],
      u[2] * v[0] - u[0] * v[2],
      u[0] * v[1] - u[1] * v[0]};
  ExactRational result;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    result = result + ExactRational{plane.coefficient(axis)} * cross[axis];
  }
  return result;
}

[[nodiscard]] inline ExactRational orientation_2d_determinant(
    const ExactPlane3& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c) {
  return orientation_2d_determinant(plane, a.exact(), b.exact(), c.exact());
}

[[nodiscard]] inline PredicateDecision decide_orientation_2d_in_plane(
    const ExactPlane3& plane,
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c,
    PredicateCounters* counters = nullptr) {
  return detail::multiprecision_decision(
      orientation_2d_determinant(plane, a, b, c).sign(), counters);
}

[[nodiscard]] inline PredicateDecision decide_orientation_2d_in_plane(
    const ExactPlane3& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    PredicateCounters* counters = nullptr) {
  return decide_orientation_2d_in_plane(
      plane, a.exact(), b.exact(), c.exact(), counters);
}

[[nodiscard]] inline Orientation2DResult orientation_2d_in_plane(
    const ExactPlane3& plane,
    const ExactRational3& a,
    const ExactRational3& b,
    const ExactRational3& c,
    PredicateCounters* counters = nullptr) {
  ExactRational orientation_value = orientation_2d_determinant(plane, a, b, c);
  const PredicateDecision decision =
      detail::multiprecision_decision(orientation_value.sign(), counters);
  return Orientation2DResult{decision, std::move(orientation_value)};
}

[[nodiscard]] inline Orientation2DResult orientation_2d_in_plane(
    const ExactPlane3& plane,
    const CertifiedPoint3& a,
    const CertifiedPoint3& b,
    const CertifiedPoint3& c,
    PredicateCounters* counters = nullptr) {
  return orientation_2d_in_plane(
      plane, a.exact(), b.exact(), c.exact(), counters);
}

[[nodiscard]] inline PredicateDecision decide_plane_side(
    const ExactPlane3& plane,
    const ExactRational3& point,
    PredicateCounters* counters = nullptr) {
  return detail::multiprecision_decision(plane.evaluate(point).sign(), counters);
}

[[nodiscard]] inline PredicateDecision decide_plane_side(
    const ExactPlane3& plane,
    const CertifiedPoint3& point,
    PredicateCounters* counters = nullptr) {
  return decide_plane_side(plane, point.exact(), counters);
}

[[nodiscard]] inline PlaneSideResult plane_side(
    const ExactPlane3& plane,
    const ExactRational3& point,
    PredicateCounters* counters = nullptr) {
  ExactRational signed_value = plane.evaluate(point);
  const PredicateDecision decision =
      detail::multiprecision_decision(signed_value.sign(), counters);
  return PlaneSideResult{decision, std::move(signed_value)};
}

[[nodiscard]] inline PlaneSideResult plane_side(
    const ExactPlane3& plane,
    const CertifiedPoint3& point,
    PredicateCounters* counters = nullptr) {
  return plane_side(plane, point.exact(), counters);
}

// The homogeneous 4x4 determinant equals det(N) times the fourth-plane value
// at the unique intersection of the first three planes. Multiplying its sign
// by sign(det(N)) therefore decides the oriented side without a division.
[[nodiscard]] inline int fourth_plane_incidence_determinant_sign(
    const ExactPlane3& first,
    const ExactPlane3& second,
    const ExactPlane3& third,
    const ExactPlane3& fourth) {
  const std::array<const ExactPlane3*, 4> planes{
      &first, &second, &third, &fourth};
  std::array<std::array<BigInt, 3>, 3> binding_normals{};
  std::array<std::array<BigInt, 4>, 4> homogeneous{};
  for (std::size_t row = 0; row < planes.size(); ++row) {
    for (std::size_t column = 0; column < 4U; ++column) {
      homogeneous[row][column] = planes[row]->coefficient(column);
      if (row < 3U && column < 3U) {
        binding_normals[row][column] = planes[row]->coefficient(column);
      }
    }
  }
  const BigInt binding_determinant = detail::determinant_3x3(binding_normals);
  if (binding_determinant == 0) {
    throw std::invalid_argument(
        "fourth-plane incidence requires a unique first-three-plane intersection");
  }
  const BigInt homogeneous_determinant = detail::determinant_4x4(homogeneous);
  const int binding_sign = binding_determinant < 0 ? -1 : 1;
  const int homogeneous_sign = homogeneous_determinant < 0
                                   ? -1
                                   : (homogeneous_determinant == 0 ? 0 : 1);
  return binding_sign * homogeneous_sign;
}

[[nodiscard]] inline PredicateDecision decide_fourth_plane_incidence(
    const ExactPlane3& first,
    const ExactPlane3& second,
    const ExactPlane3& third,
    const ExactPlane3& fourth,
    PredicateCounters* counters = nullptr) {
  return detail::multiprecision_decision(
      fourth_plane_incidence_determinant_sign(first, second, third, fourth),
      counters);
}

[[nodiscard]] inline FourthPlaneIncidenceResult fourth_plane_incidence(
    const ExactPlane3& first,
    const ExactPlane3& second,
    const ExactPlane3& third,
    const ExactPlane3& fourth,
    PredicateCounters* counters = nullptr) {
  const int determinant_sign = fourth_plane_incidence_determinant_sign(
      first, second, third, fourth);
  const ThreePlaneIntersection intersection =
      intersect_three_planes(first, second, third);
  if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
      !intersection.point().has_value()) {
    throw std::invalid_argument(
        "fourth-plane incidence requires a unique first-three-plane intersection");
  }
  ExactRational signed_value = fourth.evaluate(*intersection.point());
  if (signed_value.sign() != determinant_sign) {
    throw std::logic_error(
        "fourth-plane determinant contradicted its exact rational witness");
  }
  const PredicateDecision decision =
      detail::multiprecision_decision(signed_value.sign(), counters);
  return FourthPlaneIncidenceResult{
      decision, *intersection.point(), std::move(signed_value)};
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
[[nodiscard]] inline ExactAffineForm3 power_bisector_affine_form(
    const ExactLabelMoments& r,
    const ExactLabelMoments& q) {
  detail::require_power_label_domain(r, q);
  std::array<ExactRational, 4> coefficients{};
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    coefficients[axis] = ExactRational{BigInt{-2}} *
                         (r.coordinate_sum(axis) - q.coordinate_sum(axis));
  }
  coefficients[3] = r.squared_norm_sum() - q.squared_norm_sum();
  return ExactAffineForm3::from_rational_coefficients(coefficients);
}

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
