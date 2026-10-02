// Raccord public Pool -> catalogue -> domaine ; refus de table apres diagnostic catalogue reussi.
#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <new>
#include <sstream>

#include "sched/sched.hpp"
#include "test_support.hpp"
#include "test.hpp"

namespace {
std::atomic<long long> remaining{-1};
std::atomic<unsigned long long> allocations{0}, injections{0};
bool deny() noexcept {
  allocations.fetch_add(1);
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
using namespace tower_test;
namespace {
Result<GlobalIndex> index_of(const Input& input, MemoryBudget& budget) {
  auto cloud = input.prepare(budget);
  if (!cloud.ok()) return cloud.outcome();
  return build_index(std::move(cloud.value()), IndexParams{2}, budget);
}
Input line() {
  std::vector<Xyz> points;
  for (u32 i = 0; i < 17; ++i) points.push_back({i, 0, 0});
  return Input(points);
}
std::array<u64, 14> timing_values(const CatalogueTimings& t) {
  return {t.prefix_ns, t.count_ns, t.replay_ns, t.fill_ns, t.sort_ns, t.level_scan_ns, t.allocation_ns,
          t.assembly_ns, t.count_task_sum_ns, t.count_task_max_ns, t.fill_task_sum_ns, t.fill_task_max_ns,
          t.sort_comparisons, t.tasks};
}
CatalogueTimings sentinel() { return {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 999, 256}; }
template <class Integer>
std::string hexadecimal(const Integer& integer) {
  const auto value = num::to_wide(integer);
  std::ostringstream out;
  if (value.sign() < 0) out << '-';
  out << std::hex << std::setfill('0');
  for (std::size_t i = value.words.size(); i != 0; --i) out << std::setw(16) << value.words[i - 1];
  return out.str();
}
// Encodage de tous les champs, jamais des octets de padding des structures C++.
std::string canonical(const Catalogue& cat) {
  std::ostringstream out;
  out << unsigned(cat.kmax()) << ':';
  for (const auto& level : cat.levels())
    out << hexadecimal(level.numerator()) << '/' << hexadecimal(level.denominator()) << ';';
  out << ':';
  for (const auto& ball : cat.balls_data()) {
    for (SiteIdx id : ball.support) out << idx(id) << ',';
    out << idx(ball.rank) << ',' << ball.p << ',' << ball.m << ',' << unsigned(ball.qmin) << ';';
  }
  out << ':';
  for (u64 offset : cat.population_offsets()) out << offset << ',';
  out << ':';
  for (SiteIdx id : cat.population()) out << idx(id) << ',';
  return out.str();
}
bool lookup_all(const FullDomain& domain) {
  const auto balls = domain.catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b)
    if (domain.find_support(balls[b].support) != BallIdx{b}) return false;
  const std::array<SiteIdx, 4> absent{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}};
  return !domain.find_support(absent);
}
}

MHGP11_TEST(equivalence, 130) {
  const u32 m = kCoordMax;
  const std::array<Input, 4> inputs{Input({{7, 8, 9}}), square(),
    Input({{0, 0, 0}, {m, m, 0}, {m, 0, m}, {0, m, m}}), line()};
  for (u32 fixture = 0; fixture < inputs.size(); ++fixture) {
    MemoryBudget owner(MemoryBudget::kUnlimited), serial(MemoryBudget::kUnlimited);
    auto index = index_of(inputs[fixture], owner);
    REQUIRE(index.ok());
    CatalogueParams params;
    params.kmax = fixture == 3 ? 1 : 5;
    params.leaf_size = fixture == 3 ? 4 : 32;
    auto baseline = prepare_full_domain(std::move(index.value()), params, serial);
    REQUIRE(baseline.ok());
    const auto expected = canonical(baseline.value().catalogue());
    CHECK(lookup_all(baseline.value()));
    for (u32 workers : {1u, 4u}) {
      auto pool = sched::make_pool({workers});
      REQUIRE(pool.ok());
      MemoryBudget work(MemoryBudget::kUnlimited);
      {
        auto source = index_of(inputs[fixture], owner);
        REQUIRE(source.ok());
        const auto* xyz = source.value().cloud().x().data();
        auto timings = sentinel();
        auto result = prepare_full_domain(std::move(source.value()), params, work, *pool.value(), &timings);
        REQUIRE(result.ok());
        CHECK_EQ(source.value().cloud().sites(), 0u);
        CHECK(result.value().index().cloud().x().data() == xyz);
        CHECK_EQ(canonical(result.value().catalogue()), expected);
        CHECK(result.value().catalogue().ledger() == baseline.value().catalogue().ledger());
        CHECK_EQ(result.value().lookup_capacity(), baseline.value().lookup_capacity());
        CHECK(lookup_all(result.value()));
        CHECK(timings.tasks > 0 && timings.tasks <= 256 && timings.sort_comparisons != 999);
        auto uninstrumented = index_of(inputs[fixture], owner);
        REQUIRE(uninstrumented.ok());
        auto without = prepare_full_domain(std::move(uninstrumented.value()), params, work, *pool.value());
        REQUIRE(without.ok());
        CHECK_EQ(canonical(without.value().catalogue()), expected);
        CHECK_EQ(work.used(), 2 * serial.used());
      }
      CHECK(work.released().ok());
    }
  }
}

