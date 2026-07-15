#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/level_order.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterKind;
using morsehgp3d::exact::CircumcenterResult;
using morsehgp3d::exact::CircumcenterSupportAnalysis;
using morsehgp3d::exact::CanonicalLevelBatchResult;
using morsehgp3d::exact::CanonicalSupportIds;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactLevelRecord;
using morsehgp3d::exact::ExactPlane3;
using morsehgp3d::exact::ExactPlane3Record;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::ExactRational3Record;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateFilterPolicy;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::SupportLevelEmission;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::classify_sphere_point;
using morsehgp3d::exact::classify_affine_form;
using morsehgp3d::exact::canonical_level_batches;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::compare_exact_levels;
using morsehgp3d::exact::circumcenter;
using morsehgp3d::exact::fourth_plane_incidence;
using morsehgp3d::exact::intersect_three_planes;
using morsehgp3d::exact::maximum_power_label_cardinality;
using morsehgp3d::exact::orientation_2d_in_plane;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::power_bisector_side;
using morsehgp3d::exact::power_bisector_affine_form;
using morsehgp3d::exact::to_string;

[[nodiscard]] std::optional<double> exact_binary64_value(
    const ExactRational& value) {
  if (value.is_zero()) {
    return 0.0;
  }

  const BigInt& denominator = value.denominator();
  if ((denominator & (denominator - 1)) != 0) {
    return std::nullopt;
  }

  const bool negative = value.numerator() < 0;
  const BigInt numerator =
      negative ? -value.numerator() : value.numerator();
  const auto denominator_exponent = boost::multiprecision::msb(denominator);
  const auto lowest_numerator_bit = boost::multiprecision::lsb(numerator);
  const auto highest_numerator_bit = boost::multiprecision::msb(numerator);
  if (highest_numerator_bit - lowest_numerator_bit + 1U > 53U) {
    return std::nullopt;
  }

  int highest_binary_exponent = 0;
  if (highest_numerator_bit >= denominator_exponent) {
    if (highest_numerator_bit - denominator_exponent > 1023U) {
      return std::nullopt;
    }
    highest_binary_exponent =
        static_cast<int>(highest_numerator_bit - denominator_exponent);
  } else if (denominator_exponent - highest_numerator_bit > 1074U) {
    return std::nullopt;
  } else {
    highest_binary_exponent =
        -static_cast<int>(denominator_exponent - highest_numerator_bit);
  }

  int binary_exponent = 0;
  if (lowest_numerator_bit >= denominator_exponent) {
    const auto difference = lowest_numerator_bit - denominator_exponent;
    if (difference > 1023U) {
      return std::nullopt;
    }
    binary_exponent = static_cast<int>(difference);
  } else {
    const auto difference = denominator_exponent - lowest_numerator_bit;
    if (difference > 1074U) {
      return std::nullopt;
    }
    binary_exponent = -static_cast<int>(difference);
  }

  const BigInt odd_significand = numerator >> lowest_numerator_bit;
  const std::uint64_t significand =
      odd_significand.convert_to<std::uint64_t>();
  constexpr std::uint64_t fraction_mask =
      (std::uint64_t{1} << 52U) - std::uint64_t{1};
  std::uint64_t bits = negative ? (std::uint64_t{1} << 63U) : 0U;
  if (highest_binary_exponent >= -1022) {
    const unsigned int significand_bits = static_cast<unsigned int>(
        highest_numerator_bit - lowest_numerator_bit + 1U);
    const unsigned int shift = 53U - significand_bits;
    const std::uint64_t normalized_significand = significand << shift;
    const auto exponent_bits =
        static_cast<std::uint64_t>(highest_binary_exponent + 1023);
    bits |= (exponent_bits << 52U) |
            (normalized_significand & fraction_mask);
  } else {
    const unsigned int shift =
        static_cast<unsigned int>(binary_exponent + 1074);
    bits |= significand << shift;
  }
  const double result = std::bit_cast<double>(bits);
  if (ExactRational::from_binary64(result) != value) {
    return std::nullopt;
  }
  return result;
}

