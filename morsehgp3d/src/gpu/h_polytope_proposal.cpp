#include "morsehgp3d/gpu/h_polytope_proposal.hpp"

#include "phase7_h_polytope_proposal_internal.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/exact/binary64.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

struct CanonicalCellSlice {
  std::uint64_t cell_id{0U};
  std::size_t halfspace_begin{0U};
  std::size_t halfspace_end{0U};
};

struct BuiltCell {
  CanonicalCellSlice input;
  HPolytopeProposalCellRequirements requirements;
  spatial::ExactBoundedHPolytopeReferenceResult exact_result;
  bool unsupported_projection{false};
  bool force_interval_fallback{false};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* label) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(label);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    const char* label) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(label);
  }
  return left * right;
}

[[nodiscard]] std::size_t plane_triple_count(std::size_t boundary_count) {
  if (boundary_count < 3U) {
    return 0U;
  }
  return boundary_count * (boundary_count - 1U) *
         (boundary_count - 2U) / 6U;
}

void validate_exact_budget(
    const spatial::ExactBoundedHPolytopeReferenceBudget& budget) {
  using Budget = spatial::ExactBoundedHPolytopeReferenceBudget;
  if (budget.maximum_input_halfspace_count >
          Budget::trusted_maximum_input_halfspace_count ||
      budget.maximum_boundary_count >
          Budget::trusted_maximum_boundary_count ||
      budget.maximum_plane_triple_count >
          Budget::trusted_maximum_plane_triple_count ||
      budget.maximum_feasibility_test_count >
          Budget::trusted_maximum_feasibility_test_count ||
      budget.maximum_vertex_count >
          Budget::trusted_maximum_vertex_count ||
      budget.maximum_incidence_test_count >
          Budget::trusted_maximum_incidence_test_count) {
    throw std::invalid_argument(
        "an H-polytope proposal exact budget exceeds the trusted 7.6 cap");
  }
}

[[nodiscard]] bool exact_budget_covers(
    const spatial::ExactBoundedHPolytopeReferenceBudget& budget,
    const spatial::ExactBoundedHPolytopeReferenceRequirements& requirements) {
  return budget.maximum_input_halfspace_count >=
             requirements.input_halfspace_count &&
         budget.maximum_boundary_count >=
             requirements.conservative_boundary_count &&
         budget.maximum_plane_triple_count >=
             requirements.conservative_plane_triple_count &&
         budget.maximum_feasibility_test_count >=
             requirements.conservative_feasibility_test_count &&
         budget.maximum_vertex_count >=
             requirements.conservative_vertex_count &&
         budget.maximum_incidence_test_count >=
             requirements.conservative_incidence_test_count;
}

[[nodiscard]] spatial::ExactBoundedHPolytopeReferenceBudget zero_budget() {
  spatial::ExactBoundedHPolytopeReferenceBudget result;
  result.maximum_input_halfspace_count = 0U;
  result.maximum_boundary_count = 0U;
  result.maximum_plane_triple_count = 0U;
  result.maximum_feasibility_test_count = 0U;
  result.maximum_vertex_count = 0U;
  result.maximum_incidence_test_count = 0U;
  return result;
}

[[nodiscard]] std::vector<CanonicalCellSlice> canonical_cell_slices(
    std::span<const std::uint64_t> cell_ids,
    std::span<const std::size_t> halfspace_offsets,
    std::span<const spatial::ExactHPolytopeHalfspace3> halfspaces) {
  if (cell_ids.empty()) {
    throw std::invalid_argument(
        "an H-polytope proposal batch requires at least one cell");
  }
  if (cell_ids.size() == std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(
        "the H-polytope proposal cell-offset count overflowed");
  }
  if (halfspace_offsets.size() != cell_ids.size() + 1U ||
      halfspace_offsets.front() != 0U ||
      halfspace_offsets.back() != halfspaces.size()) {
    throw std::invalid_argument(
        "H-polytope proposal halfspace offsets do not close their CSR");
  }

  std::vector<CanonicalCellSlice> result;
  result.reserve(cell_ids.size());
  for (std::size_t index = 0U; index < cell_ids.size(); ++index) {
    const std::size_t begin = halfspace_offsets[index];
    const std::size_t end = halfspace_offsets[index + 1U];
    if (begin > end || end > halfspaces.size()) {
      throw std::invalid_argument(
          "H-polytope proposal halfspace offsets are not monotone");
    }
    if (end - begin >
        spatial::ExactBoundedHPolytopeReferenceBudget::
            trusted_maximum_input_halfspace_count) {
      throw std::invalid_argument(
          "an H-polytope proposal cell exceeds the 55-halfspace reference "
          "domain");
    }
    result.push_back(CanonicalCellSlice{cell_ids[index], begin, end});
  }
  std::sort(
      result.begin(),
      result.end(),
      [](const CanonicalCellSlice& left, const CanonicalCellSlice& right) {
        return left.cell_id < right.cell_id;
      });
  if (std::adjacent_find(
          result.begin(),
          result.end(),
          [](const CanonicalCellSlice& left, const CanonicalCellSlice& right) {
            return left.cell_id == right.cell_id;
          }) != result.end()) {
    throw std::invalid_argument(
        "H-polytope proposal cell ids must be unique");
  }
  return result;
}

