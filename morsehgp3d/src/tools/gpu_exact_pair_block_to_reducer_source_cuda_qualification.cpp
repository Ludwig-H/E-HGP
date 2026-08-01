#include "morsehgp3d/gpu/exact_pair_block_to_direct_pair_terminal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/exact_pair_block_automatic_prune_recipe_catalog.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_forest_reduction.hpp"
#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_pipeline.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockToDirectPairTerminalBudget;
using morsehgp3d::gpu::ExactPairBlockAutomaticPruneRecipeCatalogBudget;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaConfig;
using morsehgp3d::gpu::ExactPairBlockTransactionalFrontierResidentCudaContext;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::hierarchy::ExactDirectSaddleArmSeedBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::spatial::CanonicalPointCloud;
namespace hierarchy = morsehgp3d::hierarchy;

#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unbound";
#endif

struct Options {
  std::size_t maximum_order{};
  bool require_complete{false};
};

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
  const auto parsed = std::from_chars(
      text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  bool order_seen = false;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--require-complete") {
      options.require_complete = true;
      continue;
    }
    if ((argument != "--K" && argument != "--maximum-order") ||
        order_seen || index + 1 >= argc) {
      throw std::invalid_argument(
          "usage: --K 5|10 [--require-complete]");
    }
    options.maximum_order = parse_size(argv[++index], "invalid --K");
    order_seen = true;
  }
  if (!order_seen ||
      (options.maximum_order != 5U && options.maximum_order != 10U) ||
      !options.require_complete) {
    throw std::invalid_argument(
        "full-chain qualification requires K equal to 5 or 10 and "
        "--require-complete");
  }
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> eight_clusters_points() {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<CertifiedPoint3> points;
  points.reserve(12U);
  for (std::size_t index = 0U; index < 12U; ++index) {
    const std::size_t cluster = index % 8U;
    const std::size_t local = index / 8U + 1U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const std::size_t permuted_y =
        (local * 40503U + cluster * 7919U) % 65536U;
    const std::size_t permuted_z =
        (local * 25717U + cluster * 104729U) % 65536U;
    points.push_back(CertifiedPoint3::from_binary64(
        center_x + static_cast<double>(local) * local_scale,
        center_y + static_cast<double>(permuted_y) * transverse_scale,
        center_z + static_cast<double>(permuted_z) * transverse_scale));
  }
  return points;
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
      maximum};
}

[[nodiscard]] ExactPairBlockAutomaticPruneRecipeCatalogBudget
automatic_recipe_catalog_budget() {
  constexpr std::size_t capacity = 4096U;
  return {capacity, capacity, capacity, capacity};
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] hierarchy::ExactDirectSparseFacetDescentStepBudget
descent_step_budget() {
  return {
      hierarchy::ExactDirectSparsePositiveFacetProbeBudget{65537U, 16384U},
      morsehgp3d::spatial::ExactLbvhTopKBudget{
          1000000U,
          1000000U,
          1000000U,
          1000000U,
          1000000U,
          16U,
          16384U},
      hierarchy::ExactDirectSparsePositiveFacetProbeBudget{65537U, 16384U},
  };
}

[[nodiscard]] hierarchy::ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 16384U;
  constexpr std::size_t key_capacity = 262144U;
  hierarchy::ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 4096U;
  budget.maximum_logical_output_entry_count = 65536U;
  budget.maximum_aggregate_closure_node_count = 1000000U;
  budget.maximum_aggregate_closure_step_call_count = 1000000U;
  budget.locator_budget = {
      capacity,
      capacity,
      key_capacity,
      capacity,
      capacity,
      65536U,
      capacity,
      capacity,
      key_capacity,
      65537U,
      65537U,
  };
  budget.closure_budget = {
      4096U,
      65536U,
      65536U,
      131073U,
      descent_step_budget(),
  };
  budget.quotient_budget = {
      4096U, capacity, capacity, 4096U, 65536U};
  return budget;
}

[[nodiscard]] hierarchy::ExactDirectMorseTerminalForestReductionConfig
forest_reduction_config() {
  hierarchy::ExactDirectMorseTerminalForestReductionConfig config;
  config.industrial_config.policy =
      hierarchy::ExactDirectMorseIndustrialPolicy::
          massive_external_streaming;
  config.industrial_config.memory_model = {
      64U,
      16U,
      16U,
      8U,
      16U,
      16U,
      16U,
      4U,
      16U,
      8U,
      16U,
      2U,
  };
  config.industrial_config.chunk_budget = {
      1'000'000U, 256U, 4096U, 4096U, 4096U, 4096U};
  config.plan_budget = {
      4096U,
      4096U,
      4096U,
      16384U,
      4096U,
      16384U,
      1'000'000U,
      1'000'000U};
  config.forest_budget = forest_budget();
  config.execution_budget = {3U, 4096U, 16384U, 262144U, 16384U};
  config.forest_config.locator_config.external_authority_id =
      UINT64_C(0x4D485047503344);
  return config;
}

