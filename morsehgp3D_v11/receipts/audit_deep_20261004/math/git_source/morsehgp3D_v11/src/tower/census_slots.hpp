// Espaces physiques distincts des memos de lanes logiques. Aucun tableau extensible ni allocation worker.
#pragma once
#include "tower/tower.hpp"
#include "sched/sched.hpp"
#include <algorithm>

namespace mhgp11::tower_detail {
inline Result<u64> owned_census_workers(u32 workers, u32 workspaces, u64 tasks) noexcept {
  if (workers == 0 || workers > sched::kMaxWorkers || workspaces > workers)
    return fail(Reason::parameter_out_of_range);
  // Les espaces couvrent les IDs [0, workspaces), pas les premiers workers actifs.
  // Au plus tasks workers sont actifs, et au plus workers-workspaces manquent d'espace.
  return std::min<u64>(tasks, u64{workers} - workspaces);
}

class CensusSlots {
 public:
  CensusSlots(const CensusSlots&) = delete;
  CensusSlots& operator=(const CensusSlots&) = delete;
  CensusSlots& operator=(CensusSlots&&) = delete;
  CensusSlots(CensusSlots&& other) noexcept
      : domain_(std::exchange(other.domain_, nullptr)), budget_(std::exchange(other.budget_, nullptr)), count_(std::exchange(other.count_, 0)),
        slots_(std::move(other.slots_)) {}
  static Result<CensusSlots> make(const FullDomain& domain, u32 count, MemoryBudget& budget) noexcept {
    if (count > sched::kMaxWorkers || domain.index().cloud().sites() == 0)
      return fail(Reason::parameter_out_of_range);
    // n<2^32, count<=256 : 4*n*count<2^42, sans produit debordant.
    MHGP11_TRY(budget.admit(4 * u64{domain.index().cloud().sites()} * count));
    CensusSlots result(domain, budget, count);
    for (u32 i = 0; i < count; ++i) {
      auto made = CensusWorkspace::make(domain.index(), budget);
      if (!made.ok()) return made.outcome();
      result.slots_[i] = std::move(made.value());
    }
    return result;
  }
  bool belongs_to(const FullDomain& d, const MemoryBudget& b) const noexcept { return domain_ == &d && budget_ == &b; }
  u32 size() const noexcept { return count_; }
  u64 reserved_bytes() const noexcept {
    return count_ == 0 ? 0 : 4 * slots_[0]->capacity() * count_;
  }
  CensusWorkspace* get(u32 slot) const noexcept { return slot < count_ ? slots_[slot].get() : nullptr; }

 private:
  CensusSlots(const FullDomain& d, MemoryBudget& b, u32 count) noexcept : domain_(&d), budget_(&b), count_(count) {}
  const FullDomain* domain_;
  const MemoryBudget* budget_;
  u32 count_;
  std::array<std::unique_ptr<CensusWorkspace>, sched::kMaxWorkers> slots_{};
};
}  // namespace mhgp11::tower_detail
