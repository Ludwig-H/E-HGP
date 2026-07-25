#include "fake_gpu_phase14_facet_top_k_proposal_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "exact_center_binary64_projection.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/direct_sparse_facet_top_k_integrated_adapter.hpp"
#include "morsehgp3d/gpu/direct_sparse_facet_top_k_proposal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalBatchResult;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalContext;
using morsehgp3d::gpu::DirectSparseFacetTopKIntegratedAdapter;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalPolicy;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalQuery;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceLease;
using morsehgp3d::gpu::test_support::
    FakePhase14FacetTopKProposalConfiguration;
using morsehgp3d::gpu::test_support::
    FakePhase14FacetTopKProposalCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_phase14_facet_top_k_proposal;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_facet_top_k_proposal_epoch_advance_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_facet_top_k_proposal_last_query_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_facet_top_k_proposal_last_record_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_facet_top_k_proposal_last_window_radius;
using morsehgp3d::gpu::test_support::
    fake_gpu_phase14_facet_top_k_proposal_launch_count;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_facet_top_k_proposal;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::hierarchy::ExactDirectSparseFacetKey;
using morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptBudget;
using morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptDecision;
using morsehgp3d::hierarchy::
    ExactDirectSparseFacetTopKProposalTranscriptMetadata;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<
        DirectSparseFacetTopKProposalContext>);
static_assert(
    !std::is_copy_assignable_v<
        DirectSparseFacetTopKProposalContext>);
static_assert(
    std::is_nothrow_move_constructible_v<
        DirectSparseFacetTopKProposalContext>);
static_assert(
    std::is_nothrow_move_assignable_v<
        DirectSparseFacetTopKProposalContext>);
static_assert(
    !std::is_constructible_v<
        DirectSparseFacetTopKIntegratedAdapter,
        DirectSparseFacetTopKProposalContext&,
        CanonicalPointCloud&&,
        DirectSparseFacetTopKProposalPolicy,
        ExactDirectSparseFacetTopKProposalTranscriptBudget>);

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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

struct LegacyNearestProjection {
  std::uint64_t bits{};
  bool supported{true};
};

[[nodiscard]] LegacyNearestProjection legacy_nearest_binary64(
    const ExactRational& value) {
  const auto enclosure =
      morsehgp3d::gpu::detail::enclose_rational(value);
  if (enclosure.status ==
      morsehgp3d::gpu::DirectedEnclosureStatus::unsupported_range) {
    return LegacyNearestProjection{0U, false};
  }
  if (enclosure.lower_bits == enclosure.upper_bits) {
    return LegacyNearestProjection{enclosure.lower_bits, true};
  }
  const ExactRational lower =
      ExactRational::from_binary64_bits(enclosure.lower_bits);
  const ExactRational upper =
      ExactRational::from_binary64_bits(enclosure.upper_bits);
  return LegacyNearestProjection{
      value - lower <= upper - value
          ? enclosure.lower_bits
          : enclosure.upper_bits,
      true};
}

void check_direct_projection(
    const ExactRational& value,
    const std::string& label) {
  const ExactCenter3 center{
      value.numerator(),
      morsehgp3d::exact::BigInt{0},
      morsehgp3d::exact::BigInt{0},
      value.denominator()};
  const auto direct =
      morsehgp3d::gpu::detail::
          project_exact_center_to_nearest_binary64(center);
  const LegacyNearestProjection legacy =
      legacy_nearest_binary64(value);
  check(
      direct.supported == legacy.supported,
      label + " preserves the legacy supported-range decision");
  if (legacy.supported) {
    check(
        direct.coordinate_bits[0U] == legacy.bits,
        label + " preserves the legacy nearest binary64 word");
  }
  check(
      direct.coordinate_bits[1U] == 0U &&
          direct.coordinate_bits[2U] == 0U,
      label + " keeps exact zero coordinates canonical");
  check(
      direct.integer_division_count <= 1U,
      label + " uses at most one integer division for one nonzero axis");
}

