// Panne de chaque allocation observee de build_support_hierarchy (sorties, temporaires, tampons par fil, second
// etage) : refus memory_budget, budget de l'appel rendu, diagnostics intacts, arbre intact ; aucun resultat partiel.
// Puis un appel sans panne rend la meme hierarchie (empreinte). Sans Pool et sur un Pool de 4 fils.
#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>

#include "hierarchy_support.hpp"
#include "sched/sched.hpp"
#include "supports_support.hpp"
#include "test.hpp"

namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injections{0};
bool deny() noexcept {
  if (calls.fetch_add(1) != fail_at.load()) return false;
  injections.fetch_add(1);
  return true;
}
}  // namespace
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

using namespace mhgp11;
using namespace hierarchy_test;
using supports_test::Points;

MHGP11_TEST(starvation, 60) {
  const std::vector<std::pair<Points, Order>> cases{
      {{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, 2},
      {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {10, 0, 0}, {12, 0, 0}, {11, 2, 0}}, 3},
      {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {0, 0, 2}, {2, 0, 2}, {0, 2, 2}, {2, 2, 2}}, 4}};
  auto made = sched::make_pool({4});
  REQUIRE(made.ok());
  const std::unique_ptr<sched::Pool> pool = std::move(made.value());
  u64 total = 0;
  for (const auto& [points, k] : cases)
    for (sched::Pool* p : {static_cast<sched::Pool*>(nullptr), pool.get()}) {
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      auto domain = supports_test::domain_of(points, k, owner);
      REQUIRE(domain.ok());
      auto tree = build_order(std::move(domain.value()), k, owner);
      REQUIRE(tree.ok());
      u64 allocations = 0, want = 0;
      {
        const u64 before = calls.load();
        auto h = supports::build_support_hierarchy(tree.value(), work, p);
        allocations = calls.load() - before;
        REQUIRE(h.ok());
        want = fingerprint(h.value());
      }
      REQUIRE(allocations >= 8 && allocations <= 16);
      const u64 owners = owner.used();
      u64 refused = 0, restored = 0, triggered = 0, unchanged = 0;
      for (u64 position = 0; position < allocations; ++position) {
        supports::HierarchyTimings timings{1, 2, 3, 4, 5, {6, 7}};
        const auto saved = timings;
        const u64 before = injections.load();
        fail_at.store(calls.load() + position);
        {
          auto failed = supports::build_support_hierarchy(tree.value(), work, p, &timings);
          fail_at.store(std::numeric_limits<u64>::max());
          refused += !failed.ok() && failed.outcome().reason == Reason::memory_budget ? 1u : 0u;
        }
        restored += work.used() == 0 && owner.used() == owners ? 1u : 0u;
        triggered += injections.load() == before + 1 ? 1u : 0u;
        unchanged += timings == saved ? 1u : 0u;
      }
      CHECK_EQ(refused, allocations);
      CHECK_EQ(restored, allocations);
      CHECK_EQ(triggered, allocations);
      CHECK_EQ(unchanged, allocations);
      auto again = supports::build_support_hierarchy(tree.value(), work, p);
      REQUIRE(again.ok());
      Totals totals;
      CHECK_EQ(check(tree.value(), again.value(), totals), std::string());
      CHECK_EQ(fingerprint(again.value()), want);
      total += allocations;
    }
  std::printf("hierarchy_fault allocations=%llu\n", static_cast<unsigned long long>(total));
}

MHGP11_TEST_MAIN()
