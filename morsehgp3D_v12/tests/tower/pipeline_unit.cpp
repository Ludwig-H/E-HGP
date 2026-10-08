// Portes de la Session recouverte (build_tower, decision D-F2 ; src/tower/pipeline.hpp), hors produit :
//   graphe       les predecesseurs declares (step_dependencies), fermes transitivement, couvrent chaque lecture d'une
//                etape (table ecrite ici, independamment : tampons lus par le corps de chaque etape) ;
//   identite     voie sequentielle (resolve_tower puis build_forests) contre build_tower a 1, 2, 3 et 8 fils, sur des
//                nuages aleatoires, des grilles (plateaux, cohortes de naissances) et des droites, K de 1 a 5 :
//                empreinte FUL1, foret complete de chaque ordre (registre et branches de R, historique d'attache,
//                evenements), compteurs de l'objet et du travail (foret et G), cibles ;
//   determinisme un nuage de 1 500 sites a K = 4 : memes empreintes et compteurs a 1, 3 et 8 fils, egaux a la voie
//                sequentielle ;
//   admission    pic de la region (au-dessus de l'usage a l'admission) au plus les octets admis ; quand l'admission
//                est la contrainte liante (pics de la chaine et de l'ouverture plus bas), une limite egale a l'usage
//                plus les octets admis suffit (aucune allocation de la region refusee) et un octet de moins refuse a
//                l'admission (memory_budget) ; rien de retenu apres un succes ou un refus ;
//   refus        limite juste au-dessus des pics de la chaine et de l'ouverture : refus memory_budget avant la
//                region, rien de retenu.
#include <bit>
#include <random>

#include "pipeline_support.hpp"
#include "tower/pipeline.hpp"

