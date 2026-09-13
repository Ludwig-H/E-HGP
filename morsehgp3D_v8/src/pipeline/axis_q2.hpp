#pragma once

#include "local_credits.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp8 {

struct AxisQ2Work {
  u64 sort_passes{};
  u64 sorted_sites{};
  u64 sort_comparisons{};
  u64 columns{};
  u64 constrained_anchors{};
  u64 slab_bound_updates{};
  u64 tree_point_visits{};
  u64 tree_nodes{};
  u64 query_nodes{};
  u64 contained_nodes{};  // Accepted tree ranges, before optional coalescing.
  u64 disjoint_nodes{};   // Rejected tree ranges (not necessarily spatially disjoint).
  u64 whole_factor_accepts{};
  u64 whole_factor_rejects{};
  u64 emitted_blocks{};   // Final stored descriptors, excluding absorbed ranges.
  u64 max_tree_depth{};
  // New modes only; the no-options Independent path leaves these at zero.
  u64 axis_bound_queries{};         // Bounds evaluated after restriction/slab shortcuts.
  u64 axis_count_queries{};         // Evaluations of a one-dimensional strict count.
  u64 axis_rank_comparisons{};      // Coordinate comparisons in bounded rank searches.
  u64 axis_pruned_nodes{};          // Sum-bound rejections, excluding slab shortcuts.
  u64 axis_slab_rejects{};          // Cheap independent-slab exclusions, including roots.
  u64 restriction_credit_copies{}; // A+B copied once; zero when need=0.
  u64 restriction_credit_visits{}; // Credit scans for factor/node extrema.
  u64 restriction_bound_queries{}; // First check of every classified box if restricted.
  u64 restriction_pruned_nodes{};  // Includes factor roots.
  u64 coalesced_blocks{};          // Accepted ranges absorbed by their predecessor.
};

enum class AxisQ2Mode { Independent, Additive };

// A range in b_order(), not in the original point array. Each block contains
// one original A point ID and a disjoint contiguous part of the B permutation.
struct AxisQ2Block {
  std::size_t a_id{};
  Range b;
};

class AxisQ2Plan;
// A restriction intersects two residuals; its credits are NEVER added to the
// axial credits. It must be q2 on this exact owner, and is privately copied.
[[nodiscard]] AxisQ2Plan make_axis_q2_plan(
    RectanglePtr rectangle, AxisQ2Mode mode = AxisQ2Mode::Independent,
    const CreditPlan* restriction = nullptr);

class AxisQ2Plan final {
 public:
  AxisQ2Plan(const AxisQ2Plan&) = delete;
  AxisQ2Plan& operator=(const AxisQ2Plan&) = delete;
  AxisQ2Plan(AxisQ2Plan&& other) noexcept;
  AxisQ2Plan& operator=(AxisQ2Plan&&) = delete;

  [[nodiscard]] const PreparedRectangle& rectangle() const {
    if (!rectangle_) {
      throw std::logic_error("mhgp8 axis plan was moved from");
    }
    return *rectangle_;
  }
  [[nodiscard]] std::uint8_t need() const noexcept { return need_; }
  [[nodiscard]] AxisQ2Mode mode() const noexcept { return mode_; }
  [[nodiscard]] bool has_restriction() const noexcept { return has_restriction_; }
  [[nodiscard]] bool keeps(std::size_t a_id, std::size_t b_id) const;
  [[nodiscard]] u64 candidate_pairs() const noexcept { return candidates_; }
  [[nodiscard]] u64 total_pairs() const noexcept { return total_; }
  [[nodiscard]] const AxisQ2Work& work() const noexcept { return work_; }
  [[nodiscard]] std::span<const AxisQ2Block> blocks() const noexcept { return blocks_; }
  [[nodiscard]] std::span<const std::size_t> b_order() const noexcept { return b_order_; }

  // Expanding the represented residual really pays O(candidate_pairs).
  // Neither construction nor counting invokes this callback implicitly.
  template <class Consumer>
  void for_each_candidate(Consumer&& consumer) const {
    if (!rectangle_) {
      throw std::logic_error("mhgp8 axis plan was moved from");
    }
    for (const auto& block : blocks_) {
      for (std::size_t index = block.b.first; index < block.b.last; ++index) {
        consumer(block.a_id, b_order_[index]);
      }
    }
  }

 private:
  AxisQ2Plan(RectanglePtr rectangle, AxisQ2Mode mode, const CreditPlan* restriction);
  friend AxisQ2Plan make_axis_q2_plan(RectanglePtr, AxisQ2Mode, const CreditPlan*);

  struct ColumnWindow {
    std::size_t first{};
    std::size_t rank{};
    std::size_t last{};
  };
  [[nodiscard]] unsigned axis_count(std::size_t a_index, std::size_t axis,
                                    std::uint16_t value, AxisQ2Work* work) const;
  [[nodiscard]] std::pair<unsigned, unsigned> axis_bounds(
      std::size_t a_index, const Box3& box, AxisQ2Work* work) const;

  RectanglePtr rectangle_;
  std::uint8_t need_{};
  AxisQ2Mode mode_{AxisQ2Mode::Independent};
  bool has_restriction_{};
  std::vector<Box3> anchor_bounds_;
  // Only Additive retains the three permutations and O(1) rank windows per
  // anchor/axis: at most need neighbours each way, without copying them.
  std::array<std::vector<std::size_t>, 3> column_orders_;
  std::vector<std::array<ColumnWindow, 3>> column_windows_;
  std::vector<std::uint8_t> restriction_a_;
  std::vector<std::uint8_t> restriction_b_;
  std::vector<std::size_t> b_order_;
  std::vector<AxisQ2Block> blocks_;
  AxisQ2Work work_;
  u64 total_{};
  u64 candidates_{};
};

}  // namespace mhgp8
