// Resolution locale/globale : hit sans allocation, miss canonique, saturation et absence admissible.
#include "tower/locate.hpp"
#include "test_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;
using namespace tower_test;

namespace {
Result<FullDomain> domain_of(const Input& input, int kmax, MemoryBudget& budget) {
  auto cloud = input.prepare(budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params; params.kmax = kmax;
  return prepare_full_domain(std::move(index.value()), params, budget);
}
std::vector<SiteIdx> select(const FullDomain& domain, std::initializer_list<Xyz> coordinates) {
  std::vector<SiteIdx> ids;
  for (auto p : coordinates) ids.push_back(site(domain.index().cloud(), p));
  return ids;
}
}

MHGP11_TEST(lookup, 10) {
  MemoryBudget owner(MemoryBudget::kUnlimited), empty(0);
  auto made = domain_of(Input({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}), 3, owner);
  REQUIRE(made.ok());
  auto& domain = made.value();
  auto part = select(domain, {{0, 0, 0}, {4, 0, 0}});
  auto located = locate_part(domain, part, 2, empty);
  REQUIRE(located.ok());
  CHECK(located.value().ball().has_value());
  CHECK_EQ(located.value().kind(), CensusKind::complete);
  CHECK(located.value().census_work() == nullptr);
  REQUIRE(located.value().support().has_value());
  CHECK_EQ(located.value().support()->arity, 2);
  CHECK_EQ(located.value().interior().size(), 1u);
  CHECK_EQ(located.value().shell().size(), 2u);
  CHECK_EQ(empty.peak(), 0u);
  CHECK(level_is(located.value().meb().sphere(), 4, 1));
}

MHGP11_TEST(global_identity, 18) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto made = domain_of(Input({{1, 2, 0}, {0, 5, 0}, {8, 1, 0}, {8, 9, 0}, {9, 8, 0}}), 4, owner);
  REQUIRE(made.ok());
  auto& domain = made.value();
  auto part = select(domain, {{1, 2, 0}, {0, 5, 0}, {8, 1, 0}, {8, 9, 0}});
  {
    auto located = locate_part(domain, part, 4, work);
    REQUIRE(located.ok());
    CHECK_EQ(located.value().meb().support().size(), 3u);
    REQUIRE(located.value().support().has_value());
    CHECK_EQ(located.value().support()->arity, 2);
    REQUIRE(located.value().ball().has_value());
    CHECK_EQ(located.value().kind(), CensusKind::complete);
    CHECK(located.value().interior().empty());
    CHECK_EQ(located.value().shell().size(), 5u);
    CHECK(located.value().census_work() != nullptr);
    CHECK_EQ(work.used(), 20u);
    CHECK(center_is(located.value().meb().sphere(), {5, 5, 0}, 1));
    const auto* shell = located.value().shell().data();
    LocatedPart moved(std::move(located.value()));
    CHECK(moved.shell().data() == shell);
    CHECK_EQ(work.used(), 20u);
    CHECK(located.value().shell().empty() && located.value().interior().empty());
    CHECK(!located.value().ball() && !located.value().support());
    CHECK(located.value().census_work() == nullptr);
  }
  CHECK(work.released().ok());
  MemoryBudget refused(19);
  CHECK_EQ(locate_part(domain, part, 4, refused).outcome().reason, Reason::memory_budget);
  CHECK(refused.released().ok());
}

MHGP11_TEST(saturated, 11) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(8);
  auto made = domain_of(Input({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0},
                               {4, 0, 0}, {5, 0, 0}, {10, 0, 0}}), 5, owner);
  REQUIRE(made.ok());
  auto& domain = made.value();
  auto part = select(domain, {{0, 0, 0}, {10, 0, 0}});
  auto located = locate_part(domain, part, 2, work);
  REQUIRE(located.ok());
  CHECK_EQ(located.value().kind(), CensusKind::saturated);
  CHECK(!located.value().ball() && !located.value().support());
  CHECK_EQ(located.value().interior().size(), 2u);
  CHECK(located.value().shell().empty());
  CHECK(located.value().census_work() != nullptr);
  CHECK_EQ(work.used(), 8u);
  auto smaller = bounded_meb(domain.index().cloud(), located.value().interior());
  REQUIRE(smaller.ok());
  CHECK(num::compare(smaller.value().sphere().level(), located.value().meb().sphere().level()) < 0);
  CHECK_EQ(locate_part(domain, part, 0, work).outcome().reason, Reason::kmax_out_of_range);
  CHECK_EQ(locate_part(domain, part, 3, work).outcome().reason, Reason::parameter_out_of_range);
}

MHGP11_TEST(outside_catalogue, 16) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto triangle = domain_of(Input({{0, 0, 0}, {4, 0, 0}, {2, 3, 0}, {2, 1, 0}, {2, 2, 0}}), 3, budget);
  REQUIRE(triangle.ok());
  auto tpart = select(triangle.value(), {{0, 0, 0}, {4, 0, 0}, {2, 3, 0}});
  auto t = locate_part(triangle.value(), tpart, 3, budget);
  REQUIRE(t.ok());
  CHECK_EQ(t.value().kind(), CensusKind::complete);
  CHECK(!t.value().ball());
  REQUIRE(t.value().support().has_value());
  CHECK_EQ(t.value().support()->arity, 3);
  CHECK_EQ(t.value().interior().size(), 2u);
  CHECK_EQ(t.value().shell().size(), 3u);
  auto tetra = domain_of(Input({{0, 0, 0}, {4, 4, 0}, {4, 0, 4}, {0, 4, 4}, {1, 1, 1}, {2, 2, 2}}), 4, budget);
  REQUIRE(tetra.ok());
  auto qpart = select(tetra.value(), {{0, 0, 0}, {4, 4, 0}, {4, 0, 4}, {0, 4, 4}});
  auto q = locate_part(tetra.value(), qpart, 4, budget);
  REQUIRE(q.ok());
  CHECK_EQ(q.value().kind(), CensusKind::complete);
  CHECK(!q.value().ball());
  REQUIRE(q.value().support().has_value());
  CHECK_EQ(q.value().support()->arity, 4);
  CHECK_EQ(q.value().interior().size(), 2u);
  CHECK_EQ(q.value().shell().size(), 4u);
  auto one = select(tetra.value(), {{2, 2, 2}});
  auto zero = locate_part(tetra.value(), one, 1, budget);
  REQUIRE(zero.ok());
  CHECK(!zero.value().ball() && zero.value().support()->arity == 1);
  CHECK_EQ(zero.value().shell().size(), 1u);
}

MHGP11_TEST_MAIN()
