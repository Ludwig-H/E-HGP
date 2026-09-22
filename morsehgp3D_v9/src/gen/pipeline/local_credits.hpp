#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "prepared_cloud.hpp"

namespace mhgp9::gen {

struct RectangleInput {
  std::vector<Point3> points;
  Range a;
  Range b;
  // Proposals, not an externally trusted count. All IDs are checked; only
  // strictly certified universal sites outside A union B receive credit.
  std::vector<std::size_t> core_candidates;
};

// No coordinate storage. Ranges still refer to ORIGINAL IDs; a future WSPD
// permutation must be represented explicitly rather than relabeling IDs.
struct RectangleSpec {
  Range a;
  Range b;
  std::vector<std::size_t> core_candidates;
};

struct Work {
  PredicateWork predicates{};
  std::uint64_t validation_points{};
  std::uint64_t uniqueness_comparisons{};
  std::uint64_t pool_selection_tests{};
  std::uint64_t pool_selected{};
  std::uint64_t tree_nodes{};
  std::uint64_t tree_point_visits{};
  std::uint64_t dual_tasks{};
  std::uint64_t credited_blocks{};
  std::uint64_t noncredit_blocks{};
  std::uint64_t leaf_pairs{};
  std::uint64_t saturated_tasks{};
  std::uint64_t max_tree_depth{};
  std::uint64_t max_task_depth{};
  std::uint64_t tube_records{};
  std::uint64_t tube_cells{};
  std::uint64_t tube_sort_comparisons{};
  std::uint64_t tube_sweep_tests{};
  std::uint64_t tube_credited_sites{};
  std::uint64_t tube_separation_fallbacks{};
};

class PreparedRectangle;
using RectanglePtr = std::shared_ptr<const PreparedRectangle>;

// Throws invalid_argument for an unsupported/invalid input, before exposing
// any plan. Separation convention: box_gap >= s * max(box_diameter).
// Copies into private storage before certification. Even an rvalue argument
// is not consumed: moving a vector would retain the caller's mutable aliases.
[[nodiscard]] RectanglePtr prepare_rectangle(const RectangleInput& input,
                                             unsigned kmax,
                                             unsigned separation_s);
// Shares the validated cloud. Only factor boxes, separation and core belong
// to this preparation; global copy/uniqueness/index costs are paid once.
[[nodiscard]] RectanglePtr prepare_rectangle(CloudPtr cloud, const RectangleSpec& input,
                                             unsigned kmax, unsigned separation_s);

class PreparedRectangle final {
 public:
  PreparedRectangle(const PreparedRectangle&) = delete;
  PreparedRectangle& operator=(const PreparedRectangle&) = delete;
  PreparedRectangle(PreparedRectangle&&) = delete;
  PreparedRectangle& operator=(PreparedRectangle&&) = delete;
  [[nodiscard]] const PreparedCloud& cloud() const noexcept { return *cloud_; }
  [[nodiscard]] const CloudPtr& cloud_ptr() const noexcept { return cloud_; }
  [[nodiscard]] std::span<const Point3> points() const noexcept { return cloud_->points(); }
  [[nodiscard]] u64 factor_box_visits() const noexcept { return factor_box_visits_; }
  [[nodiscard]] u64 factor_box_steps() const noexcept { return factor_box_steps_; }
  [[nodiscard]] Range a_range() const noexcept { return a_; }
  [[nodiscard]] Range b_range() const noexcept { return b_; }
  [[nodiscard]] const Box3& a_box() const noexcept { return box_a_; }
  [[nodiscard]] const Box3& b_box() const noexcept { return box_b_; }
  [[nodiscard]] unsigned kmax() const noexcept { return kmax_; }
  [[nodiscard]] unsigned separation_s() const noexcept { return separation_s_; }
  [[nodiscard]] std::uint8_t threshold(Lane lane) const;
  [[nodiscard]] std::uint8_t core_credit(Lane lane) const;
  [[nodiscard]] const Work& preparation_work() const noexcept { return work_; }

