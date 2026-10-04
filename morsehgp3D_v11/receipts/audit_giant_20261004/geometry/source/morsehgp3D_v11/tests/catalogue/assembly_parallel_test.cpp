// Groupes analytiques, halo et representants non reduits ; aucune execution native locale de cette tranche.
#include "assembly_support.hpp"
#include "test.hpp"

using namespace assembly_test;
namespace {
void judge(const Fixture& f, bool indirect, sched::Pool* pool) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Input input; REQUIRE(input.load(f, budget).ok());
  const u64 held = budget.used();
  CatalogueParams params; params.parallel_assembly = true; params.indirect_sort = indirect;
  CatalogueLedger ledger; ledger.emitted = f.sorted.size(); ledger.incidences = f.population.size();
  auto result = cat::Assembly::finish(input.records, input.population, params, ledger, budget, nullptr, pool);
  REQUIRE(result.ok());
  const auto& value = result.value();
  CHECK_EQ(value.balls(), f.sorted.size()); CHECK(value.ledger() == ledger);
  CHECK_EQ(value.levels().size(), f.sorted.empty() ? 1u : (f.sorted.size() - 1) / f.group + 2);
  CHECK(same_level(value.levels()[0], num::Level{}));
  u64 offset = 0;
  bool balls = true, populations = true, levels = true;
  for (u32 i = 0; i < value.balls(); ++i) {
    auto expected = f.sorted[i].ball; expected.rank = LevelRank{i / f.group + 1};
    balls = balls && same_ball(value.balls_data()[i], expected);
    populations = populations && value.population_offsets()[i] == offset;
    if (i % f.group == 0) levels = levels && same_level(value.levels()[i / f.group + 1], f.sorted[i].level);
    for (u32 j = 0; j < expected.p + expected.m; ++j) {
      populations = populations && value.population()[offset] == SiteIdx{7 * i + j};
      ++offset;
    }
  }
  CHECK(balls); CHECK(populations); CHECK(levels);
  CHECK_EQ(value.population_offsets().back(), offset);
  CHECK_EQ(budget.used(), held + output_bytes(f));
  const u64 count = f.sorted.size(), grain = std::max(cat::kAssemblyGrain, (count + 1023) / 1024);
  const u64 plan_bytes = 32 * ((count + grain - 1) / grain);
  CHECK(budget.peak() >= budget.used() + plan_bytes + (indirect ? 4 * count : 0));
}

struct Nested {
  sched::Pool& pool;
  cat::AssemblyPlan& plan;
  Reason reason = Reason::none;
  static Outcome body(void* context, u64, u64, u32) noexcept {
    auto& self = *static_cast<Nested*>(context);
    self.reason = self.plan.scan(&self.pool).reason;
    return {};
  }
};
}

MHGP11_TEST(boundaries, 500) {
  auto one = sched::make_pool({1}), eight = sched::make_pool({8});
  REQUIRE(one.ok() && eight.ok());
  for (u32 count : {0u, 1u, 4095u, 4096u, 4097u, 8193u}) {
    for (u32 group : {1u, 4097u, 20000u}) {
      auto made = fixture(count, group); REQUIRE(made.ok());
      for (bool indirect : {false, true})
        for (sched::Pool* pool : {static_cast<sched::Pool*>(nullptr), one.value().get(), eight.value().get()})
          judge(made.value(), indirect, pool);
    }
  }
}

MHGP11_TEST(wide_levels, 60) {
  auto four = sched::make_pool({4}); REQUIRE(four.ok());
  for (u32 group : {17u, 4097u, 20000u}) {
    auto made = fixture(8193, group, true); REQUIRE(made.ok());
    for (bool indirect : {false, true}) judge(made.value(), indirect, four.value().get());
  }
}

MHGP11_TEST(refusals, 45) {
  auto eight = sched::make_pool({8}); REQUIRE(eight.ok());
  auto made = fixture(8193, 20000); REQUIRE(made.ok());
  for (u32 corruption = 0; corruption < 5; ++corruption) {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    Input input; REQUIRE(input.load(made.value(), budget, false).ok());
    const u64 held = budget.used();
    // Corruptions au debut du deuxieme bloc : elles exigent le vrai halo.
    if (corruption == 0) input.records[0].level = num::Level{};
    if (corruption == 1) input.records[4096].ball.support = input.records[4095].ball.support;
    if (corruption == 2) input.records[4096].level = num::Level{};
    if (corruption == 3) input.records[4096].population_begin = input.population.size();
    if (corruption == 4) ++input.records[4096].ball.m;
    {
      auto plan = cat::AssemblyPlan::make(input.records.span(), {}, input.population.size(), budget);
      REQUIRE(plan.ok()); CHECK_EQ(plan.value().blocks(), 3u);
      CHECK_EQ(plan.value().scan(eight.value().get()).reason, Reason::catalogue_invariant);
      CHECK_EQ(idx(input.records[8192].ball.rank), 0u);  // dernier bloc execute/joint meme apres refus ailleurs
    }
    CHECK_EQ(budget.used(), held);
    REQUIRE(input.load(made.value(), budget, false).ok());
    auto plan = cat::AssemblyPlan::make(input.records.span(), {}, input.population.size(), budget);
    REQUIRE(plan.ok()); CHECK(plan.value().scan(eight.value().get()).ok());
  }
}

