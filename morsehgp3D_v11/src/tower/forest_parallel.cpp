// Anticiper les descentes est licite : elles ne lisent jamais le DSU, seulement le domaine immobile.
#include "tower/forest_parallel.hpp"
#include <algorithm>
#include <limits>

namespace mhgp11::tower_detail {

Outcome ForestParallel::validate(FullParams p, sched::Pool* pool) noexcept {
  if (p.regular_batch_capacity == 0)
    return p.descent_lanes == 1 && p.lane_memo_capacity == 0 && !p.parallel_verticals ?
           Outcome{} : fail(Reason::parameter_out_of_range);
  if (pool == nullptr || p.regular_batch_capacity > kRegularBatchLimit || p.descent_lanes == 0 ||
      p.descent_lanes > sched::kMaxWorkers ||
      (p.lane_memo_capacity != 0 && (p.lane_memo_capacity & (p.lane_memo_capacity - 1)) != 0))
    return fail(Reason::parameter_out_of_range);
  return {};
}

Result<ForestParallel> ForestParallel::make(const FullDomain& domain, FullParams p, MemoryBudget& budget,
                                           sched::Pool& pool, CensusSlots* scratch) noexcept {
  MHGP11_TRY(validate(p, &pool));
  if (p.regular_batch_capacity == 0) return fail(Reason::parameter_out_of_range);
  const u64 width = DescentMemo::slot_bytes();
  if (p.lane_memo_capacity > std::numeric_limits<u64>::max() / width / p.descent_lanes)
    return fail(Reason::tower_capacity);
  if (p.reuse_census_workspace != (scratch != nullptr)) return fail(Reason::parameter_out_of_range);
  if (scratch != nullptr && (!scratch->belongs_to(domain, budget) ||
      scratch->size() != std::min({pool.size(), p.descent_lanes, p.regular_batch_capacity})))
    return fail(Reason::parameter_out_of_range);
  ForestParallel result(domain, budget, pool, p.descent_lanes);
  result.scratch_ = scratch;
  result.memo_bytes_ = p.lane_memo_capacity * width * p.descent_lanes;
  u64 bytes = result.memo_bytes_;
  MHGP11_TRY(cell_add(bytes, u64{p.regular_batch_capacity} * sizeof(Job)));
  MHGP11_TRY(cell_add(bytes, u64{p.descent_lanes} * sizeof(Lane)));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.jobs_.allocate(p.regular_batch_capacity, budget));
  MHGP11_TRY(result.work_.allocate(p.descent_lanes, budget));
  for (u32 i = 0; i < p.descent_lanes; ++i) if (p.lane_memo_capacity != 0) {
    auto memo = DescentMemo::make(domain, p.lane_memo_capacity, budget);
    if (!memo.ok()) return memo.outcome();
    result.memos_[i].emplace(std::move(memo.value()));
  }
  return result;
}

Outcome ForestParallel::resolve(ForestBuilder& builder, u32 slot, u32 worker) noexcept {
  const u32 physical = std::min(lanes_, count_) < pool_->size() ? slot : worker;
  CensusWorkspace* scratch = scratch_ == nullptr ? nullptr : scratch_->get(physical);
  if (scratch_ != nullptr && scratch == nullptr) return fail(Reason::tower_invariant);
  const u32 lane = (next_lane_ + slot) % lanes_;
  auto& work = work_[slot]; work = {};
  std::optional<Stopwatch> clock;
  if (builder.timings != nullptr) clock.emplace();
  DescentMemo* memo = memos_[lane] ? &*memos_[lane] : nullptr;
  for (u32 ordinal = slot; ordinal < count_; ordinal += lanes_) {
    auto& job = jobs_[ordinal];
    const auto& data = domain_->catalogue().balls_data()[idx(job.ball)];
    const auto inner = domain_->catalogue().interior(job.ball), shell = domain_->catalogue().shell(job.ball);
    if (data.m != data.qmin || data.qmin < 2 || data.qmin > 4 ||
        builder.k != u64{data.p} + data.qmin - 1) return fail(Reason::tower_invariant);
    // t=q-1. Le sommet omis decroit : ordre lexicographique EXACT de build_cell.
    for (u32 trace = 0; trace < data.qmin; ++trace) {
      const u32 omitted = data.qmin - 1 - trace;
      std::array<SiteIdx, 4> face{}; u32 used = 0;
      for (u32 j = 0; j < data.qmin; ++j) if (j != omitted) face[used++] = shell[j];
      std::array<SiteIdx, kMaxMebSites> part{};
      std::merge(inner.begin(), inner.end(), face.begin(), face.begin() + used, part.begin(),
                 [](SiteIdx a, SiteIdx b) noexcept { return idx(a) < idx(b); });
      auto down = resolve_descent(*domain_, {part.data(), builder.k}, builder.k, *budget_, memo, scratch);
      if (!down.ok()) return down.outcome();
      const auto& level = domain_->catalogue().levels()[idx(data.rank)];
      if (num::compare(down.value().initial_level(), level) >= 0) return fail(Reason::tower_invariant);
      const auto seed = builder.result.birth_node(down.value().seed());
      if (!seed || idx(builder.result.nodes()[idx(*seed)].rank) >= idx(data.rank))
        return fail(Reason::tower_invariant);
      job.seeds[trace] = *seed;
      MHGP11_TRY(add_descent(work.work, down.value().ledger()));
    }
  }
  if (clock) work.nanoseconds = clock->nanoseconds();
  return {};
}

