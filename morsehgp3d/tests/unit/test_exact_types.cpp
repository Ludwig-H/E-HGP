#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::binary64_total_order_key;
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactLevelRecord;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::exact::ExactRational3Record;
using morsehgp3d::exact::power_of_two;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

void test_rational_canonicalization_and_arithmetic() {
  check(ExactRational{BigInt{6}, BigInt{-8}} == ExactRational{BigInt{-3}, BigInt{4}},
        "rational sign and gcd normalization");
  check(ExactRational{BigInt{0}, BigInt{-19}}.canonical_key() == "0/1",
        "rational zero canonicalization");
  check(ExactRational::parse_canonical("-3/4") == ExactRational{BigInt{-3}, BigInt{4}},
        "canonical rational parsing");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(ExactRational::parse_canonical("02/3")); },
      "leading zero rejection");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(ExactRational::parse_canonical("2/4")); },
      "non-reduced rational rejection");
  check_throws<std::domain_error>(
      [] { static_cast<void>(ExactRational{BigInt{1}, BigInt{0}}); },
      "zero denominator rejection");

  const ExactRational left{BigInt{2}, BigInt{3}};
  const ExactRational right{BigInt{5}, BigInt{7}};
  check(left + right == ExactRational{BigInt{29}, BigInt{21}}, "exact addition");
  check(left - right == ExactRational{BigInt{-1}, BigInt{21}}, "exact subtraction");
  check(left * right == ExactRational{BigInt{10}, BigInt{21}}, "exact multiplication");
  check(left / right == ExactRational{BigInt{14}, BigInt{15}}, "exact division");
  check_throws<std::domain_error>(
      [&left] { static_cast<void>(left / ExactRational{}); },
      "exact division by zero rejection");
}

void test_binary64_exact_decoding() {
  check(ExactRational::from_binary64(0.1) ==
            ExactRational{BigInt{"3602879701896397"}, BigInt{"36028797018963968"}},
        "0.1 is decoded as its exact binary64 dyadic");
  check(ExactRational::from_binary64(0.1) != ExactRational{BigInt{1}, BigInt{10}},
        "0.1 is not replaced by its decimal spelling");
  check(ExactRational::from_binary64_bits(1U) ==
            ExactRational{BigInt{1}, power_of_two(1074U)},
        "minimum subnormal decoding");
  check(ExactRational::from_binary64_bits(0x8000000000000001ULL) ==
            ExactRational{BigInt{-1}, power_of_two(1074U)},
        "negative minimum subnormal decoding");
  check(ExactRational::from_binary64_bits(0x0010000000000000ULL) ==
            ExactRational{BigInt{1}, power_of_two(1022U)},
        "minimum normal binary64 decoding");
  check(ExactRational::from_binary64_bits(0x000fffffffffffffULL) ==
            ExactRational{power_of_two(52U) - 1, power_of_two(1074U)},
        "maximum subnormal binary64 decoding");
  check(ExactRational::from_binary64_bits(0x7fefffffffffffffULL) ==
            ExactRational{(power_of_two(53U) - 1) << 971U},
        "maximum finite binary64 decoding");
  check(ExactRational::from_binary64_bits(0xffefffffffffffffULL) ==
            ExactRational{-((power_of_two(53U) - 1) << 971U)},
        "negative maximum finite binary64 decoding");
  check(ExactRational::from_binary64_bits(0U) ==
            ExactRational::from_binary64_bits(0x8000000000000000ULL),
        "signed zero has one exact geometric value");
  check(parse_binary64_hex("3ff0000000000000") == 0x3ff0000000000000ULL,
        "canonical binary64 hex parsing");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(parse_binary64_hex("8000000000000000")); },
      "negative zero canonical bit rejection");
  check(parse_binary64_hex("8000000000000000", false) == 0x8000000000000000ULL,
        "negative zero replay bit acceptance");
  check_throws<std::invalid_argument>(
      [] { static_cast<void>(parse_binary64_hex("3FF0000000000000")); },
      "uppercase binary64 hex rejection");
  check_throws<std::domain_error>(
      [] { static_cast<void>(ExactRational::from_binary64_bits(0x7ff0000000000000ULL)); },
      "positive infinity rejection");
  check_throws<std::domain_error>(
      [] { static_cast<void>(ExactRational::from_binary64_bits(0x7ff8000000000001ULL)); },
      "NaN rejection");
  check_throws<std::domain_error>(
      [] { static_cast<void>(ExactRational::from_binary64_bits(0xfff8000000000001ULL)); },
      "negative NaN rejection");

  const double below = std::nextafter(1.0, 0.0);
  const double above = std::nextafter(1.0, std::numeric_limits<double>::infinity());
  check(ExactRational::from_binary64(below) < ExactRational{BigInt{1}},
        "one ULP below one stays below");
  check(ExactRational::from_binary64(above) > ExactRational{BigInt{1}},
        "one ULP above one stays above");
  check(binary64_total_order_key(std::bit_cast<std::uint64_t>(-2.0)) <
            binary64_total_order_key(std::bit_cast<std::uint64_t>(-1.0)) &&
            binary64_total_order_key(std::bit_cast<std::uint64_t>(-1.0)) <
                binary64_total_order_key(std::bit_cast<std::uint64_t>(0.0)) &&
            binary64_total_order_key(std::bit_cast<std::uint64_t>(0.0)) <
                binary64_total_order_key(std::bit_cast<std::uint64_t>(1.0)),
        "canonical finite binary64 total order");
  check(binary64_total_order_key(0x8000000000000000ULL) ==
            binary64_total_order_key(0U),
        "signed zeros share one canonical total-order key");
}

