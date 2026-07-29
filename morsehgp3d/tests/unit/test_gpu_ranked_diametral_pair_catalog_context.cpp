#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/gpu/ranked_diametral_pair_catalog.hpp"
#include "phase15_ranked_diametral_pair_catalog_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {

enum class FakeRankedPairCatalogBehavior : std::uint8_t {
  closed_complete_gp_unknown,
  closed_complete_gp_complete,
  closed_complete_gp_violated,
  budget_exhausted,
};

enum class FakeRankedPairCatalogCorruption : std::uint8_t {
  none,
  wrong_digest,
  wrong_contract_schema,
  wrong_source_epoch,
  wrong_capacity_echo,
  broken_closed_mass,
  broken_survivor_occurrence_mass,
  inverse_prune_under_unilateral,
  broken_classification_mass,
  broken_gp_mass,
  gp_reuses_closed_prune,
  gp_complete_with_frontier,
  fake_cuda_claim,
  global_gp_claim,
  output_without_closed_authority,
  missing_output_owner,
  morton_partition_not_validated,
  yao48_policy_not_validated,
  candidates_not_point_id_canonicalized,
  candidates_not_deduplicated,
  morton_scientific_authority_claim,
  raw_morton_pair_stream,
  invalid_yao48_filter_policy,
};

struct FakeRankedPairCatalogConfiguration {
  FakeRankedPairCatalogBehavior behavior{
      FakeRankedPairCatalogBehavior::closed_complete_gp_unknown};
  FakeRankedPairCatalogCorruption corruption{
      FakeRankedPairCatalogCorruption::none};
  RankedPairYao48FilterPolicy yao48_filter_policy{
      RankedPairYao48FilterPolicy::owner_unilateral};
};

namespace {

FakeRankedPairCatalogConfiguration configuration;
std::size_t launcher_call_count{};

}  // namespace

void configure_fake_ranked_pair_catalog(
    FakeRankedPairCatalogConfiguration value) noexcept {
  configuration = value;
}

void reset_fake_ranked_pair_catalog() noexcept {
  configuration = {};
  launcher_call_count = 0U;
}

[[nodiscard]] FakeRankedPairCatalogConfiguration
fake_ranked_pair_catalog_configuration() noexcept {
  return configuration;
}

void note_fake_ranked_pair_catalog_call() noexcept {
  ++launcher_call_count;
}

