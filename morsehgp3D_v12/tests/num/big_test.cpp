// Portes unitaires de la tranche S8 : capacite et refus de num::Big, operandes confondus, division par defaut,
// formes reduites de Rational, signature d'un carre parfait (defaut b4632db51), budgets de RadicalSum et de RootTable.
#include <array>
#include <vector>

#include "num/num.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

namespace {

Big power_of_two(u32 bits) {
  Big out;
  if (!shift_left(Big::from_u64(1), bits, out).ok()) out = Big::from_u64(0);
  return out;
}

Big all_ones(u32 words) {
  std::vector<u64> w(words, ~u64{0});
  Big out;
  if (!out.assign_words(w, false).ok()) out = Big::from_u64(0);
  return out;
}

Reason reason(const Outcome& o) { return o.reason; }

}  // namespace

MHGP12_TEST(capacity, 12) {
  const Big max = all_ones(kBigWords);
  CHECK_EQ(max.bit_length(), kBigCapacityBits);
  Big out;
  std::vector<u64> wide(kBigWords + 1, 1);
  CHECK(reason(out.assign_words(wide, false)) == Reason::radical_sign_budget);
  wide.back() = 0;  // zeros de tete admis
  CHECK(out.assign_words(wide, true).ok());
  CHECK(reason(add(max, Big::from_u64(1), out)) == Reason::radical_sign_budget);
  CHECK(reason(subtract(max, Big::from_i64(-1), out)) == Reason::radical_sign_budget);
  CHECK(subtract(max, Big::from_u64(1), out).ok());
  CHECK(reason(multiply(power_of_two(8704), power_of_two(8704), out)) == Reason::radical_sign_budget);
  CHECK(multiply(power_of_two(8703), power_of_two(8704), out).ok());
  CHECK_EQ(out.bit_length(), kBigCapacityBits);
  CHECK(shift_left(Big::from_u64(1), kBigCapacityBits - 1, out).ok());
  CHECK(reason(shift_left(Big::from_u64(1), kBigCapacityBits, out)) == Reason::radical_sign_budget);
  CHECK(reason(multiply(max, Big::from_u64(2), out)) == Reason::radical_sign_budget);
}

MHGP12_TEST(aliasing, 12) {
  const Big a = Big::from_i128(-(static_cast<i128>(1) << 100) + 12345);
  const Big b = Big::from_u128((static_cast<u128>(7) << 90) + 99);
  Big expected, x;
  REQUIRE(add(a, b, expected).ok());
  x.assign(a);
  REQUIRE(add(x, b, x).ok());
  CHECK_EQ(compare(x, expected), 0);
  REQUIRE(multiply(a, b, expected).ok());
  x.assign(b);
  REQUIRE(multiply(a, x, x).ok());
  CHECK_EQ(compare(x, expected), 0);
  REQUIRE(shift_left(b, 70, expected).ok());
  x.assign(b);
  REQUIRE(shift_left(x, 70, x).ok());
  CHECK_EQ(compare(x, expected), 0);
  Big q, r;
  x.assign(expected);
  REQUIRE(divide(x, b, x, r).ok());  // quotient sur le dividende
  CHECK_EQ(compare(x, power_of_two(70)), 0);
  CHECK(r.is_zero());
}

MHGP12_TEST(division, 12) {
  Big q, r;
  CHECK(reason(divide(Big::from_u64(5), Big{}, q, r)) == Reason::arithmetic_invariant);
  CHECK(reason(divide(Big::from_u64(5), Big::from_u64(2), q, q)) == Reason::arithmetic_invariant);
  REQUIRE(divide(Big::from_i64(-7), Big::from_i64(2), q, r).ok());  // divmod(-7, 2) = (-4, 1)
  CHECK_EQ(compare(q, Big::from_i64(-4)), 0);
  CHECK_EQ(compare(r, Big::from_i64(1)), 0);
  REQUIRE(divide(Big::from_i64(7), Big::from_i64(-2), q, r).ok());  // (-4, -1)
  CHECK_EQ(compare(q, Big::from_i64(-4)), 0);
  CHECK_EQ(compare(r, Big::from_i64(-1)), 0);
  CHECK_EQ(residue(Big::from_i64(-7), 5), 3u);
  CHECK(reason(isqrt(Big::from_i64(-1), q)) == Reason::arithmetic_invariant);
  REQUIRE(isqrt(all_ones(kBigWords), q).ok());
  CHECK_EQ(q.bit_length(), kBigCapacityBits / 2);
}

MHGP12_TEST(rational_form, 9) {
  Rational x;
  REQUIRE(Rational::make(Big::from_i64(6), Big::from_i64(-4), x).ok());
  CHECK_EQ(compare(x.numerator(), Big::from_i64(-3)), 0);
  CHECK_EQ(compare(x.denominator(), Big::from_i64(2)), 0);
  REQUIRE(Rational::make(Big{}, Big::from_i64(-9), x).ok());
  CHECK(x.numerator().is_zero() && x.denominator().is_one());
  CHECK(reason(Rational::make(Big::from_u64(1), Big{}, x)) == Reason::arithmetic_invariant);
  CHECK(reason(divide(Rational::from_i64(1), Rational{}, x)) == Reason::arithmetic_invariant);
  int order = 2;
  REQUIRE(compare(Rational::from_i64(-1), Rational{}, order).ok());
  CHECK_EQ(order, -1);
}

