// Table verticale temporaire : admission exacte, refus transactionnels et coexistence avec les anciens resultats.
#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>
#include "census_reuse_support.hpp"
#include "tower/forest_internal.hpp"
#include "tower/regular_vertical_seeds.hpp"
#include "test.hpp"
namespace {
using mhgp11::u64;
std::atomic<u64> calls{0}, fail_at{std::numeric_limits<u64>::max()}, injections{0};
bool deny() noexcept {
  if (calls.fetch_add(1) != fail_at.load()) return false;
  injections.fetch_add(1); return true;
}
}
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
using namespace census_reuse_test;

namespace {
Input diamond() { return Input({{0,1,0},{1,0,0},{2,1,0},{1,2,0}}); }
Input all_k_line() {
  std::vector<Xyz> xyz;
  for (u32 i = 0; i < 12; ++i) xyz.push_back({2*i,0,0});
  return Input(xyz);
}
FullParams parameters(bool parallel) {
  FullParams p{8,parallel ? 2u : 0u,parallel ? 4u : 1u,parallel ? 8u : 0u,parallel,true,true};
  p.reuse_regular_verticals = true; return p;
}
FullTimings reuse_sentinel() {
  auto t = scratch_sentinel(); t.reuse_regular_verticals = true;
  t.regular_vertical_reserved_bytes = 149; return t;
}
u64 all_k_retained() {
  // Ligne de 12 sites : M=66, b_k=13-k, capacites fixes et table dense par ordre.
  u64 bytes = 0;
  for (u64 k = 1; k <= 12; ++k) {
    const u64 b = 13-k;
    bytes += 24*(2*b-1)+4*(2*b-2)+4*(k == 1 ? 12 : 66)+(k == 1 ? 0 : 4*(2*b-1));
  }
  return bytes;
}
bool prior_intact(const Buffer<u8>& prior, const u8* address) {
  return prior.data() == address &&
      std::all_of(prior.span().begin(),prior.span().end(),[](u8 value) { return value == 91; });
}
u64 reuses(const FullTower& full) {
  u64 total = 0;
  for (Order k = 1; k <= full.kmax(); ++k) total += full.order(k).ledger().vertical_reuses;
  return total;
}
}  // namespace
static_assert(sizeof(NodeIdx) == 4 && sizeof(ForestNode) == 24);

MHGP11_TEST(factory, 30) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto domain = domain_of(diamond(),owner,4); REQUIRE(domain.ok());
  const u64 bytes = 4*u64{domain.value().catalogue().balls()}, owners = owner.used();
  REQUIRE(bytes > 0);
  for (u64 deficit : {u64{1},u64{0}}) {
    MemoryBudget work(17+bytes-deficit); Buffer<u8> prior; REQUIRE(prior.allocate(17,work).ok());
    std::fill(prior.span().begin(),prior.span().end(),u8{91}); const auto* address = prior.data();
    const u64 before = calls.load();
    {
      auto table = RegularVerticalSeeds::make(domain.value(),work);
      if (deficit) {
        CHECK_EQ(table.outcome().reason,Reason::memory_budget); CHECK_EQ(calls.load(),before);
        CHECK_EQ(work.used(),17u); CHECK_EQ(work.peak(),17u);
      } else {
        REQUIRE(table.ok()); CHECK(table.value().belongs_to(domain.value()));
        CHECK_EQ(table.value().reserved_bytes(),bytes); CHECK_EQ(calls.load(),before+1);
        CHECK_EQ(work.used(),17+bytes); CHECK_EQ(work.peak(),17+bytes);
      }
      CHECK_EQ(owner.used(),owners); CHECK(prior_intact(prior,address));
    }
    CHECK_EQ(work.used(),17u);
  }
  MemoryBudget work(17+bytes); Buffer<u8> prior; REQUIRE(prior.allocate(17,work).ok());
  std::fill(prior.span().begin(),prior.span().end(),u8{91}); const auto* address = prior.data();
  const u64 before = calls.load(), injected = injections.load(); fail_at.store(before);
  auto refused = RegularVerticalSeeds::make(domain.value(),work);
  fail_at.store(std::numeric_limits<u64>::max());
  CHECK_EQ(refused.outcome().reason,Reason::memory_budget); CHECK_EQ(calls.load(),before+1);
  CHECK_EQ(injections.load(),injected+1); CHECK_EQ(work.used(),17u); CHECK_EQ(work.peak(),17+bytes);
  CHECK_EQ(owner.used(),owners); CHECK(prior_intact(prior,address));
  {
    auto recovered = RegularVerticalSeeds::make(domain.value(),work); REQUIRE(recovered.ok());
    CHECK_EQ(recovered.value().reserved_bytes(),bytes); CHECK_EQ(work.used(),17+bytes);
    auto other = domain_of(Input({{1,2,3},{3,2,1}}),owner,1); REQUIRE(other.ok());
    OrderTimings times{7,11,13,17}; const auto initial = times; const u64 before_run = calls.load();
    ForestBuilder builder(other.value(),1,work,&times,nullptr,nullptr,false,&recovered.value());
    auto foreign = builder.run(); CHECK_EQ(foreign.outcome().reason,Reason::parameter_out_of_range);
    CHECK_EQ(calls.load(),before_run); CHECK_EQ(work.used(),17+bytes); CHECK(times == initial);
  }
  CHECK_EQ(work.used(),17u);
}

