// Interne aux etages T, M, V et R (forest.hpp) : evenements binaires du noyau, etat de travail d'un ordre, etapes
// appelees par l'orchestration (forest_build.cpp). Port des structures du prototype de MES-M4.
#pragma once

#include "tower/tower.hpp"

namespace mhgp12::tower::detail {

// Operande d'un evenement : feuille (naissance canonique, bit 31 nul) ou evenement (bit 31 mis). 31 bits utiles :
// au plus 2^31 - 1 naissances, donc au plus 2^31 - 2 evenements (CST-0212).
inline constexpr u32 kEventBit = 0x80000000u;
inline constexpr u32 kEventIndex = 0x7FFFFFFFu;
static_assert(kMaxOrderItems == kMaxOrderBirths && kMaxOrderItems == kMaxOrderCells,
              "tour : meme domaine des operandes que l'etage G");

// Evenement binaire du noyau : rang, deux operandes (sommets courants des deux composantes), plus petite feuille de
// l'union, survivant (racine d'union-find qui garde la composante).
struct Event {
  u32 rank, a, b, minleaf, surv;
};
static_assert(sizeof(Event) == 20, "tour : evenement de 20 octets");

// Case d'union-find d'une feuille : parent, taille, dernier evenement survecu (kNone si aucun), plus petite feuille.
struct UnionCell {
  u32 up, size, last, minleaf;
};
static_assert(sizeof(UnionCell) == 16, "tour : case d'union-find de 16 octets");

// Etat de travail d'un ordre entre T et M (tampons du budget, rendus a la fin de build_forests).
struct OrderWork {
  Buffer<u32> birth_order;     // position canonique -> indice d'entree de la naissance
  Buffer<u32> leaves;          // par representant : feuille canonique, ou cible << cellule >> (pre-passe de T)
  Buffer<Event> events;        // capacite naissances - 1 ; event_count ecrits
  u32 event_count = 0;
  Buffer<u32> cell_top;        // par cellule : sommet apres traitement (feuille, ou evenement | kEventBit)
  // Contraction : union-find local, plus petite feuille et cle (rang, plus petite feuille, racine locale) par classe.
  Buffer<u32> local;
  Buffer<u32> class_min;
  Buffer<u64> keys;            // (plus petite feuille << 32) | indice local de la racine, tries par tranche
  Buffer<u32> child_count;     // par noeud
  // Registre : noeuds a la coupe ouverte de tous les representants des cellules retenues (lignes aux decalages
  // branch_off), puis nombre de branches distinctes de chaque ligne.
  Buffer<u32> branch_nodes;
  Buffer<u64> branch_off;
  Buffer<u32> branch_count;
};

// Tranche de la contraction : evenements [lo, hi) d'un ordre, alignes sur les rangs ; ses classes et ses noeuds.
struct Slice {
  u32 order = 0;               // indice de l'ordre (k - 1)
  u32 lo = 0, hi = 0;
  u32 classes = 0, node0 = 0;
  u64 children = 0, child0 = 0;
  u64 ns = 0;                  // duree des quatre phases (diagnostic physique)
};

// Noeud d'un sommet de noyau (feuille ou evenement) une fois la contraction faite.
inline u32 node_of_top(u32 top, std::span<const u32> event_node) noexcept {
  return (top & kEventBit) ? event_node[top & kEventIndex] : top;
}

// Sphere de la boule `ball` par son support S* (q_min = 1 : le site, rayon nul) ; refus tower_invariant (boule hors
// des supports, site hors du nuage, support degenere).
[[nodiscard]] Result<num::Sphere> ball_sphere(const Cloud& cloud, const BallSource& balls, u32 ball) noexcept;

// M, avant T : numerotation canonique des naissances de l'ordre. order_out[i] = indice d'entree de la naissance de
// position canonique i ; node_out[j] = position canonique de l'indice d'entree j. Refus tower_invariant (deux centres
// egaux au meme rang, rangs non croissants, support degenere), memory_budget (tampon des spheres d'une cohorte).
[[nodiscard]] Outcome number_births(const Cloud& cloud, const BallSource& balls, const ForestInput& input,
                                    std::span<u32> order_out, std::span<u32> node_out, ForestWork& work,
                                    MemoryBudget& budget) noexcept;

// T, pre-passe (parallele) sur les cellules [begin, end) : feuille canonique de chaque representant, ou son codage
// << cellule >> ; date de LEM-T3 controlee. Refus tower_invariant (cible hors domaine ou non anterieure).
[[nodiscard]] Outcome resolve_leaves(const ForestInput& input, const OrderForest& forest, std::span<u32> leaves,
                                     u64 begin, u64 end, ForestWork& counters) noexcept;

// T : noyau d'un ordre (feuilles = naissances canoniques, de la pre-passe). Remplit work.events, work.cell_top,
// l'historique d'attache et les cellules des evenements de forest, le nombre d'evenements ; refus tower_invariant
// (racines multiples), memory_budget.
[[nodiscard]] Outcome run_kernel(const ForestInput& input, std::span<const u32> leaves, OrderForest& forest,
                                 OrderWork& work, ForestWork& counters, MemoryBudget& budget) noexcept;

// T, fin : profondeur d'attache maximale (compteur du travail) et evenements par survivant (CSR de LEM-T5).
[[nodiscard]] Outcome build_history(OrderForest& forest, const OrderWork& work, ForestWork& counters,
                                    MemoryBudget& budget) noexcept;

// M, phases de la contraction d'une tranche (forest_contract.cpp). Phase A : classes et cles ; phase B : noeuds,
// rangs, plus petites naissances, noeud de chaque evenement ; phase C : parents et nombres d'enfants ; phase D :
// decalages et enfants tries. Les phases B, C et D lisent les tranches precedentes : chacune attend la fin de la
// precedente pour toutes les tranches de tous les ordres.
void contract_classes(OrderWork& work, Slice& slice) noexcept;
void contract_nodes(const OrderWork& work, OrderForest& forest, const Slice& slice) noexcept;
void contract_parents(OrderWork& work, OrderForest& forest, Slice& slice) noexcept;
void contract_children(OrderWork& work, OrderForest& forest, const Slice& slice) noexcept;

// V : images des naissances d'un ordre k >= 2 (LEM-T6), indices d'entree [begin, end) ; puis images des fusions
// [begin, end) (LEM-T5). Refus tower_invariant, tower_query_domain.
[[nodiscard]] Outcome birth_images(const ForestInput& input, const ForestInput& below_input,
                                   const OrderForest& below, OrderForest& forest, u64 begin, u64 end,
                                   ForestWork& counters) noexcept;
[[nodiscard]] Outcome merge_images(const OrderForest& below, OrderForest& forest, u64 begin, u64 end,
                                   ForestWork& counters) noexcept;

// Etages de build_forests, dans l'ordre d'execution (indices de BuildState::admitted).
inline constexpr u32 kStageT = 0, kStageM = 1, kStageV = 2, kStageR = 3, kStages = 4;

// Etat d'un appel de build_forests : entrees empruntees, registre en construction, travail et compteurs par ordre.
struct BuildState {
  BuildState(const Cloud& c, const BallSource& b, std::span<const ForestInput> in, const ForestParams& p,
             MemoryBudget& m) noexcept
      : cloud(c), balls(b), inputs(in), params(p), budget(m) {}
  const Cloud& cloud;
  const BallSource& balls;
  std::span<const ForestInput> inputs;
  const ForestParams& params;
  MemoryBudget& budget;
  TowerForests forests;
  std::array<OrderWork, kMaxOrder> work;
  std::array<ForestWork, kMaxOrder> counters{};
  std::array<ForestPhysical, kMaxOrder> physical{};
  Buffer<Slice> slices;
  u64 kernels_ns = 0, contraction_ns = 0, vertical_births_ns = 0, vertical_merges_ns = 0, registry_ns = 0;
  std::array<u64, kStages> admitted{};  // octets admis par etage (admit_stage)
};

// Admission d'un etage par son pilote (MemoryBudget::admit, avant les allocations qu'elle couvre) ; les octets admis
// sont cumules par etage. La porte << admission >> (tests/tower/forest_unit.cpp) joue les etages un par un, budget sans
// cache (tailles exactes), et exige que le pic mesure de chacun, au-dessus de l'usage a son entree, ne depasse pas ce
// cumul (recu audit_registre_branches_20261007 : decalages de la CSR des branches alloues sans admission).
[[nodiscard]] inline Outcome admit_stage(BuildState& state, u32 stage, u64 bytes) noexcept {
  const Outcome admitted = state.budget.admit(bytes);
  if (admitted.ok()) state.admitted[stage] += bytes;
  return admitted;
}

// Octets admis par etage pour un ordre : T (naissances, spheres de la plus large cohorte, noyau, historique) ; M avec
// ne evenements (noeuds au plus naissances + ne).
u64 kernel_bytes(const ForestInput& input) noexcept;
u64 contraction_bytes(const ForestInput& input, u64 events) noexcept;

// Etages de build_forests (forest_build.cpp, forest_stages.cpp), chacun admis dans le budget avant ses allocations.
[[nodiscard]] Outcome run_kernels(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_contraction(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_verticals(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_registry(BuildState& state, sched::Pool& pool) noexcept;
void fill_ledger(const BuildState& state, u32 threads, ForestLedger& ledger) noexcept;

}  // namespace mhgp12::tower::detail