MHGP12_TEST(square_signature, 5) {
  // Un carre parfait a la signature de 1 et rejoint les termes rationnels (defaut b4632db51) ; la classe de 2
  // contient 72, 128 et 288 (temoin F6).
  CHECK(class_signature(Big::from_u64(36)) == class_signature(Big::from_u64(1)));
  CHECK(class_signature(Big::from_u64(144)) == class_signature(Big::from_u64(64)));
  CHECK(class_signature(Big::from_u64(72)) == class_signature(Big::from_u64(2)));
  CHECK(class_signature(Big::from_u64(288)) == class_signature(Big::from_u64(128)));
  CHECK(class_signature(Big::from_u64(3)) != class_signature(Big::from_u64(1)));
}

MHGP12_TEST(radical_budget, 28) {
  {
    MemoryBudget small(RadicalSum::bytes() - 1);
    CHECK(reason(RadicalSum::make(small).outcome()) == Reason::memory_budget);
    CHECK(small.released().ok());
  }
  MemoryBudget budget(RadicalSum::bytes());
  {
    auto made = RadicalSum::make(budget);
    REQUIRE(made.ok());
    RadicalSum sum = std::move(made).take();
    const Rational one = Rational::from_i64(1);
    for (u32 i = 0; i < RadicalSum::kMaxTerms; ++i) REQUIRE(sum.add(one, Rational::from_i64(2 + i)).ok());
    CHECK(reason(sum.add(one, one)) == Reason::radical_sign_budget);
    CHECK(reason(sum.add(one, Rational::from_i64(-2))) == Reason::arithmetic_invariant);
    sum.clear();
    REQUIRE(sum.add(one, Rational::from_i64(8)).ok());
    REQUIRE(sum.add(Rational::from_i64(-2), Rational::from_i64(2)).ok());
    int sign = 7;
    REQUIRE(sum.sign(sign).ok());
    CHECK_EQ(sign, 0);  // sqrt 8 = 2 sqrt 2 : egalite certifiee, aucune classe restante
    CHECK_EQ(sum.trace().classes, 0u);
    CHECK_EQ(budget.used(), RadicalSum::bytes());
  }
  CHECK(budget.released().ok());
}

MHGP12_TEST(roots_limits, 14) {
  // Capacite des rangs u32 : UINT32_MAX niveaux admis, un de plus refuse (RootTable::allocate le refuse alors).
  CHECK(rank_count_fits(0xFFFFFFFFull));
  CHECK(!rank_count_fits(0x100000000ull));
  CHECK(!rank_count_fits(~u64{0}));
  // Niveaux : 2 (sqrt 2), 1/4 (1/2), et 2^(2B+2) (rayon 2^(B+1), hors du domaine geometrique).
  const auto two = Level::make(Wide<1>::from_u64(2), Wide<1>::from_u64(1));
  const auto quarter = Level::make(Wide<1>::from_u64(1), Wide<1>::from_u64(4));
  const auto outside = Level::make(Wide<2>::from_u128(static_cast<u128>(1) << (2 * kCoordBits + 2)),
                                   Wide<1>::from_u64(1));
  REQUIRE(two.ok() && quarter.ok() && outside.ok());
  std::vector<Level> levels{two.value(), quarter.value()};
  MemoryBudget budget(1 << 20);
  {
    auto table = RootTable::build(levels, budget);
    REQUIRE(table.ok());
    CHECK(*table.value().root(1) == static_cast<u128>(1) << 63);  // 2^64 / 2
    CHECK(!table.value().root(2).has_value());
    std::array<SignedRank, 17> many{};
    i128 lo = 0, hi = 0;
    CHECK(reason(table.value().bracket(many, lo, hi)) == Reason::radical_sign_budget);
    const std::array<SignedRank, 1> absent{SignedRank{5, 1}};
    CHECK(reason(table.value().bracket(absent, lo, hi)) == Reason::arithmetic_invariant);
    const std::array<SignedRank, 1> zero_sign{SignedRank{0, 0}};
    CHECK(reason(table.value().bracket(zero_sign, lo, hi)) == Reason::arithmetic_invariant);
    u128 root = 0;
    CHECK(reason(RootTable::root_of(outside.value(), root)) == Reason::arithmetic_invariant);
    levels.push_back(outside.value());
  }
  CHECK(reason(RootTable::build(levels, budget).outcome()) == Reason::arithmetic_invariant);
  MemoryBudget tiny(RootTable::bytes(levels.size()) - 1);
  CHECK(reason(RootTable::build(levels, tiny).outcome()) == Reason::memory_budget);
  CHECK(budget.released().ok());
}

MHGP12_TEST_MAIN()
