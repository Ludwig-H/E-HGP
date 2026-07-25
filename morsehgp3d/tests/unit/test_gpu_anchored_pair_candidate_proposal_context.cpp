#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/anchored_pair_candidate_proposal.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "phase14_anchored_pair_candidate_proposal_internal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeAnchoredPairProposalBehavior : std::uint8_t {
  complete,
  budget_exhausted,
  capacity_exhausted,
};

enum class FakeAnchoredPairProposalCorruption : std::uint8_t {
  none,
  wrong_digest,
  wrong_source_epoch,
  broken_segment_partition,
  out_of_domain_node,
  wrong_prune_cardinality,
  cuda_metadata_from_host_fake,
  wrong_capacity_echo,
};

struct FakeAnchoredPairProposalConfiguration {
  FakeAnchoredPairProposalBehavior behavior{
      FakeAnchoredPairProposalBehavior::complete};
  FakeAnchoredPairProposalCorruption corruption{
      FakeAnchoredPairProposalCorruption::none};
};

namespace {

FakeAnchoredPairProposalConfiguration configuration;
std::size_t launcher_call_count{};

}  // namespace

void configure_fake_anchored_pair_proposal(
    FakeAnchoredPairProposalConfiguration value) noexcept {
  configuration = value;
}

void reset_fake_anchored_pair_proposal() noexcept {
  configuration = {};
  launcher_call_count = 0U;
}

[[nodiscard]] FakeAnchoredPairProposalConfiguration
fake_anchored_pair_proposal_configuration() noexcept {
  return configuration;
}

void note_fake_anchored_pair_proposal_call() noexcept {
  ++launcher_call_count;
}

[[nodiscard]] std::size_t
fake_anchored_pair_proposal_call_count() noexcept {
  return launcher_call_count;
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

[[nodiscard]] std::uint64_t as_u64(std::size_t value) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::overflow_error("the fake proposal extent does not fit u64");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t complete_cursor_kind() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateCursorKind::complete);
}

[[nodiscard]] std::uint64_t active_cursor_kind() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateCursorKind::active);
}

[[nodiscard]] std::uint64_t complete_status() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateSegmentStatus::complete);
}

[[nodiscard]] std::uint64_t budget_status() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateSegmentStatus::budget_exhausted);
}

[[nodiscard]] std::uint64_t capacity_status() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateSegmentStatus::capacity_exhausted);
}

[[nodiscard]] std::uint64_t no_stop_reason() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateStopReason::none);
}

[[nodiscard]] std::uint64_t node_stop_reason() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateStopReason::node_visit_limit);
}

[[nodiscard]] std::uint64_t record_stop_reason() noexcept {
  return static_cast<std::uint64_t>(
      Phase14AnchoredPairCandidateStopReason::transcript_record_limit);
}

[[nodiscard]] Phase14AnchoredPairCandidateTranscriptRecord
candidate_record(std::size_t node_index) {
  return Phase14AnchoredPairCandidateTranscriptRecord{
      as_u64(node_index), 0U};
}

[[nodiscard]] std::uint64_t prefix_mask(std::uint64_t bit_count) {
  if (bit_count == 0U || bit_count >= 64U) {
    throw std::logic_error("the fake prune cardinality is out of range");
  }
  return (UINT64_C(1) << bit_count) - UINT64_C(1);
}

void append_default_records(
    Phase14AnchoredPairCandidateDeviceBatch& batch,
    const Phase14AnchoredPairCandidateQueryInputRecord& query,
    std::size_t record_limit,
    std::size_t node_count) {
  if (record_limit == 0U) {
    return;
  }
  batch.records.push_back(candidate_record(node_count > 1U ? 1U : 0U));
  const std::uint64_t required_witness_count =
      query.maximum_closed_rank - UINT64_C(1);
  if (record_limit > 1U && node_count > 1U &&
      query.witness_count >= required_witness_count) {
    batch.records.push_back(
        Phase14AnchoredPairCandidateTranscriptRecord{
            UINT64_C(0), prefix_mask(required_witness_count)});
  }
}

}  // namespace

