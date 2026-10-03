// Faits analytiques, date minimale, tous refus/proprietaires ; la connectivite globale a son juge Fraction.
#include <limits>
#include <thread>
#include <type_traits>
#include "descent_support.hpp"
#include "test.hpp"
using namespace descent_test;

static_assert(!std::is_default_constructible_v<BirthSeed> && !std::is_default_constructible_v<DescentResult>);
static_assert(std::is_nothrow_copy_constructible_v<DescentStep> && std::is_nothrow_copy_constructible_v<DescentResult>);

MHGP11_TEST(interiors, 30) {
  MemoryBudget owner(MemoryBudget::kUnlimited), zero(0), work(MemoryBudget::kUnlimited);
  auto hit = domain_of(Input({{0,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0}}), owner, 5);
  REQUIRE(hit.ok());
  auto ends = part_of(hit.value(), {{0,0,0},{4,0,0}});
  auto located = locate_part(hit.value(), ends, 2, zero); REQUIRE(located.ok());
  CHECK(located.value().ball().has_value()); CHECK_EQ(located.value().kind(), CensusKind::complete);
  CHECK_EQ(located.value().interior().size(), 3u);
  auto step = descent_step(hit.value(), ends, 2, zero); REQUIRE(step.ok());
  CHECK(strict_step(hit.value(), step.value(), 2)); CHECK_EQ(step.value().ledger().interior_steps, 1u);
  CHECK_EQ(step.value().ledger().catalogue_hits, 1u); CHECK_EQ(step.value().ledger().census_calls, 0u);
  CHECK(equal(step.value().next().part(), located.value().interior().first(2)));
  auto result = descend(hit.value(), ends, 2, zero); REQUIRE(result.ok());
  CHECK(level_is(result.value().initial_level(), 4, 1)); CHECK(level_is(result.value().terminal_level(), 1, 4));
  CHECK_EQ(result.value().ledger().steps, 2u); CHECK(replay(hit.value(), ends, 2, result.value()));
  CHECK(zero.released().ok()); CHECK_EQ(zero.peak(), 0u);
  auto outside = domain_of(Input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}}), owner, 2); REQUIRE(outside.ok());
  ends = part_of(outside.value(), {{0,0,0},{11,0,0}});
  auto sat = descent_step(outside.value(), ends, 2, work); REQUIRE(sat.ok());
  CHECK(strict_step(outside.value(), sat.value(), 2)); CHECK_EQ(sat.value().ledger().interior_steps, 1u);
  CHECK_EQ(sat.value().ledger().census_calls, 1u); CHECK_EQ(sat.value().ledger().census.passes, 2u);
  CHECK_EQ(work.peak(), 8u); CHECK(work.released().ok());
  auto done = descend(outside.value(), ends, 2, work); REQUIRE(done.ok());
  CHECK(level_is(done.value().initial_level(), 121, 4)); CHECK(level_is(done.value().terminal_level(), 1, 4));
  CHECK(replay(outside.value(), ends, 2, done.value())); CHECK(work.released().ok());
}

MHGP11_TEST(outside, 24) {
  const std::array<Input, 2> fixtures{
    Input({{0,0,0},{4,0,0},{2,3,0},{2,1,0},{2,2,0}}),
    Input({{0,0,0},{4,4,0},{4,0,4},{0,4,4},{1,1,1},{2,2,2}})};
  for (u32 i = 0; i < fixtures.size(); ++i) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const u32 k = i + 3;
    auto domain = domain_of(fixtures[i], owner, static_cast<int>(k)); REQUIRE(domain.ok());
    auto part = i == 0 ? part_of(domain.value(), {{0,0,0},{4,0,0},{2,3,0}}) :
                        part_of(domain.value(), {{0,0,0},{4,4,0},{4,0,4},{0,4,4}});
    auto located = locate_part(domain.value(), part, k, work); REQUIRE(located.ok());
    CHECK_EQ(located.value().kind(), CensusKind::complete); CHECK(!located.value().ball());
    CHECK_EQ(located.value().interior().size(), 2u);
    auto step = descent_step(domain.value(), part, k, work); REQUIRE(step.ok());
    CHECK(strict_step(domain.value(), step.value(), k)); CHECK_EQ(step.value().ledger().trace_steps, 1u);
    CHECK_EQ(step.value().ledger().candidate_traces, 1u); CHECK_EQ(step.value().ledger().trace_meb_calls, 0u);
    CHECK(std::includes(step.value().next().part().begin(), step.value().next().part().end(),
                        located.value().interior().begin(), located.value().interior().end()));
    auto result = descend(domain.value(), part, k, work); REQUIRE(result.ok());
    CHECK(replay(domain.value(), part, k, result.value()));
    CHECK(num::compare(result.value().terminal_level(), result.value().initial_level()) < 0);
  }
}

