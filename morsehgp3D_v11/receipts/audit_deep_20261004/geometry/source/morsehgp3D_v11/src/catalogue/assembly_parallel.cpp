// Un halo a gauche par bloc ; chaque paire adjacente est comparee une fois, aucune comparaison au remplissage.
#include "catalogue/assembly_parallel.hpp"

namespace mhgp11::catalogue_detail {
namespace {
Outcome execute(sched::Pool* pool, u64 tasks, void* context, sched::Pool::Body body) noexcept {
  if (tasks == 0) return {};
  return pool == nullptr ? body(context, 0, tasks, 0) : pool->parallel_for(tasks, 1, context, body);
}
}  // namespace

Result<AssemblyPlan> AssemblyPlan::make(std::span<Emission> records, std::span<const u32> permutation,
                                       u64 population_size, MemoryBudget& budget) noexcept {
  if (records.size() >= kNone) return fail(Reason::index_overflow_u32);
  if (!permutation.empty() && permutation.size() != records.size()) return fail(Reason::catalogue_invariant);
  AssemblyPlan plan(records, permutation, population_size);
  // B<2^32 : ces additions et les produits ordinal*grain (<B+grain) tiennent en u64.
  const u64 count = records.size();
  plan.grain_ = assembly_grain(count);
  const u64 blocks = assembly_blocks(count);
  MHGP11_TRY(budget.admit(blocks * sizeof(AssemblyBlock)));
  MHGP11_TRY(plan.blocks_.allocate(blocks, budget));
  for (auto& block : plan.blocks_) block = {};
  return plan;
}

Outcome AssemblyPlan::scan_block(u64 ordinal) noexcept {
  const u64 first = ordinal * grain_, last = std::min<u64>(first + grain_, records_.size());
  auto& block = blocks_[ordinal];
  u64 rank = 0, incidences = 0;
  for (u64 i = first; i < last; ++i) {
    auto& current = record(i);
    bool starts = i == 0;
    if (i == 0) {
      if (num::compare(current.level, num::Level{}) <= 0) return fail(Reason::catalogue_invariant);
    } else {
      const auto& previous = record(i - 1);
      const int order = num::compare(previous.level, current.level);
      if (order > 0 || (order == 0 && !(previous.ball.support < current.ball.support)))
        return fail(Reason::catalogue_invariant);
      starts = order < 0;
    }
    if (starts) ++rank;  // <= taille du bloc <= B < kNone ; aucun compteur sature.
    current.ball.rank = LevelRank{static_cast<u32>(rank)};
    const u64 length = u64(current.ball.p) + current.ball.m;
    if (current.population_begin > population_size_ || length > population_size_ - current.population_begin)
      return fail(Reason::catalogue_invariant);
    MHGP11_TRY(checked_add(incidences, length));
  }
  block.transitions = rank; block.incidences = incidences;
  return {};
}

Outcome AssemblyPlan::scan_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
  auto& plan = *static_cast<AssemblyPlan*>(context);
  if (end > plan.blocks() || worker >= sched::kMaxWorkers) return fail(Reason::catalogue_invariant);
  for (u64 i = begin; i < end; ++i) MHGP11_TRY(plan.scan_block(i));
  return {};
}

Outcome AssemblyPlan::scan(sched::Pool* pool) noexcept {
  scanned_ = false;
  MHGP11_TRY(execute(pool, blocks(), this, scan_body));  // joint tous les jobs, y compris sur refus
  u64 rank = 0, incidences = 0;
  for (auto& block : blocks_) {
    block.rank_begin = rank; block.population_begin = incidences;
    MHGP11_TRY(checked_add(rank, block.transitions));
    MHGP11_TRY(checked_add(incidences, block.incidences));
  }
  if (rank >= kNone) return fail(Reason::index_overflow_u32);
  if (incidences != population_size_) return fail(Reason::catalogue_invariant);
  levels_ = rank + 1; scanned_ = true;
  return {};
}

struct AssemblyPlan::Filling {
  const AssemblyPlan& plan;
  std::span<const SiteIdx> population;
  std::span<CatalogueBall> balls;
  std::span<num::Level> levels;
  std::span<u64> offsets;
  std::span<SiteIdx> values;

  Outcome block(u64 ordinal) noexcept {
    const auto& prefix = plan.blocks_[ordinal];
    const u64 first = ordinal * plan.grain_, last = std::min<u64>(first + plan.grain_, plan.records_.size());
    u64 offset = prefix.population_begin, local_before = 0;
    for (u64 i = first; i < last; ++i) {
      const auto& record = plan.record(i);
      const u64 local = idx(record.ball.rank), rank = prefix.rank_begin + local;
      if (local < local_before || local > local_before + 1 || rank == 0 || rank >= levels.size())
        return fail(Reason::catalogue_invariant);
      if (local > local_before) levels[rank] = record.level;  // unique premier representant non reduit
      local_before = local;
      balls[i] = record.ball; balls[i].rank = LevelRank{static_cast<u32>(rank)};
      const u64 length = u64(record.ball.p) + record.ball.m;
      if (record.population_begin > population.size() || length > population.size() - record.population_begin ||
          offset > values.size() || length > values.size() - offset)
        return fail(Reason::catalogue_invariant);
      std::copy_n(population.data() + record.population_begin, length, values.data() + offset);
      offset += length; offsets[i + 1] = offset;
    }
    if (offset != prefix.population_begin + prefix.incidences || local_before != prefix.transitions)
      return fail(Reason::catalogue_invariant);
    return {};
  }

  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& fill = *static_cast<Filling*>(context);
    if (end > fill.plan.blocks() || worker >= sched::kMaxWorkers) return fail(Reason::catalogue_invariant);
    for (u64 i = begin; i < end; ++i) MHGP11_TRY(fill.block(i));
    return {};
  }
};

Outcome AssemblyPlan::fill(std::span<const SiteIdx> population, std::span<CatalogueBall> balls,
                           std::span<num::Level> levels, std::span<u64> offsets, std::span<SiteIdx> values,
                           sched::Pool* pool) const noexcept {
  if (!scanned_ || population.size() != population_size_ || values.size() != population_size_ ||
      balls.size() != records_.size() || levels.size() != levels_ || offsets.size() != records_.size() + 1)
    return fail(Reason::catalogue_invariant);
  levels[0] = num::Level{}; offsets[0] = 0;
  Filling output{*this, population, balls, levels, offsets, values};
  MHGP11_TRY(execute(pool, blocks(), &output, Filling::body));
  if (offsets.back() != population_size_ || (!balls.empty() && u64(idx(balls.back().rank)) + 1 != levels_))
    return fail(Reason::catalogue_invariant);
  return {};
}

}  // namespace mhgp11::catalogue_detail