void test_direct_integer_binary64_projection() {
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::power_of_two;

  const ExactRational zero{};
  const ExactRational one{BigInt{1}};
  const ExactRational one_third{BigInt{1}, BigInt{3}};
  const ExactRational minimum_subnormal{
      BigInt{1}, power_of_two(1074U)};
  const ExactRational half_minimum_subnormal{
      BigInt{1}, power_of_two(1075U)};
  const ExactRational minimum_normal{
      BigInt{1}, power_of_two(1022U)};
  const ExactRational maximum_finite =
      ExactRational::from_binary64_bits(
          UINT64_C(0x7fefffffffffffff));
  const ExactRational above_maximum =
      maximum_finite + one;

  const std::array boundaries{
      zero,
      one_third,
      minimum_subnormal,
      half_minimum_subnormal,
      minimum_normal,
      one,
      maximum_finite,
      above_maximum};
  for (std::size_t index = 0U;
       index < boundaries.size();
       ++index) {
    check_direct_projection(
        boundaries[index],
        "direct projection boundary " + std::to_string(index));
    if (!boundaries[index].is_zero()) {
      check_direct_projection(
          -boundaries[index],
          "negative direct projection boundary " +
              std::to_string(index));
    }
  }

  const std::array adjacent_lower_bits{
      UINT64_C(0x0000000000000000),
      UINT64_C(0x000fffffffffffff),
      UINT64_C(0x3fefffffffffffff),
      UINT64_C(0x3ff0000000000000),
      UINT64_C(0x400fffffffffffff),
      UINT64_C(0x7feffffffffffffe)};
  for (const std::uint64_t lower_bits : adjacent_lower_bits) {
    const ExactRational lower =
        ExactRational::from_binary64_bits(lower_bits);
    const ExactRational upper =
        ExactRational::from_binary64_bits(lower_bits + 1U);
    const ExactRational midpoint =
        (lower + upper) / ExactRational{BigInt{2}};
    check_direct_projection(
        midpoint,
        "positive adjacent midpoint " + std::to_string(lower_bits));
    if (!midpoint.is_zero()) {
      check_direct_projection(
          -midpoint,
          "negative adjacent midpoint " +
              std::to_string(lower_bits));
    }
  }

  for (std::uint64_t sample = 1U; sample <= 64U; ++sample) {
    BigInt numerator =
        BigInt{sample * sample * UINT64_C(7919)} -
        BigInt{sample * UINT64_C(104729)};
    if (numerator == 0) {
      numerator = 1;
    }
    const BigInt denominator =
        power_of_two(
            static_cast<unsigned int>(sample % 80U)) +
        BigInt{2U * sample + 1U};
    check_direct_projection(
        ExactRational{std::move(numerator), denominator},
        "deterministic direct projection sample " +
            std::to_string(sample));
  }

  check(
      morsehgp3d::gpu::
              direct_sparse_facet_top_k_gpu_proposal_schema_version ==
          4U,
      "the direct integer projector advances the proposal schema");
}

[[nodiscard]] CertifiedPoint3 point(double x) {
  return CertifiedPoint3::from_binary64(x, 0.0, 0.0);
}

