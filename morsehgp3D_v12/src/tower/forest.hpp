// Etages T, M, V et R de la tour (docs/CONTRAT_TOUR.md, paragraphes 3, 5, 7 et 8 ; docs/ARCHITECTURE.md, paragraphes
// 4.3 et 4.4), voie CPU de reference. Port du prototype de MES-M4 (microbancs/mes_m3_m4_tour/mes_m4/mes_m4.cpp) :
//   T  noyau union-find par TAILLE sans lots (D-F1) : par ordre, les cellules de fenetre non naissances (jonctions et
//      cellules inertes, pont de LEM-T4) dans l'ordre des rangs ; chaque union de deux composantes distinctes est un
//      evenement binaire de 20 octets ; attache de chaque perdant avec son rang (historique de LEM-T5) ; une cible
//      << cellule >> se lit comme l'element de la cellule deja traitee, relu a sa racine courante ;
//   M  numerotation canonique des naissances (rang, centre exact, comparaison en deux temps) ; contraction des plateaux
//      (LEM-T4 : classes d'evenements lies de meme rang = multifusions N-aires), fusions par (rang, plus petite
//      naissance), parents, enfants en CSR tries ; par tranches alignees sur les rangs, sortie independante du decoupage ;
//   V  image d'une naissance par LEM-T6, d'une fusion par LEM-T5 ;
//   R  registre des noeuds (rang, parent, genre par la position, boule de naissance) et des verticales, ecrit par T, M
//      et V au fil du calcul (OrderForest) ; hyperaretes retenues par Kruskal et leurs branches ouvertes, collectees
//      apres M par des requetes de LEM-T5 a la coupe ouverte de leur rang (registry_branches.cpp).
// Entree : les cibles des representants de l'etage G (ou de l'adaptateur de test des vidages de la v11, tests/tower).
// Aucune decision en flottant ; refus transactionnels : un refus ne publie rien.
#pragma once

#include <array>
#include <span>

#include "catalogue/catalogue.hpp"
#include "cloud/cloud.hpp"
#include "core/core.hpp"
#include "num/num.hpp"

