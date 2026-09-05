// MorseHGP3D v6 — L'INDEX SPATIAL UNIQUE : positions uniques triees par
// Morton, buckets d'identites, arbre radix binaire de Karras.
//
// Un seul arbre sert les trois consommateurs (source WSPD, comptage de
// temoins, requetes de lentille). Trois passes plates, chacune calquee sur une
// primitive GPU : tri par cle de Morton 48 bits ; bucketisation des positions
// dupliquees (la geometrie vit sur les positions UNIQUES, cles distinctes sans
// tie-break ; les `PointId` et leur multiplicite vivent dans un CSR) ; arbre
// radix sur les cles uniques + remontee des boites serrees.
//
// L'arbre est le deroule binaire de l'octree comprime : un nœud par longueur
// de prefixe ; seuls les prefixes multiples de trois donnent des cellules
// cubiques (rapport d'aspect 2 ou 4 sinon) — l'argument d'empilement de
// Callahan-Kosaraju se degrade d'un facteur BORNE, jamais d'une pente.
//
// Separation sur la BOITE SERREE : la cellule porte la borne de packing, la
// boite serree — incluse — rend la separation plus facile donc le front plus
// petit ; la partition des paires est garantie par la structure de la
// recursion, pas par le test.
//
// FRONTIERE D'IDENTITE : `PointId != index dense != rang Morton`. La conversion
// GeometryIndex -> PointId n'a lieu qu'a travers `point_id(u)` / les buckets,
// jamais par un cast du rang.
//
// GARDE D'ENTREE (statut `invalid_input`) : coordonnees hors profil u16 ou
// PointId dupliques => index VIDE (`valid() == false`), avant toute construction.
#pragma once

#include <algorithm>
#include <vector>

#include "../core/morton.hpp"
#include "../core/types.hpp"

namespace mhgp7 {

// Reference de nœud : >= 0 nœud interne, < 0 feuille `-1 - u` (position unique u).
using NodeRef = i32;
inline constexpr NodeRef leaf_ref(i32 u) { return -1 - u; }
inline constexpr i32 leaf_index(NodeRef v) { return -1 - v; }
inline constexpr bool is_leaf(NodeRef v) { return v < 0; }

struct RadixNode {
  i32 left = 0, right = 0;   // NodeRef
  i32 first = 0, last = 0;   // plage [first, last] de positions uniques (ordre Morton)
  i32 parent = -1;
  i64 clo[3] = {0, 0, 0}, chi[3] = {0, 0, 0};  // cellule de Morton alignee (packing)
  i64 tlo[3] = {0, 0, 0}, thi[3] = {0, 0, 0};  // boite serree du contenu (certificats)
};

struct AxisBox {
  i64 lo[3], hi[3];
};

struct NodeRange {
  i32 first, last;
};

struct CloudIndex {
  std::vector<u64> keys;           // m cles distinctes, croissantes
  std::vector<P3> upos;            // position de chaque cle
  std::vector<u32> bucket_start;   // m + 1 (CSR des identites)
  std::vector<PointId> bucket_ids; // n, ids croissants dans chaque bucket
  std::vector<u64> wsum;           // m + 1, multiplicites cumulees
  std::vector<RadixNode> nodes;    // m - 1 nœuds internes, racine 0 si m >= 2
  u64 input_count = 0;             // n recu
  bool valid = false;              // false = refus invalid_input

  int unique_count() const { return (int)keys.size(); }
  u64 range_weight(i32 first, i32 last) const { return wsum[(size_t)last + 1] - wsum[(size_t)first]; }
  u64 multiplicity(i32 u) const { return bucket_start[(size_t)u + 1] - bucket_start[(size_t)u]; }
  // Identite externe REPRESENTANTE de la position unique u (plus petit id du
  // bucket). Univoque quand les positions sont distinctes.
  PointId point_id(i32 u) const { return bucket_ids[bucket_start[(size_t)u]]; }
  bool has_duplicate_positions() const { return (u64)keys.size() != input_count; }
  NodeRef root() const { return nodes.empty() ? leaf_ref(0) : 0; }

