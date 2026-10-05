// Interne de head (tranche S10) : arbre condense, encadrements des plateaux et repli exact. Hors API.
#pragma once

#include "head/head.hpp"

namespace mhgp11::head::detail {

// Entier large signe des encadrements : 2^192 phi tient en 264 bits (date >= 2^-24, z <= 3), une somme de n <= 2^32
// termes ponderes par des comptes <= 2^32 en 328 bits.
using Fixed = num::Wide<6>;
inline constexpr u32 kFixedShift = 192;
// E = 2^64 e en dessous de 2^40 (e < 2^-24) : aucun encadrement utile, le repli exact decide.
inline constexpr u32 kMinBracketBits = 40;

// Arbre condense (Condensed de bench/points_flat.py) : clusters dans l'ordre de naissance, enfants avant parents,
// racine virtuelle en dernier pour une foret. top = kNone : haut infini (phi = 0). Jonctions en CSR par cluster.
struct Condensed {
  u32 count = 0;
  Buffer<u32> parent, top;
  Buffer<u64> size;
  Csr<u32> children;
  Buffer<u64> join_off;
  Buffer<u32> join_plateau;
  Buffer<u64> join_count;
  Buffer<u32> first;  // premier cluster de chaque site, kNone si aucun
};

// Critere A (condense, bench/points_flat.py:647-750).
[[nodiscard]] Outcome condense(const TreeView& tree, u32 mcs, MemoryBudget& budget, Condensed& out) noexcept;

// Encadrement de 2^192 phi par plateau ; zero : niveau nul (phi(0) interdit) ; open : pas d'encadrement utile.
struct Brackets {
  Buffer<Fixed> lo, hi;
  Buffer<u8> zero, open;
  u64 unbracketed = 0;
};
[[nodiscard]] Outcome bracket_plateaus(const TreeView& tree, const LevelSource& levels, u32 z, MemoryBudget& budget,
                                       Brackets& out) noexcept;

// Poids entier d'un plateau dans une somme de scores.
struct Weight {
  u32 plateau;
  i64 weight;
};
// Signe exact de sum w_p phi(p) (poids deja fusionnes par plateau, non nuls) : 0 seulement sur egalite certifiee.
[[nodiscard]] Outcome exact_sign(const TreeView& tree, const LevelSource& levels, u32 z, std::span<const Weight> weights,
                                 MemoryBudget& budget, int& out) noexcept;

}  // namespace mhgp11::head::detail
