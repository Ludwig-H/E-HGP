// Frontiere adaptative : egalite exacte, plan possede, coexistence, fantomes et refus transactionnels.
#include "adaptive_support.hpp"
#include "catalogue/adaptive_frontier.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace adaptive_test;
namespace cd = mhgp11::catalogue_detail;

MHGP11_TEST(equivalence, 800) {
  struct Fixture { std::vector<Position> points; int k; u32 leaf; };
  const u32 m = kCoordMax;
  const std::vector<Fixture> fixtures{
      {line(5),1,4}, {ghost(),1,4}, {line(1025),1,4},
      {{{10,5,5},{9,8,5},{5,2,1},{1,5,8},{9,2,5}},4,7},
      {{{0,2,2},{4,2,2},{2,0,2},{2,4,2},{2,2,0},{2,2,4},{2,2,2}},5,8},
      {{{0,0,0},{m,m,0},{m,0,m},{0,m,m}},3,6}};
  for (const auto& f : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited);
    auto cloud = prepare(f.points, owner);
    REQUIRE(cloud.ok());
    for (u32 option = 0; option < 4; ++option) {
      CatalogueParams p;
      p.kmax = f.k; p.leaf_size = f.leaf;
      p.cache_center_lines = (option & 1) != 0; p.indirect_sort = (option & 2) != 0;
      MemoryBudget reference_budget(MemoryBudget::kUnlimited);
      auto reference = build_catalogue(cloud.value(), p, reference_budget);
      REQUIRE(reference.ok());
      const auto expected = canonical(reference.value());
      std::string first_plan;
      for (u32 workers : {1u, 4u}) {
        auto pool = sched::make_pool({workers});
        REQUIRE(pool.ok());
        MemoryBudget work(MemoryBudget::kUnlimited);
        {
          CatalogueDiagnostics diagnostic;
          CatalogueTimings timings;
          p.adaptive_frontier = true;
          auto actual = build_catalogue(cloud.value(), p, work, *pool.value(), &timings, &diagnostic);
          REQUIRE(actual.ok());
          CHECK_EQ(canonical(actual.value()), expected);
          CHECK(actual.value().ledger() == reference.value().ledger());
          CHECK(diagnostic.planning().adaptive);
          CHECK_EQ(diagnostic.planning().plan_nodes, 2 * diagnostic.planning().plan_leaves - 1);
          CHECK_EQ(diagnostic.tasks().size() + diagnostic.planning().empty_leaves, diagnostic.planning().plan_leaves);
          CHECK(diagnostic.planning().plan_leaves <= cd::kAdaptiveTasks);
          CHECK_EQ(diagnostic.tasks().size(), timings.tasks);
          CHECK(!diagnostic.planning().memory_fallback);
          if (workers == 1) first_plan = plan(diagnostic); else CHECK_EQ(plan(diagnostic), first_plan);
          bool descriptions = true, ordered = true;
          u64 count_time = 0, fill_time = 0, emitted = 0, incidences = 0;
          std::array<u64,2> previous{};
          for (const auto& t : diagnostic.tasks()) {
            descriptions = descriptions && t.path_known && t.inside_known && t.count <= t.capacity && t.inside <= t.count;
            ordered = ordered && previous <= t.path; previous = t.path;
            count_time += t.count_ns; fill_time += t.fill_ns;
            emitted += t.ledger.emitted; incidences += t.ledger.incidences;
          }
          CHECK(descriptions && ordered);
          CHECK_EQ(count_time, timings.count_task_sum_ns);
          CHECK_EQ(fill_time, timings.fill_task_sum_ns);
          CHECK_EQ(emitted, actual.value().balls());
          CHECK_EQ(incidences, actual.value().population().size());
        }
        CHECK(work.released().ok());
      }
    }
  }
}

MHGP11_TEST(round_memory, 38) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare(line(5), owner);
  auto pool = sched::make_pool({4});
  REQUIRE(cloud.ok()); REQUIRE(pool.ok());
  CatalogueParams p; p.kmax = 1; p.leaf_size = 4; p.adaptive_frontier = true;
  cd::Workspace unused; cd::Collector collector;
  for (u64 limit : {40u, 60u, 100u}) {
    MemoryBudget work(limit);
    {
      cd::Run first{cloud.value(),p,work,unused,collector,{}};
      cd::AdaptiveFrontier front;
      REQUIRE(front.prepare(first,*pool.value()).ok());
      const bool fallback = limit == 40;
      CHECK_EQ(front.planning().memory_fallback, fallback);
      CHECK_EQ(front.size(), fallback ? 1u : 2u);
      CHECK_EQ(front.planning().plan_nodes, fallback ? 1u : 3u);
      CHECK_EQ(front.planning().priority_tests, fallback ? 0u : 6u);
      CHECK_EQ(work.used(), fallback ? 20u : 40u);
      CHECK_EQ(work.peak(), fallback ? 40u : 60u);
      u64 replay_bytes = 0;
      CHECK(front.verify_memory_bound(replay_bytes).ok());
      CHECK_EQ(replay_bytes, fallback ? 40u : 60u);
      cd::Run second{cloud.value(),p,work,unused,collector,{}};
      const auto replay = front.verify(second,*pool.value());
      if (limit == 100) { CHECK(replay.ok()); CHECK(second.ledger == first.ledger); }
      else CHECK_EQ(replay.reason, Reason::memory_budget);
      CHECK_EQ(work.used(), fallback ? 20u : 40u);
    }
    CHECK(work.released().ok());
  }
}

