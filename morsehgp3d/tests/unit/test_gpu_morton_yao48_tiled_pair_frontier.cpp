#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_yao48_tiled_pair_frontier.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

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

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonYao48CertifiedPruneKind;
using morsehgp3d::gpu::MortonYao48CertifiedPruneRegion;
using morsehgp3d::gpu::MortonYao48PairCandidate;
using morsehgp3d::gpu::MortonYao48PairFrontierAdvanceBudget;
using morsehgp3d::gpu::MortonYao48PairFrontierConfig;
using morsehgp3d::gpu::MortonYao48PairFrontierContext;
using morsehgp3d::gpu::MortonYao48PairFrontierStatus;
using morsehgp3d::gpu::MortonYao48TiledPairFrontierConfig;
using morsehgp3d::gpu::MortonYao48TiledPairFrontierContext;
using morsehgp3d::gpu::MortonYao48TiledPairFrontierStatus;
using morsehgp3d::gpu::MortonYao48TiledPairFrontierStopReason;
using morsehgp3d::gpu::MortonYao48WitnessBankReceipt;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::spatial::CanonicalPointCloud;

static_assert(!std::is_copy_constructible_v<
              MortonYao48TiledPairFrontierContext>);
static_assert(!std::is_copy_assignable_v<
              MortonYao48TiledPairFrontierContext>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48TiledPairFrontierContext>);