[[nodiscard]] std::optional<CertifiedPoint3> exact_binary64_point(
    const ExactRational3& point) {
  std::array<double, 3> coordinates{};
  for (std::size_t axis = 0; axis < coordinates.size(); ++axis) {
    const std::optional<double> coordinate =
        exact_binary64_value(point.coordinate(axis));
    if (!coordinate.has_value()) {
      return std::nullopt;
    }
    coordinates[axis] = *coordinate;
  }
  return CertifiedPoint3::from_binary64(
      coordinates[0], coordinates[1], coordinates[2]);
}

std::string counters_json(const PredicateCounters& counters) {
  return "{\"cpu_multiprecision_certified\":" +
         std::to_string(counters.cpu_multiprecision_certified()) +
         ",\"exact_zeros\":" + std::to_string(counters.exact_zeros()) +
         ",\"expansion_certified\":" + std::to_string(counters.expansion_certified()) +
         ",\"fp32_proposals\":" + std::to_string(counters.fp32_proposals()) +
         ",\"fp64_filtered_certified\":" +
         std::to_string(counters.fp64_filtered_certified()) +
         ",\"remaining_unknown\":" + std::to_string(counters.remaining_unknown()) + "}";
}

std::string rational_json(const ExactRational& value) {
  return "{\"denominator\":\"" + canonical_integer_string(value.denominator()) +
         "\",\"numerator\":\"" + canonical_integer_string(value.numerator()) + "\"}";
}

std::string affine_form_json(const ExactAffineForm3& form) {
  return "{\"a\":" + rational_json(form.a()) +
         ",\"b\":" + rational_json(form.b()) +
         ",\"c\":" + rational_json(form.c()) +
         ",\"d\":" + rational_json(form.d()) +
         ",\"schema_version\":\"" + ExactAffineForm3::schema_version + "\"}";
}

ExactPlane3 parse_plane(
    std::span<const std::string_view> arguments, std::size_t offset) {
  if (offset > arguments.size() || arguments.size() - offset < 4U) {
    throw std::invalid_argument("a replay plane is incomplete");
  }
  return ExactPlane3::from_record(ExactPlane3Record{
      ExactPlane3::schema_version,
      std::string{arguments[offset]},
      std::string{arguments[offset + 1U]},
      std::string{arguments[offset + 2U]},
      std::string{arguments[offset + 3U]}});
}

ExactLevel parse_level(
    std::span<const std::string_view> arguments, std::size_t offset) {
  if (offset > arguments.size() || arguments.size() - offset < 2U) {
    throw std::invalid_argument("a replay exact level is incomplete");
  }
  return ExactLevel::from_record(ExactLevelRecord{
      ExactLevel::schema_version,
      std::string{arguments[offset]},
      std::string{arguments[offset + 1U]},
      ExactLevel::unit});
}

CertifiedPoint3 parse_point(
    std::span<const std::string_view> arguments, std::size_t offset) {
  if (offset > arguments.size() || arguments.size() - offset < 3U) {
    throw std::invalid_argument("a replay point is incomplete");
  }
  std::array<std::uint64_t, 3> bits{};
  for (std::size_t axis = 0; axis < bits.size(); ++axis) {
    bits[axis] = parse_binary64_hex(arguments[offset + axis], false);
  }
  return CertifiedPoint3::from_binary64_bits(bits);
}

