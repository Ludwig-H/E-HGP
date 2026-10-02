// Chaque allocation de construction et de census est refusee a son tour ; aucune publication partielle.
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
using namespace index_test;

MHGP11_TEST(starvation, 20) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto input = octa();
  auto cloud = input.prepare(budget);
  REQUIRE(cloud.ok());
  const auto coordinate_view = cloud.value().x();
  const auto before = budget.used();
  countdown = 0;
  auto refused = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  countdown = -1;
  CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
  CHECK_EQ(cloud.value().sites(), 11u);
  CHECK(cloud.value().x().data() == coordinate_view.data());
  CHECK_EQ(budget.used(), before);
  const auto build_start = calls;
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  const auto build_calls = calls - build_start;
  REQUIRE(index.ok());
  CHECK_EQ(build_calls, 1u);
  CHECK_EQ(cloud.value().sites(), 0u);
  const auto base = budget.used();
  for (u32 threshold : {1u, 4u}) {
    unsigned long long count = 0;
    {
      const auto begin = calls;
      auto good = census(index.value(), ball(), threshold, budget);
      count = calls - begin;
      REQUIRE(good.ok());
      CHECK(analytic(index.value().cloud(), good.value(), threshold));
      CHECK_EQ(count, threshold == 1 ? 1u : 2u);
    }
    CHECK_EQ(budget.used(), base);
    for (unsigned long long i = 0; i < count; ++i) {
      countdown = static_cast<long>(i);
      auto failed = census(index.value(), ball(), threshold, budget);
      countdown = -1;
      CHECK(!failed.ok() && failed.outcome().reason == Reason::memory_budget);
      CHECK_EQ(budget.used(), base);
      CHECK_EQ(index.value().cloud().sites(), 11u);
    }
    {
      auto recovered = census(index.value(), ball(), threshold, budget);
      REQUIRE(recovered.ok());
      CHECK(analytic(index.value().cloud(), recovered.value(), threshold));
    }
    CHECK_EQ(budget.used(), base);
  }
  // Une requete vide n'alloue aucune population, meme sous refus systeme total.
  const auto outside = num::Sphere::point(point(100, 100, 100));
  countdown = 0;
  const auto no_memory_needed = census(index.value(), outside, 1, budget);
  countdown = -1;
  REQUIRE(no_memory_needed.ok());
  CHECK(no_memory_needed.value().interior().empty() && no_memory_needed.value().shell().empty());
  CHECK_EQ(budget.used(), base);
  std::printf("index_allocations=%llu census_allocations=1,2 injected_refusals=4\n", build_calls);
}

MHGP11_TEST_MAIN()
