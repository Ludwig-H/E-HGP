// Plan interne de l'etage G : comptes par bloc de boules et par ordre, sommes prefixes, memoire de travail des fils ;
// ouverture de l'etage et morceaux des passes, partages par resolve_tower et la Session recouverte (pipeline.cpp).
#pragma once

#include <memory>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {

inline constexpr u64 kBallBlock = 2048;  // boules par bloc de comptage et de remplissage
inline constexpr u64 kCellGrain = 256;   // cellules par tranche de resolution
inline constexpr int kCountFields = 5;
enum CountField : int { kBirths = 0, kCells = 1, kInert = 2, kExtended = 3, kReps = 4 };
// Mots de travail d'un fil : supports d'une coquille, traces d'une cellule et leur copie triee.
inline constexpr u64 kScratchWordsPerWorker = u64{kMaxShellWitnesses} + 2 * kMaxCellCombinations;

struct CellPlan {
  Order orders = 0;
  u64 blocks = 0;
  Buffer<u64> counts;         // blocks * 12 * 5 : comptes d'un bloc
  Buffer<u64> starts;         // idem : premieres places du bloc
  Buffer<u64> windows;        // cellules de fenetre (b, k) d'un bloc, naissances comprises
  Buffer<u64> window_starts;  // premiere place du bloc dans la table des fenetres
  std::array<std::array<u64, kCountFields>, kMaxPart> totals{};
  u64 total_windows = 0;
  CellScratch scratch(std::span<u64> words, std::span<u32> parent, u32 worker) const noexcept;
};

// Comptage exact par blocs, puis sommes prefixes (plan.counts, starts, windows, totals rendus).
[[nodiscard]] Outcome count_cells(const Domain& d, CellPlan& plan, std::span<u64> words, std::span<u32> parent,
                                  sched::Pool& pool) noexcept;
// Remplissage des naissances (k >= 2), cellules, traces et fenetres aux places du plan.
[[nodiscard]] Outcome fill_cells(const Domain& d, const CellPlan& plan, Resolution& out, std::span<u64> words,
                                 std::span<u32> parent, sched::Pool& pool) noexcept;

// Espaces de census, compteurs et profils par fil (passes.cpp).
struct Workers {
  std::array<std::unique_ptr<CensusWorkspace>, sched::kMaxWorkers> census;
  Buffer<OrderCounters> counters;
  Buffer<SectionCycles> profiles;  // construction MHGP12_TOWER_PROFILE seulement
};
// Index des naissances et tampons de la jointure, gardes d'un ordre a l'autre.
struct OrderBuffers {
  PopulationTable table;
  JoinBuffers join;
};
// Ordres 1..orders() : ordre 1, puis index, premieres sondes, passe et controles de chaque ordre k >= 2 ; chronos et
// octets de diag (frontieres : table_ns, join_ns, pass_ns disjoints dans order_ns).
[[nodiscard]] Outcome resolve_orders(const Domain& d, Resolution& out, OrderBuffers& buffers, Workers& workers,
                                     MemoryBudget& budget, sched::Pool& pool, ResolutionDiagnostics& diag) noexcept;

// Morceaux des passes (memes corps que resolve_orders). Ordre 1 : cibles des traces des cellules [begin, end) (les
// sites). Ordre k >= 2 : representants des cellules [begin, end) resolus par le fil worker, compteurs et profils dans
// counters[worker] et profiles[worker] (une case par fil, propres a l'ordre), espace de census du fil. Cloture d'un
// ordre k >= 2 : compteurs des fils fusionnes dans l'ordre des fils, puis controles globaux (check_order).
[[nodiscard]] Outcome first_order_cells(const Catalogue& catalogue, ResolvedOrder& order, u64 begin, u64 end) noexcept;
[[nodiscard]] Outcome resolve_cells(const ResolveContext& context, ResolvedOrder& order, Workers& workers,
                                    std::span<OrderCounters> counters, std::span<SectionCycles> profiles,
                                    const PopulationTable& table, u64 begin, u64 end, u32 worker) noexcept;
[[nodiscard]] Outcome close_order(ResolvedOrder& order, std::span<const OrderCounters> counters) noexcept;

// Etage ouvert : points exacts des sites, puis sorties de tous les ordres allouees et cellules de fenetre remplies
// (naissances, cellules, traces, cibles des fenetres, compteurs de l'objet) ; les cibles des representants restent a
// resoudre. open_stage enchaine controle du catalogue, comptage, admission, reservation et remplissage (refus dans cet
// ordre, rien de publie) ; diag recoit prepare_ns, count_ns, setup_ns, fill_ns.
struct OpenedStage {
  Buffer<num::Point> points;
  Resolution out;
};
[[nodiscard]] Outcome open_stage(const GlobalIndex& index, const Catalogue& catalogue, MemoryBudget& budget,
                                 sched::Pool& pool, ResolutionDiagnostics& diag, OpenedStage& opened) noexcept;
// Espaces de census des fils (memoire admise par l'appelant : workers * sites * sizeof(SiteIdx)), compteurs et
// profils d'un ordre par fil.
[[nodiscard]] Outcome staff_workers(const GlobalIndex& index, u64 workers, MemoryBudget& budget, Workers& team) noexcept;
// Octets des espaces de census, compteurs et profils de `workers` fils (sans l'index des naissances).
u64 workers_bytes(u64 workers, u32 sites) noexcept;

}  // namespace mhgp12::tower_detail
