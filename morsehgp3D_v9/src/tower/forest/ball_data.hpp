// MorseHGP3D v9 — enregistrement de catalogue consomme par la tour FULL.
// Port de morsehgp3D_v7/src/pipeline/expand.hpp:41-58 (dc57ffd5), sans le
// pipeline d'expansion ni le fold v4 que ce fichier incluait.
//
// Une BallData est UNE boule du catalogue canonique : cle primitive exacte,
// niveau exact (rayon au carre), arite = taille du support minimal (q_min),
// interieurs stricts et coquille COMPLETE en index de positions uniques de
// l'index. Plafonds : interieurs <= 9 (la boule ne sert aucun K <= 10 au-dela),
// coquille <= 12 ; une coquille plus grande est un REFUS DE DOMAINE explicite
// du producteur de catalogue, jamais une troncature (contre-audit A § 1).
#pragma once

#include <cstddef>
#include <span>

#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"

namespace mhgp9::tower {

inline constexpr size_t kBallInteriorMax = 9;
inline constexpr size_t kBallShellMax = 12;

struct BallData {
  BallKey key;
  ExactLevel level;
  u8 arity = 0;
  u8 n_interior = 0, n_shell = 0;
  i32 interior_ids[kBallInteriorMax] = {};
  i32 shell_ids[kBallShellMax] = {};
  std::span<const i32> interior() const { return std::span<const i32>(interior_ids, (size_t)n_interior); }
  std::span<const i32> shell() const { return std::span<const i32>(shell_ids, (size_t)n_shell); }
};

}  // namespace mhgp9::tower
