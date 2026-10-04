// Resolution privee des naissances puis balayage ferme pilote, toutes images enfants conservees.
#include "forest_vertical_parallel_support.hpp"
#include "test.hpp"
using namespace forest_vertical_test;

MHGP11_TEST(equivalence, 3000) {
  auto p1 = sched::make_pool({1}), p4 = sched::make_pool({4}), p48 = sched::make_pool({48});
  REQUIRE(p1.ok()); REQUIRE(p4.ok()); REQUIRE(p48.ok());
  const std::array<sched::Pool*,3> pools{p1.value().get(),p4.value().get(),p48.value().get()};
  const u32 high = (u32{1} << kCoordBits) - 1;
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{2,0,0},{4,0,0}}, {{0,0,0},{4,0,0},{0,4,0},{4,4,0}},
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}},
    {{0,0,0},{high,high,0},{high,0,high},{0,high,high}}};
  u64 batches = 0, hits = 0, checks = 0;
  for (const auto& points : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(points); const auto kmax = static_cast<Order>(points.size());
    auto baseline = tower_of(input,kmax,owner,work); REQUIRE(baseline.ok());
    const u64 held = work.used();
    for (u64 memo : {u64{0},u64{8}}) {
      std::array<ForestLedger,kMaxMebSites> expected{};
      for (u32 q : {1u,2u,4096u}) for (auto* pool : pools) {
        auto domain = domain_of(input,owner,kmax); REQUIRE(domain.ok());
        FullTimings times = marked();
        {
          auto full = build_full(std::move(domain.value()),work,&times,FullParams{memo,q,4,memo,true},pool);
          REQUIRE(full.ok()); CHECK(times.parallel_verticals); CHECK_EQ(domain.value().catalogue().kmax(),0u);
          u64 bytes = held;
          for (Order k = 1; k <= kmax; ++k) {
            const auto& f = full.value().order(k); const auto& t = times.orders[k-1]; const auto& l = f.ledger();
            CHECK(same(f,baseline.value().order(k))); CHECK(structure(f));
            CHECK(structural(l) == structural(baseline.value().order(k).ledger()));
            if (memo == 0) CHECK(l == baseline.value().order(k).ledger());
            else { CHECK(counts(l.descent)); CHECK_EQ(l.descent.memo.queries,l.trace_resolutions+l.vertical_descents); }
            if (q == 1 && pool->size() == 1) expected[k-1] = l;
            else CHECK(l == expected[k-1]);  // Memes flux par lane pour tous Q ET W.
            CHECK(vertical_bounds(t,pool->size(),4,q));
            if (k == 1) CHECK(vertical_inactive(t));
            else {
              CHECK_EQ(t.vertical_resolutions,f.births()); CHECK_EQ(l.vertical_descents,f.births());
              CHECK_EQ(t.vertical_batches,(u64{f.births()}+q-1)/q);
              CHECK_EQ(t.max_vertical_batch,std::min(q,f.births()));
              batches += t.vertical_batches; checks += l.vertical_checks;
            }
            bytes += retained(f); hits += l.descent.memo.hits;
          }
          CHECK_EQ(work.used(),bytes); CHECK(times.orders[kmax] == OrderTimings{});
        }
        CHECK_EQ(work.used(),held);
      }
    }
  }
  CHECK(batches > 0); CHECK(hits > 0); CHECK(checks > 0);
}

MHGP11_TEST(closed_dates, 70) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  for (const auto& points : std::vector<std::vector<Xyz>>{
      {{0,0,0},{2,0,0},{4,0,0}}, {{0,0,0},{4,0,0},{0,4,0},{4,4,0}}}) {
    const Input input(points); const auto kmax = static_cast<Order>(points.size());
    const Order previous = static_cast<Order>(kmax-1);
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain = domain_of(input,owner,kmax); REQUIRE(domain.ok());
    auto lower = build_forest(domain.value(),previous,work); REQUIRE(lower.ok());
    auto upper = build_forest(domain.value(),kmax,work); REQUIRE(upper.ok());
    CHECK_EQ(upper.value().births(),1u);
    DescentLedger paid;
    auto seed = vertical_seed(domain.value(),lower.value(),kmax,upper.value().nodes()[0],work,nullptr,paid);
    REQUIRE(seed.ok()); CHECK(idx(seed.value()) < lower.value().births());
    u64 hops = 0;
    auto image = lower.value().ancestor_closed(seed.value(),upper.value().nodes()[0].rank,hops);
    REQUIRE(image.ok()); CHECK_EQ(image.value(),lower.value().root());
    if (points.size() == 3) { CHECK(seed.value() != image.value()); CHECK_EQ(hops,1u); }
    else CHECK_EQ(lower.value().nodes()[idx(seed.value())].rank,upper.value().nodes()[0].rank);
    for (u32 q : {1u,2u,4096u}) {
      auto fresh = domain_of(input,owner,kmax); REQUIRE(fresh.ok()); FullTimings times;
      auto full = build_full(std::move(fresh.value()),work,&times,FullParams{0,q,4,0,true},pool.value().get());
      REQUIRE(full.ok()); const auto& high = full.value().order(kmax); const auto& low = full.value().order(previous);
      CHECK_EQ(high.lower()[0],low.root()); CHECK_EQ(high.ledger().vertical_descents,1u);
      CHECK_EQ(times.orders[kmax-1].vertical_batches,1u); CHECK(structure(high)); CHECK(structure(low));
      if (points.size() == 3) for (NodeIdx node : low.lower()) CHECK_EQ(node,full.value().order(1).root());
    }
  }
}

