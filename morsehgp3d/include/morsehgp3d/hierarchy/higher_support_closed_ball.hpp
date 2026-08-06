#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// Outcome of one exact closed-ball classification of a minimal
// higher-support sphere.  The rank exit is taken as soon as the strict
// interior exceeds its cap, which is exactly the statement that the
// support is above the relevant closed rank: the interior count is
// monotone along the traversal, so the outcome does not depend on the
// visiting order.
enum class ExactHigherSupportClosedBallOutcome : std::uint8_t {
  complete,
  rank_exceeded,
};

// Traversal work actually performed by one indexed closed-ball query.
// The host stream builder folds these into its audit; the device-tiled
// bridge reports them in its tile payload.  They are observability, never
// a correctness argument.
struct ExactHigherSupportClosedBallCounters {
  std::size_t node_visit_count{};
  std::size_t bulk_interior_subtree_count{};
  std::size_t bulk_exterior_subtree_count{};
  std::size_t exact_point_distance_evaluation_count{};
  std::size_t point_classification_count{};
  std::size_t maximum_traversal_frontier_entry_count{};
};

struct ExactHigherSupportClosedBallClassification {
  ExactHigherSupportClosedBallOutcome outcome{
      ExactHigherSupportClosedBallOutcome::rank_exceeded};
  // Sorted by canonical PointId; meaningful only when the outcome is
  // complete.
  std::vector<spatial::PointId> interior_ids;
  std::size_t shell_count{};
  std::optional<spatial::PointId> canonical_extra_shell_witness_id;
  std::size_t exterior_count{};
  ExactHigherSupportClosedBallCounters counters{};
};

// The single definition of the exact closed-ball classification of one
// minimal support sphere against the certified Morton LBVH: bulk
// interior and exterior subtree decisions taken from the exact node
// distance bounds, exact per-point sphere classification only where the
// sphere actually cuts a leaf, and the rank exit above.  A complete
// traversal certifies its own partition (every cloud point classified
// exactly once, every support point observed on the shell), so the
// sparsity of the query never weakens the exactness of the record it
// produces.
//
// Both the host stream builder (fresh-replay basis) and the device-tiled
// tile-certified bridge consume this one implementation: the two
// verification bases therefore cannot drift on the geometry they record.
class ExactHigherSupportIndexedClosedBallQuery {
 public:
  // Proved depth-first stack bound of the traversal: the stack holds at
  // most one entry per tree level, root included.
  [[nodiscard]] static std::size_t proved_traversal_frontier_bound(
      const spatial::MortonLbvhIndex& index);

  [[nodiscard]] static ExactHigherSupportClosedBallClassification classify(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const std::array<spatial::PointId, 4>& support_ids,
      std::size_t support_size,
      const exact::ExactCenter3& center,
      const exact::ExactLevel& squared_level,
      std::size_t interior_cap,
      std::size_t maximum_traversal_frontier_entry_count);
};

}  // namespace morsehgp3d::hierarchy