Phase14AnchoredPairCandidateDeviceBatch
propose_phase14_anchored_pair_candidates_on_gpu(
    Phase14AnchoredPairCandidateProposalContextState& context,
    const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
    std::span<const Phase14AnchoredPairCandidateQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t physical_transcript_record_capacity,
    std::size_t active_total_transcript_record_limit) {
  using test_support::FakeAnchoredPairProposalBehavior;
  using test_support::FakeAnchoredPairProposalCorruption;

  if (!traversal.owner || !traversal.host_fake ||
      traversal.cuda_resident || traversal.cuda_device != -1 ||
      traversal.device_coordinate_bits != nullptr ||
      traversal.device_morton_point_ids != nullptr ||
      traversal.device_nodes != nullptr || traversal.point_count == 0U ||
      traversal.node_count != 2U * traversal.point_count - 1U ||
      queries.empty() || queries.size() > maximum_query_count ||
      !context.fixed_capacities_match(
          maximum_query_count, physical_transcript_record_capacity) ||
      active_total_transcript_record_limit >
          physical_transcript_record_capacity) {
    throw std::runtime_error(
        "the fake proposal launcher received an invalid traversal");
  }
  test_support::note_fake_anchored_pair_proposal_call();
  const test_support::FakeAnchoredPairProposalConfiguration config =
      test_support::fake_anchored_pair_proposal_configuration();

  Phase14AnchoredPairCandidateDeviceBatch batch;
  batch.buffer_epoch = context.advance_epoch();
  batch.source_snapshot_epoch = traversal.source_snapshot_epoch;
  batch.execution_kind =
      Phase14AnchoredPairCandidateExecutionKind::host_fake;
  batch.compaction_kind =
      Phase14AnchoredPairCandidateCompactionKind::host_fake;
  batch.cuda_device = -1;
  batch.cuda_path_qualified = false;
  batch.physical_query_capacity = maximum_query_count;
  batch.physical_transcript_record_capacity =
      physical_transcript_record_capacity;
  batch.active_total_transcript_record_limit =
      active_total_transcript_record_limit;

  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const Phase14AnchoredPairCandidateQueryInputRecord& query =
        queries[query_index];
    const std::size_t record_begin = batch.records.size();
    const std::size_t total_remaining =
        active_total_transcript_record_limit - batch.records.size();
    const std::size_t query_record_limit = static_cast<std::size_t>(
        query.maximum_transcript_record_count);
    const std::size_t visit_limit =
        static_cast<std::size_t>(query.maximum_node_visit_count);
    const std::size_t emission_limit = std::min(
        {total_remaining, query_record_limit, visit_limit});

    Phase14AnchoredPairCandidateSegmentRecord segment;
    segment.query_index = query.query_index;
    segment.query_digest_fnv1a = query.query_digest_fnv1a;
    segment.buffer_epoch = batch.buffer_epoch;
    segment.source_snapshot_epoch = traversal.source_snapshot_epoch;
    segment.input_cursor_kind = query.cursor_kind;
    segment.input_cursor_node_index = query.cursor_node_index;
    segment.failure_code = static_cast<std::uint64_t>(
        Phase14AnchoredPairCandidateFailureCode::none);
    segment.record_begin = as_u64(record_begin);

    switch (config.behavior) {
      case FakeAnchoredPairProposalBehavior::complete:
        append_default_records(
            batch,
            query,
            std::min<std::size_t>(emission_limit, 2U),
            traversal.node_count);
        segment.status = complete_status();
        segment.stop_reason = no_stop_reason();
        segment.next_cursor_kind = complete_cursor_kind();
        segment.next_cursor_node_index =
            phase14_anchored_pair_candidate_invalid_node_index;
        segment.node_visit_count =
            as_u64(batch.records.size() - record_begin);
        break;
      case FakeAnchoredPairProposalBehavior::budget_exhausted:
        append_default_records(
            batch,
            query,
            std::min<std::size_t>(emission_limit, 1U),
            traversal.node_count);
        segment.status = budget_status();
        segment.stop_reason = node_stop_reason();
        segment.next_cursor_kind = active_cursor_kind();
        segment.next_cursor_node_index = UINT64_C(0);
        segment.node_visit_count = query.maximum_node_visit_count;
        break;
      case FakeAnchoredPairProposalBehavior::capacity_exhausted: {
        if (emission_limit != query_record_limit ||
            emission_limit > traversal.node_count) {
          throw std::runtime_error(
              "the fake capacity scenario cannot close its exact cap");
        }
        for (std::size_t offset = 0U;
             offset < emission_limit;
             ++offset) {
          batch.records.push_back(
              candidate_record(emission_limit - 1U - offset));
        }
        segment.status = capacity_status();
        segment.stop_reason = record_stop_reason();
        segment.next_cursor_kind = active_cursor_kind();
        segment.next_cursor_node_index = UINT64_C(0);
        segment.node_visit_count = as_u64(emission_limit);
        break;
      }
    }
    segment.record_count = as_u64(batch.records.size() - record_begin);
    batch.segments.push_back(segment);
  }

  batch.segment_count = batch.segments.size();
  batch.record_count = batch.records.size();
  batch.host_to_device_query_byte_count =
      queries.size() *
      sizeof(Phase14AnchoredPairCandidateQueryInputRecord);
  batch.initialized_segment_byte_count =
      batch.segments.size() *
      sizeof(Phase14AnchoredPairCandidateSegmentRecord);
  batch.initialized_transcript_byte_count =
      active_total_transcript_record_limit *
      sizeof(Phase14AnchoredPairCandidateTranscriptRecord);
  batch.device_to_host_segment_byte_count =
      batch.initialized_segment_byte_count;
  batch.device_to_host_transcript_byte_count =
      batch.initialized_transcript_byte_count;

  switch (config.corruption) {
    case FakeAnchoredPairProposalCorruption::none:
    case FakeAnchoredPairProposalCorruption::wrong_digest:
      break;
    case FakeAnchoredPairProposalCorruption::wrong_source_epoch:
      ++batch.source_snapshot_epoch;
      break;
    case FakeAnchoredPairProposalCorruption::broken_segment_partition:
      ++batch.segments.front().record_begin;
      break;
    case FakeAnchoredPairProposalCorruption::out_of_domain_node:
      if (batch.records.empty()) {
        throw std::logic_error("the fake transcript has no record to forge");
      }
      batch.records.front().node_index = as_u64(traversal.node_count);
      break;
    case FakeAnchoredPairProposalCorruption::wrong_prune_cardinality:
      if (batch.records.size() < 2U) {
        throw std::logic_error("the fake transcript has no prune to forge");
      }
      batch.records[1U].witness_mask = UINT64_C(1);
      break;
    case FakeAnchoredPairProposalCorruption::cuda_metadata_from_host_fake:
      batch.cuda_device = 0;
      batch.cuda_path_qualified = true;
      batch.kernel_launch_count = 1U;
      batch.synchronization_count = 1U;
      break;
    case FakeAnchoredPairProposalCorruption::wrong_capacity_echo:
      ++batch.physical_transcript_record_capacity;
      break;
  }
  batch.proposal_digest_fnv1a =
      phase14_anchored_pair_candidate_batch_digest(batch);
  if (config.corruption ==
      FakeAnchoredPairProposalCorruption::wrong_digest) {
    batch.proposal_digest_fnv1a ^= UINT64_C(1);
  }
  return batch;
}

}  // namespace morsehgp3d::gpu::detail

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::AnchoredPairCandidateProposalBatchResult;
using morsehgp3d::gpu::AnchoredPairCandidateProposalBudget;
using morsehgp3d::gpu::AnchoredPairCandidateProposalContext;
using morsehgp3d::gpu::AnchoredPairCandidateProposalCursor;
using morsehgp3d::gpu::AnchoredPairCandidateProposalCursorKind;
using morsehgp3d::gpu::AnchoredPairCandidateProposalQuery;
using morsehgp3d::gpu::AnchoredPairCandidateProposalRecord;
using morsehgp3d::gpu::AnchoredPairCandidateProposalSegmentStatus;
using morsehgp3d::gpu::AnchoredPairCandidateProposalStopReason;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::anchored_pair_candidate_gpu_invalid_node_index;
using morsehgp3d::gpu::test_support::
    FakeAnchoredPairProposalBehavior;
