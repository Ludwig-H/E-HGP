// Lots bornes de cellules REGULIERES ; sorties privees et DSU seulement sur le pilote apres join.
#pragma once
#include <algorithm>
#include "tower/forest_internal.hpp"
#include "sched/sched.hpp"
#include "tower/census_slots.hpp"

namespace mhgp11::tower_detail {
inline constexpr u32 kRegularBatchLimit = 4096;

// Voie des ordres concurrents : cellules regulieres de jonction d'UN ordre, ordinaux locaux croissants.
struct OrderJobs {
  ForestBuilder* builder = nullptr;
  std::span<const BallIdx> jobs;
  std::span<NodeIdx> seeds;  // 4 cases par cellule (q<=4), ordinal local
  u64 first = 0;             // ordinal global de la premiere cellule ; ordres concatenes par k croissant
};

// Contexte emprunte par build_forest, possede par build_full. Aucun deplacement du domaine avant destruction.
// Une lane logique garde son memo sur TOUS les ordres, independamment du worker qui l'execute.
class ForestParallel {
 public:
  ForestParallel(const ForestParallel&) = delete;
  ForestParallel& operator=(const ForestParallel&) = delete;
  ForestParallel& operator=(ForestParallel&&) = delete;
  ForestParallel(ForestParallel&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), budget_(std::exchange(other.budget_, nullptr)),
        pool_(std::exchange(other.pool_, nullptr)), scratch_(std::exchange(other.scratch_, nullptr)), lanes_(std::exchange(other.lanes_, 0)),
        count_(std::exchange(other.count_, 0)), next_lane_(std::exchange(other.next_lane_, 0)),
        memo_bytes_(std::exchange(other.memo_bytes_, 0)), place_pipeline_(std::exchange(other.place_pipeline_, false)),
        jobs_(std::move(other.jobs_)),
        work_(std::move(other.work_)), memos_(std::move(other.memos_)) {}
  static Outcome validate(FullParams, sched::Pool*) noexcept;
  // Espaces census physiques : un par tache simultanee. Lots : min(W,lanes,Q) ; ordres concurrents : la
  // distribution unique lance jusqu'a min(lanes,cellules) taches, d'ou min(W,lanes) independamment de Q.
  static u32 census_workspaces(FullParams p, const sched::Pool* pool) noexcept {
    if (!p.reuse_census_workspace) return 0;
    if (p.regular_batch_capacity == 0 || pool == nullptr) return 1;
    if (p.concurrent_orders) return std::min(pool->size(), p.descent_lanes);
    return std::min({pool->size(), p.descent_lanes, p.regular_batch_capacity});
  }
  static Result<ForestParallel> make(const FullDomain&, FullParams, MemoryBudget&, sched::Pool&,
                                     CensusSlots* = nullptr) noexcept;
  bool belongs_to(const FullDomain& d, const MemoryBudget& b) const noexcept { return domain_ == &d && budget_ == &b; }
  Outcome run(ForestBuilder&) noexcept;
  Outcome verticals(const OrderForest& lower, OrderForest& upper, OrderTimings*,
                    const RegularVerticalSeeds* = nullptr, const PopulationLookup* = nullptr) noexcept;
  // Toutes les cellules regulieres de tous les ordres en UNE distribution : l'ordinal global g va a la lane
  // (next+g) mod lanes, chaque lane suit g croissant avec son memo ; lane_work recoit lanes*ordres ledgers.
  Outcome resolve_orders(std::span<const OrderJobs> orders, std::span<DescentLedger> lane_work) noexcept;
  // Images basses des naissances de tous les ordres hauts de forests (lower_ deja alloue), sans memo.
  Outcome vertical_images(std::span<OrderForest* const> forests, const RegularVerticalSeeds*,
                          const PopulationLookup*, std::span<OrderTimings> times) noexcept;
  u32 lanes() const noexcept { return lanes_; }
  CensusWorkspace* census_slot(u32 worker) const noexcept {
    return scratch_ == nullptr ? nullptr : scratch_->get(worker);
  }
  u32 census_slots() const noexcept { return scratch_ == nullptr ? 0 : scratch_->size(); }
  // Pipeline (forest_pipeline.cpp) : une cellule reguliere, sans memo de lane, sur l'espace census fourni.
  Outcome resolve_regular(ForestBuilder& builder, BallIdx ball, std::array<NodeIdx, 4>& seeds,
                          CensusWorkspace* scratch, DescentLedger& work) noexcept {
    return resolve_job(builder, ball, seeds, nullptr, scratch, work);
  }
  u64 memo_bytes() const noexcept { return memo_bytes_; }
  bool place_pipeline() const noexcept { return place_pipeline_; }

 private:
  struct Job { BallIdx ball{kNone}; std::array<NodeIdx, 4> seeds{}; };
  struct Lane { DescentLedger work; u64 nanoseconds = 0; };
  ForestParallel(const FullDomain& d, MemoryBudget& b, sched::Pool& p, u32 lanes) noexcept
      : domain_(&d), budget_(&b), pool_(&p), lanes_(lanes) {}
  Outcome resolve(ForestBuilder&, u32 slot, u32 worker) noexcept;
  Outcome resolve_job(ForestBuilder&, BallIdx, std::array<NodeIdx, 4>&, DescentMemo*, CensusWorkspace*,
                      DescentLedger&) noexcept;
  Outcome flush(ForestBuilder&, std::optional<LevelRank>& active) noexcept;
  Outcome select(ForestBuilder&, LevelRank, std::optional<LevelRank>& active) noexcept;
  struct Dispatch;
  struct VerticalDispatch;
  struct OrdersDispatch;
  struct ImagesDispatch;
  Outcome resolve_lane(std::span<const OrderJobs>, std::span<DescentLedger>, u64 total, u32 slot,
                       u32 worker) noexcept;
  Outcome vertical_lane(const OrderForest&, OrderForest&, u32 begin, u32 count, u32 slot, u32 worker,
                        bool timed, bool reused, const PopulationLookup* population) noexcept;
  const FullDomain* domain_;
  MemoryBudget* budget_;
  sched::Pool* pool_;
  CensusSlots* scratch_ = nullptr;
  u32 lanes_, count_ = 0, next_lane_ = 0;
  u64 memo_bytes_ = 0;
  bool place_pipeline_ = false;  // FullParams::place_pipeline
  Buffer<Job> jobs_;
  Buffer<Lane> work_;
  std::array<std::optional<DescentMemo>, sched::kMaxWorkers> memos_;
};
}  // namespace mhgp11::tower_detail
