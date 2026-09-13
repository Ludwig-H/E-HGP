#pragma once

#include "local_credits.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
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
  u64 contained_nodes{};
  u64 disjoint_nodes{};
  u64 whole_factor_accepts{};
  u64 whole_factor_rejects{};
  u64 emitted_blocks{};
  u64 max_tree_depth{};
};

// A range in b_order(), not in the original point array. Each block contains
// one original A point ID and a disjoint contiguous part of the B permutation.
struct AxisQ2Block {
  std::size_t a_id{};
  Range b;
};

class AxisQ2Plan;
[[nodiscard]] AxisQ2Plan make_axis_q2_plan(RectanglePtr rectangle);

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
  explicit AxisQ2Plan(RectanglePtr rectangle);
  friend AxisQ2Plan make_axis_q2_plan(RectanglePtr rectangle);

  RectanglePtr rectangle_;
  std::uint8_t need_{};
  std::vector<Box3> anchor_bounds_;
  std::vector<std::size_t> b_order_;
  std::vector<AxisQ2Block> blocks_;
  AxisQ2Work work_;
  u64 total_{};
  u64 candidates_{};
};

}  // namespace mhgp8