[[nodiscard]] std::span<const spatial::ExactHPolytopeHalfspace3>
cell_halfspaces(
    std::span<const spatial::ExactHPolytopeHalfspace3> halfspaces,
    const CanonicalCellSlice& cell) {
  return halfspaces.subspan(
      cell.halfspace_begin, cell.halfspace_end - cell.halfspace_begin);
}

[[nodiscard]] std::size_t proper_boundary_count(
    std::span<const spatial::ExactHPolytopeHalfspace3> halfspaces) {
  std::size_t result = 6U;
  for (const spatial::ExactHPolytopeHalfspace3& halfspace : halfspaces) {
    if (exact::classify_affine_form(halfspace.retained_nonpositive_form)
            .kind() == exact::AffineFormKind::proper_plane) {
      ++result;
    }
  }
  return result;
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] bool all_zero(
    const std::array<std::uint64_t, 3>& words) noexcept {
  return words[0U] == 0U && words[1U] == 0U && words[2U] == 0U;
}

[[nodiscard]] bool is_zero_tail_record(
    const detail::HPolytopeProposalDeviceRecord& record) noexcept {
  return record.initialized_slot_sentinel == 0U &&
         record.buffer_epoch == 0U && record.cell_id == 0U &&
         record.first_boundary_index == 0U &&
         record.second_boundary_index == 0U &&
         record.third_boundary_index == 0U && record.status_code == 0U &&
         record.strict_reject_boundary_witness == 0U &&
         record.could_be_active_boundary_mask == 0U &&
         all_zero(record.coordinate_lower_bits) &&
         all_zero(record.coordinate_upper_bits);
}

[[nodiscard]] exact::ExactRational finite_canonical_binary64_rational(
    std::uint64_t bits) {
  try {
    if (exact::canonicalize_binary64_bits(bits) != bits) {
      throw std::runtime_error(
          "a proposal interval endpoint is not canonical binary64");
    }
    return exact::ExactRational::from_binary64_bits(bits);
  } catch (const std::exception&) {
    throw std::runtime_error(
        "a proposal interval endpoint is not canonical finite binary64");
  }
}

[[nodiscard]] bool exact_result_contains_vertex(
    const spatial::ExactBoundedHPolytopeReferenceResult& result,
    const exact::ExactRational3& point) {
  return std::any_of(
      result.vertices.begin(),
      result.vertices.end(),
      [&point](const spatial::ExactHPolytopeVertex& vertex) {
        return vertex.position == point;
      });
}

[[nodiscard]] std::uint64_t valid_boundary_mask(std::size_t boundary_count) {
  if (boundary_count == 0U || boundary_count > 61U) {
    throw std::logic_error(
        "an H-polytope proposal boundary mask is outside B <= 61");
  }
  return (UINT64_C(1) << boundary_count) - UINT64_C(1);
}

[[nodiscard]] HPolytopeProposalRecordStatus public_record_status(
    detail::HPolytopeProposalDeviceRecordStatus status) {
  switch (status) {
    case detail::HPolytopeProposalDeviceRecordStatus::
        unknown_requires_cpu_exact:
      return HPolytopeProposalRecordStatus::unknown_requires_cpu_exact;
    case detail::HPolytopeProposalDeviceRecordStatus::proposed_strict_reject:
      return HPolytopeProposalRecordStatus::proposed_strict_reject;
    case detail::HPolytopeProposalDeviceRecordStatus::proposed_survivor:
      return HPolytopeProposalRecordStatus::proposed_survivor;
  }
  throw std::runtime_error(
      "the H-polytope proposal returned an invalid record status");
}