namespace mhgp12 {
namespace sched {
class Pool;
}

namespace tower {

inline constexpr Order kMaxOrder = 12;

// Cibles de 4 octets (contrat, paragraphe 7) : le codage de l'etage G (tower.hpp : kTargetCellBit, kTargetIndexMask,
// kNoTarget, birth_target, cell_target, target_is_cell, target_index), indice d'une naissance dans la liste de
// l'ordre (ForestInput::birth_key) ou d'une cellule de l'ordre. Domaine des operandes (CST-0212) : au plus 2^31 - 1
// naissances et 2^31 - 1 cellules par ordre (kMaxOrderBirths et kMaxOrderCells de G), refus avant allocation au-dela
// (tower_capacity). Evenements au plus 2^31 - 2, noeuds au plus 2^32 - 3, sous kNone.
inline constexpr u64 kMaxOrderItems = (u64{1} << 31) - 1;
constexpr bool operand_domain(u64 count) noexcept { return count <= kMaxOrderItems; }
static_assert(2 * kMaxOrderItems - 1 < u64{kNone}, "tour : identifiants de noeuds sous la sentinelle");

// Entree d'un ordre k (sortie de l'etage G). Vues empruntees pendant build_forests.
//   birth_key   naissances : SiteIdx a k = 1, BallIdx ensuite ; strictement croissantes, rangs croissants au sens large
//   birth_rank  LevelRank de chaque naissance
//   cell_ball   cellules de fenetre non naissances : BallIdx strictement croissants, donc rangs croissants au sens large
//               (ordre canonique fixe du contrat, paragraphe 8)
//   cell_rank   LevelRank de chaque cellule
//   rep_offsets cellules + 1 decalages des representants (une cellule en a au moins un ; un seul : cellule inerte)
//   targets     cible de chaque representant : naissance de rang strictement inferieur a la cellule, ou cellule
//               deja traitee de rang strictement inferieur (LEM-T3)
struct ForestInput {
  Order k = 0;
  std::span<const u32> birth_key;
  std::span<const LevelRank> birth_rank;
  std::span<const BallIdx> cell_ball;
  std::span<const LevelRank> cell_rank;
  std::span<const u64> rep_offsets;
  std::span<const u32> targets;
};

// Adaptateur sans copie d'un ordre resolu par l'etage G (ResolvedOrder de G : order, birth_keys, birth_ranks,
// cell_balls, cell_ranks, cell_offsets, targets ; INTERFACE.md de G, paragraphe 3) vers l'entree de T, M et V.
template <class Resolved>
ForestInput forest_input(const Resolved& order) noexcept {
  return ForestInput{order.order(),       order.birth_keys(),   order.birth_ranks(), order.cell_balls(),
                     order.cell_ranks(),  order.cell_offsets(), order.targets()};
}

// S* d'une boule du catalogue : quatre SiteIdx croissants, kNone au-dela de q_min (q_min = 1 : un site, rayon nul ;
// oracle borne seulement). Rappel sur le domaine immuable (catalogue de T1, ou adaptateur de test), lu pour les centres
// des naissances : ordre canonique (M) et export.
struct BallSource {
  const void* context = nullptr;
  u64 balls = 0;
  std::array<u32, 4> (*support)(const void* context, u32 ball) noexcept = nullptr;
};

// Supports du catalogue de T1 (CatalogueBall::support), sans copie : la source des centres des naissances et de
// l'export dans la chaine du produit. Le catalogue doit survivre a la source.
[[nodiscard]] BallSource catalogue_balls(const Catalogue& catalogue) noexcept;

// Parametres : evenements par tranche de la contraction (au moins un ; les tranches s'alignent sur les rangs). Le
// resultat ne depend ni des tranches ni du nombre de fils.
struct ForestParams {
  u32 slice_events = 1u << 16;
};

// Compteurs de l'objet (paragraphe 8) : fonctions de la tour seule. arity[i] : fusions a i + 2 enfants (derniere case :
// 9 enfants et plus).
struct ForestObject {
  u64 births = 0, merges = 0, verticals = 0, cells = 0, inert_cells = 0, representatives = 0;
  std::array<u64, 8> arity{};
  u64 max_arity = 0;
  friend bool operator==(const ForestObject&, const ForestObject&) = default;
};

// Compteurs du travail (paragraphe 8) : fonctions de la politique et de l'ordre canonique fixe, identiques a 1 et N
// fils et quel que soit le decoupage ; hors de toute empreinte de l'objet.
struct ForestWork {
  u64 cohorts = 0, max_cohort = 0;                       // naissances : cohortes de meme rang a plus d'une naissance
  u64 birth_targets = 0, cell_targets = 0;               // representants par genre de cible
  u64 events = 0, attaches = 0, max_attach_depth = 0;    // noyau
  u64 retained_cells = 0;                                // cellules qui ont produit au moins un evenement
  u64 classes = 0;                                       // contraction : classes d'evenements lies
  u64 t6_from_cell = 0, t6_climbs = 0, t6_from_birth = 0;  // naissances par LEM-T6
  u64 t5_queries = 0, t5_hops = 0, t5_max_hops = 0, t5_probes = 0;  // fusions par LEM-T5
  u64 branch_reads = 0, branches = 0;                    // registre : representants relus, branches publiees
  friend bool operator==(const ForestWork&, const ForestWork&) = default;
};

// Diagnostics physiques : jamais dans une empreinte.
struct ForestPhysical {
  u64 births_ns = 0, leaves_ns = 0, kernel_ns = 0, history_ns = 0;  // T (pre-passe : somme des morceaux)
  u64 contraction_ns = 0, vertical_ns = 0;            // M et V : somme des durees des tranches et morceaux de l'ordre
  u64 registry_ns = 0;                                // R : somme des durees des morceaux de l'ordre
};

struct ForestLedger {
  Order kmax = 0;
  std::array<ForestObject, kMaxOrder> object{};
  std::array<ForestWork, kMaxOrder> work{};
  std::array<ForestPhysical, kMaxOrder> physical{};
  u64 kernels_ns = 0, contraction_ns = 0, vertical_births_ns = 0, vertical_merges_ns = 0;  // etages, tous ordres
  u64 registry_ns = 0;
  u64 slices = 0;
  u32 threads = 0;
};

// Registre d'un ordre (etage R, premiere forme), numerotation canonique (contrat, paragraphe 1) : naissances
// 0 .. births - 1 par (rang, centre exact), puis fusions par (rang, plus petite naissance). Genre : v < births.
struct OrderForest {
  Order k = 0;
  u32 births = 0;
  u32 root = kNone;
  Buffer<u32> birth_key;      // noeud de naissance -> cle (SiteIdx a k = 1, BallIdx ensuite)
  Buffer<u32> birth_node;     // indice d'entree (ForestInput::birth_key) -> noeud de naissance
  Buffer<u32> rank;           // par noeud : LevelRank
  Buffer<u32> parent;         // par noeud : kNone pour la racine
  Buffer<u32> minleaf;        // par noeud : plus petite naissance du sous-arbre
  Csr<u32> children;          // par noeud : enfants tries ; vide pour une naissance
  Buffer<u32> lower;          // par noeud : verticale vers l'ordre k - 1 (vide a k = 1)
  Buffer<u32> cell_node;      // par cellule d'entree : noeud du sommet laisse par la cellule (jtop)
  Buffer<u32> event_cell;     // par evenement : cellule (jonction) qui l'a produit
  // Hyperaretes retenues par Kruskal (cellules qui ont uni au moins deux composantes), dans l'ordre des cellules : indice
  // de cellule d'entree, boule, rang ; leurs BRANCHES (CSR, une ligne par cellule retenue, noeuds croissants) : les
  // noeuds vivants a la coupe OUVERTE du rang de la cellule qui contiennent ses representants (ant(b)), y compris ceux
  // que d'autres cellules du meme plateau ont deja reunis.
  Buffer<u32> retained_cell, retained_ball, retained_rank;
  Csr<u32> branches;
  // Historique d'attache (LEM-T5) : perdant -> survivant et rang ; evenements par survivant dans l'ordre de traitement.
  Buffer<u32> attach_parent, attach_rank;
  Csr<u32> survivor_events;
  Buffer<u32> event_rank, event_node;
  u32 nodes() const noexcept { return static_cast<u32>(rank.size()); }
};

struct TowerForests {
  Order kmax = 0;
  std::array<OrderForest, kMaxOrder> orders;
};

// Controle d'une entree, sans allocation : refus tower_capacity hors du domaine des operandes (naissances ou cellules
// au-dela de 2^31 - 1), tower_invariant si les tailles, les decalages, l'ordre des cles ou des rangs sont incoherents.
[[nodiscard]] Outcome check_forest_input(const ForestInput& input) noexcept;

// T, M, V et R pour les ordres 1 .. inputs.size() (inputs[i].k = i + 1, au plus kMaxOrder). Refus dans cet ordre :
// entrees (check_forest_input), memoire (memory_budget, admission par etage avant allocation), invariants
// (tower_invariant : cible hors domaine ou non anterieure, racine non unique, image verticale introuvable),
// tower_query_domain (requete de LEM-T5 hors domaine). Le nuage, les supports, les entrees et le Pool sont empruntes
// pendant l'appel ; le budget a un seul pilote. ledger, s'il est donne, n'est rempli qu'au succes.
[[nodiscard]] Result<TowerForests> build_forests(const Cloud& cloud, const BallSource& balls,
                                                 std::span<const ForestInput> inputs, const ForestParams& params,
                                                 MemoryBudget& budget, sched::Pool& pool,
                                                 ForestLedger* ledger = nullptr) noexcept;

// LEM-T5 : noeud de l'ordre vivant a la coupe fermee de rang r dont la composante contient la naissance leaf (noeud
// canonique), sous l'hypothese rang(leaf) <= r (CST-0105) ; sinon refus tower_query_domain. hops, probes : sauts
// d'attache et sondes de la dichotomie (compteurs du travail).
[[nodiscard]] Result<u32> component_at(const OrderForest& forest, u32 leaf, u32 rank, u64* hops = nullptr,
                                       u64* probes = nullptr) noexcept;

// Invariants globaux (portes, hors du chemin chronometre) : par ordre une seule racine, une arete par noeud sauf la
// racine, enfants tries de rang strictement inferieur, CSR des enfants coherente, profondeur d'attache au plus
// floor(log2(naissances)) (LEM-T5 (i)), branches du registre vivantes a la coupe ouverte ; a k >= 2, verticales
// vivantes a la coupe fermee et naturelles (theoreme F (ii)). Le reste de l'historique de LEM-T5 (attach_rank,
// survivor_events, event_rank, event_node, event_cell) n'est PAS relu (recu audit_foret_validation_20261008 : gardes
// proposees, semantique des rangs d'attache a juger par une porte dediee). Refus tower_invariant ; memory_budget
// (tampons de travail, admis puis rendus au retour).
[[nodiscard]] Outcome validate_forests(const TowerForests& forests, MemoryBudget& budget) noexcept;

}  // namespace tower
}  // namespace mhgp12
