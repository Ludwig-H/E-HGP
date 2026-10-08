// Index global possede, independant du catalogue : arbre equilibre de plages Morton du meme Cloud.
// Construction neuve v11 ; aucun index ou certificat de feuille v8/v10 n'est porte implicitement.
#pragma once

#include <span>
#include <utility>
#include <atomic>
#include <memory>

#include "cloud/cloud.hpp"
#include "num/num.hpp"

namespace mhgp12 {

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

// Travail reel CUMULE des parcours executes : deux pour census possede (compte puis remplissage),
// un pour CensusWorkspace. `passes` rend ce nombre ; jamais une estimation de passe logique.
// `lanes` compte les voies numeriques des evaluations (repere local, docs/CONTRAT_NUMERIQUE.md, paragraphe 3) ;
// les compteurs `guard_*` comptent les decisions que la garde (census d'une boule certifiee) prend sans
// arithmetique : boites disjointes du pave, boites partielles a raffiner, sites hors du pave, et noeuds raffines par
// un temoin sur la sphere (census a temoins, CensusWorkspace::query). `bounds` compte les noeuds interroges (bornes
// decidees par arithmetique, par la garde ou par un temoin), pas les appels a bound_signs : guard_witness compte les
// appels evites (contre-lecture de l'auditeur Codex du 8 octobre, receipts/audit_reponses_20261008/census_temoins).
struct CensusLedger {
  u64 nodes = 0, bounds = 0, point_tests = 0, inside_blocks = 0, outside_blocks = 0, passes = 0;
  num::LaneCount lanes;
  u64 guard_disjoint = 0, guard_partial = 0, guard_outside = 0, guard_witness = 0;
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
  friend Result<Census> census(const GlobalIndex&, const num::CertifiedBall&, u32, MemoryBudget&) noexcept;
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
// Census GARDE (NUM-CERTIFIEE, NUM-GARDE ; CST-0108, CST-0109) : le meme contrat pour une boule certifiee, dont le
// centre est prouve dans l'enveloppe de son support. Memes populations et memes refus que census(index,
// ball.sphere(), ...) ; le parcours rejette sans arithmetique les boites disjointes de son pave et les sites hors du
// pave, ne forme jamais de centre absolu et evalue au budget mixte de la boule (6s+11). Une sphere non certifiee n'a
// pas ce type : elle reste dans le census generique.
[[nodiscard]] Result<Census> census(const GlobalIndex& index, const num::CertifiedBall& ball, u32 threshold,
                                    MemoryBudget& budget) noexcept;

class CensusWorkspace;

// Certificat emprunte, construit seulement apres UN parcours complet ou la saturation certifiee.
// Les vues sont constantes et valides SEULEMENT pendant le callback de query. Ni les spans ni une reference
// a cet objet ne doivent etre conserves. C++ ne peut pas empecher la copie d'un span par le consommateur.
class BorrowedCensus {
 public:
  BorrowedCensus(const BorrowedCensus&) = delete;
  BorrowedCensus& operator=(const BorrowedCensus&) = delete;
  CensusKind kind() const noexcept { return kind_; }
  std::span<const SiteIdx> interior() const noexcept { return interior_; }
  std::span<const SiteIdx> shell() const noexcept { return shell_; }
  const CensusLedger& ledger() const noexcept { return ledger_; }

 private:
  friend class CensusWorkspace;
  BorrowedCensus(CensusKind kind, std::span<const SiteIdx> interior, std::span<const SiteIdx> shell,
                 CensusLedger ledger) noexcept : kind_(kind), interior_(interior), shell_(shell), ledger_(ledger) {}
  CensusKind kind_;
  std::span<const SiteIdx> interior_, shell_;
  CensusLedger ledger_;
};

// Stockage prive de EXACTEMENT n SiteIdx, reserve une fois dans le budget. L'objet de controle a une taille
// constante, hors compte Buffer ; son allocation peut aussi refuser. Aucun alias mutable du stockage expose.
// L'index d'origine doit rester vivant et immobile jusqu'a la destruction du workspace ; le budget survit aussi.
// Une Session peut preparer C=min(W,L,Q) workspaces avant Pool, apres admission commune de 4*n*C octets.
class CensusWorkspace {
 public:
  using Callback = Outcome (*)(void*, const BorrowedCensus&);
  CensusWorkspace(const CensusWorkspace&) = delete;
  CensusWorkspace& operator=(const CensusWorkspace&) = delete;
  CensusWorkspace(CensusWorkspace&&) = delete;
  CensusWorkspace& operator=(CensusWorkspace&&) = delete;
  static Result<std::unique_ptr<CensusWorkspace>> make(const GlobalIndex&, MemoryBudget&) noexcept;
  u64 capacity() const noexcept { return storage_.size(); }
  // Identite verifiee sans toucher au stockage ; reste requise meme avant un hit sans census.
  bool belongs_to(const GlobalIndex& index) const noexcept {
    return index_ == &index && storage_.size() == index.cloud().sites();
  }
  // Concurrence/reentrance, autre index, index deplace, seuil nul ou callback nul : parameter_out_of_range.
  // Le workspace et l'index restent vivants durant TOUT l'appel. Le callback ne voit aucun resultat partiel ;
  // il doit capturer sa propre sortie dans un brouillon. Son refus est propage, bad_alloc devient memory_budget,
  // toute autre exception task_exception. Le verrou est libere dans tous les cas. Aucun Buffer alloue ici.
  [[nodiscard]] Outcome query(const GlobalIndex&, const num::Sphere&, u32 threshold,
                               void* context, Callback) noexcept;
  // Meme requete pour une boule certifiee : census garde, une passe.
  [[nodiscard]] Outcome query(const GlobalIndex&, const num::CertifiedBall&, u32 threshold,
                               void* context, Callback) noexcept;
  // Census garde A TEMOINS (T2-d) : witnesses donne au plus kMaxWitnesses SiteIdx de sites SUR la sphere (le support
  // de la boule certifiee). Un noeud dont la plage contient un temoin a un minorant entier <= 0 (le temoin est un point
  // entier de sa boite, de puissance nulle) et un majorant >= 0 : il n'est ni exterieur ni interieur, il est raffine
  // (ses sites testes s'il est une feuille) SANS evaluer ses bornes. Memes resultats, memes noeuds et memes sites
  // testes que query sans temoins quand les temoins sont sur la sphere ; un faux temoin ne change que le travail (des
  // noeuds raffines au lieu d'etre tranches), jamais I ni U, car tout site reste teste exactement. Refus
  // parameter_out_of_range au-dela de kMaxWitnesses temoins ou pour un temoin hors du nuage.
  static constexpr std::size_t kMaxWitnesses = 4;
  [[nodiscard]] Outcome query(const GlobalIndex&, const num::CertifiedBall&, u32 threshold,
                               std::span<const SiteIdx> witnesses, void* context, Callback) noexcept;

 private:
  explicit CensusWorkspace(const GlobalIndex& index) noexcept : index_(&index) {}
  template <class Bounds, class Ball>
  Outcome run(const GlobalIndex&, const Ball&, u32 threshold, std::span<const SiteIdx> witnesses, void* context,
              Callback) noexcept;
  const GlobalIndex* index_;
  Buffer<SiteIdx> storage_;
  std::atomic_flag active_ = ATOMIC_FLAG_INIT;
};

}  // namespace mhgp12
