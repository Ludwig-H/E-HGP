// Memo date : faits analytiques, comparaison au resolveur non memoise, proprietaires et budgets.
#include "memo_support.hpp"
#include "test.hpp"
#include <thread>
using namespace memo_test;

MHGP11_TEST(dates, 28) {
  MemoryBudget owner(MemoryBudget::kUnlimited), storage(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(line(), owner, 4); REQUIRE(domain.ok());
  auto memo = DescentMemo::make(domain.value(), 8, storage); REQUIRE(memo.ok());
  const std::array<SiteIdx,2> near{SiteIdx{1},SiteIdx{2}}, far{SiteIdx{0},SiteIdx{3}};
  auto a = memo.value().resolve(domain.value(), near, 2, work); REQUIRE(a.ok());
  CHECK(level_is(a.value().initial_level(),1,1)); CHECK_EQ(a.value().ledger().memo.insertions,1u);
  auto b = memo.value().resolve(domain.value(), far, 2, work); REQUIRE(b.ok());
  CHECK(level_is(b.value().initial_level(),9,1)); CHECK(level_is(b.value().terminal_level(),1,1));
  CHECK_EQ(b.value().ledger().memo.suffix_hits,1u); CHECK_EQ(b.value().ledger().steps,1u);
  CHECK_EQ(b.value().ledger().memo.lookups,2u); CHECK(counts(b.value().ledger()));
  auto exact = descend(domain.value(), far, 2, work); REQUIRE(exact.ok()); CHECK(answer(exact.value(),b.value()));
  const std::array<SiteIdx,2> reverse{far[1],far[0]};
  auto hit = memo.value().resolve(domain.value(), reverse, 2, work); REQUIRE(hit.ok());
  CHECK(answer(exact.value(),hit.value())); CHECK_EQ(hit.value().ledger().memo.hits,1u);
  CHECK_EQ(hit.value().ledger().memo.suffix_hits,0u); CHECK(counts(hit.value().ledger()));
  CHECK(without_memo(hit.value().ledger()) == DescentLedger{});
  CHECK(num::compare(hit.value().initial_level(),num::Level::make(num::to_wide(i64{4}),num::to_wide(i64{1})).value()) > 0);
  CHECK_EQ(storage.used(),8*DescentMemo::slot_bytes()); CHECK(work.released().ok());
  const auto saved = hit.value();
  DescentMemo moved(std::move(memo.value()));
  CHECK_EQ(memo.value().resolve(domain.value(),near,2,work).outcome().reason,Reason::parameter_out_of_range);
  auto again = moved.resolve(domain.value(),far,2,work); REQUIRE(again.ok()); CHECK(answer(again.value(),saved));
  CHECK_EQ(again.value().ledger().memo.hits,1u); CHECK_EQ(moved.capacity(),8u);
}

MHGP11_TEST(collisions_refusals, 34) {
  MemoryBudget owner(MemoryBudget::kUnlimited), storage(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(line(), owner, 2); REQUIRE(domain.ok());
  auto memo = DescentMemo::make(domain.value(),1,storage); REQUIRE(memo.ok());
  const std::array<SiteIdx,2> near{SiteIdx{0},SiteIdx{1}}, far{SiteIdx{0},SiteIdx{3}};
  auto kept = memo.value().resolve(domain.value(),near,2,work); REQUIRE(kept.ok());
  CHECK_EQ(memo.value().resolve(domain.value(),far,2,zero).outcome().reason,Reason::memory_budget);
  auto old = memo.value().resolve(domain.value(),near,2,zero); REQUIRE(old.ok());
  CHECK(answer(old.value(),kept.value())); CHECK_EQ(old.value().ledger().memo.hits,1u);
  auto changed = memo.value().resolve(domain.value(),far,2,work); REQUIRE(changed.ok());
  CHECK(level_is(changed.value().initial_level(),9,1)); CHECK_EQ(changed.value().ledger().memo.evictions,1u);
  CHECK(changed.value().ledger().memo.collisions > 0); CHECK(counts(changed.value().ledger()));
  CHECK(level_is(kept.value().initial_level(),1,1));
  auto replaced = memo.value().resolve(domain.value(),near,2,work); REQUIRE(replaced.ok());
  CHECK_EQ(replaced.value().ledger().memo.hits,0u); CHECK_EQ(replaced.value().ledger().memo.evictions,1u);
  const std::array<SiteIdx,2> duplicate{near[0],near[0]}, outside{near[0],SiteIdx{kNone}};
  for (auto part : {std::span<const SiteIdx>{},std::span<const SiteIdx>{duplicate},std::span<const SiteIdx>{outside}})
    CHECK_EQ(memo.value().resolve(domain.value(),part,2,work).outcome().reason,Reason::parameter_out_of_range);
  CHECK_EQ(memo.value().resolve(domain.value(),near,0,work).outcome().reason,Reason::kmax_out_of_range);
  auto retained = memo.value().resolve(domain.value(),near,2,work); REQUIRE(retained.ok());
  CHECK_EQ(retained.value().ledger().memo.hits,1u);
  auto foreign = domain_of(line(),owner,2); REQUIRE(foreign.ok());
  CHECK_EQ(memo.value().resolve(foreign.value(),near,2,work).outcome().reason,Reason::parameter_out_of_range);
  auto disabled = DescentMemo::make(domain.value(),0,zero); REQUIRE(disabled.ok());
  CHECK_EQ(disabled.value().resolve(foreign.value(),near,2,work).outcome().reason,Reason::parameter_out_of_range);
  auto base = disabled.value().resolve(domain.value(),near,2,work); REQUIRE(base.ok());
  CHECK(base.value().ledger().memo == MemoLedger{}); CHECK(answer(base.value(),kept.value()));
  auto one = memo.value().resolve(domain.value(),std::span<const SiteIdx>{near}.first(1),1,work); REQUIRE(one.ok());
  CHECK(one.value().seed().site() == near[0]); CHECK_EQ(one.value().seed().order(),1u);
  CHECK(work.released().ok()); CHECK(zero.released().ok());
}

MHGP11_TEST(differential, 1000) {
  const u32 m=kCoordMax;
  const std::vector<std::vector<Xyz>> fixtures{
    {{0,0,0},{m,m,0},{m,0,m},{0,m,m}},
    {{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}},
    {{0,0,0},{4,0,0},{0,4,0},{4,4,0},{2,2,0}},
    {{0,0,0},{0,0,4},{0,4,0},{0,4,4},{4,0,0},{4,0,4},{4,4,0},{4,4,4}}};
  for (const auto& points : fixtures) {
    MemoryBudget owner(MemoryBudget::kUnlimited), storage(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto domain=domain_of(Input(points),owner,5); REQUIRE(domain.ok());
    for (u64 capacity : {u64{0},u64{1},u64{2},u64{16},u64{64}}) {
      auto memo=DescentMemo::make(domain.value(),capacity,storage); REQUIRE(memo.ok());
      for (auto part : parts(domain.value().index().cloud().sites())) {
        const u32 k=static_cast<u32>(part.size());
        auto exact=descend(domain.value(),part,k,work); REQUIRE(exact.ok());
        auto first=memo.value().resolve(domain.value(),part,k,work); REQUIRE(first.ok());
        CHECK(answer(first.value(),exact.value()));
        std::reverse(part.begin(),part.end());
        auto second=memo.value().resolve(domain.value(),part,k,work); REQUIRE(second.ok());
        CHECK(answer(second.value(),exact.value()));
        if (capacity) { CHECK_EQ(second.value().ledger().memo.hits,1u); CHECK(counts(first.value().ledger())); }
        else CHECK(first.value().ledger() == exact.value().ledger());
        CHECK(work.released().ok());
      }
    }
    CHECK(storage.released().ok());
  }
}

MHGP11_TEST(capacity, 37) {
  MemoryBudget owner(MemoryBudget::kUnlimited), huge(MemoryBudget::kUnlimited), zero(0);
  auto domain=domain_of(line(),owner,4); REQUIRE(domain.ok());
  CHECK_EQ(DescentMemo::make(domain.value(),3,huge).outcome().reason,Reason::parameter_out_of_range);
  CHECK_EQ(DescentMemo::make(domain.value(),u64{1}<<63,huge).outcome().reason,Reason::tower_capacity);
  CHECK_EQ(DescentMemo::make(domain.value(),1,zero).outcome().reason,Reason::memory_budget);
  CHECK(huge.released().ok()); CHECK(zero.released().ok());
  for (auto field : {&MemoLedger::queries,&MemoLedger::lookups,&MemoLedger::hits,&MemoLedger::misses,
                    &MemoLedger::collisions,&MemoLedger::insertions,&MemoLedger::evictions,&MemoLedger::suffix_hits}) {
    DescentLedger sum, one; sum.memo.*field=std::numeric_limits<u64>::max(); one.memo.*field=1;
    one.steps=1; const auto before=sum;
    CHECK_EQ(add_descent(sum,one).reason,Reason::tower_capacity); CHECK(sum == before);
    one.memo.*field=0; CHECK(add_descent(sum,one).ok());
  }
  const u64 bytes=DescentMemo::slot_bytes(); MemoryBudget short_budget(bytes-1), exact(bytes);
  CHECK_EQ(DescentMemo::make(domain.value(),1,short_budget).outcome().reason,Reason::memory_budget);
  { auto memo=DescentMemo::make(domain.value(),1,exact); REQUIRE(memo.ok()); CHECK_EQ(exact.used(),bytes); }
  CHECK(exact.released().ok()); CHECK_EQ(exact.peak(),bytes); CHECK(short_budget.released().ok());
  CHECK(DescentMemo::slot_bytes() >= 48+2*sizeof(num::Level)+9);
}
MHGP11_TEST_MAIN()
