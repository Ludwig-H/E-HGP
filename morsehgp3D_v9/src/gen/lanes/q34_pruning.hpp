#pragma once

#include "lanes/q34_cover.hpp"

#include <memory>
#include <span>
#include <vector>

namespace mhgp9::gen {

struct Q34WitnessPoolWork {
  u64 range_visits{}, selected_sites{};
};

class Q34WitnessPool;
using Q34WitnessPoolPtr = std::shared_ptr<const Q34WitnessPool>;

// One immutable proposal pool per edge, owning its exact cover/index/cloud.
// Select min(budget,m) distinct, evenly spaced positions in covered spatial
// order. IDs are ORIGINAL IDs, not ranks; neither sorted nor copied per seed.
// O(ranges+budget) preparation, no scan of m coordinates. A proposal budget
// is NOT a search cap: uncertified lanes always take the exact fallback.
class Q34WitnessPool final {
 public:
  [[nodiscard]] static Q34WitnessPoolPtr make(Q34EdgeCoverPtr cover, std::size_t budget);
  Q34WitnessPool(const Q34WitnessPool&) = delete;
  Q34WitnessPool& operator=(const Q34WitnessPool&) = delete;
  Q34WitnessPool(Q34WitnessPool&&) = delete;
  Q34WitnessPool& operator=(Q34WitnessPool&&) = delete;
  [[nodiscard]] const Q34EdgeCoverPtr& cover() const noexcept { return cover_; }
  [[nodiscard]] std::span<const std::size_t> ids() const noexcept { return ids_; }
  [[nodiscard]] const Q34WitnessPoolWork& work() const noexcept { return work_; }
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  Q34WitnessPool(Q34EdgeCoverPtr cover, std::size_t budget);
  Q34EdgeCoverPtr cover_;
  std::vector<std::size_t> ids_;
  Q34WitnessPoolWork work_{};
};

struct Q34FamilyPruningWork {
  u64 seed_queries{}, certificate_builds{}, sqrt_iterations{};
  u64 proposed_sites{}, paired_predicate_tests{};
  // Distinct-site credits saturated independently at h3/h4. They are NEVER
  // added to the fallback census. q4 implies q3 geometrically, not rejection:
  // h4=K-2 differs from h3=K-1.
  u64 q3_credits{}, q4_credits{};
  // both_rejected counts rejection of ALL AVAILABLE lanes; at K=2 there
  // is only q3, so both_rejected may be 1 while q4_rejected stays 0.
  u64 q3_rejected{}, q4_rejected{}, both_rejected{};
  u64 q3_only_survivors{}, q4_only_survivors{}, both_survivors{};
};

struct Q34PrunedSeedWork {
  Q34CoverSeedWork covered;  // Work actually done by the remaining lanes.
  Q34FamilyPruningWork pruning;
};
struct Q34PrunedEdgeWork {
  Q34EdgeWork edge;
  Q34FamilyPruningWork pruning;
};

// Same complete positive-candidate outputs as the covered reference. The
// certificate only kills entire lanes. Every surviving census restarts at
// ZERO; no credit/identity is inherited. If only q3 remains, its census may
// stop at h3. All accepted shells remain complete and sorted by original ID.
// budget=0 preserves reference counters exactly. Null pool/callback or K=0
// throws before emission; seed validity/ownership follow the reference.
// Views are borrowed in synchronous callbacks; pool ownership outlives them.
[[nodiscard]] Q34PrunedSeedWork run_q34_pruned_seed_candidates(
    Q34WitnessPoolPtr pool, std::size_t x_id, std::size_t kmax,
    const Q34SeedConsumer& consumer);
[[nodiscard]] Q34PrunedEdgeWork run_q34_pruned_edge_candidates(
    Q34WitnessPoolPtr pool, std::size_t kmax, const Q34SeedConsumer& consumer);

}  // namespace mhgp9::gen