void test_exact_level_contract() {
  const ExactLevel level{BigInt{6}, BigInt{8}};
  check(level.numerator_string() == "3" && level.denominator_string() == "4",
        "ExactLevel canonical reduction");
  check(level.to_record() == ExactLevelRecord{
                                 "2.0.0", "3", "4", "input_coordinate_unit_squared"},
        "ExactLevel v2 record");
  check(level.canonical_json() ==
            "{\"denominator\":\"4\",\"numerator\":\"3\",\"schema_version\":\"2.0.0\",\"unit\":\"input_coordinate_unit_squared\"}",
        "ExactLevel byte-stable JSON");
  check(ExactLevel::from_record(level.to_record()) == level,
        "ExactLevel record round trip");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(ExactLevel::from_record(
            ExactLevelRecord{"2.0.0", "6", "8", "input_coordinate_unit_squared"}));
      },
      "non-reduced ExactLevel record rejection");
  check_throws<std::domain_error>(
      [] { static_cast<void>(ExactLevel{BigInt{-1}, BigInt{2}}); },
      "negative squared level rejection");

  const BigInt scale = power_of_two(512U);
  const ExactLevel lower{scale, scale + 1};
  const ExactLevel upper{scale + 1, scale + 2};
  check(lower < upper, "arbitrarily close levels compare by exact cross product");

  std::vector<ExactLevel> levels{
      ExactLevel{BigInt{1}, BigInt{2}},
      ExactLevel{BigInt{2}, BigInt{4}},
      ExactLevel{BigInt{0}},
      ExactLevel{BigInt{3}, BigInt{4}}};
  std::sort(levels.begin(), levels.end());
  const std::set<ExactLevel> unique(levels.begin(), levels.end());
  check(unique.size() == 3U, "ExactLevel sorting and deduplication agree");
  check(ExactLevel{BigInt{1}, BigInt{2}}.table_hash64() ==
            ExactLevel{BigInt{2}, BigInt{4}}.table_hash64(),
        "ExactLevel canonical hash agrees with equality");
}

void test_exact_rational3_and_replay_bits() {
  const ExactRational3 point{std::array<ExactRational, 3>{
      ExactRational{BigInt{1}, BigInt{2}},
      ExactRational{BigInt{-1}, BigInt{3}},
      ExactRational{BigInt{0}}}};
  check(point.to_record() == ExactRational3Record{
                                  "2.0.0", "3", "-2", "0", "6", "input_coordinate_unit"},
        "ExactRational3 common denominator canonicalization");
  check(ExactRational3::from_record(point.to_record()) == point,
        "ExactRational3 record round trip");
  check(point.canonical_json() ==
            "{\"denominator\":\"6\",\"schema_version\":\"2.0.0\",\"unit\":\"input_coordinate_unit\",\"x_numerator\":\"3\",\"y_numerator\":\"-2\",\"z_numerator\":\"0\"}",
        "ExactRational3 byte-stable JSON");
  check_throws<std::invalid_argument>(
      [] {
        static_cast<void>(ExactRational3::from_record(
            ExactRational3Record{
                "2.0.0", "2", "-4", "0", "6", "input_coordinate_unit"}));
      },
      "non-reduced ExactRational3 record rejection");
  check(ExactRational3{BigInt{2}, BigInt{-4}, BigInt{0}, BigInt{-6}} ==
            ExactRational3{BigInt{-1}, BigInt{2}, BigInt{0}, BigInt{3}},
        "ExactRational3 negative denominator canonicalization");
  check(ExactRational3{BigInt{0}, BigInt{0}, BigInt{0}, BigInt{99}} ==
            ExactRational3{},
        "ExactRational3 all-zero canonicalization");

  const CertifiedPoint3 positive_zero = CertifiedPoint3::from_binary64(0.0, 1.0, -2.0);
  const CertifiedPoint3 negative_zero = CertifiedPoint3::from_binary64(-0.0, 1.0, -2.0);
  check(positive_zero.exact() == negative_zero.exact(),
        "signed zero points share one exact geometry");
  check(positive_zero.canonical_input_bits() == negative_zero.canonical_input_bits(),
        "signed zero points share canonical embedded bits");
  check(positive_zero.replay_key() == "0000000000000000:3ff0000000000000:c000000000000000",
        "positive-zero replay key");
  check(negative_zero.replay_key() == "8000000000000000:3ff0000000000000:c000000000000000",
        "negative-zero replay key preserves input bits");
}

}  // namespace

int main() {
  test_rational_canonicalization_and_arithmetic();
  test_binary64_exact_decoding();
  test_exact_level_contract();
  test_exact_rational3_and_replay_bits();

  if (failures != 0) {
    std::cerr << failures << " exact-type test(s) failed\n";
    return 1;
  }
  std::cout << "MorseHGP3D exact-type tests passed\n";
  return 0;
}
