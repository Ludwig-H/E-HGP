// Session recouverte de la tour (decision D-F2 ; CONTRAT_TOUR.md, paragraphe 4.4 ; ARCHITECTURE.md, paragraphe 4.3) :
// l'etage G et les etages T, M, V, R dans UNE seule region du Pool (build_tower, pipeline.cpp ; region,
// pipeline_run.cpp). Les index des naissances de tous les ordres sont construits avant la region (un objet par ordre).
// Dans la region, chaque fil prend, dans cet ordre de preference : un noyau pret, une tranche a indicer en avance du
// noyau, un morceau d'une etape prete de la foret, une tranche de G (kCellGrain cellules ; ordres decroissants : la
// foret de l'ordre K est la plus longue).
//   - Tranches de G : memes corps que resolve_orders (passes.cpp), compteurs par (ordre, fil) ; le fil qui finit le
//     calcul d'une tranche note sa fin (fin de G : maximum des fins de calcul, publie par le fil qui acheve le dernier
//     calcul, de l'ordre ou de tous les ordres), calcule ensuite ses feuilles (pre-passe de T, travail de la foret) si
//     la numerotation de l'ordre est finie, puis publie la tranche (drapeau atomique, ecrit une fois, liberation) ;
//   - noyau de l'ordre k : REPRENABLE, il traite les cellules des tranches terminees, dans l'ordre des cellules (donc
//     des rangs) ; arrete sur une tranche non terminee, il rend la main sans bloquer de fil ;
//   - indices de racine (levier I de T2-d-A6) : une tache prend la prochaine tranche, au plus kHintWindow tranches
//     au-dela du noyau, deja resolue avec ses feuilles, et remplace ses feuilles par leurs racines dans l'union-find
//     vivant (hint_leaves) ; sortie du noyau inchangee ; le noyau ne rend son union-find qu'apres la fin des taches en
//     cours (garde hint_closed / hint_active) ;
//   - autres etapes : graphe ci-dessous, une etape part quand tous ses predecesseurs ont reussi.
// Les sorties sont aux places fixees par l'entree, par les sommes prefixes ou par l'ordre des cellules : rien ne depend
// de l'entrelacement des fils. Refus deterministes : toutes les tranches de G sont jouees ; une etape ne part que si ses
// predecesseurs ont reussi ; l'issue rendue est celle de la voie sequentielle (G d'abord, puis le premier rang de
// build_forests en echec, fusion merge), jamais la premiere arrivee.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>

#include "tower/forest_internal.hpp"
#include "tower/stage.hpp"

