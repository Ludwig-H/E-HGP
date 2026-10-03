// Raccourci exact de descente par population : table possedee I union U -> boule, ouverte a sondage lineaire.
// Lemme : si une partie F de k sites est EXACTEMENT la population I union U d'une boule b du catalogue, alors
// b contient F et son support canonique S* est dans U, donc MEB(F)=MEB(S*)=b ; la descente de reference
// trouve ensuite p<k et t=k-p=m, terminal immediat : graine (b,k), niveaux initial et terminal egaux a celui
// de b. Les populations sont distinctes (unicite de la MEB). Inspiration : semis H_K de la v10 (tower.cpp R2).
#pragma once
#include "tower/descent.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::tower_detail {

class PopulationLookup {
 public:
  PopulationLookup(const PopulationLookup&) = delete;
  PopulationLookup& operator=(const PopulationLookup&) = delete;
  PopulationLookup& operator=(PopulationLookup&&) = delete;
  PopulationLookup(PopulationLookup&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), slots_(std::move(other.slots_)),
        rows_(std::move(other.rows_)), entries_(std::exchange(other.entries_, 0)),
        width_(std::exchange(other.width_, 0)) {}

  // Boules eligibles E : |I|+|U| <= K (seules elles peuvent egaler une partie de descente). Chaque entree a une
  // ligne contigue [boule, sites croissants, kNone...] de K+1 mots ; une case vaut (etiquette 32 bits, entree+1).
  // Capacite : plus petite puissance de deux >= 2E (charge <= 1/2). Octets : 8C + 4E(K+1), admis avant
  // allocation. Pool facultatif : lignes par ordinal, cases par CAS ; reponses independantes de l'ordonnancement.
  static Result<PopulationLookup> make(const FullDomain&, MemoryBudget&, sched::Pool* = nullptr) noexcept;
  bool belongs_to(const FullDomain& domain) const noexcept { return domain_ == &domain; }
  u64 reserved_bytes() const noexcept { return slots_.size() * sizeof(u64) + rows_.size() * sizeof(u32); }
  u64 entries() const noexcept { return entries_; }

  // Partie quelconque de k sites (ordre libre). Hors domaine ou miss : rien, la descente complete decide.
  // k=1 : le site lui-meme au niveau nul (meme terminal que la reference). Le ledger compte un pas,
  // population_hits=1 et catalogue_hits ou singleton_hits=1 : census+catalogue+singleton=steps reste vrai.
  // L'etiquette ne fait que filtrer les sondages : seule l'egalite exacte de la ligne decide.
  Result<std::optional<DescentResult>> descend(std::span<const SiteIdx> part, u32 k) const noexcept;
  // Meme decision que descend, sans DescentResult ni ledger : graine et niveau du catalogue (initial et
  // terminal egaux, niveau nul pour k=1). Pour les appelants qui comptent eux-memes le pas.
  struct Hit {
    BirthSeed seed;
    const num::Level* level;
  };
  Result<std::optional<Hit>> hit(std::span<const SiteIdx> part, u32 k) const noexcept;
  // Descente de reference ou la table est consultee AVANT chaque pas : un succes remplace exactement le pas
  // terminal que descent_step rendrait (lemme ci-dessus, meme niveau, meme graine, decroissance controlee) ;
  // sinon descent_step decide. Memes refus que descend. first_missed : premiere partie deja cherchee sans succes.
  Result<DescentResult> descend_each_step(std::span<const SiteIdx> part, u32 k, MemoryBudget&,
                                          CensusWorkspace*, bool first_missed) const noexcept;
  // Prechargement seul, aucune reponse : case de depart du sondage d'une partie de k >= 2 sites (ordre libre).
  void prefetch(std::span<const SiteIdx> part, u32 k) const noexcept;

 private:
  struct Builder;
  explicit PopulationLookup(const FullDomain& domain) noexcept : domain_(&domain) {}
  std::optional<BallIdx> find(const std::array<SiteIdx, kMaxMebSites>& sorted, u32 count) const noexcept;
  const FullDomain* domain_;
  Buffer<u64> slots_;  // (etiquette << 32) | (entree + 1), zero = case vide
  Buffer<u32> rows_;   // entrees * width_ mots : boule puis population triee completee par kNone
  u64 entries_ = 0;
  u32 width_ = 0;      // K + 1
};

}  // namespace mhgp11::tower_detail
