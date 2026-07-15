#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterKind;
using morsehgp3d::exact::CircumcenterResult;
using morsehgp3d::exact::CircumcenterSupportAnalysis;
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
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::analyze_circumcenter_support;
using morsehgp3d::exact::classify_sphere_point;
using morsehgp3d::exact::classify_affine_form;
using morsehgp3d::exact::compare_squared_distances;
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
    std::span<const std::string_view> arguments, std::ostream& output) {
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
  const auto result = power_bisector_side(witness, labels.r, labels.q, &counters);
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
    return replay_power_bisector(arguments, output);
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
            << "   or: " << executable << " --batch < predicate-lines.txt\n"
            << "prefix any form with --multiprecision-only to disable FP64 filters\n";
}

}  // namespace

int main(int argument_count, char** arguments) {
  try {
    if (argument_count < 2) {
      print_usage(arguments[0]);
      return 2;
    }
    int first_argument = 1;
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_fp64;
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
