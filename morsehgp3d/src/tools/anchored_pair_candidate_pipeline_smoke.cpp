#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationBudget;
using morsehgp3d::hierarchy::
    ExactAnchoredPairCandidateClassificationStatus;
using morsehgp3d::hierarchy::ExactAnchoredPairCandidateStopReason;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::
    ExactAnchoredPairWitnessBankProposalPolicy;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::ExactLbvhTopKStopReason;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

struct Options {
  std::size_t point_count{12500U};
  std::string family{"uniform_latin"};
  std::size_t maximum_order{10U};
  std::size_t witness_bank_size{64U};
  std::size_t sampled_anchor_count{64U};
  std::string bank_policy{"exact_top_k"};
  std::size_t morton_window_radius{256U};
  std::size_t maximum_morton_window_inspection_count{512U};
};

struct Metrics {
  std::size_t sampled_anchor_requested_count{};
  std::size_t sampled_anchor_started_count{};
  std::size_t sampled_anchor_bank_completed_count{};
  std::size_t sampled_anchor_completed_count{};
  std::size_t candidate_pair_count{};
  std::size_t maximum_candidate_pairs_per_anchor{};
  std::size_t bank_pruned_subtree_count{};
  std::size_t bank_pruned_leaf_upper_bound{};
  std::size_t bank_candidate_node_visit_count{};
  std::size_t bank_top_k_node_visit_count{};
  std::size_t bank_morton_window_inspection_count{};
  std::size_t bank_witness_predicate_count{};
  std::size_t bank_exact_predicate_fallback_count{};
  std::size_t classification_started_count{};
  std::size_t classification_complete_count{};
  std::size_t classification_above_rank_count{};
  std::size_t classification_event_count{};
  std::size_t classification_extra_shell_count{};
  std::size_t classification_node_visit_count{};
  std::size_t classification_interval_exterior_node_count{};
  std::size_t classification_interval_interior_node_count{};
  std::size_t classification_exact_node_fallback_count{};
  std::size_t classification_minimum_bound_count{};
  std::size_t classification_maximum_bound_count{};
  std::size_t classification_exact_point_count{};
  std::size_t classification_bulk_interior_point_count{};
  std::size_t classification_bulk_exterior_point_count{};
  double bank_milliseconds{};
  double classification_milliseconds{};
};

struct PipelineStop {
  bool budget_exhausted{false};
  std::string_view stage{"none"};
  std::string_view reason{"none"};
  PointId anchor_point_id{};
  PointId candidate_point_id{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

void add_to(
    std::size_t& accumulator,
    std::size_t value,
    const char* message) {
  accumulator = checked_add(accumulator, value, message);
}

[[nodiscard]] std::size_t saturated_linear_envelope(
    std::size_t coefficient,
    std::size_t value,
    std::size_t constant) {
  if (value >
      (std::numeric_limits<std::size_t>::max() - constant) /
          coefficient) {
    return std::numeric_limits<std::size_t>::max();
  }
  return coefficient * value + constant;
}

[[nodiscard]] std::size_t saturated_product(
    std::size_t left,
    std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::numeric_limits<std::size_t>::max();
  }
  return left * right;
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    std::string_view option) {
  std::size_t value = 0U;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != text.data() + text.size()) {
    throw std::invalid_argument(
        std::string{option} + " expects an unsigned integer");
  }
  return value;
}

