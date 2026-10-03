// Deux passes exactes, tri par tas sans allocation cachee, puis assemblage CSR et rangs canoniques.
// Les anciens et nouveaux tableaux sont simultanement reserves jusqu'a la fin de leur copie.
#include <optional>

#include "catalogue/internal.hpp"
#include "catalogue/assembly_parallel.hpp"
#include "catalogue/sort_indices.hpp"

namespace mhgp11::catalogue_detail {
namespace {

bool less(const Emission& a, const Emission& b, u64* comparisons) noexcept {
  if (comparisons != nullptr) ++*comparisons;  // heapsort : <4B*ceil(log2 B), B<2^32
  const int comparison = num::compare(a.level, b.level);
  return comparison != 0 ? comparison < 0 : a.ball.support < b.ball.support;
}

void sift(std::span<Emission> records, u64 root, u64 count, u64* comparisons) noexcept {
  while (root < count / 2) {
    u64 child = 2 * root + 1;
    if (child + 1 < count && less(records[child], records[child + 1], comparisons)) ++child;
    if (!less(records[root], records[child], comparisons)) return;
    std::swap(records[root], records[child]);
    root = child;
  }
}

void heap_sort(std::span<Emission> records, u64* comparisons) noexcept {
  const u64 count = records.size();
  if (count < 2) return;
  for (u64 i = count / 2; i > 0; --i) sift(records, i - 1, count, comparisons);
  for (u64 end = count; end > 1; --end) {
    std::swap(records[0], records[end - 1]);
    sift(records, 0, end - 1, comparisons);
  }
}

Outcome generate(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                 Buffer<Emission>& records, Buffer<SiteIdx>& population, CatalogueLedger& ledger) noexcept {
  Workspace workspace;
  MHGP11_TRY(workspace.allocate(std::min(cloud.sites(), params.max_leaf), budget, params.cache_center_lines));
  Collector counter;
  Run first{cloud, params, budget, workspace, counter, {}};
  MHGP11_TRY(walk(first));
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<Emission>(bytes, counter.balls));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, counter.incidences));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(records.allocate(counter.balls, budget));
  MHGP11_TRY(population.allocate(counter.incidences, budget));
  Collector writer{true, records.span(), population.span(), 0, 0};
  Run second{cloud, params, budget, workspace, writer, {}};
  MHGP11_TRY(walk(second));
  if (writer.balls != counter.balls || writer.incidences != counter.incidences || first.ledger != second.ledger)
    return fail(Reason::catalogue_invariant);
  ledger = first.ledger;
  return {};  // workspace rendu avant l'assemblage ; listes DFS deja rendues par walk.
}

struct SortedRecords {
  std::span<const Emission> records;
  std::span<const u32> permutation;
  u64 size() const noexcept { return records.size(); }
  bool empty() const noexcept { return records.empty(); }
  const Emission& operator[](u64 i) const noexcept { return records[permutation.empty() ? i : permutation[i]]; }
};

Result<u64> level_count(const SortedRecords& records) noexcept {
  if (records.empty()) return u64{1};  // niveau zero, meme sans boule positive
  u64 count = 2;
  const num::Level zero;
  if (num::compare(records[0].level, zero) <= 0) return fail(Reason::catalogue_invariant);
  for (u64 i = 1; i < records.size(); ++i) {
    const int comparison = num::compare(records[i - 1].level, records[i].level);
    if (comparison > 0 || (comparison == 0 && records[i - 1].ball.support == records[i].ball.support))
      return fail(Reason::catalogue_invariant);
    if (comparison < 0) MHGP11_TRY(checked_add(count, 1));
  }
  // count comprend zero : le plus grand rang est count-1, strictement inferieur a kNone.
  if (count > kNone) return fail(Reason::index_overflow_u32);
  return count;
}