[[nodiscard]] CanonicalPointCloud make_cloud() {
  const std::array<CertifiedPoint3, 12U> points{
      point(-11.0),
      point(-9.0),
      point(-7.0),
      point(-5.0),
      point(-3.0),
      point(-1.0),
      point(1.0),
      point(3.0),
      point(5.0),
      point(7.0),
      point(9.0),
      point(11.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

struct Fixture {
  Fixture() : cloud(make_cloud()), index(MortonLbvhIndex::build(cloud)) {}

  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
};

[[nodiscard]] ExactDirectSparseFacetKey key(
    PointId first, PointId second) {
  ExactDirectSparseFacetKey result;
  result.point_count = 2U;
  result.point_ids[0U] = std::min(first, second);
  result.point_ids[1U] = std::max(first, second);
  return result;
}

[[nodiscard]] ExactCenter3 center_at(
    const CanonicalPointCloud& cloud, PointId point_id) {
  return cloud.point(point_id).exact();
}

[[nodiscard]] std::vector<DirectSparseFacetTopKProposalQuery>
canonical_queries(const Fixture& fixture) {
  const auto leaves = fixture.index.leaves();
  std::vector<DirectSparseFacetTopKProposalQuery> queries{
      {key(leaves[2U].point_id, leaves[3U].point_id),
       center_at(fixture.cloud, leaves[2U].point_id)},
      {key(leaves[7U].point_id, leaves[8U].point_id),
       center_at(fixture.cloud, leaves[7U].point_id)},
  };
  std::sort(
      queries.begin(),
      queries.end(),
      [](const DirectSparseFacetTopKProposalQuery& left,
         const DirectSparseFacetTopKProposalQuery& right) {
        return std::lexicographical_compare(
            left.source_facet_key.point_ids.begin(),
            left.source_facet_key.point_ids.begin() + 2,
            right.source_facet_key.point_ids.begin(),
            right.source_facet_key.point_ids.begin() + 2);
      });
  return queries;
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptMetadata
metadata() {
  ExactDirectSparseFacetTopKProposalTranscriptMetadata result;
  result.source_batch_index = 17U;
  result.locator_snapshot_stamp.schema_version =
      morsehgp3d::hierarchy::
          direct_sparse_positive_facet_locator_schema_version;
  result.locator_snapshot_stamp.external_authority_id = 91U;
  return result;
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptBudget
generous_budget() {
  const std::size_t maximum =
      std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] DirectSparseFacetTopKProposalBatchResult build(
    DirectSparseFacetTopKProposalContext& context,
    const CanonicalPointCloud& cloud,
    std::span<const DirectSparseFacetTopKProposalQuery> queries,
    std::size_t window_radius = 1U) {
  return context.build(
      cloud,
      metadata(),
      queries,
      DirectSparseFacetTopKProposalPolicy{window_radius},
      generous_budget());
}

void test_result_epochs_and_digest() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  Fixture fixture;
  const auto queries = canonical_queries(fixture);
  DirectSparseFacetTopKProposalContext context{
      fixture.index, fixture.cloud, 6U};

  const auto first = build(context, fixture.cloud, queries);
  const auto second = build(context, fixture.cloud, queries);

  check(
      first.complete_proposal_batch() &&
          second.complete_proposal_batch() &&
          first.certified_outcome() && second.certified_outcome(),
      "two healthy resident epochs close the proposal-only batch");
  check(
      first.transcript.decision ==
              ExactDirectSparseFacetTopKProposalTranscriptDecision::
                  complete_validated_proposal_transcript &&
          first.transcript.proposal_records.size() == queries.size() &&
          second.transcript == first.transcript,
      "the fake Morton producer yields the same owned 14F transcript twice");
  check(
      first.audit.buffer_epoch == 1U &&
          second.audit.buffer_epoch == 2U &&
          first.audit.proposal_digest_fnv1a != 0U &&
          first.audit.proposal_digest_fnv1a ==
              second.audit.proposal_digest_fnv1a,
      "epochs advance while the logical proposal digest stays deterministic");
  check(
      first.audit.snapshot_point_count == fixture.cloud.size() &&
          first.audit.static_device_coordinate_word_capacity ==
              3U * fixture.cloud.size() &&
          first.audit.static_device_morton_point_id_capacity ==
              fixture.cloud.size() &&
          first.audit.static_device_snapshot_byte_capacity ==
              4U * fixture.cloud.size() * sizeof(std::uint64_t) &&
          first.audit.snapshot_host_to_device_byte_count ==
              first.audit.static_device_snapshot_byte_capacity &&
          second.audit.snapshot_host_to_device_byte_count == 0U &&
          first.audit.host_snapshot_byte_capacity ==
              (4U * sizeof(std::uint64_t) + sizeof(std::size_t)) *
                  fixture.cloud.size() &&
          first.audit.maximum_query_count == 6U &&
          first.audit.physical_device_query_capacity == 6U &&
          first.audit.physical_device_record_capacity == 6U &&
          first.audit.static_device_query_buffer_byte_capacity ==
              6U * 26U * sizeof(std::uint64_t) &&
          first.audit.static_device_record_buffer_byte_capacity ==
              6U * 18U * sizeof(std::uint64_t) &&
          first.audit.host_record_copy_byte_capacity ==
              first.audit.static_device_record_buffer_byte_capacity &&
          first.audit.active_host_to_device_query_record_count == 2U &&
          first.audit.active_host_to_device_query_byte_count ==
              2U * 26U * sizeof(std::uint64_t) &&
          first.audit.initialized_device_output_record_count == 2U &&
          first.audit.initialized_device_output_byte_count ==
              2U * 18U * sizeof(std::uint64_t) &&
          first.audit.copied_device_to_host_record_count == 2U &&
          first.audit.copied_device_to_host_byte_count ==
              2U * 18U * sizeof(std::uint64_t) &&
          first.audit.exact_center_projection_axis_count == 6U &&
          first.audit.exact_center_projection_integer_division_count ==
              2U &&
          first.audit.exact_center_projection_zero_axis_count == 4U &&
          first.audit.exact_center_projection_unsupported_axis_count ==
              0U &&
          first.audit
              .exact_center_projection_division_bound_validated,
      "the audit exposes the lazy 32n device snapshot, full host snapshot and fixed batch capacities");
  auto undercounted_projection = first;
  --undercounted_projection.audit
        .exact_center_projection_integer_division_count;
  check(
      !undercounted_projection.certified_outcome(),
      "an undercounted integer projection partition is rejected");
  check(
      first.audit.canonical_query_count == 2U &&
          first.audit.gpu_supported_center_query_count == 2U &&
          first.audit.unsupported_center_query_count == 0U &&
          first.audit.gpu_output_record_count == 2U &&
          first.audit.nonempty_proposal_record_count == 2U &&
          first.audit.proposed_candidate_count == 4U &&
          first.audit.maximum_inspection_count_per_query == 4U &&
          first.audit.aggregate_inspection_count_upper_bound == 8U &&
          first.audit.inspected_neighbor_count == 8U &&
          first.audit.gpu_kernel_launch_count == 1U &&
          first.audit.gpu_synchronization_count == 1U,
      "the two-sided radius-one windows close their exact bounded work");
  check(
      first.audit.supported_query_permutation_validated &&
          first.audit
              .active_record_candidate_tail_sentinel_validated &&
          first.audit.every_candidate_domain_validated &&
          first.audit
              .every_candidate_source_facet_exclusion_validated &&
          first.audit.every_candidate_morton_window_validated &&
          first.audit.candidate_distinctness_validated &&
          first.audit.bounded_work_validated &&
          first.audit.transcript_builder_invoked &&
          first.audit.gpu_execution_performed &&
          first.audit.floating_ordering_only &&
          !first.audit.exact_distance_or_partition_published &&
          !first.audit.scientific_decision_published &&
          !first.audit.forbidden_global_structure_materialized,
      "the healthy result remains an authenticated proposal without scientific authority");
  for (std::size_t index = 0U;
       index < first.transcript.proposal_records.size();
       ++index) {
    const auto& record = first.transcript.proposal_records[index];
    check(
        record.source_facet_key == queries[index].source_facet_key &&
            record.candidate_point_count == 2U &&
            record.candidate_point_ids[0U] <
                record.candidate_point_ids[1U] &&
            !std::binary_search(
                record.source_facet_key.point_ids.begin(),
                record.source_facet_key.point_ids.begin() + 2,
                record.candidate_point_ids[0U]) &&
            !std::binary_search(
                record.source_facet_key.point_ids.begin(),
                record.source_facet_key.point_ids.begin() + 2,
                record.candidate_point_ids[1U]),
        "each transcript row contains two sorted external Morton-window candidates");
  }
  check(
      fake_gpu_phase14_facet_top_k_proposal_launch_count() == 2U &&
          fake_gpu_phase14_facet_top_k_proposal_epoch_advance_count() ==
              2U &&
          fake_gpu_phase14_facet_top_k_proposal_last_query_count() == 2U &&
          fake_gpu_phase14_facet_top_k_proposal_last_record_capacity() ==
              6U &&
          fake_gpu_phase14_facet_top_k_proposal_last_window_radius() ==
              1U,
      "the fake launcher observes one bounded physical transaction per epoch");
}

void test_phase14n_snapshot_adoption() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  reset_fake_gpu_phase14_morton_lbvh_build();
  Fixture fixture;
  const auto queries = canonical_queries(fixture);

  MortonLbvhBuildContext builder{fixture.cloud.size()};
  auto built = builder.build(fixture.cloud);
  check(
      built.complete_certified_build(),
      "the fake 14M builder certifies the adoption fixture");
  MortonLbvhDeviceLease lease =
      builder.release_device_lease(built);
  check(
      lease.ready() && !lease.cuda_resident(),
      "the host fake exposes a ready non-CUDA lifecycle lease");

  DirectSparseFacetTopKProposalContext legacy{
      built.certified_index(), fixture.cloud, 6U};
  DirectSparseFacetTopKProposalContext adopted{
      built.certified_index(),
      fixture.cloud,
      std::move(lease),
      6U};
  check(
      !lease.ready(),
      "constructing the adopted context consumes the move-only lease once");
  auto rebuilt = builder.build(fixture.cloud);
  auto rebuilt_lease = builder.release_device_lease(rebuilt);
  check(
      rebuilt.complete_certified_build() && rebuilt_lease.ready() &&
          rebuilt.audit().snapshot_buffer_epoch >
              built.audit().snapshot_buffer_epoch,
      "the detached 14M builder lazily rebuilds after lease adoption");

  const auto legacy_first =
      build(legacy, fixture.cloud, queries);
  const auto adopted_first =
      build(adopted, fixture.cloud, queries);
  const auto adopted_second =
      build(adopted, fixture.cloud, queries);
  check(
      legacy_first.transcript == adopted_first.transcript &&
          adopted_second.transcript == adopted_first.transcript &&
          legacy_first.audit.proposal_digest_fnv1a ==
              adopted_first.audit.proposal_digest_fnv1a,
      "legacy copy and 14N adoption produce the same proposal transcript");
  const std::size_t point_count = fixture.cloud.size();
  const std::size_t device_snapshot_bytes =
      4U * point_count * sizeof(std::uint64_t);
  check(
      !legacy_first.audit
           .device_snapshot_adopted_from_morton_lbvh_lease &&
          !legacy_first.audit.adopted_device_snapshot_owner_retained &&
          legacy_first.audit.host_coordinate_snapshot_word_count ==
              3U * point_count &&
          legacy_first.audit.host_morton_order_snapshot_entry_count ==
              point_count &&
          legacy_first.audit.host_snapshot_byte_capacity ==
              device_snapshot_bytes +
                  point_count * sizeof(std::size_t) &&
          legacy_first.audit.snapshot_host_to_device_byte_count ==
              device_snapshot_bytes,
      "the legacy path keeps and uploads its explicit 40n host snapshot");
  check(
      adopted_first.audit
              .device_snapshot_adopted_from_morton_lbvh_lease &&
          adopted_first.audit.adopted_device_snapshot_owner_retained &&
          adopted_first.audit.source_morton_lbvh_lease_epoch ==
              built.audit().snapshot_buffer_epoch &&
          adopted_first.audit.host_coordinate_snapshot_word_count == 0U &&
          adopted_first.audit.host_morton_order_snapshot_entry_count ==
              0U &&
          adopted_first.audit.host_inverse_morton_entry_count ==
              point_count &&
          adopted_first.audit.host_snapshot_byte_capacity ==
              point_count * sizeof(std::size_t) &&
          adopted_first.audit.snapshot_host_to_device_byte_count == 0U &&
          adopted_second.audit.snapshot_host_to_device_byte_count == 0U,
      "the adopted path retains only the 8n inverse and performs zero "
      "snapshot H2D traffic");

  std::unique_ptr<DirectSparseFacetTopKProposalContext>
      context_outliving_index;
  {
    MortonLbvhBuildContext short_builder{point_count};
    auto short_build = short_builder.build(fixture.cloud);
    auto short_lease =
        short_builder.release_device_lease(short_build);
    context_outliving_index =
        std::make_unique<DirectSparseFacetTopKProposalContext>(
            short_build.certified_index(),
            fixture.cloud,
            std::move(short_lease),
            4U);
  }
  const auto after_index_destruction =
      build(*context_outliving_index, fixture.cloud, queries);
  check(
      after_index_destruction.complete_proposal_batch() &&
          after_index_destruction.audit
              .device_snapshot_adopted_from_morton_lbvh_lease,
      "the adopted context owns its inverse and does not borrow the 14M "
      "index lifetime");
}

void test_phase14o_integrated_callback_adapter() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  reset_fake_gpu_phase14_morton_lbvh_build();
  Fixture fixture;
  const auto source_queries = canonical_queries(fixture);

  MortonLbvhBuildContext builder{fixture.cloud.size()};
  auto built = builder.build(fixture.cloud);
  auto lease = builder.release_device_lease(built);
  DirectSparseFacetTopKProposalContext context{
      built.certified_index(),
      fixture.cloud,
      std::move(lease),
      2U};
  DirectSparseFacetTopKIntegratedAdapter adapter{
      context,
      fixture.cloud,
      DirectSparseFacetTopKProposalPolicy{1U},
      generous_budget()};

  std::vector<morsehgp3d::hierarchy::
                  ExactDirectSparseFacetDescentBatchPreparedProposalQuery>
      prepared_queries;
  prepared_queries.reserve(source_queries.size());
  for (const auto& source : source_queries) {
    prepared_queries.push_back(
        {source.source_facet_key,
         source.query_center,
         morsehgp3d::exact::ExactLevel{},
         1U,
         true});
  }
  const morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentBatchRunNextPrepareInputs inputs{
          metadata(),
          prepared_queries,
          2U,
          prepared_queries.size(),
          0U,
          0U,
          1U,
          2U,
          true};
  auto prepared = adapter.prepare_inputs(inputs);
  check(
      prepared.has_value() &&
          prepared->proposal_records.size() ==
              source_queries.size() &&
          prepared->gpu_supported_query_count ==
              source_queries.size() &&
          prepared->active_host_to_device_query_byte_count ==
              source_queries.size() * 26U *
                  sizeof(std::uint64_t) &&
          prepared->copied_device_to_host_byte_count ==
              source_queries.size() * 18U *
                  sizeof(std::uint64_t) &&
          prepared->gpu_execution_performed &&
          prepared->proposal_only &&
          !prepared->exact_or_scientific_decision_published,
      "the 14O adapter maps one certified 14L chunk to active CUDA "
      "proposal traffic only");

  if (prepared.has_value()) {
    const morsehgp3d::hierarchy::
        ExactDirectSparseFacetDescentBatchRunNextSealInputs seal{
            inputs.metadata,
            prepared->proposal_records,
            generous_budget(),
            inputs.selected_arm_seed_count,
            inputs.distinct_source_facet_key_count,
            prepared_queries.size(),
            1U,
            true};
    const auto transcript = adapter.seal_inputs(seal);
    check(
        transcript.has_value() &&
            transcript->complete_proposal_transcript() &&
            transcript->proposal_records ==
                prepared->proposal_records,
        "the adapter delegates aggregate sealing to the unchanged 14F "
        "transcript builder");
  }

  const auto& audit = adapter.audit();
  check(
      audit.prepared_chunk_count == 1U &&
          audit.supported_query_count == source_queries.size() &&
          audit.snapshot_host_to_device_byte_count == 0U &&
          audit.host_snapshot_byte_capacity ==
              fixture.cloud.size() * sizeof(std::size_t) &&
          audit.persistent_device_snapshot_byte_capacity ==
              4U * fixture.cloud.size() *
                  sizeof(std::uint64_t) &&
          audit.source_morton_lbvh_lease_epoch ==
              built.audit().snapshot_buffer_epoch &&
          audit.every_chunk_used_adopted_device_snapshot &&
          audit.every_chunk_retained_snapshot_owner &&
          audit.every_chunk_was_proposal_only &&
          audit.aggregate_transcript_sealed &&
          !audit.forbidden_global_structure_materialized &&
          !audit.public_status_claimed,
      "the integrated adapter preserves the 14O adopted-owner and zero "
      "snapshot-H2D evidence across 14L callbacks");

  auto empty_metadata = metadata();
  ++empty_metadata.source_batch_index;
  using ProposalRecord =
      morsehgp3d::hierarchy::
          ExactDirectSparseFacetTopKProposalRecord;
  using SealInputs =
      morsehgp3d::hierarchy::
          ExactDirectSparseFacetDescentBatchRunNextSealInputs;
  const SealInputs rejected_empty{
      empty_metadata,
      std::span<const ProposalRecord>{},
      generous_budget(),
      0U,
      0U,
      0U,
      0U,
      false};
  check(
      !adapter.seal_inputs(rejected_empty).has_value() &&
          adapter.audit().prepared_chunk_count == 1U,
      "an invalid empty-batch seal is rejected without destroying the "
      "previous completed audit");
  SealInputs retried_empty = rejected_empty;
  retried_empty.every_prepare_inputs_chunk_certified = true;
  const auto empty_transcript =
      adapter.seal_inputs(retried_empty);
  check(
      empty_transcript.has_value() &&
          empty_transcript->complete_proposal_transcript() &&
          empty_transcript->proposal_records.empty() &&
          adapter.audit().prepared_chunk_count == 0U &&
          adapter.audit().supported_query_count == 0U &&
          adapter.audit().aggregate_transcript_sealed,
      "a valid empty batch after a nonempty batch resets stale chunk audit "
      "state and succeeds on retry");

  DirectSparseFacetTopKProposalContext legacy_context{
      fixture.index, fixture.cloud, 2U};
  check(
      context.device_snapshot_adopted_from_morton_lbvh_lease() &&
          !legacy_context
               .device_snapshot_adopted_from_morton_lbvh_lease(),
      "the proposal context exposes only the immutable 14O adoption fact");
  check_throws<std::invalid_argument>(
      [&] {
        DirectSparseFacetTopKIntegratedAdapter rejected{
            legacy_context,
            fixture.cloud,
            DirectSparseFacetTopKProposalPolicy{1U},
            generous_budget()};
      },
      "the 14P adapter rejects a legacy snapshot context before callbacks "
      "or ticket issuance");
}

void test_phase14n_adoption_rejections_are_atomic() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  reset_fake_gpu_phase14_morton_lbvh_build();
  Fixture fixture;

  {
    MortonLbvhBuildContext builder{fixture.cloud.size()};
    auto built = builder.build(fixture.cloud);
    auto lease = builder.release_device_lease(built);
    const CanonicalPointCloud foreign_cloud = make_cloud();
    const MortonLbvhIndex foreign_index =
        MortonLbvhIndex::build(foreign_cloud);
    check_throws<std::invalid_argument>(
        [&] {
          DirectSparseFacetTopKProposalContext rejected{
              foreign_index,
              foreign_cloud,
              std::move(lease),
              2U};
        },
        "an adopted lease rejects a foreign immutable PointId namespace");
    check(
        lease.ready(),
        "a foreign-namespace rejection does not consume the lease");
  }

  {
    MortonLbvhBuildContext oversized{
        fixture.cloud.size() + 1U};
    auto built = oversized.build(fixture.cloud);
    auto lease = oversized.release_device_lease(built);
    check_throws<std::invalid_argument>(
        [&] {
          DirectSparseFacetTopKProposalContext rejected{
              built.certified_index(),
              fixture.cloud,
              std::move(lease),
              2U};
        },
        "14O rejects a lease whose maximum capacity exceeds point_count");
    check(
        lease.ready(),
        "an exact-capacity rejection does not consume the lease");
  }

  {
    MortonLbvhBuildContext builder{fixture.cloud.size()};
    auto built = builder.build(fixture.cloud);
    auto source = builder.release_device_lease(built);
    MortonLbvhDeviceLease moved{std::move(source)};
    check(
        !source.ready() && moved.ready(),
        "moving a 14N lease transfers its one-shot adoption capability");
    check_throws<std::invalid_argument>(
        [&] {
          DirectSparseFacetTopKProposalContext rejected{
              built.certified_index(),
              fixture.cloud,
              std::move(source),
              2U};
        },
        "14O rejects a moved-from lease");
    DirectSparseFacetTopKProposalContext accepted{
        built.certified_index(),
        fixture.cloud,
        std::move(moved),
        2U};
    check(
        !moved.ready() &&
            accepted.point_count() == fixture.cloud.size(),
        "the moved-to lease is consumed by exactly one context");
  }
}

void test_empty_batch_has_no_launch_or_epoch() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  Fixture fixture;
  DirectSparseFacetTopKProposalContext context{
      fixture.index, fixture.cloud, 4U};
  const std::array<DirectSparseFacetTopKProposalQuery, 0U> empty{};

  const auto result = build(context, fixture.cloud, empty);
  check(
      result.complete_proposal_batch() &&
          result.transcript.decision ==
              ExactDirectSparseFacetTopKProposalTranscriptDecision::
                  complete_empty_proposal_transcript &&
          result.transcript.proposal_records.empty(),
      "an empty query batch produces the complete empty 14F transcript");
  check(
      result.audit.canonical_query_count == 0U &&
          result.audit.gpu_supported_center_query_count == 0U &&
          result.audit.gpu_output_record_count == 0U &&
          result.audit.active_host_to_device_query_record_count == 0U &&
          result.audit.active_host_to_device_query_byte_count == 0U &&
          result.audit.initialized_device_output_record_count == 0U &&
          result.audit.initialized_device_output_byte_count == 0U &&
          result.audit.copied_device_to_host_record_count == 0U &&
          result.audit.copied_device_to_host_byte_count == 0U &&
          result.audit.inspected_neighbor_count == 0U &&
          result.audit.gpu_kernel_launch_count == 0U &&
          result.audit.gpu_synchronization_count == 0U &&
          result.audit.buffer_epoch == 0U &&
          !result.audit.gpu_execution_performed &&
          fake_gpu_phase14_facet_top_k_proposal_launch_count() == 0U &&
          fake_gpu_phase14_facet_top_k_proposal_epoch_advance_count() ==
              0U,
      "an empty batch touches neither device nor epoch");

  const auto queries = canonical_queries(fixture);
  const auto nonempty = build(context, fixture.cloud, queries);
  check(
      nonempty.audit.buffer_epoch == 1U,
      "the first nonempty transaction after an empty batch owns epoch one");
}

