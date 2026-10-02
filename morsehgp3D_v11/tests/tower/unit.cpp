// MEB geometrique locale, replis et support strict canonique ; le wrapper conserve census global et proprietaire.
#include <array>
#include <optional>
#include <thread>
#include <type_traits>

#include "test_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace tower_test;
namespace {
struct Fixture {
  std::vector<Xyz> points;
  unsigned arity;
  std::array<i64, 3> center;
  i64 center_den, radius_num, radius_den;
};
std::vector<Fixture> fixtures() {
  std::vector<Fixture> out{
    {{{3, 4, 5}}, 1, {3, 4, 5}, 1, 0, 1},
    {{{0, 0, 0}, {6, 0, 0}}, 2, {3, 0, 0}, 1, 9, 1},
    {{{0, 0, 0}, {4, 0, 0}, {5, 0, 0}, {11, 0, 0}}, 2, {11, 0, 0}, 2, 121, 4},
    {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}}, 2, {2, 2, 0}, 1, 8, 1},
    {{{0, 0, 0}, {6, 0, 0}, {1, 1, 0}}, 2, {3, 0, 0}, 1, 9, 1},
    {{{0, 0, 0}, {4, 0, 0}, {2, 3, 0}}, 3, {12, 5, 0}, 6, 169, 36},
    {{{0, 0, 0}, {4, 4, 0}, {4, 0, 4}, {0, 4, 4}}, 4, {2, 2, 2}, 1, 12, 1},
    {{{10, 5, 5}, {9, 8, 5}, {5, 2, 1}, {1, 5, 8}}, 4, {5, 5, 5}, 1, 25, 1},
    {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {0, 2, 2}}, 2, {2, 2, 0}, 1, 8, 1},
    {{{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {4, 4, 0}, {2, 2, 0}}, 2, {2, 2, 0}, 1, 8, 1}
  };
  std::vector<Xyz> cube, line;
  for (u32 x : {0u, 4u}) for (u32 y : {0u, 4u}) for (u32 z : {0u, 4u}) cube.push_back({x, y, z});
  for (u32 x = 0; x < 12; ++x) line.push_back({x, 0, 0});
  out.push_back({cube, 2, {2, 2, 2}, 1, 12, 1});
  out.push_back({line, 2, {11, 0, 0}, 2, 121, 4});
  const u32 m = kCoordMax;
  out.push_back({{{0, 0, 0}, {m, m, 0}, {m, 0, m}, {0, m, m}}, 4,
                  {m, m, m}, 2, 3 * i64{m} * m, 4});
  return out;
}
}  // namespace

MHGP11_TEST(geometry, 175) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  for (const auto& f : fixtures()) {
    auto cloud = Input(f.points).prepare(budget);
    REQUIRE(cloud.ok());
    auto part = all(cloud.value());
    const auto before = budget.used();
    const auto answer = bounded_meb(cloud.value(), part);
    REQUIRE(answer.ok());
    const auto& meb = answer.value(); const auto& l = meb.ledger();
    CHECK_EQ(meb.support().size(), f.arity);
    CHECK_EQ(meb.sphere().presentation_arity(), f.arity);
    CHECK(center_is(meb.sphere(), f.center, f.center_den));
    CHECK(level_is(meb.sphere(), f.radius_num, f.radius_den));
    CHECK(support_strict(cloud.value(), meb));
    CHECK(l.presentations > 0 && l.presentations <= presentations(part.size()));
    CHECK(l.containing == 1 && l.containing <= l.positive && l.positive <= l.nondegenerate &&
          l.nondegenerate <= l.presentations);
    CHECK_EQ(l.comparisons, 0u);
    CHECK(l.point_tests <= l.positive * part.size() && l.point_tests >= part.size() * l.containing);
    std::reverse(part.begin(), part.end());
    const auto reversed = bounded_meb(cloud.value(), part);
    REQUIRE(reversed.ok());
    CHECK(equal(meb.support(), reversed.value().support()));
    CHECK(meb.ledger() == reversed.value().ledger());
    CHECK_EQ(budget.used(), before);
  }
  CHECK(budget.released().ok());
}