MHGP11_TEST(all_k_memory, 80) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({1}); REQUIRE(pool.ok());
  const auto input = all_k_line(); const auto params = parameters(true);
  const u64 retained_bytes = all_k_retained(); u64 peak = 0, held = 0;
  {
    Buffer<u8> prior; REQUIRE(prior.allocate(17,work).ok());
    auto kept = tower_of(diamond(),4,owner,work); REQUIRE(kept.ok()); held = work.used();
    const auto* old = kept.value().order(1).nodes().data();
    auto domain = domain_of(input,owner,12); REQUIRE(domain.ok());
    CHECK_EQ(domain.value().catalogue().balls(),66u); work.restart_peak();
    auto times = reuse_sentinel();
    {
      auto made = build_full(std::move(domain.value()),work,&times,params,pool.value().get()); REQUIRE(made.ok());
      CHECK_EQ(work.used(),held+retained_bytes); peak = work.peak();
      CHECK(times.reuse_regular_verticals); CHECK_EQ(times.regular_vertical_reserved_bytes,4u*66u);
      CHECK_EQ(times.memo_reserved_bytes,8*DescentMemo::slot_bytes());
      CHECK_EQ(times.lane_memo_reserved_bytes,32*DescentMemo::slot_bytes());
      CHECK_EQ(times.census_workspaces,1u); CHECK_EQ(times.census_workspace_reserved_bytes,4u*12u);
      CHECK(peak >= held+retained_bytes+4*66+40*DescentMemo::slot_bytes()+4*12);
      CHECK_EQ(reuses(made.value()),66u);
      for (Order k = 1; k <= 12; ++k) {
        CHECK_EQ(made.value().order(k).births(),13u-k); CHECK(structure(made.value().order(k)));
        CHECK_EQ(made.value().order(k).ledger().vertical_descents,0u);
        CHECK_EQ(made.value().order(k).lookup_reserved_bytes(),4u*(k == 1 ? 12u : 66u));
      }
      CHECK(kept.value().order(1).nodes().data() == old); CHECK(structure(kept.value().order(1)));
    }
    CHECK_EQ(work.used(),held);
  }
  CHECK(work.released().ok()); CHECK(owner.released().ok());
  for (u64 deficit : {u64{1},u64{0}}) {
    MemoryBudget exact(peak-deficit); Buffer<u8> prior; REQUIRE(prior.allocate(17,exact).ok());
    std::fill(prior.span().begin(),prior.span().end(),u8{91}); const auto* prior_address = prior.data();
    auto kept = tower_of(diamond(),4,owner,exact); REQUIRE(kept.ok()); CHECK_EQ(exact.used(),held);
    const auto* old = kept.value().order(1).nodes().data();
    auto domain = domain_of(input,owner,12); REQUIRE(domain.ok());
    const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
    auto times = reuse_sentinel(); const auto before = times; exact.restart_peak();
    {
      auto made = build_full(std::move(domain.value()),exact,&times,params,pool.value().get());
      if (deficit) {
        CHECK_EQ(made.outcome().reason,Reason::memory_budget); CHECK(times == before);
        CHECK_EQ(exact.used(),held); CHECK_EQ(owner.used(),owners);
        CHECK(domain.value().index().cloud().x().data() == saved);
      } else {
        REQUIRE(made.ok()); CHECK_EQ(exact.used(),held+retained_bytes); CHECK_EQ(exact.peak(),peak);
        CHECK(times.reuse_regular_verticals); CHECK_EQ(times.regular_vertical_reserved_bytes,4u*66u);
        CHECK_EQ(reuses(made.value()),66u); CHECK_EQ(domain.value().catalogue().kmax(),0u);
      }
      CHECK(prior_intact(prior,prior_address)); CHECK(kept.value().order(1).nodes().data() == old);
      CHECK(structure(kept.value().order(1)));
    }
    CHECK_EQ(exact.used(),held);
  }
  std::printf("regular_vertical_memory peak=%llu retained=%llu table=264 all_k=12\n",
              static_cast<unsigned long long>(peak),static_cast<unsigned long long>(retained_bytes));
}

