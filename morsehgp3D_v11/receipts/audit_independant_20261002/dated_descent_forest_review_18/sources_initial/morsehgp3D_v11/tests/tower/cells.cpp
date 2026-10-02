// Cellules : faits analytiques independants, traces exhaustives, proprietaires et capacites.
#include <limits>
#include <thread>
#include <type_traits>

#include "cells_support.hpp"
#include "test.hpp"

using namespace cells_test;

static_assert(!std::is_default_constructible_v<LocalCell> && !std::is_copy_constructible_v<LocalCell>);
static_assert(!std::is_move_assignable_v<LocalCell> && std::is_nothrow_move_constructible_v<LocalCell>);
static_assert(std::is_trivially_copyable_v<CellTrace>);

MHGP11_TEST(regular, 70) {
  const std::array<Input, 3> inputs{Input({{0,0,0},{4,0,0},{5,0,0},{11,0,0}}),
    Input({{0,0,0},{4,0,0},{2,3,0}}), Input({{0,0,0},{2,2,0},{2,0,2},{0,2,2}})};
  for (u32 fixture = 0; fixture < inputs.size(); ++fixture) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto made = domain_of(inputs[fixture], owner);
    REQUIRE(made.ok());
    const auto& domain = made.value();
    const u8 q = static_cast<u8>(fixture + 2);
    const u32 p = fixture == 0 ? 2 : 0;
    const auto b = select(domain, p, q, q);
    const auto k = static_cast<Order>(p + q - 1);
    auto joined = build_cell(domain, b, k, work);
    REQUIRE(joined.ok());
    const auto& cell = joined.value();
    CHECK(cell.regular()); CHECK(cell.kind() == CellKind::strict_traces);
    CHECK_EQ(cell.traces().size(), q); CHECK_EQ(cell.order(), k); CHECK(cell.ball() == b);
    CHECK_EQ(cell.ledger().combinations, q); CHECK_EQ(cell.ledger().passes, 0u);
    CHECK_EQ(cell.ledger().trace_tests, 0u); CHECK_EQ(cell.ledger().meb_calls, 0u);
    CHECK(cell.ledger().meb == MebLedger{});
    CHECK_EQ(work.used(), sizeof(CellTrace) * q);
    for (const auto& trace : cell.traces()) {
      CHECK(trace_valid(trace, domain.index().cloud(), k));
      CHECK(std::includes(trace.part().begin(), trace.part().end(),
                          domain.catalogue().interior(b).begin(), domain.catalogue().interior(b).end()));
    }
    for (std::size_t i = 1; i < cell.traces().size(); ++i) CHECK(cell.traces()[i-1].sites < cell.traces()[i].sites);
    MemoryBudget zero(0);
    auto birth = build_cell(domain, b, static_cast<Order>(p + q), zero);
    REQUIRE(birth.ok());
    CHECK(birth.value().kind() == CellKind::birth); CHECK(birth.value().regular());
    CHECK(birth.value().traces().empty()); CHECK_EQ(birth.value().ledger().combinations, 1u);
    CHECK_EQ(birth.value().ledger().passes, 0u); CHECK(zero.released().ok());
  }
}

MHGP11_TEST(extended, 90) {
  for (u32 p : {0u, 1u}) {
    Input input = p == 1 ? square() : Input({{0,0,0},{4,0,0},{0,4,0},{4,4,0}});
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto made = domain_of(input, owner);
    REQUIRE(made.ok());
    const auto& domain = made.value();
    const auto b = select(domain, p, 4, 2);
    for (u32 t = 1; t <= 4; ++t) {
      const Order k = static_cast<Order>(p + t);
      auto result = build_cell(domain, b, k, work);
      REQUIRE(result.ok());
      const auto& cell = result.value();
      const u64 size = t <= 2 ? 4 : 0;
      CHECK(!cell.regular()); CHECK_EQ(cell.traces().size(), size);
      CHECK(cell.kind() == (size == 0 ? CellKind::birth : CellKind::strict_traces));
      CHECK_EQ(cell.ledger().combinations, t == 2 ? 6u : t == 4 ? 1u : 4u);
      CHECK_EQ(cell.ledger().passes, t == 4 ? 0u : 2u);
      CHECK_EQ(cell.ledger().trace_tests, t == 4 ? 0u : t == 2 ? 12u : 8u);
      CHECK_EQ(cell.ledger().meb_calls, t == 2 ? 12u : t == 3 ? 8u : 0u);
      CHECK_EQ(work.used(), size * sizeof(CellTrace));
      for (const auto& trace : cell.traces()) {
        CHECK(trace_valid(trace, domain.index().cloud(), k));
        if (t == 2) {
          std::array<SiteIdx, 2> corners{}; u32 n = 0;
          for (SiteIdx s : trace.part())
            if (std::find(domain.catalogue().shell(b).begin(), domain.catalogue().shell(b).end(), s) !=
                domain.catalogue().shell(b).end()) corners[n++] = s;
          REQUIRE(n == 2);
          const auto& c = domain.index().cloud();
          const i64 dx = i64{c.x()[idx(corners[0])]} - c.x()[idx(corners[1])];
          const i64 dy = i64{c.y()[idx(corners[0])]} - c.y()[idx(corners[1])];
          CHECK_EQ(dx * dx + dy * dy, 16);  // Quatre aretes, jamais les deux diagonales de niveau egal.
        }
      }
    }
    CHECK(work.released().ok());
  }
}

