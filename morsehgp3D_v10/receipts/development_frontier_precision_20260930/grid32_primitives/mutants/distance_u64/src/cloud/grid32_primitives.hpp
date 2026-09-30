// Primitives autonomes de grille u32 : ni moteur u32, ni qualification FULL.
// Les bits definissent le domaine entier, pas le pas physique de la grille.
#pragma once

#include <optional>

#include "core/types.hpp"

namespace mhgp10::grid32 {

// Coordonnees uniquement : aucune identite PointId/SiteIdx n'est encodee ici.
struct PointGrid32 {
  u32 x = 0, y = 0, z = 0;
  constexpr bool operator==(const PointGrid32&) const = default;
};

inline constexpr u128 kMaxSquaredDistance =
    u128{3} * u128{0xffffffffu} * u128{0xffffffffu};
inline constexpr u128 kMaxMortonKey = (u128{1} << 96) - 1;

// |delta| <= 2^32-1 : la difference tient en i64, chaque carre en i128.
// La somme maximale vaut 55340232195358851075 < 2^66 : resultat u128.
// Trois differences, trois produits elargis, deux additions ; aucun tas.
constexpr u128 squared_distance(PointGrid32 a, PointGrid32 b) noexcept {
  const i64 dx = i64{a.x} - i64{b.x};
  const i64 dy = i64{a.y} - i64{b.y};
  const i64 dz = i64{a.z} - i64{b.z};
  return static_cast<u128>(static_cast<u64>(
      static_cast<u128>(i128{dx} * i128{dx}) +
      static_cast<u128>(i128{dy} * i128{dy}) +
      static_cast<u128>(i128{dz} * i128{dz})));
}

// Bit b de x/y/z -> bit 3b/3b+1/3b+2. Tous les 32 bits sont conserves.
// Cle de 96 bits portee par u128, pas une representation binaire native.
// 32 tours fixes, stockage constant, aucun tas ni depassement de decalage.
constexpr u128 morton96(PointGrid32 point) noexcept {
  u128 key = 0;
  for (unsigned bit = 0; bit < 32; ++bit) {
    key |= u128{(point.x >> bit) & 1u} << (3 * bit);
    key |= u128{(point.y >> bit) & 1u} << (3 * bit + 1);
    key |= u128{(point.z >> bit) & 1u} << (3 * bit + 2);
  }
  return key;
}

// Une cle publique avec un bit >=96 n'est PAS tronquee : refus explicite.
// Sinon extraction disjointe des trois axes, en 32 tours fixes sans tas.
constexpr std::optional<PointGrid32> decode_morton96(u128 key) noexcept {
  if ((key >> 96) != 0) return std::nullopt;
  PointGrid32 point;
  for (unsigned bit = 0; bit < 32; ++bit) {
    point.x |= static_cast<u32>((key >> (3 * bit)) & 1) << bit;
    point.y |= static_cast<u32>((key >> (3 * bit + 1)) & 1) << bit;
    point.z |= static_cast<u32>((key >> (3 * bit + 2)) & 1) << bit;
  }
  return point;
}

}  // namespace mhgp10::grid32