MHGP11_TEST(local_support, 20) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Xyz a{1, 2, 0}, b{0, 5, 0}, c{8, 1, 0}, d{8, 9, 0}, e{9, 8, 0};
  auto cloud = Input({a, b, c, d, e}).prepare(budget);
  REQUIRE(cloud.ok());
  std::array<SiteIdx, 4> part{site(cloud.value(), d), site(cloud.value(), c), site(cloud.value(), b), site(cloud.value(), a)};
  std::array<SiteIdx, 3> expected{site(cloud.value(), a), site(cloud.value(), c), site(cloud.value(), d)};
  std::sort(expected.begin(), expected.end());
  const auto local = bounded_meb(cloud.value(), part);
  REQUIRE(local.ok());
  CHECK(equal(local.value().support(), expected));
  CHECK(center_is(local.value().sphere(), {5, 5, 0}, 1));
  CHECK(level_is(local.value().sphere(), 25, 1));
  CHECK(support_strict(cloud.value(), local.value()));
  const auto global = bounded_meb(cloud.value(), all(cloud.value()));
  REQUIRE(global.ok());
  CHECK_EQ(global.value().support().size(), 2u);
  CHECK(num::compare(local.value().sphere().level(), global.value().sphere().level()) == 0);
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  auto combined = meb_census(index.value(), part, 1, budget);
  REQUIRE(combined.ok());
  CHECK(equal(combined.value().meb().support(), expected));
  CHECK_EQ(combined.value().population().kind(), CensusKind::complete);
  CHECK(combined.value().population().interior().empty());
  CHECK_EQ(combined.value().population().shell().size(), 5u);
  CHECK_EQ(local.value().ledger().presentations, 13u);  // 4 points + 6 paires + troisieme triplet strict.
  // Le premier minimiseur geometrique a poids negatif ne remplace pas le certificat strict local.
  const auto invalid = num::Sphere::through(num::Point::make(1, 2, 0).value(), num::Point::make(0, 5, 0).value(),
                                           num::Point::make(8, 1, 0).value());
  REQUIRE(invalid.ok() && invalid.value());
  CHECK(num::compare(invalid.value()->level(), local.value().sphere().level()) == 0);
  CHECK(!num::strictly_acute(num::Point::make(1, 2, 0).value(), num::Point::make(0, 5, 0).value(),
                            num::Point::make(8, 1, 0).value()));
  CHECK_EQ(expected.size(), 3u);
}

MHGP11_TEST(refusals, 24) {
  MemoryBudget budget(MemoryBudget::kUnlimited), denied(0);
  auto cloud = square().prepare(budget);
  REQUIRE(cloud.ok());
  auto part = all(cloud.value());
  const auto base = budget.used();
  std::array<SiteIdx, 13> oversized{};
  const std::array<SiteIdx, 2> duplicate{SiteIdx{0}, SiteIdx{0}};
  const std::array<SiteIdx, 1> out{SiteIdx{kNone}};
  for (const auto ids : {std::span<const SiteIdx>{}, std::span<const SiteIdx>{oversized},
                         std::span<const SiteIdx>{duplicate}, std::span<const SiteIdx>{out}}) {
    const auto bad = bounded_meb(cloud.value(), ids);
    CHECK(!bad.ok());
    CHECK_EQ(bad.outcome().reason, ids.empty() ? Reason::empty_input : Reason::parameter_out_of_range);
    CHECK_EQ(budget.used(), base);
  }
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  auto empty = bounded_meb(cloud.value(), part);
  CHECK(!empty.ok() && empty.outcome().reason == Reason::empty_input);
  auto size_first = bounded_meb(cloud.value(), oversized);
  CHECK(!size_first.ok() && size_first.outcome().reason == Reason::parameter_out_of_range);
  for (const auto ids : {std::span<const SiteIdx>{}, std::span<const SiteIdx>{oversized}}) {
    const auto bad = meb_census(index.value(), ids, 0, denied);
    CHECK(!bad.ok() && bad.outcome().reason == Reason::parameter_out_of_range);
    CHECK_EQ(denied.used(), 0u);
  }
  const auto missing = meb_census(index.value(), {}, 1, denied);
  CHECK(!missing.ok() && missing.outcome().reason == Reason::empty_input);
  const auto refused = meb_census(index.value(), part, 2, denied);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
  CHECK_EQ(denied.used(), 0u);
  GlobalIndex moved(std::move(index.value()));
  const auto moved_query = meb_census(index.value(), part, 1, denied);
  CHECK(!moved_query.ok() && moved_query.outcome().reason == Reason::empty_input);
  CHECK_EQ(moved.cloud().sites(), 5u);
}

