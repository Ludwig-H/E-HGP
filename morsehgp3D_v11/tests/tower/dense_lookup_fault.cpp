// Allocations du FULL dense : appels de lookup sans tas et refus tardifs avant publication des diagnostics.
#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>
#include "census_reuse_support.hpp"
#include "tower/forest_internal.hpp"
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

MHGP11_TEST(allocation_free, 15) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,4,0},{4,0,0},{4,4,0}}),owner,2); REQUIRE(domain.ok());
  auto dense = build_forest(domain.value(),2,work,nullptr,nullptr,nullptr,true); REQUIRE(dense.ok());
  std::vector<BirthSeed> seeds;
  for (const auto& part : parts(3)) if (part.size() == 2) {
    auto made = descend(domain.value(),part,2,work); REQUIRE(made.ok()); seeds.push_back(made.value().seed());
  }
  REQUIRE(seeds.size() == 3);
  std::vector<std::optional<NodeIdx>> expected;
  for (const auto& seed : seeds) { expected.push_back(dense.value().birth_node(seed)); CHECK(expected.back().has_value()); }
  const u64 before = calls.load(), prior_injections = injections.load(), held = work.used();
  bool correct = true;
  deny_all.store(true);
  for (u32 repeat = 0; repeat < 1000; ++repeat)
    for (u32 i = 0; i < seeds.size(); ++i) correct = correct && dense.value().birth_node(seeds[i]) == expected[i];
  deny_all.store(false);
  CHECK(correct); CHECK_EQ(calls.load(),before); CHECK_EQ(injections.load(),prior_injections);
  CHECK_EQ(work.used(),held); CHECK_EQ(dense.value().lookup_reserved_bytes(),12u);
  CHECK(dense.value().dense_birth_lookup());
}

MHGP11_TEST(direct_timings, 25) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,0,0},{2,0,0},{4,0,0}}),owner,3); REQUIRE(domain.ok());
  auto kept = build_forest(domain.value(),2,work,nullptr,nullptr,nullptr,true); REQUIRE(kept.ok());
  const u64 held = work.used(), owners = owner.used(); const auto* saved = kept.value().nodes().data();
  u64 prefix_allocations = 0;
  {
    const u64 before = calls.load();
    ForestBuilder prefix(domain.value(),2,work,nullptr,nullptr,nullptr,true);
    REQUIRE(prefix.classify().ok()); REQUIRE(prefix.births().ok());
    prefix_allocations = calls.load()-before; CHECK(prefix_allocations >= 4);
    CHECK(prefix.result.dense_birth_lookup());
  }
  CHECK_EQ(work.used(),held);
  for (u64 offset : {u64{0},u64{1}}) {
    OrderTimings times{7,11,13,17}; const auto before = times;
    const u64 old_calls = calls.load(), old_injections = injections.load();
    fail_at.store(old_calls+prefix_allocations+offset);
    auto refused = build_forest(domain.value(),2,work,&times,nullptr,nullptr,true);
    fail_at.store(std::numeric_limits<u64>::max());
    CHECK_EQ(refused.outcome().reason,Reason::memory_budget); CHECK(times == before);
    CHECK_EQ(calls.load()-old_calls,prefix_allocations+offset+1); CHECK_EQ(injections.load(),old_injections+1);
    CHECK_EQ(work.used(),held); CHECK_EQ(owner.used(),owners);
    CHECK(kept.value().nodes().data() == saved); CHECK(structure(kept.value()));
  }
  OrderTimings times{7,11,13,17};
  auto again = build_forest(domain.value(),2,work,&times,nullptr,nullptr,true); REQUIRE(again.ok());
  CHECK(same(again.value(),kept.value())); CHECK(again.value().ledger() == kept.value().ledger());
  CHECK_EQ(times.verticals_ns,0u); CHECK(again.value().dense_birth_lookup());
}

MHGP11_TEST(starvation, 35) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,1,0},{1,0,0},{2,1,0},{1,2,0}});
  auto kept = tower_of(input,4,owner,work); REQUIRE(kept.ok());
  const auto* old_nodes = kept.value().order(1).nodes().data(); const u64 held = work.used();
  for (u32 workers : {1u,4u}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    const FullParams params{8,2,4,8,true,true,true}; u64 allocations = 0;
    {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
      const u64 before = calls.load();
      auto full = build_full(std::move(domain.value()),work,nullptr,params,pool.value().get());
      allocations = calls.load()-before; REQUIRE(full.ok());
      CHECK(same(full.value().order(2),kept.value().order(2))); CHECK(full.value().order(2).dense_birth_lookup());
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
      for (Order k = 1; k <= 4; ++k) {
        CHECK(same(recovered.value().order(k),kept.value().order(k))); CHECK(recovered.value().order(k).dense_birth_lookup());
      }
    }
    CHECK_EQ(work.used(),held);
    std::printf("dense_lookup_allocations workers=%u allocations=%llu refused=%llu restored=%llu injected=%llu\n",
                workers,static_cast<unsigned long long>(allocations),static_cast<unsigned long long>(refused_count),
                static_cast<unsigned long long>(restored),static_cast<unsigned long long>(triggered));
  }
}
MHGP11_TEST_MAIN()