MHGP11_TEST(capacity, 20) {
  CHECK_EQ(sizeof(CellTrace), 52u);
  auto c = cell_binomial(24, 12); REQUIRE(c.ok()); CHECK_EQ(c.value(), 2704156u);
  c = cell_binomial(kNone, 2); REQUIRE(c.ok()); CHECK_EQ(c.value(), 9223372030412324865ULL);
  c = cell_binomial(kNone, 12); CHECK(!c.ok() && c.outcome().reason == Reason::tower_capacity);
  c = cell_binomial(3, 4); CHECK(!c.ok() && c.outcome().reason == Reason::parameter_out_of_range);
  c = cell_binomial(100, 13); CHECK(!c.ok() && c.outcome().reason == Reason::parameter_out_of_range);
  u64 value = std::numeric_limits<u64>::max() - 1;
  CHECK(cell_add(value, 1).ok()); CHECK_EQ(value, std::numeric_limits<u64>::max());
  CHECK(cell_add(value, 1).reason == Reason::tower_capacity); CHECK_EQ(value, std::numeric_limits<u64>::max());
  CellLedger a, d;
  CHECK(cell_same_pass(1, 1, a, d).ok());
  CHECK(cell_same_pass(1, 2, a, d).reason == Reason::tower_invariant);
  d.trace_tests = 1; CHECK(cell_same_pass(1, 1, a, d).reason == Reason::tower_invariant);
  MemoryBudget owner(MemoryBudget::kUnlimited), short_budget(4 * sizeof(CellTrace) - 1), exact(4 * sizeof(CellTrace));
  auto domain = domain_of(square(), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  auto refused = build_cell(domain.value(), b, 3, short_budget);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::memory_budget); CHECK(short_budget.released().ok());
  auto good = build_cell(domain.value(), b, 3, exact); REQUIRE(good.ok());
  CHECK_EQ(exact.used(), 4 * sizeof(CellTrace)); CHECK_EQ(exact.peak(), exact.used());
}

MHGP11_TEST(ownership, 15) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(4 * sizeof(CellTrace));
  Input input = square();
  auto domain = domain_of(input, owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  {
    auto result = build_cell(domain.value(), b, 3, work); REQUIRE(result.ok());
    const auto* address = result.value().traces().data();
    const auto first = result.value().traces()[0];
    LocalCell moved(std::move(result.value()));
    CHECK(result.value().traces().empty()); CHECK_EQ(result.value().order(), 0u);
    CHECK(moved.traces().data() == address); CHECK(moved.traces()[0].sites == first.sites);
    input.x[0] = kCoordMax;
    auto failed = build_cell(domain.value(), b, 3, work);
    CHECK(!failed.ok() && failed.outcome().reason == Reason::memory_budget);
    CHECK(moved.traces()[0].sites == first.sites); CHECK_EQ(work.used(), 4 * sizeof(CellTrace));
    auto birth = build_cell(domain.value(), b, 5, work); REQUIRE(birth.ok());
    CHECK(birth.value().traces().empty()); CHECK_EQ(work.used(), 4 * sizeof(CellTrace));
  }
  CHECK(work.released().ok());
  auto recovery = build_cell(domain.value(), b, 3, work); REQUIRE(recovery.ok()); CHECK_EQ(recovery.value().traces().size(), 4u);
}

