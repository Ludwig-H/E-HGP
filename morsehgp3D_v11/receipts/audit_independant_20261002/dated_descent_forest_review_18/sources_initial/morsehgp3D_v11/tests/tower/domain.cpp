// Domaine index/catalogue ferme : lookup exact, conservation des proprietaires et budgets reels.
#include <optional>
#include <thread>
#include <type_traits>

#include "test_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace tower_test;
namespace {
using Key = std::array<SiteIdx, 4>;
Key pair(u32 a, u32 b) { return {SiteIdx{a}, SiteIdx{b}, SiteIdx{kNone}, SiteIdx{kNone}}; }
Input line() {
  std::vector<Xyz> points;
  for (u32 i = 0; i < 8; ++i) points.push_back({i, 0, 0});
  return Input(points);
}
Result<GlobalIndex> index_of(const Input& input, MemoryBudget& budget) {
  auto cloud = input.prepare(budget);
  if (!cloud.ok()) return cloud.outcome();
  return build_index(std::move(cloud.value()), IndexParams{2}, budget);
}
bool level_equals(const num::Level& level, u32 numerator, u32 denominator) {
  auto expected = num::Level::make(num::to_wide(i64{numerator}), num::to_wide(i64{denominator}));
  return expected.ok() && num::compare(level, expected.value()) == 0;
}
u64 catalogue_bytes(const Catalogue& cat) {
  return cat.balls_data().size_bytes() + cat.levels().size_bytes() +
         cat.population_offsets().size_bytes() + cat.population().size_bytes();
}
}  // namespace

static_assert(!std::is_default_constructible_v<FullDomain> && !std::is_copy_constructible_v<FullDomain>);
static_assert(!std::is_move_assignable_v<FullDomain> && std::is_nothrow_move_constructible_v<FullDomain>);

MHGP11_TEST(context, 18) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  {
    auto index = index_of(Input({{7, 8, 9}}), owner);
    REQUIRE(index.ok());
    const auto* xyz = index.value().cloud().x().data();
    const auto* ids = index.value().cloud().ids().data();
    auto result = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    REQUIRE(result.ok());
    const auto& domain = result.value();
    CHECK_EQ(index.value().cloud().sites(), 0u);
    CHECK_EQ(index.value().nodes(), 0u);
    CHECK_EQ(domain.index().cloud().sites(), 1u);
    CHECK(domain.index().cloud().x().data() == xyz && domain.index().cloud().ids().data() == ids);
    CHECK_EQ(domain.catalogue().kmax(), 5u);
    CHECK_EQ(domain.catalogue().balls(), 0u);
    CHECK_EQ(domain.lookup_capacity(), 0u);
    CHECK_EQ(domain.catalogue().levels().size(), 1u);
    CHECK(level_equals(domain.catalogue().levels()[0], 0, 1));
    CHECK(!domain.find_support(pair(0, 0)));
    CHECK(!domain.find_support(pair(kNone, kNone)));
    CHECK_EQ(work.used(), catalogue_bytes(domain.catalogue()));
    auto invalid = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    CHECK(!invalid.ok() && invalid.outcome().reason == Reason::empty_input);
    CHECK_EQ(work.used(), catalogue_bytes(domain.catalogue()));
  }
  CHECK(owner.released().ok());
  CHECK(work.released().ok());
}

