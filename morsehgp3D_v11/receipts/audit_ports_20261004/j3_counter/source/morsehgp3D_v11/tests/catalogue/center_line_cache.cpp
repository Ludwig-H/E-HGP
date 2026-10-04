// Cache J2 : etats exacts, rangs sans collision, reset de feuille/passe, budget et equivalence publique.
#include <array>
#include <vector>

#include "catalogue/center_line_cache.hpp"
#include "catalogue/internal.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace cat_detail = mhgp11::catalogue_detail;
using Cache = cat_detail::CenterLineCache;
using Relation = num::CenterLineRelation;
using Coordinates = std::array<u32, 3>;

namespace {
std::vector<num::Point> points(const std::vector<Coordinates>& source) {
  std::vector<num::Point> out;
  for (const auto& p : source) out.push_back(num::Point::make(p[0], p[1], p[2]).value());
  return out;
}

void query(Cache& cache, Relation expected, bool hit, bool fallback) {
  const auto result = cache.lookup(0, 1, 2);
  REQUIRE(result.ok());
  CHECK_EQ(result.value().relation, expected);
  CHECK_EQ(result.value().hit, hit);
  CHECK_EQ(result.value().fallback, fallback);
}

Result<Cloud> cloud_of(const std::vector<Coordinates>& source, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (const auto& p : source) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(ids.size())));
  }
  return prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
}

template <class Integer>
bool same_integer(const Integer& a, const Integer& b) {
  const auto x = num::to_wide(a), y = num::to_wide(b);
  return x.neg == y.neg && x.words == y.words;
}

bool same_geometry(const Catalogue& a, const Catalogue& b) {
  if (a.kmax() != b.kmax() || a.balls() != b.balls() || a.levels().size() != b.levels().size()) return false;
  for (std::size_t i = 0; i < a.levels().size(); ++i)
    if (!same_integer(a.levels()[i].numerator(), b.levels()[i].numerator()) ||
        !same_integer(a.levels()[i].denominator(), b.levels()[i].denominator())) return false;
  for (u32 i = 0; i < a.balls(); ++i) {
    const auto& x = a.balls_data()[i];
    const auto& y = b.balls_data()[i];
    if (x.support != y.support || x.rank != y.rank || x.qmin != y.qmin || x.p != y.p || x.m != y.m) return false;
  }
  return std::equal(a.population_offsets().begin(), a.population_offsets().end(),
                    b.population_offsets().begin(), b.population_offsets().end()) &&
         std::equal(a.population().begin(), a.population().end(), b.population().begin(), b.population().end());
}

CatalogueLedger logical(CatalogueLedger value) {
  value.region_line_evaluations = value.region_line_cache_hits = value.region_line_fallbacks = 0;
  return value;
}

std::vector<Coordinates> line(u32 count) {
  std::vector<Coordinates> out;
  for (u32 i = 0; i < count; ++i) out.push_back({i, 0, 0});
  return out;
}
}  // namespace

MHGP11_TEST(ranks, 96) {
  CHECK_EQ(Cache::entries(0), 0u);
  CHECK_EQ(Cache::entries(2), 0u);
  CHECK_EQ(Cache::entries(3), 1u);
  CHECK_EQ(Cache::entries(32), 4960u);
  CHECK_EQ(Cache::entries(33), 4960u);
  CHECK_EQ(Cache::entries(1024), 4960u);
  for (u32 m = 3; m <= 32; ++m) {
    std::vector<u32> seen(Cache::entries(m), 0);
    bool bounds = true;
    u32 visited = 0;
    for (u32 i = 0; i < m; ++i)
      for (u32 j = i + 1; j < m; ++j)
        for (u32 k = j + 1; k < m; ++k) {
          const u32 at = Cache::rank(i, j, k);
          if (at >= seen.size()) bounds = false;
          else ++seen[at];
          ++visited;
        }
    CHECK(bounds);
    CHECK_EQ(visited, m * (m - 1) * (m - 2) / 6);
    CHECK(std::all_of(seen.begin(), seen.end(), [](u32 count) { return count == 1; }));
  }
}

MHGP11_TEST(relations_reset, 69) {
  std::array<u8, 4960> storage;
  storage.fill(0xa5);  // ni l'allocateur ni la passe precedente ne fournissent les etats initiaux.
  const auto wide = num::CenterRegion::make({0, 0, 0}, {i64(kCoordMax) + 1, i64(kCoordMax) + 1, i64(kCoordMax) + 1});
  const auto narrow = num::CenterRegion::make({0, 0, 0}, {1, 1, 1});
  REQUIRE(wide.ok()); REQUIRE(narrow.ok());
  auto data = points({{0, 0, 0}, {kCoordMax, kCoordMax, 0}, {kCoordMax, 0, kCoordMax}});
  for (int pass = 0; pass < 2; ++pass) {
    auto first = Cache::make(storage, data, wide.value(), true);
    REQUIRE(first.ok());
    query(first.value(), Relation::intersects, false, false);
    query(first.value(), Relation::intersects, true, false);
    auto other = Cache::make(storage, data, narrow.value(), true);
    REQUIRE(other.ok());
    query(other.value(), Relation::disjoint, false, false);
    query(other.value(), Relation::disjoint, true, false);
  }
  data = points({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}});
  auto aligned = Cache::make(storage, data, wide.value(), true);
  REQUIRE(aligned.ok());
  query(aligned.value(), Relation::degenerate, false, false);
  query(aligned.value(), Relation::degenerate, true, false);
  data = points({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}});
  const auto contact = num::CenterRegion::make({0, 0, 0}, {2, 2, 1});
  REQUIRE(contact.ok());
  auto touching = Cache::make(storage, data, contact.value(), true);
  REQUIRE(touching.ok());
  query(touching.value(), Relation::intersects, false, false);
  query(touching.value(), Relation::intersects, true, false);
  CHECK_EQ(touching.value().lookup(1, 0, 2).outcome().reason, Reason::catalogue_invariant);
  CHECK_EQ(touching.value().lookup(0, 1, 3).outcome().reason, Reason::catalogue_invariant);
  CHECK_EQ(Cache::make({}, data, contact.value(), true).outcome().reason, Reason::catalogue_invariant);
  auto disabled = Cache::make({}, data, contact.value(), false);
  REQUIRE(disabled.ok());
  query(disabled.value(), Relation::intersects, false, false);
  query(disabled.value(), Relation::intersects, false, false);
}

