// Portes natives du catalogue : attendus geometriques, refus et transaction memoire.
#include <array>
#include <limits>
#include <type_traits>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "catalogue/internal.hpp"
#include "test.hpp"

using namespace mhgp11;

namespace {
using Coordinates = std::array<u32, 3>;

Result<Cloud> prepare(std::initializer_list<Coordinates> points, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (const auto& p : points) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(ids.size())));
  }
  return prepare_cloud(x, y, z, ids, CoordWidth(), budget);
}

bool level_is(const num::Level& level, u64 numerator, u64 denominator = 1) {
  auto wanted = num::Level::make(num::Wide<1>::from_u64(numerator), num::Wide<1>::from_u64(denominator));
  return wanted.ok() && num::compare(level, wanted.value()) == 0;
}

bool same(const Catalogue& a, const Catalogue& b) {
  if (a.balls() != b.balls() || a.levels().size() != b.levels().size() ||
      a.population().size() != b.population().size() || a.ledger() != b.ledger()) return false;
  for (u32 i = 0; i < a.balls(); ++i) {
    const auto& x = a.balls_data()[i];
    const auto& y = b.balls_data()[i];
    if (x.support != y.support || x.rank != y.rank || x.p != y.p || x.m != y.m || x.qmin != y.qmin) return false;
  }
  for (std::size_t i = 0; i < a.levels().size(); ++i)
    if (num::compare(a.levels()[i], b.levels()[i]) != 0) return false;
  return std::equal(a.population().begin(), a.population().end(), b.population().begin()) &&
         std::equal(a.population_offsets().begin(), a.population_offsets().end(), b.population_offsets().begin());
}
}  // namespace

static_assert(!std::is_copy_constructible_v<Catalogue> && !std::is_move_assignable_v<Catalogue>);
static_assert(std::is_nothrow_move_constructible_v<Catalogue>);

MHGP11_TEST(fixtures, 21) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = prepare({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}, budget);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1;
  auto k1 = build_catalogue(cloud.value(), params, budget);
  REQUIRE(k1.ok());
  CHECK_EQ(k1.value().balls(), 2u);
  CHECK_EQ(k1.value().levels().size(), 2u);
  CHECK(level_is(k1.value().levels()[0], 0));
  CHECK(level_is(k1.value().levels()[1], 1));
  for (const auto& ball : k1.value().balls_data()) {
    CHECK_EQ(ball.qmin, 2);
    CHECK_EQ(ball.p, 0u);
    CHECK_EQ(ball.m, 2u);
  }
  params.kmax = 2;
  auto k2 = build_catalogue(cloud.value(), params, budget);
  REQUIRE(k2.ok());
  CHECK_EQ(k2.value().balls(), 3u);
  CHECK(level_is(k2.value().levels().back(), 4));
  const auto& last = k2.value().balls_data().back();
  CHECK_EQ(last.p, 1u);
  CHECK_EQ(last.m, 2u);
  CHECK_EQ(k2.value().interior(make_id<BallIdx>(2)).size(), 1u);
  auto singleton = prepare({{7, 8, 9}}, budget);
  REQUIRE(singleton.ok());
  auto empty = build_catalogue(singleton.value(), params, budget);
  REQUIRE(empty.ok());
  CHECK_EQ(empty.value().balls(), 0u);
  CHECK_EQ(empty.value().levels().size(), 1u);
  CHECK(level_is(empty.value().levels()[0], 0));
}

MHGP11_TEST(refusals, 23) {
  CatalogueParams p;
  p.kmax = 0;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::kmax_out_of_range);
  p.kmax = 13;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::kmax_out_of_range);
  p.kmax = 5;
  p.leaf_size = 7;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::parameter_out_of_range);
  p.leaf_size = 32; p.max_leaf = 1025;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::parameter_out_of_range);
  p.max_leaf = 256; p.ball_limit = 0;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::parameter_out_of_range);
  p.ball_limit = u64{kNone} + 1;
  CHECK_EQ(check_catalogue_params(p).reason, Reason::parameter_out_of_range);
  p = {};
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto duplicate = prepare({{0, 0, 0}, {0, 0, 0}, {2, 0, 0}}, owner);
  REQUIRE(duplicate.ok());
  CHECK_EQ(build_catalogue(duplicate.value(), p, work).outcome().reason, Reason::multiplicity_unsupported);
  CHECK_EQ(work.peak(), 0u);
  auto cloud = prepare({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}, owner);
  REQUIRE(cloud.ok());
  p.ball_limit = 2;
  CHECK_EQ(build_catalogue(cloud.value(), p, work).outcome().reason, Reason::index_overflow_u32);
  CHECK(work.released().ok());
  auto cube = prepare({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0},
                       {0, 0, 2}, {2, 0, 2}, {0, 2, 2}, {2, 2, 2}}, owner);
  REQUIRE(cube.ok());
  p = {}; p.kmax = 1; p.leaf_size = 4; p.max_leaf = 4; p.max_nodes = 1;
  CHECK_EQ(build_catalogue(cube.value(), p, work).outcome().reason, Reason::node_budget);
  CHECK(work.released().ok());
  p.max_nodes = 0;
  CHECK_EQ(build_catalogue(cube.value(), p, work).outcome().reason, Reason::wide_leaf);
  CHECK(work.released().ok());
  u64 value = std::numeric_limits<u64>::max() - 1;
  CHECK(catalogue_detail::checked_add(value, 1).ok());
  CHECK_EQ(value, std::numeric_limits<u64>::max());
  CHECK_EQ(catalogue_detail::checked_add(value, 1).reason, Reason::catalogue_counter_overflow);
  CHECK_EQ(value, std::numeric_limits<u64>::max());
  CHECK_EQ(status_of(Reason::catalogue_invariant), Status::invariant_violated);
  CHECK_EQ(status_of(Reason::wide_leaf), Status::unsupported_degeneracy);
}

