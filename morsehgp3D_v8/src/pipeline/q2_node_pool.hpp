#pragma once

// Explicitly reevaluated port of the audit prototype NodePoolPlan published
// at fbbecc01: audits/q2_pool_bridge_20260914/node_pool.hpp. This product
// adapter keeps its original-ID projection tie break, spatially stable
// credit grouping and work-counter meanings. It replaces transient vector
// insertions by a fixed buffer and forbids every copy/move/assignment.
// Scope: strict q2 prefilter on disjoint nodes of one immutable global index.
// No audit qualification, timing, complete census or FULL claim is inherited.

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

namespace mhgp8::detail {

struct Q2NodePoolWork {
  PredicateWork predicates{};
  u64 factor_visits{};  // Selection + certification anchor-position visits.
  u64 selection_point_visits{};
  u64 selection_tests{};  // One complete score/ID ordering comparison.
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

// The exact index is borrowed for the entire lifetime of the plan and every
// consuming callback/job. All retained spans are private, immutable views of
// this plan; neither coordinates, owner, index nor a query tree are copied.
// Existing node handles and disjoint ranges are checked without an n-site
// validation. Separation is a front contract, unnecessary for q2 credit.
//
// Each h_a uses distinct witnesses in A excluding a, each h_b uses witnesses
// in B excluding b, and each witness is strict for the opposite whole box.
// Since A and B are disjoint, h_a+h_b is a lower bound on the global depth.
// Only pairs with h_a+h_b >= K are removed. Every survivor requires a fresh
// all-cloud census starting at zero: these credits must NEVER preload it.
// At most K disjoint A-class x B-prefix bands describe all survivors, even
// though their B prefixes overlap. Saturated A class K has an empty prefix.
//
// Work is O(K*(|A|+|B|)) per plan, with linear retained factor storage. This
// is not a bound on the sum of factor sizes across an arbitrary WSPD front.
class Q2NodePoolPlan final {
 public:
  Q2NodePoolPlan(const Q2CensusIndex& index, std::size_t a_node,
                 std::size_t b_node, unsigned kmax)
      : index_(&index), a_node_(a_node), b_node_(b_node), kmax_(kmax) {
    const auto nodes = index.spatial_nodes();
    if (kmax == 0 || kmax > 10 || a_node >= nodes.size() || b_node >= nodes.size()) {
      throw std::invalid_argument("q2 node Pool requires valid nodes and Kmax in [1,10]");
    }
    a_ = nodes[a_node].range;
    b_ = nodes[b_node].range;
    const auto size = index.spatial_order().size();
    if (a_.first >= a_.last || b_.first >= b_.last || a_.last > size ||
        b_.last > size || !(a_.last <= b_.first || b_.last <= a_.first)) {
      throw std::invalid_argument("q2 node Pool requires disjoint nonempty index factors");
    }
    total_ = pair_count(a_.size(), b_.size());
    a_credits_ = credits(a_, nodes[a_node].box, nodes[b_node].box);
    b_credits_ = credits(b_, nodes[b_node].box, nodes[a_node].box);
    group(a_credits_, a_, true, a_ranks_, a_groups_);
    group(b_credits_, b_, false, b_order_, b_groups_);
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

  Q2NodePoolPlan(const Q2NodePoolPlan&) = delete;
  Q2NodePoolPlan& operator=(const Q2NodePoolPlan&) = delete;
  Q2NodePoolPlan(Q2NodePoolPlan&&) = delete;
  Q2NodePoolPlan& operator=(Q2NodePoolPlan&&) = delete;

  [[nodiscard]] const Q2CensusIndex& index() const noexcept { return *index_; }
  [[nodiscard]] std::size_t a_node() const noexcept { return a_node_; }
  [[nodiscard]] std::size_t b_node() const noexcept { return b_node_; }
  [[nodiscard]] Range a_range() const noexcept { return a_; }
  [[nodiscard]] Range b_range() const noexcept { return b_; }
  [[nodiscard]] unsigned kmax() const noexcept { return kmax_; }
  // Credits are indexed by global spatial rank minus their factor's first.
  [[nodiscard]] std::span<const std::uint8_t> a_credits() const noexcept {
    return a_credits_;
  }
  [[nodiscard]] std::span<const std::uint8_t> b_credits() const noexcept {
    return b_credits_;
  }
  // A contains GLOBAL SPATIAL RANKS; B contains ORIGINAL point IDs. Both
  // are grouped by increasing credit, stable within each credit class.
  [[nodiscard]] std::span<const std::size_t> a_ranks() const noexcept { return a_ranks_; }
  [[nodiscard]] std::span<const std::size_t> b_order() const noexcept { return b_order_; }
  // Ranges index a_ranks(), not the original spatial_order(). Includes K.
  [[nodiscard]] std::span<const Range> a_groups() const noexcept {
    return {a_groups_.data(), static_cast<std::size_t>(kmax_) + 1};
  }
  [[nodiscard]] std::size_t prefix_for_credit(unsigned credit) const {
    if (credit > kmax_) throw std::invalid_argument("q2 Pool credit exceeds saturation");
    return prefixes_[credit];
  }
  [[nodiscard]] std::size_t prefix_for_a_rank(std::size_t rank) const {
    if (rank < a_.first || rank >= a_.last) {
      throw std::invalid_argument("q2 Pool anchor rank escaped its factor");
    }
    return prefixes_[a_credits_[rank - a_.first]];
  }
  [[nodiscard]] u64 total_pairs() const noexcept { return total_; }
  [[nodiscard]] u64 candidate_pairs() const noexcept { return candidates_; }
  [[nodiscard]] std::size_t max_prefix() const noexcept { return max_prefix_; }
  [[nodiscard]] const Q2NodePoolWork& work() const noexcept { return work_; }

  // Retained vector capacities only: excludes this object's O(K) metadata,
  // borrowed index/cloud, transient fixed proposal buffer and process RSS.
  [[nodiscard]] std::size_t retained_bytes() const {
    std::size_t result = 0;
    const auto add = [&](std::size_t capacity, std::size_t element_size) {
      if (capacity > (std::numeric_limits<std::size_t>::max() - result) / element_size) {
        throw std::overflow_error("q2 Pool retained capacity overflow");
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
  struct Proposal {
    i64 score;
    std::size_t id;
  };

  [[nodiscard]] static u64 pair_count(std::size_t a, std::size_t b) {
    constexpr auto maximum = std::numeric_limits<u64>::max();
    if (std::cmp_greater(a, maximum) || std::cmp_greater(b, maximum)) {
      throw std::overflow_error("q2 Pool pair count exceeds u64");
    }
    const auto a64 = static_cast<u64>(a);
    const auto b64 = static_cast<u64>(b);
    if (b64 != 0 && a64 > maximum / b64) {
      throw std::overflow_error("q2 Pool pair count exceeds u64");
    }
    return a64 * b64;
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
    // |direction_i| <= 2*65535, hence every score fits signed i64. The
    // projection only proposes witnesses; it is never a geometric credit.
    const auto capacity = std::min(factor.size(), static_cast<std::size_t>(kmax_) + 1);
    // K <= 10: keep at most 11 entries, plus one insertion scratch slot.
    // Preserve every prototype logical shift, including the discarded tail.
    std::array<Proposal, 12> pool{};
    std::size_t pool_size = 0;
    for (auto rank = factor.first; rank < factor.last; ++rank) {
      counter_add(work_.factor_visits);
      counter_add(work_.selection_point_visits);
      const auto id = order[rank];
      i64 score = 0;
      for (std::size_t axis = 0; axis < 3; ++axis) score += direction[axis] * points[id][axis];
      std::size_t position = 0;
      while (position < pool_size) {
        counter_add(work_.selection_tests);
        if (score > pool[position].score ||
            (score == pool[position].score && id < pool[position].id)) {
          break;
        }
        ++position;
      }
      if (position < pool_size || pool_size < capacity) {
        counter_add(work_.pool_insertions);
        counter_add(work_.pool_shifted_entries, static_cast<u64>(pool_size - position));
        for (auto target = pool_size; target > position; --target) {
          pool[target] = pool[target - 1];
        }
        pool[position] = {score, id};
        if (pool_size < capacity) ++pool_size;
      }
    }
    counter_add(work_.pool_selected, static_cast<u64>(pool_size));
    std::vector<std::uint8_t> result(factor.size(), 0);
    for (auto rank = factor.first; rank < factor.last; ++rank) {
      counter_add(work_.factor_visits);
      counter_add(work_.certification_anchor_visits);
      const auto id = order[rank];
      auto& value = result[rank - factor.first];
      for (std::size_t proposal_index = 0; proposal_index < pool_size; ++proposal_index) {
        counter_add(work_.witness_attempts);
        const auto& proposal = pool[proposal_index];
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

  const Q2CensusIndex* const index_;
  std::size_t a_node_{}, b_node_{};
  Range a_{}, b_{};
  unsigned kmax_{};
  std::vector<std::uint8_t> a_credits_, b_credits_;
  std::vector<std::size_t> a_ranks_, b_order_;
  std::array<Range, 11> a_groups_{}, b_groups_{};
  std::array<std::size_t, 11> prefixes_{};
  u64 total_{}, candidates_{};
  std::size_t max_prefix_{};
  Q2NodePoolWork work_{};
};

}  // namespace mhgp8::detail