MHGP11_TEST(capacity_fallback, 37) {
  std::array<u8, 4960> storage;
  storage.fill(0xa5);
  const auto region = num::CenterRegion::make({0, 0, 0}, {64, 64, 64});
  REQUIRE(region.ok());
  for (u32 m : {32u, 33u, 1024u}) {
    auto data = points(line(m));
    auto cache = Cache::make(storage, data, region.value(), true);
    REQUIRE(cache.ok());
    query(cache.value(), Relation::degenerate, false, m > 32);
    query(cache.value(), Relation::degenerate, m <= 32, m > 32);
    auto tail = cache.value().lookup(m - 3, m - 2, m - 1);
    REQUIRE(tail.ok());
    CHECK_EQ(tail.value().relation, Relation::degenerate);
    CHECK(!tail.value().hit);
  }
}

MHGP11_TEST(memory, 45) {
  for (u32 m : {2u, 3u, 32u, 33u, 1024u}) {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    u64 without = 0;
    {
      cat_detail::Workspace work;
      REQUIRE(work.allocate(m, budget).ok());
      without = budget.used();
      CHECK_EQ(work.center_lines.size(), 0u);
    }
    CHECK(budget.released().ok());
    {
      cat_detail::Workspace work;
      REQUIRE(work.allocate(m, budget, true).ok());
      CHECK_EQ(budget.used(), without + Cache::entries(m));
      CHECK_EQ(work.center_lines.size(), Cache::entries(m));
    }
    CHECK(budget.released().ok());
    MemoryBudget short_budget(without + Cache::entries(m) - 1);
    cat_detail::Workspace rejected;
    CHECK_EQ(rejected.allocate(m, short_budget, true).reason, Reason::memory_budget);
    CHECK_EQ(short_budget.used(), 0u);  // admission globale avant la premiere allocation
  }
}

MHGP11_TEST(equivalence, 250) {
  const u32 m = kCoordMax;
  const std::vector<std::vector<Coordinates>> cases{
      {{1, 2, 6}, {8, 4, 8}, {2, 1, 3}, {7, 8, 5}},  // q4 positif, face012 obtuse
      {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0}, {0, 0, 4}, {4, 0, 4}, {0, 4, 4}, {4, 4, 4}},
      {{0, 0, 0}, {m, m, 0}, {m, 0, m}, {0, m, m}}, line(33), line(257)};
  for (std::size_t f = 0; f < cases.size(); ++f) {
    MemoryBudget owner(MemoryBudget::kUnlimited), reference_budget(MemoryBudget::kUnlimited);
    auto cloud = cloud_of(cases[f], owner);
    REQUIRE(cloud.ok());
    CatalogueParams params;
    params.kmax = 3;
    params.leaf_size = f == 3 ? 64 : 32;
    auto reference = build_catalogue(cloud.value(), params, reference_budget);
    REQUIRE(reference.ok());
    CHECK_EQ(reference.value().ledger().region_line_evaluations, reference.value().ledger().region_line_tests);
    CHECK_EQ(reference.value().ledger().region_line_cache_hits, 0u);
    params.cache_center_lines = true;
    for (u32 workers : {0u, 1u, 2u, 4u, 8u}) {
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto pool = sched::make_pool({std::max(workers, 1u)});
      REQUIRE(pool.ok());
      {
        auto actual = workers == 0 ? build_catalogue(cloud.value(), params, budget) :
                                    build_catalogue(cloud.value(), params, budget, *pool.value());
        REQUIRE(actual.ok());
        CHECK(same_geometry(reference.value(), actual.value()));
        const auto& l = actual.value().ledger();
        CHECK(logical(reference.value().ledger()) == logical(l));
        CHECK_EQ(l.region_line_tests, l.region_line_evaluations + l.region_line_cache_hits);
        CHECK(l.region_line_fallbacks <= l.region_line_evaluations);
        if (f == 0 || f == 2) {
          CHECK_EQ(l.region_line_tests, 7u);
          CHECK_EQ(l.region_line_evaluations, 4u);
          CHECK_EQ(l.region_line_cache_hits, 3u);
          CHECK_EQ(l.q4_candidates, 1u);
        }
        if (f == 3) {
          CHECK(l.region_line_tests > 0);
          CHECK_EQ(l.region_line_fallbacks, l.region_line_tests);
          CHECK_EQ(l.region_line_cache_hits, 0u);
        }
      }
      CHECK(budget.released().ok());
    }
  }
}

MHGP11_TEST_MAIN()
