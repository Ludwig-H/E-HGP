// Frontiere fixe possedee : au plus 2^8 taches, independamment du nombre de workers.
// Les seuls tableaux variables sont les Buffer des listes ; aucun doublement ou arena relocalisable.
#pragma once

#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {

inline constexpr u32 kFrontierDepth = 8;
inline constexpr u32 kFrontierTasks = u32{1} << kFrontierDepth;

// Borne supplementaire avant construction : listes possedees + pile du preambule + liste racine.
// Conservatrice : toutes les capacites de listes sont majorees par sites. Aucun tableau n'est alloue ici.
Outcome frontier_memory_bound(u32 sites, u32 cut_depth, u64& bytes) noexcept;

class Frontier {
 public:
  Frontier() = default;
  Frontier(const Frontier&) = delete;
  Frontier& operator=(const Frontier&) = delete;

  // Run commence avec ledger vide. Coupe APRES filtre/ajustement ; chaque liste est deplacee sans copie.
  // Une feuille precoce devient aussi une tache. Un refus rend tous les Buffer acquis par cette preparation.
  Outcome prepare(Run& run, u32 cut_depth = kFrontierDepth) noexcept;
  // Rejoue exactement le preambule, compare les listes initialisees/boites/profondeurs et son ledger.
  // Ne remplace aucun Buffer possede ; les temporaires du rejeu coexistent avec toute la frontiere.
  Outcome verify(Run& run) const noexcept;

  u32 size() const noexcept { return count_; }
  const ReadyNode& task(u32 i) const noexcept { return tasks_[i]; }  // i<size()
  const CatalogueLedger& ledger() const noexcept { return ledger_; }
  Outcome execute_task(u32 i, Run& run) const noexcept;

  // Octets Buffer possedes ; metadonnees fixes de 256 proprietaires hors compte, comme les etats du Pool.
  Outcome owned_bytes(u64& bytes) const noexcept;
  // Borne supplementaire du preambule de verify ; n'inclut pas les listes possedees deja dans budget.used().
  Outcome verify_memory_bound(u64& bytes) const noexcept;
  // Borne supplementaire des DFS concurrents seulement : somme des W plus grandes bornes de taches.
  // Les workspaces de feuille, sorties et listes possedees doivent etre comptes separement par le pilote.
  Outcome suffix_memory_bound(u32 workers, u64& bytes) const noexcept;

 private:
  bool matches(const Run& run) const noexcept;
  void clear() noexcept;
  std::array<ReadyNode, kFrontierTasks> tasks_{};
  const Cloud* cloud_ = nullptr;
  CatalogueParams params_{};
  CatalogueLedger ledger_{};
  u32 count_ = 0, cut_depth_ = kFrontierDepth;
  bool prepared_ = false;
};

}  // namespace mhgp11::catalogue_detail
