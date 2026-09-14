#pragma once

#include "axis_q2.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <vector>

namespace mhgp8 {

enum class Q2CensusMode { Pairwise, SharedBlocks };

struct Q2IndexWork {
  u64 point_visits{};
  u64 nodes{};
  u64 max_depth{};
  u64 escape_links{};  // One validated preorder escape assigned per global node.
};

struct Q2CensusWork {
  u64 query_build_point_visits{};
  u64 query_build_nodes{};
  u64 query_build_max_depth{};
  u64 input_descriptors{};
  u64 query_cover_visits{};
  u64 query_tasks{};
  u64 query_splits{};
  u64 witness_splits{};
  u64 count_root_starts{};
  u64 shared_splits_after_credit{};
  u64 cursor_advances{};  // Shared transitions to an escape or first Z child.
  u64 cursor_reuses{};    // Two inherited cursor values per query split.
  u64 count_node_visits{};
  u64 count_bound_tests{};
  u64 count_point_tests{};
  // Events affecting >1 query pair. Crediting can occur several times per
  // pair; acceptance/rejection occurs exactly once for any surviving pair.
  u64 uniform_credited_pairs{};
  u64 uniform_rejected_pairs{};
  u64 uniform_accepted_pairs{};
  // Sum of Z populations consumed by tasks, without multiplication by |B|.
  u64 consumed_witness_sites{};
  u64 frontier_restarts{};  // Root starts with a nonzero acquired count: forbidden.
  u64 payload_node_visits{};
  u64 payload_bound_tests{};
  u64 payload_point_tests{};
  u64 payload_interior_sites{};
  u64 payload_shell_sites{};
  u64 payload_supports{};
};

struct Q2BallKey {
  std::array<std::uint32_t, 3> center_twice{};
  u64 diameter_squared{};
  bool operator==(const Q2BallKey&) const = default;
};

// Borrowed only during the synchronous callback. The shell includes both
// support endpoints. IDs are original IDs, not spatial-permutation positions.
// Equal keys from different supports remain separate incidences in this flux.
struct Q2Support {
  std::size_t a_id{};
  std::size_t b_id{};
  Q2BallKey key;
  std::span<const std::size_t> interior;
  std::span<const std::size_t> shell;
};

using Q2CensusConsumer = std::function<void(const Q2Support&)>;

struct Q2CensusResult {
  u64 candidate_pairs{};
  u64 accepted_pairs{};
  u64 rejected_pairs{};
  Q2CensusWork work;
  double query_index_ms{};
  double count_ms{};    // Residual total: includes traversal/instrumentation overhead.
  double payload_ms{};  // Second indexed collection and synchronous callback.
  double total_ms{};    // One enclosing clock, not a sum of disjoint intervals.
};

class Q2CensusIndex;
using Q2CensusIndexPtr = std::shared_ptr<const Q2CensusIndex>;
[[nodiscard]] Q2CensusIndexPtr make_q2_census_index(RectanglePtr rectangle);
[[nodiscard]] Q2CensusIndexPtr make_q2_cloud_index(CloudPtr cloud);

// Ranges are ranks in spatial_order(), never original point IDs. Nodes and
// their boxes are immutable certificates built with this particular index.
struct Q2SpatialNode {
  static constexpr std::size_t absent = std::numeric_limits<std::size_t>::max();
  Range range;
  Box3 box;
  std::size_t left{absent};
  std::size_t right{absent};
  std::size_t escape{absent};
};

// An immutable index over EVERY point of this owner, not just A union B.
// The owner is already validated; its coordinates are neither copied nor
// revalidated. Noncopyability prevents mutation through an aliased copy.
class Q2CensusIndex final {
 public:
  Q2CensusIndex(const Q2CensusIndex&) = delete;
  Q2CensusIndex& operator=(const Q2CensusIndex&) = delete;
  Q2CensusIndex(Q2CensusIndex&&) = delete;
  Q2CensusIndex& operator=(Q2CensusIndex&&) = delete;
  [[nodiscard]] const PreparedCloud& cloud() const noexcept { return *cloud_; }
  [[nodiscard]] const Q2IndexWork& work() const noexcept { return work_; }
  [[nodiscard]] std::span<const std::size_t> spatial_order() const noexcept { return order_; }
  [[nodiscard]] std::span<const Q2SpatialNode> spatial_nodes() const noexcept { return nodes_; }
  // Vector capacities only, excluding the shared cloud and object metadata.
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  using Node = Q2SpatialNode;
  static constexpr std::size_t absent = Node::absent;
  explicit Q2CensusIndex(CloudPtr cloud);
  [[nodiscard]] std::size_t build(Range range, u64 depth);
  friend Q2CensusIndexPtr make_q2_cloud_index(CloudPtr);
  friend struct Q2CensusEngine;
  CloudPtr cloud_;
  std::vector<std::size_t> order_;
  std::vector<Node> nodes_;
  Q2IndexWork work_;
};

// No initial credit parameter: all census counts start from zero. The axis
// plan is only a prefilter; its core/local credits cannot be counted twice.
// Index and plan must refer to the same immutable CLOUD. The threshold comes
// from the plan's rectangle; the index has no rectangle, core or Kmax. A moved
// axis plan, invalid mode, or empty callback is rejected before processing.
// Borrowing contract: index, plan and consumer must remain alive and must not
// be invalidated for the ENTIRE call, including callbacks (in particular, do
// not move the plan from a callback). An exception propagates; previously
// emitted incidences are not rolled back. A retry starts from zero, and the
// caller must discard or otherwise distinguish any incomplete prior output.
[[nodiscard]] Q2CensusResult run_q2_census(
    const Q2CensusIndex& index, const AxisQ2Plan& plan, Q2CensusMode mode,
    const Q2CensusConsumer& consumer);

}  // namespace mhgp8
