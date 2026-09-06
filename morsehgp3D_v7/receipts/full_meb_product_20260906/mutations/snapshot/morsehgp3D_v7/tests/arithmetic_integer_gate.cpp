// Isolated arithmetic gate, 2026-09-04; no pipeline/exactness promotion.
// Root's explicit local scope: OBig + pinned literals when Boost is absent.
// The authority actually compiled is printed; no two-authority claim on absence.
// OBig Stein/division helpers explicitly port linked_arcs_gate.cpp:80-139,
// SHA256 cb11c5e16ba613ee87b2c27848adaa25d5722373f4d16ab5aa151cacfc047614.
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if __has_include(<boost/multiprecision/cpp_int.hpp>)
#define INTEGER_GATE_BOOST 1
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/version.hpp>
#else
#define INTEGER_GATE_BOOST 0
#define BOOST_LIB_VERSION "absent_local"
#endif

#include "oracle/obig.hpp"
#include "src/core/intmath.hpp"
#include "src/core/wide.hpp"
#include "src/lanes/keys.hpp"
#include "src/lanes/level.hpp"

namespace {
using namespace mhgp7;
using Big = mhgp7_oracle::OBig384;
#if INTEGER_GATE_BOOST
using boost::multiprecision::cpp_int;
#endif
constexpr u64 kRMinus1 = std::numeric_limits<u64>::max();
constexpr u128 kUMax = ~u128{0};
constexpr i128 kIMax = static_cast<i128>((u128{1} << 127) - 1);
constexpr i128 kIMin = -kIMax - 1;

struct Counters {
  unsigned oracle_literal = 0, overflow_refused = 0, conversions = 0;
  unsigned u192_calls = 0, u192_high = 0, u320_calls = 0, u320_w4 = 0, first_bit_256 = 0;
  unsigned sums = 0, sum_carries = 0, comparisons = 0;
  unsigned gcd_zero = 0, gcd_one = 0, gcd_wide = 0, gcd_cases = 0, fibonacci = 0;
  unsigned abs_cases = 0, keys = 0, rationals = 0, floors = 0;
  unsigned reject_product = 0, reject_sum = 0, reject_den = 0;
  unsigned reject_num = 0, reject_cross = 0, reject_floor = 0, reject_key = 0;
} counts;

struct Failure : std::runtime_error {
  explicit Failure(const std::string& name) : std::runtime_error(name) {}
};
void check(bool ok, const char* name) {
  if (!ok) throw Failure(name);
}
void oracle_clean() {
  check(!mhgp7_oracle::overflow_seen(), "oracle.unexpected_overflow");
}
Big ob_u(u128 n) {
  const u64 w[2] = {static_cast<u64>(n), static_cast<u64>(n >> 64)};
  return Big::from_u64_words(w, 2);
}
#if INTEGER_GATE_BOOST
cpp_int boost_words(const u64* words, int n) {
  cpp_int r = 0;
  for (int i = n - 1; i >= 0; --i) { r <<= 64; r += words[i]; }
  return r;
}
cpp_int boost_u(u128 n) {
  const u64 w[2] = {static_cast<u64>(n), static_cast<u64>(n >> 64)};
  return boost_words(w, 2);
}
cpp_int boost_s(i128 n) {
  // No signed negation of INT128_MIN in the judge.
  const u128 m = n < 0 ? static_cast<u128>(-(n + 1)) + 1 : static_cast<u128>(n);
  const cpp_int r = boost_u(m);
  return n < 0 ? -r : r;
}
cpp_int boost_ob(const Big& n) {
  cpp_int r = 0;
  for (int i = Big::kLimbs - 1; i >= 0; --i) { r <<= 32; r += n.w[i]; }
  return n.neg ? -r : r;
}
void agree(const Big& a, const cpp_int& b, const char* name) {
  oracle_clean();
  check(boost_ob(a) == b, name);
}
#else
// Remove only the explicitly optional second-authority cross-check. OBig,
// literals, domain guards and product comparisons remain active below.
#define agree(a, b, label) static_cast<void>(0)
#endif

// Independently represented Stein GCD and binary long division. No product GCD,
// division, or reduction participates in these reference computations.
bool ob_even(const Big& a) { return (a.w[0] & 1u) == 0; }
void ob_shr1(Big& a) {
  for (int i = 0; i + 1 < Big::kLimbs; ++i) a.w[i] = (a.w[i] >> 1) | (a.w[i + 1] << 31);
  a.w[Big::kLimbs - 1] >>= 1;
}
Big ob_gcd(Big a, Big b) {
  a.neg = b.neg = false;
  if (a.is_zero()) return b;
  if (b.is_zero()) return a;
  unsigned shift = 0;
  while (ob_even(a) && ob_even(b)) { ob_shr1(a); ob_shr1(b); ++shift; }
  while (ob_even(a)) ob_shr1(a);
  for (;;) {
    while (ob_even(b)) ob_shr1(b);
    if (Big::cmp_mag(a, b) > 0) std::swap(a, b);
    b = Big::sub_mag(b, a);
    if (b.is_zero()) break;
  }
  while (shift != 0) { a = Big::add_mag(a, a); --shift; }
  oracle_clean();
  return a;
}
std::pair<Big, Big> ob_divmod(Big a, Big d) {
  check(!d.is_zero(), "fixture.oracle_divisor_zero");
  a.neg = d.neg = false;
  Big q, r;
  const Big one = Big::from_i64(1);
  for (int bit = a.bit_length() - 1; bit >= 0; --bit) {
    r = Big::add_mag(r, r);
    if ((a.w[bit / 32] >> (bit % 32)) & 1u) r = Big::add_mag(r, one);
    if (Big::cmp_mag(r, d) >= 0) {
      r = Big::sub_mag(r, d);
      q.w[bit / 32] |= std::uint32_t{1} << (bit % 32);
    }
  }
  oracle_clean();
  return {q, r};
}
Big ob_div_exact(const Big& a, const Big& d) {
  auto [q, r] = ob_divmod(a, d);
  check(r.is_zero(), "oracle.division_not_exact");
  q.neg = a.neg;
  q.canon();
  return q;
}
#if INTEGER_GATE_BOOST
cpp_int boost_gcd(cpp_int a, cpp_int b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) { const cpp_int r = a % b; a = b; b = r; }
  return a;
}
#endif

