// Etage M, contraction des plateaux (LEM-T4 = lemme P de MATHEMATIQUES.md de la v11, paragraphe 10.3 ; theoreme T4 de
// CONCEPTION_TOUR.md, annexe A.4) : deux evenements de meme rang sont LIES quand l'un a pour operande le sommet produit
// par l'autre ; les classes de la cloture sont exactement les multifusions N-aires du rang, leurs enfants les operandes
// hors de la classe (feuilles ou sommets de rang strictement inferieur). Numerotation des fusions par (rang, plus
// petite naissance), unique : deux noeuds de meme rang ont des sous-arbres disjoints. Port de contract_parallel de
// MES-M4, en quatre phases par tranche d'evenements alignee sur les rangs (une tranche ne coupe jamais un rang, donc
// jamais une classe) : ecritures disjointes par tranche, aucun atomique ; les sommes prefixes entre phases sont faites
// par l'orchestration (forest_build.cpp). Une seule tranche donne la voie sequentielle ; la sortie ne depend pas du
// decoupage.
#include <algorithm>

#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {
namespace {

inline u32 find_local(Buffer<u32>& local, u32 base, u32 x) noexcept {
  while (local[base + x] != x) {
    local[base + x] = local[base + local[base + x]];
    x = local[base + x];
  }
  return x;
}

// Operande lie : evenement de la meme tranche et de meme rang (lien choisi du theoreme T4).
inline bool linked(const Event* ev, u32 e, u32 op, u32 lo) noexcept {
  return (op & kEventBit) && (op & kEventIndex) >= lo && ev[op & kEventIndex].rank == ev[e].rank;
}

}  // namespace

void contract_classes(OrderWork& work, Slice& slice) noexcept {
  const Event* ev = work.events.data();
  const u32 lo = slice.lo, hi = slice.hi;
  for (u32 e = lo; e < hi; ++e) work.local[e] = e - lo;
  for (u32 e = lo; e < hi; ++e)
    for (const u32 op : {ev[e].a, ev[e].b})
      if (linked(ev, e, op, lo)) {
        // La racine d'une classe est son dernier evenement : on rattache toujours a la classe de e, posterieur.
        const u32 x = find_local(work.local, lo, (op & kEventIndex) - lo), y = find_local(work.local, lo, e - lo);
        if (x != y) work.local[lo + x] = y;
      }
  for (u32 e = lo; e < hi; ++e) work.class_min[e] = kNone;
  for (u32 e = lo; e < hi; ++e) {
    const u32 r = find_local(work.local, lo, e - lo);
    work.class_min[lo + r] = std::min(work.class_min[lo + r], ev[e].minleaf);
  }
  // Cles (rang de la racine, plus petite naissance) : rangs croissants dans la tranche, donc le tri par
  // (plus petite naissance, racine) a l'interieur de chaque rang suffit ; la tranche range ses cles a [lo, lo + n).
  u32 n = 0;
  for (u32 g0 = lo; g0 < hi;) {
    u32 g1 = g0 + 1;
    while (g1 < hi && ev[g1].rank == ev[g0].rank) ++g1;
    const u32 first = n;
    for (u32 e = g0; e < g1; ++e)
      if (work.local[e] == e - lo) work.keys[lo + n++] = (u64{work.class_min[e]} << 32) | (e - lo);
    std::sort(work.keys.data() + lo + first, work.keys.data() + lo + n);
    g0 = g1;
  }
  slice.classes = n;
}

void contract_nodes(const OrderWork& work, OrderForest& forest, const Slice& slice) noexcept {
  const Event* ev = work.events.data();
  const u32 lo = slice.lo;
  // Chaque classe recoit son noeud ; class_min de sa racine devient l'identifiant (la plus petite naissance est gardee
  // dans minleaf). Lecture seule de local : les racines sont deja comprimees par la phase A.
  for (u32 j = 0; j < slice.classes; ++j) {
    const u64 key = work.keys[lo + j];
    const u32 root = static_cast<u32>(key & 0xFFFFFFFFu), node = slice.node0 + j;
    forest.rank[node] = ev[lo + root].rank;
    forest.minleaf[node] = static_cast<u32>(key >> 32);
    forest.event_node[lo + root] = node;  // provisoire : noeud de la racine, recopie a ses evenements ci-dessous
  }
  for (u32 e = slice.lo; e < slice.hi; ++e) {
    u32 x = e - lo;
    while (work.local[lo + x] != x) x = work.local[lo + x];
    forest.event_node[e] = forest.event_node[lo + x];
    forest.event_rank[e] = ev[e].rank;
  }
}

void contract_parents(OrderWork& work, OrderForest& forest, Slice& slice) noexcept {
  const Event* ev = work.events.data();
  u64 children = 0;
  for (u32 e = slice.lo; e < slice.hi; ++e) {
    const u32 id = forest.event_node[e];
    for (const u32 op : {ev[e].a, ev[e].b}) {
      const u32 child = node_of_top(op, forest.event_node.span());
      if (child == id) continue;  // operande interne a la classe
      forest.parent[child] = id;
      ++work.child_count[id];
      ++children;
    }
  }
  slice.children = children;
}

void contract_children(OrderWork& work, OrderForest& forest, const Slice& slice) noexcept {
  const Event* ev = work.events.data();
  const u32 n0 = slice.node0, n1 = slice.node0 + slice.classes;
  u64 at = slice.child0;
  for (u32 v = n0; v < n1; ++v) {
    forest.children.off[v] = at;
    at += work.child_count[v];
    work.child_count[v] = 0;  // curseur de remplissage
  }
  for (u32 e = slice.lo; e < slice.hi; ++e) {
    const u32 id = forest.event_node[e];
    for (const u32 op : {ev[e].a, ev[e].b}) {
      const u32 child = node_of_top(op, forest.event_node.span());
      if (child == id) continue;
      forest.children.val[forest.children.off[id] + work.child_count[id]++] = child;
    }
  }
  for (u32 v = n0; v < n1; ++v) {
    u32* first = forest.children.val.data() + forest.children.off[v];
    std::sort(first, first + work.child_count[v]);
  }
}

}  // namespace mhgp12::tower::detail
