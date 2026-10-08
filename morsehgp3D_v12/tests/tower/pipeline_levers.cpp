// Portes des leviers de T2-d-A6 dans la Session recouverte (src/tower/pipeline.hpp), hors produit, sur les entrees de
// la chaine du produit (nuages aleatoires de K 1 a 5, grilles a grandes cohortes de meme rang, droite) :
//   numerotation  number_births_range par morceaux de 1, 2, 3, 7, 64 naissances et d'un seul tenant, joues dans un
//                 ordre melange : ordre canonique et compteurs de cohortes identiques a number_births ; une entree a
//                 rangs decroissants refusee tower_invariant par le morceau qui possede la cohorte fautive ;
//   indices       noyau du produit sur des feuilles remplacees par leurs racines (hint_leaves) a plusieurs ages
//                 d'instantane (tranche courante, tranches suivantes, cibles cellule traitees ou non) : evenements,
//                 cellules des evenements, attaches, sommets des cellules identiques au noyau sans indices ;
//   historique    controle par morceaux, profondeur par remontee des attaches et CSR par survivant contre
//                 build_history : memes evenements par survivant, meme profondeur maximale ; une attache corrompue
//                 refusee tower_invariant par le controle comme par build_history.
#include <random>

#include "pipeline_support.hpp"
#include "tower/pipeline.hpp"