void qualify_oracles() {
  const u64 nw[3] = {kRMinus1, kRMinus1, kRMinus1};
  const u64 dw[2] = {kRMinus1, kRMinus1};
  const u64 expected[5] = {1, 0, kRMinus1, kRMinus1 - 1, kRMinus1};
  const Big n = Big::from_u64_words(nw, 3), d = Big::from_u64_words(dw, 2);
  const Big literal = Big::from_u64_words(expected, 5);
  check(literal == Big::pow2(320) - Big::pow2(192) - Big::pow2(128) + Big::from_i64(1),
        "oracle.W2_literal_identity");
#if INTEGER_GATE_BOOST
  const cpp_int exact = (cpp_int(1) << 320) - (cpp_int(1) << 192) - (cpp_int(1) << 128) + 1;
  check(boost_words(expected, 5) == exact && boost_words(nw, 3) * boost_words(dw, 2) == exact,
        "oracle.boost_literal");
#endif
  const Big multiplied = n * d;
  oracle_clean();
  ++counts.oracle_literal;
  check(multiplied == literal, "oracle.W2_carry");
  agree(multiplied, exact, "oracle.W2_boost");

  // Expected width overflow is tested in a separate interval. The poisoned
  // value is never consumed, and the sticky bit must survive another operation.
  mhgp7_oracle::overflow_log() = false;
  const Big top = Big::pow2(383);
  static_cast<void>(top + top);
  check(mhgp7_oracle::overflow_seen(), "oracle.overflow_not_flagged");
  static_cast<void>(Big::from_i64(1) + Big::from_i64(1));
  check(mhgp7_oracle::overflow_seen(), "oracle.overflow_not_sticky");
  ++counts.overflow_refused;
  mhgp7_oracle::clear_overflow();
  mhgp7_oracle::overflow_log() = true;

  for (const i128 value : {kIMin, i128{-1}, i128{0}, kIMax}) {
    const Big ob = Big::from_i128(value);
    mhgp7_oracle::oi128 back = 123;
    check(ob.to_i128(&back) && back == value, "oracle.conversion_roundtrip");
    agree(ob, boost_s(value), "oracle.conversion_boost");
    ++counts.conversions;
  }
  for (const Big bad : {Big::pow2(127), -Big::pow2(128)}) {
    mhgp7_oracle::oi128 unchanged = 123;
    check(!bad.to_i128(&unchanged) && unchanged == 123, "oracle.conversion_refusal");
    ++counts.conversions;
  }
}

