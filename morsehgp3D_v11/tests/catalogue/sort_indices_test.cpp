// Permutations contre des rangs analytiques independants de num::compare ; aucun oracle de tri recopie.
#include <bit>
#include <numeric>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/sort_indices.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace cat = mhgp11::catalogue_detail;

namespace {
struct Fixture {
  std::vector<cat::Emission> records;
  std::vector<u32> ranks;
};

Result<Fixture> make_fixture(u32 count, u32 mode, bool wide) {
  Fixture result;
  result.records.resize(count);
  result.ranks.resize(count);
  for (u32 i = 0; i < count; ++i) {
    const u32 rank = mode == 0 ? i : mode == 1 ? count - 1 - i : mode == 2 ? (i * 2053) % 23 : 0;
    result.ranks[i] = rank;
    auto numerator = num::Wide<4>::from_u64(u64(rank) + 1);
    auto denominator = num::Wide<3>::from_u64(1);
    if (wide) {
      constexpr int n = num::Budget::level_numerator - 2;
      constexpr int d = num::Budget::level_denominator - 2;
      numerator.words[n / 64] |= u64{1} << (n % 64);
      denominator = {};
      denominator.words[d / 64] = u64{1} << (d % 64);
    }
    if ((i & 1u) != 0) {  // Deux presentations non reduites du MEME niveau dans les groupes egaux.
      num::Wide<4> twice_n;
      num::Wide<3> twice_d;
      if (!num::add(numerator, numerator, twice_n) || !num::add(denominator, denominator, twice_d))
        return fail(Reason::arithmetic_invariant);
      numerator = twice_n; denominator = twice_d;
    }
    auto level = num::Level::make(numerator, denominator);
    if (!level.ok()) return level.outcome();
    auto& record = result.records[i];
    record.level = level.value();
    const u32 support = mode == 2 ? (i * 13) % 17 : 0;
    record.ball.support = {SiteIdx{2 * support}, SiteIdx{2 * support + 1}, SiteIdx{kNone}, SiteIdx{kNone}};
    record.ball.qmin = 2;
    record.ball.p = i % 5; record.ball.m = 2;
    record.population_begin = 7 * u64(i);
  }
  return result;
}

std::vector<u32> expected_order(const Fixture& fixture) {
  std::vector<u32> indices(fixture.records.size());
  std::iota(indices.begin(), indices.end(), 0u);
  std::sort(indices.begin(), indices.end(), [&](u32 a, u32 b) {
    if (fixture.ranks[a] != fixture.ranks[b]) return fixture.ranks[a] < fixture.ranks[b];
    const auto& left = fixture.records[a].ball.support;
    const auto& right = fixture.records[b].ball.support;
    return left != right ? left < right : a < b;
  });
  return indices;
}

bool unchanged(std::span<const cat::Emission> a, std::span<const cat::Emission> b) {
  if (a.size() != b.size()) return false;
  for (u64 i = 0; i < a.size(); ++i) {
    if (a[i].ball.support != b[i].ball.support || a[i].ball.rank != b[i].ball.rank ||
        a[i].ball.p != b[i].ball.p || a[i].ball.m != b[i].ball.m || a[i].ball.qmin != b[i].ball.qmin ||
        a[i].population_begin != b[i].population_begin) return false;
    const auto an = num::to_wide(a[i].level.numerator()), bn = num::to_wide(b[i].level.numerator());
    const auto ad = num::to_wide(a[i].level.denominator()), bd = num::to_wide(b[i].level.denominator());
    if (an.neg != bn.neg || an.words != bn.words || ad.neg != bd.neg || ad.words != bd.words) return false;
  }
  return true;
}

void judge(u32 count, u32 mode, bool wide, sched::Pool& one, sched::Pool& four) {
  auto made = make_fixture(count, mode, wide);
  REQUIRE(made.ok());
  const auto& fixture = made.value();
  const auto original = fixture.records;
  const auto expected = expected_order(fixture);
  u64 baseline = 0;
  for (sched::Pool* pool : {static_cast<sched::Pool*>(nullptr), &one, &four}) {
    MemoryBudget budget(7 + 8 * u64(count));
    Buffer<u8> sentinel;
    REQUIRE(sentinel.allocate(7, budget).ok());
    sentinel[0] = 123;
    u64 comparisons = 987;
    auto result = cat::sort_indices(fixture.records, budget, pool, &comparisons);
    REQUIRE(result.ok());
    CHECK_EQ(result.value().size(), count);
    CHECK(std::equal(result.value().span().begin(), result.value().span().end(), expected.begin(), expected.end()));
    CHECK(unchanged(fixture.records, original));
    CHECK(count < 2 ? comparisons == 0 :
          comparisons > 0 && comparisons <= 4 * u64(count) * std::bit_width(u64(count) - 1));
    CHECK_EQ(budget.used(), 7 + 4 * u64(count));
    CHECK_EQ(budget.peak(), 7 + 8 * u64(count));
    CHECK_EQ(sentinel[0], 123u);
    if (pool == nullptr) baseline = comparisons;
    else CHECK_EQ(comparisons, baseline);
    result.value().reset();
    CHECK_EQ(budget.used(), 7u);
    sentinel.reset();
    CHECK(budget.released().ok());
  }
}

struct Nested {
  sched::Pool& pool;
  MemoryBudget& budget;
  std::span<const cat::Emission> records;
  Reason reason = Reason::none;
  u64 comparisons = 123, after = 123;
  static Outcome body(void* context, u64, u64, u32) {
    auto& self = *static_cast<Nested*>(context);
    auto result = cat::sort_indices(self.records, self.budget, &self.pool, &self.comparisons);
    self.reason = result.outcome().reason;
    self.after = self.budget.used();
    return {};
  }
};
}  // namespace

