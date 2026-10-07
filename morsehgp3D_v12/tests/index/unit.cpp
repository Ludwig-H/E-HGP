// Portes exactes, transactionnelles et concurrentes de l'index global possede.
#include <array>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>

#include "test_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace index_test;

MHGP12_TEST(fixtures, 190) {
  MemoryBudget cloud_budget(MemoryBudget::kUnlimited), index_budget(MemoryBudget::kUnlimited), query_budget(4096);
  for (u32 leaf : {1u, 4u, 16u, 256u}) {
    auto cloud = octa().prepare(cloud_budget);
    REQUIRE(cloud.ok());
    auto index = build_index(std::move(cloud.value()), IndexParams{leaf}, index_budget);
    REQUIRE(index.ok());
    CHECK_EQ(cloud.value().sites(), 0u);
    CHECK_EQ(index.value().cloud().sites(), 11u);
    const Shape shape = radix_shape(octa().points, leaf);
    CHECK_EQ(index.value().nodes(), shape.nodes);
    CHECK_EQ(index.value().max_depth(), shape.depth);
    for (u32 threshold : {1u, 2u, 3u, 4u, 10u, kNone}) {
      {
        const auto result = census(index.value(), ball(), threshold, query_budget);
        REQUIRE(result.ok());
        CHECK(analytic(index.value().cloud(), result.value(), threshold));
        CHECK_EQ(result.value().ledger().passes, 2u);
        CHECK(result.value().ledger().point_tests <= 22);
        const auto repeated = census(index.value(), ball(), threshold, query_budget);
        REQUIRE(repeated.ok());
        CHECK(equal(result.value().interior(), repeated.value().interior()));
        CHECK(equal(result.value().shell(), repeated.value().shell()));
        CHECK(result.value().ledger() == repeated.value().ledger());
      }
      CHECK_EQ(query_budget.used(), 0u);
    }
  }
  CHECK(cloud_budget.released().ok() && index_budget.released().ok());
}

MHGP12_TEST(structure, 200) {
  for (u32 n : {1u, 2u, 7u, 8u, 9u, 15u, 16u, 17u, 24u, 25u, 255u, 256u, 257u}) {
    std::vector<std::array<u32, 3>> points;
    for (u32 i = 0; i < n; ++i) points.push_back({i, i % 3, i % 7});
    for (u32 leaf : {1u, 8u, 16u, 256u}) {
      MemoryBudget cloud_budget(MemoryBudget::kUnlimited);
      const Shape shape = radix_shape(points, leaf);
      const u64 bytes = shape.nodes * sizeof(index_detail::Node);
      MemoryBudget index_budget(bytes);
      auto cloud = Input(points).prepare(cloud_budget);
      REQUIRE(cloud.ok());
      {
        const auto index = build_index(std::move(cloud.value()), IndexParams{leaf}, index_budget);
        REQUIRE(index.ok());
        CHECK_EQ(index.value().nodes(), shape.nodes);
        CHECK_EQ(index.value().max_depth(), shape.depth);
        CHECK_EQ(index_budget.used(), bytes);
        CHECK_EQ(index_budget.peak(), bytes);
        CHECK_EQ(index.value().cloud().sites(), n);
      }
      CHECK_EQ(index_budget.used(), 0u);
      CHECK_EQ(cloud_budget.used(), 0u);
    }
  }
  // Profondeur maximale de l'arbre radix : l'origine et les 3B points dont la cle de Morton n'a qu'un bit forment une
  // chaine, chaque coupe isolant un site ; feuille 1, profondeur kMortonBits+1 et 2(3B+1)-1 noeuds.
  std::vector<std::array<u32, 3>> chain{{0, 0, 0}};
  for (int bit = 0; bit < kCoordBits; ++bit)
    for (int axis = 0; axis < 3; ++axis) {
      std::array<u32, 3> p{0, 0, 0};
      p[axis] = u32{1} << bit;
      chain.push_back(p);
    }
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = Input(chain).prepare(budget);
  REQUIRE(cloud.ok());
  auto deep = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(deep.ok());
  CHECK_EQ(deep.value().max_depth(), u64(kMortonBits) + 1);
  CHECK_EQ(deep.value().nodes(), 2 * u64(chain.size()) - 1);
  CHECK_EQ(radix_shape(chain, 1).depth, u64(kMortonBits) + 1);
  // Le parcours sans pile reste exact sur cette chaine : boule ponctuelle a l'origine, coquille = le seul site 0.
  auto origin = census(deep.value(), num::Sphere::point(point(0, 0, 0)), kNone, budget);
  REQUIRE(origin.ok());
  CHECK(origin.value().interior().empty());
  CHECK_EQ(origin.value().shell().size(), 1u);
  CHECK(origin.value().shell().size() == 1 && idx(origin.value().shell()[0]) == 0);
}

