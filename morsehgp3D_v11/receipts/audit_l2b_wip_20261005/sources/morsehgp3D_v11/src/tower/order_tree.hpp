// Arbre d'ordre K seul et rattachement des boules d'evenement (tranche S3 de la sortie parametree) : la foret
// d'ordre K de FULL, construite seule par la voie non concurrente de build_full, sans autre ordre ni verticale
// (build_order), ou tiree de FULL avec le journal (build_order_full, livraison L2b), et le
// rattachement exact de W_K = {b dans Cat_K : p+q-1 <= K <= p+m} sur ses noeuds (lemmes A a E de la specification,
// MATHEMATIQUES.md T1 a T5). Expose par le parapluie tower/tower.hpp.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::tower_detail {

// Role d'une boule de W_K, decide par les rangs (lemme B) : naissance (sa propre naissance, meme rang), fusion (le
// noeud de rattachement est une fusion creee a ce rang) ou interne (continuation : rang du noeud < rang de la boule
// < rang de son parent, ou racine).
enum class BallRole : u8 { birth = 0, merge = 1, internal = 2 };

// W_K en BallIdx croissants (donc rangs croissants), une entree par boule ; une naissance est forte (p+q <= K), une
// boule faible (K = p+q-1) est toujours une cellule.
//   node          : att(b), noeud vivant a la coupe FERMEE lambda_b qui contient toutes les K-parties de P_b (lemme A),
//                   lu apres tout le plateau
//   strict_traces : traces strictes I union A resolues par le constructeur (journal) ; 0 pour une naissance ; au-dela
//                   de UINT32_MAX, build_order refuse (tower_capacity) au lieu de tronquer
//   components    : |ant(b)|, noeuds de la coupe OUVERTE (rang r_b - 1) qui contiennent une K-partie stricte de P_b,
//                   dedupliques ; 0 pour une naissance, 1 pour une boule interne (lemme C)
//   prior         : ant(b) croissants, publie pour le role fusion seulement (prior_offsets : size()+1 decalages)
// Les unions DSU effectuees par une cellule dependent de l'ordre de traitement : elles ne sont jamais publiees.
class WindowAttachment {
 public:
  WindowAttachment(const WindowAttachment&) = delete;
  WindowAttachment& operator=(const WindowAttachment&) = delete;
  WindowAttachment& operator=(WindowAttachment&&) = delete;
  WindowAttachment(WindowAttachment&& other) noexcept
      : balls_(std::move(other.balls_)), node_(std::move(other.node_)), role_(std::move(other.role_)),
        strict_(std::move(other.strict_)), components_(std::move(other.components_)),
        prior_offsets_(std::move(other.prior_offsets_)), prior_(std::move(other.prior_)) {}
  u32 size() const noexcept { return static_cast<u32>(balls_.size()); }
  std::span<const BallIdx> balls() const noexcept { return balls_.span(); }
  std::span<const NodeIdx> node() const noexcept { return node_.span(); }
  std::span<const BallRole> role() const noexcept { return role_.span(); }
  std::span<const u32> strict_traces() const noexcept { return strict_.span(); }
  std::span<const u32> components() const noexcept { return components_.span(); }
  std::span<const u64> prior_offsets() const noexcept { return prior_offsets_.span(); }
  std::span<const NodeIdx> prior() const noexcept { return prior_.span(); }

 private:
  friend struct AttachmentBuilder;
  WindowAttachment() = default;
  Buffer<BallIdx> balls_;
  Buffer<NodeIdx> node_;
  Buffer<BallRole> role_;
  Buffer<u32> strict_, components_;
  Buffer<u64> prior_offsets_;
  Buffer<NodeIdx> prior_;
};

// Registres du journal des graines, diagnostic de porte (L2b : egaux par les deux voies) : cellules, graines et
// empreinte FNV-1a 64 de la suite (BallIdx, nombre de graines, graines) de chaque cellule, prise a la fermeture du
// journal, avant le balayage qui reecrit les graines en place.
struct SeedLogRegisters {
  u64 cells = 0, seeds = 0, digest = 0;
  friend bool operator==(const SeedLogRegisters&, const SeedLogRegisters&) = default;
};

