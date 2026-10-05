// Harnais de l'arbre d'ordre K et du rattachement (tranche S3) : nuages, identite des forets avec build_full,
// juge E2 par descente (port de ball_nodes, bench/points_export.cpp, fenetre au lieu du predicat fort) et branches
// recomptees par descentes neuves des traces strictes ; jamais inclus par le produit.
#pragma once
#include <random>
#include <set>

#include "forest_support.hpp"
#include "sched/sched.hpp"
#include "tower/order_tree.hpp"

namespace order_tree_test {
using namespace forest_test;

// Fenetre de W_K ecrite ici, independamment du produit (seed_log.hpp) : evenements faibles compris.
inline bool window_ball(const CatalogueBall& ball, u64 k) {
  return u64{ball.p} + ball.qmin <= k + 1 && k <= u64{ball.p} + ball.m;
}

inline std::vector<Xyz> random_cloud(u32 count, u32 side, u32 seed, bool clustered) {
  std::mt19937 rng(seed);
  std::vector<Xyz> points;
  std::set<std::array<u32, 3>> seen;
  auto draw = [&rng](u32 bound) { return static_cast<u32>(rng() % bound); };
  while (points.size() < count) {
    const u32 centre = clustered ? side / 4 * draw(3) : 0, spread = clustered ? side / 8 : side;
    const std::array<u32, 3> p{centre + draw(spread), centre + draw(spread), draw(spread)};
    if (seen.insert(p).second) points.push_back({p[0], p[1], p[2]});
  }
  return points;
}

struct Cloud3 { std::vector<Xyz> points; Order kmax; };
// Nuages bornes : fixtures du paragraphe 2.9 de la specification, coquilles etendues (petites grilles) et grappes.
inline std::vector<Cloud3> clouds() {
  return {{{{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, 4},
          {{{0,0,0},{2,2,0},{4,0,0},{8,0,0}}, 4},
          {{{0,0,0},{2,2,0},{2,0,2}}, 3},
          {{{20,20,20},{20,0,0},{0,20,0},{0,0,20},{10,10,10},{11,10,10},{10,11,10}}, 5},
          {{{0,0,0},{4,0,0},{6,0,0},{8,0,0},{12,0,0}}, 4},
          {{{0,0,0},{2,0,0},{4,0,0},{6,0,0},{8,0,0},{10,0,0},{12,0,0},{14,0,0},{16,0,0}}, 5},
          {{{0,0,0},{2,0,0},{0,2,0},{2,2,0},{10,0,0},{12,0,0},{11,2,0}}, 5},
          {random_cloud(40, 6, 3, false), 5}, {random_cloud(70, 16, 5, false), 5},
          {random_cloud(90, 64, 7, false), 5}, {random_cloud(120, 256, 11, true), 5},
          {random_cloud(60, 8, 13, false), 5}};
}

// Foret d'ordre k identique noeud pour noeud (rangs, parents, enfants, cles de naissance), verticales exclues.
inline bool same_tree(const OrderForest& a, const OrderForest& b) {
  if (a.order() != b.order() || a.births() != b.births() || a.root() != b.root() ||
      a.nodes().size() != b.nodes().size() || a.edges().size() != b.edges().size() ||
      !std::equal(a.edges().begin(), a.edges().end(), b.edges().begin())) return false;
  for (u32 i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.rank != y.rank || x.parent != y.parent || x.child_count != y.child_count ||
        x.child_begin != y.child_begin || x.birth_key != y.birth_key) return false;
  }
  return true;
}
// Champs logiques du registre de la foret : sans travail paye des descentes (memos, census) ni travail vertical.
inline ForestLedger logical(ForestLedger value) {
  value.descent = {};
  value.ancestor_hops = value.vertical_descents = value.vertical_checks = value.vertical_reuses = 0;
  value.ancestor_queries = value.ancestor_activations = value.ancestor_unions = value.ancestor_find_steps = 0;
  return value;
}
inline bool same_attachment(const WindowAttachment& a, const WindowAttachment& b) {
  auto eq = [](auto x, auto y) { return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin()); };
  return eq(a.balls(), b.balls()) && eq(a.node(), b.node()) && eq(a.role(), b.role()) &&
         eq(a.strict_traces(), b.strict_traces()) && eq(a.components(), b.components()) &&
         eq(a.prior_offsets(), b.prior_offsets()) && eq(a.prior(), b.prior());
}

// Parametres exerces : voie serielle, lots Q=1 et Q=4096, memos, table de populations, census reutilises, lookup dense.
inline std::vector<FullParams> variants() {
  std::vector<FullParams> out(1);
  for (u32 q : {1u, 4096u}) for (unsigned flags = 0; flags < 8; ++flags) {
    FullParams p;
    p.regular_batch_capacity = q; p.descent_lanes = 4;
    p.memo_capacity = (flags & 1u) != 0 ? 8 : 0; p.lane_memo_capacity = p.memo_capacity;
    p.population_lookup = (flags & 2u) != 0;
    p.reuse_census_workspace = (flags & 4u) != 0;
    p.dense_birth_lookup = q == 4096;
    out.push_back(p);
  }
  return out;
}

inline std::vector<SiteIdx> population(const FullDomain& domain, BallIdx ball) {
  std::vector<SiteIdx> part;
  for (SiteIdx s : domain.catalogue().interior(ball)) part.push_back(s);
  for (SiteIdx s : domain.catalogue().shell(ball)) part.push_back(s);
  return part;
}

// Lemme E : noeud vivant a la coupe fermee lambda_b de la naissance rendue par la descente d'une K-partie de P_b.
// first : les K premiers sites (interieurs puis coquille, comme ball_nodes) ; sinon les K derniers (T3, I7).
inline Result<NodeIdx> judge_e2(const FullDomain& domain, const OrderForest& forest, BallIdx ball, bool first,
                                MemoryBudget& budget) {
  const auto pop = population(domain, ball);
  const u32 k = forest.order();
  if (pop.size() < k) return fail(Reason::tower_invariant);
  std::vector<SiteIdx> part(first ? pop.begin() : pop.end() - k, first ? pop.begin() + k : pop.end());
  std::sort(part.begin(), part.end(), [](SiteIdx a, SiteIdx b) { return idx(a) < idx(b); });
  const auto& data = domain.catalogue().balls_data()[idx(ball)];
  auto down = descend(domain, part, k, budget);
  if (!down.ok()) return down.outcome();
  if (num::compare(down.value().initial_level(), domain.catalogue().levels()[idx(data.rank)]) > 0)
    return fail(Reason::tower_invariant);
  const auto birth = forest.birth_node(down.value().seed());
  if (!birth) return fail(Reason::tower_invariant);
  u64 hops = 0;
  return forest.ancestor_closed(*birth, data.rank, hops);
}

// Branches par une autre voie : traces strictes de build_cell, descentes neuves sans memo, noeud vivant a la coupe
// fermee r_b - 1 par la marche des parents (ancestor_closed), dedupliques.
inline Result<std::vector<u32>> judge_branches(const FullDomain& domain, const OrderForest& forest, BallIdx ball,
                                               u64& traces, MemoryBudget& budget) {
  std::vector<u32> out;
  const auto& data = domain.catalogue().balls_data()[idx(ball)];
  auto cell = build_cell(domain, ball, forest.order(), budget);
  if (!cell.ok()) return cell.outcome();
  traces = cell.value().traces().size();
  for (const auto& trace : cell.value().traces()) {
    auto down = descend(domain, trace.part(), forest.order(), budget);
    if (!down.ok()) return down.outcome();
    const auto birth = forest.birth_node(down.value().seed());
    if (!birth || idx(data.rank) == 0) return fail(Reason::tower_invariant);
    u64 hops = 0;
    auto open = forest.ancestor_closed(*birth, LevelRank{idx(data.rank) - 1}, hops);
    if (!open.ok()) return open.outcome();
    out.push_back(idx(open.value()));
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}
}  // namespace order_tree_test