MHGP11_TEST(memory_and_equivalence, 32) {
  auto four = sched::make_pool({4}); REQUIRE(four.ok());
  auto made = fixture(4097, 4097); REQUIRE(made.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited);
  Input input; REQUIRE(input.load(made.value(), owner).ok());
  const u64 exact = output_bytes(made.value()) + 64;
  for (u64 limit : {u64{63}, exact - 1, exact}) {
    MemoryBudget work(limit);
    CatalogueParams params; params.parallel_assembly = true;
    auto result = cat::Assembly::finish(input.records, input.population, params, {}, work, nullptr, four.value().get());
    CHECK_EQ(result.ok(), limit == exact);
    if (result.ok()) CHECK_EQ(work.used(), output_bytes(made.value()));
    else { CHECK_EQ(result.outcome().reason, Reason::memory_budget); CHECK(work.released().ok()); }
    CHECK(work.peak() <= limit);
  }
  auto points = cloud(owner); REQUIRE(points.ok());
  MemoryBudget work(MemoryBudget::kUnlimited);
  CatalogueParams params; params.kmax = 3;
  auto baseline = build_catalogue(points.value(), params, work); REQUIRE(baseline.ok());
  for (bool indirect : {false, true}) {
    params.indirect_sort = indirect; params.parallel_assembly = true;
    auto serial = build_catalogue(points.value(), params, work); REQUIRE(serial.ok());
    CHECK(same(serial.value(), baseline.value()));
    for (bool adaptive : {false, true}) {
      params.adaptive_frontier = adaptive;
      CatalogueTimings times;
      auto result = build_catalogue(points.value(), params, work, *four.value(), &times);
      REQUIRE(result.ok()); CHECK(same(result.value(), baseline.value()));
      CHECK_EQ(points.value().sites(), 5u);
    }
  }
}

MHGP11_TEST(pool_and_limits, 45) {
  for (u64 count : {u64{0}, u64{1}, u64{4096}, u64{4097}, u64{4194304}, u64{4194305}, u64{kNone} - 1}) {
    const u64 grain = cat::assembly_grain(count), blocks = cat::assembly_blocks(count);
    CHECK(grain >= 4096); CHECK(blocks <= 1024);
    CHECK(blocks * grain >= count);
    CHECK(count == 0 ? blocks == 0 : (blocks - 1) * grain < count);
  }
  auto eight = sched::make_pool({8}); REQUIRE(eight.ok());
  auto made = fixture(4097, 20000); REQUIRE(made.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(64);
  Input input; REQUIRE(input.load(made.value(), owner, false).ok());
  auto plan = cat::AssemblyPlan::make(input.records.span(), {}, input.population.size(), work); REQUIRE(plan.ok());
  CHECK_EQ(work.used(), 64u); CHECK_EQ(plan.value().blocks(), 2u);
  Nested nested{*eight.value(), plan.value()};
  REQUIRE(eight.value()->parallel_for(1, 1, &nested, Nested::body).ok());
  CHECK_EQ(nested.reason, Reason::pool_busy); CHECK_EQ(work.used(), 64u);
  REQUIRE(plan.value().scan(eight.value().get()).ok());
  CHECK_EQ(plan.value().levels(), 2u);
  CHECK_EQ(idx(input.records[4095].ball.rank), 1u); CHECK_EQ(idx(input.records[4096].ball.rank), 0u);
  std::vector<CatalogueBall> balls(input.records.size());
  std::vector<num::Level> levels(plan.value().levels());
  std::vector<u64> offsets(input.records.size() + 1);
  std::vector<SiteIdx> values(input.population.size());
  CHECK_EQ(plan.value().fill(input.population.span(), balls, levels, std::span(offsets).first(1), values,
                            eight.value().get()).reason, Reason::catalogue_invariant);
  REQUIRE(plan.value().fill(input.population.span(), balls, levels, offsets, values, eight.value().get()).ok());
  CHECK_EQ(idx(balls.back().rank), 1u); CHECK_EQ(offsets.back(), values.size());
  CHECK(same_level(levels[1], made.value().sorted[0].level)); CHECK_EQ(work.used(), 64u);
}

MHGP11_TEST_MAIN()