[[nodiscard]] HPolytopeProposalCellStatus public_cell_status(
    detail::HPolytopeProposalDeviceCellStatus status) {
  switch (status) {
    case detail::HPolytopeProposalDeviceCellStatus::
        validated_exhaustive_transcript:
      return HPolytopeProposalCellStatus::validated_exhaustive_transcript;
    case detail::HPolytopeProposalDeviceCellStatus::
        exact_fallback_interval_unknown:
      return HPolytopeProposalCellStatus::exact_fallback_interval_unknown;
    case detail::HPolytopeProposalDeviceCellStatus::
        exact_fallback_capacity_exhausted:
      return HPolytopeProposalCellStatus::
          exact_fallback_capacity_exhausted;
    case detail::HPolytopeProposalDeviceCellStatus::
        exact_fallback_unsupported_projection:
      return HPolytopeProposalCellStatus::
          exact_fallback_unsupported_projection;
  }
  throw std::runtime_error(
      "the H-polytope proposal returned an invalid cell status");
}

void count_cell_status(
    HPolytopeProposalAudit& audit,
    HPolytopeProposalCellStatus status) {
  switch (status) {
    case HPolytopeProposalCellStatus::validated_exhaustive_transcript:
      ++audit.validated_exhaustive_transcript_cell_count;
      break;
    case HPolytopeProposalCellStatus::exact_fallback_interval_unknown:
      ++audit.interval_unknown_fallback_cell_count;
      break;
    case HPolytopeProposalCellStatus::exact_fallback_capacity_exhausted:
      ++audit.capacity_exhausted_fallback_cell_count;
      break;
    case HPolytopeProposalCellStatus::exact_fallback_unsupported_projection:
      ++audit.unsupported_projection_fallback_cell_count;
      break;
  }
}

[[nodiscard]] HPolytopeProposalBatchResult no_launch_fallback_result(
    std::vector<BuiltCell> built_cells,
    std::size_t physical_capacity,
    std::size_t required_record_count) {
  HPolytopeProposalBatchResult result;
  result.decision = HPolytopeProposalBatchDecision::exact_recertified_local;
  result.proposal_offsets.assign(built_cells.size() + 1U, 0U);
  result.audit.canonical_input_cell_count = built_cells.size();
  result.audit.physical_proposal_record_capacity = physical_capacity;
  result.audit.required_exhaustive_proposal_record_count =
      required_record_count;
  result.audit.canonical_cell_order_validated = true;
  result.audit.exhaustive_transcript_validated = true;
  result.audit.cpu_exact_recertification_complete = true;
  std::uint64_t digest = kFnvOffsetBasis;
  for (BuiltCell& cell : built_cells) {
    result.requirements.push_back(cell.requirements);
    const HPolytopeProposalCellStatus status = cell.unsupported_projection
        ? HPolytopeProposalCellStatus::exact_fallback_unsupported_projection
        : cell.force_interval_fallback
              ? HPolytopeProposalCellStatus::exact_fallback_interval_unknown
              : HPolytopeProposalCellStatus::
                    exact_fallback_capacity_exhausted;
    if (!cell.unsupported_projection) {
      ++result.audit.projectable_cell_count;
    }
    count_cell_status(result.audit, status);
    result.audit.exact_unique_vertex_recertification_count = checked_add(
        result.audit.exact_unique_vertex_recertification_count,
        cell.exact_result.vertices.size(),
        "the H-polytope exact vertex audit count overflowed");
    hash_word(digest, cell.input.cell_id);
    hash_word(digest, static_cast<std::uint64_t>(status));
    hash_word(digest, UINT64_C(0));
    result.cell_results.push_back(HPolytopeProposalCellResult{
        cell.input.cell_id, status, std::move(cell.exact_result)});
  }
  result.audit.proposal_digest_fnv1a = digest;
  return result;
}

