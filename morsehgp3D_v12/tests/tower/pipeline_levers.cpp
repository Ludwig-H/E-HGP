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
//   priorite      A6b : dans des Sessions de 1 000 a 3 000 sites a K5, a 2, 3 et 8 fils, aucune tache d'indices ne
//                 commence tant qu'une tranche de G reste a reclamer (compteur hint_jobs_during_g nul).
//   tranches      fixture de l'auditeur Codex (a6_prefixe_tranches, complement de CST-0242), entree T structurelle de
//                 513 cellules aux tranches reelles du produit : historique du noyau contre un oracle par ensembles,
//                 identique sous toutes les dates legales d'indice (fenetre du planificateur, prefixes quelconques
//                 anterieurs a la cellule, indices repris) ; temoins : l'indice futur que le pont de publication
//                 exclut donne un historique faux que la porte detecte, ou un refus si les deux feuilles le recoivent.
//   bascule       A6c : decision de la chaine (chain_engaged, seuil kChainSites epingle, bornes) ; une Session sous le
//                 seuil par defaut suit le chemin de la base (aucun morceau de N ni de H, aucune aide) ; sur des nuages
//                 de 1 000 a 3 000 sites a K5, a 1, 3 et 8 fils, chaine forcee engagee et forcee au chemin de la base :
//                 meme empreinte FUL1, memes tableaux de chaque foret (registre, attaches, evenements par survivant,
//                 rangs et noeuds des evenements), memes compteurs de travail ; compteurs de chemin nuls d'un cote,
//                 positifs de l'autre (aides jouees sur l'ensemble des Sessions engagees).
// Les Sessions de priorite et de bascule sont jouees par SessionRun avec un seuil force (0 : chaine engagee ;
// maximum : chemin de la base), les nuages des portes etant tous sous kChainSites.
#include <limits>
#include <numeric>
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
    CHECK(tower::detail::advance_kernel(in, w.leaves.span(), f, w, counters, std::min(nc, b + block), true).ok());
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

// ---- tranches -------------------------------------------------------------------------------------------------------
// Fixture de l'auditeur Codex (receipts/audit_reponses_20261008/a6_prefixe_tranches) : entree T structurelle, PAS une
// trame HGP (ni catalogue ni G ; numerotation identite, precondition explicite du sous-test) : k = 1, naissances 0..4 de
// rang 0, 513 cellules de rangs 1..513 et de boules 100..612 ; cellule 0 -> {2, 3}, 1 -> {2, 4}, 2..510 -> {2}
// (inertes), t = 511 -> {0, 1}, u = 512 -> {0, 2} ; 517 representants. Aux tranches du produit (kCellGrain = 256) :
// [0, 256), [256, 512), [512, 513) ; t est la derniere cellule de la tranche 1, u la premiere de la tranche 2.
constexpr u32 kFixtureT = 511, kFixtureU = 512;

OrderData tranches_input() {
  OrderData d;
  d.k = 1;
  d.births(5, 0);
  for (u32 c = 0; c <= kFixtureU; ++c) {
    std::vector<u32> targets{2};
    if (c == 0) targets = {2, 3};
    if (c == 1) targets = {2, 4};
    if (c == kFixtureT) targets = {0, 1};
    if (c == kFixtureU) targets = {0, 2};
    d.cell(c + 1, targets, 100 + c);
  }
  return d;
}

// Oracle par ensembles, sans union-find : unions binaires successives des representants de chaque cellule ; par
// evenement, sa cellule et les naissances de la composante formee (cibles de naissance seulement).
struct SetHistory {
  std::vector<u32> cell;
  std::vector<std::vector<u32>> merged;
};
SetHistory set_oracle(const ForestInput& in, u32 births) {
  std::vector<u32> label(births);
  std::iota(label.begin(), label.end(), 0u);
  SetHistory h;
  for (u64 t = 0; t < in.cell_ball.size(); ++t)
    for (u64 p = in.rep_offsets[t] + 1; p < in.rep_offsets[t + 1]; ++p) {
      const u32 run = label[in.targets[in.rep_offsets[t]]], other = label[in.targets[p]];
      if (run == other) continue;
      std::vector<u32> merged;
      for (u32 b = 0; b < births; ++b) {
        if (label[b] == other) label[b] = run;
        if (label[b] == run) merged.push_back(b);
      }
      h.cell.push_back(static_cast<u32>(t));
      h.merged.push_back(merged);
    }
  return h;
}