using morsehgp3d::gpu::test_support::
    FakeAnchoredPairProposalConfiguration;
using morsehgp3d::gpu::test_support::
    FakeAnchoredPairProposalCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_anchored_pair_proposal;
using morsehgp3d::gpu::test_support::
    fake_anchored_pair_proposal_call_count;
using morsehgp3d::gpu::test_support::
    reset_fake_anchored_pair_proposal;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<AnchoredPairCandidateProposalContext>);
static_assert(
    !std::is_copy_assignable_v<AnchoredPairCandidateProposalContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        AnchoredPairCandidateProposalContext>);
static_assert(
    std::is_nothrow_move_assignable_v<
        AnchoredPairCandidateProposalContext>);
static_assert(sizeof(AnchoredPairCandidateProposalRecord) == 16U);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud proposal_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(70U);
  for (std::size_t index = 0U; index < 70U; ++index) {
    const double x = static_cast<double>(index);
    points.push_back(point(x, x * x + 0.25 * x, -0.5 * x));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::vector<PointId> prefix_witnesses(
    std::size_t count) {
  std::vector<PointId> witnesses;
  witnesses.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    witnesses.push_back(static_cast<PointId>(index));
  }
  return witnesses;
}

[[nodiscard]] AnchoredPairCandidateProposalContext make_context(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_query_count = 4U,
    std::size_t maximum_record_count = 32U) {
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  AnchoredPairCandidateProposalContext context{
      cloud,
      std::move(lease),
      maximum_query_count,
      maximum_record_count};
  check(
      !lease.ready(),
      "the proposal context consumes and neutralizes the traversal lease");
  return context;
}

