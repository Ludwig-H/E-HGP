// Plateaux, continuation, verticales fermees, memoire possedee, bords et concurrence ; sans chronos FULL.
#include <limits>
#include <thread>
#include <type_traits>
#include "forest_support.hpp"
#include "test.hpp"
using namespace forest_test;
static_assert(!std::is_copy_constructible_v<OrderForest> && !std::is_move_assignable_v<OrderForest>);
static_assert(!std::is_copy_constructible_v<FullTower> && std::is_nothrow_move_constructible_v<FullTower>);
static_assert(sizeof(BirthEntry) == 8 && sizeof(ForestNode) == 24);

MHGP11_TEST(plateau, 21) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto made = tower_of(Input({{0,0,0},{2,0,0},{4,0,0}}), 3, owner, work); REQUIRE(made.ok());
  const auto& full = made.value(); const auto& one = full.order(1);
  CHECK_EQ(one.births(), 3u); CHECK_EQ(one.nodes().size(), 4u); CHECK(structure(one));
  CHECK_EQ(one.root(), NodeIdx{3}); CHECK_EQ(one.children(one.root()).size(), 3u);
  CHECK(level_is(full.domain().catalogue().levels()[idx(one.nodes()[3].rank)], 1, 1));
  CHECK(one.lower().empty()); CHECK_EQ(one.ledger().plateaus, 1u);
  CHECK_EQ(one.ledger().unions, 2u);
  const auto& two = full.order(2); CHECK_EQ(two.births(), 2u); CHECK_EQ(two.nodes().size(), 3u);
  CHECK_EQ(two.children(two.root()).size(), 2u); CHECK(structure(two));
  CHECK(level_is(full.domain().catalogue().levels()[idx(two.nodes()[2].rank)], 4, 1));
  const auto& three = full.order(3); CHECK_EQ(three.births(), 1u); CHECK_EQ(three.nodes().size(), 1u);
  CHECK(structure(three)); CHECK_EQ(three.node_capacity(), 1u); CHECK_EQ(three.edge_capacity(), 0u);
  CHECK_EQ(work.used(), retained(one) + retained(two) + retained(three));
  auto square_four = tower_of(Input({{0,0,0},{4,0,0},{0,4,0},{4,4,0}}), 1, owner, work);
  REQUIRE(square_four.ok()); CHECK_EQ(square_four.value().order(1).nodes().size(), 5u);
  CHECK(square_four.value().order(1).ledger().continuations > 0);
}

MHGP11_TEST(multigroup, 13) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto made = tower_of(Input({{0,0,0},{2,0,0},{4,0,0},{20,0,0},{22,0,0},{24,0,0}}), 1, owner, work);
  REQUIRE(made.ok()); const auto& full = made.value(); const auto& f = full.order(1);
  CHECK(structure(f)); CHECK_EQ(f.births(), 6u); CHECK_EQ(f.nodes().size(), 9u);
  CHECK_EQ(f.nodes()[6].child_count, 3u); CHECK_EQ(f.nodes()[7].child_count, 3u);
  CHECK(f.nodes()[6].rank == f.nodes()[7].rank);
  CHECK_EQ(f.children(NodeIdx{6})[0], NodeIdx{0}); CHECK_EQ(f.children(NodeIdx{7})[0], NodeIdx{3});
  CHECK_EQ(f.nodes()[8].child_count, 2u); CHECK_EQ(f.children(NodeIdx{8})[0], NodeIdx{6});
  CHECK_EQ(f.children(NodeIdx{8})[1], NodeIdx{7});
  CHECK(level_is(full.domain().catalogue().levels()[idx(f.nodes()[8].rank)], 64, 1));
}

MHGP11_TEST(verticals, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto full = tower_of(Input({{0,0,0},{2,0,0},{4,0,0}}), 3, owner, work); REQUIRE(full.ok());
  const auto& a = full.value().order(1); const auto& b = full.value().order(2); const auto& c = full.value().order(3);
  CHECK_EQ(b.lower().size(), 3u);
  for (NodeIdx v : b.lower()) CHECK_EQ(v, a.root());  // Egalite au niveau1 : coupe fermee.
  CHECK_EQ(c.lower().size(), 1u); CHECK_EQ(c.lower()[0], b.root());
  CHECK_EQ(b.ledger().vertical_descents, b.births()); CHECK_EQ(c.ledger().vertical_descents, c.births());
  CHECK_EQ(b.ledger().vertical_checks, 2u); CHECK_EQ(c.ledger().vertical_checks, 0u);
  u64 hops = 0;
  auto equal_level = a.ancestor_closed(NodeIdx{0}, a.nodes()[idx(a.root())].rank, hops);
  REQUIRE(equal_level.ok()); CHECK_EQ(equal_level.value(), a.root()); CHECK_EQ(hops, 1u);
  auto before = a.ancestor_closed(NodeIdx{0}, LevelRank{0}, hops); REQUIRE(before.ok());
  CHECK_EQ(before.value(), NodeIdx{0}); CHECK_EQ(hops, 1u);
  CHECK_EQ(a.ancestor_closed(NodeIdx{kNone}, LevelRank{0}, hops).outcome().reason, Reason::parameter_out_of_range);
  hops = std::numeric_limits<u64>::max();
  CHECK_EQ(a.ancestor_closed(NodeIdx{0}, a.nodes()[idx(a.root())].rank, hops).outcome().reason, Reason::tower_capacity);
  CHECK_EQ(hops, std::numeric_limits<u64>::max());
}