// Ensembles des evenements du noyau (operandes : feuille = naissance, numerotation identite ; ou evenement anterieur).
std::vector<std::vector<u32>> kernel_sets(const KernelOut& k) {
  std::vector<std::vector<u32>> sets;
  for (const Event& ev : k.events) {
    std::vector<u32> merged;
    for (const u32 top : {ev.a, ev.b}) {
      if (!(top & tower::detail::kEventBit)) merged.push_back(top);
      else if ((top & tower::detail::kEventIndex) < sets.size())
        merged.insert(merged.end(), sets[top & tower::detail::kEventIndex].begin(),
                      sets[top & tower::detail::kEventIndex].end());
    }
    std::sort(merged.begin(), merged.end());
    sets.push_back(merged);
  }
  return sets;
}

bool matches(const KernelOut& k, const SetHistory& h) {
  return k.event_cell == h.cell && kernel_sets(k) == h.merged;
}

// Indice : avant la cellule `at` (union-find apres le prefixe [0, at)), hint_leaves sur les cellules [begin, end), avec
// processed = at.
struct Hint {
  u64 at, begin, end;
};

// Noyau du produit sur la fixture : feuilles `leaves`, indices aux dates donnees ; issue de close_kernel dans closed.
KernelOut kernel_scheduled(const ForestInput& in, OrderForest& f, tower::detail::OrderWork& w, MemoryBudget& budget,
                           const std::vector<u32>& leaves, std::vector<Hint> hints, u64& hinted, Outcome& closed) {
  std::copy(leaves.begin(), leaves.end(), w.leaves.begin());
  std::stable_sort(hints.begin(), hints.end(), [](const Hint& a, const Hint& b) { return a.at < b.at; });
  ForestWork counters;
  KernelOut out;
  if (!CHECK(tower::detail::open_kernel(in, f, w, budget).ok())) return out;
  for (const Hint& h : hints) {
    CHECK(tower::detail::advance_kernel(in, w.leaves.span(), f, w, counters, h.at, true).ok());
    hinted += tower::detail::hint_leaves(in, w, h.begin, h.end, h.at);
  }
  CHECK(tower::detail::advance_kernel(in, w.leaves.span(), f, w, counters, in.cell_ball.size(), true).ok());
  out.cell_top.assign(w.cell_top.begin(), w.cell_top.end());
  out.events.assign(w.events.data(), w.events.data() + w.event_count);
  out.event_cell.assign(f.event_cell.begin(), f.event_cell.begin() + w.event_count);
  out.attach_parent.assign(f.attach_parent.begin(), f.attach_parent.end());
  out.attach_rank.assign(f.attach_rank.begin(), f.attach_rank.end());
  closed = tower::detail::close_kernel(in, f, w, counters);
  return out;
}

