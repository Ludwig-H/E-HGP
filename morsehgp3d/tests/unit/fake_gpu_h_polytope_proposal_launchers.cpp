#include "fake_gpu_h_polytope_proposal_launchers.hpp"

#include "phase7_h_polytope_proposal_internal.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<FakeHPolytopeProposalValues> proposal_values{
    FakeHPolytopeProposalValues::actual_binary64_recipe};
std::atomic<FakeHPolytopeProposalCorruption> proposal_corruption{
    FakeHPolytopeProposalCorruption::none};
std::atomic<std::size_t> proposal_launch_count{0U};
std::atomic<std::size_t> proposal_last_cell_count{0U};
std::atomic<std::size_t> proposal_last_boundary_count{0U};
std::atomic<std::size_t> proposal_last_record_capacity{0U};
std::atomic<std::size_t> proposal_last_incidence_word_capacity{0U};

}  // namespace

void configure_fake_gpu_h_polytope_proposal(
    FakeHPolytopeProposalConfiguration configuration) noexcept {
  proposal_values.store(configuration.values, std::memory_order_relaxed);
  proposal_corruption.store(
      configuration.corruption, std::memory_order_relaxed);
}

void reset_fake_gpu_h_polytope_proposal() noexcept {
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{});
  proposal_launch_count.store(0U, std::memory_order_relaxed);
  proposal_last_cell_count.store(0U, std::memory_order_relaxed);
  proposal_last_boundary_count.store(0U, std::memory_order_relaxed);
  proposal_last_record_capacity.store(0U, std::memory_order_relaxed);
  proposal_last_incidence_word_capacity.store(0U, std::memory_order_relaxed);
}

