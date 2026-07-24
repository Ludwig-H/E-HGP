#include "morsehgp3d/gpu/direct_sparse_facet_top_k_proposal.hpp"

#include "../cuda/phase14_facet_top_k_proposal_internal.hpp"
#include "exact_center_binary64_projection.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

class Phase14FacetTopKProposalHostState final {
 public:
  std::shared_ptr<const void> cloud_identity;
  std::size_t point_count{};
  std::vector<std::uint64_t> coordinate_bits;
  std::vector<std::uint64_t> morton_point_ids;
  std::vector<std::size_t> morton_position_by_point_id;
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

using DeviceBatch = detail::Phase14FacetTopKProposalDeviceBatch;
using DeviceRecord = detail::Phase14FacetTopKProposalDeviceRecord;
using InputRecord = detail::Phase14FacetTopKProposalQueryInputRecord;
using TranscriptRecord =
    hierarchy::ExactDirectSparseFacetTopKProposalRecord;

constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
constexpr std::uint64_t kFullFingerprintMask =
    std::numeric_limits<std::uint64_t>::max();

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (!std::in_range<std::size_t>(value)) {
    throw std::runtime_error(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] spatial::PointId checked_point_id(
    std::size_t value,
    const char* message) {
  if (!std::in_range<spatial::PointId>(value) ||
      static_cast<spatial::PointId>(value) >
          spatial::CanonicalPointCloud::max_point_id) {
    throw std::length_error(message);
  }
  return static_cast<spatial::PointId>(value);
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] bool key_less(
    const hierarchy::ExactDirectSparseFacetKey& left,
    const hierarchy::ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.begin() +
          static_cast<std::ptrdiff_t>(left.point_count),
      right.point_ids.begin(),
      right.point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.point_count));
}

void validate_key(
    const hierarchy::ExactDirectSparseFacetKey& key,
    std::size_t point_count) {
  if (key.point_count == 0U ||
      key.point_count >
          hierarchy::direct_sparse_positive_facet_maximum_point_count) {
    throw std::invalid_argument(
        "a Phase 14 GPU proposal key must contain between one and ten points");
  }
  for (std::size_t point_index = 0U;
       point_index < key.point_count;
       ++point_index) {
    if (key.point_ids[point_index] >= point_count ||
        (point_index != 0U &&
         key.point_ids[point_index - 1U] >=
             key.point_ids[point_index])) {
      throw std::invalid_argument(
          "a Phase 14 GPU proposal key is not a canonical in-domain facet");
    }
  }
  if (!std::all_of(
          key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(key.point_count),
          key.point_ids.end(),
          [](spatial::PointId point_id) { return point_id == 0U; })) {
    throw std::invalid_argument(
        "a Phase 14 GPU proposal key has a nonzero unused tail");
  }
}

[[nodiscard]] std::size_t source_window_inspection_bound(
    std::size_t morton_position,
    std::size_t point_count,
    std::size_t window_radius) {
  const std::size_t left =
      std::min(window_radius, morton_position);
  const std::size_t right = std::min(
      window_radius, point_count - 1U - morton_position);
  return checked_size_sum(
      left,
      right,
      "the Phase 14 per-source Morton inspection bound overflowed");
}

[[nodiscard]] bool candidate_inside_any_window(
    spatial::PointId candidate,
    const hierarchy::ExactDirectSparseFacetKey& key,
    std::span<const std::size_t> morton_position_by_point_id,
    std::size_t window_radius) {
  const std::size_t candidate_position =
      morton_position_by_point_id[static_cast<std::size_t>(candidate)];
  for (std::size_t source_index = 0U;
       source_index < key.point_count;
       ++source_index) {
    const std::size_t source_position =
        morton_position_by_point_id[
            static_cast<std::size_t>(key.point_ids[source_index])];
    const std::size_t separation =
        candidate_position > source_position
            ? candidate_position - source_position
            : source_position - candidate_position;
    if (separation != 0U && separation <= window_radius) {
      return true;
    }
  }
  return false;
}

struct PackedQueries {
  std::vector<InputRecord> gpu_queries;
  std::vector<unsigned char> center_supported;
  std::vector<std::size_t> inspection_bounds;
  std::size_t facet_cardinality{};
  std::size_t exact_center_projection_axis_count{};
  std::size_t exact_center_projection_integer_division_count{};
  std::size_t unsupported_center_count{};
  std::size_t aggregate_inspection_bound{};
  std::size_t maximum_inspection_bound{};
};

