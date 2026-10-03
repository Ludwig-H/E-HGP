// Partie verticale historique : k-1 premiers sites de I union U, puis graine BASSE non elevee.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::tower_detail {
inline Result<NodeIdx> vertical_seed(const FullDomain& domain, const OrderForest& lower, Order order,
                                    const ForestNode& node, MemoryBudget& budget, DescentMemo* memo,
                                    DescentLedger& paid) noexcept {
  if (order < 2 || order > kMaxMebSites || lower.order() + 1 != order ||
      node.birth_key >= domain.catalogue().balls() || idx(node.rank) >= domain.catalogue().levels().size())
    return fail(Reason::tower_invariant);
  const BallIdx ball{node.birth_key};
  const auto inner = domain.catalogue().interior(ball), shell = domain.catalogue().shell(ball);
  std::array<SiteIdx, kMaxMebSites> part{};
  const u32 size = order - 1;
  u32 i = 0, j = 0;
  for (u32 n = 0; n < size; ++n) {
    if (i == inner.size() && j == shell.size()) return fail(Reason::tower_invariant);
    if (j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]))) part[n] = inner[i++];
    else part[n] = shell[j++];
  }
  auto down = resolve_descent(domain, {part.data(), size}, size, budget, memo);
  if (!down.ok()) return down.outcome();
  const auto& level = domain.catalogue().levels()[idx(node.rank)];
  if (num::compare(down.value().initial_level(), level) > 0) return fail(Reason::tower_invariant);
  const auto seed = lower.birth_node(down.value().seed());
  if (!seed || idx(lower.nodes()[idx(*seed)].rank) > idx(node.rank)) return fail(Reason::tower_invariant);
  MHGP11_TRY(add_descent(paid, down.value().ledger()));
  return *seed;
}
}  // namespace mhgp11::tower_detail
