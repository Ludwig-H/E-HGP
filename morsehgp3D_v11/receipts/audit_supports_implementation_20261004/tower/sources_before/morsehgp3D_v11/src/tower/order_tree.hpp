// Arbre d'ordre K seul et rattachement des boules d'evenement (tranche S3 de la sortie parametree) : la foret
// d'ordre K de FULL, construite seule par la voie non concurrente de build_full, sans autre ordre ni verticale, et le
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

// W_K en BallIdx croissants (donc rangs croissants), une entree par boule.
//   node          : att(b), noeud vivant a la coupe FERMEE lambda_b qui contient toutes les K-parties de P_b (lemme A)
//   strict_traces : traces strictes I union A resolues par le constructeur (journal) ; 0 pour une naissance
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
                                       u64*) noexcept;
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
// plateaux, lots) et attach_ns (balayage et controles du rattachement).
[[nodiscard]] Result<OrderTree> build_order(FullDomain&&, Order k, MemoryBudget&, FullParams = {},
                                           sched::Pool* = nullptr, OrderTimings* = nullptr,
                                           u64* attach_ns = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