std::size_t fake_gpu_h_polytope_proposal_launch_count() noexcept {
  return proposal_launch_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_h_polytope_proposal_last_cell_count() noexcept {
  return proposal_last_cell_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_h_polytope_proposal_last_boundary_count() noexcept {
  return proposal_last_boundary_count.load(std::memory_order_relaxed);
}

std::size_t fake_gpu_h_polytope_proposal_last_record_capacity() noexcept {
  return proposal_last_record_capacity.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_h_polytope_proposal_last_incidence_word_capacity() noexcept {
  // B <= 61 gives one mask word per transcript record in the 7.7 ABI.
  return proposal_last_incidence_word_capacity.load(
      std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

constexpr double kDeterminantTolerance = 1.0e-12;
constexpr double kStrictSignTolerance = 1.0e-10;

struct Vector3 {
  double x{};
  double y{};
  double z{};
};

struct Plane4 {
  Vector3 normal;
  double offset{};
  bool finite{true};
};

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value, const char* message) {
  if (!std::in_range<std::size_t>(value)) {
    throw std::invalid_argument(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::size_t triplet_count(std::size_t boundary_count) {
  if (boundary_count < 3U) {
    return 0U;
  }
  return boundary_count * (boundary_count - 1U) *
         (boundary_count - 2U) / 6U;
}

[[nodiscard]] double midpoint(std::uint64_t lower_bits,
                              std::uint64_t upper_bits,
                              bool& finite) {
  const double lower = std::bit_cast<double>(lower_bits);
  const double upper = std::bit_cast<double>(upper_bits);
  if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
    finite = false;
    return 0.0;
  }
  const double value = lower * 0.5 + upper * 0.5;
  if (!std::isfinite(value)) {
    finite = false;
    return 0.0;
  }
  return value;
}

[[nodiscard]] Plane4 plane(
    const HPolytopeProposalInputBoundary& boundary) {
  Plane4 result;
  result.normal.x = midpoint(
      boundary.coefficient_lower_bits[0U],
      boundary.coefficient_upper_bits[0U], result.finite);
  result.normal.y = midpoint(
      boundary.coefficient_lower_bits[1U],
      boundary.coefficient_upper_bits[1U], result.finite);
  result.normal.z = midpoint(
      boundary.coefficient_lower_bits[2U],
      boundary.coefficient_upper_bits[2U], result.finite);
  result.offset = midpoint(
      boundary.coefficient_lower_bits[3U],
      boundary.coefficient_upper_bits[3U], result.finite);
  return result;
}

[[nodiscard]] Vector3 cross(const Vector3& left, const Vector3& right) {
  return Vector3{
      left.y * right.z - left.z * right.y,
      left.z * right.x - left.x * right.z,
      left.x * right.y - left.y * right.x};
}

[[nodiscard]] double dot(const Vector3& left, const Vector3& right) {
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

[[nodiscard]] Vector3 scaled_add(
    double first_scale,
    const Vector3& first,
    double second_scale,
    const Vector3& second,
    double third_scale,
    const Vector3& third) {
  return Vector3{
      first_scale * first.x + second_scale * second.x +
          third_scale * third.x,
      first_scale * first.y + second_scale * second.y +
          third_scale * third.y,
      first_scale * first.z + second_scale * second.z +
          third_scale * third.z};
}

[[nodiscard]] bool unique_intersection(
    const Plane4& first,
    const Plane4& second,
    const Plane4& third,
    Vector3& point) {
  if (!first.finite || !second.finite || !third.finite) {
    return false;
  }
  const Vector3 second_cross_third =
      cross(second.normal, third.normal);
  const double determinant = dot(first.normal, second_cross_third);
  if (!std::isfinite(determinant) ||
      std::abs(determinant) <= kDeterminantTolerance) {
    return false;
  }
  const Vector3 numerator = scaled_add(
      -first.offset,
      second_cross_third,
      -second.offset,
      cross(third.normal, first.normal),
      -third.offset,
      cross(first.normal, second.normal));
  point = Vector3{
      numerator.x / determinant,
      numerator.y / determinant,
      numerator.z / determinant};
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z);
}

[[nodiscard]] double evaluate(const Plane4& plane, const Vector3& point) {
  return dot(plane.normal, point) + plane.offset;
}

void set_triplet_identity(
    HPolytopeProposalDeviceRecord& record,
    std::uint64_t cell_id,
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::uint64_t epoch) {
  record.initialized_slot_sentinel =
      kHPolytopeProposalInitializedSlotSentinel;
  record.cell_id = cell_id;
  record.first_boundary_index = static_cast<std::uint64_t>(first);
  record.second_boundary_index = static_cast<std::uint64_t>(second);
  record.third_boundary_index = static_cast<std::uint64_t>(third);
  record.buffer_epoch = epoch;
}

[[nodiscard]] std::uint64_t full_boundary_mask(
    std::size_t boundary_count) {
  if (boundary_count == 0U || boundary_count > 63U) {
    throw std::logic_error(
        "the fake Phase 7 mask needs between one and 63 boundaries");
  }
  return (UINT64_C(1) << boundary_count) - UINT64_C(1);
}

void make_unknown(HPolytopeProposalDeviceRecord& record) {
  record.status_code = static_cast<std::uint64_t>(
      HPolytopeProposalDeviceRecordStatus::unknown_requires_cpu_exact);
  record.strict_reject_boundary_witness =
      kHPolytopeProposalNoBoundaryWitness;
}

void make_survivor(
    HPolytopeProposalDeviceRecord& record,
    std::size_t boundary_count) {
  record.status_code = static_cast<std::uint64_t>(
      HPolytopeProposalDeviceRecordStatus::proposed_survivor);
  record.strict_reject_boundary_witness =
      kHPolytopeProposalNoBoundaryWitness;
  const double lower = -std::numeric_limits<double>::max();
  const double upper = std::numeric_limits<double>::max();
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    record.coordinate_lower_bits[axis] =
        std::bit_cast<std::uint64_t>(lower);
    record.coordinate_upper_bits[axis] =
        std::bit_cast<std::uint64_t>(upper);
  }
  record.could_be_active_boundary_mask =
      full_boundary_mask(boundary_count);
}

void make_reject(
    HPolytopeProposalDeviceRecord& record,
    std::size_t witness_boundary) {
  record.status_code = static_cast<std::uint64_t>(
      HPolytopeProposalDeviceRecordStatus::proposed_strict_reject);
  record.strict_reject_boundary_witness =
      static_cast<std::uint64_t>(witness_boundary);
}

[[nodiscard]] HPolytopeProposalDeviceRecord recipe_record(
    std::uint64_t cell_id,
    std::span<const HPolytopeProposalInputBoundary> boundaries,
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::uint64_t epoch,
    test_support::FakeHPolytopeProposalValues values) {
  HPolytopeProposalDeviceRecord result;
  set_triplet_identity(result, cell_id, first, second, third, epoch);
  if (values ==
      test_support::FakeHPolytopeProposalValues::all_unknown) {
    make_unknown(result);
    return result;
  }

  const Plane4 first_plane = plane(boundaries[first]);
  const Plane4 second_plane = plane(boundaries[second]);
  const Plane4 third_plane = plane(boundaries[third]);
  Vector3 point;
  if (!unique_intersection(
          first_plane, second_plane, third_plane, point)) {
    make_unknown(result);
    return result;
  }

  for (std::size_t boundary_index = 0U;
       boundary_index < boundaries.size(); ++boundary_index) {
    const Plane4 candidate = plane(boundaries[boundary_index]);
    const double value = candidate.finite
                             ? evaluate(candidate, point)
                             : std::numeric_limits<double>::quiet_NaN();
    if (!std::isfinite(value)) {
      make_unknown(result);
      return result;
    }
    if (value > kStrictSignTolerance) {
      make_reject(result, boundary_index);
      return result;
    }
  }
  make_survivor(result, boundaries.size());
  return result;
}

[[nodiscard]] std::size_t first_survivor(
    const HPolytopeProposalDeviceBatch& batch) {
  const std::size_t record_count = checked_size(
      batch.record_count,
      "the fake Phase 7 record count does not fit size_t");
  for (std::size_t index = 0U; index < record_count; ++index) {
    if (batch.records[index].status_code ==
        static_cast<std::uint64_t>(
            HPolytopeProposalDeviceRecordStatus::proposed_survivor)) {
      return index;
    }
  }
  throw std::logic_error(
      "the requested fake Phase 7 corruption needs a survivor record");
}

[[nodiscard]] std::size_t boundary_count_for_cell(
    std::span<const HPolytopeProposalInputCell> cells,
    std::uint64_t cell_id) {
  const auto iterator = std::find_if(
      cells.begin(), cells.end(), [cell_id](const auto& cell) {
        return cell.cell_id == cell_id;
      });
  if (iterator == cells.end()) {
    throw std::logic_error(
        "the fake Phase 7 output names an unknown input cell");
  }
  const std::size_t begin = checked_size(
      iterator->boundary_begin,
      "the fake Phase 7 boundary begin does not fit size_t");
  const std::size_t end = checked_size(
      iterator->boundary_end,
      "the fake Phase 7 boundary end does not fit size_t");
  if (begin > end) {
    throw std::logic_error("the fake Phase 7 cell has reversed boundaries");
  }
  return end - begin;
}

void reset_floating_payload(HPolytopeProposalDeviceRecord& record) {
  record.strict_reject_boundary_witness =
      kHPolytopeProposalNoBoundaryWitness;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    record.coordinate_lower_bits[axis] = 0U;
    record.coordinate_upper_bits[axis] = 0U;
  }
  record.could_be_active_boundary_mask = 0U;
}

void inject_corruption(
    HPolytopeProposalDeviceBatch& batch,
    std::span<const HPolytopeProposalInputCell> cells,
    test_support::FakeHPolytopeProposalCorruption corruption) {
  if (corruption == test_support::FakeHPolytopeProposalCorruption::none) {
    return;
  }
  if (batch.record_count == 0U &&
      corruption !=
          test_support::FakeHPolytopeProposalCorruption::invalid_offsets &&
      corruption !=
          test_support::FakeHPolytopeProposalCorruption::invalid_cell_id &&
      corruption !=
          test_support::FakeHPolytopeProposalCorruption::invalid_cell_status) {
    throw std::logic_error(
        "the requested fake Phase 7 corruption needs transcript records");
  }

  switch (corruption) {
    case test_support::FakeHPolytopeProposalCorruption::none:
      return;
    case test_support::FakeHPolytopeProposalCorruption::missing_slot:
      batch.records.front() = HPolytopeProposalDeviceRecord{};
      return;
    case test_support::FakeHPolytopeProposalCorruption::duplicate_slot:
      if (batch.record_count < 2U) {
        throw std::logic_error(
            "duplicate-slot corruption needs two transcript records");
      }
      batch.records[1U] = batch.records[0U];
      return;
    case test_support::FakeHPolytopeProposalCorruption::omitted_true_incidence: {
      HPolytopeProposalDeviceRecord& record =
          batch.records[first_survivor(batch)];
      record.could_be_active_boundary_mask &=
          ~(UINT64_C(1) << record.first_boundary_index);
      return;
    }
    case test_support::FakeHPolytopeProposalCorruption::
        out_of_range_incidence_bit: {
      HPolytopeProposalDeviceRecord& record =
          batch.records[first_survivor(batch)];
      const std::size_t boundary_count =
          boundary_count_for_cell(cells, record.cell_id);
      record.could_be_active_boundary_mask |=
          UINT64_C(1) << boundary_count;
      return;
    }
    case test_support::FakeHPolytopeProposalCorruption::
        nonfinite_survivor_coordinate: {
      HPolytopeProposalDeviceRecord& record =
          batch.records[first_survivor(batch)];
      record.coordinate_lower_bits[0U] = std::bit_cast<std::uint64_t>(
          std::numeric_limits<double>::infinity());
      return;
    }
    case test_support::FakeHPolytopeProposalCorruption::false_strict_reject: {
      HPolytopeProposalDeviceRecord& record =
          batch.records[first_survivor(batch)];
      const std::uint64_t witness = record.first_boundary_index;
      reset_floating_payload(record);
      record.status_code = static_cast<std::uint64_t>(
          HPolytopeProposalDeviceRecordStatus::proposed_strict_reject);
      record.strict_reject_boundary_witness = witness;
      return;
    }
    case test_support::FakeHPolytopeProposalCorruption::wrong_epoch:
      ++batch.records.front().buffer_epoch;
      return;
    case test_support::FakeHPolytopeProposalCorruption::wrong_batch_epoch: {
      ++batch.buffer_epoch;
      const std::size_t record_count = checked_size(
          batch.record_count,
          "the fake Phase 7 record count does not fit size_t");
      for (std::size_t index = 0U; index < record_count; ++index) {
        ++batch.records[index].buffer_epoch;
      }
      return;
    }
    case test_support::FakeHPolytopeProposalCorruption::
        stale_epoch_without_advance:
    case test_support::FakeHPolytopeProposalCorruption::double_epoch_advance:
      return;
    case test_support::FakeHPolytopeProposalCorruption::wrong_ordinal:
      batch.records.front().second_boundary_index =
          batch.records.front().first_boundary_index;
      return;
    case test_support::FakeHPolytopeProposalCorruption::invalid_offsets:
      if (batch.cell_record_offsets.size() < 2U) {
        throw std::logic_error(
            "invalid-offset corruption needs a nonempty CSR");
      }
      ++batch.cell_record_offsets[1U];
      return;
    case test_support::FakeHPolytopeProposalCorruption::invalid_cell_id:
      ++batch.cell_ids.front();
      return;
    case test_support::FakeHPolytopeProposalCorruption::invalid_cell_status:
      batch.cell_statuses.front() =
          static_cast<HPolytopeProposalDeviceCellStatus>(999U);
      return;
    case test_support::FakeHPolytopeProposalCorruption::invalid_record_status:
      batch.records.front().status_code = 999U;
      return;
    case test_support::FakeHPolytopeProposalCorruption::tail_write:
      if (!std::in_range<std::size_t>(batch.record_count) ||
          static_cast<std::size_t>(batch.record_count) >=
              batch.records.size()) {
        throw std::logic_error(
            "tail-write corruption needs spare physical capacity");
      }
      batch.records[static_cast<std::size_t>(batch.record_count)]
          .initialized_slot_sentinel =
          kHPolytopeProposalInitializedSlotSentinel;
      return;
    case test_support::FakeHPolytopeProposalCorruption::
        simulated_async_failure:
      throw std::runtime_error(
          "simulated asynchronous Phase 7 H-polytope failure");
  }
  throw std::logic_error("unknown fake Phase 7 corruption mode");
}

}  // namespace

HPolytopeProposalDeviceBatch propose_h_polytope_transcript_on_gpu(
    HPolytopeProposalContextState& context,
    std::span<const HPolytopeProposalInputCell> cells,
    std::span<const HPolytopeProposalInputBoundary> boundaries,
    std::size_t maximum_total_proposal_record_count) {
  if (cells.empty()) {
    throw std::invalid_argument(
        "the fake Phase 7 launcher requires at least one cell");
  }

  const test_support::FakeHPolytopeProposalValues values =
      test_support::proposal_values.load(std::memory_order_relaxed);
  const test_support::FakeHPolytopeProposalCorruption corruption =
      test_support::proposal_corruption.load(std::memory_order_relaxed);

  test_support::proposal_launch_count.fetch_add(1U, std::memory_order_relaxed);
  test_support::proposal_last_cell_count.store(
      cells.size(), std::memory_order_relaxed);
  test_support::proposal_last_boundary_count.store(
      boundaries.size(), std::memory_order_relaxed);
  test_support::proposal_last_record_capacity.store(
      maximum_total_proposal_record_count, std::memory_order_relaxed);
  test_support::proposal_last_incidence_word_capacity.store(
      maximum_total_proposal_record_count, std::memory_order_relaxed);

  HPolytopeProposalDeviceBatch batch;
  if (corruption == test_support::FakeHPolytopeProposalCorruption::
                        stale_epoch_without_advance) {
    batch.buffer_epoch = context.current_epoch();
  } else if (corruption ==
             test_support::FakeHPolytopeProposalCorruption::
                 double_epoch_advance) {
    static_cast<void>(context.advance_epoch());
    batch.buffer_epoch = context.advance_epoch();
  } else {
    batch.buffer_epoch = context.advance_epoch();
  }
  batch.records.resize(maximum_total_proposal_record_count);
  batch.cell_ids.reserve(cells.size());
  batch.cell_statuses.reserve(cells.size());
  batch.cell_record_offsets.reserve(cells.size() + 1U);
  batch.cell_record_offsets.push_back(0U);
  std::size_t record_count = 0U;

  for (const HPolytopeProposalInputCell& cell : cells) {
    const std::size_t boundary_begin = checked_size(
        cell.boundary_begin,
        "the fake Phase 7 boundary begin does not fit size_t");
    const std::size_t boundary_end = checked_size(
        cell.boundary_end,
        "the fake Phase 7 boundary end does not fit size_t");
    if (boundary_begin > boundary_end || boundary_end > boundaries.size()) {
      throw std::invalid_argument(
          "the fake Phase 7 launcher received invalid boundary offsets");
    }
    const std::size_t boundary_count = boundary_end - boundary_begin;
    if (boundary_count < 6U || boundary_count > 61U) {
      throw std::invalid_argument(
          "the fake Phase 7 launcher received an unsupported boundary count");
    }
    const std::size_t required_records = triplet_count(boundary_count);
    batch.cell_ids.push_back(cell.cell_id);

    const bool projection_fallback = cell.unsupported_projection != 0U;
    const bool interval_fallback =
        cell.force_interval_fallback != 0U ||
        values == test_support::FakeHPolytopeProposalValues::
                      whole_cell_interval_fallback;
    const bool capacity_fallback =
        required_records > maximum_total_proposal_record_count -
                               record_count;
    if (projection_fallback || interval_fallback || capacity_fallback) {
      batch.cell_statuses.push_back(
          projection_fallback
              ? HPolytopeProposalDeviceCellStatus::
                    exact_fallback_unsupported_projection
              : interval_fallback
                    ? HPolytopeProposalDeviceCellStatus::
                          exact_fallback_interval_unknown
                    : HPolytopeProposalDeviceCellStatus::
                          exact_fallback_capacity_exhausted);
      batch.cell_record_offsets.push_back(
          static_cast<std::uint64_t>(record_count));
      continue;
    }

    batch.cell_statuses.push_back(
        HPolytopeProposalDeviceCellStatus::validated_exhaustive_transcript);
    const std::span<const HPolytopeProposalInputBoundary>
        cell_boundaries = boundaries.subspan(boundary_begin, boundary_count);
    for (std::size_t first = 0U; first + 2U < boundary_count; ++first) {
      for (std::size_t second = first + 1U;
           second + 1U < boundary_count; ++second) {
        for (std::size_t third = second + 1U; third < boundary_count;
             ++third) {
          batch.records[record_count] = recipe_record(
              cell.cell_id,
              cell_boundaries,
              first,
              second,
              third,
              batch.buffer_epoch,
              values);
          ++record_count;
        }
      }
    }
    batch.cell_record_offsets.push_back(
        static_cast<std::uint64_t>(record_count));
  }

  batch.record_count = static_cast<std::uint64_t>(record_count);
  inject_corruption(batch, cells, corruption);
  return batch;
}

}  // namespace morsehgp3d::gpu::detail