void print_usage() {
  std::cout
      << "usage: morsehgp3d_anchored_pair_candidate_pipeline_smoke "
         "[options]\n"
      << "  --point-count N\n"
      << "  --family uniform_latin|eight_clusters\n"
      << "  --K K (1 <= K <= 10)\n"
      << "  --bank L (1 <= L <= 64)\n"
      << "  --bank-policy exact_top_k|morton_window\n"
      << "  --window-radius R\n"
      << "  --window-inspections I\n"
      << "  --sample A\n";
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option{argv[index]};
    if (option == "--help" || option == "-h") {
      print_usage();
      std::exit(0);
    }
    if (index + 1 >= argc) {
      throw std::invalid_argument(
          std::string{option} + " requires one value");
    }
    const std::string_view value{argv[++index]};
    if (option == "--point-count") {
      options.point_count = parse_size(value, option);
    } else if (option == "--family") {
      options.family = value;
    } else if (option == "--K" || option == "--maximum-order") {
      options.maximum_order = parse_size(value, option);
    } else if (option == "--bank" ||
               option == "--witness-bank-size") {
      options.witness_bank_size = parse_size(value, option);
    } else if (option == "--sample" ||
               option == "--sampled-anchor-count") {
      options.sampled_anchor_count = parse_size(value, option);
    } else if (option == "--bank-policy") {
      options.bank_policy = value;
    } else if (option == "--window-radius") {
      options.morton_window_radius = parse_size(value, option);
    } else if (option == "--window-inspections") {
      options.maximum_morton_window_inspection_count =
          parse_size(value, option);
    } else {
      throw std::invalid_argument(
          "unknown anchored candidate-pipeline smoke option " +
          std::string{option});
    }
  }
  if (options.point_count < 2U) {
    throw std::invalid_argument("--point-count must be at least two");
  }
  if (options.family != "uniform_latin" &&
      options.family != "eight_clusters") {
    throw std::invalid_argument(
        "--family must be uniform_latin or eight_clusters");
  }
  if (options.maximum_order == 0U || options.maximum_order > 10U) {
    throw std::invalid_argument("--K must be in [1, 10]");
  }
  if (options.witness_bank_size == 0U ||
      options.witness_bank_size >
          morsehgp3d::hierarchy::
              exact_anchored_pair_witness_bank_maximum_size) {
    throw std::invalid_argument("--bank must be in [1, 64]");
  }
  if (options.sampled_anchor_count == 0U) {
    throw std::invalid_argument("--sample must be strictly positive");
  }
  if (options.bank_policy != "exact_top_k" &&
      options.bank_policy != "morton_window") {
    throw std::invalid_argument(
        "--bank-policy must be exact_top_k or morton_window");
  }
  return options;
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  return smoke_clouds::eight_clusters_points(options.point_count);
}

[[nodiscard]] ExactLbvhTopKBudget top_k_budget(
    std::size_t point_count,
    std::size_t rank) {
  const std::size_t envelope =
      saturated_linear_envelope(8U, point_count, 64U);
  ExactLbvhTopKBudget budget;
  budget.maximum_node_visit_count = envelope;
  budget.maximum_internal_node_expansion_count = envelope;
  budget.maximum_exact_aabb_bound_evaluation_count = envelope;
  budget.maximum_exact_point_distance_evaluation_count = envelope;
  budget.maximum_frontier_entry_count = envelope;
  budget.maximum_best_neighbor_entry_count = rank;
  budget.maximum_cutoff_shell_entry_count = point_count;
  return budget;
}

[[nodiscard]] ExactAnchoredPairWitnessBankBudget bank_budget(
    std::size_t point_count,
    std::size_t witness_bank_size,
    const Options& options) {
  const std::size_t node_envelope =
      saturated_linear_envelope(2U, point_count, 16U);
  ExactAnchoredPairWitnessBankBudget budget;
  budget.proposed_witness_bank_size = witness_bank_size;
  budget.proposal_policy =
      options.bank_policy == "exact_top_k"
          ? ExactAnchoredPairWitnessBankProposalPolicy::exact_top_k
          : ExactAnchoredPairWitnessBankProposalPolicy::
                bounded_morton_window;
  budget.witness_search_budget =
      top_k_budget(point_count, witness_bank_size);
  budget.morton_window_radius = options.morton_window_radius;
  budget.maximum_morton_window_inspection_count =
      options.maximum_morton_window_inspection_count;
  budget.maximum_node_visit_count = node_envelope;
  budget.maximum_internal_node_expansion_count = node_envelope;
  budget.maximum_traversal_stack_entry_count = node_envelope;
  budget.maximum_witness_node_predicate_count =
      saturated_product(node_envelope, witness_bank_size);
  budget.maximum_candidate_entry_count = point_count;
  budget.maximum_prune_record_count = node_envelope;
  return budget;
}

