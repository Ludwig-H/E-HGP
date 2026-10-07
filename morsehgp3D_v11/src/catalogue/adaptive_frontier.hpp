// Plan binaire possede et borne : les feuilles vides consomment aussi une place, sans garder leur Buffer.
#pragma once

#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {

inline constexpr u32 kAdaptiveTasks = 1024;
inline constexpr u32 kAdaptiveNodes = 2 * kAdaptiveTasks - 1;

namespace adaptive_detail {
enum class Kind : u8 { vacant, job, empty, split };
struct PlanNode {
  Box box{};
  std::array<u64, 2> path{};
  u32 depth = 0, count = 0, capacity = 0, inside = 0, first_child = kNone;
  Kind kind = Kind::vacant;
};
struct LiveNode { ReadyNode ready; u32 node = 0; };
struct State {
  std::array<LiveNode, kAdaptiveTasks> live{};
  std::array<u32, kAdaptiveNodes> slots{};
  u32 count = 0;
};
struct Builder;
struct Replay;
}  // namespace adaptive_detail

class AdaptiveFrontier {
 public:
  AdaptiveFrontier() = default;
  AdaptiveFrontier(const AdaptiveFrontier&) = delete;
  AdaptiveFrontier& operator=(const AdaptiveFrontier&) = delete;
  // timings, si non nul : sous-chronos du pilote (prefix_root_ns ... prefix_publish_ns), diagnostic seulement.
  Outcome prepare(Run& run, sched::Pool& pool, CatalogueTimings* timings = nullptr) noexcept;
  Outcome verify(Run& run, sched::Pool& pool) const noexcept;
  u32 size() const noexcept { return count_; }
  const ReadyNode& task(u32 i) const noexcept { return state_.live[tasks_[i]].ready; }
  const CatalogueLedger& ledger() const noexcept { return ledger_; }
  const CataloguePlanning& planning() const noexcept { return planning_; }
  Outcome describe(u32 i, CatalogueTaskDiagnostic& out) const noexcept;
  Outcome execute_task(u32 i, Run& run) const noexcept;
  Outcome owned_bytes(u64& bytes) const noexcept;
  Outcome verify_memory_bound(u64& bytes) const noexcept;
  Outcome suffix_memory_bound(u32 workers, u64& bytes) const noexcept;

 private:
  friend struct adaptive_detail::Builder;
  friend struct adaptive_detail::Replay;
  bool matches(const Run& run) const noexcept;
  void clear() noexcept;
  adaptive_detail::State state_{};
  std::array<adaptive_detail::PlanNode, kAdaptiveNodes> plan_{};
  std::array<u32, kAdaptiveTasks> tasks_{}, round_end_{};
  std::array<u32, kAdaptiveTasks - 1> parents_{};
  const Cloud* cloud_ = nullptr;
  CatalogueParams params_{};
  CatalogueLedger ledger_{};
  CataloguePlanning planning_{};
  u32 count_ = 0;
  bool prepared_ = false;
};

// Meme addition exacte que les suffixes ; les maxima geometriques ne s'additionnent jamais.
Outcome add_catalogue_ledger(CatalogueLedger& sum, const CatalogueLedger& part) noexcept;

}  // namespace mhgp11::catalogue_detail