Outcome serial_copy(const SortedRecords& ordered, std::span<const SiteIdx> population,
                    std::span<CatalogueBall> balls, std::span<num::Level> levels,
                    std::span<u64> offsets, std::span<SiteIdx> values) noexcept {
  levels[0] = num::Level{};
  offsets[0] = 0;
  u32 rank = 0;
  u64 offset = 0;
  for (u64 i = 0; i < ordered.size(); ++i) {
    const auto& record = ordered[i];
    if (i == 0 || num::compare(ordered[i - 1].level, record.level) < 0) levels[++rank] = record.level;
    balls[i] = record.ball;
    balls[i].rank = make_id<LevelRank>(rank);
    const u64 length = u64(record.ball.p) + record.ball.m;
    if (record.population_begin > population.size() || length > population.size() - record.population_begin ||
        offset > population.size() || length > population.size() - offset)
      return fail(Reason::catalogue_invariant);
    std::copy_n(population.data() + record.population_begin, length, values.data() + offset);
    offset += length;
    offsets[i + 1] = offset;
  }
  if (offset != population.size() || u64(rank) + 1 != levels.size()) return fail(Reason::catalogue_invariant);
  return {};
}

}  // namespace

Result<Catalogue> Assembly::finish(Buffer<Emission>& records, Buffer<SiteIdx>& population,
                                   const CatalogueParams& params, const CatalogueLedger& ledger,
                                   MemoryBudget& budget, CatalogueTimings* timings, sched::Pool* pool) noexcept {
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  Buffer<u32> permutation;
  if (params.indirect_sort) {
    auto sorted = sort_indices(records.span(), budget, pool, timings == nullptr ? nullptr : &timings->sort_comparisons);
    if (!sorted.ok()) return sorted.outcome();
    permutation = std::move(sorted.value());
    if (permutation.size() != records.size()) return fail(Reason::catalogue_invariant);
  } else {
    heap_sort(records.span(), timings == nullptr ? nullptr : &timings->sort_comparisons);
  }
  const SortedRecords ordered{records.span(), permutation.span()};
  if (timings != nullptr) timings->sort_ns = stage->nanoseconds();
  std::optional<AssemblyPlan> plan;
  if (params.parallel_assembly) {
    if (timings != nullptr) stage.emplace();
    auto made = AssemblyPlan::make(records.span(), permutation.span(), population.size(), budget);
    if (!made.ok()) return made.outcome();
    plan.emplace(std::move(made.value()));
    if (timings != nullptr) MHGP11_TRY(checked_add(timings->allocation_ns, stage->nanoseconds()));
  }
  if (timings != nullptr) stage.emplace();
  u64 number_levels = 1;
  if (plan) {
    MHGP11_TRY(plan->scan(pool));
    number_levels = plan->levels();
  } else {
    const auto counted = level_count(ordered);
    if (!counted.ok()) return counted.outcome();
    number_levels = counted.value();
  }
  if (timings != nullptr) timings->level_scan_ns = stage->nanoseconds();
  if (timings != nullptr) stage.emplace();
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<CatalogueBall>(bytes, records.size()));
  MHGP11_TRY(add_bytes<num::Level>(bytes, number_levels));
  MHGP11_TRY(add_bytes<u64>(bytes, records.size() + 1));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, population.size()));
  MHGP11_TRY(budget.admit(bytes));
  Catalogue result;
  MHGP11_TRY(result.balls_.allocate(records.size(), budget));
  MHGP11_TRY(result.levels_.allocate(number_levels, budget));
  MHGP11_TRY(result.population_.off.allocate(records.size() + 1, budget));
  MHGP11_TRY(result.population_.val.allocate(population.size(), budget));
  if (timings != nullptr) MHGP11_TRY(checked_add(timings->allocation_ns, stage->nanoseconds()));
  if (timings != nullptr) stage.emplace();
  result.kmax_ = static_cast<Order>(params.kmax);
  result.ledger_ = ledger;
  if (plan) {
    MHGP11_TRY(plan->fill(population.span(), result.balls_.span(), result.levels_.span(),
                         result.population_.off.span(), result.population_.val.span(), pool));
  } else {
    MHGP11_TRY(serial_copy(ordered, population.span(), result.balls_.span(), result.levels_.span(),
                           result.population_.off.span(), result.population_.val.span()));
  }
  if (timings != nullptr) timings->assembly_ns = stage->nanoseconds();
  return result;
}

Result<Catalogue> Assembly::build(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget) noexcept {
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  CatalogueLedger ledger;
  MHGP11_TRY(generate(cloud, params, budget, records, population, ledger));
  return finish(records, population, params, ledger, budget);
}

}  // namespace mhgp11::catalogue_detail