[[nodiscard]]
hierarchy::ExactDirectMorseVerticalTargetProposalPipelineBudget
vertical_target_pipeline_budget(
    const hierarchy::ExactDirectMorseForestJournalResult& forest) {
  constexpr std::size_t capacity = 16384U;
  constexpr std::size_t group_capacity = 4096U;
  constexpr std::size_t large_capacity = 1000000U;
  hierarchy::ExactDirectMorseVerticalTargetProposalPipelineBudget budget;

  auto& carrier = budget.session_budget.carrier_state_budget;
  carrier.maximum_forest_birth_record_scan_count = capacity;
  carrier.maximum_forest_node_scan_count = capacity;
  carrier.maximum_forest_batch_scan_count = capacity;
  carrier.maximum_forest_atomic_group_scan_count = capacity;
  carrier.maximum_forest_saddle_scan_count = capacity;
  carrier.maximum_forest_arm_binding_scan_count = capacity;
  carrier.maximum_forest_child_reference_scan_count = capacity;
  carrier.maximum_forest_final_root_scan_count = capacity;
  carrier.maximum_component_state_count = capacity;
  carrier.maximum_node_marker_state_count = capacity;
  carrier.maximum_index_entry_count = capacity;
  carrier.maximum_group_carrier_scratch_count = capacity;
  carrier.maximum_group_prior_root_scratch_count = capacity;
  carrier.maximum_parent_hop_count = large_capacity;
  carrier.maximum_exact_level_comparison_count = large_capacity;
  carrier.maximum_single_exact_level_integer_bit_count = capacity;
  carrier.maximum_logical_output_entry_count = 65536U;

  budget.session_budget.locator_budget =
      forest.requested_budget.locator_budget;
  budget.session_budget.maximum_replayed_global_batch_count = capacity;
  budget.session_budget.maximum_replayed_locator_union_count = capacity;
  budget.session_budget.maximum_replayed_locator_binding_count = capacity;
  budget.session_budget.maximum_batch_group_plan_count = capacity;
  budget.session_budget.maximum_batch_group_carrier_reference_count =
      capacity;
  budget.session_budget.maximum_batch_group_prior_root_reference_count =
      capacity;
  budget.session_budget.maximum_batch_locator_union_scratch_count =
      capacity;
  budget.session_budget.maximum_batch_locator_binding_scratch_count =
      capacity;

  auto& plan = budget.facet_plan_budget;
  plan.maximum_group_saddle_scan_count = capacity;
  plan.maximum_group_arm_binding_scan_count = capacity;
  plan.maximum_binding_sort_scratch_count = group_capacity;
  plan.maximum_representative_binding_count = group_capacity;
  plan.maximum_projected_target_facet_reference_count = 65536U;
  plan.maximum_distinct_target_facet_count = group_capacity;
  plan.maximum_sort_comparison_count = large_capacity;
  plan.maximum_target_facet_lookup_comparison_count = large_capacity;
  plan.maximum_retained_key_point_reference_count = 131072U;
  plan.maximum_logical_output_entry_count = 131072U;

  auto& closure = budget.closure_budget;
  closure.maximum_canonical_key_scan_count = group_capacity;
  closure.maximum_terminal_summary_count = group_capacity;
  closure.maximum_terminal_key_point_reference_count = 131072U;
  closure.maximum_carrier_cut_lookup_count = group_capacity;
  closure.maximum_logical_output_entry_count = 65536U;
  closure.closure_budget = {
      group_capacity,
      65536U,
      65536U,
      131073U,
      descent_step_budget(),
  };

  auto& proposal = budget.proposal_budget;
  proposal.maximum_source_saddle_revalidation_count = capacity;
  proposal.maximum_source_binding_revalidation_count = capacity;
  proposal.maximum_source_key_lookup_comparison_count = large_capacity;
  proposal.maximum_closure_summary_scan_count = group_capacity;
  proposal.maximum_positive_terminal_probe_count = group_capacity;
  proposal.maximum_positive_terminal_probe_slot_visit_count =
      large_capacity;
  proposal.maximum_positive_terminal_probe_parent_hop_count =
      large_capacity;
  proposal.positive_terminal_probe_budget = {65537U, capacity};
  proposal.maximum_carrier_entry_revalidation_count = group_capacity;
  proposal.maximum_target_node_lookup_count = group_capacity;
  proposal.maximum_exact_level_comparison_count = large_capacity;
  proposal.maximum_single_exact_level_integer_bit_count = capacity;
  proposal.maximum_proposal_count = group_capacity;
  proposal.maximum_logical_output_entry_count = 65536U;

  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_atomic_group_scan_count = capacity;
  budget.maximum_referenced_target_order_count = 9U;
  budget.maximum_target_order_lookup_count = large_capacity;
  budget.maximum_preflight_facet_plan_count = capacity;
  budget.maximum_executed_facet_plan_count = capacity;
  budget.maximum_replay_advance_count = capacity;
  budget.maximum_closure_build_count = capacity;
  budget.maximum_proposal_adapter_count = capacity;
  budget.maximum_aggregate_representative_count = capacity;
  budget.maximum_aggregate_projected_target_facet_reference_count =
      163840U;
  budget.maximum_aggregate_distinct_target_facet_count = 163840U;
  budget.maximum_aggregate_retained_key_point_reference_count = 2000000U;
  budget.maximum_aggregate_plan_logical_output_entry_count = large_capacity;
  budget.maximum_aggregate_closure_terminal_summary_count = 163840U;
  budget.maximum_aggregate_proposal_count = capacity;
  budget.maximum_session_audit_count = 9U;
  budget.maximum_group_audit_count = capacity;
  budget.maximum_logical_output_entry_count = 65536U;
  return budget;
}

