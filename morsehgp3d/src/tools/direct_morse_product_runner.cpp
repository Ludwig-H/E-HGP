#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"
#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"
#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/sparse_anchored_pair_session.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

constexpr std::uint64_t locator_authority_id =
    UINT64_C(0x4D485047503344);

struct Options {
  std::string family{"uniform_latin"};
  std::string mode{"resident_timed"};
  std::size_t point_count{};
  std::size_t maximum_order{10U};
  std::size_t support_work_budget{20'000U};
  std::size_t support_record_budget{4'096U};
  std::size_t higher_chunk_limit{1U};
  std::size_t downstream_record_budget{1'000'000U};
  std::size_t descent_work_budget{65'536U};
  std::uint64_t chunk_byte_budget{UINT64_C(1) << 30U};
  bool point_count_supplied{false};
};

struct Timings {
  double generation_ms{};
  double canonicalization_ms{};
  double lbvh_ms{};
  double pair_support_ms{};
  double higher_support_ms{};
  double terminal_facade_ms{};
  double event_journal_ms{};
  double saddle_seed_journal_ms{};
  double batch_plan_ms{};
  double reducer_setup_ms{};
  double reducer_stream_ms{};
  double forest_finish_ms{};
  double total_ms{};
};

struct Report {
  Options options;
  Timings timings;
  std::string terminal_stage{"not_started"};
  std::string stop_category{"none"};
  std::string stop_detail{"none"};
  bool pipeline_complete{false};
  bool budget_exhausted{false};
  bool scientific_result_materialized{false};
  bool conditional_h0_candidate_certified{false};
  bool architecture_audit_complete{true};
  bool no_forbidden_global_structure_materialized{true};

  std::string pair_status{"not_run"};
  std::string pair_stop_reason{"none"};
  std::string pair_source_kind{"not_run"};
  std::string pair_authority_kind{"not_run"};
  std::size_t pair_maximum_closed_rank{};
  ExactMortonGroupedAnchoredPairScheduleConfig pair_schedule_config{};
  ExactSparseAnchoredPairSessionAdvanceBudget pair_advance_budget{};
  ExactSparseAnchoredPairSessionTotalCapacity pair_total_capacity{};
  std::size_t pair_advance_calls{};
  std::size_t pair_schedule_advances{};
  std::size_t pair_orientation_checks{};
  std::size_t pair_reverse_or_self_orientation_skips{};
  std::size_t pair_prepared_groups{};
  std::size_t pair_completed_groups{};
  std::size_t pair_grouped_traversal_node_visits{};
  std::size_t pair_witness_subtree_node_visits{};
  std::size_t pair_grouped_common_node_visits{};
  std::size_t pair_anchor_subgroup_node_visits{};
  std::size_t pair_singleton_node_visits{};
  std::size_t pair_grouped_witness_slots{};
  std::size_t pair_grouped_inherited_witness_reuses{};
  std::size_t pair_grouped_traversal_exact_predicates{};
  std::size_t pair_fp64_filtered_negative_predicates{};
  std::size_t pair_fp64_filtered_positive_predicates{};
  std::size_t pair_exact_fallback_predicates{};
  std::size_t pair_floating_witness_order_preparations{};
  std::size_t pair_floating_witness_score_evaluations{};
  std::size_t pair_floating_witness_nonfinite_scores{};
  bool pair_floating_witness_order_requested{false};
  bool pair_floating_witness_order_effective_for_every_prepared_traversal{
      true};
  bool pair_fp64_filter_partition_certified{false};
  std::size_t pair_witness_subtree_exact_predicates{};
  std::size_t pair_witness_subtree_receipts{};
  std::size_t pair_witness_subtree_successes{};
  std::size_t pair_witness_subtree_fail_opens{};
  std::size_t pair_grouped_common_exact_predicates{};
  std::size_t pair_anchor_subgroup_exact_predicates{};
  std::size_t pair_singleton_exact_predicates{};
  std::size_t pair_grouped_strict_witness_discoveries{};
  std::size_t pair_grouped_diagonal_node_descents{};
  std::size_t pair_grouped_common_frontiers{};
  std::size_t pair_delegated_frontier_anchors{};
  std::size_t pair_prepared_anchor_subgroup_probes{};
  std::size_t pair_anchor_subgroup_witness_pool_entries{};
  std::size_t pair_query_facing_fallback_witness_pool_entries{};
  std::size_t pair_anchor_subgroup_splits{};
  std::size_t pair_query_subtree_splits{};
  std::size_t pair_anchor_subgroup_certified_prunes{};
  std::size_t pair_anchor_subgroup_certified_anchors{};
  std::size_t pair_prepared_singleton_fallbacks{};
  std::size_t pair_completed_singleton_fallbacks{};
  std::size_t pair_singleton_witness_pool_entries{};
  std::size_t pair_singleton_certified_prunes{};
  std::size_t pair_maximum_pending_anchor_subgroups{};
  std::size_t pair_triangular_block_pair_visits{};
  std::size_t pair_triangular_diagonal_splits{};
  std::size_t pair_triangular_oversized_anchor_splits{};
  std::size_t pair_triangular_consumer_query_splits{};
  std::size_t pair_triangular_self_pairs{};
  std::size_t pair_triangular_cross_blocks{};
  std::size_t pair_triangular_certified_cross_blocks{};
  std::size_t pair_triangular_certified_unordered_pairs{};
  std::size_t pair_triangular_opened_singleton_cross_blocks{};
  std::size_t pair_triangular_opened_unordered_pairs{};
  std::size_t pair_triangular_maximum_pending_block_pairs{};
  bool pair_triangular_partition_complete{false};
  bool pair_no_dynamic_dual_tree_or_pair_arena{true};
  std::size_t pair_authenticated_prunes{};
  std::size_t pair_authenticated_pruned_directed_pairs{};
  std::size_t pair_directed_pair_universe{};
  std::size_t pair_admitted_candidates{};
  std::size_t pair_classification_advances{};
  std::size_t pair_classification_node_visits{};
  std::size_t pair_classification_terminals{};
  std::size_t pair_above_rank{};
  std::size_t pair_output_records{};
  std::size_t pair_output_point_id_references{};
  std::size_t pair_local_budget_exhaustions{};
  std::size_t pair_total_capacity_exhaustions{};
  std::size_t pair_maximum_live_candidates{};
  std::size_t pair_accepted_events{};
  std::size_t pair_extra_shell_diagnostics{};
  bool pair_directed_coverage_certified{false};
  bool pair_orientation_partition_certified{false};
  bool pair_classification_partition_certified{false};
  bool pair_output_partition_certified{false};
  bool pair_records_certified{false};

  std::string higher_status{"not_run"};
  std::string higher_stop_reason{"none"};
  std::size_t higher_work_units{};
  std::size_t higher_product_visits{};
  std::size_t higher_closed_ball_queries{};
  std::size_t higher_accepted_events{};
  std::size_t higher_extra_shell_diagnostics{};
  std::size_t higher_prune_certificates{};
  std::size_t higher_chunk_count{};
  std::string higher_authority_kind{"not_run"};

  bool terminal_catalog_certified{false};
  std::size_t canonical_point_count{};
  std::size_t effective_maximum_order{};
  std::size_t terminal_event_count{};
  std::size_t terminal_extra_shell_diagnostic_count{};
  std::size_t event_batch_count{};
  std::size_t event_role_count{};
  std::size_t saddle_family_count{};
  std::size_t arm_seed_count{};
  std::size_t industrial_chunk_count{};
  std::size_t plan_lane_count{};
  std::size_t committed_batch_count{};
  std::size_t prepared_ticket_count{};
  unsigned industrial_plan_decision{};
  unsigned plan_decision{};
  unsigned batch_execution_decision{};
  unsigned preparation_decision{};
  unsigned reducer_fold_decision{};
  unsigned live_commit_decision{};

  std::size_t forest_birth_count{};
  std::size_t forest_materialized_birth_count{};
  std::size_t forest_saddle_count{};
  std::size_t forest_atomic_group_count{};
  std::size_t forest_node_count{};
  std::size_t forest_materialized_node_count{};
  std::size_t forest_final_root_count{};
  std::size_t forest_logical_output_entry_count{};
  std::size_t forest_aggregate_closure_node_count{};
  std::size_t forest_aggregate_closure_step_call_count{};
};

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>{duration}.count();
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view description) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{description});
  }
  return left + right;
}

[[nodiscard]] bool three_sum_matches(
    std::size_t first,
    std::size_t second,
    std::size_t third,
    std::size_t expected) noexcept {
  return second <= std::numeric_limits<std::size_t>::max() - first &&
      third <= std::numeric_limits<std::size_t>::max() - first - second &&
      first + second + third == expected;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view description) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{description});
  }
  return left * right;
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    std::string_view option) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::invalid_argument(
        std::string{option} + " expects an unsigned integer");
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(
    std::string_view text,
    std::string_view option) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    throw std::invalid_argument(
        std::string{option} + " expects an unsigned integer");
  }
  return value;
}

