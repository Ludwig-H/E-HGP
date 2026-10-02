// Index global possede, independant du catalogue : arbre equilibre de plages Morton du meme Cloud.
// Construction neuve v11 ; aucun index ou certificat de feuille v8/v10 n'est porte implicitement.
#pragma once

#include <span>
#include <utility>

#include "cloud/cloud.hpp"
#include "num/num.hpp"

namespace mhgp11 {

struct IndexParams { u32 leaf_size = 8; };  // 1..256 ; taille, jamais un quota de recherche

namespace index_detail {
struct Node {
  num::Box box;
  u32 begin, end;  // plage de SiteIdx [begin,end), non vide
  u64 escape;     // premier noeud suivant le sous-arbre en parcours prefixe
};
struct Access;
}

// Possede le Cloud ET les boites. Aucun pointeur vers l'objet Cloud de l'appelant : les vues sur ses tableaux
// suivent les constructions par deplacement. Les budgets d'origine et d'index doivent survivre au resultat.
// Construire le catalogue futur avec index.cloud() preserve exactement l'interpretation de ses SiteIdx.
class GlobalIndex {
 public:
  GlobalIndex(const GlobalIndex&) = delete;
  GlobalIndex& operator=(const GlobalIndex&) = delete;
  GlobalIndex& operator=(GlobalIndex&&) = delete;
  GlobalIndex(GlobalIndex&& other) noexcept
      : cloud_(std::move(other.cloud_)), nodes_(std::move(other.nodes_)),
        leaf_size_(std::exchange(other.leaf_size_, 0)), max_depth_(std::exchange(other.max_depth_, 0)) {}
  const Cloud& cloud() const noexcept { return cloud_; }
  u64 nodes() const noexcept { return nodes_.size(); }
  u64 max_depth() const noexcept { return max_depth_; }  // racine a profondeur 1
  u32 leaf_size() const noexcept { return leaf_size_; }

 private:
  friend struct index_detail::Access;
  friend Result<GlobalIndex> build_index(Cloud&&, const IndexParams&, MemoryBudget&) noexcept;
  GlobalIndex(Cloud&& cloud, Buffer<index_detail::Node>&& nodes, u32 leaf_size, u64 depth) noexcept
      : cloud_(std::move(cloud)), nodes_(std::move(nodes)), leaf_size_(leaf_size), max_depth_(depth) {}
  Cloud cloud_;
  Buffer<index_detail::Node> nodes_;
  u32 leaf_size_;
  u64 max_depth_;
};

// Parametres puis nuage vide puis memoire. Echec : cloud reste entier, toutes ses vues restent valides.
// Succes seulement : transfere le Cloud sans copier ses tableaux ; cloud devient vide. Le budget d'index
// finance seulement nodes()*sizeof(index_detail::Node), en plus de ses reservations preexistantes.
[[nodiscard]] Result<GlobalIndex> build_index(Cloud&& cloud, const IndexParams& params,
                                            MemoryBudget& budget) noexcept;

enum class CensusKind : u8 { complete, saturated };

// Travail reel CUMULE des deux parcours (compte puis remplissage), pas seulement une passe logique.
struct CensusLedger {
  u64 nodes = 0, bounds = 0, point_tests = 0, inside_blocks = 0, outside_blocks = 0, passes = 0;
  friend bool operator==(const CensusLedger&, const CensusLedger&) = default;
};

class Census {
 public:
  Census(const Census&) = delete;
  Census& operator=(const Census&) = delete;
  Census& operator=(Census&&) = delete;
  Census(Census&& other) noexcept
      : interior_(std::move(other.interior_)), shell_(std::move(other.shell_)),
        kind_(std::exchange(other.kind_, CensusKind::complete)), ledger_(std::exchange(other.ledger_, {})) {}
  CensusKind kind() const noexcept { return kind_; }
  std::span<const SiteIdx> interior() const noexcept { return interior_.span(); }
  std::span<const SiteIdx> shell() const noexcept { return shell_.span(); }
  const CensusLedger& ledger() const noexcept { return ledger_; }

 private:
  friend Result<Census> census(const GlobalIndex&, const num::Sphere&, u32, MemoryBudget&) noexcept;
  Census() = default;
  Buffer<SiteIdx> interior_, shell_;
  CensusKind kind_ = CensusKind::complete;
  CensusLedger ledger_;
};

// Seuil strictement positif, puis refus empty_input sur index deplace. Soit K sites strictement interieurs
// distincts (saturated, coquille vide), soit I et TOUTE U (complete, |I|<K). Les deux listes sont croissantes.
// Ce sont des SITES : les multiplicites du Cloud ne sont pas developpees. Ni census pondere ni K plus proches.
// Le centre peut sortir du domaine des points ou de toute boite de supports. Aucun emprunt au catalogue.
// Index immuable partageable ; resultats possedes, etat prive, un pilote par budget, pas d'allocation cachee.
// Reservations propres exactes : sizeof(SiteIdx)*(K si sature, sinon |I|+|U|). Refus sans resultat partiel.
[[nodiscard]] Result<Census> census(const GlobalIndex& index, const num::Sphere& sphere, u32 threshold,
                                    MemoryBudget& budget) noexcept;

}  // namespace mhgp11
