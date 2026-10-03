// Faute sur chaque allocation du FULL avec table : domaine et diagnostics preserves, reprise admise.
#include <cstdlib>
#include <new>
#include "memo_support.hpp"
#include "test.hpp"
namespace {
mhgp11::u64 calls=0, fail_at=std::numeric_limits<mhgp11::u64>::max();
bool deny() noexcept { return calls++ == fail_at; }
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p=std::malloc(n==0 ? 1 : n); if (p==nullptr) throw std::bad_alloc(); return p;
}
[[gnu::noinline]] void* operator new(std::size_t n,const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(n==0 ? 1 : n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p,std::size_t) noexcept { std::free(p); }
using namespace memo_test;
MHGP11_TEST(starvation, 50) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto domain=domain_of(line(),owner,4); REQUIRE(domain.ok());
  const u64 before=calls;
  auto kept=build_full(std::move(domain.value()),work,nullptr,FullParams{8});
  const u64 allocations=calls-before; REQUIRE(kept.ok()); CHECK(allocations>10);
  const u64 held=work.used(); const auto* saved=kept.value().order(1).nodes().data();
  for (u64 i=0;i<allocations;++i) {
    auto next=domain_of(line(),owner,4); REQUIRE(next.ok());
    const auto* points=next.value().index().cloud().x().data(); const u64 owners=owner.used();
    FullTimings times; times.memo_capacity=7; times.memo_slot_bytes=11; times.memo_reserved_bytes=13;
    for (auto& t : times.orders) { t={2,3,5,7}; }
    const auto preserved=times;
    fail_at=calls+i;
    auto refused=build_full(std::move(next.value()),work,&times,FullParams{8});
    fail_at=std::numeric_limits<u64>::max();
    CHECK(!refused.ok() && refused.outcome().reason==Reason::memory_budget);
    CHECK(times==preserved); CHECK_EQ(work.used(),held); CHECK_EQ(owner.used(),owners);
    CHECK(next.value().index().cloud().x().data()==points);
    CHECK(kept.value().order(1).nodes().data()==saved);
  }
  auto next=domain_of(line(),owner,4); REQUIRE(next.ok());
  auto success=build_full(std::move(next.value()),work,nullptr,FullParams{8}); REQUIRE(success.ok());
  for (Order k=1;k<=4;++k) CHECK(same(success.value().order(k),kept.value().order(k)));
}
MHGP11_TEST_MAIN()
