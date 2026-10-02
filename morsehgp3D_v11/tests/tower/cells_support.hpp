// Aides du harnais cells, jamais incluses par le produit.
#pragma once
#include "test_support.hpp"
#include "tower/cells.hpp"

namespace cells_test {
using namespace mhgp11;
using namespace tower_test;
using namespace mhgp11::tower_detail;
inline Result<FullDomain> domain_of(const Input& input, MemoryBudget& budget, int kmax = 12) {
  auto cloud = input.prepare(budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, budget);
  if (!index.ok()) return index.outcome();
  CatalogueParams params;
  params.kmax = kmax;
  return prepare_full_domain(std::move(index.value()), params, budget);
}
inline BallIdx select(const FullDomain& domain, u32 p, u32 m, u8 q) {
  const auto balls = domain.catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b)
    if (balls[b].p == p && balls[b].m == m && balls[b].qmin == q) return BallIdx{b};
  throw std::runtime_error("boule de cellule absente");
}
inline bool trace_valid(const CellTrace& trace, const Cloud& cloud, Order order) {
  if (trace.arity != order) return false;
  for (u32 i = 0; i < kMaxMebSites; ++i) {
    if (i >= order) { if (idx(trace.sites[i]) != kNone) return false; }
    else if (idx(trace.sites[i]) >= cloud.sites() || (i != 0 && idx(trace.sites[i-1]) >= idx(trace.sites[i]))) return false;
  }
  return true;
}
inline bool same_traces(const LocalCell& a, const LocalCell& b) {
  if (a.traces().size() != b.traces().size()) return false;
  for (std::size_t i = 0; i < a.traces().size(); ++i)
    if (a.traces()[i].sites != b.traces()[i].sites || a.traces()[i].arity != b.traces()[i].arity) return false;
  return true;
}
}  // namespace cells_test
