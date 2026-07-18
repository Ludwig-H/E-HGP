#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/query.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::spatial::detail {

[[nodiscard]] inline exact::ExactRational3 validated_query(
    const exact::ExactRational3& query) {
  exact::ExactRational3 validated{
      query.numerator(0U),
      query.numerator(1U),
      query.numerator(2U),
      query.denominator()};
  if (validated != query) {
    throw std::invalid_argument("an exact spatial query must be canonical");
  }
  return validated;
}

[[nodiscard]] inline exact::ExactLevel validated_squared_radius(
    const exact::ExactLevel& squared_radius) {
  exact::ExactLevel validated{
      squared_radius.numerator(), squared_radius.denominator()};
  if (validated != squared_radius) {
    throw std::invalid_argument("an exact squared radius must be canonical");
  }
  return validated;
}

[[nodiscard]] inline exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& query,
    const exact::CertifiedPoint3& point) {
  const exact::ExactRational3& exact_point = point.exact();
  const exact::BigInt common_denominator =
      query.denominator() * exact_point.denominator();
  exact::BigInt squared_numerator = 0;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const exact::BigInt difference_numerator =
        exact_point.numerator(axis) * query.denominator() -
        query.numerator(axis) * exact_point.denominator();
    squared_numerator += difference_numerator * difference_numerator;
  }
  return exact::ExactLevel{
      std::move(squared_numerator), common_denominator * common_denominator};
}

[[nodiscard]] inline bool exact_neighbor_less(
    const ExactNeighbor& left,
    const ExactNeighbor& right) {
  if (left.squared_distance != right.squared_distance) {
    return left.squared_distance < right.squared_distance;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] inline exact::ExactLevel strict_level_margin(
    const exact::ExactLevel& greater,
    const exact::ExactLevel& lesser) {
  if (!(greater > lesser)) {
    throw std::logic_error("a pruning margin must be strictly positive");
  }
  return exact::ExactLevel{greater.rational() - lesser.rational()};
}

inline void record_strict_pruning_margin(
    SpatialQueryCounters& counters,
    const exact::ExactLevel& greater,
    const exact::ExactLevel& lesser) {
  exact::ExactLevel margin = strict_level_margin(greater, lesser);
  if (!counters.minimum_strict_pruning_margin.has_value() ||
      margin < *counters.minimum_strict_pruning_margin) {
    counters.minimum_strict_pruning_margin = std::move(margin);
  }
}

}  // namespace morsehgp3d::spatial::detail