[[nodiscard]] PackedQueries validate_and_pack_queries(
    const detail::Phase14FacetTopKProposalHostState& host,
    std::span<const DirectSparseFacetTopKProposalQuery> queries,
    std::size_t maximum_query_count,
    DirectSparseFacetTopKProposalPolicy policy) {
  if (queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "a Phase 14 GPU proposal batch exceeds its fixed query capacity");
  }
  if (policy.morton_window_radius == 0U ||
      policy.morton_window_radius >
          std::numeric_limits<std::size_t>::max() /
              (2U * hierarchy::
                        direct_sparse_positive_facet_maximum_point_count)) {
    throw std::invalid_argument(
        "a Phase 14 GPU proposal Morton window has an invalid work bound");
  }

  PackedQueries packed;
  packed.gpu_queries.reserve(queries.size());
  packed.center_supported.assign(queries.size(), 0U);
  packed.inspection_bounds.assign(queries.size(), 0U);

  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const DirectSparseFacetTopKProposalQuery& query =
        queries[query_index];
    validate_key(query.source_facet_key, host.point_count);
    if (query_index == 0U) {
      packed.facet_cardinality =
          query.source_facet_key.point_count;
    } else if (
        query.source_facet_key.point_count !=
            packed.facet_cardinality ||
        !key_less(
            queries[query_index - 1U].source_facet_key,
            query.source_facet_key)) {
      throw std::invalid_argument(
          "Phase 14 GPU proposal queries must have one cardinality and strictly increasing full keys");
    }

    const detail::ExactCenterBinary64Projection projection =
        detail::project_exact_center_to_nearest_binary64(
            query.query_center);
    packed.exact_center_projection_axis_count = checked_size_sum(
        packed.exact_center_projection_axis_count,
        projection.coordinate_bits.size(),
        "the Phase 14 center projection axis count overflowed");
    packed.exact_center_projection_integer_division_count =
        checked_size_sum(
            packed.exact_center_projection_integer_division_count,
            projection.integer_division_count,
            "the Phase 14 center projection division count overflowed");
    const bool center_supported = projection.supported;
    if (!center_supported) {
      ++packed.unsupported_center_count;
      continue;
    }

    InputRecord input;
    input.query_index = static_cast<std::uint64_t>(query_index);
    input.key_fingerprint =
        hierarchy::fingerprint_exact_direct_sparse_facet_key(
            query.source_facet_key, kFullFingerprintMask);
    input.point_count = static_cast<std::uint64_t>(
        query.source_facet_key.point_count);
    std::copy(
        projection.coordinate_bits.begin(),
        projection.coordinate_bits.end(),
        input.center_bits);
    std::fill(
        std::begin(input.point_ids),
        std::end(input.point_ids),
        detail::phase14_facet_top_k_proposal_sentinel);
    std::fill(
        std::begin(input.morton_positions),
        std::end(input.morton_positions),
        detail::phase14_facet_top_k_proposal_sentinel);

    std::size_t query_inspection_bound = 0U;
    for (std::size_t point_index = 0U;
         point_index < query.source_facet_key.point_count;
         ++point_index) {
      const spatial::PointId point_id =
          query.source_facet_key.point_ids[point_index];
      const std::size_t point_offset =
          static_cast<std::size_t>(point_id);
      const std::size_t morton_position =
          host.morton_position_by_point_id[point_offset];
      input.point_ids[point_index] = point_id;
      input.morton_positions[point_index] =
          static_cast<std::uint64_t>(morton_position);
      query_inspection_bound = checked_size_sum(
          query_inspection_bound,
          source_window_inspection_bound(
              morton_position,
              host.point_count,
              policy.morton_window_radius),
          "the Phase 14 per-query Morton inspection bound overflowed");
    }
    packed.center_supported[query_index] = 1U;
    packed.inspection_bounds[query_index] =
        query_inspection_bound;
    packed.aggregate_inspection_bound = checked_size_sum(
        packed.aggregate_inspection_bound,
        query_inspection_bound,
        "the Phase 14 batch Morton inspection bound overflowed");
    packed.maximum_inspection_bound = std::max(
        packed.maximum_inspection_bound, query_inspection_bound);
    packed.gpu_queries.push_back(input);
  }
  return packed;
}

