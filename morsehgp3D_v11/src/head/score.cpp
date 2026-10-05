// Encadrements entiers de phi par plateau et repli exact de la tete plate (tranche S10).
//
// Encadrement (remplace le filtre flottant de Level.phi, bench/points_flat.py:233-268) : R = floor(2^64 sqrt(l)) par
// rang ; un niveau sqrt(l_t) a E = 2^64 e dans [R_t, R_t + 1], une date sqrt(l_t) + sqrt(l_m) - sqrt(l_q) dans
// [R_t + R_m - R_q - 1, R_t + R_m - R_q + 2] ; alors 2^192 phi = 2^(192 + 64 z) / E^z est dans
// [floor(2^(192+64z) / E_hi^z), ceil(2^(192+64z) / E_lo^z)]. E_lo < 2^40 : aucun encadrement (repli exact force).
//
// Repli exact : port de Level.phi_exact (:270-292), _inverse_date (:192-206), _mask_mul (:209-218) et de RadSum.sign
// (:153-170) par num::RadicalSum (classes de carres, encadrements, refus radical_sign_budget). La date doit etre
// strictement positive (sqrt_cmp2), sinon head_invariant ; un niveau nul demande est un head_invariant.
#include <algorithm>
#include <array>

#include "head/internal.hpp"

