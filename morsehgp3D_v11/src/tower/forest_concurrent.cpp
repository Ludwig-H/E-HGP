// Ordres concurrents (FullParams::concurrent_orders) : la tour FULL par etages plutot qu'ordre apres ordre.
//   A classification de chaque ordre par blocs de boules (Pool), sommes de compteurs inchangees ;
//   B naissances, etats DSU et liste des cellules regulieres : une tache par ordre ;
//   C descentes de TOUTES les cellules regulieres de jonction de tous les ordres en une distribution par lanes
//     (memes lanes, memos et workspaces que les lots historiques ; aucune lecture du DSU) ;
//   D publication par plateaux, une tache par ordre, dans l'ordre canonique exact de la voie par lots ;
//   E verticales : images basses par ordre (Pool) puis K-1 balayages fermes concurrents.
// Chaque foret ne lit que ses propres graines et son DSU : forets et verticales sont identiques a la voie
// sequentielle. La voie historique cumulait ~600 barrieres et ~330 ms de pilote seul sur G4 W48 (ng00).
#include "tower/forest_parallel.hpp"
#include "tower/population_lookup.hpp"
#include "tower/regular_vertical_seeds.hpp"

namespace mhgp11::tower_detail {

Outcome ForestBuilder::collect_jobs(std::span<BallIdx> jobs) const noexcept {
  const auto balls = domain.catalogue().balls_data();
  u64 count = 0;
  for (u32 b = 0; b < balls.size(); ++b)
    if (kinds[b] == 2 && balls[b].m == balls[b].qmin) {
      if (count >= jobs.size()) return fail(Reason::tower_invariant);
      jobs[count++] = BallIdx{b};
    }
  return count == jobs.size() ? Outcome{} : fail(Reason::tower_invariant);
}

Outcome ForestBuilder::publish(std::span<const BallIdx> jobs, std::span<const NodeIdx> seeds) noexcept {
  if (seeds.size() != 4 * jobs.size()) return fail(Reason::tower_invariant);
  const auto balls = domain.catalogue().balls_data();
  std::optional<LevelRank> active;
  u64 job = 0;
  for (u32 b = 0; b < balls.size(); ++b) {
    if (kinds[b] != 2) continue;
    // Meme fermeture que ForestParallel::select : un plateau se ferme au premier niveau superieur.
    const LevelRank level = balls[b].rank;
    if (active && *active != level) {
      if (idx(level) < idx(*active)) return fail(Reason::tower_invariant);
      MHGP11_TRY(close(*active));
      active.reset();
    }
    if (!active) { active = level; MHGP11_TRY(regular_plateau()); }
    if (balls[b].m == balls[b].qmin) {
      if (job >= jobs.size() || jobs[job] != BallIdx{b}) return fail(Reason::tower_invariant);
      MHGP11_TRY(regular_cell(BallIdx{b}, seeds.subspan(4 * job, balls[b].qmin)));
      ++job;
    } else {
      MHGP11_TRY(cell(BallIdx{b}));  // Voie etendue complete, descentes sur l'espace census du worker.
    }
  }
  if (job != jobs.size()) return fail(Reason::tower_invariant);
  if (active) MHGP11_TRY(close(*active));
  return {};
}

namespace {
constexpr u64 kClassifyChunks = 256;

struct ClassifyRun {
  const FullDomain& domain;
  u32 k;
  std::span<u8> kinds;
  std::span<ClassifyCounts> chunks;
  u64 width, balls;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<ClassifyRun*>(context);
    for (u64 c = begin; c < end; ++c) {
      const u64 lo = c * self.width, hi = std::min(self.balls, lo + self.width);
      self.chunks[c] = ClassifyCounts{};
      MHGP11_TRY(classify_range(self.domain, self.k, self.kinds, static_cast<u32>(lo), static_cast<u32>(hi),
                                self.chunks[c]));
    }
    return {};
  }
};

Outcome classify_parallel(ForestBuilder& builder, sched::Pool& pool) noexcept {
  const u64 balls = builder.domain.catalogue().balls();
  MHGP11_TRY(builder.kinds.allocate(balls, builder.budget));
  std::array<ClassifyCounts, kClassifyChunks> chunks{};
  const u64 width = std::max<u64>(1, (balls + kClassifyChunks - 1) / kClassifyChunks);
  const u64 count = (balls + width - 1) / width;
  ClassifyRun run{builder.domain, builder.k, builder.kinds.span(), chunks, width, balls};
  if (count != 0) MHGP11_TRY(pool.parallel_for(count, 1, &run, ClassifyRun::body));
  ClassifyCounts total;
  for (u64 c = 0; c < count; ++c) MHGP11_TRY(add_classify_counts(total, chunks[c]));  // ordre fixe des blocs
  return builder.adopt(total);
}

struct Staged {
  std::array<std::optional<ForestBuilder>, kMaxMebSites> builders;
  std::array<Buffer<BallIdx>, kMaxMebSites> jobs;
  std::array<Buffer<NodeIdx>, kMaxMebSites> seeds;
  std::array<u64, kMaxMebSites> nanoseconds{};
  u32 kmax = 0;
  bool timed = false;
};

struct BirthTasks {
  Staged& staged;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& s = static_cast<BirthTasks*>(context)->staged;
    for (u64 i = begin; i < end; ++i) {
      std::optional<Stopwatch> clock;
      if (s.timed) clock.emplace();
      auto& builder = *s.builders[i];
      MHGP11_TRY(builder.births());
      MHGP11_TRY(builder.prepare_states());
      MHGP11_TRY(builder.collect_jobs(s.jobs[i].span()));
      if (clock) s.nanoseconds[i] = clock->nanoseconds();
    }
    return {};
  }
};

