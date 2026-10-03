// Refus avant publication et aucun appel allocateur dans la descente empruntee apres preparation.
#include <atomic>
#include <cstdlib>
#include <new>
#include "census_reuse_support.hpp"
#include "test.hpp"
namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injections{0};
std::atomic<bool> deny_all{false};
bool deny() noexcept {
  const auto call = calls.fetch_add(1);
  if (!deny_all.load() && call != fail_at.load()) return false;
  injections.fetch_add(1); return true;
}
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p = std::malloc(n == 0 ? 1 : n);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}
[[gnu::noinline]] void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(n == 0 ? 1 : n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { std::free(p); }
using namespace census_reuse_test;

MHGP11_TEST(allocation_free, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(Input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}}),owner,2); REQUIRE(domain.ok());
  const auto part = part_of(domain.value(),{{0,0,0},{11,0,0}});
  auto expected = descend(domain.value(),part,2,work); REQUIRE(expected.ok());
  auto scratch = CensusWorkspace::make(domain.value().index(),work); REQUIRE(scratch.ok());
  const u64 held = work.used();
  for (u64 capacity : {u64{0},u64{1},u64{64}}) {
    auto memo = DescentMemo::make(domain.value(),capacity,work,scratch.value().get()); REQUIRE(memo.ok());
    const u64 before = calls.load(), old_injections = injections.load();
    bool correct = true;
    deny_all.store(true);
    for (u32 i = 0; i < 100; ++i) {
      auto direct = descend(domain.value(),part,2,zero,scratch.value().get());
      auto cached = memo.value().resolve(domain.value(),part,2,zero);
      correct = correct && direct.ok() && cached.ok();
      if (direct.ok() && cached.ok()) correct = correct && answer(direct.value(),expected.value()) &&
          answer(cached.value(),expected.value()) && same_population_work(expected.value().ledger(),direct.value().ledger());
    }
    deny_all.store(false);
    CHECK(correct); CHECK_EQ(calls.load(),before); CHECK_EQ(injections.load(),old_injections);
    CHECK_EQ(zero.peak(),0u); CHECK_EQ(work.used(),held+capacity*DescentMemo::slot_bytes());
  }
  CHECK_EQ(work.used(),held); scratch.value().reset(); CHECK(work.released().ok());
}

MHGP11_TEST(starvation, 35) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,1,0},{1,0,0},{2,1,0},{1,2,0}});
  auto kept = tower_of(input,4,owner,work); REQUIRE(kept.ok());
  const auto* old_nodes = kept.value().order(1).nodes().data(); const u64 held = work.used();
  for (u32 workers : {1u,4u}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    const FullParams params{8,2,4,8,true,true}; u64 allocations = 0;
    {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
      const u64 before = calls.load();
      auto full = build_full(std::move(domain.value()),work,nullptr,params,pool.value().get());
      allocations = calls.load()-before; REQUIRE(full.ok());
      CHECK(same(full.value().order(2),kept.value().order(2)));
    }
    REQUIRE(allocations >= 20 && allocations <= 4096); CHECK_EQ(work.used(),held);
    u64 refused_count = 0, restored = 0, triggered = 0, unchanged = 0;
    for (u64 position = 0; position < allocations; ++position) {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
      const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
      auto times = scratch_sentinel(); const auto before = times; const auto old_injections = injections.load();
      fail_at.store(calls.load()+position);
      {
        auto refused = build_full(std::move(domain.value()),work,&times,params,pool.value().get());
        fail_at.store(std::numeric_limits<u64>::max());
        refused_count += !refused.ok() && refused.outcome().reason == Reason::memory_budget ? 1u : 0u;
      }
      restored += work.used() == held && owner.used() == owners ? 1u : 0u;
      triggered += injections.load() == old_injections+1 ? 1u : 0u;
      unchanged += times == before && domain.value().index().cloud().x().data() == saved ? 1u : 0u;
    }
    CHECK_EQ(refused_count,allocations); CHECK_EQ(restored,allocations);
    CHECK_EQ(triggered,allocations); CHECK_EQ(unchanged,allocations);
    CHECK(kept.value().order(1).nodes().data() == old_nodes); CHECK(structure(kept.value().order(1)));
    {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
      auto recovered = build_full(std::move(domain.value()),work,nullptr,params,pool.value().get()); REQUIRE(recovered.ok());
      for (Order k = 1; k <= 4; ++k) CHECK(same(recovered.value().order(k),kept.value().order(k)));
    }
    CHECK_EQ(work.used(),held);
    std::printf("census_reuse_allocations workers=%u allocations=%llu refused=%llu restored=%llu injected=%llu\n",
                workers,static_cast<unsigned long long>(allocations),static_cast<unsigned long long>(refused_count),
                static_cast<unsigned long long>(restored),static_cast<unsigned long long>(triggered));
  }
}
MHGP11_TEST_MAIN()
