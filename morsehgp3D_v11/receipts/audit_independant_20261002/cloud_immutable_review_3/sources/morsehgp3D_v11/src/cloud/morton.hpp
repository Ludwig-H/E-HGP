// Cle de Morton d'une position : entrelacement des bits des trois coordonnees. Le bit i de x va a la position 3i de
// la cle, celui de y a 3i + 1, celui de z a 3i + 2 : x porte le bit de poids faible de chaque triplet, convention de
// la v10 (src/cloud/cloud.cpp du raccord R2, commit 865f5e6, fonction morton3), dont ce fichier est le port.
//
// Ce qui change par rapport a la v10 : le type de la cle suit le nombre de bits B des coordonnees. La cle porte 3B
// bits : 54 a B = 18, 63 a B = 21, 72 a B = 24. La v10 n'avait qu'une cle de 63 bits et masquait toute coordonnee a
// 21 bits (v &= 0x1FFFFF) : une coordonnee plus large aurait perdu ses bits hauts en silence. Ici la cle est un u64
// tant que 3B <= 64, un u128 au-dela, et aucun bit n'est masque : chaque bit d'une coordonnee de [0, 2^B) a sa
// position dans la cle (static_assert plus bas).
//
// Preuve de l'ecartement (spread21). Chaque etape calcule v = (v | (v << s)) & m. Avant l'etape, les bits de v sont
// dans le masque de l'etape precedente, m_prec. Les static_assert etablissent m_prec & (m_prec << s) == 0 : v et
// v << s n'ont aucun bit commun, donc v | (v << s) = v XOR (v << s), et l'etape est lineaire sur GF(2). La
// composee des cinq etapes l'est aussi, sur les entrees v < 2^21. Une application lineaire est fixee par ses valeurs
// sur une base : le static_assert final controle les 21 entrees a un seul bit (le bit i va a la position 3i), ce qui
// etablit le resultat pour toute entree v < 2^21. Les trois profils (18, 21 et 24 bits) sont controles dans toute
// construction, quel que soit le profil compile.
#pragma once

#include <type_traits>

#include "core/core.hpp"

namespace mhgp11 {

namespace detail {

// Masques de l'ecartement : m0 est le domaine (21 bits), m5 les positions multiples de 3.
inline constexpr u64 kSpreadM0 = 0x00000000001FFFFFull;
inline constexpr u64 kSpreadM1 = 0x001F00000000FFFFull;
inline constexpr u64 kSpreadM2 = 0x001F0000FF0000FFull;
inline constexpr u64 kSpreadM3 = 0x100F00F00F00F00Full;
inline constexpr u64 kSpreadM4 = 0x10C30C30C30C30C3ull;
inline constexpr u64 kSpreadM5 = 0x1249249249249249ull;
static_assert((kSpreadM0 & (kSpreadM0 << 32)) == 0, "morton : etape 1 sans recouvrement");
static_assert((kSpreadM1 & (kSpreadM1 << 16)) == 0, "morton : etape 2 sans recouvrement");
static_assert((kSpreadM2 & (kSpreadM2 << 8)) == 0, "morton : etape 3 sans recouvrement");
static_assert((kSpreadM3 & (kSpreadM3 << 4)) == 0, "morton : etape 4 sans recouvrement");
static_assert((kSpreadM4 & (kSpreadM4 << 2)) == 0, "morton : etape 5 sans recouvrement");

// Ecarte les 21 bits de v : le bit i va a la position 3i. Precondition : v < 2^21.
constexpr u64 spread21(u64 v) noexcept {
  v = (v | (v << 32)) & kSpreadM1;
  v = (v | (v << 16)) & kSpreadM2;
  v = (v | (v << 8)) & kSpreadM3;
  v = (v | (v << 4)) & kSpreadM4;
  v = (v | (v << 2)) & kSpreadM5;
  return v;
}

constexpr bool spread21_on_basis() noexcept {
  for (int i = 0; i < 21; ++i)
    if (spread21(u64{1} << i) != u64{1} << (3 * i)) return false;
  return true;
}
static_assert(spread21_on_basis(), "morton : le bit i d'une coordonnee va a la position 3i de la cle");

// Type de la cle pour des coordonnees de `Bits` bits : le plus petit des deux entiers qui porte 3 * Bits bits.
template <int Bits>
using MortonKeyFor = std::conditional_t<(3 * Bits <= 64), u64, u128>;

// Ecarte les `Bits` bits de v. Precondition : v < 2^Bits. Jusqu'a 21 bits, un seul ecartement. Au-dela, les 21 bits
// bas puis les bits hauts : 3 * 21 = 63, le bit 21 va donc a la position 63, et v >> 21 < 2^(Bits - 21) <= 2^21.
template <int Bits>
constexpr MortonKeyFor<Bits> spread(u32 v) noexcept {
  static_assert(Bits >= 1 && Bits <= 32, "morton : 1 a 32 bits par coordonnee");
  static_assert(static_cast<int>(sizeof(MortonKeyFor<Bits>)) * 8 >= 3 * Bits, "morton : la cle porte 3B bits");
  if constexpr (Bits <= 21) {
    return spread21(v);
  } else {
    static_assert(Bits - 21 <= 21, "morton : les bits hauts tiennent dans un second ecartement");
    const u128 low = spread21(v & kSpreadM0);
    const u128 high = spread21(v >> 21);
    return low | (high << 63);
  }
}

template <int Bits>
constexpr MortonKeyFor<Bits> morton_key_for(u32 x, u32 y, u32 z) noexcept {
  // Bit le plus haut : 3 (Bits - 1) + 2 = 3 Bits - 1, dans la cle (static_assert de spread) : aucun decalage ne
  // sort du type.
  return static_cast<MortonKeyFor<Bits>>(spread<Bits>(x) | (spread<Bits>(y) << 1) | (spread<Bits>(z) << 2));
}

// Controle d'un profil : chaque bit de chaque axe est a sa position, seul.
template <int Bits>
constexpr bool morton_on_basis() noexcept {
  using Key = MortonKeyFor<Bits>;
  for (int i = 0; i < Bits; ++i) {
    const u32 bit = u32{1} << i;
    if (morton_key_for<Bits>(bit, 0, 0) != Key{1} << (3 * i)) return false;
    if (morton_key_for<Bits>(0, bit, 0) != Key{1} << (3 * i + 1)) return false;
    if (morton_key_for<Bits>(0, 0, bit) != Key{1} << (3 * i + 2)) return false;
  }
  return true;
}
static_assert(morton_on_basis<18>() && morton_on_basis<21>() && morton_on_basis<24>(),
              "morton : convention x, y, z controlee aux trois profils");

}  // namespace detail

// Bits de la cle de Morton du profil : 54, 63 ou 72.
inline constexpr int kMortonBits = 3 * kCoordBits;

// Cle de Morton du profil : u64 a B = 18 et 21, u128 a B = 24.
using MortonKey = detail::MortonKeyFor<kCoordBits>;
static_assert(static_cast<int>(sizeof(MortonKey)) * 8 >= kMortonBits, "morton : la cle du profil porte 3B bits");

// Cle de Morton de la position (x, y, z). Precondition : x, y, z <= kCoordMax (domaine controle par prepare_cloud).
// Deux positions du domaine ont la meme cle si et seulement si elles sont egales : l'entrelacement est injectif.
constexpr MortonKey morton_key(u32 x, u32 y, u32 z) noexcept { return detail::morton_key_for<kCoordBits>(x, y, z); }

}  // namespace mhgp11