MHGP11_TEST(lookup, 200) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto index = index_of(line(), budget);
  REQUIRE(index.ok());
  auto result = prepare_full_domain(std::move(index.value()), CatalogueParams{}, budget);
  REQUIRE(result.ok());
  const auto& domain = result.value();
  const auto& cat = domain.catalogue();
  // Ligne unitaire : seules les paires de distance <=5 sont admises, p=distance-1, beta=distance^2/4.
  CHECK_EQ(cat.balls(), 25u);
  CHECK_EQ(domain.lookup_capacity(), 64u);
  u32 expected = 0;
  for (u32 gap = 1; gap <= 5; ++gap) for (u32 a = 0; a + gap < 8; ++a) {
    const auto found = domain.find_support(pair(a, a + gap));
    REQUIRE(found.has_value());
    CHECK_EQ(idx(*found), expected++);
    const auto& ball = cat.balls_data()[idx(*found)];
    CHECK_EQ(ball.p, gap - 1);
    CHECK_EQ(ball.m, 2u);
    CHECK_EQ(ball.qmin, 2u);
    CHECK(level_equals(cat.levels()[idx(ball.rank)], gap * gap, 4));
    CHECK(equal(cat.shell(*found), std::array<SiteIdx, 2>{SiteIdx{a}, SiteIdx{a + gap}}));
    CHECK_EQ(cat.interior(*found).size(), gap - 1);
  }
  CHECK_EQ(expected, 25u);
  // Collisions de la fonction epinglee : (1,2)/(2,4) a la case1 ; (2,3)/(0,3) a la case41, modulo64.
  const auto a = domain.find_support(pair(1, 2)), b = domain.find_support(pair(2, 4));
  REQUIRE(a && b);
  CHECK(*a != *b);
  CHECK(domain.find_support(pair(2, 3)) != domain.find_support(pair(0, 3)));
  for (const Key key : {pair(0, 6), pair(0, 7), pair(1, 7), pair(8, 9), pair(kNone, kNone), pair(2, 1)})
    CHECK(!domain.find_support(key));
  Key bad = pair(1, 2);
  bad[3] = SiteIdx{253};  // Meme case initiale1 que (1,2,None,None), mais padding different.
  CHECK(!domain.find_support(bad));
  bad[2] = SiteIdx{3};
  CHECK(!domain.find_support(bad));
}

MHGP11_TEST(global_support, 17) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Xyz a{1, 2, 0}, b{0, 5, 0}, c{8, 1, 0}, d{8, 9, 0}, e{9, 8, 0};
  auto index = index_of(Input({a, b, c, d, e}), budget);
  REQUIRE(index.ok());
  auto result = prepare_full_domain(std::move(index.value()), CatalogueParams{}, budget);
  REQUIRE(result.ok());
  const auto& domain = result.value();
  const auto& cloud = domain.index().cloud();
  std::array<SiteIdx, 4> part{site(cloud, a), site(cloud, b), site(cloud, c), site(cloud, d)};
  auto local = bounded_meb(cloud, part);
  REQUIRE(local.ok());
  REQUIRE(local.value().support().size() == 3);
  Key local_key{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}};
  std::copy(local.value().support().begin(), local.value().support().end(), local_key.begin());
  CHECK(!domain.find_support(local_key));
  auto pop = census(domain.index(), local.value().sphere(), 1, budget);
  REQUIRE(pop.ok());
  CHECK_EQ(pop.value().kind(), CensusKind::complete);
  CHECK(pop.value().interior().empty());
  CHECK_EQ(pop.value().shell().size(), 5u);
  // Canonicalisation globale connue analytiquement : a/e est la seule paire antipodale.
  auto left = idx(site(cloud, a)), right = idx(site(cloud, e));
  if (left > right) std::swap(left, right);
  const auto global = domain.find_support(pair(left, right));
  REQUIRE(global.has_value());
  const auto& ball = domain.catalogue().balls_data()[idx(*global)];
  CHECK_EQ(ball.qmin, 2u);
  CHECK_EQ(ball.p, 0u);
  CHECK_EQ(ball.m, 5u);
  CHECK(level_equals(domain.catalogue().levels()[idx(ball.rank)], 25, 1));
  CHECK(equal(domain.catalogue().shell(*global), pop.value().shell()));
  CHECK(center_is(local.value().sphere(), {5, 5, 0}, 1));
  CHECK(level_is(local.value().sphere(), 25, 1));
}

