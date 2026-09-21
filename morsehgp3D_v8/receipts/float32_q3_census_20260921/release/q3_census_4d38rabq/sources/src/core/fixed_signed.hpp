#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>

namespace mhgp8::float32_ball_detail {

// Signed magnitude with a fixed 1728-bit capacity, no dynamic storage, and
// arithmetic restricted to the active magnitude prefix. Zero is uniquely
// used_=0, negative_=false. Limbs outside the active prefix are always zero.
// This is an integer, NOT a fixed-point type: from_bits supplies coordinates
// in units of 2^-149; from_unsigned(1) supplies the multiplicative integer 1.
// Addition/subtraction require consistent units; callers track homogeneous
// polynomial degrees. Basic arithmetic never rounds, saturates or truncates.
// Right shifts explicitly truncate the magnitude; division requires an exact
// quotient. These additional integer operations do not change coordinate units.
//
// Domain justification for the current binary32 ball predicates: a finite
// coordinate integer has magnitude <2^277, hence each translated component
// has magnitude <M=2^278. For q3, G<=12*M^4, |W_i|<=36*M^5 and relative
// power <=144*M^6<2^1676. The global expansion is <=360*M^6<2^1677.
// For q4, |det|<=6*M^3, |N_i|<=18*M^4; power has degree5 and four-weight
// positivity intermediates are <=396*M^6<2^1677. Thus the proposed 2^1690
// envelope fits, with further headroom here. This is NOT a bound for arbitrary
// higher-degree formulas (notably unreduced products in event comparison).
// Every public operation still checks capacity independently of that proof.
class FixedSigned final {
 public:
  static constexpr std::size_t limb_count = 54;

  FixedSigned() noexcept = default;

  [[nodiscard]] static FixedSigned from_unsigned(std::uint32_t value) noexcept {
    FixedSigned result;
    if (value != 0) {
      result.limbs_[0] = value;
      result.used_ = 1;
    }
    return result;
  }

  [[nodiscard]] static FixedSigned from_bits(std::uint32_t bits) {
    const auto exponent = (bits >> 23) & 0xffU;
    if (exponent == 0xffU)
      throw std::invalid_argument("mhgp8 fixed integer requires finite binary32 bits");
    const auto fraction = bits & 0x007fffffU;
    const auto mantissa = exponent == 0 ? fraction : fraction | 0x00800000U;
    if (mantissa == 0) return {};  // Both signed zeros become integer zero.

    // Subnormals are integer multiples of 2^-149. For a normal, its 24-bit
    // mantissa is shifted by exponent_field-1, in [0,253], in that same unit.
    // Splitting a <=24-bit mantissa at offset<=31 never overflows u64.
    const unsigned shift = exponent == 0 ? 0 : exponent - 1;
    const auto first = static_cast<std::size_t>(shift / 32U);
    const auto value = static_cast<std::uint64_t>(mantissa) << (shift % 32U);
    FixedSigned result;
    result.limbs_[first] = static_cast<std::uint32_t>(value);
    result.limbs_[first + 1] = static_cast<std::uint32_t>(value >> 32);
    result.used_ = first + (result.limbs_[first + 1] == 0 ? 1 : 2);
    result.negative_ = (bits & 0x80000000U) != 0;
    return result;
  }

  [[nodiscard]] int sign() const noexcept {
    return used_ == 0 ? 0 : (negative_ ? -1 : 1);
  }

  [[nodiscard]] FixedSigned abs() const noexcept {
    FixedSigned result = *this;
    result.negative_ = false;
    return result;
  }

  // Signed comparison without forming a potentially overflowing difference.
  [[nodiscard]] int compare(const FixedSigned& other) const noexcept {
    const int mine = sign(), theirs = other.sign();
    if (mine != theirs) return mine < theirs ? -1 : 1;
    return mine == 0 ? 0 : mine * compare_magnitudes(*this, other);
  }

  // Little-endian base-2^32 magnitude, no leading zero word; zero is empty.
  // The view borrows this value and expires if it is destroyed or assigned.
  // Sign is separate: this is an exact export, not native-endian byte packing.
  [[nodiscard]] std::span<const std::uint32_t> magnitude_words() const noexcept {
    return {limbs_.data(), used_};
  }

  // Zero's sentinel is the fixed capacity, 1728, not an infinite valuation.
  [[nodiscard]] unsigned trailing_zero_bits() const noexcept {
    for (std::size_t i = 0; i != used_; ++i) {
      if (limbs_[i] != 0)
        return static_cast<unsigned>(32 * i) + static_cast<unsigned>(std::countr_zero(limbs_[i]));
    }
    return static_cast<unsigned>(32 * limb_count);
  }

