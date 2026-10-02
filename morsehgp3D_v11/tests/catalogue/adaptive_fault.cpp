// Injection de chaque allocation du plan adaptatif et de son diagnostic proprietaire ; publication transactionnelle.
// Compteur atomique partage : la k-ieme allocation echoue une fois, dans le pilote ou dans un worker quelconque.
#include <array>
#include <atomic>
#include <cstdlib>
#include <new>
#include <thread>

#include "catalogue/catalogue.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

namespace {
std::atomic<long long> remaining{-1};
std::atomic<unsigned long long> calls{0}, injections{0};
std::atomic<unsigned long long> off_main_injections{0};
const std::thread::id pilot = std::this_thread::get_id();

bool deny() noexcept {
  calls.fetch_add(1);
  if (remaining.load() >= 0 && remaining.fetch_sub(1) == 0) {
    injections.fetch_add(1);
    if (std::this_thread::get_id() != pilot) off_main_injections.fetch_add(1);
    return true;
  }
  return false;
}
}

[[gnu::noinline]] void* operator new(std::size_t size) {
  if (deny()) throw std::bad_alloc();
  if (void* block = std::malloc(size == 0 ? 1 : size)) return block;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(size == 0 ? 1 : size);
}
[[gnu::noinline]] void operator delete(void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp11;

MHGP11_TEST(allocations, 40) {
  std::array<u32, 17> x{}, y{}, z{};
  std::array<PointId, 17> ids{};
  for (u32 i = 0; i < 17; ++i) {
    x[i] = i == 0 ? 0 : u32{1} << (i - 1);
    ids[i] = PointId{i};
  }
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, owner);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 1; params.leaf_size = 4; params.adaptive_frontier = true;
  for (u32 workers : {1u, 4u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget work(MemoryBudget::kUnlimited);
    Buffer<u32> sentinel;
    REQUIRE(sentinel.allocate(17, work).ok());
    sentinel[0] = 123;
    CatalogueDiagnostics diagnostic;
    {
      auto seeded = build_catalogue(cloud.value(), params, work, *pool.value(), nullptr, &diagnostic);
      REQUIRE(seeded.ok());
    }
    const auto* previous_tasks = diagnostic.tasks().data();
    const u64 base = work.used();
    unsigned long long count = 0;
    {
      const auto before = calls.load();
      auto result = build_catalogue(cloud.value(), params, work, *pool.value(), nullptr, &diagnostic);
      count = calls.load() - before;
      REQUIRE(result.ok());
      CHECK_EQ(result.value().balls(), 16u);
      CHECK(result.value().ledger().max_depth > 8);
    }
    REQUIRE(count >= 12 && count <= 1024);
    CHECK(diagnostic.planning().adaptive);
    CHECK_EQ(work.used(), base);
    const auto* frozen_tasks = diagnostic.tasks().data();
    CHECK(frozen_tasks != previous_tasks);
    unsigned long long refused = 0, clean = 0, triggered = 0, preserved = 0;
    const auto off_main_before = off_main_injections.load();
    for (unsigned long long i = 0; i < count; ++i) {
      const auto previous = injections.load();
      remaining.store(static_cast<long long>(i));
      {
        auto result = build_catalogue(cloud.value(), params, work, *pool.value(), nullptr, &diagnostic);
        remaining.store(-1);
        refused += !result.ok() && result.outcome().reason == Reason::memory_budget ? 1u : 0u;
      }
      clean += work.used() == base ? 1u : 0u;
      triggered += injections.load() - previous == 1 ? 1u : 0u;
      preserved += diagnostic.tasks().data() == frozen_tasks ? 1u : 0u;
    }
    CHECK_EQ(refused, count);
    CHECK_EQ(clean, count);
    CHECK_EQ(triggered, count);
    CHECK_EQ(preserved, count);
    CHECK_EQ(sentinel[0], 123u);
    {
      auto recovered = build_catalogue(cloud.value(), params, work, *pool.value());
      REQUIRE(recovered.ok());
      CHECK_EQ(recovered.value().balls(), 16u);
    }
    CHECK_EQ(work.used(), base);
    { CatalogueDiagnostics empty; diagnostic.swap(empty); }
    sentinel.reset();
    CHECK(work.released().ok());
    CHECK_EQ(cloud.value().sites(), 17u);
    std::printf("catalogue_adaptive_allocations workers=%u allocations=%llu refused=%llu clean=%llu injected=%llu "
                "off_main=%llu\n", workers, count, refused, clean, triggered,
                off_main_injections.load() - off_main_before);
  }
}

MHGP11_TEST_MAIN()
