// Plan interne de l'etage G : comptes par bloc de boules et par ordre, sommes prefixes, memoire de travail des fils.
#pragma once

#include <memory>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {

inline constexpr u64 kBallBlock = 2048;  // boules par bloc de comptage et de remplissage
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

}  // namespace mhgp12::tower_detail
