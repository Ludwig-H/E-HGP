// Domaine possede des futures descentes : un index, son catalogue et un lookup exact des supports globaux.
// Ce contexte ne construit aucune cellule, descente, foret ou verticale FULL.
#pragma once

#include <array>
#include <optional>
#include <utility>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"

namespace mhgp11 {

class FullDomain {
 public:
  FullDomain(const FullDomain&) = delete;
  FullDomain& operator=(const FullDomain&) = delete;
  FullDomain& operator=(FullDomain&&) = delete;
  FullDomain(FullDomain&&) noexcept = default;

  const GlobalIndex& index() const noexcept { return index_; }
  const Catalogue& catalogue() const noexcept { return catalogue_; }
  u64 lookup_capacity() const noexcept { return slots_.size(); }

  // Egalite des QUATRE SiteIdx, y compris le padding kNone : toute autre presentation est un miss.
  // Une cle arbitraire est permise ; elle ne sert jamais a indexer le Cloud. Aucun calcul ni allocation
  // geometrique. Un miss du support LOCAL d'une MEB n'est pas une preuve d'absence de sa boule globale.
  // Apres deplacement, le contexte source est vide et toute recherche y rend nullopt.
  std::optional<BallIdx> find_support(const std::array<SiteIdx, 4>& key) const noexcept;

 private:
  friend Result<FullDomain> prepare_full_domain(GlobalIndex&&, const CatalogueParams&, MemoryBudget&) noexcept;
  friend Result<FullDomain> prepare_full_domain(GlobalIndex&&, const CatalogueParams&, MemoryBudget&,
                                               sched::Pool&, CatalogueTimings*) noexcept;
  FullDomain(GlobalIndex&& index, Catalogue&& catalogue, Buffer<BallIdx>&& slots) noexcept
      : index_(std::move(index)), catalogue_(std::move(catalogue)), slots_(std::move(slots)) {}
  GlobalIndex index_;
  Catalogue catalogue_;
  Buffer<BallIdx> slots_;
};

// Construit CatK sur index.cloud(), puis le lookup, avant de transferer index au SUCCES final seulement.
// Ordre des refus du catalogue : parametres, nuage vide, poids !=1, ressources/calcul ; puis table.
// Tout refus conserve l'index et ses vues, et rend les reservations de cet appel sans toucher aux anciennes.
// Le resultat possede les trois stockages, sans emprunt vers l'objet index source. Leurs budgets survivent
// au resultat. Construction deplacement seulement ; les vues anterieures suivent le nouveau proprietaire.
//
// B boules : C=0 si B=0, sinon la plus petite puissance de deux >=2B. La table reserve exactement
// sizeof(BallIdx)*C = 4C octets, charge <=1/2 ; offsets et compteurs de sondage en u64. Aucun tag ni copie
// des supports. Pic propre = max(F_cat, R_cat+4C), F_cat pic propre de construction, R_cat sortie retenue.
// Pour U octets preexistants stables : peak_final=max(peak_initial,U+pic_propre). Le stockage d'index/Cloud
// est deja dans U s'il utilise ce budget ; sinon il reste a compter separement au pic physique.
[[nodiscard]] Result<FullDomain> prepare_full_domain(GlobalIndex&& index, const CatalogueParams& params,
                                                    MemoryBudget& budget) noexcept;

// Meme domaine et lookup, catalogue construit par l'API parallele publique. Pool emprunte jusqu'au retour.
// Le diagnostic mesure uniquement le catalogue et reste prive jusqu'au SUCCES COMPLET, table comprise :
// tout refus conserve aussi *timings. Un pointeur nul garde les horloges internes desactivees.
[[nodiscard]] Result<FullDomain> prepare_full_domain(GlobalIndex&& index, const CatalogueParams& params,
                                                    MemoryBudget& budget, sched::Pool& pool,
                                                    CatalogueTimings* timings = nullptr) noexcept;

}  // namespace mhgp11
