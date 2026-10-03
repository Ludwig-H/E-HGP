// Anticiper les descentes est licite : elles ne lisent jamais le DSU, seulement le domaine immobile.
#include "tower/forest_parallel.hpp"
#include "tower/population_lookup.hpp"
#include <algorithm>
#include <limits>

namespace mhgp11::tower_detail {

Outcome ForestParallel::validate(FullParams p, sched::Pool* pool) noexcept {
  if (p.regular_batch_capacity == 0)
    return p.descent_lanes == 1 && p.lane_memo_capacity == 0 && !p.parallel_verticals && !p.concurrent_orders ?
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
  if (scratch != nullptr && (!scratch->belongs_to(domain, budget) || scratch->size() != census_workspaces(p, &pool)))
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

// Les q faces d'une cellule reguliere, sommet omis decroissant : ordre lexicographique EXACT de build_cell.
Outcome ForestParallel::resolve_job(ForestBuilder& builder, BallIdx ball, std::array<NodeIdx, 4>& seeds,
                                    DescentMemo* memo, CensusWorkspace* scratch, DescentLedger& work) noexcept {
  const auto& data = domain_->catalogue().balls_data()[idx(ball)];
  const auto inner = domain_->catalogue().interior(ball), shell = domain_->catalogue().shell(ball);
  if (data.m != data.qmin || data.qmin < 2 || data.qmin > 4 ||
      builder.k != u64{data.p} + data.qmin - 1) return fail(Reason::tower_invariant);
  const PopulationLookup* population = builder.population;
  if (population != nullptr && !population->belongs_to(*domain_)) return fail(Reason::parameter_out_of_range);
  const auto& level = domain_->catalogue().levels()[idx(data.rank)];
  // t=q-1. Le sommet omis decroit : ordre lexicographique EXACT de build_cell.
  for (u32 trace = 0; trace < data.qmin; ++trace) {
    const u32 omitted = data.qmin - 1 - trace;
    std::array<SiteIdx, 4> face{}; u32 used = 0;
    for (u32 j = 0; j < data.qmin; ++j) if (j != omitted) face[used++] = shell[j];
    std::array<SiteIdx, kMaxMebSites> part{};
    std::merge(inner.begin(), inner.end(), face.begin(), face.begin() + used, part.begin(),
               [](SiteIdx a, SiteIdx b) noexcept { return idx(a) < idx(b); });
    const std::span<const SiteIdx> traced{part.data(), builder.k};
    std::optional<PopulationLookup::Hit> hit;
    if (population != nullptr) {
      // Succes de table : meme graine et meme niveau que resolve_descent ; le pas est compte ci-dessous,
      // sans DescentResult ni addition du ledger complet.
      auto found = population->hit(traced, builder.k);
      if (!found.ok()) return found.outcome();
      hit = found.value();
    }
    std::optional<DescentResult> down;
    if (!hit) {
      // Echec deja constate : la table n'est pas reinterrogee.
      auto made = population != nullptr && memo == nullptr ?
          population->descend_each_step(traced, builder.k, *budget_, scratch, true) :
          resolve_descent(*domain_, traced, builder.k, *budget_, memo, scratch, nullptr);
      if (!made.ok()) return made.outcome();
      down.emplace(made.value());
    }
    const num::Level& initial = hit ? *hit->level : down->initial_level();
    if (num::compare(initial, level) >= 0) return fail(Reason::tower_invariant);
    const auto seed = builder.result.birth_node(hit ? hit->seed : down->seed());
    if (!seed || idx(*seed) >= builder.result.births() ||
        idx(builder.result.nodes_[idx(*seed)].rank) >= idx(data.rank))
      return fail(Reason::tower_invariant);
    seeds[trace] = *seed;
    if (down) { MHGP11_TRY(add_descent(work, down->ledger())); continue; }
    // Ledger d'un succes de table (PopulationLookup::descend) : un pas, population_hits, catalogue ou singleton.
    MHGP11_TRY(cell_add(work.steps, 1));
    MHGP11_TRY(cell_add(work.population_hits, 1));
    MHGP11_TRY(cell_add(builder.k == 1 ? work.singleton_hits : work.catalogue_hits, 1));
  }
  return {};
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
    MHGP11_TRY(resolve_job(builder, job.ball, job.seeds, memo, scratch, work.work));
  }
  if (clock) work.nanoseconds = clock->nanoseconds();
  return {};
}

namespace {
// Job d'ordinal global g (ordres concatenes), ou rien hors bornes ; from : ordre deja atteint, g croissant.
std::optional<std::pair<const OrderJobs*, BallIdx>> job_at(std::span<const OrderJobs> orders, u32 from,
                                                           u64 g) noexcept {
  u32 o = from;
  while (o + 1 < orders.size() && g >= orders[o + 1].first) ++o;
  if (g < orders[o].first || g - orders[o].first >= orders[o].jobs.size()) return std::nullopt;
  return std::pair{&orders[o], orders[o].jobs[g - orders[o].first]};
}

// Prechargement en trois etages, sans effet semantique (comme les lots de la v10 : empreintes et cases avant
// la descente) : boule et offsets du job g+3L, population du job g+2L, cases de table des traces du job g+L.
void prefetch_jobs(const Catalogue& cat, std::span<const OrderJobs> orders, u32 order, u64 g, u64 lanes) noexcept {
  if (const auto far = job_at(orders, order, g + 3 * lanes)) {
    __builtin_prefetch(cat.balls_data().data() + idx(far->second));
    __builtin_prefetch(cat.population_offsets().data() + idx(far->second));
  }
  if (const auto mid = job_at(orders, order, g + 2 * lanes)) {
    const u64 begin = cat.population_offsets()[idx(mid->second)], end = cat.population_offsets()[idx(mid->second) + 1];
    __builtin_prefetch(cat.population().data() + begin);
    if (end > begin) __builtin_prefetch(cat.population().data() + end - 1);
  }
  const auto near = job_at(orders, order, g + lanes);
  if (!near || near->first->builder == nullptr || near->first->builder->population == nullptr) return;
  const auto& data = cat.balls_data()[idx(near->second)];
  const u32 k = near->first->builder->k;
  if (data.m != data.qmin || data.qmin < 2 || data.qmin > 4 || k != u64{data.p} + data.qmin - 1) return;
  const auto inner = cat.interior(near->second), shell = cat.shell(near->second);
  for (u32 omitted = 0; omitted < data.qmin; ++omitted) {  // memes faces que resolve_job, ordre indifferent
    std::array<SiteIdx, kMaxMebSites> part{};
    u32 used = 0;
    for (SiteIdx s : inner) part[used++] = s;
    for (u32 j = 0; j < data.qmin; ++j) if (j != omitted) part[used++] = shell[j];
    if (used == k) near->first->builder->population->prefetch({part.data(), k}, k);
  }
}
}  // namespace

Outcome ForestParallel::resolve_lane(std::span<const OrderJobs> orders, std::span<DescentLedger> lane_work,
                                     u64 total, u32 slot, u32 worker) noexcept {
  const u32 physical = std::min<u64>(lanes_, total) < pool_->size() ? slot : worker;
  CensusWorkspace* scratch = scratch_ == nullptr ? nullptr : scratch_->get(physical);
  if (scratch_ != nullptr && scratch == nullptr) return fail(Reason::tower_invariant);
  const u32 lane = (next_lane_ + slot) % lanes_;
  DescentMemo* memo = memos_[lane] ? &*memos_[lane] : nullptr;
  u32 order = 0;
  for (u64 g = slot; g < total; g += lanes_) {
    while (order + 1 < orders.size() && g >= orders[order + 1].first) ++order;  // g croissant
    prefetch_jobs(domain_->catalogue(), orders, order, g, lanes_);
    const auto& o = orders[order];
    const u64 local = g - o.first;
    if (o.builder == nullptr || local >= o.jobs.size() || 4 * local + 4 > o.seeds.size())
      return fail(Reason::tower_invariant);
    std::array<NodeIdx, 4> seeds{};
    MHGP11_TRY(resolve_job(*o.builder, o.jobs[local], seeds, memo, scratch,
                           lane_work[u64{slot} * orders.size() + order]));
    std::copy(seeds.begin(), seeds.end(), o.seeds.begin() + 4 * local);  // cases propres a cet ordinal
  }
  return {};
}

struct ForestParallel::OrdersDispatch {
  ForestParallel& self;
  std::span<const OrderJobs> orders;
  std::span<DescentLedger> lane_work;
  u64 total;
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& d = *static_cast<OrdersDispatch*>(context);
    if (end > std::min<u64>(d.self.lanes_, d.total)) return fail(Reason::tower_invariant);
    for (u64 slot = begin; slot < end; ++slot)
      MHGP11_TRY(d.self.resolve_lane(d.orders, d.lane_work, d.total, static_cast<u32>(slot), worker));
    return {};
  }
};

