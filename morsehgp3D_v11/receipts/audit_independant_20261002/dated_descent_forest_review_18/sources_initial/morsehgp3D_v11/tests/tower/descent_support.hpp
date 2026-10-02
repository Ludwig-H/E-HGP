// Fixtures de descente et controles locaux du harnais, sans choix de terminal impose par un autre moteur.
#pragma once
#include "cells_support.hpp"
#include "tower/descent.hpp"
#include "tower/locate.hpp"

namespace descent_test {
using namespace cells_test;
inline std::vector<SiteIdx> part_of(const FullDomain& domain, std::initializer_list<Xyz> points) {
  std::vector<SiteIdx> part;
  for (auto point : points) part.push_back(site(domain.index().cloud(), point));
  return part;
}
inline bool level_is(const num::Level& level, i64 n, i64 d) {
  auto expected = num::Level::make(num::to_wide(n), num::to_wide(d));
  return expected.ok() && num::compare(level, expected.value()) == 0;
}
inline bool strict_step(const FullDomain& domain, const DescentStep& step, u32 k) {
  if (step.seed() || !trace_valid(step.next(), domain.index().cloud(), static_cast<Order>(k))) return false;
  auto next = bounded_meb(domain.index().cloud(), step.next().part());
  return next.ok() && num::compare(next.value().sphere().level(), step.level()) < 0;
}
inline bool same(const DescentResult& a, const DescentResult& b) {
  return a.seed() == b.seed() && num::compare(a.initial_level(), b.initial_level()) == 0 &&
         num::compare(a.terminal_level(), b.terminal_level()) == 0 && a.ledger() == b.ledger();
}
inline bool valid_terminal(const FullDomain& domain, const DescentResult& result) {
  const auto seed = result.seed();
  if (seed.site()) return !seed.ball() && seed.order() == 1 && idx(*seed.site()) < domain.index().cloud().sites() &&
                          level_is(result.terminal_level(), 0, 1);
  if (!seed.ball() || idx(*seed.ball()) >= domain.catalogue().balls()) return false;
  const auto b = *seed.ball();
  const auto& data = domain.catalogue().balls_data()[idx(b)];
  if (num::compare(result.terminal_level(), domain.catalogue().levels()[idx(data.rank)]) != 0) return false;
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cell = build_cell(domain, b, seed.order(), budget);
  return cell.ok() && cell.value().kind() == CellKind::birth;
}
inline bool replay(const FullDomain& domain, std::span<const SiteIdx> initial, u32 k,
                   const DescentResult& result) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  std::span<const SiteIdx> current = initial;
  CellTrace next;
  std::optional<num::Level> previous;
  DescentLedger sum;
  for (u64 i = 0; i < result.ledger().steps; ++i) {
    auto step = descent_step(domain, current, k, budget);
    if (!step.ok() || budget.used() != 0) return false;
    if (previous && num::compare(step.value().level(), *previous) >= 0) return false;
    if (!add_descent(sum, step.value().ledger()).ok()) return false;
    if (step.value().seed()) return i + 1 == result.ledger().steps && sum == result.ledger() &&
                                    *step.value().seed() == result.seed() && valid_terminal(domain, result);
    if (!strict_step(domain, step.value(), k)) return false;
    previous = step.value().level(); next = step.value().next(); current = next.part();
  }
  return false;
}
}  // namespace descent_test
