// Pendaison H^r_m a marge en rayon (tranche S9) : port explicite, decision par decision, de hang_margin_radius
// (bench/points_radius.py), qualified_starts et first_points (bench/points_hierarchy.py:249-312) :
//   depart qualifie d'une incidence (v, r) : (v, max(r, qual v)) si v est qualifie, sinon (u, qual u) pour le premier
//     ancetre strict qualifie u (aucun : points_invariant, « jamais_qualifie ») ;
//   t = plus petit rang de depart, p1 = noeud de la PREMIERE incidence (ordre (rang, noeud)) de depart de rang t ;
//   rival : depart (q, Q) hors de la remontee de p1, w = lca(p1, q), M = rang de w, effectif si M > Q ; on garde le
//     premier rival, dans l'ordre des incidences, de sqrt(l_M) - sqrt(l_Q) strictement maximal (elagage par dominance
//     des rangs, puis deux racines contre deux) ;
//   sans rival : date (t, 0, 0), proprietaire p1, plancher t, non strict ; avec rival : date (t, M, Q), proprietaire =
//     plus haut ancetre de p1 de rayon de naissance <= date (coupe fermee), plancher = plus grand rang de niveau
//     <= date au carre (proposition binary64, puis certificat exact P(r) et non P(r + 1)), strict si la date depasse.
// Chaque site est traite par un fil, a positions fixes ; les comptes sont sommes apres la jointure.
#include "points/internal.hpp"

namespace mhgp11::points {

Outcome HangBuilder::start(u64 packed, u32& node, u32& rank) const noexcept {
  const u32 v = points_detail::node_of(packed), r = points_detail::rank_of(packed);
  if (qual[v] != kNone) {
    node = v;
    rank = std::max(r, qual[v]);
    return {};
  }
  const u32 u = above[v];
  if (u == kNone) return fail(Reason::points_invariant);  // jamais qualifie
  node = u;
  rank = qual[u];
  return {};
}

// Index d'ancetres, qualification, premier ancetre strict qualifie de chaque noeud.
Outcome HangBuilder::prepare() noexcept {
  Result<AncestorIndex> index = AncestorIndex::build(tree.forest(), budget);
  if (!index.ok()) return index.outcome().reason == Reason::tower_invariant ? fail(Reason::points_invariant)
                                                                           : index.outcome();
  ancestors.emplace(std::move(index).take());
  const Stopwatch clock;
  MHGP11_TRY(points_detail::qualify(tree, inc, m, budget, pool, qual, out.stats_.qualified_nodes));
  MHGP11_TRY(budget.admit(4 * u64{nodes.size()}));
  MHGP11_TRY(above.allocate(nodes.size(), budget));
  for (u64 j = nodes.size(); j-- > 0;) {
    const NodeIdx up = nodes[j].parent;
    above[j] = up == NodeIdx{kNone} ? kNone : (qual[idx(up)] != kNone ? idx(up) : above[idx(up)]);
  }
  out.stats_.qualify_ns = clock.nanoseconds();
  return {};
}

// Table des racines : rangs des noeuds, rang 0 et rangs de depart de toutes les incidences (t, Q, M en sont tires).
Outcome HangBuilder::fill_roots() noexcept {
  const Stopwatch clock;
  Result<num::RootTable> made = num::RootTable::allocate(levels, budget);
  if (!made.ok()) return made.outcome();
  table = std::move(made).take();
  Buffer<u8> wanted;
  MHGP11_TRY(budget.admit(levels.size()));
  MHGP11_TRY(wanted.allocate(levels.size(), budget));
  std::fill(wanted.begin(), wanted.end(), u8{0});
  wanted[0] = 1;
  for (const ForestNode& node : nodes) wanted[idx(node.rank)] = 1;
  for (const u64 packed : inc.packed) {
    u32 v = 0, r = 0;
    MHGP11_TRY(start(packed, v, r));
    wanted[r] = 1;
  }
  u64 count = 0;
  for (const u8 flag : wanted) count += flag;
  Buffer<u32> ranks;
  MHGP11_TRY(budget.admit(4 * count));
  MHGP11_TRY(ranks.allocate(count, budget));
  count = 0;
  for (u64 r = 0; r < wanted.size(); ++r)
    if (wanted[r] != 0) ranks[count++] = static_cast<u32>(r);
  wanted.reset();
  struct Fill {
    num::RootTable* table;
    std::span<const u32> ranks;
    static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
      Fill& f = *static_cast<Fill*>(context);
      return f.table->fill_ranks(f.ranks.subspan(begin, end - begin));
    }
  } fill{&table, ranks.span()};
  MHGP11_TRY(points_detail::run(pool, count, &fill, &Fill::body));
  out.stats_.roots = count;
  out.stats_.roots_ns = clock.nanoseconds();
  roots.emplace(table, levels);
  return {};
}

