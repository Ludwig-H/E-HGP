// MorseHGP3D v4 — TYPES DE BASE DU PROFIL D'ENTREE.
//
// Profil unique de cette exploration : `quantized_u16_input_only`. Les
// coordonnees vivent sur la grille entiere [0, 65536)^3 — 2^48 sites — et
// TOUTE l'arithmetique de decision est entiere et dimensionnee. Aucun `double`
// non encadre ne decide quoi que ce soit.
//
// Identites (heritees des lecons v3, PROPOSITION.md § 2.1) :
//  - `PointId` est une identite STABLE, distincte de l'index dense, du rang
//    de Morton et de tout ordre de tri. Elle tient sur 32 bits : le contrat
//    v4 vise des nuages de dizaines de millions de points, u16 ne borne que
//    la GRILLE (2^48 sites), jamais le cardinal du nuage.
//  - Les positions dupliquees sont autorisees : la geometrie est bucketisee
//    par position unique, les vrais `PointId` et leur multiplicite sont
//    conserves dans les pools de temoins. Un quotient silencieux changerait
//    les profondeurs.
//
// Largeurs (profil u16, deltas dans [-65535, 65535]) :
//  - difference de coordonnees : 17 bits signes ;
//  - carre d'une distance : <= 3 * 65535^2 < 2^34 — tient dans i64 ;
//  - produit scalaire de deux deltas : |.| <= 3 * 65535^2 < 2^34 — i64 ;
//  - composante d'un produit vectoriel : |.| <= 2 * 65535^2 < 2^34 — i64 ;
//  - norme carree d'un produit vectoriel : < 3 * 2^66 < 2^68 — i128 ;
//  - tout produit de deux quantites en 2^34 : < 2^68 — i128.
// Chaque predicat declare sa largeur maximale en commentaire et le selftest
// arithmetique la verifie sur les extremes u16.
#pragma once

#include <cstdint>

namespace mhgp4 {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
// `__extension__` : __int128 n'est pas ISO, le build est -Wpedantic -Werror.
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

// Identite stable d'un point du nuage. Jamais un index de tri.
using PointId = u32;

// Coordonnee quantifiee du profil.
inline constexpr int kCoordBits = 16;
inline constexpr i64 kCoordMax = 65535;

// Point de calcul : coordonnees u16 promues en i64 pour que toute soustraction
// soit deja dans le type de l'arithmetique de decision.
struct P3 {
  i64 x = 0, y = 0, z = 0;
};

inline constexpr P3 p3_sub(const P3& a, const P3& b) {
  return P3{a.x - b.x, a.y - b.y, a.z - b.z};
}

// dot de deltas u16 : |.| < 2^34, le i128 est une marge, pas une necessite.
inline constexpr i64 p3_dot(const P3& a, const P3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline constexpr i64 p3_norm2(const P3& a) { return p3_dot(a, a); }

inline constexpr P3 p3_cross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Nuage en SoA : le layout memoire du GPU et du fallback CPU vectorise.
// `positions dupliquees` : voir CloudIndex (tree/) pour la bucketisation.
struct Cloud {
  // Coordonnees par axe, indexees par PointId (l'ordre d'entree fait foi).
  // 6 octets par point : 10 M de points = 60 Mo, 50 M = 300 Mo.
  std::uint32_t n = 0;
  const u16* xs = nullptr;
  const u16* ys = nullptr;
  const u16* zs = nullptr;

  P3 at(PointId id) const {
    return P3{(i64)xs[id], (i64)ys[id], (i64)zs[id]};
  }
};

}  // namespace mhgp4
