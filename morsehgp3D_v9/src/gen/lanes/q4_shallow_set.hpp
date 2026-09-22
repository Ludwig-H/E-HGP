#pragma once

#include "lanes/q4_local_partition.hpp"

namespace mhgp9::gen {

struct Q4ShallowSetWork {
  u64 preparations{}, input_sites{}, form_tests{};
  u64 zero_sites{}, positive_sites{}, negative_sites{};
  // Includes both sorting comparisons and equality tests used for grouping.
  u64 lex_comparisons{}, orientation_tests{}, coordinate_groups{}, duplicate_ids{};
  // An attempted degenerate final layer counts as one executed layer.
  u64 positive_layers{}, negative_layers{}, layer_input_groups{}, layer_input_ids{};
  u64 boundary_groups{}, degenerate_groups{}, retained_ids{}, discarded_ids{};
  u64 retained_id_sort_comparisons{}, record_insertions{}, group_insertions{};
  // Every push to the temporary hull, even if later popped; compaction_moves
  // counts only assignments changing a group's position in the active list.
  u64 hull_index_copies{}, compaction_moves{};
  u64 peak_live_bytes{}, retained_bytes{};
  bool operator==(const Q4ShallowSetWork&) const = default;
};

class Q4ShallowSet;
using Q4ShallowSetPtr = std::shared_ptr<const Q4ShallowSet>;

// Exact shallow reduction of ONE supplied edge's cover, never a global q4
// bound. For L_z(t)=c+x*t.x+y*t.y, split c>0/c<0 and peel T=K-2 complete
// convex boundaries of the rational dual points (x/c,y/c). Every boundary
// point, collinear boundary point and coincident site's ORIGINAL ID stays.
// c==0 and all sites in any remaining affine-dimension<=1 set stay too.
//
// STRONG certificate: a removed site with L_z(t)<=0 implies at least T
// RETAINED sites with L(t)<0, one from each earlier layer of its sign.
// Thus depth_retained(t)<T implies every removed site is strictly outside:
// depth and the entire shell then agree with the original cover. A removed
// seed cannot define an accepted root. A retained-depth rejection certifies
// rejection but is not an exact unsaturated census of the original cover.
// No q2/q3 acceptance gate, sample, numerical approximation or quota.
//
// Sort the two rational lists ONCE and preserve order during layer removal:
// O(m log(1+m)+T*m) work, O(m) preparation buffers, output O(r). The residual
// r may still equal m; no subquadratic bound on later r^2 work, all edges or
// full-shell output follows. No per-layer re-sort and no cloud copy.
//
// The geometry/index/cloud owner is retained. Construction publishes only
// on success; allocation/counter failure leaves the input unchanged. All
// dynamic-capacity peaks count simultaneous owned output, records of BOTH
// signs, active groups, hull indices and boundary flags. Fixed objects,
// shared geometry/index/cloud, sorting stack, allocator metadata and
// reallocation overlap are excluded; this is not an RSS bound.
class Q4ShallowSet final {
 public:
  [[nodiscard]] static Q4ShallowSetPtr make(Q4LocalGeometryPtr geometry,
                                           std::size_t kmax);
  Q4ShallowSet(const Q4ShallowSet&) = delete;
  Q4ShallowSet& operator=(const Q4ShallowSet&) = delete;
  Q4ShallowSet(Q4ShallowSet&&) = delete;
  Q4ShallowSet& operator=(Q4ShallowSet&&) = delete;
  [[nodiscard]] const Q4LocalGeometryPtr& geometry() const noexcept { return geometry_; }
  [[nodiscard]] std::span<const std::size_t> retained_ids() const noexcept { return retained_; }
  [[nodiscard]] const Q4ShallowSetWork& work() const noexcept { return work_; }
  [[nodiscard]] std::size_t kmax() const noexcept { return kmax_; }
  [[nodiscard]] std::size_t retained_bytes() const;
 private:
  explicit Q4ShallowSet(Q4LocalGeometryPtr geometry, std::size_t kmax);
  Q4LocalGeometryPtr geometry_;
  std::size_t kmax_{};
  std::vector<std::size_t> retained_;
  Q4ShallowSetWork work_{};
};

}  // namespace mhgp9::gen
