#pragma once

#include "morsehgp3d/spatial/h_polytope_reference.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class HPolytopeProposalContextState;
}  // namespace detail

enum class HPolytopeProposalBatchDecision : std::uint8_t {
  exact_recertified_local,
  insufficient_exact_budget,
};

[[nodiscard]] std::string_view to_string(
    HPolytopeProposalBatchDecision decision);

enum class HPolytopeProposalCellStatus : std::uint8_t {
  validated_exhaustive_transcript,
  exact_fallback_interval_unknown,
  exact_fallback_capacity_exhausted,
  exact_fallback_unsupported_projection,
};

[[nodiscard]] std::string_view to_string(
    HPolytopeProposalCellStatus status);

// These are proposal statuses only.  None is a scientific decision: every
// plane triple is replayed against the exact Phase 7.6 reference geometry.
enum class HPolytopeProposalRecordStatus : std::uint8_t {
  unknown_requires_cpu_exact,
  proposed_strict_reject,
  proposed_survivor,
};

[[nodiscard]] std::string_view to_string(
    HPolytopeProposalRecordStatus status);

struct HPolytopeProposalCoordinateIntervals3 {
  std::array<std::uint64_t, 3> lower_binary64_bits{};
  std::array<std::uint64_t, 3> upper_binary64_bits{};

  friend bool operator==(
      const HPolytopeProposalCoordinateIntervals3&,
      const HPolytopeProposalCoordinateIntervals3&) = default;
};

struct HPolytopeProposalRecord {
  std::uint64_t cell_id{0U};
  std::size_t first_boundary_index{0U};
  std::size_t second_boundary_index{0U};
  std::size_t third_boundary_index{0U};
  HPolytopeProposalRecordStatus status{
      HPolytopeProposalRecordStatus::unknown_requires_cpu_exact};
  std::optional<std::size_t> strict_reject_boundary_witness;
  std::optional<HPolytopeProposalCoordinateIntervals3>
      survivor_coordinate_intervals;
  std::uint64_t could_be_active_boundary_mask{0U};

  friend bool operator==(
      const HPolytopeProposalRecord&,
      const HPolytopeProposalRecord&) = default;
};

struct HPolytopeProposalCellRequirements {
  std::uint64_t cell_id{0U};
  spatial::ExactBoundedHPolytopeReferenceRequirements exact_requirements;
  std::size_t boundary_plane_count{0U};
  std::size_t exhaustive_proposal_record_count{0U};

  friend bool operator==(
      const HPolytopeProposalCellRequirements&,
      const HPolytopeProposalCellRequirements&) = default;
};

struct HPolytopeProposalCellResult {
  std::uint64_t cell_id{0U};
  HPolytopeProposalCellStatus status{
      HPolytopeProposalCellStatus::exact_fallback_interval_unknown};
  spatial::ExactBoundedHPolytopeReferenceResult exact_result;

  friend bool operator==(
      const HPolytopeProposalCellResult&,
      const HPolytopeProposalCellResult&) = default;
};

struct HPolytopeProposalAudit {
  static constexpr const char* proposal_semantics =
      "proposal_only_exhaustive_plane_triple_transcript";
  static constexpr const char* decision_semantics =
      "reference_cpu_exact_all_constraints";

  std::size_t canonical_input_cell_count{0U};
  std::size_t projectable_cell_count{0U};
  std::size_t validated_exhaustive_transcript_cell_count{0U};
  std::size_t interval_unknown_fallback_cell_count{0U};
  std::size_t capacity_exhausted_fallback_cell_count{0U};
  std::size_t unsupported_projection_fallback_cell_count{0U};
  std::size_t physical_proposal_record_capacity{0U};
  std::size_t required_exhaustive_proposal_record_count{0U};
  std::size_t initialized_proposal_record_count{0U};
  std::size_t unknown_proposal_record_count{0U};
  std::size_t strict_reject_proposal_record_count{0U};
  std::size_t survivor_proposal_record_count{0U};
  std::size_t exact_plane_triple_replay_count{0U};
  std::size_t exact_unique_vertex_recertification_count{0U};
  std::size_t gpu_launch_count{0U};
  std::uint64_t buffer_epoch{0U};
  std::uint64_t proposal_digest_fnv1a{0U};
  bool canonical_cell_order_validated{false};
  bool exhaustive_transcript_validated{false};
  bool cpu_exact_recertification_complete{false};
  bool global_status_published{false};

  friend bool operator==(
      const HPolytopeProposalAudit&,
      const HPolytopeProposalAudit&) = default;
};

struct HPolytopeProposalBatchResult {
  HPolytopeProposalBatchDecision decision{
      HPolytopeProposalBatchDecision::insufficient_exact_budget};
  std::vector<HPolytopeProposalCellRequirements> requirements;
  std::vector<HPolytopeProposalCellResult> cell_results;
  // CSR offsets index proposal_records by canonical cell_results order.
  // Every fallback row is empty.
  std::vector<std::size_t> proposal_offsets;
  std::vector<HPolytopeProposalRecord> proposal_records;
  HPolytopeProposalAudit audit;

  friend bool operator==(
      const HPolytopeProposalBatchResult&,
      const HPolytopeProposalBatchResult&) = default;
};

// A context is tied to one exact clipping box and one explicit physical
// transcript capacity.  The capacity bounds allocation only; it is not a
// scientific or global cell-count cap.
class HPolytopeProposalContext final {
 public:
  HPolytopeProposalContext(
      spatial::ExactDyadicAabb3 clipping_box,
      std::size_t maximum_total_proposal_record_count);
  ~HPolytopeProposalContext() noexcept;

  HPolytopeProposalContext(HPolytopeProposalContext&&) noexcept;
  HPolytopeProposalContext& operator=(
      HPolytopeProposalContext&&) noexcept;

  HPolytopeProposalContext(const HPolytopeProposalContext&) = delete;
  HPolytopeProposalContext& operator=(
      const HPolytopeProposalContext&) = delete;

  // A build is a nonempty transaction: cell_ids must contain at least one
  // unique id and halfspace_offsets must be its closing CSR of size C + 1.
  [[nodiscard]] HPolytopeProposalBatchResult build(
      std::span<const std::uint64_t> cell_ids,
      std::span<const std::size_t> halfspace_offsets,
      std::span<const spatial::ExactHPolytopeHalfspace3> halfspaces,
      spatial::ExactBoundedHPolytopeReferenceBudget exact_budget = {});

  [[nodiscard]] const spatial::ExactDyadicAabb3& clipping_box() const
      noexcept {
    return clipping_box_;
  }

  [[nodiscard]] std::size_t maximum_total_proposal_record_count() const
      noexcept {
    return maximum_total_proposal_record_count_;
  }

 private:
  std::shared_ptr<detail::HPolytopeProposalContextState> state_;
  spatial::ExactDyadicAabb3 clipping_box_;
  std::size_t maximum_total_proposal_record_count_{0U};
};

}  // namespace morsehgp3d::gpu