MHGP11_TEST(lanes48, 65) {
  std::vector<Xyz> points;
  for (u32 i = 0; i < 64; ++i) points.push_back({2*i,0,0});
  const Input input(points); MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto baseline = tower_of(input,2,owner,work); REQUIRE(baseline.ok());
  for (u64 memo : {u64{0},u64{8}}) {
    ForestLedger expected;
    for (u32 w : {1u,4u,48u}) {
      auto pool = sched::make_pool({w}); REQUIRE(pool.ok());
      auto domain = domain_of(input,owner,2); REQUIRE(domain.ok()); FullTimings times;
      auto full = build_full(std::move(domain.value()),work,&times,FullParams{memo,64,48,memo,true},pool.value().get());
      REQUIRE(full.ok()); const auto& f = full.value().order(2); const auto& t = times.orders[1];
      CHECK_EQ(f.births(),63u); CHECK(same(f,baseline.value().order(2)));
      CHECK(structural(f.ledger()) == structural(baseline.value().order(2).ledger()));
      if (w == 1) expected = f.ledger(); else CHECK(f.ledger() == expected);
      CHECK_EQ(t.vertical_batches,1u); CHECK_EQ(t.max_vertical_batch,63u); CHECK_EQ(t.vertical_resolutions,63u);
      CHECK(vertical_bounds(t,w,48,64)); CHECK_EQ(times.lane_memo_reserved_bytes,48*memo*DescentMemo::slot_bytes());
    }
  }
}

MHGP11_TEST(refusals, 30) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(line(),owner,4); REQUIRE(domain.ok());
  const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
  for (FullParams p : {FullParams{0,0,1,0,true},FullParams{0,2,4,0,true}}) {
    auto times = marked(); const auto before = times;
    auto rejected = build_full(std::move(domain.value()),work,&times,p);
    CHECK_EQ(rejected.outcome().reason,Reason::parameter_out_of_range); CHECK(times == before);
    CHECK(domain.value().index().cloud().x().data() == saved); CHECK_EQ(owner.used(),owners);
    CHECK(work.released().ok());
  }
  auto times = marked(); const auto before = times;
  CHECK_EQ(build_full(std::move(domain.value()),zero,&times,FullParams{0,2,4,0,true},pool.value().get()).outcome().reason,
           Reason::memory_budget);
  CHECK(times == before); CHECK(zero.released().ok());
  auto good = build_full(std::move(domain.value()),work,&times,{},pool.value().get()); REQUIRE(good.ok());
  CHECK(!times.parallel_verticals);
  for (const auto& t : times.orders) CHECK(vertical_inactive(t));
  CHECK(structure(good.value().order(4))); CHECK_EQ(good.value().order(4).lower()[0],good.value().order(3).root());
}

namespace {
struct NestedVertical {
  const FullDomain& domain; const OrderForest& lower; OrderForest& upper;
  MemoryBudget& budget; ForestParallel& context;
  OrderTimings times{1,2,3,4}; Outcome outcome{};
  static Outcome call(void* opaque,u64,u64,u32) noexcept {
    auto& self = *static_cast<NestedVertical*>(opaque);
    self.outcome = forest_verticals(self.domain,self.lower,self.upper,self.budget,nullptr,&self.context,&self.times);
    return {};
  }
};
}
MHGP11_TEST(pool_busy, 20) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(line(),owner,2); REQUIRE(domain.ok());
  auto lower = build_forest(domain.value(),1,work); REQUIRE(lower.ok());
  auto context = ForestParallel::make(domain.value(),FullParams{0,2,4,0,true},work,*pool.value());
  REQUIRE(context.ok()); const auto lower_ledger = lower.value().ledger(); const u64 held = work.used();
  {
    auto upper = build_forest(domain.value(),2,work); REQUIRE(upper.ok());
    NestedVertical call{domain.value(),lower.value(),upper.value(),work,context.value()}; const auto before = call.times;
    REQUIRE(pool.value()->parallel_for(1,1,&call,NestedVertical::call).ok());
    CHECK_EQ(call.outcome.reason,Reason::pool_busy); CHECK(call.times == before);
    CHECK(lower.value().ledger() == lower_ledger); CHECK_EQ(upper.value().ledger().vertical_descents,0u);
  }
  CHECK_EQ(work.used(),held);
  {
    auto upper = build_forest(domain.value(),2,work); REQUIRE(upper.ok());
    REQUIRE(forest_verticals(domain.value(),lower.value(),upper.value(),work,nullptr,&context.value()).ok());
    CHECK_EQ(upper.value().ledger().vertical_descents,upper.value().births());
    for (NodeIdx image : upper.value().lower()) CHECK_EQ(image,lower.value().root());
  }
  CHECK_EQ(work.used(),held); CHECK(lower.value().ledger() == lower_ledger);
}
MHGP11_TEST_MAIN()