  [[nodiscard]] FixedSigned shifted_left(unsigned bits) const {
    if (used_ == 0 || bits == 0) return *this;
    if (bits >= 32 * limb_count)
      throw std::overflow_error("mhgp8 fixed integer left shift exceeds capacity");
    const auto words = static_cast<std::size_t>(bits / 32U);
    const unsigned offset = bits % 32U;
    if (used_ + words > limb_count)
      throw std::overflow_error("mhgp8 fixed integer left shift exceeds capacity");
    FixedSigned result;
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i != used_; ++i) {
      // The old carry occupies only the low offset bits. Even with offset31,
      // the sum is <2^63 and the shift never reaches the u64 width.
      const auto value = (static_cast<std::uint64_t>(limbs_[i]) << offset) + carry;
      result.limbs_[i + words] = static_cast<std::uint32_t>(value);
      carry = value >> 32;
    }
    result.used_ = used_ + words;
    if (carry != 0) {
      if (result.used_ == limb_count)
        throw std::overflow_error("mhgp8 fixed integer left shift exceeds capacity");
      result.limbs_[result.used_++] = static_cast<std::uint32_t>(carry);
    }
    result.negative_ = negative_;
    return result;
  }

  // sign(x)*floor(|x|/2^bits): truncation TOWARD ZERO for negative values,
  // not signed arithmetic-floor shifting and not a divisibility certificate.
  [[nodiscard]] FixedSigned shifted_right(unsigned bits) const noexcept {
    const auto words = static_cast<std::size_t>(bits / 32U);
    if (words >= used_) return {};
    const unsigned offset = bits % 32U;
    FixedSigned result;
    result.used_ = used_ - words;
    result.negative_ = negative_;
    for (std::size_t i = 0; i != result.used_; ++i) {
      auto value = static_cast<std::uint64_t>(limbs_[i + words]) >> offset;
      if (offset != 0 && i + words + 1 < used_)
        value |= static_cast<std::uint64_t>(limbs_[i + words + 1]) << (32U - offset);
      // High bits belong to the next output limb; discarded low input bits
      // are exactly the documented remainder of division by a power of two.
      result.limbs_[i] = static_cast<std::uint32_t>(value);
    }
    result.normalize();
    return result;
  }

  // Binary GCD. Accepts either sign, gcd(0,x)=abs(x), gcd(0,0)=0.
  // Stripping powers of two and subtracting ordered positive magnitudes
  // cannot increase either operand. The restored common factor still fits.
  [[nodiscard]] static FixedSigned gcd(FixedSigned left, FixedSigned right) {
    left = left.abs();
    right = right.abs();
    if (left.used_ == 0) return right;
    if (right.used_ == 0) return left;
    const auto left_zeros = left.trailing_zero_bits();
    const auto right_zeros = right.trailing_zero_bits();
    const auto common = left_zeros < right_zeros ? left_zeros : right_zeros;
    left = left.shifted_right(left_zeros);
    do {
      right = right.shifted_right(right.trailing_zero_bits());
      if (compare_magnitudes(left, right) > 0) std::swap(left, right);
      right = subtract_magnitudes(right, left);
    } while (right.used_ != 0);
    return left.shifted_left(common);
  }

  // A strictly positive divisor is required, even for a zero dividend.
  // The quotient retains the dividend's sign; nonzero remainder is refused.
  // Shift the DIVISOR down, never a full-width remainder up. Every aligned
  // divisor has at most the dividend's bit length; subtraction only decreases
  // the remainder. No extra carry limb or widened temporary is needed.
  [[nodiscard]] FixedSigned divided_exact(const FixedSigned& positive_divisor) const {
    if (positive_divisor.sign() <= 0)
      throw std::invalid_argument("mhgp8 fixed integer division requires a positive divisor");
    if (used_ == 0) return {};
    auto remainder = abs();
    if (compare_magnitudes(remainder, positive_divisor) < 0)
      throw std::domain_error("mhgp8 fixed integer division has a nonzero remainder");
    const unsigned initial_shift = remainder.bit_length() - positive_divisor.bit_length();
    auto aligned = positive_divisor.shifted_left(initial_shift);
    FixedSigned quotient;
    for (unsigned bit = initial_shift;; --bit) {
      if (compare_magnitudes(remainder, aligned) >= 0) {
        remainder = subtract_magnitudes(remainder, aligned);
        const auto word = static_cast<std::size_t>(bit / 32U);
        quotient.limbs_[word] |= std::uint32_t{1} << (bit % 32U);
        if (word + 1 > quotient.used_) quotient.used_ = word + 1;
      }
      if (bit == 0) break;
      aligned = aligned.shifted_right(1);
    }
    if (remainder.used_ != 0)
      throw std::domain_error("mhgp8 fixed integer division has a nonzero remainder");
    quotient.negative_ = negative_;
    quotient.normalize();
    return quotient;
  }

  [[nodiscard]] friend FixedSigned operator-(const FixedSigned& value) noexcept {
    FixedSigned result = value;
    if (result.used_ != 0) result.negative_ = !result.negative_;
    return result;
  }

