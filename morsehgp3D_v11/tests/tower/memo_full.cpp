// FULL identique avec memo bornes : aucun cache partage entre fils ni reserve retenue dans le resultat.
#include "memo_support.hpp"
#include "test.hpp"
#include <thread>
using namespace memo_test;

MHGP11_TEST(equivalence, 150) {
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{2,0,0},{4,0,0},{6,0,0}}, {{0,0,0},{4,0,0},{0,4,0},{4,4,0}},
    {{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}}};
  u64 hits=0;
  for (const auto& points : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const auto k=static_cast<Order>(points.size()); const Input input(points);
    auto baseline=tower_of(input,k,owner,work); REQUIRE(baseline.ok());
    const u64 held=work.used();
    for (u64 capacity : {u64{0},u64{1},u64{8},u64{64}}) {
      auto domain=domain_of(input,owner,k); REQUIRE(domain.ok());
      FullTimings times; times.memo_capacity=7; times.memo_slot_bytes=11; times.memo_reserved_bytes=13;
      work.restart_peak();
      auto made=build_full(std::move(domain.value()),work,&times,FullParams{capacity}); REQUIRE(made.ok());
      CHECK_EQ(domain.value().catalogue().kmax(),0u); CHECK_EQ(times.memo_capacity,capacity);
      CHECK_EQ(times.memo_slot_bytes,DescentMemo::slot_bytes());
      CHECK_EQ(times.memo_reserved_bytes,capacity*DescentMemo::slot_bytes());
      u64 retained_bytes=held;
      for (Order order=1; order<=k; ++order) {
        CHECK(same(made.value().order(order),baseline.value().order(order)));
        retained_bytes+=retained(made.value().order(order));
        const auto& l=made.value().order(order).ledger(); hits+=l.descent.memo.hits;
        if (capacity) { CHECK(counts(l.descent)); CHECK_EQ(l.descent.memo.queries,l.trace_resolutions+l.vertical_descents); }
        else CHECK(l.descent == baseline.value().order(order).ledger().descent);
      }
      CHECK_EQ(work.used(),retained_bytes);
      CHECK(work.peak() >= retained_bytes+times.memo_reserved_bytes);
    }
    CHECK_EQ(work.used(),held);
  }
  CHECK(hits > 0);
}

MHGP11_TEST(refusals, 26) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain=domain_of(line(),owner,4); REQUIRE(domain.ok());
  const auto* saved=domain.value().index().cloud().x().data(); const u64 held=owner.used();
  FullTimings sentinel; sentinel.memo_capacity=7; sentinel.memo_slot_bytes=11; sentinel.memo_reserved_bytes=13;
  for (auto& t : sentinel.orders) t={2,3,5,7};
  for (u64 capacity : {u64{3},u64{1}<<63,u64{64}}) {
    FullTimings current=sentinel;
    auto refused=build_full(std::move(domain.value()),zero,&current,FullParams{capacity});
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason,capacity==3 ? Reason::parameter_out_of_range :
                                  capacity==(u64{1}<<63) ? Reason::tower_capacity : Reason::memory_budget);
    CHECK(current == sentinel); CHECK(domain.value().index().cloud().x().data() == saved);
    CHECK_EQ(owner.used(),held); CHECK(zero.released().ok());
  }
  auto foreign=domain_of(line(),owner,4); REQUIRE(foreign.ok());
  auto memo=DescentMemo::make(foreign.value(),0,work); REQUIRE(memo.ok());
  OrderTimings times{2,3,5,7}; const auto before=times;
  CHECK_EQ(build_forest(domain.value(),2,work,&times,&memo.value()).outcome().reason,Reason::parameter_out_of_range);
  CHECK(times == before); CHECK(work.released().ok());
  auto again=build_full(std::move(domain.value()),work,nullptr,FullParams{8}); REQUIRE(again.ok());
  CHECK_EQ(again.value().kmax(),4u);
}

MHGP11_TEST(concurrency, 13) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto domain=domain_of(line(),owner,4); REQUIRE(domain.ok());
  const std::array<SiteIdx,2> part{SiteIdx{0},SiteIdx{3}};
  std::array<bool,4> results{}; std::array<std::thread,4> threads;
  for (u32 i=0;i<4;++i) threads[i]=std::thread([&,i] {
    MemoryBudget private_budget(MemoryBudget::kUnlimited);
    {
      auto memo=DescentMemo::make(domain.value(),8,private_budget);
      if (!memo.ok()) return;
      for (u32 n=0;n<16;++n) {
        auto result=memo.value().resolve(domain.value(),part,2,private_budget);
        if (!result.ok() || !level_is(result.value().initial_level(),9,1) ||
            !level_is(result.value().terminal_level(),1,1) || !counts(result.value().ledger()) ||
            result.value().ledger().memo.hits != (n==0 ? 0u : 1u)) return;
      }
    }
    results[i]=private_budget.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (bool good : results) { CHECK(good); CHECK(domain.value().catalogue().kmax()==4); CHECK(domain.value().index().cloud().sites()==4); }
}
MHGP11_TEST_MAIN()