namespace mhgp11::head::detail {

namespace {

Outcome to_fixed(const num::Big& value, Fixed& out) noexcept {
  MHGP11_CHECK(value.size() <= 6, arithmetic_invariant);
  out = Fixed{};
  for (u32 i = 0; i < value.size(); ++i) out.words[i] = value.word(i);
  out.neg = value.negative();
  return {};
}

Outcome power(const num::Big& base, u32 z, num::Big& out) noexcept {
  out.assign(base);
  for (u32 i = 1; i < z; ++i) MHGP11_TRY(num::multiply(out, base, out));
  return {};
}

// Racines des rangs references, dans l'ordre des rangs (recherche dichotomique).
struct RootCache {
  Buffer<u32> ranks;
  Buffer<u128> roots;
  u128 at(u32 rank) const noexcept {
    const u32* hit = std::lower_bound(ranks.begin(), ranks.end(), rank);
    return roots[static_cast<u64>(hit - ranks.begin())];
  }
};

Outcome cache_roots(const TreeView& tree, const LevelSource& levels, MemoryBudget& budget, RootCache& out) noexcept {
  const u64 plateaus = tree.plateau_t.size();
  Buffer<u32> all;
  MHGP11_TRY(budget.admit(4 * 3 * plateaus * 2 + 16 * 3 * plateaus));
  MHGP11_TRY(all.allocate(3 * plateaus, budget));
  u64 used = 0;
  for (u64 p = 0; p < plateaus; ++p) {
    all[used++] = tree.plateau_t[p];
    if (tree.plateau_m[p] != tree.plateau_q[p]) {
      all[used++] = tree.plateau_m[p];
      all[used++] = tree.plateau_q[p];
    }
  }
  std::sort(all.begin(), all.begin() + used);
  const u64 distinct = static_cast<u64>(std::unique(all.begin(), all.begin() + used) - all.begin());
  MHGP11_TRY(out.ranks.allocate(distinct, budget));
  MHGP11_TRY(out.roots.allocate(distinct, budget));
  std::copy_n(all.begin(), distinct, out.ranks.begin());
  for (u64 i = 0; i < distinct; ++i) MHGP11_TRY(levels.root(out.ranks[i], out.roots[i]));
  return {};
}

}  // namespace

Outcome bracket_plateaus(const TreeView& tree, const LevelSource& levels, u32 z, MemoryBudget& budget,
                         Brackets& out) noexcept {
  const u64 plateaus = tree.plateau_t.size();
  RootCache cache;
  MHGP11_TRY(cache_roots(tree, levels, budget, cache));
  MHGP11_TRY(budget.admit(2 * sizeof(Fixed) * plateaus + 2 * plateaus));
  MHGP11_TRY(out.lo.allocate(plateaus, budget));
  MHGP11_TRY(out.hi.allocate(plateaus, budget));
  MHGP11_TRY(out.zero.allocate(plateaus, budget));
  MHGP11_TRY(out.open.allocate(plateaus, budget));
  out.unbracketed = 0;
  num::Big numerator;
  MHGP11_TRY(num::shift_left(num::Big::from_u64(1), kFixedShift + 64 * z, numerator));
  const num::Big one = num::Big::from_u64(1);
  for (u64 p = 0; p < plateaus; ++p) {
    out.lo[p] = out.hi[p] = Fixed{};
    out.zero[p] = out.open[p] = 0;
    i128 e_lo = 0, e_hi = 0;
    const u128 rt = cache.at(tree.plateau_t[p]);
    const bool sq = tree.plateau_m[p] == tree.plateau_q[p];
    // Racines au-dela de 2^kMaxRootBits (source abstraite : le catalogue les borne a 2^89) : aucune conversion signee,
    // aucun encadrement, le repli exact decide (audit 100fcc12b).
    if (rt > kMaxRoot || (!sq && (cache.at(tree.plateau_m[p]) > kMaxRoot || cache.at(tree.plateau_q[p]) > kMaxRoot))) {
      out.open[p] = 1;
      ++out.unbracketed;
      continue;
    }
    if (sq) {
      if (rt == 0) {
        num::Rational value;
        MHGP11_TRY(levels.value(tree.plateau_t[p], value));
        if (value.is_zero()) {
          out.zero[p] = 1;
          continue;
        }
      }
      e_lo = static_cast<i128>(rt);
      e_hi = static_cast<i128>(rt) + 1;
    } else {
      const i128 sum = static_cast<i128>(rt) + static_cast<i128>(cache.at(tree.plateau_m[p])) -
                       static_cast<i128>(cache.at(tree.plateau_q[p]));
      e_lo = sum - 1;
      e_hi = sum + 2;
    }
    if (e_lo < (i128{1} << kMinBracketBits)) {
      out.open[p] = 1;
      ++out.unbracketed;
      continue;
    }
    num::Big low_power, high_power, quotient, rest;
    MHGP11_TRY(power(num::Big::from_i128(e_hi), z, high_power));
    MHGP11_TRY(power(num::Big::from_i128(e_lo), z, low_power));
    MHGP11_TRY(num::divide(numerator, high_power, quotient, rest));
    MHGP11_TRY(to_fixed(quotient, out.lo[p]));
    MHGP11_TRY(num::divide(numerator, low_power, quotient, rest));
    if (!rest.is_zero()) MHGP11_TRY(num::add(quotient, one, quotient));
    MHGP11_TRY(to_fixed(quotient, out.hi[p]));
  }
  return {};
}

namespace {

using num::Rational;

// Base {1, sqrt t, sqrt m, sqrt q, sqrt(tm), ...} codee par masques (bit 1 : t, 2 : m, 4 : q).
struct Masks {
  std::array<Rational, 8> coef;
  std::array<bool, 8> present{};
};

Outcome mask_value(u32 mask, const std::array<Rational, 3>& tmq, Rational& out) noexcept {
  out = Rational::from_i64(1);
  for (u32 bit = 0; bit < 3; ++bit)
    if (mask & (1u << bit)) MHGP11_TRY(num::multiply(out, tmq[bit], out));
  return {};
}

// 1/e pour e = sqrt t + sqrt m - sqrt q > 0 (formule du recu eom_exact_audit_20261004).
Outcome inverse_date(const std::array<Rational, 3>& tmq, Masks& out) noexcept {
  const Rational &t = tmq[0], &m = tmq[1], &q = tmq[2];
  int positive = 0;
  MHGP11_TRY(num::sqrt_cmp2(t, m, q, Rational{}, positive));
  MHGP11_CHECK(positive > 0, head_invariant);  // date non positive
  Rational d, delta, product, four_tm, scratch;
  MHGP11_TRY(num::add(t, m, d));
  MHGP11_TRY(num::subtract(d, q, d));
  MHGP11_TRY(num::multiply(d, d, delta));
  MHGP11_TRY(num::multiply(t, m, product));
  MHGP11_TRY(num::multiply(Rational::from_i64(4), product, four_tm));
  MHGP11_TRY(num::subtract(delta, four_tm, delta));
  out.present.fill(false);
  if (!delta.is_zero()) {
    // (t - m - q) / D, (m - t - q) / D, d / D, -2 / D.
    MHGP11_TRY(num::subtract(t, m, scratch));
    MHGP11_TRY(num::subtract(scratch, q, scratch));
    MHGP11_TRY(num::divide(scratch, delta, out.coef[1]));
    MHGP11_TRY(num::subtract(m, t, scratch));
    MHGP11_TRY(num::subtract(scratch, q, scratch));
    MHGP11_TRY(num::divide(scratch, delta, out.coef[2]));
    MHGP11_TRY(num::divide(d, delta, out.coef[4]));
    MHGP11_TRY(num::divide(Rational::from_i64(-2), delta, out.coef[7]));
    out.present[1] = out.present[2] = out.present[4] = out.present[7] = true;
    return {};
  }
  // Delta = 0 et e > 0 : q = (sqrt t - sqrt m)^2, e = 2 sqrt(min(t, m)).
  int order = 0;
  MHGP11_TRY(num::compare(t, m, order));
  const u32 mask = order <= 0 ? 1u : 2u;
  const Rational& low = order <= 0 ? t : m;
  MHGP11_CHECK(!low.is_zero(), head_invariant);
  MHGP11_TRY(num::multiply(Rational::from_i64(2), low, scratch));
  MHGP11_TRY(num::divide(Rational::from_i64(1), scratch, out.coef[mask]));
  out.present[mask] = true;
  return {};
}

Outcome mask_mul(const Masks& x, const Masks& y, const std::array<Rational, 3>& tmq, Masks& out) noexcept {
  out.present.fill(false);
  Rational value, term;
  for (u32 a = 0; a < 8; ++a) {
    if (!x.present[a]) continue;
    for (u32 b = 0; b < 8; ++b) {
      if (!y.present[b]) continue;
      MHGP11_TRY(num::multiply(x.coef[a], y.coef[b], term));
      MHGP11_TRY(mask_value(a & b, tmq, value));
      MHGP11_TRY(num::multiply(term, value, term));
      const u32 key = a ^ b;
      if (!out.present[key]) {
        out.coef[key].assign(term);
        out.present[key] = true;
      } else {
        MHGP11_TRY(num::add(out.coef[key], term, out.coef[key]));
      }
    }
  }
  return {};
}

// w phi_exact(p) ajoute a sum.
Outcome add_plateau(const TreeView& tree, const LevelSource& levels, u32 z, u32 p, i64 w,
                    num::RadicalSum& sum) noexcept {
  const Rational weight = Rational::from_i64(w);
  Rational t, coef, scratch;
  MHGP11_TRY(levels.value(tree.plateau_t[p], t));
  if (tree.plateau_m[p] == tree.plateau_q[p]) {
    MHGP11_CHECK(!t.is_zero(), head_invariant);  // phi(0)
    // z impair : t^(-(z+1)/2) sqrt(t) ; z pair : t^(-z/2).
    const u32 half = z % 2 ? (z + 1) / 2 : z / 2;
    Rational denominator = Rational::from_i64(1);
    for (u32 i = 0; i < half; ++i) MHGP11_TRY(num::multiply(denominator, t, denominator));
    MHGP11_TRY(num::divide(weight, denominator, coef));
    return sum.add(coef, z % 2 ? t : Rational::from_i64(1));
  }
  std::array<Rational, 3> tmq;
  tmq[0].assign(t);
  MHGP11_TRY(levels.value(tree.plateau_m[p], tmq[1]));
  MHGP11_TRY(levels.value(tree.plateau_q[p], tmq[2]));
  Masks inverse, acc, next;
  MHGP11_TRY(inverse_date(tmq, inverse));
  acc = inverse;
  for (u32 i = 1; i < z; ++i) {
    MHGP11_TRY(mask_mul(acc, inverse, tmq, next));
    acc = next;
  }
  for (u32 mask = 0; mask < 8; ++mask) {
    if (!acc.present[mask] || acc.coef[mask].is_zero()) continue;
    MHGP11_TRY(num::multiply(weight, acc.coef[mask], coef));
    MHGP11_TRY(mask_value(mask, tmq, scratch));
    MHGP11_TRY(sum.add(coef, scratch));
  }
  return {};
}

}  // namespace

Outcome exact_sign(const TreeView& tree, const LevelSource& levels, u32 z, std::span<const Weight> weights,
                   MemoryBudget& budget, int& out) noexcept {
  out = 0;
  if (weights.empty()) return {};
  const u64 wanted = 4 * u64{weights.size()};
  const u32 capacity = static_cast<u32>(std::min<u64>(wanted, kExactTerms));
  MHGP11_TRY(budget.admit(num::RadicalSum::bytes(capacity)));
  auto made = num::RadicalSum::make(budget, capacity);
  if (!made.ok()) return made.outcome();
  num::RadicalSum sum = std::move(made).take();
  for (const Weight& w : weights) MHGP11_TRY(add_plateau(tree, levels, z, w.plateau, w.weight, sum));
  return sum.sign(out);
}

}  // namespace mhgp11::head::detail
