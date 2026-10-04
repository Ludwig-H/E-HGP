// Pannes de metadata et des quatre sorties ; l'appel public conserve ses diagnostics jusqu'au succes.
#include <atomic>
#include <cstdlib>
#include <new>
#include "assembly_support.hpp"
#include "test.hpp"

namespace {
std::atomic<long long> remaining{-1};
std::atomic<mhgp11::u64> calls{0}, injections{0};
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

using namespace assembly_test;

MHGP11_TEST(allocations, 50) {
  auto four = sched::make_pool({4}); REQUIRE(four.ok());
  auto fixture_value = fixture(8193, 4097, true); REQUIRE(fixture_value.ok());
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  Input input; REQUIRE(input.load(fixture_value.value(), owner).ok());
  CatalogueParams params; params.parallel_assembly = true;
  const u64 before = calls.load();
  auto kept = cat::Assembly::finish(input.records, input.population, params, {}, work, nullptr, four.value().get());
  const u64 allocations = calls.load() - before;
  REQUIRE(kept.ok()); CHECK_EQ(allocations, 5u);
  const u64 held = work.used(), owners = owner.used();
  const auto* saved = kept.value().balls_data().data();
  for (u64 position = 0; position < allocations; ++position) {
    const u64 previous = injections.load();
    remaining.store(static_cast<long long>(position));
    auto refused = cat::Assembly::finish(input.records, input.population, params, {}, work, nullptr, four.value().get());
    remaining.store(-1);
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason, Reason::memory_budget);
    CHECK_EQ(injections.load() - previous, 1u); CHECK_EQ(work.used(), held);
    CHECK_EQ(owner.used(), owners); CHECK(kept.value().balls_data().data() == saved);
  }
  auto again = cat::Assembly::finish(input.records, input.population, params, {}, work, nullptr, four.value().get());
  REQUIRE(again.ok()); CHECK(same(again.value(), kept.value()));
  auto points = cloud(owner); REQUIRE(points.ok());
  CatalogueDiagnostics diagnostic;
  CatalogueTimings times;
  params.kmax = 3; params.indirect_sort = true;
  u64 full_allocations = 0;
  {
    const u64 start = calls.load();
    auto result = build_catalogue(points.value(), params, work, *four.value(), &times, &diagnostic);
    full_allocations = calls.load() - start;
    REQUIRE(result.ok());
  }
  CHECK(full_allocations > allocations);
  const auto saved_times = times;
  const auto* tasks = diagnostic.tasks().data();
  const u64 all_held = work.used();
  // Derniere sortie allouee, APRES le tri et le scan : aucune publication prematuree du brouillon.
  remaining.store(static_cast<long long>(full_allocations - 1));
  auto refused = build_catalogue(points.value(), params, work, *four.value(), &times, &diagnostic);
  remaining.store(-1);
  CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason, Reason::memory_budget);
  CHECK_EQ(work.used(), all_held); CHECK(diagnostic.tasks().data() == tasks);
  CHECK_EQ(times.sort_ns, saved_times.sort_ns); CHECK_EQ(times.level_scan_ns, saved_times.level_scan_ns);
  CHECK_EQ(times.assembly_ns, saved_times.assembly_ns); CHECK_EQ(times.allocation_ns, saved_times.allocation_ns);
  CHECK(timing_values(times) == timing_values(saved_times));
  auto result = build_catalogue(points.value(), params, work, *four.value()); REQUIRE(result.ok());
  CHECK(result.value().balls() > 0); CHECK_EQ(points.value().sites(), 5u);
}
MHGP11_TEST_MAIN()