 private:
  PreparedRectangle() = default;
  friend RectanglePtr prepare_rectangle(const RectangleInput&, unsigned, unsigned);
  friend RectanglePtr prepare_rectangle(CloudPtr, const RectangleSpec&, unsigned, unsigned);
  [[nodiscard]] static std::shared_ptr<PreparedRectangle> build(
      CloudPtr, Range, Range, std::span<const std::size_t>, unsigned, unsigned);
  CloudPtr cloud_;
  u64 factor_box_visits_{};
  u64 factor_box_steps_{};
  Range a_;
  Range b_;
  Box3 box_a_;
  Box3 box_b_;
  unsigned kmax_{};
  unsigned separation_s_{};
  std::array<std::uint8_t, 3> core_{};
  Work work_;
};

enum class Strategy { Pool, DualBlocks, Tubes };

// Index ranges into the plan's grouped a_order()/b_order(), not point ranges.
struct CandidateBlock {
  Range a;
  Range b;
};

class CreditPlan;
class CreditBatch;
[[nodiscard]] CreditPlan make_credit_plan(RectanglePtr rectangle,
                                          Lane lane, Strategy strategy);
// Three geometric lanes, NOT an HGP hierarchy tower. Tubes share their
// factor preparation; the other strategies keep their independent work.
[[nodiscard]] CreditBatch make_credit_batch(RectanglePtr rectangle, Strategy strategy);

class CreditPlan final {
 public:
  CreditPlan(const CreditPlan&) = default;
  CreditPlan(CreditPlan&& other) noexcept { swap(other); }
  CreditPlan& operator=(const CreditPlan& other) {
    if (this != &other) {
      CreditPlan replacement(other);
      swap(replacement);
    }
    return *this;
  }
  CreditPlan& operator=(CreditPlan&& other) noexcept {
    if (this != &other) {
      CreditPlan replacement(std::move(other));
      swap(replacement);
    }
    return *this;
  }
  [[nodiscard]] const PreparedRectangle& rectangle() const {
    if (!rectangle_) {
      throw std::logic_error("mhgp9 gen moved-from credit plan has no geometry");
    }
    return *rectangle_;
  }
  [[nodiscard]] Lane lane() const noexcept { return lane_; }
  [[nodiscard]] Strategy strategy() const noexcept { return strategy_; }
  [[nodiscard]] std::uint8_t threshold() const noexcept { return threshold_; }
  [[nodiscard]] std::uint8_t core_credit() const noexcept { return core_; }
  [[nodiscard]] std::span<const std::uint8_t> a_credits() const noexcept { return a_; }
  [[nodiscard]] std::span<const std::uint8_t> b_credits() const noexcept { return b_; }
  [[nodiscard]] std::span<const std::size_t> a_order() const noexcept { return a_order_; }
  [[nodiscard]] std::span<const std::size_t> b_order() const noexcept { return b_order_; }
  [[nodiscard]] std::span<const CandidateBlock> blocks() const noexcept { return blocks_; }
  [[nodiscard]] std::uint64_t total_pairs() const noexcept { return total_; }
  [[nodiscard]] std::uint64_t candidate_pairs() const noexcept { return candidates_; }
  [[nodiscard]] const Work& work() const noexcept { return work_; }
  [[nodiscard]] bool keeps(std::size_t a_id, std::size_t b_id) const;

  // Explicit expansion is consumer work O(candidate_pairs), never required
  // merely to construct/count this plan. The callback receives original IDs.
  template <class Consumer>
  void for_each_candidate(Consumer&& consumer) const {
    static_cast<void>(rectangle());
    for (const auto& block : blocks_)
      for (std::size_t a = block.a.first; a < block.a.last; ++a)
        for (std::size_t b = block.b.first; b < block.b.last; ++b)
          consumer(a_order_[a], b_order_[b]);
  }

 private:
  CreditPlan() = default;
  friend CreditPlan make_credit_plan(RectanglePtr, Lane, Strategy);
  friend CreditBatch make_credit_batch(RectanglePtr, Strategy);
  friend class CreditBatch;
  // Copy assignment must not install a new owner before allocating all of
  // its associated arrays. Swapping a complete replacement cannot throw.
  void swap(CreditPlan& other) noexcept {
    rectangle_.swap(other.rectangle_);
    std::swap(lane_, other.lane_);
    std::swap(strategy_, other.strategy_);
    std::swap(threshold_, other.threshold_);
    std::swap(core_, other.core_);
    a_.swap(other.a_);
    b_.swap(other.b_);
    a_order_.swap(other.a_order_);
    b_order_.swap(other.b_order_);
    blocks_.swap(other.blocks_);
    std::swap(total_, other.total_);
    std::swap(candidates_, other.candidates_);
    std::swap(work_, other.work_);
  }
  void initialize(RectanglePtr rectangle, Lane lane, Strategy strategy);
  void group_residual();
  RectanglePtr rectangle_;
  Lane lane_{Lane::Q2};
  Strategy strategy_{Strategy::Pool};
  std::uint8_t threshold_{};
  std::uint8_t core_{};
  std::vector<std::uint8_t> a_;
  std::vector<std::uint8_t> b_;
  std::vector<std::size_t> a_order_;
  std::vector<std::size_t> b_order_;
  std::vector<CandidateBlock> blocks_;
  std::uint64_t total_{};
  std::uint64_t candidates_{};
  Work work_;
};

class CreditBatch final {
 public:
  CreditBatch(const CreditBatch&) = default;
  CreditBatch(CreditBatch&&) noexcept = default;
  CreditBatch& operator=(const CreditBatch& other) {
    if (this != &other) {
      CreditBatch replacement(other);
      swap(replacement);
    }
    return *this;
  }
  CreditBatch& operator=(CreditBatch&& other) noexcept {
    if (this != &other) {
      CreditBatch replacement(std::move(other));
      swap(replacement);
    }
    return *this;
  }
  [[nodiscard]] const CreditPlan& plan(Lane lane) const {
    return plans_[arity(lane) - 2];
  }
  // Count once, not once per plan. For Tubes, per-plan work contains only
  // the lane-dependent sweep, whereas grid/sort/cells belong here.
  [[nodiscard]] const Work& shared_work() const noexcept { return shared_work_; }

 private:
  friend CreditBatch make_credit_batch(RectanglePtr, Strategy);
  void swap(CreditBatch& other) noexcept {
    for (std::size_t index = 0; index < plans_.size(); ++index) {
      plans_[index].swap(other.plans_[index]);
    }
    std::swap(shared_work_, other.shared_work_);
  }
  CreditBatch(std::array<CreditPlan, 3> plans, Work shared_work)
      : plans_(std::move(plans)), shared_work_(shared_work) {}
  std::array<CreditPlan, 3> plans_;
  Work shared_work_;
};

}  // namespace mhgp9::gen