struct ValidatedPayload {
  std::vector<TranscriptRecord> proposal_records;
  DirectSparseFacetTopKProposalAudit audit;
};

[[nodiscard]] ValidatedPayload validate_gpu_batch(
    const DeviceBatch& batch,
    const detail::Phase14FacetTopKProposalHostState& host,
    std::span<const DirectSparseFacetTopKProposalQuery> queries,
    const PackedQueries& packed,
    DirectSparseFacetTopKProposalPolicy policy,
    std::uint64_t previous_buffer_epoch) {
  const std::size_t expected_query_bytes = checked_size_product(
      packed.gpu_queries.size(),
      sizeof(InputRecord),
      "the Phase 14 expected active query byte count overflowed");
  const std::size_t expected_output_bytes = checked_size_product(
      packed.gpu_queries.size(),
      sizeof(DeviceRecord),
      "the Phase 14 expected active output byte count overflowed");
  if (previous_buffer_epoch ==
          std::numeric_limits<std::uint64_t>::max() ||
      batch.records.size() != packed.gpu_queries.size() ||
      batch.record_count != packed.gpu_queries.size() ||
      batch.host_to_device_query_byte_count != expected_query_bytes ||
      batch.initialized_output_byte_count != expected_output_bytes ||
      batch.device_to_host_record_byte_count != expected_output_bytes ||
      batch.kernel_launch_count != 1U ||
      batch.synchronization_count != 1U ||
      batch.buffer_epoch != previous_buffer_epoch + UINT64_C(1)) {
    throw std::runtime_error(
        "the Phase 14 GPU proposal batch returned invalid extent or epoch metadata");
  }
  ValidatedPayload payload;
  DirectSparseFacetTopKProposalAudit& audit = payload.audit;
  audit.gpu_output_record_count = batch.record_count;
  audit.active_host_to_device_query_record_count =
      batch.record_count;
  audit.active_host_to_device_query_byte_count =
      batch.host_to_device_query_byte_count;
  audit.initialized_device_output_record_count =
      batch.record_count;
  audit.initialized_device_output_byte_count =
      batch.initialized_output_byte_count;
  audit.copied_device_to_host_record_count =
      batch.record_count;
  audit.copied_device_to_host_byte_count =
      batch.device_to_host_record_byte_count;
  audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  audit.gpu_synchronization_count = batch.synchronization_count;
  audit.buffer_epoch = batch.buffer_epoch;
  audit.gpu_execution_performed = true;

  std::vector<unsigned char> seen(queries.size(), 0U);
  std::vector<const DeviceRecord*> records_by_query(
      queries.size(), nullptr);
  for (std::size_t record_index = 0U;
       record_index < batch.record_count;
       ++record_index) {
    const DeviceRecord& record = batch.records[record_index];
    const std::size_t query_index = checked_size(
        record.query_index,
        "a Phase 14 GPU proposal query index does not fit size_t");
    if (query_index >= queries.size() ||
        packed.center_supported[query_index] == 0U ||
        seen[query_index] != 0U ||
        record.buffer_epoch != batch.buffer_epoch ||
        record.failure_code !=
            static_cast<std::uint64_t>(
                detail::Phase14FacetTopKProposalFailureCode::none)) {
      throw std::runtime_error(
          "the Phase 14 GPU proposal transcript is not the supported-query permutation");
    }
    const auto& key = queries[query_index].source_facet_key;
    const std::uint64_t expected_fingerprint =
        hierarchy::fingerprint_exact_direct_sparse_facet_key(
            key, kFullFingerprintMask);
    if (record.key_fingerprint != expected_fingerprint) {
      throw std::runtime_error(
          "the Phase 14 GPU proposal changed a full facet key fingerprint");
    }

    const std::size_t candidate_count = checked_size(
        record.candidate_count,
        "a Phase 14 GPU proposal candidate count does not fit size_t");
    const std::size_t inspected_count = checked_size(
        record.inspected_neighbor_count,
        "a Phase 14 GPU proposal inspection count does not fit size_t");
    const std::size_t distance_count = checked_size(
        record.floating_distance_evaluation_count,
        "a Phase 14 GPU proposal distance count does not fit size_t");
    const std::size_t rejection_count = checked_size(
        record.floating_rejection_count,
        "a Phase 14 GPU proposal rejection count does not fit size_t");
    if (candidate_count > key.point_count ||
        inspected_count != packed.inspection_bounds[query_index] ||
        candidate_count > distance_count ||
        rejection_count != distance_count - candidate_count ||
        distance_count > inspected_count ||
        rejection_count > distance_count) {
      throw std::runtime_error(
          "the Phase 14 GPU proposal violates its fixed-k or Morton work bound");
    }

    spatial::PointId previous_candidate{};
    for (std::size_t candidate_index = 0U;
         candidate_index < candidate_count;
         ++candidate_index) {
      const std::uint64_t raw_candidate =
          record.candidates[candidate_index];
      if (raw_candidate >=
          static_cast<std::uint64_t>(host.point_count)) {
        throw std::runtime_error(
            "a Phase 14 GPU proposal candidate is outside the cloud");
      }
      const spatial::PointId candidate =
          static_cast<spatial::PointId>(raw_candidate);
      if ((candidate_index != 0U &&
           previous_candidate >= candidate) ||
          std::binary_search(
              key.point_ids.begin(),
              key.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(key.point_count),
              candidate) ||
          !candidate_inside_any_window(
              candidate,
              key,
              host.morton_position_by_point_id,
              policy.morton_window_radius)) {
        throw std::runtime_error(
            "a Phase 14 GPU proposal candidate is duplicate, excluded, or outside every trusted window");
      }
      previous_candidate = candidate;
    }
    for (std::size_t candidate_index = candidate_count;
         candidate_index <
             detail::
                 phase14_facet_top_k_proposal_maximum_point_count;
         ++candidate_index) {
      if (record.candidates[candidate_index] !=
          detail::phase14_facet_top_k_proposal_sentinel) {
        throw std::runtime_error(
            "a Phase 14 GPU proposal record has a stale candidate tail");
      }
    }

    seen[query_index] = 1U;
    records_by_query[query_index] = &record;
    audit.inspected_neighbor_count = checked_size_sum(
        audit.inspected_neighbor_count,
        inspected_count,
        "the Phase 14 GPU inspected-neighbor total overflowed");
    audit.floating_distance_evaluation_count = checked_size_sum(
        audit.floating_distance_evaluation_count,
        distance_count,
        "the Phase 14 GPU floating-distance total overflowed");
    audit.floating_distance_rejection_count = checked_size_sum(
        audit.floating_distance_rejection_count,
        rejection_count,
        "the Phase 14 GPU floating-rejection total overflowed");
    audit.proposed_candidate_count = checked_size_sum(
        audit.proposed_candidate_count,
        candidate_count,
        "the Phase 14 GPU candidate total overflowed");
  }

  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    if ((seen[query_index] != 0U) !=
        (packed.center_supported[query_index] != 0U)) {
      throw std::runtime_error(
          "the Phase 14 GPU proposal lost or fabricated a supported query");
    }
    const DeviceRecord* record = records_by_query[query_index];
    if (record == nullptr || record->candidate_count == 0U) {
      continue;
    }
    TranscriptRecord transcript_record;
    transcript_record.source_facet_key =
        queries[query_index].source_facet_key;
    transcript_record.candidate_point_count =
        static_cast<std::size_t>(record->candidate_count);
    for (std::size_t candidate_index = 0U;
         candidate_index <
             transcript_record.candidate_point_count;
         ++candidate_index) {
      transcript_record.candidate_point_ids[candidate_index] =
          static_cast<spatial::PointId>(
              record->candidates[candidate_index]);
    }
    payload.proposal_records.push_back(
        std::move(transcript_record));
  }
  audit.nonempty_proposal_record_count =
      payload.proposal_records.size();
  audit.supported_query_permutation_validated = true;
  audit.active_record_candidate_tail_sentinel_validated = true;
  audit.every_candidate_domain_validated = true;
  audit.every_candidate_source_facet_exclusion_validated = true;
  audit.every_candidate_morton_window_validated = true;
  audit.candidate_distinctness_validated = true;
  audit.bounded_work_validated =
      audit.inspected_neighbor_count ==
      packed.aggregate_inspection_bound;
  if (!audit.bounded_work_validated) {
    throw std::runtime_error(
        "the Phase 14 GPU proposal aggregate work counter does not close");
  }
  return payload;
}

