// Etat du constructeur FULL, interne au module tower ; capacites bornees et tous tableaux budgetes.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::tower_detail {
class RegularVerticalSeeds;
class PopulationLookup;

template<class T, class Less>
void forest_sort(std::span<T> values, Less less) noexcept {
  const auto sift = [&](u64 root, u64 count) noexcept {
    while (root < count / 2) {
      u64 child = 2 * root + 1;
      if (child + 1 < count && less(values[child], values[child + 1])) ++child;
      if (!less(values[root], values[child])) break;
      std::swap(values[root], values[child]); root = child;
    }
  };
  for (u64 i = values.size() / 2; i != 0; --i) sift(i - 1, values.size());
  for (u64 end = values.size(); end > 1; --end) {
    std::swap(values[0], values[end - 1]); sift(0, end - 1);
  }
}

struct ForestState {
  u32 parent, top, head, tail, next;
  bool touched;
};
// Compteurs d'une plage de classification ; leur somme ne depend pas du decoupage.
struct ClassifyCounts {
  u64 classified = 0, births = 0, regular_jobs = 0;
  ClassificationLedger classification;
};
[[nodiscard]] Outcome classify_range(const FullDomain&, u32 k, std::span<u8> kinds, u32 begin, u32 end,
                                     ClassifyCounts&) noexcept;
[[nodiscard]] Outcome add_classify_counts(ClassifyCounts& sum, const ClassifyCounts& part) noexcept;
struct ForestBuilder {
  const FullDomain& domain;
  u32 k;
  MemoryBudget& budget;
  OrderTimings* timings;
  DescentMemo* memo;
  ForestParallel* parallel;
  bool dense_birth_lookup;
  RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population = nullptr;  // facultatif ; aucune decision du DSU n'en depend
  OrderTimings* parallel_timings = nullptr;
  OrderForest result;
  Buffer<u8> kinds;  // 0 hors fenetre, 1 naissance, 2 traces strictes ; B octets.
  Buffer<ForestState> states;
  Buffer<u32> touched;
  u32 touched_count = 0;
  u64 regular_jobs = 0;  // cellules regulieres de jonction (kinds=2, m=qmin) ; voie des ordres concurrents
  CensusWorkspace* extended_scratch = nullptr;  // espace census des cellules etendues, voie concurrente

  ForestBuilder(const FullDomain& d, u32 order, MemoryBudget& b, OrderTimings* t = nullptr,
                DescentMemo* m = nullptr, ForestParallel* p = nullptr, bool dense = false,
                RegularVerticalSeeds* seeds = nullptr) noexcept
      : domain(d), k(order), budget(b), timings(t), memo(m), parallel(p), dense_birth_lookup(dense),
        vertical_seeds(seeds) {}
  Result<OrderForest> run() noexcept;
  Outcome classify() noexcept;
  Outcome adopt(const ClassifyCounts&) noexcept;
  u64 birth_bytes() const noexcept;  // octets que births() reservera ; admission prealable des taches
  Outcome births() noexcept;
  Outcome prepare_states() noexcept;
  Outcome plateaus() noexcept;
  Outcome finish() noexcept;
  // Voie des ordres concurrents (forest_concurrent.cpp) : graines regulieres deja resolues par ordinal.
  Outcome collect_jobs(std::span<BallIdx> jobs) const noexcept;
  Outcome publish(std::span<const BallIdx> jobs, std::span<const NodeIdx> seeds) noexcept;
  Outcome cell(BallIdx) noexcept;
  Outcome regular_cell(BallIdx, std::span<const NodeIdx>) noexcept;
  Outcome regular_work(const DescentLedger& work) noexcept { return add_descent(result.ledger_.descent, work); }
  Outcome regular_plateau() noexcept { return cell_add(result.ledger_.plateaus, 1); }
  Outcome close(LevelRank) noexcept;
  u32 find(u32) noexcept;
  Outcome touch(u32) noexcept;
  Outcome unite_roots(u32& root, u32 other) noexcept;
};

[[nodiscard]] Outcome add_cell_work(CellLedger&, const CellLedger&) noexcept;
// Voie des ordres concurrents (forest_concurrent.cpp), Pool obligatoire : forets de 1..kmax dans orders.
[[nodiscard]] Outcome build_concurrent(const FullDomain&, Order kmax, MemoryBudget&, FullTimings*, ForestParallel&,
                                       sched::Pool&, RegularVerticalSeeds*, const PopulationLookup*, bool dense,
                                       std::array<std::optional<OrderForest>, kMaxMebSites>& orders) noexcept;
// Images basses de chaque ordre (Pool, ordre apres ordre) puis K-1 balayages fermes concurrents.
[[nodiscard]] Outcome concurrent_verticals(const FullDomain&, std::span<OrderForest* const> forests, MemoryBudget&,
                                           ForestParallel&, sched::Pool&, std::span<OrderTimings> times,
                                           const RegularVerticalSeeds*, const PopulationLookup*) noexcept;
[[nodiscard]] Outcome forest_verticals(const FullDomain&, const OrderForest&, OrderForest&, MemoryBudget&,
                                      DescentMemo* = nullptr, ForestParallel* = nullptr,
                                      OrderTimings* = nullptr, const RegularVerticalSeeds* = nullptr,
                                      const PopulationLookup* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