bool fixture_product_fits(u128 a, u128 b) {
  const Big product = ob_u(a) * ob_u(b);
  agree(product, boost_u(a) * boost_u(b), "fixture.product_oracles");
  return product < Big::pow2(192);
}
bool fixture_sum_fits(u128 a, u128 b, u128 c) {
  const Big sum = ob_u(a) * ob_u(a) + ob_u(b) * ob_u(b) + ob_u(c) * ob_u(c);
  agree(sum, boost_u(a) * boost_u(a) + boost_u(b) * boost_u(b) + boost_u(c) * boost_u(c),
        "fixture.sum_oracles");
  return sum < Big::pow2(192);
}
bool fixture_rational_pair(const Rational128& x, const Rational128& y) {
  if (x.den <= 0 || y.den <= 0) { ++counts.reject_den; return false; }
  if (x.num < 0 || y.num < 0) { ++counts.reject_num; return false; }
  if (!fixture_product_fits(static_cast<u128>(x.num), static_cast<u128>(y.den)) ||
      !fixture_product_fits(static_cast<u128>(y.num), static_cast<u128>(x.den))) {
    ++counts.reject_cross;
    return false;
  }
  return true;
}
bool fixture_floor_valid(i128 num, i128 den) {
  if (den == 0 || (num == kIMin && den == -1)) { ++counts.reject_floor; return false; }
  return true;
}

void u320_only() {
  // No U192 multiplication or sum-of-squares call in this entire route.
  const U192 n{{kRMinus1, kRMinus1, kRMinus1}};
  const u64 expected[5] = {1, 0, kRMinus1, kRMinus1 - 1, kRMinus1};
  const U320 result = mul_192x128_320(n, kUMax);
  ++counts.u320_calls;
  check(expected[4] != 0, "W2.high_literal_vacuous");
  ++counts.u320_w4;
  for (int i = 0; i < 4; ++i) check(result.w[i] == expected[i], "W2.u320_lower_words");

  const ExactLevel x{{0, 0, u64{1} << 62}, 1};  // 2^190, built literally.
  const ExactLevel y{{1, 0, 0}, static_cast<i128>(u128{1} << 126)};
  const int xy = compare_exact_level(x, y), yx = compare_exact_level(y, x);
  counts.u320_calls += 4;
  counts.comparisons += 2;
  const Big left = Big::pow2(190) * Big::pow2(126);
  agree(left, cpp_int(1) << 316, "W2.level_oracle");
  check(left > Big::from_i64(1), "W2.level_literal");
  // External auditor's first-bit witness, explicitly attributed:
  // audits/ARITHMETIQUE_LARGE_COURANTE.md §4, SHA256
  // dcaa93cd2573d15db8bd36d83dbd1b29a5b7380c3211eae6c8da899b1dc6cdf9.
  // Independent Python literal receipt SHA256
  // a2e00adfd322672ff787e63d062718ca7aac9c8ac3ceef7c098bd88b32a9a90e.
  const ExactLevel first{{0, 0, 4}, 1};
  const int fy = compare_exact_level(first, y), yf = compare_exact_level(y, first);
  const u64 first_literal[5] = {0, 0, 0, 0, 1};
  check(Big::pow2(130) * Big::pow2(126) == Big::from_u64_words(first_literal, 5),
        "W2.first_bit_oracle");
  counts.u320_calls += 4;
  counts.comparisons += 2;
  ++counts.first_bit_256;
  ++counts.u320_w4;
  // For level-trunc-hi require both independent consequences at its U320 site.
  if (result.w[4] != expected[4]) {
    check(result.w[4] == 0 && xy == -1 && yx == 1 && fy == -1 && yf == 1 && counts.u192_calls == 0,
          "W2.unrecognized_divergence");
    throw Failure("W2.u320_word4");
  }
  check(xy == 1 && yx == -1 && fy == 1 && yf == -1, "W2.semantic_comparison");
  agree(Big::from_u64_words(result.w, 5), boost_words(expected, 5), "W2.u320_boost");
}