void print_usage(std::ostream& output) {
  output
      << "usage: morsehgp3d_direct_morse_product_runner "
         "--point-count N [options]\n"
      << "  --mode resident_timed\n"
      << "  --family uniform_latin|eight_clusters\n"
      << "  --maximum-order K (alias: --K; 1 <= K <= 10)\n"
      << "  --support-work-budget N (cap for each P8l work axis)\n"
      << "  --support-record-budget N (P8l output-record cap)\n"
      << "  --higher-chunk-limit N\n"
      << "  --downstream-record-budget N\n"
      << "  --descent-work-budget N\n"
      << "  --chunk-byte-budget N\n"
      << "Default caps are fail-fast diagnostics, not a 50k "
         "qualification envelope.\n";
}

void parse_options(int argc, char** argv, Options& options) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--help" || option == "-h") {
      print_usage(std::cout);
      std::exit(0);
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{option} + " requires one value");
    }
    const std::string_view value{argv[++index]};
    if (option == "--point-count") {
      options.point_count = parse_size(value, option);
      options.point_count_supplied = true;
    } else if (option == "--family") {
      options.family = value;
    } else if (option == "--mode") {
      options.mode = value;
    } else if (option == "--maximum-order" || option == "--K") {
      options.maximum_order = parse_size(value, option);
    } else if (option == "--support-work-budget") {
      options.support_work_budget = parse_size(value, option);
    } else if (option == "--support-record-budget") {
      options.support_record_budget = parse_size(value, option);
    } else if (option == "--higher-chunk-limit") {
      options.higher_chunk_limit = parse_size(value, option);
    } else if (option == "--downstream-record-budget") {
      options.downstream_record_budget = parse_size(value, option);
    } else if (option == "--descent-work-budget") {
      options.descent_work_budget = parse_size(value, option);
    } else if (option == "--chunk-byte-budget") {
      options.chunk_byte_budget = parse_u64(value, option);
    } else {
      throw std::invalid_argument(
          "unknown option: " + std::string{option});
    }
  }
  if (!options.point_count_supplied || options.point_count == 0U) {
    throw std::invalid_argument(
        "--point-count must be supplied and strictly positive");
  }
  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  if (options.mode != "resident_timed") {
    throw std::invalid_argument(
        "this runner currently supports only --mode resident_timed");
  }
  if (options.maximum_order == 0U ||
      options.maximum_order >
          higher_support_maximum_requested_order) {
    throw std::invalid_argument(
        "--maximum-order must be in [1, 10]");
  }
  if (options.support_work_budget == 0U ||
      options.support_record_budget == 0U ||
      options.higher_chunk_limit == 0U ||
      options.downstream_record_budget == 0U ||
      options.descent_work_budget == 0U ||
      options.chunk_byte_budget == 0U) {
    throw std::invalid_argument(
        "all operational budgets must be strictly positive");
  }
}

[[nodiscard]] std::string_view pair_stop_reason_text(
    ExactSparseAnchoredPairSessionStopReason reason) {
  switch (reason) {
    case ExactSparseAnchoredPairSessionStopReason::none:
      return "none";
    case ExactSparseAnchoredPairSessionStopReason::schedule_advance_limit:
      return "schedule_advance_limit";
    case ExactSparseAnchoredPairSessionStopReason::orientation_check_limit:
      return "orientation_check_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        grouped_traversal_node_visit_limit:
      return "grouped_traversal_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        grouped_traversal_exact_predicate_limit:
      return "grouped_traversal_exact_predicate_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        classification_node_visit_limit:
      return "classification_node_visit_limit";
    case ExactSparseAnchoredPairSessionStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactSparseAnchoredPairSessionStopReason::
        total_schedule_advance_capacity:
      return "total_schedule_advance_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_orientation_check_capacity:
      return "total_orientation_check_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_grouped_traversal_node_visit_capacity:
      return "total_grouped_traversal_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_grouped_traversal_exact_predicate_capacity:
      return "total_grouped_traversal_exact_predicate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_admitted_candidate_capacity:
      return "total_admitted_candidate_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_classification_node_visit_capacity:
      return "total_classification_node_visit_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_output_record_capacity:
      return "total_output_record_capacity";
    case ExactSparseAnchoredPairSessionStopReason::
        total_output_point_id_reference_capacity:
      return "total_output_point_id_reference_capacity";
  }
  return "invalid";
}

[[nodiscard]] bool industrial_plan_budget_failure(
    ExactDirectMorseIndustrialPlanDecision decision) {
  return decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_resident_requirements_not_met ||
         decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_atomic_batch_exceeds_chunk_budget ||
         decision ==
             ExactDirectMorseIndustrialPlanDecision::
                 no_plan_chunk_count_budget_exhausted;
}