std::size_t parse_size(std::string_view text, std::string_view label) {
  if (text.empty() || (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument(std::string{label} + " must be a canonical nonnegative integer");
  }
  std::uint64_t parsed = 0;
  const auto conversion = std::from_chars(text.data(), text.data() + text.size(), parsed);
  if (conversion.ec != std::errc{} || conversion.ptr != text.data() + text.size() ||
      parsed > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument(std::string{label} + " is outside the supported size range");
  }
  return static_cast<std::size_t>(parsed);
}

std::uint32_t parse_id(std::string_view text) {
  const std::size_t parsed = parse_size(text, "a point identifier");
  if (parsed > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument("a point identifier exceeds uint32");
  }
  return static_cast<std::uint32_t>(parsed);
}

std::uint64_t parse_point_id(std::string_view text) {
  if (text.empty() || (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument(
        "a support identifier must be a canonical nonnegative integer");
  }
  std::uint64_t parsed = 0;
  const auto conversion =
      std::from_chars(text.data(), text.data() + text.size(), parsed);
  if (conversion.ec != std::errc{} ||
      conversion.ptr != text.data() + text.size() ||
      parsed > CanonicalSupportIds::maximum_point_id) {
    throw std::invalid_argument(
        "a support identifier is outside the v2 PointId domain");
  }
  return parsed;
}

std::string support_ids_json(const CanonicalSupportIds& support) {
  std::string result{"["};
  for (std::size_t index = 0; index < support.size(); ++index) {
    if (index != 0U) {
      result += ',';
    }
    result += std::to_string(support.id(index));
  }
  result += ']';
  return result;
}

struct ParsedPowerLabels {
  ExactLabelMoments r;
  ExactLabelMoments q;
};

ParsedPowerLabels parse_power_labels(
    std::span<const std::string_view> arguments, std::size_t cursor) {
  if (cursor >= arguments.size()) {
    throw std::invalid_argument("the power-bisector point table size is missing");
  }
  const std::size_t point_count =
      parse_size(arguments[cursor], "point table size");
  ++cursor;
  if (point_count > (arguments.size() - cursor) / 3U) {
    throw std::invalid_argument("the power-bisector point table is truncated");
  }

  std::vector<CertifiedPoint3> point_table;
  point_table.reserve(point_count);
  for (std::size_t index = 0; index < point_count; ++index) {
    point_table.push_back(parse_point(arguments, cursor));
    cursor += 3U;
  }

  if (cursor >= arguments.size()) {
    throw std::invalid_argument("the power-bisector R label is missing");
  }
  const std::size_t r_count = parse_size(arguments[cursor], "R label size");
  ++cursor;
  if (r_count == 0U || r_count > maximum_power_label_cardinality) {
    throw std::invalid_argument(
        "a power-bisector label cardinality must be between one and ten");
  }
  if (r_count > arguments.size() - cursor) {
    throw std::invalid_argument("the power-bisector R label is truncated");
  }
  std::vector<std::uint32_t> r_ids;
  r_ids.reserve(r_count);
  for (std::size_t index = 0; index < r_count; ++index) {
    r_ids.push_back(parse_id(arguments[cursor]));
    ++cursor;
  }

  if (cursor >= arguments.size()) {
    throw std::invalid_argument("the power-bisector Q label is missing");
  }
  const std::size_t q_count = parse_size(arguments[cursor], "Q label size");
  ++cursor;
  if (q_count == 0U || q_count > maximum_power_label_cardinality) {
    throw std::invalid_argument(
        "a power-bisector label cardinality must be between one and ten");
  }
  if (q_count != r_count || q_count != arguments.size() - cursor) {
    throw std::invalid_argument(
        "power-bisector labels must have equal declared and serialized sizes");
  }
  std::vector<std::uint32_t> q_ids;
  q_ids.reserve(q_count);
  for (std::size_t index = 0; index < q_count; ++index) {
    q_ids.push_back(parse_id(arguments[cursor]));
    ++cursor;
  }

  return ParsedPowerLabels{
      ExactLabelMoments::from_canonical_ids(r_ids, point_table),
      ExactLabelMoments::from_canonical_ids(q_ids, point_table)};
}

int replay_distance(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  const CertifiedPoint3 witness = parse_point(arguments, 1U);
  const CertifiedPoint3 left = parse_point(arguments, 4U);
  const CertifiedPoint3 right = parse_point(arguments, 7U);
  PredicateCounters counters;
  const auto result = compare_squared_distances(
      witness, left, right, &counters, filter_policy);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"left_squared_distance\":" << result.left_squared_distance.canonical_json()
         << ",\"predicate\":\"compare_squared_distances\""
         << ",\"right_squared_distance\":" << result.right_squared_distance.canonical_json()
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_orientation(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  const CertifiedPoint3 a = parse_point(arguments, 1U);
  const CertifiedPoint3 b = parse_point(arguments, 4U);
  const CertifiedPoint3 c = parse_point(arguments, 7U);
  const CertifiedPoint3 d = parse_point(arguments, 10U);
  PredicateCounters counters;
  const auto result = orientation_3d(a, b, c, d, &counters, filter_policy);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"determinant_exact\":" << rational_json(result.determinant)
         << ",\"predicate\":\"orientation_3d\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_power_bisector(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.size() < 7U) {
    throw std::invalid_argument("a power-bisector replay is incomplete");
  }
  const ExactRational3 witness = ExactRational3::from_record(ExactRational3Record{
      ExactRational3::schema_version,
      std::string{arguments[1]},
      std::string{arguments[2]},
      std::string{arguments[3]},
      std::string{arguments[4]},
      ExactRational3::unit});
  const ParsedPowerLabels labels = parse_power_labels(arguments, 5U);
  PredicateCounters counters;
  const std::optional<CertifiedPoint3> binary64_witness =
      exact_binary64_point(witness);
  const auto result = binary64_witness.has_value()
                          ? power_bisector_side(
                                *binary64_witness,
                                labels.r,
                                labels.q,
                                &counters,
                                filter_policy)
                          : power_bisector_side(
                                witness, labels.r, labels.q, &counters);
  output << "{\"affine_value_exact\":" << rational_json(result.witness.affine_value)
         << ",\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"delta_coordinate_sum_exact\":"
         << result.witness.delta_coordinate_sum.canonical_json()
         << ",\"delta_squared_norm_sum_exact\":"
         << rational_json(result.witness.delta_squared_norm_sum)
         << ",\"predicate\":\"power_bisector_side\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_plane_through_points(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const ExactPlane3 plane = ExactPlane3::through_points(
      parse_point(arguments, 1U),
      parse_point(arguments, 4U),
      parse_point(arguments, 7U));
  output << "{\"plane\":" << plane.canonical_json()
         << ",\"predicate\":\"plane_through_points\"}\n";
  return 0;
}

int replay_power_bisector_affine_form(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const ParsedPowerLabels labels = parse_power_labels(arguments, 1U);
  const ExactAffineForm3 form = power_bisector_affine_form(labels.r, labels.q);
  const auto classification = classify_affine_form(form);
  output << "{\"affine_form\":" << affine_form_json(form)
         << ",\"classification\":\"" << to_string(classification.kind()) << "\"";
  if (classification.plane().has_value()) {
    output << ",\"plane\":" << classification.plane()->canonical_json();
  }
  output << ",\"predicate\":\"power_bisector_affine_form\"}\n";
  return 0;
}

int replay_orientation_2d(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const ExactPlane3 plane = parse_plane(arguments, 1U);
  const CertifiedPoint3 a = parse_point(arguments, 5U);
  const CertifiedPoint3 b = parse_point(arguments, 8U);
  const CertifiedPoint3 c = parse_point(arguments, 11U);
  PredicateCounters counters;
  const auto result = orientation_2d_in_plane(plane, a, b, c, &counters);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"orientation_value_exact\":"
         << rational_json(result.orientation_value)
         << ",\"predicate\":\"orientation_2d_in_plane\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_three_plane_intersection(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const auto result = intersect_three_planes(
      parse_plane(arguments, 1U),
      parse_plane(arguments, 5U),
      parse_plane(arguments, 9U));
  output << "{\"affine_dimension\":";
  if (result.affine_dimension().has_value()) {
    output << *result.affine_dimension();
  } else {
    output << "null";
  }
  output << ",\"augmented_rank\":" << result.augmented_rank()
         << ",\"intersection_exact\":";
  if (result.point().has_value()) {
    output << result.point()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"intersection_kind\":\"" << to_string(result.kind())
         << "\",\"normal_rank\":" << result.normal_rank()
         << ",\"predicate\":\"intersect_three_planes\"}\n";
  return 0;
}

int replay_fourth_plane_incidence(
    std::span<const std::string_view> arguments, std::ostream& output) {
  PredicateCounters counters;
  const auto result = fourth_plane_incidence(
      parse_plane(arguments, 1U),
      parse_plane(arguments, 5U),
      parse_plane(arguments, 9U),
      parse_plane(arguments, 13U),
      &counters);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"intersection_exact\":" << result.intersection.canonical_json()
         << ",\"predicate\":\"fourth_plane_incidence\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\""
         << ",\"signed_value_exact\":" << rational_json(result.signed_value)
         << "}\n";
  return 0;
}

int replay_circumcenter_support(
    std::span<const std::string_view> arguments, std::ostream& output) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument("the circumcenter support size is missing");
  }
  const std::size_t support_size =
      parse_size(arguments[1], "circumcenter support size");
  if (support_size < 2U || support_size > 4U) {
    throw std::invalid_argument(
        "a circumcenter support must contain two, three or four points");
  }
  if (arguments.size() != 2U + 3U * support_size) {
    throw std::invalid_argument(
        "the circumcenter support has the wrong serialized point count");
  }

  std::vector<CertifiedPoint3> support;
  support.reserve(support_size);
  for (std::size_t index = 0; index < support_size; ++index) {
    support.push_back(parse_point(arguments, 2U + 3U * index));
  }

  CircumcenterResult result = [&support, support_size] {
    if (support_size == 2U) {
      return circumcenter(support[0], support[1]);
    }
    if (support_size == 3U) {
      return circumcenter(support[0], support[1], support[2]);
    }
    return circumcenter(support[0], support[1], support[2], support[3]);
  }();

  const bool independent = result.kind() == CircumcenterKind::unique;
  output << "{\"affine_dimension\":" << result.affine_dimension()
         << ",\"center_exact\":";
  if (result.center().has_value()) {
    output << result.center()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"predicate\":\"circumcenter_support\""
         << ",\"squared_level_exact\":";
  if (result.squared_level().has_value()) {
    output << result.squared_level()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"support_kind\":\""
         << (independent ? "affinely_independent" : "affinely_dependent")
         << "\",\"support_size\":" << result.support_size() << "}\n";
  return 0;
}

int replay_circumcenter_support_analysis(
    std::span<const std::string_view> arguments, std::ostream& output) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument("the analyzed support size is missing");
  }
  const std::size_t support_size =
      parse_size(arguments[1], "analyzed support size");
  if (support_size < 1U || support_size > 4U) {
    throw std::invalid_argument(
        "a support analysis requires between one and four points");
  }
  if (arguments.size() != 2U + 3U * support_size) {
    throw std::invalid_argument(
        "the analyzed support has the wrong serialized point count");
  }

  std::vector<CertifiedPoint3> support;
  support.reserve(support_size);
  for (std::size_t index = 0; index < support_size; ++index) {
    support.push_back(parse_point(arguments, 2U + 3U * index));
  }

  PredicateCounters counters;
  CircumcenterSupportAnalysis analysis = [&support, support_size, &counters] {
    if (support_size == 1U) {
      return analyze_circumcenter_support(
          std::array<CertifiedPoint3, 1>{support[0]}, &counters);
    }
    if (support_size == 2U) {
      return analyze_circumcenter_support(
          std::array<CertifiedPoint3, 2>{support[0], support[1]}, &counters);
    }
    if (support_size == 3U) {
      return analyze_circumcenter_support(
          std::array<CertifiedPoint3, 3>{
              support[0], support[1], support[2]},
          &counters);
    }
    return analyze_circumcenter_support(
        std::array<CertifiedPoint3, 4>{
            support[0], support[1], support[2], support[3]},
        &counters);
  }();

  const CircumcenterResult& center = analysis.circumcenter_result();
  const bool independent = center.kind() == CircumcenterKind::unique;
  output << "{\"affine_dimension\":" << center.affine_dimension()
         << ",\"barycentric_coordinates_exact\":";
  if (analysis.barycentric().has_value()) {
    output << '[';
    for (std::size_t index = 0; index < support_size; ++index) {
      if (index != 0U) {
        output << ',';
      }
      output << rational_json(analysis.barycentric()->coordinate(index));
    }
    output << ']';
  } else {
    output << "null";
  }
  output << ",\"barycentric_signs\":";
  if (analysis.barycentric().has_value()) {
    output << '[';
    for (std::size_t index = 0; index < support_size; ++index) {
      if (index != 0U) {
        output << ',';
      }
      output << '\"' << to_string(analysis.barycentric()->sign(index)) << '\"';
    }
    output << ']';
  } else {
    output << "null";
  }
  output << ",\"center_exact\":";
  if (center.center().has_value()) {
    output << center.center()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"certification_stage\":";
  if (independent) {
    output << "\"cpu_multiprecision\"";
  } else {
    output << "null";
  }
  output << ",\"convex_hull_location\":";
  if (analysis.barycentric().has_value()) {
    output << '\"' << to_string(analysis.barycentric()->location()) << '\"';
  } else {
    output << "null";
  }
  output << ",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"circumcenter_support_analysis\""
         << ",\"reduced_support_indices\":";
  if (analysis.reduced_support_mask().has_value()) {
    output << '[';
    bool first = true;
    for (std::size_t index = 0; index < support_size; ++index) {
      if (analysis.reduced_support_contains(index)) {
        if (!first) {
          output << ',';
        }
        output << index;
        first = false;
      }
    }
    output << ']';
  } else {
    output << "null";
  }
  output << ",\"squared_level_exact\":";
  if (center.squared_level().has_value()) {
    output << center.squared_level()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"support_kind\":\""
         << (independent ? "affinely_independent" : "affinely_dependent")
         << "\",\"support_size\":" << support_size
         << ",\"support_status\":\"" << to_string(analysis.status())
         << "\"}\n";
  return 0;
}

int replay_sphere_side(
    std::span<const std::string_view> arguments, std::ostream& output) {
  if (arguments.size() != 10U) {
    throw std::invalid_argument(
        "sphere-side replay requires one center, one level and one point");
  }
  const ExactRational3 center = ExactRational3::from_record(ExactRational3Record{
      ExactRational3::schema_version,
      std::string{arguments[1]},
      std::string{arguments[2]},
      std::string{arguments[3]},
      std::string{arguments[4]},
      ExactRational3::unit});
  const ExactLevel squared_level = ExactLevel::from_record(ExactLevelRecord{
      ExactLevel::schema_version,
      std::string{arguments[5]},
      std::string{arguments[6]},
      ExactLevel::unit});
  PredicateCounters counters;
  const auto result = classify_sphere_point(
      center, squared_level, parse_point(arguments, 7U), &counters);
  output << "{\"certification_stage\":\""
         << to_string(result.decision().certification_stage())
         << "\",\"classification\":\"" << to_string(result.location())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"sphere_side\""
         << ",\"sign\":\"" << to_string(result.decision().sign()) << "\""
         << ",\"signed_offset_exact\":" << rational_json(result.signed_power())
         << ",\"squared_distance_exact\":"
         << result.point_squared_distance().canonical_json() << "}\n";
  return 0;
}

int replay_exact_level_comparison(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const ExactLevel left = parse_level(arguments, 1U);
  const ExactLevel right = parse_level(arguments, 3U);
  PredicateCounters counters;
  const auto result = compare_exact_levels(left, right, &counters);
  const PredicateSign sign = result.decision.sign();
  const char* ordering = sign == PredicateSign::negative
                             ? "less"
                             : sign == PredicateSign::zero ? "equal" : "greater";
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"cross_product_difference_exact\":\""
         << canonical_integer_string(result.cross_product_difference)
         << "\",\"equal\":" << (sign == PredicateSign::zero ? "true" : "false")
         << ",\"ordering\":\"" << ordering
         << "\",\"predicate\":\"compare_exact_levels\""
         << ",\"sign\":\"" << to_string(sign) << "\"}\n";
  return 0;
}

int replay_canonical_level_batches(
    std::span<const std::string_view> arguments, std::ostream& output) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument("the canonical level-batch item count is missing");
  }
  const std::size_t emission_count =
      parse_size(arguments[1], "canonical level-batch item count");
  if (emission_count < 1U || emission_count > 64U) {
    throw std::invalid_argument(
        "a canonical level batch requires between one and 64 emissions");
  }

  std::vector<SupportLevelEmission> emissions;
  emissions.reserve(emission_count);
  std::size_t cursor = 2U;
  for (std::size_t emission_index = 0U;
       emission_index < emission_count;
       ++emission_index) {
    if (cursor > arguments.size() || arguments.size() - cursor < 3U) {
      throw std::invalid_argument("a canonical level-batch emission is incomplete");
    }
    ExactLevel level = parse_level(arguments, cursor);
    cursor += 2U;
    const std::size_t minimal_size =
        parse_size(arguments[cursor], "minimal support size");
    ++cursor;
    if (minimal_size < 1U || minimal_size > 4U ||
        minimal_size > arguments.size() - cursor) {
      throw std::invalid_argument(
          "a minimal support must contain between one and four identifiers");
    }
    std::vector<std::uint64_t> minimal_ids;
    minimal_ids.reserve(minimal_size);
    for (std::size_t index = 0U; index < minimal_size; ++index) {
      minimal_ids.push_back(parse_point_id(arguments[cursor]));
      ++cursor;
    }
    if (cursor >= arguments.size()) {
      throw std::invalid_argument("a source support size is missing");
    }
    const std::size_t source_size =
        parse_size(arguments[cursor], "source support size");
    ++cursor;
    if (source_size < 1U || source_size > 4U ||
        source_size > arguments.size() - cursor) {
      throw std::invalid_argument(
          "a source support must contain between one and four identifiers");
    }
    std::vector<std::uint64_t> source_ids;
    source_ids.reserve(source_size);
    for (std::size_t index = 0U; index < source_size; ++index) {
      source_ids.push_back(parse_point_id(arguments[cursor]));
      ++cursor;
    }
    emissions.push_back(SupportLevelEmission::create(
        std::move(level),
        CanonicalSupportIds::from_ids(minimal_ids),
        CanonicalSupportIds::from_ids(source_ids)));
  }
  if (cursor != arguments.size()) {
    throw std::invalid_argument(
        "the canonical level batch has trailing serialized fields");
  }

  const CanonicalLevelBatchResult result = canonical_level_batches(emissions);
  output << "{\"duplicate_emission_count\":"
         << result.duplicate_emission_count
         << ",\"emission_count\":" << result.emission_count
         << ",\"equal_level_batches\":[";
  for (std::size_t batch_index = 0U;
       batch_index < result.batches.size();
       ++batch_index) {
    if (batch_index != 0U) {
      output << ',';
    }
    const auto& batch = result.batches[batch_index];
    output << "{\"emission_count\":" << batch.emission_count
           << ",\"squared_level_exact\":" << batch.squared_level.canonical_json()
           << ",\"supports\":[";
    for (std::size_t support_index = 0U;
         support_index < batch.supports.size();
         ++support_index) {
      if (support_index != 0U) {
        output << ',';
      }
      const auto& support = batch.supports[support_index];
      output << "{\"emission_count\":" << support.emission_count
             << ",\"minimal_support_ids\":"
             << support_ids_json(support.minimal_support_ids)
             << ",\"source_provenance\":[";
      for (std::size_t source_index = 0U;
           source_index < support.source_provenance.size();
           ++source_index) {
        if (source_index != 0U) {
          output << ',';
        }
        const auto& provenance = support.source_provenance[source_index];
        output << "{\"emission_count\":" << provenance.emission_count
               << ",\"source_support_ids\":"
               << support_ids_json(provenance.source_support_ids) << '}';
      }
      output << "],\"squared_level_exact\":"
             << support.squared_level.canonical_json() << '}';
    }
    output << "]}";
  }
  output << "],\"predicate\":\"canonical_level_batches\""
         << ",\"unique_emission_count\":" << result.unique_emission_count
         << "}\n";
  return 0;
}