[[nodiscard]] hierarchy::ExactDirectMorseVerticalBudget vertical_budget() {
  constexpr std::size_t capacity = 16384U;
  return {
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
      capacity,
      capacity * capacity,
  };
}

[[nodiscard]] std::uint64_t elapsed_nanoseconds(
    Clock::time_point begin,
    Clock::time_point end) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());
}

int run(const Options& options) {
  const Clock::time_point total_begin = Clock::now();
  const auto points = eight_clusters_points();
  const Clock::time_point generation_end = Clock::now();
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  const Clock::time_point canonical_end = Clock::now();

  MortonLbvhBuildContext builder{cloud.size()};
  auto build = builder.build(cloud);
  const Clock::time_point lbvh_end = Clock::now();
  if (!build.cuda_qualified_build()) {
    throw std::runtime_error(
        "the full-chain qualification requires a CUDA-certified LBVH");
  }
  const auto& index = build.certified_index();
  const Clock::time_point recipe_catalog_begin = lbvh_end;
  auto recipe_catalog = morsehgp3d::gpu::
      build_exact_pair_block_automatic_prune_recipe_catalog(
          index,
          cloud,
          options.maximum_order + 1U,
          automatic_recipe_catalog_budget());
  const Clock::time_point recipe_catalog_end = Clock::now();
  if (!recipe_catalog.complete()) {
    throw std::runtime_error(
        "the exact automatic prune-recipe catalog did not close");
  }
  const Clock::time_point scheduler_setup_begin = recipe_catalog_end;
  auto traversal = builder.release_device_traversal_lease(build);
  auto scheduler = ExactPairBlockTransactionalFrontierResidentCudaContext::
      start(
          std::move(traversal),
          index,
          cloud,
          ExactPairBlockTransactionalFrontierResidentCudaConfig{
              options.maximum_order + 1U, 16U, 8U, 16U, 4U});
  const Clock::time_point scheduler_begin = Clock::now();
  auto cut = scheduler.run(recipe_catalog.recipes);
  const Clock::time_point scheduler_end = Clock::now();
  const Clock::time_point cut_validation_begin = Clock::now();
  const bool cut_certified =
      cut.qualified_cuda_terminal_authority() && cut.complete() &&
      cut.validated_for(index, cloud);
  const Clock::time_point cut_validation_end = Clock::now();
  const auto cut_audit = cut.audit();
  const auto recipe_catalog_audit = recipe_catalog.audit;
  const bool recipe_catalog_certified =
      recipe_catalog.complete() && !recipe_catalog.recipes.empty() &&
      recipe_catalog_audit.pruned_unordered_pair_mass != 0U &&
      recipe_catalog_audit.terminal_unordered_pair_mass != 0U &&
      cut_audit.submitted_prune_recipe_count ==
          recipe_catalog_audit.prune_recipe_count &&
      cut_audit.matched_prune_recipe_count ==
          recipe_catalog_audit.prune_recipe_count &&
      cut_audit.unused_prune_recipe_count == 0U &&
      cut_audit.certified_prune_count ==
          recipe_catalog_audit.prune_recipe_count;

  const Clock::time_point pair_adapter_begin = Clock::now();
  auto pair_attempt =
      morsehgp3d::gpu::build_exact_pair_block_to_direct_pair_terminal(
          index,
          cloud,
          options.maximum_order,
          ExactPairBlockToDirectPairTerminalBudget::unlimited(),
          std::move(cut));
  const Clock::time_point pair_adapter_end = Clock::now();
  const bool pair_authority_certified =
      pair_attempt.complete() &&
      pair_attempt.audit.qualified_cuda_execution &&
      !pair_attempt.audit.host_fake_execution;
  if (!pair_attempt.complete()) {
    throw std::runtime_error(
        "the CUDA pair cut did not produce a neutral terminal authority");
  }

  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  const Clock::time_point higher_begin = Clock::now();
  ExactHigherSupportTerminalSession higher_session{
      index, cloud, options.maximum_order, higher_budget, 256U};
  const bool higher_terminal =
      higher_session.run_to_terminal() ==
      ExactHigherSupportTerminalRunStatus::terminal;
  auto higher_authority = std::move(higher_session).seal();
  const Clock::time_point higher_end = Clock::now();

  const Clock::time_point bridge_begin = Clock::now();
  auto bridge_result = morsehgp3d::hierarchy::
      build_exact_direct_morse_terminal_reducer_source_bridge(
          index,
          cloud,
          options.maximum_order,
          higher_budget,
          unlimited_seed_budget(),
          2U,
          std::move(*pair_attempt.authority),
          std::move(higher_authority));
  const Clock::time_point bridge_end = Clock::now();
  bool provider_replay_certified = bridge_result.certified();
  std::size_t visited_batch_count = 0U;
  std::size_t source_batch_count = 0U;
  std::size_t normalized_event_count = 0U;
  std::size_t source_seed_count = 0U;
  std::string pair_cloud_digest(64U, '0');
  std::string pair_lbvh_digest(64U, '0');
  std::string pair_output_digest(64U, '0');
  std::string pair_semantic_digest(64U, '0');
  std::string higher_semantic_digest(64U, '0');
  std::string higher_output_chain_digest(64U, '0');
  std::string higher_checkpoint_digest(64U, '0');
  std::string normalized_terminal_output_digest(64U, '0');
  std::string reducer_source_manifest_digest(64U, '0');
  Clock::time_point provider_replay_begin = bridge_end;
  if (bridge_result.bridge != nullptr) {
    source_batch_count = bridge_result.bridge->source_manifest().batch_count;
    normalized_event_count =
        bridge_result.bridge->direct_support_facade().events.size();
    source_seed_count =
        bridge_result.bridge->saddle_arm_seed_journal().arm_seeds.size();
    const auto& certificate =
        bridge_result.bridge->direct_support_facade().certificate;
    pair_cloud_digest = certificate.pair_canonical_cloud_digest.to_lower_hex();
    pair_lbvh_digest = certificate.pair_lbvh_digest.to_lower_hex();
    pair_output_digest =
        certificate.pair_terminal_output_digest.to_lower_hex();
    pair_semantic_digest = certificate.pair_semantic_digest.to_lower_hex();
    higher_semantic_digest =
        certificate.higher_semantic_digest.to_lower_hex();
    higher_output_chain_digest =
        certificate.higher_output_chain_digest.to_lower_hex();
    higher_checkpoint_digest =
        certificate.higher_terminal_checkpoint_digest.to_lower_hex();
    normalized_terminal_output_digest =
        certificate.normalized_terminal_output_digest.to_lower_hex();
    reducer_source_manifest_digest =
        bridge_result.bridge->source_manifest().manifest_digest.to_lower_hex();
    provider_replay_begin = Clock::now();
    auto provider = bridge_result.bridge->source_provider();
    for (std::size_t batch = 0U; batch < source_batch_count; ++batch) {
      auto verify_window = [&](const auto& window) {
        ++visited_batch_count;
        return window.certified_relative_to(
            bridge_result.bridge->source_manifest());
      };
      provider_replay_certified = provider_replay_certified &&
          provider(batch, verify_window) ==
              morsehgp3d::hierarchy::
                  ExactDirectMorseForestSourceBatchVisitDecision::
                      complete_synchronous_visit;
    }
  }
  provider_replay_certified = provider_replay_certified &&
      visited_batch_count == source_batch_count;
  const Clock::time_point provider_replay_end = Clock::now();

  const Clock::time_point forest_reduction_begin = provider_replay_end;
  std::optional<hierarchy::ExactDirectMorseTerminalForestReductionResult>
      forest_reduction;
  if (bridge_result.bridge != nullptr) {
    forest_reduction.emplace(
        hierarchy::reduce_exact_direct_morse_terminal_source_to_forest(
            index,
            cloud,
            *bridge_result.bridge,
            forest_reduction_config()));
  }
  const Clock::time_point forest_reduction_end = Clock::now();

  const Clock::time_point vertical_target_pipeline_begin =
      forest_reduction_end;
  std::optional<
      hierarchy::ExactDirectMorseVerticalTargetProposalPipelineResult>
      vertical_target_pipeline;
  if (forest_reduction.has_value() &&
      forest_reduction->certified_reduction() &&
      forest_reduction->forest.has_value()) {
    vertical_target_pipeline.emplace(
        hierarchy::build_exact_direct_morse_vertical_target_proposal_pipeline(
            *forest_reduction->forest,
            index,
            cloud,
            vertical_target_pipeline_budget(*forest_reduction->forest)));
  }
  const Clock::time_point vertical_target_pipeline_end = Clock::now();

  const Clock::time_point vertical_journal_begin =
      vertical_target_pipeline_end;
  std::optional<hierarchy::ExactDirectMorseVerticalJournalResult>
      vertical_journal;
  if (forest_reduction.has_value() &&
      forest_reduction->certified_reduction() &&
      forest_reduction->forest.has_value() &&
      vertical_target_pipeline.has_value() &&
      vertical_target_pipeline->certified_multiorder_target_proposals()) {
    vertical_journal.emplace(
        hierarchy::build_exact_direct_morse_vertical_journal(
            *forest_reduction->forest,
            std::span<
                const hierarchy::ExactDirectMorseVerticalTargetProposal>{
                vertical_target_pipeline->proposals},
            vertical_budget(),
            hierarchy::ExactDirectMorseVerticalConfig{
                vertical_target_pipeline->external_target_authority_id}));
  }
  const Clock::time_point vertical_journal_end = Clock::now();

  const hierarchy::ExactDirectMorseTerminalForestReductionAudit
      empty_forest_audit{};
  const auto& forest_audit = forest_reduction.has_value()
      ? forest_reduction->audit
      : empty_forest_audit;
  const hierarchy::ExactDirectMorseVerticalTargetProposalPipelineResult
      empty_vertical_target_pipeline{};
  const auto& vertical_pipeline = vertical_target_pipeline.has_value()
      ? *vertical_target_pipeline
      : empty_vertical_target_pipeline;
  const hierarchy::ExactDirectMorseVerticalJournalResult
      empty_vertical_journal{};
  const auto& vertical = vertical_journal.has_value()
      ? *vertical_journal
      : empty_vertical_journal;
  const bool forest_reduction_certified =
      forest_reduction.has_value() &&
      forest_reduction->certified_reduction();
  const bool vertical_target_pipeline_certified =
      vertical_target_pipeline.has_value() &&
      vertical_target_pipeline->certified_multiorder_target_proposals() &&
      vertical_target_pipeline->required_referenced_target_order_count != 0U &&
      vertical_target_pipeline->required_group_count != 0U &&
      !vertical_target_pipeline->proposals.empty();
  const bool vertical_journal_certified =
      vertical_journal.has_value() &&
      vertical_journal->certified_conditional_vertical_candidate() &&
      vertical_target_pipeline_certified &&
      vertical_journal->counters.expected_label_count ==
          vertical_target_pipeline->proposals.size() &&
      vertical_journal->counters.missing_label_count == 0U &&
      vertical_journal->counters.unresolved_label_count <=
          vertical_journal->counters.expected_label_count &&
      vertical_journal->counters.resolved_label_count ==
          vertical_journal->counters.expected_label_count -
              vertical_journal->counters.unresolved_label_count &&
      vertical_journal->counters.complete_group_count +
              vertical_journal->counters.partial_group_count ==
          vertical_target_pipeline->required_group_count &&
      !vertical_journal->external_target_authority_replayed &&
      !vertical_journal->all_naturality_squares_replayed &&
      !vertical_journal->vertical_maps_complete &&
      !vertical_journal->public_status_claimed;
  const bool qualified = recipe_catalog_certified && cut_certified &&
      pair_authority_certified && higher_terminal && bridge_result.certified() &&
      provider_replay_certified && forest_reduction_certified &&
      vertical_target_pipeline_certified && vertical_journal_certified;

  std::cout
      << "{\"schema\":\"morsehgp3d.phase15.transactional_pair_to_"
         "conditional_forest_qualification.v4\","
      << "\"backend\":\"cuda_g4_plus_reference_cpu\","
      << "\"git_sha\":\"" << kGitSha << "\","
      << "\"profile\":\"hgp_reduced\","
      << "\"mode\":\"automatic_exact_prune_recipes_to_complete_direct_"
         "terminal_source_to_conditional_forest_to_forest_relative_"
         "multiorder_vertical_targets_and_zero_missing_label_vertical_"
         "journal\","
      << "\"public_status\":\"not_claimed\","
      << "\"fixture\":\"eight_clusters_12\","
      << "\"point_count\":" << cloud.size() << ','
      << "\"maximum_order\":" << options.maximum_order << ','
      << "\"maximum_closed_rank\":"
      << pair_attempt.audit.maximum_closed_rank << ','
      << "\"require_complete\":"
      << (options.require_complete ? "true" : "false") << ','
      << "\"qualified\":" << (qualified ? "true" : "false") << ','
      << "\"recipe_catalog_certified\":"
      << (recipe_catalog_certified ? "true" : "false") << ','
      << "\"cut_certified\":"
      << (cut_certified ? "true" : "false") << ','
      << "\"pair_authority_certified\":"
      << (pair_authority_certified ? "true" : "false") << ','
      << "\"higher_terminal\":"
      << (higher_terminal ? "true" : "false") << ','
      << "\"bridge_certified\":"
      << (bridge_result.certified() ? "true" : "false") << ','
      << "\"provider_replay_certified\":"
      << (provider_replay_certified ? "true" : "false") << ','
      << "\"forest_reduction_certified\":"
      << (forest_reduction_certified ? "true" : "false") << ','
      << "\"vertical_target_pipeline_certified\":"
      << (vertical_target_pipeline_certified ? "true" : "false") << ','
      << "\"vertical_journal_certified\":"
      << (vertical_journal_certified ? "true" : "false") << ','
      << "\"automatic_recipe_catalog\":{\"decision\":"
      << static_cast<unsigned>(recipe_catalog_audit.decision)
      << ",\"product_visits\":"
      << recipe_catalog_audit.product_visit_count
      << ",\"maximum_frontier_blocks\":"
      << recipe_catalog_audit.maximum_frontier_block_count
      << ",\"receipt_attempts\":"
      << recipe_catalog_audit.automatic_receipt_attempt_count
      << ",\"certified_receipts\":"
      << recipe_catalog_audit.automatic_receipt_certified_count
      << ",\"inconclusive_receipts\":"
      << recipe_catalog_audit.automatic_receipt_inconclusive_count
      << ",\"automatic_search_node_visits\":"
      << recipe_catalog_audit.automatic_search_node_visit_count
      << ",\"exact_phi_aabb_maximum_evaluations\":"
      << recipe_catalog_audit.exact_phi_aabb_maximum_evaluation_count
      << ",\"recipes\":" << recipe_catalog_audit.prune_recipe_count
      << ",\"terminal_pairs_without_payload\":"
      << recipe_catalog_audit.terminal_pair_count
      << ",\"pruned_mass\":"
      << recipe_catalog_audit.pruned_unordered_pair_mass
      << ",\"terminal_mass\":"
      << recipe_catalog_audit.terminal_unordered_pair_mass
      << ",\"mass_closed\":"
      << (recipe_catalog_audit.product_partition_mass_closed
              ? "true"
              : "false")
      << "},"
      << "\"pair_cut\":{\"universe\":"
      << cut_audit.unordered_pair_universe_mass
      << ",\"pruned\":" << cut_audit.pruned_unordered_pair_mass
      << ",\"terminal\":" << cut_audit.terminal_unordered_pair_mass
      << ",\"submitted_recipes\":"
      << cut_audit.submitted_prune_recipe_count
      << ",\"matched_recipes\":"
      << cut_audit.matched_prune_recipe_count
      << ",\"unused_recipes\":"
      << cut_audit.unused_prune_recipe_count
      << ",\"certified_prunes\":"
      << cut_audit.certified_prune_count
      << ",\"kernel_launches\":" << cut_audit.kernel_launch_count
      << ",\"synchronizations\":" << cut_audit.synchronization_count
      << ",\"kernel_elapsed_nanoseconds\":"
      << cut_audit.kernel_elapsed_nanoseconds
      << ",\"cuda_device\":" << cut_audit.cuda_device
      << ",\"serial_device_reference\":"
      << (cut_audit.serial_device_reference ? "true" : "false")
      << ",\"scale_eligible\":"
      << (cut_audit.scale_eligible ? "true" : "false") << "},"
      << "\"pair_classification\":{\"terminal_pairs\":"
      << pair_attempt.audit.classification_terminal_count
      << ",\"above_rank\":" << pair_attempt.audit.above_rank_count
      << ",\"records\":" << pair_attempt.audit.emitted_record_count
      << ",\"node_visits\":"
      << pair_attempt.audit.classification_node_visit_count << "},"
      << "\"reducer_source\":{\"events\":"
      << normalized_event_count
      << ",\"seeds\":" << source_seed_count
      << ",\"batches\":" << source_batch_count
      << ",\"visited_batches\":" << visited_batch_count << "},"
      << "\"forest_reduction\":{\"decision\":"
      << static_cast<unsigned>(forest_audit.decision)
      << ",\"plan_decision\":"
      << static_cast<unsigned>(forest_audit.plan_decision)
      << ",\"last_preparation_decision\":"
      << static_cast<unsigned>(forest_audit.last_preparation_decision)
      << ",\"last_live_commit_decision\":"
      << static_cast<unsigned>(forest_audit.last_live_commit_decision)
      << ",\"last_reducer_fold_decision\":"
      << static_cast<unsigned>(forest_audit.last_reducer_fold_decision)
      << ",\"plan_lanes\":" << forest_audit.plan_lane_count
      << ",\"prepared_tickets\":"
      << forest_audit.prepared_ticket_count
      << ",\"committed_batches\":"
      << forest_audit.committed_batch_count
      << ",\"resolved_keys\":"
      << forest_audit.aggregate_resolved_key_count
      << ",\"arm_joins\":" << forest_audit.aggregate_arm_join_count
      << ",\"maximum_transient_closure_nodes\":"
      << forest_audit.maximum_transient_closure_node_count
      << ",\"birth_records\":"
      << forest_audit.forest_birth_record_count
      << ",\"saddles\":" << forest_audit.forest_saddle_record_count
      << ",\"atomic_groups\":"
      << forest_audit.forest_atomic_group_count
      << ",\"nodes\":" << forest_audit.forest_node_count
      << ",\"final_roots\":" << forest_audit.forest_final_root_count
      << ",\"logical_output_entries\":"
      << forest_audit.forest_logical_output_entry_count
      << ",\"conditional_h0_candidate\":"
      << (forest_audit.forest_conditional_h0_candidate_certified
              ? "true"
              : "false")
      << "},"
      << "\"vertical_target_pipeline\":{\"decision\":"
      << static_cast<unsigned>(vertical_pipeline.decision)
      << ",\"source_batches_scanned\":"
      << vertical_pipeline.counters.source_batch_scan_count
      << ",\"source_groups_scanned\":"
      << vertical_pipeline.counters.source_atomic_group_scan_count
      << ",\"target_order_lookups\":"
      << vertical_pipeline.counters.target_order_lookup_count
      << ",\"required_sessions\":"
      << vertical_pipeline.required_referenced_target_order_count
      << ",\"initialized_sessions\":"
      << vertical_pipeline.counters.initialized_session_count
      << ",\"session_audits\":"
      << vertical_pipeline.session_audits.size()
      << ",\"required_groups\":"
      << vertical_pipeline.required_group_count
      << ",\"preflight_plans\":"
      << vertical_pipeline.counters.preflight_facet_plan_count
      << ",\"executed_plans\":"
      << vertical_pipeline.counters.executed_facet_plan_count
      << ",\"replay_advances\":"
      << vertical_pipeline.counters.replay_advance_count
      << ",\"closure_builds\":"
      << vertical_pipeline.counters.closure_build_count
      << ",\"proposal_adapters\":"
      << vertical_pipeline.counters.proposal_adapter_count
      << ",\"group_audits\":"
      << vertical_pipeline.group_audits.size()
      << ",\"representatives\":"
      << vertical_pipeline.counters.representative_count
      << ",\"projected_target_facets\":"
      << vertical_pipeline.counters.projected_target_facet_reference_count
      << ",\"distinct_target_facets\":"
      << vertical_pipeline.counters.distinct_target_facet_count
      << ",\"retained_key_point_references\":"
      << vertical_pipeline.counters.retained_key_point_reference_count
      << ",\"closure_terminal_summaries\":"
      << vertical_pipeline.counters.closure_terminal_summary_count
      << ",\"closure_unresolved_terminals\":"
      << vertical_pipeline.counters.closure_unresolved_terminal_count
      << ",\"closure_active_latent_terminals\":"
      << vertical_pipeline.counters.closure_active_latent_terminal_count
      << ",\"closure_resolved_terminals\":"
      << vertical_pipeline.counters.closure_resolved_terminal_count
      << ",\"proposals\":" << vertical_pipeline.proposals.size()
      << ",\"unresolved_proposals\":"
      << vertical_pipeline.counters.unresolved_proposal_count
      << ",\"resolved_proposals\":"
      << vertical_pipeline.counters.resolved_proposal_count
      << ",\"equal_level_same_target_order_groups\":"
      << vertical_pipeline.counters
             .equal_level_same_target_order_group_count
      << ",\"matching_canonical_point_namespace_required\":"
      << (vertical_pipeline.matching_canonical_point_namespace_required
              ? "true"
              : "false")
      << ",\"forest_to_cloud_namespace_identity_certified\":"
      << (vertical_pipeline.forest_to_cloud_namespace_identity_certified
              ? "true"
              : "false")
      << ",\"forest_relative_only\":"
      << (vertical_pipeline.forest_relative_only ? "true" : "false")
      << ",\"global_forbidden_structure_materialized\":"
      << (vertical_pipeline
                  .global_facet_coface_incidence_cell_gamma_or_delaunay_materialized
              ? "true"
              : "false")
      << ",\"external_target_authority_replayed\":"
      << (vertical_pipeline.external_target_authority_replayed
              ? "true"
              : "false")
      << ",\"vertical_maps_complete\":"
      << (vertical_pipeline.vertical_maps_complete ? "true" : "false")
      << "},"
      << "\"vertical_journal\":{\"decision\":"
      << static_cast<unsigned>(vertical.decision)
      << ",\"expected_labels\":"
      << vertical.counters.expected_label_count
      << ",\"missing_labels\":"
      << vertical.counters.missing_label_count
      << ",\"unresolved_labels\":"
      << vertical.counters.unresolved_label_count
      << ",\"resolved_labels\":"
      << vertical.counters.resolved_label_count
      << ",\"partial_groups\":"
      << vertical.counters.partial_group_count
      << ",\"complete_groups\":"
      << vertical.counters.complete_group_count
      << ",\"external_target_authority_replayed\":"
      << (vertical.external_target_authority_replayed ? "true" : "false")
      << ",\"all_naturality_squares_replayed\":"
      << (vertical.all_naturality_squares_replayed ? "true" : "false")
      << ",\"vertical_maps_complete\":"
      << (vertical.vertical_maps_complete ? "true" : "false")
      << ",\"public_status_claimed\":"
      << (vertical.public_status_claimed ? "true" : "false") << "},"
      << "\"digests\":{\"submitted_recipe_fnv1a\":"
      << cut_audit.submitted_recipe_digest
      << ",\"final_cut_fnv1a\":" << cut_audit.final_cut_digest
      << ",\"pair_cloud_sha256\":\"" << pair_cloud_digest
      << "\",\"pair_lbvh_sha256\":\"" << pair_lbvh_digest
      << "\",\"pair_output_sha256\":\"" << pair_output_digest
      << "\",\"pair_semantic_sha256\":\"" << pair_semantic_digest
      << "\",\"higher_semantic_sha256\":\"" << higher_semantic_digest
      << "\",\"higher_output_chain_sha256\":\""
      << higher_output_chain_digest
      << "\",\"higher_checkpoint_sha256\":\""
      << higher_checkpoint_digest
      << "\",\"normalized_terminal_output_sha256\":\""
      << normalized_terminal_output_digest
      << "\",\"reducer_source_manifest_sha256\":\""
      << reducer_source_manifest_digest << "\"},"
      << "\"timings_nanoseconds\":{\"generation\":"
      << elapsed_nanoseconds(total_begin, generation_end)
      << ",\"canonicalization\":"
      << elapsed_nanoseconds(generation_end, canonical_end)
      << ",\"lbvh_build\":"
      << elapsed_nanoseconds(canonical_end, lbvh_end)
      << ",\"automatic_recipe_catalog_wall\":"
      << elapsed_nanoseconds(recipe_catalog_begin, recipe_catalog_end)
      << ",\"scheduler_setup_wall\":"
      << elapsed_nanoseconds(scheduler_setup_begin, scheduler_begin)
      << ",\"scheduler_wall\":"
      << elapsed_nanoseconds(scheduler_begin, scheduler_end)
      << ",\"cut_validation_wall\":"
      << elapsed_nanoseconds(cut_validation_begin, cut_validation_end)
      << ",\"pair_adapter_wall\":"
      << elapsed_nanoseconds(pair_adapter_begin, pair_adapter_end)
      << ",\"higher_support_wall\":"
      << elapsed_nanoseconds(higher_begin, higher_end)
      << ",\"bridge_wall\":"
      << elapsed_nanoseconds(bridge_begin, bridge_end)
      << ",\"bridge_output_inspection_wall\":"
      << elapsed_nanoseconds(bridge_end, provider_replay_begin)
      << ",\"provider_replay_wall\":"
      << elapsed_nanoseconds(provider_replay_begin, provider_replay_end)
      << ",\"forest_reduction_wrapper_wall\":"
      << elapsed_nanoseconds(forest_reduction_begin, forest_reduction_end)
      << ",\"forest_plan_wall\":"
      << forest_audit.timings.plan_wall_nanoseconds
      << ",\"forest_reducer_setup_wall\":"
      << forest_audit.timings.reducer_setup_wall_nanoseconds
      << ",\"forest_reducer_stream_wall\":"
      << forest_audit.timings.reducer_stream_wall_nanoseconds
      << ",\"forest_finish_wall\":"
      << forest_audit.timings.forest_finish_wall_nanoseconds
      << ",\"forest_reduction_internal_total_wall\":"
      << forest_audit.timings.total_wall_nanoseconds
      << ",\"vertical_target_proposal_pipeline_wall\":"
      << elapsed_nanoseconds(
             vertical_target_pipeline_begin,
             vertical_target_pipeline_end)
      << ",\"vertical_journal_wall\":"
      << elapsed_nanoseconds(vertical_journal_begin, vertical_journal_end)
      << ",\"total\":"
      << elapsed_nanoseconds(total_begin, vertical_journal_end) << "},"
      << "\"claims\":{\"ordinary_or_higher_order_delaunay\":false,"
         "\"global_pair_matrix\":false,\"hierarchy_reduction\":true,"
         "\"conditional_h0_only\":true,"
         "\"forest_relative_vertical_target_proposals\":true,"
         "\"vertical_target_authority\":false,"
         "\"vertical_maps_complete\":false,\"public_exact\":false},"
      << "\"qualified_scope\":\"automatic_exact_prune_cut_to_terminal_"
         "direct_supports_bounded_conditional_h0_forest_forest_relative_"
         "multiorder_vertical_target_proposals_and_zero_missing_label_"
         "conditional_vertical_journal_only\"}"
      << '\n';
  return qualified ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "full-chain qualification failed: " << error.what() << '\n';
    return 1;
  }
}
