// Forets FULL privees : naissances canoniques, multifusions de plateaux, parents et verticales fermees.
#pragma once
#include "tower/descent_memo.hpp"

namespace mhgp11::tower_detail {

struct ForestNode {
  LevelRank rank;
  NodeIdx parent;
  u64 child_begin;
  u32 child_count;
  u32 birth_key;  // SiteIdx si ordre1, BallIdx sinon ; kNone pour une fusion.
};
struct BirthEntry { u32 key; NodeIdx node; };
// Bornes scalaires testables sans fabriquer un nuage gigantesque : noeuds/aretes retenus, sentinelle exclue.
[[nodiscard]] Result<std::array<u64, 2>> forest_capacities(u64 births) noexcept;
struct ForestLedger {
  u64 classified_cells = 0, replayed_cells = 0, plateaus = 0, trace_resolutions = 0;
  u64 unions = 0, touched_components = 0, continuations = 0, center_comparisons = 0;
  u64 birth_presentations = 0, ancestor_hops = 0, vertical_descents = 0, vertical_checks = 0;
  // ancestor_hops garde le sens de la marche de reference ; le balayage ne l'incremente pas.
  u64 ancestor_queries = 0, ancestor_activations = 0, ancestor_unions = 0, ancestor_find_steps = 0;
  ClassificationLedger classification;
  CellLedger cells;
  DescentLedger descent;
  friend bool operator==(const ForestLedger&, const ForestLedger&) = default;
};
struct OrderTimings {
  u64 classify_ns = 0, births_ns = 0, plateaus_ns = 0, verticals_ns = 0;
  friend bool operator==(const OrderTimings&, const OrderTimings&) = default;
};
struct FullParams { u64 memo_capacity = 0; };
struct FullTimings {
  u64 memo_capacity = 0, memo_slot_bytes = 0, memo_reserved_bytes = 0;
  std::array<OrderTimings, kMaxMebSites> orders{};
  friend bool operator==(const FullTimings&, const FullTimings&) = default;
};

struct ForestBuilder;
struct VerticalBuilder;
class OrderForest {
 public:
  OrderForest(const OrderForest&) = delete;
  OrderForest& operator=(const OrderForest&) = delete;
  OrderForest& operator=(OrderForest&&) = delete;
  OrderForest(OrderForest&& other) noexcept
      : nodes_(std::move(other.nodes_)), children_(std::move(other.children_)), lookup_(std::move(other.lookup_)),
        lower_(std::move(other.lower_)), order_(std::exchange(other.order_, 0)),
        births_(std::exchange(other.births_, 0)), count_(std::exchange(other.count_, 0)),
        edges_(std::exchange(other.edges_, 0)), root_(std::exchange(other.root_, NodeIdx{kNone})),
        ledger_(std::exchange(other.ledger_, {})) {}
  Order order() const noexcept { return order_; }
  u32 births() const noexcept { return births_; }
  NodeIdx root() const noexcept { return root_; }
  std::span<const ForestNode> nodes() const noexcept { return nodes_.span().first(count_); }
  std::span<const NodeIdx> edges() const noexcept { return children_.span().first(edges_); }
  std::span<const NodeIdx> children(NodeIdx node) const noexcept {
    const auto& data = nodes_[idx(node)];
    return children_.span().subspan(data.child_begin, data.child_count);
  }
  std::span<const NodeIdx> lower() const noexcept { return lower_.empty() ? lower_.span() : lower_.span().first(count_); }
  u64 node_capacity() const noexcept { return nodes_.size(); }
  u64 edge_capacity() const noexcept { return children_.size(); }
  const ForestLedger& ledger() const noexcept { return ledger_; }
  // Domaine de la graine ferme : meme ordre et un identifiant dont la naissance existe dans cet ordre.
  std::optional<NodeIdx> birth_node(const BirthSeed&) const noexcept;
  // Le rang appartient aux niveaux du meme FullDomain. Marche des parents, coupe FERMEE (<=).
  Result<NodeIdx> ancestor_closed(NodeIdx, LevelRank, u64& hops) const noexcept;

 private:
  friend struct ForestBuilder;
  friend struct VerticalBuilder;
  OrderForest() = default;
  Buffer<ForestNode> nodes_;
  Buffer<NodeIdx> children_;
  Buffer<BirthEntry> lookup_;
  Buffer<NodeIdx> lower_;
  Order order_ = 0;
  u32 births_ = 0, count_ = 0;
  u64 edges_ = 0;
  NodeIdx root_{kNone};
  ForestLedger ledger_;
};

// Domaine emprunte stable ; k=1..min(K,n), sinon parameter_out_of_range AVANT tout travail/allocation.
// Capacites retenues 2b-1 noeuds, 2b-2 enfants et b entrees de lookup, mais vues logiques seulement.
// Classe toutes les cellules ; chaque plateau touche ses anciennes composantes. Memo prive facultatif.
[[nodiscard]] Result<OrderForest> build_forest(const FullDomain&, u32 k, MemoryBudget&,
                                             OrderTimings* = nullptr, DescentMemo* = nullptr) noexcept;

class FullTower {
 public:
  FullTower(const FullTower&) = delete;
  FullTower& operator=(const FullTower&) = delete;
  FullTower& operator=(FullTower&&) = delete;
  FullTower(FullTower&& other) noexcept
      : domain_(std::move(other.domain_)), orders_(std::move(other.orders_)), kmax_(std::exchange(other.kmax_, 0)) {}
  const FullDomain& domain() const noexcept { return domain_; }
  Order kmax() const noexcept { return kmax_; }
  // Precondition : 1<=k<=kmax(). Aucune foret ne peut etre ajoutee depuis un autre domaine.
  const OrderForest& order(Order k) const noexcept { return *orders_[k - 1]; }

 private:
  friend Result<FullTower> build_full(FullDomain&&, MemoryBudget&, FullTimings*, FullParams) noexcept;
  FullTower(FullDomain&& domain, std::array<std::optional<OrderForest>, kMaxMebSites>&& orders, Order kmax) noexcept
      : domain_(std::move(domain)), orders_(std::move(orders)), kmax_(kmax) {}
  FullDomain domain_;
  std::array<std::optional<OrderForest>, kMaxMebSites> orders_;
  Order kmax_;
};

// Construit toutes les forets et verticales K1..K AVANT transfert final du domaine. K>n refuse explicitement.
// Un refus rend toutes les reservations de cet appel ; domaine et anciens resultats restent entiers.
// Budgets d'origine et de forets survivent au resultat. Aucune attache de points/core/cover n'est fabriquee ici.
// Diagnostics facultatifs : publies ensemble seulement au succes, cases k>=K remises a zero.
[[nodiscard]] Result<FullTower> build_full(FullDomain&&, MemoryBudget&, FullTimings* = nullptr, FullParams = {}) noexcept;

}  // namespace mhgp11::tower_detail
