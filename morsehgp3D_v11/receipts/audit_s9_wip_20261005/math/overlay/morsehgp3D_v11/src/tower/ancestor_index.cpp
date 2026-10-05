// Construction et requetes de l'index d'ancetres (tranche S9) : pointeurs de saut de Myers, racine d'abord.
#include "tower/ancestor_index.hpp"

namespace mhgp11::tower_detail {

Result<AncestorIndex> AncestorIndex::build(const OrderForest& forest, MemoryBudget& budget) noexcept {
  const auto nodes = forest.nodes();
  const u64 n = nodes.size();
  if (n == 0 || n >= kNone || idx(forest.root()) != n - 1 || nodes[n - 1].parent != NodeIdx{kNone})
    return fail(Reason::tower_invariant);
  AncestorIndex out;
  MHGP11_TRY(budget.admit(bytes(n)));
  MHGP11_TRY(out.jump_.allocate(n, budget));
  MHGP11_TRY(out.depth_.allocate(n, budget));
  out.nodes_ = nodes;
  // Parents avant enfants : la numerotation canonique donne a tout parent un numero plus grand.
  for (u64 j = n; j-- > 0;) {
    const u32 v = static_cast<u32>(j);
    const NodeIdx up = nodes[v].parent;
    if (up == NodeIdx{kNone}) {
      if (v != n - 1) return fail(Reason::tower_invariant);
      out.jump_[v] = v;
      out.depth_[v] = 0;
      continue;
    }
    const u32 p = idx(up);
    if (p <= v || p >= n) return fail(Reason::tower_invariant);
    const u32 jp = out.jump_[p], jjp = out.jump_[jp];
    out.depth_[v] = out.depth_[p] + 1;
    // Saut de longueur 2 l + 1 si les deux sauts au-dessus du parent ont la meme longueur l, sinon le parent.
    out.jump_[v] = out.depth_[p] - out.depth_[jp] == out.depth_[jp] - out.depth_[jjp] ? jjp : p;
  }
  return out;
}

NodeIdx AncestorIndex::at_depth(NodeIdx v, u32 d) const noexcept {
  u32 x = idx(v);
  while (depth_[x] > d) x = depth_[jump_[x]] < d ? idx(nodes_[x].parent) : jump_[x];
  return NodeIdx{x};
}

NodeIdx AncestorIndex::lca(NodeIdx a, NodeIdx b) const noexcept {
  if (depth_[idx(a)] < depth_[idx(b)]) std::swap(a, b);
  u32 x = idx(at_depth(a, depth_[idx(b)])), y = idx(b);
  // Meme profondeur : les sauts ont la meme longueur des deux cotes.
  while (x != y) {
    if (jump_[x] == jump_[y]) {
      x = idx(nodes_[x].parent);
      y = idx(nodes_[y].parent);
    } else {
      x = jump_[x];
      y = jump_[y];
    }
  }
  return NodeIdx{x};
}

}  // namespace mhgp11::tower_detail
