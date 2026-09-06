// MorseHGP3D v6 — code de Morton 48 bits.
//
// Morton n'est pas une structure de donnees : c'est la CLE DE TRI qui
// construit l'unique arbre spatial (tree/cloud_index.hpp) et le layout qui
// rend chaque nœud contigu. Les cles sont formees sur POSITIONS UNIQUES, donc
// distinctes : l'arbre n'a besoin d'aucun tie-break par index.
#pragma once

#include "types.hpp"

namespace mhgp7 {

// Bits d'une coordonnee 16 bits ecartes au pas TROIS (le masque 0x5555 du pas
// deux ferait se chevaucher les axes).
inline constexpr u64 morton_spread3(u64 v) {
  v &= 0xFFFFull;
  v = (v | (v << 32)) & 0x001F00000000FFFFull;
  v = (v | (v << 16)) & 0x001F0000FF0000FFull;
  v = (v | (v << 8)) & 0x100F00F00F00F00Full;
  v = (v | (v << 4)) & 0x10C30C30C30C30C3ull;
  v = (v | (v << 2)) & 0x1249249249249249ull;
  return v;
}

// Cle 48 bits : x au bit 0, y au bit 1, z au bit 2 de chaque triplet.
inline constexpr u64 morton48(u64 x, u64 y, u64 z) {
  return morton_spread3(x) | (morton_spread3(y) << 1) | (morton_spread3(z) << 2);
}

inline constexpr u64 morton48(const P3& p) { return morton48((u64)p.x, (u64)p.y, (u64)p.z); }

// Extraction inverse d'un axe (0 = x, 1 = y, 2 = z).
inline constexpr u64 morton_axis16(u64 key, int axis) {
  u64 v = 0;
  for (int b = 0; b < 16; ++b)
    if (key & (1ull << (3 * b + axis))) v |= (1ull << b);
  return v;
}

}  // namespace mhgp7