// Rival maximal du site s (port de la boucle `candidate` de hang_margin_radius) ; big_m = q = 0 sans rival.
Outcome HangBuilder::rival(u32 s, u32 worker, u32 p1, u32& big_m, u32& q) noexcept {
  const u64 begin = inc.offsets[s], count = inc.offsets[s + 1] - begin;
  u64* starts = scratch[worker].data();
  u64* meets = starts + count;  // (noeud de depart << 32 | lca), noeuds distincts tries
  for (u64 j = 0; j < count; ++j) meets[j] = u64{points_detail::node_of(starts[j])} << 32;
  std::sort(meets, meets + count);
  u64 distinct = 0;
  for (u64 j = 0; j < count; ++j)
    if (j == 0 || (meets[j] >> 32) != (meets[distinct - 1] >> 32)) meets[distinct++] = meets[j];
  for (u64 j = 0; j < distinct; ++j)
    meets[j] |= idx(ancestors->lca(NodeIdx{p1}, NodeIdx{static_cast<u32>(meets[j] >> 32)}));
  PointsStats& c = counts[worker];
  bool has = false;
  for (u64 j = 0; j < count; ++j) {
    const u32 v = points_detail::node_of(starts[j]), r = points_detail::rank_of(starts[j]);
    const u64* hit = std::lower_bound(meets, meets + distinct, u64{v} << 32);
    const u32 w = static_cast<u32>(*hit);
    if (w == v) continue;  // depart sur la remontee de p1
    const u32 meet = idx(nodes[w].rank);
    if (meet <= r) continue;  // terme nul ou negatif : pas un rival effectif
    ++c.rivals;
    if (!has || (meet >= big_m && r <= q)) {  // premier rival, ou dominant (strictement : rangs distincts)
      has = true;
      big_m = meet;
      q = r;
      continue;
    }
    if (meet <= big_m && r >= q) {  // domine : jamais strictement plus grand
      ++c.dominated;
      continue;
    }
    int sign = 0;  // sqrt(l_meet) - sqrt(l_r) > sqrt(l_M) - sqrt(l_Q) ?
    MHGP11_TRY(roots->two_vs_two(meet, q, big_m, r, sign, tallies[worker]));
    if (sign > 0) {
      big_m = meet;
      q = r;
    }
  }
  if (!has) big_m = q = 0;
  return {};
}

Outcome HangBuilder::site(u32 s, u32 worker) noexcept {
  const u64 begin = inc.offsets[s], count = inc.offsets[s + 1] - begin;
  u64* starts = scratch[worker].data();
  u32 t = kNone, p1 = kNone;
  for (u64 j = 0; j < count; ++j) {
    u32 v = 0, r = 0;
    MHGP11_TRY(start(inc.packed[begin + j], v, r));
    starts[j] = points_detail::pack(r, v);
    if (r < t) {  // premiere incidence de depart de rang minimal
      t = r;
      p1 = v;
    }
  }
  if (p1 == kNone) return fail(Reason::points_invariant);
  out.t_[s] = t;
  u32 big_m = 0, q = 0;
  MHGP11_TRY(rival(s, worker, p1, big_m, q));
  return settle(s, worker, p1, big_m, q);
}

}  // namespace mhgp11::points
