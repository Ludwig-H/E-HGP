// Refus avant hit et admission des census par IDs physiques, sans dependance a l'ordonnancement du Pool.
#include "memo_support.hpp"
#include "tower/census_slots.hpp"
#include "tower/population_lookup.hpp"
#include "test.hpp"
using namespace memo_test;

namespace {
Input fixture() { return Input({{0,0,0},{2,0,0},{4,0,0}}); }
struct Request { std::vector<SiteIdx> part; u32 k; bool hit; };
std::vector<Request> requests(const FullDomain& domain) {
  return {{part_of(domain, {{0,0,0}}), 1, true},
          {part_of(domain, {{0,0,0},{2,0,0}}), 2, true},
          {part_of(domain, {{0,0,0},{4,0,0}}), 2, false}};
}
}  // namespace

MHGP11_TEST(contexts, 80) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto first = domain_of(fixture(), owner, 3), second = domain_of(fixture(), owner, 3);
  REQUIRE(first.ok() && second.ok());
  const auto& domain = first.value();
  auto population = PopulationLookup::make(domain, work); REQUIRE(population.ok());
  auto local = CensusWorkspace::make(domain.index(), work);
  auto foreign = CensusWorkspace::make(second.value().index(), work);
  REQUIRE(local.ok() && foreign.ok());
  for (u64 capacity : {u64{0}, u64{8}}) {
    auto memo = DescentMemo::make(domain, capacity, work, local.value().get());
    auto other = DescentMemo::make(second.value(), capacity, work, foreign.value().get());
    REQUIRE(memo.ok() && other.ok());
    const u64 held = work.used();
    for (const auto& request : requests(domain)) {
      auto hit = population.value().descend(request.part, request.k); REQUIRE(hit.ok());
      CHECK_EQ(hit.value().has_value(), request.hit);
      // Deux domaines geometriquement identiques restent deux proprietaires distincts.
      CHECK_EQ(descend(domain, request.part, request.k, zero, foreign.value().get()).outcome().reason,
               Reason::parameter_out_of_range);
      CHECK_EQ(resolve_descent(domain, request.part, request.k, zero, nullptr,
                               foreign.value().get(), &population.value()).outcome().reason,
               Reason::parameter_out_of_range);
      CHECK_EQ(resolve_descent(domain, request.part, request.k, zero, &memo.value(),
                               foreign.value().get(), &population.value()).outcome().reason,
               Reason::parameter_out_of_range);
      CHECK_EQ(resolve_descent(domain, request.part, request.k, zero, &other.value(),
                               nullptr, &population.value()).outcome().reason,
               Reason::parameter_out_of_range);
      CHECK_EQ(work.used(), held); CHECK_EQ(zero.used(), 0u); CHECK_EQ(zero.peak(), 0u);
      auto slow = descend(domain, request.part, request.k, work, local.value().get()); REQUIRE(slow.ok());
      auto fast = resolve_descent(domain, request.part, request.k, work, &memo.value(),
                                  local.value().get(), &population.value()); REQUIRE(fast.ok());
      CHECK(answer(slow.value(), fast.value())); CHECK(valid_terminal(domain, fast.value()));
      if (!request.hit) {
        CHECK(level_is(fast.value().initial_level(), 4, 1));
        CHECK(level_is(fast.value().terminal_level(), 1, 1));
      }
    }
  }
}

MHGP11_TEST(each_step, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), zero(0);
  auto first = domain_of(fixture(), owner, 3), second = domain_of(fixture(), owner, 3);
  REQUIRE(first.ok() && second.ok());
  auto population = PopulationLookup::make(first.value(), work); REQUIRE(population.ok());
  auto local = CensusWorkspace::make(first.value().index(), work);
  auto foreign = CensusWorkspace::make(second.value().index(), work);
  REQUIRE(local.ok() && foreign.ok());
  for (const auto& request : requests(first.value())) {
    auto refused = population.value().descend_each_step(request.part, request.k, zero, foreign.value().get(), false);
    CHECK_EQ(refused.outcome().reason, Reason::parameter_out_of_range);
    auto direct = population.value().descend_each_step(request.part, request.k, work, local.value().get(), false);
    auto slow = descend(first.value(), request.part, request.k, work, local.value().get());
    REQUIRE(direct.ok() && slow.ok()); CHECK(answer(direct.value(), slow.value()));
    CHECK_EQ(direct.value().ledger().steps, request.hit ? 1u : 2u);
    if (!request.hit) {
      CHECK_EQ(population.value().descend_each_step(request.part, request.k, zero, foreign.value().get(), true)
                   .outcome().reason, Reason::parameter_out_of_range);
      auto skipped = population.value().descend_each_step(request.part, request.k, work, local.value().get(), true);
      REQUIRE(skipped.ok()); CHECK(answer(skipped.value(), slow.value()));
    }
    CHECK_EQ(zero.used(), 0u); CHECK_EQ(zero.peak(), 0u);
  }
}