  NodeRange range_of(NodeRef v) const {
    if (v >= 0) return NodeRange{nodes[(size_t)v].first, nodes[(size_t)v].last};
    return NodeRange{leaf_index(v), leaf_index(v)};
  }
  AxisBox box_of(NodeRef v) const {
    AxisBox b{};
    if (v >= 0) {
      for (int i = 0; i < 3; ++i) {
        b.lo[i] = nodes[(size_t)v].tlo[i];
        b.hi[i] = nodes[(size_t)v].thi[i];
      }
    } else {
      const P3& p = upos[(size_t)leaf_index(v)];
      b.lo[0] = b.hi[0] = p.x;
      b.lo[1] = b.hi[1] = p.y;
      b.lo[2] = b.hi[2] = p.z;
    }
    return b;
  }
  u64 node_weight(NodeRef v) const {
    const NodeRange r = range_of(v);
    return range_weight(r.first, r.last);
  }
};

namespace detail {

// Longueur du prefixe commun de deux cles DISTINCTES ; -1 hors bornes.
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

inline CloudIndex build_cloud_index(const std::vector<InputPoint>& pts) {
  CloudIndex ix;
  ix.input_count = pts.size();
  ix.bucket_start = {0};
  ix.wsum = {0};
  const size_t n = pts.size();
  if (n == 0) {
    ix.valid = true;
    return ix;
  }
  for (const InputPoint& p : pts)
    if (!p3_in_profile(p.position)) return ix;  // invalid_input
  {
    std::vector<PointId> ids(n);
    for (size_t i = 0; i < n; ++i) ids[i] = pts[i].id;
    std::sort(ids.begin(), ids.end());
    for (size_t i = 1; i < n; ++i)
      if (ids[i] == ids[i - 1]) return ix;  // invalid_input
  }
  ix.bucket_start.clear();
  ix.wsum.clear();

  // 1. Tri par (cle, PointId) : ordre des buckets deterministe sous
  // permutation physique des enregistrements.
  struct Rec {
    u64 key;
    PointId id;
    P3 pos;
  };
  std::vector<Rec> order(n);
  for (size_t i = 0; i < n; ++i) order[i] = {morton48(pts[i].position), pts[i].id, pts[i].position};
  std::sort(order.begin(), order.end(),
            [](const Rec& a, const Rec& b) { return a.key != b.key ? a.key < b.key : a.id < b.id; });

  // 2. Bucketisation des positions dupliquees.
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
  for (int u = 0; u < m; ++u) ix.wsum[(size_t)u + 1] = ix.wsum[(size_t)u] + ix.multiplicity(u);
  ix.valid = true;

  // 3. Arbre radix de Karras : chaque nœud interne est independant (un thread
  // par nœud sur device).
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
    node.left = (split == first) ? leaf_ref(split) : split;
    node.right = (split + 1 == last) ? leaf_ref(split + 1) : split + 1;
    detail::cell_of_prefix(ix.keys[(size_t)first], pref, &node);
  }

  // Parents puis boites serrees, remontee postfixe sans recursion.
  std::vector<int> post;
  post.reserve(ix.nodes.size());
  std::vector<int> stack{0};
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
        const P3& p = ix.upos[(size_t)leaf_index(c)];
        const i64 lo[3] = {p.x, p.y, p.z};
        absorb(lo, lo);
      } else {
        absorb(ix.nodes[(size_t)c].tlo, ix.nodes[(size_t)c].thi);
      }
    }
  }
  return ix;
}

// Commodite de test : id = index d'entree.
inline CloudIndex build_cloud_index(const std::vector<P3>& pts) {
  std::vector<InputPoint> in(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) in[i] = {(PointId)i, pts[i]};
  return build_cloud_index(in);
}

}  // namespace mhgp7
