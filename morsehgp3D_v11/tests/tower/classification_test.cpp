// Classification T2 sans allocation : faits de prefixe, qmin global et rejeu exhaustif toujours distinct.
#include <thread>
#include <type_traits>
#include "cells_support.hpp"
#include "test.hpp"

using namespace cells_test;
static_assert(std::is_trivially_copyable_v<CellClassification>);
static_assert(!std::is_default_constructible_v<CellClassification>);

MHGP11_TEST(analytic, 45) {
  const std::array<Input, 3> inputs{Input({{0,0,0},{3,0,0}}), Input({{0,0,0},{4,0,0},{2,3,0}}),
                                  Input({{0,0,0},{2,2,0},{2,0,2},{0,2,2}})};
  for (u32 i = 0; i < inputs.size(); ++i) {
    MemoryBudget owner(MemoryBudget::kUnlimited);
    auto domain = domain_of(inputs[i], owner); REQUIRE(domain.ok());
    const u8 q = static_cast<u8>(i + 2); const auto b = select(domain.value(), 0, q, q);
    const u64 held = owner.used();
    for (Order k : {static_cast<Order>(q - 1), static_cast<Order>(q)}) {
      auto result = classify_cell(domain.value(), b, k); REQUIRE(result.ok());
      CHECK(result.value().kind() == (k == q ? CellKind::birth : CellKind::strict_traces));
      CHECK_EQ(result.value().ledger().combinations, k == q ? 1u : q);
      CHECK_EQ(result.value().ledger().examined, 0u); CHECK_EQ(result.value().ledger().meb_calls, 0u);
      CHECK(result.value().ledger().meb == MebLedger{});
    }
    CHECK_EQ(owner.used(), held);
  }
  MemoryBudget owner(MemoryBudget::kUnlimited);
  Input wide({{0,5,5},{1,2,5},{1,8,5},{2,1,5},{2,9,5},{5,0,5},{5,10,5},
              {8,1,5},{8,9,5},{9,2,5},{9,8,5},{10,5,5},{5,5,0},{5,5,10}});
  auto domain = domain_of(wide, owner, 1); REQUIRE(domain.ok());
  auto result = classify_cell(domain.value(), select(domain.value(), 0, 14, 2), 1); REQUIRE(result.ok());
  CHECK(result.value().kind() == CellKind::strict_traces);
  CHECK_EQ(result.value().ledger().combinations, 14u); CHECK_EQ(result.value().ledger().examined, 0u);
  CHECK(result.value().ledger().meb == MebLedger{});
}

