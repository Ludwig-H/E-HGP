#include "morsehgp3d/gpu/direct_sparse_facet_top_k_proposal.hpp"

#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

namespace {

using morsehgp3d::gpu::DirectSparseFacetTopKProposalBatchResult;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalQuery;
using morsehgp3d::gpu::MortonLbvhDeviceBuildResult;
using morsehgp3d::gpu::MortonLbvhDeviceLease;
using morsehgp3d::hierarchy::ExactDirectSparseFacetKey;
using morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalRecord;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::copy(
      point_ids.begin(), point_ids.end(), result.point_ids.begin());
  return result;
}

[[nodiscard]] morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptMetadata
metadata() {
  morsehgp3d::hierarchy::
      ExactDirectSparseFacetTopKProposalTranscriptMetadata result;
  result.source_batch_index = 14U;
  result.closed_batch_squared_level =
      morsehgp3d::exact::ExactLevel{morsehgp3d::exact::BigInt{4}};
  result.locator_snapshot_stamp.schema_version =
      morsehgp3d::hierarchy::
          direct_sparse_positive_facet_locator_schema_version;
  result.locator_snapshot_stamp.external_authority_id = 1401U;
  return result;
}

[[nodiscard]] morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptBudget
generous_transcript_budget() {
  const std::size_t maximum =
      std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] morsehgp3d::spatial::ExactLbvhTopKBudget
generous_top_k_budget() {
  const std::size_t maximum =
      std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] bool same_neighbors(
    std::span<const morsehgp3d::spatial::ExactNeighbor> left,
    std::span<const morsehgp3d::spatial::ExactNeighbor> right) {
  return left.size() == right.size() &&
         std::equal(
             left.begin(),
             left.end(),
             right.begin(),
             [](const auto& first, const auto& second) {
               return first.point_id == second.point_id &&
                      first.squared_distance ==
                          second.squared_distance;
             });
}

[[nodiscard]] bool same_partition(
    const morsehgp3d::spatial::TopKPartition& left,
    const morsehgp3d::spatial::TopKPartition& right) {
  return left.requested_rank() == right.requested_rank() &&
         left.cutoff_squared_distance() ==
             right.cutoff_squared_distance() &&
         same_neighbors(left.strict_below(), right.strict_below()) &&
         left.cutoff_shell_ids().size() ==
             right.cutoff_shell_ids().size() &&
         std::equal(
             left.cutoff_shell_ids().begin(),
             left.cutoff_shell_ids().end(),
             right.cutoff_shell_ids().begin(),
             right.cutoff_shell_ids().end()) &&
         left.canonical_choice_ids().size() ==
             right.canonical_choice_ids().size() &&
         std::equal(
             left.canonical_choice_ids().begin(),
             left.canonical_choice_ids().end(),
             right.canonical_choice_ids().begin(),
             right.canonical_choice_ids().end());
}

template <std::size_t CandidateCount>
[[nodiscard]] bool has_candidate_ids(
    const ExactDirectSparseFacetTopKProposalRecord& record,
    const std::array<PointId, CandidateCount>& expected) {
  return record.candidate_point_count == expected.size() &&
         std::equal(
             record.candidate_point_ids.begin(),
             record.candidate_point_ids.begin() +
                 static_cast<std::ptrdiff_t>(
                     record.candidate_point_count),
             expected.begin(),
             expected.end());
}

[[nodiscard]] const ExactDirectSparseFacetTopKProposalRecord&
require_record(
    const DirectSparseFacetTopKProposalBatchResult& result,
    const ExactDirectSparseFacetKey& source_key) {
  const auto found = std::find_if(
      result.transcript.proposal_records.begin(),
      result.transcript.proposal_records.end(),
      [&source_key](const auto& record) {
        return record.source_facet_key == source_key;
      });
  if (found == result.transcript.proposal_records.end()) {
    throw std::runtime_error(
        "the Phase 14 GPU qualification produced no useful proposal row");
  }
  return *found;
}

