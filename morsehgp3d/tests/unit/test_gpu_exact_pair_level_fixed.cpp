#include "phase15_exact_pair_level_fixed.cuh"

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace pair_level =
    morsehgp3d::gpu::detail::exact_pair_level_fixed;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using PointBits = std::array<std::uint64_t, 3U>;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] std::uint64_t bits(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] PointBits point(double x, double y, double z) {
  return {bits(x), bits(y), bits(z)};
}

[[nodiscard]] pair_level::Result fixed_level(
    const PointBits& first,
    const PointBits& second) {
  return pair_level::exact_pair_level(first.data(), second.data());
}

[[nodiscard]] ExactLevel oracle_level(
    const PointBits& first,
    const PointBits& second) {
  ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational left =
        ExactRational::from_binary64_bits(first[axis]);
    const ExactRational right =
        ExactRational::from_binary64_bits(second[axis]);
    const ExactRational difference = left - right;
    squared_distance = squared_distance + difference * difference;
  }
  return ExactLevel{ExactRational{
      squared_distance.numerator(),
      squared_distance.denominator() * BigInt{4}}};
}

[[nodiscard]] BigInt fixed_numerator(const pair_level::Result& result) {
  BigInt numerator = 0;
  for (std::size_t index = pair_level::maximum_limb_count;
       index != 0U;
       --index) {
    numerator <<= 64U;
    numerator += result.level.numerator.limb[index - 1U];
  }
  return numerator;
}

[[nodiscard]] ExactLevel materialize(const pair_level::Result& result) {
  if (!pair_level::result_is_exact(result)) {
    throw std::logic_error("only an exact fixed pair level can materialize");
  }
  BigInt numerator = fixed_numerator(result);
  const int exponent = result.level.exponent;
  if (exponent >= 0) {
    numerator <<= static_cast<unsigned int>(exponent);
    return ExactLevel{std::move(numerator)};
  }
  return ExactLevel{
      std::move(numerator),
      morsehgp3d::exact::power_of_two(
          static_cast<unsigned int>(-exponent))};
}

void check_exact_against_oracle(
    const PointBits& first,
    const PointBits& second,
    const std::string& label) {
  const pair_level::Result result = fixed_level(first, second);
  check(
      pair_level::result_is_exact(result),
      label + " must fit the exact fixed lane");
  if (pair_level::result_is_exact(result)) {
    check(
        materialize(result) == oracle_level(first, second),
        label + " must agree with the BigInt ExactLevel oracle");
  }
}

void test_minimum_subnormal_and_power_boundaries() {
  const PointBits zero{};
  PointBits minimum_subnormal{};
  minimum_subnormal[0] = UINT64_C(1);
  const pair_level::Result minimum = fixed_level(zero, minimum_subnormal);
  check(
      pair_level::result_is_exact(minimum) &&
          fixed_numerator(minimum) == 1 &&
          minimum.level.exponent == pair_level::minimum_canonical_exponent &&
          minimum.level.limb_count == 1U,
      "the minimum subnormal pair level must be 1*2^-2150");
  if (pair_level::result_is_exact(minimum)) {
    check(
        materialize(minimum) == oracle_level(zero, minimum_subnormal),
        "the minimum subnormal level must agree with ExactLevel");
  }

  const double high_power = std::bit_cast<double>(UINT64_C(0x7fe0000000000000));
  const PointBits positive{bits(high_power), bits(high_power), 0U};
  const PointBits negative{bits(-high_power), bits(-high_power), 0U};
  const pair_level::Result maximum_low_bit = fixed_level(positive, negative);
  check(
      pair_level::result_is_exact(maximum_low_bit) &&
          fixed_numerator(maximum_low_bit) == 1 &&
          maximum_low_bit.level.exponent ==
              pair_level::maximum_canonical_exponent,
      "two axes spanning +/-2^1023 must produce 1*2^2047");
  if (pair_level::result_is_exact(maximum_low_bit)) {
    check(
        materialize(maximum_low_bit) == oracle_level(positive, negative),
        "the maximum canonical exponent fixture must agree with ExactLevel");
  }
}