MHGP12_TEST(ownership, 35) {
  CHECK(!std::is_default_constructible_v<GlobalIndex>);
  CHECK(!std::is_copy_constructible_v<GlobalIndex>);
  CHECK(!std::is_move_assignable_v<GlobalIndex>);
  CHECK(std::is_nothrow_move_constructible_v<GlobalIndex>);
  CHECK(!std::is_copy_constructible_v<Census> && !std::is_move_assignable_v<Census>);
  CHECK(std::is_nothrow_move_constructible_v<Census>);
  MemoryBudget budget(MemoryBudget::kUnlimited), denied(0);
  auto input = octa();
  auto cloud = input.prepare(budget);
  REQUIRE(cloud.ok());
  const auto old_x = cloud.value().x();
  const auto old_ids = cloud.value().ids();
  const auto base = budget.used();
  for (u32 leaf : {0u, 257u, 4u}) {
    auto result = build_index(std::move(cloud.value()), IndexParams{leaf}, denied);
    CHECK(!result.ok());
    CHECK_EQ(result.outcome().reason, leaf == 4 ? Reason::memory_budget : Reason::parameter_out_of_range);
    CHECK_EQ(cloud.value().sites(), 11u);
    CHECK(cloud.value().x().data() == old_x.data() && cloud.value().ids().data() == old_ids.data());
    CHECK_EQ(budget.used(), base);
    CHECK_EQ(denied.used(), 0u);
  }
  std::optional<Census> retained;
  {
    auto made = build_index(std::move(cloud.value()), IndexParams{4}, budget);
    REQUIRE(made.ok());
    CHECK_EQ(cloud.value().sites(), 0u);
    auto empty = build_index(std::move(cloud.value()), IndexParams{}, budget);
    CHECK(!empty.ok() && empty.outcome().reason == Reason::empty_input);
    GlobalIndex moved(std::move(made.value()));
    CHECK_EQ(made.value().nodes(), 0u);
    CHECK_EQ(made.value().cloud().sites(), 0u);
    auto moved_query = census(made.value(), ball(), 4, budget);
    CHECK(!moved_query.ok() && moved_query.outcome().reason == Reason::empty_input);
    CHECK(moved.cloud().x().data() == old_x.data() && moved.cloud().ids().data() == old_ids.data());
    input.x.assign(input.x.size(), 200);
    auto query = census(moved, ball(), 4, budget);
    REQUIRE(query.ok());
    CHECK(analytic(moved.cloud(), query.value(), 4));
    const auto view = query.value().interior();
    retained.emplace(std::move(query.value()));
    CHECK(query.value().interior().empty() && query.value().shell().empty());
    CHECK(retained->interior().data() == view.data());
  }
  CHECK_EQ(retained->interior().size(), 3u);
  CHECK_EQ(retained->shell().size(), 6u);
  CHECK_EQ(budget.used(), 9u * sizeof(SiteIdx));
  retained.reset();
  CHECK(budget.released().ok());
}

MHGP12_TEST(budget, 22) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = octa().prepare(budget);
  REQUIRE(cloud.ok());
  auto made = build_index(std::move(cloud.value()), IndexParams{4}, budget);
  REQUIRE(made.ok());
  for (u32 threshold : {1u, 3u, 4u, kNone}) {
    const u64 bytes = (threshold <= 3 ? threshold : 9u) * sizeof(SiteIdx);
    MemoryBudget short_budget(bytes + 6), exact_budget(bytes + 7);
    Buffer<u8> occupied_short, occupied_exact;
    REQUIRE(occupied_short.allocate(7, short_budget).ok() && occupied_exact.allocate(7, exact_budget).ok());
    {
      const auto refused = census(made.value(), ball(), threshold, short_budget);
      CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget);
      CHECK_EQ(short_budget.used(), 7u);
      const auto success = census(made.value(), ball(), threshold, exact_budget);
      REQUIRE(success.ok());
      CHECK(analytic(made.value().cloud(), success.value(), threshold));
      CHECK_EQ(exact_budget.used(), bytes + 7);
    }
    CHECK_EQ(exact_budget.used(), 7u);
  }
  const u64 before = budget.used();
  auto invalid = census(made.value(), ball(), 0, budget);
  CHECK(!invalid.ok() && invalid.outcome().reason == Reason::parameter_out_of_range);
  CHECK_EQ(budget.used(), before);
  auto outside = census(made.value(), num::Sphere::point(point(100, 100, 100)), kNone, budget);
  REQUIRE(outside.ok());
  CHECK(outside.value().interior().empty() && outside.value().shell().empty());
  CHECK_EQ(budget.used(), before);
}