void require_active_traffic(
    const DirectSparseFacetTopKProposalBatchResult& result,
    std::size_t active_query_count,
    std::size_t maximum_query_count) {
  constexpr std::size_t kQueryBytes = 208U;
  constexpr std::size_t kRecordBytes = 144U;
  const auto& audit = result.audit;
  if (audit.maximum_query_count != maximum_query_count ||
      audit.static_device_query_buffer_byte_capacity !=
          kQueryBytes * maximum_query_count ||
      audit.static_device_record_buffer_byte_capacity !=
          kRecordBytes * maximum_query_count ||
      audit.host_record_copy_byte_capacity !=
          kRecordBytes * maximum_query_count ||
      audit.active_host_to_device_query_record_count !=
          active_query_count ||
      audit.active_host_to_device_query_byte_count !=
          kQueryBytes * active_query_count ||
      audit.initialized_device_output_record_count !=
          active_query_count ||
      audit.initialized_device_output_byte_count !=
          kRecordBytes * active_query_count ||
      audit.copied_device_to_host_record_count !=
          active_query_count ||
      audit.copied_device_to_host_byte_count !=
          kRecordBytes * active_query_count ||
      audit.exact_center_projection_axis_count !=
          3U * audit.canonical_query_count ||
      audit.exact_center_projection_integer_division_count +
              audit.exact_center_projection_zero_axis_count +
              audit.exact_center_projection_unsupported_axis_count !=
          audit.exact_center_projection_axis_count ||
      !audit.exact_center_projection_division_bound_validated) {
    throw std::runtime_error(
        "the Phase 14K GPU qualification observed invalid active traffic or center projection work");
  }
}

void require_cuda_morton_lease(
    const MortonLbvhDeviceBuildResult& build,
    const MortonLbvhDeviceLease& lease,
    const CanonicalPointCloud& cloud) {
  const std::size_t point_count = cloud.size();
  const std::size_t expected_persistent_device_bytes =
      32U * point_count;
  const auto& build_audit = build.audit();
  const auto& lease_audit = lease.audit();
  if (!build.cuda_qualified_build() ||
      !build.certified_index().validated_for(cloud) ||
      !lease.ready() || !lease.cuda_resident() ||
      lease_audit.maximum_point_count != point_count ||
      lease_audit.point_count != point_count ||
      lease_audit.persistent_device_byte_capacity !=
          expected_persistent_device_bytes ||
      lease_audit.retained_host_snapshot_byte_count != 0U ||
      lease_audit.source_snapshot_epoch !=
          build_audit.snapshot_buffer_epoch ||
      !lease_audit.source_snapshot_import_certified ||
      !lease_audit.canonical_coordinate_words_retained ||
      !lease_audit.active_morton_point_ids_retained ||
      !lease_audit.builder_transients_released ||
      !lease_audit.cuda_device_storage_retained ||
      lease_audit.host_fake_lifecycle_exercised ||
      lease_audit.second_host_snapshot_retained) {
    throw std::runtime_error(
        "the Phase 14M/14N CUDA qualification did not produce one compact resident lease");
  }
}

void require_legacy_snapshot(
    const DirectSparseFacetTopKProposalBatchResult& first,
    const DirectSparseFacetTopKProposalBatchResult& second,
    std::size_t point_count) {
  const std::size_t expected_device_snapshot_bytes =
      32U * point_count;
  const std::size_t expected_host_snapshot_bytes =
      40U * point_count;
  if (sizeof(std::size_t) != 8U ||
      first.audit.device_snapshot_adopted_from_morton_lbvh_lease ||
      first.audit.adopted_device_snapshot_owner_retained ||
      first.audit.source_morton_lbvh_lease_epoch != 0U ||
      first.audit.host_coordinate_snapshot_word_count !=
          3U * point_count ||
      first.audit.host_morton_order_snapshot_entry_count !=
          point_count ||
      first.audit.host_inverse_morton_entry_count != point_count ||
      first.audit.host_snapshot_byte_capacity !=
          expected_host_snapshot_bytes ||
      first.audit.snapshot_host_to_device_byte_count !=
          expected_device_snapshot_bytes ||
      second.audit.snapshot_host_to_device_byte_count != 0U) {
    throw std::runtime_error(
        "the Phase 14J legacy snapshot witness did not retain 40n host bytes and one 32n upload");
  }
}

