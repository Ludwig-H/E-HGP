// Panne de chaque allocation observee de build_order (journal, foret, balayage, rattachement) : refus
// memory_budget, domaine non transfere, budget rendu, diagnostics intacts ; aucun resultat partiel.
#include <atomic>
#include <cstdlib>
#include <new>
#include <thread>
#include "order_tree_support.hpp"
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
using namespace order_tree_test;

// Carre a K2 (cellule etendue, fusion a quatre enfants) et passagere a K1, voie serielle puis lots W1 et W4.
MHGP11_TEST(starvation, 300) {
  struct Case { std::vector<Xyz> points; Order k; };
  const std::vector<Case> cases{{{{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, 2}, {{{0,0,0},{2,2,0},{4,0,0},{8,0,0}}, 1},
                                {{{0,0,0},{2,0,0},{0,2,0},{2,2,0},{10,0,0},{12,0,0},{11,2,0}}, 3}};
  u64 total = 0;
  for (const auto& c : cases) for (u32 workers : {0u, 1u, 4u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(c.points);
    std::unique_ptr<sched::Pool> pool;
    FullParams params;
    if (workers != 0) {
      auto made = sched::make_pool({workers}); REQUIRE(made.ok()); pool = std::move(made.value());
      params.regular_batch_capacity = 2; params.descent_lanes = 3; params.memo_capacity = 8;
      params.lane_memo_capacity = 8; params.population_lookup = true; params.reuse_census_workspace = true;
      params.dense_birth_lookup = true;
    }
    std::optional<OrderTree> kept;
    u64 allocations = 0;
    {
      auto domain = domain_of(input, owner, c.k); REQUIRE(domain.ok());
      const u64 before = calls.load();
      auto made = build_order(std::move(domain.value()), c.k, work, params, pool.get());
      allocations = calls.load() - before; REQUIRE(made.ok());
      kept.emplace(std::move(made.value()));
    }
    REQUIRE(allocations >= 8 && allocations <= 4096);
    const u64 held = work.used();
    u64 refused = 0, restored = 0, triggered = 0, unchanged = 0;
    for (u64 position = 0; position < allocations; ++position) {
      auto domain = domain_of(input, owner, c.k); REQUIRE(domain.ok());
      const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
      OrderTimings times{7, 11, 13, 17}; const auto saved_times = times; u64 attach = 99;
      const u64 before = injections.load();
      fail_at.store(calls.load() + position);
      {
        auto failed = build_order(std::move(domain.value()), c.k, work, params, pool.get(), &times, &attach);
        fail_at.store(std::numeric_limits<u64>::max());
        refused += !failed.ok() && failed.outcome().reason == Reason::memory_budget ? 1u : 0u;
      }
      restored += work.used() == held && owner.used() == owners ? 1u : 0u;
      triggered += injections.load() == before + 1 ? 1u : 0u;
      unchanged += times == saved_times && attach == 99 && domain.value().index().cloud().x().data() == saved ? 1u : 0u;
    }
    CHECK_EQ(refused, allocations); CHECK_EQ(restored, allocations);
    CHECK_EQ(triggered, allocations); CHECK_EQ(unchanged, allocations);
    {
      auto domain = domain_of(input, owner, c.k); REQUIRE(domain.ok());
      auto again = build_order(std::move(domain.value()), c.k, work, params, pool.get()); REQUIRE(again.ok());
      CHECK(same_tree(again.value().forest(), kept->forest()));
      CHECK(same_attachment(again.value().attachment(), kept->attachment()));
    }
    CHECK_EQ(work.used(), held);
    kept.reset();
    CHECK_EQ(work.used(), 0u);
    total += allocations;
    std::printf("order_fault points=%zu k=%u workers=%u allocations=%llu\n", c.points.size(), unsigned{c.k}, workers,
                static_cast<unsigned long long>(allocations));
  }
  CHECK(total >= 200);
}

MHGP11_TEST_MAIN()
