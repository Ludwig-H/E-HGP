// Budget global avant allocation des slots et refus injectes sans publication de sortie ou diagnostics.
#include <atomic>
#include <cstdlib>
#include <new>
#include "small_pair_graph_support.hpp"
#include "test.hpp"
namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injected{0};
bool deny() noexcept {
  if (calls.fetch_add(1)!=fail_at.load()) return false;
  injected.fetch_add(1); return true;
}
}
[[gnu::noinline]] void* operator new(std::size_t n) {
  if (deny()) throw std::bad_alloc();
  void* p=std::malloc(n==0 ? 1:n);
  if (p==nullptr) { throw std::bad_alloc(); }
  return p;
}
[[gnu::noinline]] void* operator new(std::size_t n,const std::nothrow_t&) noexcept {
  return deny() ? nullptr:std::malloc(n==0 ? 1:n);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p,std::size_t) noexcept { std::free(p); }
using namespace pair_test;

MHGP11_TEST(memory, 130) {
  for (u32 capacity:{1u,32u,33u,64u,1024u}) for (bool cache:{false,true}) {
    u64 before=0,after=0;
    REQUIRE(workspace_memory_bound(capacity,1,cache,before).ok());
    REQUIRE(workspace_memory_bound(capacity,1,cache,after,true).ok()); CHECK_EQ(after-before,256u);
    MemoryBudget exact(after),short_budget(after-1);
    {
      Workspace scratch; REQUIRE(scratch.allocate(capacity,exact,cache,true).ok());
      CHECK_EQ(exact.used(),after); CHECK_EQ(scratch.pair_rows.size(),32u);
    }
    CHECK(exact.released().ok());
    const u64 prior=calls.load(); Workspace refused;
    CHECK_EQ(refused.allocate(capacity,short_budget,cache,true).reason,Reason::memory_budget);
    CHECK_EQ(calls.load(),prior); CHECK(short_budget.released().ok());
    for (u32 workers:{0u,1u,4u,48u}) {
      u64 a=0,b=0; REQUIRE(workspace_memory_bound(capacity,workers,cache,a).ok());
      REQUIRE(workspace_memory_bound(capacity,workers,cache,b,true).ok()); CHECK_EQ(b-a,256*u64(workers));
    }
  }
}

MHGP11_TEST(all_slots, 40) {
  for (u32 workers:{1u,4u,48u}) {
    u64 bytes=0; REQUIRE(workspace_memory_bound(32,workers,true,bytes,true).ok());
    MemoryBudget exact(bytes+17),short_budget(bytes+16);
    Buffer<u8> old,old_short; REQUIRE(old.allocate(17,exact).ok()); REQUIRE(old_short.allocate(17,short_budget).ok());
    old[0]=71; old_short[0]=79;
    const u64 before=calls.load(); CHECK_EQ(short_budget.admit(bytes).reason,Reason::memory_budget);
    CHECK_EQ(calls.load(),before); CHECK_EQ(short_budget.used(),17u); CHECK_EQ(old_short[0],79u);
    REQUIRE(exact.admit(bytes).ok());
    {
      std::array<Workspace,48> slots;
      for (u32 i=0;i<workers;++i) REQUIRE(slots[i].allocate(32,exact,true,true).ok());
      CHECK_EQ(exact.used(),bytes+17); CHECK_EQ(exact.peak(),bytes+17); CHECK_EQ(old[0],71u);
    }
    CHECK_EQ(exact.used(),17u); old.reset(); old_short.reset();
    CHECK(exact.released().ok()); CHECK(short_budget.released().ok());
  }
}

MHGP11_TEST(pair_allocation, 14) {
  MemoryBudget budget(MemoryBudget::kUnlimited); Buffer<u8> kept;
  REQUIRE(kept.allocate(17,budget).ok()); kept[0]=83;
  const u64 previous=injected.load();
  {
    Workspace scratch;
    fail_at.store(calls.load()+6);  // points, dominance, dominated, I, U, cache ; puis les 32 mots du graphe
    const auto issue=scratch.allocate(32,budget,true,true);
    fail_at.store(std::numeric_limits<u64>::max());
    CHECK_EQ(issue.reason,Reason::memory_budget); CHECK_EQ(injected.load(),previous+1);
    CHECK_EQ(scratch.center_lines.size(),4960u); CHECK_EQ(scratch.pair_rows.size(),0u);
    CHECK_EQ(kept[0],83u);
  }
  CHECK_EQ(budget.used(),17u);
  {
    Workspace scratch; const u64 before=calls.load();
    REQUIRE(scratch.allocate(32,budget,true,true).ok()); CHECK_EQ(calls.load()-before,7u);
    CHECK_EQ(scratch.pair_rows.size(),32u);
    auto graph=SmallPairGraph::make(scratch.pair_rows.span(),32,true); REQUIRE(graph.ok());
    graph.value().connect(0,31); CHECK_EQ(graph.value().neighbors(0),u64{1}<<31);
  }
  CHECK_EQ(budget.used(),17u); kept.reset(); CHECK(budget.released().ok());
}

MHGP11_TEST(starvation, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=cloud_of(fixtures()[5],owner); REQUIRE(cloud.ok());
  CatalogueParams p; p.kmax=3; p.leaf_size=6; p.cache_center_lines=true;
  auto kept=build_catalogue(cloud.value(),p,work); REQUIRE(kept.ok());
  const auto* saved=kept.value().balls_data().data(); p.pair_graph=true;
  for (u32 workers:{1u,4u}) for (bool single:{false,true}) {
    auto pool=sched::make_pool({workers}); REQUIRE(pool.ok());
    p.single_pass=single; p.adaptive_frontier=single;
    CatalogueDiagnostics d; CatalogueTimings t;
    u64 allocations=0;
    {
      const u64 before=calls.load();
      auto good=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d);
      allocations=calls.load()-before; REQUIRE(good.ok()); CHECK(same_geometry(good.value(),kept.value()));
    }
    REQUIRE(allocations>=6 && allocations<=4096);
    const u64 held=work.used(); const auto prior_times=times(t); const auto* prior_diag=d.tasks().data();
    u64 refused=0,clean=0,unchanged=0,triggered=0;
    for (u64 pos=0;pos<allocations;++pos) {
      const u64 previous=injected.load(); fail_at.store(calls.load()+pos);
      {
        auto attempt=build_catalogue(cloud.value(),p,work,*pool.value(),&t,&d);
        fail_at.store(std::numeric_limits<u64>::max());
        refused+=!attempt.ok() && attempt.outcome().reason==Reason::memory_budget ? 1u:0u;
      }
      clean+=work.used()==held ? 1u:0u;
      unchanged+=times(t)==prior_times && d.tasks().data()==prior_diag ? 1u:0u;
      triggered+=injected.load()==previous+1 ? 1u:0u;
    }
    CHECK_EQ(refused,allocations); CHECK_EQ(clean,allocations); CHECK_EQ(unchanged,allocations); CHECK_EQ(triggered,allocations);
    CHECK(kept.value().balls_data().data()==saved);
    auto recovered=build_catalogue(cloud.value(),p,work,*pool.value()); REQUIRE(recovered.ok());
    CHECK(same_geometry(recovered.value(),kept.value())); CHECK(same_work(kept.value().ledger(),recovered.value().ledger()));
  }
}
MHGP11_TEST_MAIN()