MHGP12_TEST(blocks, 14) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = Input({{4, 4, 4}, {4, 5, 4}, {4, 4, 5}}).prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  for (u32 threshold : {2u, 4u}) {
    auto query = census(index.value(), ball(), threshold, budget);
    REQUIRE(query.ok());
    CHECK(analytic(index.value().cloud(), query.value(), threshold));
    CHECK_EQ(query.value().ledger().inside_blocks, 2u);
    CHECK_EQ(query.value().ledger().point_tests, 0u);
    CHECK_EQ(query.value().ledger().nodes, 2u);
  }
  auto outside = census(index.value(), num::Sphere::point(point(0, 0, 0)), 1, budget);
  REQUIRE(outside.ok());
  CHECK_EQ(outside.value().ledger().outside_blocks, 2u);
  CHECK_EQ(outside.value().ledger().point_tests, 0u);
  CHECK(outside.value().interior().empty() && outside.value().shell().empty());
}

// Levier V3 : la borne entiere decide a la racine ce que la borne continue separee laisse raffiner (supports externes
// au nuage). Registres absolus de deux passes : un retour a power_bound_signs dans le parcours est visible ici.
MHGP12_TEST(lattice, 64) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto beside_ball = num::Sphere::through(point(0, 0, 0), point(4, 0, 0));  // c=(2,0,0), r=2
  const auto core_ball = num::Sphere::through(point(0, 0, 0), point(8, 0, 0));    // c=(4,0,0), r=4
  REQUIRE(beside_ball.ok() && beside_ball.value() && core_ball.ok() && core_ball.value());
  // [3,5]x[2,3]x{0} : point entier le plus proche (3,2,0) a distance^2 5 > 4. [2,6]x[0,1]x[0,1] : coin eloigne a 6 < 16.
  const Input beside({{3, 2, 0}, {5, 2, 0}, {4, 3, 0}, {3, 3, 0}, {5, 3, 0}});
  const Input core({{2, 0, 0}, {6, 0, 0}, {2, 1, 1}, {6, 1, 1}, {4, 0, 1}});
  for (u32 leaf : {1u, 16u}) {
    for (const bool inside : {false, true}) {
      auto cloud = (inside ? core : beside).prepare(budget);
      REQUIRE(cloud.ok());
      auto index = build_index(std::move(cloud.value()), IndexParams{leaf}, budget);
      REQUIRE(index.ok());
      const num::Sphere& sphere = inside ? *core_ball.value() : *beside_ball.value();
      for (u32 threshold : {3u, kNone}) {
        auto query = census(index.value(), sphere, threshold, budget);
        REQUIRE(query.ok());
        const auto& ledger = query.value().ledger();
        CHECK_EQ(ledger.passes, 2u);
        CHECK_EQ(ledger.nodes, 2u);
        CHECK_EQ(ledger.bounds, 2u);
        CHECK_EQ(ledger.point_tests, 0u);
        CHECK_EQ(ledger.inside_blocks, inside ? 2u : 0u);
        CHECK_EQ(ledger.outside_blocks, inside ? 0u : 2u);
        CHECK(query.value().shell().empty());
        CHECK_EQ(query.value().interior().size(), inside ? std::min<u64>(threshold, 5) : 0u);
      }
    }
  }
}

MHGP12_TEST(concurrency, 10) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = octa().prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), IndexParams{1}, budget);
  REQUIRE(index.ok());
  const u64 before = budget.used();
  std::array<bool, 4> okay{};
  std::array<CensusLedger, 4> ledgers{};
  std::array<std::thread, 4> threads;
  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i] = std::thread([&, i] {
      MemoryBudget query_budget(4096);
      bool good = true;
      for (int repetition = 0; repetition < 16; ++repetition) {
        const auto result = census(index.value(), ball(), 4, query_budget);
        if (!result.ok() || !analytic(index.value().cloud(), result.value(), 4)) { good = false; break; }
        if (repetition != 0 && !(ledgers[i] == result.value().ledger())) good = false;
        ledgers[i] = result.value().ledger();
      }
      okay[i] = good && query_budget.released().ok();
    });
  }
  for (auto& thread : threads) thread.join();
  for (std::size_t i = 0; i < threads.size(); ++i) {
    CHECK(okay[i]);
    CHECK(ledgers[i] == ledgers[0]);
  }
  CHECK_EQ(budget.used(), before);
}

MHGP12_TEST_MAIN()
