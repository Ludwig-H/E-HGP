// Injection a chaque allocation observee du FULL : domaine non consomme et aucun resultat partiel.
#include <cstdlib>
#include <new>
#include "forest_support.hpp"
#include "tower/forest_internal.hpp"
#include "test.hpp"
namespace {
mhgp11::u64 calls = 0, fail_at = std::numeric_limits<mhgp11::u64>::max();
bool deny() noexcept { return calls++ == fail_at; }
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p = std::malloc(n == 0 ? 1 : n); if (p == nullptr) throw std::bad_alloc(); return p;
}
[[gnu::noinline]] void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(n == 0 ? 1 : n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept { std::free(p); }
using namespace forest_test;
MHGP11_TEST(starvation, 50) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,0,0},{2,0,0},{4,0,0}});
  auto domain = domain_of(input, owner, 3); REQUIRE(domain.ok());
  const u64 before = calls;
  auto kept = build_full(std::move(domain.value()), work);
  const u64 allocations = calls - before;
  REQUIRE(kept.ok()); CHECK(allocations >= 10u);
  const auto* saved = kept.value().order(1).nodes().data();
  const u64 held = work.used();
  for (u64 position = 0; position < allocations; ++position) {
    auto next = domain_of(input, owner, 3); REQUIRE(next.ok());
    const auto* xyz = next.value().index().cloud().x().data(); const u64 owners = owner.used();
    FullTimings timings;
    for (auto& t : timings.orders) t = {7,11,13,17};
    const auto preserved = timings;
    fail_at = calls + position;
    auto refused = build_full(std::move(next.value()), work, &timings);
    fail_at = std::numeric_limits<u64>::max();
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
    CHECK_EQ(work.used(), held); CHECK_EQ(owner.used(), owners);
    CHECK(next.value().index().cloud().x().data() == xyz);
    CHECK(timings == preserved);
    CHECK(kept.value().order(1).nodes().data() == saved); CHECK(structure(kept.value().order(1)));
  }
  auto again = tower_of(input, 3, owner, work); REQUIRE(again.ok());
  CHECK(same(again.value().order(1), kept.value().order(1)));
  CHECK(same(again.value().order(2), kept.value().order(2)));
  CHECK(same(again.value().order(3), kept.value().order(3)));
}
MHGP11_TEST(direct_timings, 25) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,0,0},{2,0,0},{4,0,0}}), owner, 3); REQUIRE(domain.ok());
  auto kept = build_forest(domain.value(), 2, work); REQUIRE(kept.ok());
  const u64 held = work.used(), owners = owner.used();
  const auto* saved = kept.value().nodes().data();
  u64 before_dsu = 0;
  {
    // Mesurer les phases reelles : la faute suivante vise states puis touched, APRES les naissances.
    const u64 before = calls;
    ForestBuilder prefix(domain.value(), 2, work);
    REQUIRE(prefix.classify().ok()); REQUIRE(prefix.births().ok());
    before_dsu = calls - before;
    CHECK(before_dsu >= 4);
  }
  CHECK_EQ(work.used(), held);
  for (u64 offset : {u64{0}, u64{1}}) {
    OrderTimings timings{7,11,13,17}; const auto preserved = timings;
    const u64 before = calls;
    fail_at = before + before_dsu + offset;
    auto refused = build_forest(domain.value(), 2, work, &timings);
    fail_at = std::numeric_limits<u64>::max();
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
    CHECK_EQ(calls - before, before_dsu + offset + 1);
    CHECK(timings == preserved); CHECK_EQ(work.used(), held); CHECK_EQ(owner.used(), owners);
    CHECK(kept.value().nodes().data() == saved); CHECK(structure(kept.value()));
  }
  OrderTimings timings{7,11,13,17};
  auto again = build_forest(domain.value(), 2, work, &timings); REQUIRE(again.ok());
  CHECK(same(again.value(), kept.value())); CHECK(again.value().ledger() == kept.value().ledger());
  CHECK_EQ(timings.verticals_ns, 0u); CHECK_EQ(owner.used(), owners);
}
MHGP11_TEST_MAIN()