MHGP11_TEST(traces, 28) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner, 5); REQUIRE(domain.ok());
  auto part = part_of(domain.value(), {{0,0,0},{4,4,0},{2,2,0}});
  auto step = descent_step(domain.value(), part, 3, work); REQUIRE(step.ok());
  CHECK(level_is(step.value().level(), 8, 1)); CHECK(strict_step(domain.value(), step.value(), 3));
  CHECK_EQ(step.value().ledger().trace_steps, 1u); CHECK(step.value().ledger().trace_meb_calls > 0);
  auto result = descend(domain.value(), part, 3, work); REQUIRE(result.ok());
  CHECK(level_is(result.value().initial_level(), 8, 1)); CHECK(level_is(result.value().terminal_level(), 4, 1));
  CHECK(replay(domain.value(), part, 3, result.value()));
  auto triple = part_of(domain.value(), {{0,0,0},{4,0,0},{4,4,0},{2,2,0}});
  auto equal_contact = descent_step(domain.value(), triple, 4, work); REQUIRE(equal_contact.ok());
  CHECK(equal_contact.value().seed().has_value()); CHECK_EQ(equal_contact.value().ledger().candidate_traces, 4u);
  CHECK_EQ(equal_contact.value().ledger().trace_meb_calls, 4u); CHECK(equal_contact.value().next().part().empty());
  auto global = domain_of(Input({{1,2,0},{0,5,0},{8,1,0},{8,9,0},{9,8,0}}), owner, 4); REQUIRE(global.ok());
  auto local = part_of(global.value(), {{1,2,0},{0,5,0},{8,1,0},{8,9,0}});
  auto located = locate_part(global.value(), local, 4, work); REQUIRE(located.ok());
  REQUIRE(located.value().support().has_value()); CHECK_EQ(located.value().meb().support().size(), 3u);
  CHECK_EQ(located.value().support()->arity, 2u);
  auto done = descend(global.value(), local, 4, work); REQUIRE(done.ok());
  CHECK(replay(global.value(), local, 4, done.value())); CHECK(level_is(done.value().initial_level(), 25, 1));
  auto extended = domain_of(Input({{10,5,5},{9,8,5},{5,2,1},{1,5,8},{9,2,5}}), owner, 4); REQUIRE(extended.ok());
  auto q4 = part_of(extended.value(), {{10,5,5},{9,8,5},{5,2,1},{1,5,8}});
  auto more = descend(extended.value(), q4, 4, work); REQUIRE(more.ok());
  CHECK(level_is(more.value().initial_level(), 25, 1)); CHECK(replay(extended.value(), q4, 4, more.value()));
  CHECK(num::compare(more.value().terminal_level(), more.value().initial_level()) < 0);
}