void test_unsupported_centers_keep_the_supported_subset_sparse() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  Fixture fixture;
  const auto canonical = canonical_queries(fixture);
  const ExactCenter3 unsupported_center{
      morsehgp3d::exact::BigInt{1} << 1024U,
      morsehgp3d::exact::BigInt{0},
      morsehgp3d::exact::BigInt{0},
      morsehgp3d::exact::BigInt{1}};
  DirectSparseFacetTopKProposalContext context{
      fixture.index, fixture.cloud, 4U};

  auto unsupported = canonical;
  unsupported[0U].query_center = unsupported_center;
  unsupported[1U].query_center = unsupported_center;
  const auto empty_fallback =
      build(context, fixture.cloud, unsupported);
  check(
      empty_fallback.complete_proposal_batch() &&
          empty_fallback.transcript.proposal_records.empty() &&
          empty_fallback.audit.canonical_query_count == 2U &&
          empty_fallback.audit.gpu_supported_center_query_count == 0U &&
          empty_fallback.audit.unsupported_center_query_count == 2U &&
          empty_fallback.audit.gpu_kernel_launch_count == 0U &&
          empty_fallback.audit.buffer_epoch == 0U &&
          fake_gpu_phase14_facet_top_k_proposal_launch_count() == 0U,
      "an all-unsupported batch becomes an explicit launch-free fallback");

  auto mixed = canonical;
  mixed[0U].query_center = unsupported_center;
  const auto sparse = build(context, fixture.cloud, mixed);
  check(
      sparse.complete_proposal_batch() &&
          sparse.audit.gpu_supported_center_query_count == 1U &&
          sparse.audit.unsupported_center_query_count == 1U &&
          sparse.audit.gpu_output_record_count == 1U &&
          sparse.audit.active_host_to_device_query_record_count == 1U &&
          sparse.audit.active_host_to_device_query_byte_count ==
              26U * sizeof(std::uint64_t) &&
          sparse.audit.initialized_device_output_record_count == 1U &&
          sparse.audit.initialized_device_output_byte_count ==
              18U * sizeof(std::uint64_t) &&
          sparse.audit.copied_device_to_host_record_count == 1U &&
          sparse.audit.copied_device_to_host_byte_count ==
              18U * sizeof(std::uint64_t) &&
          sparse.audit.buffer_epoch == 1U &&
          sparse.transcript.proposal_records.size() == 1U &&
          sparse.transcript.proposal_records[0U].source_facet_key ==
              canonical[1U].source_facet_key &&
          fake_gpu_phase14_facet_top_k_proposal_launch_count() == 1U,
      "a mixed batch preserves global query identity for its supported subset");
}

