#include "fake_gpu_phase14_facet_top_k_proposal_launchers.hpp"

#include "exact_center_binary64_projection.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/direct_sparse_facet_top_k_proposal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
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
using morsehgp3d::gpu::DirectSparseFacetTopKProposalPolicy;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalQuery;
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
          3U,
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
