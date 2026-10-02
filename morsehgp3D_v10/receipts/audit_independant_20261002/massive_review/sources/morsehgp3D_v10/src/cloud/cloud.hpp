// Preparation du nuage : controle du domaine, tri de Morton, sites a multiplicite, table site -> PointId.
//
// Un SITE est une position distincte ; les points de meme coordonnee forment un site de poids w >= 1
// (la specification compte les doublons avec multiplicite ; la v9 les refusait). L'index de site est
// le rang dans l'ordre de Morton : canonique, invariant par permutation de l'entree et par
// renumerotation des PointId.
#pragma once

#include <span>

#include "core/buffer.hpp"
#include "core/status.hpp"

namespace mhgp10 {

inline constexpr int kMaxCoordinateBits = 21;  // cle de Morton sur 63 bits

struct Cloud {
  int bits = kCoordinateBits;
  Buffer<u32> x, y, z;   // coordonnees des sites (ordre de Morton)
  Buffer<u32> w;         // multiplicite de chaque site
  Csr<PointId> ids;      // site -> PointId tries
  u64 weight = 0;        // somme des multiplicites = nombre de points d'entree
  u32 sites() const { return static_cast<u32>(x.size()); }
};

u64 morton3(u32 x, u32 y, u32 z);

// Controle et prepare. invalid_input : entree vide, tailles differentes, coordonnee hors de
// [0, 2^bits), PointId en double, bits hors de [1, 21].
Result<Cloud> prepare_cloud(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                            std::span<const u32> point_ids, int bits = kCoordinateBits,
                            MemoryBudget& budget = default_budget());

}  // namespace mhgp10
