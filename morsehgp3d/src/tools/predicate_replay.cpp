#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"

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
using morsehgp3d::exact::ExactLabelMoments;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::ExactRational3Record;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::maximum_power_label_cardinality;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::power_bisector_side;
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

int replay_distance(
    std::span<const std::string_view> arguments, std::ostream& output) {
  const CertifiedPoint3 witness = parse_point(arguments, 1U);
  const CertifiedPoint3 left = parse_point(arguments, 4U);
  const CertifiedPoint3 right = parse_point(arguments, 7U);
  PredicateCounters counters;
  const auto result = compare_squared_distances(witness, left, right, &counters);
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
    std::span<const std::string_view> arguments, std::ostream& output) {
  const CertifiedPoint3 a = parse_point(arguments, 1U);
  const CertifiedPoint3 b = parse_point(arguments, 4U);
  const CertifiedPoint3 c = parse_point(arguments, 7U);
  const CertifiedPoint3 d = parse_point(arguments, 10U);
  PredicateCounters counters;
  const auto result = orientation_3d(a, b, c, d, &counters);
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
  if (arguments.size() < 8U) {
    throw std::invalid_argument("a power-bisector replay is incomplete");
  }
  const ExactRational3 witness = ExactRational3::from_record(ExactRational3Record{
      ExactRational3::schema_version,
      std::string{arguments[1]},
      std::string{arguments[2]},
      std::string{arguments[3]},
      std::string{arguments[4]},
      ExactRational3::unit});
  const std::size_t point_count = parse_size(arguments[5], "point table size");
  constexpr std::size_t prefix_size = 6U;
  if (point_count > (arguments.size() - prefix_size) / 3U) {
    throw std::invalid_argument("the power-bisector point table is truncated");
  }

  std::vector<CertifiedPoint3> point_table;
  point_table.reserve(point_count);
  std::size_t cursor = prefix_size;
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

  const ExactLabelMoments r = ExactLabelMoments::from_canonical_ids(r_ids, point_table);
  const ExactLabelMoments q = ExactLabelMoments::from_canonical_ids(q_ids, point_table);
  PredicateCounters counters;
  const auto result = power_bisector_side(witness, r, q, &counters);
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

int replay_tokens(
    std::span<const std::string_view> arguments, std::ostream& output) {
  if (arguments.empty()) {
    throw std::invalid_argument("a predicate name is required");
  }
  if (arguments[0] == "compare_squared_distances" && arguments.size() == 10U) {
    return replay_distance(arguments, output);
  }
  if (arguments[0] == "orientation_3d" && arguments.size() == 13U) {
    return replay_orientation(arguments, output);
  }
  if (arguments[0] == "power_bisector_side") {
    return replay_power_bisector(arguments, output);
  }
  throw std::invalid_argument("the predicate name or argument count is unsupported");
}

int replay_batch(std::istream& input, std::ostream& output) {
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
      static_cast<void>(replay_tokens(arguments, output));
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
            << "   or: " << executable << " --batch < predicate-lines.txt\n";
}

}  // namespace

int main(int argument_count, char** arguments) {
  try {
    if (argument_count == 2 && std::string_view{arguments[1]} == "--batch") {
      return replay_batch(std::cin, std::cout);
    }
    if (argument_count < 2) {
      print_usage(arguments[0]);
      return 2;
    }
    std::vector<std::string_view> tokens;
    tokens.reserve(static_cast<std::size_t>(argument_count - 1));
    for (int index = 1; index < argument_count; ++index) {
      tokens.emplace_back(arguments[index]);
    }
    const int status = replay_tokens(tokens, std::cout);
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
