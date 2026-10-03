// Injection atomique de chaque allocation observee : inclut les lanes, puis allocations tardives des ordres/verticales.
#include <atomic>
#include <cstdlib>
#include <new>
#include <thread>
#include "forest_parallel_support.hpp"
#include "test.hpp"
namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injections{0}, off_pilot{0};
const auto pilot = std::this_thread::get_id();
bool deny() noexcept {
  if (calls.fetch_add(1) != fail_at.load()) return false;
  injections.fetch_add(1);
  if (std::this_thread::get_id() != pilot) off_pilot.fetch_add(1);
  return true;
}
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p = std::malloc(n == 0 ? 1 : n);
  if (p == nullptr) { throw std::bad_alloc(); }
  return p;
}
[[gnu::noinline]] void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(n == 0 ? 1 : n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { std::free(p); }
using namespace forest_parallel_test;

MHGP11_TEST(starvation, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,0,0},{2,0,0},{4,0,0},{6,0,0}});
  auto kept = tower_of(input, 4, owner, work); REQUIRE(kept.ok());
  const auto* old_nodes = kept.value().order(1).nodes().data(); const u64 held = work.used();
  for (u32 workers : {1u,4u}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    const FullParams params{8,2,4,8}; u64 allocations = 0;
    {
      auto domain = domain_of(input, owner, 4); REQUIRE(domain.ok());
      const u64 before = calls.load();
      auto full = build_full(std::move(domain.value()), work, nullptr, params, pool.value().get());
      allocations = calls.load() - before; REQUIRE(full.ok());
      CHECK(same(full.value().order(1), kept.value().order(1)));
    }
    REQUIRE(allocations >= 20 && allocations <= 4096);
    CHECK_EQ(work.used(), held);
    u64 refused_count = 0, restored = 0, triggered = 0, unchanged = 0;
    const u64 off_before = off_pilot.load();
    for (u64 position = 0; position < allocations; ++position) {
      auto domain = domain_of(input, owner, 4); REQUIRE(domain.ok());
      const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
      auto times = sentinel(); const auto saved_times = times; const auto before = injections.load();
      fail_at.store(calls.load() + position);
      {
        auto refused = build_full(std::move(domain.value()), work, &times, params, pool.value().get());
        fail_at.store(std::numeric_limits<u64>::max());
        refused_count += !refused.ok() && refused.outcome().reason == Reason::memory_budget ? 1u : 0u;
      }
      restored += work.used() == held && owner.used() == owners ? 1u : 0u;
      triggered += injections.load() == before + 1 ? 1u : 0u;
      unchanged += times == saved_times && domain.value().index().cloud().x().data() == saved ? 1u : 0u;
    }
    CHECK_EQ(refused_count, allocations); CHECK_EQ(restored, allocations);
    CHECK_EQ(triggered, allocations); CHECK_EQ(unchanged, allocations);
    CHECK(kept.value().order(1).nodes().data() == old_nodes); CHECK(structure(kept.value().order(1)));
    {
      auto domain = domain_of(input, owner, 4); REQUIRE(domain.ok());
      auto recovered = build_full(std::move(domain.value()), work, nullptr, params, pool.value().get());
      REQUIRE(recovered.ok());
      for (Order k = 1; k <= 4; ++k) CHECK(same(recovered.value().order(k), kept.value().order(k)));
    }
    CHECK_EQ(work.used(), held);
    std::printf("forest_parallel_allocations workers=%u allocations=%llu refused=%llu restored=%llu injected=%llu "
                "off_pilot=%llu\n", workers, static_cast<unsigned long long>(allocations),
                static_cast<unsigned long long>(refused_count), static_cast<unsigned long long>(restored),
                static_cast<unsigned long long>(triggered), static_cast<unsigned long long>(off_pilot.load()-off_before));
  }
}
MHGP11_TEST_MAIN()
