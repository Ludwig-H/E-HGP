// MorseHGP3D v4 — L'ARBRE SPATIAL UNIQUE : RADIX DE KARRAS SUR POSITIONS UNIQUES.
//
// Decision d'architecture (voir docs/ARCHITECTURE.md § arbre) : la v4 n'a
// QU'UN SEUL arbre spatial, qui sert les trois consommateurs — source WSPD,
// comptage de temoins dual-tree (h_coeur, h_a, h_b), et requetes de lentille
// des ancres survivantes. La v3 en avait deux (arbre a mediane de rang dans un
// probe, arbre radix dans un autre) et la confrontation des deux a produit une
// refutation invalide, retractee le 16 aout 2026. Un seul arbre, une seule
// verite geometrique.
//
// Construction en trois passes plates, chacune calquee sur une primitive GPU
// connue (tri radix, un thread par nœud, remontee par compteurs) :
//
//   1. TRI par cle de Morton 48 bits des positions ;
//   2. BUCKETISATION des positions dupliquees : la geometrie vit sur les
//      positions UNIQUES (cles distinctes — l'arbre n'a besoin d'aucun
//      tie-break par index, defaut qui bornait la v3 a n <= 65535), les vrais
//      `PointId` et leur multiplicite vivent dans les buckets CSR ;
//   3. ARBRE RADIX BINAIRE de Karras sur les cles uniques + remontee des
//      boites serrees et des poids.
//
// L'arbre est le DEROULE BINAIRE de l'octree comprime : un nœud par longueur
// de prefixe en bits ; seuls les prefixes multiples de trois donnent des
// cellules cubiques, les autres ont un rapport d'aspect 2 ou 4. L'argument
// d'empilement de Callahan-Kosaraju se degrade d'un facteur BORNE, jamais
// d'une pente (rectification v3 du 16 aout 2026).
//
// SEPARATION SUR LA CELLULE OU LA BOITE SERREE : la cellule porte la borne de
// packing ; la boite serree, incluse, rend la separation plus facile donc le
// front plus petit, et la partition des paires reste garantie par la
// STRUCTURE de la recursion, pas par le test (lecon v3, mesure : la cellule
// seule multiplie le front par 6,5).
#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "../core/morton.hpp"
#include "../core/types.hpp"

namespace mhgp4 {

// Reference de nœud : >= 0 nœud interne, < 0 feuille `-1 - u` (position unique u).
using NodeRef = i32;

// Point d'entree de la bibliotheque : identite externe STABLE + position.
// Contrat v4 (audit e7e4d5e) : PointId != index dense != rang Morton — le
// noyau ne deduit JAMAIS l'identite de l'ordre physique des enregistrements.
struct InputPoint {
  PointId id = 0;
  P3 position{};
};

struct RadixNode {
  // < 0 : feuille, encodee `-1 - u` ou `u` est l'index de position unique.
  i32 left = 0, right = 0;
  // Plage [first, last] de positions uniques dans l'ordre de Morton : tout
  // nœud est CONTIGU dans les tableaux tries — le layout des kernels.
  i32 first = 0, last = 0;
  i32 parent = -1;
  // Cellule de Morton alignee (porte la borne de packing).
  i64 clo[3] = {0, 0, 0}, chi[3] = {0, 0, 0};
  // Boite serree du contenu, incluse dans la cellule (porte les certificats).
  i64 tlo[3] = {0, 0, 0}, thi[3] = {0, 0, 0};
};

// Index spatial complet d'un nuage : positions uniques triees par Morton,
// buckets d'identites, arbre radix, poids par prefixe.
struct CloudIndex {
  // Positions uniques dans l'ordre de Morton croissant.
  std::vector<u64> keys;              // m cles distinctes
  std::vector<P3> upos;               // position de chaque cle
  // CSR des identites : bucket_ids[bucket_start[u] .. bucket_start[u+1])
  // sont les `PointId` de la position unique u, en ordre d'identite croissant
  // (determinisme sous permutation d'entree).
  std::vector<u32> bucket_start;      // m + 1
  std::vector<PointId> bucket_ids;    // n
  // Poids cumulatif : weight(first, last) = wsum[last+1] - wsum[first] rend la
  // MULTIPLICITE totale d'une plage de positions uniques en O(1).
  std::vector<u64> wsum;              // m + 1
  // Arbre radix : m - 1 nœuds internes ; racine a l'index 0 si m >= 2.
  std::vector<RadixNode> nodes;

