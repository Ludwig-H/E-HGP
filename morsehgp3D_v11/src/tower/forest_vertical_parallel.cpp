// Les naissances sont resolues avant le balayage ; ni parents ni DSU ne sont lus dans les workers.
#include "tower/forest_parallel.hpp"
#include "tower/forest_vertical_seed.hpp"
#include "tower/regular_vertical_seeds.hpp"
#include <algorithm>

namespace mhgp11::tower_detail {

Outcome ForestParallel::vertical_lane(const OrderForest& lower, OrderForest& upper, u32 begin, u32 count,
                                      u32 slot, u32 worker, bool timed, bool reused,
                                      const PopulationLookup* population) noexcept {
  const u32 physical = std::min(lanes_, count) < pool_->size() ? slot : worker;
  CensusWorkspace* scratch = scratch_ == nullptr ? nullptr : scratch_->get(physical);
  if (scratch_ != nullptr && scratch == nullptr) return fail(Reason::tower_invariant);
  // Ordinal de naissance LOCAL a cet ordre, stable quand Q ou W changent.
  const u32 lane = (begin + slot) % lanes_;
  auto& work = work_[slot]; work = {};  // Seulement le delta de cette fenetre, jamais le travail regulier deja paye.
  std::optional<Stopwatch> clock;
  if (timed) clock.emplace();
  DescentMemo* memo = memos_[lane] ? &*memos_[lane] : nullptr;
  for (u32 offset = slot; offset < count; offset += lanes_) {
    const u32 ordinal = begin + offset;
    if (reused && upper.lower_[ordinal] != NodeIdx{kNone}) continue;
    auto seed = vertical_seed(*domain_, lower, upper.order_, upper.nodes_[ordinal], *budget_, memo, work.work, scratch,
                              population);
    if (!seed.ok()) return seed.outcome();
    upper.lower_[ordinal] = seed.value();  // Cases de naissance disjointes ; aucune image elevee n'existe encore.
  }
  if (clock) work.nanoseconds = clock->nanoseconds();
  return {};
}

struct ForestParallel::VerticalDispatch {
  ForestParallel& self;
  const OrderForest& lower;
  OrderForest& upper;
  u32 begin, count;
  bool timed, reused;
  const PopulationLookup* population;
  static Outcome body(void* context, u64 begin_slot, u64 end_slot, u32 worker) noexcept {
    auto& d = *static_cast<VerticalDispatch*>(context);
    if (end_slot > std::min(d.self.lanes_, d.count)) return fail(Reason::tower_invariant);
    for (u64 slot = begin_slot; slot < end_slot; ++slot)
      MHGP11_TRY(d.self.vertical_lane(d.lower, d.upper, d.begin, d.count, static_cast<u32>(slot), worker,
                                     d.timed, d.reused, d.population));
    return {};
  }
};

Outcome ForestParallel::verticals(const OrderForest& lower, OrderForest& upper, OrderTimings* times,
                                  const RegularVerticalSeeds* vertical_seeds,
                                  const PopulationLookup* population) noexcept {
  if (domain_ == nullptr || budget_ == nullptr || pool_ == nullptr || lanes_ == 0 || jobs_.empty() ||
      count_ != 0 || upper.order_ < 2 || lower.order_ + 1 != upper.order_ ||
      upper.lower_.size() != upper.nodes_.size()) return fail(Reason::parameter_out_of_range);
  if (vertical_seeds != nullptr && !vertical_seeds->belongs_to(*domain_)) return fail(Reason::parameter_out_of_range);
  for (u32 begin = 0; begin < upper.births_;) {
    const u32 count = static_cast<u32>(std::min(jobs_.size(), u64{upper.births_ - begin}));
    u32 misses = count;
    if (vertical_seeds != nullptr) {
      misses = 0;
      for (u32 offset = 0; offset < count; ++offset) {
        auto seed = vertical_seeds->find(lower, upper.nodes_[begin + offset]);
        if (!seed.ok()) return seed.outcome();
        upper.lower_[begin + offset] = seed.value().value_or(NodeIdx{kNone});
        if (!seed.value()) ++misses;
      }
      MHGP11_TRY(cell_add(upper.ledger_.vertical_reuses, count - misses));
    }
    if (misses == 0) { begin += count; continue; }  // Aucune fenetre vide envoyee au Pool.
    const u32 jobs = std::min(lanes_, count), concurrent = std::min(pool_->size(), jobs);
    // Precontrole, pas reservation. Un census possede au plus n SiteIdx par lane en vol.
    // Une allocation tardive peut encore refuser ; Pool joint toutes les lanes avant de rendre le refus.
    if (scratch_ == nullptr)
      MHGP11_TRY(budget_->admit(4 * u64{domain_->index().cloud().sites()} * concurrent));
    VerticalDispatch dispatch{*this, lower, upper, begin, count, times != nullptr, vertical_seeds != nullptr,
                              population};
    std::optional<Stopwatch> clock;
    if (times != nullptr) clock.emplace();
    MHGP11_TRY(pool_->parallel_for(jobs, 1, &dispatch, VerticalDispatch::body));
    if (clock) MHGP11_TRY(cell_add(times->vertical_dispatch_ns, clock->nanoseconds()));
    for (u32 slot = 0; slot < jobs; ++slot) {
      MHGP11_TRY(add_descent(upper.ledger_.descent, work_[slot].work));
      if (times != nullptr) {
        MHGP11_TRY(cell_add(times->vertical_task_sum_ns, work_[slot].nanoseconds));
        times->vertical_task_max_ns = std::max(times->vertical_task_max_ns, work_[slot].nanoseconds);
      }
    }
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, misses));
    if (times != nullptr) {
      MHGP11_TRY(cell_add(times->vertical_batches, 1));
      MHGP11_TRY(cell_add(times->vertical_resolutions, misses));
      times->max_vertical_batch = std::max(times->max_vertical_batch, u64{count});
    }
    begin += count;
  }
  return {};
}
namespace {
struct VerticalChunk {
  u64 reuses = 0, descents = 0;
  DescentLedger work;
};
}  // namespace

