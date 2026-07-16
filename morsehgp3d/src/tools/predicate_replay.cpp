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
using morsehgp3d::exact::AffineFormKind;
using morsehgp3d::exact::BarycentricCoordinates;
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
using morsehgp3d::exact::ExactLevelOrderResult;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::PredicateDecision;
using morsehgp3d::exact::PredicateFilterPolicy;
using morsehgp3d::exact::PredicateSign;
using morsehgp3d::exact::SupportLevelEmission;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::barycentric_coordinates;
using morsehgp3d::exact::classify_sphere_point;
using morsehgp3d::exact::classify_affine_form;
using morsehgp3d::exact::canonical_level_batches;
using morsehgp3d::exact::certified_intersect_three_planes;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::compare_exact_levels;
using morsehgp3d::exact::compare_support_levels;
using morsehgp3d::exact::circumcenter;
using morsehgp3d::exact::decide_orientation_3d;
using morsehgp3d::exact::decide_power_bisector_side;
using morsehgp3d::exact::decide_squared_distance_order;
using morsehgp3d::exact::fourth_plane_incidence;
using morsehgp3d::exact::intersect_three_planes;
using morsehgp3d::exact::maximum_power_label_cardinality;
using morsehgp3d::exact::orientation_2d_in_plane;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::plane_side;
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