[[nodiscard]] HPolytopeProposalBatchResult validate_device_batch(
    const detail::HPolytopeProposalDeviceBatch& batch,
    std::vector<BuiltCell> built_cells,
    std::size_t physical_capacity,
    std::size_t required_record_count) {
  if (batch.cell_ids.size() != built_cells.size() ||
      batch.cell_statuses.size() != built_cells.size() ||
      batch.cell_record_offsets.size() != built_cells.size() + 1U ||
      batch.cell_record_offsets.empty() ||
      batch.cell_record_offsets.front() != 0U ||
      batch.records.size() != physical_capacity ||
      !std::in_range<std::size_t>(batch.record_count) ||
      static_cast<std::size_t>(batch.record_count) > physical_capacity ||
      batch.cell_record_offsets.back() != batch.record_count ||
      batch.buffer_epoch == 0U) {
    throw std::runtime_error(
        "the H-polytope proposal returned malformed batch metadata");
  }
  const std::size_t record_count =
      static_cast<std::size_t>(batch.record_count);
  for (std::size_t index = 0U; index < built_cells.size(); ++index) {
    if (batch.cell_ids[index] != built_cells[index].input.cell_id ||
        batch.cell_record_offsets[index] >
            batch.cell_record_offsets[index + 1U] ||
        batch.cell_record_offsets[index + 1U] > batch.record_count) {
      throw std::runtime_error(
          "the H-polytope proposal changed a canonical cell or CSR offset");
    }
  }
  for (std::size_t index = 0U; index < record_count; ++index) {
    const auto& record = batch.records[index];
    if (record.initialized_slot_sentinel !=
            detail::kHPolytopeProposalInitializedSlotSentinel ||
        record.buffer_epoch != batch.buffer_epoch) {
      throw std::runtime_error(
          "the H-polytope proposal exposed a stale or uninitialized slot");
    }
  }
  for (std::size_t index = record_count; index < batch.records.size();
       ++index) {
    if (!is_zero_tail_record(batch.records[index])) {
      throw std::runtime_error(
          "the H-polytope proposal wrote outside its initialized prefix");
    }
  }

  HPolytopeProposalBatchResult result;
  result.decision = HPolytopeProposalBatchDecision::exact_recertified_local;
  result.audit.canonical_input_cell_count = built_cells.size();
  result.audit.physical_proposal_record_capacity = physical_capacity;
  result.audit.required_exhaustive_proposal_record_count =
      required_record_count;
  result.audit.initialized_proposal_record_count = record_count;
  result.audit.gpu_launch_count = 1U;
  result.audit.buffer_epoch = batch.buffer_epoch;
  result.audit.canonical_cell_order_validated = true;
  result.proposal_offsets.reserve(built_cells.size() + 1U);
  result.proposal_offsets.push_back(0U);
  result.proposal_records.reserve(record_count);
  std::size_t exact_vertex_count = 0U;
  std::uint64_t digest = kFnvOffsetBasis;

  for (std::size_t cell_index = 0U; cell_index < built_cells.size();
       ++cell_index) {
    BuiltCell& cell = built_cells[cell_index];
    exact_vertex_count = checked_add(
        exact_vertex_count,
        cell.exact_result.vertices.size(),
        "the H-polytope exact vertex audit count overflowed");
    result.requirements.push_back(cell.requirements);
    if (!cell.unsupported_projection) {
      ++result.audit.projectable_cell_count;
    }
    const HPolytopeProposalCellStatus cell_status =
        public_cell_status(batch.cell_statuses[cell_index]);
    const std::size_t begin = static_cast<std::size_t>(
        batch.cell_record_offsets[cell_index]);
    const std::size_t end = static_cast<std::size_t>(
        batch.cell_record_offsets[cell_index + 1U]);
    const std::size_t row_size = end - begin;
    if (cell.unsupported_projection) {
      if (cell_status != HPolytopeProposalCellStatus::
                             exact_fallback_unsupported_projection ||
          row_size != 0U) {
        throw std::runtime_error(
            "an unsupported H-polytope projection exposed a transcript");
      }
    } else if (cell.force_interval_fallback) {
      if (cell_status !=
              HPolytopeProposalCellStatus::exact_fallback_interval_unknown ||
          row_size != 0U) {
        throw std::runtime_error(
            "an exactly classified H-polytope cell exposed a transcript");
      }
    } else if (cell_status == HPolytopeProposalCellStatus::
                                  exact_fallback_unsupported_projection) {
      throw std::runtime_error(
          "the H-polytope proposal forged an unsupported projection");
    }
    if (cell_status !=
            HPolytopeProposalCellStatus::validated_exhaustive_transcript &&
        row_size != 0U) {
      throw std::runtime_error(
          "an H-polytope fallback cell exposed partial proposal records");
    }
    if (cell_status ==
            HPolytopeProposalCellStatus::validated_exhaustive_transcript &&
        row_size != cell.requirements.exhaustive_proposal_record_count) {
      throw std::runtime_error(
          "an H-polytope transcript omitted or added a plane triple");
    }
    count_cell_status(result.audit, cell_status);
    hash_word(digest, cell.input.cell_id);
    hash_word(digest, static_cast<std::uint64_t>(cell_status));
    hash_word(digest, static_cast<std::uint64_t>(row_size));

    if (cell_status ==
        HPolytopeProposalCellStatus::validated_exhaustive_transcript) {
      const std::size_t boundary_count =
          cell.exact_result.boundary_planes.size();
      const std::uint64_t valid_mask =
          valid_boundary_mask(boundary_count);
      std::size_t record_index = begin;
      for (std::size_t first = 0U; first < boundary_count; ++first) {
        for (std::size_t second = first + 1U; second < boundary_count;
             ++second) {
          for (std::size_t third = second + 1U; third < boundary_count;
               ++third) {
            if (record_index >= end) {
              throw std::runtime_error(
                  "an H-polytope transcript ended before its final ordinal");
            }
            const detail::HPolytopeProposalDeviceRecord& device_record =
                batch.records[record_index++];
            if (device_record.cell_id != cell.input.cell_id ||
                device_record.first_boundary_index != first ||
                device_record.second_boundary_index != second ||
                device_record.third_boundary_index != third) {
              throw std::runtime_error(
                  "an H-polytope proposal slot has the wrong combinadic "
                  "ordinal");
            }
            const auto record_status = public_record_status(
                static_cast<detail::HPolytopeProposalDeviceRecordStatus>(
                    device_record.status_code));
            const exact::ThreePlaneIntersection intersection =
                exact::intersect_three_planes(
                    cell.exact_result.boundary_planes[first].plane,
                    cell.exact_result.boundary_planes[second].plane,
                    cell.exact_result.boundary_planes[third].plane);
            ++result.audit.exact_plane_triple_replay_count;

            HPolytopeProposalRecord public_record;
            public_record.cell_id = cell.input.cell_id;
            public_record.first_boundary_index = first;
            public_record.second_boundary_index = second;
            public_record.third_boundary_index = third;
            public_record.status = record_status;
            switch (record_status) {
              case HPolytopeProposalRecordStatus::
                  unknown_requires_cpu_exact:
                if (device_record.strict_reject_boundary_witness !=
                        detail::kHPolytopeProposalNoBoundaryWitness ||
                    device_record.could_be_active_boundary_mask != 0U ||
                    !all_zero(device_record.coordinate_lower_bits) ||
                    !all_zero(device_record.coordinate_upper_bits)) {
                  throw std::runtime_error(
                      "an unknown H-polytope proposal carried a payload");
                }
                ++result.audit.unknown_proposal_record_count;
                break;
              case HPolytopeProposalRecordStatus::proposed_strict_reject: {
                if (device_record.strict_reject_boundary_witness >=
                        boundary_count ||
                    device_record.could_be_active_boundary_mask != 0U ||
                    !all_zero(device_record.coordinate_lower_bits) ||
                    !all_zero(device_record.coordinate_upper_bits) ||
                    intersection.kind() !=
                        exact::ThreePlaneIntersectionKind::unique ||
                    cell.exact_result
                            .boundary_planes[static_cast<std::size_t>(
                                device_record
                                    .strict_reject_boundary_witness)]
                            .retained_nonpositive_form
                            .evaluate(*intersection.point())
                            .sign() <= 0) {
                  throw std::runtime_error(
                      "an H-polytope strict-reject proposal has no exact "
                      "positive witness");
                }
                public_record.strict_reject_boundary_witness =
                    static_cast<std::size_t>(
                        device_record.strict_reject_boundary_witness);
                ++result.audit.strict_reject_proposal_record_count;
                break;
              }
              case HPolytopeProposalRecordStatus::proposed_survivor: {
                if (device_record.strict_reject_boundary_witness !=
                        detail::kHPolytopeProposalNoBoundaryWitness ||
                    intersection.kind() !=
                        exact::ThreePlaneIntersectionKind::unique ||
                    (device_record.could_be_active_boundary_mask &
                     ~valid_mask) != 0U) {
                  throw std::runtime_error(
                      "an H-polytope survivor proposal has malformed "
                      "metadata");
                }
                std::uint64_t exact_active_mask = 0U;
                for (std::size_t boundary_index = 0U;
                     boundary_index < boundary_count;
                     ++boundary_index) {
                  const int sign = cell.exact_result
                                       .boundary_planes[boundary_index]
                                       .retained_nonpositive_form
                                       .evaluate(*intersection.point())
                                       .sign();
                  if (sign > 0) {
                    throw std::runtime_error(
                        "an H-polytope survivor proposal is exactly "
                        "infeasible");
                  }
                  if (sign == 0) {
                    exact_active_mask |= UINT64_C(1) << boundary_index;
                  }
                }
                if ((exact_active_mask &
                     ~device_record.could_be_active_boundary_mask) != 0U ||
                    !exact_result_contains_vertex(
                        cell.exact_result, *intersection.point())) {
                  throw std::runtime_error(
                      "an H-polytope survivor omitted an exact incidence or "
                      "vertex");
                }
                HPolytopeProposalCoordinateIntervals3 intervals;
                for (std::size_t axis = 0U; axis < 3U; ++axis) {
                  const exact::ExactRational lower =
                      finite_canonical_binary64_rational(
                          device_record.coordinate_lower_bits[axis]);
                  const exact::ExactRational upper =
                      finite_canonical_binary64_rational(
                          device_record.coordinate_upper_bits[axis]);
                  if (lower > upper ||
                      intersection.point()->coordinate(axis) < lower ||
                      intersection.point()->coordinate(axis) > upper) {
                    throw std::runtime_error(
                        "an H-polytope survivor interval excludes its exact "
                        "intersection");
                  }
                  intervals.lower_binary64_bits[axis] =
                      device_record.coordinate_lower_bits[axis];
                  intervals.upper_binary64_bits[axis] =
                      device_record.coordinate_upper_bits[axis];
                }
                public_record.survivor_coordinate_intervals = intervals;
                public_record.could_be_active_boundary_mask =
                    device_record.could_be_active_boundary_mask;
                ++result.audit.survivor_proposal_record_count;
                break;
              }
            }
            hash_word(digest, device_record.first_boundary_index);
            hash_word(digest, device_record.second_boundary_index);
            hash_word(digest, device_record.third_boundary_index);
            hash_word(digest, device_record.status_code);
            hash_word(
                digest, device_record.strict_reject_boundary_witness);
            hash_word(
                digest, device_record.could_be_active_boundary_mask);
            for (std::size_t axis = 0U; axis < 3U; ++axis) {
              hash_word(digest, device_record.coordinate_lower_bits[axis]);
              hash_word(digest, device_record.coordinate_upper_bits[axis]);
            }
            result.proposal_records.push_back(std::move(public_record));
          }
        }
      }
      if (record_index != end) {
        throw std::runtime_error(
            "an H-polytope transcript continued beyond its final ordinal");
      }
    }
    result.proposal_offsets.push_back(result.proposal_records.size());
    result.cell_results.push_back(HPolytopeProposalCellResult{
        cell.input.cell_id, cell_status, std::move(cell.exact_result)});
  }
  if (result.proposal_records.size() != record_count ||
      result.proposal_offsets.back() != record_count ||
      result.audit.unknown_proposal_record_count +
              result.audit.strict_reject_proposal_record_count +
              result.audit.survivor_proposal_record_count !=
          record_count) {
    throw std::logic_error(
        "the validated H-polytope transcript counters did not close");
  }
  result.audit.exact_unique_vertex_recertification_count =
      exact_vertex_count;
  result.audit.proposal_digest_fnv1a = digest;
  result.audit.exhaustive_transcript_validated = true;
  result.audit.cpu_exact_recertification_complete = true;
  return result;
}

}  // namespace

