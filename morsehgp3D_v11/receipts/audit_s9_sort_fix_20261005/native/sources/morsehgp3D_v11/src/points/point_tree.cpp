// Arbre de points a plateaux atomiques (tranche S9) : port explicite de tower_point_tree (bench/points_flat.py:462-539)
// et de PointTree.finish (:352-403). Entrees triees par (rang plancher, strict, SiteIdx), puis, a plancher egal et
// strictes, par date exacte (tri stable de sort_strict_groups, bench/points_hierarchy.py : egalites dans l'ordre des
// sites). Balayage : au rang r, un plateau (r, 0, 0) ; fusions de la foret de rang r (par numero croissant, donc par
// (rang, numero)) : un bloc nouveau si au moins deux enfants portent un bloc, sinon le bloc unique remonte ; entrees de
// plancher r non strictes ; puis un plateau par date exacte des entrees strictes de plancher r. Un site entre dans le
// bloc de son proprietaire, cree a la premiere entree. Les plateaux sans evenement sont retires (finish).
#include "points/internal.hpp"

namespace mhgp11::points {

namespace {

struct Sweep {
  Sweep(const PointTreeBuilder::Input& i, num::RadicalSum& s, points_detail::RootTally& t) noexcept
      : in(i), sum(s), tally(t) {}
  const PointTreeBuilder::Input& in;
  num::RadicalSum& sum;
  points_detail::RootTally& tally;
  Buffer<u32> plateau_t, plateau_m, plateau_q, block_plateau, block_parent, block_of;
  u32 plateaus = 0, blocks = 0;
  Buffer<u32>* site_block = nullptr;
  Buffer<u32>* site_plateau = nullptr;

  u32 add_plateau(u32 t, u32 m, u32 q) noexcept {
    plateau_t[plateaus] = t;
    plateau_m[plateaus] = m;
    plateau_q[plateaus] = q;
    return plateaus++;
  }
  u32 add_block(u32 plateau) noexcept {
    block_plateau[blocks] = plateau;
    block_parent[blocks] = kNone;
    return blocks++;
  }
  void enter(u32 site, u32 plateau) noexcept {
    const u32 o = in.owner[site];
    if (block_of[o] == kNone) block_of[o] = add_block(plateau);
    (*site_block)[site] = block_of[o];
    (*site_plateau)[site] = plateau;
  }
  // Fusion du noeud v : blocs de ses enfants.
  void merge(u32 v, u32 plateau) noexcept {
    u32 parts = 0, single = kNone, made = kNone;
    for (const NodeIdx child : in.forest->children(NodeIdx{v})) {
      const u32 b = block_of[idx(child)];
      block_of[idx(child)] = kNone;
      if (b == kNone) continue;
      if (++parts == 1) {
        single = b;
        continue;
      }
      if (made == kNone) {
        made = add_block(plateau);
        block_parent[single] = made;
      }
      block_parent[b] = made;
    }
    block_of[v] = parts >= 2 ? made : single;
  }
  Outcome compare(u32 a, u32 b, int& order) noexcept {
    return in.roots->compare_dates({in.t[a], in.m[a], in.q[a]}, {in.t[b], in.m[b], in.q[b]}, sum, order, tally);
  }
};

// Ordre strict d'un groupe : date exacte croissante, egalites exactes departagees par SiteIdx. Un refus de la
// comparaison exacte est rendu tel quel : aucune reponse de substitution (audit d448b3d03).
Outcome date_less(Sweep& s, u32 a, u32 b, bool& less) noexcept {
  int sign = 0;
  MHGP11_TRY(s.compare(a, b, sign));
  less = sign < 0 || (sign == 0 && a < b);
  return {};
}

// Ordre des entrees : (plancher, strict, SiteIdx), puis dates exactes croissantes dans chaque groupe strict.
Outcome entry_order(Sweep& s, Buffer<u32>& order) noexcept {
  const PointTreeBuilder::Input& in = s.in;
  for (u32 i = 0; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
    return in.floor[a] != in.floor[b] ? in.floor[a] < in.floor[b]
                                      : (in.strict[a] != in.strict[b] ? in.strict[a] < in.strict[b] : a < b);
  });
  for (u64 j = 0; j < order.size();) {
    u64 e = j + 1;
    while (e < order.size() && in.floor[order[e]] == in.floor[order[j]] && in.strict[order[e]] == in.strict[order[j]])
      ++e;
    if (in.strict[order[j]] != 0 && e - j > 1)
      MHGP11_TRY(points_detail::heap_sort_until_refusal(
          &order[j], e - j, [&](u32 a, u32 b, bool& less) { return date_less(s, a, b, less); }));
    j = e;
  }
  return {};
}

// finish : plateaux sans evenement retires et renumerotes dans l'ordre ; formes controlees ; colonnes publiees.
// columns : plateau_t, plateau_m, plateau_q, block_plateau, block_parent de l'arbre publie.
Outcome finish(Sweep& s, u64 n, MemoryBudget& budget, const std::array<Buffer<u32>*, 5>& columns) noexcept {
  Buffer<u32>& site_plateau = *s.site_plateau;
  Buffer<u32> remap;
  MHGP11_TRY(remap.allocate(s.plateaus, budget));
  std::fill(remap.begin(), remap.end(), kNone);
  for (u32 b = 0; b < s.blocks; ++b) remap[s.block_plateau[b]] = 0;
  for (u64 i = 0; i < n; ++i) {
    if (site_plateau[i] >= s.plateaus) return fail(Reason::points_invariant);  // site jamais entre
    remap[site_plateau[i]] = 0;
  }
  u32 kept = 0;
  for (u32 p = 0; p < s.plateaus; ++p) {
    if (remap[p] == kNone) continue;
    remap[p] = kept;
    s.plateau_t[kept] = s.plateau_t[p];
    s.plateau_m[kept] = s.plateau_m[p];
    s.plateau_q[kept] = s.plateau_q[p];
    ++kept;
  }
  for (u64 i = 0; i < n; ++i) site_plateau[i] = remap[site_plateau[i]];
  for (u32 b = 0; b < s.blocks; ++b) s.block_plateau[b] = remap[s.block_plateau[b]];
  for (u32 b = 0; b < s.blocks; ++b) {
    const u32 up = s.block_parent[b];
    if (up != kNone && (up <= b || s.block_plateau[up] <= s.block_plateau[b])) return fail(Reason::points_invariant);
  }
  const std::array<const Buffer<u32>*, 5> from = {&s.plateau_t, &s.plateau_m, &s.plateau_q, &s.block_plateau,
                                                 &s.block_parent};
  for (u32 c = 0; c < 5; ++c) {
    const u32 size = c < 3 ? kept : s.blocks;
    MHGP11_TRY(columns[c]->allocate(size, budget));
    std::copy_n(from[c]->begin(), size, columns[c]->begin());
  }
  return {};
}

// Balayage des rangs : fusions de rang r, entrees non strictes de plancher r, puis un plateau par date stricte.
Outcome sweep(Sweep& s, const Buffer<u32>& order) noexcept {
  const PointTreeBuilder::Input& in = s.in;
  const auto nodes = in.forest->nodes();
  const u64 n = order.size(), count = nodes.size(), births = in.forest->births();
  for (u64 v = births + 1; v < count; ++v)
    if (idx(nodes[v - 1].rank) > idx(nodes[v].rank)) return fail(Reason::points_invariant);  // fusions par rang
  u64 mi = births, ei = 0;
  while (mi < count || ei < n) {
    const u32 r = std::min(mi < count ? idx(nodes[mi].rank) : kNone, ei < n ? in.floor[order[ei]] : kNone);
    const u32 p = s.add_plateau(r, 0, 0);
    for (; mi < count && idx(nodes[mi].rank) == r; ++mi) s.merge(static_cast<u32>(mi), p);
    for (; ei < n && in.floor[order[ei]] == r && in.strict[order[ei]] == 0; ++ei) s.enter(order[ei], p);
    while (ei < n && in.floor[order[ei]] == r) {
      const u32 first = order[ei];
      const u32 q = s.add_plateau(in.t[first], in.m[first], in.q[first]);
      for (; ei < n && in.floor[order[ei]] == r; ++ei) {
        int sign = 0;
        if (order[ei] != first) MHGP11_TRY(s.compare(order[ei], first, sign));
        if (sign != 0) break;
        s.enter(order[ei], q);
      }
    }
  }
  return {};
}

}  // namespace

