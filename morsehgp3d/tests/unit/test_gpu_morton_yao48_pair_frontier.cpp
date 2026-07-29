#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/morton_yao48_pair_frontier.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonYao48CertifiedPruneKind;
using morsehgp3d::gpu::MortonYao48CertifiedPruneRegion;
using morsehgp3d::gpu::MortonYao48PairCandidate;
using morsehgp3d::gpu::MortonYao48PairFrontierAdvanceBudget;
using morsehgp3d::gpu::MortonYao48PairFrontierAudit;
using morsehgp3d::gpu::MortonYao48PairFrontierConfig;
using morsehgp3d::gpu::MortonYao48PairFrontierContext;
using morsehgp3d::gpu::MortonYao48PairFrontierStatus;
using morsehgp3d::gpu::MortonYao48PairFrontierStopReason;
using morsehgp3d::gpu::MortonYao48WitnessBankReceipt;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::hierarchy::classify_exact_yao48_cone;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLeafRecord;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<
              MortonYao48PairFrontierContext>);
static_assert(!std::is_copy_assignable_v<
              MortonYao48PairFrontierContext>);
static_assert(std::is_nothrow_move_constructible_v<
              MortonYao48PairFrontierContext>);
static_assert(std::is_nothrow_move_assignable_v<
              MortonYao48PairFrontierContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y,
    double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud collision_cloud() {
  const double close_to_zero = std::ldexp(1.0, -30);
  const std::array<CertifiedPoint3, 9U> points{
      point(0.0, 0.0, 0.0),
      point(close_to_zero, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(-1.0, -1.0, -1.0),
      point(1.0, 1.0, 1.0),
      point(-2.0, 0.5, 0.25),
      point(2.0, -0.5, -0.25)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    points.push_back(point(
        static_cast<double>(index), 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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

[[nodiscard]] CanonicalPointCloud irregular_cloud() {
  const std::array<CertifiedPoint3, 12U> points{
      point(-3.0, -1.0, 0.0),
      point(-2.0, 2.0, 1.0),
      point(-1.0, 0.0, 3.0),
      point(0.0, -3.0, 2.0),
      point(1.0, 1.0, -2.0),
      point(2.0, -2.0, -1.0),
      point(3.0, 0.0, 1.0),
      point(-0.5, 2.5, -3.0),
      point(1.5, 3.0, 2.5),
      point(-2.5, -2.0, -1.5),
      point(2.25, 0.75, -2.75),
      point(0.25, -0.75, 0.5)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

struct Enumeration {
  std::vector<MortonLeafRecord> leaves;
  std::vector<MortonYao48PairCandidate> candidates;
  std::vector<MortonYao48WitnessBankReceipt> receipts;
  std::vector<MortonYao48CertifiedPruneRegion> prunes;
  MortonYao48PairFrontierAudit final_audit{};
  std::size_t exhausted_step_count{};
  std::size_t node_stop_count{};
  std::size_t candidate_stop_count{};
  std::size_t receipt_stop_count{};
  std::size_t prune_stop_count{};
};

[[nodiscard]] Enumeration enumerate(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    MortonYao48PairFrontierAdvanceBudget step_budget) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  Enumeration enumeration;
  enumeration.leaves.assign(
      build.certified_index().leaves().begin(),
      build.certified_index().leaves().end());
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  MortonYao48PairFrontierContext context{
      std::move(lease),
      MortonYao48PairFrontierConfig{maximum_closed_rank}};
  check(
      !lease.ready() && context.ready(),
      "the Morton/Yao48 context consumes and retains the traversal lease");

  constexpr std::size_t maximum_step_count = 1'000'000U;
  for (std::size_t step_index = 0U;
       step_index < maximum_step_count;
       ++step_index) {
    auto step = context.advance(build, cloud, step_budget);
    enumeration.candidates.insert(
        enumeration.candidates.end(),
        step.candidates.begin(),
        step.candidates.end());
    enumeration.receipts.insert(
        enumeration.receipts.end(),
        step.witness_bank_receipts.begin(),
        step.witness_bank_receipts.end());
    enumeration.prunes.insert(
        enumeration.prunes.end(),
        step.certified_prune_regions.begin(),
        step.certified_prune_regions.end());
    enumeration.final_audit = step.audit;
    if (step.status == MortonYao48PairFrontierStatus::complete) {
      check(
          step.stop_reason == MortonYao48PairFrontierStopReason::none,
          "a complete Morton/Yao48 step has no stop reason");
      auto idempotent = context.advance(
          build, cloud, MortonYao48PairFrontierAdvanceBudget{});
      check(
          idempotent.status == MortonYao48PairFrontierStatus::complete &&
              idempotent.stop_reason ==
                  MortonYao48PairFrontierStopReason::none &&
              idempotent.candidates.empty() &&
              idempotent.witness_bank_receipts.empty() &&
              idempotent.certified_prune_regions.empty() &&
              idempotent.audit == enumeration.final_audit,
          "a completed Morton/Yao48 cursor is idempotent at zero budget");
      return enumeration;
    }
    ++enumeration.exhausted_step_count;
    check(
        !step.audit.frontier_complete &&
            step.audit.cumulative_candidate_pair_mass +
                    step.audit.cumulative_certified_pruned_pair_mass +
                    step.audit.unresolved_pair_mass ==
                step.audit.unordered_pair_universe_count,
        "budget exhaustion preserves the exact unresolved pair mass");
    switch (step.stop_reason) {
      case MortonYao48PairFrontierStopReason::node_visit_capacity:
        ++enumeration.node_stop_count;
        break;
      case MortonYao48PairFrontierStopReason::candidate_capacity:
        ++enumeration.candidate_stop_count;
        break;
      case MortonYao48PairFrontierStopReason::
          witness_bank_receipt_capacity:
        ++enumeration.receipt_stop_count;
        break;
      case MortonYao48PairFrontierStopReason::
          certified_prune_region_capacity:
        ++enumeration.prune_stop_count;
        break;
      case MortonYao48PairFrontierStopReason::none:
        check(false, "an exhausted Morton/Yao48 step names its active cap");
        break;
    }
  }
  check(false, "the Morton/Yao48 resume loop terminates");
  return enumeration;
}

using CanonicalPair = std::pair<PointId, PointId>;

[[nodiscard]] CanonicalPair canonical_pair(
    PointId left,
    PointId right) {
  return left < right ? CanonicalPair{left, right}
                      : CanonicalPair{right, left};
}

[[nodiscard]] std::size_t exact_closed_rank(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second) {
  std::size_t rank = 0U;
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId witness = static_cast<PointId>(point_index);
    morsehgp3d::exact::ExactRational dot;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const auto first_delta =
          cloud.point(witness).coordinate(axis) -
          cloud.point(first).coordinate(axis);
      const auto second_delta =
          cloud.point(witness).coordinate(axis) -
          cloud.point(second).coordinate(axis);
      dot = dot + first_delta * second_delta;
    }
    if (dot <= morsehgp3d::exact::ExactRational{}) {
      ++rank;
    }
  }
  return rank;
}

[[nodiscard]] morsehgp3d::exact::ExactLevel exact_squared_distance(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second) {
  morsehgp3d::exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const auto delta = cloud.point(first).coordinate(axis) -
                       cloud.point(second).coordinate(axis);
    squared_distance = squared_distance + delta * delta;
  }
  return morsehgp3d::exact::ExactLevel{std::move(squared_distance)};
}

using ReceiptKey = std::tuple<PointId, std::size_t>;

void verify_differential_partition(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    const Enumeration& enumeration) {
  std::map<CanonicalPair, std::size_t> candidate_multiplicity;
  std::map<CanonicalPair, std::size_t> prune_multiplicity;
  std::map<ReceiptKey, const MortonYao48WitnessBankReceipt*> receipts;
  const std::size_t required_witness_count = maximum_closed_rank - 1U;

  for (const MortonYao48WitnessBankReceipt& receipt :
       enumeration.receipts) {
    check(
        receipt.source_snapshot_epoch ==
                enumeration.final_audit.source_snapshot_epoch &&
            receipt.anchor_position < enumeration.leaves.size() &&
            receipt.anchor_point_id ==
                enumeration.leaves[static_cast<std::size_t>(
                    receipt.anchor_position)]
                    .point_id &&
            receipt.cone_index < 48U &&
            receipt.maximum_closed_rank == maximum_closed_rank &&
            receipt.witness_count == required_witness_count,
        "every frozen bank receipt binds epoch, anchor, chamber and K");
    std::set<PointId> distinct_witnesses;
    morsehgp3d::exact::ExactLevel observed_radius;
    for (std::size_t witness_index = 0U;
         witness_index < receipt.witness_count;
         ++witness_index) {
      const PointId witness = receipt.witness_point_ids[witness_index];
      distinct_witnesses.insert(witness);
      check(
          witness != receipt.anchor_point_id &&
              classify_exact_yao48_cone(
                  cloud.point(receipt.anchor_point_id),
                  cloud.point(witness))
                      .cone_index == receipt.cone_index,
          "every receipt witness is distinct from the anchor and in its "
          "half-open chamber");
      const auto squared_distance = exact_squared_distance(
          cloud, receipt.anchor_point_id, witness);
      if (witness_index == 0U || observed_radius < squared_distance) {
        observed_radius = squared_distance;
      }
    }
    check(
        distinct_witnesses.size() == receipt.witness_count &&
            observed_radius ==
                receipt.witness_squared_radius_upper_bound,
        "a receipt stores K distinct witnesses and their exact maximum D");
    const auto [unused, inserted] = receipts.emplace(
        ReceiptKey{receipt.anchor_point_id, receipt.cone_index},
        &receipt);
    static_cast<void>(unused);
    check(inserted, "a bank freezes exactly once per anchor and chamber");
  }

  for (const MortonYao48PairCandidate& candidate :
       enumeration.candidates) {
    check(
        candidate.source_snapshot_epoch ==
                enumeration.final_audit.source_snapshot_epoch &&
            candidate.maximum_closed_rank == maximum_closed_rank &&
            candidate.support_ids[0] < candidate.support_ids[1] &&
            candidate.morton_partner_position <
                candidate.morton_owner_position &&
            candidate.morton_owner_position < enumeration.leaves.size() &&
            candidate.owner_cone_index < 48U,
        "every candidate is canonical and owned by a strict Morton prefix");
    const std::size_t owner = static_cast<std::size_t>(
        candidate.morton_owner_position);
    const std::size_t partner = static_cast<std::size_t>(
        candidate.morton_partner_position);
    check(
        canonical_pair(
            enumeration.leaves[owner].point_id,
            enumeration.leaves[partner].point_id) ==
                canonical_pair(
                    candidate.support_ids[0],
                    candidate.support_ids[1]) &&
            classify_exact_yao48_cone(
                cloud.point(enumeration.leaves[owner].point_id),
                cloud.point(enumeration.leaves[partner].point_id))
                    .cone_index == candidate.owner_cone_index,
        "candidate metadata replays its Morton owner and exact chamber");
    ++candidate_multiplicity[canonical_pair(
        candidate.support_ids[0], candidate.support_ids[1])];
  }

  for (const MortonYao48CertifiedPruneRegion& prune :
       enumeration.prunes) {
    check(
        prune.source_snapshot_epoch ==
                enumeration.final_audit.source_snapshot_epoch &&
            prune.anchor_position < enumeration.leaves.size() &&
            prune.anchor_point_id ==
                enumeration.leaves[static_cast<std::size_t>(
                    prune.anchor_position)]
                    .point_id &&
            prune.leaf_begin < prune.leaf_end &&
            prune.leaf_end <= prune.anchor_position &&
            prune.certified_pair_mass ==
                prune.leaf_end - prune.leaf_begin &&
            prune.maximum_closed_rank == maximum_closed_rank &&
            prune.cone_mask != UINT64_C(0),
        "each prune region is bound to its epoch, owner and strict prefix");
    if (prune.kind == MortonYao48CertifiedPruneKind::directional_leaf) {
      check(
          prune.certified_pair_mass == UINT64_C(1) &&
              (prune.cone_mask & (prune.cone_mask - UINT64_C(1))) ==
                  UINT64_C(0),
          "a directional record covers one leaf and one chamber");
    } else {
      check(
          prune.kind == MortonYao48CertifiedPruneKind::radial_subtree,
          "a non-leaf prune uses the conservative radial certificate");
    }

    for (std::size_t cone_index = 0U; cone_index < 48U; ++cone_index) {
      if ((prune.cone_mask & (UINT64_C(1) << cone_index)) != UINT64_C(0)) {
        check(
            receipts.contains(
                ReceiptKey{prune.anchor_point_id, cone_index}),
            "every chamber named by a prune has an earlier full-bank "
            "receipt");
      }
    }

    for (std::uint64_t leaf_position = prune.leaf_begin;
         leaf_position < prune.leaf_end;
         ++leaf_position) {
      const PointId target =
          enumeration.leaves[static_cast<std::size_t>(leaf_position)]
              .point_id;
      const std::size_t target_cone = classify_exact_yao48_cone(
          cloud.point(prune.anchor_point_id), cloud.point(target))
                                          .cone_index;
      const auto receipt_it = receipts.find(
          ReceiptKey{prune.anchor_point_id, target_cone});
      check(
          receipt_it != receipts.end(),
          "each expanded target selects one frozen chamber bank");
      if (receipt_it != receipts.end()) {
        const MortonYao48WitnessBankReceipt& receipt =
            *receipt_it->second;
        check(
            std::find(
                receipt.witness_point_ids.begin(),
                receipt.witness_point_ids.begin() +
                    static_cast<std::ptrdiff_t>(receipt.witness_count),
                target) ==
                receipt.witness_point_ids.begin() +
                    static_cast<std::ptrdiff_t>(receipt.witness_count),
            "a pruned target was never credited as its own witness");
      }
      check(
          exact_closed_rank(cloud, prune.anchor_point_id, target) >=
              required_witness_count + 2U,
          "every expanded Yao48 prune has independently verified closed "
          "rank at least K+2");
      ++prune_multiplicity[
          canonical_pair(prune.anchor_point_id, target)];
    }
  }

  const std::uint64_t expected_pair_count =
      static_cast<std::uint64_t>(cloud.size()) *
      static_cast<std::uint64_t>(cloud.size() - 1U) /
      UINT64_C(2);
  check(
      enumeration.final_audit.frontier_complete &&
          enumeration.final_audit.unordered_pair_universe_count ==
              expected_pair_count &&
          enumeration.final_audit.cumulative_candidate_pair_mass ==
              enumeration.candidates.size() &&
          enumeration.final_audit.cumulative_certified_pruned_pair_mass +
                  enumeration.final_audit.cumulative_candidate_pair_mass ==
              expected_pair_count &&
          enumeration.final_audit.unresolved_pair_mass == 0U,
      "candidate plus certified-pruned mass closes n(n-1)/2 exactly");

  for (PointId first = 0U;
       first < static_cast<PointId>(cloud.size());
       ++first) {
    for (PointId second = first + 1U;
         second < static_cast<PointId>(cloud.size());
         ++second) {
      const CanonicalPair pair{first, second};
      const std::size_t candidate_count = candidate_multiplicity[pair];
      const std::size_t prune_count = prune_multiplicity[pair];
      check(
          candidate_count + prune_count == 1U,
          "every unordered pair is represented exactly once by a survivor "
          "or a certified prune");
      if (exact_closed_rank(cloud, first, second) <=
          maximum_closed_rank) {
        check(
            candidate_count == 1U,
            "every pair at or below the requested closed-rank cap survives");
      }
    }
  }
}

void test_small_exact_differential_frontiers() {
  const MortonYao48PairFrontierAdvanceBudget roomy{
      1'000'000U, 1'000'000U, 1'000'000U, 1'000'000U};
  for (const auto& fixture : std::array{
           std::pair{collision_cloud(), std::size_t{2U}},
           std::pair{irregular_cloud(), std::size_t{3U}},
           std::pair{line_cloud(18U), std::size_t{4U}}}) {
    const Enumeration enumeration = enumerate(
        fixture.first, fixture.second, roomy);
    verify_differential_partition(
        fixture.first, fixture.second, enumeration);
  }
}

void test_favourable_line_prunes_without_dense_pair_work() {
  const CanonicalPointCloud cloud = skew_line_cloud(96U);
  const Enumeration enumeration = enumerate(
      cloud,
      2U,
      MortonYao48PairFrontierAdvanceBudget{
          1'000'000U, 1'000'000U, 1'000'000U, 1'000'000U});
  verify_differential_partition(cloud, 2U, enumeration);
  const std::uint64_t universe =
      static_cast<std::uint64_t>(cloud.size()) *
      static_cast<std::uint64_t>(cloud.size() - 1U) /
      UINT64_C(2);
  check(
      enumeration.final_audit.cumulative_radial_subtree_prune_count > 0U &&
          enumeration.final_audit
                  .cumulative_radial_subtree_pruned_pair_mass >
              0U &&
          enumeration.candidates.size() < cloud.size() * 4U &&
          enumeration.candidates.size() * 8U < universe,
      "a favourable Morton line visits a linear-size survivor frontier and "
      "cuts distant ranges as LBVH subtrees");
}

void test_underfull_banks_keep_all_pairs() {
  const CanonicalPointCloud cloud = line_cloud(4U);
  const Enumeration enumeration = enumerate(
      cloud,
      5U,
      MortonYao48PairFrontierAdvanceBudget{
          10'000U, 10'000U, 10'000U, 10'000U});
  verify_differential_partition(cloud, 5U, enumeration);
  check(
      enumeration.prunes.empty() && enumeration.receipts.empty() &&
          enumeration.candidates.size() == 6U &&
          enumeration.final_audit
                  .cumulative_certified_pruned_pair_mass == 0U,
      "underfull chambers fail open to explicit leaf survivors");
}

void test_budgets_resume_atomically() {
  const CanonicalPointCloud cloud = line_cloud(40U);
  const Enumeration roomy = enumerate(
      cloud,
      2U,
      MortonYao48PairFrontierAdvanceBudget{
          1'000'000U, 1'000'000U, 1'000'000U, 1'000'000U});
  const Enumeration node_limited = enumerate(
      cloud,
      2U,
      MortonYao48PairFrontierAdvanceBudget{
          1U, 1'000'000U, 1'000'000U, 1'000'000U});
  check(
      node_limited.node_stop_count > 0U &&
          node_limited.candidates == roomy.candidates &&
          node_limited.receipts == roomy.receipts &&
          node_limited.prunes == roomy.prunes &&
          node_limited.final_audit == roomy.final_audit,
      "node-limited resumes preserve the exact transcript and counters");

  const Enumeration output_limited = enumerate(
      cloud,
      2U,
      MortonYao48PairFrontierAdvanceBudget{1'000'000U, 1U, 1U, 1U});
  check(
      output_limited.exhausted_step_count > 0U &&
          output_limited.candidate_stop_count > 0U &&
          output_limited.prune_stop_count > 0U &&
          output_limited.candidates == roomy.candidates &&
          output_limited.receipts == roomy.receipts &&
          output_limited.prunes == roomy.prunes &&
          output_limited.final_audit == roomy.final_audit,
      "output-limited resumes commit candidate, bank and prune records "
      "atomically");

  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48PairFrontierContext context{
      std::move(lease), MortonYao48PairFrontierConfig{2U}};
  const auto blocked = context.advance(
      build,
      cloud,
      MortonYao48PairFrontierAdvanceBudget{
          1'000'000U, 1'000'000U, 0U, 1'000'000U});
  check(
      blocked.status == MortonYao48PairFrontierStatus::budget_exhausted &&
          blocked.stop_reason == MortonYao48PairFrontierStopReason::
                                     witness_bank_receipt_capacity &&
          blocked.candidates.empty() &&
          blocked.witness_bank_receipts.empty() &&
          blocked.audit.cumulative_candidate_pair_mass == 0U &&
          blocked.audit.active_retained_witness_count == 0U,
      "a zero receipt cap stops before candidate and bank state are "
      "published");
  const auto resumed = context.advance(
      build,
      cloud,
      MortonYao48PairFrontierAdvanceBudget{
          1'000'000U, 1'000'000U, 1'000'000U, 1'000'000U});
  check(
      resumed.status == MortonYao48PairFrontierStatus::complete &&
          resumed.candidates == roomy.candidates &&
          resumed.witness_bank_receipts == roomy.receipts &&
          resumed.certified_prune_regions == roomy.prunes &&
          resumed.audit == roomy.final_audit,
      "lifting the receipt cap resumes the exact transcript without replay");
}

void test_worst_case_is_budget_exhausted_without_fallback() {
  const CanonicalPointCloud cloud = irregular_cloud();
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  auto build = builder.build(cloud);
  auto lease = builder.release_device_traversal_lease(build);
  MortonYao48PairFrontierContext context{
      std::move(lease), MortonYao48PairFrontierConfig{4U}};
  const auto step = context.advance(
      build,
      cloud,
      MortonYao48PairFrontierAdvanceBudget{3U, 1U, 1U, 1U});
  check(
      step.status == MortonYao48PairFrontierStatus::budget_exhausted &&
          step.audit.unresolved_pair_mass > 0U &&
          step.candidates.size() <= 1U &&
          step.certified_prune_regions.size() <= 1U &&
          !step.audit.unfiltered_owned_subtree_emitted &&
          !step.audit.dense_pair_fallback_performed &&
          !step.audit.global_pair_matrix_materialized &&
          !step.audit.frontier_complete,
      "a hard cap reports unresolved work and never falls back to a dense "
      "pair scan");
}

void test_singleton_and_configuration_guards() {
  const std::array<CertifiedPoint3, 1U> points{
      point(0.0, 0.0, 0.0)};
  const CanonicalPointCloud singleton =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  const Enumeration enumeration = enumerate(
      singleton, 2U, MortonYao48PairFrontierAdvanceBudget{});
  check(
      enumeration.candidates.empty() && enumeration.prunes.empty() &&
          enumeration.final_audit.frontier_complete &&
          enumeration.final_audit.unordered_pair_universe_count == 0U &&
          enumeration.final_audit.unresolved_pair_mass == 0U &&
          enumeration.final_audit.active_storage_bounded_by_48_times_k &&
          enumeration.final_audit.source_build_authenticated &&
          enumeration.final_audit.source_cloud_authenticated &&
          enumeration.final_audit.host_fake_reference_execution &&
          !enumeration.final_audit.cuda_execution_performed &&
          !enumeration.final_audit.scientific_decision_published &&
          !enumeration.final_audit.public_status_claimed,
      "a singleton closes an empty frontier without inflating its status");

  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{3U};
  auto build = builder.build(singleton);
  auto lease = builder.release_device_traversal_lease(build);
  bool rejected = false;
  try {
    MortonYao48PairFrontierContext invalid{
        std::move(lease), MortonYao48PairFrontierConfig{12U}};
    static_cast<void>(invalid);
  } catch (const std::out_of_range&) {
    rejected = true;
  }
  check(rejected, "the frontier rejects closed-rank caps above eleven");
}

}  // namespace

int main() {
  try {
    test_small_exact_differential_frontiers();
    test_favourable_line_prunes_without_dense_pair_work();
    test_underfull_banks_keep_all_pairs();
    test_budgets_resume_atomically();
    test_worst_case_is_budget_exhausted_without_fallback();
    test_singleton_and_configuration_guards();
  } catch (const std::exception& error) {
    std::cerr << "unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures
              << " Morton/Yao48 pair-frontier check(s) failed\n";
    return 1;
  }
  std::cout << "Morton/Yao48 pair-frontier checks passed\n";
  return 0;
}