struct ForestParallel::Dispatch {
  ForestParallel& self;
  ForestBuilder& builder;
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& dispatch = *static_cast<Dispatch*>(context);
    if (end > std::min(dispatch.self.lanes_, dispatch.self.count_)) return fail(Reason::tower_invariant);
    for (u64 i = begin; i < end; ++i) MHGP11_TRY(dispatch.self.resolve(dispatch.builder, static_cast<u32>(i), worker));
    return {};
  }
};

Outcome ForestParallel::select(ForestBuilder& builder, LevelRank level,
                               std::optional<LevelRank>& active) noexcept {
  if (active && *active != level) {
    if (idx(level) < idx(*active)) return fail(Reason::tower_invariant);
    MHGP11_TRY(builder.close(*active));
    active.reset();
  }
  if (!active) {
    active = level;
    // Compteur historique : une seule fermeture pour tout le plateau, y compris lots et etendues melees.
    return builder.regular_plateau();
  }
  return {};
}

Outcome ForestParallel::flush(ForestBuilder& builder, std::optional<LevelRank>& active) noexcept {
  if (count_ == 0) return {};
  const u32 jobs = std::min(lanes_, count_), concurrent = std::min(pool_->size(), jobs);
  // Un LocatedPart a la fois par lane active ; ses I/U sont disjoints et totalisent au plus n sites.
  // Descente/MEB n'allouent aucun autre Buffer. Aucun autre pilote n'alloue durant cet appel Pool.
  if (scratch_ == nullptr)
    MHGP11_TRY(budget_->admit(4 * u64{domain_->index().cloud().sites()} * concurrent));
  Dispatch dispatch{*this, builder};
  std::optional<Stopwatch> clock;
  if (builder.timings != nullptr) clock.emplace();
  MHGP11_TRY(pool_->parallel_for(jobs, 1, &dispatch, Dispatch::body));
  auto& timing = *builder.parallel_timings;
  if (clock) MHGP11_TRY(cell_add(timing.regular_dispatch_ns, clock->nanoseconds()));
  if (builder.timings != nullptr) clock.emplace();
  for (u32 i = 0; i < jobs; ++i) {
    MHGP11_TRY(builder.regular_work(work_[i].work));
    MHGP11_TRY(cell_add(timing.regular_task_sum_ns, work_[i].nanoseconds));
    timing.regular_task_max_ns = std::max(timing.regular_task_max_ns, work_[i].nanoseconds);
  }
  for (u32 i = 0; i < count_; ++i) {
    const auto& job = jobs_[i];
    const auto& data = domain_->catalogue().balls_data()[idx(job.ball)];
    MHGP11_TRY(select(builder, data.rank, active));
    MHGP11_TRY(builder.regular_cell(job.ball, {job.seeds.data(), data.qmin}));
    MHGP11_TRY(cell_add(timing.regular_traces, data.qmin));
  }
  MHGP11_TRY(cell_add(timing.regular_batches, 1));
  MHGP11_TRY(cell_add(timing.regular_cells, count_));
  timing.max_regular_batch = std::max(timing.max_regular_batch, u64{count_});
  next_lane_ = (next_lane_ + count_) % lanes_;
  count_ = 0;
  if (clock) MHGP11_TRY(cell_add(timing.regular_publish_ns, clock->nanoseconds()));
  return {};
}

Outcome ForestParallel::run(ForestBuilder& builder) noexcept {
  if (!belongs_to(builder.domain, builder.budget) || count_ != 0 || builder.parallel_timings == nullptr)
    return fail(Reason::parameter_out_of_range);
  std::optional<LevelRank> active;
  const auto balls = domain_->catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b) if (builder.kinds[b] == 2) {
    if (balls[b].m == balls[b].qmin) {
      auto& job = jobs_[count_++]; job.ball = BallIdx{b}; job.seeds.fill(NodeIdx{kNone});
      if (count_ == jobs_.size()) MHGP11_TRY(flush(builder, active));
    } else {
      MHGP11_TRY(flush(builder, active));
      std::optional<Stopwatch> clock;
      if (builder.timings != nullptr) clock.emplace();
      MHGP11_TRY(select(builder, balls[b].rank, active));
      MHGP11_TRY(builder.cell(BallIdx{b}));  // Voie etendue complete, aucune troncation ni nouveau curseur combinatoire.
      MHGP11_TRY(cell_add(builder.parallel_timings->extended_cells, 1));
      if (clock) MHGP11_TRY(cell_add(builder.parallel_timings->extended_ns, clock->nanoseconds()));
    }
  }
  MHGP11_TRY(flush(builder, active));
  if (active) MHGP11_TRY(builder.close(*active));
  return {};
}

}  // namespace mhgp11::tower_detail