namespace mhgp12::tower::detail {

// Etapes de la foret d'un ordre i (k = i + 1). Etapes << par morceaux >> : kNumber (kNumberItems naissances),
// kNumberClose (kNumberItems positions), kHistoryCheck (kHistoryItems evenements), kDepth (kHistoryItems noeuds),
// kClasses, kNodes, kParents, kChildren (une tranche de contraction par morceau), kBirths, kMerges (kVerticalItems
// noeuds), kCollect, kFill (kRowItems lignes).
enum Step : u8 {
  kCheck = 0,     // controle de l'entree
  kNumberOpen,    // N : tampons de la numerotation
  kNumber,        // N : cohortes qui commencent dans le morceau, triees
  kNumberClose,   // N : noeud et cle de chaque naissance
  kKernelOpen,    // tampon des feuilles, ouverture du noyau
  kKernel,        // noyau reprenable, puis cloture
  kHistoryCheck,  // H : controle des evenements (en meme temps que la contraction)
  kDepth,         // H : profondeur d'attache maximale
  kHistory,       // H : evenements par survivant
  kSlices,     // tranches de la contraction
  kClasses,    // phase A
  kNodes0,     // premiers noeuds, noeuds
  kNodes,      // phase B
  kParents,    // phase C
  kPlace,      // decalages des enfants
  kChildren,   // phase D
  kFinish,     // sommets des cellules, racine unique
  kLower,      // V : verticales allouees (k >= 2)
  kBirths,     // V : images des naissances (k >= 2)
  kMerges,     // V : images des fusions (k >= 2)
  kRows,       // R : evenements liberes, lignes
  kCollect,    // R : passe 1
  kPlaceRows,  // R : decalages et sortie
  kFill,       // R : passe 2, puis liberation du travail
  kStepCount
};

inline constexpr u64 kNumberItems = 16384;   // naissances (ou positions) par morceau de la numerotation
inline constexpr u64 kHistoryItems = 65536;  // evenements ou noeuds par morceau de l'historique
inline constexpr u64 kHintWindow = 32;       // tranches indicees au plus en avance du noyau
inline constexpr u64 kVerticalItems = 8192;  // noeuds par morceau de V
inline constexpr u64 kRowItems = 2048;       // lignes par morceau de R
inline constexpr u32 kSessionSliceEvents = 4096;  // evenements par tranche de contraction dans la Session
inline constexpr u32 kRefusalRanks = 8;      // rangs de refus de la voie sequentielle (refusal_rank)

// Predecesseurs d'une etape : au plus deux, au meme ordre ou a l'ordre inferieur (below). Aux etapes de V de l'ordre 1,
// aucune : elles n'existent pas (terminees des le depart).
struct StepEdge {
  Step step;
  bool below;
};
struct StepDeps {
  u32 count;
  StepEdge edge[2];
};
StepDeps step_dependencies(Step step) noexcept;
// Rang de refus de la voie sequentielle (build_forests) : 0 controle, 1 numerotation, 2 feuilles, 3 noyau et
// historique, 4 contraction, 5 naissances de V, 6 fusions de V, 7 registre.
u32 refusal_rank(Step step) noexcept;

// Etat partage d'une etape. pending : predecesseurs non termines ; items : morceaux (fixes par le predecesseur avant
// de liberer l'etape) ; next, finished : morceaux reclames, termines ; flags : kStepDone, kStepFailed, kStepBusy.
inline constexpr u32 kStepDone = 1, kStepFailed = 2, kStepBusy = 4;
struct StepState {
  std::atomic<u32> pending{0};
  std::atomic<u64> items{1};
  std::atomic<u64> next{0};
  std::atomic<u64> finished{0};
  std::atomic<u32> flags{0};
};

// Drapeaux d'une tranche de G (un octet, ecrit une fois avec liberation) : resolue, en echec, feuilles calculees.
inline constexpr u8 kSliceDone = 1, kSliceFailed = 2, kSliceLeaves = 4;

// Accumulateurs d'un fil : issues par rang de refus, temps-fils (physiques) de G et de la foret, taches d'indices et
// feuilles indicees.
struct WorkerTotals {
  Outcome g_outcome;
  std::array<Outcome, kRefusalRanks> forest_outcome{};
  u64 g_ns = 0, forest_ns = 0, forest_after_g_ns = 0, kernel_jobs = 0, kernel_stops = 0, hint_jobs = 0, hinted = 0;
};

// Etat de la Session recouverte : domaine de G, sorties, index des naissances par ordre, etat de la foret, graphe.
struct Pipeline {
  Pipeline(const tower_detail::Domain& d, Resolution& r, BuildState& f, tower_detail::Workers& t, u32 workers) noexcept
      : domain(d), out(r), forest(f), team(t), threads(workers) {}
  const tower_detail::Domain& domain;
  Resolution& out;
  BuildState& forest;
  tower_detail::Workers& team;  // espaces de census par fil
  u32 threads;
  u32 orders = 0;
  std::array<tower_detail::PopulationTable, kMaxOrder> tables;  // index des naissances, ordre k a la case k - 1
  // Tranches de G : par ordre, nombre, debut dans la suite globale (ordres decroissants), drapeaux.
  std::array<u64, kMaxOrder> g_items{}, g_start{};
  std::array<Buffer<u8>, kMaxOrder> g_flags;
  u64 g_total = 0;
  std::atomic<u64> g_next{0};
  std::atomic<bool> g_failed{false};
  std::array<std::atomic<u64>, kMaxOrder> kernel_slice{};  // prochaine tranche du noyau de l'ordre
  // Indices (levier I) : prochaine tranche a indicer ; taches en cours ; noyau clos (plus aucune tache ne demarre, et
  // la cloture attend la fin de celles en cours avant de rendre l'union-find, les elements et les feuilles).
  std::array<std::atomic<u64>, kMaxOrder> hint_next{};
  std::array<std::atomic<u32>, kMaxOrder> hint_active{};
  std::array<std::atomic<bool>, kMaxOrder> hint_closed{};
  std::array<std::array<StepState, kStepCount>, kMaxOrder> steps;
  // Compteurs de G par (ordre, fil), case i * threads + w ; profils (MHGP12_TOWER_PROFILE) ; compteurs et durees de
  // la foret des morceaux par (ordre, fil).
  Buffer<OrderCounters> g_counters;
  Buffer<SectionCycles> g_profiles;
  Buffer<ForestWork> f_counters;
  Buffer<ForestPhysical> f_physical;
  Buffer<WorkerTotals> totals;
  std::atomic<u32> in_flight{0};
  // Epoque : incrementee APRES tout ce qui rend un travail reclamable (tranche publiee, etape terminee, noyau rendu) ;
  // un fil sans travail n'explore a nouveau que si elle change (ou si plus aucun travail n'est en cours).
  std::atomic<u64> epoch{0};
  // Fin du dernier calcul de G depuis le debut de la region (0 : pas encore) : g_max recoit la fin de chaque calcul
  // (maximum atomique) AVANT le comptage g_computed ; le fil qui acheve le dernier calcul publie g_max dans g_end_ns.
  // La pre-passe des feuilles qui suit un calcul n'en fait pas partie.
  std::atomic<u64> g_end_ns{0}, g_max{0}, g_computed{0};
  // Instants (ns depuis le debut de la region) par ordre : fin du dernier calcul de G (meme protocole, g_done et
  // order_g_max), du noyau, de la contraction, de V, de R.
  std::array<std::atomic<u64>, kMaxOrder> g_done{}, order_g_max{}, order_g_end{}, order_kernel_end{}, order_m_end{},
      order_v_end{}, order_r_end{};
  std::chrono::steady_clock::time_point start;
};

// Session d'une trame en trois temps (build_tower les enchaine ; la porte << admission >> de tests/tower/
// pipeline_unit.cpp mesure le pic entre le deuxieme et la fin) : open_session ouvre l'etage G (sorties allouees,
// cellules remplies), prepare l'etat de la foret et calcule les octets de la region sans rien admettre ;
// admit_session admet ces octets d'un coup, puis alloue espaces des fils, accumulateurs et index des naissances ;
// play_session joue la region et clot (issues dans l'ordre de la voie sequentielle, grand livre, diagnostics).
struct SessionRun {
  SessionRun(const GlobalIndex& i, const Catalogue& c, MemoryBudget& b, sched::Pool& p) noexcept
      : index(i), catalogue(c), budget(b), pool(p), balls(catalogue_balls(c)) {}
  const GlobalIndex& index;
  const Catalogue& catalogue;
  MemoryBudget& budget;
  sched::Pool& pool;
  const BallSource balls;
  tower_detail::OpenedStage opened;
  std::optional<tower_detail::Domain> domain;
  std::array<ForestInput, kMaxOrder> inputs{};
  ForestParams params;
  std::unique_ptr<BuildState> forest;
  tower_detail::Workers team;
  std::unique_ptr<Pipeline> pipeline;
  TowerDiagnostics diag;
};
// open_session n'est PAS noexcept : ses deux std::make_unique (BuildState, Pipeline) peuvent lever std::bad_alloc, que
// la frontiere publique build_tower (noexcept, guarded) convertit en memory_budget apres avoir rendu les tampons
// (relecture de l'auditeur Codex, prelecture_t2d_corps ; porte mhgp12_tower_pipeline_fault). Les deux autres temps
// n'allouent que par le budget (Buffer, CensusWorkspace::make : new sans exception).
[[nodiscard]] Outcome open_session(SessionRun& run);
[[nodiscard]] Outcome admit_session(SessionRun& run) noexcept;
[[nodiscard]] Outcome play_session(SessionRun& run) noexcept;

// Graphe initial : predecesseurs comptes, etapes absentes (V de l'ordre 1) terminees, tranches de G numerotees.
void prepare_graph(Pipeline& p) noexcept;
// Region : un corps par fil (Pool::parallel_for(threads, 1, &p, run_region)), jusqu'a ce que plus rien ne soit
// reclamable ni en cours. Les issues sont dans p.totals (jamais rendues par le corps).
Outcome run_region(void* pipeline, u64 begin, u64 end, u32 worker) noexcept;
// Une etape est-elle terminee ? (lecture avec acquisition)
bool step_done(const Pipeline& p, u32 order, Step step) noexcept;

// Corps des etapes de la foret (pipeline_steps.cpp) : un morceau `item` de l'etape `step` de l'ordre i par le fil w ;
// rang de refus de la voie sequentielle dans rank. Duree d'un morceau imputee a l'etage de son etape (diagnostic
// physique par ordre et par fil). Taches d'indices : tranche `slice` de l'ordre i, si l'union-find vit encore (garde) ;
// rend le nombre de feuilles indicees.
[[nodiscard]] Outcome step_body(Pipeline& p, u32 w, u32 i, Step step, u64 item, u32& rank) noexcept;
void charge_stage(Pipeline& p, u32 w, u32 i, Step step, u64 ns) noexcept;
u64 run_hint(Pipeline& p, u32 i, u64 slice) noexcept;
// Outils communs : instant depuis le debut de la region ; compteurs et durees d'un fil pour un ordre ; morceaux.
u64 now_ns(const Pipeline& p) noexcept;
ForestWork& work_of(Pipeline& p, u32 i, u32 w) noexcept;
ForestPhysical& physical_of(Pipeline& p, u32 i, u32 w) noexcept;
inline u64 piece_end(u64 item, u64 size, u64 count) noexcept { return std::min(count, (item + 1) * size); }
inline u64 pieces(u64 count, u64 size) noexcept { return (count + size - 1) / size; }
inline void set_items(Pipeline& p, u32 i, Step s, u64 items) noexcept {
  p.steps[i][s].items.store(items, std::memory_order_relaxed);
}

#ifdef MHGP12_REGION_HOOKS
// Porte native de terminaison (CST-0241 ; tests/tower/region_unit.cpp) : points d'observation de run_region, fournis
// par la seule cible de test qui compile pipeline_run.cpp avec cette macro ; region_hooks_build marque ce corps-la.
inline constexpr int kHookAnnounce = 0, kHookWithdrawn = 1, kHookWait = 2;
void region_hook(int point, u32 worker) noexcept;
extern const int region_hooks_build;
#endif

}  // namespace mhgp12::tower::detail
