// Etage V : verticales de l'ordre k vers l'ordre k - 1 (contrat, paragraphe 5 ; CONCEPTION_TOUR.md de la v11,
// paragraphe 5 et annexes A.5, A.6). Pour chaque noeud v, le noeud de l'ordre k - 1 vivant a la coupe fermee du niveau
// de v qui contient ses representants.
//   Naissance (LEM-T6), en O(1) : la boule b d'une naissance de l'ordre k est, a l'ordre k - 1, une naissance (image :
//   son noeud de naissance) ou une cellule de fenetre (une naissance a k exige k - p >= q_min) : w = noeud du sommet
//   laisse par la cellule ; image = parent de w si ce parent a le rang de b, w sinon (une seule remontee).
//   Fusion (LEM-T5) : image = component_at(l, rang(v)) a l'ordre k - 1, ou l est la plus petite naissance sous l'image
//   de la plus petite naissance de v ; rang(l) <= rang(v) par construction (CST-0105).
// Les naissances de l'ordre k et les cellules et naissances de l'ordre k - 1 sont rangees par boule : jointure en flux.
#include <algorithm>

#include "tower/forest_internal.hpp"

namespace mhgp12::tower {

Result<u32> component_at(const OrderForest& forest, u32 leaf, u32 rank, u64* hops, u64* probes) noexcept {
  if (leaf >= forest.births || forest.attach_parent.size() != forest.births) return fail(Reason::tower_invariant);
  if (forest.rank[leaf] > rank) return fail(Reason::tower_query_domain);  // hypothese rang(l) <= r de LEM-T5
  u32 x = leaf;
  u64 steps = 0;
  while (forest.attach_parent[x] != kNone && forest.attach_rank[x] <= rank) {
    x = forest.attach_parent[x];
    ++steps;
  }
  // Dernier evenement de survivant x de rang au plus r : dichotomie dans sa liste (ordre de traitement).
  const u64 begin = forest.survivor_events.off[x], end = forest.survivor_events.off[u64{x} + 1];
  u64 lo = begin, hi = end, looks = 0;
  while (lo < hi) {
    const u64 mid = lo + (hi - lo) / 2;
    ++looks;
    if (forest.event_rank[forest.survivor_events.val[mid]] <= rank) lo = mid + 1;
    else hi = mid;
  }
  if (hops != nullptr) *hops = steps;
  if (probes != nullptr) *probes = looks;
  return lo == begin ? x : forest.event_node[forest.survivor_events.val[lo - 1]];
}

namespace detail {
namespace {

// Premiere position dont la cle n'est pas inferieure a key (cles croissantes).
u64 lower_index(std::span<const u32> keys, u32 key) noexcept {
  return static_cast<u64>(std::lower_bound(keys.begin(), keys.end(), key) - keys.begin());
}
u64 lower_index(std::span<const BallIdx> balls, u32 key) noexcept {
  return static_cast<u64>(std::lower_bound(balls.begin(), balls.end(), make_id<BallIdx>(key)) - balls.begin());
}

}  // namespace

Outcome birth_images(const ForestInput& input, const ForestInput& below_input, const OrderForest& below,
                     OrderForest& forest, u64 begin, u64 end, ForestWork& counters) noexcept {
  // A k - 1 = 1 les naissances sont des sites : seules les cellules de l'ordre 1 portent des boules.
  const bool below_balls = below_input.k >= 2;
  if (begin >= end) return {};
  u64 bi = below_balls ? lower_index(below_input.birth_key, input.birth_key[begin]) : below_input.birth_key.size();
  u64 ci = lower_index(below_input.cell_ball, input.birth_key[begin]);
  const u64 nbb = below_input.birth_key.size(), ncb = below_input.cell_ball.size();
  for (u64 i = begin; i < end; ++i) {
    const u32 ball = input.birth_key[i], r = idx(input.birth_rank[i]);
    while (below_balls && bi < nbb && below_input.birth_key[bi] < ball) ++bi;
    while (ci < ncb && idx(below_input.cell_ball[ci]) < ball) ++ci;
    u32 image = kNone;
    if (below_balls && bi < nbb && below_input.birth_key[bi] == ball) {
      image = below.birth_node[bi];
      ++counters.t6_from_birth;
    } else if (ci < ncb && idx(below_input.cell_ball[ci]) == ball) {
      image = below.cell_node[ci];
      const u32 up = below.parent[image];
      if (up != kNone && below.rank[up] == r) {
        image = up;  // remontee d'un cran : le plateau du rang de la boule contient la jonction
        ++counters.t6_climbs;
      }
      ++counters.t6_from_cell;
    } else {
      return fail(Reason::tower_invariant);  // boule absente de l'ordre k - 1 : hors du domaine de LEM-T6
    }
    if (below.rank[image] > r) return fail(Reason::tower_invariant);
    forest.lower[forest.birth_node[i]] = image;
  }
  return {};
}

Outcome merge_images(const OrderForest& below, OrderForest& forest, u64 begin, u64 end,
                     ForestWork& counters) noexcept {
  for (u64 v = begin; v < end; ++v) {
    const u32 smallest = forest.minleaf[v];
    if (smallest >= forest.births) return fail(Reason::tower_invariant);
    const u32 under = forest.lower[smallest];
    if (under >= below.nodes()) return fail(Reason::tower_invariant);
    u64 hops = 0, probes = 0;
    auto image = component_at(below, below.minleaf[under], forest.rank[v], &hops, &probes);
    if (!image.ok()) return image.outcome();
    forest.lower[v] = image.value();
    ++counters.t5_queries;
    counters.t5_hops += hops;
    counters.t5_probes += probes;
    counters.t5_max_hops = std::max(counters.t5_max_hops, hops);
  }
  return {};
}

}  // namespace detail
}  // namespace mhgp12::tower
