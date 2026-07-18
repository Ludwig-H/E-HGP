#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class SpatialBoundsContextState;
struct SpatialBoundsInputRecord;
}  // namespace detail

enum class DirectedEnclosureStatus : std::uint8_t {
  exact,
  enclosed,
  unsupported_range,
};

// Only prune is a certified terminal decision.  Visit is a safe traversal
// hint, while unknown requires an exact fallback without making any claim.
enum class SpatialBoundsDecision : std::uint8_t {
  prune,
  visit,
  unknown,
};

struct SpatialBoundsAudit {
  static constexpr const char* proposal_semantics =
      "gpu_outward_interval_aabb";
  static constexpr const char* decision_semantics =
      "cpu_exact_recertified_strict_prune";

  std::size_t gpu_input_box_count{0U};
  std::size_t gpu_output_record_count{0U};
  std::size_t gpu_unique_box_index_count{0U};
  std::size_t gpu_prune_proposal_count{0U};
  std::size_t gpu_visit_proposal_count{0U};
  std::size_t gpu_unknown_proposal_count{0U};
  std::size_t gpu_launch_count{0U};
  std::size_t cpu_exact_prune_recertification_count{0U};
  std::size_t certified_prune_count{0U};
  std::size_t unsupported_range_fallback_count{0U};
  std::uint64_t buffer_epoch{0U};
  std::uint64_t proposal_digest_fnv1a{0U};
  std::array<std::uint64_t, 3> query_lower_bits{};
  std::array<std::uint64_t, 3> query_upper_bits{};
  std::array<DirectedEnclosureStatus, 3> query_enclosure{};
  std::uint64_t cutoff_lower_bits{0U};
  std::uint64_t cutoff_upper_bits{0U};
  DirectedEnclosureStatus cutoff_enclosure{DirectedEnclosureStatus::exact};
  std::optional<exact::ExactLevel> minimum_certified_strict_margin;
  bool proposal_permutation_complete{false};
  bool cpu_exact_recertification_complete{false};
  bool all_boxes_classified{false};

  friend bool operator==(
      const SpatialBoundsAudit&,
      const SpatialBoundsAudit&) = default;
};

struct SpatialBoundsResult {
  // Decisions are indexed by the canonical input AABB position.  Every prune
  // has been recertified exactly; visit and unknown remain non-terminal hints.
  std::vector<SpatialBoundsDecision> decisions;
  SpatialBoundsAudit audit;
};

// The immutable AABB batch is copied into a persistent CUDA workspace on the
// first supported query.  A context serializes its own launches and is poisoned
// after any launch, copy-back, permutation, or exact-recertification failure.
class SpatialBoundsContext final {
 public:
  explicit SpatialBoundsContext(
      std::span<const spatial::ExactDyadicAabb3> boxes);
  ~SpatialBoundsContext() noexcept;

  SpatialBoundsContext(SpatialBoundsContext&&) noexcept;
  SpatialBoundsContext& operator=(SpatialBoundsContext&&) noexcept;

  SpatialBoundsContext(const SpatialBoundsContext&) = delete;
  SpatialBoundsContext& operator=(const SpatialBoundsContext&) = delete;

  [[nodiscard]] SpatialBoundsResult classify_strict_prune(
      const exact::ExactRational3& query,
      const exact::ExactLevel& squared_cutoff);

  [[nodiscard]] std::size_t box_count() const noexcept {
    return boxes_.size();
  }

 private:
  std::shared_ptr<detail::SpatialBoundsContextState> state_;
  std::vector<spatial::ExactDyadicAabb3> boxes_;
  std::vector<detail::SpatialBoundsInputRecord> packed_boxes_;
};

}  // namespace morsehgp3d::gpu
