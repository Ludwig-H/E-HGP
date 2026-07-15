#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/predicates.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::PredicateCounters;
using morsehgp3d::exact::canonical_integer_string;
using morsehgp3d::exact::compare_squared_distances;
using morsehgp3d::exact::orientation_3d;
using morsehgp3d::exact::parse_binary64_hex;
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

CertifiedPoint3 parse_point(char** arguments, std::size_t offset) {
  std::array<std::uint64_t, 3> bits{};
  for (std::size_t axis = 0; axis < bits.size(); ++axis) {
    bits[axis] = parse_binary64_hex(arguments[offset + axis], false);
  }
  return CertifiedPoint3::from_binary64_bits(bits);
}

int replay_distance(char** arguments) {
  const CertifiedPoint3 witness = parse_point(arguments, 2U);
  const CertifiedPoint3 left = parse_point(arguments, 5U);
  const CertifiedPoint3 right = parse_point(arguments, 8U);
  PredicateCounters counters;
  const auto result = compare_squared_distances(witness, left, right, &counters);
  std::cout << "{\"certification_stage\":\""
            << to_string(result.decision.certification_stage())
            << "\",\"counters\":" << counters_json(counters)
            << ",\"left_squared_distance\":" << result.left_squared_distance.canonical_json()
            << ",\"predicate\":\"compare_squared_distances\""
            << ",\"right_squared_distance\":" << result.right_squared_distance.canonical_json()
            << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

int replay_orientation(char** arguments) {
  const CertifiedPoint3 a = parse_point(arguments, 2U);
  const CertifiedPoint3 b = parse_point(arguments, 5U);
  const CertifiedPoint3 c = parse_point(arguments, 8U);
  const CertifiedPoint3 d = parse_point(arguments, 11U);
  PredicateCounters counters;
  const auto result = orientation_3d(a, b, c, d, &counters);
  std::cout << "{\"certification_stage\":\""
            << to_string(result.decision.certification_stage())
            << "\",\"counters\":" << counters_json(counters)
            << ",\"determinant_exact\":{\"denominator\":\""
            << canonical_integer_string(result.determinant.denominator())
            << "\",\"numerator\":\""
            << canonical_integer_string(result.determinant.numerator()) << "\"}"
            << ",\"predicate\":\"orientation_3d\""
            << ",\"sign\":\"" << to_string(result.decision.sign()) << "\"}\n";
  return 0;
}

void print_usage(const char* executable) {
  std::cerr << "usage: " << executable
            << " compare_squared_distances HEX_X HEX_Y HEX_Z ... (3 points)\n"
            << "   or: " << executable
            << " orientation_3d HEX_X HEX_Y HEX_Z ... (4 points)\n";
}

}  // namespace

int main(int argument_count, char** arguments) {
  try {
    if (argument_count < 2) {
      print_usage(arguments[0]);
      return 2;
    }
    const std::string_view predicate = arguments[1];
    if (predicate == "compare_squared_distances" && argument_count == 11) {
      return replay_distance(arguments);
    }
    if (predicate == "orientation_3d" && argument_count == 14) {
      return replay_orientation(arguments);
    }
    print_usage(arguments[0]);
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "predicate replay failed closed: " << error.what() << '\n';
    return 2;
  }
}
