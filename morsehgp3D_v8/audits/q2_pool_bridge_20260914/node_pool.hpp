#pragma once

// Explicit AUDIT adaptation of pool_credits/group in the e3af11a7 snapshot:
// morsehgp3D_v8/src/pipeline/local_credits.cpp SHA256
// 0d069aca65dcffc96bb26d55874fdb34b2748eb5f0b956a991d584d928ab1392,
// q2_order_lidar_20260914/r1_sources.zip SHA256
// ab184fa236e0379e396cd21de893df781373cbaa242365ddc2f1ea8f3c7d6bd8.
// The projection order still breaks ties by ORIGINAL point ID. Iteration and
// stable credit grouping instead follow the global SPATIAL order. This may
// change preparation work and query-tree geometry relative to the old plan.
// It does not change the selected pool or per-original-ID certified credits.
// No coordinates, global index, owner, inverse map or core credit are copied.

#include "pipeline/q2_census.hpp"
#include "spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp8::audit {

struct NodePoolWork {
  PredicateWork predicates{};
  u64 factor_visits{};  // Selection + certification anchor-position visits.
  u64 selection_point_visits{};
  u64 selection_tests{};
  u64 pool_selected{};
  u64 pool_insertions{};
  u64 pool_shifted_entries{};  // Logical insertion shifts; excludes allocator moves.
  u64 certification_anchor_visits{};
  u64 witness_attempts{};  // Includes the skipped self proposal, if present.
  u64 group_credit_visits{};
  u64 group_scatter_visits{};
  u64 prefix_class_visits{};
  u64 prefix_anchor_visits{};  // Zero: class sizes suffice for the pair sum.
};

// All retained views/IDs belong to the exact borrowed index. It must remain
// alive and immutable for the plan's lifetime and every consuming callback.
// Copying, assigning or moving the plan during a borrowed census is forbidden.
// A real front callback supplies the handles; constructor validation adopts
// only existing disjoint nodes from this index, without scanning n sites.
// Separation is the front's contract, not needed by the strict q2 predicate.
class NodePoolPlan final {
 public:
  NodePoolPlan(const Q2CensusIndex& index, std::size_t a_node,
               std::size_t b_node, unsigned kmax)
      : index_(&index), a_node_(a_node), b_node_(b_node), kmax_(kmax) {
    const auto nodes = index.spatial_nodes();
    if (kmax == 0 || kmax > 10 || a_node >= nodes.size() || b_node >= nodes.size()) {
      throw std::invalid_argument("audit node Pool requires valid nodes and Kmax in [1,10]");
    }
    a_ = nodes[a_node].range;
    b_ = nodes[b_node].range;
    if (a_.first >= a_.last || b_.first >= b_.last ||
        a_.last > index.spatial_order().size() || b_.last > index.spatial_order().size() ||
        !(a_.last <= b_.first || b_.last <= a_.first)) {
      throw std::invalid_argument("audit node Pool requires disjoint nonempty index factors");
    }
    total_ = pair_count(a_.size(), b_.size());
    a_credits_ = credits(a_, nodes[a_node].box, nodes[b_node].box);
    b_credits_ = credits(b_, nodes[b_node].box, nodes[a_node].box);
    group(a_credits_, a_, true, a_ranks_, a_groups_);
    group(b_credits_, b_, false, b_order_, b_groups_);
    // h=Kmax: this audit adapter has no externally supplied/core credit.
    // The union of admitted B classes is exactly one prefix per A class.
    for (unsigned credit = 0; credit <= kmax_; ++credit) {
      counter_add(work_.prefix_class_visits);
      prefixes_[credit] = credit == kmax_ ? 0 : b_groups_[kmax_ - credit - 1].last;
      if (a_groups_[credit].size() != 0) {
        counter_add(candidates_, pair_count(a_groups_[credit].size(), prefixes_[credit]));
        max_prefix_ = std::max(max_prefix_, prefixes_[credit]);
      }
    }
    static_cast<void>(retained_bytes());
  }