MHGP12_TEST(tranches, 400) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const u64 grain = tower_detail::kCellGrain;
  REQUIRE(grain == 256 && tower::detail::kHintWindow >= 2);  // geometrie de la fixture : trois tranches
  const OrderData d = tranches_input();
  const ForestInput in = d.view();
  REQUIRE(tower::check_forest_input(in).ok());
  const u64 nc = in.cell_ball.size();
  REQUIRE(nc == 513 && in.targets.size() == 517);
  OrderForest f;
  f.k = 1;
  f.births = 5;
  REQUIRE(f.birth_node.allocate(5, budget).ok());
  for (u32 b = 0; b < 5; ++b) f.birth_node[b] = b;  // numerotation identite
  tower::detail::OrderWork w;
  REQUIRE(w.leaves.allocate(in.targets.size(), budget).ok());
  ForestWork lc;
  REQUIRE(tower::detail::resolve_leaves(in, f, w.leaves.span(), 0, nc, lc).ok());
  REQUIRE(lc.cell_targets == 0 && lc.birth_targets == 517);
  const std::vector<u32> leaves0(w.leaves.begin(), w.leaves.end());

  // Oracle : quatre evenements aux cellules 0, 1, t, u ; t unit {0} et {1}, u reunit tout (valeurs de l'auditeur).
  const SetHistory oracle = set_oracle(in, 5);
  CHECK(oracle.cell == (std::vector<u32>{0, 1, kFixtureT, kFixtureU}));
  CHECK(oracle.merged == (std::vector<std::vector<u32>>{{2, 3}, {2, 3, 4}, {0, 1}, {0, 1, 2, 3, 4}}));
  u64 hinted = 0;
  Outcome closed;
  const KernelOut ref = kernel_scheduled(in, f, w, budget, leaves0, {}, hinted, closed);
  CHECK(closed.ok());
  CHECK(matches(ref, oracle));
  // racine finale 2, et u attache 0 a 2 (up[0] = 2 au rang de u)
  CHECK(ref.events.size() == 4 && ref.events.back().surv == 2);
  CHECK(ref.attach_parent[0] == 2 && ref.attach_rank[0] == kFixtureU + 1);

  // Dates legales : (a) fenetre du planificateur, noyau a la frontiere de la tranche j, tranches s avec
  // j < s <= j + kHintWindow ; (b) tache qui lit l'union-find vivant pendant que le noyau avance : tout prefixe P
  // anterieur aux cellules indicees ; (c) indices repris a une date posterieure (P < Q).
  std::vector<std::vector<Hint>> schedules;
  const Hint s1_0{0, grain, 2 * grain}, s2_0{0, 2 * grain, nc}, s2_1{grain, 2 * grain, nc};
  for (const auto& s : std::vector<std::vector<Hint>>{{s1_0}, {s2_0}, {s2_1}, {s1_0, s2_0}, {s1_0, s2_1}})
    schedules.push_back(s);
  const std::vector<u64> prefixes{0, 1, 2, 3, 128, 255, 256, 257, 300, 510, kFixtureT, kFixtureU};
  for (const u64 p : prefixes) schedules.push_back({Hint{p, p, nc}});
  for (u64 a = 0; a < prefixes.size(); ++a)
    for (u64 b = a + 1; b < prefixes.size(); ++b)
      schedules.push_back({Hint{prefixes[a], prefixes[a], nc}, Hint{prefixes[b], prefixes[b], nc}});
  u64 runs = 0;
  for (const auto& s : schedules) {
    const KernelOut got = kernel_scheduled(in, f, w, budget, leaves0, s, hinted, closed);
    CHECK(closed.ok() && got == ref);
    CHECK(matches(got, oracle));
    ++runs;
  }

  // Temoins : l'indice futur (racine de la feuille 0 dans l'union-find FINAL, apres u), que le pont de publication
  // exclut. Calcule par hint_leaves du produit sur l'etat final, avant la cloture.
  REQUIRE(tower::detail::open_kernel(in, f, w, budget).ok());
  std::copy(leaves0.begin(), leaves0.end(), w.leaves.begin());
  ForestWork scratch;
  REQUIRE(tower::detail::advance_kernel(in, w.leaves.span(), f, w, scratch, nc, true).ok());
  const u64 t0 = in.rep_offsets[kFixtureT];
  CHECK_EQ(tower::detail::hint_leaves(in, w, kFixtureT, kFixtureT + 1, nc), 2u);
  const u32 future0 = w.leaves[t0], future1 = w.leaves[t0 + 1];
  CHECK(tower::detail::close_kernel(in, f, w, scratch).ok());
  CHECK(future0 == 2 && future1 == 2);
  // (i) premiere feuille de t seule (lecture de up[0] = 2 publie par u, puis ancien parent pour la seconde) : quatre
  // evenements et cloture acceptee, mais t unit {1} et {2, 3, 4} : historique faux, detecte par la porte.
  std::vector<u32> mixed = leaves0;
  mixed[t0] = future0;
  const KernelOut wrong = kernel_scheduled(in, f, w, budget, mixed, {}, hinted, closed);
  CHECK(closed.ok());
  CHECK(!(wrong == ref) && !matches(wrong, oracle));
  const auto wrong_sets = kernel_sets(wrong);
  CHECK(wrong.event_cell.size() == 4 && wrong.event_cell[2] == kFixtureT &&
        wrong_sets[2] == (std::vector<u32>{1, 2, 3, 4}));
  // (ii) les deux feuilles de t : t n'unit rien, racine non unique, cloture refusee.
  std::vector<u32> both = mixed;
  both[t0 + 1] = future1;
  (void)kernel_scheduled(in, f, w, budget, both, {}, hinted, closed);
  CHECK_EQ(closed.reason, Reason::tower_invariant);
  std::printf("tranches passes=%llu feuilles_indicees=%llu temoin_futur=detecte double_futur=refuse\n",
              static_cast<unsigned long long>(runs), static_cast<unsigned long long>(hinted));
}

// Priorite d'A6b : les aides passent apres la reclamation de toutes les tranches G. Le compteur est nul : une aide
// n'est reclamee que si aucune tranche de G ne l'est plus, et g_next ne fait que croitre. L'ancien ordre d'A6 (aides
// avant les etapes et G) peut le rendre positif tant que des tranches restent a reclamer.
// Session jouee par ses trois temps avec le seuil de la chaine donne (avant l'admission, ou il est lu).
std::unique_ptr<tower::detail::SessionRun> play(const Chain& c, MemoryBudget& budget, sched::Pool& pool,
                                                u64 chain_sites) {
  auto run = std::make_unique<tower::detail::SessionRun>(*c.index, *c.catalogue, budget, pool);
  if (!tower::detail::open_session(*run).ok()) return nullptr;
  run->chain_sites = chain_sites;
  if (!tower::detail::admit_session(*run).ok() || !tower::detail::play_session(*run).ok()) return nullptr;
  return run;
}

