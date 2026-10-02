// Injection de chacune des allocations, sur la vraie frontiere noexcept du catalogue.
#include <array>
#include <cstdlib>
#include <new>

#include "catalogue/catalogue.hpp"
#include "test.hpp"

namespace {
long countdown = -1;
unsigned long long calls = 0;
bool deny() noexcept {
  ++calls;
  if (countdown == 0) return true;
  if (countdown > 0) --countdown;
  return false;
}
}

[[gnu::noinline]] void* operator new(std::size_t size) {
  if (deny()) throw std::bad_alloc();
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) throw std::bad_alloc();
  return block;
}
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  return deny() ? nullptr : std::malloc(size == 0 ? 1 : size);
}
[[gnu::noinline]] void operator delete(void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp11;

MHGP11_TEST(starvation, 10) {
  const std::array<u32, 6> x{0, 2, 1, 1, 1, 1}, y{1, 1, 0, 2, 1, 1}, z{1, 1, 1, 1, 0, 2};
  const std::array<PointId, 6> ids{PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4}, PointId{5}};
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  REQUIRE(cloud.ok());
  const u64 base = budget.used();
  unsigned long long count = 0;
  u32 balls = 0;
  {
    const auto before = calls;
    auto result = build_catalogue(cloud.value(), CatalogueParams{}, budget);
    count = calls - before;
    REQUIRE(result.ok());
    balls = result.value().balls();
  }
  CHECK_EQ(budget.used(), base);
  REQUIRE(count >= 8 && count <= 4096);
  REQUIRE(balls > 0);
  unsigned long long refused = 0, clean = 0;
  for (unsigned long long i = 0; i < count; ++i) {
    countdown = static_cast<long>(i);
    {
      auto result = build_catalogue(cloud.value(), CatalogueParams{}, budget);
      countdown = -1;
      if (!result.ok() && result.outcome().reason == Reason::memory_budget) ++refused;
      else CHECK(false);
    }
    if (budget.used() == base) ++clean;
  }
  CHECK_EQ(refused, count);
  CHECK_EQ(clean, count);
  {
    auto recovery = build_catalogue(cloud.value(), CatalogueParams{}, budget);
    REQUIRE(recovery.ok());
    CHECK_EQ(recovery.value().balls(), balls);
  }
  CHECK_EQ(budget.used(), base);
  CHECK_EQ(cloud.value().sites(), 6u);
  std::printf("catalogue_allocations=%llu refusals=%llu clean=%llu\n", count, refused, clean);
}

MHGP11_TEST_MAIN()
