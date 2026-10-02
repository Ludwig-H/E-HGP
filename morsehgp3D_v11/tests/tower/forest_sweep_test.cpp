// Balayage ferme contre marches de parents conservees, memoire explicite, profondeur et diagnostics.
#include <thread>
#include "forest_support.hpp"
#include "tower/forest_ancestor_sweep.hpp"
#include "test.hpp"
using namespace forest_test;

MHGP11_TEST(reference, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), queries(MemoryBudget::kUnlimited);
  auto full = tower_of(Input({{0,0,0},{1,0,0},{2,0,0},{6,0,0},{9,0,0}}), 5, owner, work);
  REQUIRE(full.ok());
  const auto& levels = full.value().domain().catalogue().levels();
  for (Order k = 1; k <= full.value().kmax(); ++k) {
    const auto& f = full.value().order(k);
    {
      auto sweep = ClosedAncestorSweep::make(f, queries); REQUIRE(sweep.ok());
      CHECK_EQ(queries.used(), 12 * f.nodes().size());
      ForestLedger ledger;
      CHECK_EQ(sweep.value().query(NodeIdx{0}, ledger).outcome().reason, Reason::parameter_out_of_range);
      for (u32 rank = 0; rank < levels.size(); ++rank) {
        REQUIRE(sweep.value().advance(LevelRank{rank}, ledger).ok());
        for (u32 node = 0; node < f.nodes().size(); ++node) if (idx(f.nodes()[node].rank) <= rank) {
          u64 hops = 0;
          auto reference = f.ancestor_closed(NodeIdx{node}, LevelRank{rank}, hops);
          auto actual = sweep.value().query(NodeIdx{node}, ledger);
          REQUIRE(reference.ok()); REQUIRE(actual.ok()); CHECK_EQ(actual.value(), reference.value());
        }
      }
      CHECK_EQ(ledger.ancestor_activations, f.nodes().size() - f.births());
      CHECK_EQ(ledger.ancestor_unions, f.edges().size()); CHECK_EQ(ledger.ancestor_hops, 0u);
      const auto before = ledger;
      CHECK_EQ(sweep.value().advance(LevelRank{0}, ledger).reason, Reason::parameter_out_of_range);
      CHECK_EQ(sweep.value().query(NodeIdx{kNone}, ledger).outcome().reason, Reason::parameter_out_of_range);
      CHECK(ledger == before);
    }
    CHECK(queries.released().ok());
  }
}

MHGP11_TEST(depth_and_memory, 20) {
  std::vector<std::array<u32, 3>> xyz;
  for (u32 i = 0; i < 16; ++i) xyz.push_back({(u32{1} << i) - 1, 0, 0});
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), queries(MemoryBudget::kUnlimited);
  auto full = tower_of(Input(xyz), 1, owner, work); REQUIRE(full.ok());
  const auto& f = full.value().order(1); CHECK_EQ(f.nodes().size(), 31u);
  const LevelRank last = f.nodes()[idx(f.root())].rank;
  u64 linear_hops = 0; auto linear = f.ancestor_closed(NodeIdx{0}, last, linear_hops);
  REQUIRE(linear.ok()); CHECK_EQ(linear.value(), f.root()); CHECK(linear_hops >= 14);
  MemoryBudget short_budget(12 * f.nodes().size() - 1);
  CHECK_EQ(ClosedAncestorSweep::make(f, short_budget).outcome().reason, Reason::memory_budget);
  CHECK(short_budget.released().ok()); CHECK_EQ(short_budget.peak(), 0u);
  {
    auto made = ClosedAncestorSweep::make(f, queries); REQUIRE(made.ok());
    ClosedAncestorSweep sweep(std::move(made.value())); ForestLedger ledger;
    CHECK_EQ(made.value().advance(last, ledger).reason, Reason::parameter_out_of_range);
    REQUIRE(sweep.advance(last, ledger).ok());
    const u64 before = ledger.ancestor_find_steps;
    auto actual = sweep.query(NodeIdx{0}, ledger); REQUIRE(actual.ok()); CHECK_EQ(actual.value(), f.root());
    // Union par taille : profondeur <=floor(log2 31)=4 ; deux marches dans find, marge de 2 pas.
    CHECK(ledger.ancestor_find_steps - before <= 10);
    CHECK_EQ(ledger.ancestor_queries, 1u); CHECK_EQ(ledger.ancestor_activations, 15u);
    CHECK_EQ(ledger.ancestor_unions, 30u); CHECK_EQ(queries.used(), 12 * f.nodes().size());
  }
  CHECK(queries.released().ok()); CHECK(structure(f));
}

MHGP11_TEST(timings, 36) {
  MemoryBudget owner(MemoryBudget::kUnlimited), first(MemoryBudget::kUnlimited), second(MemoryBudget::kUnlimited), zero(0);
  const Input input({{0,0,0},{1,0,0},{2,0,0},{6,0,0},{9,0,0}});
  auto plain = tower_of(input, 5, owner, first); REQUIRE(plain.ok());
  auto domain = domain_of(input, owner, 5); REQUIRE(domain.ok());
  FullTimings times;
  for (auto& t : times.orders) t = {7,11,13,17};
  const auto preserved = times;
  CHECK_EQ(build_full(std::move(domain.value()), zero, &times).outcome().reason, Reason::memory_budget);
  CHECK(times == preserved);
  Stopwatch wall;
  auto timed = build_full(std::move(domain.value()), second, &times);
  const u64 elapsed = wall.nanoseconds(); REQUIRE(timed.ok());
  u64 measured = 0;
  for (Order k = 1; k <= 5; ++k) {
    CHECK(same(plain.value().order(k), timed.value().order(k)));
    CHECK(plain.value().order(k).ledger() == timed.value().order(k).ledger());
    const auto& l = timed.value().order(k).ledger();
    CHECK_EQ(l.ancestor_hops, 0u);
    CHECK_EQ(l.ancestor_queries, l.vertical_descents + l.vertical_checks);
    const auto& t = times.orders[k - 1];
    const u64 phases = t.classify_ns + t.births_ns + t.plateaus_ns + t.verticals_ns;
    CHECK(phases <= elapsed); measured += phases;
  }
  CHECK(measured <= elapsed);
  CHECK_EQ(times.orders[0].verticals_ns, 0u);
  for (u32 k = 5; k < times.orders.size(); ++k) CHECK(times.orders[k] == OrderTimings{});
  CHECK_EQ(first.used(), second.used()); CHECK_EQ(first.peak(), second.peak());
}

MHGP11_TEST(concurrency, 7) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto full = tower_of(Input({{0,0,0},{2,0,0},{4,0,0},{20,0,0},{22,0,0},{24,0,0}}), 2, owner, work);
  REQUIRE(full.ok()); const auto& f = full.value().order(1);
  std::array<bool, 4> correct{}; std::array<std::thread, 4> threads;
  for (u32 i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    MemoryBudget local(MemoryBudget::kUnlimited);
    {
      auto sweep = ClosedAncestorSweep::make(f, local); ForestLedger ledger;
      if (!sweep.ok() || !sweep.value().advance(f.nodes()[idx(f.root())].rank, ledger).ok()) return;
      const auto result = sweep.value().query(NodeIdx{i}, ledger);
      correct[i] = result.ok() && result.value() == f.root();
    }
    correct[i] = correct[i] && local.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (bool okay : correct) CHECK(okay);
  CHECK(structure(f)); CHECK_EQ(f.ledger().ancestor_queries, 0u);
}
MHGP11_TEST_MAIN()