[[nodiscard]] std::size_t
fake_ranked_pair_catalog_call_count() noexcept {
  return launcher_call_count;
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

struct FakeResidentRankedPairCatalog {
  std::vector<std::uint64_t> support_u{UINT64_C(0)};
  std::vector<std::uint64_t> support_v{UINT64_C(1)};
  std::vector<std::uint8_t> closed_rank{UINT8_C(3)};
  std::vector<std::uint64_t> rank_offsets;
  std::vector<std::uint64_t> strict_offsets{UINT64_C(0), UINT64_C(1)};
  std::vector<std::uint64_t> strict_point_ids{UINT64_C(2)};
  std::vector<std::uint64_t> shell_offsets{UINT64_C(0), UINT64_C(0)};
  std::vector<std::uint64_t> shell_point_ids;

  explicit FakeResidentRankedPairCatalog(std::size_t maximum_level)
      : rank_offsets(maximum_level + 2U, UINT64_C(0)) {
    rank_offsets.back() = UINT64_C(1);
  }
};

[[nodiscard]] std::uint64_t as_u64(std::size_t value) {
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t unordered_pair_count(std::size_t count) {
  return as_u64(count) * as_u64(count - 1U) / UINT64_C(2);
}

void configure_closed_complete(
    Phase15RankedPairCatalogDeviceReceipt& receipt,
    RankedPairYao48FilterPolicy yao48_filter_policy,
    std::uint64_t universe) {
  ClosedRankCatalogClosure& closed = receipt.closed_rank_closure;
  closed.yao48_filter_policy = yao48_filter_policy;
  closed.owned_pair_universe_count = universe;
  closed.yao48_owner_pruned_pair_count = universe - UINT64_C(5);
  closed.yao48_inverse_pruned_pair_count =
      yao48_filter_policy ==
              RankedPairYao48FilterPolicy::bidirectional_intersection
          ? UINT64_C(2)
          : UINT64_C(0);
  closed.canonical_candidate_pair_count =
      UINT64_C(5) - closed.yao48_inverse_pruned_pair_count;
  closed.duplicate_survivor_occurrence_count = UINT64_C(2);
  closed.yao48_survivor_occurrence_count =
      closed.canonical_candidate_pair_count +
      closed.duplicate_survivor_occurrence_count;
  closed.above_window_pair_count =
      closed.canonical_candidate_pair_count - UINT64_C(1);
  closed.ranked_record_count = UINT64_C(1);
  closed.closed_rank_catalog_complete = true;
}

void configure_gp_unknown(
    Phase15RankedPairCatalogDeviceReceipt& receipt,
    std::uint64_t universe) {
  PairSupportRelevantGpClosure& gp =
      receipt.pair_support_relevant_gp_closure;
  gp.unordered_pair_universe_count = universe;
  gp.strict_interior_pruned_pair_count = UINT64_C(3);
  gp.shell_scanned_pair_count = UINT64_C(2);
  gp.unresolved_pair_count = universe - UINT64_C(5);
}

void configure_complete_output(
    Phase15RankedPairCatalogDeviceReceipt& receipt,
    std::size_t maximum_level,
    const RankedDiametralPairCatalogBudget& budget) {
  auto resident =
      std::make_shared<FakeResidentRankedPairCatalog>(maximum_level);
  receipt.output.owner = resident;
  receipt.output.support_u = resident->support_u.data();
  receipt.output.support_v = resident->support_v.data();
  receipt.output.closed_rank = resident->closed_rank.data();
  receipt.output.rank_offsets = resident->rank_offsets.data();
  receipt.output.strict_offsets = resident->strict_offsets.data();
  receipt.output.strict_point_ids = resident->strict_point_ids.data();
  receipt.output.shell_offsets = resident->shell_offsets.data();
  receipt.output.catalog_record_capacity =
      budget.maximum_catalog_record_count;
  receipt.output.payload_point_id_capacity =
      budget.maximum_payload_point_id_count;
  receipt.output.catalog_record_count = 1U;
  receipt.output.strict_point_id_count = 1U;
  receipt.output.rank_offset_count = maximum_level + 2U;
  receipt.device_output_layout_validated = true;
  receipt.device_rank_offsets_validated = true;
  receipt.device_records_sorted_unique = true;
  receipt.device_payload_partition_validated = true;
}

}  // namespace

Phase15RankedPairCatalogDeviceReceipt
build_phase15_ranked_diametral_pair_catalog_on_device(
    Phase15RankedDiametralPairCatalogContextState& context,
    const Phase15RankedPairCatalogAdoptedTraversal& traversal,
    std::size_t maximum_level,
    const RankedDiametralPairCatalogBudget& fixed_budget) {
  if (!traversal.owner || !traversal.source_cloud_identity ||
      !traversal.host_fake || traversal.cuda_resident ||
      traversal.cuda_device != -1 ||
      traversal.device_coordinate_bits != nullptr ||
      traversal.device_morton_point_ids != nullptr ||
      traversal.device_nodes != nullptr || traversal.point_count == 0U ||
      traversal.node_count != 2U * traversal.point_count - 1U ||
      maximum_level == 0U ||
      maximum_level > ranked_diametral_pair_catalog_maximum_level ||
      !context.fixed_budget_matches(fixed_budget)) {
    throw std::runtime_error(
        "the fake ranked-pair launcher received an invalid request");
  }
  test_support::note_fake_ranked_pair_catalog_call();
  const test_support::FakeRankedPairCatalogConfiguration config =
      test_support::fake_ranked_pair_catalog_configuration();

  Phase15RankedPairCatalogDeviceReceipt receipt;
  receipt.fixed_budget = fixed_budget;
  receipt.output.catalog_record_capacity =
      fixed_budget.maximum_catalog_record_count;
  receipt.output.payload_point_id_capacity =
      fixed_budget.maximum_payload_point_id_count;
  receipt.point_count = traversal.point_count;
  receipt.node_count = traversal.node_count;
  receipt.maximum_level = maximum_level;
  receipt.launcher_call_count = 1U;
  receipt.source_snapshot_epoch = traversal.source_snapshot_epoch;
  receipt.buffer_epoch = context.advance_epoch();
  receipt.cuda_device = -1;
  receipt.execution_kind =
      Phase15RankedPairCatalogExecutionKind::host_fake;
  receipt.morton_ownership_partition_validated = true;
  receipt.yao48_filter_policy_validated = true;
  receipt.candidate_point_id_canonicalization_validated = true;
  receipt.candidate_deduplication_before_exact_classification_validated =
      true;
  const std::uint64_t universe = unordered_pair_count(traversal.point_count);

  switch (config.behavior) {
    case test_support::FakeRankedPairCatalogBehavior::
        closed_complete_gp_unknown:
    case test_support::FakeRankedPairCatalogBehavior::
        closed_complete_gp_complete:
    case test_support::FakeRankedPairCatalogBehavior::
        closed_complete_gp_violated:
      receipt.status = static_cast<std::uint64_t>(
          RankedDiametralPairCatalogBuildStatus::complete);
      receipt.stop_reason = static_cast<std::uint64_t>(
          RankedDiametralPairCatalogStopReason::none);
      receipt.failure_code = static_cast<std::uint64_t>(
          Phase15RankedPairCatalogFailureCode::none);
      configure_closed_complete(
          receipt, config.yao48_filter_policy, universe);
      configure_gp_unknown(receipt, universe);
      configure_complete_output(receipt, maximum_level, fixed_budget);
      if (config.behavior ==
          test_support::FakeRankedPairCatalogBehavior::
              closed_complete_gp_complete) {
        PairSupportRelevantGpClosure& gp =
            receipt.pair_support_relevant_gp_closure;
        gp.strict_interior_pruned_pair_count = universe - UINT64_C(5);
        gp.shell_scanned_pair_count = UINT64_C(5);
        gp.unresolved_pair_count = 0U;
        gp.decision = PairSupportRelevantGpDecision::satisfied;
        gp.relevant_gp_complete = true;
      } else if (
          config.behavior ==
          test_support::FakeRankedPairCatalogBehavior::
              closed_complete_gp_violated) {
        PairSupportRelevantGpClosure& gp =
            receipt.pair_support_relevant_gp_closure;
        gp.exact_violation_count = UINT64_C(1);
        gp.canonical_violation_support_u = UINT64_C(0);
        gp.canonical_violation_support_v = UINT64_C(1);
        gp.canonical_violation_extra_shell = UINT64_C(2);
        gp.decision = PairSupportRelevantGpDecision::violated;
        gp.canonical_violation_present = true;
      }
      break;
    case test_support::FakeRankedPairCatalogBehavior::budget_exhausted: {
      receipt.status = static_cast<std::uint64_t>(
          RankedDiametralPairCatalogBuildStatus::budget_exhausted);
      receipt.stop_reason = static_cast<std::uint64_t>(
          RankedDiametralPairCatalogStopReason::work_item_capacity);
      receipt.failure_code = static_cast<std::uint64_t>(
          Phase15RankedPairCatalogFailureCode::capacity_exhausted);
      ClosedRankCatalogClosure& closed = receipt.closed_rank_closure;
      closed.yao48_filter_policy = config.yao48_filter_policy;
      closed.owned_pair_universe_count = universe;
      closed.yao48_owner_pruned_pair_count = UINT64_C(2);
      closed.yao48_inverse_pruned_pair_count =
          config.yao48_filter_policy ==
                  RankedPairYao48FilterPolicy::bidirectional_intersection
              ? UINT64_C(1)
              : UINT64_C(0);
      closed.canonical_candidate_pair_count = UINT64_C(2);
      closed.duplicate_survivor_occurrence_count = UINT64_C(1);
      closed.yao48_survivor_occurrence_count = UINT64_C(3);
      closed.unresolved_owned_pair_count =
          universe - closed.yao48_owner_pruned_pair_count -
          closed.yao48_inverse_pruned_pair_count -
          closed.canonical_candidate_pair_count;
      closed.unresolved_candidate_pair_count = UINT64_C(2);
      configure_gp_unknown(receipt, universe);
      receipt.pair_support_relevant_gp_closure.strict_interior_pruned_pair_count =
          0U;
      receipt.pair_support_relevant_gp_closure.shell_scanned_pair_count = 0U;
      receipt.pair_support_relevant_gp_closure.unresolved_pair_count =
          universe;
      break;
    }
  }

  switch (config.corruption) {
    case test_support::FakeRankedPairCatalogCorruption::none:
    case test_support::FakeRankedPairCatalogCorruption::wrong_digest:
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        wrong_contract_schema:
      ++receipt.contract_schema_version;
      break;
    case test_support::FakeRankedPairCatalogCorruption::wrong_source_epoch:
      ++receipt.source_snapshot_epoch;
      break;
    case test_support::FakeRankedPairCatalogCorruption::wrong_capacity_echo:
      ++receipt.output.catalog_record_capacity;
      break;
    case test_support::FakeRankedPairCatalogCorruption::broken_closed_mass:
      ++receipt.closed_rank_closure.yao48_owner_pruned_pair_count;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        broken_survivor_occurrence_mass:
      ++receipt.closed_rank_closure.duplicate_survivor_occurrence_count;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        inverse_prune_under_unilateral:
      --receipt.closed_rank_closure.yao48_owner_pruned_pair_count;
      ++receipt.closed_rank_closure.yao48_inverse_pruned_pair_count;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        broken_classification_mass:
      ++receipt.closed_rank_closure.above_window_pair_count;
      break;
    case test_support::FakeRankedPairCatalogCorruption::broken_gp_mass:
      ++receipt.pair_support_relevant_gp_closure.unresolved_pair_count;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        gp_reuses_closed_prune:
      receipt.closed_prune_mass_reused_for_relevant_gp = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        gp_complete_with_frontier:
      receipt.pair_support_relevant_gp_closure.decision =
          PairSupportRelevantGpDecision::satisfied;
      receipt.pair_support_relevant_gp_closure.relevant_gp_complete = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::fake_cuda_claim:
      receipt.execution_kind =
          Phase15RankedPairCatalogExecutionKind::cuda;
      receipt.cuda_device = 0;
      receipt.cuda_path_qualified = true;
      receipt.kernel_launch_count = 1U;
      receipt.synchronization_count = 1U;
      receipt.exact_gpu_predicates_implemented = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::global_gp_claim:
      receipt.global_relevant_gp_complete_published = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        output_without_closed_authority:
      receipt.closed_rank_closure.closed_rank_catalog_complete = false;
      break;
    case test_support::FakeRankedPairCatalogCorruption::missing_output_owner:
      receipt.output.owner.reset();
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        morton_partition_not_validated:
      receipt.morton_ownership_partition_validated = false;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        yao48_policy_not_validated:
      receipt.yao48_filter_policy_validated = false;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        candidates_not_point_id_canonicalized:
      receipt.candidate_point_id_canonicalization_validated = false;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        candidates_not_deduplicated:
      receipt
          .candidate_deduplication_before_exact_classification_validated =
          false;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        morton_scientific_authority_claim:
      receipt.morton_scientific_authority_claimed = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::raw_morton_pair_stream:
      receipt.raw_morton_pair_stream_materialized = true;
      break;
    case test_support::FakeRankedPairCatalogCorruption::
        invalid_yao48_filter_policy:
      receipt.closed_rank_closure.yao48_filter_policy =
          static_cast<RankedPairYao48FilterPolicy>(UINT8_C(255));
      break;
  }
  receipt.receipt_digest_fnv1a =
      phase15_ranked_pair_catalog_receipt_digest(receipt);
  if (config.corruption ==
      test_support::FakeRankedPairCatalogCorruption::wrong_digest) {
    receipt.receipt_digest_fnv1a ^= UINT64_C(1);
  }
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ClosedRankCatalogClosure;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::PairSupportRelevantGpDecision;
using morsehgp3d::gpu::RankedDiametralPairCatalogBudget;
using morsehgp3d::gpu::RankedDiametralPairCatalogBuildStatus;
using morsehgp3d::gpu::RankedDiametralPairCatalogContext;
using morsehgp3d::gpu::RankedDiametralPairCatalogDeviceLease;
using morsehgp3d::gpu::RankedDiametralPairCatalogStopReason;
using morsehgp3d::gpu::RankedPairYao48FilterPolicy;
using morsehgp3d::gpu::test_support::FakeRankedPairCatalogBehavior;
using morsehgp3d::gpu::test_support::FakeRankedPairCatalogConfiguration;
using morsehgp3d::gpu::test_support::FakeRankedPairCatalogCorruption;
using morsehgp3d::gpu::test_support::configure_fake_ranked_pair_catalog;
using morsehgp3d::gpu::test_support::fake_ranked_pair_catalog_call_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::gpu::test_support::reset_fake_ranked_pair_catalog;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<RankedDiametralPairCatalogContext>);
static_assert(!std::is_copy_assignable_v<RankedDiametralPairCatalogContext>);
static_assert(
    std::is_nothrow_move_constructible_v<RankedDiametralPairCatalogContext>);
static_assert(
    std::is_nothrow_move_assignable_v<RankedDiametralPairCatalogContext>);
static_assert(
    !std::is_copy_constructible_v<RankedDiametralPairCatalogDeviceLease>);
static_assert(
    morsehgp3d::gpu::ranked_diametral_pair_catalog_contract_schema_version ==
    2U);

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

[[nodiscard]] CanonicalPointCloud catalog_cloud() {
  const std::array<CertifiedPoint3, 6U> points{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(2.0, 3.0, 5.0),
      point(-1.0, 4.0, 2.0)};
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] RankedDiametralPairCatalogBudget standard_budget() {
  return RankedDiametralPairCatalogBudget{
      4U, 256U, 64U, 32U, 16U, 64U, 8U};
}

[[nodiscard]] RankedDiametralPairCatalogContext make_context(
    const CanonicalPointCloud& cloud,
    RankedDiametralPairCatalogBudget budget = standard_budget()) {
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  RankedDiametralPairCatalogContext context{
      std::move(lease), budget};
  check(
      !lease.ready(),
      "the ranked-pair context consumes the traversal lease");
  return context;
}

void check_scaffold_only(
    const morsehgp3d::gpu::RankedDiametralPairCatalogBuildAttempt& attempt,
    const std::string& label) {
  check(
      attempt.validated_contract_attempt() &&
          !attempt.scientific_outcome(),
      label + " validates only the host/fake contract");
  const auto& audit = attempt.audit();
  check(
      audit.contract_schema_version ==
              morsehgp3d::gpu::
                  ranked_diametral_pair_catalog_contract_schema_version &&
          audit.host_fake_launcher_exercised &&
          !audit.cuda_execution_performed &&
          !audit.exact_gpu_predicates_implemented &&
          audit.morton_ownership_partition_validated &&
          audit.yao48_filter_policy_validated &&
          audit.candidate_point_id_canonicalization_validated &&
          audit
              .candidate_deduplication_before_exact_classification_validated &&
          !audit.scientific_decision_published &&
          !audit.global_relevant_gp_complete_published &&
          !audit.hierarchy_reduction_or_attachment_published &&
          !audit.morton_scientific_authority_claimed &&
          !audit.raw_morton_pair_stream_materialized &&
          !audit.forbidden_global_pair_matrix_materialized &&
          !audit.public_status_claimed,
      label + " keeps Morton execution-only and classifies only canonical "
              "deduplicated Yao48 survivors");
}

void test_complete_closed_catalog_gp_unknown_and_single_build() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  const CanonicalPointCloud cloud = catalog_cloud();
  RankedDiametralPairCatalogContext context = make_context(cloud);
  auto attempt = context.build(3U);

  check_scaffold_only(attempt, "the complete closed catalog");
  check(
      attempt.status() == RankedDiametralPairCatalogBuildStatus::complete &&
          attempt.stop_reason() ==
              RankedDiametralPairCatalogStopReason::none &&
          attempt.closed_rank_catalog_available(),
      "a closed receipt publishes one fake resident catalog lease");
  const ClosedRankCatalogClosure& closed = attempt.closed_rank_closure();
  check(
      closed.yao48_filter_policy ==
              RankedPairYao48FilterPolicy::owner_unilateral &&
          closed.owned_pair_universe_count == 15U &&
          closed.yao48_owner_pruned_pair_count == 10U &&
          closed.yao48_inverse_pruned_pair_count == 0U &&
          closed.yao48_survivor_occurrence_count == 7U &&
          closed.duplicate_survivor_occurrence_count == 2U &&
          closed.canonical_candidate_pair_count == 5U &&
          closed.closed_rank_catalog_complete,
      "the default report is unilateral Yao48 and its PointId-canonical "
      "deduplication mass closes before exact classification");
  check(
      !attempt.pair_support_relevant_gp_closure().relevant_gp_complete &&
          attempt.pair_support_relevant_gp_closure().decision ==
              PairSupportRelevantGpDecision::unknown,
      "closed-rank completeness does not publish pair-support RelevantGP");
  check(
      attempt.catalog().ready() && attempt.catalog().host_fake() &&
          !attempt.catalog().cuda_resident() &&
          attempt.catalog().audit().contract_schema_version == 2U &&
          attempt.catalog()
              .audit()
              .candidate_point_id_canonicalization_validated &&
          attempt.catalog()
              .audit()
              .candidate_deduplication_before_exact_classification_validated &&
          !attempt.catalog().audit().morton_scientific_authority_claimed &&
          !attempt.catalog().audit().raw_morton_pair_stream_materialized &&
          attempt.catalog().audit().catalog_record_count == 1U &&
          attempt.catalog().audit().strict_interior_point_id_count == 1U &&
          !attempt.catalog().audit().global_relevant_gp_complete_published,
      "the fake lease exposes the intended resident SoA extent without a "
      "CUDA claim");
  check(
      fake_ranked_pair_catalog_call_count() == 1U &&
          attempt.audit().launcher_call_count == 1U &&
          attempt.audit().maximum_level == 3U,
      "all levels are routed by one Kmax launcher call");
  check_throws<std::logic_error>(
      [&context]() { static_cast<void>(context.build(3U)); },
      "one context cannot relaunch once per level");

  RankedDiametralPairCatalogDeviceLease lease =
      attempt.release_catalog();
  check(
      lease.ready() && !attempt.closed_rank_catalog_available(),
      "the complete catalog lease transfers move-only ownership");
}

