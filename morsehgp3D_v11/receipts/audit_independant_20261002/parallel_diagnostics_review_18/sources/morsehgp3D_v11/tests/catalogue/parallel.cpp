// Catalogue parallele : comparaison des valeurs/encodages exacts et des 17 compteurs avec le chemin sequentiel.
// Les tests de frontiere jugent aussi des listes possedees qui se recouvrent et leurs capacites simultanees.
#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "catalogue/frontier.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace detail = mhgp11::catalogue_detail;

namespace {
using Coordinates = std::array<u32, 3>;

Result<Cloud> prepare(const std::vector<Coordinates>& points, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (const auto& point : points) {
    x.push_back(point[0]); y.push_back(point[1]); z.push_back(point[2]);
    ids.push_back(PointId{1000 + static_cast<u32>(ids.size())});
  }
  return prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
}

std::vector<Coordinates> line(u32 count) {
  std::vector<Coordinates> points;
  for (u32 i = 0; i < count; ++i) points.push_back({i, 0, 0});
  return points;
}

// Encodage deterministe des champs exacts, sans memcmp des paddings des structures C++.
std::string canonical(const Catalogue& catalogue) {
  std::ostringstream out;
  out << catalogue.balls() << ':' << catalogue.levels().size() << ':';
  for (const auto& level : catalogue.levels())
    out << num::to_wide(level.numerator()).hex() << '/' << num::to_wide(level.denominator()).hex() << ';';
  for (const auto& ball : catalogue.balls_data()) {
    for (SiteIdx site : ball.support) out << idx(site) << ',';
    out << idx(ball.rank) << ',' << ball.p << ',' << ball.m << ',' << unsigned(ball.qmin) << ';';
  }
  for (u64 offset : catalogue.population_offsets()) out << offset << ',';
  out << ':';
  for (SiteIdx site : catalogue.population()) out << idx(site) << ',';
  return out.str();
}

std::array<u64, 14> timing_values(const CatalogueTimings& value) {
  return {value.prefix_ns, value.count_ns, value.replay_ns, value.fill_ns, value.sort_ns,
          value.level_scan_ns, value.allocation_ns, value.assembly_ns, value.count_task_sum_ns,
          value.count_task_max_ns, value.fill_task_sum_ns, value.fill_task_max_ns,
          value.sort_comparisons, value.tasks};
}

struct Fixture {
  std::vector<Coordinates> points;
  int order;
  u32 leaf;
};

std::vector<Fixture> fixtures() {
  const u32 m = kCoordMax;
  return {{line(5), 1, 4},
          {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0},
            {0, 0, 4}, {4, 0, 4}, {0, 4, 4}, {4, 4, 4}}, 3, 6},
          {{{0, 2, 2}, {4, 2, 2}, {2, 0, 2}, {2, 4, 2}, {2, 2, 0}, {2, 2, 4}, {2, 2, 2}}, 5, 8},
          {{{10, 5, 5}, {9, 8, 5}, {5, 2, 1}, {1, 5, 8}, {9, 2, 5}}, 4, 7},
          {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0}}, 2, 5},
          {{{0, 0, 0}, {m, m, 0}, {m, 0, m}, {0, m, m}}, 3, 6},
          {line(1025), 1, 4}};
}

}  // namespace

MHGP11_TEST(equivalence, 182) {
  for (const auto& fixture : fixtures()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), sequential(MemoryBudget::kUnlimited);
    auto cloud = prepare(fixture.points, owner);
    REQUIRE(cloud.ok());
    CatalogueParams params;
    params.kmax = fixture.order;
    params.leaf_size = fixture.leaf;
    auto baseline = build_catalogue(cloud.value(), params, sequential);
    REQUIRE(baseline.ok());
    const auto expected = canonical(baseline.value());
    for (u32 workers : {1u, 2u, 4u, 8u}) {
      auto pool = sched::make_pool({workers});
      REQUIRE(pool.ok());
      MemoryBudget work(MemoryBudget::kUnlimited);
      {
        auto actual = build_catalogue(cloud.value(), params, work, *pool.value());
        REQUIRE(actual.ok());
        CHECK_EQ(canonical(actual.value()), expected);
        CHECK(actual.value().ledger() == baseline.value().ledger());
        CHECK_EQ(actual.value().kmax(), baseline.value().kmax());
      }
      CHECK(work.released().ok());
    }
  }
}

