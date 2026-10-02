// Retenues, emprunts, signes, conversions et budgets ; aucune valeur flottante de reference.
#include <limits>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

MHGP11_TEST(wide, 35) {
  using W = Wide<2>;
  const auto one = W::from_u64(1), zero = W{};
  W negative_zero;
  negative_zero.neg = true;
  CHECK_EQ(compare(zero, negative_zero), 0);
  CHECK_EQ(negative_zero.sign(), 0);
  CHECK(!negative_zero.negated().neg);
  CHECK_EQ(one.bit_length(), 1);
  W maximum;
  maximum.words.fill(std::numeric_limits<u64>::max());
  CHECK_EQ(maximum.bit_length(), 128);
  W out = one;
  CHECK(!add(maximum, one, out));
  CHECK_EQ(compare(out, one), 0);
  CHECK(add(maximum, one.negated(), out));
  CHECK_EQ(out.words[0], std::numeric_limits<u64>::max() - 1);
  CHECK_EQ(out.words[1], std::numeric_limits<u64>::max());
  CHECK(add(out, one, out));
  CHECK_EQ(compare(out, maximum), 0);
  CHECK(subtract(one, maximum, out));
  CHECK_EQ(out.sign(), -1);
  CHECK_EQ(out.words[0], std::numeric_limits<u64>::max() - 1);
  CHECK(subtract(maximum, maximum, out));
  CHECK_EQ(out.sign(), 0);
  CHECK(!out.neg);
  const W high = W::from_u128(u128{1} << 64);
  CHECK(subtract(high, one, out));
  CHECK_EQ(out.words[0], std::numeric_limits<u64>::max());
  CHECK_EQ(out.words[1], 0u);
  const auto square = multiply(maximum, maximum);
  CHECK_EQ(square.words[0], 1u);
  CHECK_EQ(square.words[1], 0u);
  CHECK_EQ(square.words[2], std::numeric_limits<u64>::max() - 1);
  CHECK_EQ(square.words[3], std::numeric_limits<u64>::max());
  CHECK_EQ(multiply(one.negated(), maximum).sign(), -1);
  CHECK_EQ(multiply(negative_zero, maximum).sign(), 0);
  Wide<1> short_value = Wide<1>::from_u64(7);
  CHECK(!resize(high, short_value));
  CHECK_EQ(short_value.words[0], 7u);
  CHECK(resize(negative_zero, short_value));
  CHECK(!short_value.neg);
  const i128 minimum = -static_cast<i128>((u128{1} << 127) - 1) - 1;
  const auto wide_minimum = W::from_i128(minimum);
  CHECK_EQ(wide_minimum.sign(), -1);
  CHECK_EQ(wide_minimum.words[1], u64{1} << 63);
  CHECK_EQ(wide_minimum.words[0], 0u);
  CHECK_EQ(compare(maximum.negated(), one.negated()), -1);
  CHECK_EQ(compare(zero, one.negated()), 1);
}

MHGP11_TEST(budgets, 16) {
  const auto edge63 = to_wide(u64{1} << 63);
  CHECK(!narrow<63>(edge63));
  REQUIRE(narrow<64>(edge63).has_value());
  CHECK_EQ(*narrow<64>(edge63), i128{1} << 63);
  CHECK(!narrow<127>(to_wide(u128{1} << 127)));
  REQUIRE(narrow<128>(to_wide(u128{1} << 127)).has_value());
  CHECK_EQ(narrow<128>(to_wide(u128{1} << 127))->bit_length(), 128);
  CHECK_EQ(integer_one<63>(), 1);
  CHECK_EQ(integer_one<127>(), 1);
  CHECK_EQ(integer_one<128>().words[0], 1u);
  CHECK_EQ(Budgets<18>::side, 116);
  CHECK_EQ(Budgets<21>::side, 134);
  CHECK_EQ(Budgets<24>::side, 152);
  CHECK_EQ(Budgets<24>::level_comparison, 356);
  auto failure = mhgp11::num::detail::require_fit<63>(edge63);
  CHECK(!failure.ok());
  CHECK_EQ(failure.outcome().reason, Reason::arithmetic_invariant);
  CHECK_EQ(failure.outcome().status(), Status::invariant_violated);
}

MHGP11_TEST(levels, 18) {
  const Level zero;
  CHECK_EQ(to_wide(zero.numerator()).sign(), 0);
  CHECK_EQ(compare(to_wide(zero.denominator()), to_wide(i64{1})), 0);
  auto a = Level::make(to_wide(i64{2}), to_wide(i64{3}));
  auto b = Level::make(to_wide(i64{4}), to_wide(i64{6}));
  auto c = Level::make(to_wide(i64{3}), to_wide(i64{4}));
  REQUIRE(a.ok());
  REQUIRE(b.ok());
  REQUIRE(c.ok());
  CHECK_EQ(compare(a.value(), b.value()), 0);
  CHECK_EQ(compare(a.value(), c.value()), -1);
  CHECK_EQ(compare(c.value(), a.value()), 1);
  CHECK_EQ(compare(a.value(), zero), 1);
  for (const auto& pair : {std::pair{i64{-1}, i64{2}}, std::pair{i64{1}, i64{0}}, std::pair{i64{1}, i64{-2}}}) {
    auto bad = Level::make(to_wide(pair.first), to_wide(pair.second));
    CHECK(!bad.ok());
    CHECK_EQ(bad.outcome().status(), Status::invalid_input);
    CHECK_EQ(bad.outcome().reason, Reason::parameter_out_of_range);
  }
  Wide<4> huge;
  huge.words[3] = u64{1} << 63;
  CHECK(!Level::make(huge, to_wide(i64{1})).ok());
  CHECK(!Level::make(to_wide(i64{1}), huge).ok());
}

MHGP11_TEST_MAIN()