[[nodiscard]] AnchoredPairCandidateProposalBudget standard_budget() {
  return AnchoredPairCandidateProposalBudget{8U, 4U, 4U};
}

void check_non_scientific_audit(
    const AnchoredPairCandidateProposalBatchResult& result,
    const std::string& label) {
  check(
      result.validated_proposal_batch() && !result.scientific_outcome(),
      label + " is a validated proposal batch, not a scientific outcome");
  check(
      !result.audit.candidate_leaf_status_recertified &&
          !result.audit.strict_witness_masks_recertified &&
          !result.audit.exact_closed_ball_partition_published &&
          !result.audit.scientific_decision_published &&
          !result.audit.hierarchy_reduction_or_attachment_published &&
          !result.audit.forbidden_global_structure_materialized &&
          !result.audit.transcript_accumulated_across_calls &&
          !result.audit.public_status_claimed,
      label + " cannot bypass the future exact CPU recertifier");
}

void test_valid_complete_empty_and_no_accumulation() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_anchored_pair_proposal();
  const CanonicalPointCloud cloud = proposal_cloud();
  AnchoredPairCandidateProposalContext context = make_context(cloud);
  std::vector<PointId> witnesses = prefix_witnesses(64U);
  std::rotate(
      witnesses.begin(), witnesses.begin() + 1, witnesses.end());
  check(
      witnesses.front() > witnesses.back(),
      "the valid bank fixture is deliberately not PointId-sorted");
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{
      AnchoredPairCandidateProposalQuery{
          static_cast<PointId>(69U),
          11U,
          std::span<const PointId>{witnesses},
          {}}};

  const auto first = context.build(cloud, queries, standard_budget());
  check_non_scientific_audit(first, "the complete 64-witness transcript");
  check(
      first.segments.size() == 1U && first.records.size() == 2U &&
          first.segments.front().status ==
              AnchoredPairCandidateProposalSegmentStatus::complete &&
          first.segments.front().stop_reason ==
              AnchoredPairCandidateProposalStopReason::none &&
          first.segments.front().next_cursor.kind ==
              AnchoredPairCandidateProposalCursorKind::complete &&
          first.records[0U].witness_mask == 0U &&
          first.records[1U].witness_mask == UINT64_C(0x3ff),
      "mask zero denotes a candidate and the rank-11 prune has ten bits");
  check(
      first.audit.maximum_observed_witness_bank_size == 64U &&
          first.audit.physical_transcript_record_capacity == 32U &&
          first.audit.active_total_transcript_record_limit == 4U &&
          first.audit.compacted_host_transcript_record_count == 2U &&
          first.audit.initialized_device_transcript_byte_count == 64U &&
          first.audit.copied_device_to_host_transcript_byte_count == 64U &&
          first.audit.candidate_leaf_proposal_count == 1U &&
          first.audit.prune_proposal_count == 1U &&
          first.audit.matching_immutable_point_namespace_validated &&
          first.audit.traversal_lease_owner_retained &&
          first.audit.traversal_lease_device_validated &&
          first.audit.fixed_resident_capacity_bound_at_construction &&
          first.audit.proposal_digest_fnv1a != 0U,
      "the valid transcript closes ownership, mask and digest audits");

  const auto second = context.build(cloud, queries, standard_budget());
  check(
      second.records.size() == first.records.size() &&
          second.audit.buffer_epoch == first.audit.buffer_epoch + 1U &&
          !second.audit.transcript_accumulated_across_calls,
      "successive calls return only their current bounded transcript");

  const std::size_t calls_before_empty =
      fake_anchored_pair_proposal_call_count();
  const std::span<const AnchoredPairCandidateProposalQuery> empty_queries;
  const auto empty = context.build(
      cloud, empty_queries, standard_budget());
  check_non_scientific_audit(empty, "the empty proposal batch");
  check(
      empty.segments.empty() && empty.records.empty() &&
          fake_anchored_pair_proposal_call_count() == calls_before_empty,
      "an empty batch is certified without launching or allocating output");
}