MHGP11_TEST(starvation, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const auto input = diamond(); auto kept = tower_of(input,4,owner,work); REQUIRE(kept.ok());
  const auto* old = kept.value().order(1).nodes().data(); const u64 held = work.used();
  for (u32 workers : {1u,4u}) {
    auto pool = sched::make_pool({workers}); REQUIRE(pool.ok());
    const auto params = parameters(workers != 1); u64 allocations = 0;
    {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok()); const u64 before = calls.load();
      auto full = build_full(std::move(domain.value()),work,nullptr,params,pool.value().get());
      allocations = calls.load()-before; REQUIRE(full.ok());
      CHECK(same(full.value().order(2),kept.value().order(2))); CHECK(reuses(full.value()) > 0);
    }
    REQUIRE(allocations >= 20 && allocations <= 4096); CHECK_EQ(work.used(),held);
    u64 refused_count = 0, restored = 0, triggered = 0, unchanged = 0;
    for (u64 position = 0; position < allocations; ++position) {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok());
      const auto* saved = domain.value().index().cloud().x().data(); const u64 owners = owner.used();
      auto times = reuse_sentinel(); const auto before = times; const u64 injected = injections.load();
      fail_at.store(calls.load()+position);
      {
        auto refused = build_full(std::move(domain.value()),work,&times,params,pool.value().get());
        fail_at.store(std::numeric_limits<u64>::max());
        refused_count += !refused.ok() && refused.outcome().reason == Reason::memory_budget ? 1u : 0u;
      }
      restored += work.used() == held && owner.used() == owners ? 1u : 0u;
      triggered += injections.load() == injected+1 ? 1u : 0u;
      unchanged += times == before && domain.value().index().cloud().x().data() == saved ? 1u : 0u;
    }
    CHECK_EQ(refused_count,allocations); CHECK_EQ(restored,allocations);
    CHECK_EQ(triggered,allocations); CHECK_EQ(unchanged,allocations);
    CHECK(kept.value().order(1).nodes().data() == old); CHECK(structure(kept.value().order(1)));
    {
      auto domain = domain_of(input,owner,4); REQUIRE(domain.ok()); auto times = reuse_sentinel();
      const u64 table_bytes = 4*u64{domain.value().catalogue().balls()};
      auto recovered = build_full(std::move(domain.value()),work,&times,params,pool.value().get()); REQUIRE(recovered.ok());
      CHECK(times.reuse_regular_verticals); CHECK_EQ(times.regular_vertical_reserved_bytes,table_bytes);
      CHECK(reuses(recovered.value()) > 0);
      for (Order k = 1; k <= 4; ++k) CHECK(same(recovered.value().order(k),kept.value().order(k)));
    }
    CHECK_EQ(work.used(),held);
    std::printf("regular_vertical_allocations workers=%u allocations=%llu refused=%llu restored=%llu injected=%llu\n",
                workers,static_cast<unsigned long long>(allocations),static_cast<unsigned long long>(refused_count),
                static_cast<unsigned long long>(restored),static_cast<unsigned long long>(triggered));
  }
}

MHGP11_TEST(inactive, 25) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,0,0},{2,0,0}}); u64 allocations = 0, peak = 0;
  std::optional<FullTower> baseline;
  for (bool enabled : {false,true}) {
    auto domain = domain_of(input,owner,1); REQUIRE(domain.ok());
    REQUIRE(domain.value().catalogue().balls() > 0);
    auto params = parameters(false); params.reuse_regular_verticals = enabled;
    auto times = reuse_sentinel(); const u64 before = calls.load(), held = work.used(); work.restart_peak();
    auto made = build_full(std::move(domain.value()),work,&times,params); REQUIRE(made.ok());
    const u64 count = calls.load()-before;
    CHECK_EQ(times.reuse_regular_verticals,enabled); CHECK_EQ(times.regular_vertical_reserved_bytes,0u);
    CHECK_EQ(reuses(made.value()),0u); CHECK_EQ(made.value().order(1).ledger().vertical_descents,0u);
    if (!enabled) {
      allocations = count; peak = work.peak()-held; baseline.emplace(std::move(made.value()));
    } else {
      REQUIRE(baseline.has_value()); CHECK(same(made.value().order(1),baseline->order(1)));
      CHECK(made.value().order(1).ledger() == baseline->order(1).ledger());
      CHECK_EQ(count,allocations); CHECK_EQ(work.peak()-held,peak);
    }
  }
  auto domain = domain_of(diamond(),owner,4); REQUIRE(domain.ok());
  auto params = parameters(false); params.reuse_regular_verticals = false;
  auto times = reuse_sentinel(); auto made = build_full(std::move(domain.value()),work,&times,params); REQUIRE(made.ok());
  CHECK(!times.reuse_regular_verticals); CHECK_EQ(times.regular_vertical_reserved_bytes,0u);
  CHECK_EQ(reuses(made.value()),0u); CHECK(made.value().order(2).ledger().vertical_descents > 0);
}
MHGP11_TEST_MAIN()