[[nodiscard]] ExactAnchoredPairCandidateClassificationBudget
classification_budget(std::size_t point_count) {
  return ExactAnchoredPairCandidateClassificationBudget{
      saturated_linear_envelope(2U, point_count, 16U)};
}

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>{duration}.count();
}

[[nodiscard]] std::string_view bank_stop_reason(
    ExactAnchoredPairCandidateStopReason reason) {
  switch (reason) {
    case ExactAnchoredPairCandidateStopReason::none:
      return "none";
    case ExactAnchoredPairCandidateStopReason::node_visit_limit:
      return "node_visit_limit";
    case ExactAnchoredPairCandidateStopReason::
        internal_node_expansion_limit:
      return "internal_node_expansion_limit";
    case ExactAnchoredPairCandidateStopReason::
        traversal_stack_entry_limit:
      return "traversal_stack_entry_limit";
    case ExactAnchoredPairCandidateStopReason::candidate_entry_limit:
      return "candidate_entry_limit";
  }
  return "invalid";
}

[[nodiscard]] std::string_view top_k_stop_reason(
    ExactLbvhTopKStopReason reason) {
  switch (reason) {
    case ExactLbvhTopKStopReason::none:
      return "none";
    case ExactLbvhTopKStopReason::node_visit_limit:
      return "node_visit_limit";
    case ExactLbvhTopKStopReason::internal_node_expansion_limit:
      return "internal_node_expansion_limit";
    case ExactLbvhTopKStopReason::exact_aabb_bound_evaluation_limit:
      return "exact_aabb_bound_evaluation_limit";
    case ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit:
      return "exact_point_distance_evaluation_limit";
    case ExactLbvhTopKStopReason::frontier_entry_limit:
      return "frontier_entry_limit";
    case ExactLbvhTopKStopReason::best_neighbor_entry_limit:
      return "best_neighbor_entry_limit";
    case ExactLbvhTopKStopReason::cutoff_shell_entry_limit:
      return "cutoff_shell_entry_limit";
  }
  return "invalid";
}