Outcome ForestParallel::resolve_orders(std::span<const OrderJobs> orders, std::span<DescentLedger> lane_work) noexcept {
  if (domain_ == nullptr || pool_ == nullptr || count_ != 0 || orders.empty()) return fail(Reason::parameter_out_of_range);
  u64 total = 0;
  for (const auto& o : orders) {
    if (o.first != total || 4 * o.jobs.size() != o.seeds.size()) return fail(Reason::parameter_out_of_range);
    MHGP11_TRY(cell_add(total, o.jobs.size()));
  }
  if (total == 0) return {};
  const u64 jobs = std::min<u64>(lanes_, total);
  if (lane_work.size() < jobs * orders.size()) return fail(Reason::parameter_out_of_range);
  for (u64 i = 0; i < jobs * orders.size(); ++i) lane_work[i] = DescentLedger{};
  if (scratch_ == nullptr)
    MHGP11_TRY(budget_->admit(4 * u64{domain_->index().cloud().sites()} * std::min<u64>(pool_->size(), jobs)));
  OrdersDispatch dispatch{*this, orders, lane_work, total};
  MHGP11_TRY(pool_->parallel_for(jobs, 1, &dispatch, OrdersDispatch::body));
  next_lane_ = static_cast<u32>((next_lane_ + total) % lanes_);
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