std::string_view to_string(HPolytopeProposalBatchDecision decision) {
  switch (decision) {
    case HPolytopeProposalBatchDecision::exact_recertified_local:
      return "exact_recertified_local";
    case HPolytopeProposalBatchDecision::insufficient_exact_budget:
      return "insufficient_exact_budget";
  }
  throw std::invalid_argument(
      "an H-polytope proposal batch decision is invalid");
}

std::string_view to_string(HPolytopeProposalCellStatus status) {
  switch (status) {
    case HPolytopeProposalCellStatus::validated_exhaustive_transcript:
      return "validated_exhaustive_transcript";
    case HPolytopeProposalCellStatus::exact_fallback_interval_unknown:
      return "exact_fallback_interval_unknown";
    case HPolytopeProposalCellStatus::exact_fallback_capacity_exhausted:
      return "exact_fallback_capacity_exhausted";
    case HPolytopeProposalCellStatus::exact_fallback_unsupported_projection:
      return "exact_fallback_unsupported_projection";
  }
  throw std::invalid_argument(
      "an H-polytope proposal cell status is invalid");
}

std::string_view to_string(HPolytopeProposalRecordStatus status) {
  switch (status) {
    case HPolytopeProposalRecordStatus::unknown_requires_cpu_exact:
      return "unknown_requires_cpu_exact";
    case HPolytopeProposalRecordStatus::proposed_strict_reject:
      return "proposed_strict_reject";
    case HPolytopeProposalRecordStatus::proposed_survivor:
      return "proposed_survivor";
  }
  throw std::invalid_argument(
      "an H-polytope proposal record status is invalid");
}