MHGP11_TEST(frontier_overlap, 27) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(5), owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  detail::Workspace workspace;
  detail::Collector collector;
  detail::Run first{cloud.value(), params, work, workspace, collector, {}};
  detail::Frontier frontier;
  REQUIRE(frontier.prepare(first, 1).ok());
  CHECK_EQ(frontier.size(), 2u);
  CHECK_EQ(first.ledger.nodes, 3u);
  CHECK_EQ(first.ledger.leaves, 0u);  // les feuilles appartiennent aux suffixes, pas au preambule
  u64 logical = 0, capacity = 0;
  for (u32 ordinal = 0; ordinal < frontier.size(); ++ordinal) {
    const auto& task = frontier.task(ordinal);
    CHECK_EQ(task.depth, 1u);
    CHECK_EQ(task.count, 3u);
    CHECK_EQ(task.storage.size(), 5u);
    logical += task.count;
    capacity += task.storage.size();
  }
  CHECK_EQ(logical, 6u);
  CHECK_EQ(capacity, 10u);
  CHECK_EQ(frontier.task(0).sites().back(), frontier.task(1).sites().front());
  CHECK_EQ(idx(frontier.task(0).sites().back()), 2u);
  u64 owned = 0, bound = 0;
  CHECK(frontier.owned_bytes(owned).ok());
  CHECK_EQ(owned, 40u);
  CHECK_EQ(work.used(), owned);
  CHECK(frontier.verify_memory_bound(bound).ok());
  CHECK_EQ(bound, 60u);
  detail::Run second{cloud.value(), params, work, workspace, collector, {}};
  CHECK(frontier.verify(second).ok());
  CHECK(first.ledger == second.ledger);
  CHECK_EQ(work.used(), owned);
  CHECK_EQ(work.peak(), 100u);  // 40 listes gardees + 20 racine + 20 parent + 20 enfant du rejeu
  CHECK(frontier.suffix_memory_bound(8, bound).ok());
  CHECK_EQ(bound, 0u);  // deux feuilles deja preparees, aucun Buffer descendant
  CHECK_EQ(frontier.suffix_memory_bound(0, bound).reason, Reason::parameter_out_of_range);
}

MHGP11_TEST(frontier_edges, 31) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(5), owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  detail::Workspace workspace;
  detail::Collector collector;
  detail::Run run{cloud.value(), params, work, workspace, collector, {}};
  detail::Frontier frontier;
  u64 bound = 99;
  CHECK(detail::frontier_memory_bound(5, 0, bound).ok());
  CHECK_EQ(bound, 60u);
  const u32 sites_max = std::numeric_limits<u32>::max();
  CHECK(detail::frontier_memory_bound(sites_max, 8, bound).ok());
  CHECK_EQ(bound, u64(sites_max) * 4 * 266);
  CHECK_EQ(detail::frontier_memory_bound(5, 9, bound).reason, Reason::parameter_out_of_range);
  CHECK_EQ(bound, 0u);
  CHECK_EQ(frontier.prepare(run, 9).reason, Reason::parameter_out_of_range);
  CHECK_EQ(frontier.size(), 0u);
  CHECK(run.ledger == CatalogueLedger{});
  CHECK_EQ(work.used(), 0u);
  run.ledger.nodes = 1;
  CHECK_EQ(frontier.prepare(run, 0).reason, Reason::catalogue_invariant);
  CHECK_EQ(run.ledger.nodes, 1u);
  CHECK_EQ(work.used(), 0u);
  run.ledger = {};
  REQUIRE(frontier.prepare(run, 0).ok());
  CHECK_EQ(frontier.size(), 1u);
  CHECK_EQ(frontier.task(0).depth, 0u);
  CHECK_EQ(frontier.task(0).count, 5u);
  CHECK_EQ(run.ledger.nodes, 1u);
  u64 owned = 0;
  CHECK(frontier.owned_bytes(owned).ok());
  CHECK_EQ(owned, 20u);
  CHECK(frontier.verify_memory_bound(bound).ok());
  CHECK_EQ(bound, 40u);
  CHECK(frontier.suffix_memory_bound(8, bound).ok());
  CHECK_EQ(bound, u64{4} * 5 * 3 * kCoordBits);
  detail::Run replay{cloud.value(), params, work, workspace, collector, {}};
  replay.ledger.nodes = 1;
  CHECK_EQ(frontier.verify(replay).reason, Reason::catalogue_invariant);
  CHECK_EQ(replay.ledger.nodes, 1u);
  replay.ledger = {};
  CHECK(frontier.verify(replay).ok());
  CHECK(replay.ledger == run.ledger);
  CHECK_EQ(work.used(), owned);
  CHECK_EQ(work.peak(), 60u);  // 20 gardes + 20 racine + 20 noeud du rejeu d0
}