int replay_tokens(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.empty()) {
    throw std::invalid_argument("a predicate name is required");
  }
  if (arguments[0] == "compare_squared_distances" && arguments.size() == 10U) {
    return replay_distance(arguments, output, filter_policy);
  }
  if (arguments[0] == "orientation_3d" && arguments.size() == 13U) {
    return replay_orientation(arguments, output, filter_policy);
  }
  if (arguments[0] == "power_bisector_side") {
    return replay_power_bisector(arguments, output, filter_policy);
  }
  if (arguments[0] == "plane_through_points" && arguments.size() == 10U) {
    return replay_plane_through_points(arguments, output);
  }
  if (arguments[0] == "power_bisector_affine_form") {
    return replay_power_bisector_affine_form(arguments, output);
  }
  if (arguments[0] == "orientation_2d_in_plane" && arguments.size() == 14U) {
    return replay_orientation_2d(arguments, output);
  }
  if (arguments[0] == "intersect_three_planes" && arguments.size() == 13U) {
    return replay_three_plane_intersection(arguments, output);
  }
  if (arguments[0] == "fourth_plane_incidence" && arguments.size() == 17U) {
    return replay_fourth_plane_incidence(arguments, output);
  }
  if (arguments[0] == "circumcenter_support") {
    return replay_circumcenter_support(arguments, output);
  }
  if (arguments[0] == "circumcenter_support_analysis") {
    return replay_circumcenter_support_analysis(arguments, output);
  }
  if (arguments[0] == "sphere_side") {
    return replay_sphere_side(arguments, output);
  }
  if (arguments[0] == "compare_exact_levels" && arguments.size() == 5U) {
    return replay_exact_level_comparison(arguments, output);
  }
  if (arguments[0] == "canonical_level_batches") {
    return replay_canonical_level_batches(arguments, output);
  }
  throw std::invalid_argument("the predicate name or argument count is unsupported");
}