void test_resume_cursor_and_typed_limits() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_anchored_pair_proposal();
  const CanonicalPointCloud cloud = proposal_cloud();
  AnchoredPairCandidateProposalContext context = make_context(cloud);
  const std::vector<PointId> witnesses = prefix_witnesses(10U);
  AnchoredPairCandidateProposalQuery query{
      static_cast<PointId>(69U),
      10U,
      std::span<const PointId>{witnesses},
      {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      initial_queries{query};

  configure_fake_anchored_pair_proposal({
      FakeAnchoredPairProposalBehavior::budget_exhausted,
      FakeAnchoredPairProposalCorruption::none});
  const AnchoredPairCandidateProposalBudget budget_limited{3U, 4U, 4U};
  const auto limited = context.build(
      cloud, initial_queries, budget_limited);
  check(
      limited.segments.front().status ==
              AnchoredPairCandidateProposalSegmentStatus::budget_exhausted &&
          limited.segments.front().stop_reason ==
              AnchoredPairCandidateProposalStopReason::node_visit_limit &&
          limited.segments.front().node_visit_count == 3U &&
          limited.segments.front().next_cursor.kind ==
              AnchoredPairCandidateProposalCursorKind::active,
      "node-budget exhaustion returns an authenticated active cursor");

  configure_fake_anchored_pair_proposal({});
  query.cursor = limited.segments.front().next_cursor;
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      resumed_queries{query};
  const auto resumed = context.build(
      cloud, resumed_queries, standard_budget());
  check(
      resumed.segments.front().input_cursor == query.cursor &&
          resumed.segments.front().status ==
              AnchoredPairCandidateProposalSegmentStatus::complete,
      "the same digest and snapshot epoch resume the stackless traversal");

  configure_fake_anchored_pair_proposal({
      FakeAnchoredPairProposalBehavior::capacity_exhausted,
      FakeAnchoredPairProposalCorruption::none});
  query.cursor = AnchoredPairCandidateProposalCursor{};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      capacity_queries{query};
  const AnchoredPairCandidateProposalBudget capacity_limited{4U, 2U, 2U};
  const auto capacity = context.build(
      cloud, capacity_queries, capacity_limited);
  check(
      capacity.records.size() == 2U &&
          capacity.segments.front().status ==
              AnchoredPairCandidateProposalSegmentStatus::capacity_exhausted &&
          capacity.segments.front().stop_reason ==
              AnchoredPairCandidateProposalStopReason::
                  transcript_record_limit &&
          capacity.segments.front().next_cursor.kind ==
              AnchoredPairCandidateProposalCursorKind::active,
      "record-capacity exhaustion is distinct and exactly reaches its cap");
}

void test_input_namespace_and_move_guards() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_anchored_pair_proposal();
  const CanonicalPointCloud cloud = proposal_cloud();
  const CanonicalPointCloud foreign_cloud = proposal_cloud();
  AnchoredPairCandidateProposalContext context = make_context(cloud);
  const std::vector<PointId> witnesses = prefix_witnesses(10U);
  AnchoredPairCandidateProposalQuery query{
      static_cast<PointId>(69U),
      10U,
      std::span<const PointId>{witnesses},
      {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{query};

  check_throws<std::invalid_argument>(
      [&context, &foreign_cloud, &queries]() {
        static_cast<void>(context.build(
            foreign_cloud, queries, standard_budget()));
      },
      "geometrically equal but foreign PointIds cannot reuse the lease");

  std::vector<PointId> too_many = prefix_witnesses(65U);
  query.witness_bank_point_ids = std::span<const PointId>{too_many};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      oversized_queries{query};
  check_throws<std::length_error>(
      [&context, &cloud, &oversized_queries]() {
        static_cast<void>(context.build(
            cloud, oversized_queries, standard_budget()));
      },
      "a witness bank larger than one uint64 mask is rejected");

  std::array<PointId, 2U> duplicate_witnesses{
      static_cast<PointId>(1U), static_cast<PointId>(1U)};
  query.witness_bank_point_ids =
      std::span<const PointId>{duplicate_witnesses};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      duplicate_queries{query};
  check_throws<std::invalid_argument>(
      [&context, &cloud, &duplicate_queries]() {
        static_cast<void>(context.build(
            cloud, duplicate_queries, standard_budget()));
      },
      "duplicate witness PointIds are rejected before launch");

  query.witness_bank_point_ids = std::span<const PointId>{witnesses};
  query.cursor = AnchoredPairCandidateProposalCursor{
      AnchoredPairCandidateProposalCursorKind::active,
      0U,
      UINT64_C(7),
      UINT64_C(9)};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      stale_queries{query};
  check_throws<std::invalid_argument>(
      [&context, &cloud, &stale_queries]() {
        static_cast<void>(context.build(
            cloud, stale_queries, standard_budget()));
      },
      "a stale or foreign active cursor is rejected before launch");

  AnchoredPairCandidateProposalContext moved = std::move(context);
  check(
      context.point_count() == 0U && context.certified_node_count() == 0U &&
          context.maximum_query_count() == 0U &&
          context.maximum_transcript_record_count() == 0U,
      "a moved-from proposal context exposes neutral capacities");
  check_throws<std::logic_error>(
      [&context, &cloud, &queries]() {
        static_cast<void>(context.build(
            cloud, queries, standard_budget()));
      },
      "a moved-from proposal context cannot launch");
  const auto valid = moved.build(cloud, queries, standard_budget());
  check_non_scientific_audit(valid, "the moved-to proposal context");
}

void test_hostile_transcripts_poison_contexts() {
  const CanonicalPointCloud cloud = proposal_cloud();
  const std::vector<PointId> witnesses = prefix_witnesses(10U);
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{
      AnchoredPairCandidateProposalQuery{
          static_cast<PointId>(69U),
          10U,
          std::span<const PointId>{witnesses},
          {}}};

  const std::array<FakeAnchoredPairProposalCorruption, 7U> corruptions{
      FakeAnchoredPairProposalCorruption::wrong_digest,
      FakeAnchoredPairProposalCorruption::wrong_source_epoch,
      FakeAnchoredPairProposalCorruption::broken_segment_partition,
      FakeAnchoredPairProposalCorruption::out_of_domain_node,
      FakeAnchoredPairProposalCorruption::wrong_prune_cardinality,
      FakeAnchoredPairProposalCorruption::cuda_metadata_from_host_fake,
      FakeAnchoredPairProposalCorruption::wrong_capacity_echo};
  for (const FakeAnchoredPairProposalCorruption corruption : corruptions) {
    reset_fake_gpu_phase14_morton_lbvh_build();
    reset_fake_anchored_pair_proposal();
    AnchoredPairCandidateProposalContext context = make_context(cloud);
    configure_fake_anchored_pair_proposal({
        FakeAnchoredPairProposalBehavior::complete, corruption});
    check_throws<std::runtime_error>(
        [&context, &cloud, &queries]() {
          static_cast<void>(context.build(
              cloud, queries, standard_budget()));
        },
        "host validation rejects a forged proposal transcript");
    configure_fake_anchored_pair_proposal({});
    check_throws<std::runtime_error>(
        [&context, &cloud, &queries]() {
          static_cast<void>(context.build(
              cloud, queries, standard_budget()));
        },
        "a hostile launcher result permanently poisons its context");
  }
}

}  // namespace

int main() {
  test_valid_complete_empty_and_no_accumulation();
  test_resume_cursor_and_typed_limits();
  test_input_namespace_and_move_guards();
  test_hostile_transcripts_poison_contexts();
  if (failures != 0) {
    std::cerr << failures
              << " anchored-pair proposal contract check(s) failed\n";
    return 1;
  }
  std::cout << "anchored-pair proposal host contract checks passed\n";
  return 0;
}