void test_gp_scope_is_independent_and_inverse_yao48_is_optional() {
  const CanonicalPointCloud cloud = catalog_cloud();

  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  configure_fake_ranked_pair_catalog({
      FakeRankedPairCatalogBehavior::closed_complete_gp_complete,
      FakeRankedPairCatalogCorruption::none,
      RankedPairYao48FilterPolicy::bidirectional_intersection});
  RankedDiametralPairCatalogContext complete_gp = make_context(cloud);
  auto complete = complete_gp.build(3U);
  check_scaffold_only(complete, "the pair-support GP-complete receipt");
  check(
      complete.closed_rank_closure().yao48_filter_policy ==
              RankedPairYao48FilterPolicy::bidirectional_intersection &&
          complete.closed_rank_closure().yao48_owner_pruned_pair_count == 10U &&
          complete.closed_rank_closure().yao48_inverse_pruned_pair_count == 2U &&
          complete.closed_rank_closure().canonical_candidate_pair_count == 3U &&
          complete.pair_support_relevant_gp_closure().relevant_gp_complete &&
          complete.pair_support_relevant_gp_closure().decision ==
              PairSupportRelevantGpDecision::satisfied &&
          complete.catalog().audit().pair_support_relevant_gp_complete &&
          !complete.audit().global_relevant_gp_complete_published,
      "the optional inverse policy is accepted but pair-scope GP never "
      "becomes global GP");

  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  configure_fake_ranked_pair_catalog({
      FakeRankedPairCatalogBehavior::closed_complete_gp_violated,
      FakeRankedPairCatalogCorruption::none,
      RankedPairYao48FilterPolicy::owner_unilateral});
  RankedDiametralPairCatalogContext violated_gp = make_context(cloud);
  auto violated = violated_gp.build(3U);
  const auto& gp = violated.pair_support_relevant_gp_closure();
  check(
      violated.closed_rank_catalog_available() &&
          gp.decision == PairSupportRelevantGpDecision::violated &&
          !gp.relevant_gp_complete && gp.canonical_violation_present &&
          gp.canonical_violation_support_u == 0U &&
          gp.canonical_violation_support_v == 1U &&
          gp.canonical_violation_extra_shell == 2U,
      "an exact pair-scope shell witness remains independent from the "
      "closed catalog lease");
}