  int unique_count() const { return (int)keys.size(); }
  u64 range_weight(i32 first, i32 last) const {
    return wsum[(size_t)last + 1] - wsum[(size_t)first];
  }
  // FRONTIERE D'IDENTITE : identite externe de la position unique u (rang
  // Morton dense). Univoque sous le refus des positions dupliquees ; la
  // conversion GeometryIndex -> PointId n'a lieu qu'a travers cet accesseur,
  // jamais par un cast du rang.
  PointId point_id(i32 u) const {
    return bucket_ids[bucket_start[(size_t)u]];
  }
};

namespace detail {

// Longueur du prefixe commun de deux cles DISTINCTES (jamais de tie-break :
// l'unicite des cles est un invariant de construction, verifie par la porte).
inline int key_delta(const std::vector<u64>& k, int i, int j) {
  if (j < 0 || j >= (int)k.size()) return -1;
  return __builtin_clzll(k[(size_t)i] ^ k[(size_t)j]);
}

// Cellule alignee du prefixe de `p` bits (les 16 bits hauts de la cle 48 bits
// sont inutilises : le prefixe utile commence au bit 16).
inline void cell_of_prefix(u64 key, int p, RadixNode* node) {
  const int used = std::max(0, p - 16);
  const int level = std::min(16, used / 3);
  const int shift = 16 - level;
  for (int axis = 0; axis < 3; ++axis) {
    const u64 coords = morton_axis16(key, axis);
    const i64 base = (i64)((coords >> shift) << shift);
    node->clo[axis] = base;
    node->chi[axis] = base + ((1ll << shift) - 1);
  }
}

}  // namespace detail

// Construit l'index complet depuis des enregistrements {id externe, position}.
// Refus explicite (retour vide) si le nuage est vide ; une seule position
// unique donne un arbre sans nœud interne, cas que tout consommateur doit
// accepter. Le tri spatial DEPLACE les enregistrements sans reecrire `id` :
// la sortie est equivariante a un relabeling des PointId et invariante a une
// permutation physique des couples (id, position).
inline CloudIndex build_cloud_index(const std::vector<InputPoint>& pts) {
  CloudIndex ix;
  const size_t n = pts.size();
  if (n == 0) {
    ix.bucket_start = {0};
    ix.wsum = {0};
    return ix;
  }

  // 1. TRI par (cle, PointId externe) — l'id secondaire rend l'ordre des
  // buckets deterministe sous permutation physique des enregistrements.
  struct Rec {
    u64 key;
    PointId id;
    P3 pos;
  };
  std::vector<Rec> order(n);
  for (size_t i = 0; i < n; ++i) {
    const P3& p = pts[i].position;
    order[i] = {morton48((u64)p.x, (u64)p.y, (u64)p.z), pts[i].id, p};
  }
  std::sort(order.begin(), order.end(), [](const Rec& a, const Rec& b) {
    return a.key != b.key ? a.key < b.key : a.id < b.id;
  });

  // 2. BUCKETISATION des positions dupliquees.
  ix.bucket_ids.resize(n);
  for (size_t i = 0; i < n; ++i) {
    ix.bucket_ids[i] = order[i].id;
    if (i == 0 || order[i].key != order[i - 1].key) {
      ix.keys.push_back(order[i].key);
      ix.upos.push_back(order[i].pos);
      ix.bucket_start.push_back((u32)i);
    }
  }
  ix.bucket_start.push_back((u32)n);
  const int m = (int)ix.keys.size();
  ix.wsum.resize((size_t)m + 1);
  ix.wsum[0] = 0;
  for (int u = 0; u < m; ++u)
    ix.wsum[(size_t)u + 1] =
        ix.wsum[(size_t)u] + (ix.bucket_start[(size_t)u + 1] - ix.bucket_start[(size_t)u]);

  // 3. ARBRE RADIX de Karras — un nœud par index interne, parallele par
  // construction (chaque iteration est independante : un thread par nœud).
  if (m < 2) return ix;
  ix.nodes.resize((size_t)m - 1);
  for (int i = 0; i < m - 1; ++i) {
    const int dl = detail::key_delta(ix.keys, i, i - 1);
    const int dr = detail::key_delta(ix.keys, i, i + 1);
    const int d = (dr > dl) ? 1 : -1;
    const int dmin = std::min(dl, dr);
    int lmax = 2;
    while (detail::key_delta(ix.keys, i, i + lmax * d) > dmin) lmax <<= 1;
    int l = 0;
    for (int t = lmax >> 1; t >= 1; t >>= 1)
      if (detail::key_delta(ix.keys, i, i + (l + t) * d) > dmin) l += t;
    const int j = i + l * d;
    const int first = std::min(i, j), last = std::max(i, j);
    const int pref = detail::key_delta(ix.keys, first, last);
    int split = first, step = last - first;
    do {
      step = (step + 1) >> 1;
      const int cand = split + step;
      if (cand < last && detail::key_delta(ix.keys, first, cand) > pref) split = cand;
    } while (step > 1);
    RadixNode& node = ix.nodes[(size_t)i];
    node.first = first;
    node.last = last;
    node.left = (split == first) ? (-1 - split) : split;
    node.right = (split + 1 == last) ? (-1 - (split + 1)) : (split + 1);
    detail::cell_of_prefix(ix.keys[(size_t)first], pref, &node);
  }

  // Parents puis boites serrees, en une remontee postfixe sans recursion.
  std::vector<int> post;
  post.reserve(ix.nodes.size());
  std::vector<int> stack;
  stack.push_back(0);
  while (!stack.empty()) {
    const int v = stack.back();
    stack.pop_back();
    post.push_back(v);
    RadixNode& node = ix.nodes[(size_t)v];
    if (node.left >= 0) { ix.nodes[(size_t)node.left].parent = v; stack.push_back(node.left); }
    if (node.right >= 0) { ix.nodes[(size_t)node.right].parent = v; stack.push_back(node.right); }
  }
  for (auto it = post.rbegin(); it != post.rend(); ++it) {
    RadixNode& v = ix.nodes[(size_t)*it];
    bool first_child = true;
    const auto absorb = [&](const i64* lo, const i64* hi) {
      for (int axis = 0; axis < 3; ++axis) {
        if (first_child) { v.tlo[axis] = lo[axis]; v.thi[axis] = hi[axis]; }
        else {
          v.tlo[axis] = std::min(v.tlo[axis], lo[axis]);
          v.thi[axis] = std::max(v.thi[axis], hi[axis]);
        }
      }
      first_child = false;
    };
    for (int side = 0; side < 2; ++side) {
      const int c = side ? v.right : v.left;
      if (c < 0) {
        const P3& p = ix.upos[(size_t)(-1 - c)];
        const i64 lo[3] = {p.x, p.y, p.z};
        absorb(lo, lo);
      } else {
        absorb(ix.nodes[(size_t)c].tlo, ix.nodes[(size_t)c].thi);
      }
    }
  }
  return ix;
}

// Commodite de test : id = index d'entree. Le chemin de bibliotheque recoit
// des InputPoint{id, position} explicites — le noyau n'a pas le droit de
// deduire de cette commodite que l'ordre physique est l'identite.
inline CloudIndex build_cloud_index(const std::vector<P3>& pts) {
  std::vector<InputPoint> in(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) in[i] = {(PointId)i, pts[i]};
  return build_cloud_index(in);
}

}  // namespace mhgp4
