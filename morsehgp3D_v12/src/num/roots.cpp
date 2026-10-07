// Construction et lecture de num::RootTable (tranche S8) : division de Knuth et racine certifiee de num::Big par rang,
// encadrement entier d'une somme signee, repli exact par RadicalSum.
#include "num/roots.hpp"

namespace mhgp12::num {

namespace {

constexpr u128 kAbsent = ~u128{0};

}  // namespace

Outcome RootTable::root_of(const Level& level, u128& out) noexcept {
  const Big numerator = Big::from_wide(to_wide(level.numerator()));
  const Big denominator = Big::from_wide(to_wide(level.denominator()));
  if (numerator.negative() || denominator.sign() <= 0) return fail(Reason::arithmetic_invariant);
  Big scaled, quotient, rest, root;
  MHGP12_TRY(shift_left(numerator, 128, scaled));
  MHGP12_TRY(divide(scaled, denominator, quotient, rest));
  MHGP12_TRY(isqrt(quotient, root));
  // Domaine geometrique : sqrt(l) < 2^(B+1).
  if (root.bit_length() > kRootBits) return fail(Reason::arithmetic_invariant);
  const auto value = root.magnitude_u128();
  if (!value) return fail(Reason::arithmetic_invariant);
  out = *value;
  return {};
}

Result<RootTable> RootTable::allocate(std::span<const Level> levels, MemoryBudget& budget) noexcept {
  // Les rangs sont des u32 (root, SignedRank::rank) : au-dela de UINT32_MAX niveaux, refus explicite plutot qu'une
  // troncature silencieuse des rangs.
  if (!rank_count_fits(levels.size())) return fail(Reason::arithmetic_invariant);
  RootTable out;
  MHGP12_TRY(budget.admit(bytes(levels.size())));
  MHGP12_TRY(out.roots_.allocate(levels.size(), budget));
  for (u128& value : out.roots_) value = kAbsent;
  out.levels_ = levels;
  return out;
}

Result<RootTable> RootTable::build(std::span<const Level> levels, MemoryBudget& budget) noexcept {
  auto table = allocate(levels, budget);
  if (!table.ok()) return table.outcome();
  MHGP12_TRY(table.value().fill(0, levels.size()));
  return table;
}

Outcome RootTable::fill(u64 begin, u64 end) noexcept {
  if (begin > end || end > roots_.size()) return fail(Reason::arithmetic_invariant);
  for (u64 r = begin; r < end; ++r) MHGP12_TRY(root_of(levels_[r], roots_[r]));
  return {};
}

Outcome RootTable::fill_ranks(std::span<const u32> ranks) noexcept {
  for (const u32 r : ranks) {
    if (r >= roots_.size()) return fail(Reason::arithmetic_invariant);
    MHGP12_TRY(root_of(levels_[r], roots_[r]));
  }
  return {};
}

std::optional<u128> RootTable::root(u32 rank) const noexcept {
  if (rank >= roots_.size() || roots_[rank] == kAbsent) return std::nullopt;
  return roots_[rank];
}

Outcome RootTable::bracket(std::span<const SignedRank> terms, i128& lo, i128& hi) const noexcept {
  MHGP12_CHECK(terms.size() <= kMaxTerms, radical_sign_budget);
  i128 low = 0, high = 0;
  for (const SignedRank& term : terms) {
    const auto value = root(term.rank);
    if (!value || (term.sign != 1 && term.sign != -1)) return fail(Reason::arithmetic_invariant);
    const i128 r = static_cast<i128>(*value);  // R <= 2^89 : j R + j < 2^94
    if (term.sign > 0) {  // R <= 2^64 sqrt(l) < R + 1
      low += r;
      high += r + 1;
    } else {  // -(R + 1) < -2^64 sqrt(l) <= -R
      low -= r + 1;
      high -= r;
    }
  }
  lo = low;
  hi = high;
  return {};
}

Outcome RootTable::sign(std::span<const SignedRank> terms, RadicalSum& scratch, int& out, bool* fallback) const noexcept {
  if (fallback != nullptr) *fallback = false;
  i128 lo = 0, hi = 0;
  MHGP12_TRY(bracket(terms, lo, hi));
  if (lo > 0) {
    out = 1;
    return {};
  }
  if (hi < 0) {
    out = -1;
    return {};
  }
  // Repli exact : sqrt(N / D) = (1 / D) sqrt(N D), niveau non reduit.
  if (fallback != nullptr) *fallback = true;
  scratch.clear();
  for (const SignedRank& term : terms) {
    const Level& level = levels_[term.rank];
    const Big numerator = Big::from_wide(to_wide(level.numerator()));
    const Big denominator = Big::from_wide(to_wide(level.denominator()));
    Big radicand;
    MHGP12_TRY(multiply(numerator, denominator, radicand));
    Rational coef;
    MHGP12_TRY(Rational::make(Big::from_i64(term.sign), denominator, coef));
    MHGP12_TRY(scratch.add_integer(coef, radicand));
  }
  return scratch.sign(out);
}

}  // namespace mhgp12::num
