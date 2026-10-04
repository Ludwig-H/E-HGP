// Toutes positions d'allocation, metadata/payload/compactage inclus ; jointure avant liberation des brouillons.
#include <atomic>
#include <cstdlib>
#include <new>
#include <thread>
#include "single_pass_support.hpp"
#include "test.hpp"
namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injected{0}, off_pilot{0};
const auto pilot=std::this_thread::get_id();
bool deny() noexcept {
  if (calls.fetch_add(1)!=fail_at.load()) return false;
  injected.fetch_add(1);
  if (std::this_thread::get_id()!=pilot) off_pilot.fetch_add(1);
  return true;
}
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p=std::malloc(n==0 ? 1 : n);
  if (p==nullptr) { throw std::bad_alloc(); }
  return p;
}
[[gnu::noinline]] void* operator new(std::size_t n,const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(n==0 ? 1 : n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p,std::size_t) noexcept { std::free(p); }
using namespace single_test;

MHGP11_TEST(append_atomic, 280) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  SinglePassOutput output;
  for (u32 i=0;i<256;++i) REQUIRE(append(output,budget).ok());
  const u64 held=budget.used();
  for (u64 offset=0;offset<4;++offset) {
    // Deux nouvelles pages simultanees : metadata puis payload de chaque chaine.
    fail_at.store(calls.load()+offset);
    const auto issue=append(output,budget);
    fail_at.store(std::numeric_limits<u64>::max());
    CHECK_EQ(issue.reason,Reason::memory_budget); CHECK_EQ(budget.used(),held);
    CHECK_EQ(output.balls(),256u); CHECK_EQ(output.incidences(),2048u);
    std::array<Emission,256> records; std::array<SiteIdx,2048> population;
    REQUIRE(output.compact(records,population,0).ok());
    CHECK_EQ(records.back().population_begin,2040u); CHECK_EQ(population.back(),ids.back());
  }
  REQUIRE(append(output,budget).ok()); CHECK_EQ(output.balls(),257u);
}

MHGP11_TEST(allocations, 24) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=cloud_of({{0,0,0},{4,0,0},{0,4,0},{4,4,0},{0,0,4},{4,0,4},{0,4,4},{4,4,4}},owner);
  REQUIRE(cloud.ok()); CatalogueParams p; p.kmax=3; p.leaf_size=6;
  auto kept=build_catalogue(cloud.value(),p,work); REQUIRE(kept.ok());
  const auto* saved=kept.value().balls_data().data();
  p.single_pass=true;
  for (u32 workers:{1u,4u}) {
    auto pool=sched::make_pool({workers}); REQUIRE(pool.ok());
    CatalogueDiagnostics d; CatalogueTimings t;
    u64 allocations=0;
    {
      const u64 before=calls.load();
      auto good=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d);
      allocations=calls.load()-before; REQUIRE(good.ok()); CHECK(same(good.value(),kept.value()));
    }
    REQUIRE(allocations>=12 && allocations<=4096);
    const u64 held=work.used(); const auto old_times=times(t); const auto* old_diag=d.tasks().data();
    u64 refused=0,clean=0,unchanged=0,triggered=0; const u64 off_before=off_pilot.load();
    for (u64 pos=0;pos<allocations;++pos) {
      const u64 previous=injected.load();
      fail_at.store(calls.load()+pos);
      {
        auto attempt=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d);
        fail_at.store(std::numeric_limits<u64>::max());
        refused+=!attempt.ok() && attempt.outcome().reason==Reason::memory_budget ? 1u:0u;
      }
      clean+=work.used()==held ? 1u:0u;
      unchanged+=times(t)==old_times && d.tasks().data()==old_diag ? 1u:0u;
      triggered+=injected.load()==previous+1 ? 1u:0u;
    }
    CHECK_EQ(refused,allocations); CHECK_EQ(clean,allocations); CHECK_EQ(unchanged,allocations); CHECK_EQ(triggered,allocations);
    CHECK(kept.value().balls_data().data()==saved);
    auto recovered=build_catalogue(cloud.value(),p,work,*pool.value()); REQUIRE(recovered.ok());
    CHECK(same(recovered.value(),kept.value()));
    std::printf("single_pass_allocations workers=%u allocations=%llu off_pilot=%llu\n",workers,
                static_cast<unsigned long long>(allocations),static_cast<unsigned long long>(off_pilot.load()-off_before));
  }
}

MHGP11_TEST(late_budget, 14) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=cloud_of({{0,0,0},{2,0,0},{4,0,0},{6,0,0},{8,0,0}},owner); REQUIRE(cloud.ok());
  auto pool=sched::make_pool({4}); REQUIRE(pool.ok());
  CatalogueParams p; p.kmax=1; p.single_pass=true;
  CatalogueDiagnostics d; CatalogueTimings t;
  auto kept=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d); REQUIRE(kept.ok());
  const u64 peak=work.peak(); REQUIRE(peak>0);
  const auto old_times=times(t); const auto* old_diag=d.tasks().data();
  const auto* saved=kept.value().balls_data().data(); const auto cost=kept.value().execution();
  MemoryBudget limited(peak-1);
  auto refused=build_catalogue(cloud.value(),p,limited,*pool.value(),&t,&d);
  CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason,Reason::memory_budget);
  // Les pages ont deja ete allouees : ce refus public n'est pas une admission complete avant calcul.
  CHECK(limited.peak()>=cost.arena_capacity_bytes+cost.arena_metadata_bytes);
  CHECK(limited.released().ok()); CHECK(times(t)==old_times); CHECK(d.tasks().data()==old_diag);
  CHECK(kept.value().balls_data().data()==saved); CHECK_EQ(kept.value().balls(),4u);
  auto again=build_catalogue(cloud.value(),p,work,*pool.value()); REQUIRE(again.ok());
  CHECK(same(again.value(),kept.value()));
}
MHGP11_TEST_MAIN()
