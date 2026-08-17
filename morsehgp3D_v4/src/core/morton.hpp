// MorseHGP3D v4 — CODE DE MORTON 48 BITS.
//
// Morton n'est PAS une structure de donnees du projet : c'est la CLE DE TRI
// qui construit l'unique arbre spatial de la v4 (tree/radix_tree.hpp) et le
// layout memoire qui rend chaque nœud CONTIGU dans l'ordre trie. Il n'existe
// ni « octree » separe, ni second arbre : l'arbre radix binaire sur les cles
// de Morton EST le deroule binaire de l'octree comprime (lecon v3 :
// `wspd_wavefront.hpp`, rectification du 16 aout 2026 — les nœuds a prefixe
// non multiple de trois ont un rapport d'aspect 2 ou 4, jamais 1 ; l'argument
// d'empilement de Callahan-Kosaraju s'en degrade d'un facteur BORNE).
//
// Les cles sont formees sur POSITIONS UNIQUES (apres bucketisation des
// doublons, voir tree/) : elles sont donc DISTINCTES et l'arbre n'a besoin
// d'aucun tie-break par index — c'est le defaut qui bornait la v3 a
// n <= 65535. La multiplicite vit dans les buckets, pas dans la cle.
#pragma once

#include "../core/types.hpp"

namespace mhgp4 {

// Entrelacement 3D d'une coordonnee 16 bits : bits ecartes au pas TROIS.
// Masques du pas trois — le masque 0x5555... du pas deux ferait se chevaucher
// les trois axes (lecon gravee dans le commentaire v3).
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

// Extraction inverse d'un axe (i = 0 pour x, 1 pour y, 2 pour z).
inline constexpr u64 morton_axis16(u64 key, int i) {
  u64 axis = 0;
  for (int b = 0; b < 16; ++b)
    if (key & (1ull << (3 * b + i))) axis |= (1ull << b);
  return axis;
}

}  // namespace mhgp4