void initialize_common_audit(
    DirectSparseFacetTopKProposalAudit& audit,
    const detail::Phase14FacetTopKProposalHostState& host,
    std::size_t maximum_query_count,
    std::span<const DirectSparseFacetTopKProposalQuery> queries,
    const PackedQueries& packed,
    DirectSparseFacetTopKProposalPolicy policy) {
  audit.snapshot_point_count = host.point_count;
  audit.static_device_coordinate_word_capacity =
      host.coordinate_bits.size();
  audit.static_device_morton_point_id_capacity =
      host.morton_point_ids.size();
  audit.static_device_snapshot_byte_capacity = checked_size_product(
      checked_size_sum(
          host.coordinate_bits.size(),
          host.morton_point_ids.size(),
          "the Phase 14 resident word count overflowed"),
      sizeof(std::uint64_t),
      "the Phase 14 resident byte count overflowed");
  audit.host_inverse_morton_entry_count =
      host.morton_position_by_point_id.size();
  audit.host_snapshot_byte_capacity = checked_size_product(
      host.morton_position_by_point_id.size(),
      sizeof(std::size_t),
      "the Phase 14 host inverse Morton byte count overflowed");
  audit.host_snapshot_byte_capacity = checked_size_sum(
      audit.host_snapshot_byte_capacity,
      audit.static_device_snapshot_byte_capacity,
      "the Phase 14 host snapshot byte count overflowed");
  audit.maximum_query_count = maximum_query_count;
  audit.physical_device_record_capacity = maximum_query_count;
  audit.physical_device_query_capacity = maximum_query_count;
  audit.static_device_record_buffer_byte_capacity =
      checked_size_product(
          maximum_query_count,
          sizeof(DeviceRecord),
          "the Phase 14 device record byte capacity overflowed");
  audit.static_device_query_buffer_byte_capacity =
      checked_size_product(
          maximum_query_count,
          sizeof(InputRecord),
          "the Phase 14 device query byte capacity overflowed");
  audit.host_record_copy_byte_capacity =
      audit.static_device_record_buffer_byte_capacity;
  audit.exact_center_projection_axis_count =
      packed.exact_center_projection_axis_count;
  audit.exact_center_projection_integer_division_count =
      packed.exact_center_projection_integer_division_count;
  audit.canonical_query_count = queries.size();
  audit.gpu_supported_center_query_count =
      packed.gpu_queries.size();
  audit.unsupported_center_query_count =
      packed.unsupported_center_count;
  audit.morton_window_radius = policy.morton_window_radius;
  audit.maximum_inspection_count_per_query =
      packed.maximum_inspection_bound;
  audit.aggregate_inspection_count_upper_bound =
      packed.aggregate_inspection_bound;
  audit.matching_immutable_point_namespace_validated = true;
  audit.canonical_query_order_validated = true;
  audit.homogeneous_facet_cardinality_validated = true;
  audit.fixed_capacity_preflight_satisfied = true;
  audit.exact_center_projection_division_bound_validated =
      audit.exact_center_projection_axis_count % 3U == 0U &&
      audit.exact_center_projection_axis_count / 3U ==
          audit.canonical_query_count &&
      audit.exact_center_projection_integer_division_count <=
          audit.exact_center_projection_axis_count;
}