MHGP11_TEST(refusals, 19) {
  MemoryBudget owner(MemoryBudget::kUnlimited), denied(0), work(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool({4});
  REQUIRE(pool.ok());
  auto index = index_of(square(), owner);
  REQUIRE(index.ok());
  const auto* xyz = index.value().cloud().x().data();
  const auto used = owner.used();
  auto timings = sentinel();
  const auto initial = timing_values(timings);
  CatalogueParams params;
  params.kmax = 0;
  auto invalid = prepare_full_domain(std::move(index.value()), params, denied, *pool.value(), &timings);
  CHECK(!invalid.ok() && invalid.outcome().reason == Reason::kmax_out_of_range);
  CHECK_EQ(denied.peak(), 0u);
  CHECK(timing_values(timings) == initial);
  params.kmax = 5;
  auto memory = prepare_full_domain(std::move(index.value()), params, denied, *pool.value(), &timings);
  CHECK(!memory.ok() && memory.outcome().reason == Reason::memory_budget);
  CHECK(timing_values(timings) == initial);
  params.ball_limit = 1;
  auto limit = prepare_full_domain(std::move(index.value()), params, work, *pool.value(), &timings);
  CHECK(!limit.ok() && limit.outcome().reason == Reason::index_overflow_u32);
  CHECK(timing_values(timings) == initial);
  CHECK(index.value().cloud().x().data() == xyz && index.value().cloud().sites() == 5);
  CHECK_EQ(owner.used(), used);
  CHECK(work.released().ok() && denied.released().ok());
  params.ball_limit = kNone;
  auto recovered = prepare_full_domain(std::move(index.value()), params, work, *pool.value(), &timings);
  REQUIRE(recovered.ok());
  CHECK(lookup_all(recovered.value()));
  CHECK(timing_values(timings) != initial);
  const auto published = timing_values(timings);
  auto empty = prepare_full_domain(std::move(index.value()), params, denied, *pool.value(), &timings);
  CHECK(!empty.ok() && empty.outcome().reason == Reason::empty_input);
  CHECK(timing_values(timings) == published);
  CHECK(recovered.value().index().cloud().x().data() == xyz);
  CHECK(denied.released().ok());
}

MHGP11_TEST(lookup_refusal, 44) {
  for (u32 workers : {1u, 4u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    Buffer<u32> existing;
    REQUIRE(existing.allocate(5, work).ok());
    existing[0] = 321;
    auto source = index_of(square(), owner);
    REQUIRE(source.ok());
    unsigned long long cat_allocations = 0, domain_allocations = 0;
    auto timings = sentinel();
    {
      const auto before = allocations.load();
      auto catalogue = build_catalogue(source.value().cloud(), CatalogueParams{}, work, *pool.value(), &timings);
      cat_allocations = allocations.load() - before;
      REQUIRE(catalogue.ok());
    }
    {
      const auto before = allocations.load();
      auto domain = prepare_full_domain(std::move(source.value()), CatalogueParams{}, work, *pool.value(), &timings);
      domain_allocations = allocations.load() - before;
      REQUIRE(domain.ok());
      CHECK(domain.value().lookup_capacity() > 0);
    }
    CHECK_EQ(domain_allocations, cat_allocations + 1);
    CHECK_EQ(work.used(), 20u);
    REQUIRE(cat_allocations > 0 && cat_allocations < 4096);
    auto index = index_of(square(), owner);
    REQUIRE(index.ok());
    const auto* xyz = index.value().cloud().x().data();
    const auto owner_used = owner.used();
    timings = sentinel();
    const auto before_timings = timing_values(timings);
    const auto before_injections = injections.load();
    remaining.store(static_cast<long long>(cat_allocations));
    auto refused = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work, *pool.value(), &timings);
    remaining.store(-1);
    CHECK_EQ(injections.load(), before_injections + 1);
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
    CHECK(timing_values(timings) == before_timings);
    CHECK(index.value().cloud().x().data() == xyz && index.value().cloud().sites() == 5);
    CHECK_EQ(owner.used(), owner_used);
    CHECK_EQ(work.used(), 20u);
    CHECK_EQ(existing[0], 321u);
    {
      auto recovery = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work, *pool.value(), &timings);
      REQUIRE(recovery.ok());
      CHECK(lookup_all(recovery.value()));
      CHECK(timing_values(timings) != before_timings);
    }
    CHECK_EQ(work.used(), 20u);
    existing.reset();
    CHECK(work.released().ok());
    CHECK(owner.released().ok());
    std::printf("domain_parallel_lookup workers=%u catalogue_allocations=%llu domain_allocations=%llu injected=1\n",
                workers, cat_allocations, domain_allocations);
  }
}

MHGP11_TEST_MAIN()