void wide_cases() {
  struct Case { u128 a, b; std::array<u64, 3> literal; };
  const std::array<Case, 2> cases{{
      {(u128{1} << 96) - 1, (u128{1} << 96) - 1, {1, u64{0} - (u64{1} << 33), kRMinus1}},
      {(u128{1} << 127) - 1, (u128{1} << 64) + 1, {kRMinus1, (u64{1} << 63) - 2, u64{1} << 63}}}};
  for (const auto& item : cases) {
    check(fixture_product_fits(item.a, item.b), "W1.fixture_capacity");
    const Big exact = ob_u(item.a) * ob_u(item.b);
#if INTEGER_GATE_BOOST
    const cpp_int other = boost_u(item.a) * boost_u(item.b);
    check(boost_words(item.literal.data(), 3) == other, "W1.literal_boost");
#endif
    check(exact == Big::from_u64_words(item.literal.data(), 3), "W1.literal_obig");
    for (const auto& [a, b] : {std::pair{item.a, item.b}, std::pair{item.b, item.a}}) {
      const U192 got = mul_128x128_192(a, b);
      ++counts.u192_calls;
      counts.u192_high += got.w[2] != 0;
      check(Big::from_u64_words(got.w, 3) == exact, "W1.u192_product");
    }
  }
  check(!fixture_product_fits(u128{1} << 96, u128{1} << 96), "W1.reject_capacity");
  ++counts.reject_product;  // No product API call for this invalid fixture.
  const u128 a = (u128{1} << 95) - 1;
  check(fixture_sum_fits(a, a, a), "W3.fixture_capacity");
  const U192 sum = sum_of_three_squares_192(a, a, a);
  counts.u192_calls += 3;
  ++counts.sums;
  const Big exact = ob_u(a) * ob_u(a) + ob_u(a) * ob_u(a) + ob_u(a) * ob_u(a);
  check(Big::from_u64_words(sum.w, 3) == exact, "W3.sum");
  agree(exact, 3 * boost_u(a) * boost_u(a), "W3.sum_boost");
  // Two additions of middle words R-2^32 generate carries to the high word.
  check(sum.w[0] == 3 && sum.w[1] == u64{0} - (u64{3} << 32) &&
        sum.w[2] == (u64{3} << 62) - 1, "W3.sum_literal_carries");
  counts.sum_carries += 2;
  const u128 large = u128{3} << 94;
  check(fixture_product_fits(large, large) && !fixture_sum_fits(large, large, 0),
        "W3.collective_rejection");
  ++counts.reject_sum;

  for (int high = 0; high < 5; ++high) {
    U320 lo{{0, 0, 0, 0, 0}}, hi{{0, 0, 0, 0, 0}};
    hi.w[high] = 1;
    check(cmp_u320(lo, hi) == -1 && cmp_u320(hi, lo) == 1 && cmp_u320(hi, hi) == 0,
          "W4.u320_word_order");
    counts.comparisons += 3;
    if (high < 3) {
      U192 l{{0, 0, 0}}, h{{0, 0, 0}};
      h.w[high] = 1;
      check(cmp_u192(l, h) == -1 && cmp_u192(h, l) == 1 && cmp_u192(h, h) == 0,
            "W4.u192_word_order");
      counts.comparisons += 3;
    }
  }
}