MHGP11_TEST(boundaries, 30) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const u32 l = kCoordMax;
  auto domain = domain_of(Input({{0,0,0},{l,l,0},{l,0,l},{0,l,l}}), owner, 4); REQUIRE(domain.ok());
  auto four = all(domain.value().index().cloud());
  auto result = descend(domain.value(), four, 4, work); REQUIRE(result.ok());
  CHECK_EQ(result.value().ledger().steps, 1u); CHECK(result.value().seed().ball().has_value());
  CHECK(valid_terminal(domain.value(), result.value())); CHECK(replay(domain.value(), four, 4, result.value()));
  for (auto id : four) {
    const std::array<SiteIdx, 1> one{id};
    auto zero = descend(domain.value(), one, 1, work); REQUIRE(zero.ok());
    CHECK(zero.value().seed().site() == id); CHECK(!zero.value().seed().ball());
    CHECK_EQ(zero.value().ledger().steps, 1u); CHECK(level_is(zero.value().initial_level(), 0, 1));
  }
  std::vector<Xyz> line;
  for (u32 i = 0; i < 13; ++i) line.push_back({i,0,0});
  auto widest = domain_of(Input(line), owner, 12); REQUIRE(widest.ok());
  auto twelve = all(widest.value().index().cloud()); twelve.erase(twelve.begin() + 6);
  auto last = descend(widest.value(), twelve, 12, work); REQUIRE(last.ok());
  CHECK(level_is(last.value().initial_level(), 36, 1)); CHECK(level_is(last.value().terminal_level(), 121, 4));
  CHECK_EQ(last.value().ledger().steps, 2u); CHECK(replay(widest.value(), twelve, 12, last.value()));
  CHECK(work.released().ok());
}

MHGP11_TEST(refusals, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(Input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}}), owner, 2); REQUIRE(domain.ok());
  const auto part = part_of(domain.value(), {{0,0,0},{11,0,0}});
  for (u32 k : {0u,3u,256u,kNone}) CHECK_EQ(descend(domain.value(), {}, k, zero).outcome().reason, Reason::kmax_out_of_range);
  CHECK_EQ(descend(domain.value(), {}, 1, zero).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), part, 1, zero).outcome().reason, Reason::parameter_out_of_range);
  const std::array<SiteIdx, 2> duplicate{part[0], part[0]}, outside{part[0], SiteIdx{kNone}};
  CHECK_EQ(descend(domain.value(), duplicate, 2, zero).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), outside, 2, zero).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), part, 2, zero).outcome().reason, Reason::memory_budget);
  CHECK(zero.released().ok()); CHECK_EQ(zero.peak(), 0u);
  MemoryBudget short_budget(7), exact(8);
  CHECK_EQ(descend(domain.value(), part, 2, short_budget).outcome().reason, Reason::memory_budget);
  CHECK(short_budget.released().ok());
  auto kept = descend(domain.value(), part, 2, exact); REQUIRE(kept.ok()); CHECK_EQ(exact.peak(), 8u);
  CHECK(exact.released().ok());
  FullDomain moved(std::move(domain.value()));
  CHECK_EQ(descend(domain.value(), part, 2, exact).outcome().reason, Reason::kmax_out_of_range);
  CHECK(valid_terminal(moved, kept.value()));
  CHECK(replay(moved, part, 2, kept.value()));
}

