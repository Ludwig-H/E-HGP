#include "morsehgp3d/morsehgp3d.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

namespace {

using morsehgp3d::api::CertifiedProjectableSimplex;
using morsehgp3d::api::CertifiedTowerEdge;
using morsehgp3d::api::CertifiedTowerInput;
using morsehgp3d::api::CertifiedTowerNode;
using morsehgp3d::api::DensityLevel;
using morsehgp3d::api::PointHierarchyOptions;
using morsehgp3d::api::SimplexPointWeighting;
using morsehgp3d::api::SourceNodeId;
using morsehgp3d::api::TowerEdgeKind;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;

using Clock = std::chrono::steady_clock;

struct Configuration {
  std::uint64_t point_count{50'000U};
};

[[nodiscard]] ExactLevel level(const std::uint64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] CanonicalId fixture_receipt(const char digit) {
  return CanonicalId::from_lower_hex(std::string(64U, digit));
}

[[nodiscard]] std::uint64_t parse_positive_count(
    const std::string_view text) {
  std::size_t consumed = 0U;
  const unsigned long long parsed = std::stoull(std::string{text}, &consumed);
  if (consumed != text.size() || parsed == 0ULL) {
    throw std::invalid_argument("--points requires a positive integer");
  }
  if (parsed > static_cast<unsigned long long>(
                   std::numeric_limits<std::uint64_t>::max())) {
    throw std::out_of_range("--points does not fit uint64");
  }
  return static_cast<std::uint64_t>(parsed);
}

[[nodiscard]] Configuration parse_arguments(
    const int argument_count, char** arguments) {
  Configuration configuration;
  for (int index = 1; index < argument_count; ++index) {
    const std::string_view argument{arguments[index]};
    if (argument == "--points") {
      if (index + 1 >= argument_count) {
        throw std::invalid_argument("--points requires a value");
      }
      configuration.point_count = parse_positive_count(arguments[++index]);
      continue;
    }
    if (argument == "--help") {
      std::cout << "usage: morsehgp3d_point_hierarchy_projection_benchmark "
                   "[--points N]\n";
      std::exit(0);
    }
    throw std::invalid_argument("unknown argument: " + std::string{argument});
  }
  return configuration;
}

[[nodiscard]] CertifiedTowerInput make_balanced_k1_fixture(
    const std::uint64_t point_count) {
  constexpr std::uint64_t maximum_points_for_fixture_ids =
      (std::numeric_limits<std::uint64_t>::max() - 1U) / 3U;
  if (point_count > static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max() / 2U) ||
      point_count > maximum_points_for_fixture_ids) {
    throw std::length_error("the fixture cannot be represented in memory");
  }
  const std::size_t count = static_cast<std::size_t>(point_count);
  CertifiedTowerInput input;
  input.point_count = point_count;
  input.maximum_order = 1U;
  input.nodes.reserve(count * 2U - 1U);
  input.edges.reserve(count == 0U ? 0U : count * 2U - 2U);
  input.projectable_simplices.reserve(count);
  input.receipts.all_orders_forest_receipt_id = fixture_receipt('7');
  input.receipts.vertical_maps_receipt_id = fixture_receipt('8');
  input.receipts.projectable_incidences_receipt_id = fixture_receipt('9');
  input.receipts.all_orders_complete = true;
  input.receipts.vertical_maps_complete = true;
  input.receipts.projectable_incidences_complete = true;
  input.receipts.exact_source_certified = true;
  input.receipts.surrogate_source_used = false;

  std::vector<SourceNodeId> layer;
  layer.reserve(count);
  SourceNodeId next_source_node_id = 1U;
  std::uint64_t next_simplex_id = point_count * 2U + 1U;
  for (std::uint64_t point = 0U; point < point_count; ++point) {
    const SourceNodeId leaf = next_source_node_id++;
    layer.push_back(leaf);
    input.nodes.push_back(CertifiedTowerNode{leaf, 1U, level(1U)});
    input.projectable_simplices.push_back(CertifiedProjectableSimplex{
        next_simplex_id++,
        1U,
        leaf,
        {point},
        {level(1U)},
        DensityLevel{1U, level(1U)}});
  }

