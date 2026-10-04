// J2 raccorde au DFS : feuilles completes controlees, rejets effectifs et contacts conserves.
#include <numeric>
#include <vector>

#include "catalogue/internal.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;

namespace {
Result<CatalogueLedger> visit(std::initializer_list<std::array<u32, 3>> points, Box box,
                             MemoryBudget& budget) {
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
  MHGP11_TRY(workspace.allocate(cloud.value().sites(), budget));
  CatalogueParams params;
  Collector collector;
  Run run{cloud.value(), params, budget, workspace, collector, {}};
  // La liste contient TOUS les sites ; G2 est donc satisfait dans chacune des boites de ces portes.
  MHGP11_TRY(enumerate_leaf(run, sites, box));
  return run.ledger;
}
}  // namespace

MHGP11_TEST(pair_region, 13) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto missed = visit({{0, 0, 0}, {4, 0, 0}}, {{0, 0, 0}, {1, 1, 1}}, budget);
  REQUIRE(missed.ok());
  CHECK_EQ(missed.value().region_pair_tests, 1u);
  CHECK_EQ(missed.value().region_pair_rejects, 1u);
  CHECK_EQ(missed.value().emitted, 0u);
  CHECK_EQ(missed.value().judged, 0u);
  CHECK(budget.released().ok());
  auto contact = visit({{0, 0, 0}, {4, 0, 0}}, {{2, 0, 0}, {3, 1, 1}}, budget);
  REQUIRE(contact.ok());
  CHECK_EQ(contact.value().region_pair_rejects, 0u);
  CHECK_EQ(contact.value().emitted, 1u);
  CHECK_EQ(contact.value().incidences, 2u);
  CHECK(budget.released().ok());
  auto invalid = visit({{0, 0, 0}, {4, 0, 0}}, {{2, 0, 0}, {2, 1, 1}}, budget);
  CHECK_EQ(invalid.outcome().reason, Reason::catalogue_invariant);
  CHECK(budget.released().ok());
}

MHGP11_TEST(line_region, 13) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto missed = visit({{7, 4, 2}, {7, 0, 1}, {7, 3, 4}}, {{0, 0, 0}, {2, 2, 2}}, budget);
  REQUIRE(missed.ok());
  CHECK_EQ(missed.value().region_pair_rejects, 0u);
  CHECK_EQ(missed.value().region_line_tests, 1u);
  CHECK_EQ(missed.value().region_line_rejects, 1u);
  CHECK_EQ(missed.value().emitted, 0u);
  CHECK(budget.released().ok());
  auto aligned = visit({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}, {{0, 0, 0}, {5, 1, 1}}, budget);
  REQUIRE(aligned.ok());
  CHECK_EQ(aligned.value().region_line_rejects, 1u);
  CHECK_EQ(aligned.value().emitted, 3u);
  CHECK_EQ(aligned.value().q4_candidates, 0u);
  auto contact = visit({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}, {{2, 2, 0}, {3, 3, 1}}, budget);
  REQUIRE(contact.ok());
  CHECK_EQ(contact.value().region_line_rejects, 0u);
  CHECK_EQ(contact.value().emitted, 1u);  // Boule du diametre, avec la coquille complete de trois sites.
  CHECK_EQ(contact.value().incidences, 3u);
  CHECK(budget.released().ok());
}

MHGP11_TEST(obtuse_region, 6) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto tetra = visit({{1, 2, 6}, {8, 4, 8}, {2, 1, 3}, {7, 8, 5}}, {{4, 4, 4}, {5, 5, 5}}, budget);
  REQUIRE(tetra.ok());
  CHECK_EQ(tetra.value().region_pair_rejects, 0u);
  CHECK_EQ(tetra.value().region_line_rejects, 0u);
  CHECK(tetra.value().region_line_tests >= 4u);
  CHECK_EQ(tetra.value().q4_candidates, 1u);
  CHECK_EQ(tetra.value().q4_levels, 1u);
  CHECK(budget.released().ok());
}

// Lemmes M3/E4 (feuille J3) : l'enveloppe ne rejette jamais un centre de la boite. Triangle strictement aigu dans le
// plan x = 2, centre circonscrit (2,2,1) sur les faces basses de [2,3)x[2,3)x[1,2) : enveloppe mediane plate en x
// (toutes les abscisses doubles valent 4 = 2 lo) ; la boule q3 et la boule q2 de (b,c) doivent etre emises.
MHGP11_TEST(median_envelope, 14) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto flat = visit({{2, 0, 0}, {2, 4, 0}, {2, 1, 3}}, {{2, 2, 1}, {3, 3, 2}}, budget);
  REQUIRE(flat.ok());
  CHECK_EQ(flat.value().judged, 2u);
  CHECK_EQ(flat.value().emitted, 2u);
  CHECK_EQ(flat.value().incidences, 5u);
  CHECK(budget.released().ok());
  // Meme triangle, boite au-dessus de son plan : M3 rejette avant toute construction, aucune emission q3.
  auto beside = visit({{2, 0, 0}, {2, 4, 0}, {2, 1, 3}}, {{3, 2, 1}, {4, 3, 2}}, budget);
  REQUIRE(beside.ok());
  CHECK_EQ(beside.value().emitted, 0u);
  CHECK(budget.released().ok());
  // Tetraedre plat : centre circonscrit (25,25,11.5) hors de l'enveloppe des sommets (z dans [20,21]). Toutes les
  // droites de faces passent par ce centre, donc le prefixe q4 arrive a q4_of : non degenere, compte, puis rejete
  // par E4 avant la fabrique ; aucun niveau q4.
  auto flat_tetra = visit({{20, 20, 20}, {30, 20, 20}, {20, 30, 20}, {21, 21, 21}}, {{25, 25, 11}, {26, 26, 12}},
                          budget);
  REQUIRE(flat_tetra.ok());
  CHECK_EQ(flat_tetra.value().q4_candidates, 1u);
  CHECK_EQ(flat_tetra.value().q4_levels, 0u);
  CHECK(budget.released().ok());
  // Base aigue au sol, sommet a la verticale de son centre : centre (15, 8, 611/60) strictement interieur, en z hors de
  // l'enveloppe des trois sommets de base (z = 0) ; seul le quatrieme sommet la porte. La boule q4 doit etre emise.
  auto tall = visit({{0, 0, 0}, {30, 0, 0}, {15, 25, 0}, {15, 8, 30}}, {{15, 8, 10}, {16, 9, 11}}, budget);
  REQUIRE(tall.ok());
  CHECK_EQ(tall.value().q4_candidates, 1u);
  CHECK_EQ(tall.value().q4_levels, 1u);
  CHECK(budget.released().ok());
}

MHGP11_TEST_MAIN()
