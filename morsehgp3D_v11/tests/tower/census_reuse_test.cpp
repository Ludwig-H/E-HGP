// Memes transitions et dates : geometrie Fraction dans la porte separee, owned reference ici explicite.
#include "census_reuse_support.hpp"
#include "test.hpp"
using namespace census_reuse_test;

MHGP11_TEST(descents, 500) {
  const u32 high = kCoordMax;
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{4,0,0},{5,0,0},{11,0,0}},
    {{0,0,0},{4,0,0},{0,4,0},{4,4,0},{2,2,0}},
    {{1,2,0},{0,5,0},{8,1,0},{8,9,0},{9,8,0}},
    {{10,5,5},{9,8,5},{5,2,1},{1,5,8},{9,2,5}},
    {{0,0,0},{high,high,0},{high,0,high},{0,high,high}}};
  u64 calls = 0, hits = 0, saturated = 0;
  for (const auto& points : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
    auto domain = domain_of(Input(points),owner,4); REQUIRE(domain.ok());
    auto scratch = CensusWorkspace::make(domain.value().index(),work); REQUIRE(scratch.ok());
    const u64 held = work.used(); CHECK_EQ(held,4*points.size());
    for (const auto& part : parts(static_cast<u32>(points.size()))) {
      const u32 k = static_cast<u32>(part.size());
      auto original = descend(domain.value(),part,k,work); REQUIRE(original.ok());
      auto borrowed = descend(domain.value(),part,k,zero,scratch.value().get()); REQUIRE(borrowed.ok());
      CHECK(answer(original.value(),borrowed.value()));
      CHECK(same_population_work(original.value().ledger(),borrowed.value().ledger()));
      CHECK_EQ(work.used(),held); CHECK_EQ(zero.peak(),0u);
      calls += borrowed.value().ledger().census_calls; hits += borrowed.value().ledger().catalogue_hits;
      auto a = descent_step(domain.value(),part,k,work);
      auto b = descent_step(domain.value(),part,k,zero,scratch.value().get()); REQUIRE(a.ok()); REQUIRE(b.ok());
      CHECK(a.value().seed() == b.value().seed()); CHECK(equal(a.value().next().part(),b.value().next().part()));
      CHECK(level_bytes(a.value().level(),b.value().level()));
      saturated += b.value().ledger().interior_steps && b.value().ledger().census_calls ? 1u : 0u;
    }
  }
  CHECK(calls > 0); CHECK(hits > 0); CHECK(saturated > 0);
}

MHGP11_TEST(memo_identity, 45) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  const Input input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}});
  auto domain = domain_of(input,owner,2), other = domain_of(input,owner,2); REQUIRE(domain.ok()); REQUIRE(other.ok());
  auto scratch = CensusWorkspace::make(domain.value().index(),work);
  auto foreign = CensusWorkspace::make(other.value().index(),work); REQUIRE(scratch.ok()); REQUIRE(foreign.ok());
  const auto part = part_of(domain.value(),{{0,0,0},{11,0,0}});
  const auto hit = part_of(domain.value(),{{4,0,0},{5,0,0}});
  auto original = descend(domain.value(),part,2,work); REQUIRE(original.ok());
  for (u64 capacity : {u64{0},u64{1},u64{64}}) {
    CHECK_EQ(DescentMemo::make(domain.value(),capacity,work,foreign.value().get()).outcome().reason,
             Reason::parameter_out_of_range);
    auto made = DescentMemo::make(domain.value(),capacity,work,scratch.value().get()); REQUIRE(made.ok());
    DescentMemo moved(std::move(made.value()));
    CHECK_EQ(made.value().resolve(domain.value(),part,2,zero).outcome().reason,Reason::parameter_out_of_range);
    auto a = moved.resolve(domain.value(),part,2,zero); REQUIRE(a.ok()); CHECK(answer(a.value(),original.value()));
    CHECK_EQ(a.value().ledger().census.passes,a.value().ledger().census_calls);
    auto b = moved.resolve(domain.value(),part,2,zero); REQUIRE(b.ok()); CHECK(answer(a.value(),b.value()));
    if (capacity != 0) { CHECK_EQ(b.value().ledger().memo.hits,1u); CHECK_EQ(b.value().ledger().steps,0u); }
    else { CHECK(b.value().ledger().memo == MemoLedger{}); CHECK(a.value().ledger() == b.value().ledger()); }
    CHECK_EQ(moved.resolve(domain.value(),part,2,zero,foreign.value().get()).outcome().reason,Reason::parameter_out_of_range);
    CHECK_EQ(moved.resolve(other.value(),part,2,zero).outcome().reason,Reason::parameter_out_of_range);
    CHECK_EQ(descend(domain.value(),hit,2,zero,foreign.value().get()).outcome().reason,Reason::parameter_out_of_range);
  }
  auto saved = descend(domain.value(),part,2,zero,scratch.value().get()); REQUIRE(saved.ok());
  scratch.value().reset(); CHECK(answer(saved.value(),original.value())); CHECK(valid_terminal(domain.value(),saved.value()));
  CHECK_EQ(zero.peak(),0u);
}

