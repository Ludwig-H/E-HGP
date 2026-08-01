#include "fixtures/moment_curve_k3_one_step_star_incompleteness_n9.hpp"
#include "fixtures/moment_curve_k3_first_incidence_omitted_noop_batch_n9.hpp"

#include "morsehgp3d/hierarchy/direct_star_h0_retraction_differential.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
namespace fixture = morsehgp3d::tests::fixtures;

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

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] std::size_t binomial(std::size_t n, std::size_t k) {
  if (k > n) {
    return 0U;
  }
  k = std::min(k, n - k);
  std::size_t value = 1U;
  for (std::size_t index = 1U; index <= k; ++index) {
    value = value * (n - k + index) / index;
  }
  return value;
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget
generous_successive_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
generous_star_budget() {
  return {
      1024U,
      11264U,
      11264U,
      112640U,
      131072U,
      131072U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_successive_budget(),
  };
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanBudget generous_plan_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      8388608U,
      1048576U,
      1048576U,
      1048576U,
      16777216U,
  };
}

[[nodiscard]] ExactDirectStarH0RetractionDifferentialBudget tight_budget(
    std::size_t point_count,
    std::size_t order) {
  const std::size_t facet_count = binomial(point_count, order);
  const std::size_t coface_count = binomial(point_count, order + 1U);
  const std::size_t union_count = order * coface_count;
  const std::size_t level_count = facet_count + coface_count;

  ExactDirectStarH0RetractionDifferentialBudget budget;
  budget.gamma_budget = {facet_count, coface_count, union_count};

  budget.maximum_checkpoint_count = 2U * coface_count;
  budget.maximum_partition_component_observation_count =
      3U * coface_count * facet_count;
  budget.maximum_batch_group_audit_count = coface_count;
  budget.maximum_omitted_coface_audit_count = coface_count;
  budget.maximum_first_incidence_facet_audit_count = facet_count;
  budget.maximum_first_incidence_cominimizer_reference_count =
      facet_count * (point_count - order);
  budget.maximum_late_star_coface_audit_count = coface_count;
  budget.maximum_regularity_label_count = level_count;
  budget.maximum_logical_output_entry_count =
      coface_count * (12U * point_count + 32U) + 2U * coface_count +
      facet_count *
          ((point_count - order) * (order + 1U) + order + 4U);
  return budget;
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

struct DirectPipeline {
  DirectSources source;
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget star_budget;
  LbvhTraversalOrder traversal_order{LbvhTraversalOrder::near_first};
  ExactDirectSparseSuccessiveIncidenceStarJournalResult star;
  ExactDirectSparseUnifiedLevelPlanBudget plan_budget;
  ExactDirectSparseUnifiedLevelPlanResult plan;
};

[[nodiscard]] DirectPipeline direct_pipeline(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  auto source = direct_sources(
      index, cloud, requested_maximum_order);
  auto star_budget = generous_star_budget();
  constexpr auto traversal_order = LbvhTraversalOrder::near_first;
  auto star = build_exact_direct_sparse_successive_incidence_star_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      traversal_order);
  auto plan_budget = generous_plan_budget();
  auto plan = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      traversal_order,
      star,
      plan_budget);
  return {
      std::move(source),
      star_budget,
      traversal_order,
      std::move(star),
      plan_budget,
      std::move(plan),
  };
}

[[nodiscard]] ExactDirectStarH0RetractionDifferentialResult build_result(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectPipeline& pipeline,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget) {
  return build_exact_direct_star_h0_retraction_differential(
      index,
      cloud,
      pipeline.source.facade,
      pipeline.source.event_journal,
      pipeline.source.arm_budget,
      pipeline.source.arm_journal,
      pipeline.source.incidence_budget,
      pipeline.source.incidence_journal,
      pipeline.star_budget,
      pipeline.traversal_order,
      pipeline.star,
      pipeline.plan_budget,
      pipeline.plan,
      order,
      budget);
}