  NodePoolPlan(const NodePoolPlan&) = delete;
  NodePoolPlan& operator=(const NodePoolPlan&) = delete;
  NodePoolPlan(NodePoolPlan&& other) noexcept { swap(other); }
  NodePoolPlan& operator=(NodePoolPlan&&) = delete;

  [[nodiscard]] const Q2CensusIndex& index() const { check_live(); return *index_; }
  [[nodiscard]] std::size_t a_node() const { check_live(); return a_node_; }
  [[nodiscard]] std::size_t b_node() const { check_live(); return b_node_; }
  [[nodiscard]] Range a_range() const { check_live(); return a_; }
  [[nodiscard]] Range b_range() const { check_live(); return b_; }
  [[nodiscard]] unsigned kmax() const { check_live(); return kmax_; }
  // Credits are indexed by rank minus the ORIGINAL factor range.first.
  [[nodiscard]] std::span<const std::uint8_t> a_credits() const { check_live(); return a_credits_; }
  [[nodiscard]] std::span<const std::uint8_t> b_credits() const { check_live(); return b_credits_; }
  // A output contains global spatial ranks, B output contains original IDs.
  [[nodiscard]] std::span<const std::size_t> a_ranks() const { check_live(); return a_ranks_; }
  [[nodiscard]] std::span<const std::size_t> b_order() const { check_live(); return b_order_; }
  // Includes the saturated class Kmax at the end, with candidate prefix zero.
  [[nodiscard]] std::span<const Range> a_groups() const {
    check_live(); return {a_groups_.data(), static_cast<std::size_t>(kmax_) + 1};
  }
  [[nodiscard]] std::size_t prefix_for_credit(unsigned credit) const {
    check_live();
    if (credit > kmax_) throw std::invalid_argument("audit Pool credit exceeds saturation");
    return prefixes_[credit];
  }
  [[nodiscard]] std::size_t prefix_for_a_rank(std::size_t rank) const {
    check_live();
    if (rank < a_.first || rank >= a_.last) {
      throw std::invalid_argument("audit Pool anchor rank escaped its factor");
    }
    return prefixes_[a_credits_[rank - a_.first]];
  }
  [[nodiscard]] u64 total_pairs() const { check_live(); return total_; }
  [[nodiscard]] u64 candidate_pairs() const { check_live(); return candidates_; }
  [[nodiscard]] std::size_t max_prefix() const { check_live(); return max_prefix_; }
  [[nodiscard]] const NodePoolWork& work() const { check_live(); return work_; }
  // Vector capacities only; excludes this object (which owns O(Kmax) class
  // arrays), the borrowed index/cloud, transient O(Kmax+1) proposals and RSS.
  // A full insertion briefly holds Kmax+2 entries before dropping the last;
  // the allocator can retain a larger capacity for this transient vector.
  [[nodiscard]] std::size_t retained_bytes() const {
    check_live();
    std::size_t result = 0;
    const auto add = [&](std::size_t capacity, std::size_t element_size) {
      if (capacity > (std::numeric_limits<std::size_t>::max() - result) / element_size) {
        throw std::overflow_error("audit Pool retained capacity overflow");
      }
      result += capacity * element_size;
    };
    add(a_credits_.capacity(), sizeof(std::uint8_t));
    add(b_credits_.capacity(), sizeof(std::uint8_t));
    add(a_ranks_.capacity(), sizeof(std::size_t));
    add(b_order_.capacity(), sizeof(std::size_t));
    return result;
  }

 private:
  struct Proposal { i64 score; std::size_t id; };

  void check_live() const {
    if (index_ == nullptr) throw std::logic_error("audit node Pool plan was moved from");
  }
  [[nodiscard]] static u64 pair_count(std::size_t a, std::size_t b) {
    if (a > std::numeric_limits<u64>::max() || b > std::numeric_limits<u64>::max() ||
        (b != 0 && a > std::numeric_limits<u64>::max() / b)) {
      throw std::overflow_error("audit Pool pair count exceeds u64");
    }
    return static_cast<u64>(a) * static_cast<u64>(b);
  }