int replay_batch(
    std::istream& input,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    std::istringstream stream{line};
    std::vector<std::string> storage;
    std::string token;
    while (stream >> token) {
      storage.push_back(std::move(token));
    }
    if (storage.empty()) {
      throw std::invalid_argument(
          "batch replay line " + std::to_string(line_number) + " is empty");
    }
    std::vector<std::string_view> arguments;
    arguments.reserve(storage.size());
    for (const std::string& value : storage) {
      arguments.emplace_back(value);
    }
    try {
      static_cast<void>(replay_tokens(arguments, output, filter_policy));
    } catch (const std::exception& error) {
      throw std::invalid_argument(
          "batch replay line " + std::to_string(line_number) + ": " + error.what());
    }
  }
  if (!input.eof()) {
    throw std::runtime_error("batch replay input could not be read completely");
  }
  output.flush();
  if (!output) {
    throw std::runtime_error("batch replay output could not be written completely");
  }
  return 0;
}

void print_usage(const char* executable) {
  std::cerr << "usage: " << executable
            << " compare_squared_distances HEX_X HEX_Y HEX_Z ... (3 points)\n"
            << "   or: " << executable
            << " orientation_3d HEX_X HEX_Y HEX_Z ... (4 points)\n"
            << "   or: " << executable
            << " power_bisector_side XN YN ZN D POINT_COUNT ... R_COUNT ... Q_COUNT ...\n"
            << "   or: " << executable
            << " plane_through_points HEX_X HEX_Y HEX_Z ... (3 points)\n"
            << "   or: " << executable
            << " power_bisector_affine_form POINT_COUNT ... R_COUNT ... Q_COUNT ...\n"
            << "   or: " << executable
            << " orientation_2d_in_plane A B C D HEX_X HEX_Y HEX_Z ... (3 points)\n"
            << "   or: " << executable
            << " intersect_three_planes A B C D ... (3 planes)\n"
            << "   or: " << executable
            << " fourth_plane_incidence A B C D ... (4 planes)\n"
            << "   or: " << executable
            << " circumcenter_support COUNT HEX_X HEX_Y HEX_Z ... (2 to 4 points)\n"
            << "   or: " << executable
            << " circumcenter_support_analysis COUNT HEX_X HEX_Y HEX_Z ... (1 to 4 points)\n"
            << "   or: " << executable
            << " sphere_side XN YN ZN D LEVEL_N LEVEL_D HEX_X HEX_Y HEX_Z\n"
            << "   or: " << executable
            << " compare_exact_levels LEFT_N LEFT_D RIGHT_N RIGHT_D\n"
            << "   or: " << executable
            << " canonical_level_batches COUNT LEVEL_N LEVEL_D MIN_COUNT ... SOURCE_COUNT ...\n"
            << "   or: " << executable << " --batch < predicate-lines.txt\n"
            << "prefix any form with --multiprecision-only to disable adaptive filters\n";
}

}  // namespace

