#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifndef MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND
#define MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND "fake_gpu"
#endif

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaExactSearchAudit;
using morsehgp3d::gpu::K1BoruvkaExactSearchStatus;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaComponentDualTreeSearchAudit;
using morsehgp3d::gpu::K1BoruvkaComponentDualTreeSearchStatus;
using morsehgp3d::gpu::K1BoruvkaComponentEnvelopeMode;
using morsehgp3d::gpu::K1BoruvkaDualTreeSearchAudit;
using morsehgp3d::gpu::K1BoruvkaDualTreeSearchStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedAudit;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::gpu::K1BoruvkaSeededComponentDualTreeRoundResolution;
using morsehgp3d::gpu::K1BoruvkaSeededDualTreeRoundResolution;
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::K1SeededExactBoruvkaResult;
using morsehgp3d::gpu::K1SeededExactBoruvkaRound;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka;
using morsehgp3d::hierarchy::K1BoruvkaRoundContraction;
using morsehgp3d::hierarchy::contract_exact_k1_boruvka_round;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::string_view backend_name =
    MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND;
constexpr std::string_view schema_name_v1 =
    "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v1";
constexpr std::string_view schema_name_v3 =
    "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v3";
constexpr std::string_view schema_name_v4 =
    "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v4";
constexpr std::string_view scientific_scope =
    "certified_local_emst_chain_work_profile_without_scalability_claim";
constexpr std::size_t maximum_profile_point_count = 1'000'000U;
static_assert(
    static_cast<PointId>(maximum_profile_point_count) <=
    CanonicalPointCloud::max_point_count);

struct Arguments {
  std::string family;
  std::size_t point_count{};
  std::size_t window_radius{};
  std::uint64_t seed{};
  std::string git_sha;
  bool compare_resolvers{false};
  bool compare_current_envelope{false};
};

struct ResolverComparisonRound {
  std::size_t round_index{};
  std::size_t pre_component_count{};
  std::size_t post_component_count{};
  K1BoruvkaDualTreeSearchAudit dynamic;
  K1BoruvkaComponentDualTreeSearchAudit direct_current;
  K1BoruvkaComponentDualTreeSearchAudit direct_frozen;
  K1BoruvkaComponentDualTreeSearchAudit direct_sparse;
};

struct ResolverComparison {
  std::vector<ResolverComparisonRound> rounds;
  bool include_current{false};
};

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error{std::string{message}};
}

void require(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    std::string_view label) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [position, error] = std::from_chars(begin, end, value);
  if (text.empty() || error != std::errc{} || position != end) {
    fail(std::string{label} + " must be a canonical natural number");
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_seed(std::string_view text) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [position, error] = std::from_chars(begin, end, value);
  if (text.empty() || error != std::errc{} || position != end) {
    fail("--seed must be a canonical uint64");
  }
  return value;
}