namespace mhgp12::tower_test {
namespace {

using tower::ForestWork;
using tower::detail::BuildState;
using tower::detail::Event;

// Une chaine resolue et ses entrees de foret.
struct Resolved {
  Resolved(const Points& p, int kmax, MemoryBudget& budget, sched::Pool& pool)
      : chain(make_chain(p, kmax, budget, pool)) {}
  Chain chain;
  std::optional<Resolution> resolution;
  std::vector<ForestInput> inputs;
  std::optional<tower::BallSource> balls;
};
std::unique_ptr<Resolved> resolved(const Points& p, int kmax, MemoryBudget& budget, sched::Pool& pool) {
  auto r = std::make_unique<Resolved>(p, kmax, budget, pool);
  if (!r->chain.catalogue) return nullptr;
  auto res = resolve_tower(*r->chain.index, *r->chain.catalogue, budget, pool);
  if (!res.ok()) return nullptr;
  r->resolution.emplace(std::move(res).take());
  for (Order k = 1; k <= r->resolution->orders(); ++k) r->inputs.push_back(tower::forest_input(r->resolution->order(k)));
  r->balls.emplace(tower::catalogue_balls(*r->chain.catalogue));
  return r;
}

std::vector<Points> clouds() {
  std::mt19937_64 rng(20261008);
  std::vector<Points> out;
  for (int it = 0; it < 6; ++it) out.push_back(random_points(rng, 40 + static_cast<u32>(rng() % 400), 1u << 12));
  out.push_back(grid_points(5, 5, 4, 9));
  out.push_back(grid_points(7, 3, 1, 5));
  out.push_back(grid_points(12, 1, 1, 3));
  return out;
}

// ---- numerotation ---------------------------------------------------------------------------------------------------
u64 check_numbering(const Resolved& r, u32 i, MemoryBudget& budget, std::mt19937_64& rng) {
  const ForestInput& in = r.inputs[i];
  const u64 nb = in.birth_key.size();
  const Cloud& cloud = r.chain.index->cloud();
  std::vector<u32> order_ref(nb), node_ref(nb);
  ForestWork ref;
  if (!CHECK(tower::detail::number_births(cloud, *r.balls, in, order_ref, node_ref, ref, budget).ok())) return 0;
  u64 compared = 0;
  for (const u64 size : {u64{1}, u64{2}, u64{3}, u64{7}, u64{64}, nb}) {
    if (size == 0) continue;
    std::vector<u32> order(nb, 0xA5A5A5A5u);
    std::vector<u64> pieces;
    for (u64 b = 0; b < nb; b += size) pieces.push_back(b);
    std::shuffle(pieces.begin(), pieces.end(), rng);
    ForestWork work;
    for (const u64 b : pieces)
      CHECK(tower::detail::number_births_range(cloud, *r.balls, in, order, b, std::min(nb, b + size), work, budget)
                .ok());
    CHECK(order == order_ref);
    CHECK_EQ(work.cohorts, ref.cohorts);
    CHECK_EQ(work.max_cohort, ref.max_cohort);
    ++compared;
  }
  return compared;
}

MHGP12_TEST(numerotation, 300) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  std::mt19937_64 rng(7);
  u64 compared = 0, cohorts = 0;
  for (const Points& p : clouds()) {
    const auto r = resolved(p, 5, budget, *pool);
    if (!CHECK(r != nullptr)) continue;
    for (u32 i = 0; i < r->inputs.size(); ++i) {
      compared += check_numbering(*r, i, budget, rng);
      for (u64 v = 1; v < r->inputs[i].birth_rank.size(); ++v)
        cohorts += r->inputs[i].birth_rank[v] == r->inputs[i].birth_rank[v - 1];
    }
  }
  CHECK(compared >= 100 && cohorts >= 50);  // des cohortes de plus d'une naissance, coupees par les morceaux
  // rangs decroissants : refus du morceau qui possede la cohorte fautive, comme number_births
  OrderData bad;
  bad.k = 2;
  bad.birth_key = {0, 1, 2};
  bad.birth_rank = ranks({0, 2, 1});
  const ForestInput in = bad.view();
  const auto r = resolved(clouds().front(), 2, budget, *pool);
  REQUIRE(r != nullptr);
  std::vector<u32> order(3), node(3);
  ForestWork work;
  CHECK_EQ(tower::detail::number_births(r->chain.index->cloud(), *r->balls, in, order, node, work, budget).reason,
           Reason::tower_invariant);
  CHECK_EQ(tower::detail::number_births_range(r->chain.index->cloud(), *r->balls, in, order, 1, 2, work, budget).reason,
           Reason::tower_invariant);
  CHECK(tower::detail::number_births_range(r->chain.index->cloud(), *r->balls, in, order, 2, 3, work, budget).ok());
  std::printf("numerotation comparaisons=%llu paires_de_meme_rang=%llu\n", static_cast<unsigned long long>(compared),
              static_cast<unsigned long long>(cohorts));
}

// ---- indices --------------------------------------------------------------------------------------------------------
struct KernelOut {
  std::vector<Event> events;
  std::vector<u32> event_cell, attach_parent, attach_rank, cell_top;
  bool operator==(const KernelOut& o) const {
    if (events.size() != o.events.size()) return false;
    for (u64 e = 0; e < events.size(); ++e)
      if (std::memcmp(&events[e], &o.events[e], sizeof(Event)) != 0) return false;
    return event_cell == o.event_cell && attach_parent == o.attach_parent && attach_rank == o.attach_rank &&
           cell_top == o.cell_top;
  }
};

// Noyau de l'ordre i par blocs de `block` cellules ; avant chaque bloc, indices sur les cellules [debut du bloc + lag
// blocs, + 1 bloc) (lag = 0 : le bloc courant) ; feuilles d'origine restaurees d'abord.
KernelOut kernel_with_hints(BuildState& s, u32 i, const std::vector<u32>& leaves0, u64 block, i64 lag, u64& hinted) {
  const ForestInput& in = s.inputs[i];
  tower::detail::OrderWork& w = s.work[i];
  OrderForest& f = s.forests.orders[i];
  const u64 nc = in.cell_ball.size();
  std::copy(leaves0.begin(), leaves0.end(), w.leaves.begin());
  ForestWork counters;
  KernelOut out;
  if (!CHECK(tower::detail::open_kernel(in, f, w, s.budget).ok())) return out;
  for (u64 b = 0; b < nc; b += block) {
    if (lag >= 0) {
      const u64 h0 = b + static_cast<u64>(lag) * block;
      if (h0 < nc) hinted += tower::detail::hint_leaves(in, w, h0, std::min(nc, h0 + block), b);
    }
    CHECK(tower::detail::advance_kernel(in, w.leaves.span(), f, w, counters, std::min(nc, b + block)).ok());
  }
  out.cell_top.assign(w.cell_top.begin(), w.cell_top.end());
  CHECK(tower::detail::close_kernel(in, f, w, counters).ok());
  out.events.assign(w.events.data(), w.events.data() + w.event_count);
  out.event_cell.assign(f.event_cell.begin(), f.event_cell.end());
  out.attach_parent.assign(f.attach_parent.begin(), f.attach_parent.end());
  out.attach_rank.assign(f.attach_rank.begin(), f.attach_rank.end());
  return out;
}

MHGP12_TEST(indices, 300) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  u64 runs = 0, hinted = 0, cell_targets = 0;
  for (const Points& p : clouds()) {
    const auto r = resolved(p, 5, budget, *pool);
    if (!CHECK(r != nullptr)) continue;
    tower::ForestParams params;
    auto s = std::make_unique<BuildState>(r->chain.index->cloud(), *r->balls, std::span<const ForestInput>(r->inputs),
                                          params, budget);
    for (u32 i = 0; i < r->inputs.size(); ++i) {
      const ForestInput& in = r->inputs[i];
      REQUIRE(tower::detail::number_order(*s, i).ok());
      ForestWork lc;
      REQUIRE(tower::detail::resolve_leaves(in, s->forests.orders[i], s->work[i].leaves.span(), 0,
                                            in.cell_ball.size(), lc).ok());
      cell_targets += lc.cell_targets;
      const std::vector<u32> leaves0(s->work[i].leaves.begin(), s->work[i].leaves.end());
      u64 none = 0;
      const KernelOut ref = kernel_with_hints(*s, i, leaves0, 1u << 30, -1, none);
      for (const u64 block : {u64{1}, u64{3}, u64{16}, u64{256}})
        for (const i64 lag : {i64{0}, i64{1}, i64{4}}) {
          CHECK(kernel_with_hints(*s, i, leaves0, block, lag, hinted) == ref);
          ++runs;
        }
    }
  }
  CHECK(runs >= 300 && hinted >= 10000 && cell_targets >= 1000);
  std::printf("indices passes=%llu feuilles_indicees=%llu cibles_cellule=%llu\n", static_cast<unsigned long long>(runs),
              static_cast<unsigned long long>(hinted), static_cast<unsigned long long>(cell_targets));
}