MHGP11_TEST(prefix, 35) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  auto first = classify_cell(domain.value(), b, 3); REQUIRE(first.ok());
  CHECK(first.value().kind() == CellKind::strict_traces);
  CHECK_EQ(first.value().ledger().combinations, 6u); CHECK_EQ(first.value().ledger().examined, 1u);
  CHECK_EQ(first.value().ledger().meb_calls, 1u); CHECK_EQ(first.value().ledger().meb.containing, 1u);
  auto none = classify_cell(domain.value(), b, 4); REQUIRE(none.ok());
  CHECK(none.value().kind() == CellKind::birth); CHECK_EQ(none.value().ledger().examined, 4u);
  CHECK_EQ(none.value().ledger().meb_calls, 4u); CHECK_EQ(none.value().ledger().meb.containing, 4u);
  auto replay = build_cell(domain.value(), b, 3, work); REQUIRE(replay.ok());
  CHECK_EQ(replay.value().traces().size(), 4u); CHECK_EQ(replay.value().ledger().trace_tests, 12u);
  Input global({{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}});
  auto other = domain_of(global, owner); REQUIRE(other.ok());
  const auto g = select(other.value(), 0, 5, 3);
  CHECK_EQ(idx(other.value().catalogue().balls_data()[idx(g)].support[0]), 1u);
  const std::array<SiteIdx, 4> local{SiteIdx{0}, SiteIdx{1}, SiteIdx{2}, SiteIdx{4}};
  auto local_meb = bounded_meb(other.value().index().cloud(), local); REQUIRE(local_meb.ok());
  CHECK_EQ(local_meb.value().support().size(), 4u);
  auto late = classify_cell(other.value(), g, 4); REQUIRE(late.ok());
  CHECK(late.value().kind() == CellKind::strict_traces);
  CHECK_EQ(late.value().ledger().combinations, 5u); CHECK_EQ(late.value().ledger().examined, 3u);
  CHECK_EQ(late.value().ledger().meb_calls, 3u);
  CHECK_EQ(late.value().ledger().meb.containing, 3u); CHECK_EQ(late.value().ledger().meb.comparisons, 0u);
  auto below = classify_cell(other.value(), g, 2); REQUIRE(below.ok());
  CHECK(below.value().kind() == CellKind::strict_traces); CHECK_EQ(below.value().ledger().examined, 0u);
  auto octa = domain_of(Input({{2,2,0},{2,0,2},{0,2,2},{4,2,2},{2,4,2},{2,2,4},{2,2,2}}), owner);
  REQUIRE(octa.ok()); const auto o = select(octa.value(), 1, 6, 2);
  auto classification = classify_cell(octa.value(), o, 3); REQUIRE(classification.ok());
  auto exhaustive = build_cell(octa.value(), o, 3, work); REQUIRE(exhaustive.ok());
  CHECK_EQ(classification.value().ledger().examined, 1u); CHECK_EQ(exhaustive.value().traces().size(), 12u);
  CHECK_EQ(exhaustive.value().ledger().trace_tests, 30u);
  CHECK_EQ(classification.value().ledger().examined + exhaustive.value().ledger().trace_tests, 31u);
}

MHGP11_TEST(refusals, 12) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto domain = domain_of(Input({{0,0,0},{2,2,0},{2,0,2},{0,2,2}}), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 0, 4, 4); const u64 held = owner.used();
  for (Order k : {Order{0}, Order{1}, Order{2}, Order{5}, Order{13}}) {
    auto no = classify_cell(domain.value(), b, k);
    CHECK(!no.ok() && no.outcome().reason == Reason::parameter_out_of_range);
  }
  auto no = classify_cell(domain.value(), BallIdx{kNone}, 3);
  CHECK(!no.ok() && no.outcome().reason == Reason::parameter_out_of_range);
  CHECK_EQ(owner.used(), held);
  FullDomain moved(std::move(domain.value()));
  auto empty = classify_cell(domain.value(), b, 3);
  CHECK(!empty.ok() && empty.outcome().reason == Reason::parameter_out_of_range);
  auto yes = classify_cell(moved, b, 3); REQUIRE(yes.ok());
  CHECK(yes.value().kind() == CellKind::strict_traces); CHECK_EQ(owner.used(), held);
}

MHGP11_TEST(ownership_concurrency, 10) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  Input input = square(); auto domain = domain_of(input, owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  auto baseline = classify_cell(domain.value(), b, 3); REQUIRE(baseline.ok());
  const auto saved = baseline.value(); const u64 held = owner.used();
  input.x[0] = kCoordMax;
  auto again = classify_cell(domain.value(), b, 3); REQUIRE(again.ok());
  CHECK(saved.ledger() == again.value().ledger());
  std::array<bool, 4> correct{};
  std::array<std::thread, 4> threads;
  for (u32 i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    bool ok = true;
    for (u32 repeat = 0; repeat < 8; ++repeat) {
      auto result = classify_cell(domain.value(), b, 3);
      ok = ok && result.ok() && result.value().kind() == saved.kind() && result.value().ledger() == saved.ledger();
    }
    correct[i] = ok;
  });
  for (auto& thread : threads) thread.join();
  for (bool ok : correct) CHECK(ok);
  CHECK_EQ(owner.used(), held); CHECK(saved.ledger() == baseline.value().ledger());
}
MHGP11_TEST_MAIN()