void require_adopted_snapshot(
    const DirectSparseFacetTopKProposalBatchResult& result,
    std::size_t point_count,
    std::size_t active_query_count,
    std::size_t maximum_query_count,
    std::uint64_t source_snapshot_epoch) {
  require_active_traffic(
      result, active_query_count, maximum_query_count);
  const std::size_t expected_host_snapshot_bytes =
      8U * point_count;
  if (sizeof(std::size_t) != 8U ||
      !result.complete_proposal_batch() ||
      result.audit.snapshot_point_count != point_count ||
      result.audit.static_device_snapshot_byte_capacity !=
          32U * point_count ||
      result.audit.host_coordinate_snapshot_word_count != 0U ||
      result.audit.host_morton_order_snapshot_entry_count != 0U ||
      result.audit.host_inverse_morton_entry_count != point_count ||
      result.audit.host_snapshot_byte_capacity !=
          expected_host_snapshot_bytes ||
      result.audit.snapshot_host_to_device_byte_count != 0U ||
      result.audit.source_morton_lbvh_lease_epoch !=
          source_snapshot_epoch ||
      !result.audit
           .device_snapshot_adopted_from_morton_lbvh_lease ||
      !result.audit.adopted_device_snapshot_owner_retained ||
      !result.audit.gpu_execution_performed) {
    throw std::runtime_error(
        "the Phase 14O qualification did not retain its adopted owner with an 8n host inverse and zero snapshot upload");
  }
}

}  // namespace