MHGP11_TEST(ownership, 16) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  {
    auto input = line();
    auto index = index_of(input, owner);
    REQUIRE(index.ok());
    const auto* xyz = index.value().cloud().x().data();
    auto result = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    REQUIRE(result.ok());
    const auto* balls = result.value().catalogue().balls_data().data();
    const auto* population = result.value().catalogue().population().data();
    const auto owner_used = owner.used(), used = work.used();
    FullDomain moved(std::move(result.value()));
    CHECK_EQ(result.value().index().cloud().sites(), 0u);
    CHECK_EQ(result.value().catalogue().balls(), 0u);
    CHECK_EQ(result.value().lookup_capacity(), 0u);
    CHECK(!result.value().find_support(pair(1, 2)));
    CHECK(moved.index().cloud().x().data() == xyz);
    CHECK(moved.catalogue().balls_data().data() == balls);
    CHECK(moved.catalogue().population().data() == population);
    CHECK_EQ(owner.used(), owner_used);
    CHECK_EQ(work.used(), used);
    input.x.assign(8, kCoordMax);
    CHECK_EQ(moved.index().cloud().x()[0], 0u);
    CHECK_EQ(moved.index().cloud().x()[7], 7u);
    CHECK(moved.find_support(pair(1, 2)).has_value());
    CHECK_EQ(moved.catalogue().balls(), 25u);
  }
  CHECK(owner.released().ok());
  CHECK(work.released().ok());
}

MHGP11_TEST(refusals, 18) {
  MemoryBudget owner(MemoryBudget::kUnlimited), denied(0);
  auto index = index_of(line(), owner);
  REQUIRE(index.ok());
  const auto* xyz = index.value().cloud().x().data();
  const auto initial = owner.used();
  CatalogueParams invalid;
  invalid.kmax = 0;
  auto bad = prepare_full_domain(std::move(index.value()), invalid, denied);
  CHECK(!bad.ok() && bad.outcome().reason == Reason::kmax_out_of_range);
  CHECK_EQ(denied.peak(), 0u);
  auto failed = prepare_full_domain(std::move(index.value()), CatalogueParams{}, denied);
  CHECK(!failed.ok() && failed.outcome().reason == Reason::memory_budget);
  CHECK_EQ(index.value().cloud().sites(), 8u);
  CHECK(index.value().cloud().x().data() == xyz);
  CHECK_EQ(owner.used(), initial);
  CHECK(denied.released().ok());
  auto weighted = index_of(Input({{0, 0, 0}, {0, 0, 0}, {2, 0, 0}}), owner);
  REQUIRE(weighted.ok());
  auto weights = prepare_full_domain(std::move(weighted.value()), CatalogueParams{}, denied);
  CHECK(!weights.ok() && weights.outcome().reason == Reason::multiplicity_unsupported);
  CHECK_EQ(weighted.value().cloud().sites(), 2u);
  CHECK_EQ(weighted.value().cloud().weight(), 3u);
  CHECK(denied.released().ok());
  MemoryBudget work(MemoryBudget::kUnlimited);
  auto recovered = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
  REQUIRE(recovered.ok());
  auto empty = prepare_full_domain(std::move(index.value()), CatalogueParams{}, denied);
  CHECK(!empty.ok() && empty.outcome().reason == Reason::empty_input);
  auto priority = prepare_full_domain(std::move(index.value()), invalid, denied);
  CHECK(!priority.ok() && priority.outcome().reason == Reason::kmax_out_of_range);
  CHECK(recovered.value().find_support(pair(1, 2)).has_value());
  CHECK_EQ(index.value().cloud().sites(), 0u);
}