void test_permutations_endpoint_exchange_and_equality() {
  const PointBits first = point(-7.0, 0.125, 32.0);
  const PointBits second = point(5.0, -4.0, 2.0);
  const std::array<std::array<std::size_t, 3U>, 6U> permutations{{
      {0U, 1U, 2U},
      {0U, 2U, 1U},
      {1U, 0U, 2U},
      {1U, 2U, 0U},
      {2U, 0U, 1U},
      {2U, 1U, 0U},
  }};
  const pair_level::Result reference = fixed_level(first, second);
  check(pair_level::result_is_exact(reference), "the permutation reference fits");
  for (const auto& permutation : permutations) {
    PointBits permuted_first{};
    PointBits permuted_second{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      permuted_first[axis] = first[permutation[axis]];
      permuted_second[axis] = second[permutation[axis]];
    }
    const pair_level::Result forward =
        fixed_level(permuted_first, permuted_second);
    const pair_level::Result reverse =
        fixed_level(permuted_second, permuted_first);
    check(
        pair_level::compare(reference, forward) ==
                pair_level::ComparisonCode::equal &&
            pair_level::compare(reference, reverse) ==
                pair_level::ComparisonCode::equal,
        "axis permutations and endpoint exchange preserve the exact level");
    check_exact_against_oracle(
        permuted_first, permuted_second, "permuted support pair");
  }

  const pair_level::Result zero_to_two =
      fixed_level(point(0.0, 0.0, 0.0), point(2.0, 0.0, 0.0));
  const pair_level::Result minus_one_to_one =
      fixed_level(point(-1.0, 0.0, 0.0), point(1.0, 0.0, 0.0));
  check(
      pair_level::compare(zero_to_two, minus_one_to_one) ==
          pair_level::ComparisonCode::equal,
      "different support pairs with equal distance have equal exact levels");
  const pair_level::RadixPrefix left_prefix =
      pair_level::radix_prefix(zero_to_two);
  const pair_level::RadixPrefix right_prefix =
      pair_level::radix_prefix(minus_one_to_one);
  check(
      pair_level::compare_radix_prefixes(left_prefix, right_prefix) ==
          pair_level::RadixPrefixComparison::
              collision_requires_exact_refinement,
      "even truly equal radix prefixes remain refinement collisions");
}

[[nodiscard]] pair_level::Result manual_exact_level(
    std::uint64_t numerator,
    std::int16_t exponent) {
  pair_level::CanonicalLevel256 level{};
  level.numerator.limb[0] = numerator;
  level.exponent = exponent;
  level.limb_count = 1U;
  return pair_level::Result{level, pair_level::ResultCode::exact};
}

void test_exact_comparator_and_certified_radix_prefix() {
  const pair_level::Result one = manual_exact_level(1U, 0);
  const pair_level::Result four = manual_exact_level(1U, 2);
  check(
      pair_level::compare(one, four) == pair_level::ComparisonCode::less &&
          pair_level::compare(four, one) ==
              pair_level::ComparisonCode::greater,
      "the exact comparator orders distinct top exponents");
  const auto one_prefix = pair_level::radix_prefix(one);
  const auto four_prefix = pair_level::radix_prefix(four);
  check(
      pair_level::compare_radix_prefixes(one_prefix, four_prefix) ==
          pair_level::RadixPrefixComparison::less,
      "different certified radix prefixes preserve exact order");

  const pair_level::Result close_lower =
      manual_exact_level((UINT64_C(1) << 60U) | UINT64_C(1), 0);
  const pair_level::Result close_upper =
      manual_exact_level((UINT64_C(1) << 60U) | UINT64_C(3), 0);
  check(
      pair_level::compare(close_lower, close_upper) ==
          pair_level::ComparisonCode::less,
      "the exact comparator resolves bits below the radix prefix");
  const auto close_lower_prefix = pair_level::radix_prefix(close_lower);
  const auto close_upper_prefix = pair_level::radix_prefix(close_upper);
  check(
      close_lower_prefix.available && close_upper_prefix.available &&
          close_lower_prefix.key == close_upper_prefix.key &&
          pair_level::compare_radix_prefixes(
              close_lower_prefix, close_upper_prefix) ==
              pair_level::RadixPrefixComparison::
                  collision_requires_exact_refinement,
      "a radix-prefix collision never fabricates exact equality");

  const pair_level::Result nonexact{
      {}, pair_level::ResultCode::requires_wide};
  check(
      pair_level::compare(one, nonexact) ==
              pair_level::ComparisonCode::not_comparable &&
          !pair_level::radix_prefix(nonexact).available,
      "comparison and radix publication require two exact canonical forms");
}

