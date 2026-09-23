#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "pipeline/q2_census.hpp"

namespace mhgp9::gen {

struct Q34EdgeCoverWork {
  u64 node_visits{};
  u64 bound_tests{};  // One min/max norm pair per visited non-singleton.
  u64 point_tests{};  // One exact predicate per visited singleton.
  u64 admitted_nodes{};
  u64 rejected_nodes{};
  u64 split_nodes{};
  u64 admitted_sites{};
  u64 rejected_sites{};
  u64 retained_ranges{};
  u64 merged_ranges{};  // Adjacent admitted node ranges joined on insertion.
  bool operator==(const Q34EdgeCoverWork&) const = default;
};

class Q34EdgeCover;
using Q34EdgeCoverPtr = std::shared_ptr<const Q34EdgeCover>;

// Exact CLOSED site set |2*z-a-b|^2 <= 4*|b-a|^2. Its spatial ranges are
// sorted, disjoint and maximally joined across adjacent retained ranks. These
// are ranks in index()->spatial_order(), never original IDs. This object owns
// the existing immutable index and hence its cloud; it does not copy either
// coordinates or an ID list. Share one instance between seeds of this edge.
//
// This geometric cover alone does not certify a candidate ball's positivity,
// ownership, depth or completeness. Those are contracts of its consumer.
class Q34EdgeCover final {
 public:
  // Null index/repeated IDs: invalid_argument. ID outside the owner's cloud:
  // out_of_range. Invalid inputs are rejected before traversing the index.
  // Allocation/counter failures propagate without publishing a partial cover.
  [[nodiscard]] static Q34EdgeCoverPtr make(
      Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids);
  // The closed DIAMETRAL ball |2*z-a-b|^2 <= |b-a|^2 of the same edge (a and
  // b on its boundary), with the same ranges contract: a subset of make().
  // Only a consumer for which any subset is sound may use it (v9 dead-lane
  // core, pipeline/wspd_q34.hpp); it never replaces the cover of a census.
  [[nodiscard]] static Q34EdgeCoverPtr make_diametral(
      Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids);

  Q34EdgeCover(const Q34EdgeCover&) = delete;
  Q34EdgeCover& operator=(const Q34EdgeCover&) = delete;
  Q34EdgeCover(Q34EdgeCover&&) = delete;
  Q34EdgeCover& operator=(Q34EdgeCover&&) = delete;

  [[nodiscard]] const Q2CensusIndexPtr& index() const noexcept { return index_; }
  [[nodiscard]] const std::array<std::size_t, 2>& edge_ids() const noexcept { return edge_ids_; }
  [[nodiscard]] std::span<const Range> ranges() const noexcept { return ranges_; }
  [[nodiscard]] std::size_t site_count() const noexcept { return site_count_; }
  // False for make_diametral(): such a core is readable by the dead-lane
  // certificate only (require_complete_q34_cover guards every other consumer).
  [[nodiscard]] bool complete() const noexcept { return complete_; }
  [[nodiscard]] const Q34EdgeCoverWork& work() const noexcept { return work_; }
  // Vector capacity only, excluding this object, shared_ptr metadata and the
  // already-owned index/cloud. No traversal stack or per-seed ID list exists.
  [[nodiscard]] std::size_t retained_bytes() const;
  // O(1) predicate on the original immutable coordinates, not a scan or an
  // original-ID/spatial-rank conversion. An invalid ID throws out_of_range.
  // This read-only query does not change the construction work counters.
  [[nodiscard]] bool contains_id(std::size_t id) const;

 private:
  explicit Q34EdgeCover(Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids, bool diametral);
  [[nodiscard]] static Q34EdgeCoverPtr make_ball(
      Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids, bool diametral);
  [[nodiscard]] bool contains_point(Point3 point) const noexcept;
  void admit(Range range);
  void reject(Range range);
  void build();

  Q2CensusIndexPtr index_;
  std::array<std::size_t, 2> edge_ids_;
  std::array<i64, 3> center_twice_{};
  i64 radius_fourfold_{};
  std::vector<Range> ranges_;
  std::size_t site_count_{};
  Q34EdgeCoverWork work_{};
  bool complete_{true};
};

// Census, atlas, seeds and shells need the COMPLETE closed cover: a diametral
// core passed to them is refused (invalid_argument). A null cover is left to
// each consumer's own refusal.
void require_complete_q34_cover(const Q34EdgeCoverPtr& cover);

}  // namespace mhgp9::gen
