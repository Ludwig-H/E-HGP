// Comparaisons exactes de sommes de racines de niveaux par rangs (tranche S9) : encadrement entier par la table des
// racines (num::RootTable, specification paragraphe 7.8), puis repli exact de num. Deux racines contre deux :
// num::sqrt_cmp2 (port de sqrt_cmp2, bench/points_radius.py:48-50), qui decide toujours. Trois contre trois :
// RootTable::sign, repli RadicalSum (port de sign_of_radicals et de RValue.cmp, bench/points_radius.py:93-150).
// Proposition binary64 d'un niveau : lecture seulement (recherche du rang plancher, certifiee en entier).
#include <cmath>

#include "points/internal.hpp"

namespace mhgp11::points_detail {

namespace {

template <int Words>
double magnitude(const num::Wide<Words>& value) noexcept {
  double out = 0;
  for (int i = Words; i-- > 0;) out = out * 18446744073709551616.0 + static_cast<double>(value.words[i]);
  return out;
}

}  // namespace

double approximate(const num::Level& level) noexcept {
  const double den = magnitude(num::to_wide(level.denominator()));
  return den > 0 ? magnitude(num::to_wide(level.numerator())) / den : 0.0;
}

Outcome Roots::root(u32 rank, u128& out, RootTally& tally) const noexcept {
  if (const auto value = table_.root(rank)) {
    out = *value;
    return {};
  }
  if (rank >= levels_.size()) return fail(Reason::points_invariant);
  ++tally.on_demand;
  return num::RootTable::root_of(levels_[rank], out);
}

Outcome Roots::two_vs_two(u32 a, u32 b, u32 c, u32 d, int& out, RootTally& tally) const noexcept {
  std::array<u128, 4> r{};
  const std::array<u32, 4> ranks = {a, b, c, d};
  for (u32 i = 0; i < 4; ++i) MHGP11_TRY(root(ranks[i], r[i], tally));
  // R <= 2^64 sqrt(l) < R + 1 : 2^64 (sqrt a + sqrt b - sqrt c - sqrt d) est dans [lo, hi].
  const i128 plus = static_cast<i128>(r[0]) + static_cast<i128>(r[1]);
  const i128 minus = static_cast<i128>(r[2]) + static_cast<i128>(r[3]);
  const i128 lo = plus - (minus + 2), hi = plus + 2 - minus;
  if (lo > 0 || hi < 0) {
    ++tally.table;
    out = lo > 0 ? 1 : -1;
    return {};
  }
  ++tally.exact;
  std::array<num::Rational, 4> x;
  for (u32 i = 0; i < 4; ++i) MHGP11_TRY(num::Rational::from_level(levels_[ranks[i]], x[i]));
  return num::sqrt_cmp2(x[0], x[1], x[2], x[3], out);
}

Outcome Roots::compare_dates(const std::array<u32, 3>& a, const std::array<u32, 3>& b, num::RadicalSum& sum,
                             int& out, RootTally& tally) const noexcept {
  const std::array<num::SignedRank, 6> terms = {num::SignedRank{a[0], 1},  num::SignedRank{a[1], 1},
                                                num::SignedRank{a[2], -1}, num::SignedRank{b[0], -1},
                                                num::SignedRank{b[1], -1}, num::SignedRank{b[2], 1}};
  bool fallback = false;
  MHGP11_TRY(table_.sign(terms, sum, out, &fallback));
  ++(fallback ? tally.exact : tally.table);
  return {};
}

}  // namespace mhgp11::points_detail