void test_fixed_overflow_and_universal_4200_bit_witness() {
  constexpr std::uint64_t fraction_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t internal_exponent_75 = 1150U;
  const std::uint64_t high =
      (internal_exponent_75 << 52U) | fraction_mask;
  const PointBits overflow_first{high, high, 0U};
  const PointBits overflow_second{bits(-1.0), bits(-1.0), 0U};
  check(
      fixed_level(overflow_first, overflow_second).code ==
          pair_level::ResultCode::requires_wide,
      "a two-axis 256-bit accumulator overflow must require the wide lane");
  check(
      oracle_level(overflow_first, overflow_second).numerator() > 0,
      "the overflow fixture remains a valid positive BigInt level");

  constexpr std::uint64_t maximum_finite = UINT64_C(0x7fefffffffffffff);
  constexpr std::uint64_t negative_maximum_finite =
      UINT64_C(0xffefffffffffffff);
  constexpr std::uint64_t negative_minimum_subnormal =
      UINT64_C(0x8000000000000001);
  const PointBits wide_first{
      maximum_finite, maximum_finite, maximum_finite};
  const PointBits wide_second{
      negative_maximum_finite,
      negative_maximum_finite,
      negative_minimum_subnormal};
  const pair_level::Result wide = fixed_level(wide_first, wide_second);
  check(
      wide.code == pair_level::ResultCode::requires_wide,
      "the attainable 4200-bit level must require the wide lane");
  const ExactLevel wide_oracle = oracle_level(wide_first, wide_second);
  const BigInt& numerator = wide_oracle.numerator();
  check(
      (numerator >> 4199U) != 0 && (numerator >> 4200U) == 0 &&
          (numerator & 1) != 0 &&
          wide_oracle.denominator() ==
              morsehgp3d::exact::power_of_two(2150U),
      "the permanent extreme witness has an odd 4200-bit numerator over 2^2150");
}

void test_status_boundaries_and_bigint_differential() {
  const PointBits ordinary = point(1.0, -2.0, 3.0);
  PointBits nonfinite = ordinary;
  nonfinite[1] = UINT64_C(0x7ff0000000000000);
  check(
      fixed_level(ordinary, ordinary).code ==
              pair_level::ResultCode::zero_duplicate &&
          fixed_level(ordinary, nonfinite).code ==
              pair_level::ResultCode::invalid_input,
      "duplicates and nonfinite coordinates have distinct fail-closed statuses");

  const std::vector<std::pair<PointBits, PointBits>> fixtures{
      {point(0.0, 0.0, 0.0), point(1.0, 2.0, 3.0)},
      {point(-16.0, 0.5, 7.0), point(8.0, -0.25, -9.0)},
      {point(0x1.0p-500, 0x1.0p-450, 0x1.0p-400),
       point(-0x1.0p-500, 0.0, 0x1.0p-401)},
      {point(0x1.0p400, 0x1.0p350, -0x1.0p300),
       point(0x1.0p399, -0x1.0p350, 0x1.0p299)},
      {point(0.1, 0.2, 0.3), point(-0.4, 0.5, -0.6)},
  };
  std::vector<pair_level::Result> exact_results;
  exact_results.reserve(fixtures.size());
  for (std::size_t index = 0U; index < fixtures.size(); ++index) {
    const auto& [first, second] = fixtures[index];
    const pair_level::Result result = fixed_level(first, second);
    check(
        pair_level::result_is_exact(result),
        "bounded differential fixture must fit the fixed lane");
    if (pair_level::result_is_exact(result)) {
      check(
          materialize(result) == oracle_level(first, second),
          "bounded fixed result must equal its BigInt ExactLevel oracle");
      exact_results.push_back(result);
    }
  }

  for (std::size_t left = 0U; left < exact_results.size(); ++left) {
    for (std::size_t right = 0U; right < exact_results.size(); ++right) {
      const ExactLevel left_oracle = materialize(exact_results[left]);
      const ExactLevel right_oracle = materialize(exact_results[right]);
      const pair_level::ComparisonCode expected =
          left_oracle < right_oracle
              ? pair_level::ComparisonCode::less
          : left_oracle > right_oracle
              ? pair_level::ComparisonCode::greater
              : pair_level::ComparisonCode::equal;
      check(
          pair_level::compare(exact_results[left], exact_results[right]) ==
              expected,
          "the fixed comparator must agree with ExactLevel strong ordering");
      const auto left_prefix = pair_level::radix_prefix(exact_results[left]);
      const auto right_prefix = pair_level::radix_prefix(exact_results[right]);
      const auto prefix_order =
          pair_level::compare_radix_prefixes(left_prefix, right_prefix);
      if (left_prefix.key != right_prefix.key) {
        check(
            (prefix_order == pair_level::RadixPrefixComparison::less) ==
                (expected == pair_level::ComparisonCode::less),
            "a distinct radix prefix must have the exact ordering direction");
      } else {
        check(
            prefix_order == pair_level::RadixPrefixComparison::
                                collision_requires_exact_refinement,
            "an equal radix prefix must remain explicitly unresolved");
      }
    }
  }
}

}  // namespace

int main() {
  test_minimum_subnormal_and_power_boundaries();
  test_permutations_endpoint_exchange_and_equality();
  test_exact_comparator_and_certified_radix_prefix();
  test_fixed_overflow_and_universal_4200_bit_witness();
  test_status_boundaries_and_bigint_differential();
  if (failures != 0) {
    std::cerr << failures << " exact fixed pair-level test(s) failed\n";
    return 1;
  }
  std::cout << "exact fixed pair-level tests passed\n";
  return 0;
}