void finalize_digest(
    DirectSparseFacetTopKProposalAudit& audit,
    std::span<const DirectSparseFacetTopKProposalQuery> queries,
    const PackedQueries& packed,
    std::span<const TranscriptRecord> records) noexcept {
  std::uint64_t digest = kFnvOffsetBasis;
  hash_word(digest, static_cast<std::uint64_t>(queries.size()));
  hash_word(
      digest,
      static_cast<std::uint64_t>(audit.morton_window_radius));
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const auto& key = queries[query_index].source_facet_key;
    hash_word(digest, static_cast<std::uint64_t>(query_index));
    hash_word(digest, static_cast<std::uint64_t>(key.point_count));
    for (std::size_t point_index = 0U;
         point_index < key.point_count;
         ++point_index) {
      hash_word(digest, key.point_ids[point_index]);
    }
    hash_word(
        digest,
        static_cast<std::uint64_t>(
            packed.center_supported[query_index]));
    hash_word(
        digest,
        static_cast<std::uint64_t>(
            packed.inspection_bounds[query_index]));
  }
  for (const TranscriptRecord& record : records) {
    hash_word(
        digest,
        hierarchy::fingerprint_exact_direct_sparse_facet_key(
            record.source_facet_key, kFullFingerprintMask));
    hash_word(
        digest,
        static_cast<std::uint64_t>(
            record.candidate_point_count));
    for (std::size_t candidate_index = 0U;
         candidate_index < record.candidate_point_count;
         ++candidate_index) {
      hash_word(
          digest, record.candidate_point_ids[candidate_index]);
    }
  }
  hash_word(
      digest,
      static_cast<std::uint64_t>(audit.inspected_neighbor_count));
  hash_word(
      digest,
      static_cast<std::uint64_t>(
          audit.floating_distance_evaluation_count));
  hash_word(
      digest,
      static_cast<std::uint64_t>(
          audit.floating_distance_rejection_count));
  audit.proposal_digest_fnv1a = digest;
}