MHGP11_TEST(ownership, 22) {
  CHECK(!std::is_default_constructible_v<BoundedMeb>);
  CHECK(std::is_nothrow_copy_constructible_v<BoundedMeb>);
  CHECK(std::is_nothrow_move_constructible_v<BoundedMeb>);
  CHECK(!std::is_copy_constructible_v<MebCensus> && !std::is_move_assignable_v<MebCensus>);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  std::optional<BoundedMeb> retained;
  std::optional<MebCensus> population;
  {
    auto input = square();
    auto cloud = input.prepare(budget);
    REQUIRE(cloud.ok());
    auto part = all(cloud.value());
    const auto value = bounded_meb(cloud.value(), part);
    REQUIRE(value.ok());
    retained.emplace(value.value());
    CHECK(retained->support().data() != value.value().support().data());
    part.assign(part.size(), SiteIdx{kNone});
    input.x.assign(input.x.size(), 999);
    CHECK(level_is(retained->sphere(), 8, 1));
    CHECK(support_strict(cloud.value(), *retained));
    auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
    REQUIRE(index.ok());
    const auto selected = all(index.value().cloud());
    auto result = meb_census(index.value(), selected, 2, budget);
    REQUIRE(result.ok());
    const auto view = result.value().population().shell();
    population.emplace(std::move(result.value()));
    CHECK(population->population().shell().data() == view.data());
    CHECK(result.value().population().interior().empty() && result.value().population().shell().empty());
    CHECK(square_census(index.value().cloud(), population->population(), 2));
  }
  CHECK(level_is(retained->sphere(), 8, 1));
  CHECK(center_is(retained->sphere(), {2, 2, 0}, 1));
  CHECK_EQ(population->population().interior().size(), 1u);
  CHECK_EQ(population->population().shell().size(), 4u);
  CHECK_EQ(budget.used(), 20u);
  population.reset();
  CHECK(budget.released().ok());
  auto weighted = Input({{0, 0, 0}, {4, 0, 0}, {0, 0, 0}}).prepare(budget);
  REQUIRE(weighted.ok());
  CHECK_EQ(weighted.value().weight(), 3u);
  const auto meb = bounded_meb(weighted.value(), all(weighted.value()));
  REQUIRE(meb.ok());
  CHECK(level_is(meb.value().sphere(), 4, 1));
}

MHGP11_TEST(wrapper, 30) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = square().prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  const auto part = all(index.value().cloud());
  for (u32 threshold : {1u, 2u, kNone}) {
    const u64 bytes = threshold == 1 ? 4 : 20;
    MemoryBudget short_budget(bytes + 6), exact_budget(2 * bytes + 7);
    Buffer<u8> first, second;
    REQUIRE(first.allocate(7, short_budget).ok() && second.allocate(7, exact_budget).ok());
    const auto bad = meb_census(index.value(), part, threshold, short_budget);
    CHECK(!bad.ok() && bad.outcome().reason == Reason::memory_budget);
    CHECK_EQ(short_budget.used(), 7u);
    {
      auto a = meb_census(index.value(), part, threshold, exact_budget);
      REQUIRE(a.ok());
      CHECK(square_census(index.value().cloud(), a.value().population(), threshold));
      CHECK_EQ(exact_budget.used(), bytes + 7);
      auto b = meb_census(index.value(), part, threshold, exact_budget);
      REQUIRE(b.ok());
      CHECK_EQ(exact_budget.used(), 2 * bytes + 7);
      CHECK(equal(a.value().meb().support(), b.value().meb().support()));
      CHECK(a.value().population().ledger() == b.value().population().ledger());
    }
    CHECK_EQ(exact_budget.used(), 7u);
  }
}