void test_preflight_identity_and_move_contracts() {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  Fixture fixture;
  const auto queries = canonical_queries(fixture);

  check_throws<std::invalid_argument>(
      [&] {
        DirectSparseFacetTopKProposalContext rejected{
            fixture.index, fixture.cloud, 0U};
      },
      "zero physical query capacity is rejected");

  DirectSparseFacetTopKProposalContext context{
      fixture.index, fixture.cloud, 2U};
  DirectSparseFacetTopKProposalContext undersized{
      fixture.index, fixture.cloud, 1U};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(build(undersized, fixture.cloud, queries)); },
      "a batch larger than the fixed context capacity is rejected");

  auto reversed = queries;
  std::reverse(reversed.begin(), reversed.end());
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(build(context, fixture.cloud, reversed)); },
      "noncanonical full-key order is rejected");

  auto out_of_domain = queries;
  out_of_domain[0U].source_facet_key.point_ids[1U] =
      static_cast<PointId>(fixture.cloud.size());
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            build(context, fixture.cloud, out_of_domain));
      },
      "an out-of-domain source key is rejected before launch");

  auto mixed_cardinalities = queries;
  mixed_cardinalities[1U].source_facet_key.point_count = 1U;
  mixed_cardinalities[1U].source_facet_key.point_ids[1U] = 0U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            build(context, fixture.cloud, mixed_cardinalities));
      },
      "mixed source-facet cardinalities are rejected before launch");

  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            fixture.cloud,
            metadata(),
            queries,
            DirectSparseFacetTopKProposalPolicy{0U},
            generous_budget()));
      },
      "a zero Morton window is rejected before launch");

  const CanonicalPointCloud foreign_cloud = make_cloud();
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build(context, foreign_cloud, queries));
      },
      "a context rejects another immutable PointId namespace");
  check_throws<std::invalid_argument>(
      [&] {
        DirectSparseFacetTopKProposalContext rejected{
            fixture.index, foreign_cloud, 2U};
      },
      "a context cannot bind an LBVH to a foreign cloud identity");
  check(
      fake_gpu_phase14_facet_top_k_proposal_launch_count() == 0U,
      "all capacity, order, domain and identity failures precede launch");

  const auto healthy = build(context, fixture.cloud, queries);
  check(
      healthy.complete_proposal_batch() &&
          healthy.audit.buffer_epoch == 1U,
      "preflight rejections do not poison a reusable context");

  DirectSparseFacetTopKProposalContext source{
      fixture.index, fixture.cloud, 2U};
  DirectSparseFacetTopKProposalContext moved{std::move(source)};
  check(
      source.point_count() == 0U &&
          source.maximum_query_count() == 0U &&
          moved.point_count() == fixture.cloud.size() &&
          moved.maximum_query_count() == 2U,
      "moving a context transfers its immutable snapshot and capacity");
  check_throws<std::logic_error>(
      [&] { static_cast<void>(build(source, fixture.cloud, queries)); },
      "a moved-from context is not queryable");
  const auto moved_result = build(moved, fixture.cloud, queries);
  check(
      moved_result.complete_proposal_batch() &&
          moved_result.audit.buffer_epoch == 1U,
      "the moved-to context remains a healthy independent producer");
}

