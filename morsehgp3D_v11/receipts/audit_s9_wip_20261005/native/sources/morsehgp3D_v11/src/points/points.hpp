// En-tete public de points : hierarchie laminaire de points H^r_{K+1} a l'ordre K (tranche S9 de la sortie parametree ;
// docs/HIERARCHIE_POINTS.md, paragraphes 3 et 9 ; docs/SORTIES.md, paragraphe 7). Un autre module n'inclut que ce
// fichier.
//
// PORT EXPLICITE, decision par decision, de la chaine Python qualifiee (docs/PROVENANCE.md, section S9) :
// bench/points_hierarchy.py (qualify, qualified_starts, first_points), bench/points_radius.py (hang_margin_radius,
// ancestor_at_radius, floor_rank_radius) et bench/points_flat.py (tower_point_tree). Ce qui change : les incidences
// fortes viennent du rattachement W_K de l'arbre d'ordre K (WindowAttachment, sans descente ; a K = 1 la feuille du
// site), le plus petit ancetre commun et la remontee passent par tower::AncestorIndex, et toute comparaison de sommes
// de racines passe d'abord par la table des racines num::RootTable (encadrement entier), puis par le repli exact de
// num (sqrt_cmp2 pour deux racines contre deux, RadicalSum pour trois contre trois). Aucune decision flottante : une
// proposition binary64 ne sert qu'a la recherche du rang plancher, toujours certifiee en entier.
//
// Conventions figees (docs/HIERARCHIE_POINTS.md, paragraphe 9) : qualification m(K) = 1 si K = 1, K + 1 sinon ; marge
// kappa = 1. Une date est sqrt(l_t) + sqrt(l_M) - sqrt(l_Q) en rangs du catalogue Cat_K (M = Q = 0 sans rival : le rang
// 0 est le niveau nul).
#pragma once

#include <span>

#include "core/core.hpp"
#include "num/num.hpp"
#include "tower/tower.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::points {

inline constexpr u32 kKappa = 1;
// Seuil de qualification m(K) des pendaisons (HIERARCHIE_POINTS.md, paragraphe 9).
constexpr u32 qualification(Order k) noexcept { return k == 1 ? 1 : u32{k} + 1; }

// Arbre de points N-aire a plateaux atomiques (port de PointTree, bench/points_flat.py). Les plateaux sont strictement
// croissants en valeur exacte ; un plateau de niveau du catalogue vaut (r, 0, 0), un plateau de date (t, M, Q). Un bloc
// est cree par la fusion d'au moins deux blocs non vides (enfants : les blocs dont il est le parent) ou par une entree ;
// block_parent vaut kNone a une racine. Au plus 2n - 1 blocs.
class PointTree {
 public:
  PointTree() = default;
  PointTree(PointTree&&) noexcept = default;
  PointTree& operator=(PointTree&&) noexcept = default;
  PointTree(const PointTree&) = delete;
  PointTree& operator=(const PointTree&) = delete;
  std::span<const u32> plateau_t() const noexcept { return plateau_t_.span(); }
  std::span<const u32> plateau_m() const noexcept { return plateau_m_.span(); }
  std::span<const u32> plateau_q() const noexcept { return plateau_q_.span(); }
  std::span<const u32> block_plateau() const noexcept { return block_plateau_.span(); }
  std::span<const u32> block_parent() const noexcept { return block_parent_.span(); }
  std::span<const u32> site_block() const noexcept { return site_block_.span(); }
  std::span<const u32> site_plateau() const noexcept { return site_plateau_.span(); }

 private:
  friend struct PointTreeBuilder;
  Buffer<u32> plateau_t_, plateau_m_, plateau_q_, block_plateau_, block_parent_, site_block_, site_plateau_;
};

// Compteurs et durees d'un appel (mesure ; jamais une decision). Les durees vont au rapport de la sonde.
struct PointsStats {
  u64 incidences = 0, qualified_nodes = 0, delayed = 0, strict = 0, rivals = 0, dominated = 0;
  u64 roots = 0, table_decisions = 0, exact_decisions = 0, floor_steps = 0;
  u64 incidences_ns = 0, qualify_ns = 0, roots_ns = 0, hang_ns = 0, tree_ns = 0;
};

// Pendaison et arbre de points d'un appel, par SiteIdx : date (t, M, Q), proprietaire (noeud de la foret d'ordre K
// vivant a la date, coupe fermee), rang plancher (plus grand rang de niveau <= date au carre) et drapeau strict (date
// strictement entre deux niveaux). levels() : rangs references par les colonnes publiees, croissants.
class PointHierarchy {
 public:
  PointHierarchy(PointHierarchy&&) noexcept = default;
  PointHierarchy(const PointHierarchy&) = delete;
  PointHierarchy& operator=(const PointHierarchy&) = delete;
  PointHierarchy& operator=(PointHierarchy&&) = delete;
  Order order() const noexcept { return order_; }
  u32 qualification() const noexcept { return m_; }
  std::span<const u32> t() const noexcept { return t_.span(); }
  std::span<const u32> m() const noexcept { return big_m_.span(); }
  std::span<const u32> q() const noexcept { return q_.span(); }
  std::span<const u32> owner() const noexcept { return owner_.span(); }
  std::span<const u32> floor() const noexcept { return floor_.span(); }
  std::span<const u8> strict() const noexcept { return strict_.span(); }
  std::span<const u32> levels() const noexcept { return levels_.span(); }
  const PointTree& tree() const noexcept { return tree_; }
  const PointsStats& stats() const noexcept { return stats_; }

 private:
  friend struct HangBuilder;
  PointHierarchy() = default;
  Buffer<u32> t_, big_m_, q_, owner_, floor_;
  Buffer<u8> strict_;
  Buffer<u32> levels_;
  PointTree tree_;
  PointsStats stats_;
  Order order_ = 0;
  u32 m_ = 0;
};

// H^r_{K+1} sur l'arbre d'ordre K et son rattachement, qualification m(K). Refus : parameter_out_of_range (K >= n
// pour K >= 2 : aucune composante n'atteint K + 1 sites), memory_budget, radical_sign_budget (repli exact hors budget),
// arithmetic_invariant, points_invariant (incidence, qualification ou proprietaire incoherents). Aucun resultat partiel.
[[nodiscard]] Result<PointHierarchy> hang(const OrderTree& tree, MemoryBudget& budget,
                                          sched::Pool* pool = nullptr) noexcept;
// Meme calcul a qualification m explicite, pour les portes (fixtures de bench/points_gate.py a m = 1 et K = 2) :
// 1 <= m <= K (aucune qualification : chaque noeud l'est a sa naissance) ou m = K + 1 ; sinon parameter_out_of_range
// (la recurrence generale de qualify_general n'est pas portee).
[[nodiscard]] Result<PointHierarchy> hang_qualified(const OrderTree& tree, u32 m, MemoryBudget& budget,
                                                    sched::Pool* pool = nullptr) noexcept;

}  // namespace mhgp11::points