MHGP12_TEST(priorite, 18) {
  std::mt19937_64 rng(808);
  u64 hints = 0, sessions = 0;
  for (int it = 0; it < 3; ++it) {
    const Points p = random_points(rng, 1000 + 1000 * static_cast<u32>(it), 1u << 14);
    for (const u32 threads : {2u, 3u, 8u}) {
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto pool = pool_of(threads);
      const Chain c = make_chain(p, 5, budget, *pool);
      if (!CHECK(c.catalogue.has_value())) continue;
      const auto run = play(c, budget, *pool, 0);  // chaine engagee (A6c : ces nuages sont sous le seuil)
      if (!CHECK(run != nullptr)) continue;
      CHECK_EQ(run->diag.hint_jobs_during_g, 0);
      hints += run->diag.hint_jobs;
      ++sessions;
    }
  }
  CHECK(hints > 0);  // des aides ont ete jouees : le compteur nul n'est pas vide de sens
  std::printf("priorite sessions=%llu taches_indices=%llu (apres reclamation de toutes les tranches G)\n",
              static_cast<unsigned long long>(sessions), static_cast<unsigned long long>(hints));
}

// ---- bascule (A6c) --------------------------------------------------------------------------------------------------
bool same_history(const OrderForest& a, const OrderForest& b) {
  return same_buffer(a.attach_parent, b.attach_parent) && same_buffer(a.attach_rank, b.attach_rank) &&
         same_buffer(a.survivor_events.off, b.survivor_events.off) &&
         same_buffer(a.survivor_events.val, b.survivor_events.val) && same_buffer(a.event_rank, b.event_rank) &&
         same_buffer(a.event_node, b.event_node);
}

MHGP12_TEST(bascule, 200) {
  using tower::detail::chain_engaged;
  using tower::detail::kChainSites;
  constexpr u64 kNever = std::numeric_limits<u64>::max();
  // decision : seuil epingle (regression du retard du noyau K sur 40 trames, recu A6c), bornes incluses
  CHECK_EQ(kChainSites, u64{43900});
  CHECK(!chain_engaged(kChainSites - 1, kChainSites) && chain_engaged(kChainSites, kChainSites));
  CHECK(chain_engaged(0, 0) && !chain_engaged(kNever - 1, kNever));
  std::mt19937_64 rng(2026100801);
  u64 hints_on = 0, compared = 0;
  for (int it = 0; it < 3; ++it) {
    const Points p = random_points(rng, 1000 + 1000 * static_cast<u32>(it), 1u << 14);
    for (const u32 threads : {1u, 3u, 8u}) {
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto pool = pool_of(threads);
      const Chain c = make_chain(p, 5, budget, *pool);
      if (!CHECK(c.catalogue.has_value())) continue;
      if (it == 0 && threads == 3) {  // seuil par defaut : nuage sous kChainSites, chemin de la base
        const auto def = play(c, budget, *pool, kChainSites);
        if (CHECK(def != nullptr))
          CHECK(def->diag.chain_orders == 0 && def->diag.chain_number_jobs == 0 &&
                def->diag.chain_history_jobs == 0 && def->diag.hint_jobs == 0);
      }
      const auto off = play(c, budget, *pool, kNever);
      const auto on = play(c, budget, *pool, 0);
      if (!CHECK(off != nullptr && on != nullptr)) continue;
      const TowerForests& a = off->forest->forests;
      const TowerForests& b = on->forest->forests;
      CHECK_EQ(digest_of(c, a), digest_of(c, b));
      REQUIRE(a.kmax == 5 && b.kmax == 5);
      for (u32 i = 0; i < a.kmax; ++i) {
        CHECK(same_registry(a.orders[i], b.orders[i]));
        CHECK(same_history(a.orders[i], b.orders[i]));
        CHECK(off->diag.forest.work[i] == on->diag.forest.work[i]);
      }
      // compteurs de chemin : nuls au chemin de la base, positifs chaine engagee
      CHECK(off->diag.chain_orders == 0 && off->diag.chain_number_jobs == 0 && off->diag.chain_history_jobs == 0 &&
            off->diag.hint_jobs == 0);
      CHECK_EQ(on->diag.chain_orders, u32{0x1F});
      CHECK(on->diag.chain_number_jobs >= 2 * u64{a.kmax} && on->diag.chain_history_jobs >= 2 * u64{a.kmax});
      CHECK_EQ(on->diag.hint_jobs_during_g, 0);
      hints_on += on->diag.hint_jobs;
      ++compared;
    }
  }
  CHECK(compared == 9 && hints_on > 0);
  std::printf("bascule sessions=%llu aides_chaine_engagee=%llu\n", static_cast<unsigned long long>(compared),
              static_cast<unsigned long long>(hints_on));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()