int main(int argument_count, char** arguments) {
  try {
    if (argument_count < 2) {
      print_usage(arguments[0]);
      return 2;
    }
    int first_argument = 1;
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive;
    if (std::string_view{arguments[first_argument]} == "--multiprecision-only") {
      filter_policy = PredicateFilterPolicy::multiprecision_only;
      ++first_argument;
    }
    if (first_argument >= argument_count) {
      print_usage(arguments[0]);
      return 2;
    }
    if (std::string_view{arguments[first_argument]} == "--batch") {
      if (first_argument + 1 != argument_count) {
        throw std::invalid_argument("--batch does not accept trailing arguments");
      }
      return replay_batch(std::cin, std::cout, filter_policy);
    }
    std::vector<std::string_view> tokens;
    tokens.reserve(static_cast<std::size_t>(argument_count - first_argument));
    for (int index = first_argument; index < argument_count; ++index) {
      tokens.emplace_back(arguments[index]);
    }
    const int status = replay_tokens(tokens, std::cout, filter_policy);
    std::cout.flush();
    if (!std::cout) {
      throw std::runtime_error("predicate replay output could not be written completely");
    }
    return status;
  } catch (const std::exception& error) {
    std::cerr << "predicate replay failed closed: " << error.what() << '\n';
    return 2;
  }
}
