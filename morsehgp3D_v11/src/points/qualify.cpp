// Qualification des noeuds de la foret d'ordre K (tranche S9) : port explicite de qualify et qualify_next,
// bench/points_hierarchy.py:215-246. m <= K : chaque noeud est qualifie a sa naissance (il couvre deja K sites). m = K + 1
// : toute fusion l'est a sa naissance (deux composantes distinctes ne couvrent jamais les memes K sites) ; une naissance
// l'est au rang de sa m-ieme paire (noeud, site) distincte, chaque paire prise a sa premiere incidence (rang minimal).
// Paires par site en deux passes paralleles a positions fixes (dedoublonnage par tri d'une copie de la ligne dans un
// brouillon par fil), puis tri par denombrement sur les noeuds (sequentiel) et m-ieme rang par noeud (parallele).
#include "points/internal.hpp"

namespace mhgp11::points_detail {

namespace {

// Paires (naissance, premier rang) de chaque site : passe count (fill faux) puis passe fill.
struct Pairs {
  const Incidences* inc = nullptr;
  u32 births = 0;
  bool fill = false;
  u64 widest = 0;
  Buffer<u64>* scratch = nullptr;  // un brouillon de `widest` mots par fil
  Buffer<u64> offsets;             // n + 1 : comptes (passe count), puis decalages
  Buffer<u32> node, rank;          // P

  // Noeuds distincts de la ligne du site s (copie triee par (noeud, rang)), naissances seules ; rend leur nombre.
  u64 distinct(u64 s, u64* copy) const noexcept {
    const u64 begin = inc->offsets[s], end = inc->offsets[s + 1];
    for (u64 j = begin; j < end; ++j) copy[j - begin] = (u64{node_of(inc->packed[j])} << 32) | rank_of(inc->packed[j]);
    std::sort(copy, copy + (end - begin));
    u64 out = 0;
    for (u64 j = 0; j < end - begin; ++j) {
      const u32 v = static_cast<u32>(copy[j] >> 32);
      if (v >= births || (j > 0 && static_cast<u32>(copy[j - 1] >> 32) == v)) continue;
      copy[out++] = copy[j];
    }
    return out;
  }
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    Pairs& p = *static_cast<Pairs*>(context);
    u64* copy = p.scratch[worker].data();
    for (u64 s = begin; s < end; ++s) {
      const u64 count = p.distinct(s, copy);
      if (!p.fill) {
        p.offsets[s + 1] = count;
        continue;
      }
      for (u64 j = 0; j < count; ++j) {
        p.node[p.offsets[s] + j] = static_cast<u32>(copy[j] >> 32);
        p.rank[p.offsets[s] + j] = static_cast<u32>(copy[j]);
      }
    }
    return {};
  }
};

// m-ieme plus petit rang des paires de chaque naissance (kNone si elle en a moins de m).
struct Select {
  const Buffer<u64>* offsets = nullptr;
  Buffer<u32>* ranks = nullptr;
  Buffer<u32>* qual = nullptr;
  u32 m = 0;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    Select& s = *static_cast<Select*>(context);
    for (u64 v = begin; v < end; ++v) {
      const u64 lo = (*s.offsets)[v], hi = (*s.offsets)[v + 1];
      if (hi - lo < s.m) {
        (*s.qual)[v] = kNone;
        continue;
      }
      u32* first = s.ranks->data() + lo;
      std::nth_element(first, first + (s.m - 1), s.ranks->data() + hi);
      (*s.qual)[v] = first[s.m - 1];
    }
    return {};
  }
};

// Paires de chaque site, puis regroupees par naissance (decalages u64 par noeud, rangs).
Outcome by_birth(const Incidences& inc, u32 births, u32 n, MemoryBudget& budget, sched::Pool* pool,
                 Buffer<u64>& node_offsets, Buffer<u32>& ranks) noexcept {
  const u32 w = workers(pool);
  Pairs pairs;
  pairs.inc = &inc;
  pairs.births = births;
  pairs.widest = inc.widest;
  std::array<Buffer<u64>, sched::kMaxWorkers> scratch;
  MHGP11_TRY(budget.admit(8 * (u64{n} + 1) + 8 * u64{w} * std::max<u64>(inc.widest, 1)));
  MHGP11_TRY(pairs.offsets.allocate(u64{n} + 1, budget));
  for (u32 i = 0; i < w; ++i) MHGP11_TRY(scratch[i].allocate(std::max<u64>(inc.widest, 1), budget));
  pairs.scratch = scratch.data();
  pairs.offsets[0] = 0;
  MHGP11_TRY(run(pool, n, &pairs, &Pairs::body));
  for (u32 s = 0; s < n; ++s) pairs.offsets[s + 1] += pairs.offsets[s];
  const u64 total = pairs.offsets[n];
  MHGP11_TRY(budget.admit(2 * 4 * total + 8 * (u64{births} + 1) + 4 * total));
  MHGP11_TRY(pairs.node.allocate(total, budget));
  MHGP11_TRY(pairs.rank.allocate(total, budget));
  pairs.fill = true;
  MHGP11_TRY(run(pool, n, &pairs, &Pairs::body));
  for (u32 i = 0; i < w; ++i) scratch[i].reset();
  MHGP11_TRY(node_offsets.allocate(u64{births} + 1, budget));
  MHGP11_TRY(ranks.allocate(total, budget));
  std::fill(node_offsets.begin(), node_offsets.end(), u64{0});
  for (u64 j = 0; j < total; ++j) node_offsets[pairs.node[j] + 1] += 1;
  for (u32 v = 0; v < births; ++v) node_offsets[v + 1] += node_offsets[v];
  for (u64 j = 0; j < total; ++j) ranks[node_offsets[pairs.node[j]]++] = pairs.rank[j];
  for (u32 v = births; v-- > 0;) node_offsets[v + 1] = node_offsets[v];
  node_offsets[0] = 0;
  return {};
}

}  // namespace

Outcome qualify(const OrderTree& tree, const Incidences& incidences, u32 m, MemoryBudget& budget, sched::Pool* pool,
                Buffer<u32>& qual, u64& qualified) noexcept {
  const OrderForest& forest = tree.forest();
  const auto nodes = forest.nodes();
  const Order k = forest.order();
  const u32 n = tree.domain().index().cloud().sites();
  if (m < 1 || m > u32{k} + 1) return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(budget.admit(4 * u64{nodes.size()}));
  MHGP11_TRY(qual.allocate(nodes.size(), budget));
  for (u64 v = 0; v < nodes.size(); ++v) qual[v] = idx(nodes[v].rank);
  qualified = nodes.size();
  if (m <= k) return {};
  // m = K + 1 : fusions qualifiees a leur naissance (rang deja ecrit), naissances par leurs paires.
  const u32 births = forest.births();
  Buffer<u64> node_offsets;
  Buffer<u32> ranks;
  MHGP11_TRY(by_birth(incidences, births, n, budget, pool, node_offsets, ranks));
  Select select{&node_offsets, &ranks, &qual, m};
  MHGP11_TRY(run(pool, births, &select, &Select::body));
  for (u32 v = 0; v < births; ++v) {
    if (qual[v] == kNone) {
      --qualified;
    } else if (qual[v] < idx(nodes[v].rank)) {
      return fail(Reason::points_invariant);  // une incidence precede la naissance de son noeud
    }
  }
  return {};
}

}  // namespace mhgp11::points_detail
