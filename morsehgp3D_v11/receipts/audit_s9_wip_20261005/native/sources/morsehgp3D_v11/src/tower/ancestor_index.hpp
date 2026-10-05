// Index d'ancetres d'une foret d'ordre K (tranche S9 de la sortie parametree, hierarchie de points) : pointeurs de
// saut a un mot plus une profondeur par noeud (construction de Myers, « skew-binary » : le saut d'un noeud ne depend
// que de sa profondeur), pour le plus petit ancetre commun et la recherche du plus haut ancetre qui satisfait un
// predicat monotone le long de la remontee, chacun en O(log N) sans table de sauts binaires. Remplace la table
// lifting de bench/points_hierarchy.py (Order.lifting, lca, ancestor_at) : memes reponses, pas les memes structures.
// Expose par le parapluie tower/tower.hpp.
#pragma once

#include "tower/forest.hpp"

namespace mhgp11::tower_detail {

class AncestorIndex {
 public:
  AncestorIndex(const AncestorIndex&) = delete;
  AncestorIndex& operator=(const AncestorIndex&) = delete;
  AncestorIndex& operator=(AncestorIndex&&) = delete;
  AncestorIndex(AncestorIndex&& other) noexcept
      : nodes_(other.nodes_), jump_(std::move(other.jump_)), depth_(std::move(other.depth_)) {}

  // Octets reserves par build pour N noeuds : formule d'admission.
  static constexpr u64 bytes(u64 nodes) noexcept { return 2 * sizeof(u32) * nodes; }
  // Index de `forest`, qui doit lui survivre. Exige une seule racine, le dernier noeud, et un parent de numero plus
  // grand que son enfant (numerotation canonique : naissances, puis fusions par niveau) ; sinon tower_invariant.
  // Refus : memory_budget.
  [[nodiscard]] static Result<AncestorIndex> build(const OrderForest& forest, MemoryBudget& budget) noexcept;

  u32 depth(NodeIdx v) const noexcept { return depth_[idx(v)]; }
  NodeIdx parent(NodeIdx v) const noexcept { return nodes_[idx(v)].parent; }
  // Plus petit ancetre commun (soi compris) ; les noeuds sont valides (precondition).
  NodeIdx lca(NodeIdx a, NodeIdx b) const noexcept;
  // Ancetre de profondeur d <= depth(v).
  NodeIdx at_depth(NodeIdx v, u32 d) const noexcept;
  // Plus haut ancetre x de `start` (soi compris) tel que keep(x) soit vrai, quand keep(start) est vrai et que keep est
  // monotone le long de la remontee (vrai sur un prefixe du chemin vers la racine). keep(NodeIdx, bool&) rend un
  // Outcome ; son premier refus est rendu tel quel.
  template <class Keep>
  [[nodiscard]] Outcome highest(NodeIdx start, Keep&& keep, NodeIdx& out) const noexcept {
    NodeIdx x = start;
    for (;;) {
      const NodeIdx up = nodes_[idx(x)].parent;
      if (up == NodeIdx{kNone}) break;
      const NodeIdx far{jump_[idx(x)]};
      bool ok = false;
      if (far != up) {
        MHGP11_TRY(keep(far, ok));
        if (ok) {
          x = far;
          continue;
        }
      }
      MHGP11_TRY(keep(up, ok));
      if (!ok) break;
      x = up;
    }
    out = x;
    return {};
  }

 private:
  AncestorIndex() = default;
  std::span<const ForestNode> nodes_;
  Buffer<u32> jump_, depth_;  // jump_ de la racine : elle-meme
};

}  // namespace mhgp11::tower_detail
