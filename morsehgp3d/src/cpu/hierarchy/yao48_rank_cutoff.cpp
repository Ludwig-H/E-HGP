#include "morsehgp3d/hierarchy/yao48_rank_cutoff.hpp"

#include "morsehgp3d/hierarchy/yao48_cone.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] exact::ExactLevel scaled_level(
    const exact::ExactLevel& level,
    unsigned int factor) {
  return exact::ExactLevel{
      exact::BigInt{factor} * level.numerator(), level.denominator()};
}

[[nodiscard]] exact::ExactLevel squared_level(
    const exact::ExactRational& value) {
  return exact::ExactLevel{value * value};
}

}  // namespace

bool exact_yao48_directional_witness_radius_cutoff_certifies(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& witness_squared_radius_upper_bound,
    ExactYao48DirectionalCutoffSemantics semantics) {
  if (!(witness_squared_radius_upper_bound > exact::ExactLevel{})) {
    throw std::invalid_argument(
        "a Yao48 rank cutoff requires a positive witness radius bound");
  }
  const ExactYao48ConeKey key = classify_exact_yao48_cone(anchor, target);
  std::array<exact::ExactRational, 3> oriented_deltas{};
  for (std::size_t position = 0U; position < 3U; ++position) {
    const std::size_t axis = static_cast<std::size_t>(
        key.axes_by_descending_absolute_delta[position]);
    exact::ExactRational delta =
        target.coordinate(axis) - anchor.coordinate(axis);
    if (delta.sign() < 0) {
      delta = -delta;
    }
    oriented_deltas[position] = std::move(delta);
  }
  const exact::ExactRational first = oriented_deltas[0];
  const exact::ExactRational first_two = first + oriented_deltas[1];
  const exact::ExactRational all_three =
      first_two + oriented_deltas[2];
  const exact::ExactLevel twice_radius =
      scaled_level(witness_squared_radius_upper_bound, 2U);
  const exact::ExactLevel three_times_radius =
      scaled_level(witness_squared_radius_upper_bound, 3U);
  switch (semantics) {
    case ExactYao48DirectionalCutoffSemantics::closed_ball:
      return squared_level(first) >= witness_squared_radius_upper_bound &&
          squared_level(first_two) >= twice_radius &&
          squared_level(all_three) >= three_times_radius;
    case ExactYao48DirectionalCutoffSemantics::strict_interior:
      return squared_level(first) > witness_squared_radius_upper_bound &&
          squared_level(first_two) > twice_radius &&
          squared_level(all_three) > three_times_radius;
  }
  throw std::invalid_argument(
      "a Yao48 rank cutoff has an invalid semantic mode");
}

bool exact_yao48_directional_rank_cutoff_certifies_above(
    const exact::CertifiedPoint3& anchor,
    const exact::CertifiedPoint3& target,
    const exact::ExactLevel& kth_squared_distance) {
  return exact_yao48_directional_witness_radius_cutoff_certifies(
      anchor,
      target,
      kth_squared_distance,
      ExactYao48DirectionalCutoffSemantics::closed_ball);
}

}  // namespace morsehgp3d::hierarchy
