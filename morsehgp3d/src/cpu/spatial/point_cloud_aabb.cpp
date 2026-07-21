#include "morsehgp3d/spatial/point_cloud_aabb.hpp"

#include "morsehgp3d/exact/binary64_neighbors.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::spatial {
namespace {

[[nodiscard]] std::uint64_t checked_product(
    std::size_t count,
    std::uint64_t factor,
    const char* label) {
  if (factor == 0U ||
      static_cast<std::uintmax_t>(count) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<std::uint64_t>::max() / factor)) {
    throw std::length_error(label);
  }
  return static_cast<std::uint64_t>(count) * factor;
}

[[nodiscard]] std::uint8_t axis_bit(std::size_t axis) {
  if (axis >= 3U) {
    throw std::logic_error("a clipping-box axis must be 0, 1 or 2");
  }
  return static_cast<std::uint8_t>(
      std::uint32_t{1} << static_cast<unsigned int>(axis));
}

[[nodiscard]] exact::ExactRational exact_word(std::uint64_t bits) {
  return exact::ExactRational::from_binary64_bits(bits);
}

[[nodiscard]] bool canonical_finite_word(std::uint64_t bits) {
  try {
    return exact::canonicalize_binary64_bits(bits) == bits;
  } catch (const std::exception&) {
    return false;
  }
}

[[nodiscard]] bool all_sites_strictly_inside(
    const CanonicalPointCloud& cloud,
    const ExactDyadicAabb3& omega) {
  std::array<exact::ExactRational, 3> lower{};
  std::array<exact::ExactRational, 3> upper{};
  try {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      lower[axis] = exact_word(omega.lower_binary64_bits[axis]);
      upper[axis] = exact_word(omega.upper_binary64_bits[axis]);
      if (!(lower[axis] < upper[axis])) {
        return false;
      }
    }
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      const PointId point_id = static_cast<PointId>(point_index);
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        const exact::ExactRational coordinate =
            cloud.point(point_id).coordinate(axis);
        if (!(lower[axis] < coordinate && coordinate < upper[axis])) {
          return false;
        }
      }
    }
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

[[nodiscard]] StrictlyPaddedDyadicAabb3Result build_unverified(
    const CanonicalPointCloud& cloud) {
  const ExactPointCloudAabb3 exact_bounds =
      build_exact_point_cloud_aabb(cloud);

  StrictlyPaddedDyadicAabb3Result result;
  result.audit = exact_bounds.audit;
  std::array<std::optional<std::uint64_t>, 3> padded_lower{};
  std::array<std::optional<std::uint64_t>, 3> padded_upper{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    padded_lower[axis] = exact::previous_canonical_finite_binary64_bits(
        exact_bounds.bounds.lower_binary64_bits[axis]);
    padded_upper[axis] = exact::next_canonical_finite_binary64_bits(
        exact_bounds.bounds.upper_binary64_bits[axis]);
    if (!padded_lower[axis].has_value()) {
      result.unavailable_lower_axis_mask = static_cast<std::uint8_t>(
          result.unavailable_lower_axis_mask | axis_bit(axis));
    }
    if (!padded_upper[axis].has_value()) {
      result.unavailable_upper_axis_mask = static_cast<std::uint8_t>(
          result.unavailable_upper_axis_mask | axis_bit(axis));
    }
  }

  if (result.unavailable_lower_axis_mask != 0U ||
      result.unavailable_upper_axis_mask != 0U) {
    result.decision =
        StrictDyadicPaddingDecision::unsupported_finite_binary64_range;
    return result;
  }

  StrictlyPaddedDyadicAabb3Certificate certificate;
  certificate.exact_site_aabb = exact_bounds;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    certificate.omega.lower_binary64_bits[axis] =
        padded_lower[axis].value();
    certificate.omega.upper_binary64_bits[axis] =
        padded_upper[axis].value();
    certificate.lower_padding[axis] =
        exact_word(exact_bounds.bounds.lower_binary64_bits[axis]) -
        exact_word(certificate.omega.lower_binary64_bits[axis]);
    certificate.upper_padding[axis] =
        exact_word(certificate.omega.upper_binary64_bits[axis]) -
        exact_word(exact_bounds.bounds.upper_binary64_bits[axis]);
  }
  certificate.all_sites_strictly_inside_certified = true;
  certificate.convex_hull_strictly_inside_certified = true;
  result.decision = StrictDyadicPaddingDecision::complete;
  result.certificate.emplace(std::move(certificate));
  return result;
}

}  // namespace

std::string_view to_string(StrictDyadicPaddingDecision decision) {
  switch (decision) {
    case StrictDyadicPaddingDecision::complete:
      return "complete";
    case StrictDyadicPaddingDecision::unsupported_finite_binary64_range:
      return "unsupported_finite_binary64_range";
  }
  throw std::invalid_argument("unknown strict dyadic padding decision");
}