HPolytopeProposalContext::HPolytopeProposalContext(
    spatial::ExactDyadicAabb3 clipping_box,
    std::size_t maximum_total_proposal_record_count)
    : clipping_box_(std::move(clipping_box)),
      maximum_total_proposal_record_count_(
          maximum_total_proposal_record_count) {
  static_cast<void>(checked_multiply(
      maximum_total_proposal_record_count_,
      sizeof(detail::HPolytopeProposalDeviceRecord),
      "the H-polytope proposal physical byte capacity overflowed"));
  const std::array<spatial::ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  static_cast<void>(spatial::build_exact_bounded_h_polytope_reference(
      no_halfspaces, clipping_box_, zero_budget()));
  state_ = std::make_shared<detail::HPolytopeProposalContextState>();
}

HPolytopeProposalContext::~HPolytopeProposalContext() noexcept = default;
HPolytopeProposalContext::HPolytopeProposalContext(
    HPolytopeProposalContext&&) noexcept = default;
HPolytopeProposalContext& HPolytopeProposalContext::operator=(
    HPolytopeProposalContext&&) noexcept = default;

HPolytopeProposalBatchResult HPolytopeProposalContext::build(
    std::span<const std::uint64_t> cell_ids,
    std::span<const std::size_t> halfspace_offsets,
    std::span<const spatial::ExactHPolytopeHalfspace3> halfspaces,
    spatial::ExactBoundedHPolytopeReferenceBudget exact_budget) {
  if (state_ == nullptr) {
    throw std::invalid_argument(
        "a moved-from H-polytope proposal context is not queryable");
  }
  validate_exact_budget(exact_budget);
  const std::vector<CanonicalCellSlice> canonical_cells =
      canonical_cell_slices(cell_ids, halfspace_offsets, halfspaces);

  HPolytopeProposalBatchResult preflight;
  preflight.audit.canonical_input_cell_count = canonical_cells.size();
  preflight.audit.physical_proposal_record_capacity =
      maximum_total_proposal_record_count_;
  preflight.audit.canonical_cell_order_validated = true;
  std::size_t required_record_count = 0U;
  bool exact_budget_is_sufficient = true;
  for (const CanonicalCellSlice& cell : canonical_cells) {
    const auto segment = cell_halfspaces(halfspaces, cell);
    const auto exact_preflight =
        spatial::build_exact_bounded_h_polytope_reference(
            segment, clipping_box_, zero_budget());
    if (exact_preflight.decision !=
        spatial::ExactBoundedHPolytopeReferenceDecision::
            insufficient_budget) {
      throw std::logic_error(
          "a zero H-polytope budget unexpectedly performed geometry");
    }
    const std::size_t boundary_count = proper_boundary_count(segment);
    const std::size_t record_count = plane_triple_count(boundary_count);
    required_record_count = checked_add(
        required_record_count,
        record_count,
        "the H-polytope proposal transcript count overflowed");
    preflight.requirements.push_back(HPolytopeProposalCellRequirements{
        cell.cell_id,
        exact_preflight.requirements,
        boundary_count,
        record_count});
    exact_budget_is_sufficient =
        exact_budget_is_sufficient &&
        exact_budget_covers(exact_budget, exact_preflight.requirements);
  }
  preflight.audit.required_exhaustive_proposal_record_count =
      required_record_count;
  if (!exact_budget_is_sufficient) {
    return preflight;
  }

  std::vector<BuiltCell> built_cells;
  built_cells.reserve(canonical_cells.size());
  std::vector<detail::HPolytopeProposalInputCell> packed_cells;
  packed_cells.reserve(canonical_cells.size());
  std::vector<detail::HPolytopeProposalInputBoundary> packed_boundaries;
  for (std::size_t index = 0U; index < canonical_cells.size(); ++index) {
    const CanonicalCellSlice& cell = canonical_cells[index];
    auto exact_result = spatial::build_exact_bounded_h_polytope_reference(
        cell_halfspaces(halfspaces, cell), clipping_box_, exact_budget);
    if (exact_result.decision ==
        spatial::ExactBoundedHPolytopeReferenceDecision::
            insufficient_budget) {
      throw std::logic_error(
          "a covered H-polytope proposal preflight became insufficient");
    }
    if (exact_result.boundary_planes.size() !=
        preflight.requirements[index].boundary_plane_count) {
      throw std::logic_error(
          "the H-polytope proposal boundary preflight did not close");
    }
    if (!std::in_range<std::uint64_t>(packed_boundaries.size())) {
      throw std::length_error(
          "the H-polytope proposal boundary offset exceeds uint64");
    }
    const std::uint64_t boundary_begin =
        static_cast<std::uint64_t>(packed_boundaries.size());
    bool unsupported_projection = false;
    for (const spatial::ExactHPolytopeBoundaryPlane& boundary :
         exact_result.boundary_planes) {
      detail::HPolytopeProposalInputBoundary packed;
      for (std::size_t coefficient = 0U; coefficient < 4U;
           ++coefficient) {
        const detail::DirectedEnclosure enclosure =
            detail::enclose_rational(
                boundary.retained_nonpositive_form.coefficient(coefficient));
        packed.coefficient_lower_bits[coefficient] = enclosure.lower_bits;
        packed.coefficient_upper_bits[coefficient] = enclosure.upper_bits;
        unsupported_projection =
            unsupported_projection ||
            enclosure.status == DirectedEnclosureStatus::unsupported_range;
      }
      packed_boundaries.push_back(packed);
    }
    if (!std::in_range<std::uint64_t>(packed_boundaries.size())) {
      throw std::length_error(
          "the H-polytope proposal boundary offset exceeds uint64");
    }
    const bool force_interval_fallback =
        exact_result.audit.infeasible_count != 0U;
    packed_cells.push_back(detail::HPolytopeProposalInputCell{
        cell.cell_id,
        boundary_begin,
        static_cast<std::uint64_t>(packed_boundaries.size()),
        unsupported_projection ? 1U : 0U,
        force_interval_fallback ? 1U : 0U});
    built_cells.push_back(BuiltCell{
        cell,
        preflight.requirements[index],
        std::move(exact_result),
        unsupported_projection,
        force_interval_fallback});
  }

  const bool any_projectable = std::any_of(
      built_cells.begin(),
      built_cells.end(),
      [](const BuiltCell& cell) {
        return !cell.unsupported_projection &&
               !cell.force_interval_fallback;
      });
  if (!any_projectable || maximum_total_proposal_record_count_ == 0U) {
    return no_launch_fallback_result(
        std::move(built_cells),
        maximum_total_proposal_record_count_,
        required_record_count);
  }

  return state_->with_gpu_section([&] {
    const std::uint64_t previous_epoch = state_->current_epoch();
    const detail::HPolytopeProposalDeviceBatch batch =
        detail::propose_h_polytope_transcript_on_gpu(
            *state_,
            packed_cells,
            packed_boundaries,
            maximum_total_proposal_record_count_);
    if (previous_epoch == std::numeric_limits<std::uint64_t>::max() ||
        state_->current_epoch() != previous_epoch + UINT64_C(1) ||
        batch.buffer_epoch != state_->current_epoch()) {
      throw std::runtime_error(
          "the H-polytope proposal did not advance exactly one authenticated "
          "epoch");
    }
    return validate_device_batch(
        batch,
        std::move(built_cells),
        maximum_total_proposal_record_count_,
        required_record_count);
  });
}

}  // namespace morsehgp3d::gpu
