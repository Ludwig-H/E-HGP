// Fin de la pendaison H^r_m (tranche S9) et appels publics : proprietaire, rang plancher et drapeau strict de chaque
// site (port de ancestor_at_radius et floor_rank_radius, bench/points_radius.py:184-206), rangs references, puis arbre
// de points. Ordre des etages : incidences, qualification, table des racines, pendaison par site (parallele), arbre de
// points (sequentiel). Tout tampon est admis avant son allocation ; les tampons de travail sont rendus avant le retour.
#include <cmath>

#include "points/internal.hpp"

namespace mhgp11::points {

// Signe de date(s) - sqrt(l_rank) : >= 0 si le rang est sous la date (coupe fermee).
Outcome HangBuilder::above_floor(u32 s, u32 worker, u32 rank, int& sign) noexcept {
  ++counts[worker].floor_steps;
  return roots->two_vs_two(out.t_[s], out.big_m_[s], rank, out.q_[s], sign, tallies[worker]);
}

// Plus grand rang r de [low, high] sous la date, P(low) vrai : proposition binary64 (recherche dichotomique sur les
// niveaux approches), puis galop et dichotomie exacts, puis certificat P(r) et non P(r + 1) sur tout le catalogue.
Outcome HangBuilder::floor_of(u32 s, u32 worker, u32 low, u32 high, u32& floor, int& sign) noexcept {
  if (low > high) return fail(Reason::points_invariant);
  const auto root = [&](u32 r) { return std::sqrt(points_detail::approximate(levels[r])); };
  const double date = root(out.t_[s]) + root(out.big_m_[s]) - root(out.q_[s]);
  u32 a = low, b = high;  // proposition : plus grand r de [low, high] de niveau approche <= date^2
  while (a < b) {
    const u32 mid = a + (b - a + 1) / 2;
    if (points_detail::approximate(levels[mid]) <= date * date) a = mid;
    else b = mid - 1;
  }
  u32 lo = low, hi = high;
  int lo_sign = 2, probe = 0;  // 2 : signe de lo inconnu
  MHGP11_TRY(above_floor(s, worker, a, probe));
  if (probe >= 0) {
    lo = a;
    lo_sign = probe;
    for (u32 step = 1; lo < hi; step = step < (1u << 30) ? 2 * step : step) {  // galop vers le haut
      const u32 c = hi - lo > step ? lo + step : hi;
      MHGP11_TRY(above_floor(s, worker, c, probe));
      if (probe < 0) {
        hi = c - 1;
        break;
      }
      lo = c;
      lo_sign = probe;
    }
  } else {
    if (a == low) return fail(Reason::points_invariant);  // P(low) faux
    hi = a - 1;
    for (u32 step = 1; lo < hi; step = step < (1u << 30) ? 2 * step : step) {  // galop vers le bas
      const u32 c = hi - lo > step ? hi - step : lo;
      MHGP11_TRY(above_floor(s, worker, c, probe));
      if (probe >= 0) {
        lo = c;
        lo_sign = probe;
        break;
      }
      hi = c - 1;
    }
  }
  while (lo < hi) {
    const u32 mid = lo + (hi - lo + 1) / 2;
    MHGP11_TRY(above_floor(s, worker, mid, probe));
    if (probe >= 0) {
      lo = mid;
      lo_sign = probe;
    } else {
      hi = mid - 1;
    }
  }
  if (lo_sign == 2) MHGP11_TRY(above_floor(s, worker, lo, lo_sign));
  if (lo_sign < 0) return fail(Reason::points_invariant);
  if (u64{lo} + 1 < levels.size()) {  // certificat : le rang suivant du catalogue depasse la date
    MHGP11_TRY(above_floor(s, worker, lo + 1, probe));
    if (probe >= 0) return fail(Reason::points_invariant);
  }
  floor = lo;
  sign = lo_sign;
  return {};
}

// Proprietaire, plancher et drapeau strict du site s, de date (t, M, Q) deja ecrite ; M = Q = 0 sans rival.
Outcome HangBuilder::settle(u32 s, u32 worker, u32 p1, u32 big_m, u32 q) noexcept {
  out.big_m_[s] = big_m;
  out.q_[s] = q;
  const u32 t = out.t_[s];
  u32 owner = p1, floor = t;
  int sign = 0;
  if (big_m != 0) {
    ++counts[worker].delayed;
    NodeIdx top{p1};
    const auto keep = [&](NodeIdx x, bool& ok) -> Outcome {
      int below = 0;
      MHGP11_TRY(above_floor(s, worker, idx(nodes[idx(x)].rank), below));
      ok = below >= 0;  // coupe fermee : rayon de naissance <= date
      return {};
    };
    MHGP11_TRY(ancestors->highest(NodeIdx{p1}, keep, top));
    owner = idx(top);
    const NodeIdx up = nodes[owner].parent;
    const u32 low = std::max(t, idx(nodes[owner].rank));
    const u32 cap = up == NodeIdx{kNone} ? static_cast<u32>(levels.size() - 1) : idx(nodes[idx(up)].rank) - 1;
    MHGP11_TRY(floor_of(s, worker, low, std::min(big_m, cap), floor, sign));
  }
  // Le proprietaire est vivant au plancher (bench/points_radius.py : proprietaire_non_vivant).
  const NodeIdx up = nodes[owner].parent;
  if (idx(nodes[owner].rank) > floor || (up != NodeIdx{kNone} && idx(nodes[idx(up)].rank) <= floor))
    return fail(Reason::points_invariant);
  if (t > floor || (big_m != 0 && (q < t || big_m <= q))) return fail(Reason::points_invariant);
  out.owner_[s] = owner;
  out.floor_[s] = floor;
  out.strict_[s] = sign > 0 ? 1 : 0;
  counts[worker].strict += sign > 0 ? 1 : 0;
  return {};
}

// Pendaison de tous les sites : sorties par site, brouillons par fil (2 x widest mots), comptes sommes.
Outcome HangBuilder::hang_sites() noexcept {
  const Stopwatch clock;
  const u32 w = points_detail::workers(pool);
  const u64 width = 2 * std::max<u64>(inc.widest, 1);
  MHGP11_TRY(budget.admit(5 * 4 * u64{n} + n + 8 * width * w));
  for (Buffer<u32>* column : {&out.t_, &out.big_m_, &out.q_, &out.owner_, &out.floor_})
    MHGP11_TRY(column->allocate(n, budget));
  MHGP11_TRY(out.strict_.allocate(n, budget));
  for (u32 i = 0; i < w; ++i) MHGP11_TRY(scratch[i].allocate(width, budget));
  MHGP11_TRY(points_detail::run(pool, n, this, &HangBuilder::site_body));
  for (u32 i = 0; i < w; ++i) scratch[i].reset();
  PointsStats& stats = out.stats_;
  for (u32 i = 0; i < w; ++i) {
    stats.delayed += counts[i].delayed;
    stats.strict += counts[i].strict;
    stats.rivals += counts[i].rivals;
    stats.dominated += counts[i].dominated;
    stats.floor_steps += counts[i].floor_steps;
    stats.table_decisions += tallies[i].table;
    stats.exact_decisions += tallies[i].exact;
  }
  stats.hang_ns = clock.nanoseconds();
  return {};
}

// Rangs references par les colonnes publiees (noeuds, t, M, Q, plancher, et 0 pour les plateaux), croissants.
Outcome HangBuilder::referenced() noexcept {
  Buffer<u8> used;
  MHGP11_TRY(budget.admit(levels.size()));
  MHGP11_TRY(used.allocate(levels.size(), budget));
  std::fill(used.begin(), used.end(), u8{0});
  used[0] = 1;  // M = Q = 0 : plateaux de niveau du catalogue et dates sans rival
  for (const ForestNode& node : nodes) used[idx(node.rank)] = 1;
  for (const Buffer<u32>* column : {&out.t_, &out.big_m_, &out.q_, &out.floor_})
    for (const u32 r : *column) used[r] = 1;
  u64 count = 0;
  for (const u8 flag : used) count += flag;
  MHGP11_TRY(budget.admit(4 * count));
  MHGP11_TRY(out.levels_.allocate(count, budget));
  count = 0;
  for (u64 r = 0; r < used.size(); ++r)
    if (used[r] != 0) out.levels_[count++] = static_cast<u32>(r);
  return {};
}

Result<PointHierarchy> HangBuilder::run(const OrderTree& tree, u32 m, MemoryBudget& budget, sched::Pool* pool) noexcept {
  const Order k = tree.order();
  const u32 n = tree.domain().index().cloud().sites();
  if (k >= 2 && k >= n) return fail(Reason::parameter_out_of_range);  // K = n : aucune composante de K + 1 sites
  if (m < 1 || m > u32{k} + 1 || tree.domain().catalogue().levels().empty()) return fail(Reason::parameter_out_of_range);
  HangBuilder h(tree, budget, pool);
  h.n = n;
  h.m = m;
  h.out.order_ = k;
  h.out.m_ = m;
  Stopwatch clock;
  MHGP11_TRY(points_detail::build_incidences(tree, budget, pool, h.inc));
  h.out.stats_.incidences = h.inc.packed.size();
  h.out.stats_.incidences_ns = clock.nanoseconds();
  MHGP11_TRY(h.prepare());
  MHGP11_TRY(h.fill_roots());
  MHGP11_TRY(h.hang_sites());
  h.inc = {};
  h.qual.reset();
  h.above.reset();
  MHGP11_TRY(h.referenced());
  const Stopwatch tree_clock;
  Result<num::RadicalSum> sum = num::RadicalSum::make(budget);
  if (!sum.ok()) return sum.outcome();
  points_detail::RootTally tally;
  const PointTreeBuilder::Input input{&tree.forest(), h.out.t_.span(), h.out.big_m_.span(), h.out.q_.span(),
                                      h.out.owner_.span(), h.out.floor_.span(), h.out.strict_.span(), &*h.roots};
  MHGP11_TRY(PointTreeBuilder::build(input, sum.value(), budget, h.out.tree_, tally));
  h.out.stats_.table_decisions += tally.table;
  h.out.stats_.exact_decisions += tally.exact;
  h.out.stats_.tree_ns = tree_clock.nanoseconds();
  return std::move(h.out);
}

Result<PointHierarchy> hang(const OrderTree& tree, MemoryBudget& budget, sched::Pool* pool) noexcept {
  return guarded([&]() { return HangBuilder::run(tree, qualification(tree.order()), budget, pool); });
}

Result<PointHierarchy> hang_qualified(const OrderTree& tree, u32 m, MemoryBudget& budget, sched::Pool* pool) noexcept {
  return guarded([&]() { return HangBuilder::run(tree, m, budget, pool); });
}

}  // namespace mhgp11::points
