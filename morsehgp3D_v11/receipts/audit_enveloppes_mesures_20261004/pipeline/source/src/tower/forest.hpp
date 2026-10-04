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
  u64 vertical_reuses = 0;
  // ancestor_hops garde le sens de la marche de reference ; le balayage ne l'incremente pas.
  u64 ancestor_queries = 0, ancestor_activations = 0, ancestor_unions = 0, ancestor_find_steps = 0;
  ClassificationLedger classification;
  CellLedger cells;
  DescentLedger descent;
  friend bool operator==(const ForestLedger&, const ForestLedger&) = default;
};
struct OrderTimings {
  u64 classify_ns = 0, births_ns = 0, plateaus_ns = 0, verticals_ns = 0;
  u64 regular_batches = 0, regular_cells = 0, regular_traces = 0, extended_cells = 0, max_regular_batch = 0;
  u64 regular_dispatch_ns = 0, regular_task_sum_ns = 0, regular_task_max_ns = 0;
  u64 regular_publish_ns = 0, extended_ns = 0;
  u64 vertical_batches = 0, vertical_resolutions = 0, max_vertical_batch = 0;
  u64 vertical_dispatch_ns = 0, vertical_task_sum_ns = 0, vertical_task_max_ns = 0, vertical_sweep_ns = 0;
  // Pipeline, diagnostic seulement (T0 de l'audit du 4 octobre) : debut depuis l'origine du pipeline, temps CPU du
  // fil et attente bloquee (futex) du publieur de cet ordre et du balayage dont il est l'ordre haut.
  u64 publish_start_ns = 0, publish_cpu_ns = 0, publish_wait_ns = 0;
  u64 vertical_start_ns = 0, vertical_cpu_ns = 0, vertical_wait_ns = 0;
  friend bool operator==(const OrderTimings&, const OrderTimings&) = default;
};
struct FullParams {
  u64 memo_capacity = 0;
  u32 regular_batch_capacity = 0, descent_lanes = 1;
  u64 lane_memo_capacity = 0;
  bool parallel_verticals = false;
  bool reuse_census_workspace = false;
  bool dense_birth_lookup = false;
  bool reuse_regular_verticals = false;
  bool population_lookup = false;  // table I union U -> boule consultee avant toute descente (opt-in)
  bool concurrent_orders = false;  // etages sur tous les ordres (forest_concurrent.cpp) ; exige Q>0
};
struct FullTimings {
  u64 memo_capacity = 0, memo_slot_bytes = 0, memo_reserved_bytes = 0;
  std::array<OrderTimings, kMaxMebSites> orders{};
  u64 regular_batch_capacity = 0, descent_lanes = 0, lane_memo_capacity = 0, lane_memo_reserved_bytes = 0;
  bool parallel_verticals = false;
  u64 census_workspaces = 0, census_workspace_reserved_bytes = 0;
  bool reuse_regular_verticals = false;
  u64 regular_vertical_reserved_bytes = 0;
  bool population_lookup = false;
  u64 population_lookup_entries = 0, population_lookup_reserved_bytes = 0;
  bool concurrent_orders = false;
  u64 classify_phase_ns = 0, birth_phase_ns = 0, regular_phase_ns = 0, publish_phase_ns = 0, vertical_phase_ns = 0;
  // Pipeline (ordres concurrents) : taches de resolution, 0 pour la voie par etages ; phases = fins depuis le debut.
  u64 pipeline_lanes = 0;
  // Voies de resolution (diagnostic) : dernier depart, premiere fin, somme des temps CPU des fils.
  u64 lanes_last_start_ns = 0, lanes_first_finish_ns = 0, lanes_cpu_ns = 0;
  friend bool operator==(const FullTimings&, const FullTimings&) = default;
};