void test_budget_exhaustion_is_fail_closed() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  configure_fake_ranked_pair_catalog({
      FakeRankedPairCatalogBehavior::budget_exhausted,
      FakeRankedPairCatalogCorruption::none,
      RankedPairYao48FilterPolicy::owner_unilateral});
  const CanonicalPointCloud cloud = catalog_cloud();
  RankedDiametralPairCatalogContext context = make_context(cloud);
  auto attempt = context.build(3U);
  check_scaffold_only(attempt, "the exhausted catalog attempt");
  check(
      attempt.status() ==
              RankedDiametralPairCatalogBuildStatus::budget_exhausted &&
          attempt.stop_reason() ==
              RankedDiametralPairCatalogStopReason::work_item_capacity &&
          !attempt.closed_rank_closure().closed_rank_catalog_complete &&
          !attempt.closed_rank_catalog_available() &&
          !attempt.pair_support_relevant_gp_closure().relevant_gp_complete,
      "budget exhaustion publishes neither catalog nor GP completeness");
  check_throws<std::logic_error>(
      [&attempt]() { static_cast<void>(attempt.catalog()); },
      "an incomplete attempt has no catalog lease");
}

void test_preflight_level_and_move_guards() {
  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  const CanonicalPointCloud cloud = catalog_cloud();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  RankedDiametralPairCatalogBudget invalid = standard_budget();
  invalid.maximum_anchor_tile_size =
      morsehgp3d::gpu::
          ranked_diametral_pair_catalog_maximum_anchor_tile_size +
      1U;
  check_throws<std::invalid_argument>(
      [&lease, &invalid]() {
        static_cast<void>(RankedDiametralPairCatalogContext{
            std::move(lease), invalid});
      },
      "an oversized anchor tile is rejected before lease consumption");
  check(
      lease.ready(),
      "a failed fixed-capacity preflight leaves the traversal lease usable");

  RankedDiametralPairCatalogContext context{
      std::move(lease), standard_budget()};
  check_throws<std::invalid_argument>(
      [&context]() { static_cast<void>(context.build(0U)); },
      "level zero is rejected before launcher entry");
  check_throws<std::invalid_argument>(
      [&context]() { static_cast<void>(context.build(11U)); },
      "levels above ten are rejected before launcher entry");
  check(
      fake_ranked_pair_catalog_call_count() == 0U,
      "invalid levels do not poison or invoke the launcher");

  RankedDiametralPairCatalogContext moved = std::move(context);
  check(
      context.point_count() == 0U &&
          context.certified_node_count() == 0U,
      "a moved-from ranked-pair context exposes neutral extents");
  check_throws<std::logic_error>(
      [&context]() { static_cast<void>(context.build(3U)); },
      "a moved-from ranked-pair context cannot launch");
  auto valid = moved.build(3U);
  check_scaffold_only(valid, "the moved-to ranked-pair context");
}