MHGP11_TEST(refusals, 12) {
  MemoryBudget owner(MemoryBudget::kUnlimited), zero(0);
  auto domain = domain_of(Input({{0,0,0},{2,2,0},{2,0,2},{0,2,2}}), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 0, 4, 4);
  for (Order k : {Order{0}, Order{1}, Order{2}, Order{5}, Order{13}}) {
    auto r = build_cell(domain.value(), b, k, zero);
    CHECK(!r.ok() && r.outcome().reason == Reason::parameter_out_of_range);
  }
  auto none = build_cell(domain.value(), BallIdx{kNone}, 3, zero);
  CHECK(!none.ok() && none.outcome().reason == Reason::parameter_out_of_range);
  auto no_memory = build_cell(domain.value(), b, 3, zero);
  CHECK(!no_memory.ok() && no_memory.outcome().reason == Reason::memory_budget);
  auto birth = build_cell(domain.value(), b, 4, zero); REQUIRE(birth.ok()); CHECK(birth.value().traces().empty());
  FullDomain moved(std::move(domain.value()));
  auto empty = build_cell(domain.value(), b, 3, zero);
  CHECK(!empty.ok() && empty.outcome().reason == Reason::parameter_out_of_range);
  CHECK(zero.released().ok());
}

MHGP11_TEST(extreme, 40) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const u32 l = kCoordMax;
  auto domain = domain_of(Input({{0,0,0},{l,l,0},{l,0,l},{0,l,l}}), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 0, 4, 4);
  auto cell = build_cell(domain.value(), b, 3, work); REQUIRE(cell.ok());
  CHECK_EQ(cell.value().traces().size(), 4u);
  for (const auto& trace : cell.value().traces()) {
    CHECK(trace_valid(trace, domain.value().index().cloud(), 3));
    auto meb = bounded_meb(domain.value().index().cloud(), trace.part()); REQUIRE(meb.ok());
    CHECK(num::compare(meb.value().sphere().level(), domain.value().catalogue().levels()[idx(
          domain.value().catalogue().balls_data()[idx(b)].rank)]) < 0);
  }
  Input extended({{10,5,5},{9,8,5},{5,2,1},{1,5,8},{9,2,5}});
  auto second = domain_of(extended, owner); REQUIRE(second.ok());
  const auto e = select(second.value(), 0, 5, 4);
  auto triples = build_cell(second.value(), e, 3, work); REQUIRE(triples.ok());
  CHECK_EQ(triples.value().traces().size(), 10u); CHECK_EQ(triples.value().ledger().meb_calls, 0u);
  auto quads = build_cell(second.value(), e, 4, work); REQUIRE(quads.ok());
  CHECK(quads.value().traces().size() > 0 && quads.value().traces().size() < 5);
  CHECK_EQ(quads.value().ledger().meb_calls, 10u);
  Input wide({{0,5,5},{1,2,5},{1,8,5},{2,1,5},{2,9,5},{5,0,5},{5,10,5},
              {8,1,5},{8,9,5},{9,2,5},{9,8,5},{10,5,5},{5,5,0},{5,5,10}});
  auto third = domain_of(wide, owner, 1); REQUIRE(third.ok());
  const auto w = select(third.value(), 0, 14, 2);
  auto singletons = build_cell(third.value(), w, 1, work); REQUIRE(singletons.ok());
  CHECK_EQ(singletons.value().traces().size(), 14u);
  CHECK_EQ(singletons.value().ledger().trace_tests, 28u);
  CHECK_EQ(singletons.value().ledger().meb_calls, 0u);
  for (const auto& trace : singletons.value().traces()) CHECK(trace_valid(trace, third.value().index().cloud(), 1));
  std::vector<Xyz> line13;
  for (u32 i = 0; i < 13; ++i) line13.push_back({i,0,0});
  auto last = domain_of(Input(line13), owner); REQUIRE(last.ok());
  const auto l13 = select(last.value(), 11, 2, 2);
  auto full_width = build_cell(last.value(), l13, 12, work); REQUIRE(full_width.ok());
  CHECK_EQ(full_width.value().traces().size(), 2u);
  for (const auto& trace : full_width.value().traces()) CHECK(trace_valid(trace, last.value().index().cloud(), 12));
}

MHGP11_TEST(concurrency, 8) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), owner); REQUIRE(domain.ok());
  const auto b = select(domain.value(), 1, 4, 2);
  auto baseline = build_cell(domain.value(), b, 3, work); REQUIRE(baseline.ok());
  std::array<bool, 4> correct{};
  std::array<std::thread, 4> threads;
  for (u32 i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] {
    MemoryBudget local(MemoryBudget::kUnlimited);
    bool ok = true;
    for (u32 repeat = 0; repeat < 4; ++repeat) {
      auto result = build_cell(domain.value(), b, 3, local);
      ok = ok && result.ok() && same_traces(baseline.value(), result.value()) &&
           result.value().ledger() == baseline.value().ledger();
    }
    correct[i] = ok && local.released().ok();
  });
  for (auto& thread : threads) thread.join();
  for (bool ok : correct) CHECK(ok);
  CHECK_EQ(work.used(), 4 * sizeof(CellTrace)); CHECK_EQ(baseline.value().traces().size(), 4u);
}

MHGP11_TEST_MAIN()
