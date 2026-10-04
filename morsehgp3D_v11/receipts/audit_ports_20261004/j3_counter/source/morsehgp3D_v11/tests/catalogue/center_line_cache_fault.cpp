// La nouvelle allocation J2 echoue vraiment, puis toutes les reservations du brouillon sont rendues.
#include <atomic>
#include <cstdlib>
#include <new>

#include "catalogue/internal.hpp"
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
}  // namespace

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

MHGP11_TEST(allocation, 13) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Buffer<u8> sentinel;
  REQUIRE(sentinel.allocate(20, budget).ok());
  sentinel[0] = 123;
  const auto before = calls.load();
  {
    catalogue_detail::Workspace work;
    REQUIRE(work.allocate(32, budget, true).ok());
    CHECK_EQ(calls.load() - before, 6u);
    CHECK_EQ(work.center_lines.size(), 4960u);
  }
  const auto triggered = injections.load();
  {
    catalogue_detail::Workspace work;
    remaining.store(5);  // points, dominance, dominated, I, U passent ; le Buffer du cache est refuse.
    const auto result = work.allocate(32, budget, true);
    remaining.store(-1);
    CHECK_EQ(result.reason, Reason::memory_budget);
    CHECK_EQ(injections.load() - triggered, 1u);
    CHECK_EQ(work.center_lines.size(), 0u);
    CHECK_EQ(sentinel[0], 123u);
  }
  CHECK_EQ(budget.used(), 20u);
  {
    catalogue_detail::Workspace recovered;
    REQUIRE(recovered.allocate(32, budget, true).ok());
    CHECK_EQ(recovered.center_lines.size(), 4960u);
  }
  CHECK_EQ(budget.used(), 20u);
  sentinel.reset();
  CHECK(budget.released().ok());
}

MHGP11_TEST_MAIN()
