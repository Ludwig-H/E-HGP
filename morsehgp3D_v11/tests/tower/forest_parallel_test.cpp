// Memes cellules, multifusions et verticales ; W ne choisit ni les traces ni les memos logiques.
#include "forest_parallel_support.hpp"
#include "test.hpp"
using namespace forest_parallel_test;

MHGP11_TEST(equivalence, 900) {
  auto p1 = sched::make_pool({1}), p4 = sched::make_pool({4}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok()); REQUIRE(p4.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,3> pools{p1.value().get(), p4.value().get(), p48.value().get()};
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{2,0,0},{4,0,0},{6,0,0}}, {{0,0,0},{4,0,0},{0,4,0},{4,4,0}},
    {{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}}};
  u64 regular = 0, extended = 0, hits = 0;
  for (const auto& points : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(points); const auto k = static_cast<Order>(points.size());
    auto baseline = tower_of(input, k, owner, work); REQUIRE(baseline.ok());
    const u64 held = work.used();
    for (u32 capacity : {1u, 2u, 4096u}) for (u64 memo : {u64{0}, u64{8}}) {
      std::array<ForestLedger,kMaxMebSites> expected{};
      std::array<OrderTimings,kMaxMebSites> counts_expected{};
      for (auto* pool : pools) {
        auto domain = domain_of(input, owner, k); REQUIRE(domain.ok());
        FullTimings times = sentinel();
        {
          auto made = build_full(std::move(domain.value()), work, &times, FullParams{memo,capacity,4,memo}, pool);
          REQUIRE(made.ok()); CHECK_EQ(domain.value().catalogue().kmax(), 0u);
          CHECK_EQ(times.regular_batch_capacity, capacity); CHECK_EQ(times.descent_lanes, 4u);
          CHECK_EQ(times.lane_memo_capacity, memo);
          CHECK_EQ(times.lane_memo_reserved_bytes, 4 * memo * DescentMemo::slot_bytes());
          u64 bytes = held;
          for (Order order = 1; order <= k; ++order) {
            const auto& f = made.value().order(order); const auto& l = f.ledger(); const auto& t = times.orders[order-1];
            CHECK(same(f, baseline.value().order(order))); CHECK(structure(f));
            CHECK(structural(l) == structural(baseline.value().order(order).ledger()));
            if (memo == 0) CHECK(l == baseline.value().order(order).ledger());
            else { CHECK(counts(l.descent)); CHECK_EQ(l.descent.memo.queries, l.trace_resolutions+l.vertical_descents); }
            CHECK(time_bounds(t, pool->size(), 4, capacity));
            CHECK_EQ(t.regular_cells + t.extended_cells, l.replayed_cells);
            CHECK(t.regular_traces <= l.trace_resolutions);
            CHECK((t.regular_cells == 0) == (t.regular_batches == 0));
            if (pool->size() == 1) { expected[order-1] = l; counts_expected[order-1] = t; }
            else {
              CHECK(l == expected[order-1]); const auto& e = counts_expected[order-1];
              CHECK_EQ(t.regular_batches, e.regular_batches); CHECK_EQ(t.regular_cells, e.regular_cells);
              CHECK_EQ(t.regular_traces, e.regular_traces); CHECK_EQ(t.extended_cells, e.extended_cells);
            }
            regular += t.regular_cells; extended += t.extended_cells; hits += l.descent.memo.hits;
            bytes += retained(f);
          }
          CHECK_EQ(work.used(), bytes); CHECK(times.orders[k] == OrderTimings{});
        }
        CHECK_EQ(work.used(), held);
      }
    }
  }
  CHECK(regular > 0); CHECK(extended > 0); CHECK(hits > 0);
}