namespace mhgp12::tower_test {
namespace {

using tower::detail::Step;

// Voie sequentielle de reference : resolve_tower puis build_forests.
struct Sequential {
  std::optional<Resolution> resolution;
  std::optional<TowerForests> forests;
  tower::ForestLedger ledger;
};
Sequential sequential(const Chain& c, MemoryBudget& budget, sched::Pool& pool) {
  Sequential s;
  auto resolution = resolve_tower(*c.index, *c.catalogue, budget, pool);
  if (!resolution.ok()) return s;
  s.resolution.emplace(std::move(resolution).take());
  std::vector<ForestInput> inputs;
  for (Order k = 1; k <= s.resolution->orders(); ++k) inputs.push_back(tower::forest_input(s.resolution->order(k)));
  auto forests = tower::build_forests(c.index->cloud(), tower::catalogue_balls(*c.catalogue), inputs,
                                      tower::ForestParams{}, budget, pool, &s.ledger);
  if (forests.ok()) s.forests.emplace(std::move(forests).take());
  return s;
}

// Foret complete d'un ordre : registre (dont les branches de R, que FUL1 n'ecrit pas), historique d'attache et ses
// evenements par survivant (LEM-T5), rangs et noeuds des evenements.
bool same_forest(const OrderForest& a, const OrderForest& b) {
  return same_registry(a, b) && same_buffer(a.attach_parent, b.attach_parent) &&
         same_buffer(a.attach_rank, b.attach_rank) && same_buffer(a.survivor_events.off, b.survivor_events.off) &&
         same_buffer(a.survivor_events.val, b.survivor_events.val) && same_buffer(a.event_rank, b.event_rank) &&
         same_buffer(a.event_node, b.event_node);
}

bool same_targets(const ResolvedOrder& a, const ResolvedOrder& b) {
  return a.representatives() == b.representatives() &&
         std::equal(a.targets().begin(), a.targets().end(), b.targets().begin()) && a.counters() == b.counters();
}

// Session contre voie sequentielle sur un nuage, a plusieurs nombres de fils ; rend le nombre d'ordres compares.
u64 compare_paths(const Points& p, int kmax, std::initializer_list<u32> threads) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  const Chain c = make_chain(p, kmax, budget, *pool);
  if (!c.catalogue) return 0;
  const Sequential ref = sequential(c, budget, *pool);
  if (!CHECK(ref.forests.has_value())) return 0;
  const std::string expected = digest_of(c, *ref.forests);
  u64 compared = 0;
  for (const u32 w : threads) {
    auto workers = pool_of(w);
    TowerDiagnostics diag;
    auto tower = build_tower(*c.index, *c.catalogue, budget, *workers, &diag);
    if (!CHECK(tower.ok())) return compared;
    const std::string got = digest_of(c, tower.value().forests);
    if (!CHECK(got == expected)) std::printf("empreinte %s, attendue %s (fils %u)\n", got.c_str(), expected.c_str(), w);
    const Order orders = ref.resolution->orders();
    CHECK_EQ(tower.value().resolution.orders(), orders);
    for (Order k = 1; k <= orders; ++k) {
      CHECK(same_forest(tower.value().forests.orders[k - 1], ref.forests->orders[k - 1]));
      CHECK(diag.forest.object[k - 1] == ref.ledger.object[k - 1]);
      CHECK(diag.forest.work[k - 1] == ref.ledger.work[k - 1]);
      CHECK(same_targets(tower.value().resolution.order(k), ref.resolution->order(k)));
      ++compared;
    }
    CHECK(diag.end_ns >= diag.g_end_ns && diag.threads == w);
  }
  return compared;
}

// Lectures de chaque etape (corps de step_body et du noyau) : (etape lue, ordre inferieur).
struct Need {
  Step step, needs;
  bool below;
};
constexpr Need kNeeds[] = {
    {tower::detail::kNumberOpen, tower::detail::kCheck, false},     // entree controlee
    {tower::detail::kNumber, tower::detail::kNumberOpen, false},    // tampon de l'ordre, naissances de l'ordre
    {tower::detail::kNumberClose, tower::detail::kNumber, false},   // ordre canonique complet
    {tower::detail::kKernelOpen, tower::detail::kNumberClose, false},  // birth_node (feuilles de G), naissances
    {tower::detail::kKernel, tower::detail::kKernelOpen, false},    // feuilles, union-find ouvert
    {tower::detail::kHistoryCheck, tower::detail::kKernel, false},  // evenements, attaches
    {tower::detail::kDepth, tower::detail::kKernel, false},         // attaches
    {tower::detail::kHistory, tower::detail::kHistoryCheck, false},  // evenements controles
    {tower::detail::kSlices, tower::detail::kKernel, false},        // evenements
    {tower::detail::kClasses, tower::detail::kSlices, false},
    {tower::detail::kNodes0, tower::detail::kClasses, false},   // classes de toutes les tranches
    {tower::detail::kNodes, tower::detail::kNodes0, false},     // premiers noeuds
    {tower::detail::kParents, tower::detail::kNodes, false},    // event_node de toutes les tranches
    {tower::detail::kPlace, tower::detail::kParents, false},
    {tower::detail::kChildren, tower::detail::kPlace, false},
    {tower::detail::kFinish, tower::detail::kChildren, false},
    {tower::detail::kLower, tower::detail::kFinish, false},     // nombre de noeuds
    {tower::detail::kBirths, tower::detail::kFinish, true},     // cell_node, parent, rang de l'ordre inferieur
    {tower::detail::kBirths, tower::detail::kNumberClose, true},  // birth_node de l'ordre inferieur
    {tower::detail::kBirths, tower::detail::kLower, false},
    {tower::detail::kMerges, tower::detail::kBirths, false},    // verticales des naissances
    {tower::detail::kMerges, tower::detail::kHistory, true},    // survivor_events de l'ordre inferieur
    {tower::detail::kMerges, tower::detail::kFinish, true},     // event_node, event_rank, minleaf inferieurs
    {tower::detail::kRows, tower::detail::kFinish, false},      // cell_node, minleaf ; evenements liberes
    {tower::detail::kRows, tower::detail::kHistory, false},     // survivor_events ; evenements liberes
    {tower::detail::kRows, tower::detail::kHistoryCheck, false},  // evenements lus par le controle, liberes
    {tower::detail::kCollect, tower::detail::kRows, false},
    {tower::detail::kPlaceRows, tower::detail::kCollect, false},
    {tower::detail::kFill, tower::detail::kPlaceRows, false},
};

MHGP12_TEST(graphe, 40) {
  constexpr u32 n = tower::detail::kStepCount;
  // reach[s][t][b] : t (a l'ordre i - b) est un predecesseur transitif de s (a l'ordre i), b = 0, 1 ou 2.
  bool reach[n][n][3] = {};
  for (u32 s = 0; s < n; ++s) {
    const auto deps = tower::detail::step_dependencies(static_cast<Step>(s));
    CHECK(deps.count <= 2);
    for (u32 e = 0; e < deps.count; ++e) reach[s][deps.edge[e].step][deps.edge[e].below ? 1 : 0] = true;
  }
  for (u32 round = 0; round < 2 * n; ++round)
    for (u32 s = 0; s < n; ++s)
      for (u32 t = 0; t < n; ++t)
        for (u32 b = 0; b < 3; ++b) {
          if (!reach[s][t][b]) continue;
          for (u32 u = 0; u < n; ++u)
            for (u32 c = 0; c + b < 3; ++c)
              if (reach[t][u][c]) reach[s][u][b + c] = true;
        }
  for (u32 s = 0; s < n; ++s) CHECK(!reach[s][s][0]);  // sans cycle a ordre egal
  for (const Need& need : kNeeds) CHECK(reach[need.step][need.needs][need.below ? 1 : 0]);
  for (u32 s = 0; s < n; ++s) CHECK(tower::detail::refusal_rank(static_cast<Step>(s)) < tower::detail::kRefusalRanks);
}

MHGP12_TEST(identite, 400) {
  std::mt19937_64 rng(20261008);
  u64 orders = 0;
  for (int it = 0; it < 12; ++it) {
    const Points p = random_points(rng, 20 + static_cast<u32>(rng() % 140), 1u << 16);
    orders += compare_paths(p, 1 + static_cast<int>(rng() % 5), {1, 2, 3, 8});
  }
  orders += compare_paths(grid_points(4, 4, 3, 7), 5, {1, 3, 8});
  orders += compare_paths(grid_points(5, 3, 1, 11), 4, {1, 8});
  orders += compare_paths(grid_points(9, 1, 1, 3), 5, {1, 2});
  CHECK(orders >= 150);
  std::printf("identite ordres=%llu\n", static_cast<unsigned long long>(orders));
}

MHGP12_TEST(determinisme, 30) {
  std::mt19937_64 rng(7);
  const Points p = random_points(rng, 1500, 1u << 14);
  CHECK_EQ(compare_paths(p, 4, {1, 3, 8}), 12);
}

// Session en trois temps sur budget sans cache (tailles exactes) : pics de la chaine (index, catalogue) et de
// l'ouverture de la Session, usage a l'admission, octets admis, pic de la region au-dessus de cet usage.
struct Measured {
  u64 chain_peak = 0, open_peak = 0, used_open = 0, admitted = 0, rise = 0;
  bool ok = false;
};
Measured measure(const Points& p, int kmax, sched::Pool& pool) {
  Measured m;
  MemoryBudget budget(MemoryBudget::kUnlimited);
  budget.restart_peak();
  const Chain c = make_chain(p, kmax, budget, pool);
  if (!c.catalogue) return m;
  m.chain_peak = budget.restart_peak();
  auto run = std::make_unique<tower::detail::SessionRun>(*c.index, *c.catalogue, budget, pool);
  if (!tower::detail::open_session(*run).ok()) return m;
  m.used_open = budget.used();
  m.open_peak = budget.restart_peak();
  m.admitted = run->diag.admitted_bytes;
  m.ok = tower::detail::admit_session(*run).ok() && tower::detail::play_session(*run).ok();
  m.rise = budget.restart_peak() - m.used_open;
  return m;
}

// build_tower sous une limite ; rend l'issue, et controle que rien n'est retenu apres.
Outcome under_limit(const Points& p, int kmax, u64 limit, sched::Pool& pool) {
  MemoryBudget budget(limit);
  const Chain c = make_chain(p, kmax, budget, pool);
  if (!CHECK(c.catalogue.has_value())) return fail(Reason::tower_invariant);
  const u64 base = budget.used();
  Outcome out;
  {
    auto tower = build_tower(*c.index, *c.catalogue, budget, pool);
    out = tower.ok() ? Outcome{} : tower.outcome();
  }
  CHECK_EQ(budget.used(), base);  // transactionnel : rien de retenu, succes ou refus
  return out;
}

MHGP12_TEST(admission, 60) {
  std::mt19937_64 rng(31);
  auto pool = pool_of(3);
  u64 binding = 0;
  for (int it = 0; it < 24; ++it) {
    const Points p = random_points(rng, 30 + static_cast<u32>(rng() % 200), 1u << 15);
    const int kmax = 2 + static_cast<int>(rng() % 4);
    const Measured m = measure(p, kmax, *pool);
    if (!m.ok) continue;
    CHECK(m.rise <= m.admitted);
    if (m.rise > m.admitted)
      std::printf("admission pic=%llu admis=%llu\n", static_cast<unsigned long long>(m.rise),
                  static_cast<unsigned long long>(m.admitted));
    // Si l'admission de la region est la contrainte liante, une limite egale a l'usage plus les octets admis suffit
    // (aucune allocation de la region refusee), et un octet de moins refuse a l'admission.
    const u64 limit = m.used_open + m.admitted;
    if (std::max(m.chain_peak, m.open_peak) >= limit) continue;
    ++binding;
    CHECK(under_limit(p, kmax, limit, *pool).ok());
    CHECK_EQ(under_limit(p, kmax, limit - 1, *pool).reason, Reason::memory_budget);
  }
  CHECK(binding >= 8);
  std::printf("admission cas_liants=%llu\n", static_cast<unsigned long long>(binding));
}

MHGP12_TEST(refus, 20) {
  std::mt19937_64 rng(43);
  auto pool = pool_of(2);
  for (int it = 0; it < 6; ++it) {
    const Points p = random_points(rng, 60 + static_cast<u32>(rng() % 100), 1u << 15);
    const Measured m = measure(p, 4, *pool);
    if (!CHECK(m.ok && m.admitted > 0)) continue;
    // Limite juste au-dessus de la chaine et de l'ouverture : refus memory_budget avant la region, rien de retenu.
    const u64 limit = std::max(m.chain_peak, m.open_peak);
    if (limit >= m.used_open + m.admitted) continue;
    CHECK_EQ(under_limit(p, 4, limit, *pool).reason, Reason::memory_budget);
  }
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()
