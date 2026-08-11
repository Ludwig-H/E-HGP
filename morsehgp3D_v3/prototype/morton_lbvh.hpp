// MorseHGP3D v3 — LA DISPOSITION RESIDENTE PARTAGEE : (MortonKey, PointId)
// et LBVH radix exact (PROPOSITION, jalon 2 : « une disposition unique et un
// LBVH exact residents, partages par toutes les lanes »).
//
// Cle Morton 48 bits : trois axes u16 entrelaces (x bit 0, y bit 1, z bit 2).
// Ordre canonique (cle, PointId) : l'ownership des colocalises est total et
// rejouable. LBVH radix : arbre binaire sur l'ordre trie, coupe au bit
// dominant de la premiere difference de cles (construction de type Karras,
// portable device), midpoint en secours pour un bloc de cles egales ; chaque
// noeud porte sa boite AABB u16 exacte et sa plage [begin,end) de positions.
// Dominance : si a >= b composante par composante alors cle(a) >= cle(b)
// (monotonie axe par axe puis chainage) — les fixtures d'ancre haute en
// dependent.
#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

inline unsigned long long morton_spread3(unsigned long long v) {
  v &= 0xFFFFULL;
  v = (v | (v << 32)) & 0x1F00000000FFFFULL;
  v = (v | (v << 16)) & 0x1F0000FF0000FFULL;
  v = (v | (v << 8)) & 0x100F00F00F00F00FULL;
  v = (v | (v << 4)) & 0x10C30C30C30C30C3ULL;
  v = (v | (v << 2)) & 0x1249249249249249ULL;
  return v;
}

inline unsigned long long morton_key_of(const mhgp::P3& p) {
  return morton_spread3((unsigned long long)(unsigned)p.x) |
         (morton_spread3((unsigned long long)(unsigned)p.y) << 1) |
         (morton_spread3((unsigned long long)(unsigned)p.z) << 2);
}

struct LbvhNode {
  long long lo[3] = {0, 0, 0};
  long long hi[3] = {0, 0, 0};
  int begin = 0, end = 0;
  int left = -1, right = -1;
};

struct MortonLbvh {
  std::vector<LbvhNode> nodes;
  std::vector<int> order;                  // order[position] = PointId
  std::vector<unsigned long long> keys;    // cles triees, parallele a order
  const std::vector<mhgp::P3>* points = nullptr;

  void build(const std::vector<mhgp::P3>& cloud, int leaf_size) {
    points = &cloud;
    const int n = (int)cloud.size();
    std::vector<std::pair<unsigned long long, int>> slots((std::size_t)n);
    for (int i = 0; i < n; ++i)
      slots[(std::size_t)i] = {morton_key_of(cloud[(std::size_t)i]), i};
    std::sort(slots.begin(), slots.end());
    order.resize((std::size_t)n);
    keys.resize((std::size_t)n);
    for (int t = 0; t < n; ++t) {
      keys[(std::size_t)t] = slots[(std::size_t)t].first;
      order[(std::size_t)t] = slots[(std::size_t)t].second;
    }
    nodes.clear();
    nodes.reserve(2 * (std::size_t)(n / leaf_size + 2));
    build_range(0, n, leaf_size);
  }

  int build_range(int begin, int end, int leaf_size) {
    const int self = (int)nodes.size();
    nodes.push_back(LbvhNode{});
    LbvhNode node{};
    node.begin = begin;
    node.end = end;
    for (int d = 0; d < 3; ++d) {
      node.lo[d] = (long long)1 << 40;
      node.hi[d] = -((long long)1 << 40);
    }
    for (int t = begin; t < end; ++t) {
      const mhgp::P3& p = (*points)[(std::size_t)order[(std::size_t)t]];
      const long long c[3] = {(long long)p.x, (long long)p.y, (long long)p.z};
      for (int d = 0; d < 3; ++d) {
        node.lo[d] = std::min(node.lo[d], c[d]);
        node.hi[d] = std::max(node.hi[d], c[d]);
      }
    }
    if (end - begin > leaf_size) {
      const unsigned long long first = keys[(std::size_t)begin];
      const unsigned long long last = keys[(std::size_t)(end - 1)];
      int mid;
      if (first == last) {
        mid = begin + (end - begin) / 2;
      } else {
        const int split_bit = 63 - __builtin_clzll(first ^ last);
        const unsigned long long pivot = (last >> split_bit) << split_bit;
        mid = (int)(std::lower_bound(keys.begin() + begin, keys.begin() + end, pivot) -
                    keys.begin());
      }
      node.left = build_range(begin, mid, leaf_size);
      node.right = build_range(mid, end, leaf_size);
    }
    nodes[(std::size_t)self] = node;
    return self;
  }
};

}  // namespace mhgp3v