void gcd_case(u128 a, u128 b, u128 literal) {
  const Big oracle = ob_gcd(ob_u(a), ob_u(b));
#if INTEGER_GATE_BOOST
  const cpp_int second = boost_gcd(boost_u(a), boost_u(b));
  agree(oracle, second, "gcd.oracles");
  check(second == boost_u(literal), "gcd.literal");
#endif
  check(oracle == ob_u(literal), "gcd.literal_obig");
  check(ugcd128(a, b) == literal, "gcd.product");
  ++counts.gcd_cases;
  counts.gcd_zero += literal == 0;
  counts.gcd_one += literal == 1;
  counts.gcd_wide += literal > kRMinus1;
}
void gcd_and_reducers() {
  const u128 r = u128{1} << 64;
  for (const auto& t : std::array<std::array<u128, 3>, 7>{{
           {{0, 0, 0}}, {{0, kUMax, kUMax}}, {{kUMax, 0, kUMax}},
           {{r + 1, r - 1, 1}}, {{r - 1, r + 1, 1}},
           {{u128{1} << 127, r, r}}, {{(u128{1} << 127) + 1, r, 1}}}})
    gcd_case(t[0], t[1], t[2]);
  u128 a = 1, b = 1;
  while (true) {
    gcd_case(a, b, 1);
    ++counts.fibonacci;
    if (a > kUMax - b) break;
    const u128 next = a + b;
    a = b;
    b = next;
  }
  check(b > (u128{1} << 127), "gcd.fibonacci_did_not_reach_type_top");
  for (const auto& t : std::array<std::pair<i128, u128>, 4>{{
           {kIMin, u128{1} << 127}, {-1, 1}, {0, 0}, {kIMax, static_cast<u128>(kIMax)}}}) {
    check(uabs128(t.first) == t.second, "abs.product");
    agree(Big::from_i128(t.first).abs(), boost_u(t.second), "abs.oracles");
    ++counts.abs_cases;
  }
  const i128 huge = static_cast<i128>(u128{1} << 100);
  const std::array<BallForm, 4> forms{{
      {6, {12, 18, 24}, 5}, {6, {12, -18, 24}, -30}, {huge, {0, 0, 0}, 0},
      {static_cast<i128>(r), {kIMin, 0, static_cast<i128>(r)}, kIMin}}};
  const i128 signed_2_63 = static_cast<i128>(u128{1} << 63);
  const std::array<BallKey, 4> key_literals{{
      {6, {12, 18, 24}, 5}, {1, {2, -3, 4}, -5}, {1, {0, 0, 0}, 0},
      {1, {-signed_2_63, 0, 1}, -signed_2_63}}};
  std::size_t key_index = 0;
  for (const BallForm& form : forms) {
    check(form.a > 0, "fixture.key_domain");
    const std::array<i128, 5> raw{form.a, form.b[0], form.b[1], form.b[2], form.c};
    Big g = Big::from_i128(raw[0]);
#if INTEGER_GATE_BOOST
    cpp_int bg = boost_s(raw[0]);
#endif
    for (std::size_t i = 1; i < raw.size(); ++i) {
      g = ob_gcd(g, Big::from_i128(raw[i]));
#if INTEGER_GATE_BOOST
      bg = boost_gcd(bg, boost_s(raw[i]));
#endif
    }
    agree(g, bg, "key.gcd_oracles");
    const BallKey got = ball_key_reduce(form);
    check(got == key_literals[key_index++], "key.expected_literal");
    const std::array<i128, 5> result{got.a, got.b[0], got.b[1], got.b[2], got.c};
    for (std::size_t i = 0; i < raw.size(); ++i) {
      const Big exact = ob_div_exact(Big::from_i128(raw[i]), g);
      agree(exact, boost_s(raw[i]) / bg, "key.division_oracles");
      check(exact == Big::from_i128(result[i]), "key.reduction");
    }
    ++counts.keys;
  }
  for (const i128 invalid_a : {i128{0}, i128{-1}}) {
    check(!(invalid_a > 0), "fixture.key_rejection");
    ++counts.reject_key;  // ball_key_reduce is NOT called.
  }
  const std::array<Rational128, 4> inputs{{{6, 8}, {-6, 8}, {0, 8}, {kIMin, static_cast<i128>(r)}}};
  const std::array<Rational128, 4> reduced_literals{{{3, 4}, {-3, 4}, {0, 1}, {-signed_2_63, 1}}};
  std::size_t rational_index = 0;
  for (const Rational128 raw : inputs) {
    check(raw.den > 0, "fixture.reducer_domain");
    const Big g = ob_gcd(Big::from_i128(raw.num), Big::from_i128(raw.den));
#if INTEGER_GATE_BOOST
    const cpp_int bg = boost_gcd(boost_s(raw.num), boost_s(raw.den));
#endif
    agree(g, bg, "rational.gcd_oracles");
    const Rational128 got = rational_reduce(raw);
    check(got.num == reduced_literals[rational_index].num && got.den == reduced_literals[rational_index].den,
          "rational.expected_literal");
    ++rational_index;
    for (const auto& p : {std::pair{raw.num, got.num}, std::pair{raw.den, got.den}}) {
      const Big exact = ob_div_exact(Big::from_i128(p.first), g);
      agree(exact, boost_s(p.first) / bg, "rational.division_oracles");
      check(exact == Big::from_i128(p.second), "rational.reduction");
    }
    ++counts.rationals;
  }
}

