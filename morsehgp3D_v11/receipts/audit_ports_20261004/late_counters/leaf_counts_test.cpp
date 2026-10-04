// Compteurs locaux de feuille (contrat R1 du 4 octobre) : debordement detecte au vidage, sans publication, et
// grandes feuilles (m = 32, 33, 256, 1024) dont le compte des tests de dominance vaut exactement C(m, 2).
#include <limits>
#include <vector>

#include "catalogue/internal.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;

namespace {
constexpr u64 kMax = std::numeric_limits<u64>::max();

// Une feuille directe : tous les sites, boite fournie ; le ledger de depart est impose.
Result<CatalogueLedger> visit(const std::vector<std::array<u32, 3>>& points, Box box, const CatalogueLedger& start,
                             MemoryBudget& budget, int kmax = 5) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  std::vector<SiteIdx> sites;
  for (auto p : points) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(ids.size())));
    sites.push_back(make_id<SiteIdx>(static_cast<u32>(sites.size())));
  }
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  Workspace workspace;
  MHGP11_TRY(workspace.allocate(std::max<u32>(cloud.value().sites(), 1), budget));
  CatalogueParams params;
  params.kmax = kmax;
  Collector collector;
  Run run{cloud.value(), params, budget, workspace, collector, start};
  MHGP11_TRY(enumerate_leaf(run, sites, box));
  return run.ledger;
}

// m sites distincts sur une helice grossiere loin de la boite : la dominance est presque totale, peu de prefixes.
std::vector<std::array<u32, 3>> spiral(u32 m) {
  std::vector<std::array<u32, 3>> out;
  for (u32 i = 0; i < m; ++i) out.push_back({100 + 3 * i, 100 + (i * 7) % 61, 100 + (i * 13) % 67});
  return out;
}
}  // namespace

MHGP11_TEST(flush_overflow, 9) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const std::vector<std::array<u32, 3>> tetra{{1, 2, 6}, {8, 4, 8}, {2, 1, 3}, {7, 8, 5}};
  const Box box{{4, 4, 4}, {5, 5, 5}};
  auto fresh = visit(tetra, box, {}, budget);
  REQUIRE(fresh.ok());
  REQUIRE(fresh.value().census_tests > 0);
  // Juste a la limite : le vidage atteint exactement u64max, sans refus.
  CatalogueLedger edge;
  edge.census_tests = kMax - fresh.value().census_tests;
  auto exact = visit(tetra, box, edge, budget);
  REQUIRE(exact.ok());
  CHECK_EQ(exact.value().census_tests, kMax);
  CHECK_EQ(exact.value().emitted, fresh.value().emitted);
  // Un de plus : le vidage deborde ; refus, aucune valeur publiee.
  CatalogueLedger over;
  over.census_tests = kMax - fresh.value().census_tests + 1;
  auto refused = visit(tetra, box, over, budget);
  CHECK(!refused.ok());
  CHECK_EQ(refused.outcome().reason, Reason::catalogue_counter_overflow);
  CatalogueLedger last;
  last.region_line_fallbacks = kMax;  // dernier champ vide : debordement des que la feuille en ajoute un
  last.region_line_tests = 0;
  auto quiet = visit(tetra, box, last, budget);  // sans cache, aucun repli : ajout nul, pas de refus
  CHECK(quiet.ok());
  CHECK(budget.released().ok());
}

MHGP11_TEST(large_leaves, 20) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Box box{{0, 0, 0}, {2, 2, 2}};
  for (u32 m : {32u, 33u, 256u, 1024u}) {
    const auto points = spiral(m);
    auto first = visit(points, box, {}, budget);
    auto second = visit(points, box, {}, budget);
    REQUIRE(first.ok() && second.ok());
    CHECK(first.value() == second.value());
    CHECK_EQ(first.value().dominance_tests, u64(m) * (m - 1) / 2);
    CHECK(first.value().prefixes >= 1);
    CHECK(first.value().census_tests >= first.value().judged);
  }
  auto too_large = visit(spiral(1025), box, {}, budget);
  CHECK(!too_large.ok());
  CHECK_EQ(too_large.outcome().reason, Reason::catalogue_invariant);
  CHECK(budget.released().ok());
}

// Grand livre complet d'une feuille gravee (tetraedre de center_region.cpp), toutes les valeurs ecrites : la voie a
// compteurs locaux rend exactement les champs de la voie a additions controlees (ledgers des trames ng00 et ng02
// identiques, 9b9244a00 contre la voie R1). Tue tout vidage omis ou detourne.
MHGP11_TEST(engraved, 16) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto got = visit({{1, 2, 6}, {8, 4, 8}, {2, 1, 3}, {7, 8, 5}}, Box{{4, 4, 4}, {5, 5, 5}}, {}, budget);
  REQUIRE(got.ok());
  const auto& l = got.value();
  // Enumeration complete (boite centrale, K = 5) : C(4,2) dominances, 15 parties non vides, 4 presentations jugees
  // de 4 sites chacune ; couples testes 6 + 2*4 + 3 (graphe de paires inactif) ; droites 4 + 3, sans cache.
  CHECK_EQ(l.dominance_tests, 6u); CHECK_EQ(l.prefixes, 15u); CHECK_EQ(l.judged, 4u);
  CHECK_EQ(l.census_tests, 16u); CHECK_EQ(l.emitted, 4u); CHECK_EQ(l.incidences, 12u);
  CHECK_EQ(l.q4_candidates, 1u); CHECK_EQ(l.q4_levels, 1u);
  CHECK_EQ(l.region_pair_tests, 17u); CHECK_EQ(l.region_pair_rejects, 0u);
  CHECK_EQ(l.region_line_tests, 7u); CHECK_EQ(l.region_line_rejects, 0u);
  CHECK_EQ(l.region_line_evaluations, 7u); CHECK_EQ(l.region_line_cache_hits, 0u);
  CHECK_EQ(l.region_line_fallbacks, 0u);
  CHECK(budget.released().ok());
}

MHGP11_TEST_MAIN()