static_assert(std::is_nothrow_move_assignable_v<
              MortonYao48TiledPairFrontierContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud skew_line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(point(
        coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud three_dimensional_cloud(
    std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const std::int64_t signed_index = static_cast<std::int64_t>(index);
    const std::int64_t x =
        static_cast<std::int64_t>((index * 73U) % 163U) - 81;
    const std::int64_t y =
        static_cast<std::int64_t>((index * 151U) % 167U) - 83;
    const std::int64_t z = signed_index -
                           static_cast<std::int64_t>(count / 2U);
    points.push_back(point(
        static_cast<double>(x),
        static_cast<double>(y),
        static_cast<double>(z)));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::uint64_t prune_mass(
    const std::vector<
        morsehgp3d::gpu::MortonYao48CertifiedPruneRegion>& regions) {
  std::uint64_t mass = 0U;
  for (const auto& region : regions) {
    mass += region.certified_pair_mass;
  }
  return mass;
}

struct FrontierTranscript {
  std::vector<MortonYao48PairCandidate> candidates;
  std::vector<MortonYao48WitnessBankReceipt> receipts;
  std::vector<MortonYao48CertifiedPruneRegion> prunes;
};

void erase_runtime_epoch(FrontierTranscript& transcript) {
  for (MortonYao48PairCandidate& candidate : transcript.candidates) {
    candidate.source_snapshot_epoch = 0U;
  }
  for (MortonYao48WitnessBankReceipt& receipt : transcript.receipts) {
    receipt.source_snapshot_epoch = 0U;
  }
  for (MortonYao48CertifiedPruneRegion& prune : transcript.prunes) {
    prune.source_snapshot_epoch = 0U;
  }
}

[[nodiscard]] FrontierTranscript enumerate_untiled(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48PairFrontierContext context{
      std::move(lease),
      MortonYao48PairFrontierConfig{maximum_closed_rank}};
  auto advance = context.advance(
      build,
      cloud,
      MortonYao48PairFrontierAdvanceBudget{
          1'000'000U, 1'000'000U, 1'000'000U, 1'000'000U});
  if (advance.status != MortonYao48PairFrontierStatus::complete ||
      !advance.audit.frontier_complete) {
    throw std::logic_error(
        "the roomy untiled reference did not close its frontier");
  }
  FrontierTranscript transcript{
      std::move(advance.candidates),
      std::move(advance.witness_bank_receipts),
      std::move(advance.certified_prune_regions)};
  erase_runtime_epoch(transcript);
  return transcript;
}

[[nodiscard]] FrontierTranscript enumerate_tiled(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    std::size_t anchor_tile_capacity) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = maximum_closed_rank;
  config.anchor_tile_capacity = anchor_tile_capacity;
  config.maximum_node_visits_per_point = 10'000U;
  config.maximum_candidates_per_point = 10'000U;
  config.maximum_witness_bank_receipts_per_point = 10'000U;
  config.maximum_certified_prune_regions_per_point = 10'000U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  FrontierTranscript transcript;
  for (std::size_t advance_index = 0U;
       advance_index < 1'000U;
       ++advance_index) {
    auto advance = context.advance(build, cloud);
    transcript.candidates.insert(
        transcript.candidates.end(),
        advance.candidates.begin(),
        advance.candidates.end());
    transcript.receipts.insert(
        transcript.receipts.end(),
        advance.witness_bank_receipts.begin(),
        advance.witness_bank_receipts.end());
    transcript.prunes.insert(
        transcript.prunes.end(),
        advance.certified_prune_regions.begin(),
        advance.certified_prune_regions.end());
    if (advance.status ==
        MortonYao48TiledPairFrontierStatus::frontier_complete) {
      if (!advance.audit.pair_coverage_partition_complete) {
        throw std::logic_error(
            "the tiled reference closed without its pair coverage");
      }
      erase_runtime_epoch(transcript);
      return transcript;
    }
    if (advance.status !=
        MortonYao48TiledPairFrontierStatus::tile_complete) {
      throw std::logic_error(
          "the roomy tiled reference was unexpectedly censored");
    }
  }
  throw std::logic_error(
      "the tiled reference exceeded its advance guard");
}

void test_anchor_tiles_close_one_global_partition() {
  const CanonicalPointCloud cloud = skew_line_cloud(64U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 2U;
  config.anchor_tile_capacity = 3U;
  config.maximum_node_visits_per_point = 2'000U;
  config.maximum_candidates_per_point = 2'000U;
  config.maximum_witness_bank_receipts_per_point = 2'000U;
  config.maximum_certified_prune_regions_per_point = 2'000U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  check(
      context.ready() && !lease.ready(),
      "the tiled context owns the existing LBVH traversal lease");

  std::uint64_t emitted_candidate_mass = 0U;
  std::uint64_t emitted_pruned_mass = 0U;
  std::size_t tile_boundary_count = 0U;
  bool radial_region_observed = false;
  constexpr std::size_t maximum_advance_count = 128U;
  for (std::size_t advance_index = 0U;
       advance_index < maximum_advance_count;
       ++advance_index) {
    const auto advance = context.advance(build, cloud);
    const std::uint64_t local_candidate_mass =
        static_cast<std::uint64_t>(advance.candidates.size());
    const std::uint64_t local_pruned_mass =
        prune_mass(advance.certified_prune_regions);
    emitted_candidate_mass += local_candidate_mass;
    emitted_pruned_mass += local_pruned_mass;
    for (const auto& region : advance.certified_prune_regions) {
      radial_region_observed = radial_region_observed ||
                               region.kind ==
                                   MortonYao48CertifiedPruneKind::
                                       radial_subtree;
    }
    check(
        advance.audit.transaction_candidate_pair_mass ==
                local_candidate_mass &&
            advance.audit.transaction_certified_pruned_pair_mass ==
                local_pruned_mass &&
            advance.audit.transaction_node_visit_count <=
                advance.audit.maximum_transaction_node_visit_count &&
            advance.audit.transaction_completed_anchor_count <=
                config.anchor_tile_capacity &&
            advance.audit.tile_anchor_capacity_enforced &&
            advance.audit.fixed_linear_work_budget_enforced &&
            advance.audit
                .candidate_pruned_unresolved_partition_validated &&
            advance.audit.transaction_records_match_mass_deltas &&
            !advance.audit.terminally_censored &&
            advance.audit.host_fake_reference_execution &&
            !advance.audit.second_host_snapshot_retained &&
            advance.audit
                .active_witness_bank_storage_bounded_by_48_times_k &&
            !advance.audit.unfiltered_owned_subtree_emitted &&
            !advance.audit.dense_pair_fallback_performed &&
            !advance.audit.global_pair_matrix_materialized &&
            !advance.audit.higher_order_structure_materialized &&
            !advance.audit.exact_diametral_rank_predicate_evaluated &&
            !advance.audit.cuda_execution_performed &&
            !advance.audit.scientific_decision_published &&
            !advance.audit.public_status_claimed,
        "every tile transaction preserves the bounded mass contract");

    if (advance.status ==
        MortonYao48TiledPairFrontierStatus::tile_complete) {
      ++tile_boundary_count;
      check(
          advance.stop_reason ==
                  MortonYao48TiledPairFrontierStopReason::
                      anchor_tile_boundary &&
              advance.audit.transaction_completed_anchor_count ==
                  config.anchor_tile_capacity &&
              advance.audit.unresolved_pair_mass > 0U &&
              !advance.audit.pair_coverage_partition_complete,
          "a tile boundary is progress, never a false exact result");
      continue;
    }

    check(
        advance.status ==
                MortonYao48TiledPairFrontierStatus::frontier_complete &&
            advance.stop_reason ==
                MortonYao48TiledPairFrontierStopReason::none &&
            advance.audit.pair_coverage_partition_complete &&
            advance.audit.unresolved_pair_mass == 0U,
        "only the final global partition receives complete status");
    const std::uint64_t universe =
        static_cast<std::uint64_t>(cloud.size()) *
        static_cast<std::uint64_t>(cloud.size() - 1U) /
        UINT64_C(2);
    check(
        emitted_candidate_mass + emitted_pruned_mass == universe &&
            advance.audit.cumulative_candidate_pair_mass ==
                emitted_candidate_mass &&
            advance.audit.cumulative_certified_pruned_pair_mass ==
                emitted_pruned_mass &&
            tile_boundary_count > 1U && radial_region_observed &&
            advance.audit.radial_subtree_certificates_reused,
        "candidate plus radial/directional prune mass closes the universe");

    const auto idempotent = context.advance(build, cloud);
    check(
        idempotent.status ==
                MortonYao48TiledPairFrontierStatus::frontier_complete &&
            idempotent.candidates.empty() &&
            idempotent.witness_bank_receipts.empty() &&
            idempotent.certified_prune_regions.empty() &&
            idempotent.audit.transaction_candidate_pair_mass == 0U &&
            idempotent.audit.transaction_certified_pruned_pair_mass == 0U &&
            idempotent.audit.pair_coverage_partition_complete,
        "a completed tiled frontier is idempotent");
    return;
  }
  check(false, "the roomy tiled frontier terminates");
}

void test_linear_node_budget_censors_without_resume_fallback() {
  const CanonicalPointCloud cloud = skew_line_cloud(64U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 2U;
  config.anchor_tile_capacity = 2U;
  config.maximum_node_visits_per_point = 1U;
  config.maximum_candidates_per_point = 64U;
  config.maximum_witness_bank_receipts_per_point = 48U;
  config.maximum_certified_prune_regions_per_point = 64U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};

  for (std::size_t advance_index = 0U;
       advance_index < 128U;
       ++advance_index) {
    const auto advance = context.advance(build, cloud);
    if (advance.status !=
        MortonYao48TiledPairFrontierStatus::censored) {
      check(
          advance.status ==
              MortonYao48TiledPairFrontierStatus::tile_complete,
          "a pre-censure step closes only a bounded anchor tile");
      continue;
    }
    check(
        advance.stop_reason ==
                MortonYao48TiledPairFrontierStopReason::
                    node_visit_budget &&
            context.terminally_censored() &&
            advance.audit.terminally_censored &&
            advance.audit.cumulative_node_visit_count ==
                advance.audit.maximum_node_visit_count &&
            advance.audit.unresolved_pair_mass > 0U &&
            !advance.audit.pair_coverage_partition_complete &&
            advance.audit
                .candidate_pruned_unresolved_partition_validated &&
            !advance.audit.dense_pair_fallback_performed,
        "the global O(n) node cap censors unresolved work explicitly");
    const auto stable = context.advance(build, cloud);
    check(
        stable.status == MortonYao48TiledPairFrontierStatus::censored &&
            stable.stop_reason == advance.stop_reason &&
            stable.candidates.empty() &&
            stable.witness_bank_receipts.empty() &&
            stable.certified_prune_regions.empty() &&
            stable.audit.cumulative_node_visit_count ==
                advance.audit.cumulative_node_visit_count &&
            stable.audit.cumulative_candidate_pair_mass ==
                advance.audit.cumulative_candidate_pair_mass &&
            stable.audit.cumulative_certified_pruned_pair_mass ==
                advance.audit.cumulative_certified_pruned_pair_mass &&
            stable.audit.unresolved_pair_mass ==
                advance.audit.unresolved_pair_mass,
        "terminal censure cannot be bypassed by repeated advance calls");
    return;
  }
  check(false, "the small linear node cap reaches terminal censure");
}

void test_zero_candidate_budget_fails_closed_atomically() {
  const CanonicalPointCloud cloud = skew_line_cloud(8U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 11U;
  config.anchor_tile_capacity = 8U;
  config.maximum_node_visits_per_point = 128U;
  config.maximum_candidates_per_point = 0U;
  config.maximum_witness_bank_receipts_per_point = 48U;
  config.maximum_certified_prune_regions_per_point = 64U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  const auto advance = context.advance(build, cloud);
  check(
      advance.status == MortonYao48TiledPairFrontierStatus::censored &&
          advance.stop_reason ==
              MortonYao48TiledPairFrontierStopReason::candidate_budget &&
          advance.candidates.empty() &&
          advance.witness_bank_receipts.empty() &&
          advance.certified_prune_regions.empty() &&
          advance.audit.cumulative_candidate_pair_mass == 0U &&
          advance.audit.cumulative_certified_pruned_pair_mass == 0U &&
          advance.audit.unresolved_pair_mass ==
              advance.audit.unordered_pair_universe_count &&
          !advance.audit.pair_coverage_partition_complete,
      "candidate capacity is checked before publishing pair or bank state");
}

void test_transaction_candidate_capacity_bounds_one_tile() {
  const CanonicalPointCloud cloud = skew_line_cloud(8U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 11U;
  config.anchor_tile_capacity = 8U;
  config.maximum_node_visits_per_point = 128U;
  config.maximum_candidates_per_point = 64U;
  config.maximum_candidates_per_transaction = 1U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  const auto advance = context.advance(build, cloud);
  check(
      advance.status == MortonYao48TiledPairFrontierStatus::censored &&
          advance.stop_reason ==
              MortonYao48TiledPairFrontierStopReason::candidate_budget &&
          advance.candidates.size() == 1U &&
          advance.audit.maximum_candidate_count == cloud.size() * 64U &&
          advance.audit.maximum_transaction_candidate_count == 1U &&
          advance.audit.cumulative_candidate_pair_mass == 1U &&
          advance.audit.fixed_linear_work_budget_enforced &&
          advance.audit.fixed_transaction_work_budget_enforced &&
          advance.audit.unresolved_pair_mass > 0U,
      "a tile cannot materialize the remaining O(n) global candidate cap");
}

void test_zero_receipt_budget_fails_closed_atomically() {
  const CanonicalPointCloud cloud = skew_line_cloud(8U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 2U;
  config.anchor_tile_capacity = 8U;
  config.maximum_node_visits_per_point = 128U;
  config.maximum_candidates_per_point = 64U;
  config.maximum_witness_bank_receipts_per_point = 0U;
  config.maximum_certified_prune_regions_per_point = 64U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  const auto advance = context.advance(build, cloud);
  check(
      advance.status == MortonYao48TiledPairFrontierStatus::censored &&
          advance.stop_reason ==
              MortonYao48TiledPairFrontierStopReason::
                  witness_bank_receipt_budget &&
          advance.candidates.empty() &&
          advance.witness_bank_receipts.empty() &&
          advance.certified_prune_regions.empty() &&
          advance.audit.cumulative_candidate_pair_mass == 0U &&
          advance.audit.cumulative_witness_bank_receipt_count == 0U &&
          advance.audit.unresolved_pair_mass ==
              advance.audit.unordered_pair_universe_count &&
          !advance.audit.pair_coverage_partition_complete,
      "receipt capacity is checked before publishing candidate or bank");
}

void test_zero_prune_budget_censors_before_prune_publication() {
  const CanonicalPointCloud cloud = skew_line_cloud(16U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 2U;
  config.anchor_tile_capacity = 16U;
  config.maximum_node_visits_per_point = 256U;
  config.maximum_candidates_per_point = 64U;
  config.maximum_witness_bank_receipts_per_point = 48U;
  config.maximum_certified_prune_regions_per_point = 0U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  const auto advance = context.advance(build, cloud);
  check(
      advance.status == MortonYao48TiledPairFrontierStatus::censored &&
          advance.stop_reason ==
              MortonYao48TiledPairFrontierStopReason::
                  certified_prune_region_budget &&
          !advance.candidates.empty() &&
          !advance.witness_bank_receipts.empty() &&
          advance.certified_prune_regions.empty() &&
          advance.audit.cumulative_certified_prune_region_count == 0U &&
          advance.audit.cumulative_certified_pruned_pair_mass == 0U &&
          advance.audit.unresolved_pair_mass > 0U &&
          !advance.audit.pair_coverage_partition_complete,
      "prune capacity censors before publishing an uncertified region");
}

void test_budget_product_overflow_preserves_the_lease() {
  const CanonicalPointCloud cloud = skew_line_cloud(2U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{4U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_node_visits_per_point =
      std::numeric_limits<std::size_t>::max();
  bool rejected = false;
  try {
    MortonYao48TiledPairFrontierContext context{
        std::move(lease), config};
    static_cast<void>(context);
  } catch (const std::length_error&) {
    rejected = true;
  }
  check(
      rejected && lease.ready(),
      "an overflowing n-times-multiplier budget is rejected before move");
}

void test_tiled_transcript_matches_untiled_for_k_and_tile_variants() {
  const CanonicalPointCloud cloud = skew_line_cloud(24U);
  const std::array<std::array<std::size_t, 2U>, 4U> variants{{
      {{2U, 1U}},
      {{3U, 4U}},
      {{5U, 9U}},
      {{11U, 32U}},
  }};
  for (const auto& variant : variants) {
    const FrontierTranscript untiled =
        enumerate_untiled(cloud, variant[0U]);
    const FrontierTranscript tiled =
        enumerate_tiled(cloud, variant[0U], variant[1U]);
    check(
        tiled.candidates == untiled.candidates &&
            tiled.receipts == untiled.receipts &&
            tiled.prunes == untiled.prunes,
        "tiled boundaries preserve the untiled transcript for K and B");
  }
}

void test_default_k10_caps_cover_bank_prefill_bound() {
  const CanonicalPointCloud cloud = three_dimensional_cloud(160U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig config;
  config.maximum_closed_rank = 11U;
  MortonYao48TiledPairFrontierContext context{
      std::move(lease), config};
  const auto advance = context.advance(build, cloud);
  check(
      advance.status ==
              MortonYao48TiledPairFrontierStatus::frontier_complete &&
          advance.audit.pair_coverage_partition_complete &&
          advance.audit.cumulative_candidate_pair_mass <=
              cloud.size() * config.maximum_candidates_per_point &&
          config.maximum_candidates_per_point >= 48U * 10U &&
          advance.audit.fixed_linear_work_budget_enforced &&
          advance.audit.fixed_transaction_work_budget_enforced,
      "default K=10 caps cover the maximum 48-times-ten bank prefill "
      "exposure in 3D");
}

void test_exception_on_mutating_path_poisons_the_wrapper() {
  const CanonicalPointCloud cloud = skew_line_cloud(8U);
  const CanonicalPointCloud foreign_identity = skew_line_cloud(8U);
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierContext context{std::move(lease)};

  bool rejected_foreign_source = false;
  try {
    static_cast<void>(context.advance(build, foreign_identity));
  } catch (const std::invalid_argument&) {
    rejected_foreign_source = true;
  }
  check(
      rejected_foreign_source && context.poisoned() && !context.ready(),
      "an exceptional exit from the mutable phase poisons the wrapper");

  bool rejected_resume = false;
  try {
    static_cast<void>(context.advance(build, cloud));
  } catch (const std::logic_error&) {
    rejected_resume = true;
  }
  check(
      rejected_resume,
      "a poisoned wrapper refuses even a later valid source");
}

void test_configuration_and_singleton_status_guards() {
  const std::array<CertifiedPoint3, 1U> points{
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud singleton =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{2U};
  auto build = builder.build(singleton);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48TiledPairFrontierConfig invalid;
  invalid.anchor_tile_capacity = 0U;
  bool rejected = false;
  try {
    MortonYao48TiledPairFrontierContext context{
        std::move(lease), invalid};
    static_cast<void>(context);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(
      rejected && lease.ready(),
      "an invalid zero-width tile is rejected before consuming its lease");

  MortonYao48TiledPairFrontierContext context{std::move(lease)};
  const auto advance = context.advance(build, singleton);
  check(
      advance.status ==
              MortonYao48TiledPairFrontierStatus::frontier_complete &&
          advance.audit.unordered_pair_universe_count == 0U &&
          advance.audit.unresolved_pair_mass == 0U &&
          advance.audit.pair_coverage_partition_complete &&
          !advance.audit.cuda_execution_performed &&
          !advance.audit.exact_diametral_rank_predicate_evaluated &&
          !advance.audit.scientific_pair_catalog_published &&
          !advance.audit.scientific_decision_published &&
          !advance.audit.public_status_claimed &&
          MortonYao48TiledPairFrontierContext::deployment_status ==
              "architecture_only" &&
          MortonYao48TiledPairFrontierContext::public_status ==
              "not_claimed",
      "an empty pair universe closes without inflating public status");
}

}  // namespace

int main() {
  try {
    test_anchor_tiles_close_one_global_partition();
    test_linear_node_budget_censors_without_resume_fallback();
    test_zero_candidate_budget_fails_closed_atomically();
    test_transaction_candidate_capacity_bounds_one_tile();
    test_zero_receipt_budget_fails_closed_atomically();
    test_zero_prune_budget_censors_before_prune_publication();
    test_budget_product_overflow_preserves_the_lease();
    test_tiled_transcript_matches_untiled_for_k_and_tile_variants();
    test_default_k10_caps_cover_bank_prefill_bound();
    test_exception_on_mutating_path_poisons_the_wrapper();
    test_configuration_and_singleton_status_guards();
  } catch (const std::exception& error) {
    std::cerr << "unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures
              << " tiled Morton/Yao48 frontier check(s) failed\n";
    return 1;
  }
  std::cout << "Tiled Morton/Yao48 pair-frontier checks passed\n";
  return 0;
}
