// Toutes les voies de classification restent sans allocation, meme sous interdiction totale du tas.
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
MHGP11_TEST(no_allocation, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  auto kept = build_cell(domain.value(), b, 3, work); REQUIRE(kept.ok());
  const auto* traces = kept.value().traces().data(); const auto initial = kept.value().traces()[0];
  const u64 baseline = owner.used(), held = work.used();
  for (Order k : {Order{2}, Order{3}, Order{4}, Order{5}}) {
    const u64 before = calls;
    denied = true;
    auto result = classify_cell(domain.value(), b, k);
    denied = false;
    REQUIRE(result.ok()); CHECK_EQ(calls, before);
    CHECK(result.value().kind() == (k >= 4 ? CellKind::birth : CellKind::strict_traces));
    CHECK_EQ(owner.used(), baseline); CHECK_EQ(work.used(), held);
    CHECK(kept.value().traces().data() == traces); CHECK(kept.value().traces()[0].sites == initial.sites);
  }
  auto other = domain_of(Input({{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}}), owner); REQUIRE(other.ok());
  const auto g = select(other.value(), 0, 5, 3); const u64 before = calls;
  denied = true;
  auto late = classify_cell(other.value(), g, 4);
  auto refused = classify_cell(other.value(), g, 1);
  denied = false;
  REQUIRE(late.ok()); CHECK_EQ(calls, before); CHECK_EQ(late.value().ledger().examined, 3u);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::parameter_out_of_range);
  CHECK_EQ(work.used(), held); CHECK(kept.value().traces().data() == traces);
  CHECK(kept.value().traces()[0].sites == initial.sites);
  auto repeated = classify_cell(other.value(), g, 4); REQUIRE(repeated.ok());
  CHECK(repeated.value().ledger() == late.value().ledger());
}
MHGP11_TEST_MAIN()
