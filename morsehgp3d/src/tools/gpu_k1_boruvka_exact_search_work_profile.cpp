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
#include <vector>

#ifndef MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND
#define MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND "fake_gpu"
#endif

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaExactSearchAudit;
using morsehgp3d::gpu::K1BoruvkaExactSearchStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedAudit;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::K1SeededExactBoruvkaResult;
using morsehgp3d::gpu::K1SeededExactBoruvkaRound;
using morsehgp3d::gpu::build_gpu_seeded_cpu_exact_external_1nn_k1_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::string_view backend_name =
    MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND;
constexpr std::string_view schema_name =
    "morsehgp3d.phase5.k1_boruvka_exact_search_work_profile.v1";
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
  for (int index = 1; index < count; ++index) {
    const std::string_view option{values[index]};
    if (option == "--help" || option == "-h") {
      std::cout
          << "Usage: morsehgp3d_gpu_k1_boruvka_exact_search_work_profile "
             "--family uniform|clusters|lattice --point-count N "
             "--window-radius W --seed S --git-sha SHA\n";
      std::exit(0);
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

void write_document(
    std::ostream& output,
    const Arguments& arguments,
    const K1SeededExactBoruvkaResult& result) {
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
  output << "],\"totals\":{\"accepted_edge_count\":"
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
         << schema_name
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
    write_document(std::cout, arguments, result);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "morsehgp3d_gpu_k1_boruvka_exact_search_work_profile: "
              << error.what() << '\n';
    return 1;
  }
}
