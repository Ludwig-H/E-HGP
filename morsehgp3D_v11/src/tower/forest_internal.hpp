// Etat du constructeur FULL, interne au module tower ; capacites bornees et tous tableaux budgetes.
#pragma once
#include "tower/forest.hpp"

namespace mhgp11::tower_detail {

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
struct ForestBuilder {
  const FullDomain& domain;
  u32 k;
  MemoryBudget& budget;
  OrderTimings* timings;
  DescentMemo* memo;
  ForestParallel* parallel;
  OrderTimings* parallel_timings = nullptr;
  OrderForest result;
  Buffer<u8> kinds;  // 0 hors fenetre, 1 naissance, 2 traces strictes ; B octets.
  Buffer<ForestState> states;
  Buffer<u32> touched;
  u32 touched_count = 0;

  ForestBuilder(const FullDomain& d, u32 order, MemoryBudget& b, OrderTimings* t = nullptr,
                DescentMemo* m = nullptr, ForestParallel* p = nullptr) noexcept
      : domain(d), k(order), budget(b), timings(t), memo(m), parallel(p) {}
  Result<OrderForest> run() noexcept;
  Outcome classify() noexcept;
  Outcome births() noexcept;
  Outcome plateaus() noexcept;
  Outcome cell(BallIdx) noexcept;
  Outcome regular_cell(BallIdx, std::span<const NodeIdx>) noexcept;
  Outcome regular_work(const DescentLedger& work) noexcept { return add_descent(result.ledger_.descent, work); }
  Outcome regular_plateau() noexcept { return cell_add(result.ledger_.plateaus, 1); }
  Outcome close(LevelRank) noexcept;
  u32 find(u32) noexcept;
  Outcome touch(u32) noexcept;
  Outcome unite(u32, u32) noexcept;
};

[[nodiscard]] Outcome add_cell_work(CellLedger&, const CellLedger&) noexcept;
[[nodiscard]] Outcome forest_verticals(const FullDomain&, const OrderForest&, OrderForest&, MemoryBudget&,
                                      DescentMemo* = nullptr, ForestParallel* = nullptr,
                                      OrderTimings* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