  [[nodiscard]] std::vector<std::uint8_t> credits(
      Range factor, const Box3& own, const Box3& opposite) {
    const auto points = index_->cloud().points();
    const auto order = index_->spatial_order();
    std::array<i64, 3> direction{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      direction[axis] = static_cast<i64>(opposite.low[axis]) + opposite.high[axis]
                        - own.low[axis] - own.high[axis];
    }
    const std::size_t capacity = std::min(factor.size(), static_cast<std::size_t>(kmax_) + 1);
    std::vector<Proposal> pool;
    pool.reserve(capacity);
    for (auto rank = factor.first; rank < factor.last; ++rank) {
      counter_add(work_.factor_visits);
      counter_add(work_.selection_point_visits);
      const auto id = order[rank];
      i64 score = 0;
      for (std::size_t axis = 0; axis < 3; ++axis) score += direction[axis] * points[id][axis];
      auto position = pool.begin();
      while (position != pool.end()) {
        counter_add(work_.selection_tests);
        if (score > position->score || (score == position->score && id < position->id)) break;
        ++position;
      }
      if (position != pool.end() || pool.size() < capacity) {
        counter_add(work_.pool_insertions);
        counter_add(work_.pool_shifted_entries, static_cast<u64>(pool.end() - position));
        pool.insert(position, Proposal{score, id});
        if (pool.size() > capacity) pool.pop_back();
      }
    }
    counter_add(work_.pool_selected, static_cast<u64>(pool.size()));
    std::vector<std::uint8_t> result(factor.size(), 0);
    for (auto rank = factor.first; rank < factor.last; ++rank) {
      counter_add(work_.factor_visits);
      counter_add(work_.certification_anchor_visits);
      const auto id = order[rank];
      auto& value = result[rank - factor.first];
      for (const auto& proposal : pool) {
        counter_add(work_.witness_attempts);
        if (proposal.id != id && universal_witness(Lane::Q2, points[id], opposite,
                                                    points[proposal.id], work_.predicates)) {
          ++value;
          if (value == kmax_) break;
        }
      }
    }
    return result;
  }

  void group(std::span<const std::uint8_t> values, Range factor, bool ranks,
             std::vector<std::size_t>& output, std::array<Range, 11>& groups) {
    for (const auto credit : values) {
      counter_add(work_.group_credit_visits);
      ++groups[credit].last;
    }
    std::size_t offset = 0;
    for (unsigned credit = 0; credit <= kmax_; ++credit) {
      const auto count = groups[credit].last;
      groups[credit] = {offset, offset + count};
      offset += count;
    }
    auto cursors = groups;
    output.resize(values.size());
    const auto order = index_->spatial_order();
    for (std::size_t local = 0; local < values.size(); ++local) {
      counter_add(work_.group_scatter_visits);
      const auto rank = factor.first + local;
      output[cursors[values[local]].first++] = ranks ? rank : order[rank];
    }
  }

  void swap(NodePoolPlan& other) noexcept {
    std::swap(index_, other.index_);
    std::swap(a_node_, other.a_node_); std::swap(b_node_, other.b_node_);
    std::swap(a_, other.a_); std::swap(b_, other.b_); std::swap(kmax_, other.kmax_);
    a_credits_.swap(other.a_credits_); b_credits_.swap(other.b_credits_);
    a_ranks_.swap(other.a_ranks_); b_order_.swap(other.b_order_);
    a_groups_.swap(other.a_groups_); b_groups_.swap(other.b_groups_); prefixes_.swap(other.prefixes_);
    std::swap(total_, other.total_); std::swap(candidates_, other.candidates_);
    std::swap(max_prefix_, other.max_prefix_); std::swap(work_, other.work_);
  }

  const Q2CensusIndex* index_{};
  std::size_t a_node_{}, b_node_{};
  Range a_{}, b_{};
  unsigned kmax_{};
  std::vector<std::uint8_t> a_credits_, b_credits_;
  std::vector<std::size_t> a_ranks_, b_order_;
  std::array<Range, 11> a_groups_{}, b_groups_{};
  std::array<std::size_t, 11> prefixes_{};
  u64 total_{}, candidates_{};
  std::size_t max_prefix_{};
  NodePoolWork work_{};
};

}  // namespace mhgp8::audit
