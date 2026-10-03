// Le chemin n'alloue rien ; seuls les census temporaires peuvent refuser, sans resultat partiel.
#include <cstdlib>
#include <new>
#include "descent_support.hpp"
#include "test.hpp"
namespace {
bool denied = false;
mhgp11::u64 calls = 0;
bool deny() noexcept { ++calls; return denied; }
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
using namespace descent_test;
MHGP11_TEST(starvation, 24) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}}), owner, 2); REQUIRE(domain.ok());
  const auto initial = part_of(domain.value(), {{0,0,0},{11,0,0}});
  const auto direct = part_of(domain.value(), {{4,0,0},{5,0,0}});
  const u64 baseline = owner.used();
  auto kept = descend(domain.value(), initial, 2, work); REQUIRE(kept.ok());
  auto copy = kept.value();
  const std::array<SiteIdx, 1> singleton{initial[1]};
  auto scratch = CensusWorkspace::make(domain.value().index(), owner); REQUIRE(scratch.ok());
  const u64 with_scratch = owner.used();
  const u64 before = calls;
  denied = true;
  auto refusal = descend(domain.value(), initial, 2, work);
  auto hit = descend(domain.value(), direct, 2, work);
  auto one = descend(domain.value(), singleton, 1, work);
  auto borrowed = descend(domain.value(), singleton, 1, work, scratch.value().get());
  denied = false;
  CHECK_EQ(calls - before, 1u);
  CHECK(!refusal.ok() && refusal.outcome().reason == Reason::memory_budget);
  REQUIRE(hit.ok()); CHECK_EQ(hit.value().ledger().steps, 1u);
  REQUIRE(one.ok()); REQUIRE(borrowed.ok()); CHECK(same(one.value(), borrowed.value()));
  CHECK_EQ(one.value().ledger().singleton_hits, 1u); CHECK_EQ(owner.used(), with_scratch);
  scratch.value().reset();
  CHECK(same(copy, kept.value())); CHECK(work.released().ok()); CHECK_EQ(owner.used(), baseline);
  auto again = descend(domain.value(), initial, 2, work); REQUIRE(again.ok());
  CHECK(same(copy, again.value())); CHECK(work.released().ok());
  // Un hit complet avec interieurs peut aussi enchainer sans aucune allocation de chemin.
  auto dense = domain_of(Input({{0,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0}}), owner, 5); REQUIRE(dense.ok());
  const auto ends = part_of(dense.value(), {{0,0,0},{4,0,0}});
  const u64 begin = calls;
  denied = true;
  auto chain = descend(dense.value(), ends, 2, work);
  denied = false;
  REQUIRE(chain.ok()); CHECK_EQ(calls - begin, 0u); CHECK_EQ(chain.value().ledger().steps, 2u);
  CHECK(replay(dense.value(), ends, 2, chain.value())); CHECK(work.released().ok());
}
MHGP11_TEST_MAIN()