[[nodiscard]] ExactDirectStarH0RetractionDifferentialVerification verify_result(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectPipeline& pipeline,
    std::size_t order,
    const ExactDirectStarH0RetractionDifferentialBudget& budget,
    const ExactDirectStarH0RetractionDifferentialResult& observed) {
  return verify_exact_direct_star_h0_retraction_differential(
      index,
      cloud,
      pipeline.source.facade,
      pipeline.source.event_journal,
      pipeline.source.arm_budget,
      pipeline.source.arm_journal,
      pipeline.source.incidence_budget,
      pipeline.source.incidence_journal,
      pipeline.star_budget,
      pipeline.traversal_order,
      pipeline.star,
      pipeline.plan_budget,
      pipeline.plan,
      order,
      budget,
      observed);
}

void print_point_ids(const std::vector<PointId>& point_ids) {
  std::cout << '[';
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    std::cout << point_ids[index];
  }
  std::cout << ']';
}

void print_level(const ExactLevel& squared_level) {
  std::cout << squared_level.numerator_string() << '/'
            << squared_level.denominator_string();
}

void print_component_witnesses(
    const std::vector<std::vector<std::vector<PointId>>>& core_components,
    const std::vector<std::vector<PointId>>& covered_components) {
  std::cout << '[';
  for (std::size_t component = 0U; component < core_components.size();
       ++component) {
    if (component != 0U) {
      std::cout << '|';
    }
    std::cout << "{core=";
    std::cout << '[';
    for (std::size_t facet = 0U;
         facet < core_components[component].size();
         ++facet) {
      if (facet != 0U) {
        std::cout << ',';
      }
      print_point_ids(core_components[component][facet]);
    }
    std::cout << "] points=";
    if (component < covered_components.size()) {
      print_point_ids(covered_components[component]);
    } else {
      std::cout << "[]";
    }
    std::cout << '}';
  }
  std::cout << ']';
}