MHGP11_TEST(owned_admission, 2000) {
  // Oracle : tous les sous-ensembles de workers, sans supposer que les IDs actifs commencent a zero.
  for (u32 workers = 1; workers <= 8; ++workers) for (u32 spaces = 0; spaces <= workers; ++spaces) {
    std::array<u64, 9> maximum{};
    for (u32 mask = 0; mask < (1u << workers); ++mask) {
      u32 active = 0, owned = 0;
      for (u32 id = 0; id < workers; ++id) if ((mask >> id) & 1u) {
        ++active; if (id >= spaces) ++owned;
      }
      maximum[active] = std::max(maximum[active], u64{owned});
      auto bound = owned_census_workers(workers, spaces, active); REQUIRE(bound.ok());
      CHECK(u64{owned} <= bound.value());
    }
    for (u64 tasks = 0; tasks <= u64{workers} + 2; ++tasks) {
      auto bound = owned_census_workers(workers, spaces, tasks); REQUIRE(bound.ok());
      CHECK_EQ(bound.value(), maximum[std::min<u64>(workers, tasks)]);
    }
  }
  constexpr u32 workers = 48, spaces = 4, tasks = 2, sites = 100;
  constexpr std::array<u32, 2> active_ids{30, 31};
  u64 actual_owned = 0;
  for (u32 id : active_ids) { CHECK(id < workers); if (id >= spaces) ++actual_owned; }
  auto bound = owned_census_workers(workers, spaces, tasks); REQUIRE(bound.ok());
  CHECK_EQ(bound.value(), actual_owned); CHECK_EQ(bound.value(), 2u);
  const u64 bytes = sizeof(SiteIdx) * u64{sites} * bound.value(), held = 17;
  for (u64 deficit : {u64{0}, u64{1}}) {
    MemoryBudget budget(held + bytes - deficit);
    Buffer<u8> prior; REQUIRE(prior.allocate(held, budget).ok()); prior[0] = 91;
    const auto* saved = prior.data();
    auto admitted = budget.admit(bytes);
    CHECK_EQ(admitted.reason, deficit == 0 ? Reason::none : Reason::memory_budget);
    CHECK_EQ(budget.used(), held); CHECK_EQ(budget.peak(), held);
    if (admitted.ok()) {
      Buffer<SiteIdx> a, b;
      REQUIRE(a.allocate(sites, budget).ok()); REQUIRE(b.allocate(sites, budget).ok());
      CHECK_EQ(budget.used(), held + bytes); CHECK_EQ(budget.peak(), held + bytes);
    }
    CHECK_EQ(budget.used(), held); CHECK_EQ(prior[0], 91u); CHECK(prior.data() == saved);
  }
  CHECK_EQ(owned_census_workers(0, 0, 0).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(owned_census_workers(sched::kMaxWorkers + 1, 0, 1).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(owned_census_workers(4, 5, 0).outcome().reason, Reason::parameter_out_of_range);
  auto maximum = owned_census_workers(workers, spaces, MemoryBudget::kUnlimited); REQUIRE(maximum.ok());
  CHECK_EQ(maximum.value(), workers - spaces);
}

MHGP11_TEST(ledger, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  auto domain = domain_of(fixture(), owner, 3); REQUIRE(domain.ok());
  auto population = PopulationLookup::make(domain.value(), work); REQUIRE(population.ok());
  auto memo = DescentMemo::make(domain.value(), 8, work); REQUIRE(memo.ok());
  const auto part = part_of(domain.value(), {{0,0,0},{2,0,0}});
  auto first = resolve_descent(domain.value(), part, 2, work, &memo.value()); REQUIRE(first.ok());
  auto saved = resolve_descent(domain.value(), part, 2, work, &memo.value()); REQUIRE(saved.ok());
  CHECK_EQ(first.value().ledger().steps, 1u); CHECK_EQ(saved.value().ledger().steps, 0u);
  CHECK_EQ(saved.value().ledger().memo.hits, 1u); CHECK(answer(first.value(), saved.value()));
  for (u32 repeat = 0; repeat < 2; ++repeat) {
    auto fast = resolve_descent(domain.value(), part, 2, work, &memo.value(), nullptr, &population.value());
    REQUIRE(fast.ok()); CHECK(answer(first.value(), fast.value()));
    const auto& ledger = fast.value().ledger();
    CHECK_EQ(ledger.steps, 1u); CHECK_EQ(ledger.population_hits, 1u); CHECK_EQ(ledger.memo.queries, 0u);
    CHECK_EQ(ledger.census_calls + ledger.catalogue_hits + ledger.singleton_hits, ledger.steps);
  }
}

MHGP11_TEST_MAIN()