[[nodiscard]] bool common_audit_closed(
    const DirectSparseFacetTopKProposalAudit& audit) noexcept {
  const bool counts_close =
      audit.gpu_supported_center_query_count +
              audit.unsupported_center_query_count ==
          audit.canonical_query_count &&
      audit.gpu_output_record_count ==
          audit.gpu_supported_center_query_count &&
      audit.nonempty_proposal_record_count <=
          audit.gpu_output_record_count &&
      audit.proposed_candidate_count <=
          hierarchy::
              direct_sparse_positive_facet_maximum_point_count *
              audit.nonempty_proposal_record_count &&
      audit.inspected_neighbor_count ==
          audit.aggregate_inspection_count_upper_bound &&
      audit.floating_distance_rejection_count <=
          audit.floating_distance_evaluation_count &&
      audit.floating_distance_evaluation_count <=
          audit.inspected_neighbor_count;
  const bool gpu_shape =
      audit.gpu_execution_performed
          ? audit.gpu_supported_center_query_count != 0U &&
                audit.gpu_kernel_launch_count == 1U &&
                audit.gpu_synchronization_count == 1U &&
                audit.buffer_epoch != 0U
          : audit.gpu_supported_center_query_count == 0U &&
                audit.gpu_output_record_count == 0U &&
                audit.gpu_kernel_launch_count == 0U &&
                audit.gpu_synchronization_count == 0U &&
                audit.buffer_epoch == 0U;
  return counts_close && gpu_shape &&
         audit.snapshot_point_count != 0U &&
         audit.static_device_coordinate_word_capacity ==
             3U * audit.snapshot_point_count &&
         audit.static_device_morton_point_id_capacity ==
             audit.snapshot_point_count &&
         audit.static_device_snapshot_byte_capacity ==
             sizeof(std::uint64_t) *
                 (audit.static_device_coordinate_word_capacity +
                  audit.static_device_morton_point_id_capacity) &&
         audit.host_inverse_morton_entry_count ==
             audit.snapshot_point_count &&
         audit.host_snapshot_byte_capacity ==
             audit.static_device_snapshot_byte_capacity +
                 sizeof(std::size_t) * audit.snapshot_point_count &&
         audit.physical_device_record_capacity ==
             audit.maximum_query_count &&
         audit.physical_device_query_capacity ==
             audit.maximum_query_count &&
         audit.static_device_record_buffer_byte_capacity ==
             sizeof(DeviceRecord) * audit.maximum_query_count &&
         audit.static_device_query_buffer_byte_capacity ==
             sizeof(InputRecord) * audit.maximum_query_count &&
         audit.host_record_copy_byte_capacity ==
             audit.static_device_record_buffer_byte_capacity &&
         audit.active_host_to_device_query_record_count ==
             audit.gpu_supported_center_query_count &&
         audit.active_host_to_device_query_byte_count ==
             sizeof(InputRecord) *
                 audit.active_host_to_device_query_record_count &&
         audit.initialized_device_output_record_count ==
             audit.gpu_supported_center_query_count &&
         audit.initialized_device_output_byte_count ==
             sizeof(DeviceRecord) *
                 audit.initialized_device_output_record_count &&
         audit.copied_device_to_host_record_count ==
             audit.gpu_supported_center_query_count &&
         audit.copied_device_to_host_byte_count ==
             sizeof(DeviceRecord) *
                 audit.copied_device_to_host_record_count &&
         audit.matching_immutable_point_namespace_validated &&
         audit.canonical_query_order_validated &&
         audit.homogeneous_facet_cardinality_validated &&
         audit.fixed_capacity_preflight_satisfied &&
         audit.exact_center_projection_division_bound_validated &&
         audit.exact_center_projection_axis_count % 3U == 0U &&
         audit.exact_center_projection_axis_count / 3U ==
             audit.canonical_query_count &&
         audit.exact_center_projection_integer_division_count <=
             audit.exact_center_projection_axis_count &&
         audit.supported_query_permutation_validated &&
         audit.active_record_candidate_tail_sentinel_validated &&
         audit.every_candidate_domain_validated &&
         audit.every_candidate_source_facet_exclusion_validated &&
         audit.every_candidate_morton_window_validated &&
         audit.candidate_distinctness_validated &&
         audit.bounded_work_validated &&
         audit.transcript_builder_invoked &&
         audit.floating_ordering_only &&
         !audit.exact_distance_or_partition_published &&
         !audit.scientific_decision_published &&
         !audit.hierarchy_reduction_or_attachment_published &&
         !audit.forbidden_global_structure_materialized &&
         !audit.public_status_claimed;
}

}  // namespace

