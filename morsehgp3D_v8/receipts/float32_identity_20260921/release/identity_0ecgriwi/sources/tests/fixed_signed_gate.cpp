#include "core/fixed_signed.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

using Integer = mhgp8::float32_ball_detail::FixedSigned;

struct Results {
  std::uint64_t tests{};
  std::uint64_t overflow_rejections{};
  std::uint64_t invalid_rejections{};
};

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

Integer small(int value) {
  const auto wide = static_cast<std::int64_t>(value);
  const auto magnitude = static_cast<std::uint32_t>(wide < 0 ? -wide : wide);
  const auto result = Integer::from_unsigned(magnitude);
  return value < 0 ? -result : result;
}

Integer power_of_two(unsigned exponent) {
  auto result = Integer::from_unsigned(1);
  const auto two = Integer::from_unsigned(2);
  for (unsigned i = 0; i != exponent; ++i) result = result * two;
  return result;
}

void exercise(Results& result) {
  const auto check = [&](bool condition, const char* message) {
    ++result.tests;
    require(condition, message);
  };
  const auto equal = [&](const Integer& actual, const Integer& expected, const char* message) {
    // Equal operands subtract without overflow even at the capacity boundary.
    // A discrepant huge value may throw: that is a gate failure, not a pass.
    check((actual - expected).sign() == 0, message);
  };
  const auto reject_overflow = [&](const Integer& left, const Integer& right, const auto& operation) {
    const auto before_left = left, before_right = right;
    bool rejected = false;
    try {
      static_cast<void>(operation(left, right));
    } catch (const std::overflow_error&) {
      rejected = true;
    }
    check(rejected, "out-of-capacity arithmetic was not rejected");
    equal(left, before_left, "overflow changed its left operand");
    equal(right, before_right, "overflow changed its right operand");
    ++result.overflow_rejections;
  };
  const auto zero = Integer::from_unsigned(0);
  const auto one = Integer::from_unsigned(1);
  const auto two = Integer::from_unsigned(2);
  check(Integer::limb_count == 54, "fixed integer capacity changed");
  check(Integer{}.sign() == 0 && zero.sign() == 0 && (-zero).sign() == 0,
        "default or negated zero has a nonzero sign");
  check(one.sign() == 1 && (-one).sign() == -1, "integer unit signs differ");

  // Independent small signed expectations use built-in values in [-16,16]
  // for addition/subtraction and [-64,64] for multiplication.
  for (int a = -8; a <= 8; ++a) {
    for (int b = -8; b <= 8; ++b) {
      const auto left = small(a), right = small(b);
      equal(left + right, small(a + b), "small signed addition differs");
      equal(left - right, small(a - b), "small signed subtraction differs");
      equal(left * right, small(a * b), "small signed multiplication differs");
    }
  }

  equal(Integer::from_bits(0), zero, "positive binary32 zero differs");
  equal(Integer::from_bits(0x80000000U), zero, "negative binary32 zero differs");
  check(Integer::from_bits(0x80000000U).sign() == 0, "negative-zero decoding is not canonical");
  equal(Integer::from_bits(1), one, "minimum subnormal has wrong integer scale");
  equal(Integer::from_bits(0x80000001U), -one, "negative minimum subnormal differs");
  equal(Integer::from_bits(2), two, "second subnormal differs");
  equal(Integer::from_bits(0x007fffffU), Integer::from_unsigned(0x007fffffU),
        "maximum subnormal differs");
  equal(Integer::from_bits(0x00800000U), Integer::from_unsigned(0x00800000U),
        "minimum normal differs");
  equal(Integer::from_bits(0x00800001U), Integer::from_unsigned(0x00800001U),
        "normal mantissa low bit disappeared");
  equal(Integer::from_bits(0x01000000U), Integer::from_unsigned(0x01000000U),
        "normal exponent shift differs");
  const auto unit_float = power_of_two(149);
  equal(Integer::from_bits(0x3f800000U), unit_float, "float one is not scaled by 2^149");
  equal(Integer::from_bits(0xbf800000U), -unit_float, "negative float one differs");
  equal(Integer::from_bits(0x40000000U), unit_float * two, "float two differs");
  equal(Integer::from_bits(0x3f000000U), power_of_two(148), "float half differs");
  const auto maximum_float = power_of_two(277) - power_of_two(253);
  equal(Integer::from_bits(0x7f7fffffU), maximum_float, "maximum finite binary32 differs");
  equal(Integer::from_bits(0xff7fffffU), -maximum_float, "negative maximum finite binary32 differs");
  equal(Integer::from_bits(0x7f7fffffU) + Integer::from_bits(0xff7fffffU), zero,
        "opposite extreme binary32 coordinates did not cancel");
  for (const auto invalid : std::array<std::uint32_t, 6>{
           0x7f800000U, 0xff800000U, 0x7f800001U, 0xff800001U, 0x7fc00000U, 0xffc00000U}) {
    bool rejected = false;
    try {
      static_cast<void>(Integer::from_bits(invalid));
    } catch (const std::invalid_argument&) {
      rejected = true;
    }
    check(rejected, "nonfinite binary32 encoding was accepted");
    ++result.invalid_rejections;
  }

  // Small residuals survive cancellation across the complete 277-bit input
  // range; these queries cannot be satisfied by rounding through double.
  equal((maximum_float + one) - maximum_float, one, "distant positive residual disappeared");
  equal((maximum_float - one) - maximum_float, -one, "distant negative residual disappeared");
  equal((maximum_float + one) + (-maximum_float), one, "opposite-sign distant cancellation differs");
  equal(-(-maximum_float), maximum_float, "double negation differs");
  const auto word_max = Integer::from_unsigned(0xffffffffU);
  equal(word_max + one, power_of_two(32), "single-word carry differs");
  equal(power_of_two(32) - one, word_max, "single-word borrow differs");
  equal(word_max * word_max, power_of_two(64) - power_of_two(33) + one,
        "maximal product-plus-carry arithmetic differs");
  const auto dense = power_of_two(256) - one;
  const auto dense_square = power_of_two(512) - power_of_two(257) + one;
  equal(dense * dense, dense_square, "dense multiword multiplication differs");
  equal((-dense) * dense, -dense_square, "negative multiword multiplication differs");
  equal((-dense) * (-dense), dense_square, "two negative factors differ");

  const auto high = power_of_two(1727);
  const auto lower = high - one;  // Borrow crosses 53 complete zero limbs.
  const auto full = lower + high;  // EXACT maximum magnitude 2^1728-1.
  const auto negative_full = -full;
  check(high.sign() == 1 && full.sign() == 1 && negative_full.sign() == -1,
        "capacity-boundary signs differ");
  equal(lower + one, high, "borrow/carry across 53 limbs differs");
  equal(full - high, lower, "maximum magnitude lost its lower bits");
  equal(full - lower, high, "maximum magnitude lost its top bit");
  equal(full - full, zero, "maximum magnitude self-subtraction differs");
  equal(full + negative_full, zero, "maximum opposite magnitudes did not cancel");
  equal(negative_full - negative_full, zero, "negative maximum self-subtraction differs");
  equal(full * one, full, "54-limb multiplication by one was rejected or changed");
  equal(one * full, full, "reverse 54-limb multiplication by one differs");
  equal(negative_full * (-one), full, "maximum negative multiplication differs");
  equal(full * zero, zero, "54-limb multiplication by zero differs");
  equal(zero * negative_full, zero, "reverse multiplication by zero differs");
  const auto high_word = power_of_two(1696);
  const auto bit31 = power_of_two(31), bit32 = power_of_two(32);
  // Active operand lengths sum to 55, but the result still fits 54 limbs.
  equal(high_word * bit31, high, "fitting 55-length-sum product was rejected");
  equal(bit31 * high_word, high, "reverse fitting 55-length-sum product differs");
  equal((high_word - one) + one, high_word, "53-limb all-ones carry differs");

  const auto add = [](const Integer& a, const Integer& b) { return a + b; };
  const auto subtract = [](const Integer& a, const Integer& b) { return a - b; };
  const auto multiply = [](const Integer& a, const Integer& b) { return a * b; };
  reject_overflow(full, one, add);
  reject_overflow(negative_full, -one, add);
  reject_overflow(full, -one, subtract);
  reject_overflow(negative_full, one, subtract);
  reject_overflow(full, two, multiply);
  reject_overflow(negative_full, two, multiply);
  reject_overflow(high_word, bit32, multiply);
  reject_overflow(bit32, high_word, multiply);
  reject_overflow(full, full, multiply);
  reject_overflow(high, two, multiply);

  // Floors are not the arithmetic oracle: every equality/refusal above was
  // checked explicitly, including preservation of BOTH operands on each throw.
  require(result.tests >= 940 && result.overflow_rejections == 10 && result.invalid_rejections == 6,
          "fixed integer gate lost required test coverage");
}

}  // namespace

int main(int argc, char** argv) {
  Results result;
  try {
    require(argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--selftest"),
            "usage: fixed_signed_gate [--selftest]");
    exercise(result);
    std::cout << "{\"status\":\"passed\",\"tests\":" << result.tests
              << ",\"overflow_rejections\":" << result.overflow_rejections
              << ",\"invalid_rejections\":" << result.invalid_rejections << "}\n";
    require(static_cast<bool>(std::cout), "gate output stream failed");
    return 0;
  } catch (const std::exception& error) {
    std::cout << "{\"status\":\"failed\",\"tests\":" << result.tests
              << ",\"overflow_rejections\":" << result.overflow_rejections
              << ",\"invalid_rejections\":" << result.invalid_rejections << "}\n";
    std::cerr << "fixed signed gate: " << error.what() << '\n';
    return 1;
  }
}