MHGP11_TEST(singleton, 99) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  const u32 h = kCoordMax;
  auto domain = domain_of(Input({{h,0,h},{0,h,h},{h,h,0},{0,0,0}}), owner, 4); REQUIRE(domain.ok());
  auto scratch = CensusWorkspace::make(domain.value().index(), work); REQUIRE(scratch.ok());
  const u64 held = work.used(), owned = owner.used();
  for (SiteIdx site_id : all(domain.value().index().cloud())) {
    const std::array<SiteIdx, 1> part{site_id};
    auto reference = bounded_meb(domain.value().index().cloud(), part); REQUIRE(reference.ok());
    // Le scan natif de reference demeure paye dans le test, jamais dans le raccourci produit.
    {
      auto population = census(domain.value().index(), reference.value().sphere(), 1, work);
      REQUIRE(population.ok()); CHECK_EQ(population.value().kind(), CensusKind::complete);
      CHECK(population.value().interior().empty()); REQUIRE(population.value().shell().size() == 1);
      CHECK_EQ(population.value().shell()[0], site_id);
    }
    auto step = descent_step(domain.value(), part, 1, zero); REQUIRE(step.ok());
    REQUIRE(step.value().seed().has_value()); CHECK(step.value().next().part().empty());
    CHECK(step.value().seed()->site() == site_id); CHECK(!step.value().seed()->ball());
    CHECK_EQ(step.value().seed()->order(), 1u);
    CHECK(step.value().level().numerator() == reference.value().sphere().level().numerator());
    CHECK(step.value().level().denominator() == reference.value().sphere().level().denominator());
    DescentLedger expected;
    expected.steps = 1; expected.singleton_hits = 1; expected.part_meb = reference.value().ledger();
    CHECK(step.value().ledger() == expected);
    auto direct = descend(domain.value(), part, 1, zero); REQUIRE(direct.ok());
    auto borrowed = descend(domain.value(), part, 1, zero, scratch.value().get()); REQUIRE(borrowed.ok());
    CHECK(same(direct.value(), borrowed.value())); CHECK(direct.value().ledger() == expected);
    CHECK(level_is(direct.value().initial_level(), 0, 1)); CHECK(level_is(direct.value().terminal_level(), 0, 1));
    CHECK_EQ(zero.peak(), 0u); CHECK_EQ(work.used(), held); CHECK_EQ(owner.used(), owned);
  }
  CHECK(zero.released().ok());
}

MHGP11_TEST(singleton_refusals, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  const Input input({{0,0,0},{2,0,0},{4,0,0}});
  auto domain = domain_of(input, owner, 2), other = domain_of(input, owner, 2);
  REQUIRE(domain.ok()); REQUIRE(other.ok());
  auto scratch = CensusWorkspace::make(domain.value().index(), work);
  auto foreign = CensusWorkspace::make(other.value().index(), work);
  REQUIRE(scratch.ok()); REQUIRE(foreign.ok());
  const std::array<SiteIdx, 1> good{SiteIdx{1}}, outside{SiteIdx{3}}, sentinel{SiteIdx{kNone}};
  const std::array<SiteIdx, 2> repeated{good[0], good[0]};
  CHECK_EQ(descend(domain.value(), {}, 1, zero).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), repeated, 1, zero).outcome().reason, Reason::parameter_out_of_range);
  for (const auto& part : {outside, sentinel})
    CHECK_EQ(descend(domain.value(), part, 1, zero).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), good, 1, zero, foreign.value().get()).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), {}, 0, zero, foreign.value().get()).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(domain.value(), {}, 0, zero, scratch.value().get()).outcome().reason, Reason::kmax_out_of_range);
  auto retained = descend(domain.value(), good, 1, zero); REQUIRE(retained.ok());
  // Des retours bruts doubles sont fusionnes par Cloud, mais le domaine refuse leur multiplicite.
  CHECK_EQ(domain_of(Input({{0,0,0},{0,0,0}}), owner, 1).outcome().reason, Reason::multiplicity_unsupported);
  FullDomain moved(std::move(domain.value()));
  CHECK_EQ(descend(domain.value(), good, 1, zero).outcome().reason, Reason::kmax_out_of_range);
  CHECK_EQ(descend(domain.value(), good, 1, zero, scratch.value().get()).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(descend(moved, good, 1, zero, scratch.value().get()).outcome().reason, Reason::parameter_out_of_range);
  auto again = descend(moved, good, 1, zero); REQUIRE(again.ok());
  CHECK(same(retained.value(), again.value())); CHECK(zero.released().ok()); CHECK_EQ(zero.peak(), 0u);
}