ExactPointCloudAabb3 build_exact_point_cloud_aabb(
    const CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "an exact point-cloud AABB requires a nonempty canonical cloud");
  }

  ExactPointCloudAabb3 result;
  result.audit.point_count = point_count;
  result.audit.exact_coordinate_evaluation_count = checked_product(
      point_count,
      std::uint64_t{3},
      "the exact point-cloud coordinate count overflows uint64");
  result.audit.exact_extremum_comparison_count = checked_product(
      point_count - 1U,
      std::uint64_t{6},
      "the exact point-cloud extremum comparison count overflows uint64");

  std::array<exact::ExactRational, 3> lower_coordinates{};
  std::array<exact::ExactRational, 3> upper_coordinates{};
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    const std::array<std::uint64_t, 3> bits =
        cloud.point(point_id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const exact::ExactRational coordinate = exact_word(bits[axis]);
      if (point_index == 0U) {
        lower_coordinates[axis] = coordinate;
        upper_coordinates[axis] = coordinate;
        result.bounds.lower_binary64_bits[axis] = bits[axis];
        result.bounds.upper_binary64_bits[axis] = bits[axis];
        result.lower_witness_point_ids[axis] = point_id;
        result.upper_witness_point_ids[axis] = point_id;
        continue;
      }
      if (coordinate < lower_coordinates[axis]) {
        lower_coordinates[axis] = coordinate;
        result.bounds.lower_binary64_bits[axis] = bits[axis];
        result.lower_witness_point_ids[axis] = point_id;
      }
      if (coordinate > upper_coordinates[axis]) {
        upper_coordinates[axis] = coordinate;
        result.bounds.upper_binary64_bits[axis] = bits[axis];
        result.upper_witness_point_ids[axis] = point_id;
      }
    }
  }
  return result;
}

StrictlyPaddedDyadicAabb3Verification verify_strictly_padded_dyadic_aabb(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& result) {
  const StrictlyPaddedDyadicAabb3Result expected = build_unverified(cloud);
  StrictlyPaddedDyadicAabb3Verification verification;
  verification.audit_certified = result.audit == expected.audit;
  verification.decision_certified = result.decision == expected.decision;
  verification.failure_masks_certified =
      result.unavailable_lower_axis_mask ==
          expected.unavailable_lower_axis_mask &&
      result.unavailable_upper_axis_mask ==
          expected.unavailable_upper_axis_mask;
  verification.payload_shape_certified =
      result.certificate.has_value() == expected.certificate.has_value() &&
      (result.decision == StrictDyadicPaddingDecision::complete) ==
          result.certificate.has_value();

  if (!expected.certificate.has_value()) {
    verification.result_certified =
        verification.audit_certified &&
        verification.decision_certified &&
        verification.failure_masks_certified &&
        verification.payload_shape_certified;
    return verification;
  }
  if (!result.certificate.has_value()) {
    return verification;
  }

  const StrictlyPaddedDyadicAabb3Certificate& observed_certificate =
      *result.certificate;
  const StrictlyPaddedDyadicAabb3Certificate& expected_certificate =
      *expected.certificate;
  verification.exact_extrema_certified =
      observed_certificate.exact_site_aabb.bounds ==
          expected_certificate.exact_site_aabb.bounds &&
      observed_certificate.exact_site_aabb.audit == expected.audit;
  verification.extremum_witnesses_certified =
      observed_certificate.exact_site_aabb.lower_witness_point_ids ==
          expected_certificate.exact_site_aabb.lower_witness_point_ids &&
      observed_certificate.exact_site_aabb.upper_witness_point_ids ==
          expected_certificate.exact_site_aabb.upper_witness_point_ids;

  bool all_words_are_canonical_and_finite = true;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    all_words_are_canonical_and_finite =
        all_words_are_canonical_and_finite &&
        canonical_finite_word(
            observed_certificate.omega.lower_binary64_bits[axis]) &&
        canonical_finite_word(
            observed_certificate.omega.upper_binary64_bits[axis]);
  }
  verification.finite_adjacent_padding_certified =
      all_words_are_canonical_and_finite &&
      observed_certificate.omega == expected_certificate.omega;

  verification.exact_positive_padding_certified =
      observed_certificate.lower_padding ==
          expected_certificate.lower_padding &&
      observed_certificate.upper_padding ==
          expected_certificate.upper_padding;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    verification.exact_positive_padding_certified =
        verification.exact_positive_padding_certified &&
        observed_certificate.lower_padding[axis].sign() > 0 &&
        observed_certificate.upper_padding[axis].sign() > 0;
  }

  verification.all_sites_strictly_inside_certified =
      observed_certificate.all_sites_strictly_inside_certified &&
      all_sites_strictly_inside(cloud, observed_certificate.omega);
  verification.convex_hull_strictly_inside_certified =
      observed_certificate.convex_hull_strictly_inside_certified &&
      verification.all_sites_strictly_inside_certified;
  verification.result_certified =
      verification.audit_certified &&
      verification.decision_certified &&
      verification.failure_masks_certified &&
      verification.payload_shape_certified &&
      verification.exact_extrema_certified &&
      verification.extremum_witnesses_certified &&
      verification.finite_adjacent_padding_certified &&
      verification.exact_positive_padding_certified &&
      verification.all_sites_strictly_inside_certified &&
      verification.convex_hull_strictly_inside_certified;
  return verification;
}

StrictlyPaddedDyadicAabb3Result build_strictly_padded_dyadic_aabb(
    const CanonicalPointCloud& cloud) {
  StrictlyPaddedDyadicAabb3Result result = build_unverified(cloud);
  const StrictlyPaddedDyadicAabb3Verification verification =
      verify_strictly_padded_dyadic_aabb(cloud, result);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the strict dyadic clipping-box certificate failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::spatial
