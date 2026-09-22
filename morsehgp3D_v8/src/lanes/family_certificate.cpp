#include "lanes/family_certificate.hpp"

#include <bit>
#include <stdexcept>

namespace mhgp8 {
namespace {

i64 distance_squared(Point3 a, Point3 b) noexcept {
  i64 value = 0;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const i64 delta = static_cast<i64>(b[axis]) - a[axis];
    value += delta * delta;
  }
  return value;  // <=3*262143^2<2^38, including partial sums.
}

struct RootBound {
  i64 ceiling;
  u64 iterations;
};

RootBound ceil_sqrt(i128 target) {
  if (target <= 0) throw std::logic_error("mhgp8 family square root requires a positive target");
  // For this caller target=ceil(J/2)<2^114. Start with a power-of-two upper
  // enclosure obtained from its exact bit length, NOT a replacement for U.
  const u64 high_word = static_cast<u64>(target >> 64);
  const unsigned bits = high_word != 0
      ? 64U + static_cast<unsigned>(std::bit_width(high_word))
      : static_cast<unsigned>(std::bit_width(static_cast<u64>(target)));
  i64 low = 0;
  i64 high = i64{1} << ((bits + 1U) / 2U);
  u64 iterations = 0;
  // Invariant: low^2<target<=high^2. The interval halves at each step,
  // hence at most 58 iterations at 18 bits (high<=2^58); no search/candidate quota exists.
  while (high - low > 1) {
    const i64 middle = low + (high - low) / 2;
    const i128 square = static_cast<i128>(middle) * middle;
    if (square >= target) high = middle;
    else low = middle;
    ++iterations;
  }
  // Independently verify the adjacent-square enclosure before publishing U.
  // high<=2^58, so BOTH products are <=2^116 and fit signed i128.
  const i128 previous = static_cast<i128>(high) - 1;
  if (static_cast<i128>(high) * high < target || previous * previous >= target)
    throw std::logic_error("mhgp8 family square root enclosure failed");
  return {high, iterations};
}

}  // namespace

Q34FamilyCertificate::Q34FamilyCertificate(const Q4FamilySeed& family, i128 j_bound,
    i64 parameter_bound, u64 sqrt_iterations) noexcept
    : family_(family), j_bound_(j_bound), parameter_bound_(parameter_bound),
      sqrt_iterations_(sqrt_iterations) {}

std::optional<Q34FamilyCertificate> Q34FamilyCertificate::make(Point3 a, Point3 b, Point3 x) {
  require_valid_point(a);
  require_valid_point(b);
  require_valid_point(x);
  const i64 d = distance_squared(a, b);
  const i64 e = distance_squared(a, x);
  const i64 other = distance_squared(b, x);
  if (d < e || d < other) return std::nullopt;
  const auto family = Q4FamilySeed::make(a, b, x);
  if (!family) return std::nullopt;
  const i128 gram = family->gram();
  const i128 j = static_cast<i128>(d) * (3 * gram - 2 * static_cast<i128>(e) * other);
  // A longest edge in an acute triangle gives R0^2=D*E*X/(4G)<=D/3.
  // Thus J=D*(3G-2EX)>=D*G/3>0, without testing an approximate radius.
  if (j <= 0) throw std::logic_error("mhgp8 acute maximal-edge family has nonpositive J");
  // M=262143, D/E/X<=3M^2, 0<G<=D*E<=9M^4. Hence J<=81M^6<2^115.
  // Every multiplication above is promoted before evaluation. The rounded
  // half avoids J+1 and the square root never squares P or a rational root.
  const i128 target = j / 2 + j % 2;
  const auto root = ceil_sqrt(target);
  return Q34FamilyCertificate(*family, j, root.ceiling, root.iterations);
}

Q34FamilyWitness Q34FamilyCertificate::witness(Point3 z) const noexcept {
  const i128 power = family_.power(z);
  const i128 side = family_.side(z);
  const i128 absolute_side = side < 0 ? -side : side;
  // G<=9M^4 and |W_i|<=36M^5 imply |P|<=27M^6+108M^6=135M^6;
  // the normal components give |B|<=6M^3. Since
  // ceil(J/2)<=49M^6, the exact root satisfies U<=7M^3<2^57. Therefore
  // |P|+U*|B|<=177M^6<2^116; product, addition and sign test fit i128.
  // If |mu|<=U, P-mu*B<=P+U*|B|. Strict negativity certifies every
  // positive q4 completion with maximal edge ab, including both mu signs.
  const i128 maximum_power = power + static_cast<i128>(parameter_bound_) * absolute_side;
  return {power < 0, maximum_power < 0};
}

}  // namespace mhgp8
