#include "core/fixed_signed.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
using Integer = mhgp8::float32_ball_detail::FixedSigned;
static_assert(std::is_same_v<decltype(std::declval<const Integer&>().magnitude_words()),
                             std::span<const std::uint32_t>>);

struct Work {
  std::uint64_t tests{}, exact_divisions{}, gcd_checks{}, shift_checks{};
  std::uint64_t nonexact_rejections{}, invalid_divisor_rejections{}, overflow_rejections{};
};

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

Integer small(int value) {
  const auto wide = static_cast<std::int64_t>(value);
  const auto result = Integer::from_unsigned(static_cast<std::uint32_t>(wide < 0 ? -wide : wide));
  return value < 0 ? -result : result;
}

Integer old_power_of_two(unsigned bits) {
  auto result = Integer::from_unsigned(1);
  const auto two = Integer::from_unsigned(2);
  for (unsigned i = 0; i != bits; ++i) result = result * two;
  return result;
}

unsigned unsigned_gcd(unsigned a, unsigned b) {
  while (b != 0) {
    const auto remainder = a % b;
    a = b;
    b = remainder;
  }
  return a;
}

void exercise(Work& work) {
  const auto check = [&](bool condition, const char* message) {
    ++work.tests;
    require(condition, message);
  };
  const auto equal = [&](const Integer& actual, const Integer& expected, const char* message) {
    check(actual.compare(expected) == 0 && (actual - expected).sign() == 0, message);
  };
  const auto exact = [&](const Integer& numerator, const Integer& divisor, const Integer& expected) {
    const auto before_n = numerator, before_d = divisor;
    const auto quotient = numerator.divided_exact(divisor);
    equal(quotient, expected, "exact quotient differs");
    equal(quotient * divisor, numerator, "quotient does not reconstruct dividend");
    equal(numerator, before_n, "division changed dividend");
    equal(divisor, before_d, "division changed divisor");
    ++work.exact_divisions;
  };
  const auto nonexact = [&](const Integer& numerator, const Integer& divisor) {
    const auto before_n = numerator, before_d = divisor;
    bool rejected = false;
    try { static_cast<void>(numerator.divided_exact(divisor)); }
    catch (const std::domain_error&) { rejected = true; }
    check(rejected, "nonexact division was accepted");
    equal(numerator, before_n, "nonexact refusal changed dividend");
    equal(divisor, before_d, "nonexact refusal changed divisor");
    ++work.nonexact_rejections;
  };
  const auto gcd = [&](const Integer& a, const Integer& b, const Integer& expected) {
    const auto before_a = a, before_b = b;
    const auto result = Integer::gcd(a, b);
    equal(result, expected, "binary gcd differs from independent expectation");
    check(result.sign() >= 0, "gcd is negative");
    equal(a, before_a, "gcd changed first operand");
    equal(b, before_b, "gcd changed second operand");
    ++work.gcd_checks;
  };
  const auto zero = Integer::from_unsigned(0), one = Integer::from_unsigned(1), two = Integer::from_unsigned(2);
  check(zero.magnitude_words().empty() && (-zero).magnitude_words().empty() &&
        zero.trailing_zero_bits() == 1728 && zero.abs().sign() == 0, "zero export/valuation is not canonical");
  for (int a = -16; a <= 16; ++a) {
    for (int b = -16; b <= 16; ++b) {
      const auto left = small(a), right = small(b);
      const int expected = (a > b) - (a < b);
      check(left.compare(right) == expected, "signed comparison differs");
      equal(left.abs(), small(a < 0 ? -a : a), "absolute value differs");
      gcd(left, right, Integer::from_unsigned(unsigned_gcd(
          static_cast<unsigned>(a < 0 ? -a : a), static_cast<unsigned>(b < 0 ? -b : b))));
    }
  }
  for (int value = -32; value <= 32; ++value) {
    for (int divisor = 1; divisor <= 8; ++divisor) {
      if (value % divisor == 0) exact(small(value), small(divisor), small(value / divisor));
      else nonexact(small(value), small(divisor));
    }
  }

  // Independent small-word expectations use unsigned 64-bit arithmetic.
  constexpr std::uint32_t word = 0xfedcba98U;
  const auto sample = Integer::from_unsigned(word);
  for (unsigned shift = 0; shift <= 31; ++shift) {
    const auto shifted = sample.shifted_left(shift);
    const auto expected = static_cast<std::uint64_t>(word) << shift;
    const auto words = shifted.magnitude_words();
    check(words.size() == (expected >> 32 ? 2U : 1U) && words[0] == static_cast<std::uint32_t>(expected) &&
          (words.size() == 1 || words[1] == static_cast<std::uint32_t>(expected >> 32)), "left-shift words differ");
    equal(shifted.shifted_right(shift), sample, "cross-word shift roundtrip differs");
    equal(sample.shifted_right(shift), Integer::from_unsigned(word >> shift), "right-shift magnitude differs");
    equal((-sample).shifted_right(shift), -Integer::from_unsigned(word >> shift), "negative right shift is not toward zero");
    ++work.shift_checks;
  }
  for (const auto exponent : std::array<unsigned, 15>{0, 1, 31, 32, 33, 63, 64, 65, 127, 128,
                                                    277, 1023, 1695, 1696, 1727}) {
    const auto expected = old_power_of_two(exponent);
    const auto shifted = one.shifted_left(exponent);
    equal(shifted, expected, "power-of-two shift differs from existing multiplication");
    equal((-one).shifted_left(exponent), -expected, "negative left shift differs");
    check(shifted.trailing_zero_bits() == exponent && (-shifted).trailing_zero_bits() == exponent,
          "multiword trailing-zero count differs");
    check(shifted.magnitude_words().size() == exponent / 32 + 1 &&
          shifted.magnitude_words().back() == (std::uint32_t{1} << (exponent % 32)),
          "active magnitude export differs");
    for (std::size_t i = 0; i + 1 < shifted.magnitude_words().size(); ++i)
      check(shifted.magnitude_words()[i] == 0, "power-of-two inactive-low word differs");
    equal(shifted.shifted_right(exponent), one, "power-of-two right shift differs");
    equal(shifted.shifted_right(exponent + 1), zero, "right shift did not canonicalize zero");
    gcd(-shifted, zero, shifted);
    gcd(shifted, -shifted, shifted);
    ++work.shift_checks;
  }

  const auto high = old_power_of_two(1727);
  const auto full = (high - one) + high;
  check(full.magnitude_words().size() == 54 && full.trailing_zero_bits() == 0, "full-capacity magnitude export differs");
  for (const auto limb : full.magnitude_words()) check(limb == 0xffffffffU, "maximum magnitude lost a word");
  check(full.compare(-full) == 1 && (-full).compare(full) == -1, "full-width comparison overflowed or reversed");
  equal(full.abs(), full, "positive full-width abs differs");
  equal((-full).abs(), full, "negative full-width abs differs");
  equal(full.shifted_right(1727), one, "full-width right shift differs");
  equal((-full).shifted_right(1727), -one, "negative full-width right shift differs");
  for (const auto shift : std::array<unsigned, 2>{1728, std::numeric_limits<unsigned>::max()}) {
    equal(full.shifted_right(shift), zero, "oversized right shift differs");
    equal((-full).shifted_right(shift), zero, "negative oversized right shift differs");
    equal(zero.shifted_left(shift), zero, "zero left shift wrongly overflowed");
  }
  for (const auto& input : std::array<Integer, 3>{one, full, -full}) {
    const auto before = input;
    for (const auto shift : std::array<unsigned, 2>{1728, std::numeric_limits<unsigned>::max()}) {
      bool rejected = false;
      try { static_cast<void>(input.shifted_left(shift)); }
      catch (const std::overflow_error&) { rejected = true; }
      check(rejected, "oversized nonzero left shift was accepted");
      equal(input, before, "overflowing left shift changed its operand");
      ++work.overflow_rejections;
    }
  }
  for (const auto& input : std::array<Integer, 3>{high, full, -full}) {
    const auto before = input;
    bool rejected = false;
    try { static_cast<void>(input.shifted_left(1)); }
    catch (const std::overflow_error&) { rejected = true; }
    check(rejected, "top carry from left shift was lost");
    equal(input, before, "top-carry refusal changed operand");
    ++work.overflow_rejections;
  }

  exact(full, one, full);
  exact(-full, one, -full);
  exact(full, full, one);
  exact(-full, full, -one);
  exact(high, two, old_power_of_two(1726));
  exact(zero, full, zero);
  const auto base_minus_one = Integer::from_unsigned(0xffffffffU);
  const auto quotient = full.divided_exact(base_minus_one);
  check(quotient.magnitude_words().size() == 54, "cross-limb geometric quotient has wrong width");
  for (const auto limb : quotient.magnitude_words()) check(limb == 1, "cross-limb geometric quotient differs");
  exact(full, base_minus_one, quotient);
  nonexact(full, two);
  nonexact(full, high);
  nonexact(full, full - one);
  nonexact(one, full);
  nonexact(-one, full);
  for (const auto& dividend : std::array<Integer, 3>{zero, one, -full}) {
    for (const auto& divisor : std::array<Integer, 3>{zero, -one, -full}) {
      const auto before_n = dividend, before_d = divisor;
      bool rejected = false;
      try { static_cast<void>(dividend.divided_exact(divisor)); }
      catch (const std::invalid_argument&) { rejected = true; }
      check(rejected, "nonpositive divisor was accepted");
      equal(dividend, before_n, "invalid-divisor refusal changed dividend");
      equal(divisor, before_d, "invalid-divisor refusal changed divisor");
      ++work.invalid_divisor_rejections;
    }
  }

  gcd(full, full - one, one);
  gcd(-full, base_minus_one, base_minus_one);
  const auto p = old_power_of_two(300), factor = old_power_of_two(64) + one;
  gcd((p - one) * factor, (p + one) * factor, factor);
  // Consecutive Fibonacci numbers are coprime; these reach >1500 bits.
  auto a = one, b = one;
  for (unsigned i = 0; i != 2200; ++i) {
    auto next = a + b;
    a = b;
    b = next;
  }
  gcd(a, b, one);
  gcd(-a.shifted_left(97), b.shifted_left(97), one.shifted_left(97));
  require(work.tests > 8000 && work.exact_divisions > 100 && work.gcd_checks > 1100 &&
          work.shift_checks == 47 && work.nonexact_rejections > 200 &&
          work.invalid_divisor_rejections == 9 && work.overflow_rejections == 9,
          "division gate lost required test coverage");
}

}  // namespace

int main(int argc, char** argv) {
  Work work;
  try {
    require(argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--selftest"),
            "usage: fixed_signed_division_gate [--selftest]");
    exercise(work);
    std::cout << "{\"status\":\"passed\",\"tests\":" << work.tests
              << ",\"exact_divisions\":" << work.exact_divisions << ",\"gcd_checks\":" << work.gcd_checks
              << ",\"shift_checks\":" << work.shift_checks << ",\"nonexact_rejections\":" << work.nonexact_rejections
              << ",\"invalid_divisor_rejections\":" << work.invalid_divisor_rejections
              << ",\"overflow_rejections\":" << work.overflow_rejections << "}\n";
    require(static_cast<bool>(std::cout), "division gate output stream failed");
    return 0;
  } catch (const std::exception& error) {
    std::cout << "{\"status\":\"failed\",\"tests\":" << work.tests << "}\n";
    std::cerr << "fixed signed division gate: " << error.what() << '\n';
    return 1;
  }
}
