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

MHGP11_TEST_MAIN()
