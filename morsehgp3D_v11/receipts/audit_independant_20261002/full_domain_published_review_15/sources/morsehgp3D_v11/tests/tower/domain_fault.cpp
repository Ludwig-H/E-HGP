// Refus de chaque allocation catalogue+lookup ; aucun transfert d'index avant le succes final.
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

namespace {
Result<GlobalIndex> fresh(MemoryBudget& budget) {
  auto cloud = square().prepare(budget);
  if (!cloud.ok()) return cloud.outcome();
  return build_index(std::move(cloud.value()), IndexParams{1}, budget);
}
}

MHGP11_TEST(starvation, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  unsigned long long cat_count = 0, count = 0;
  {
    auto index = fresh(owner);
    REQUIRE(index.ok());
    {
      const auto start = calls;
      auto cat = build_catalogue(index.value().cloud(), CatalogueParams{}, work);
      cat_count = calls - start;
      REQUIRE(cat.ok());
    }
    CHECK(work.released().ok());
    const auto start = calls;
    auto domain = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    count = calls - start;
    REQUIRE(domain.ok());
    CHECK_EQ(count, cat_count + 1);
    CHECK(domain.value().lookup_capacity() > 0);
    CHECK_EQ(index.value().cloud().sites(), 0u);
    bool found = true;
    const auto before = calls;
    countdown = 0;
    for (u32 b = 0; b < domain.value().catalogue().balls(); ++b)
      found = found && domain.value().find_support(domain.value().catalogue().balls_data()[b].support) == BallIdx{b};
    const std::array<SiteIdx, 4> missing{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}};
    found = found && !domain.value().find_support(missing);
    countdown = -1;
    CHECK(found);
    CHECK_EQ(calls, before);
  }
  CHECK(work.released().ok());
  CHECK(owner.released().ok());
  REQUIRE(count >= 2 && count < 4096);
  unsigned long long refusals = 0, intact = 0;
  for (unsigned long long i = 0; i < count; ++i) {
    auto index = fresh(owner);
    REQUIRE(index.ok());
    const auto* xyz = index.value().cloud().x().data();
    const u64 used = owner.used();
    countdown = static_cast<long>(i);
    const auto refused = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    countdown = -1;
    if (!refused.ok() && refused.outcome().reason == Reason::memory_budget) ++refusals;
    else CHECK(false);
    if (index.value().cloud().sites() == 5 && index.value().cloud().x().data() == xyz &&
        owner.used() == used && work.released().ok()) ++intact;
    else CHECK(false);
    // i=count-1 vise uniquement la table : toutes les allocations du catalogue ont deja reussi.
    if (i + 1 == count) {
      auto recovery = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
      REQUIRE(recovery.ok());
      CHECK(recovery.value().lookup_capacity() > 0);
      CHECK_EQ(index.value().cloud().sites(), 0u);
    }
  }
  CHECK_EQ(refusals, count);
  CHECK_EQ(intact, count);
  CHECK(work.released().ok());
  CHECK(owner.released().ok());
  std::printf("domain_allocations=%llu catalogue_allocations=%llu lookup_allocations=1 refusals=%llu intact=%llu\n",
              count, cat_count, refusals, intact);
}

MHGP11_TEST_MAIN()