void test_yao48_survivor_and_canonical_candidate_caps_are_enforced() {
  const CanonicalPointCloud cloud = catalog_cloud();

  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  RankedDiametralPairCatalogBudget survivor_limited = standard_budget();
  survivor_limited.maximum_yao48_survivor_occurrence_count = 6U;
  RankedDiametralPairCatalogContext survivor_context =
      make_context(cloud, survivor_limited);
  check_throws<std::runtime_error>(
      [&survivor_context]() {
        static_cast<void>(survivor_context.build(3U));
      },
      "the receipt cannot exceed its Yao48 survivor occurrence capacity");

  reset_fake_gpu_phase14_morton_lbvh_build();
  reset_fake_ranked_pair_catalog();
  RankedDiametralPairCatalogBudget canonical_limited = standard_budget();
  canonical_limited.maximum_canonical_candidate_pair_count = 4U;
  canonical_limited.maximum_catalog_record_count = 4U;
  RankedDiametralPairCatalogContext canonical_context =
      make_context(cloud, canonical_limited);
  check_throws<std::runtime_error>(
      [&canonical_context]() {
        static_cast<void>(canonical_context.build(3U));
      },
      "the receipt cannot exceed its PointId-canonical candidate capacity");
}

void test_hostile_receipts_poison_contexts() {
  const CanonicalPointCloud cloud = catalog_cloud();
  const std::array<FakeRankedPairCatalogCorruption, 22U> corruptions{
      FakeRankedPairCatalogCorruption::wrong_digest,
      FakeRankedPairCatalogCorruption::wrong_contract_schema,
      FakeRankedPairCatalogCorruption::wrong_source_epoch,
      FakeRankedPairCatalogCorruption::wrong_capacity_echo,
      FakeRankedPairCatalogCorruption::broken_closed_mass,
      FakeRankedPairCatalogCorruption::broken_survivor_occurrence_mass,
      FakeRankedPairCatalogCorruption::inverse_prune_under_unilateral,
      FakeRankedPairCatalogCorruption::broken_classification_mass,
      FakeRankedPairCatalogCorruption::broken_gp_mass,
      FakeRankedPairCatalogCorruption::gp_reuses_closed_prune,
      FakeRankedPairCatalogCorruption::gp_complete_with_frontier,
      FakeRankedPairCatalogCorruption::fake_cuda_claim,
      FakeRankedPairCatalogCorruption::global_gp_claim,
      FakeRankedPairCatalogCorruption::output_without_closed_authority,
      FakeRankedPairCatalogCorruption::missing_output_owner,
      FakeRankedPairCatalogCorruption::morton_partition_not_validated,
      FakeRankedPairCatalogCorruption::yao48_policy_not_validated,
      FakeRankedPairCatalogCorruption::
          candidates_not_point_id_canonicalized,
      FakeRankedPairCatalogCorruption::candidates_not_deduplicated,
      FakeRankedPairCatalogCorruption::morton_scientific_authority_claim,
      FakeRankedPairCatalogCorruption::raw_morton_pair_stream,
      FakeRankedPairCatalogCorruption::invalid_yao48_filter_policy};
  for (const FakeRankedPairCatalogCorruption corruption : corruptions) {
    reset_fake_gpu_phase14_morton_lbvh_build();
    reset_fake_ranked_pair_catalog();
    RankedDiametralPairCatalogContext context = make_context(cloud);
    configure_fake_ranked_pair_catalog({
        FakeRankedPairCatalogBehavior::closed_complete_gp_unknown,
        corruption,
        RankedPairYao48FilterPolicy::owner_unilateral});
    check_throws<std::runtime_error>(
        [&context]() { static_cast<void>(context.build(3U)); },
        "host validation rejects a forged ranked-pair receipt");
    configure_fake_ranked_pair_catalog({});
    check_throws<std::runtime_error>(
        [&context]() { static_cast<void>(context.build(3U)); },
        "a forged ranked-pair receipt permanently poisons its context");
  }
}

}  // namespace

int main() {
  test_complete_closed_catalog_gp_unknown_and_single_build();
  test_gp_scope_is_independent_and_inverse_yao48_is_optional();
  test_budget_exhaustion_is_fail_closed();
  test_preflight_level_and_move_guards();
  test_yao48_survivor_and_canonical_candidate_caps_are_enforced();
  test_hostile_receipts_poison_contexts();
  if (failures != 0) {
    std::cerr << failures
              << " ranked-pair catalog host contract check(s) failed\n";
    return 1;
  }
  std::cout << "ranked-pair catalog host contract checks passed\n";
  return 0;
}