  [[nodiscard]] friend FixedSigned operator+(const FixedSigned& left, const FixedSigned& right) {
    if (left.used_ == 0) return right;
    if (right.used_ == 0) return left;
    if (left.negative_ == right.negative_) {
      auto result = add_magnitudes(left, right);
      result.negative_ = left.negative_;
      return result;
    }
    const int comparison = compare_magnitudes(left, right);
    if (comparison == 0) return {};
    const auto& larger = comparison > 0 ? left : right;
    const auto& smaller = comparison > 0 ? right : left;
    auto result = subtract_magnitudes(larger, smaller);
    result.negative_ = larger.negative_;
    return result;
  }

  [[nodiscard]] friend FixedSigned operator-(const FixedSigned& left, const FixedSigned& right) {
    return left + (-right);
  }

  [[nodiscard]] friend FixedSigned operator*(const FixedSigned& left, const FixedSigned& right) {
    if (left.used_ == 0 || right.used_ == 0) return {};
    // A nonzero product needs at least used_left+used_right-1 limbs. Equality
    // with the capacity is NOT an overflow: its final carry decides that case.
    const auto total = left.used_ + right.used_;
    if (total > limb_count + 1)
      throw std::overflow_error("mhgp8 fixed integer multiplication exceeds capacity");
    FixedSigned result;
    for (std::size_t i = 0; i != left.used_; ++i) {
      std::uint64_t carry = 0;
      for (std::size_t j = 0; j != right.used_; ++j) {
        // i+j <= total-2 <= limb_count-1 after the early capacity check.
        // For base B=2^32, (B-1)^2+(B-1)+(B-1)=B^2-1: u64 holds the
        // entire product, existing limb and carry WITHOUT unsigned wrapping.
        const std::uint64_t value = static_cast<std::uint64_t>(left.limbs_[i]) * right.limbs_[j]
                                  + result.limbs_[i + j] + carry;
        result.limbs_[i + j] = static_cast<std::uint32_t>(value);
        carry = value >> 32;
      }
      if (carry != 0) {
        const auto tail = i + right.used_;
        if (tail == limb_count)
          throw std::overflow_error("mhgp8 fixed integer multiplication exceeds capacity");
        // Previous rows end at <=tail-1. This is a new, still-zero limb;
        // their carries into the current row were consumed by its inner loop.
        result.limbs_[tail] = static_cast<std::uint32_t>(carry);
      }
    }
    result.used_ = total < limb_count ? total : limb_count;
    result.negative_ = left.negative_ != right.negative_;
    result.normalize();
    return result;
  }

 private:
  [[nodiscard]] unsigned bit_length() const noexcept {
    return used_ == 0 ? 0 : static_cast<unsigned>(32 * (used_ - 1)) +
        (32U - static_cast<unsigned>(std::countl_zero(limbs_[used_ - 1])));
  }

  [[nodiscard]] static int compare_magnitudes(const FixedSigned& left, const FixedSigned& right) noexcept {
    if (left.used_ != right.used_) return left.used_ < right.used_ ? -1 : 1;
    for (std::size_t i = left.used_; i != 0; --i) {
      if (left.limbs_[i - 1] != right.limbs_[i - 1])
        return left.limbs_[i - 1] < right.limbs_[i - 1] ? -1 : 1;
    }
    return 0;
  }

  [[nodiscard]] static FixedSigned add_magnitudes(const FixedSigned& left, const FixedSigned& right) {
    FixedSigned result;
    result.used_ = left.used_ > right.used_ ? left.used_ : right.used_;
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i != result.used_; ++i) {
      // Missing operand limbs are zero. A sum is at most 2*(2^32-1)+1.
      const std::uint64_t value = static_cast<std::uint64_t>(left.limbs_[i]) + right.limbs_[i] + carry;
      result.limbs_[i] = static_cast<std::uint32_t>(value);
      carry = value >> 32;
    }
    if (carry != 0) {
      if (result.used_ == limb_count)
        throw std::overflow_error("mhgp8 fixed integer addition exceeds capacity");
      result.limbs_[result.used_++] = static_cast<std::uint32_t>(carry);
    }
    return result;
  }

  [[nodiscard]] static FixedSigned subtract_magnitudes(const FixedSigned& larger, const FixedSigned& smaller) {
    FixedSigned result;
    result.used_ = larger.used_;
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i != larger.used_; ++i) {
      const auto amount = static_cast<std::uint64_t>(smaller.limbs_[i]) + borrow;
      const auto digit = static_cast<std::uint64_t>(larger.limbs_[i]);
      borrow = digit < amount ? 1 : 0;
      const auto value = digit + (borrow << 32) - amount;
      result.limbs_[i] = static_cast<std::uint32_t>(value);
    }
    if (borrow != 0)
      throw std::logic_error("mhgp8 fixed integer magnitude subtraction underflow");
    result.normalize();
    return result;
  }

  void normalize() noexcept {
    while (used_ != 0 && limbs_[used_ - 1] == 0) --used_;
    if (used_ == 0) negative_ = false;
  }

  std::array<std::uint32_t, limb_count> limbs_{};
  std::size_t used_{};
  bool negative_{};
};

}  // namespace mhgp8::float32_ball_detail