MHGP11_TEST(canonical, 15) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input({{0,4,0},{4,0,0}});
  auto full = tower_of(input, 2, owner, work); REQUIRE(full.ok());
  const auto& domain = full.value().domain(); const auto& one = full.value().order(1);
  const auto first = site(domain.index().cloud(), {0,4,0});
  CHECK_EQ(idx(first), 1u); CHECK_EQ(one.nodes()[0].birth_key, idx(first));
  CHECK_EQ(one.nodes()[1].birth_key, 0u); CHECK(structure(one));
  const std::array<SiteIdx, 1> part{first};
  auto seed = descend(domain, part, 1, work); REQUIRE(seed.ok());
  CHECK(one.birth_node(seed.value().seed()) == NodeIdx{0});
  CHECK(!full.value().order(2).birth_node(seed.value().seed()));
  auto reverse = tower_of(Input({{4,0,0},{0,4,0}}), 2, owner, work); REQUIRE(reverse.ok());
  CHECK(same(one, reverse.value().order(1))); CHECK(same(full.value().order(2), reverse.value().order(2)));
  auto singleton = tower_of(Input({{7,8,9}}), 1, owner, work); REQUIRE(singleton.ok());
  CHECK_EQ(singleton.value().order(1).nodes().size(), 1u); CHECK(structure(singleton.value().order(1)));
  CHECK_EQ(singleton.value().order(1).ledger().birth_presentations, 1u);
}

MHGP11_TEST(ownership, 21) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(square(), owner, 5); REQUIRE(domain.ok());
  const auto* points = domain.value().index().cloud().x().data(); const u64 initial = owner.used();
  auto refused = build_full(std::move(domain.value()), zero);
  CHECK_EQ(refused.outcome().reason, Reason::memory_budget); CHECK_EQ(owner.used(), initial);
  CHECK(domain.value().index().cloud().x().data() == points); CHECK(zero.released().ok());
  {
    auto made = build_full(std::move(domain.value()), work); REQUIRE(made.ok());
    CHECK_EQ(domain.value().index().cloud().sites(), 0u); CHECK_EQ(domain.value().catalogue().kmax(), 0u);
    CHECK(made.value().domain().index().cloud().x().data() == points);
    const auto* nodes = made.value().order(1).nodes().data();
    FullTower moved(std::move(made.value()));
    CHECK_EQ(made.value().kmax(), 0u); CHECK_EQ(made.value().domain().index().cloud().sites(), 0u);
    CHECK(moved.order(1).nodes().data() == nodes); CHECK(structure(moved.order(1)));
    u64 retained_bytes = 0;
    for (Order k = 1; k <= moved.kmax(); ++k) retained_bytes += retained(moved.order(k));
    CHECK_EQ(work.used(), retained_bytes); CHECK_EQ(owner.used(), initial);
    auto second_domain = domain_of(square(), owner, 5); REQUIRE(second_domain.ok());
    auto second = build_full(std::move(second_domain.value()), zero);
    CHECK_EQ(second.outcome().reason, Reason::memory_budget); CHECK(moved.order(1).nodes().data() == nodes);
    CHECK_EQ(work.used(), retained_bytes);
  }
  CHECK(work.released().ok()); CHECK(owner.released().ok());
}

MHGP11_TEST(refusals, 14) {
  auto limit = forest_capacities((u64{1} << 31) - 1); REQUIRE(limit.ok());
  CHECK_EQ(limit.value()[0], u64{kNone} - 2); CHECK_EQ(limit.value()[1], u64{kNone} - 3);
  CHECK_EQ(forest_capacities(u64{1} << 31).outcome().reason, Reason::tower_capacity);
  CHECK_EQ(forest_capacities(std::numeric_limits<u64>::max()).outcome().reason, Reason::tower_capacity);
  CHECK_EQ(forest_capacities(0).outcome().reason, Reason::tower_invariant);
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,0,0},{2,0,0}}), owner, 3); REQUIRE(domain.ok());
  const auto* address = domain.value().index().cloud().x().data(); const u64 baseline = owner.used();
  CHECK_EQ(build_full(std::move(domain.value()), work).outcome().reason, Reason::parameter_out_of_range);
  CHECK(domain.value().index().cloud().x().data() == address); CHECK_EQ(owner.used(), baseline);
  CHECK(work.released().ok()); CHECK_EQ(work.peak(), 0u);
  for (u32 k : {0u,3u,4u,kNone}) CHECK_EQ(build_forest(domain.value(), k, work).outcome().reason, Reason::parameter_out_of_range);
  auto one = build_forest(domain.value(), 1, work); REQUIRE(one.ok());
  OrderForest moved(std::move(one.value()));
  CHECK(one.value().nodes().empty() && one.value().edges().empty() && one.value().lower().empty());
  CHECK_EQ(one.value().order(), 0u); CHECK_EQ(one.value().root(), NodeIdx{kNone}); CHECK(structure(moved));
}

MHGP11_TEST(concurrency, 8) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner, 5); REQUIRE(domain.ok());
  auto baseline = build_forest(domain.value(), 3, work); REQUIRE(baseline.ok());
  std::array<bool, 4> correct{}; std::array<std::thread, 4> threads;
  for (u32 i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    MemoryBudget local(MemoryBudget::kUnlimited);
    {
      auto result = build_forest(domain.value(), 3, local);
      correct[i] = result.ok() && same(result.value(), baseline.value()) && structure(result.value());
    }
    correct[i] = correct[i] && local.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (bool ok : correct) CHECK(ok);
  CHECK(structure(baseline.value())); CHECK(domain.value().catalogue().balls() > 0);
}
MHGP11_TEST_MAIN()
