// Les deux allocations du tri sont payees, testees separement et rendues sur refus ; aucune allocation de tache.
#include <atomic>
#include <cstdlib>
#include <new>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/sort_indices.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

namespace {
std::atomic<long long> remaining{-1};
std::atomic<unsigned long long> calls{0}, injections{0};

bool deny() noexcept {
  calls.fetch_add(1);
  if (remaining.load() >= 0 && remaining.fetch_sub(1) == 0) {
    injections.fetch_add(1);
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
  std::vector<catalogue_detail::Emission> records(4097);
  for (u32 i = 0; i < records.size(); ++i) {
    auto level = num::Level::make(num::Wide<1>::from_u64(records.size() - i), num::Wide<1>::from_u64(1));
    if (!level.ok()) { CHECK(level.ok()); return; }
    records[i].level = level.value();
  }
  for (u32 workers : {1u, 4u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget work(MemoryBudget::kUnlimited);
    Buffer<u8> sentinel;
    REQUIRE(sentinel.allocate(7, work).ok());
    sentinel[0] = 123;
    const auto before = calls.load();
    {
      auto measured = catalogue_detail::sort_indices(records, work, pool.value().get());
      REQUIRE(measured.ok());
      CHECK_EQ(calls.load() - before, 2u);
      CHECK_EQ(work.used(), 7 + 4 * records.size());
      CHECK_EQ(work.peak(), 7 + 8 * records.size());
    }
    CHECK_EQ(work.used(), 7u);
    for (long long failed = 0; failed < 2; ++failed) {
      const auto triggered = injections.load();
      u64 comparisons = 123;
      remaining.store(failed);
      auto result = catalogue_detail::sort_indices(records, work, pool.value().get(), &comparisons);
      remaining.store(-1);
      CHECK_EQ(result.outcome().reason, Reason::memory_budget);
      CHECK_EQ(injections.load() - triggered, 1u);
      CHECK_EQ(comparisons, 123u);
      CHECK_EQ(work.used(), 7u);
      CHECK_EQ(sentinel[0], 123u);
    }
    {
      auto recovered = catalogue_detail::sort_indices(records, work, pool.value().get());
      REQUIRE(recovered.ok());
      CHECK_EQ(recovered.value()[0], records.size() - 1);
      CHECK_EQ(recovered.value()[records.size() - 1], 0u);
    }
    CHECK_EQ(work.used(), 7u);
    sentinel.reset();
    CHECK(work.released().ok());
  }
}

MHGP11_TEST_MAIN()