MHGP11_TEST(plateaus, 45) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  const Input input({{0,0,0},{2,0,0},{4,0,0}});
  for (u32 capacity : {1u,2u,4u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(input, owner, 3); REQUIRE(domain.ok());
    FullTimings times;
    auto full = build_full(std::move(domain.value()), work, &times, FullParams{0,capacity,4,0}, pool.value().get());
    REQUIRE(full.ok());
    const auto& first = full.value().order(1); const auto& second = full.value().order(2);
    CHECK_EQ(first.nodes().size(), 4u); CHECK_EQ(first.nodes()[idx(first.root())].child_count, 3u);
    CHECK_EQ(first.ledger().plateaus, 1u); CHECK_EQ(times.orders[0].regular_cells, 2u);
    CHECK_EQ(times.orders[0].regular_traces, 4u);
    CHECK_EQ(times.orders[0].regular_batches, capacity == 1 ? 2u : 1u);
    CHECK_EQ(second.nodes().size(), 3u); CHECK_EQ(times.orders[1].extended_cells, 1u);
    for (NodeIdx image : second.lower()) CHECK_EQ(image, first.root());
    CHECK_EQ(full.value().order(3).lower()[0], second.root());
    CHECK(structure(first)); CHECK(structure(second));
  }
}

MHGP11_TEST(lanes48, 65) {
  // 63 cellules dans un lot : les 48 lanes sont toutes non vides, et 15 en resolvent deux.
  std::vector<Xyz> points;
  for (u32 i = 0; i < 64; ++i) points.push_back({2*i,0,0});
  const Input input(points);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto baseline = tower_of(input, 1, owner, work); REQUIRE(baseline.ok());
  for (u64 memo : {u64{0},u64{8}}) {
    ForestLedger expected;
    for (u32 workers : {1u,4u,48u}) {
      auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
      auto domain = domain_of(input, owner, 1); REQUIRE(domain.ok());
      FullTimings times;
      auto made = build_full(std::move(domain.value()), work, &times, FullParams{memo,64,48,memo}, pool.value().get());
      REQUIRE(made.ok()); const auto& f = made.value().order(1); const auto& l = f.ledger();
      CHECK(same(f, baseline.value().order(1))); CHECK(structural(l) == structural(baseline.value().order(1).ledger()));
      if (workers == 1) expected = l; else CHECK(l == expected);
      CHECK_EQ(f.nodes()[idx(f.root())].child_count, 64u); CHECK_EQ(times.orders[0].regular_cells, 63u);
      CHECK_EQ(times.orders[0].regular_batches, 1u); CHECK_EQ(times.orders[0].max_regular_batch, 63u);
      CHECK_EQ(times.orders[0].regular_traces, 126u); CHECK(time_bounds(times.orders[0], workers, 48, 64));
      CHECK_EQ(times.lane_memo_reserved_bytes, 48 * memo * DescentMemo::slot_bytes());
    }
  }
}

MHGP11_TEST(refusals, 60) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(line(), owner, 4); REQUIRE(domain.ok());
  const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
  const std::array<FullParams,8> invalid{{{0,0,2,0},{0,0,1,8},{0,4097,4,0},{0,2,0,0},
                                      {0,2,257,0},{0,2,4,3},{0,2,4,u64{1}<<63},{0,2,4,8}}};
  for (const auto& params : invalid) {
    auto times = sentinel(); const auto saved_times = times;
    auto refused = build_full(std::move(domain.value()), zero, &times, params, pool.value().get());
    CHECK(!refused.ok());
    const auto reason = params.lane_memo_capacity == (u64{1}<<63) ? Reason::tower_capacity :
                        params.regular_batch_capacity == 2 && params.lane_memo_capacity == 8 ?
                        Reason::memory_budget : Reason::parameter_out_of_range;
    CHECK_EQ(refused.outcome().reason, reason); CHECK(times == saved_times);
    CHECK(domain.value().index().cloud().x().data() == saved); CHECK_EQ(owner.used(), owners);
    CHECK(zero.released().ok());
  }
  CHECK_EQ(build_full(std::move(domain.value()), work, nullptr, FullParams{0,2,4,0}).outcome().reason,
           Reason::parameter_out_of_range);
  CHECK(work.released().ok());
  auto foreign = domain_of(line(), owner, 4); REQUIRE(foreign.ok());
  {
    auto context = ForestParallel::make(domain.value(), FullParams{0,2,4,0}, work, *pool.value());
    REQUIRE(context.ok()); const u64 held = work.used(); OrderTimings times{2,3,5,7}; const auto before = times;
    auto moved = std::move(context.value());
    CHECK(!context.value().belongs_to(domain.value(), work)); CHECK(moved.belongs_to(domain.value(), work));
    CHECK_EQ(build_forest(domain.value(), 2, work, &times, nullptr, &context.value()).outcome().reason,
             Reason::parameter_out_of_range);
    CHECK_EQ(build_forest(foreign.value(), 2, work, &times, nullptr, &moved).outcome().reason,
             Reason::parameter_out_of_range);
    CHECK(times == before); CHECK_EQ(work.used(), held);
  }
  CHECK(work.released().ok()); FullTimings times = sentinel();
  auto good = build_full(std::move(domain.value()), work, &times, {}, pool.value().get()); REQUIRE(good.ok());
  CHECK(inactive(times)); CHECK_EQ(good.value().kmax(), 4u);
}

