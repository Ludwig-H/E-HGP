#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
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
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankResult;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

struct Options {
  std::string family{"uniform_latin"};
  std::size_t point_count{12500U};
  std::size_t maximum_order{10U};
  std::size_t witness_bank_size{64U};
  std::size_t sampled_anchor_count{64U};
};

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
      << "usage: morsehgp3d_anchored_pair_witness_bank_smoke [options]\n"
      << "  --point-count N\n"
      << "  --family uniform_latin|eight_clusters\n"
      << "  --maximum-order K (1 <= K <= 10)\n"
      << "  --witness-bank-size L\n"
      << "  --sampled-anchor-count A\n";
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
    } else if (option == "--maximum-order" || option == "--K") {
      options.maximum_order = parse_size(value, option);
    } else if (option == "--witness-bank-size") {
      options.witness_bank_size = parse_size(value, option);
    } else if (option == "--sampled-anchor-count") {
      options.sampled_anchor_count = parse_size(value, option);
    } else {
      throw std::invalid_argument(
          "unknown anchored witness-bank smoke option " +
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
  if (options.maximum_order == 0U ||
      options.maximum_order > 10U) {
    throw std::invalid_argument("--maximum-order must be in [1, 10]");
  }
  if (options.witness_bank_size == 0U) {
    throw std::invalid_argument(
        "--witness-bank-size must be strictly positive");
  }
  if (options.sampled_anchor_count == 0U) {
    throw std::invalid_argument(
        "--sampled-anchor-count must be strictly positive");
  }
  return options;
}

[[nodiscard]] ExactLbvhTopKBudget top_k_budget(
    std::size_t point_count,
    std::size_t rank) {
  const std::size_t roomy =
      point_count > (std::numeric_limits<std::size_t>::max() - 64U) / 8U
          ? std::numeric_limits<std::size_t>::max()
          : 8U * point_count + 64U;
  ExactLbvhTopKBudget budget;
  budget.maximum_node_visit_count = roomy;
  budget.maximum_internal_node_expansion_count = roomy;
  budget.maximum_exact_aabb_bound_evaluation_count = roomy;
  budget.maximum_exact_point_distance_evaluation_count = roomy;
  budget.maximum_frontier_entry_count = roomy;
  budget.maximum_best_neighbor_entry_count = rank;
  budget.maximum_cutoff_shell_entry_count = point_count;
  return budget;
}

[[nodiscard]] ExactAnchoredPairWitnessBankBudget candidate_budget(
    std::size_t point_count,
    std::size_t witness_bank_size) {
  const std::size_t node_envelope =
      point_count > (std::numeric_limits<std::size_t>::max() - 16U) / 2U
          ? std::numeric_limits<std::size_t>::max()
          : 2U * point_count + 16U;
  const std::size_t predicate_envelope =
      witness_bank_size != 0U &&
              node_envelope >
                  std::numeric_limits<std::size_t>::max() /
                      witness_bank_size
          ? std::numeric_limits<std::size_t>::max()
          : node_envelope * witness_bank_size;
  ExactAnchoredPairWitnessBankBudget budget;
  budget.proposed_witness_bank_size = witness_bank_size;
  budget.witness_search_budget =
      top_k_budget(point_count, witness_bank_size);
  budget.maximum_node_visit_count = node_envelope;
  budget.maximum_internal_node_expansion_count = point_count;
  budget.maximum_traversal_stack_entry_count = node_envelope;
  budget.maximum_witness_node_predicate_count =
      predicate_envelope;
  budget.maximum_candidate_entry_count = point_count;
  budget.maximum_prune_record_count = node_envelope;
  return budget;
}

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>{duration}.count();
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_points(
    const Options& options) {
  if (options.family == "uniform_latin") {
    return smoke_clouds::uniform_latin_points(options.point_count);
  }
  return smoke_clouds::eight_clusters_points(options.point_count);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    const Clock::time_point start = Clock::now();
    const std::vector<CertifiedPoint3> input = generate_points(options);
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
    const ExactAnchoredPairWitnessBankBudget budget =
        candidate_budget(cloud.size(), effective_bank_size);

    bool all_complete = true;
    std::size_t sampled_candidates = 0U;
    std::size_t maximum_candidates_per_anchor = 0U;
    std::size_t sampled_pruned_subtrees = 0U;
    std::size_t sampled_pruned_leaf_upper_bound = 0U;
    std::size_t sampled_node_visits = 0U;
    std::size_t sampled_predicates = 0U;
    std::size_t sampled_fp64_positive = 0U;
    std::size_t sampled_fp64_negative = 0U;
    std::size_t sampled_exact_fallbacks = 0U;
    std::size_t sampled_witness_search_node_visits = 0U;
    for (std::size_t sample = 0U;
         sample < sampled_anchor_count;
         ++sample) {
      const std::size_t anchor_index = sample * anchor_step;
      const ExactAnchoredPairWitnessBankResult result =
          morsehgp3d::hierarchy::
              build_exact_anchored_pair_witness_bank_candidates(
                  index,
                  cloud,
                  static_cast<PointId>(anchor_index),
                  options.maximum_order + 1U,
                  budget);
      all_complete = all_complete && result.complete();
      sampled_candidates += result.candidate_point_ids.size();
      maximum_candidates_per_anchor = std::max(
          maximum_candidates_per_anchor,
          result.candidate_point_ids.size());
      sampled_pruned_subtrees +=
          result.audit.certified_pruned_subtree_count;
      sampled_pruned_leaf_upper_bound +=
          result.audit.certified_pruned_leaf_upper_bound;
      sampled_node_visits += result.audit.node_visit_count;
      sampled_predicates +=
          result.audit.witness_node_predicate_count;
      sampled_fp64_positive +=
          result.audit.fp64_interval_positive_count;
      sampled_fp64_negative +=
          result.audit.fp64_interval_negative_count;
      sampled_exact_fallbacks +=
          result.audit.exact_witness_node_predicate_count;
      sampled_witness_search_node_visits +=
          result.audit.witness_search_audit.node_visit_count;
    }
    const Clock::time_point finished = Clock::now();

    const double mean_candidates =
        static_cast<double>(sampled_candidates) /
        static_cast<double>(sampled_anchor_count);
    const long double extrapolated_candidate_pairs =
        static_cast<long double>(sampled_candidates) *
        static_cast<long double>(cloud.size()) /
        static_cast<long double>(sampled_anchor_count);
    std::cout
        << "{\n"
        << "  \"schema\":\"morsehgp3d.anchored-pair-witness-bank-smoke.v1\",\n"
        << "  \"phase\":\"14Q-P8b\",\n"
        << "  \"backend\":\"reference_cpu\",\n"
        << "  \"profile\":\"hgp_reduced\",\n"
        << "  \"mode\":\"exact_sparsification_diagnostic\",\n"
        << "  \"public_status\":\"not_claimed\",\n"
        << "  \"qualification_claimed\":false,\n"
        << "  \"family\":\"" << options.family << "\",\n"
        << "  \"point_count\":" << cloud.size() << ",\n"
        << "  \"maximum_order\":" << options.maximum_order << ",\n"
        << "  \"maximum_relevant_closed_rank\":"
        << options.maximum_order + 1U << ",\n"
        << "  \"witness_bank_size\":" << effective_bank_size << ",\n"
        << "  \"sampled_anchor_count\":" << sampled_anchor_count << ",\n"
        << "  \"all_sampled_anchors_complete\":"
        << (all_complete ? "true" : "false") << ",\n"
        << "  \"sampled_candidate_pairs\":" << sampled_candidates << ",\n"
        << "  \"mean_candidate_pairs_per_anchor\":"
        << std::fixed << std::setprecision(3) << mean_candidates << ",\n"
        << "  \"maximum_candidate_pairs_per_anchor\":"
        << maximum_candidates_per_anchor << ",\n"
        << "  \"extrapolated_candidate_pairs\":"
        << std::setprecision(0) << extrapolated_candidate_pairs << ",\n"
        << "  \"sampled_pruned_subtrees\":"
        << sampled_pruned_subtrees << ",\n"
        << "  \"sampled_pruned_leaf_upper_bound\":"
        << sampled_pruned_leaf_upper_bound << ",\n"
        << "  \"sampled_node_visits\":" << sampled_node_visits << ",\n"
        << "  \"sampled_witness_node_predicates\":"
        << sampled_predicates << ",\n"
        << "  \"sampled_fp64_interval_positive\":"
        << sampled_fp64_positive << ",\n"
        << "  \"sampled_fp64_interval_negative\":"
        << sampled_fp64_negative << ",\n"
        << "  \"sampled_exact_fallbacks\":"
        << sampled_exact_fallbacks << ",\n"
        << "  \"sampled_witness_search_node_visits\":"
        << sampled_witness_search_node_visits << ",\n"
        << "  \"timings_ms\":{\"canonicalization\":"
        << std::setprecision(3)
        << milliseconds(canonicalized - start)
        << ",\"lbvh\":" << milliseconds(indexed - canonicalized)
        << ",\"sampled_stream\":" << milliseconds(finished - indexed)
        << ",\"total\":" << milliseconds(finished - start) << "},\n"
        << "  \"no_forbidden_global_structure_materialized\":true,\n"
        << "  \"full_cloud_pipeline_executed\":false,\n"
        << "  \"warm_e2e_protocol_executed\":false\n"
        << "}\n";
    return all_complete ? 0 : 2;
  } catch (const std::exception& error) {
    std::cerr << "anchored pair witness-bank smoke failed: "
              << error.what() << '\n';
    return 1;
  }
}
