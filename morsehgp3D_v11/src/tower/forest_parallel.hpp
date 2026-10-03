// Lots bornes de cellules REGULIERES ; sorties privees et DSU seulement sur le pilote apres join.
#pragma once
#include "tower/forest_internal.hpp"
#include "sched/sched.hpp"

namespace mhgp11::tower_detail {
inline constexpr u32 kRegularBatchLimit = 4096;

// Contexte emprunte par build_forest, possede par build_full. Aucun deplacement du domaine avant destruction.
// Une lane logique garde son memo sur TOUS les ordres, independamment du worker qui l'execute.
class ForestParallel {
 public:
  ForestParallel(const ForestParallel&) = delete;
  ForestParallel& operator=(const ForestParallel&) = delete;
  ForestParallel& operator=(ForestParallel&&) = delete;
  ForestParallel(ForestParallel&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), budget_(std::exchange(other.budget_, nullptr)),
        pool_(std::exchange(other.pool_, nullptr)), lanes_(std::exchange(other.lanes_, 0)),
        count_(std::exchange(other.count_, 0)), next_lane_(std::exchange(other.next_lane_, 0)),
        memo_bytes_(std::exchange(other.memo_bytes_, 0)), jobs_(std::move(other.jobs_)),
        work_(std::move(other.work_)), memos_(std::move(other.memos_)) {}
  static Outcome validate(FullParams, sched::Pool*) noexcept;
  static Result<ForestParallel> make(const FullDomain&, FullParams, MemoryBudget&, sched::Pool&) noexcept;
  bool belongs_to(const FullDomain& d, const MemoryBudget& b) const noexcept { return domain_ == &d && budget_ == &b; }
  Outcome run(ForestBuilder&) noexcept;
  u64 memo_bytes() const noexcept { return memo_bytes_; }

 private:
  struct Job { BallIdx ball{kNone}; std::array<NodeIdx, 4> seeds{}; };
  struct Lane { DescentLedger work; u64 nanoseconds = 0; };
  ForestParallel(const FullDomain& d, MemoryBudget& b, sched::Pool& p, u32 lanes) noexcept
      : domain_(&d), budget_(&b), pool_(&p), lanes_(lanes) {}
  Outcome resolve(ForestBuilder&, u32 slot) noexcept;
  Outcome flush(ForestBuilder&, std::optional<LevelRank>& active) noexcept;
  Outcome select(ForestBuilder&, LevelRank, std::optional<LevelRank>& active) noexcept;
  struct Dispatch;
  const FullDomain* domain_;
  MemoryBudget* budget_;
  sched::Pool* pool_;
  u32 lanes_, count_ = 0, next_lane_ = 0;
  u64 memo_bytes_ = 0;
  Buffer<Job> jobs_;
  Buffer<Lane> work_;
  std::array<std::optional<DescentMemo>, sched::kMaxWorkers> memos_;
};
}  // namespace mhgp11::tower_detail
