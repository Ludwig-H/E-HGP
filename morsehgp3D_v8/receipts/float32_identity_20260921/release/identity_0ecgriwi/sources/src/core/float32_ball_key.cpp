#include "core/float32_ball_key.hpp"

#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {
using Integer = float32_ball_detail::FixedSigned;
using float32_predicate_detail::count;

std::array<Integer, 3> point(const Float32Point3& p, Float32KeyWork& work) {
  count(work.support.exact_point_decodes);
  return {Integer::from_bits(p.bits()[0]), Integer::from_bits(p.bits()[1]), Integer::from_bits(p.bits()[2])};
}
}  // namespace

std::optional<Float32BallKey> Float32BallKey::make_q2(
    const Float32Point3& a, const Float32Point3& b, Float32KeyWork& work) {
  count(work.q2_requests);
  const auto av = point(a, work), bv = point(b, work);
  bool distinct = false;
  for (std::size_t i = 0; i != 3; ++i) distinct = distinct || av[i].compare(bv[i]) != 0;
  if (!distinct) { count(work.rejected_supports); return std::nullopt; }
  std::array<Integer, 5> coefficients{};
  coefficients[0] = Integer::from_unsigned(1);
  for (std::size_t i = 0; i != 3; ++i) {
    count(work.support.exact_additions, 2);
    coefficients[i + 1] = -(av[i] + bv[i]);
    count(work.support.exact_products);
    coefficients[4] = coefficients[4] + av[i] * bv[i];
  }
  return canonical(std::move(coefficients), work);
}

std::optional<Float32BallKey> Float32BallKey::make_q3(
    std::array<Float32Point3, 3> points, Float32KeyWork& work) {
  count(work.q3_requests);
  const auto support = Float32Ball::make_q3(points, Float32PredicateMode::ExactOnly, work.support);
  if (!support) { count(work.rejected_supports); return std::nullopt; }
  return from_support(*support, work);
}

std::optional<Float32BallKey> Float32BallKey::make_q4(
    std::array<Float32Point3, 4> points, Float32KeyWork& work) {
  count(work.q4_requests);
  const auto support = Float32Ball::make_q4(points, Float32PredicateMode::ExactOnly, work.support);
  if (!support) { count(work.rejected_supports); return std::nullopt; }
  return from_support(*support, work);
}

Float32BallKey Float32BallKey::from_support(const Float32Ball& support, Float32KeyWork& work) {
  count(work.from_support_requests);
  return canonical(support.global_coefficients(work.support), work);
}

Float32BallKey Float32BallKey::canonical(std::array<Integer, 5> coefficients, Float32KeyWork& work) {
  if (coefficients[0].sign() <= 0)
    throw std::logic_error("mhgp8 float32 key requires positive quadratic coefficient");
  auto divisor = coefficients[0];
  for (std::size_t i = 1; i != coefficients.size(); ++i) {
    count(work.canonical_gcd_calls);
    divisor = Integer::gcd(divisor, coefficients[i]);
  }
  // Normalize exactly once, in the same global unit for EVERY arity.
  // Strip per-coefficient powers of two for storage only AFTER the common gcd.
  std::array<unsigned, 5> shifts{};
  std::array<bool, 5> negative{};
  std::size_t size = 1;
  for (std::size_t i = 0; i != coefficients.size(); ++i) {
    count(work.canonical_divisions);
    coefficients[i] = coefficients[i].divided_exact(divisor);
    negative[i] = coefficients[i].sign() < 0;
    if (coefficients[i].sign() == 0) { ++size; continue; }
    shifts[i] = coefficients[i].trailing_zero_bits();
    coefficients[i] = coefficients[i].abs().shifted_right(shifts[i]);
    size += 2 + coefficients[i].magnitude_words().size();
  }
  std::vector<std::uint32_t> words;
  words.reserve(size);
  words.push_back(1);
  for (std::size_t i = 0; i != coefficients.size(); ++i) {
    const auto magnitude = coefficients[i].magnitude_words();
    if (magnitude.empty()) { words.push_back(0); continue; }
    words.push_back(static_cast<std::uint32_t>(2 * magnitude.size() + (negative[i] ? 1 : 0)));
    words.push_back(shifts[i]);
    words.insert(words.end(), magnitude.begin(), magnitude.end());
  }
  if (words.size() != size) throw std::logic_error("mhgp8 float32 key encoding size mismatch");
  count(work.keys_created);
  count(work.packed_words, words.size());
  return Float32BallKey(std::move(words));
}

}  // namespace mhgp8