MHGP11_TEST(capacity, 86) {
  const u64 maximum = std::numeric_limits<u64>::max();
  const std::array<u64 DescentLedger::*, 8> top{&DescentLedger::steps, &DescentLedger::interior_steps,
    &DescentLedger::trace_steps, &DescentLedger::candidate_traces, &DescentLedger::trace_meb_calls,
    &DescentLedger::census_calls, &DescentLedger::catalogue_hits, &DescentLedger::singleton_hits};
  const std::array<u64 MebLedger::*, 7> meb{&MebLedger::presentations, &MebLedger::nondegenerate,
    &MebLedger::positive, &MebLedger::containing, &MebLedger::comparisons, &MebLedger::point_tests, &MebLedger::diameter_pairs};
  const std::array<u64 CensusLedger::*, 6> census{&CensusLedger::nodes, &CensusLedger::bounds,
    &CensusLedger::point_tests, &CensusLedger::inside_blocks, &CensusLedger::outside_blocks, &CensusLedger::passes};
  for (auto field : top) {
    DescentLedger sum, one; sum.*field = maximum; one.*field = 1;
    if (field == &DescentLedger::singleton_hits) one.steps = 1;  // Addition precedente annulee aussi.
    const auto before = sum;
    CHECK_EQ(add_descent(sum, one).reason, Reason::tower_capacity); CHECK(sum == before);
    one.*field = 0; CHECK(add_descent(sum, one).ok());
  }
  for (auto member : {&DescentLedger::part_meb, &DescentLedger::trace_meb}) for (auto field : meb) {
    DescentLedger sum, one; (sum.*member).*field = maximum; (one.*member).*field = 1;
    one.steps = 1; const auto before = sum;
    CHECK_EQ(add_descent(sum, one).reason, Reason::tower_capacity); CHECK(sum == before);
    (one.*member).*field = 0; CHECK(add_descent(sum, one).ok());
  }
  for (auto field : census) {
    DescentLedger sum, one; sum.census.*field = maximum; one.census.*field = 1;
    one.steps = 1; const auto before = sum;
    CHECK_EQ(add_descent(sum, one).reason, Reason::tower_capacity); CHECK(sum == before);
    one.census.*field = 0; CHECK(add_descent(sum, one).ok());
  }
  DescentLedger sum, one; one.steps = 1;
  CHECK(add_descent(sum, one).ok()); CHECK_EQ(sum.steps, 1u);
}

MHGP11_TEST(ownership, 14) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(8), zero(0);
  Input input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}});
  auto domain = domain_of(input, owner, 2); REQUIRE(domain.ok());
  const auto part = part_of(domain.value(), {{0,0,0},{11,0,0}});
  const u64 baseline = owner.used();
  auto result = descend(domain.value(), part, 2, work); REQUIRE(result.ok());
  auto retained = result.value();
  CHECK(work.released().ok()); CHECK_EQ(owner.used(), baseline);
  input.x[0] = kCoordMax;
  CHECK_EQ(descend(domain.value(), part, 2, zero).outcome().reason, Reason::memory_budget);
  CHECK(same(retained, result.value())); CHECK_EQ(owner.used(), baseline);
  auto second = descend(domain.value(), part, 2, work); REQUIRE(second.ok()); CHECK(same(retained, second.value()));
  auto direct = part_of(domain.value(), {{4,0,0},{5,0,0}});
  auto no_alloc = descend(domain.value(), direct, 2, zero); REQUIRE(no_alloc.ok());
  CHECK_EQ(no_alloc.value().ledger().steps, 1u); CHECK(work.released().ok()); CHECK(zero.released().ok());
  CHECK(valid_terminal(domain.value(), retained));
}

MHGP11_TEST(concurrency, 8) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner, 3); REQUIRE(domain.ok());
  const auto part = part_of(domain.value(), {{0,0,0},{4,4,0},{2,2,0}});
  auto baseline = descend(domain.value(), part, 3, work); REQUIRE(baseline.ok());
  std::array<bool, 4> correct{}; std::array<std::thread, 4> threads;
  for (u32 i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    MemoryBudget local(MemoryBudget::kUnlimited);
    bool ok = true;
    for (u32 repeat = 0; repeat < 4; ++repeat) {
      auto result = descend(domain.value(), part, 3, local);
      ok = ok && result.ok() && same(result.value(), baseline.value());
    }
    correct[i] = ok && local.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (bool ok : correct) CHECK(ok);
  CHECK(work.released().ok()); CHECK(replay(domain.value(), part, 3, baseline.value()));
}
MHGP11_TEST_MAIN()
