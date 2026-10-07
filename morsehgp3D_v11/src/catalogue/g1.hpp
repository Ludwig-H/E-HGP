// Test G1 de dominance du filtre des noeuds (boxes.cpp, 7 octobre 2026). Un site x quitte la liste d'un noeud s'il a
// kmax temoins y qui le dominent strictement sur toute la fermeture de la boite :
//   x.square - y.square > max(0, x.scaled[0] - y.scaled[0]) + max(0, x.scaled[1] - y.scaled[1])
//                         + max(0, x.scaled[2] - y.scaled[2]).
// Deux executions aux memes decisions et au meme compte de tests : la boucle de reference (temoins dans leur ordre,
// arret au kmax-ieme dominateur, compte = temoins examines) et un masque AVX2 de tous les temoins, dont on deduit le
// meme arret (rang du kmax-ieme dominateur, plus un). La seconde est choisie a l'execution si le processeur la porte.
// Entiers seulement : aucun flottant, aucune decision approchee. Diagnostic claudediag1 : en voie lot, ce filtre fait
// a peu pres toute la passe unique (1,4 a 1,6 s de CPU a K5 sur G4) et une partie des rondes de la frontiere.
#pragma once

#include <array>
#include <span>

#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {

// Termes separes par site : 2*largeur*(x-lo) par axe et |x-lo|^2. Leur difference redonne exactement 2*largeur*(x-y) ;
// memes entiers (< 2^(2B+2)), donc memes decisions G1.
struct G1Terms {
  std::array<i64, 3> scaled;
  i64 square;
};

inline G1Terms g1_terms(const std::array<i64, 3>& site, const Box& box) noexcept {
  G1Terms t{};
  for (int axis = 0; axis < 3; ++axis) {
    const i64 x = site[axis] - box.lo[axis];
    t.scaled[axis] = 2 * (box.hi[axis] - box.lo[axis]) * x;
    t.square += x * x;
  }
  return t;
}

inline constexpr u32 kG1Slots = 3 * kMaxOrder;  // 36 temoins au plus (reservoir de 3K)
static_assert(kG1Slots % 4 == 0, "G1 : colonnes par blocs de quatre");

// Temoins d'un noeud en colonnes alignees ; les cases de selected au multiple de quatre suivant valent zero.
struct G1Witnesses {
  alignas(32) std::array<i64, kG1Slots> s0, s1, s2, square;
  u32 selected = 0;
};

// Remplit les colonnes depuis les termes des temoins (au plus kG1Slots) et met a zero le reste du dernier bloc.
void g1_witnesses(std::span<const G1Terms> terms, G1Witnesses& out) noexcept;

struct G1Result {
  u32 found = 0, tests = 0;
};

// Un site : boucle de reference, ou masque AVX2 (n'appeler que si g1_avx2_available()).
[[nodiscard]] G1Result g1_scalar(const G1Terms& x, const G1Witnesses& w, u32 kmax) noexcept;
[[nodiscard]] G1Result g1_avx2(const G1Terms& x, const G1Witnesses& w, u32 kmax) noexcept;
[[nodiscard]] bool g1_avx2_available() noexcept;

// Toute la liste d'un noeud : garde dans out (dans l'ordre du parent) les sites qui ont moins de kmax dominateurs,
// rend leur nombre dans count et ajoute les tests examines a tests. Execution retenue une fois par processus.
void g1_filter(const Cloud& cloud, std::span<const SiteIdx> parent, const Box& box, const G1Witnesses& w, u32 kmax,
               SiteIdx* out, u32& count, u64& tests) noexcept;

}  // namespace mhgp11::catalogue_detail
