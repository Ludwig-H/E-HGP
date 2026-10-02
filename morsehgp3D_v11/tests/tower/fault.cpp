// MEB sans allocation, puis refus de chaque allocation du wrapper sans publication ni reservation residuelle.
#include <cstdlib>
#include <new>

#include "test_support.hpp"
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
using namespace tower_test;

MHGP11_TEST(starvation, 30) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  std::vector<Xyz> points;
  for (u32 i = 0; i < 12; ++i) points.push_back({i, 0, 0});
  auto line = Input(points).prepare(budget);
  REQUIRE(line.ok());
  auto selected = all(line.value());
  const auto before = calls;
  countdown = 0;
  auto meb = bounded_meb(line.value(), selected);
  countdown = -1;
  REQUIRE(meb.ok());
  CHECK_EQ(calls, before);
  CHECK_EQ(meb.value().ledger().presentations, 793u);
  CHECK(level_is(meb.value().sphere(), 121, 4));
  auto cloud = square().prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  const auto part = all(index.value().cloud());
  const auto base = budget.used();
  unsigned long long refusals = 0;
  for (u32 threshold : {1u, 2u}) {
    unsigned long long count = 0;
    {
      const auto start = calls;
      const auto result = meb_census(index.value(), part, threshold, budget);
      count = calls - start;
      REQUIRE(result.ok());
      CHECK_EQ(count, threshold == 1 ? 1u : 2u);
      CHECK(square_census(index.value().cloud(), result.value().population(), threshold));
    }
    CHECK_EQ(budget.used(), base);
    for (unsigned long long i = 0; i < count; ++i) {
      countdown = static_cast<long>(i);
      const auto refused = meb_census(index.value(), part, threshold, budget);
      countdown = -1;
      CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
      CHECK_EQ(budget.used(), base);
      CHECK_EQ(index.value().cloud().sites(), 5u);
      ++refusals;
    }
    {
      const auto recovered = meb_census(index.value(), part, threshold, budget);
      REQUIRE(recovered.ok());
      CHECK(square_census(index.value().cloud(), recovered.value().population(), threshold));
    }
    CHECK_EQ(budget.used(), base);
  }
  CHECK_EQ(refusals, 3u);
  const auto start = calls;
  countdown = 0;
  const auto invalid = meb_census(index.value(), {}, 0, budget);
  countdown = -1;
  CHECK(!invalid.ok() && invalid.outcome().reason == Reason::parameter_out_of_range);
  CHECK_EQ(calls, start);
  std::printf("meb_allocations=0 wrapper_allocations=1,2 injected_refusals=%llu\n", refusals);
}

MHGP11_TEST_MAIN()