[[nodiscard]] bool batch_execution_budget_failure(
    ExactDirectSparseFacetDescentBatchExecutionDecision decision) {
  return decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_batch_budget_exhausted ||
         decision ==
             ExactDirectSparseFacetDescentBatchExecutionDecision::
                 no_execution_shared_closure_budget_exhausted;
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptResult
empty_proposal_transcript(
    std::size_t source_batch_index,
    const morsehgp3d::exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::array<ExactDirectSparseFacetTopKProposalRecord, 0U> records{};
  return build_exact_direct_sparse_facet_top_k_proposal_transcript(
      {source_batch_index,
       closed_batch_squared_level,
       locator.snapshot_stamp()},
      records,
      {0U, 0U, 0U, 0U, 0U});
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget make_seed_budget(
    const CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& facade,
    std::size_t downstream_cap) {
  const std::size_t event_count = facade.events.size();
  const std::size_t replay_bound = checked_add(
      checked_multiply(
          3U, cloud.size(), "seed replay bound overflow"),
      checked_multiply(
          5U, event_count, "seed replay bound overflow"),
      "seed replay bound overflow");
  std::size_t role_count = cloud.size();
  std::size_t family_count = 0U;
  std::size_t arm_count = 0U;
  for (const ExactDirectSupportEvent& event : facade.events) {
    if (event.birth_order.has_value()) {
      role_count =
          checked_add(role_count, 1U, "role count overflow");
    }
    if (event.saddle_order.has_value()) {
      role_count =
          checked_add(role_count, 1U, "role count overflow");
      family_count =
          checked_add(family_count, 1U, "family count overflow");
      arm_count = checked_add(
          arm_count,
          static_cast<std::size_t>(event.support_size),
          "arm count overflow");
    }
  }
  if (replay_bound > downstream_cap || role_count > downstream_cap ||
      family_count > downstream_cap || arm_count > downstream_cap) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_seed_journal");
  }
  return {replay_bound, role_count, family_count, arm_count};
}

[[nodiscard]] std::size_t make_support_frontier_capacity(
    const Options& options) {
  return checked_add(
      checked_multiply(
          8U,
          options.support_work_budget,
          "support frontier capacity overflow"),
      2U,
      "support frontier capacity overflow");
}

[[nodiscard]] std::size_t make_higher_point_reference_capacity(
    const Options& options) {
  return checked_multiply(
      options.support_record_budget,
      checked_add(
          options.maximum_order,
          4U,
          "support point-reference factor overflow"),
      "support point-reference capacity overflow");
}

[[nodiscard]] ExactMortonGroupedAnchoredPairScheduleConfig
make_sparse_pair_schedule_config() {
  return {32U, 64U, true, false, true, false, true};
}

[[nodiscard]] std::size_t make_sparse_pair_maximum_closed_rank(
    const Options& options) {
  return checked_add(
      options.maximum_order,
      1U,
      "sparse pair maximum closed rank overflow");
}

[[nodiscard]] std::size_t make_sparse_pair_point_reference_capacity(
    const Options& options) {
  return checked_multiply(
      options.support_record_budget,
      checked_add(
          options.maximum_order,
          2U,
          "sparse pair point-reference factor overflow"),
      "sparse pair point-reference capacity overflow");
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget
make_sparse_pair_advance_budget(const Options& options) {
  return {
      {options.support_work_budget,
       options.support_work_budget,
       options.support_work_budget,
       options.support_work_budget},
      {options.support_work_budget},
      1U,
      checked_add(
          options.maximum_order,
          2U,
          "sparse pair per-advance reference capacity overflow"),
  };
}

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity
make_sparse_pair_total_capacity(const Options& options) {
  return {
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_work_budget,
      options.support_record_budget,
      make_sparse_pair_point_reference_capacity(options),
  };
}

[[nodiscard]] ExactHigherSupportStreamBudget make_higher_budget(
    const Options& options) {
  const std::size_t frontier_capacity =
      make_support_frontier_capacity(options);
  const std::size_t higher_point_classification_capacity =
      checked_multiply(
          options.support_work_budget,
          options.point_count,
          "higher-support point-classification capacity overflow");
  return {
      options.support_work_budget,
      frontier_capacity,
      frontier_capacity,
      options.support_record_budget,
      make_higher_point_reference_capacity(options),
      options.support_work_budget,
      options.support_work_budget,
      higher_point_classification_capacity,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
make_closure_budget(
    const Options& options,
    std::size_t locator_table_slot_count,
    std::size_t component_count) {
  const std::size_t closure_capacity = std::min(
      {options.downstream_record_budget,
       options.descent_work_budget,
       direct_sparse_facet_descent_closure_maximum_node_count});
  const std::size_t memo_slot_count = checked_add(
      checked_multiply(
          2U, closure_capacity, "closure memo capacity overflow"),
      1U,
      "closure memo capacity overflow");
  const std::size_t probe_slot_limit =
      std::min(locator_table_slot_count, options.descent_work_budget);
  const std::size_t parent_hop_limit =
      std::min(component_count, options.descent_work_budget);
  const ExactDirectSparsePositiveFacetProbeBudget probe_budget{
      probe_slot_limit, parent_hop_limit};
  const std::size_t neighbor_capacity =
      std::min(options.maximum_order, options.descent_work_budget);
  const std::size_t shell_capacity = std::min(
      options.downstream_record_budget,
      options.descent_work_budget);
  const ExactLbvhTopKBudget top_k_budget{
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      options.descent_work_budget,
      neighbor_capacity,
      shell_capacity};
  return {
      closure_capacity,
      closure_capacity,
      closure_capacity,
      memo_slot_count,
      {probe_budget, top_k_budget, probe_budget},
  };
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig
make_industrial_config(const Options& options) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy =
      ExactDirectMorseIndustrialPolicy::interactive_resident_50k;
  config.memory_model = {
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
  const std::uint64_t cap =
      static_cast<std::uint64_t>(
          options.downstream_record_budget);
  config.chunk_budget = {
      options.chunk_byte_budget,
      cap,
      cap,
      cap,
      cap,
      static_cast<std::uint64_t>(
          std::min(
              options.downstream_record_budget,
              options.descent_work_budget)),
  };
  return config;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget
make_plan_budget(
    const Options& options,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  const std::size_t lane_cap = checked_multiply(
      3U,
      event_journal.batches.size(),
      "batch-plan lane capacity overflow");
  return {
      1U,
      event_journal.batches.size(),
      seed_journal.families.size(),
      seed_journal.arm_seeds.size(),
      lane_cap,
      seed_journal.arm_seeds.size(),
      options.descent_work_budget,
      options.descent_work_budget,
  };
}

[[nodiscard]] ExactDirectMorseForestBudget make_forest_budget(
    const Options& options,
    const CanonicalPointCloud& cloud,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  const std::size_t direct_birth_count =
      static_cast<std::size_t>(std::count_if(
          event_journal.materialized_direct_role_records.begin(),
          event_journal.materialized_direct_role_records.end(),
          [](const ExactDirectMorseH0RoleRecord& role) {
            return role.role == ExactDirectMorseH0Role::birth;
          }));
  const std::size_t birth_count = checked_add(
      cloud.size(), direct_birth_count, "forest birth count overflow");
  const std::size_t family_count = seed_journal.families.size();
  const std::size_t arm_count = seed_journal.arm_seeds.size();
  const std::size_t batch_count = event_journal.batches.size();
  const std::size_t component_count = checked_add(
      birth_count, arm_count, "locator component capacity overflow");
  const std::size_t committed_binding_count = checked_add(
      birth_count, arm_count, "locator binding capacity overflow");
  const std::size_t table_slot_count = checked_add(
      checked_multiply(
          2U,
          committed_binding_count,
          "locator table capacity overflow"),
      1U,
      "locator table capacity overflow");
  const std::size_t maximum_batch_mutation_count = checked_add(
      birth_count, arm_count, "locator batch capacity overflow");
  const std::size_t batch_scratch_slot_count = checked_add(
      checked_multiply(
          2U,
          maximum_batch_mutation_count,
          "locator scratch capacity overflow"),
      1U,
      "locator scratch capacity overflow");
  const std::size_t key_point_count = checked_add(
      cloud.size(),
      checked_multiply(
          options.maximum_order,
          checked_add(
              direct_birth_count,
              arm_count,
              "locator key population overflow"),
          "locator key-point capacity overflow"),
      "locator key-point capacity overflow");
  const std::size_t node_count = checked_add(
      birth_count, family_count, "forest node capacity overflow");
  std::size_t logical_output_count = checked_multiply(
      13U, birth_count, "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      checked_multiply(
          12U, arm_count, "forest logical-output capacity overflow"),
      "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      checked_multiply(
          3U, family_count, "forest logical-output capacity overflow"),
      "forest logical-output capacity overflow");
  logical_output_count = checked_add(
      logical_output_count,
      batch_count,
      "forest logical-output capacity overflow");
  const std::size_t maximum_required_population =
      std::max(
          {birth_count,
           component_count,
           committed_binding_count,
           key_point_count,
           node_count,
           logical_output_count});
  if (maximum_required_population >
      options.downstream_record_budget) {
    throw std::length_error(
        "downstream_record_budget_exhausted_before_forest");
  }
  const ExactDirectSparseFacetDescentClosureBudget closure_budget =
      make_closure_budget(
          options, table_slot_count, component_count);
  const std::size_t closure_capacity =
      closure_budget.maximum_node_count;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count =
      event_journal.role_record_count;
  budget.maximum_source_batch_scan_count = batch_count;
  budget.maximum_source_family_scan_count = family_count;
  budget.maximum_source_arm_seed_scan_count = arm_count;
  budget.maximum_birth_record_count = birth_count;
  budget.maximum_arm_root_binding_count = arm_count;
  budget.maximum_saddle_record_count = family_count;
  budget.maximum_atomic_group_count = family_count;
  budget.maximum_child_reference_count = arm_count;
  budget.maximum_batch_record_count = batch_count;
  budget.maximum_node_count = node_count;
  budget.maximum_final_root_count =
      event_journal.effective_maximum_order;
  budget.maximum_batch_distinct_arm_count = arm_count;
  budget.maximum_logical_output_entry_count = logical_output_count;
  budget.maximum_aggregate_closure_node_count =
      options.downstream_record_budget;
  budget.maximum_aggregate_closure_step_call_count =
      options.downstream_record_budget;
  budget.locator_budget = {
      component_count,
      committed_binding_count,
      key_point_count,
      arm_count,
      checked_add(batch_count, 1U, "locator batch count overflow"),
      closure_capacity,
      maximum_batch_mutation_count,
      maximum_batch_mutation_count,
      checked_multiply(
          options.maximum_order,
          maximum_batch_mutation_count,
          "locator batch key-point capacity overflow"),
      table_slot_count,
      batch_scratch_slot_count,
  };
  budget.closure_budget = closure_budget;
  budget.quotient_budget = {
      family_count,
      arm_count,
      arm_count,
      arm_count,
      options.downstream_record_budget};
  return budget;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionBudget
make_execution_budget(
    const Options& options,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  return {
      3U,
      seed_journal.families.size(),
      seed_journal.arm_seeds.size(),
      checked_multiply(
          options.maximum_order,
          seed_journal.arm_seeds.size(),
          "execution key-reference capacity overflow"),
      seed_journal.arm_seeds.size(),
  };
}

[[nodiscard]] std::string json_escape(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char character : text) {
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(character);
        break;
    }
  }
  return escaped;
}

void emit_report(const Report& report) {
  const auto boolean = [](bool value) {
    return value ? "true" : "false";
  };
  std::cout
      << "{\n"
      << "  \"schema\":\"morsehgp3d.direct-morse-product-run.v3\",\n"
      << "  \"phase\":\"14Q_to_15D\",\n"
      << "  \"backend\":\"reference_cpu\",\n"
      << "  \"profile\":\"hgp_reduced\",\n"
      << "  \"mode\":\"" << json_escape(report.options.mode)
      << "\",\n"
      << "  \"public_status\":\"not_claimed\",\n"
      << "  \"family\":\"" << json_escape(report.options.family)
      << "\",\n"
      << "  \"point_count\":" << report.options.point_count << ",\n"
      << "  \"canonical_point_count\":"
      << report.canonical_point_count << ",\n"
      << "  \"requested_maximum_order\":"
      << report.options.maximum_order << ",\n"
      << "  \"effective_maximum_order\":"
      << report.effective_maximum_order << ",\n"
      << "  \"terminal_stage\":\""
      << json_escape(report.terminal_stage) << "\",\n"
      << "  \"stop_category\":\""
      << json_escape(report.stop_category) << "\",\n"
      << "  \"stop_detail\":\""
      << json_escape(report.stop_detail) << "\",\n"
      << "  \"pipeline_complete\":"
      << boolean(report.pipeline_complete) << ",\n"
      << "  \"resident_conditional_pipeline_complete\":"
      << boolean(report.pipeline_complete) << ",\n"
      << "  \"budget_exhausted\":"
      << boolean(report.budget_exhausted) << ",\n"
      << "  \"scientific_result_materialized\":"
      << boolean(report.scientific_result_materialized) << ",\n"
      << "  \"conditional_h0_candidate_certified\":"
      << boolean(report.conditional_h0_candidate_certified) << ",\n"
      << "  \"global_morse_obligation_replayed\":false,\n"
      << "  \"warm_e2e_protocol_executed\":false,\n"
      << "  \"warm_e2e_slo_claimed\":false,\n"
      << "  \"qualification_claimed\":false,\n"
      << "  \"timing_scope\":"
         "\"attempted_single_process_cpu_generation_to_materialized_forest\",\n"
      << "  \"architecture_audit_complete\":"
      << boolean(report.architecture_audit_complete) << ",\n"
      << "  \"no_forbidden_global_structure_materialized\":"
      << boolean(
             report.no_forbidden_global_structure_materialized)
      << ",\n"
      << "  \"budgets\":{\"support_work\":"
      << report.options.support_work_budget
      << ",\"support_records\":"
      << report.options.support_record_budget
      << ",\"higher_chunks\":"
      << report.options.higher_chunk_limit
      << ",\"downstream_records\":"
      << report.options.downstream_record_budget
      << ",\"descent_work\":"
      << report.options.descent_work_budget
      << ",\"chunk_bytes\":"
      << report.options.chunk_byte_budget << "},\n"
      << "  \"timings_ms\":{\"generation\":" << std::fixed
      << std::setprecision(3) << report.timings.generation_ms
      << ",\"canonicalization\":"
      << report.timings.canonicalization_ms
      << ",\"lbvh\":" << report.timings.lbvh_ms
      << ",\"pair_support\":"
      << report.timings.pair_support_ms
      << ",\"higher_support\":"
      << report.timings.higher_support_ms
      << ",\"terminal_facade\":"
      << report.timings.terminal_facade_ms
      << ",\"event_journal\":"
      << report.timings.event_journal_ms
      << ",\"saddle_seed_journal\":"
      << report.timings.saddle_seed_journal_ms
      << ",\"batch_plan\":"
      << report.timings.batch_plan_ms
      << ",\"reducer_setup\":"
      << report.timings.reducer_setup_ms
      << ",\"reducer_stream\":"
      << report.timings.reducer_stream_ms
      << ",\"forest_finish\":"
      << report.timings.forest_finish_ms
      << ",\"total\":" << report.timings.total_ms << "},\n"
      << "  \"pair_support\":{\"source_kind\":\""
      << report.pair_source_kind << "\",\"authority_kind\":\""
      << report.pair_authority_kind
      << "\",\"p7b_replay_performed\":false,\"status\":\""
      << report.pair_status << "\",\"stop_reason\":\""
      << report.pair_stop_reason << "\",\"maximum_closed_rank\":"
      << report.pair_maximum_closed_rank
      << ",\"schedule_config\":{\"maximum_anchors_per_group\":"
      << report.pair_schedule_config.maximum_anchor_count_per_group
      << ",\"proposed_witness_pool_size\":"
      << report.pair_schedule_config.proposed_witness_pool_size
      << ",\"triangular_block_pair_schedule\":"
      << (report.pair_schedule_config.use_triangular_block_pair_schedule
              ? "true"
              : "false")
      << ",\"symmetric_inconclusive_cross_block_splitting\":"
      << (report.pair_schedule_config
                  .use_symmetric_inconclusive_cross_block_splitting
              ? "true"
              : "false")
      << ",\"prioritize_cross_blocks\":"
      << (report.pair_schedule_config.prioritize_cross_blocks
              ? "true"
              : "false")
      << ",\"witness_subtree_first_for_triangular_blocks\":"
      << (report.pair_schedule_config
                  .use_witness_subtree_first_for_triangular_blocks
              ? "true"
              : "false")
      << ",\"floating_witness_order_for_triangular_blocks\":"
      << (report.pair_schedule_config
                  .use_floating_witness_order_for_triangular_blocks
              ? "true"
              : "false")
      << "},\"advance_budget\":{\"schedule_advances\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_schedule_advance_count
      << ",\"orientation_checks\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_orientation_check_count
      << ",\"grouped_node_visits\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_node_visit_count
      << ",\"grouped_exact_predicates\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"grouped_logical_signs\":"
      << report.pair_advance_budget.candidate_cursor
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"classification_node_visits\":"
      << report.pair_advance_budget.classifier.maximum_node_visit_count
      << ",\"emitted_records\":"
      << report.pair_advance_budget.maximum_emitted_record_count
      << ",\"emitted_point_id_references\":"
      << report.pair_advance_budget
             .maximum_emitted_point_id_reference_count
      << "},\"total_capacity\":{\"schedule_advances\":"
      << report.pair_total_capacity.maximum_schedule_advance_count
      << ",\"orientation_checks\":"
      << report.pair_total_capacity.maximum_orientation_check_count
      << ",\"grouped_node_visits\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_node_visit_count
      << ",\"grouped_exact_predicates\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"grouped_logical_signs\":"
      << report.pair_total_capacity
             .maximum_grouped_traversal_exact_predicate_count
      << ",\"admitted_candidates\":"
      << report.pair_total_capacity.maximum_admitted_candidate_count
      << ",\"classification_node_visits\":"
      << report.pair_total_capacity
             .maximum_classification_node_visit_count
      << ",\"output_records\":"
      << report.pair_total_capacity.maximum_output_record_count
      << ",\"output_point_id_references\":"
      << report.pair_total_capacity
             .maximum_output_point_id_reference_count
      << "},\"audit\":{\"advance_calls\":"
      << report.pair_advance_calls << ",\"schedule_advances\":"
      << report.pair_schedule_advances << ",\"orientation_checks\":"
      << report.pair_orientation_checks
      << ",\"reverse_or_self_orientation_skips\":"
      << report.pair_reverse_or_self_orientation_skips
      << ",\"prepared_groups\":"
      << report.pair_prepared_groups
      << ",\"completed_groups\":"
      << report.pair_completed_groups
      << ",\"grouped_node_visits\":"
      << report.pair_grouped_traversal_node_visits
      << ",\"witness_subtree_node_visits\":"
      << report.pair_witness_subtree_node_visits
      << ",\"grouped_common_node_visits\":"
      << report.pair_grouped_common_node_visits
      << ",\"anchor_subgroup_node_visits\":"
      << report.pair_anchor_subgroup_node_visits
      << ",\"singleton_node_visits\":"
      << report.pair_singleton_node_visits
      << ",\"grouped_witness_slots\":"
      << report.pair_grouped_witness_slots
      << ",\"grouped_inherited_witness_reuses\":"
      << report.pair_grouped_inherited_witness_reuses
      << ",\"grouped_exact_predicates\":"
      << report.pair_grouped_traversal_exact_predicates
      << ",\"grouped_logical_signs\":"
      << report.pair_grouped_traversal_exact_predicates
      << ",\"fp64_filtered_negative_predicates\":"
      << report.pair_fp64_filtered_negative_predicates
      << ",\"fp64_filtered_positive_predicates\":"
      << report.pair_fp64_filtered_positive_predicates
      << ",\"exact_fallback_predicates\":"
      << report.pair_exact_fallback_predicates
      << ",\"floating_witness_order_preparations\":"
      << report.pair_floating_witness_order_preparations
      << ",\"floating_witness_score_evaluations\":"
      << report.pair_floating_witness_score_evaluations
      << ",\"floating_witness_nonfinite_scores\":"
      << report.pair_floating_witness_nonfinite_scores
      << ",\"floating_witness_order_requested\":"
      << (report.pair_floating_witness_order_requested ? "true" : "false")
      << ",\"floating_witness_order_effective_for_every_prepared_traversal\":"
      << (report
                  .pair_floating_witness_order_effective_for_every_prepared_traversal
              ? "true"
              : "false")
      << ",\"fp64_filter_partition_certified\":"
      << (report.pair_fp64_filter_partition_certified ? "true" : "false")
      << ",\"witness_subtree_exact_predicates\":"
      << report.pair_witness_subtree_exact_predicates
      << ",\"witness_subtree_receipts\":"
      << report.pair_witness_subtree_receipts
      << ",\"witness_subtree_successes\":"
      << report.pair_witness_subtree_successes
      << ",\"witness_subtree_fail_opens\":"
      << report.pair_witness_subtree_fail_opens
      << ",\"grouped_common_exact_predicates\":"
      << report.pair_grouped_common_exact_predicates
      << ",\"anchor_subgroup_exact_predicates\":"
      << report.pair_anchor_subgroup_exact_predicates
      << ",\"singleton_exact_predicates\":"
      << report.pair_singleton_exact_predicates
      << ",\"grouped_strict_witness_discoveries\":"
      << report.pair_grouped_strict_witness_discoveries
      << ",\"grouped_diagonal_node_descents\":"
      << report.pair_grouped_diagonal_node_descents
      << ",\"grouped_common_frontiers\":"
      << report.pair_grouped_common_frontiers
      << ",\"delegated_frontier_anchors\":"
      << report.pair_delegated_frontier_anchors
      << ",\"prepared_anchor_subgroup_probes\":"
      << report.pair_prepared_anchor_subgroup_probes
      << ",\"anchor_subgroup_witness_pool_entries\":"
      << report.pair_anchor_subgroup_witness_pool_entries
      << ",\"query_facing_fallback_witness_pool_entries\":"
      << report.pair_query_facing_fallback_witness_pool_entries
      << ",\"anchor_subgroup_splits\":"
      << report.pair_anchor_subgroup_splits
      << ",\"query_subtree_splits\":"
      << report.pair_query_subtree_splits
      << ",\"anchor_subgroup_certified_prunes\":"
      << report.pair_anchor_subgroup_certified_prunes
      << ",\"anchor_subgroup_certified_anchors\":"
      << report.pair_anchor_subgroup_certified_anchors
      << ",\"prepared_singleton_fallbacks\":"
      << report.pair_prepared_singleton_fallbacks
      << ",\"completed_singleton_fallbacks\":"
      << report.pair_completed_singleton_fallbacks
      << ",\"singleton_witness_pool_entries\":"
      << report.pair_singleton_witness_pool_entries
      << ",\"singleton_certified_prunes\":"
      << report.pair_singleton_certified_prunes
      << ",\"maximum_pending_anchor_subgroups\":"
      << report.pair_maximum_pending_anchor_subgroups
      << ",\"triangular_block_pair_visits\":"
      << report.pair_triangular_block_pair_visits
      << ",\"triangular_diagonal_splits\":"
      << report.pair_triangular_diagonal_splits
      << ",\"triangular_oversized_anchor_splits\":"
      << report.pair_triangular_oversized_anchor_splits
      << ",\"triangular_consumer_query_splits\":"
      << report.pair_triangular_consumer_query_splits
      << ",\"triangular_self_pairs\":"
      << report.pair_triangular_self_pairs
      << ",\"triangular_cross_blocks\":"
      << report.pair_triangular_cross_blocks
      << ",\"triangular_certified_cross_blocks\":"
      << report.pair_triangular_certified_cross_blocks
      << ",\"triangular_certified_unordered_pairs\":"
      << report.pair_triangular_certified_unordered_pairs
      << ",\"triangular_opened_singleton_cross_blocks\":"
      << report.pair_triangular_opened_singleton_cross_blocks
      << ",\"triangular_opened_unordered_pairs\":"
      << report.pair_triangular_opened_unordered_pairs
      << ",\"triangular_maximum_pending_block_pairs\":"
      << report.pair_triangular_maximum_pending_block_pairs
      << ",\"triangular_partition_complete\":"
      << (report.pair_triangular_partition_complete ? "true" : "false")
      << ",\"no_dynamic_dual_tree_or_pair_arena\":"
      << (report.pair_no_dynamic_dual_tree_or_pair_arena ? "true" : "false")
      << ",\"authenticated_prunes\":"
      << report.pair_authenticated_prunes
      << ",\"authenticated_pruned_directed_pairs\":"
      << report.pair_authenticated_pruned_directed_pairs
      << ",\"directed_pair_universe\":"
      << report.pair_directed_pair_universe
      << ",\"admitted_candidates\":"
      << report.pair_admitted_candidates
      << ",\"classification_advances\":"
      << report.pair_classification_advances
      << ",\"classification_node_visits\":"
      << report.pair_classification_node_visits
      << ",\"classification_terminals\":"
      << report.pair_classification_terminals
      << ",\"above_rank\":" << report.pair_above_rank
      << ",\"output_records\":" << report.pair_output_records
      << ",\"output_point_id_references\":"
      << report.pair_output_point_id_references
      << ",\"local_budget_exhaustions\":"
      << report.pair_local_budget_exhaustions
      << ",\"total_capacity_exhaustions\":"
      << report.pair_total_capacity_exhaustions
      << ",\"maximum_live_candidates\":"
      << report.pair_maximum_live_candidates
      << ",\"accepted_events\":"
      << report.pair_accepted_events
      << ",\"extra_shell_diagnostics\":"
      << report.pair_extra_shell_diagnostics
      << ",\"directed_coverage_certified\":"
      << boolean(report.pair_directed_coverage_certified)
      << ",\"orientation_partition_certified\":"
      << boolean(report.pair_orientation_partition_certified)
      << ",\"classification_partition_certified\":"
      << boolean(report.pair_classification_partition_certified)
      << ",\"output_partition_certified\":"
      << boolean(report.pair_output_partition_certified)
      << ",\"records_certified\":"
      << boolean(report.pair_records_certified) << "}},\n"
      << "  \"higher_support\":{\"status\":\""
      << report.higher_status << "\",\"stop_reason\":\""
      << report.higher_stop_reason << "\",\"work_units\":"
      << report.higher_work_units << ",\"product_visits\":"
      << report.higher_product_visits
      << ",\"closed_ball_queries\":"
      << report.higher_closed_ball_queries
      << ",\"accepted_events\":"
      << report.higher_accepted_events
      << ",\"extra_shell_diagnostics\":"
      << report.higher_extra_shell_diagnostics
      << ",\"prune_certificates\":"
      << report.higher_prune_certificates
      << ",\"chunks\":" << report.higher_chunk_count
      << ",\"authority_kind\":\""
      << report.higher_authority_kind
      << "\",\"full_geometry_replay_avoided\":"
      << boolean(
             report.higher_authority_kind ==
             "sealed_anchored_fixed_chunk_run")
      << "},\n"
      << "  \"pipeline_counts\":{\"terminal_catalog_certified\":"
      << boolean(report.terminal_catalog_certified)
      << ",\"terminal_events\":"
      << report.terminal_event_count
      << ",\"terminal_extra_shell_diagnostics\":"
      << report.terminal_extra_shell_diagnostic_count
      << ",\"event_batches\":" << report.event_batch_count
      << ",\"event_roles\":" << report.event_role_count
      << ",\"saddle_families\":" << report.saddle_family_count
      << ",\"arm_seeds\":" << report.arm_seed_count
      << ",\"industrial_chunks\":"
      << report.industrial_chunk_count
      << ",\"plan_lanes\":" << report.plan_lane_count
      << ",\"prepared_tickets\":"
      << report.prepared_ticket_count
      << ",\"committed_batches\":"
      << report.committed_batch_count << "},\n"
      << "  \"decisions\":{\"industrial_plan\":"
      << report.industrial_plan_decision
      << ",\"batch_plan\":" << report.plan_decision
      << ",\"last_batch_execution\":"
      << report.batch_execution_decision
      << ",\"last_preparation\":"
      << report.preparation_decision
      << ",\"last_reducer_fold\":"
      << report.reducer_fold_decision
      << ",\"last_live_commit\":"
      << report.live_commit_decision << "},\n"
      << "  \"forest\":{\"birth_records\":"
      << report.forest_birth_count
      << ",\"materialized_birth_records\":"
      << report.forest_materialized_birth_count
      << ",\"saddles\":" << report.forest_saddle_count
      << ",\"atomic_groups\":"
      << report.forest_atomic_group_count
      << ",\"nodes\":" << report.forest_node_count
      << ",\"materialized_nodes\":"
      << report.forest_materialized_node_count
      << ",\"final_roots\":" << report.forest_final_root_count
      << ",\"logical_output_entries\":"
      << report.forest_logical_output_entry_count
      << ",\"aggregate_closure_nodes\":"
      << report.forest_aggregate_closure_node_count
      << ",\"aggregate_closure_step_calls\":"
      << report.forest_aggregate_closure_step_call_count << "}\n"
      << "}\n";
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  return smoke_clouds::eight_clusters_points(options.point_count);
}

[[nodiscard]] int run(const Options& options) {
  Report report;
  report.options = options;
  const Clock::time_point total_start = Clock::now();
  if (options.maximum_order >= options.point_count) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "maximum_order_must_be_strictly_less_than_point_count";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }
  if (options.point_count >
      direct_morse_interactive_resident_maximum_point_count) {
    report.terminal_stage = "input_preflight";
    report.stop_category = "invalid_input";
    report.stop_detail =
        "resident_timed_point_count_exceeds_50000";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 4;
  }

  const Clock::time_point generation_start = Clock::now();
  const std::vector<CertifiedPoint3> input = generate_points(options);
  const Clock::time_point generation_end = Clock::now();
  report.timings.generation_ms =
      milliseconds(generation_end - generation_start);

  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{input});
  report.canonical_point_count = cloud.size();
  const Clock::time_point canonicalization_end = Clock::now();
  report.timings.canonicalization_ms =
      milliseconds(canonicalization_end - generation_end);

  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const Clock::time_point lbvh_end = Clock::now();
  report.timings.lbvh_ms =
      milliseconds(lbvh_end - canonicalization_end);

  const ExactMortonGroupedAnchoredPairScheduleConfig pair_schedule_config =
      make_sparse_pair_schedule_config();
  const std::size_t pair_maximum_closed_rank =
      make_sparse_pair_maximum_closed_rank(options);
  const ExactSparseAnchoredPairSessionAdvanceBudget pair_advance_budget =
      make_sparse_pair_advance_budget(options);
  const ExactSparseAnchoredPairSessionTotalCapacity pair_total_capacity =
      make_sparse_pair_total_capacity(options);
  const ExactHigherSupportStreamBudget higher_budget =
      make_higher_budget(options);
  report.pair_maximum_closed_rank = pair_maximum_closed_rank;
  report.pair_schedule_config = pair_schedule_config;
  report.pair_advance_budget = pair_advance_budget;
  report.pair_total_capacity = pair_total_capacity;
  report.pair_source_kind = "sparse_anchored_session";
  report.pair_authority_kind = "unsealed_sparse_anchored_session";
  report.effective_maximum_order = options.maximum_order;

  ExactSparseAnchoredPairSession pair_session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          pair_maximum_closed_rank,
          pair_schedule_config,
          pair_total_capacity);
  ExactSparseAnchoredPairSessionStopReason pair_terminal_stop_reason =
      ExactSparseAnchoredPairSessionStopReason::none;
  while (!pair_session.complete() &&
         !pair_session.total_capacity_exhausted() &&
         !pair_session.poisoned()) {
    const ExactSparseAnchoredPairSessionStep step = pair_session.advance(
        index, cloud, pair_advance_budget);
    pair_terminal_stop_reason = step.stop_reason();
  }
  const Clock::time_point pair_end = Clock::now();
  report.timings.pair_support_ms =
      milliseconds(pair_end - lbvh_end);
  report.pair_stop_reason =
      pair_stop_reason_text(pair_terminal_stop_reason);
  const ExactSparseAnchoredPairSessionAudit pair_audit =
      pair_session.audit();
  const ExactMortonGroupedAnchoredPairCandidateAudit pair_candidate_audit =
      pair_session.candidate_audit();
  const ExactMortonGroupedAnchoredPairScheduleAudit pair_schedule_audit =
      pair_session.schedule_audit();
  report.pair_advance_calls = pair_audit.advance_call_count;
  report.pair_schedule_advances =
      pair_candidate_audit.schedule_advance_count;
  report.pair_orientation_checks =
      pair_candidate_audit.orientation_check_count;
  report.pair_reverse_or_self_orientation_skips =
      pair_candidate_audit.reverse_or_self_orientation_skip_count;
  report.pair_prepared_groups = pair_schedule_audit.prepared_group_count;
  report.pair_completed_groups = pair_schedule_audit.completed_group_count;
  report.pair_grouped_traversal_node_visits =
      pair_candidate_audit.grouped_traversal_node_visit_count;
  report.pair_witness_subtree_node_visits =
      pair_schedule_audit.witness_subtree_node_visit_count;
  report.pair_grouped_common_node_visits =
      pair_schedule_audit.common_traversal_node_visit_count;
  report.pair_anchor_subgroup_node_visits =
      pair_schedule_audit.anchor_subgroup_node_visit_count;
  report.pair_singleton_node_visits =
      pair_schedule_audit.singleton_node_visit_count;
  report.pair_grouped_witness_slots =
      pair_schedule_audit.witness_slot_scan_count;
  report.pair_grouped_inherited_witness_reuses =
      pair_schedule_audit.inherited_witness_reuse_count;
  report.pair_grouped_traversal_exact_predicates =
      pair_candidate_audit.grouped_traversal_exact_predicate_count;
  report.pair_fp64_filtered_negative_predicates =
      pair_schedule_audit.fp64_filtered_negative_predicate_count;
  report.pair_fp64_filtered_positive_predicates =
      pair_schedule_audit.fp64_filtered_positive_predicate_count;
  report.pair_exact_fallback_predicates =
      pair_schedule_audit.exact_fallback_predicate_count;
  report.pair_floating_witness_order_preparations =
      pair_schedule_audit.floating_witness_order_preparation_count;
  report.pair_floating_witness_score_evaluations =
      pair_schedule_audit.floating_witness_score_evaluation_count;
  report.pair_floating_witness_nonfinite_scores =
      pair_schedule_audit.floating_witness_nonfinite_score_count;
  report.pair_floating_witness_order_requested =
      pair_schedule_audit.floating_witness_order_requested;
  report.pair_floating_witness_order_effective_for_every_prepared_traversal =
      pair_schedule_audit
          .floating_witness_order_effective_for_every_prepared_traversal;
  report.pair_fp64_filter_partition_certified = three_sum_matches(
      report.pair_fp64_filtered_negative_predicates,
      report.pair_fp64_filtered_positive_predicates,
      report.pair_exact_fallback_predicates,
      report.pair_grouped_traversal_exact_predicates);
  report.pair_witness_subtree_exact_predicates =
      pair_schedule_audit.witness_subtree_exact_predicate_count;
  report.pair_witness_subtree_receipts =
      pair_schedule_audit.witness_subtree_receipt_count;
  report.pair_witness_subtree_successes =
      pair_schedule_audit.witness_subtree_success_count;
  report.pair_witness_subtree_fail_opens =
      pair_schedule_audit.witness_subtree_fail_open_count;
  report.pair_grouped_common_exact_predicates =
      pair_schedule_audit.common_exact_predicate_count;
  report.pair_anchor_subgroup_exact_predicates =
      pair_schedule_audit.anchor_subgroup_exact_predicate_count;
  report.pair_singleton_exact_predicates =
      pair_schedule_audit.singleton_exact_predicate_count;
  report.pair_grouped_strict_witness_discoveries =
      pair_schedule_audit.strict_witness_discovery_count;
  report.pair_grouped_diagonal_node_descents =
      pair_schedule_audit.diagonal_node_descent_count;
  report.pair_grouped_common_frontiers =
      pair_schedule_audit.common_frontier_count;
  report.pair_delegated_frontier_anchors =
      pair_schedule_audit.delegated_frontier_anchor_count;
  report.pair_prepared_anchor_subgroup_probes =
      pair_schedule_audit.prepared_anchor_subgroup_probe_count;
  report.pair_anchor_subgroup_witness_pool_entries =
      pair_schedule_audit.proposed_anchor_subgroup_witness_pool_entry_count;
  report.pair_query_facing_fallback_witness_pool_entries =
      pair_schedule_audit.query_facing_fallback_witness_pool_entry_count;
  report.pair_anchor_subgroup_splits =
      pair_schedule_audit.anchor_subgroup_split_count;
  report.pair_query_subtree_splits =
      pair_schedule_audit.query_subtree_split_count;
  report.pair_anchor_subgroup_certified_prunes =
      pair_schedule_audit.anchor_subgroup_certified_prune_count;
  report.pair_anchor_subgroup_certified_anchors =
      pair_schedule_audit.anchor_subgroup_certified_anchor_count;
  report.pair_prepared_singleton_fallbacks =
      pair_schedule_audit.prepared_singleton_fallback_count;
  report.pair_completed_singleton_fallbacks =
      pair_schedule_audit.completed_singleton_fallback_count;
  report.pair_singleton_witness_pool_entries =
      pair_schedule_audit.proposed_singleton_witness_pool_entry_count;
  report.pair_singleton_certified_prunes =
      pair_schedule_audit.singleton_certified_prune_count;
  report.pair_maximum_pending_anchor_subgroups =
      pair_schedule_audit.maximum_pending_anchor_subgroup_count;
  report.pair_triangular_block_pair_visits =
      pair_schedule_audit.triangular_block_pair_visit_count;
  report.pair_triangular_diagonal_splits =
      pair_schedule_audit.triangular_diagonal_split_count;
  report.pair_triangular_oversized_anchor_splits =
      pair_schedule_audit.triangular_oversized_anchor_split_count;
  report.pair_triangular_consumer_query_splits =
      pair_schedule_audit.triangular_consumer_query_split_count;
  report.pair_triangular_self_pairs =
      pair_schedule_audit.triangular_self_pair_count;
  report.pair_triangular_cross_blocks =
      pair_schedule_audit.triangular_cross_block_count;
  report.pair_triangular_certified_cross_blocks =
      pair_schedule_audit.triangular_certified_cross_block_count;
  report.pair_triangular_certified_unordered_pairs =
      pair_schedule_audit.triangular_certified_unordered_pair_count;
  report.pair_triangular_opened_singleton_cross_blocks =
      pair_schedule_audit.triangular_opened_singleton_cross_block_count;
  report.pair_triangular_opened_unordered_pairs =
      pair_schedule_audit.triangular_opened_unordered_pair_count;
  report.pair_triangular_maximum_pending_block_pairs =
      pair_schedule_audit.triangular_maximum_pending_block_pair_count;
  report.pair_triangular_partition_complete =
      pair_schedule_audit.triangular_partition_complete;
  report.pair_no_dynamic_dual_tree_or_pair_arena =
      pair_schedule_audit.no_dynamic_dual_tree_or_pair_arena_materialized;
  report.pair_authenticated_prunes =
      pair_audit.authenticated_prune_count;
  report.pair_authenticated_pruned_directed_pairs =
      pair_audit.authenticated_pruned_directed_pair_count;
  report.pair_directed_pair_universe =
      pair_audit.directed_pair_universe_size;
  report.pair_admitted_candidates =
      pair_audit.admitted_candidate_count;
  report.pair_classification_advances =
      pair_audit.classification_advance_count;
  report.pair_classification_node_visits =
      pair_audit.classification_node_visit_count;
  report.pair_classification_terminals =
      pair_audit.classification_terminal_count;
  report.pair_above_rank = pair_audit.above_rank_count;
  report.pair_output_records = pair_audit.emitted_record_count;
  report.pair_output_point_id_references =
      pair_audit.emitted_point_id_reference_count;
  report.pair_local_budget_exhaustions =
      pair_audit.budget_exhaustion_count;
  report.pair_total_capacity_exhaustions =
      pair_audit.total_capacity_exhaustion_count;
  report.pair_maximum_live_candidates =
      pair_audit.maximum_live_candidate_count;
  report.pair_accepted_events = pair_audit.accepted_event_count;
  report.pair_extra_shell_diagnostics =
      pair_audit.relevant_extra_shell_diagnostic_count;
  report.pair_directed_coverage_certified =
      pair_audit.directed_coverage_certified;
  report.pair_orientation_partition_certified =
      pair_audit.orientation_partition_certified;
  report.pair_classification_partition_certified =
      pair_audit.candidate_classification_partition_certified;
  report.pair_output_partition_certified =
      pair_audit.output_partition_certified;
  report.pair_records_certified =
      pair_audit.retained_records_certified;
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      pair_audit.no_forbidden_global_structure_materialized &&
      pair_candidate_audit.no_dynamic_candidate_or_output_arena_materialized &&
      pair_schedule_audit
          .no_global_anchor_pair_or_output_arena_materialized;
  if (!pair_session.complete()) {
    const bool capacity_exhausted =
        pair_session.total_capacity_exhausted();
    report.pair_status = capacity_exhausted
        ? "total_capacity_exhausted"
        : "not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_detail = capacity_exhausted
        ? report.pair_stop_reason
        : "sparse_pair_session_not_terminal";
    report.budget_exhausted = capacity_exhausted;
    report.stop_category = capacity_exhausted
        ? "budget_exhausted"
        : "certification_failure";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return capacity_exhausted ? 2 : 3;
  }
  const bool triangular_pair_schedule =
      pair_schedule_config.use_triangular_block_pair_schedule;
  const bool pair_coverage_identity = triangular_pair_schedule
      ? checked_add(
            checked_add(
                pair_audit.authenticated_pruned_directed_pair_count,
                checked_multiply(
                    2U,
                    pair_audit.admitted_candidate_count,
                    "sparse triangular pair classification mass overflow"),
                "sparse triangular pair coverage overflow"),
            cloud.size(),
            "sparse triangular pair self mass overflow") ==
          pair_audit.directed_pair_universe_size
      : checked_add(
            pair_audit.authenticated_pruned_directed_pair_count,
            pair_candidate_audit.orientation_check_count,
            "sparse pair directed partition overflow") ==
          pair_audit.directed_pair_universe_size;
  const bool pair_orientation_identity = triangular_pair_schedule
      ? pair_audit.admitted_candidate_count ==
              pair_candidate_audit.orientation_check_count &&
          pair_candidate_audit.reverse_or_self_orientation_skip_count == 0U
      : checked_add(
            pair_audit.admitted_candidate_count,
            pair_candidate_audit.reverse_or_self_orientation_skip_count,
            "sparse pair orientation partition overflow") ==
          pair_candidate_audit.orientation_check_count;
  const bool pair_session_certified =
      !pair_session.poisoned() &&
      !pair_session.total_capacity_exhausted() &&
      pair_session.validated_for(index, cloud) &&
      pair_audit.every_prune_recertified &&
      pair_audit.directed_coverage_certified &&
      pair_audit.orientation_partition_certified &&
      pair_audit.candidate_classification_partition_certified &&
      pair_audit.output_partition_certified &&
      pair_audit.retained_records_certified &&
      pair_audit.no_forbidden_global_structure_materialized &&
      pair_candidate_audit.complete &&
      pair_candidate_audit.no_dynamic_candidate_or_output_arena_materialized &&
      pair_schedule_audit.complete &&
      pair_schedule_audit.morton_anchor_partition_complete &&
      pair_schedule_audit
          .no_global_anchor_pair_or_output_arena_materialized &&
      pair_coverage_identity && pair_orientation_identity &&
      pair_audit.admitted_candidate_count ==
          pair_audit.classification_terminal_count &&
      checked_add(
          pair_audit.above_rank_count,
          pair_audit.emitted_record_count,
          "sparse pair output partition overflow") ==
          pair_audit.classification_terminal_count &&
      checked_add(
          pair_audit.accepted_event_count,
          pair_audit.relevant_extra_shell_diagnostic_count,
          "sparse pair record partition overflow") ==
          pair_audit.emitted_record_count;
  if (!pair_session_certified) {
    report.pair_status = "session_not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_category = "certification_failure";
    report.stop_detail = "sparse_pair_session_invariants_failed";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  report.pair_status = "complete";
  report.pair_stop_reason = "none";
  ExactSparseAnchoredPairTerminalAuthority pair_authority =
      std::move(pair_session).seal();
  const bool pair_authority_certified =
      pair_authority.sealed_in_process_terminal_authority() &&
      pair_authority.bound_to(
          index,
          cloud,
          pair_maximum_closed_rank,
          pair_schedule_config,
          pair_total_capacity) &&
      pair_authority.maximum_closed_rank() == pair_maximum_closed_rank &&
      pair_authority.schedule_config() == pair_schedule_config &&
      pair_authority.total_capacity() == pair_total_capacity;
  if (!pair_authority_certified) {
    report.pair_status = "authority_not_certified";
    report.terminal_stage = "sparse_pair_session";
    report.stop_category = "certification_failure";
    report.stop_detail = "sparse_pair_authority_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }
  report.pair_source_kind = "sealed_sparse_anchored_session";
  report.pair_authority_kind = "sealed_in_process_terminal_authority";
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      pair_authority.audit().no_forbidden_global_structure_materialized;

  ExactHigherSupportTerminalSession higher_session{
      index,
      cloud,
      options.maximum_order,
      higher_budget,
      options.higher_chunk_limit};
  const ExactHigherSupportTerminalRunStatus higher_run_status =
      higher_session.run_to_terminal();
  const Clock::time_point higher_end = Clock::now();
  report.timings.higher_support_ms =
      milliseconds(higher_end - pair_end);
  const ExactHigherSupportStreamAudit& higher_audit =
      higher_session.trusted_checkpoint().cumulative_audit;
  report.higher_work_units = higher_audit.work_unit_count;
  report.higher_product_visits =
      higher_audit.support_product_visit_count;
  report.higher_closed_ball_queries =
      higher_audit.global_closed_ball_query_count;
  report.higher_accepted_events =
      higher_audit.accepted_event_count;
  report.higher_extra_shell_diagnostics =
      higher_audit.relevant_extra_shell_diagnostic_count;
  report.higher_prune_certificates =
      higher_audit.emitted_prune_certificate_count;
  report.higher_chunk_count = higher_session.chunk_count();

  if (higher_run_status !=
      ExactHigherSupportTerminalRunStatus::terminal) {
    report.higher_status = "budget_exhausted";
    report.higher_stop_reason = "maximum_chunk_count_reached";
    report.higher_authority_kind =
        "unsealed_root_anchored_fixed_chunk_session";
    report.terminal_stage = "higher_support";
    report.stop_detail = "higher_support_chunk_limit_reached";
    report.budget_exhausted = true;
    report.stop_category = "budget_exhausted";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  ExactHigherSupportTerminalAuthority higher_authority =
      std::move(higher_session).seal();
  report.higher_status = "complete";
  report.higher_stop_reason = "none";
  report.higher_authority_kind =
      "sealed_anchored_fixed_chunk_run";
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      higher_authority.no_forbidden_global_structure_materialized();

  const ExactDirectSupportTerminalFacade facade =
      build_exact_direct_support_terminal_facade(
          index,
          cloud,
          options.maximum_order,
          higher_budget,
          std::move(pair_authority),
          std::move(higher_authority));
  const Clock::time_point facade_end = Clock::now();
  report.timings.terminal_facade_ms =
      milliseconds(facade_end - higher_end);
  report.terminal_catalog_certified =
      facade.terminal_catalog_certified();
  report.terminal_event_count = facade.events.size();
  report.terminal_extra_shell_diagnostic_count =
      facade.relevant_extra_shell_diagnostics.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      facade.certificate.no_forbidden_global_structure_materialized;
  if (!facade.terminal_catalog_certified()) {
    report.terminal_stage = "terminal_facade";
    report.stop_category = "certification_failure";
    report.stop_detail = "terminal_facade_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }

  const ExactDirectMorseEventJournalResult event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const Clock::time_point event_end = Clock::now();
  report.timings.event_journal_ms =
      milliseconds(event_end - facade_end);
  report.event_batch_count = event_journal.batches.size();
  report.event_role_count = event_journal.role_record_count;
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      event_journal.no_forbidden_global_structure_materialized;
  if (!event_journal.certified_partial_refinement()) {
    report.terminal_stage = "event_journal";
    report.stop_category = "certification_failure";
    report.stop_detail = "event_journal_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }

  ExactDirectSaddleArmSeedBudget seed_budget;
  try {
    seed_budget = make_seed_budget(
        cloud, facade, options.downstream_record_budget);
  } catch (const std::length_error& error) {
    report.terminal_stage = "saddle_seed_budget";
    report.stop_category = "budget_exhausted";
    report.stop_detail = error.what();
    report.budget_exhausted = true;
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  const ExactDirectSaddleArmSeedJournalResult seed_journal =
      build_exact_direct_saddle_arm_seed_journal(
          cloud, facade, event_journal, seed_budget);
  const Clock::time_point seed_end = Clock::now();
  report.timings.saddle_seed_journal_ms =
      milliseconds(seed_end - event_end);
  report.saddle_family_count = seed_journal.families.size();
  report.arm_seed_count = seed_journal.arm_seeds.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      seed_journal.no_forbidden_global_structure_materialized;
  if (!seed_journal.certified_partial_refinement()) {
    report.terminal_stage = "saddle_seed_journal";
    report.stop_category = "certification_failure";
    report.stop_detail = "saddle_seed_journal_not_certified";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }

  const ExactDirectMorseIndustrialPlanConfig industrial_config =
      make_industrial_config(options);
  const ExactDirectSparseFacetDescentBatchPlanBudget plan_budget =
      make_plan_budget(options, event_journal, seed_journal);
  const ExactDirectSparseFacetDescentBatchPlanResult plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          cloud,
          facade,
          event_journal,
          seed_budget,
          seed_journal,
          industrial_config,
          plan_budget);
  const Clock::time_point plan_end = Clock::now();
  report.timings.batch_plan_ms =
      milliseconds(plan_end - seed_end);
  report.plan_decision =
      static_cast<unsigned>(plan.decision);
  report.industrial_plan_decision =
      static_cast<unsigned>(
          plan.source_industrial_plan.decision);
  report.industrial_chunk_count =
      plan.source_industrial_plan.chunks.size();
  report.plan_lane_count = plan.lanes.size();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      !plan.forbidden_global_structure_materialized;
  if (!plan.complete_architecture_plan()) {
    report.terminal_stage = "batch_plan";
    report.stop_detail = "batch_plan_not_complete";
    report.budget_exhausted =
        plan.decision ==
            ExactDirectSparseFacetDescentBatchPlanDecision::
                no_plan_budget_exhausted ||
        industrial_plan_budget_failure(
            plan.source_industrial_plan.decision);
    report.stop_category =
        report.budget_exhausted
            ? "budget_exhausted"
            : "certification_failure";
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return report.budget_exhausted ? 2 : 3;
  }

  ExactDirectMorseForestBudget forest_budget;
  try {
    forest_budget = make_forest_budget(
        options, cloud, event_journal, seed_journal);
  } catch (const std::length_error& error) {
    report.terminal_stage = "forest_budget";
    report.stop_category = "budget_exhausted";
    report.stop_detail = error.what();
    report.budget_exhausted = true;
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 2;
  }
  ExactDirectMorseForestConfig forest_config;
  forest_config.locator_config.external_authority_id =
      locator_authority_id;
  const ExactDirectSparseFacetDescentBatchExecutionBudget
      execution_budget =
          make_execution_budget(options, seed_journal);

  ExactDirectMorseForestReducer reducer(
      cloud,
      facade,
      event_journal,
      seed_budget,
      seed_journal,
      forest_budget,
      forest_config);
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      index,
      cloud,
      facade,
      event_journal,
      seed_budget,
      seed_journal,
      industrial_config,
      plan_budget,
      plan,
      reducer.strict_locator());

  const Clock::time_point reducer_start = Clock::now();
  report.timings.reducer_setup_ms =
      milliseconds(reducer_start - plan_end);
  while (!executor.complete()) {
    const std::size_t batch_index =
        executor.next_source_batch_index();
    if (batch_index >= event_journal.batches.size()) {
      report.terminal_stage = "reducer_stream";
      report.stop_category = "certification_failure";
      report.stop_detail = "executor_cursor_out_of_range";
      break;
    }
    const std::uint64_t batch_token =
        checked_multiply(
            batch_index + 1U,
            3U,
            "locator replay token overflow");
    const ExactDirectSparseFacetWitness witness{
        locator_authority_id, batch_token};
    const ExactDirectSparseFacetTopKProposalTranscriptResult
        transcript = empty_proposal_transcript(
            batch_index,
            event_journal.batches[batch_index].squared_level,
            reducer.strict_locator());
    auto ticket =
        executor
            .prepare_next_sealed_with_top_k_proposal_transcript(
                witness,
                execution_budget,
                forest_budget.closure_budget,
                transcript);
    report.preparation_decision = static_cast<unsigned>(
        ticket.preparation().decision);
    report.batch_execution_decision = static_cast<unsigned>(
        ticket.preparation().batch_execution_decision);
    if (!ticket.prepared()) {
      report.terminal_stage = "reducer_preparation";
      report.budget_exhausted = batch_execution_budget_failure(
          ticket.preparation().batch_execution_decision);
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : "certification_failure";
      report.stop_detail = "exact_batch_preparation_rejected";
      break;
    }
    ++report.prepared_ticket_count;
    const ExactDirectMorseForestLiveCommitResult committed =
        reducer.fold_prepared_ticket(executor, std::move(ticket));
    report.live_commit_decision =
        static_cast<unsigned>(committed.decision);
    report.reducer_fold_decision = static_cast<unsigned>(
        committed.reducer_fold.decision);
    report.no_forbidden_global_structure_materialized =
        report.no_forbidden_global_structure_materialized &&
        !committed.forbidden_global_structure_materialized;
    if (!committed.certified_live_commit()) {
      report.terminal_stage = "reducer_live_commit";
      report.budget_exhausted =
          committed.decision ==
              ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_executor_audit_capacity_exhausted ||
          committed.reducer_fold.decision ==
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_budget_exhausted;
      report.stop_category =
          report.budget_exhausted
              ? "budget_exhausted"
              : "certification_failure";
      report.stop_detail = "live_commit_rejected";
      break;
    }
    ++report.committed_batch_count;
  }
  const Clock::time_point reducer_end = Clock::now();
  report.timings.reducer_stream_ms =
      milliseconds(reducer_end - reducer_start);
  if (!executor.complete() || !reducer.complete()) {
    if (report.terminal_stage == "not_started") {
      report.terminal_stage = "reducer_stream";
      report.stop_category = "certification_failure";
      report.stop_detail = "reducer_stream_incomplete";
    }
    report.timings.total_ms =
        milliseconds(Clock::now() - total_start);
    emit_report(report);
    return 3;
  }

  const Clock::time_point finish_start = Clock::now();
  std::optional<ExactDirectMorseForestJournalResult> forest;
  try {
    forest.emplace(reducer.finish());
  } catch (const std::logic_error& error) {
    const Clock::time_point finish_failure = Clock::now();
    report.timings.forest_finish_ms =
        milliseconds(finish_failure - finish_start);
    report.timings.total_ms =
        milliseconds(finish_failure - total_start);
    report.terminal_stage = "forest_finish";
    report.stop_category =
        std::string_view{error.what()} ==
                "a reducer final carrier partition is incomplete"
            ? "carrier_incomplete"
            : "certification_failure";
    report.stop_detail = error.what();
    emit_report(report);
    return 3;
  }
  const Clock::time_point finish_end = Clock::now();
  report.timings.forest_finish_ms =
      milliseconds(finish_end - finish_start);
  report.scientific_result_materialized = true;
  report.conditional_h0_candidate_certified =
      forest->certified_conditional_h0_candidate();
  report.no_forbidden_global_structure_materialized =
      report.no_forbidden_global_structure_materialized &&
      !forest->forbidden_global_structure_materialized &&
      !forest->gamma_cells_or_global_cofaces_materialized &&
      !forest->higher_order_delaunay_materialized;
  report.forest_birth_count =
      forest->counters.birth_record_count;
  report.forest_materialized_birth_count =
      forest->birth_records.size();
  report.forest_saddle_count =
      forest->counters.saddle_record_count;
  report.forest_atomic_group_count =
      forest->counters.atomic_group_count;
  report.forest_node_count = forest->counters.node_count;
  report.forest_materialized_node_count = forest->nodes.size();
  report.forest_final_root_count =
      forest->counters.final_root_count;
  report.forest_logical_output_entry_count =
      forest->logical_output_entry_count;
  report.forest_aggregate_closure_node_count =
      forest->counters.aggregate_closure_node_count;
  report.forest_aggregate_closure_step_call_count =
      forest->counters.aggregate_closure_step_call_count;
  report.pipeline_complete =
      report.conditional_h0_candidate_certified &&
      report.no_forbidden_global_structure_materialized;
  report.terminal_stage = "forest_materialized";
  if (report.pipeline_complete) {
    report.stop_category = "none";
    report.stop_detail = "none";
  } else if (
      !report.no_forbidden_global_structure_materialized) {
    report.stop_category = "architecture_violation";
    report.stop_detail =
        "forbidden_global_structure_materialized";
  } else {
    report.stop_category = "certification_failure";
    report.stop_detail =
        "conditional_h0_candidate_not_certified";
  }
  report.timings.total_ms =
      milliseconds(finish_end - total_start);
  emit_report(report);
  return report.pipeline_complete ? 0 : 3;
}

}  // namespace

int main(int argc, char** argv) {
  const Clock::time_point process_start = Clock::now();
  Options options;
  bool options_parsed = false;
  try {
    parse_options(argc, argv, options);
    options_parsed = true;
    return run(options);
  } catch (const std::exception& error) {
    Report report;
    report.options = options;
    report.terminal_stage =
        options_parsed ? "operational_failure" : "input_parse";
    report.stop_category =
        options_parsed ? "operational_failure" : "invalid_input";
    report.stop_detail = error.what();
    report.architecture_audit_complete = false;
    report.no_forbidden_global_structure_materialized = false;
    report.timings.total_ms =
        milliseconds(Clock::now() - process_start);
    emit_report(report);
    return options_parsed ? 5 : 4;
  } catch (...) {
    Report report;
    report.options = options;
    report.terminal_stage =
        options_parsed ? "operational_failure" : "input_parse";
    report.stop_category =
        options_parsed ? "operational_failure" : "invalid_input";
    report.stop_detail = "unknown_nonstandard_exception";
    report.architecture_audit_complete = false;
    report.no_forbidden_global_structure_materialized = false;
    report.timings.total_ms =
        milliseconds(Clock::now() - process_start);
    emit_report(report);
    return options_parsed ? 5 : 4;
  }
}