void levels_and_domains() {
  const std::array<std::pair<Rational128, Rational128>, 3> pairs{{
      {{1, 2}, {2, 4}}, {{0, 1}, {0, 7}}, {{static_cast<i128>((u128{1} << 100) + 1), 3},
                                          {static_cast<i128>(u128{1} << 100), 3}}}};
  for (const auto& [x, y] : pairs) {
    check(fixture_rational_pair(x, y), "fixture.valid_rational_pair");
    const Big left = Big::from_i128(x.num) * Big::from_i128(y.den);
    const Big right = Big::from_i128(y.num) * Big::from_i128(x.den);
    agree(left, boost_s(x.num) * boost_s(y.den), "levels.left_oracle");
    agree(right, boost_s(y.num) * boost_s(x.den), "levels.right_oracle");
    const int expected = cmp(left, right);
    check(compare_rational(x, y) == expected && compare_rational(y, x) == -expected,
          "levels.rational_order");
    counts.u192_calls += 4;
    const ExactLevel ex = promote_level(x), ey = promote_level(y);
    check(compare_exact_level(ex, ey) == expected && compare_exact_level(ey, ex) == -expected &&
          same_exact_level(ex, ey) == (expected == 0), "levels.exact_order");
    counts.u320_calls += 6;
    counts.comparisons += 5;
    check((ex == ey) == (x.num == y.num && x.den == y.den), "levels.representation");
  }
  const ExactLevel high{{0, 0, 2}, 7}, low{{0, 0, 1}, 7};
  check(compare_exact_level(high, low) == 1 && compare_exact_level(low, high) == -1,
        "levels.high_word_order");
  counts.u320_calls += 4;
  counts.comparisons += 2;
  for (const Rational128 bad : {Rational128{1, 0}, Rational128{1, -1}, Rational128{-1, 1}})
    check(!fixture_rational_pair(bad, {1, 1}), "fixture.rational_rejection");
  check(!fixture_rational_pair({static_cast<i128>(u128{1} << 100), 1},
                              {0, static_cast<i128>(u128{1} << 100)}), "fixture.cross_rejection");
  for (const i128 bad_den : {i128{0}, i128{-1}}) {
    check(!(bad_den > 0), "fixture.exact_den_rejection");
    ++counts.reject_den;  // No ExactLevel comparison or reduction is called.
  }
  check(!fixture_floor_valid(1, 0) && !fixture_floor_valid(kIMin, -1), "fixture.floor_rejection");
  const std::array<std::array<i128, 3>, 6> floors{{
      {{-5, 2, -3}}, {{5, -2, -3}}, {{-4, 2, -2}}, {{kIMin, 1, kIMin}},
      {{kIMax, -1, -kIMax}}, {{0, -2, 0}}}};
  for (const auto& f : floors) {
    check(fixture_floor_valid(f[0], f[1]), "fixture.floor_valid");
    auto [q, rem] = ob_divmod(Big::from_i128(f[0]), Big::from_i128(f[1]));
    if ((f[0] < 0) != (f[1] < 0)) {
      q = -q;
      if (!rem.is_zero()) q -= Big::from_i64(1);
    }
    q.canon();
    check(q == Big::from_i128(f[2]), "floor.literal_obig");
    agree(q, boost_s(f[2]), "floor.literal_oracle");
    check(floor_div128(f[0], f[1]) == f[2], "floor.product");
    ++counts.floors;
  }
}

