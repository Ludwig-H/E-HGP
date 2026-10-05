// Table d'encadrements des racines de niveaux (tranche S8, specification paragraphe 7.8). Pour chaque rang r reference,
// R_r = floor(2^64 sqrt(l_r)) = isqrt(floor(N 2^128 / D)) pour le niveau non reduit l_r = N / D. Le centre d'une
// boule est dans l'enveloppe de son support, donc sqrt(l_r) < 2^(B+1) et R_r < 2^(B+65) <= 2^89 : un u128 suffit
// (au-dela : arithmetic_invariant, un niveau du catalogue hors du domaine geometrique). La racine entiere recoit une
// proposition binary64 et la certifie en entier (num::isqrt) : le flottant propose, l'entier decide.
//
// Une somme de j racines signees est encadree a j 2^-64 pres : 2^64 S est dans [lo, hi]. La decision est prise si 0
// est hors de [lo, hi] ; sinon (egalites, quasi-egalites), repli exact par RadicalSum sur les niveaux N / D, soit
// (1 / D) sqrt(N D), avec les decisions de sign_of_radicals. Aucune decision flottante (F1).
//
// Positions fixes : fill(begin, end) ecrit les seuls rangs de [begin, end), des appels disjoints peuvent courir en
// parallele et la table ne depend pas de leur ordre. La table emprunte les niveaux : ils doivent lui survivre.
#pragma once

#include <optional>
#include <span>

#include "core/core.hpp"
#include "num/level.hpp"
#include "num/radical.hpp"

namespace mhgp11::num {

struct SignedRank {
  u32 rank = 0;
  i32 sign = 1;  // +1 ou -1
};

// Nombre de niveaux qu'une table de racines indexe par des rangs u32 : au plus UINT32_MAX (rangs 0..2^32-2).
constexpr bool rank_count_fits(u64 levels) noexcept { return levels <= u64{0xFFFFFFFFu}; }

class RootTable {
 public:
  static constexpr u32 kRootBits = static_cast<u32>(kCoordBits) + 65;
  static constexpr u32 kMaxTerms = RadicalSum::kMaxTerms;
  static_assert(kRootBits <= 89, "num : racines de la table en u128, budget de la specification");

  RootTable() = default;
  // Octets reserves par allocate pour `ranks` rangs : formule d'admission.
  static constexpr u64 bytes(u64 ranks) noexcept { return ranks * sizeof(u128); }
  // Reserve un u128 par rang dans le budget ; tous les rangs sont absents.
  [[nodiscard]] static Result<RootTable> allocate(std::span<const Level> levels, MemoryBudget& budget) noexcept;
  // allocate puis fill de tous les rangs.
  [[nodiscard]] static Result<RootTable> build(std::span<const Level> levels, MemoryBudget& budget) noexcept;
  // R_r d'un niveau, sans table.
  [[nodiscard]] static Outcome root_of(const Level& level, u128& out) noexcept;

  [[nodiscard]] Outcome fill(u64 begin, u64 end) noexcept;
  [[nodiscard]] Outcome fill_ranks(std::span<const u32> ranks) noexcept;
  u64 size() const noexcept { return roots_.size(); }
  // R_r, ou rien si le rang est absent ou hors de la table.
  std::optional<u128> root(u32 rank) const noexcept;

  // 2^64 sum_i sign_i sqrt(l_{rank_i}) est dans [lo, hi] (bornes atteintes seulement par une racine exacte).
  // Rang absent : arithmetic_invariant ; plus de kMaxTerms termes ou signe hors de {-1, +1} : radical_sign_budget,
  // respectivement arithmetic_invariant.
  [[nodiscard]] Outcome bracket(std::span<const SignedRank> terms, i128& lo, i128& hi) const noexcept;
  // Signe exact de la somme : encadrement, puis repli exact dans `scratch` si 0 est dans [lo, hi]. `fallback`
  // (facultatif) recoit vrai si le repli a ete joue.
  [[nodiscard]] Outcome sign(std::span<const SignedRank> terms, RadicalSum& scratch, int& out,
                             bool* fallback = nullptr) const noexcept;

 private:
  std::span<const Level> levels_;
  Buffer<u128> roots_;
};

}  // namespace mhgp11::num