// Voie des ordres concurrents : images basses des naissances de TOUS les ordres hauts en une distribution par
// blocs fixes de naissances, sans memo ; graine reutilisee sinon descente sur l'espace census du worker.
// Compteurs sommes par ordre dans l'ordre des blocs : independants de W. Les balayages suivent separement.
struct ForestParallel::ImagesDispatch {
  ForestParallel& self;
  std::span<OrderForest* const> forests;
  std::span<const std::array<u64, 3>> chunks;  // (ordre haut, debut, fin)
  std::span<VerticalChunk> results;
  const RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population;
  static Outcome body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& d = *static_cast<ImagesDispatch*>(context);
    CensusWorkspace* scratch = d.self.census_slot(worker);
    for (u64 c = begin; c < end; ++c) {
      const auto [order, first, last] = d.chunks[c];
      const OrderForest& lower = *d.forests[order - 1];
      OrderForest& upper = *d.forests[order];
      auto& out = d.results[c];
      out = VerticalChunk{};
      for (u64 i = first; i < last; ++i) {
        if (d.vertical_seeds != nullptr) {
          auto seed = d.vertical_seeds->find(lower, upper.nodes_[i]);
          if (!seed.ok()) return seed.outcome();
          if (seed.value()) { upper.lower_[i] = *seed.value(); ++out.reuses; continue; }
        }
        auto seed = vertical_seed(*d.self.domain_, lower, upper.order_, upper.nodes_[i], *d.self.budget_, nullptr,
                                  out.work, scratch, d.population);
        if (!seed.ok()) return seed.outcome();
        upper.lower_[i] = seed.value();  // Cases de naissance disjointes par bloc.
        ++out.descents;
      }
    }
    return {};
  }
};

Outcome ForestParallel::vertical_images(std::span<OrderForest* const> forests, const RegularVerticalSeeds* vertical_seeds,
                                        const PopulationLookup* population, std::span<OrderTimings> times) noexcept {
  constexpr u64 kWidth = 2048;
  if (domain_ == nullptr || pool_ == nullptr || forests.size() > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  u64 count = 0;
  for (u64 o = 1; o < forests.size(); ++o) {
    const auto& upper = *forests[o];
    if (upper.order_ != forests[o - 1]->order_ + 1 || upper.lower_.size() != upper.nodes_.size())
      return fail(Reason::parameter_out_of_range);
    MHGP11_TRY(cell_add(count, (u64{upper.births_} + kWidth - 1) / kWidth));
  }
  if (count == 0) return {};
  Buffer<std::array<u64, 3>> chunks;
  Buffer<VerticalChunk> results;
  // Precontrole : un census possede (n SiteIdx) par tache simultanee sans espace physique de worker.
  const u64 concurrent = std::min<u64>(pool_->size(), count);
  const u64 owned = concurrent - std::min<u64>(concurrent, scratch_ == nullptr ? 0 : scratch_->size());
  MHGP11_TRY(budget_->admit(count * (sizeof(std::array<u64, 3>) + sizeof(VerticalChunk)) +
                            4 * u64{domain_->index().cloud().sites()} * owned));
  MHGP11_TRY(chunks.allocate(count, *budget_));
  MHGP11_TRY(results.allocate(count, *budget_));
  u64 at = 0;
  for (u64 o = 1; o < forests.size(); ++o)
    for (u64 first = 0; first < forests[o]->births_; first += kWidth)
      chunks[at++] = {o, first, std::min<u64>(first + kWidth, forests[o]->births_)};
  std::optional<Stopwatch> clock;
  if (!times.empty()) clock.emplace();
  ImagesDispatch dispatch{*this, forests, chunks.span(), results.span(), vertical_seeds, population};
  MHGP11_TRY(pool_->parallel_for(count, 1, &dispatch, ImagesDispatch::body));
  const u64 elapsed = clock ? clock->nanoseconds() : 0;
  for (u64 c = 0; c < count; ++c) {
    OrderForest& upper = *forests[chunks[c][0]];
    MHGP11_TRY(cell_add(upper.ledger_.vertical_reuses, results[c].reuses));
    MHGP11_TRY(cell_add(upper.ledger_.vertical_descents, results[c].descents));
    MHGP11_TRY(add_descent(upper.ledger_.descent, results[c].work));
    if (times.empty()) continue;
    auto& own = times[chunks[c][0]];
    own.vertical_dispatch_ns = elapsed;  // distribution commune a tous les ordres
    MHGP11_TRY(cell_add(own.vertical_batches, 1));
    MHGP11_TRY(cell_add(own.vertical_resolutions, results[c].descents));
    own.max_vertical_batch = std::max(own.max_vertical_batch, chunks[c][2] - chunks[c][1]);
  }
  return {};
}
}  // namespace mhgp11::tower_detail