MHGP11_TEST(frontier_deep, 18) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(1025), owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  detail::Workspace workspace;
  detail::Collector collector;
  detail::Run run{cloud.value(), params, work, workspace, collector, {}};
  detail::Frontier frontier;
  REQUIRE(frontier.prepare(run).ok());
  CHECK_EQ(frontier.size(), 256u);
  CHECK_EQ(run.ledger.max_depth, 8u);
  std::vector<u64> bounds;
  for (u32 i = 0; i < frontier.size(); ++i) {
    const auto& task = frontier.task(i);
    i64 width = 0;
    for (int axis = 0; axis < 3; ++axis) width = std::max(width, task.box.hi[axis] - task.box.lo[axis]);
    if (task.count > params.leaf_size && width > 1)
      bounds.push_back(4 * u64(task.count) * (3 * kCoordBits - task.depth));
  }
  CHECK(!bounds.empty());
  std::sort(bounds.rbegin(), bounds.rend());
  for (u32 workers : {1u, 2u, 4u, 8u}) {
    u64 expected = 0, actual = 0;
    for (u32 i = 0; i < workers && i < bounds.size(); ++i) expected += bounds[i];
    CHECK(frontier.suffix_memory_bound(workers, actual).ok());
    CHECK_EQ(actual, expected);
  }
  detail::Run replay{cloud.value(), params, work, workspace, collector, {}};
  CHECK(frontier.verify(replay).ok());
  CHECK(replay.ledger == frontier.ledger());
  CatalogueParams other = params;
  other.kmax = 2;
  detail::Run mismatched{cloud.value(), other, work, workspace, collector, {}};
  CHECK_EQ(frontier.verify(mismatched).reason, Reason::catalogue_invariant);
  CHECK_EQ(frontier.execute_task(frontier.size(), run).reason, Reason::catalogue_invariant);
  CHECK_EQ(frontier.prepare(run).reason, Reason::catalogue_invariant);
}