void print_counts(const char* status, const std::string& which, const std::string& mutant,
                  const char* divergence) {
  std::printf("arithmetic_integer_gate=%s case=%s mutant=%s divergence=%s boost=%s authority=%s public_status=not_claimed\n",
              status, which.c_str(), mutant.empty() ? "none" : mutant.c_str(), divergence, BOOST_LIB_VERSION,
              INTEGER_GATE_BOOST ? "obig_literals_and_boost" : "obig_and_literals");
  std::printf("integer_counts oracle_literal=%u overflow_refused=%u conversions=%u u192_calls=%u u192_high=%u "
              "u320_calls=%u u320_w4=%u first_bit_256=%u sums=%u sum_carries=%u comparisons=%u gcd_zero=%u gcd_one=%u "
              "gcd_wide=%u gcd_cases=%u fibonacci=%u abs=%u keys=%u rationals=%u floors=%u "
              "fixture_reject_product=%u fixture_reject_sum=%u fixture_reject_den=%u fixture_reject_num=%u "
              "fixture_reject_cross=%u fixture_reject_floor=%u fixture_reject_key=%u\n",
              counts.oracle_literal, counts.overflow_refused, counts.conversions, counts.u192_calls, counts.u192_high,
              counts.u320_calls, counts.u320_w4, counts.first_bit_256, counts.sums, counts.sum_carries, counts.comparisons,
              counts.gcd_zero, counts.gcd_one, counts.gcd_wide, counts.gcd_cases, counts.fibonacci, counts.abs_cases,
              counts.keys, counts.rationals, counts.floors, counts.reject_product, counts.reject_sum, counts.reject_den,
              counts.reject_num, counts.reject_cross, counts.reject_floor, counts.reject_key);
}
}  // namespace

int main(int argc, char** argv) {
  std::string which = "all", mutant;
  bool seen_case = false, seen_mutant = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.starts_with("--case=") && !seen_case) { which = arg.substr(7); seen_case = true; }
    else if (arg.starts_with("--mutant=") && !seen_mutant) { mutant = arg.substr(9); seen_mutant = true; }
    else { std::fprintf(stderr, "usage: --case=all|u320-only|oracle-only [--mutant=NAME]\n"); return 2; }
  }
  if ((which != "all" && which != "u320-only" && which != "oracle-only") ||
      (seen_mutant && mutant != "level-trunc-hi" && mutant != "obig-carry-lost") ||
      (mutant == "level-trunc-hi" && which != "u320-only") ||
      (mutant == "obig-carry-lost" && which != "oracle-only") ||
      (!mutant.empty() && !mhgp7::mutants_enable(mutant))) return 2;
  try {
    mhgp7_oracle::clear_overflow();
    qualify_oracles();
    if (which != "oracle-only") u320_only();
    if (which == "all") {
      wide_cases();
      gcd_and_reducers();
      levels_and_domains();
      check(counts.u192_high == 4 && counts.sum_carries == 2 && counts.gcd_zero > 0 &&
            counts.gcd_one > 0 && counts.gcd_wide >= 3 && counts.fibonacci > 180 && counts.keys == 4 &&
            counts.rationals == 4 && counts.floors == 6 && counts.reject_product == 1 &&
            counts.reject_sum == 1 && counts.reject_den == 4 && counts.reject_num == 1 &&
            counts.reject_cross == 1 && counts.reject_floor == 2 && counts.reject_key == 2,
            "gate.nonvacuity");
    }
    oracle_clean();
    check(mutant.empty(), "gate.mutant_survived");
    print_counts("passed", which, mutant, "none");
    return 0;
  } catch (const Failure& error) {
    const bool recognized = (mutant == "obig-carry-lost" && std::string(error.what()) == "oracle.W2_carry" &&
                             counts.oracle_literal == 1 && counts.u192_calls == 0 && counts.u320_calls == 0) ||
                            (mutant == "level-trunc-hi" && std::string(error.what()) == "W2.u320_word4" &&
                             counts.oracle_literal == 1 && counts.u192_calls == 0 && counts.u320_w4 == 2 &&
                             counts.first_bit_256 == 1 && counts.u320_calls == 9 && counts.comparisons == 4);
    print_counts(recognized ? "mutant_killed" : "failed", which, mutant, error.what());
    return recognized ? 4 : 1;
  } catch (const std::exception& error) {
    print_counts("failed", which, mutant, error.what());
    return 1;
  }
}
