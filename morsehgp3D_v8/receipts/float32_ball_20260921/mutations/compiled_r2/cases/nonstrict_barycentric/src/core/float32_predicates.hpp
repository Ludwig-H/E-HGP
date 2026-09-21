#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

#if defined(__FAST_MATH__)
#error "mhgp8 float32 interval predicates require compilation without fast-math"
#endif

namespace mhgp8 {

// A finite IEEE-754 binary32 POINT supplied as its three original bit words.
// No rounding, quantization, translation or conversion to the u16 engine.
// Both signed zero encodings are legal. NaNs and infinities are rejected
// before a value is published. Immutable/nonforgeable; copying is harmless.
class Float32Point3 final {
 public:
  [[nodiscard]] static Float32Point3 from_bits(std::array<std::uint32_t, 3> bits) {
    for (const auto word : bits) {
      if ((word & 0x7f800000U) == 0x7f800000U)
        throw std::invalid_argument("mhgp8 binary32 point requires finite coordinates");
    }
    return Float32Point3(bits);
  }
  [[nodiscard]] const std::array<std::uint32_t, 3>& bits() const noexcept { return bits_; }

 private:
  explicit Float32Point3(std::array<std::uint32_t, 3> bits) noexcept : bits_(bits) {}
  const std::array<std::uint32_t, 3> bits_;
};

enum class Float32PredicateMode { ExactOnly, Filtered };

// All fields add by SUM; these are operations, not times or memory sizes.
struct Float32PredicateWork {
  std::uint64_t queries{};
  std::uint64_t exact_queries{};
  std::uint64_t filter_queries{};
  std::uint64_t filter_accepts{};
  std::uint64_t filter_fallbacks{};
  std::uint64_t exact_terms{};  // Twelve algebraic products/query, zeros included.
  std::uint64_t filter_axis_products{};  // Four-corner interval products.
  std::uint64_t filter_interval_additions{};
  bool operator==(const Float32PredicateWork&) const = default;
};

namespace float32_predicate_detail {

inline void count(std::uint64_t& value, std::uint64_t increment = 1) {
  if (increment > std::numeric_limits<std::uint64_t>::max() - value)
    throw std::overflow_error("mhgp8 binary32 predicate counter overflow");
  value += increment;
}

struct Decoded {
  std::uint32_t mantissa{};
  int exponent{};
  bool negative{};
};

// value=(-1)^negative * mantissa * 2^exponent. This also decodes either
// zero exactly (mantissa=0), and has no floating-point operations.
[[nodiscard]] inline Decoded decode(std::uint32_t bits) noexcept {
  const auto exponent = (bits >> 23) & 0xffU;
  const auto fraction = bits & 0x007fffffU;
  return {exponent == 0 ? fraction : fraction | 0x00800000U,
          exponent == 0 ? -149 : static_cast<int>(exponent) - 150,
          (bits & 0x80000000U) != 0};
}

class Accumulator final {
 public:
  // Each finite binary32 mantissa has <2^24 and exponent in [-149,104].
  // Product mantissas fit u64 (<2^48). In units of 2^-298, the shift is
  // in [0,506], hence each term is <2^554. Twelve terms have magnitude
  // <12*2^554<2^558, strictly below these 18*32=576 bits. There is no
  // heap allocation, i128 dependency, signed shift, or search-size bound.
  void add(std::uint64_t product, unsigned shift) {
    if (shift > 506)
      throw std::logic_error("mhgp8 finite binary32 product has invalid exponent");
    const auto limb = static_cast<std::size_t>(shift / 32U);
    const auto offset = shift % 32U;
    const std::uint64_t low = (product & 0xffffffffULL) << offset;
    const std::uint64_t high = (product >> 32) << offset;
    add_word(limb, static_cast<std::uint32_t>(low));
    add_word(limb + 1, static_cast<std::uint32_t>(low >> 32));
    add_word(limb + 1, static_cast<std::uint32_t>(high));
    add_word(limb + 2, static_cast<std::uint32_t>(high >> 32));
  }

  [[nodiscard]] int compare(const Accumulator& other) const noexcept {
    for (std::size_t i = limbs_.size(); i != 0; --i) {
      if (limbs_[i - 1] < other.limbs_[i - 1]) return -1;
      if (limbs_[i - 1] > other.limbs_[i - 1]) return 1;
    }
    return 0;
  }