bool DirectSparseFacetTopKProposalBatchResult::
    complete_proposal_batch() const noexcept {
  return schema_version ==
             direct_sparse_facet_top_k_gpu_proposal_schema_version &&
         transcript.complete_proposal_transcript() &&
         common_audit_closed(audit);
}

bool DirectSparseFacetTopKProposalBatchResult::
    certified_atomic_transcript_rejection() const noexcept {
  return schema_version ==
             direct_sparse_facet_top_k_gpu_proposal_schema_version &&
         transcript.certified_atomic_failure() &&
         common_audit_closed(audit);
}

bool DirectSparseFacetTopKProposalBatchResult::
    certified_outcome() const noexcept {
  return complete_proposal_batch() ||
         certified_atomic_transcript_rejection();
}

DirectSparseFacetTopKProposalContext::
    DirectSparseFacetTopKProposalContext(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        std::size_t maximum_query_count)
    : state_(
          std::make_shared<
              detail::Phase14FacetTopKProposalContextState>()),
      host_(
          std::make_unique<
              detail::Phase14FacetTopKProposalHostState>()),
      maximum_query_count_(maximum_query_count) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Phase 14 GPU proposal context requires a matching ready LBVH");
  }
  if (maximum_query_count == 0U) {
    throw std::invalid_argument(
        "the Phase 14 GPU proposal context requires nonzero query capacity");
  }
  (void)checked_size_product(
      maximum_query_count,
      sizeof(InputRecord),
      "the Phase 14 GPU proposal query capacity overflows size_t");
  (void)checked_size_product(
      maximum_query_count,
      sizeof(DeviceRecord),
      "the Phase 14 GPU proposal output capacity overflows size_t");

  host_->cloud_identity = cloud.identity_;
  host_->point_count = cloud.size();
  if (host_->point_count == 0U ||
      host_->point_count >
          static_cast<std::size_t>(
              spatial::CanonicalPointCloud::max_point_count)) {
    throw std::invalid_argument(
        "the Phase 14 GPU proposal context requires a nonempty PointId-sized cloud");
  }
  host_->coordinate_bits.resize(checked_size_product(
      host_->point_count,
      3U,
      "the Phase 14 resident coordinate extent overflows size_t"));
  for (std::size_t point_index = 0U;
       point_index < host_->point_count;
       ++point_index) {
    const auto bits = cloud.point(checked_point_id(
        point_index,
        "a Phase 14 resident point does not fit PointId"))
                          .canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      host_->coordinate_bits[
          axis * host_->point_count + point_index] = bits[axis];
    }
  }

  host_->morton_point_ids.reserve(host_->point_count);
  host_->morton_position_by_point_id.assign(
      host_->point_count, host_->point_count);
  const std::span<const spatial::MortonLeafRecord> leaves =
      index.leaves();
  if (leaves.size() != host_->point_count) {
    throw std::logic_error(
        "the Phase 14 GPU proposal LBVH has an incomplete Morton order");
  }
  for (std::size_t morton_position = 0U;
       morton_position < leaves.size();
       ++morton_position) {
    const spatial::PointId point_id = leaves[morton_position].point_id;
    if (point_id >= host_->point_count) {
      throw std::logic_error(
          "the Phase 14 GPU proposal Morton order has an invalid PointId");
    }
    const std::size_t point_index =
        static_cast<std::size_t>(point_id);
    if (host_->morton_position_by_point_id[point_index] !=
        host_->point_count) {
      throw std::logic_error(
          "the Phase 14 GPU proposal Morton order repeats a PointId");
    }
    host_->morton_position_by_point_id[point_index] =
        morton_position;
    host_->morton_point_ids.push_back(point_id);
  }
  if (std::find(
          host_->morton_position_by_point_id.begin(),
          host_->morton_position_by_point_id.end(),
          host_->point_count) !=
      host_->morton_position_by_point_id.end()) {
    throw std::logic_error(
        "the Phase 14 GPU proposal Morton order is not a permutation");
  }
}