Outcome PointTreeBuilder::build(const Input& in, num::RadicalSum& sum, MemoryBudget& budget, PointTree& out,
                                points_detail::RootTally& tally) noexcept {
  const u64 n = in.t.size(), count = in.forest->nodes().size(), births = in.forest->births();
  // Plateaux : au plus un par rang de fusion ou de plancher, plus un par date stricte ; blocs : au plus 2n - 1.
  const u64 most_plateaus = (count - births) + 2 * n, most_blocks = 2 * n;
  Sweep s(in, sum, tally);
  s.site_block = &out.site_block_;
  s.site_plateau = &out.site_plateau_;
  MHGP11_TRY(budget.admit(2 * (3 * 4 * most_plateaus + 2 * 4 * most_blocks) + 4 * count + 3 * 4 * n +
                          4 * most_plateaus));
  for (Buffer<u32>* column : {&s.plateau_t, &s.plateau_m, &s.plateau_q})
    MHGP11_TRY(column->allocate(most_plateaus, budget));
  MHGP11_TRY(s.block_plateau.allocate(most_blocks, budget));
  MHGP11_TRY(s.block_parent.allocate(most_blocks, budget));
  MHGP11_TRY(s.block_of.allocate(count, budget));
  std::fill(s.block_of.begin(), s.block_of.end(), kNone);
  MHGP11_TRY(out.site_block_.allocate(n, budget));
  MHGP11_TRY(out.site_plateau_.allocate(n, budget));
  std::fill(out.site_plateau_.begin(), out.site_plateau_.end(), kNone);
  Buffer<u32> order;
  MHGP11_TRY(order.allocate(n, budget));
  MHGP11_TRY(entry_order(s, order));
  MHGP11_TRY(sweep(s, order));
  order.reset();
  s.block_of.reset();
  return finish(s, n, budget, {&out.plateau_t_, &out.plateau_m_, &out.plateau_q_, &out.block_plateau_,
                               &out.block_parent_});
}

}  // namespace mhgp11::points