[[nodiscard]] std::vector<PointId> reconstructed_star_coface_ids(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    std::size_t coface_index) {
  const auto key =
      reconstruct_exact_direct_sparse_successive_incidence_star_coface(
          star, coface_index);
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

void print_n9_failure_diagnostic(
    const DirectPipeline& pipeline,
    const ExactDirectStarH0RetractionDifferentialResult& result,
    std::size_t order) {
  const auto& counters = result.counters;
  std::cout << "DIAGNOSTIC decision="
            << static_cast<unsigned>(result.decision)
            << " counters{preflight=" << counters.preflight_count
            << ",plan_verify=" << counters.source_plan_verification_count
            << ",diameter_pairs="
            << counters.diameter_pair_distance_evaluation_count
            << ",gamma_build=" << counters.high_cut_gamma_build_count
            << ",gamma_verify="
            << counters.high_cut_gamma_verification_count
            << ",regularity_build="
            << counters.regularity_miniball_build_count
            << ",regularity_verify="
            << counters.regularity_miniball_verification_count
            << ",regularity_ball="
            << counters.regularity_closed_ball_query_count
            << ",gamma=" << counters.exhaustive_gamma_coface_count
            << ",star=" << counters.retained_star_coface_count
            << ",first=" << counters.first_incidence_subflow_coface_count
            << ",direct=" << counters.direct_core_coface_count
            << ",core_facets=" << counters.direct_core_facet_count
            << ",outside_star=" << counters.omitted_gamma_coface_count
            << ",lambda_facets=" << counters.first_incidence_facet_count
            << ",comin_refs="
            << counters.first_incidence_cominimizer_reference_count
            << ",late_star=" << counters.late_star_omitted_coface_count
            << ",late_silent="
            << counters.late_star_silent_continuation_count
            << ",candidate_noop_groups="
            << counters.first_incidence_omitted_noop_group_count
            << ",star_noop_groups="
            << counters.retained_star_omitted_noop_group_count
            << ",checkpoints=" << counters.checkpoint_count
            << ",component_obs="
            << counters.partition_component_observation_count
            << ",groups=" << counters.batch_group_count
            << ",outside_silent="
            << counters.omitted_silent_continuation_count
            << ",logical=" << counters.logical_output_entry_count << "}\n";

  const auto checkpoint = std::find_if(
      result.checkpoint_audits.begin(),
      result.checkpoint_audits.end(),
      [](const auto& audit) {
        return !audit
                    .first_incidence_subflow_canonical_core_partition_equal ||
               !audit.first_incidence_subflow_covered_point_unions_equal;
      });
  if (checkpoint == result.checkpoint_audits.end()) {
    std::cout << "DIAGNOSTIC first_partition_divergence=none\n";
  } else {
    std::cout << "DIAGNOSTIC first_partition_divergence level=";
    print_level(checkpoint->squared_level);
    std::cout << " cut="
              << (checkpoint->cut_side ==
                          ExactDirectStarH0RetractionCutSide::strict
                      ? "strict"
                      : "closed")
              << " gamma_components="
              << checkpoint->exhaustive_gamma_component_count
              << " candidate_components="
              << checkpoint->first_incidence_subflow_component_count
              << " gamma_core_facets="
              << checkpoint->exhaustive_gamma_core_facet_count
              << " candidate_core_facets="
              << checkpoint->first_incidence_subflow_core_facet_count
              << " gamma_point_refs="
              << checkpoint->exhaustive_gamma_covered_point_reference_count
              << " candidate_point_refs="
              << checkpoint
                     ->first_incidence_subflow_covered_point_reference_count
              << " gamma_digest="
              << checkpoint->exhaustive_gamma_projected_partition_digest
                     .to_lower_hex()
              << " candidate_digest="
              << checkpoint
                     ->first_incidence_subflow_projected_partition_digest
                     .to_lower_hex()
              << "\nDIAGNOSTIC gamma_partition=";
    print_component_witnesses(
        checkpoint->divergent_exhaustive_gamma_component_core_facets,
        checkpoint
            ->divergent_exhaustive_gamma_component_covered_point_ids);
    std::cout << "\nDIAGNOSTIC candidate_partition=";
    print_component_witnesses(
        checkpoint
            ->divergent_first_incidence_subflow_component_core_facets,
        checkpoint
            ->divergent_first_incidence_subflow_component_covered_point_ids);

    std::set<std::vector<PointId>> candidate_keys;
    for (const auto& audit : result.first_incidence_audits) {
      candidate_keys.insert(
          audit.complete_cominimizer_coface_point_ids.begin(),
          audit.complete_cominimizer_coface_point_ids.end());
    }
    for (const auto& batch : pipeline.plan.batches) {
      if (batch.order != order) {
        continue;
      }
      for (std::size_t local = 0U; local < batch.direct_reference_count;
           ++local) {
        const auto& reference = pipeline.plan.direct_references[
            batch.direct_reference_offset + local];
        if (reference.role == ExactDirectMorseH0Role::saddle &&
            reference.source_star_direct_coface_index.has_value()) {
          candidate_keys.insert(reconstructed_star_coface_ids(
              pipeline.star,
              *reference.source_star_direct_coface_index));
        }
      }
    }
    std::cout << "\nDIAGNOSTIC active_star_absent_from_candidate=";
    std::cout << '[';
    bool first = true;
    for (const auto& coface : pipeline.star.cofaces) {
      const auto ids = reconstructed_star_coface_ids(
          pipeline.star, coface.coface_index);
      const bool target_order = ids.size() == order + 1U;
      const bool active =
          checkpoint->cut_side ==
                  ExactDirectStarH0RetractionCutSide::strict
              ? coface.squared_level < checkpoint->squared_level
              : coface.squared_level <= checkpoint->squared_level;
      if (!target_order || !active || candidate_keys.contains(ids)) {
        continue;
      }
      if (!first) {
        std::cout << ',';
      }
      first = false;
      print_point_ids(ids);
      std::cout << '@';
      print_level(coface.squared_level);
    }
    std::cout << "]\n";
  }

  const auto batch = std::find_if(
      result.batch_group_audits.begin(),
      result.batch_group_audits.end(),
      [](const auto& audit) {
        return (!audit.first_incidence_subflow_group_present &&
                !audit
                     .first_incidence_subflow_group_omitted_as_certified_noop) ||
               !audit.first_incidence_subflow_q_r_equal ||
               !audit.first_incidence_subflow_action_equal ||
               !audit.first_incidence_subflow_parent_partition_equal ||
               !audit.first_incidence_subflow_direct_core_delta_equal ||
               !audit.first_incidence_subflow_added_point_delta_equal;
      });
  if (batch == result.batch_group_audits.end()) {
    std::cout << "DIAGNOSTIC first_batch_divergence=none\n";
  } else {
    std::cout << "DIAGNOSTIC first_batch_divergence level=";
    print_level(batch->squared_level);
    std::cout << " candidate_present="
              << batch->first_incidence_subflow_group_present
              << " candidate_omitted_noop="
              << batch
                     ->first_incidence_subflow_group_omitted_as_certified_noop
              << " gamma_qr=" << batch->exhaustive_gamma_q_r
              << " candidate_qr=" << batch->first_incidence_subflow_q_r
              << " gamma_action="
              << static_cast<unsigned>(batch->exhaustive_gamma_action)
              << " candidate_action="
              << static_cast<unsigned>(batch->first_incidence_subflow_action)
              << " parent_equal="
              << batch->first_incidence_subflow_parent_partition_equal
              << " gamma_parent="
              << batch->exhaustive_gamma_parent_partition_digest.to_lower_hex()
              << " candidate_parent="
              << batch->first_incidence_subflow_parent_partition_digest
                     .to_lower_hex()
              << " core_delta_equal="
              << batch->first_incidence_subflow_direct_core_delta_equal
              << " point_delta_equal="
              << batch->first_incidence_subflow_added_point_delta_equal
              << " gamma_added_points=";
    print_point_ids(batch->exhaustive_gamma_added_point_ids);
    std::cout << " candidate_added_points=";
    print_point_ids(batch->first_incidence_subflow_added_point_ids);
    std::cout << '\n';
  }
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  return canonical_cloud(std::array{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  });
}

[[nodiscard]] CanonicalPointCloud moment_n9_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(
      fixture::moment_curve_k3_one_step_star_incompleteness_n9_points.size());
  for (const auto& source :
       fixture::moment_curve_k3_one_step_star_incompleteness_n9_points) {
    points.push_back(point(
        static_cast<double>(source[0]),
        static_cast<double>(source[1]),
        static_cast<double>(source[2])));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

void test_e5_silent_equal_level_batch() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectPipeline pipeline = direct_pipeline(index, cloud, 2U);
  const auto budget = tight_budget(cloud.size(), 2U);
  const auto result = build_result(index, cloud, pipeline, 2U, budget);

  check(
      result.certified_differential(),
      "E5 direct-star/Gamma H0 retraction did not certify");
  check(
      result.counters.exhaustive_gamma_coface_count == 10U &&
          result.counters.retained_star_coface_count == 10U &&
          result.counters.omitted_gamma_coface_count == 0U &&
          result.counters.first_incidence_subflow_coface_count <= 10U &&
          result.counters.first_incidence_subflow_coface_count >=
              result.counters.direct_core_coface_count &&
          result.first_incidence_subflow_equals_direct_union_complete_cominimizers,
      "E5 did not retain the complete ten-coface direct star");

  const auto silent_batch = std::find_if(
      result.batch_group_audits.begin(),
      result.batch_group_audits.end(),
      [](const auto& audit) {
        return audit.squared_level == level(33, 2);
      });
  check(
      silent_batch != result.batch_group_audits.end() &&
          silent_batch->exhaustive_gamma_q_r == 1U &&
          silent_batch->retained_star_q_r == 1U &&
          silent_batch->first_incidence_subflow_q_r == 1U &&
          silent_batch->exhaustive_gamma_action ==
              ExactDirectStarH0RetractionAction::continuation &&
          silent_batch->retained_star_action ==
              ExactDirectStarH0RetractionAction::continuation &&
          silent_batch->first_incidence_subflow_action ==
              ExactDirectStarH0RetractionAction::continuation &&
          silent_batch->added_direct_core_facet_count == 1U &&
          silent_batch->retained_new_coface_count == 2U &&
          silent_batch->omitted_new_coface_count == 0U &&
          silent_batch->first_incidence_subflow_new_coface_count == 2U &&
          silent_batch->exhaustive_gamma_added_point_ids.empty() &&
          silent_batch->retained_star_added_point_ids.empty() &&
          silent_batch->first_incidence_subflow_added_point_ids.empty(),
      "E5 lost the exact 33/2 qR=1 silent AC attachment");
}

void test_moment_n9_048_omissions_preserve_h0() {
  const CanonicalPointCloud cloud = moment_n9_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectPipeline pipeline = direct_pipeline(index, cloud, 3U);
  const auto budget = tight_budget(cloud.size(), 3U);
  const auto result = build_result(index, cloud, pipeline, 3U, budget);
  if (!result.certified_differential()) {
    print_n9_failure_diagnostic(pipeline, result, 3U);
  }

  check(
      result.certified_differential(),
      "moment10000 n9/k3 direct-star/Gamma H0 retraction did not certify");
  check(
      result.counters.exhaustive_gamma_coface_count == 126U &&
          result.counters.retained_star_coface_count == 77U &&
          result.counters.direct_core_coface_count == 6U &&
          result.counters.direct_core_facet_count == 19U &&
          result.counters.omitted_gamma_coface_count == 49U &&
          result.counters.omitted_silent_continuation_count == 49U &&
          result.counters.first_incidence_facet_count == 19U &&
          result.counters.first_incidence_subflow_coface_count >= 6U &&
          result.counters.first_incidence_subflow_coface_count < 77U &&
          result.counters.late_star_omitted_coface_count ==
              77U - result.counters.first_incidence_subflow_coface_count &&
          result.counters.late_star_silent_continuation_count ==
              result.counters.late_star_omitted_coface_count,
      "moment10000 n9/k3 lost the exact 126/77/6/19/49 counts");
  check(
      result.conditional_horizontal_h0_retraction_certified &&
          result.conditional_first_incidence_subflow_h0_retraction_certified &&
          result.every_direct_core_facet_has_complete_first_incidence_cominimizers &&
          result.first_incidence_subflow_equals_direct_union_complete_cominimizers &&
          result.every_late_star_omission_has_strictly_earlier_core_first_incidence &&
          result.every_late_star_omission_is_qr1_zero_point_continuation &&
          result.every_missing_first_incidence_group_is_certified_noop &&
          result.every_missing_retained_star_group_is_certified_noop &&
          result.conditional_reduced_h0_forest_semantics_equivalent &&
          result.sparse_publication_requires_new_contract_revision &&
          !result.contract_v2_identity_equivalence_claimed &&
          !result.contract_v2_batch_identity_equivalence_claimed &&
          !result.contract_v2_forest_identity_equivalence_claimed &&
          !result.exhaustive_gamma_faceted_payload_reproduced &&
          !result.forest_semantics_exact_claimed &&
          !result.incidence_complete_reduction_proved &&
          !result.universal_product_completeness_claimed &&
          !result.public_status_claimed,
      "bounded H0 equivalence was not separated from public v2 identity");
  std::cout << "moment10000 n9/k3: exhaustive="
            << result.counters.exhaustive_gamma_coface_count
            << " retained=" << result.counters.retained_star_coface_count
            << " omitted=" << result.counters.omitted_gamma_coface_count
            << " first_incidence="
            << result.counters.first_incidence_subflow_coface_count
            << " late_star_omitted="
            << result.counters.late_star_omitted_coface_count
            << " h0="
            << (result.conditional_horizontal_h0_retraction_certified
                    ? "certified"
                    : "not-certified")
            << '\n';

  const ExactLevel omitted_noop_level = level(
      fixture::
          moment_curve_k3_first_incidence_omitted_noop_batch_n9_squared_level);
  const auto omitted_noop_batch = std::find_if(
      result.batch_group_audits.begin(),
      result.batch_group_audits.end(),
      [&omitted_noop_level](const auto& audit) {
        return audit.squared_level == omitted_noop_level;
      });
  const auto expected_parent =
      morsehgp3d::contract::CanonicalId::from_lower_hex(
          fixture::
              moment_curve_k3_first_incidence_omitted_noop_batch_n9_parent_digest);
  check(
      omitted_noop_batch != result.batch_group_audits.end() &&
          omitted_noop_batch->exhaustive_gamma_q_r ==
              fixture::
                  moment_curve_k3_first_incidence_omitted_noop_batch_n9_q_r &&
          omitted_noop_batch->exhaustive_gamma_action ==
              ExactDirectStarH0RetractionAction::continuation &&
          omitted_noop_batch->retained_new_coface_count == 3U &&
          omitted_noop_batch->first_incidence_subflow_new_coface_count == 0U &&
          omitted_noop_batch->late_star_omitted_new_coface_count == 3U &&
          !omitted_noop_batch->first_incidence_subflow_group_present &&
          omitted_noop_batch
              ->first_incidence_subflow_group_omitted_as_certified_noop &&
          omitted_noop_batch->added_direct_core_facet_count == 0U &&
          omitted_noop_batch->exhaustive_gamma_added_point_ids.empty() &&
          omitted_noop_batch
              ->first_incidence_subflow_added_point_ids.empty() &&
          omitted_noop_batch->first_incidence_subflow_parent_partition_digest ==
              expected_parent &&
          omitted_noop_batch
              ->first_incidence_subflow_parent_partition_equal &&
          omitted_noop_batch
              ->first_incidence_subflow_direct_core_delta_equal &&
          omitted_noop_batch
              ->first_incidence_subflow_added_point_delta_equal,
      "the permanent 04 batch did not remain an omitted qR=1 no-op");

  bool omitted_noop_cofaces_exact = true;
  for (const auto& source_coface :
       fixture::moment_curve_k3_first_incidence_omitted_noop_batch_n9_cofaces) {
    const std::vector<PointId> expected{
        source_coface.begin(), source_coface.end()};
    const auto found = std::find_if(
        result.late_star_coface_audits.begin(),
        result.late_star_coface_audits.end(),
        [&expected](const auto& audit) {
          return audit.coface_point_ids == expected;
        });
    omitted_noop_cofaces_exact =
        omitted_noop_cofaces_exact &&
        found != result.late_star_coface_audits.end() &&
        found->squared_level == omitted_noop_level &&
        found->continuation_q_r_one && found->point_delta_empty &&
        found->no_present_or_future_h0_effect;
  }
  check(
      omitted_noop_cofaces_exact,
      "the permanent 0124/0134/0234 omitted cofaces lost their no-op audit");

  bool target_six_exact = true;
  for (const std::size_t source_added :
       fixture::moment_curve_k3_one_step_star_incompleteness_n9_silent_added_points) {
    std::vector<PointId> target{0U, 4U, 8U, source_added};
    std::sort(target.begin(), target.end());
    const auto found = std::find_if(
        result.omitted_coface_audits.begin(),
        result.omitted_coface_audits.end(),
        [&target](const auto& audit) {
          return audit.coface_point_ids == target;
        });
    target_six_exact =
        target_six_exact && found != result.omitted_coface_audits.end() &&
        found->squared_level ==
            level(fixture::
                      moment_curve_k3_one_step_star_incompleteness_n9_squared_level) &&
        found->absent_from_direct_star && found->non_gabriel_certified &&
        found->strict_prior_root_count == 1U &&
        found->continuation_q_r_one && found->point_delta_empty &&
        found->added_point_ids.empty() && found->no_present_or_future_h0_effect;
  }
  check(
      target_six_exact,
      "the six permanent 048x omissions lost their exact silent audit");

  const auto verification =
      verify_result(index, cloud, pipeline, 3U, budget, result);
  check(
      verification.source_pipeline_freshly_replayed &&
          verification.gamma_oracle_freshly_replayed &&
          verification.expected_result_freshly_rebuilt &&
          verification.observed_recursively_equal &&
          verification.h0_and_v2_identity_claims_separated &&
          verification.fresh_replay_certified &&
          verification.result_certified,
      "moment10000 n9/k3 did not close under a fresh recursive replay");
}

void test_hostile_public_identity_claim_is_rejected() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectPipeline pipeline = direct_pipeline(index, cloud, 2U);
  const auto budget = tight_budget(cloud.size(), 2U);
  auto hostile = build_result(index, cloud, pipeline, 2U, budget);
  hostile.contract_v2_identity_equivalence_claimed = true;
  const auto verification =
      verify_result(index, cloud, pipeline, 2U, budget, hostile);
  check(
      !verification.observed_recursively_equal &&
          !verification.fresh_replay_certified &&
          !verification.result_certified,
      "a hostile public v2 identity claim passed the fresh verifier");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 &&
      std::string_view{argv[1]} == "--moment-n9-only") {
    test_moment_n9_048_omissions_preserve_h0();
  } else {
    check(argc == 1, "unknown direct-star differential test argument");
    test_e5_silent_equal_level_batch();
    test_moment_n9_048_omissions_preserve_h0();
    test_hostile_public_identity_claim_is_rejected();
  }
  if (failures != 0) {
    std::cerr << failures
              << " direct-star H0 retraction differential test(s) failed\n";
    return 1;
  }
  std::cout << "All direct-star H0 retraction differential tests passed\n";
  return 0;
}
