// Coupe fermee d'une foret immuable : union par taille, compression, top geometrique separe du representant.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::tower_detail {

class ClosedAncestorSweep {
 public:
  ClosedAncestorSweep(const ClosedAncestorSweep&) = delete;
  ClosedAncestorSweep& operator=(const ClosedAncestorSweep&) = delete;
  ClosedAncestorSweep& operator=(ClosedAncestorSweep&&) = delete;
  ClosedAncestorSweep(ClosedAncestorSweep&& other) noexcept
      : forest_(std::exchange(other.forest_, nullptr)), parent_(std::move(other.parent_)),
        size_(std::move(other.size_)), top_(std::move(other.top_)), next_(other.next_),
        level_(other.level_), advanced_(other.advanced_), limit_(other.limit_) {}

  // Foret close : un etat par noeud publie.
  static Result<ClosedAncestorSweep> make(const OrderForest& forest, MemoryBudget& budget) noexcept {
    return make(forest, budget, forest.nodes().size());
  }
  // Foret en cours de publication (ordres concurrents) : n etats, n au plus la capacite de noeuds de la foret ;
  // advance(level, work, available) ne lit alors que les noeuds deja publies.
  static Result<ClosedAncestorSweep> make(const OrderForest& forest, MemoryBudget& budget, u64 n) noexcept {
    if (n == 0 || n >= kNone || n > forest.node_capacity()) return fail(Reason::tower_invariant);
    // n<2^32, trois tableaux u32 : produit strictement inferieur a 2^36.
    MHGP11_TRY(budget.admit(3 * n * sizeof(u32)));
    ClosedAncestorSweep out(forest);
    MHGP11_TRY(out.parent_.allocate(n, budget));
    MHGP11_TRY(out.size_.allocate(n, budget));
    MHGP11_TRY(out.top_.allocate(n, budget));
    for (u32 i = 0; i < n; ++i) { out.parent_[i] = i; out.size_[i] = 1; out.top_[i] = i; }
    return out;
  }

  Outcome advance(LevelRank level, ForestLedger& work) noexcept {
    return advance(level, work, forest_ == nullptr ? 0 : forest_->nodes().size());
  }
  // Precondition de la voie concurrente : les noeuds [0,available) sont publies (acquire) et toute fusion de rang
  // <=level en fait partie. Sur une foret close, available = nodes().size() redonne exactement advance(level, work).
  Outcome advance(LevelRank level, ForestLedger& work, u64 available) noexcept {
    if (forest_ == nullptr || available > parent_.size() || available < limit_ ||
        (advanced_ && idx(level) < idx(level_))) return fail(Reason::parameter_out_of_range);
    const auto nodes = forest_->nodes_.span().first(available);
    // TOUTES les fusions de niveau egal sont actives avant la premiere requete de ce niveau.
    while (next_ < nodes.size() && idx(nodes[next_].rank) <= idx(level)) {
      const u32 node = next_;
      for (NodeIdx child : forest_->children(NodeIdx{node})) {
        if (idx(child) >= node || idx(nodes[idx(child)].rank) >= idx(nodes[node].rank))
          return fail(Reason::tower_invariant);
        auto united = unite(node, idx(child), work);
        if (!united.ok()) return united.outcome();
        top_[united.value()] = node;
      }
      MHGP11_TRY(cell_add(work.ancestor_activations, 1)); ++next_;
    }
    level_ = level; advanced_ = true; limit_ = available;
    return {};
  }

  Result<NodeIdx> query(NodeIdx seed, ForestLedger& work) noexcept {
    if (forest_ == nullptr || !advanced_ || idx(seed) >= limit_ ||
        idx(forest_->nodes_[idx(seed)].rank) > idx(level_)) return fail(Reason::parameter_out_of_range);
    auto root = find(idx(seed), work);
    if (!root.ok()) return root.outcome();
    MHGP11_TRY(cell_add(work.ancestor_queries, 1));
    return NodeIdx{top_[root.value()]};
  }

 private:
  explicit ClosedAncestorSweep(const OrderForest& forest) noexcept : forest_(&forest), next_(forest.births()) {}
  Result<u32> find(u32 start, ForestLedger& work) noexcept {
    u32 root = start;
    u64 steps = 0;  // un pas par saut des deux boucles, ajoute une fois : meme compteur, < 2n
    while (parent_[root] != root) { ++steps; root = parent_[root]; }
    while (parent_[start] != start) {
      ++steps;
      const u32 next = parent_[start]; parent_[start] = root; start = next;
    }
    MHGP11_TRY(cell_add(work.ancestor_find_steps, steps));
    return root;
  }
  Result<u32> unite(u32 a, u32 b, ForestLedger& work) noexcept {
    auto first = find(a, work), second = find(b, work);
    if (!first.ok()) return first.outcome();
    if (!second.ok()) return second.outcome();
    a = first.value(); b = second.value();
    if (a == b) return fail(Reason::tower_invariant);  // Enfants disjoints d'une vraie fusion.
    if (size_[a] < size_[b]) std::swap(a, b);
    if (size_[b] > parent_.size() - size_[a]) return fail(Reason::tower_invariant);
    MHGP11_TRY(cell_add(work.ancestor_unions, 1));
    parent_[b] = a; size_[a] += size_[b];
    return a;
  }
  const OrderForest* forest_;
  Buffer<u32> parent_, size_, top_;
  u32 next_;
  LevelRank level_{0};
  bool advanced_ = false;
  u64 limit_ = 0;  // noeuds publies lors du dernier advance : seuls ceux-la peuvent etre interroges
};

}  // namespace mhgp11::tower_detail