MHGP11_TEST(boundaries, 1400) {
  auto one = sched::make_pool({1}), four = sched::make_pool({4});
  REQUIRE(one.ok() && four.ok());
  for (u32 count : {0u, 1u, 2047u, 2048u, 2049u, 4095u, 4096u, 4097u, 8193u, 16385u})
    for (u32 mode : {0u, 1u, 2u, 3u}) judge(count, mode, false, *one.value(), *four.value());
}

MHGP11_TEST(wide_levels, 140) {
  auto one = sched::make_pool({1}), four = sched::make_pool({4});
  REQUIRE(one.ok() && four.ok());
  for (u32 mode : {0u, 1u, 2u, 3u}) judge(8193, mode, true, *one.value(), *four.value());
}

MHGP11_TEST(refusals, 20) {
  auto made = make_fixture(4097, 2, true);
  REQUIRE(made.ok());
  const auto& records = made.value().records;
  const auto original = records;
  for (u32 workers : {1u, 4u}) {
    auto pool = sched::make_pool({workers});
    REQUIRE(pool.ok());
    MemoryBudget too_small(8 * records.size() - 1);
    u64 comparisons = 123;
    auto refused = cat::sort_indices(records, too_small, pool.value().get(), &comparisons);
    CHECK_EQ(refused.outcome().reason, Reason::memory_budget);
    CHECK_EQ(comparisons, 123u);
    CHECK_EQ(too_small.peak(), 0u);
    CHECK(too_small.released().ok());
    CHECK(unchanged(records, original));
    MemoryBudget work(MemoryBudget::kUnlimited);
    Nested nested{*pool.value(), work, records};
    REQUIRE(pool.value()->parallel_for(1, 1, &nested, Nested::body).ok());
    CHECK_EQ(nested.reason, Reason::pool_busy);
    CHECK_EQ(nested.comparisons, 123u);
    CHECK_EQ(nested.after, 0u);
    CHECK(work.released().ok());
    auto recovered = cat::sort_indices(records, work, pool.value().get());
    REQUIRE(recovered.ok());
    CHECK(std::equal(recovered.value().span().begin(), recovered.value().span().end(),
                     expected_order(made.value()).begin()));
    CHECK(unchanged(records, original));
  }
}

MHGP11_TEST_MAIN()