MHGP11_TEST(plan_limits, 32) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  for (bool capped : {false, true}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto cloud = prepare(capped ? line(4097) : ghost(), owner);
    REQUIRE(cloud.ok());
    CatalogueParams p; p.kmax = 1; p.leaf_size = 4;
    cd::Workspace unused; cd::Collector collector;
    {
      cd::Run first{cloud.value(),p,work,unused,collector,{}};
      cd::AdaptiveFrontier front;
      REQUIRE(front.prepare(first,*pool.value()).ok());
      CHECK_EQ(first.ledger.nodes, front.planning().plan_nodes);
      CHECK_EQ(first.ledger.leaves, 0u);
      CHECK_EQ(front.size() + front.planning().empty_leaves, front.planning().plan_leaves);
      CHECK(front.planning().rounds > 0 && front.planning().rounds <= cd::kMaxDepth);
      if (capped) {
        CHECK_EQ(front.planning().plan_leaves, 1024u);
        CHECK_EQ(front.planning().plan_nodes, 2047u);
        u64 suffix = 0;
        CHECK(front.suffix_memory_bound(4,suffix).ok()); CHECK(suffix > 0);
      } else {
        CHECK_EQ(front.planning().plan_nodes, 75u);
        CHECK_EQ(front.planning().empty_leaves, 1u);
        CHECK_EQ(front.size(), 37u);
        CHECK(first.ledger.max_depth > 8);
      }
      const u64 base = work.used();
      cd::Run second{cloud.value(),p,work,unused,collector,{}};
      CHECK(front.verify(second,*pool.value()).ok()); CHECK(first.ledger == second.ledger);
      CHECK_EQ(work.used(), base);
      u64 bound = 0;
      CHECK_EQ(front.suffix_memory_bound(0,bound).reason, Reason::parameter_out_of_range);
      CHECK_EQ(front.suffix_memory_bound(257,bound).reason, Reason::parameter_out_of_range);
      CHECK_EQ(front.execute_task(front.size(),second).reason, Reason::catalogue_invariant);
      CHECK_EQ(front.prepare(second,*pool.value()).reason, Reason::catalogue_invariant);
    }
    CHECK(work.released().ok());
  }
}

MHGP11_TEST(diagnostics_transaction, 36) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto cloud = prepare(line(17),owner);
  auto pool = sched::make_pool({4});
  REQUIRE(cloud.ok()); REQUIRE(pool.ok());
  CatalogueParams p; p.kmax = 1; p.leaf_size = 4;
  CatalogueDiagnostics diagnostic;
  CatalogueTimings timing;
  u64 nodes = 0, balls = 0;
  {
    auto result = build_catalogue(cloud.value(),p,work,*pool.value(),&timing,&diagnostic);
    REQUIRE(result.ok());
    nodes = result.value().ledger().nodes; balls = result.value().balls();
    CHECK(!diagnostic.planning().adaptive);
    for (const auto& t : diagnostic.tasks()) CHECK(!t.path_known && !t.inside_known);
  }
  const auto old = plan(diagnostic);
  const auto* pointer = diagnostic.tasks().data();
  const u64 retained = work.used(), old_prefix = timing.prefix_ns;
  CHECK_EQ(retained, diagnostic.tasks().size() * sizeof(CatalogueTaskDiagnostic));
  p.adaptive_frontier = true;
  for (u32 refusal = 0; refusal < 3; ++refusal) {
    p.max_nodes = refusal == 0 ? nodes - 1 : 0;
    p.ball_limit = refusal == 1 ? balls : kNone;
    auto result = build_catalogue(cloud.value(),p,refusal == 2 ? zero : work,*pool.value(),&timing,&diagnostic);
    CHECK(!result.ok());
    CHECK_EQ(result.outcome().reason, refusal == 0 ? Reason::node_budget :
             refusal == 1 ? Reason::index_overflow_u32 : Reason::memory_budget);
    CHECK_EQ(plan(diagnostic),old); CHECK(diagnostic.tasks().data() == pointer);
    CHECK_EQ(timing.prefix_ns,old_prefix); CHECK_EQ(work.used(),retained); CHECK(zero.released().ok());
  }
  p.max_nodes = nodes; p.ball_limit = balls + 1;
  {
    auto result = build_catalogue(cloud.value(),p,work,*pool.value(),&timing,&diagnostic);
    REQUIRE(result.ok()); CHECK_EQ(result.value().balls(),balls);
    CHECK(diagnostic.planning().adaptive); CHECK(diagnostic.tasks().data() != pointer);
    CHECK(work.peak() >= retained + diagnostic.tasks().size() * sizeof(CatalogueTaskDiagnostic));
  }
  CHECK_EQ(work.used(),diagnostic.tasks().size() * sizeof(CatalogueTaskDiagnostic));
  CatalogueDiagnostics moved(std::move(diagnostic));
  CHECK(diagnostic.tasks().empty()); CHECK(!moved.tasks().empty());
  CatalogueDiagnostics empty; moved.swap(empty);
  CHECK(moved.tasks().empty()); CHECK(!empty.tasks().empty());
}

MHGP11_TEST_MAIN()