struct PublishTasks {
  Staged& staged;
  ForestParallel& parallel;
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<PublishTasks*>(context);
    auto& s = self.staged;
    for (u64 i = begin; i < end; ++i) {
      std::optional<Stopwatch> clock;
      if (s.timed) clock.emplace();
      auto& builder = *s.builders[i];
      builder.extended_scratch = self.parallel.census_slot(worker);  // un appel a la fois par worker
      MHGP11_TRY(builder.publish(s.jobs[i].span(), s.seeds[i].span()));
      MHGP11_TRY(builder.finish());
      builder.extended_scratch = nullptr;
      if (clock) s.nanoseconds[i] = clock->nanoseconds();
    }
    return {};
  }
};

// B : admission de toutes les naissances, etats DSU et listes de cellules AVANT les K taches.
Outcome stage_births(Staged& s, MemoryBudget& budget, sched::Pool& pool) noexcept {
  u64 bytes = 0;
  for (u32 i = 0; i < s.kmax; ++i) {
    const auto& b = *s.builders[i];
    MHGP11_TRY(cell_add(bytes, b.birth_bytes()));
    MHGP11_TRY(cell_add(bytes, u64{b.result.births()} * (sizeof(ForestState) + sizeof(u32))));
    MHGP11_TRY(cell_add(bytes, b.regular_jobs * (sizeof(BallIdx) + 4 * sizeof(NodeIdx))));
  }
  MHGP11_TRY(budget.admit(bytes));
  for (u32 i = 0; i < s.kmax; ++i) {
    MHGP11_TRY(s.jobs[i].allocate(s.builders[i]->regular_jobs, budget));
    MHGP11_TRY(s.seeds[i].allocate(4 * s.builders[i]->regular_jobs, budget));
  }
  BirthTasks tasks{s};
  return pool.parallel_for(s.kmax, 1, &tasks, BirthTasks::body);
}

