#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/level_order.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/capped_distinct_point_coverage.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_plan.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_top_k_proposal_transcript.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_gateway_historical_quotient.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_prefix_sweep.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence_star_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"
#include "morsehgp3d/hierarchy/direct_frozen_incidence_quotient.hpp"
#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"
#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_root_coverage_index.hpp"
#include "morsehgp3d/hierarchy/direct_morse_gamma_carrier_conformance.hpp"
#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_candidate_cursor.hpp"
#include "morsehgp3d/hierarchy/morton_grouped_anchored_pair_schedule.hpp"
#include "morsehgp3d/hierarchy/silent_pair_gateway.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

int main() {
  morsehgp3d::hierarchy::CappedDistinctPointCoverage installed_coverage{20U};
  for (std::uint64_t point_id = 0U; point_id < 19U; ++point_id) {
    installed_coverage.insert_point_id(point_id);
  }
  if (installed_coverage.coverage_size_at_least_threshold()) {
    std::cerr << "installed capped coverage exposed nineteen points\n";
    return 1;
  }
  installed_coverage.insert_point_id(19U);
  if (!installed_coverage.coverage_size_at_least_threshold() ||
      installed_coverage.exact_coverage_size_if_below_threshold().has_value()) {
    std::cerr << "installed capped coverage missed the at-least-20 crossing\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::silent_pair_gateway_schema_version == 1U);

  using GroupedTraversalContext = morsehgp3d::hierarchy::
      ExactGroupedAnchoredPairTraversalContext;
  using GroupedTraversalStep = morsehgp3d::hierarchy::
      ExactGroupedAnchoredPairTraversalStep;
  using MortonGroupedScheduleContext = morsehgp3d::hierarchy::
      ExactMortonGroupedAnchoredPairScheduleContext;
  using MortonGroupedScheduleStep = morsehgp3d::hierarchy::
      ExactMortonGroupedAnchoredPairScheduleStep;
  using MortonGroupedCandidateContext = morsehgp3d::hierarchy::
      ExactMortonGroupedAnchoredPairCandidateContext;
  using MortonGroupedCandidateStep = morsehgp3d::hierarchy::
      ExactMortonGroupedAnchoredPairCandidateStep;
  using AnchoredPairClassificationContext = morsehgp3d::hierarchy::
      ExactAnchoredPairCandidateClassificationContext;
  using SparseAnchoredPairSession =
      morsehgp3d::hierarchy::ExactSparseAnchoredPairSession;
  using SparseAnchoredPairTerminalAuthority =
      morsehgp3d::hierarchy::ExactSparseAnchoredPairTerminalAuthority;
  static_assert(!std::is_aggregate_v<GroupedTraversalContext>);
  static_assert(!std::is_default_constructible_v<GroupedTraversalContext>);
  static_assert(!std::is_copy_constructible_v<GroupedTraversalContext>);
  static_assert(std::is_nothrow_move_constructible_v<GroupedTraversalContext>);
  static_assert(!std::is_move_assignable_v<GroupedTraversalContext>);
  static_assert(!std::is_default_constructible_v<GroupedTraversalStep>);
  static_assert(!std::is_aggregate_v<MortonGroupedScheduleContext>);
  static_assert(
      !std::is_default_constructible_v<MortonGroupedScheduleContext>);
  static_assert(!std::is_copy_constructible_v<MortonGroupedScheduleContext>);
  static_assert(
      std::is_nothrow_move_constructible_v<MortonGroupedScheduleContext>);
  static_assert(!std::is_move_assignable_v<MortonGroupedScheduleContext>);
  static_assert(!std::is_default_constructible_v<MortonGroupedScheduleStep>);
  static_assert(
      !std::is_default_constructible_v<MortonGroupedCandidateContext>);
  static_assert(!std::is_copy_constructible_v<MortonGroupedCandidateContext>);
  static_assert(
      std::is_nothrow_move_constructible_v<MortonGroupedCandidateContext>);
  static_assert(!std::is_move_assignable_v<MortonGroupedCandidateContext>);
  static_assert(!std::is_default_constructible_v<MortonGroupedCandidateStep>);
  static_assert(!std::is_copy_constructible_v<MortonGroupedCandidateStep>);
  static_assert(
      std::is_nothrow_move_constructible_v<MortonGroupedCandidateStep>);
  static_assert(
      !std::is_default_constructible_v<AnchoredPairClassificationContext>);
  static_assert(
      !std::is_copy_constructible_v<AnchoredPairClassificationContext>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          AnchoredPairClassificationContext>);
  static_assert(
      !std::is_move_assignable_v<AnchoredPairClassificationContext>);
  static_assert(!std::is_default_constructible_v<SparseAnchoredPairSession>);
  static_assert(!std::is_copy_constructible_v<SparseAnchoredPairSession>);
  static_assert(
      std::is_nothrow_move_constructible_v<SparseAnchoredPairSession>);
  static_assert(
      !std::is_default_constructible_v<
          SparseAnchoredPairTerminalAuthority>);
  static_assert(
      !std::is_copy_constructible_v<SparseAnchoredPairTerminalAuthority>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          SparseAnchoredPairTerminalAuthority>);
  static_assert(
      morsehgp3d::hierarchy::direct_support_terminal_certificate_schema_version ==
      5U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_morse_gamma_carrier_conformance_schema_version == 1U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_successive_incidence_schema_version == 1U);
  const morsehgp3d::hierarchy::
      ExactDirectSparseSuccessiveIncidenceResult
          installed_successive_incidence_probe;
  if (installed_successive_incidence_probe
          .certified_complete_next_incidence() ||
      installed_successive_incidence_probe
          .certified_complete_no_strictly_higher_coface() ||
      installed_successive_incidence_probe.certified_budget_exhaustion()) {
    std::cerr << "installed successive-incidence predicate accepted an empty result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_successive_incidence_star_journal_schema_version ==
      1U);
  const morsehgp3d::hierarchy::
      ExactDirectSparseSuccessiveIncidenceStarJournalResult
          installed_successive_incidence_star_probe;
  if (installed_successive_incidence_star_probe.certified_bounded_star()) {
    std::cerr
        << "installed successive-incidence star predicate accepted an empty result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::direct_sparse_unified_level_plan_schema_version ==
      1U);
  const morsehgp3d::hierarchy::ExactDirectSparseUnifiedLevelPlanResult
      installed_unified_level_plan_probe;
  if (installed_unified_level_plan_probe.certified_bounded_plan()) {
    std::cerr
        << "installed unified-level plan predicate accepted an empty result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::direct_frozen_incidence_quotient_schema_version ==
      1U);
  const morsehgp3d::hierarchy::ExactFrozenIncidenceQuotientResult
      installed_frozen_incidence_quotient_probe;
  if (installed_frozen_incidence_quotient_probe
          .certified_frozen_incidence_quotient() ||
      installed_frozen_incidence_quotient_probe.atomic_empty_failure()) {
    std::cerr
        << "installed frozen-incidence quotient predicate accepted a default result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::
          direct_frozen_incidence_hgp_action_plan_schema_version == 1U);
  const morsehgp3d::hierarchy::ExactFrozenIncidenceHgpActionPlanResult
      installed_frozen_incidence_action_probe;
  if (installed_frozen_incidence_action_probe
          .certified_frozen_incidence_hgp_action_plan() ||
      installed_frozen_incidence_action_probe.atomic_empty_failure()) {
    std::cerr
        << "installed frozen-incidence action predicate accepted a default result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::
          direct_frozen_unified_incidence_batch_schema_version == 1U);
  const morsehgp3d::hierarchy::
      ExactDirectFrozenUnifiedIncidenceBatchResult
          installed_frozen_unified_batch_probe;
  if (installed_frozen_unified_batch_probe
          .certified_frozen_unified_incidence_batch() ||
      installed_frozen_unified_batch_probe.atomic_empty_failure()) {
    std::cerr
        << "installed frozen-unified predicate accepted a default result\n";
    return 1;
  }
  static_assert(
      morsehgp3d::hierarchy::
          direct_morse_forest_root_coverage_index_schema_version == 1U);
  const morsehgp3d::hierarchy::
      ExactDirectMorseForestRootCoverageIndexResult
          installed_root_coverage_index_probe;
  if (installed_root_coverage_index_probe
          .certified_forest_relative_root_coverage_index() ||
      installed_root_coverage_index_probe.certified_atomic_failure() ||
      installed_root_coverage_index_probe.certified_outcome()) {
    std::cerr
        << "installed root-coverage predicate accepted an empty result\n";
    return 1;
  }
  const morsehgp3d::hierarchy::
      ExactDirectMorseGammaCarrierConformanceResult
          installed_carrier_conformance_probe;
  if (installed_carrier_conformance_probe.certified_outcome()) {
    std::cerr
        << "installed carrier-conformance predicate accepted an empty result\n";
    return 1;
  }

  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_gateway_historical_quotient_schema_version == 1U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_facet_descent_batch_plan_schema_version == 1U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_facet_descent_batch_executor_schema_version == 2U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_facet_descent_batch_top_k_proposal_preparation_schema_version ==
      1U);
  static_assert(
      morsehgp3d::hierarchy::
          direct_sparse_facet_descent_batch_sealed_commit_schema_version ==
      1U);
  using SealedPreparedBatch = morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentAnchoredBatchExecutor::
          PreparedTopKProposalBatch;
  static_assert(!std::is_default_constructible_v<SealedPreparedBatch>);
  static_assert(!std::is_copy_constructible_v<SealedPreparedBatch>);
  static_assert(
      std::is_nothrow_move_constructible_v<SealedPreparedBatch>);
  using BatchExecutionResult = morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentBatchExecutionResult;
  const BatchExecutionResult installed_batch_execution_probe;
  if (installed_batch_execution_probe.complete_architecture_execution()) {
    std::cerr
        << "installed batch-execution predicate accepted an empty result\n";
    return 1;
  }
  using ProposalPreparationResult = morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentBatchTopKProposalPreparationResult;
  const ProposalPreparationResult installed_proposal_preparation_probe;
  if (installed_proposal_preparation_probe.certified_outcome()) {
    std::cerr
        << "installed proposal-preparation predicate accepted an empty result\n";
    return 1;
  }
  using SealedCommitVerification = morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentBatchSealedCommitVerification;
  const SealedCommitVerification installed_sealed_commit_probe;
  if (installed_sealed_commit_probe.certified_cursor_advance()) {
    std::cerr
        << "installed sealed-commit predicate accepted an empty result\n";
    return 1;
  }
  using morsehgp3d::exact::BigInt;
  using morsehgp3d::exact::CertifiedPoint3;
  using morsehgp3d::exact::CircumcenterKind;
  using morsehgp3d::exact::CircumcenterSupportStatus;
  using morsehgp3d::exact::CanonicalSupportIds;
  using morsehgp3d::exact::ExactLabelMoments;
  using morsehgp3d::exact::ExactLevel;
  using morsehgp3d::exact::ExactPlane3;
  using morsehgp3d::exact::ExactRational3;
  using morsehgp3d::exact::CertificationStage;
  using morsehgp3d::exact::PredicateFilterPolicy;
  using morsehgp3d::exact::PredicateCounters;
  using morsehgp3d::exact::PredicateSign;
  using morsehgp3d::exact::SpherePointLocation;
  using morsehgp3d::exact::ThreePlaneIntersectionKind;

  const auto installed_input_id =
      morsehgp3d::contract::canonical_v2_id_from_canonical_json_unchecked(
          "Input", "[]");
  if (installed_input_id.to_lower_hex() !=
      "9acfc97d93e6b378a936c94c9c63a3b8263358ba1d19092d3b6f51c9fd2053b5") {
    std::cerr << "installed canonical v2 identifier changed semantics\n";
    return 1;
  }

  if (!morsehgp3d::exact::fp64_filter_environment_supported()) {
    std::cerr << "installed target did not preserve the strict FP64 environment\n";
    return 1;
  }

  const ExactLevel level{BigInt{2}, BigInt{8}};
  if (level.canonical_key() != "1/4") {
    std::cerr << "installed ExactLevel did not preserve canonical semantics\n";
    return 1;
  }
  PredicateCounters level_counters;
  const auto equal_level_order = morsehgp3d::exact::compare_exact_levels(
      ExactLevel{BigInt{1}, BigInt{2}},
      ExactLevel{BigInt{2}, BigInt{4}},
      &level_counters);
  if (equal_level_order.decision.sign() != PredicateSign::zero ||
      equal_level_order.cross_product_difference != 0 ||
      level_counters.cpu_multiprecision_certified() != 1U ||
      level_counters.exact_zeros() != 1U) {
    std::cerr << "installed exact-level ordering changed semantics\n";
    return 1;
  }

  const std::array<CertifiedPoint3, 2> points{
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0)};
  const std::array<std::uint32_t, 1> q_ids{0U};
  const std::array<std::uint32_t, 1> r_ids{1U};
  const ExactLabelMoments q =
      ExactLabelMoments::from_canonical_ids(q_ids, points);
  const ExactLabelMoments r =
      ExactLabelMoments::from_canonical_ids(r_ids, points);
  const auto side = morsehgp3d::exact::decide_power_bisector_side(
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0), r, q);
  if (side.sign() != PredicateSign::positive) {
    std::cerr << "installed power-bisector predicate changed exact semantics\n";
    return 1;
  }

  const auto exact_only_distance =
      morsehgp3d::exact::decide_squared_distance_order(
          CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
          points[0],
          points[1],
          nullptr,
          PredicateFilterPolicy::multiprecision_only);
  if (exact_only_distance.sign() != PredicateSign::negative ||
      exact_only_distance.certification_stage() !=
          CertificationStage::cpu_multiprecision) {
    std::cerr << "installed exact-only predicate path changed semantics\n";
    return 1;
  }
  const auto filtered_distance =
      morsehgp3d::exact::decide_squared_distance_order(
          CertifiedPoint3::from_binary64(0.0, 0.0, 0.0), points[0], points[1]);
  if (filtered_distance.sign() != PredicateSign::negative ||
      filtered_distance.certification_stage() !=
          CertificationStage::fp64_filtered) {
    std::cerr << "installed filtered predicate path changed semantics\n";
    return 1;
  }

  const ExactPlane3 x_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{1}, BigInt{0}, BigInt{0}, BigInt{0}});
  const ExactPlane3 y_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{0}, BigInt{1}, BigInt{0}, BigInt{0}});
  const ExactPlane3 z_zero = ExactPlane3::from_integer_coefficients(
      {BigInt{0}, BigInt{0}, BigInt{1}, BigInt{0}});
  const auto intersection =
      morsehgp3d::exact::intersect_three_planes(x_zero, y_zero, z_zero);
  const auto fourth = morsehgp3d::exact::decide_fourth_plane_incidence(
      x_zero,
      y_zero,
      z_zero,
      ExactPlane3::from_integer_coefficients(
          {BigInt{1}, BigInt{1}, BigInt{1}, BigInt{0}}));
  if (intersection.kind() != ThreePlaneIntersectionKind::unique ||
      !intersection.point().has_value() ||
      intersection.affine_dimension() != 0U ||
      fourth.sign() != PredicateSign::zero) {
    std::cerr << "installed affine exact kernel changed semantics\n";
    return 1;
  }

  const ExactPlane3 adaptive_x = ExactPlane3::from_binary64_coefficients(
      std::array<double, 4>{1.0, 0.0, 0.0, 0.0});
  const ExactPlane3 adaptive_y = ExactPlane3::from_binary64_coefficients(
      std::array<double, 4>{0.0, 1.0, 0.0, 0.0});
  const ExactPlane3 adaptive_z = ExactPlane3::through_points(
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 1.0, 0.0));
  const auto adaptive_orientation = morsehgp3d::exact::orientation_2d_in_plane(
      adaptive_z,
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 1.0, 0.0));
  const auto adaptive_plane_side = morsehgp3d::exact::plane_side(
      adaptive_x, CertifiedPoint3::from_binary64(1.0, 0.0, 0.0));
  const auto adaptive_intersection =
      morsehgp3d::exact::certified_intersect_three_planes(
          adaptive_x, adaptive_y, adaptive_z);
  const auto adaptive_fourth = morsehgp3d::exact::fourth_plane_incidence(
      adaptive_x,
      adaptive_y,
      adaptive_z,
      ExactPlane3::from_binary64_coefficients(
          std::array<double, 4>{1.0, 1.0, 1.0, 1.0}));
  if (adaptive_orientation.decision.sign() != PredicateSign::positive ||
      adaptive_orientation.decision.certification_stage() !=
          CertificationStage::fp64_filtered ||
      adaptive_plane_side.decision.sign() != PredicateSign::positive ||
      adaptive_plane_side.decision.certification_stage() !=
          CertificationStage::fp64_filtered ||
      adaptive_intersection.intersection().kind() !=
          ThreePlaneIntersectionKind::unique ||
      adaptive_intersection.intersection().point() != ExactRational3{} ||
      adaptive_intersection.certification_stage() !=
          CertificationStage::fp64_filtered ||
      adaptive_fourth.decision.sign() != PredicateSign::positive ||
      adaptive_fourth.decision.certification_stage() !=
          CertificationStage::fp64_filtered) {
    std::cerr << "installed affine-provenance cascade changed semantics\n";
    return 1;
  }

  const auto center = morsehgp3d::exact::circumcenter(
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 1.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 0.0, 1.0));
  if (center.kind() != CircumcenterKind::unique ||
      center.support_size() != 4U || center.affine_dimension() != 3U ||
      !center.center().has_value() || !center.squared_level().has_value() ||
      *center.center() !=
          ExactRational3{BigInt{1}, BigInt{1}, BigInt{1}, BigInt{2}} ||
      *center.squared_level() != ExactLevel{BigInt{3}, BigInt{4}}) {
    std::cerr << "installed exact center construction changed semantics\n";
    return 1;
  }

  const std::array<CertifiedPoint3, 3> right_triangle{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 1.0, 0.0)};
  const auto support =
      morsehgp3d::exact::analyze_circumcenter_support(right_triangle);
  if (support.status() != CircumcenterSupportStatus::boundary_reduced ||
      support.reduced_support_size() != 2U ||
      !support.reduced_support_contains(0U) ||
      support.reduced_support_contains(1U) ||
      !support.reduced_support_contains(2U)) {
    std::cerr << "installed exact support reduction changed semantics\n";
    return 1;
  }
  const std::array<std::uint64_t, 3> source_support_ids{0U, 1U, 2U};
  const auto reduced_emission =
      morsehgp3d::exact::support_level_emission_from_analysis(
          source_support_ids, support);
  const std::array<std::uint64_t, 2> reduced_support_ids{0U, 2U};
  const auto direct_emission = morsehgp3d::exact::SupportLevelEmission::create(
      reduced_emission.squared_level(),
      CanonicalSupportIds::from_ids(reduced_support_ids),
      CanonicalSupportIds::from_ids(reduced_support_ids));
  const std::vector<morsehgp3d::exact::SupportLevelEmission> emissions{
      reduced_emission, direct_emission, direct_emission};
  const auto batches = morsehgp3d::exact::canonical_level_batches(emissions);
  if (batches.batches.size() != 1U ||
      batches.batches[0].supports.size() != 1U ||
      batches.batches[0].supports[0].source_provenance.size() != 2U ||
      batches.emission_count != 3U || batches.unique_emission_count != 2U ||
      batches.duplicate_emission_count != 1U) {
    std::cerr << "installed canonical level batching changed semantics\n";
    return 1;
  }
  const auto sphere_side = morsehgp3d::exact::classify_sphere_point(
      support.circumcenter_result(), right_triangle[1]);
  if (sphere_side.location() != SpherePointLocation::boundary ||
      sphere_side.decision().sign() != PredicateSign::zero ||
      sphere_side.decision().certification_stage() !=
          CertificationStage::cpu_multiprecision) {
    std::cerr << "installed exact sphere classification changed semantics\n";
    return 1;
  }
  const auto adaptive_sphere_side = morsehgp3d::exact::classify_sphere_point(
      right_triangle, right_triangle[1]);
  const auto adaptive_level_order = morsehgp3d::exact::compare_support_levels(
      reduced_emission, reduced_emission);
  if (adaptive_sphere_side.location() != SpherePointLocation::boundary ||
      adaptive_sphere_side.decision().certification_stage() !=
          CertificationStage::expansion ||
      adaptive_level_order.decision.sign() != PredicateSign::zero ||
      adaptive_level_order.decision.certification_stage() !=
          CertificationStage::expansion) {
    std::cerr << "installed support-provenance cascade changed semantics\n";
    return 1;
  }

  using morsehgp3d::spatial::CanonicalPointCloud;
  using morsehgp3d::spatial::ExclusionSet;
  using morsehgp3d::spatial::PointId;
  const std::array<CertifiedPoint3, 3> spatial_points{
      CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 2.0, 0.0)};
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(spatial_points);
  const std::array<PointId, 0> no_exclusion_ids{};
  const ExclusionSet no_exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{no_exclusion_ids}, cloud, 0U);
  const auto nearest = morsehgp3d::spatial::brute_force_nearest(
      cloud, ExactRational3{}, no_exclusions);
  const auto unit_ball = morsehgp3d::spatial::brute_force_closed_ball(
      cloud, ExactRational3{}, ExactLevel{BigInt{1}});
  const auto spatial_index =
      morsehgp3d::spatial::MortonLbvhIndex::build(cloud);
  const auto accelerated_nearest = morsehgp3d::spatial::lbvh_nearest(
      spatial_index, cloud, ExactRational3{}, no_exclusions);
  const auto accelerated_ball = morsehgp3d::spatial::lbvh_closed_ball(
      spatial_index, cloud, ExactRational3{}, ExactLevel{BigInt{1}});
  const std::array<PointId, 1U> installed_incumbent_ids{PointId{0U}};
  const morsehgp3d::spatial::ExactLbvhTopKBudget installed_top_k_budget{
      32U, 32U, 32U, 32U, 8U, 1U, 3U};
  const auto incumbent_nearest =
      morsehgp3d::spatial::lbvh_top_k_budgeted(
          spatial_index,
          cloud,
          ExactRational3{},
          1U,
          no_exclusions,
          std::span<const PointId>{installed_incumbent_ids},
          installed_top_k_budget);
  const std::array<PointId, 1U> installed_proposal_ids{PointId{1U}};
  const auto pooled_nearest =
      morsehgp3d::spatial::lbvh_top_k_budgeted(
          spatial_index,
          cloud,
          ExactRational3{},
          1U,
          no_exclusions,
          std::span<const PointId>{installed_incumbent_ids},
          std::span<const PointId>{installed_proposal_ids},
          installed_top_k_budget);
  const std::array<PointId, 1U> installed_grouped_anchors{PointId{0U}};
  const std::array<PointId, 0U> installed_grouped_empty_pool{};
  auto installed_grouped_traversal = GroupedTraversalContext::start_at_root(
      spatial_index,
      cloud,
      installed_grouped_anchors,
      installed_grouped_empty_pool,
      2U);
  const GroupedTraversalStep installed_grouped_fallback =
      installed_grouped_traversal.advance(
          spatial_index,
          cloud,
          morsehgp3d::hierarchy::
              ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  if (installed_grouped_fallback.kind() !=
          morsehgp3d::hierarchy::
              ExactGroupedAnchoredPairTraversalStepKind::fallback_subtree ||
      !installed_grouped_fallback.traversal_complete_after_step() ||
      !installed_grouped_traversal.complete() ||
      installed_grouped_traversal.audit().fallback_subtree_count != 1U ||
      installed_grouped_traversal.audit().exact_predicate_count != 0U) {
    std::cerr << "installed grouped traversal changed semantics\n";
    return 1;
  }
  auto installed_morton_grouped_schedule =
      MortonGroupedScheduleContext::start(
          spatial_index,
          cloud,
          2U,
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairScheduleConfig{3U, 0U});
  const MortonGroupedScheduleStep installed_scheduled_fallback =
      installed_morton_grouped_schedule.advance(
          spatial_index,
          cloud,
          morsehgp3d::hierarchy::
              ExactGroupedAnchoredPairTraversalWorkBudget{1U, 0U});
  const MortonGroupedScheduleStep installed_group_boundary =
      installed_morton_grouped_schedule.advance(
          spatial_index,
          cloud,
          morsehgp3d::hierarchy::
              ExactGroupedAnchoredPairTraversalWorkBudget{0U, 0U});
  if (installed_scheduled_fallback.kind() !=
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairScheduleStepKind::
                  fallback_subtree ||
      installed_group_boundary.kind() !=
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairScheduleStepKind::
                  group_complete ||
      !installed_morton_grouped_schedule.complete() ||
      installed_morton_grouped_schedule.audit().completed_group_count != 1U ||
      installed_morton_grouped_schedule.audit().scheduled_anchor_count !=
          cloud.size()) {
    std::cerr << "installed Morton grouped schedule changed semantics\n";
    return 1;
  }
  auto installed_morton_grouped_candidate_cursor =
      MortonGroupedCandidateContext::start(
          spatial_index,
          cloud,
          2U,
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairScheduleConfig{3U, 0U});
  const MortonGroupedCandidateStep installed_oriented_candidate =
      installed_morton_grouped_candidate_cursor.advance(
          spatial_index,
          cloud,
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairCandidateWorkBudget{
                  8U, 16U, 8U, 0U});
  if (installed_oriented_candidate.kind() !=
          morsehgp3d::hierarchy::
              ExactMortonGroupedAnchoredPairCandidateStepKind::
                  candidate_pair ||
      !installed_oriented_candidate.support_ids().has_value() ||
      (*installed_oriented_candidate.support_ids())[0] >=
          (*installed_oriented_candidate.support_ids())[1] ||
      installed_morton_grouped_candidate_cursor.audit().candidate_pair_count !=
          1U ||
      !installed_morton_grouped_candidate_cursor.audit()
           .no_dynamic_candidate_or_output_arena_materialized) {
    std::cerr << "installed Morton grouped candidate cursor changed semantics\n";
    return 1;
  }
  auto installed_anchored_pair_classification =
      AnchoredPairClassificationContext::start(
          spatial_index,
          cloud,
          *installed_oriented_candidate.support_ids(),
          3U);
  const auto installed_classification_ready =
      installed_anchored_pair_classification.advance(
          spatial_index,
          cloud,
          morsehgp3d::hierarchy::
              ExactAnchoredPairCandidateClassificationBudget{32U});
  if (installed_classification_ready.kind !=
          morsehgp3d::hierarchy::
              ExactAnchoredPairCandidateClassificationStepKind::record_ready ||
      !installed_anchored_pair_classification.record_ready()) {
    std::cerr << "installed resumable anchored classifier changed traversal\n";
    return 1;
  }
  const auto installed_pair_record =
      installed_anchored_pair_classification.take_result(
          spatial_index, cloud);
  if (!installed_pair_record.complete() ||
      (!installed_pair_record.event.has_value() &&
       !installed_pair_record.relevant_extra_shell_diagnostic.has_value()) ||
      !installed_anchored_pair_classification.complete()) {
    std::cerr << "installed resumable anchored classifier changed emission\n";
    return 1;
  }
  const morsehgp3d::hierarchy::
      ExactSparseAnchoredPairSessionTotalCapacity installed_session_capacity{
          1024U, 1024U, 1024U, 1024U, 16U, 1024U, 8U, 32U};
  const morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleConfig
      installed_session_config{3U, 0U};
  auto installed_sparse_session = SparseAnchoredPairSession::start(
      spatial_index,
      cloud,
      3U,
      installed_session_config,
      installed_session_capacity);
  for (std::size_t call = 0U;
       call < 1024U && !installed_sparse_session.complete();
       ++call) {
    const auto step = installed_sparse_session.advance(
        spatial_index,
        cloud,
        morsehgp3d::hierarchy::
            ExactSparseAnchoredPairSessionAdvanceBudget{
                {64U, 64U, 64U, 64U}, {64U}, 1U, 4U});
    if (step.kind() == morsehgp3d::hierarchy::
            ExactSparseAnchoredPairSessionStepKind::
                total_capacity_exhausted) {
      std::cerr << "installed sparse anchored session exhausted capacity\n";
      return 1;
    }
  }
  if (!installed_sparse_session.complete()) {
    std::cerr << "installed sparse anchored session did not terminate\n";
    return 1;
  }
  auto installed_sparse_authority =
      std::move(installed_sparse_session).seal();
  if (!installed_sparse_authority.sealed_in_process_terminal_authority() ||
      !installed_sparse_authority.bound_to(
          spatial_index,
          cloud,
          3U,
          installed_session_config,
          installed_session_capacity) ||
      installed_sparse_authority.records().size() != 3U ||
      installed_sparse_authority.audit()
                  .authenticated_pruned_directed_pair_count +
              installed_sparse_authority.candidate_audit()
                  .orientation_check_count !=
          9U) {
    std::cerr << "installed sparse anchored authority changed semantics\n";
    return 1;
  }
  const morsehgp3d::hierarchy::ExactHigherSupportStreamBudget
      installed_higher_budget{
          1024U, 16U, 16U, 1024U, 4096U, 1024U, 1024U, 4096U};
  morsehgp3d::hierarchy::ExactHigherSupportTerminalSession
      installed_higher_session{
          spatial_index, cloud, 2U, installed_higher_budget, 64U};
  if (installed_higher_session.run_to_terminal() !=
      morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus::terminal) {
    std::cerr << "installed higher terminal session did not terminate\n";
    return 1;
  }
  auto installed_direct_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          spatial_index,
          cloud,
          2U,
          installed_higher_budget,
          std::move(installed_sparse_authority),
          std::move(installed_higher_session).seal());
  if (!installed_direct_facade.terminal_catalog_certified() ||
      installed_direct_facade.certificate.pair_source_kind !=
          morsehgp3d::hierarchy::ExactDirectSupportPairSourceKind::
              sealed_sparse_anchored_session ||
      installed_direct_facade.certificate.pair_result_freshly_replayed) {
    std::cerr << "installed P8m facade changed authority semantics\n";
    return 1;
  }
  if (!nearest.shell_complete() || nearest.strict_below().size() != 0U ||
      nearest.cutoff_shell_ids().size() != 2U ||
      nearest.canonical_choice_ids().size() != 1U ||
      nearest.cutoff_squared_distance() != ExactLevel{BigInt{1}} ||
      !unit_ball.partition_complete() || unit_ball.closed_rank() != 2U ||
      unit_ball.interior_ids().size() != 0U ||
      unit_ball.shell_ids().size() != 2U ||
      unit_ball.exterior_ids().size() != 1U || !spatial_index.ready() ||
      !spatial_index.validated_for(cloud) ||
      !accelerated_nearest.validated_for(cloud) ||
      accelerated_nearest.cutoff_squared_distance() !=
          nearest.cutoff_squared_distance() ||
      accelerated_nearest.cutoff_shell_ids().size() != 2U ||
      !accelerated_ball.validated_for(cloud) ||
      accelerated_ball.closed_rank() != unit_ball.closed_rank() ||
      accelerated_ball.shell_ids().size() != 2U ||
      !incumbent_nearest.complete() ||
      incumbent_nearest.partition().cutoff_squared_distance() !=
          nearest.cutoff_squared_distance() ||
      incumbent_nearest.audit().supplied_incumbent_point_count != 1U ||
      incumbent_nearest.audit()
              .exact_incumbent_distance_evaluation_count !=
          1U ||
      !pooled_nearest.complete() ||
      pooled_nearest.partition().cutoff_squared_distance() !=
          nearest.cutoff_squared_distance() ||
      pooled_nearest.partition().cutoff_shell_ids().size() != 2U ||
      pooled_nearest.audit().supplied_incumbent_point_count != 2U ||
      pooled_nearest.audit().exact_incumbent_distance_evaluation_count !=
          2U) {
    std::cerr << "installed spatial reference oracle changed semantics\n";
    return 1;
  }

  const morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetLocatorBudget
      locator_budget{0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U};
  const auto locator = morsehgp3d::hierarchy::
      build_exact_direct_sparse_positive_facet_locator(
          0U, locator_budget, {UINT64_C(0x51a7), UINT64_C(0xffff)});
  const morsehgp3d::hierarchy::
      ExactDirectSparsePositiveFacetPrefixSweepBudget sweep_budget{
          0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, {0U, 0U}};
  const auto empty_prefix_sweep = morsehgp3d::hierarchy::
      build_exact_direct_sparse_positive_facet_prefix_sweep(
          std::span<const morsehgp3d::hierarchy::
              ExactDirectSparsePositiveFacetPrefixQuery>{},
          {UINT64_C(0x51a7), UINT64_C(0x1001)},
          locator,
          sweep_budget);
  if (!empty_prefix_sweep.certified_partial_refinement() ||
      empty_prefix_sweep.counters.locator_snapshot_check_count != 2U ||
      !empty_prefix_sweep.resolutions.empty()) {
    std::cerr << "installed positive-facet prefix sweep changed semantics\n";
    return 1;
  }
  const morsehgp3d::hierarchy::
      ExactDirectSparseFacetDescentClosureBudget empty_closure_budget{
          0U, 0U, 0U, 1U, {}};
  const auto empty_canonical_closure = morsehgp3d::hierarchy::
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          spatial_index,
          cloud,
          std::span<const morsehgp3d::hierarchy::
              ExactDirectSparseFacetKey>{},
          ExactLevel{BigInt{0}},
          {UINT64_C(0x51a7), UINT64_C(0x1002)},
          locator,
          empty_closure_budget);
  const morsehgp3d::hierarchy::
      ExactDirectSparseFacetTopKProposalTranscriptMetadata
          empty_transcript_metadata{
              0U, ExactLevel{BigInt{0}}, locator.snapshot_stamp()};
  const morsehgp3d::hierarchy::
      ExactDirectSparseFacetTopKProposalTranscriptBudget
          empty_transcript_budget{};
  const auto empty_transcript = morsehgp3d::hierarchy::
      build_exact_direct_sparse_facet_top_k_proposal_transcript(
          empty_transcript_metadata,
          std::span<const morsehgp3d::hierarchy::
              ExactDirectSparseFacetTopKProposalRecord>{},
          empty_transcript_budget);
  const auto empty_proposal_closure = morsehgp3d::hierarchy::
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys_with_top_k_proposal_transcript(
          spatial_index,
          cloud,
          std::span<const morsehgp3d::hierarchy::
              ExactDirectSparseFacetKey>{},
          0U,
          ExactLevel{BigInt{0}},
          {UINT64_C(0x51a7), UINT64_C(0x1002)},
          locator,
          empty_transcript,
          empty_closure_budget);
  if (!empty_canonical_closure
           .certified_complete_relative_positive_closure() ||
      empty_canonical_closure.counters.input_seed_reference_count != 0U ||
      !empty_transcript.complete_proposal_transcript() ||
      !empty_proposal_closure.certified_exact_consumption_outcome() ||
      !empty_proposal_closure.scientific_closure.has_value() ||
      !empty_proposal_closure.scientific_closure
           ->certified_complete_relative_positive_closure() ||
      empty_proposal_closure.consumption_audit.top_k_query_count != 0U) {
    std::cerr << "installed canonical closure entry changed semantics\n";
    return 1;
  }

  const morsehgp3d::hierarchy::
      ExactDirectSparseGatewayCandidateJournalResult empty_gateway_source{};
  const morsehgp3d::hierarchy::
      ExactDirectSparseGatewayCandidateScientificIdentityBudget
          identity_budget{0U, 0U, 0U, 0U, 0U, 0U, 0U, 194U};
  const auto source_identity = morsehgp3d::hierarchy::
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          empty_gateway_source, identity_budget);
  if (!source_identity.certified_identity() ||
      source_identity.arena_record_scan_count != 0U ||
      source_identity.required_exact_level_decimal_byte_count != 0U ||
      source_identity.required_digest_payload_byte_count != 194U) {
    std::cerr << "installed gateway source identity changed semantics\n";
    return 1;
  }

  morsehgp3d::hierarchy::ExactDirectSparseGatewayClockCertificate
      empty_clock_certificate;
  empty_clock_certificate.authority_id = UINT64_C(0x51a7);
  empty_clock_certificate.replay_token = UINT64_C(0x1002);
  empty_clock_certificate.source_scientific_identity_digest =
      source_identity.scientific_identity_digest;
  empty_clock_certificate.final_locator_stamp = locator.snapshot_stamp();
  const auto clock_digest = morsehgp3d::hierarchy::
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          empty_clock_certificate, {0U, 136U});
  if (!clock_digest.certified_digest() ||
      clock_digest.boundary_scan_count != 0U ||
      clock_digest.required_digest_payload_byte_count != 136U) {
    std::cerr << "installed gateway clock digest changed semantics\n";
    return 1;
  }

  auto empty_clock_authority = morsehgp3d::hierarchy::
      build_exact_direct_sparse_gateway_clock_authority_journal(
          UINT64_C(0xa071),
          UINT64_C(0x1003),
          source_identity,
          locator,
          {0U, 0U, 0U, 0U});
  const auto empty_authority_seal =
      empty_clock_authority.seal_clock_certificate(
          empty_gateway_source,
          locator,
          {0U, 0U, 0U, 0U, identity_budget, {0U, 136U}});
  if (!empty_clock_authority.certified_initialized_authority() ||
      !empty_authority_seal.certified_seal() ||
      !empty_clock_authority.certified_sealed_once()) {
    std::cerr << "installed in-memory clock authority changed semantics\n";
    return 1;
  }
  return 0;
}