MHGP11_TEST(capacity, 18) {
  MemoryBudget owner(MemoryBudget::kUnlimited), budget(24);
  auto cloud = square().prepare(owner);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, owner);
  REQUIRE(index.ok());
  const auto part = all(index.value().cloud());
  {
    const auto retained = meb_census(index.value(), part, 2, budget);
    REQUIRE(retained.ok());
    CHECK_EQ(budget.used(), 20u);
    const auto view = retained.value().population().shell();
    const auto ledger = retained.value().meb().ledger();
    const auto refused = meb_census(index.value(), part, 2, budget);
    CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
    CHECK_EQ(budget.used(), 20u);
    CHECK(retained.value().population().shell().data() == view.data());
    CHECK(square_census(index.value().cloud(), retained.value().population(), 2));
    CHECK(retained.value().meb().ledger() == ledger);
    {
      const auto saturated = meb_census(index.value(), part, 1, budget);
      REQUIRE(saturated.ok());
      CHECK_EQ(budget.used(), 24u);
      CHECK_EQ(budget.peak(), 24u);
      CHECK(square_census(index.value().cloud(), saturated.value().population(), 1));
      CHECK(equal(retained.value().meb().support(), saturated.value().meb().support()));
    }
    CHECK_EQ(budget.used(), 20u);
    CHECK(square_census(index.value().cloud(), retained.value().population(), 2));
  }
  CHECK_EQ(budget.used(), 0u);
  CHECK(budget.released().ok());
}

MHGP11_TEST(shell, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  // Quatorze sites sur une sphere, au-dela du plafond LOCAL de douze sites de la partie MEB.
  auto cloud = Input({{0, 5, 5}, {10, 5, 5}, {5, 0, 5}, {5, 10, 5}, {1, 2, 5}, {1, 8, 5},
                      {9, 2, 5}, {9, 8, 5}, {2, 1, 5}, {2, 9, 5}, {8, 1, 5}, {8, 9, 5},
                      {5, 5, 0}, {5, 5, 10}, {5, 5, 5}}).prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  REQUIRE(index.ok());
  const auto& owner = index.value().cloud();
  const std::array<SiteIdx, 2> part{site(owner, {0, 5, 5}), site(owner, {10, 5, 5})};
  const auto result = meb_census(index.value(), part, 2, budget);
  REQUIRE(result.ok());
  CHECK_EQ(result.value().meb().support().size(), 2u);
  CHECK(level_is(result.value().meb().sphere(), 25, 1));
  CHECK_EQ(result.value().population().kind(), CensusKind::complete);
  CHECK_EQ(result.value().population().shell().size(), 14u);
  REQUIRE(result.value().population().interior().size() == 1);
  CHECK_EQ(result.value().population().interior()[0], site(owner, {5, 5, 5}));
  CHECK_EQ(result.value().population().ledger().passes, 2u);
}

MHGP11_TEST(concurrency, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = square().prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  const auto part = all(index.value().cloud()); const auto before = budget.used();
  std::array<bool, 4> okay{};
  std::array<MebLedger, 4> ledgers{};
  std::array<std::thread, 4> threads;
  for (std::size_t i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    MemoryBudget query_budget(4096);
    bool good = true;
    for (int repetition = 0; repetition < 16; ++repetition) {
      const auto result = meb_census(index.value(), part, 2, query_budget);
      if (!result.ok() || !square_census(index.value().cloud(), result.value().population(), 2)) { good = false; break; }
      if (repetition != 0 && !(ledgers[i] == result.value().meb().ledger())) good = false;
      ledgers[i] = result.value().meb().ledger();
    }
    okay[i] = good && query_budget.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (std::size_t i = 0; i < threads.size(); ++i) { CHECK(okay[i]); CHECK(ledgers[i] == ledgers[0]); }
  CHECK_EQ(budget.used(), before);
}

MHGP11_TEST_MAIN()