void check_corruption_poisoning(
    const Fixture& fixture,
    FakePhase14FacetTopKProposalCorruption corruption,
    const std::string& label,
    bool prime_epoch = false) {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  const auto queries = canonical_queries(fixture);
  DirectSparseFacetTopKProposalContext context{
      fixture.index, fixture.cloud, 5U};
  if (prime_epoch) {
    const auto first = build(context, fixture.cloud, queries);
    check(
        first.complete_proposal_batch() &&
            first.audit.buffer_epoch == 1U,
        label + " primes a healthy first epoch");
  }

  configure_fake_gpu_phase14_facet_top_k_proposal(
      FakePhase14FacetTopKProposalConfiguration{corruption});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(context, fixture.cloud, queries)); },
      label + " is rejected by authenticated host validation");

  configure_fake_gpu_phase14_facet_top_k_proposal(
      FakePhase14FacetTopKProposalConfiguration{});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(context, fixture.cloud, queries)); },
      label + " poisons the failed context");

  DirectSparseFacetTopKProposalContext fresh{
      fixture.index, fixture.cloud, 5U};
  const auto recovered = build(fresh, fixture.cloud, queries);
  check(
      recovered.complete_proposal_batch() &&
          recovered.audit.buffer_epoch == 1U,
      label + " does not contaminate a fresh context");
}