void print_report(
    const Options& options,
    std::size_t point_count,
    std::size_t maximum_closed_rank,
    std::size_t effective_bank_size,
    const Metrics& metrics,
    const PipelineStop& stop,
    double generation_milliseconds,
    double canonicalization_milliseconds,
    double lbvh_milliseconds,
    double total_milliseconds) {
  const double mean_candidates =
      metrics.sampled_anchor_bank_completed_count == 0U
          ? 0.0
          : static_cast<double>(metrics.candidate_pair_count) /
                static_cast<double>(
                    metrics.sampled_anchor_bank_completed_count);
  std::cout
      << "{\n"
      << "  \"schema\":\"morsehgp3d.anchored-pair-candidate-pipeline-"
         "smoke.v1\",\n"
      << "  \"phase\":\""
      << (options.bank_policy == "exact_top_k" ? "14Q-P8c" : "14Q-P8d")
      << "\",\n"
      << "  \"backend\":\"reference_cpu\",\n"
      << "  \"profile\":\"hgp_reduced\",\n"
      << "  \"mode\":\"exact_sparsification_diagnostic\",\n"
      << "  \"public_status\":\"not_claimed\",\n"
      << "  \"qualification\":false,\n"
      << "  \"pipeline_status\":\""
      << (stop.budget_exhausted ? "budget_exhausted" : "complete")
      << "\",\n"
      << "  \"stop_stage\":\"" << stop.stage << "\",\n"
      << "  \"stop_reason\":\"" << stop.reason << "\",\n"
      << "  \"stop_anchor_point_id\":" << stop.anchor_point_id
      << ",\n"
      << "  \"stop_candidate_point_id\":" << stop.candidate_point_id
      << ",\n"
      << "  \"family\":\"" << options.family << "\",\n"
      << "  \"point_count\":" << point_count << ",\n"
      << "  \"maximum_order\":" << options.maximum_order << ",\n"
      << "  \"maximum_relevant_closed_rank\":"
      << maximum_closed_rank << ",\n"
      << "  \"witness_bank_size\":" << effective_bank_size << ",\n"
      << "  \"witness_bank_proposal\":\"" << options.bank_policy
      << "\",\n"
      << "  \"morton_window_radius\":"
      << options.morton_window_radius << ",\n"
      << "  \"maximum_morton_window_inspection_count\":"
      << options.maximum_morton_window_inspection_count << ",\n"
      << "  \"sampled_anchor_requested_count\":"
      << metrics.sampled_anchor_requested_count << ",\n"
      << "  \"sampled_anchor_started_count\":"
      << metrics.sampled_anchor_started_count << ",\n"
      << "  \"sampled_anchor_bank_completed_count\":"
      << metrics.sampled_anchor_bank_completed_count << ",\n"
      << "  \"sampled_anchor_completed_count\":"
      << metrics.sampled_anchor_completed_count << ",\n"
      << "  \"candidate_pair_count\":"
      << metrics.candidate_pair_count << ",\n"
      << "  \"mean_candidate_pairs_per_bank_completed_anchor\":"
      << std::fixed << std::setprecision(3) << mean_candidates << ",\n"
      << "  \"maximum_candidate_pairs_per_anchor\":"
      << metrics.maximum_candidate_pairs_per_anchor << ",\n"
      << "  \"bank_pruned_subtree_count\":"
      << metrics.bank_pruned_subtree_count << ",\n"
      << "  \"bank_pruned_leaf_upper_bound\":"
      << metrics.bank_pruned_leaf_upper_bound << ",\n"
      << "  \"bank_candidate_node_visit_count\":"
      << metrics.bank_candidate_node_visit_count << ",\n"
      << "  \"bank_top_k_node_visit_count\":"
      << metrics.bank_top_k_node_visit_count << ",\n"
      << "  \"bank_morton_window_inspection_count\":"
      << metrics.bank_morton_window_inspection_count << ",\n"
      << "  \"bank_witness_predicate_count\":"
      << metrics.bank_witness_predicate_count << ",\n"
      << "  \"bank_exact_predicate_fallback_count\":"
      << metrics.bank_exact_predicate_fallback_count << ",\n"
      << "  \"classification_started_count\":"
      << metrics.classification_started_count << ",\n"
      << "  \"classification_complete_count\":"
      << metrics.classification_complete_count << ",\n"
      << "  \"classification_above_rank_count\":"
      << metrics.classification_above_rank_count << ",\n"
      << "  \"classification_event_count\":"
      << metrics.classification_event_count << ",\n"
      << "  \"classification_extra_shell_count\":"
      << metrics.classification_extra_shell_count << ",\n"
      << "  \"classification_node_visit_count\":"
      << metrics.classification_node_visit_count << ",\n"
      << "  \"classification_interval_exterior_node_count\":"
      << metrics.classification_interval_exterior_node_count << ",\n"
      << "  \"classification_interval_interior_node_count\":"
      << metrics.classification_interval_interior_node_count << ",\n"
      << "  \"classification_exact_node_fallback_count\":"
      << metrics.classification_exact_node_fallback_count << ",\n"
      << "  \"classification_minimum_bound_count\":"
      << metrics.classification_minimum_bound_count << ",\n"
      << "  \"classification_maximum_bound_count\":"
      << metrics.classification_maximum_bound_count << ",\n"
      << "  \"classification_exact_point_count\":"
      << metrics.classification_exact_point_count << ",\n"
      << "  \"classification_bulk_interior_point_count\":"
      << metrics.classification_bulk_interior_point_count << ",\n"
      << "  \"classification_bulk_exterior_point_count\":"
      << metrics.classification_bulk_exterior_point_count << ",\n"
      << "  \"timings_ms\":{\"generation\":"
      << generation_milliseconds
      << ",\"canonicalization\":" << canonicalization_milliseconds
      << ",\"lbvh\":" << lbvh_milliseconds
      << ",\"bank\":" << metrics.bank_milliseconds
      << ",\"classification\":"
      << metrics.classification_milliseconds
      << ",\"total\":" << total_milliseconds << "},\n"
      << "  \"candidate_state_scope\":\"single_anchor_only\",\n"
      << "  \"no_global_pair_cell_coface_incidence_or_gamma_arena\":true\n"
      << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const Clock::time_point started = Clock::now();
    const std::vector<CertifiedPoint3> input = generate_points(options);
    const Clock::time_point generated = Clock::now();
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{input});
    const Clock::time_point canonicalized = Clock::now();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const Clock::time_point indexed = Clock::now();

    const std::size_t sampled_anchor_count =
        std::min(options.sampled_anchor_count, cloud.size());
    const std::size_t anchor_step =
        std::max(std::size_t{1U}, cloud.size() / sampled_anchor_count);
    const std::size_t effective_bank_size =
        std::min(options.witness_bank_size, cloud.size() - 1U);
    const std::size_t maximum_closed_rank =
        std::min(options.maximum_order + 1U, cloud.size());
    const ExactAnchoredPairWitnessBankBudget candidate_budget =
        bank_budget(cloud.size(), effective_bank_size, options);
    const ExactAnchoredPairCandidateClassificationBudget classifier_budget =
        classification_budget(cloud.size());

    Metrics metrics;
    metrics.sampled_anchor_requested_count = sampled_anchor_count;
    PipelineStop stop;
    bool should_stop = false;
    for (std::size_t sample = 0U;
         sample < sampled_anchor_count && !should_stop;
         ++sample) {
      const PointId anchor_point_id =
          static_cast<PointId>(sample * anchor_step);
      ++metrics.sampled_anchor_started_count;
      const Clock::time_point bank_started = Clock::now();
      const auto candidates = morsehgp3d::hierarchy::
          build_exact_anchored_pair_witness_bank_candidates(
              index,
              cloud,
              anchor_point_id,
              maximum_closed_rank,
              candidate_budget);
      metrics.bank_milliseconds +=
          milliseconds(Clock::now() - bank_started);
      add_to(
          metrics.bank_pruned_subtree_count,
          candidates.audit.certified_pruned_subtree_count,
          "the sampled bank prune count overflows size_t");
      add_to(
          metrics.bank_pruned_leaf_upper_bound,
          candidates.audit.certified_pruned_leaf_upper_bound,
          "the sampled bank pruned-leaf bound overflows size_t");
      add_to(
          metrics.bank_candidate_node_visit_count,
          candidates.audit.node_visit_count,
          "the sampled bank node-visit count overflows size_t");
      add_to(
          metrics.bank_top_k_node_visit_count,
          candidates.audit.witness_search_audit.node_visit_count,
          "the sampled top-k node-visit count overflows size_t");
      add_to(
          metrics.bank_morton_window_inspection_count,
          candidates.audit.morton_window_inspection_count,
          "the sampled Morton-window inspection count overflows size_t");
      add_to(
          metrics.bank_witness_predicate_count,
          candidates.audit.witness_node_predicate_count,
          "the sampled witness-predicate count overflows size_t");
      add_to(
          metrics.bank_exact_predicate_fallback_count,
          candidates.audit.exact_witness_node_predicate_count,
          "the sampled exact-predicate count overflows size_t");

      if (!candidates.complete()) {
        stop = PipelineStop{
            true,
            "candidate_bank",
            bank_stop_reason(candidates.stop_reason),
            anchor_point_id,
            PointId{0}};
        should_stop = true;
        break;
      }
      if (!candidates.audit.witness_search_complete) {
        stop = PipelineStop{
            true,
            "witness_top_k",
            top_k_stop_reason(
                candidates.audit.witness_search_stop_reason),
            anchor_point_id,
            PointId{0}};
        should_stop = true;
        break;
      }
      if (candidates.audit.predicate_budget_fail_open_count != 0U ||
          candidates.audit.prune_record_capacity_fail_open_count != 0U) {
        stop = PipelineStop{
            true,
            "candidate_bank_fail_open",
            candidates.audit.predicate_budget_fail_open_count != 0U
                ? "witness_predicate_limit"
                : "prune_record_limit",
            anchor_point_id,
            PointId{0}};
        should_stop = true;
        break;
      }

      add_to(
          metrics.candidate_pair_count,
          candidates.candidate_point_ids.size(),
          "the sampled candidate-pair count overflows size_t");
      ++metrics.sampled_anchor_bank_completed_count;
      metrics.maximum_candidate_pairs_per_anchor = std::max(
          metrics.maximum_candidate_pairs_per_anchor,
          candidates.candidate_point_ids.size());
      for (const PointId candidate_point_id :
           candidates.candidate_point_ids) {
        ++metrics.classification_started_count;
        const Clock::time_point classification_started = Clock::now();
        const auto classification = morsehgp3d::hierarchy::
            classify_exact_anchored_pair_candidate(
                index,
                cloud,
                std::array<PointId, 2>{
                    anchor_point_id, candidate_point_id},
                maximum_closed_rank,
                classifier_budget);
        metrics.classification_milliseconds += milliseconds(
            Clock::now() - classification_started);
        add_to(
            metrics.classification_node_visit_count,
            classification.audit.node_visit_count,
            "the classification node-visit count overflows size_t");
        add_to(
            metrics.classification_interval_exterior_node_count,
            classification.audit.interval_exterior_node_count,
            "the interval-exterior node count overflows size_t");
        add_to(
            metrics.classification_interval_interior_node_count,
            classification.audit.interval_interior_node_count,
            "the interval-interior node count overflows size_t");
        add_to(
            metrics.classification_exact_node_fallback_count,
            classification.audit.exact_node_fallback_count,
            "the exact-fallback node count overflows size_t");
        add_to(
            metrics.classification_minimum_bound_count,
            classification.audit.exact_minimum_phi_aabb_bound_count,
            "the classification minimum-bound count overflows size_t");
        add_to(
            metrics.classification_maximum_bound_count,
            classification.audit.exact_maximum_phi_aabb_bound_count,
            "the classification maximum-bound count overflows size_t");
        add_to(
            metrics.classification_exact_point_count,
            classification.audit.exact_point_classification_count,
            "the classification exact-point count overflows size_t");
        add_to(
            metrics.classification_bulk_interior_point_count,
            classification.audit.bulk_interior_point_count,
            "the classification bulk-interior count overflows size_t");
        add_to(
            metrics.classification_bulk_exterior_point_count,
            classification.audit.bulk_exterior_point_count,
            "the classification bulk-exterior count overflows size_t");

        if (classification.status ==
            ExactAnchoredPairCandidateClassificationStatus::
                budget_exhausted) {
          stop = PipelineStop{
              true,
              "candidate_classification",
              "node_visit_limit",
              anchor_point_id,
              candidate_point_id};
          should_stop = true;
          break;
        }
        if (classification.above_rank()) {
          ++metrics.classification_above_rank_count;
          if (classification.event.has_value() ||
              classification.relevant_extra_shell_diagnostic.has_value()) {
            throw std::logic_error(
                "an above-rank classification published a record");
          }
          continue;
        }
        if (!classification.complete()) {
          throw std::logic_error(
              "an anchored classification returned an unknown status");
        }
        ++metrics.classification_complete_count;
        const bool has_event = classification.event.has_value();
        const bool has_extra_shell =
            classification.relevant_extra_shell_diagnostic.has_value();
        if (has_event == has_extra_shell) {
          throw std::logic_error(
              "a complete classification must own exactly one record");
        }
        if (has_event) {
          ++metrics.classification_event_count;
        } else {
          ++metrics.classification_extra_shell_count;
        }
      }
      if (!should_stop) {
        ++metrics.sampled_anchor_completed_count;
      }
    }

    const Clock::time_point finished = Clock::now();
    print_report(
        options,
        cloud.size(),
        maximum_closed_rank,
        effective_bank_size,
        metrics,
        stop,
        milliseconds(generated - started),
        milliseconds(canonicalized - generated),
        milliseconds(indexed - canonicalized),
        milliseconds(finished - started));
    return stop.budget_exhausted ? 2 : 0;
  } catch (const std::exception& error) {
    std::cerr << "anchored pair candidate-pipeline smoke failed: "
              << error.what() << '\n';
    return 1;
  }
}
