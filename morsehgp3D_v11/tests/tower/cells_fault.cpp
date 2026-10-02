// La cellule possede un unique Buffer de traces ; allocation refusee apres comptage, transaction intacte.
#include <cstdlib>
#include <new>
#include "cells_support.hpp"
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
using namespace cells_test;
MHGP11_TEST(starvation, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  const u64 baseline = owner.used();
  {
    const auto before = calls;
    auto result = build_cell(domain.value(), b, 3, work);
    const auto allocations = calls - before;
    REQUIRE(result.ok()); CHECK_EQ(allocations, 1u);
    const auto first = result.value().traces()[0];
    const u64 kept = work.used();
    denied = true;
    auto fail_cell = build_cell(domain.value(), b, 3, work);
    auto birth = build_cell(domain.value(), b, 5, work);
    denied = false;
    CHECK(!fail_cell.ok() && fail_cell.outcome().reason == Reason::memory_budget);
    REQUIRE(birth.ok()); CHECK(birth.value().traces().empty());
    CHECK_EQ(work.used(), kept); CHECK_EQ(owner.used(), baseline);
    CHECK(result.value().traces()[0].sites == first.sites);
    CHECK(domain.value().find_support(domain.value().catalogue().balls_data()[idx(b)].support) == b);
  }
  CHECK(work.released().ok());
  const auto* xyz = domain.value().index().cloud().x().data();
  const auto before = calls;
  denied = true;
  auto failed = build_cell(domain.value(), b, 3, work);
  denied = false;
  CHECK_EQ(calls - before, 1u);
  CHECK(!failed.ok() && failed.outcome().reason == Reason::memory_budget);
  CHECK(work.released().ok()); CHECK_EQ(owner.used(), baseline);
  CHECK(domain.value().index().cloud().x().data() == xyz);
  auto again = build_cell(domain.value(), b, 3, work); REQUIRE(again.ok());
  CHECK_EQ(again.value().traces().size(), 4u); CHECK_EQ(work.used(), 4 * sizeof(CellTrace));
  CHECK_EQ(again.value().ledger().trace_tests, 12u);
}
MHGP11_TEST_MAIN()