int main() {
  try {
    using morsehgp3d::exact::CertifiedPoint3;
    using morsehgp3d::gpu::DirectSparseFacetTopKProposalContext;
    using morsehgp3d::gpu::DirectSparseFacetTopKProposalPolicy;
    using morsehgp3d::gpu::MortonLbvhBuildContext;
    using morsehgp3d::hierarchy::build_exact_facet_miniball;
    using morsehgp3d::spatial::ExclusionSet;
    using morsehgp3d::spatial::MortonLbvhIndex;

    const std::array points{
        CertifiedPoint3::from_binary64(-2.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(2.0, 0.0, 0.0)};
    CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(points);
    MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

    const ExactDirectSparseFacetKey outer_key = key({0U, 4U});
    const ExactDirectSparseFacetKey inner_key = key({1U, 3U});
    const auto outer_miniball = build_exact_facet_miniball(
        cloud,
        std::span{
            outer_key.point_ids.data(), outer_key.point_count});
    const auto inner_miniball = build_exact_facet_miniball(
        cloud,
        std::span{
            inner_key.point_ids.data(), inner_key.point_count});
    const morsehgp3d::exact::ExactCenter3 rational_hint_center{
        morsehgp3d::exact::BigInt{1},
        morsehgp3d::exact::BigInt{0},
        morsehgp3d::exact::BigInt{0},
        morsehgp3d::exact::BigInt{3}};
    const std::array queries{
        DirectSparseFacetTopKProposalQuery{
            outer_key, rational_hint_center},
        DirectSparseFacetTopKProposalQuery{
            inner_key, inner_miniball.center}};

    DirectSparseFacetTopKProposalContext context{index, cloud, 4U};
    const DirectSparseFacetTopKProposalPolicy policy{4U};
    const auto transcript_metadata = metadata();
    const auto transcript_budget = generous_transcript_budget();
    const DirectSparseFacetTopKProposalBatchResult first =
        context.build(
            cloud,
            transcript_metadata,
            queries,
            policy,
            transcript_budget);
    const DirectSparseFacetTopKProposalBatchResult second =
        context.build(
            cloud,
            transcript_metadata,
            queries,
            policy,
            transcript_budget);
    const auto& outer_record = require_record(first, outer_key);
    const auto& inner_record = require_record(first, inner_key);
    const std::array<PointId, 2U> expected_outer{2U, 3U};
    const std::array<PointId, 2U> expected_inner{0U, 2U};
    require_active_traffic(first, 2U, 4U);
    require_active_traffic(second, 2U, 4U);
    require_legacy_snapshot(first, second, cloud.size());
    if (!first.complete_proposal_batch() ||
        !second.complete_proposal_batch() ||
        first.audit.buffer_epoch != 1U ||
        second.audit.buffer_epoch != 2U ||
        first.audit.proposal_digest_fnv1a !=
            second.audit.proposal_digest_fnv1a ||
        first.transcript.proposal_records !=
            second.transcript.proposal_records ||
        first.audit.gpu_kernel_launch_count != 1U ||
        first.audit.gpu_synchronization_count != 1U ||
        first.audit.inspected_neighbor_count >
            2U * 2U * policy.morton_window_radius *
                queries.size() ||
        first.audit.static_device_snapshot_byte_capacity !=
            32U * cloud.size() ||
        first.audit.host_snapshot_byte_capacity !=
            (32U + sizeof(std::size_t)) * cloud.size() ||
        !has_candidate_ids(outer_record, expected_outer) ||
        !has_candidate_ids(inner_record, expected_inner) ||
        first.audit.scientific_decision_published ||
        first.audit.forbidden_global_structure_materialized ||
        first.audit.public_status_claimed) {
      throw std::runtime_error(
          "the Phase 14 GPU qualification audit did not close");
    }

    MortonLbvhBuildContext adoption_builder{cloud.size()};
    MortonLbvhDeviceBuildResult adoption_build =
        adoption_builder.build(cloud);
    MortonLbvhDeviceLease adoption_lease =
        adoption_builder.release_device_lease(adoption_build);
    require_cuda_morton_lease(
        adoption_build, adoption_lease, cloud);
    const auto adoption_lease_audit = adoption_lease.audit();
    DirectSparseFacetTopKProposalContext adopted_context{
        adoption_build.certified_index(),
        cloud,
        std::move(adoption_lease),
        4U};
    if (adoption_lease.ready()) {
      throw std::runtime_error(
          "the Phase 14O context did not consume the move-only device lease");
    }
    const DirectSparseFacetTopKProposalBatchResult adopted_first =
        adopted_context.build(
            cloud,
            transcript_metadata,
            queries,
            policy,
            transcript_budget);
    const DirectSparseFacetTopKProposalBatchResult adopted_second =
        adopted_context.build(
            cloud,
            transcript_metadata,
            queries,
            policy,
            transcript_budget);
    require_adopted_snapshot(
        adopted_first,
        cloud.size(),
        queries.size(),
        4U,
        adoption_lease_audit.source_snapshot_epoch);
    require_adopted_snapshot(
        adopted_second,
        cloud.size(),
        queries.size(),
        4U,
        adoption_lease_audit.source_snapshot_epoch);
    if (adopted_first.audit.buffer_epoch != 1U ||
        adopted_second.audit.buffer_epoch != 2U ||
        adopted_first.transcript != first.transcript ||
        adopted_second.transcript != adopted_first.transcript ||
        adopted_first.audit.proposal_digest_fnv1a !=
            first.audit.proposal_digest_fnv1a ||
        adopted_second.audit.proposal_digest_fnv1a !=
            adopted_first.audit.proposal_digest_fnv1a) {
      throw std::runtime_error(
          "the Phase 14O adopted CUDA path changed the legacy transcript, digest, or repeated-batch identity");
    }

    const std::array<PointId, 0U> none{};
    const ExclusionSet exclusions =
        ExclusionSet::from_ids(none, cloud, 0U);
    const auto unseeded = morsehgp3d::spatial::lbvh_top_k(
        index,
        cloud,
        outer_miniball.center,
        outer_key.point_count,
        exclusions);
    const auto seeded = morsehgp3d::spatial::lbvh_top_k_budgeted(
        index,
        cloud,
        outer_miniball.center,
        outer_key.point_count,
        exclusions,
        std::span{
            outer_key.point_ids.data(), outer_key.point_count},
        std::span{
            outer_record.candidate_point_ids.data(),
            outer_record.candidate_point_count},
        generous_top_k_budget());
    if (!seeded.complete() ||
        !same_partition(unseeded, seeded.partition())) {
      throw std::runtime_error(
          "the exact CPU top-k result changed after the GPU proposal");
    }

    const std::array rank_ten_points{
        CertifiedPoint3::from_binary64(-10.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(10.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, -9.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 9.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, -8.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 8.0),
        CertifiedPoint3::from_binary64(3.0, 4.0, 0.0),
        CertifiedPoint3::from_binary64(-3.0, -4.0, 0.0),
        CertifiedPoint3::from_binary64(1.0, 2.0, 3.0),
        CertifiedPoint3::from_binary64(-1.0, -2.0, -3.0),
        CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 2.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 3.0),
        CertifiedPoint3::from_binary64(2.0, 2.0, 2.0),
        CertifiedPoint3::from_binary64(4.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 5.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 6.0),
        CertifiedPoint3::from_binary64(4.0, 4.0, 2.0),
        CertifiedPoint3::from_binary64(7.0, 0.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 8.0, 0.0),
        CertifiedPoint3::from_binary64(0.0, 0.0, 9.0),
        CertifiedPoint3::from_binary64(5.0, 5.0, 5.5),
        CertifiedPoint3::from_binary64(9.0, 5.0, 0.0),
        CertifiedPoint3::from_binary64(8.0, 8.0, 0.0)};
    CanonicalPointCloud rank_ten_cloud =
        CanonicalPointCloud::rejecting_duplicates(rank_ten_points);
    MortonLbvhIndex rank_ten_index =
        MortonLbvhIndex::build(rank_ten_cloud);
    const ExactDirectSparseFacetKey rank_ten_key = key(
        {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});
    const auto rank_ten_miniball = build_exact_facet_miniball(
        rank_ten_cloud,
        std::span{
            rank_ten_key.point_ids.data(),
            rank_ten_key.point_count});
    const std::array rank_ten_queries{
        DirectSparseFacetTopKProposalQuery{
            rank_ten_key, rank_ten_miniball.center}};
    const DirectSparseFacetTopKProposalPolicy rank_ten_policy{
        rank_ten_cloud.size() - 1U};
    DirectSparseFacetTopKProposalContext rank_ten_context{
        rank_ten_index, rank_ten_cloud, 1U};
    const auto rank_ten_result = rank_ten_context.build(
        rank_ten_cloud,
        transcript_metadata,
        rank_ten_queries,
        rank_ten_policy,
        transcript_budget);
    const auto& rank_ten_record =
        require_record(rank_ten_result, rank_ten_key);

    MortonLbvhBuildContext rank_ten_adoption_builder{
        rank_ten_cloud.size()};
    MortonLbvhDeviceBuildResult rank_ten_adoption_build =
        rank_ten_adoption_builder.build(rank_ten_cloud);
    MortonLbvhDeviceLease rank_ten_adoption_lease =
        rank_ten_adoption_builder.release_device_lease(
            rank_ten_adoption_build);
    require_cuda_morton_lease(
        rank_ten_adoption_build,
        rank_ten_adoption_lease,
        rank_ten_cloud);
    const auto rank_ten_adoption_lease_audit =
        rank_ten_adoption_lease.audit();
    DirectSparseFacetTopKProposalContext
        rank_ten_adopted_context{
            rank_ten_adoption_build.certified_index(),
            rank_ten_cloud,
            std::move(rank_ten_adoption_lease),
            1U};
    if (rank_ten_adoption_lease.ready()) {
      throw std::runtime_error(
          "the Phase 14O K=10 context did not consume its device lease");
    }
    const auto rank_ten_adopted_result =
        rank_ten_adopted_context.build(
            rank_ten_cloud,
            transcript_metadata,
            rank_ten_queries,
            rank_ten_policy,
            transcript_budget);
    require_adopted_snapshot(
        rank_ten_adopted_result,
        rank_ten_cloud.size(),
        rank_ten_queries.size(),
        1U,
        rank_ten_adoption_lease_audit.source_snapshot_epoch);
    const auto& rank_ten_adopted_record =
        require_record(rank_ten_adopted_result, rank_ten_key);
    const std::array<PointId, 10U> expected_rank_ten{
        10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 20U};
    const auto rank_ten_unseeded = morsehgp3d::spatial::lbvh_top_k(
        rank_ten_index,
        rank_ten_cloud,
        rank_ten_miniball.center,
        rank_ten_key.point_count,
        ExclusionSet::from_ids(none, rank_ten_cloud, 0U));
    const auto rank_ten_seeded =
        morsehgp3d::spatial::lbvh_top_k_budgeted(
            rank_ten_index,
            rank_ten_cloud,
            rank_ten_miniball.center,
            rank_ten_key.point_count,
            ExclusionSet::from_ids(none, rank_ten_cloud, 0U),
            std::span{
                rank_ten_key.point_ids.data(),
                rank_ten_key.point_count},
            std::span{
                rank_ten_record.candidate_point_ids.data(),
                rank_ten_record.candidate_point_count},
            generous_top_k_budget());
    if (!rank_ten_result.complete_proposal_batch() ||
        !has_candidate_ids(rank_ten_record, expected_rank_ten) ||
        !has_candidate_ids(
            rank_ten_adopted_record, expected_rank_ten) ||
        rank_ten_result.audit.inspected_neighbor_count != 230U ||
        rank_ten_adopted_result.audit.inspected_neighbor_count !=
            230U ||
        rank_ten_adopted_result.audit.buffer_epoch != 1U ||
        rank_ten_adopted_result.transcript !=
            rank_ten_result.transcript ||
        rank_ten_adopted_result.audit.proposal_digest_fnv1a !=
            rank_ten_result.audit.proposal_digest_fnv1a ||
        !rank_ten_seeded.complete() ||
        !same_partition(
            rank_ten_unseeded, rank_ten_seeded.partition())) {
      throw std::runtime_error(
          "the Phase 14 GPU qualification did not close its K=10 proposal");
    }
    require_active_traffic(rank_ten_result, 1U, 1U);

    const std::array transition_queries{
        DirectSparseFacetTopKProposalQuery{
            key({0U, 1U}), rational_hint_center},
        DirectSparseFacetTopKProposalQuery{
            key({0U, 2U}), rational_hint_center},
        DirectSparseFacetTopKProposalQuery{
            key({0U, 3U}), rational_hint_center},
        DirectSparseFacetTopKProposalQuery{
            key({0U, 4U}), rational_hint_center},
        DirectSparseFacetTopKProposalQuery{
            key({1U, 2U}), rational_hint_center}};
    DirectSparseFacetTopKProposalContext transition_context{
        index, cloud, 6U};
    const std::span transition_span{transition_queries};
    const auto transition_four = transition_context.build(
        cloud,
        transcript_metadata,
        transition_span.first(4U),
        policy,
        transcript_budget);
    const auto transition_one = transition_context.build(
        cloud,
        transcript_metadata,
        transition_span.first(1U),
        policy,
        transcript_budget);
    const auto transition_five = transition_context.build(
        cloud,
        transcript_metadata,
        transition_span,
        policy,
        transcript_budget);
    require_active_traffic(transition_four, 4U, 6U);
    require_active_traffic(transition_one, 1U, 6U);
    require_active_traffic(transition_five, 5U, 6U);
    if (!transition_four.complete_proposal_batch() ||
        !transition_one.complete_proposal_batch() ||
        !transition_five.complete_proposal_batch() ||
        transition_four.audit.buffer_epoch != 1U ||
        transition_one.audit.buffer_epoch != 2U ||
        transition_five.audit.buffer_epoch != 3U) {
      throw std::runtime_error(
          "the Phase 14J active-prefix transition did not close");
    }

    std::cout
        << "{\"active_device_to_host_byte_count\":"
        << first.audit.copied_device_to_host_byte_count << ','
        << "\"active_host_to_device_query_byte_count\":"
        << first.audit.active_host_to_device_query_byte_count << ','
        << "\"active_output_initialization_byte_count\":"
        << first.audit.initialized_device_output_byte_count << ','
        << "\"active_prefix_transition_4_1_5_validated\":true,"
        << "\"adopted_device_snapshot_owner_retained\":true,"
        << "\"backend\":\"cuda_g4\","
        << "\"buffer_epoch\":2,"
        << "\"candidate_count\":"
        << first.audit.proposed_candidate_count << ','
        << "\"device_snapshot_adopted_from_morton_lbvh_lease\":true,"
        << "\"exact_cpu_partition_equal\":true,"
        << "\"exact_center_projection_axis_count\":"
        << first.audit.exact_center_projection_axis_count << ','
        << "\"exact_center_projection_integer_division_count\":"
        << first.audit
               .exact_center_projection_integer_division_count
        << ','
        << "\"exact_center_projection_unsupported_axis_count\":"
        << first.audit
               .exact_center_projection_unsupported_axis_count
        << ','
        << "\"exact_center_projection_zero_axis_count\":"
        << first.audit.exact_center_projection_zero_axis_count
        << ','
        << "\"forbidden_global_structure_materialized\":false,"
        << "\"gpu_kernel_launch_count\":1,"
        << "\"host_snapshot_byte_capacity\":"
        << first.audit.host_snapshot_byte_capacity << ','
        << "\"inspected_neighbor_count\":"
        << first.audit.inspected_neighbor_count << ','
        << "\"legacy_14j_first_snapshot_h2d_byte_count\":"
        << first.audit.snapshot_host_to_device_byte_count << ','
        << "\"legacy_14j_second_snapshot_h2d_byte_count\":"
        << second.audit.snapshot_host_to_device_byte_count << ','
        << "\"mode\":\"proposal_only\","
        << "\"non_dyadic_hint_candidate_ids_validated\":true,"
        << "\"phase\":\"14K\","
        << "\"phase14m_cuda_builder_qualified\":true,"
        << "\"phase14n_cuda_resident_lease_qualified\":true,"
        << "\"phase14n_persistent_device_byte_capacity\":"
        << adoption_lease_audit.persistent_device_byte_capacity << ','
        << "\"phase14o_first_buffer_epoch\":"
        << adopted_first.audit.buffer_epoch << ','
        << "\"phase14o_first_snapshot_h2d_byte_count\":"
        << adopted_first.audit.snapshot_host_to_device_byte_count << ','
        << "\"phase14o_host_coordinate_snapshot_word_count\":"
        << adopted_first.audit.host_coordinate_snapshot_word_count << ','
        << "\"phase14o_host_inverse_morton_entry_count\":"
        << adopted_first.audit.host_inverse_morton_entry_count << ','
        << "\"phase14o_host_morton_order_snapshot_entry_count\":"
        << adopted_first.audit
               .host_morton_order_snapshot_entry_count
        << ','
        << "\"phase14o_host_snapshot_byte_capacity\":"
        << adopted_first.audit.host_snapshot_byte_capacity << ','
        << "\"phase14o_k10_adoption_qualified\":true,"
        << "\"phase14o_k10_snapshot_h2d_byte_count\":"
        << rank_ten_adopted_result.audit
               .snapshot_host_to_device_byte_count
        << ','
        << "\"phase14o_legacy_digest_equal\":true,"
        << "\"phase14o_legacy_transcript_equal\":true,"
        << "\"phase14o_mode\":\"device_morton_lbvh_lease_adoption\","
        << "\"phase14o_repeated_batch_equal\":true,"
        << "\"phase14o_second_buffer_epoch\":"
        << adopted_second.audit.buffer_epoch << ','
        << "\"phase14o_second_snapshot_h2d_byte_count\":"
        << adopted_second.audit.snapshot_host_to_device_byte_count << ','
        << "\"phase14o_source_snapshot_epoch\":"
        << adopted_first.audit.source_morton_lbvh_lease_epoch << ','
        << "\"phase14o_validated\":true,"
        << "\"profile\":\"hgp_reduced\","
        << "\"proposal_digest_fnv1a\":"
        << adopted_first.audit.proposal_digest_fnv1a << ','
        << "\"public_status\":null,"
        << "\"public_status_claimed\":false,"
        << "\"rank_ten_candidate_count\":"
        << rank_ten_record.candidate_point_count << ','
        << "\"rank_ten_three_axis_candidate_ids_validated\":true,"
        << "\"rank_ten_exact_cpu_partition_equal\":true,"
        << "\"rank_ten_inspected_neighbor_count\":"
        << rank_ten_result.audit.inspected_neighbor_count << ','
        << "\"static_device_snapshot_byte_capacity\":"
        << first.audit.static_device_snapshot_byte_capacity << ','
        << "\"static_device_query_buffer_byte_capacity\":"
        << first.audit.static_device_query_buffer_byte_capacity << ','
        << "\"static_device_record_buffer_byte_capacity\":"
        << adopted_first.audit.static_device_record_buffer_byte_capacity
        << ','
        << "\"schema\":\"morsehgp3d.phase14k.facet_top_k_cuda_qualification.v4\"}"
        << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Phase 14O GPU facet top-k qualification failed: "
        << error.what() << '\n';
    return 1;
  }
}
