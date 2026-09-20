#pragma once

#include "lanes/q34_pruning.hpp"

#include <cstddef>
#include <vector>

namespace mhgp8 {

enum class Q34ChordBound { Jung, Variance };
enum class Q34PoolReduction { Universal, Collective };

struct Q34PoolOptions {
  Q34ChordBound chord{Q34ChordBound::Jung};
  Q34PoolReduction reduction{Q34PoolReduction::Universal};
};

struct Q34PoolWork {
  u64 seed_owner_tests{}, seed_owner_rejections{};
  u64 seed_queries{}, certificate_builds{}, sqrt_iterations{};
  u64 variance_bounds{}, variance_sqrt_iterations{};
  u64 proposed_sites{}, paired_predicate_tests{};
  u64 q3_credits{}, q4_universal_credits{};
  u64 q3_rejected{}, q4_universal_rejected{}, collective_queries{};
  // Initial collective preparation, only until a universal q4 rejection:
  // two endpoint powers per noncoplanar site; one constant test for B=0.
  u64 endpoint_tests{}, constant_tests{}, event_count{};
  u64 sort_comparisons{}, group_comparisons{}, event_side_tests{}, groups{};
  u64 max_group{};  // MAX across seeds, not a sum.
  u64 collective_minimum_sum{}, collective_q4_rejected{}, q4_rejected{};
  // "Both" means all AVAILABLE lanes, including q3 alone at K=2.
  u64 both_rejected{}, q3_only_survivors{}, q4_only_survivors{}, both_survivors{};
  // MAX across seeds. Includes capacity retained from earlier assessments.
  u64 peak_event_bytes{};
};

class Q34PoolWorkspace;

struct Q34PoolAssessment {
  bool q3_rejected{}, q4_rejected{};
  i64 jung_parameter_bound{}, parameter_bound{};  // Zero if no certificate built.
  // Exact POOL minimum, valid only if work.collective_queries==1. Never a
  // claim of global census equality, and never saturated to a lane threshold.
  std::size_t collective_minimum{};
  Q34PoolWork work;
};

// Exclusive mutable scratch for one synchronous assessment at a time. It may
// be reused between seeds/options/pools after return (also after an exception),
// but must not be shared by concurrent calls. It owns no cloud/index/pool.
// No P/B list or copied coordinates: only original event IDs are retained.
class Q34PoolWorkspace final {
 public:
  Q34PoolWorkspace() = default;
  Q34PoolWorkspace(const Q34PoolWorkspace&) = delete;
  Q34PoolWorkspace& operator=(const Q34PoolWorkspace&) = delete;
  Q34PoolWorkspace(Q34PoolWorkspace&&) = delete;
  Q34PoolWorkspace& operator=(Q34PoolWorkspace&&) = delete;
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  friend Q34PoolAssessment assess_q34_family_pool(Q34WitnessPoolPtr, std::size_t,
      std::size_t, Q34PoolOptions, Q34PoolWorkspace&);
  std::vector<std::size_t> events_;
};

// Explicit port of audit 4215dd16's chord and collective reasoning, not its
// Python implementation/qualification. Validate pool/options/ID/acuteness even
// at K1 or with an empty pool; those cases do no filtering/certificate work.
// Non-owned seeds return an inactive assessment with paid owner tests.
// Null pool/K0/invalid options/nonacute seed throw invalid_argument; invalid
// x ID throws out_of_range. Allocation/arithmetic failures propagate: there is
// no successful partial assessment. Pool ownership keeps all points alive.
//
// q3 and q4 have independent thresholds K-1/K-2. Universal counts may saturate;
// a collective population that subsequently loses exits MUST NOT saturate.
// Collective mode prepares IDs during the initial shared P/B scan, then sorts
// ONLY if universal q4 rejection failed. It computes the full closed-chord
// minimum, including roots at -U/+U and coincident entries/exits. No credit
// may be added to a fallback census. Failure to certify means exact fallback,
// never search truncation. O(c log(1+c)) work/O(c) scratch for c pool sites;
// Universal uses O(c) work without adding events. No global bound follows.
[[nodiscard]] Q34PoolAssessment assess_q34_family_pool(
    Q34WitnessPoolPtr pool, std::size_t x_id, std::size_t kmax,
    Q34PoolOptions options, Q34PoolWorkspace& workspace);

struct Q34CollectiveSeedWork {
  Q34CoverSeedWork covered;
  Q34PoolWork filter;
  u64 peak_live_buffer_bytes{};  // Coupled workspace+fallback capacities.
};
struct Q34CollectiveEdgeWork {
  Q34EdgeWork edge;
  Q34PoolWork filter;
  u64 peak_live_buffer_bytes{};  // MAX of per-seed coupled observations.
};

// Same positive-candidate and synchronous-view contract as the covered path.
// Options are explicit; no historical/default entry is changed by this API.
// One workspace per call/edge is reused between seeds. Surviving lanes always
// start their exact census at zero; callbacks may retain only copied values.
// A callback failure propagates, leaving earlier emissions with the caller.
[[nodiscard]] Q34CollectiveSeedWork run_q34_collective_seed_candidates(
    Q34WitnessPoolPtr pool, std::size_t x_id, std::size_t kmax,
    Q34PoolOptions options, const Q34SeedConsumer& consumer);
[[nodiscard]] Q34CollectiveEdgeWork run_q34_collective_edge_candidates(
    Q34WitnessPoolPtr pool, std::size_t kmax,
    Q34PoolOptions options, const Q34SeedConsumer& consumer);

}  // namespace mhgp8