  std::uint64_t depth = 0U;
  while (layer.size() > 1U) {
    if (depth > 4'000'000'000ULL) {
      throw std::overflow_error("the synthetic hierarchy depth is invalid");
    }
    const std::uint64_t radius = depth + 2U;
    if (radius > std::numeric_limits<std::uint64_t>::max() / radius) {
      throw std::overflow_error("the synthetic merge radius overflows");
    }
    const ExactLevel merge_level = level(radius * radius);
    std::vector<SourceNodeId> next_layer;
    next_layer.reserve((layer.size() + 1U) / 2U);
    for (std::size_t begin = 0U; begin < layer.size(); begin += 2U) {
      if (begin + 1U == layer.size()) {
        next_layer.push_back(layer[begin]);
        continue;
      }
      const SourceNodeId parent = next_source_node_id++;
      input.nodes.push_back(CertifiedTowerNode{parent, 1U, merge_level});
      input.edges.push_back(CertifiedTowerEdge{
          parent,
          layer[begin],
          TowerEdgeKind::horizontal_forest,
          DensityLevel{1U, merge_level}});
      input.edges.push_back(CertifiedTowerEdge{
          parent,
          layer[begin + 1U],
          TowerEdgeKind::horizontal_forest,
          DensityLevel{1U, merge_level}});
      next_layer.push_back(parent);
    }
    layer = std::move(next_layer);
    ++depth;
  }
  return input;
}

[[nodiscard]] std::uint64_t elapsed_nanoseconds(const Clock::time_point start) {
  const auto elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start)
          .count();
  if (elapsed < 0) {
    throw std::runtime_error("the steady clock moved backwards");
  }
  return static_cast<std::uint64_t>(elapsed);
}

[[nodiscard]] std::uint64_t maximum_resident_set_kib() {
#if defined(__linux__)
  std::ifstream status{"/proc/self/status"};
  std::string line_text;
  while (std::getline(status, line_text)) {
    if (!line_text.starts_with("VmHWM:")) {
      continue;
    }
    std::istringstream fields{line_text.substr(6U)};
    std::uint64_t value = 0U;
    std::string unit;
    if (fields >> value >> unit && unit == "kB") {
      return value;
    }
    break;
  }
#endif
#if defined(__unix__) || defined(__APPLE__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) == 0 && usage.ru_maxrss > 0) {
#if defined(__APPLE__)
    return static_cast<std::uint64_t>(usage.ru_maxrss) / 1024U;
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#endif
  }
#endif
  return 0U;
}

}  // namespace