// ---- historique -----------------------------------------------------------------------------------------------------
MHGP12_TEST(historique, 100) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  u64 orders = 0, deepest = 0;
  for (const Points& p : clouds()) {
    const auto r = resolved(p, 5, budget, *pool);
    if (!CHECK(r != nullptr)) continue;
    tower::ForestParams params;
    auto a = std::make_unique<BuildState>(r->chain.index->cloud(), *r->balls, std::span<const ForestInput>(r->inputs),
                                          params, budget);
    auto b = std::make_unique<BuildState>(r->chain.index->cloud(), *r->balls, std::span<const ForestInput>(r->inputs),
                                          params, budget);
    for (u32 i = 0; i < r->inputs.size(); ++i) {
      const ForestInput& in = r->inputs[i];
      for (BuildState* s : {a.get(), b.get()}) {
        REQUIRE(tower::detail::number_order(*s, i).ok());
        ForestWork lc;
        REQUIRE(tower::detail::resolve_leaves(in, s->forests.orders[i], s->work[i].leaves.span(), 0,
                                              in.cell_ball.size(), lc).ok());
        REQUIRE(tower::detail::run_kernel(in, s->work[i].leaves.span(), s->forests.orders[i], s->work[i],
                                          s->counters[i], budget).ok());
      }
      ForestWork ref, got;
      OrderForest& fa = a->forests.orders[i];
      OrderForest& fb = b->forests.orders[i];
      CHECK(tower::detail::build_history(fa, a->work[i], ref, budget).ok());
      const u64 ne = b->work[i].event_count, nb = fb.births;
      for (u64 e = 0; e < ne; e += 5) CHECK(tower::detail::check_history(fb, b->work[i], e, std::min(ne, e + 5)).ok());
      for (u64 v = 0; v < nb; v += 7) tower::detail::history_depth(fb, v, std::min(nb, v + 7), got);
      CHECK(tower::detail::build_survivor_events(fb, b->work[i], budget).ok());
      CHECK_EQ(got.max_attach_depth, ref.max_attach_depth);
      CHECK(same_buffer(fa.survivor_events.off, fb.survivor_events.off));
      CHECK(same_buffer(fa.survivor_events.val, fb.survivor_events.val));
      deepest = std::max(deepest, ref.max_attach_depth);
      ++orders;
      // attache corrompue (premier perdant detache) : refus du controle comme de build_history
      u32 loser = 0;
      while (loser < nb && fb.attach_parent[loser] == kNone) ++loser;
      if (ne > 0 && CHECK(loser < nb)) {
        const u32 saved = fb.attach_parent[loser];
        fb.attach_parent[loser] = kNone;
        fa.attach_parent[loser] = kNone;
        Outcome refused;
        for (u64 e = 0; e < ne; e += 5)
          refused = merge(refused, tower::detail::check_history(fb, b->work[i], e, std::min(ne, e + 5)));
        ForestWork scratch;
        CHECK_EQ(refused.reason, Reason::tower_invariant);
        CHECK_EQ(tower::detail::build_history(fa, a->work[i], scratch, budget).reason, Reason::tower_invariant);
        fb.attach_parent[loser] = saved;
        fa.attach_parent[loser] = saved;
      }
    }
  }
  CHECK(orders >= 30 && deepest >= 2);
  std::printf("historique ordres=%llu profondeur_max=%llu\n", static_cast<unsigned long long>(orders),
              static_cast<unsigned long long>(deepest));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()
