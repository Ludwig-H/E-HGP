// Deux passes exactes, tri par tas sans allocation cachee, puis assemblage CSR et rangs canoniques.
// Les anciens et nouveaux tableaux sont simultanement reserves jusqu'a la fin de leur copie.
#include <optional>

#include "catalogue/internal.hpp"

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
  MHGP11_TRY(workspace.allocate(std::min(cloud.sites(), params.max_leaf), budget));
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

Result<u64> level_count(std::span<const Emission> records) noexcept {
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

}  // namespace

Result<Catalogue> Assembly::finish(Buffer<Emission>& records, Buffer<SiteIdx>& population,
                                   const CatalogueParams& params, const CatalogueLedger& ledger,
                                   MemoryBudget& budget, CatalogueTimings* timings) noexcept {
  std::optional<Stopwatch> stage;
  if (timings != nullptr) stage.emplace();
  heap_sort(records.span(), timings == nullptr ? nullptr : &timings->sort_comparisons);
  if (timings != nullptr) timings->sort_ns = stage->nanoseconds();
  if (timings != nullptr) stage.emplace();
  const auto number_levels = level_count(records.span());
  if (!number_levels.ok()) return number_levels.outcome();
  if (timings != nullptr) timings->level_scan_ns = stage->nanoseconds();
  if (timings != nullptr) stage.emplace();
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<CatalogueBall>(bytes, records.size()));
  MHGP11_TRY(add_bytes<num::Level>(bytes, number_levels.value()));
  MHGP11_TRY(add_bytes<u64>(bytes, records.size() + 1));
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, population.size()));
  MHGP11_TRY(budget.admit(bytes));
  Catalogue result;
  MHGP11_TRY(result.balls_.allocate(records.size(), budget));
  MHGP11_TRY(result.levels_.allocate(number_levels.value(), budget));
  MHGP11_TRY(result.population_.off.allocate(records.size() + 1, budget));
  MHGP11_TRY(result.population_.val.allocate(population.size(), budget));
  if (timings != nullptr) MHGP11_TRY(checked_add(timings->allocation_ns, stage->nanoseconds()));
  if (timings != nullptr) stage.emplace();
  result.levels_[0] = num::Level{};
  result.population_.off[0] = 0;
  result.kmax_ = static_cast<Order>(params.kmax);
  result.ledger_ = ledger;
  u32 rank = 0;
  u64 offset = 0;
  for (u64 i = 0; i < records.size(); ++i) {
    const auto& record = records[i];
    if (i == 0 || num::compare(records[i - 1].level, record.level) < 0) result.levels_[++rank] = record.level;
    result.balls_[i] = record.ball;
    result.balls_[i].rank = make_id<LevelRank>(rank);
    const u64 length = u64(record.ball.p) + record.ball.m;
    if (record.population_begin > population.size() || length > population.size() - record.population_begin ||
        offset > population.size() || length > population.size() - offset)
      return fail(Reason::catalogue_invariant);
    std::copy_n(population.data() + record.population_begin, length, result.population_.val.data() + offset);
    offset += length;
    result.population_.off[i + 1] = offset;
  }
  if (offset != population.size() || u64(rank) + 1 != result.levels_.size()) return fail(Reason::catalogue_invariant);
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