 private:
  void add_word(std::size_t limb, std::uint32_t word) {
    std::uint64_t carry = word;
    while (carry != 0) {
      if (limb == limbs_.size())
        throw std::logic_error("mhgp8 binary32 accumulator exceeded its proven capacity");
      const std::uint64_t sum = static_cast<std::uint64_t>(limbs_[limb]) + carry;
      limbs_[limb] = static_cast<std::uint32_t>(sum);
      carry = sum >> 32;
      ++limb;
    }
  }
  std::array<std::uint32_t, 18> limbs_{};
};

[[nodiscard]] inline int exact(const Float32Point3& a, const Float32Point3& b,
                               const Float32Point3& z, Float32PredicateWork& work) {
  count(work.exact_queries);
  Accumulator positive, negative;
  const auto term = [&](Decoded left, Decoded right, bool subtract) {
    count(work.exact_terms);
    const auto product = static_cast<std::uint64_t>(left.mantissa) * right.mantissa;
    const auto shift = static_cast<unsigned>(left.exponent + right.exponent + 298);
    const bool is_negative = (left.negative != right.negative) != subtract;
    (is_negative ? negative : positive).add(product, shift);
  };
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const auto av = decode(a.bits()[axis]);
    const auto bv = decode(b.bits()[axis]);
    const auto zv = decode(z.bits()[axis]);
    // (z-a).(z-b) = sum(z*z-z*a-z*b+a*b). Keep positive/negative
    // sums separate: cancellation never rounds and +/-zero adds nothing.
    term(zv, zv, false);
    term(zv, av, true);
    term(zv, bv, true);
    term(av, bv, false);
  }
  return positive.compare(negative);
}

static_assert(std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::radix == 2 &&
              std::numeric_limits<double>::digits >= 53 &&
              std::numeric_limits<double>::max_exponent >= 1024 &&
              std::numeric_limits<double>::min_exponent <= -1021,
              "mhgp8 interval filter requires IEEE binary64 or wider double");

[[nodiscard]] inline double as_double(std::uint32_t bits) {
  const auto value = decode(bits);
  // Exact: <=24 significant bits and exponent >=-149 are normal in double.
  // Do not bit_cast to float first: a caller's DAZ setting could flush a
  // binary32 subnormal while converting that intermediate to double.
  const double magnitude = std::ldexp(static_cast<double>(value.mantissa), value.exponent);
  return value.negative ? -magnitude : magnitude;
}

struct Interval { double low{}, high{}; };

[[nodiscard]] inline double down(double value) {
  const auto lower = std::nextafter(value, -std::numeric_limits<double>::infinity());
  // Widen tiny endpoints to NORMAL double numbers. This also encloses an
  // underflow flushed to zero, without exposing subnormal interval operands.
  const auto normal = std::numeric_limits<double>::min();
  return lower > -normal && lower < normal ? -normal : lower;
}
[[nodiscard]] inline double up(double value) {
  const auto upper = std::nextafter(value, std::numeric_limits<double>::infinity());
  const auto normal = std::numeric_limits<double>::min();
  return upper > -normal && upper < normal ? normal : upper;
}
[[nodiscard]] inline Interval difference(double a, double b) {
  const double value = a - b;
  return {down(value), up(value)};
}
[[nodiscard]] inline Interval multiply(Interval a, Interval b) {
  const std::array<double, 4> products{
      a.low * b.low, a.low * b.high, a.high * b.low, a.high * b.high};
  Interval result{down(products[0]), up(products[0])};
  for (std::size_t i = 1; i != products.size(); ++i) {
    result.low = std::min(result.low, down(products[i]));
    result.high = std::max(result.high, up(products[i]));
  }
  return result;
}
[[nodiscard]] inline Interval add(Interval a, Interval b) {
  return {down(a.low + b.low), up(a.high + b.high)};
}

[[nodiscard]] inline int filter(const Float32Point3& a, const Float32Point3& b,
                                const Float32Point3& z, Float32PredicateWork& work) {
  count(work.filter_queries);
  Interval total{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const double av = as_double(a.bits()[axis]);
    const double bv = as_double(b.bits()[axis]);
    const double zv = as_double(z.bits()[axis]);
    const auto left = difference(zv, av), right = difference(zv, bv);
    count(work.filter_axis_products);
    const auto product = multiply(left, right);
    count(work.filter_interval_additions);
    total = add(total, product);
  }
  // Every finite binary32 coordinate has magnitude <2^128, so differences,
  // products and three-term sums (including outward ulps) are <2^260. No
  // double overflow occurs. Rounded elementary operations are enclosed by
  // nextafter in both directions; no epsilon decides a sign or equality.
  if (total.low > 0) return 1;
  if (total.high < 0) return -1;
  return 0;  // UNKNOWN, including every possible exact zero.
}

}  // namespace float32_predicate_detail

// Sign of the EXACT rational power (z-a).(z-b) of finite binary32 inputs.
// -1: strict diameter-ball interior; 0: contact; +1: exterior. Repeated
// endpoints are permitted as an algebraic query, not a valid q2 support.
// ExactOnly performs NO floating arithmetic. Filtered falls back to precisely
// that path unless an outward interval separates zero. It cannot certify a
// zero itself. Compile without fast-math and with -ffp-contract=off.
//
// Invalid mode leaves work unchanged. Counter exceptions can leave a partial
// work record, never a fabricated sign. Const points may be shared; mutable
// work is exclusive per call. Fixed integer arrays are accelerator-friendly
// arithmetic, but this is a CPU primitive, NOT a GPU or full-engine port.
[[nodiscard]] inline int q2_power_sign(const Float32Point3& a, const Float32Point3& b,
                                      const Float32Point3& z, Float32PredicateMode mode,
                                      Float32PredicateWork& work) {
  if (mode != Float32PredicateMode::ExactOnly && mode != Float32PredicateMode::Filtered)
    throw std::invalid_argument("mhgp8 invalid binary32 predicate mode");
  float32_predicate_detail::count(work.queries);
  if (mode == Float32PredicateMode::Filtered) {
    const auto sign = float32_predicate_detail::filter(a, b, z, work);
    if (sign != 0) {
      float32_predicate_detail::count(work.filter_accepts);
      return sign;
    }
    float32_predicate_detail::count(work.filter_fallbacks);
  }
  return float32_predicate_detail::exact(a, b, z, work);
}

}  // namespace mhgp8
