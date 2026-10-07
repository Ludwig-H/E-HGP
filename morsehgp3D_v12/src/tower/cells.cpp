// Cellules de fenetre des coquilles etendues (port de cells.cpp et cells_classify.cpp de la v11 gelee, ac081a06f,
// reecrit) : traces strictes = t-parties SEPARABLES de U (OBJ-T2 : beta(A) < lambda ssi le centre est hors de
// l'enveloppe fermee de A), enumerees dans l'ordre lexicographique des A comme la v11 ; la v11 decidait chaque partie
// par sa plus petite boule (bounded_meb), la v12 par les supports de la sphere contenus dans U (une partie est non
// separable ssi elle en contient un : Caratheodory et minimalite des supports). Morceaux : A ~ A' si A u A' est
// separable ; il suffit d'unir les t-parties de chaque (t+1)-partie separable B = A u {j} (deux t-parties voisines
// different d'un site). Un seul morceau : cellule inerte ; au moins deux : jonction ; aucune trace : naissance.
// Plafonds declares : C(m, t) <= kMaxCellCombinations parties par cellule, sinon cell_capacity.
#include <algorithm>
#include <bit>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

bool separable(u64 mask, std::span<const u64> witnesses) noexcept {
  for (const u64 w : witnesses)
    if ((w & ~mask) == 0) return false;
  return true;
}

// C(m, t) sature au-dela du plafond (m <= 64, t <= m).
u64 binomial_capped(u32 m, u32 t) noexcept {
  if (t > m) return 0;
  t = std::min(t, m - t);
  u64 result = 1;
  for (u32 i = 1; i <= t; ++i) {
    result = result * (m - t + i) / i;  // exact : produit de i entiers consecutifs divisible par i!
    if (result > kMaxCellCombinations) return kMaxCellCombinations + 1;
  }
  return result;
}

// Partie suivante dans l'ordre lexicographique des t-uples croissants de [0, m).
bool next_tuple(std::array<u32, kMaxShell>& tuple, u32 m, u32 t) noexcept {
  for (u32 j = t; j != 0; --j) {
    const u32 i = j - 1;
    if (tuple[i] == m - t + i) continue;
    ++tuple[i];
    for (u32 l = i + 1; l < t; ++l) tuple[l] = tuple[l - 1] + 1;
    return true;
  }
  return false;
}

u32 root(std::span<u32> parent, u32 x) noexcept {
  while (parent[x] != x) {
    parent[x] = parent[parent[x]];
    x = parent[x];
  }
  return x;
}

// Nombre de morceaux des traces strictes masks[0 .. s) (toutes de cardinal t).
Result<u64> count_pieces(u32 m, u64 s, std::span<const u64> witnesses, CellScratch& scratch) noexcept {
  std::copy(scratch.masks.begin(), scratch.masks.begin() + static_cast<std::ptrdiff_t>(s), scratch.sorted.begin());
  const auto sorted = scratch.sorted.first(s);
  std::sort(sorted.begin(), sorted.end());
  const auto parent = scratch.parent.first(s);
  for (u64 i = 0; i < s; ++i) parent[i] = static_cast<u32>(i);
  u64 pieces = s;
  for (u64 i = 0; i < s; ++i) {
    const u64 a = sorted[i];
    for (u32 j = 0; j < m; ++j) {
      const u64 bit = u64{1} << j;
      if ((a & bit) != 0 || !separable(a | bit, witnesses)) continue;
      for (u64 rest = a; rest != 0; rest &= rest - 1) {  // A' = (A u {j}) prive d'un site de A
        const u64 other = (a | bit) & ~(rest & (~rest + 1));
        const auto at = std::lower_bound(sorted.begin(), sorted.end(), other);
        if (at == sorted.end() || *at != other) return fail(Reason::tower_invariant);  // A' separable, donc trace
        const u32 x = root(parent, static_cast<u32>(i)), y = root(parent, static_cast<u32>(at - sorted.begin()));
        if (x == y) continue;
        parent[std::max(x, y)] = std::min(x, y);
        --pieces;
      }
    }
  }
  return pieces;
}

}  // namespace

Window ball_window(const CatalogueBall& ball, Order orders) noexcept {
  const u32 lo = ball.p + ball.qmin - 1;
  const u32 hi = std::min<u32>(ball.p + ball.m, orders);
  return {lo, hi};
}

Result<CellShape> classify_extended(u32 m, u32 t, std::span<const u64> witnesses, CellScratch& scratch) noexcept {
  if (t == 0 || t >= m || m > kMaxShell) return fail(Reason::tower_invariant);
  if (binomial_capped(m, t) > kMaxCellCombinations) return fail(Reason::cell_capacity);
  std::array<u32, kMaxShell> tuple{};
  for (u32 i = 0; i < t; ++i) tuple[i] = i;
  u64 s = 0;
  do {
    u64 mask = 0;
    for (u32 i = 0; i < t; ++i) mask |= u64{1} << tuple[i];
    if (separable(mask, witnesses)) scratch.masks[s++] = mask;
  } while (next_tuple(tuple, m, t));
  CellShape shape;
  shape.flags = kCellExtended;
  if (s == 0) {
    shape.birth = true;
    return shape;
  }
  auto pieces = count_pieces(m, s, witnesses, scratch);
  if (!pieces.ok()) return pieces.outcome();
  if (pieces.value() == 1) shape.flags |= kCellInert;
  shape.reps = s;
  return shape;
}

}  // namespace mhgp12::tower_detail
