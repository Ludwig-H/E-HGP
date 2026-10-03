// Les naissances sont resolues avant le balayage ; ni parents ni DSU ne sont lus dans les workers.
#include "tower/forest_parallel.hpp"
#include "tower/forest_vertical_seed.hpp"
#include "tower/regular_vertical_seeds.hpp"
#include <algorithm>

namespace mhgp11::tower_detail {

Outcome ForestParallel::vertical_lane(const OrderForest& lower, OrderForest& upper, u32 begin, u32 count,
                                      u32 slot, u32 worker, bool timed, bool reused) noexcept {
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
    auto seed = vertical_seed(*domain_, lower, upper.order_, upper.nodes_[ordinal], *budget_, memo, work.work, scratch);
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
  static Outcome body(void* context, u64 begin_slot, u64 end_slot, u32 worker) noexcept {
    auto& d = *static_cast<VerticalDispatch*>(context);
    if (end_slot > std::min(d.self.lanes_, d.count)) return fail(Reason::tower_invariant);
    for (u64 slot = begin_slot; slot < end_slot; ++slot)
      MHGP11_TRY(d.self.vertical_lane(d.lower, d.upper, d.begin, d.count, static_cast<u32>(slot), worker,
                                     d.timed, d.reused));
    return {};
  }
};

Outcome ForestParallel::verticals(const OrderForest& lower, OrderForest& upper, OrderTimings* times,
                                  const RegularVerticalSeeds* vertical_seeds) noexcept {
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
    VerticalDispatch dispatch{*this, lower, upper, begin, count, times != nullptr, vertical_seeds != nullptr};
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
}  // namespace mhgp11::tower_detail