// Possede le domaine, la foret d'ordre k et son rattachement ; deplacement seulement.
class OrderTree {
 public:
  OrderTree(const OrderTree&) = delete;
  OrderTree& operator=(const OrderTree&) = delete;
  OrderTree& operator=(OrderTree&&) = delete;
  OrderTree(OrderTree&& other) noexcept
      : domain_(std::move(other.domain_)), forest_(std::move(other.forest_)),
        attachment_(std::move(other.attachment_)) {}
  const FullDomain& domain() const noexcept { return domain_; }
  Order order() const noexcept { return forest_.order(); }
  const OrderForest& forest() const noexcept { return forest_; }
  const WindowAttachment& attachment() const noexcept { return attachment_; }

 private:
  friend Result<OrderTree> build_order(FullDomain&&, Order, MemoryBudget&, FullParams, sched::Pool*, OrderTimings*,
                                       u64*, SeedLogRegisters*) noexcept;
  friend Result<OrderTree> build_order_full(FullDomain&&, Order, MemoryBudget&, FullParams, sched::Pool*,
                                            FullTimings*, u64*, SeedLogRegisters*) noexcept;
  OrderTree(FullDomain&& domain, OrderForest&& forest, WindowAttachment&& attachment) noexcept
      : domain_(std::move(domain)), forest_(std::move(forest)), attachment_(std::move(attachment)) {}
  FullDomain domain_;
  OrderForest forest_;
  WindowAttachment attachment_;
};

// Ordre k seul, 1 <= k <= min(kmax, n), sans autre ordre ni verticale. Meme mise en place que la boucle non
// concurrente de build_full (table de populations non liee, espaces census, memo, ForestParallel), foret identique a
// build_full(...).order(k) sur le meme domaine (porte I10). FullParams comme build_full ; concurrent_orders,
// parallel_verticals et reuse_regular_verticals refuses (parameter_out_of_range : sans objet pour un ordre seul).
// Journal des graines admis avant le parcours, puis balayage du lemme D et controles I1 a I4 apres finish().
// Refus : parameter_out_of_range, memory_budget, tower_capacity, tower_invariant ; domaine intact et reservations de
// l'appel rendues sur refus. Diagnostics publies au succes seulement : timings (classification, naissances,
// plateaux, lots), attach_ns (balayage et controles du rattachement) et registres du journal.
[[nodiscard]] Result<OrderTree> build_order(FullDomain&&, Order k, MemoryBudget&, FullParams = {},
                                           sched::Pool* = nullptr, OrderTimings* = nullptr,
                                           u64* attach_ns = nullptr, SeedLogRegisters* registers = nullptr) noexcept;

// Ordre k tire de FULL (livraison L2b, docs/SORTIES.md paragraphe 11) : build_full sur le domaine (ordres 1..kmax,
// verticales comprises, FullParams quelconques, ordres concurrents compris), avec le journal des graines pose sur le
// seul constructeur de l'ordre k (voie non concurrente : le pilote ; voie concurrente : la tache de publication de
// l'ordre k, par etages ou en pipeline). Apres la fin : les autres forets et les verticales de l'ordre k sont rendues
// au budget, puis le balayage du lemme D et les controles I1 a I4 (attach_window, comme build_order). Meme foret
// (porte I10), meme rattachement et memes registres du journal que build_order sur le meme domaine ; le registre de
// la foret garde en plus le travail vertical de FULL (diagnostic). 1 <= k <= kmax <= n. Capacites du journal admises
// avant toute allocation par le majorant de build_order (SeedLog::make), apres les refus de parametres de build_full.
// Refus : parameter_out_of_range, memory_budget, tower_capacity, tower_invariant ; domaine intact et reservations de
// l'appel rendues sur refus. Diagnostics publies au succes seulement : timings (ceux de build_full), attach_ns et
// registres du journal.
[[nodiscard]] Result<OrderTree> build_order_full(FullDomain&&, Order k, MemoryBudget&, FullParams = {},
                                                sched::Pool* = nullptr, FullTimings* = nullptr,
                                                u64* attach_ns = nullptr,
                                                SeedLogRegisters* registers = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