MHGP11_TEST(capacity, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), measured(MemoryBudget::kUnlimited);
  auto index = index_of(line(), owner);
  REQUIRE(index.ok());
  u64 retained = 0;
  {
    auto cat = build_catalogue(index.value().cloud(), CatalogueParams{}, measured);
    REQUIRE(cat.ok());
    retained = catalogue_bytes(cat.value());
    CHECK_EQ(measured.used(), retained);
  }
  CHECK(measured.released().ok());
  const u64 peak = std::max(measured.peak(), retained + 4 * u64{64});
  MemoryBudget work(20 + peak);
  Buffer<u32> existing;
  REQUIRE(existing.allocate(5, work).ok());
  existing[0] = 123;
  std::optional<FullDomain> kept;
  {
    auto made = prepare_full_domain(std::move(index.value()), CatalogueParams{}, work);
    REQUIRE(made.ok());
    CHECK_EQ(work.peak(), 20 + peak);
    CHECK_EQ(work.used(), 20 + retained + 4 * u64{64});
    kept.emplace(std::move(made.value()));
  }
  auto second = index_of(line(), owner);
  REQUIRE(second.ok());
  const auto* xyz = second.value().cloud().x().data();
  const auto before = work.used();
  auto refused = prepare_full_domain(std::move(second.value()), CatalogueParams{}, work);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
  CHECK_EQ(work.used(), before);
  CHECK(second.value().cloud().x().data() == xyz);
  CHECK_EQ(second.value().cloud().sites(), 8u);
  CHECK(kept->find_support(pair(1, 2)).has_value());
  CHECK_EQ(existing[0], 123u);
  kept.reset();
  CHECK_EQ(work.used(), 20u);
  {
    auto recovered = prepare_full_domain(std::move(second.value()), CatalogueParams{}, work);
    REQUIRE(recovered.ok());
    CHECK_EQ(recovered.value().lookup_capacity(), 64u);
  }
  CHECK_EQ(work.used(), 20u);
  existing.reset();
  CHECK(work.released().ok());
  CHECK(owner.released().ok());
}

MHGP11_TEST(concurrency, 9) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto index = index_of(line(), budget);
  REQUIRE(index.ok());
  auto made = prepare_full_domain(std::move(index.value()), CatalogueParams{}, budget);
  REQUIRE(made.ok());
  const FullDomain& domain = made.value();
  const auto before = budget.used();
  std::array<bool, 4> correct{};
  std::array<std::thread, 4> threads;
  for (unsigned worker = 0; worker < threads.size(); ++worker) threads[worker] = std::thread([&, worker] {
    bool ok = true;
    for (unsigned repeat = 0; repeat < 64; ++repeat) {
      for (u32 b = 0; b < domain.catalogue().balls(); ++b)
        ok = ok && domain.find_support(domain.catalogue().balls_data()[b].support) == BallIdx{b};
      ok = ok && !domain.find_support(pair(0, 7));
    }
    correct[worker] = ok;
  });
  for (auto& thread : threads) thread.join();
  for (bool ok : correct) CHECK(ok);
  CHECK_EQ(budget.used(), before);
  CHECK_EQ(domain.lookup_capacity(), 64u);
  CHECK_EQ(domain.catalogue().balls(), 25u);
}

MHGP11_TEST(permutation, 100) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto input = line();
  auto first_index = index_of(input, budget);
  REQUIRE(first_index.ok());
  std::reverse(input.x.begin(), input.x.end());
  std::reverse(input.y.begin(), input.y.end());
  std::reverse(input.z.begin(), input.z.end());
  std::reverse(input.ids.begin(), input.ids.end());
  auto second_index = index_of(input, budget);
  REQUIRE(second_index.ok());
  auto first = prepare_full_domain(std::move(first_index.value()), CatalogueParams{}, budget);
  auto second = prepare_full_domain(std::move(second_index.value()), CatalogueParams{}, budget);
  REQUIRE(first.ok() && second.ok());
  CHECK_EQ(first.value().lookup_capacity(), second.value().lookup_capacity());
  const auto& a = first.value().catalogue();
  const auto& b = second.value().catalogue();
  REQUIRE(a.balls() == 25 && b.balls() == 25);
  for (u32 i = 0; i < a.balls(); ++i) {
    CHECK(a.balls_data()[i].support == b.balls_data()[i].support);
    CHECK_EQ(a.balls_data()[i].rank, b.balls_data()[i].rank);
    CHECK_EQ(first.value().find_support(a.balls_data()[i].support), second.value().find_support(a.balls_data()[i].support));
    CHECK(equal(a.interior(BallIdx{i}), b.interior(BallIdx{i})));
    CHECK(equal(a.shell(BallIdx{i}), b.shell(BallIdx{i})));
  }
  CHECK(a.ledger() == b.ledger());
}

MHGP11_TEST_MAIN()