DirectSparseFacetTopKProposalContext::
    ~DirectSparseFacetTopKProposalContext() noexcept = default;

DirectSparseFacetTopKProposalContext::
    DirectSparseFacetTopKProposalContext(
        DirectSparseFacetTopKProposalContext&&) noexcept = default;

DirectSparseFacetTopKProposalContext&
DirectSparseFacetTopKProposalContext::operator=(
    DirectSparseFacetTopKProposalContext&&) noexcept = default;

void DirectSparseFacetTopKProposalContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (!state_ || !host_) {
    throw std::logic_error(
        "a moved-from Phase 14 GPU proposal context cannot be used");
  }
  if (cloud.identity_ == nullptr ||
      cloud.identity_.get() != host_->cloud_identity.get() ||
      cloud.size() != host_->point_count) {
    throw std::invalid_argument(
        "the Phase 14 GPU proposal context was called with another PointId namespace");
  }
}

DirectSparseFacetTopKProposalBatchResult
DirectSparseFacetTopKProposalContext::build(
    const spatial::CanonicalPointCloud& cloud,
    const hierarchy::
        ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata,
    std::span<const DirectSparseFacetTopKProposalQuery>
        canonical_queries,
    DirectSparseFacetTopKProposalPolicy policy,
    const hierarchy::
        ExactDirectSparseFacetTopKProposalTranscriptBudget&
            transcript_budget) {
  require_matching_cloud(cloud);
  const PackedQueries packed = validate_and_pack_queries(
      *host_,
      canonical_queries,
      maximum_query_count_,
      policy);

  ValidatedPayload payload = state_->with_gpu_section([&]() {
    ValidatedPayload validated;
    if (packed.gpu_queries.empty()) {
      validated.audit.supported_query_permutation_validated = true;
      validated.audit.active_record_candidate_tail_sentinel_validated =
          true;
      validated.audit.every_candidate_domain_validated = true;
      validated.audit.every_candidate_source_facet_exclusion_validated =
          true;
      validated.audit.every_candidate_morton_window_validated = true;
      validated.audit.candidate_distinctness_validated = true;
      validated.audit.bounded_work_validated = true;
      return validated;
    }
    const DeviceBatch batch =
        detail::propose_phase14_facet_top_k_candidates_on_gpu(
            *state_,
            host_->coordinate_bits,
            host_->point_count,
            host_->morton_point_ids,
            packed.gpu_queries,
            maximum_query_count_,
            policy.morton_window_radius);
    ValidatedPayload gpu_payload = validate_gpu_batch(
        batch,
        *host_,
        canonical_queries,
        packed,
        policy,
        last_buffer_epoch_);
    last_buffer_epoch_ = batch.buffer_epoch;
    return gpu_payload;
  });

  initialize_common_audit(
      payload.audit,
      *host_,
      maximum_query_count_,
      canonical_queries,
      packed,
      policy);
  payload.audit.transcript_builder_invoked = true;
  finalize_digest(
      payload.audit,
      canonical_queries,
      packed,
      payload.proposal_records);

  DirectSparseFacetTopKProposalBatchResult result;
  result.transcript =
      hierarchy::
          build_exact_direct_sparse_facet_top_k_proposal_transcript(
              metadata,
              payload.proposal_records,
              transcript_budget);
  result.audit = std::move(payload.audit);
  if (!result.certified_outcome()) {
    throw std::logic_error(
        "the Phase 14 GPU proposal result did not close its bounded producer contract");
  }
  return result;
}

std::size_t DirectSparseFacetTopKProposalContext::point_count()
    const noexcept {
  return host_ == nullptr ? 0U : host_->point_count;
}

std::size_t
DirectSparseFacetTopKProposalContext::maximum_query_count()
    const noexcept {
  return state_ == nullptr ? 0U : maximum_query_count_;
}

}  // namespace morsehgp3d::gpu