int write_decision_only_result(
    std::string_view predicate,
    const PredicateDecision& decision,
    const PredicateCounters& counters,
    std::ostream& output) {
  output << "{\"certification_stage\":\""
         << to_string(decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"" << predicate
         << "\",\"sign\":\"" << to_string(decision.sign()) << "\"}\n";
  return 0;
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

std::vector<CertifiedPoint3> parse_binary64_support(
    std::span<const std::string_view> arguments,
    std::size_t offset,
    std::size_t support_size,
    std::string_view label) {
  if (support_size < 1U || support_size > 4U) {
    throw std::invalid_argument(
        std::string{label} + " must contain between one and four points");
  }
  if (offset > arguments.size() ||
      support_size > (arguments.size() - offset) / 3U) {
    throw std::invalid_argument(std::string{label} + " is truncated");
  }
  std::vector<CertifiedPoint3> support;
  support.reserve(support_size);
  for (std::size_t index = 0; index < support_size; ++index) {
    support.push_back(parse_point(arguments, offset + 3U * index));
  }
  return support;
}

template <typename Function>
decltype(auto) dispatch_binary64_support(
    const std::vector<CertifiedPoint3>& support, Function&& function) {
  switch (support.size()) {
    case 1U:
      return function(std::array<CertifiedPoint3, 1>{support[0]});
    case 2U:
      return function(
          std::array<CertifiedPoint3, 2>{support[0], support[1]});
    case 3U:
      return function(std::array<CertifiedPoint3, 3>{
          support[0], support[1], support[2]});
    case 4U:
      return function(std::array<CertifiedPoint3, 4>{
          support[0], support[1], support[2], support[3]});
    default:
      throw std::invalid_argument(
          "a binary64 support must contain between one and four points");
  }
}

CircumcenterResult circumcenter_from_binary64_support(
    const std::vector<CertifiedPoint3>& support) {
  return dispatch_binary64_support(
      support,
      []<std::size_t SupportSize>(
          const std::array<CertifiedPoint3, SupportSize>& points) {
        if constexpr (SupportSize == 1U) {
          return circumcenter(points[0]);
        } else if constexpr (SupportSize == 2U) {
          return circumcenter(points[0], points[1]);
        } else if constexpr (SupportSize == 3U) {
          return circumcenter(points[0], points[1], points[2]);
        } else {
          return circumcenter(points[0], points[1], points[2], points[3]);
        }
      });
}

void write_barycentric_coordinates_json(
    std::ostream& output, const BarycentricCoordinates& barycentric) {
  output << '[';
  for (std::size_t index = 0; index < barycentric.support_size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << rational_json(barycentric.coordinate(index));
  }
  output << ']';
}

void write_barycentric_signs_json(
    std::ostream& output, const BarycentricCoordinates& barycentric) {
  output << '[';
  for (std::size_t index = 0; index < barycentric.support_size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << '"' << to_string(barycentric.sign(index)) << '"';
  }
  output << ']';
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

class ReplayTokenCursor {
 public:
  explicit ReplayTokenCursor(
      std::span<const std::string_view> arguments,
      std::size_t cursor = 1U) noexcept
      : arguments_(arguments), cursor_(cursor) {}

  [[nodiscard]] std::string_view take(std::string_view label) {
    if (cursor_ >= arguments_.size()) {
      throw std::invalid_argument(std::string{label} + " is missing");
    }
    return arguments_[cursor_++];
  }

  [[nodiscard]] CertifiedPoint3 take_point(std::string_view label) {
    if (cursor_ > arguments_.size() || arguments_.size() - cursor_ < 3U) {
      throw std::invalid_argument(std::string{label} + " is incomplete");
    }
    const CertifiedPoint3 point = parse_point(arguments_, cursor_);
    cursor_ += 3U;
    return point;
  }

  void require_end(std::string_view label) const {
    if (cursor_ != arguments_.size()) {
      throw std::invalid_argument(
          std::string{label} + " has trailing serialized fields");
    }
  }

 private:
  std::span<const std::string_view> arguments_;
  std::size_t cursor_;
};

[[nodiscard]] ExactPlane3 parse_adaptive_plane_source(
    ReplayTokenCursor& cursor) {
  const std::string_view source_kind =
      cursor.take("the adaptive plane source kind");
  if (source_kind == "coeff") {
    std::array<std::uint64_t, 4> coefficient_bits{};
    for (std::size_t index = 0U; index < coefficient_bits.size(); ++index) {
      coefficient_bits[index] = parse_binary64_hex(
          cursor.take("an adaptive plane coefficient"), false);
    }
    return ExactPlane3::from_binary64_coefficient_bits(coefficient_bits);
  }
  if (source_kind == "through") {
    const CertifiedPoint3 first =
        cursor.take_point("the first through-plane point");
    const CertifiedPoint3 second =
        cursor.take_point("the second through-plane point");
    const CertifiedPoint3 third =
        cursor.take_point("the third through-plane point");
    return ExactPlane3::through_points(first, second, third);
  }
  if (source_kind == "power") {
    const std::size_t cardinality = parse_size(
        cursor.take("the power-plane cardinality"),
        "power-plane cardinality");
    if (cardinality == 0U ||
        cardinality > maximum_power_label_cardinality) {
      throw std::invalid_argument(
          "a power-plane cardinality must be between one and ten");
    }

    std::vector<CertifiedPoint3> point_table;
    point_table.reserve(2U * cardinality);
    for (std::size_t index = 0U; index < cardinality; ++index) {
      point_table.push_back(cursor.take_point("a power-plane R point"));
    }
    for (std::size_t index = 0U; index < cardinality; ++index) {
      point_table.push_back(cursor.take_point("a power-plane Q point"));
    }

    std::vector<std::uint32_t> r_ids;
    std::vector<std::uint32_t> q_ids;
    r_ids.reserve(cardinality);
    q_ids.reserve(cardinality);
    for (std::size_t index = 0U; index < cardinality; ++index) {
      r_ids.push_back(static_cast<std::uint32_t>(index));
      q_ids.push_back(static_cast<std::uint32_t>(cardinality + index));
    }
    const ExactLabelMoments r =
        ExactLabelMoments::from_canonical_ids(r_ids, point_table);
    const ExactLabelMoments q =
        ExactLabelMoments::from_canonical_ids(q_ids, point_table);
    const auto classification =
        classify_affine_form(power_bisector_affine_form(r, q));
    if (classification.kind() != AffineFormKind::proper_plane ||
        !classification.plane().has_value()) {
      throw std::invalid_argument(
          "a power-plane source must classify as a proper plane");
    }
    return *classification.plane();
  }
  if (source_kind == "exact") {
    return ExactPlane3::from_record(ExactPlane3Record{
        ExactPlane3::schema_version,
        std::string{cursor.take("the exact plane A coefficient")},
        std::string{cursor.take("the exact plane B coefficient")},
        std::string{cursor.take("the exact plane C coefficient")},
        std::string{cursor.take("the exact plane D coefficient")}});
  }
  throw std::invalid_argument(
      "an adaptive plane source kind must be coeff, through, power or exact");
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

int replay_decision_only_distance(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  PredicateCounters counters;
  const PredicateDecision decision = decide_squared_distance_order(
      parse_point(arguments, 1U),
      parse_point(arguments, 4U),
      parse_point(arguments, 7U),
      &counters,
      filter_policy);
  return write_decision_only_result(
      "compare_squared_distances", decision, counters, output);
}

int replay_decision_only_orientation(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  PredicateCounters counters;
  const PredicateDecision decision = decide_orientation_3d(
      parse_point(arguments, 1U),
      parse_point(arguments, 4U),
      parse_point(arguments, 7U),
      parse_point(arguments, 10U),
      &counters,
      filter_policy);
  return write_decision_only_result(
      "orientation_3d", decision, counters, output);
}

int replay_decision_only_power_bisector(
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
  const PredicateDecision decision =
      binary64_witness.has_value()
          ? decide_power_bisector_side(
                *binary64_witness,
                labels.r,
                labels.q,
                &counters,
                filter_policy)
          : decide_power_bisector_side(witness, labels.r, labels.q, &counters);
  return write_decision_only_result(
      "power_bisector_side", decision, counters, output);
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

int replay_adaptive_orientation_2d(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  ReplayTokenCursor cursor{arguments};
  const ExactPlane3 plane = parse_adaptive_plane_source(cursor);
  const CertifiedPoint3 a =
      cursor.take_point("the first adaptive orientation point");
  const CertifiedPoint3 b =
      cursor.take_point("the second adaptive orientation point");
  const CertifiedPoint3 c =
      cursor.take_point("the third adaptive orientation point");
  cursor.require_end("the adaptive orientation replay");

  PredicateCounters counters;
  const auto result = orientation_2d_in_plane(
      plane, a, b, c, &counters, filter_policy);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"orientation_value_exact\":"
         << rational_json(result.orientation_value)
         << ",\"predicate\":\"adaptive_orientation_2d_in_plane\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_adaptive_plane_side(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  ReplayTokenCursor cursor{arguments};
  const ExactPlane3 plane = parse_adaptive_plane_source(cursor);
  const CertifiedPoint3 point =
      cursor.take_point("the adaptive plane-side point");
  cursor.require_end("the adaptive plane-side replay");

  PredicateCounters counters;
  const auto result = plane_side(plane, point, &counters, filter_policy);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"adaptive_plane_side\""
         << ",\"sign\":\"" << to_string(result.decision.sign()) << "\""
         << ",\"signed_value_exact\":" << rational_json(result.signed_value)
         << "}\n";
  return 0;
}

int replay_adaptive_three_plane_intersection(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  ReplayTokenCursor cursor{arguments};
  const ExactPlane3 first = parse_adaptive_plane_source(cursor);
  const ExactPlane3 second = parse_adaptive_plane_source(cursor);
  const ExactPlane3 third = parse_adaptive_plane_source(cursor);
  cursor.require_end("the adaptive three-plane intersection replay");

  PredicateCounters counters;
  const auto certified = certified_intersect_three_planes(
      first, second, third, &counters, filter_policy);
  const auto& intersection = certified.intersection();
  output << "{\"affine_dimension\":";
  if (intersection.affine_dimension().has_value()) {
    output << *intersection.affine_dimension();
  } else {
    output << "null";
  }
  output << ",\"augmented_rank\":" << intersection.augmented_rank()
         << ",\"certification_stage\":\""
         << to_string(certified.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"intersection_exact\":";
  if (intersection.point().has_value()) {
    output << intersection.point()->canonical_json();
  } else {
    output << "null";
  }
  output << ",\"intersection_kind\":\"" << to_string(intersection.kind())
         << "\",\"normal_determinant_sign\":\""
         << to_string(certified.canonical_normal_determinant_sign())
         << "\",\"normal_rank\":" << intersection.normal_rank()
         << ",\"predicate\":\"adaptive_intersect_three_planes\"}\n";
  return 0;
}

int replay_adaptive_fourth_plane_incidence(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  ReplayTokenCursor cursor{arguments};
  const ExactPlane3 first = parse_adaptive_plane_source(cursor);
  const ExactPlane3 second = parse_adaptive_plane_source(cursor);
  const ExactPlane3 third = parse_adaptive_plane_source(cursor);
  const ExactPlane3 fourth = parse_adaptive_plane_source(cursor);
  cursor.require_end("the adaptive fourth-plane incidence replay");

  PredicateCounters counters;
  const auto result = fourth_plane_incidence(
      first, second, third, fourth, &counters, filter_policy);
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"intersection_exact\":" << result.intersection.canonical_json()
         << ",\"predicate\":\"adaptive_fourth_plane_incidence\""
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

int replay_binary64_barycentric_coordinates(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument(
        "the binary64 barycentric support size is missing");
  }
  const std::size_t support_size =
      parse_size(arguments[1], "binary64 barycentric support size");
  if (support_size < 1U || support_size > 4U) {
    throw std::invalid_argument(
        "binary64 barycentric coordinates require one to four support points");
  }
  if (arguments.size() != 5U + 3U * support_size) {
    throw std::invalid_argument(
        "the binary64 barycentric replay has the wrong serialized point count");
  }
  const std::vector<CertifiedPoint3> support = parse_binary64_support(
      arguments, 2U, support_size, "the binary64 barycentric support");
  const CertifiedPoint3 query =
      parse_point(arguments, 2U + 3U * support_size);

  PredicateCounters counters;
  const BarycentricCoordinates barycentric = dispatch_binary64_support(
      support,
      [&query, &counters, filter_policy]<std::size_t SupportSize>(
          const std::array<CertifiedPoint3, SupportSize>& points) {
        return barycentric_coordinates(
            query, points, &counters, filter_policy);
      });

  output << "{\"barycentric_coordinates_exact\":";
  write_barycentric_coordinates_json(output, barycentric);
  output << ",\"barycentric_signs\":";
  write_barycentric_signs_json(output, barycentric);
  output << ",\"certification_stage\":\""
         << to_string(barycentric.certification_stage())
         << "\",\"convex_hull_location\":\""
         << to_string(barycentric.location())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"binary64_barycentric_coordinates\""
         << ",\"support_size\":" << support_size << "}\n";
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
          std::array<CertifiedPoint3, 1>{support[0]},
          &counters,
          PredicateFilterPolicy::multiprecision_only);
    }
    if (support_size == 2U) {
      return analyze_circumcenter_support(
          std::array<CertifiedPoint3, 2>{support[0], support[1]},
          &counters,
          PredicateFilterPolicy::multiprecision_only);
    }
    if (support_size == 3U) {
      return analyze_circumcenter_support(
          std::array<CertifiedPoint3, 3>{
              support[0], support[1], support[2]},
          &counters,
          PredicateFilterPolicy::multiprecision_only);
    }
    return analyze_circumcenter_support(
        std::array<CertifiedPoint3, 4>{
            support[0], support[1], support[2], support[3]},
        &counters,
        PredicateFilterPolicy::multiprecision_only);
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

int replay_binary64_circumcenter_support_analysis(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument(
        "the binary64 analyzed support size is missing");
  }
  const std::size_t support_size =
      parse_size(arguments[1], "binary64 analyzed support size");
  if (support_size < 1U || support_size > 4U) {
    throw std::invalid_argument(
        "a binary64 support analysis requires between one and four points");
  }
  if (arguments.size() != 2U + 3U * support_size) {
    throw std::invalid_argument(
        "the binary64 analyzed support has the wrong serialized point count");
  }
  const std::vector<CertifiedPoint3> support = parse_binary64_support(
      arguments, 2U, support_size, "the binary64 analyzed support");

  PredicateCounters counters;
  const CircumcenterSupportAnalysis analysis = dispatch_binary64_support(
      support,
      [&counters, filter_policy]<std::size_t SupportSize>(
          const std::array<CertifiedPoint3, SupportSize>& points) {
        return analyze_circumcenter_support(
            points, &counters, filter_policy);
      });

  const CircumcenterResult& center = analysis.circumcenter_result();
  const bool independent = center.kind() == CircumcenterKind::unique;
  output << "{\"affine_dimension\":" << center.affine_dimension()
         << ",\"barycentric_coordinates_exact\":";
  if (analysis.barycentric().has_value()) {
    write_barycentric_coordinates_json(output, *analysis.barycentric());
  } else {
    output << "null";
  }
  output << ",\"barycentric_signs\":";
  if (analysis.barycentric().has_value()) {
    write_barycentric_signs_json(output, *analysis.barycentric());
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
  if (analysis.barycentric().has_value()) {
    output << '"' << to_string(analysis.barycentric()->certification_stage())
           << '"';
  } else {
    output << "null";
  }
  output << ",\"convex_hull_location\":";
  if (analysis.barycentric().has_value()) {
    output << '"' << to_string(analysis.barycentric()->location()) << '"';
  } else {
    output << "null";
  }
  output << ",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"binary64_circumcenter_support_analysis\""
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

int replay_support_sphere_side(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.size() < 2U) {
    throw std::invalid_argument("the support-sphere size is missing");
  }
  const std::size_t support_size =
      parse_size(arguments[1], "support-sphere support size");
  if (support_size < 1U || support_size > 4U) {
    throw std::invalid_argument(
        "support-sphere classification requires one to four support points");
  }
  if (arguments.size() != 5U + 3U * support_size) {
    throw std::invalid_argument(
        "the support-sphere replay has the wrong serialized point count");
  }
  const std::vector<CertifiedPoint3> support = parse_binary64_support(
      arguments, 2U, support_size, "the support-sphere support");
  const CertifiedPoint3 point =
      parse_point(arguments, 2U + 3U * support_size);

  PredicateCounters counters;
  const auto classification = dispatch_binary64_support(
      support,
      [&point, &counters, filter_policy]<std::size_t SupportSize>(
          const std::array<CertifiedPoint3, SupportSize>& points) {
        return classify_sphere_point(
            points, point, &counters, filter_policy);
      });
  const CircumcenterResult sphere =
      circumcenter_from_binary64_support(support);
  if (sphere.kind() != CircumcenterKind::unique ||
      !sphere.center().has_value() || !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "a support-sphere classification omitted its exact sphere witness");
  }

  output << "{\"center_exact\":" << sphere.center()->canonical_json()
         << ",\"certification_stage\":\""
         << to_string(classification.decision().certification_stage())
         << "\",\"classification\":\""
         << to_string(classification.location())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"predicate\":\"support_sphere_side\""
         << ",\"sign\":\"" << to_string(classification.decision().sign())
         << "\",\"signed_offset_exact\":"
         << rational_json(classification.signed_power())
         << ",\"squared_distance_exact\":"
         << classification.point_squared_distance().canonical_json()
         << ",\"squared_level_exact\":"
         << sphere.squared_level()->canonical_json() << "}\n";
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

int replay_support_level_comparison(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.size() < 3U) {
    throw std::invalid_argument(
        "the support-level comparison is incomplete");
  }
  const std::size_t left_size =
      parse_size(arguments[1], "left support size");
  if (left_size < 1U || left_size > 4U) {
    throw std::invalid_argument(
        "the left level support must contain between one and four points");
  }
  std::size_t cursor = 2U;
  const std::vector<CertifiedPoint3> left_support = parse_binary64_support(
      arguments, cursor, left_size, "the left level support");
  cursor += 3U * left_size;
  if (cursor >= arguments.size()) {
    throw std::invalid_argument("the right support size is missing");
  }
  const std::size_t right_size =
      parse_size(arguments[cursor], "right support size");
  ++cursor;
  if (right_size < 1U || right_size > 4U) {
    throw std::invalid_argument(
        "the right level support must contain between one and four points");
  }
  const std::vector<CertifiedPoint3> right_support = parse_binary64_support(
      arguments, cursor, right_size, "the right level support");
  cursor += 3U * right_size;
  if (cursor != arguments.size()) {
    throw std::invalid_argument(
        "the support-level comparison has trailing serialized fields");
  }

  PredicateCounters counters;
  const ExactLevelOrderResult result = dispatch_binary64_support(
      left_support,
      [&right_support, &counters, filter_policy]<std::size_t LeftSize>(
          const std::array<CertifiedPoint3, LeftSize>& left) {
        return dispatch_binary64_support(
            right_support,
            [&left, &counters, filter_policy]<std::size_t RightSize>(
                const std::array<CertifiedPoint3, RightSize>& right) {
              return compare_support_levels(
                  left, right, &counters, filter_policy);
            });
      });
  const CircumcenterResult left_sphere =
      circumcenter_from_binary64_support(left_support);
  const CircumcenterResult right_sphere =
      circumcenter_from_binary64_support(right_support);
  if (left_sphere.kind() != CircumcenterKind::unique ||
      right_sphere.kind() != CircumcenterKind::unique ||
      !left_sphere.squared_level().has_value() ||
      !right_sphere.squared_level().has_value()) {
    throw std::logic_error(
        "a support-level comparison omitted an exact level witness");
  }

  const PredicateSign sign = result.decision.sign();
  const char* ordering = sign == PredicateSign::negative
                             ? "less"
                             : sign == PredicateSign::zero ? "equal" : "greater";
  output << "{\"certification_stage\":\""
         << to_string(result.decision.certification_stage())
         << "\",\"counters\":" << counters_json(counters)
         << ",\"cross_product_difference_exact\":\""
         << canonical_integer_string(result.cross_product_difference)
         << "\",\"equal\":"
         << (sign == PredicateSign::zero ? "true" : "false")
         << ",\"left_squared_level_exact\":"
         << left_sphere.squared_level()->canonical_json()
         << ",\"ordering\":\"" << ordering
         << "\",\"predicate\":\"compare_support_levels\""
         << ",\"right_squared_level_exact\":"
         << right_sphere.squared_level()->canonical_json()
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
  if (arguments[0] == "adaptive_orientation_2d_in_plane") {
    return replay_adaptive_orientation_2d(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "adaptive_plane_side") {
    return replay_adaptive_plane_side(arguments, output, filter_policy);
  }
  if (arguments[0] == "adaptive_intersect_three_planes") {
    return replay_adaptive_three_plane_intersection(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "adaptive_fourth_plane_incidence") {
    return replay_adaptive_fourth_plane_incidence(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "circumcenter_support") {
    return replay_circumcenter_support(arguments, output);
  }
  if (arguments[0] == "binary64_barycentric_coordinates") {
    return replay_binary64_barycentric_coordinates(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "circumcenter_support_analysis") {
    return replay_circumcenter_support_analysis(arguments, output);
  }
  if (arguments[0] == "binary64_circumcenter_support_analysis") {
    return replay_binary64_circumcenter_support_analysis(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "sphere_side") {
    return replay_sphere_side(arguments, output);
  }
  if (arguments[0] == "support_sphere_side") {
    return replay_support_sphere_side(arguments, output, filter_policy);
  }
  if (arguments[0] == "compare_exact_levels" && arguments.size() == 5U) {
    return replay_exact_level_comparison(arguments, output);
  }
  if (arguments[0] == "compare_support_levels") {
    return replay_support_level_comparison(
        arguments, output, filter_policy);
  }
  if (arguments[0] == "canonical_level_batches") {
    return replay_canonical_level_batches(arguments, output);
  }
  throw std::invalid_argument("the predicate name or argument count is unsupported");
}

int replay_decision_only_tokens(
    std::span<const std::string_view> arguments,
    std::ostream& output,
    PredicateFilterPolicy filter_policy) {
  if (arguments.empty()) {
    throw std::invalid_argument("a predicate name is required");
  }
  if (arguments[0] == "compare_squared_distances" && arguments.size() == 10U) {
    return replay_decision_only_distance(arguments, output, filter_policy);
  }
  if (arguments[0] == "orientation_3d" && arguments.size() == 13U) {
    return replay_decision_only_orientation(arguments, output, filter_policy);
  }
  if (arguments[0] == "power_bisector_side") {
    return replay_decision_only_power_bisector(
        arguments, output, filter_policy);
  }
  throw std::invalid_argument(
      "the predicate name or argument count is unsupported in decision-only batch mode");
}

int replay_batch(
    std::istream& input,
    std::ostream& output,
    PredicateFilterPolicy filter_policy,
    bool decision_only) {
  // A decision-only invocation is the transaction boundary used by the
  // campaign runner. Buffer its compact rows until every input line has been
  // parsed and certified so a late malformed command cannot publish a valid
  // prefix that looks like a completed chunk. The historical rich batch keeps
  // its streaming behavior unchanged.
  std::ostringstream decision_transaction;
  std::ostream& batch_output = decision_only
                                   ? static_cast<std::ostream&>(decision_transaction)
                                   : output;
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
      static_cast<void>(
          decision_only
              ? replay_decision_only_tokens(arguments, batch_output, filter_policy)
              : replay_tokens(arguments, batch_output, filter_policy));
    } catch (const std::exception& error) {
      throw std::invalid_argument(
          "batch replay line " + std::to_string(line_number) + ": " + error.what());
    }
  }
  if (!input.eof()) {
    throw std::runtime_error("batch replay input could not be read completely");
  }
  if (decision_only) {
    output << std::move(decision_transaction).str();
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
            << " adaptive_orientation_2d_in_plane SOURCE ... (3 points)\n"
            << "   or: " << executable
            << " adaptive_plane_side SOURCE ... POINT_X POINT_Y POINT_Z\n"
            << "   or: " << executable
            << " adaptive_intersect_three_planes SOURCE ... (3 plane sources)\n"
            << "   or: " << executable
            << " adaptive_fourth_plane_incidence SOURCE ... (4 plane sources)\n"
            << "   or: " << executable
            << " circumcenter_support COUNT HEX_X HEX_Y HEX_Z ... (2 to 4 points)\n"
            << "   or: " << executable
            << " binary64_barycentric_coordinates COUNT HEX_X HEX_Y HEX_Z ... QUERY_X QUERY_Y QUERY_Z\n"
            << "   or: " << executable
            << " circumcenter_support_analysis COUNT HEX_X HEX_Y HEX_Z ... (1 to 4 points)\n"
            << "   or: " << executable
            << " binary64_circumcenter_support_analysis COUNT HEX_X HEX_Y HEX_Z ... (1 to 4 points)\n"
            << "   or: " << executable
            << " sphere_side XN YN ZN D LEVEL_N LEVEL_D HEX_X HEX_Y HEX_Z\n"
            << "   or: " << executable
            << " support_sphere_side COUNT HEX_X HEX_Y HEX_Z ... POINT_X POINT_Y POINT_Z\n"
            << "   or: " << executable
            << " compare_exact_levels LEFT_N LEFT_D RIGHT_N RIGHT_D\n"
            << "   or: " << executable
            << " compare_support_levels LEFT_COUNT HEX_X HEX_Y HEX_Z ... RIGHT_COUNT HEX_X HEX_Y HEX_Z ...\n"
            << "   or: " << executable
            << " canonical_level_batches COUNT LEVEL_N LEVEL_D MIN_COUNT ... SOURCE_COUNT ...\n"
            << "   or: " << executable << " --batch < predicate-lines.txt\n"
            << "   or: " << executable
            << " --decision-only --batch < predicate-lines.txt\n"
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
    bool decision_only = false;
    if (first_argument < argument_count &&
        std::string_view{arguments[first_argument]} == "--decision-only") {
      decision_only = true;
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
      return replay_batch(std::cin, std::cout, filter_policy, decision_only);
    }
    if (decision_only) {
      throw std::invalid_argument("--decision-only requires --batch");
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
