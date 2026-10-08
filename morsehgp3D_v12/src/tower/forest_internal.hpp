// Interne aux etages T, M, V et R (forest.hpp) : evenements binaires du noyau, etat de travail d'un ordre, etapes PAR
// ORDRE partagees par les deux pilotes : build_forests (etage par etage sur tous les ordres, forest_build.cpp,
// forest_stages.cpp, registry_branches.cpp) et la Session recouverte (pipeline.cpp, decision D-F2). Port des
// structures du prototype de MES-M4.
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

// Tranche de la contraction d'un ordre : evenements [lo, hi), alignes sur les rangs ; ses classes et ses noeuds.
struct Slice {
  u32 lo = 0, hi = 0;
  u32 classes = 0, node0 = 0;
  u64 children = 0, child0 = 0;
  u64 ns = 0;                  // duree des quatre phases (diagnostic physique)
};

// Etat de travail d'un ordre entre T et R (tampons du budget, rendus au plus tard a la fin du pilote).
struct OrderWork {
  Buffer<u32> birth_order;     // position canonique -> indice d'entree de la naissance
  Buffer<u32> leaves;          // par representant : feuille canonique, ou cible << cellule >> (pre-passe de T)
  // Noyau reprenable (open_kernel, advance_kernel, close_kernel) : union-find, element de chaque cellule traitee,
  // prochaine cellule a traiter.
  Buffer<UnionCell> cells;
  Buffer<u32> element;
  u64 next_cell = 0;
  Buffer<Event> events;        // capacite naissances - 1 ; event_count ecrits
  u32 event_count = 0;
  Buffer<u32> cell_top;        // par cellule : sommet apres traitement (feuille, ou evenement | kEventBit)
  // Contraction : tranches de l'ordre ; union-find local, plus petite feuille et cle par classe.
  Buffer<Slice> slices;
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
// Morceau [begin, end) de la meme numerotation (levier N de T2-d-A6) : order_out des cohortes qui COMMENCENT dans la
// plage (une cohorte qui la deborde est triee ici en entier ; celle qui y entre en venant d'avant appartient au morceau
// precedent), meme tri exact, memes refus ; compteurs de cohortes dans work ; tampon de spheres propre au morceau.
[[nodiscard]] Outcome number_births_range(const Cloud& cloud, const BallSource& balls, const ForestInput& input,
                                          std::span<u32> order_out, u64 begin, u64 end, ForestWork& work,
                                          MemoryBudget& budget) noexcept;

// T, pre-passe (parallele) sur les cellules [begin, end) : feuille canonique de chaque representant, ou son codage
// << cellule >> ; date de LEM-T3 controlee. Refus tower_invariant (cible hors domaine ou non anterieure).
[[nodiscard]] Outcome resolve_leaves(const ForestInput& input, const OrderForest& forest, std::span<u32> leaves,
                                     u64 begin, u64 end, ForestWork& counters) noexcept;

// T : noyau d'un ordre (feuilles = naissances canoniques, de la pre-passe), REPRENABLE par cellules croissantes :
// open_kernel alloue et initialise l'union-find, les evenements et l'historique d'attache ; advance_kernel traite les
// cellules [work.next_cell, end_cell) (leurs feuilles doivent etre pretes) ; close_kernel exige la racine unique,
// publie le nombre d'evenements et rend l'union-find. Refus tower_invariant (racines multiples, feuilles absentes),
// tower_capacity, memory_budget. run_kernel enchaine les trois sur toutes les cellules.
[[nodiscard]] Outcome open_kernel(const ForestInput& input, OrderForest& forest, OrderWork& work,
                                  MemoryBudget& budget) noexcept;
[[nodiscard]] Outcome advance_kernel(const ForestInput& input, std::span<u32> leaves, OrderForest& forest,
                                     OrderWork& work, ForestWork& counters, u64 end_cell) noexcept;
[[nodiscard]] Outcome close_kernel(const ForestInput& input, OrderForest& forest, OrderWork& work,
                                   ForestWork& counters) noexcept;
[[nodiscard]] Outcome run_kernel(const ForestInput& input, std::span<u32> leaves, OrderForest& forest,
                                 OrderWork& work, ForestWork& counters, MemoryBudget& budget) noexcept;
// Indices de racine (Session recouverte, levier I de T2-d-A6) : feuilles des cellules [begin, end) remplacees par la
// racine de leur composante dans l'union-find vivant (lectures atomiques, sans ecriture de up) ; une cible cellule
// devient la racine de l'element de sa cellule si celle-ci est deja traitee (indice < processed, elements publies par
// le noyau), sinon elle est gardee. Sortie du noyau inchangee (forest_kernel.cpp). Rend le nombre de feuilles
// remplacees. L'union-find, les elements et les feuilles doivent survivre a l'appel (garde de la Session).
u64 hint_leaves(const ForestInput& input, OrderWork& work, u64 begin, u64 end, u64 processed) noexcept;

// T, fin : profondeur d'attache maximale (compteur du travail) et evenements par survivant (CSR de LEM-T5). Lit les
// evenements du noyau, jamais l'etat de la contraction : peut tourner en meme temps qu'elle.
[[nodiscard]] Outcome build_history(OrderForest& forest, const OrderWork& work, ForestWork& counters,
                                    MemoryBudget& budget) noexcept;
// Le meme historique en trois parties (Session recouverte, levier H de T2-d-A6) : controle des evenements
// [begin, end) (perdant dans le domaine, attache au survivant ; refus tower_invariant) ; profondeur d'attache maximale
// des noeuds [begin, end), longueur de la chaine d'attaches saturee a 255 (meme valeur que la passe arriere ; maximum
// dans counters) ; CSR des evenements par survivant (tache seule, apres le controle de tous les evenements).
[[nodiscard]] Outcome check_history(const OrderForest& forest, const OrderWork& work, u64 begin, u64 end) noexcept;
void history_depth(const OrderForest& forest, u64 begin, u64 end, ForestWork& counters) noexcept;
[[nodiscard]] Outcome build_survivor_events(OrderForest& forest, const OrderWork& work,
                                            MemoryBudget& budget) noexcept;

// M, phases de la contraction d'une tranche (forest_contract.cpp). Phase A : classes et cles ; phase B : noeuds,
// rangs, plus petites naissances, noeud de chaque evenement ; phase C : parents et nombres d'enfants ; phase D :
// decalages et enfants tries. Les phases B, C et D lisent les tranches precedentes du meme ordre : chacune attend la
// fin de la precedente pour toutes les tranches de l'ordre.
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

// Etat d'un appel de build_forests ou de la Session recouverte : entrees empruntees, registre en construction, travail
// et compteurs par ordre.
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
// ne evenements (noeuds au plus naissances + ne) ; V (verticales, noeuds au plus nn) ; R au plus (lignes au plus ne,
// representants relus au plus nr, branches au plus les representants relus, sortie comprise).
u64 kernel_bytes(const ForestInput& input) noexcept;
// Taille de la plus large cohorte de naissances de meme rang (1 si aucune).
u64 widest_cohort(const ForestInput& input) noexcept;
u64 contraction_bytes(const ForestInput& input, u64 events) noexcept;
u64 registry_bytes_bound(const ForestInput& input, u64 events) noexcept;

// ---- Etapes par ordre i (k = i + 1), appelees par les deux pilotes ---------------------------------------------------
// T : numerotation canonique des naissances et tampon des feuilles (compteurs de cohortes dans state.counters[i]).
[[nodiscard]] Outcome number_order(BuildState& state, u32 i) noexcept;
// La meme numerotation par morceaux (Session recouverte, levier N de T2-d-A6) : tampons des cles, des noeuds et de
// l'ordre ; morceau [begin, end) des naissances (cohortes qui y commencent, number_births_range ; compteurs de cohortes
// dans counters) ; cloture d'un morceau de positions (noeud et cle de chaque naissance) ; tampon des feuilles.
[[nodiscard]] Outcome open_numbering(BuildState& state, u32 i) noexcept;
[[nodiscard]] Outcome number_piece(BuildState& state, u32 i, u64 begin, u64 end, ForestWork& counters) noexcept;
void close_numbering(BuildState& state, u32 i, u64 begin, u64 end) noexcept;
[[nodiscard]] Outcome open_leaves(BuildState& state, u32 i) noexcept;
// M : tranches de l'ordre (au moins params.slice_events evenements, alignees sur les rangs) et tampons par evenement ;
// apres la phase A, premiers noeuds et noeuds ; apres la phase C, decalages des enfants ; apres la phase D, sommet de
// chaque cellule, racine unique, liberation du travail de la contraction (les evenements restent : l'historique peut
// les lire ; liberes par release_events).
[[nodiscard]] Outcome open_contraction(BuildState& state, u32 i) noexcept;
[[nodiscard]] Outcome allocate_nodes(BuildState& state, u32 i) noexcept;
[[nodiscard]] Outcome place_children(BuildState& state, u32 i) noexcept;
[[nodiscard]] Outcome finish_order(BuildState& state, u32 i) noexcept;
void release_events(BuildState& state, u32 i) noexcept;
// V (i >= 1) : verticales de l'ordre allouees, initialisees a kNone.
[[nodiscard]] Outcome open_verticals(BuildState& state, u32 i) noexcept;
// R : lignes (cellules retenues) et decalages de leurs representants (reads : representants relus) ; passe 1 sur les
// lignes [begin, end) ; decalages exacts et reservation de la sortie ; passe 2 ; liberation du travail.
[[nodiscard]] Outcome prepare_rows(BuildState& state, u32 i, u64& reads) noexcept;
[[nodiscard]] Outcome collect_rows(BuildState& state, u32 i, u64 begin, u64 end, ForestWork& counters) noexcept;
[[nodiscard]] Outcome place_rows(BuildState& state, u32 i) noexcept;
void fill_rows(BuildState& state, u32 i, u64 begin, u64 end) noexcept;
void close_rows(BuildState& state, u32 i) noexcept;
// Fusion d'un compteur du travail dans un total : sommes, et maxima pour max_cohort, max_attach_depth, t5_max_hops.
void add_work(ForestWork& total, const ForestWork& part) noexcept;

// Etages de build_forests (forest_build.cpp, forest_stages.cpp, registry_branches.cpp), chacun admis dans le budget
// avant ses allocations.
[[nodiscard]] Outcome run_kernels(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_contraction(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_verticals(BuildState& state, sched::Pool& pool) noexcept;
[[nodiscard]] Outcome run_registry(BuildState& state, sched::Pool& pool) noexcept;
void fill_ledger(const BuildState& state, u32 threads, ForestLedger& ledger) noexcept;

}  // namespace mhgp12::tower::detail