[[nodiscard]] bool valid_git_sha(std::string_view value) {
  return value.size() == 40U &&
         std::all_of(value.begin(), value.end(), [](char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

[[nodiscard]] Arguments parse_arguments(int count, char** values) {
  Arguments arguments;
  bool family_seen = false;
  bool point_count_seen = false;
  bool radius_seen = false;
  bool seed_seen = false;
  bool git_seen = false;
  bool comparison_seen = false;
  for (int index = 1; index < count; ++index) {
    const std::string_view option{values[index]};
    if (option == "--help" || option == "-h") {
      std::cout
          << "Usage: morsehgp3d_gpu_k1_boruvka_exact_search_work_profile "
             "--family uniform|clusters|lattice --point-count N "
             "--window-radius W --seed S --git-sha SHA "
             "[--compare-resolvers|--compare-current-envelope]\n";
      std::exit(0);
    }
    if (option == "--compare-resolvers") {
      require(
          !comparison_seen,
          "resolver comparison options are mutually exclusive");
      arguments.compare_resolvers = true;
      comparison_seen = true;
      continue;
    }
    if (option == "--compare-current-envelope") {
      require(
          !comparison_seen,
          "resolver comparison options are mutually exclusive");
      arguments.compare_current_envelope = true;
      comparison_seen = true;
      continue;
    }
    if (index + 1 >= count) {
      fail(std::string{"missing value after "} + std::string{option});
    }
    const std::string_view value{values[++index]};
    if (option == "--family") {
      require(!family_seen, "--family may be supplied only once");
      arguments.family = std::string{value};
      family_seen = true;
    } else if (option == "--point-count") {
      require(
          !point_count_seen, "--point-count may be supplied only once");
      arguments.point_count = parse_size(value, "--point-count");
      point_count_seen = true;
    } else if (option == "--window-radius") {
      require(
          !radius_seen, "--window-radius may be supplied only once");
      arguments.window_radius = parse_size(value, "--window-radius");
      radius_seen = true;
    } else if (option == "--seed") {
      require(!seed_seen, "--seed may be supplied only once");
      arguments.seed = parse_seed(value);
      seed_seen = true;
    } else if (option == "--git-sha") {
      require(!git_seen, "--git-sha may be supplied only once");
      arguments.git_sha = std::string{value};
      git_seen = true;
    } else {
      fail(std::string{"unknown option: "} + std::string{option});
    }
  }
  require(
      family_seen && point_count_seen && radius_seen && seed_seen && git_seen,
      "all exact-search work-profile options are mandatory");
  require(
      arguments.family == "uniform" || arguments.family == "clusters" ||
          arguments.family == "lattice",
      "--family must be uniform, clusters or lattice");
  require(arguments.point_count >= 2U, "--point-count must be at least two");
  require(
      arguments.point_count <= maximum_profile_point_count,
      "--point-count exceeds the injective work-profile generator domain");
  require(
      arguments.window_radius > 0U &&
          arguments.window_radius <=
              std::numeric_limits<std::size_t>::max() / 2U,
      "--window-radius must have a finite nonzero 2W bound");
  require(
      valid_git_sha(arguments.git_sha),
      "--git-sha must be 40 lowercase hexadecimal characters");
  return arguments;
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value =
      (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value =
      (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] double unit_dyadic(std::uint64_t value) noexcept {
  constexpr std::uint64_t mask =
      (std::uint64_t{1U} << 21U) - std::uint64_t{1U};
  constexpr double denominator = 2097152.0;
  return static_cast<double>(value & mask) / denominator;
}

[[nodiscard]] std::vector<CertifiedPoint3> make_points(
    const Arguments& arguments) {
  std::vector<CertifiedPoint3> points;
  points.reserve(arguments.point_count);
  if (arguments.family == "lattice") {
    std::size_t side = 1U;
    while (side <= arguments.point_count / side / side &&
           side * side * side < arguments.point_count) {
      ++side;
    }
    while (side * side * side < arguments.point_count) {
      ++side;
    }
    for (std::size_t index = 0U; index < arguments.point_count; ++index) {
      const std::size_t x = index % side;
      const std::size_t y = (index / side) % side;
      const std::size_t z = index / (side * side);
      points.push_back(CertifiedPoint3::from_binary64(
          static_cast<double>(x),
          static_cast<double>(y),
          static_cast<double>(z)));
    }
    return points;
  }

  const double point_denominator =
      static_cast<double>(arguments.point_count) * 64.0;
  for (std::size_t index = 0U; index < arguments.point_count; ++index) {
    const std::uint64_t keyed =
        arguments.seed ^ static_cast<std::uint64_t>(index);
    const double random_y = unit_dyadic(splitmix64(keyed));
    const double random_z = unit_dyadic(splitmix64(keyed + 1U));
    if (arguments.family == "uniform") {
      const double x =
          (static_cast<double>(index) + 0.5) /
          static_cast<double>(arguments.point_count);
      points.push_back(CertifiedPoint3::from_binary64(x, random_y, random_z));
      continue;
    }

    const std::size_t cluster = index % 8U;
    const std::size_t local_index = index / 8U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const double unique_offset =
        (static_cast<double>(local_index) + 1.0) / point_denominator;
    const double jitter_y = (random_y - 0.5) / 32.0;
    const double jitter_z = (random_z - 0.5) / 32.0;
    points.push_back(CertifiedPoint3::from_binary64(
        center_x + unique_offset,
        center_y + jitter_y,
        center_z + jitter_z));
  }
  return points;
}

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    fail("an exact-search work-profile counter overflowed");
  }
  return left + right;
}

[[nodiscard]] bool dual_tree_expansion_arity_closed(
    std::size_t visit_count,
    std::size_t expansion_count) noexcept {
  if (visit_count < 2U) {
    return false;
  }
  const std::size_t edge_count = visit_count - 1U;
  const std::size_t minimum_expansion_count =
      edge_count / 3U + (edge_count % 3U == 0U ? 0U : 1U);
  const std::size_t maximum_expansion_count = (visit_count - 2U) / 2U;
  return minimum_expansion_count <= expansion_count &&
         expansion_count <= maximum_expansion_count;
}

void require_result_certificates(
    const K1SeededExactBoruvkaResult& result,
    const Arguments& arguments) {
  require(
      result.point_count == arguments.point_count &&
          result.morton_seed_policy ==
              K1BoruvkaMortonSeedPolicy{arguments.window_radius} &&
          result.bounded_morton_seed_chain_certified &&
          result.exact_external_1nn_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.fresh_replay_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified &&
          result.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          result.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only,
      "the exact-search work profile did not close its chain certificates");
  require(
      result.counters.point_count == arguments.point_count &&
          result.counters.round_count == result.rounds.size() &&
          result.counters.final_component_count == 1U &&
          result.counters.accepted_edge_count == arguments.point_count - 1U &&
          result.counters.morton_gpu_kernel_launch_count ==
              result.rounds.size() &&
          result.counters.morton_gpu_synchronization_count ==
              result.rounds.size() &&
          result.counters.exact_point_query_count ==
              result.counters.exact_point_minimum_count &&
          result.counters.exact_aabb_bound_evaluation_count ==
              result.counters.exact_node_visit_count,
      "the exact-search work profile did not close its aggregate counters");
  for (const K1SeededExactBoruvkaRound& round : result.rounds) {
    const K1BoruvkaMortonSeedAudit& seed = round.morton_seed_audit;
    const K1BoruvkaExactSearchAudit& search = round.exact_search_audit;
    require(
        round.seed_status ==
                K1BoruvkaSeedStatus::
                    bounded_morton_window_external_exact_monotone_certified &&
            round.search_status ==
                K1BoruvkaExactSearchStatus::
                    exact_external_1nn_branch_and_bound_certified &&
            round.decision_status ==
                K1HybridBoruvkaDecisionStatus::
                    cpu_exact_kappa_minima_certified &&
            round.contraction_status ==
                K1HybridBoruvkaContractionStatus::
                    cpu_exact_canonical_contraction_certified &&
            seed.complete_source_coverage_certified &&
            seed.bounded_window_certified &&
            seed.external_targets_recertified &&
            seed.exact_monotone_cutoff_certified &&
            search.complete_frontier_exhaustion_certified &&
            search.strict_only_aabb_pruning_certified &&
            search.canonical_kappa_resolution_certified &&
            search.point_minima_complete && search.component_minima_complete,
        "an exact-search work-profile round lost a certificate");
  }
}

void require_dynamic_resolver_audit(
    const K1BoruvkaSeededDualTreeRoundResolution& resolution,
    const MortonLbvhIndex& index,
    std::size_t point_count,
    std::size_t component_count) {
  const K1BoruvkaDualTreeSearchAudit& audit = resolution.search_audit;
  const std::size_t unordered_pair_count =
      point_count * (point_count - 1U) / 2U;
  const std::size_t visited_partition = checked_sum(
      audit.cpu_node_pair_expansion_count,
      checked_sum(
          audit.cpu_uniform_same_component_pair_prune_count,
          checked_sum(
              audit.cpu_strict_aabb_pair_prune_count,
              audit.cpu_exact_point_pair_distance_evaluation_count)));
  const std::size_t maximum_strict_decrease_count = checked_sum(
      audit.cpu_exact_point_pair_distance_evaluation_count,
      audit.cpu_exact_point_pair_distance_evaluation_count);
  const bool ancestor_update_bound_representable =
      audit.cpu_strict_incumbent_decrease_count == 0U ||
      audit.lbvh_maximum_depth <=
          std::numeric_limits<std::size_t>::max() /
              audit.cpu_strict_incumbent_decrease_count;
  require(
      resolution.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          resolution.search_status ==
              K1BoruvkaDualTreeSearchStatus::
                  exact_external_1nn_shared_lbvh_dual_tree_certified &&
          audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count &&
          audit.seed_incumbent_count == point_count &&
          audit.dynamic_incumbent_node_count == audit.resident_node_count &&
          resolution.point_minima.size() == point_count &&
          resolution.component_minima.size() == component_count &&
          audit.point_minimum_count == point_count &&
          audit.component_minimum_count == component_count &&
          audit.unordered_point_pair_count == unordered_pair_count &&
          audit.covered_unordered_point_pair_count == unordered_pair_count &&
          audit.lbvh_maximum_depth ==
              index.build_counters().maximum_depth &&
          audit.certified_depth_first_frontier_bound ==
              2U * audit.lbvh_maximum_depth + 1U &&
          audit.certified_node_pair_visit_bound ==
              point_count * (point_count + 1U) - 1U &&
          audit.maximum_cpu_frontier_size > 0U &&
          audit.maximum_cpu_frontier_size <=
              audit.certified_depth_first_frontier_bound &&
          audit.maximum_cpu_frontier_size <=
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_aabb_pair_bound_evaluation_count ==
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_point_pair_distance_evaluation_count <=
              unordered_pair_count &&
          audit.cpu_node_pair_visit_count <=
              audit.certified_node_pair_visit_bound &&
          visited_partition == audit.cpu_node_pair_visit_count &&
          dual_tree_expansion_arity_closed(
              audit.cpu_node_pair_visit_count,
              audit.cpu_node_pair_expansion_count) &&
          audit.cpu_strict_incumbent_decrease_count <=
              maximum_strict_decrease_count &&
          ancestor_update_bound_representable &&
          audit.cpu_incumbent_ancestor_update_count <=
              audit.lbvh_maximum_depth *
                  audit.cpu_strict_incumbent_decrease_count,
      "the dynamic dual-tree comparison audit lost a work invariant");
  require(
      audit.frozen_labels_certified &&
          audit.lbvh_topology_and_exact_aabbs_certified &&
          audit.complete_source_seed_coverage_certified &&
          audit.external_seed_targets_recertified &&
          audit.exact_seed_cutoffs_recertified &&
          audit.dynamic_incumbent_tree_certified &&
          audit.canonical_unordered_pair_partition_certified &&
          audit.uniform_component_pair_prunes_certified &&
          audit.strict_only_aabb_pair_pruning_certified &&
          audit.depth_first_frontier_bound_certified &&
          audit.node_pair_visit_bound_certified &&
          audit.complete_frontier_exhaustion_certified &&
          audit.canonical_kappa_resolution_certified &&
          audit.point_minima_complete && audit.component_minima_complete,
      "the dynamic dual-tree comparison audit lost a certificate");
}

void require_direct_resolver_audit(
    const K1BoruvkaSeededComponentDualTreeRoundResolution& resolution,
    const MortonLbvhIndex& index,
    std::size_t point_count,
    std::size_t component_count,
    K1BoruvkaComponentEnvelopeMode envelope_mode) {
  const K1BoruvkaComponentDualTreeSearchAudit& audit =
      resolution.search_audit;
  const std::size_t unordered_pair_count =
      point_count * (point_count - 1U) / 2U;
  const std::size_t visited_partition = checked_sum(
      audit.cpu_node_pair_expansion_count,
      checked_sum(
          audit.cpu_uniform_same_component_pair_prune_count,
          checked_sum(
              audit.cpu_strict_aabb_pair_prune_count,
              audit.cpu_exact_point_pair_distance_evaluation_count)));
  const auto count_at_most_product = [](
      std::size_t count, std::size_t left, std::size_t right) {
    return left != 0U &&
                   right > std::numeric_limits<std::size_t>::max() / left
               ? true
               : count <= left * right;
  };
  const bool current_mode =
      envelope_mode == K1BoruvkaComponentEnvelopeMode::
                           exact_current_maximal_uniform_roots;
  const bool envelope_updates_closed =
      (envelope_mode == K1BoruvkaComponentEnvelopeMode::frozen_initial &&
       audit.cpu_component_witness_leaf_update_count == 0U &&
       audit.cpu_component_witness_ancestor_update_count == 0U &&
       audit.component_uniform_root_count == 0U &&
       audit.component_uniform_root_leaf_coverage_count == 0U &&
       audit.cpu_component_uniform_root_update_count == 0U &&
       audit.cpu_component_mixed_ancestor_recomputation_count == 0U &&
       audit.cpu_component_mixed_ancestor_update_count == 0U) ||
      (envelope_mode ==
           K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone &&
       audit.cpu_component_witness_leaf_update_count ==
           audit.cpu_strict_component_cutoff_decrease_count &&
       count_at_most_product(
           audit.cpu_component_witness_ancestor_update_count,
           audit.cpu_component_witness_leaf_update_count,
           audit.lbvh_maximum_depth) &&
       audit.component_uniform_root_count == 0U &&
       audit.component_uniform_root_leaf_coverage_count == 0U &&
       audit.cpu_component_uniform_root_update_count == 0U &&
       audit.cpu_component_mixed_ancestor_recomputation_count == 0U &&
       audit.cpu_component_mixed_ancestor_update_count == 0U) ||
      (current_mode &&
       audit.cpu_component_witness_leaf_update_count == 0U &&
       audit.cpu_component_witness_ancestor_update_count == 0U &&
       audit.component_uniform_root_count >= component_count &&
       audit.component_uniform_root_count <= point_count &&
       audit.component_uniform_root_leaf_coverage_count == point_count &&
       audit.cpu_component_uniform_root_update_count >=
           audit.cpu_strict_component_cutoff_decrease_count &&
       count_at_most_product(
           audit.cpu_component_uniform_root_update_count,
           audit.cpu_strict_component_cutoff_decrease_count,
           audit.component_uniform_root_count) &&
       count_at_most_product(
           audit.cpu_component_mixed_ancestor_recomputation_count,
           audit.cpu_component_uniform_root_update_count,
           audit.lbvh_maximum_depth) &&
       audit.cpu_component_mixed_ancestor_update_count <=
           audit.cpu_component_mixed_ancestor_recomputation_count);
  require(
      resolution.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          resolution.search_status ==
              K1BoruvkaComponentDualTreeSearchStatus::
                  exact_component_minima_shared_lbvh_dual_tree_certified &&
          audit.component_envelope_mode == envelope_mode &&
          audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count &&
          audit.point_seed_count == point_count &&
          audit.component_seed_incumbent_count == component_count &&
          audit.target_component_seed_offer_count == point_count &&
          audit.target_component_seed_kappa_update_count <= point_count &&
          audit.target_component_seed_strict_cutoff_decrease_count <=
              audit.target_component_seed_kappa_update_count &&
          audit.component_cutoff_upper_envelope_node_count ==
              audit.resident_node_count &&
          resolution.component_minima.size() == component_count &&
          audit.component_minimum_count == component_count &&
          audit.unordered_point_pair_count == unordered_pair_count &&
          audit.covered_unordered_point_pair_count == unordered_pair_count &&
          audit.lbvh_maximum_depth ==
              index.build_counters().maximum_depth &&
          audit.certified_depth_first_frontier_bound ==
              2U * audit.lbvh_maximum_depth + 1U &&
          audit.certified_node_pair_visit_bound ==
              point_count * (point_count + 1U) - 1U &&
          audit.maximum_cpu_frontier_size > 0U &&
          audit.maximum_cpu_frontier_size <=
              audit.certified_depth_first_frontier_bound &&
          audit.maximum_cpu_frontier_size <=
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_aabb_pair_bound_evaluation_count ==
              audit.cpu_node_pair_visit_count &&
          audit.cpu_exact_point_pair_distance_evaluation_count <=
              unordered_pair_count &&
          audit.cpu_node_pair_visit_count <=
              audit.certified_node_pair_visit_bound &&
          visited_partition == audit.cpu_node_pair_visit_count &&
          dual_tree_expansion_arity_closed(
              audit.cpu_node_pair_visit_count,
              audit.cpu_node_pair_expansion_count) &&
          audit.cpu_component_kappa_update_count <=
              2U * audit.cpu_exact_point_pair_distance_evaluation_count &&
          audit.cpu_strict_component_cutoff_decrease_count <=
              audit.cpu_component_kappa_update_count &&
          envelope_updates_closed,
      "the component-direct comparison audit lost a work invariant");
  require(
      audit.frozen_labels_certified &&
          audit.lbvh_topology_and_exact_aabbs_certified &&
          audit.complete_source_seed_coverage_certified &&
          audit.external_seed_targets_recertified &&
          audit.exact_seed_cutoffs_recertified &&
          audit.component_seed_reduction_certified &&
          audit.bidirectional_component_seed_reduction_certified &&
          audit.component_cutoff_upper_envelope_certified &&
          audit.live_component_cutoff_upper_bound_certified &&
          audit.pointwise_at_most_frozen_envelope_certified &&
          audit.maximal_uniform_component_roots_certified == current_mode &&
          audit.exact_current_component_envelope_certified == current_mode &&
          audit.canonical_unordered_pair_partition_certified &&
          audit.uniform_component_pair_prunes_certified &&
          audit.strict_only_aabb_pair_pruning_certified &&
          audit.depth_first_frontier_bound_certified &&
          audit.node_pair_visit_bound_certified &&
          audit.complete_frontier_exhaustion_certified &&
          audit.canonical_kappa_resolution_certified &&
          audit.component_minima_complete,
      "the component-direct comparison audit lost a certificate");
}

[[nodiscard]] ResolverComparison build_resolver_comparison(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const Arguments& arguments,
    const K1SeededExactBoruvkaResult& reference,
    bool include_current) {
  ResolverComparison comparison;
  comparison.include_current = include_current;
  comparison.rounds.reserve(reference.rounds.size());
  std::vector<PointId> labels(arguments.point_count);
  for (std::size_t point_index = 0U;
       point_index < arguments.point_count;
       ++point_index) {
    labels[point_index] = static_cast<PointId>(point_index);
  }

  K1BoruvkaCandidateContext context{index, cloud};
  std::size_t component_count = arguments.point_count;
  const K1BoruvkaMortonSeedPolicy policy{arguments.window_radius};
  for (std::size_t round_index = 0U;
       round_index < reference.rounds.size();
       ++round_index) {
    const K1SeededExactBoruvkaRound& expected =
        reference.rounds[round_index];
    require(
        expected.exact_decision.round_index == round_index &&
            expected.exact_decision.frozen_component_count ==
                component_count,
        "the comparison component path differs before resolution");
    const K1BoruvkaSeededDualTreeRoundResolution dynamic =
        context.resolve_round_exact_external_1nn_dual_tree(
            cloud, std::span<const PointId>{labels}, policy);
    const K1BoruvkaSeededComponentDualTreeRoundResolution direct_frozen =
        context.resolve_round_exact_component_minima_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            policy,
            K1BoruvkaComponentEnvelopeMode::frozen_initial);
    const K1BoruvkaSeededComponentDualTreeRoundResolution direct_sparse =
        context.resolve_round_exact_component_minima_dual_tree(
            cloud,
            std::span<const PointId>{labels},
            policy,
            K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone);
    K1BoruvkaSeededComponentDualTreeRoundResolution direct_current;
    if (include_current) {
      direct_current =
          context.resolve_round_exact_component_minima_dual_tree(
              cloud,
              std::span<const PointId>{labels},
              policy,
              K1BoruvkaComponentEnvelopeMode::
                  exact_current_maximal_uniform_roots);
    }
    require_dynamic_resolver_audit(
        dynamic, index, arguments.point_count, component_count);
    require_direct_resolver_audit(
        direct_frozen,
        index,
        arguments.point_count,
        component_count,
        K1BoruvkaComponentEnvelopeMode::frozen_initial);
    require_direct_resolver_audit(
        direct_sparse,
        index,
        arguments.point_count,
        component_count,
        K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone);
    if (include_current) {
      require_direct_resolver_audit(
          direct_current,
          index,
          arguments.point_count,
          component_count,
          K1BoruvkaComponentEnvelopeMode::
              exact_current_maximal_uniform_roots);
    }
    require(
        dynamic.morton_seed_audit == expected.morton_seed_audit &&
            direct_frozen.morton_seed_audit == expected.morton_seed_audit &&
            direct_sparse.morton_seed_audit == expected.morton_seed_audit &&
            (!include_current ||
             direct_current.morton_seed_audit == expected.morton_seed_audit),
        "the compared resolvers did not consume the reference Morton seed");
    require(
        dynamic.component_minima == expected.exact_decision.component_minima &&
            direct_frozen.component_minima ==
                expected.exact_decision.component_minima &&
            direct_sparse.component_minima ==
                expected.exact_decision.component_minima &&
            (!include_current ||
             direct_current.component_minima ==
                 expected.exact_decision.component_minima),
        "the compared resolvers disagree on exact component minima");
    require(
        dynamic.search_audit.unordered_point_pair_count ==
                direct_frozen.search_audit.unordered_point_pair_count &&
            dynamic.search_audit.covered_unordered_point_pair_count ==
                direct_frozen.search_audit.covered_unordered_point_pair_count &&
            dynamic.search_audit.unordered_point_pair_count ==
                direct_sparse.search_audit.unordered_point_pair_count &&
            dynamic.search_audit.covered_unordered_point_pair_count ==
                direct_sparse.search_audit.covered_unordered_point_pair_count &&
            (!include_current ||
             (dynamic.search_audit.unordered_point_pair_count ==
                  direct_current.search_audit.unordered_point_pair_count &&
              dynamic.search_audit.covered_unordered_point_pair_count ==
                  direct_current.search_audit
                      .covered_unordered_point_pair_count)),
        "the compared resolvers disagree on unordered-pair coverage");
    require(
        direct_sparse.search_audit.cpu_node_pair_visit_count <=
                direct_frozen.search_audit.cpu_node_pair_visit_count &&
            direct_sparse.search_audit.cpu_node_pair_expansion_count <=
                direct_frozen.search_audit.cpu_node_pair_expansion_count &&
            direct_sparse.search_audit
                    .cpu_exact_aabb_pair_bound_evaluation_count <=
                direct_frozen.search_audit
                    .cpu_exact_aabb_pair_bound_evaluation_count &&
            direct_sparse.search_audit
                    .cpu_exact_point_pair_distance_evaluation_count <=
                direct_frozen.search_audit
                    .cpu_exact_point_pair_distance_evaluation_count,
        "the sparse component envelope exceeded frozen direct traversal work");
    if (include_current) {
      const auto current_work_at_most = [](
          const K1BoruvkaComponentDualTreeSearchAudit& current,
          const auto& other) {
        return current.cpu_node_pair_visit_count <=
                   other.cpu_node_pair_visit_count &&
               current.cpu_node_pair_expansion_count <=
                   other.cpu_node_pair_expansion_count &&
               current.cpu_exact_aabb_pair_bound_evaluation_count <=
                   other.cpu_exact_aabb_pair_bound_evaluation_count &&
               current.cpu_exact_point_pair_distance_evaluation_count <=
                   other.cpu_exact_point_pair_distance_evaluation_count;
      };
      require(
          current_work_at_most(
              direct_current.search_audit, dynamic.search_audit) &&
              current_work_at_most(
                  direct_current.search_audit,
                  direct_frozen.search_audit) &&
              current_work_at_most(
                  direct_current.search_audit,
                  direct_sparse.search_audit),
          "the exact-current component envelope exceeded another resolver's traversal work");
    }

    K1BoruvkaRoundContraction contraction =
        contract_exact_k1_boruvka_round(
            cloud,
            std::span<const PointId>{labels},
            std::span<const morsehgp3d::hierarchy::K1BoruvkaComponentMinimum>{
                direct_frozen.component_minima});
    require(
        contraction.accepted_edges ==
                expected.canonical_contraction.accepted_edges &&
            contraction.post_round_component_count ==
                expected.canonical_contraction.post_round_component_count,
        "the component-direct comparison contraction differs from reference");
    if (include_current) {
      const K1BoruvkaRoundContraction current_contraction =
          contract_exact_k1_boruvka_round(
              cloud,
              std::span<const PointId>{labels},
              std::span<
                  const morsehgp3d::hierarchy::K1BoruvkaComponentMinimum>{
                  direct_current.component_minima});
      require(
          current_contraction.accepted_edges ==
                  contraction.accepted_edges &&
              current_contraction.post_round_component_labels ==
                  contraction.post_round_component_labels &&
              current_contraction.post_round_component_count ==
                  contraction.post_round_component_count,
          "the exact-current comparison contraction differs from frozen direct");
    }
    comparison.rounds.push_back(ResolverComparisonRound{
        round_index,
        component_count,
        contraction.post_round_component_count,
        dynamic.search_audit,
        direct_current.search_audit,
        direct_frozen.search_audit,
        direct_sparse.search_audit});
    labels = std::move(contraction.post_round_component_labels);
    component_count = contraction.post_round_component_count;
  }
  require(
      component_count == 1U &&
          comparison.rounds.size() == reference.rounds.size(),
      "the resolver comparison did not close the exact component chain");
  return comparison;
}

void write_boolean(std::ostream& output, bool value) {
  output << (value ? "true" : "false");
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator()
         << "\",\"numerator\":\"" << level.numerator() << "\"}";
}

void write_round(
    std::ostream& output,
    const K1SeededExactBoruvkaRound& round) {
  const K1BoruvkaMortonSeedAudit& seed = round.morton_seed_audit;
  const K1BoruvkaExactSearchAudit& search = round.exact_search_audit;
  const std::size_t exact_operation_count = checked_sum(
      seed.exact_seed_distance_evaluation_count,
      checked_sum(
          search.cpu_exact_aabb_bound_evaluation_count,
          search.cpu_exact_point_distance_evaluation_count));
  output << "{\"accepted_edge_count\":"
         << round.canonical_contraction.accepted_edges.size()
         << ",\"component_minimum_count\":"
         << round.exact_decision.component_minima.size()
         << ",\"exact_operation_count_unweighted\":"
         << exact_operation_count
         << ",\"exact_search\":{\"aabb_bound_evaluation_count\":"
         << search.cpu_exact_aabb_bound_evaluation_count
         << ",\"frontier_peak_per_source\":"
         << search.maximum_cpu_frontier_size_per_source
         << ",\"internal_node_expansion_count\":"
         << search.cpu_internal_node_expansion_count
         << ",\"node_visit_count\":" << search.cpu_node_visit_count
         << ",\"node_visit_peak_per_source\":"
         << search.maximum_cpu_node_visit_count_per_source
         << ",\"point_distance_evaluation_count\":"
         << search.cpu_exact_point_distance_evaluation_count
         << ",\"point_distance_evaluation_peak_per_source\":"
         << search
                .maximum_cpu_exact_point_distance_evaluation_count_per_source
         << ",\"point_query_count\":" << search.point_query_count
         << ",\"seed_leaf_distance_reuse_count\":"
         << search.cpu_seed_leaf_distance_reuse_count
         << ",\"strict_aabb_prune_count\":"
         << search.cpu_strict_aabb_prune_count
         << ",\"uniform_component_prune_count\":"
         << search.cpu_uniform_component_prune_count
         << "},\"morton_seed\":{\"exact_fallback_count\":"
         << seed.exact_fallback_count
         << ",\"exact_seed_distance_evaluation_count\":"
         << seed.exact_seed_distance_evaluation_count
         << ",\"exact_selected_proposal_count\":"
         << seed.exact_selected_proposal_count
         << ",\"exact_strict_improvement_count\":"
         << seed.exact_strict_improvement_count
         << ",\"external_neighbor_count\":"
         << seed.external_neighbor_count
         << ",\"floating_proposal_count\":"
         << seed.floating_proposal_count
         << ",\"inspected_neighbor_count\":"
         << seed.inspected_neighbor_count
         << ",\"source_count\":" << seed.source_count
         << "},\"persistent_point_minimum_count\":0"
         << ",\"post_component_count\":"
         << round.canonical_contraction.post_round_component_count
         << ",\"pre_component_count\":"
         << round.exact_decision.frozen_component_count
         << ",\"round_index\":" << round.exact_decision.round_index
         << '}';
}

void write_dynamic_resolver_work(
    std::ostream& output,
    const K1BoruvkaDualTreeSearchAudit& audit) {
  output << "{\"aabb_pair_bound_evaluation_count\":"
         << audit.cpu_exact_aabb_pair_bound_evaluation_count
         << ",\"ancestor_update_count\":"
         << audit.cpu_incumbent_ancestor_update_count
         << ",\"frontier_peak\":" << audit.maximum_cpu_frontier_size
         << ",\"node_pair_expansion_count\":"
         << audit.cpu_node_pair_expansion_count
         << ",\"node_pair_visit_count\":"
         << audit.cpu_node_pair_visit_count
         << ",\"point_pair_distance_evaluation_count\":"
         << audit.cpu_exact_point_pair_distance_evaluation_count
         << ",\"strict_aabb_pair_prune_count\":"
         << audit.cpu_strict_aabb_pair_prune_count
         << ",\"strict_incumbent_decrease_count\":"
         << audit.cpu_strict_incumbent_decrease_count
         << ",\"uniform_same_component_pair_prune_count\":"
         << audit.cpu_uniform_same_component_pair_prune_count << '}';
}

void write_direct_resolver_work(
    std::ostream& output,
    const K1BoruvkaComponentDualTreeSearchAudit& audit) {
  output << "{\"aabb_pair_bound_evaluation_count\":"
         << audit.cpu_exact_aabb_pair_bound_evaluation_count
         << ",\"component_cutoff_upper_envelope_node_count\":"
         << audit.component_cutoff_upper_envelope_node_count
         << ",\"component_kappa_update_count\":"
         << audit.cpu_component_kappa_update_count
         << ",\"component_witness_ancestor_update_count\":"
         << audit.cpu_component_witness_ancestor_update_count
         << ",\"component_witness_leaf_update_count\":"
         << audit.cpu_component_witness_leaf_update_count
         << ",\"frontier_peak\":" << audit.maximum_cpu_frontier_size
         << ",\"node_pair_expansion_count\":"
         << audit.cpu_node_pair_expansion_count
         << ",\"node_pair_visit_count\":"
         << audit.cpu_node_pair_visit_count
         << ",\"point_pair_distance_evaluation_count\":"
         << audit.cpu_exact_point_pair_distance_evaluation_count
         << ",\"strict_aabb_pair_prune_count\":"
         << audit.cpu_strict_aabb_pair_prune_count
         << ",\"strict_component_cutoff_decrease_count\":"
         << audit.cpu_strict_component_cutoff_decrease_count
         << ",\"target_component_seed_kappa_update_count\":"
         << audit.target_component_seed_kappa_update_count
         << ",\"target_component_seed_offer_count\":"
         << audit.target_component_seed_offer_count
         << ",\"target_component_seed_strict_cutoff_decrease_count\":"
         << audit.target_component_seed_strict_cutoff_decrease_count
         << ",\"uniform_same_component_pair_prune_count\":"
         << audit.cpu_uniform_same_component_pair_prune_count << '}';
}

void write_current_resolver_work(
    std::ostream& output,
    const K1BoruvkaComponentDualTreeSearchAudit& audit) {
  output << "{\"aabb_pair_bound_evaluation_count\":"
         << audit.cpu_exact_aabb_pair_bound_evaluation_count
         << ",\"component_cutoff_upper_envelope_node_count\":"
         << audit.component_cutoff_upper_envelope_node_count
         << ",\"component_kappa_update_count\":"
         << audit.cpu_component_kappa_update_count
         << ",\"component_mixed_ancestor_recomputation_count\":"
         << audit.cpu_component_mixed_ancestor_recomputation_count
         << ",\"component_mixed_ancestor_update_count\":"
         << audit.cpu_component_mixed_ancestor_update_count
         << ",\"component_uniform_root_count\":"
         << audit.component_uniform_root_count
         << ",\"component_uniform_root_leaf_coverage_count\":"
         << audit.component_uniform_root_leaf_coverage_count
         << ",\"component_uniform_root_update_count\":"
         << audit.cpu_component_uniform_root_update_count
         << ",\"component_witness_ancestor_update_count\":"
         << audit.cpu_component_witness_ancestor_update_count
         << ",\"component_witness_leaf_update_count\":"
         << audit.cpu_component_witness_leaf_update_count
         << ",\"frontier_peak\":" << audit.maximum_cpu_frontier_size
         << ",\"node_pair_expansion_count\":"
         << audit.cpu_node_pair_expansion_count
         << ",\"node_pair_visit_count\":"
         << audit.cpu_node_pair_visit_count
         << ",\"point_pair_distance_evaluation_count\":"
         << audit.cpu_exact_point_pair_distance_evaluation_count
         << ",\"strict_aabb_pair_prune_count\":"
         << audit.cpu_strict_aabb_pair_prune_count
         << ",\"strict_component_cutoff_decrease_count\":"
         << audit.cpu_strict_component_cutoff_decrease_count
         << ",\"target_component_seed_kappa_update_count\":"
         << audit.target_component_seed_kappa_update_count
         << ",\"target_component_seed_offer_count\":"
         << audit.target_component_seed_offer_count
         << ",\"target_component_seed_strict_cutoff_decrease_count\":"
         << audit.target_component_seed_strict_cutoff_decrease_count
         << ",\"uniform_same_component_pair_prune_count\":"
         << audit.cpu_uniform_same_component_pair_prune_count << '}';
}

void accumulate_dynamic_resolver_work(
    K1BoruvkaDualTreeSearchAudit& totals,
    const K1BoruvkaDualTreeSearchAudit& round) {
  totals.maximum_cpu_frontier_size = std::max(
      totals.maximum_cpu_frontier_size, round.maximum_cpu_frontier_size);
  totals.cpu_node_pair_visit_count = checked_sum(
      totals.cpu_node_pair_visit_count, round.cpu_node_pair_visit_count);
  totals.cpu_node_pair_expansion_count = checked_sum(
      totals.cpu_node_pair_expansion_count,
      round.cpu_node_pair_expansion_count);
  totals.cpu_exact_aabb_pair_bound_evaluation_count = checked_sum(
      totals.cpu_exact_aabb_pair_bound_evaluation_count,
      round.cpu_exact_aabb_pair_bound_evaluation_count);
  totals.cpu_exact_point_pair_distance_evaluation_count = checked_sum(
      totals.cpu_exact_point_pair_distance_evaluation_count,
      round.cpu_exact_point_pair_distance_evaluation_count);
  totals.cpu_strict_incumbent_decrease_count = checked_sum(
      totals.cpu_strict_incumbent_decrease_count,
      round.cpu_strict_incumbent_decrease_count);
  totals.cpu_incumbent_ancestor_update_count = checked_sum(
      totals.cpu_incumbent_ancestor_update_count,
      round.cpu_incumbent_ancestor_update_count);
  totals.cpu_uniform_same_component_pair_prune_count = checked_sum(
      totals.cpu_uniform_same_component_pair_prune_count,
      round.cpu_uniform_same_component_pair_prune_count);
  totals.cpu_strict_aabb_pair_prune_count = checked_sum(
      totals.cpu_strict_aabb_pair_prune_count,
      round.cpu_strict_aabb_pair_prune_count);
}

void accumulate_direct_resolver_work(
    K1BoruvkaComponentDualTreeSearchAudit& totals,
    const K1BoruvkaComponentDualTreeSearchAudit& round) {
  totals.maximum_cpu_frontier_size = std::max(
      totals.maximum_cpu_frontier_size, round.maximum_cpu_frontier_size);
  totals.cpu_node_pair_visit_count = checked_sum(
      totals.cpu_node_pair_visit_count, round.cpu_node_pair_visit_count);
  totals.cpu_node_pair_expansion_count = checked_sum(
      totals.cpu_node_pair_expansion_count,
      round.cpu_node_pair_expansion_count);
  totals.cpu_exact_aabb_pair_bound_evaluation_count = checked_sum(
      totals.cpu_exact_aabb_pair_bound_evaluation_count,
      round.cpu_exact_aabb_pair_bound_evaluation_count);
  totals.cpu_exact_point_pair_distance_evaluation_count = checked_sum(
      totals.cpu_exact_point_pair_distance_evaluation_count,
      round.cpu_exact_point_pair_distance_evaluation_count);
  totals.cpu_component_kappa_update_count = checked_sum(
      totals.cpu_component_kappa_update_count,
      round.cpu_component_kappa_update_count);
  totals.cpu_strict_component_cutoff_decrease_count = checked_sum(
      totals.cpu_strict_component_cutoff_decrease_count,
      round.cpu_strict_component_cutoff_decrease_count);
  totals.cpu_component_witness_ancestor_update_count = checked_sum(
      totals.cpu_component_witness_ancestor_update_count,
      round.cpu_component_witness_ancestor_update_count);
  totals.cpu_component_witness_leaf_update_count = checked_sum(
      totals.cpu_component_witness_leaf_update_count,
      round.cpu_component_witness_leaf_update_count);
  totals.component_uniform_root_count = checked_sum(
      totals.component_uniform_root_count,
      round.component_uniform_root_count);
  totals.component_uniform_root_leaf_coverage_count = checked_sum(
      totals.component_uniform_root_leaf_coverage_count,
      round.component_uniform_root_leaf_coverage_count);
  totals.cpu_component_uniform_root_update_count = checked_sum(
      totals.cpu_component_uniform_root_update_count,
      round.cpu_component_uniform_root_update_count);
  totals.cpu_component_mixed_ancestor_recomputation_count = checked_sum(
      totals.cpu_component_mixed_ancestor_recomputation_count,
      round.cpu_component_mixed_ancestor_recomputation_count);
  totals.cpu_component_mixed_ancestor_update_count = checked_sum(
      totals.cpu_component_mixed_ancestor_update_count,
      round.cpu_component_mixed_ancestor_update_count);
  totals.cpu_uniform_same_component_pair_prune_count = checked_sum(
      totals.cpu_uniform_same_component_pair_prune_count,
      round.cpu_uniform_same_component_pair_prune_count);
  totals.cpu_strict_aabb_pair_prune_count = checked_sum(
      totals.cpu_strict_aabb_pair_prune_count,
      round.cpu_strict_aabb_pair_prune_count);
  totals.target_component_seed_offer_count = checked_sum(
      totals.target_component_seed_offer_count,
      round.target_component_seed_offer_count);
  totals.target_component_seed_kappa_update_count = checked_sum(
      totals.target_component_seed_kappa_update_count,
      round.target_component_seed_kappa_update_count);
  totals.target_component_seed_strict_cutoff_decrease_count = checked_sum(
      totals.target_component_seed_strict_cutoff_decrease_count,
      round.target_component_seed_strict_cutoff_decrease_count);
  totals.component_cutoff_upper_envelope_node_count = checked_sum(
      totals.component_cutoff_upper_envelope_node_count,
      round.component_cutoff_upper_envelope_node_count);
}

void write_resolver_comparison(
    std::ostream& output,
    const ResolverComparison& comparison) {
  K1BoruvkaDualTreeSearchAudit dynamic_totals;
  K1BoruvkaComponentDualTreeSearchAudit direct_current_totals;
  K1BoruvkaComponentDualTreeSearchAudit direct_frozen_totals;
  K1BoruvkaComponentDualTreeSearchAudit direct_sparse_totals;
  output << "{\"rounds\":[";
  for (std::size_t index = 0U; index < comparison.rounds.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const ResolverComparisonRound& round = comparison.rounds[index];
    output << '{';
    if (comparison.include_current) {
      output << "\"direct_current\":";
      write_current_resolver_work(output, round.direct_current);
      output << ',';
    }
    output << "\"direct_frozen\":";
    write_direct_resolver_work(output, round.direct_frozen);
    output << ",\"direct_sparse\":";
    write_direct_resolver_work(output, round.direct_sparse);
    output << ",\"dynamic\":";
    write_dynamic_resolver_work(output, round.dynamic);
    output << ",\"post_component_count\":" << round.post_component_count
           << ",\"pre_component_count\":" << round.pre_component_count
           << ",\"round_index\":" << round.round_index
           << ",\"unordered_point_pair_count\":"
           << round.dynamic.unordered_point_pair_count << '}';
    accumulate_dynamic_resolver_work(dynamic_totals, round.dynamic);
    if (comparison.include_current) {
      accumulate_direct_resolver_work(
          direct_current_totals, round.direct_current);
    }
    accumulate_direct_resolver_work(
        direct_frozen_totals, round.direct_frozen);
    accumulate_direct_resolver_work(
        direct_sparse_totals, round.direct_sparse);
  }
  output << "],\"totals\":{";
  if (comparison.include_current) {
    output << "\"direct_current\":";
    write_current_resolver_work(output, direct_current_totals);
    output << ',';
  }
  output << "\"direct_frozen\":";
  write_direct_resolver_work(output, direct_frozen_totals);
  output << ",\"direct_sparse\":";
  write_direct_resolver_work(output, direct_sparse_totals);
  output << ",\"dynamic\":";
  write_dynamic_resolver_work(output, dynamic_totals);
  output << "}}";
}

void write_document(
    std::ostream& output,
    const Arguments& arguments,
    const K1SeededExactBoruvkaResult& result,
    const ResolverComparison* comparison) {
  const auto& counters = result.counters;
  const std::size_t exact_operation_count = checked_sum(
      counters.morton_exact_seed_distance_evaluation_count,
      checked_sum(
          counters.exact_aabb_bound_evaluation_count,
          counters.exact_point_distance_evaluation_count));
  output << "{\"artifact_kind\":\"work_profile\""
         << ",\"artifact_role\":\"benchmark_only\",\"backend\":\""
         << backend_name
         << "\",\"candidate_record_count\":0"
         << ",\"decision_backend\":\"reference_cpu\""
         << ",\"generator\":{\"algorithm\":\"deterministic_dyadic_v1\""
         << ",\"family\":\"" << arguments.family
         << "\",\"seed\":" << arguments.seed << '}'
         << ",\"git\":{\"sha\":\"" << arguments.git_sha << "\"}"
         << ",\"hierarchy_reduction_status\":\"not_performed\""
         << ",\"mode\":\"benchmark\",\"phase\":\"5\""
         << ",\"point_count\":" << arguments.point_count
         << ",\"policy\":{\"window_radius\":"
         << arguments.window_radius << '}'
         << ",\"profile\":\"hgp_reduced\""
         << ",\"qualification_claimed\":false"
         << ",\"reference_backend\":\"reference_cpu\""
         << ",\"replay_backend\":\"" << backend_name
         << "\",\"result\":{"
         << "\"certificates\":{\"bounded_morton_seed_chain\":";
  write_boolean(output, result.bounded_morton_seed_chain_certified);
  output << ",\"canonical_contraction_chain\":";
  write_boolean(output, result.canonical_contraction_chain_certified);
  output << ",\"cpu_exact_decision_chain\":";
  write_boolean(output, result.cpu_exact_decision_chain_certified);
  output << ",\"exact_external_1nn_chain\":";
  write_boolean(output, result.exact_external_1nn_chain_certified);
  output << ",\"fresh_replay\":";
  write_boolean(output, result.fresh_replay_certified);
  output << ",\"local_emst_witness\":";
  write_boolean(output, result.emst_witness_certified);
  output << ",\"reference_cpu_witness\":";
  write_boolean(output, result.reference_cpu_witness_certified);
  output << "},\"component_count_path\":[" << arguments.point_count;
  for (const K1SeededExactBoruvkaRound& round : result.rounds) {
    output << ','
           << round.canonical_contraction.post_round_component_count;
  }
  output << "],\"exact_weights\":{\"hgp\":";
  write_level(output, result.total_hgp_weight);
  output << ",\"squared\":";
  write_level(output, result.total_squared_weight);
  output << "},\"rounds\":[";
  for (std::size_t index = 0U; index < result.rounds.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_round(output, result.rounds[index]);
  }
  output << ']';
  if (comparison != nullptr) {
    output << ",\"resolver_comparison\":";
    write_resolver_comparison(output, *comparison);
  }
  output << ",\"totals\":{\"accepted_edge_count\":"
         << counters.accepted_edge_count
         << ",\"component_minimum_count\":"
         << counters.component_minimum_count
         << ",\"exact_operation_count_unweighted\":"
         << exact_operation_count
         << ",\"exact_search\":{\"aabb_bound_evaluation_count\":"
         << counters.exact_aabb_bound_evaluation_count
         << ",\"frontier_peak_per_source\":"
         << counters.maximum_exact_frontier_size_per_source
         << ",\"internal_node_expansion_count\":"
         << counters.exact_internal_node_expansion_count
         << ",\"node_visit_count\":" << counters.exact_node_visit_count
         << ",\"node_visit_peak_per_source\":"
         << counters.maximum_exact_node_visit_count_per_source
         << ",\"point_distance_evaluation_count\":"
         << counters.exact_point_distance_evaluation_count
         << ",\"point_distance_evaluation_peak_per_source\":"
         << counters
                .maximum_exact_point_distance_evaluation_count_per_source
         << ",\"point_query_count\":"
         << counters.exact_point_query_count
         << ",\"seed_leaf_distance_reuse_count\":"
         << counters.exact_seed_leaf_distance_reuse_count
         << ",\"strict_aabb_prune_count\":"
         << counters.exact_strict_aabb_prune_count
         << ",\"uniform_component_prune_count\":"
         << counters.exact_uniform_component_prune_count
         << "},\"morton_seed\":{\"exact_fallback_count\":"
         << counters.morton_exact_fallback_count
         << ",\"exact_seed_distance_evaluation_count\":"
         << counters.morton_exact_seed_distance_evaluation_count
         << ",\"exact_selected_proposal_count\":"
         << counters.morton_exact_selected_proposal_count
         << ",\"exact_strict_improvement_count\":"
         << counters.morton_exact_strict_improvement_count
         << ",\"external_neighbor_count\":"
         << counters.morton_external_neighbor_count
         << ",\"floating_proposal_count\":"
         << counters.morton_floating_proposal_count
         << ",\"inspected_neighbor_count\":"
         << counters.morton_inspected_neighbor_count
         << ",\"source_count\":" << counters.morton_seed_source_count
         << "},\"persistent_point_minimum_count\":0"
         << ",\"round_count\":" << counters.round_count << "}}"
         << ",\"scalability_claimed\":false,\"schema\":\""
         << (comparison == nullptr
                 ? schema_name_v1
                 : (comparison->include_current
                        ? schema_name_v4
                        : schema_name_v3))
         << "\",\"scientific_public_status\":null"
         << ",\"scientific_result_claimed\":false"
         << ",\"scope\":\"" << scientific_scope
         << "\",\"status\":\"measured\""
         << ",\"work_accounting\":\"producer_chain_only_replay_and_reference_excluded\"}\n";
}

}  // namespace

int main(int argument_count, char** argument_values) {
  try {
    const Arguments arguments =
        parse_arguments(argument_count, argument_values);
    const std::vector<CertifiedPoint3> points = make_points(arguments);
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{points});
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const K1SeededExactBoruvkaResult result =
        build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka(
            index,
            cloud,
            K1BoruvkaMortonSeedPolicy{arguments.window_radius});
    require_result_certificates(result, arguments);
    ResolverComparison comparison;
    const ResolverComparison* comparison_output = nullptr;
    if (arguments.compare_resolvers ||
        arguments.compare_current_envelope) {
      comparison =
          build_resolver_comparison(
              index,
              cloud,
              arguments,
              result,
              arguments.compare_current_envelope);
      comparison_output = &comparison;
    }
    write_document(std::cout, arguments, result, comparison_output);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "morsehgp3d_gpu_k1_boruvka_exact_search_work_profile: "
              << error.what() << '\n';
    return 1;
  }
}