struct ForestBuilder;
class ForestParallel;
struct VerticalBuilder;
class ClosedAncestorSweep;
class OrderForest {
 public:
  OrderForest(const OrderForest&) = delete;
  OrderForest& operator=(const OrderForest&) = delete;
  OrderForest& operator=(OrderForest&&) = delete;
  OrderForest(OrderForest&& other) noexcept
      : nodes_(std::move(other.nodes_)), children_(std::move(other.children_)), lookup_(std::move(other.lookup_)),
        dense_(std::move(other.dense_)), lower_(std::move(other.lower_)), order_(std::exchange(other.order_, 0)),
        births_(std::exchange(other.births_, 0)), count_(std::exchange(other.count_, 0)),
        edges_(std::exchange(other.edges_, 0)), root_(std::exchange(other.root_, NodeIdx{kNone})),
        ledger_(std::exchange(other.ledger_, {})) {}
  Order order() const noexcept { return order_; }
  u32 births() const noexcept { return births_; }
  NodeIdx root() const noexcept { return root_; }
  std::span<const ForestNode> nodes() const noexcept { return nodes_.span().first(count_); }
  // Naissances seules : fixes des la fin de births(), lisibles pendant une publication concurrente (count_ change).
  std::span<const ForestNode> birth_nodes() const noexcept { return nodes_.span().first(births_); }
  std::span<const NodeIdx> edges() const noexcept { return children_.span().first(edges_); }
  std::span<const NodeIdx> children(NodeIdx node) const noexcept {
    const auto& data = nodes_[idx(node)];
    return children_.span().subspan(data.child_begin, data.child_count);
  }
  std::span<const NodeIdx> lower() const noexcept { return lower_.empty() ? lower_.span() : lower_.span().first(count_); }
  u64 node_capacity() const noexcept { return nodes_.size(); }
  u64 edge_capacity() const noexcept { return children_.size(); }
  bool dense_birth_lookup() const noexcept { return !dense_.empty(); }
  u64 lookup_reserved_bytes() const noexcept {
    return lookup_.size() * sizeof(BirthEntry) + dense_.size() * sizeof(NodeIdx);
  }
  const ForestLedger& ledger() const noexcept { return ledger_; }
  // Domaine de la graine ferme : meme ordre et un identifiant dont la naissance existe dans cet ordre.
  std::optional<NodeIdx> birth_node(const BirthSeed&) const noexcept;
  // Le rang appartient aux niveaux du meme FullDomain. Marche des parents, coupe FERMEE (<=).
  Result<NodeIdx> ancestor_closed(NodeIdx, LevelRank, u64& hops) const noexcept;

 private:
  friend struct ForestBuilder;
  friend struct VerticalBuilder;
  friend class ForestParallel;
  friend class ClosedAncestorSweep;  // lit les noeuds publies au-dela de count_ (ordres concurrents)
  OrderForest() = default;
  Buffer<ForestNode> nodes_;
  Buffer<NodeIdx> children_;
  Buffer<BirthEntry> lookup_;
  Buffer<NodeIdx> dense_;
  Buffer<NodeIdx> lower_;
  Order order_ = 0;
  u32 births_ = 0, count_ = 0;
  u64 edges_ = 0;
  NodeIdx root_{kNone};
  ForestLedger ledger_;
};

// Domaine emprunte stable ; k=1..min(K,n), sinon parameter_out_of_range AVANT tout travail/allocation.
// Capacites retenues 2b-1 noeuds, 2b-2 enfants ; lookup sparse8b ou dense4n (K1)/4M (K>1).
// Dense opt-in : meme application partielle cle->naissance, remplie apres l'ordre canonique.
// Classe toutes les cellules ; chaque plateau touche ses anciennes composantes. Memo prive facultatif.
[[nodiscard]] Result<OrderForest> build_forest(const FullDomain&, u32 k, MemoryBudget&,
                                             OrderTimings* = nullptr, DescentMemo* = nullptr,
                                             ForestParallel* = nullptr, bool dense_birth_lookup = false) noexcept;

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
  friend Result<FullTower> build_full(FullDomain&&, MemoryBudget&, FullTimings*, FullParams, sched::Pool*) noexcept;
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
// Option reguliere Q>0 : Pool obligatoire, Q<=4096 et 1<=lanes<=256. Q borne le tampon, jamais le parcours.
// Verticales paralleles opt-in : exigent aussi Q>0 ; seul le calcul des graines est distribue, jamais le DSU.
// Census reutilise opt-in : C=1 si Q=0, sinon min(W,lanes,Q), exactement 4*n*C octets retenus durant FULL.
// Memos de lanes et workspaces physiques sont distincts ; tous les emprunts finissent avant transfert du domaine.
// Reemploi regulier opt-in : une table temporaire4M pour K>1, graine basse puis coupe fermee a la naissance.
[[nodiscard]] Result<FullTower> build_full(FullDomain&&, MemoryBudget&, FullTimings* = nullptr,
                                         FullParams = {}, sched::Pool* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