MHGP11_TEST(global_limits, 48) {
  MemoryBudget owner(MemoryBudget::kUnlimited), baseline_budget(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(1025), owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  auto baseline = build_catalogue(cloud.value(), params, baseline_budget);
  REQUIRE(baseline.ok());
  const u64 nodes = baseline.value().ledger().nodes, balls = baseline.value().balls();
  CHECK(nodes > 512);
  CHECK_EQ(balls, 1024u);
  const auto encoded = canonical(baseline.value());
  for (u32 workers : {1u, 2u, 4u, 8u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget work(MemoryBudget::kUnlimited);
    for (bool allowed : {false, true}) {
      params.max_nodes = allowed ? nodes : nodes - 1;
      auto result = build_catalogue(cloud.value(), params, work, *pool.value());
      CHECK_EQ(result.ok(), allowed);
      if (result.ok()) CHECK_EQ(canonical(result.value()), encoded);
      else CHECK_EQ(result.outcome().reason, Reason::node_budget);
    }
    params.max_nodes = 0;
    for (bool allowed : {false, true}) {
      params.ball_limit = balls + (allowed ? 1 : 0);
      auto result = build_catalogue(cloud.value(), params, work, *pool.value());
      CHECK_EQ(result.ok(), allowed);
      if (result.ok()) CHECK_EQ(canonical(result.value()), encoded);
      else CHECK_EQ(result.outcome().reason, Reason::index_overflow_u32);
    }
    params.ball_limit = kNone;
    CHECK(work.released().ok());
    CHECK(pool.value()->parallel_for(0, 1, nullptr, [](void*, u64, u64, u32) -> Outcome { return {}; }).ok());
  }
}

MHGP11_TEST(memory_and_refusals, 32) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(17), owner);
  auto duplicate = prepare({{0, 0, 0}, {0, 0, 0}}, owner);
  REQUIRE(cloud.ok()); REQUIRE(duplicate.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  for (u32 workers : {1u, 4u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget small(68);
    Buffer<u32> sentinel;
    REQUIRE(sentinel.allocate(17, small).ok());
    sentinel[0] = 123;
    CHECK_EQ(build_catalogue(cloud.value(), params, small, *pool.value()).outcome().reason, Reason::memory_budget);
    CHECK_EQ(small.used(), 68u);
    CHECK_EQ(sentinel[0], 123u);
    CHECK_EQ(build_catalogue(duplicate.value(), params, small, *pool.value()).outcome().reason,
             Reason::multiplicity_unsupported);
    params.kmax = 0;
    CHECK_EQ(build_catalogue(duplicate.value(), params, small, *pool.value()).outcome().reason, Reason::kmax_out_of_range);
    params.kmax = 1;
    MemoryBudget work(MemoryBudget::kUnlimited);
    u64 first_bytes = 0;
    {
      auto first = build_catalogue(cloud.value(), params, work, *pool.value());
      REQUIRE(first.ok());
      first_bytes = work.used();
      const auto encoded = canonical(first.value());
      auto second = build_catalogue(cloud.value(), params, work, *pool.value());
      REQUIRE(second.ok());
      CHECK_EQ(work.used(), 2 * first_bytes);
      CHECK_EQ(canonical(second.value()), encoded);
      CHECK(work.peak() >= work.used());
    }
    CHECK(first_bytes > 0);
    CHECK(work.released().ok());
    sentinel.reset();
    CHECK(small.released().ok());
  }
}

MHGP11_TEST(timings, 35) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(17), owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4;
  for (u32 workers : {1u, 4u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget work(MemoryBudget::kUnlimited), denied(0);
    auto plain = build_catalogue(cloud.value(), params, work, *pool.value());
    REQUIRE(plain.ok());
    CatalogueTimings timing;
    timing.sort_comparisons = std::numeric_limits<u64>::max();
    auto measured = build_catalogue(cloud.value(), params, work, *pool.value(), &timing);
    REQUIRE(measured.ok());
    CHECK_EQ(canonical(measured.value()), canonical(plain.value()));
    CHECK(measured.value().ledger() == plain.value().ledger());
    CHECK(timing.sort_comparisons > 0 && timing.sort_comparisons < std::numeric_limits<u64>::max());
    CHECK(timing.tasks > 0 && timing.tasks <= detail::kFrontierTasks);
    CHECK(timing.count_task_max_ns <= timing.count_task_sum_ns);
    CHECK(timing.fill_task_max_ns <= timing.fill_task_sum_ns);
    CHECK(timing.count_task_max_ns <= timing.count_ns);
    CHECK(timing.fill_task_max_ns <= timing.fill_ns);
    const auto kept = timing_values(timing);
    params.kmax = 0;
    CHECK_EQ(build_catalogue(cloud.value(), params, work, *pool.value(), &timing).outcome().reason,
             Reason::kmax_out_of_range);
    CHECK(timing_values(timing) == kept);
    params.kmax = 1; params.ball_limit = 1;
    CHECK_EQ(build_catalogue(cloud.value(), params, work, *pool.value(), &timing).outcome().reason,
             Reason::index_overflow_u32);
    CHECK(timing_values(timing) == kept);
    params.ball_limit = kNone;
    CHECK_EQ(build_catalogue(cloud.value(), params, denied, *pool.value(), &timing).outcome().reason,
             Reason::memory_budget);
    CHECK(timing_values(timing) == kept);
  }
}

MHGP11_TEST_MAIN()