MHGP11_TEST(full, 1200) {
  auto p1 = sched::make_pool({1}), p4 = sched::make_pool({4}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok()); REQUIRE(p4.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,3> pools{p1.value().get(),p4.value().get(),p48.value().get()};
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{2,0,0},{4,0,0},{6,0,0}}, {{0,1,0},{1,0,0},{2,1,0},{1,2,0}},
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}};
  u64 census_calls = 0;
  for (const auto& points : fixtures) for (u64 memo : {u64{0},u64{8}}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(points);
    for (u32 q : {0u,1u,2u,64u}) for (auto* pool : pools) {
      if (q == 0 && pool->size() != 1) continue;
      FullParams params{memo,q,q == 0 ? 1u : 48u,q == 0 ? 0 : memo,q != 0};
      auto a = domain_of(input,owner,4), b = domain_of(input,owner,4); REQUIRE(a.ok()); REQUIRE(b.ok());
      FullTimings at, bt = scratch_sentinel();
      auto owned = build_full(std::move(a.value()),work,&at,params,pool); REQUIRE(owned.ok());
      params.reuse_census_workspace = true;
      const u64 held = work.used();
      auto borrowed = build_full(std::move(b.value()),work,&bt,params,pool); REQUIRE(borrowed.ok());
      const u32 count = q == 0 ? 1 : std::min({pool->size(),48u,q});
      CHECK_EQ(bt.census_workspaces,count); CHECK_EQ(bt.census_workspace_reserved_bytes,4*points.size()*count);
      CHECK_EQ(at.census_workspaces,0u); CHECK_EQ(at.census_workspace_reserved_bytes,0u);
      u64 bytes = held;
      for (Order k = 1; k <= 4; ++k) {
        const auto& x = owned.value().order(k); const auto& y = borrowed.value().order(k);
        CHECK(same(x,y)); CHECK(x.ledger() == twice_census(y.ledger())); CHECK(structure(y));
        CHECK_EQ(y.ledger().descent.census.passes,y.ledger().descent.census_calls);
        bytes += retained(y); census_calls += y.ledger().descent.census_calls;
      }
      CHECK_EQ(work.used(),bytes);  // Aucun scratch ni memo conserve dans FullTower.
      CHECK_EQ(b.value().catalogue().kmax(),0u);
    }
  }
  CHECK(census_calls > 0);
}

MHGP11_TEST(admission, 25) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(line(),owner,4); REQUIRE(domain.ok());
  auto pool = sched::make_pool({48}); REQUIRE(pool.ok());
  const FullParams params{0,1,48,0,true,true};
  const auto* saved = domain.value().index().cloud().x().data();
  auto times = scratch_sentinel(); const auto before = times;
  MemoryBudget short_one(15), exact_one(16);
  CHECK_EQ(CensusSlots::make(domain.value(),1,short_one).outcome().reason,Reason::memory_budget);
  CHECK_EQ(short_one.peak(),0u); CHECK(short_one.released().ok());
  {
    auto slots = CensusSlots::make(domain.value(),1,exact_one); REQUIRE(slots.ok());
    CHECK_EQ(slots.value().size(),1u); CHECK_EQ(slots.value().reserved_bytes(),16u);
    auto* address = slots.value().get(0); CensusSlots moved(std::move(slots.value()));
    CHECK(moved.get(0) == address); CHECK(slots.value().get(0) == nullptr);
    CHECK(moved.belongs_to(domain.value(),exact_one)); CHECK(!moved.belongs_to(domain.value(),work));
  }
  CHECK(exact_one.released().ok());
  CHECK_EQ(build_full(std::move(domain.value()),short_one,&times,params,pool.value().get()).outcome().reason,
           Reason::memory_budget);
  CHECK(times == before); CHECK(domain.value().index().cloud().x().data() == saved);
  CHECK(short_one.released().ok());
  u64 peak = 0;
  {
    auto probe = domain_of(line(),owner,4); REQUIRE(probe.ok());
    auto tower = build_full(std::move(probe.value()),work,nullptr,params,pool.value().get()); REQUIRE(tower.ok());
    peak = work.peak();
  }
  CHECK(work.released().ok()); MemoryBudget exact(peak), under(peak-1);
  CHECK_EQ(build_full(std::move(domain.value()),under,&times,params,pool.value().get()).outcome().reason,Reason::memory_budget);
  CHECK(times == before); CHECK(under.released().ok()); CHECK(domain.value().index().cloud().x().data() == saved);
  auto recovered = build_full(std::move(domain.value()),exact,&times,params,pool.value().get()); REQUIRE(recovered.ok());
  CHECK_EQ(exact.peak(),peak); CHECK_EQ(times.census_workspaces,1u);
}
MHGP11_TEST_MAIN()