void test_corruption_matrix_and_poisoning() {
  Fixture fixture;
  using Corruption = FakePhase14FacetTopKProposalCorruption;
  check_corruption_poisoning(
      fixture,
      Corruption::duplicate_query_index,
      "a duplicate supported-query index");
  check_corruption_poisoning(
      fixture,
      Corruption::wrong_key_fingerprint,
      "a changed full-key fingerprint");
  check_corruption_poisoning(
      fixture,
      Corruption::zero_epoch,
      "a zero device epoch");
  check_corruption_poisoning(
      fixture,
      Corruption::stale_epoch_without_advance,
      "a stale device epoch",
      true);
  check_corruption_poisoning(
      fixture,
      Corruption::jumped_epoch,
      "a skipped device epoch");
  check_corruption_poisoning(
      fixture,
      Corruption::wrong_active_transfer_extent,
      "a mismatched active transfer extent");
  check_corruption_poisoning(
      fixture,
      Corruption::missing_initial_snapshot_transfer,
      "a missing initial resident-snapshot transfer");
  check_corruption_poisoning(
      fixture,
      Corruption::repeated_snapshot_transfer,
      "a repeated resident-snapshot transfer",
      true);
  check_corruption_poisoning(
      fixture,
      Corruption::stale_active_candidate_tail,
      "a stale candidate in an active record tail");
  check_corruption_poisoning(
      fixture,
      Corruption::duplicate_candidate,
      "a duplicate candidate");
  check_corruption_poisoning(
      fixture,
      Corruption::out_of_domain_candidate,
      "an out-of-domain candidate");
  check_corruption_poisoning(
      fixture,
      Corruption::out_of_window_candidate,
      "an out-of-window candidate");
  check_corruption_poisoning(
      fixture,
      Corruption::counter_overrun,
      "a floating counter beyond the Morton inspection bound");
  check_corruption_poisoning(
      fixture,
      Corruption::counter_partition_mismatch,
      "an undercounted floating distance partition");
  check_corruption_poisoning(
      fixture,
      Corruption::simulated_async_failure,
      "an asynchronous launcher failure");
}

}  // namespace

int main() {
  test_direct_integer_binary64_projection();
  test_result_epochs_and_digest();
  test_phase14n_snapshot_adoption();
  test_phase14o_integrated_callback_adapter();
  test_phase14n_adoption_rejections_are_atomic();
  test_empty_batch_has_no_launch_or_epoch();
  test_unsupported_centers_keep_the_supported_subset_sparse();
  test_preflight_identity_and_move_contracts();
  test_corruption_matrix_and_poisoning();
  if (failures != 0) {
    std::cerr << failures
              << " Phase 14 facet top-k proposal test(s) failed\n";
    return 1;
  }
  std::cout
      << "Phase 14 facet top-k proposal context tests passed\n";
  return 0;
}