int main(const int argument_count, char** arguments) {
  try {
    const Configuration configuration =
        parse_arguments(argument_count, arguments);

    const Clock::time_point generation_start = Clock::now();
    CertifiedTowerInput input =
        make_balanced_k1_fixture(configuration.point_count);
    const std::uint64_t generation_ns = elapsed_nanoseconds(generation_start);

    const Clock::time_point seal_start = Clock::now();
    input.receipts.tower_payload_id =
        morsehgp3d::api::compute_tower_payload_id(input);
    const std::uint64_t payload_seal_ns = elapsed_nanoseconds(seal_start);

    PointHierarchyOptions options;
    options.exp_z = {2U, 1U};
    options.simplex_point_weighting = SimplexPointWeighting::uniform;
    options.budget.maximum_points = configuration.point_count;
    options.budget.maximum_source_nodes =
        static_cast<std::uint64_t>(input.nodes.size());
    options.budget.maximum_source_edges =
        static_cast<std::uint64_t>(input.edges.size());
    options.budget.maximum_projectable_simplices =
        static_cast<std::uint64_t>(input.projectable_simplices.size());
    options.budget.maximum_point_simplex_incidences =
        configuration.point_count;
    options.budget.maximum_coface_radius_atoms = configuration.point_count;
    options.budget.maximum_hierarchy_nodes =
        static_cast<std::uint64_t>(input.nodes.size() + input.edges.size()) + 1U;

    const Clock::time_point build_start = Clock::now();
    const auto hierarchy = morsehgp3d::api::build_exact_point_hierarchy(
        input, options);
    const std::uint64_t hierarchy_build_ns = elapsed_nanoseconds(build_start);

    const std::uint64_t minimum_cluster_size =
        std::min<std::uint64_t>(20U, configuration.point_count);
    const Clock::time_point dbscan_start = Clock::now();
    const auto dbscan = hierarchy.select_dbscan_radius(
        level(36U), 1U, minimum_cluster_size);
    const std::uint64_t dbscan_ns = elapsed_nanoseconds(dbscan_start);

    const Clock::time_point eom_start = Clock::now();
    const auto eom =
        hierarchy.select_excess_of_mass(minimum_cluster_size);
    const std::uint64_t eom_ns = elapsed_nanoseconds(eom_start);

    const Clock::time_point invariant_start = Clock::now();
    const bool invariants_valid = hierarchy.validate_partition_invariants();
    const std::uint64_t invariant_replay_ns =
        elapsed_nanoseconds(invariant_start);
    if (!invariants_valid || !dbscan.clusters_are_pairwise_disjoint ||
        !eom.clusters_are_pairwise_disjoint) {
      throw std::logic_error("the benchmark result violates a point partition");
    }

    std::cout << "{\n"
              << "  \"schema\": \"morsehgp3d.point_hierarchy.projection_benchmark.v1\",\n"
              << "  \"scope\": \"synthetic_k1_projection_only_not_product_qualification\",\n"
              << "  \"backend\": \"reference_cpu\",\n"
              << "  \"profile\": \"balanced_k1_exact_relative\",\n"
              << "  \"mode\": \"resident\",\n"
              << "  \"public_status\": \"not_claimed\",\n"
              << "  \"point_count\": " << configuration.point_count << ",\n"
              << "  \"maximum_order\": 1,\n"
              << "  \"source_node_count\": " << input.nodes.size() << ",\n"
              << "  \"source_edge_count\": " << input.edges.size() << ",\n"
              << "  \"projectable_simplex_count\": "
              << input.projectable_simplices.size() << ",\n"
              << "  \"point_simplex_incidence_count\": "
              << configuration.point_count << ",\n"
              << "  \"output_node_count\": " << hierarchy.nodes().size()
              << ",\n"
              << "  \"input_generation_ns\": " << generation_ns << ",\n"
              << "  \"payload_seal_ns\": " << payload_seal_ns << ",\n"
              << "  \"hierarchy_build_ns\": " << hierarchy_build_ns << ",\n"
              << "  \"dbscan_render_ns\": " << dbscan_ns << ",\n"
              << "  \"eom_render_ns\": " << eom_ns << ",\n"
              << "  \"invariant_replay_ns\": " << invariant_replay_ns << ",\n"
              << "  \"maximum_resident_set_kib\": "
              << maximum_resident_set_kib() << ",\n"
              << "  \"minimum_cluster_size\": " << minimum_cluster_size
              << ",\n"
              << "  \"dbscan_cluster_count\": "
              << dbscan.selected_node_ids.size() << ",\n"
              << "  \"dbscan_noise_point_count\": "
              << dbscan.noise_point_count << ",\n"
              << "  \"eom_cluster_count\": "
              << eom.selected_node_ids.size() << ",\n"
              << "  \"eom_noise_point_count\": " << eom.noise_point_count
              << ",\n"
              << "  \"partition_invariants_valid\": true,\n"
              << "  \"tower_payload_id\": \""
              << input.receipts.tower_payload_id.to_lower_hex() << "\",\n"
              << "  \"hierarchy_reduction_receipt_id\": \""
              << hierarchy.receipt().reduction_receipt_id.to_lower_hex()
              << "\",\n"
              << "  \"dbscan_selection_receipt_id\": \""
              << dbscan.selection_receipt_id.to_lower_hex() << "\",\n"
              << "  \"eom_selection_receipt_id\": \""
              << eom.selection_receipt_id.to_lower_hex() << "\",\n"
              << "  \"source_payload_binding_verified\": "
              << (hierarchy.receipt().source_payload_binding_verified
                      ? "true"
                      : "false")
              << ",\n"
              << "  \"surrogate_source_used\": "
              << (hierarchy.receipt().surrogate_source_used ? "true" : "false")
              << ",\n"
              << "  \"upstream_source_authority_replayed\": "
              << (hierarchy.receipt().upstream_source_authority_replayed
                      ? "true"
                      : "false")
              << ",\n"
              << "  \"exact_reduction_of_bound_payload\": "
              << (hierarchy.receipt().exact_reduction_of_bound_payload
                      ? "true"
                      : "false")
              << ",\n"
              << "  \"public_exact_status_claimed\": "
              << (hierarchy.receipt().public_exact_status_claimed
                      ? "true"
                      : "false")
              << ",\n"
              << "  \"gcp_used\": false\n"
              << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "point-hierarchy projection benchmark failed: "
              << error.what() << '\n';
    return 1;
  }
}