MHGP11_TEST(transaction, 17) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare({{0, 1, 1}, {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}}, owner);
  REQUIRE(cloud.ok());
  CatalogueParams p;
  MemoryBudget measured(MemoryBudget::kUnlimited);
  Buffer<u32> existing;
  REQUIRE(existing.allocate(17, measured).ok());
  existing[0] = 123;
  u64 exact_peak = 0;
  {
    auto first = build_catalogue(cloud.value(), p, measured);
    REQUIRE(first.ok());
    exact_peak = measured.peak();
    CHECK(exact_peak > existing.size() * sizeof(u32));
    const u64 before = measured.used();
    Catalogue moved = std::move(first.value());
    CHECK_EQ(measured.used(), before);
    CHECK_EQ(first.value().balls(), 0u);
    CHECK_EQ(first.value().kmax(), 0);
    auto repeated = build_catalogue(cloud.value(), p, measured);
    REQUIRE(repeated.ok());
    CHECK(same(moved, repeated.value()));
  }
  CHECK_EQ(measured.used(), 17u * sizeof(u32));
  CHECK_EQ(existing[0], 123u);
  for (u64 limit : {exact_peak - 1, exact_peak}) {
    MemoryBudget work(limit);
    Buffer<u32> sentinel;
    REQUIRE(sentinel.allocate(17, work).ok());
    {
      auto result = build_catalogue(cloud.value(), p, work);
      CHECK_EQ(result.ok(), limit == exact_peak);
      if (!result.ok()) CHECK_EQ(result.outcome().reason, Reason::memory_budget);
    }
    CHECK_EQ(work.used(), 17u * sizeof(u32));
    sentinel.reset();
    CHECK(work.released().ok());
  }
}

MHGP11_TEST(obtuse_prefix, 5) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = prepare({{10, 5, 5}, {9, 8, 5}, {5, 2, 1}, {1, 5, 8}}, budget);
  REQUIRE(cloud.ok());
  CatalogueParams p; p.kmax = 3;
  auto result = build_catalogue(cloud.value(), p, budget);
  REQUIRE(result.ok());
  unsigned matches = 0;
  for (const auto& ball : result.value().balls_data()) {
    if (ball.qmin != 4) continue;
    CHECK(level_is(result.value().levels()[idx(ball.rank)], 25));
    CHECK_EQ(ball.p, 0u);
    ++matches;
  }
  CHECK_EQ(matches, 1u);
}

MHGP11_TEST(q4_deferred, 25) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto regular = prepare({{0, 0, 0}, {4, 4, 0}, {4, 0, 4}, {0, 4, 4}}, budget);
  auto outside = prepare({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {0, 0, 4}}, budget);
  auto cube = prepare({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0},
                       {0, 0, 4}, {4, 0, 4}, {0, 4, 4}, {4, 4, 4}}, budget);
  auto extended = prepare({{10, 5, 5}, {9, 8, 5}, {5, 2, 1}, {1, 5, 8}, {9, 2, 5}}, budget);
  REQUIRE(regular.ok()); REQUIRE(outside.ok()); REQUIRE(cube.ok()); REQUIRE(extended.ok());
  CatalogueParams p; p.kmax = 3;
  auto a = build_catalogue(regular.value(), p, budget);
  auto b = build_catalogue(outside.value(), p, budget);
  auto c = build_catalogue(cube.value(), p, budget);
  auto d = build_catalogue(extended.value(), p, budget);
  REQUIRE(a.ok()); REQUIRE(b.ok()); REQUIRE(c.ok()); REQUIRE(d.ok());
  CHECK_EQ(a.value().ledger().q4_candidates, 1u);
  CHECK_EQ(a.value().ledger().q4_levels, 1u);
  CHECK_EQ(b.value().ledger().q4_candidates, 1u);
  CHECK_EQ(b.value().ledger().q4_levels, 0u);
  CHECK(c.value().ledger().q4_candidates > 0);
  CHECK_EQ(c.value().ledger().q4_levels, 0u);
  CHECK(d.value().ledger().q4_candidates > 1);
  CHECK_EQ(d.value().ledger().q4_levels, 1u);
  for (const Catalogue* cat : {&a.value(), &b.value(), &c.value(), &d.value()}) {
    u64 emitted_q4 = 0;
    for (const auto& ball : cat->balls_data()) emitted_q4 += ball.qmin == 4;
    CHECK_EQ(cat->ledger().q4_levels, emitted_q4);
    CHECK(cat->ledger().q4_candidates >= emitted_q4);
  }
  CHECK(level_is(a.value().levels().back(), 12));
}

MHGP11_TEST_MAIN()