MHGP11_TEST(mixed_plateau, 65) {
  // Faits recalcules independamment par forest_parallel_model.py / Definition, pas tires du mono.
  const Input input({{0,10,0},{2,10,0},{10,10,0},{11,9,0},{12,10,0},{11,11,0},{20,10,0},{22,10,0}});
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto baseline = tower_of(input, 1, owner, work); REQUIRE(baseline.ok());
  for (u32 q : {1u,2u,4096u}) {
    auto domain = domain_of(input, owner, 1); REQUIRE(domain.ok());
    CHECK_EQ(domain.value().catalogue().balls(), 9u);
    FullTimings times;
    auto made = build_full(std::move(domain.value()), work, &times, FullParams{0,q,4,0}, pool.value().get());
    REQUIRE(made.ok()); const auto& f = made.value().order(1); const auto& l = f.ledger();
    CHECK(same(f, baseline.value().order(1))); CHECK(l == baseline.value().order(1).ledger());
    CHECK_EQ(f.nodes().size(), 12u); CHECK_EQ(f.births(), 8u); CHECK_EQ(l.plateaus, 3u);
    CHECK_EQ(l.trace_resolutions, 20u); CHECK_EQ(l.continuations, 1u); CHECK_EQ(l.touched_components, 12u);
    CHECK_EQ(times.orders[0].regular_cells, 8u); CHECK_EQ(times.orders[0].extended_cells, 1u);
    CHECK_EQ(times.orders[0].regular_traces, 16u);
    const std::array<u32,4> arities{4,2,2,3}, ranks{1,2,2,3};
    for (u32 i = 0; i < 4; ++i) {
      CHECK_EQ(f.nodes()[8+i].child_count, arities[i]); CHECK_EQ(idx(f.nodes()[8+i].rank), ranks[i]);
    }
    CHECK(structure(f));
  }
}

namespace {
struct Nested {
  FullDomain& domain; MemoryBudget& work; sched::Pool& pool; FullTimings times = sentinel(); Outcome result{};
  static Outcome call(void* context, u64, u64, u32) noexcept {
    auto& self = *static_cast<Nested*>(context);
    auto made = build_full(std::move(self.domain), self.work, &self.times, FullParams{8,2,4,8}, &self.pool);
    self.result = made.outcome(); return {};
  }
};
}
MHGP11_TEST(pool_busy, 10) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(line(), owner, 4); REQUIRE(domain.ok());
  const auto* saved = domain.value().index().cloud().x().data(); const u64 held = owner.used();
  Nested nested{domain.value(),work,*pool.value()}; const auto times = nested.times;
  REQUIRE(pool.value()->parallel_for(1,1,&nested,Nested::call).ok());
  CHECK_EQ(nested.result.reason, Reason::pool_busy); CHECK(nested.times == times);
  CHECK(domain.value().index().cloud().x().data() == saved); CHECK_EQ(owner.used(), held);
  CHECK(work.released().ok());
  auto again = build_full(std::move(domain.value()), work, nullptr, FullParams{8,2,4,8}, pool.value().get());
  REQUIRE(again.ok()); CHECK_EQ(again.value().kmax(), 4u); CHECK(structure(again.value().order(1)));
}
MHGP11_TEST_MAIN()