// C : une distribution par lanes pour tous les ordres, puis travail des lanes ajoute dans l'ordre des slots.
Outcome stage_regular(Staged& s, MemoryBudget& budget, ForestParallel& parallel) noexcept {
  std::array<OrderJobs, kMaxMebSites> orders{};
  u64 first = 0;
  for (u32 i = 0; i < s.kmax; ++i) {
    orders[i] = OrderJobs{&*s.builders[i], s.jobs[i].span(), s.seeds[i].span(), first};
    MHGP11_TRY(cell_add(first, s.jobs[i].size()));
  }
  const u64 slots = std::min<u64>(parallel.lanes(), first);
  Buffer<DescentLedger> lane_work;
  MHGP11_TRY(budget.admit(slots * s.kmax * sizeof(DescentLedger)));
  MHGP11_TRY(lane_work.allocate(slots * s.kmax, budget));
  MHGP11_TRY(parallel.resolve_orders(std::span(orders).first(s.kmax), lane_work.span()));
  for (u32 i = 0; i < s.kmax; ++i)
    for (u64 slot = 0; slot < slots; ++slot)
      MHGP11_TRY(s.builders[i]->regular_work(lane_work[slot * s.kmax + i]));
  return {};
}

}  // namespace

Outcome build_concurrent(const FullDomain& domain, Order kmax, MemoryBudget& budget, FullTimings* timings,
                         ForestParallel& parallel, sched::Pool& pool, RegularVerticalSeeds* vertical_seeds,
                         const PopulationLookup* population, bool dense,
                         std::array<std::optional<OrderForest>, kMaxMebSites>& orders) noexcept {
  if (kmax == 0 || kmax > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  Staged s;
  s.kmax = kmax;
  s.timed = timings != nullptr;
  std::optional<Stopwatch> phase;
  if (s.timed) phase.emplace();
  for (u32 k = 1; k <= kmax; ++k) {
    std::optional<Stopwatch> clock;
    if (s.timed) clock.emplace();
    auto& builder = s.builders[k - 1].emplace(domain, k, budget, s.timed ? &timings->orders[k - 1] : nullptr,
                                              nullptr, &parallel, dense, vertical_seeds);
    builder.population = population;
    MHGP11_TRY(classify_parallel(builder, pool));
    if (clock) timings->orders[k - 1].classify_ns = clock->nanoseconds();
  }
  if (phase) { timings->classify_phase_ns = phase->nanoseconds(); phase.emplace(); }
  MHGP11_TRY(stage_births(s, budget, pool));
  if (phase) {
    timings->birth_phase_ns = phase->nanoseconds(); phase.emplace();
    for (u32 i = 0; i < kmax; ++i) timings->orders[i].births_ns = s.nanoseconds[i];
  }
  MHGP11_TRY(stage_regular(s, budget, parallel));
  if (phase) { timings->regular_phase_ns = phase->nanoseconds(); phase.emplace(); }
  // D : census possedee par tache quand le worker n'a pas d'espace reutilise ; build_cell paie ses traces.
  MHGP11_TRY(budget.admit(4 * u64{domain.index().cloud().sites()} * kmax));
  PublishTasks publish{s, parallel};
  MHGP11_TRY(pool.parallel_for(kmax, 1, &publish, PublishTasks::body));
  if (phase) {
    timings->publish_phase_ns = phase->nanoseconds(); phase.emplace();
    for (u32 i = 0; i < kmax; ++i) timings->orders[i].plateaus_ns = s.nanoseconds[i];
  }
  std::array<OrderForest*, kMaxMebSites> forests{};
  for (u32 i = 0; i < kmax; ++i) forests[i] = &s.builders[i]->result;
  MHGP11_TRY(concurrent_verticals(domain, std::span(forests).first(kmax), budget, parallel, pool,
                                  s.timed ? std::span(timings->orders).first(kmax) : std::span<OrderTimings>{},
                                  vertical_seeds, population));
  if (phase) timings->vertical_phase_ns = phase->nanoseconds();
  for (u32 i = 0; i < kmax; ++i) orders[i].emplace(std::move(s.builders[i]->result));
  return {};
}

}  // namespace mhgp11::tower_detail
