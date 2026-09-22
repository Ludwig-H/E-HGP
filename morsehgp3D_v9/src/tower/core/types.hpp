// MorseHGP3D v9 (port de v7 dc57ffd5) — types de base du profil d'entree
// `quantized_u18_input_only`.
//
// Coordonnees sur la grille entiere [0, 2^18)^3, M = 262143 ; toute
// arithmetique de decision est entiere et dimensionnee. Largeurs sous u18
// (deltas dans [-M, M]) : difference 19 bits signes ; carre d'une distance,
// produit scalaire < 3M^2 < 2^38, composante d'un produit vectoriel < 2M^2 <
// 2^37 (i64) ; produit de deux telles quantites < 2^76 (i128). Chaque predicat
// declare sa largeur en tete de fichier ; `mhgp9_tower_arith_selftest` la
// verifie sur les extremes u18 contre un oracle Boost.
//
// Identites : `PointId` (u32) est l'identite STABLE fournie par l'appelant —
// distincte de l'index dense, du rang de Morton et de tout ordre de tri. u16
// borne la GRILLE (2^48 sites), jamais le cardinal du nuage.
#pragma once

#include <cstdint>

namespace mhgp9::tower {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
// `__extension__` : __int128 n'est pas ISO, le build est -Wpedantic -Werror.
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

using PointId = u32;

inline constexpr int kCoordBits = 18;
inline constexpr i64 kCoordMax = (i64{1} << kCoordBits) - 1;  // 262143

// Point de calcul : coordonnees u18 promues en i64 pour que toute
// soustraction soit deja dans le type de l'arithmetique de decision.
struct P3 {
  i64 x = 0, y = 0, z = 0;
  constexpr bool operator==(const P3& o) const { return x == o.x && y == o.y && z == o.z; }
  constexpr bool operator!=(const P3& o) const { return !(*this == o); }
};

inline constexpr P3 p3_sub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline constexpr P3 p3_add(const P3& a, const P3& b) { return P3{a.x + b.x, a.y + b.y, a.z + b.z}; }
// |.| < 2^38 sous u18.
inline constexpr i64 p3_dot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr i64 p3_norm2(const P3& a) { return p3_dot(a, a); }
inline constexpr P3 p3_cross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline constexpr bool p3_in_profile(const P3& p) {
  return p.x >= 0 && p.x <= kCoordMax && p.y >= 0 && p.y <= kCoordMax && p.z >= 0 && p.z <= kCoordMax;
}

// Enregistrement d'entree de la bibliotheque : identite externe + position.
// Le noyau ne deduit JAMAIS l'identite de l'ordre physique des enregistrements.
struct InputPoint {
  PointId id = 0;
  P3 position{};
};

}  // namespace mhgp9::tower

