// Harnais de forets : proprietaires, invariants de structure et formule des capacites retenues.
#pragma once
#include "descent_support.hpp"
#include "tower/forest.hpp"

namespace forest_test {
using namespace descent_test;
inline Result<FullTower> tower_of(const Input& input, int k, MemoryBudget& owner, MemoryBudget& work) {
  auto domain = domain_of(input, owner, k);
  if (!domain.ok()) return domain.outcome();
  return build_full(std::move(domain.value()), work);
}
inline u64 retained(const OrderForest& forest) {
  return forest.node_capacity() * sizeof(ForestNode) + forest.edge_capacity() * sizeof(NodeIdx) +
         forest.births() * sizeof(BirthEntry) + (forest.order() == 1 ? 0 : forest.node_capacity() * sizeof(NodeIdx));
}
inline bool structure(const OrderForest& f) {
  if (f.births() == 0 || f.node_capacity() != 2 * u64{f.births()} - 1 ||
      f.edge_capacity() != 2 * u64{f.births()} - 2 || f.edges().size() + 1 != f.nodes().size() ||
      idx(f.root()) >= f.nodes().size()) return false;
  std::vector<u32> parents(f.nodes().size(), 0);
  for (u32 i = 0; i < f.nodes().size(); ++i) {
    const auto& node = f.nodes()[i];
    if ((i < f.births()) != (node.child_count == 0) || (i >= f.births() && node.child_count < 2)) return false;
    auto children = f.children(NodeIdx{i});
    if (!std::is_sorted(children.begin(), children.end())) return false;
    for (NodeIdx child : children) {
      if (idx(child) >= i || ++parents[idx(child)] != 1 || f.nodes()[idx(child)].parent != NodeIdx{i} ||
          idx(f.nodes()[idx(child)].rank) >= idx(node.rank)) return false;
    }
  }
  for (u32 i = 0; i < parents.size(); ++i)
    if (parents[i] != (i == idx(f.root()) ? 0u : 1u)) return false;
  return f.nodes()[idx(f.root())].parent == NodeIdx{kNone};
}
inline bool same(const OrderForest& a, const OrderForest& b) {
  if (a.order() != b.order() || a.births() != b.births() || a.nodes().size() != b.nodes().size() ||
      a.edges().size() != b.edges().size() || a.lower().size() != b.lower().size() ||
      !std::equal(a.edges().begin(), a.edges().end(), b.edges().begin()) ||
      !std::equal(a.lower().begin(), a.lower().end(), b.lower().begin())) return false;
  for (u32 i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.rank != y.rank || x.parent != y.parent || x.child_count != y.child_count ||
        x.child_begin != y.child_begin || x.birth_key != y.birth_key) return false;
  }
  return true;
}
}  // namespace forest_test
