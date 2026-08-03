#include "morsehgp3d/morsehgp3d.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::api::CertifiedProjectableSimplex;
using morsehgp3d::api::CertifiedTowerEdge;
using morsehgp3d::api::CertifiedTowerInput;
using morsehgp3d::api::CertifiedTowerNode;
using morsehgp3d::api::DensityLevel;
using morsehgp3d::api::FlatClustering;
using morsehgp3d::api::OrderWeight;
using morsehgp3d::api::PointHierarchyOptions;
using morsehgp3d::api::SimplexPointWeighting;
using morsehgp3d::api::TowerEdgeKind;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;

constexpr std::size_t point_count = 9U;
constexpr std::array<std::array<int, 2U>, point_count> points{{
    {{0, 0}},
    {{0, 1}},
    {{1, 0}},
    {{1, 1}},
    {{10, 0}},
    {{10, 1}},
    {{11, 0}},
    {{11, 1}},
    {{100, 100}},
}};

[[nodiscard]] ExactLevel level(
    const std::int64_t numerator, const std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] CanonicalId fixture_receipt(const char digit) {
  return CanonicalId::from_lower_hex(std::string(64U, digit));
}

void certify_fixture(CertifiedTowerInput& input) {
  input.receipts.all_orders_forest_receipt_id = fixture_receipt('4');
  input.receipts.vertical_maps_receipt_id = fixture_receipt('5');
  input.receipts.projectable_incidences_receipt_id = fixture_receipt('6');
  input.receipts.all_orders_complete = true;
  input.receipts.vertical_maps_complete = true;
  input.receipts.projectable_incidences_complete = true;
  input.receipts.exact_source_certified = true;
  input.receipts.surrogate_source_used = false;
  input.receipts.tower_payload_id =
      morsehgp3d::api::compute_tower_payload_id(input);
}

[[nodiscard]] CertifiedTowerInput make_k2_fixture() {
  CertifiedTowerInput input;
  input.point_count = point_count;
  input.maximum_order = 2U;

  for (std::uint64_t point = 0U; point < point_count; ++point) {
    input.nodes.push_back(CertifiedTowerNode{100U + point, 1U, level(1, 4)});
    input.projectable_simplices.push_back(CertifiedProjectableSimplex{
        1'000U + point,
        1U,
        100U + point,
        {point},
        {level(1)},
        DensityLevel{1U, level(1, 4)}});
  }

  // A Hartigan source edge records one absorption of each endpoint carrier.
  // Keep one carrier per dendrogram component instead of reusing point leaves
  // across several later merge levels.
  input.nodes.push_back(CertifiedTowerNode{200U, 1U, level(1)});
  input.nodes.push_back(CertifiedTowerNode{201U, 1U, level(1)});
  input.nodes.push_back(CertifiedTowerNode{202U, 1U, level(81)});
  input.nodes.push_back(CertifiedTowerNode{203U, 1U, level(17'722)});

  const auto horizontal = [&](const std::uint64_t first_source_node,
                              const std::uint64_t second_source_node,
                              const std::int64_t squared_distance) {
    input.edges.push_back(CertifiedTowerEdge{
        first_source_node,
        second_source_node,
        TowerEdgeKind::horizontal_forest,
        DensityLevel{1U, level(squared_distance)}});
  };

  for (std::uint64_t point = 0U; point < 4U; ++point) {
    horizontal(200U, 100U + point, 1);
  }
  for (std::uint64_t point = 4U; point < 8U; ++point) {
    horizontal(201U, 100U + point, 1);
  }
  horizontal(202U, 200U, 81);
  horizontal(202U, 201U, 81);
  horizontal(203U, 202U, 17'722);
  horizontal(203U, 108U, 17'722);

  // A sparse order-two forest contributes genuine multi-order evidence while
  // the order weights keep the point routing on the intended T1 branches.
  input.nodes.push_back(CertifiedTowerNode{300U, 2U, level(1)});
  input.nodes.push_back(CertifiedTowerNode{301U, 2U, level(1)});
  input.nodes.push_back(CertifiedTowerNode{302U, 2U, level(81)});
  input.edges.push_back(CertifiedTowerEdge{
      302U, 300U, TowerEdgeKind::horizontal_forest, {2U, level(81)}});
  input.edges.push_back(CertifiedTowerEdge{
      302U, 301U, TowerEdgeKind::horizontal_forest, {2U, level(81)}});
  input.edges.push_back(CertifiedTowerEdge{
      300U, 200U, TowerEdgeKind::adjacent_order_vertical, {1U, level(1)}});
  input.edges.push_back(CertifiedTowerEdge{
      301U, 201U, TowerEdgeKind::adjacent_order_vertical, {1U, level(1)}});
  input.edges.push_back(CertifiedTowerEdge{
      302U, 202U, TowerEdgeKind::adjacent_order_vertical, {1U, level(81)}});
  input.projectable_simplices.push_back(CertifiedProjectableSimplex{
      2'000U, 2U, 300U, {0U, 1U}, {level(1)}, {2U, level(1)}});
  input.projectable_simplices.push_back(CertifiedProjectableSimplex{
      2'001U, 2U, 300U, {2U, 3U}, {level(1)}, {2U, level(1)}});
  input.projectable_simplices.push_back(CertifiedProjectableSimplex{
      2'002U, 2U, 301U, {4U, 5U}, {level(1)}, {2U, level(1)}});
  input.projectable_simplices.push_back(CertifiedProjectableSimplex{
      2'003U, 2U, 301U, {6U, 7U}, {level(1)}, {2U, level(1)}});

  certify_fixture(input);
  return input;
}

void emit_labels(const char* name, const FlatClustering& clustering) {
  std::cout << name;
  for (const std::int64_t label : clustering.labels) {
    std::cout << ' ' << label;
  }
  std::cout << '\n';
}

}  // namespace

int main() {
  try {
    PointHierarchyOptions options;
    options.exp_z = {2U, 1U};
    options.simplex_point_weighting = SimplexPointWeighting::uniform;
    options.order_weights = {
        OrderWeight{1U, ExactRational{BigInt{4}}},
        OrderWeight{2U, ExactRational{BigInt{1}}}};
    const auto hierarchy = morsehgp3d::api::build_exact_point_hierarchy(
        make_k2_fixture(), options);
    if (!hierarchy.validate_partition_invariants()) {
      throw std::logic_error("the differential fixture is not a point hierarchy");
    }

    const FlatClustering dbscan =
        hierarchy.select_dbscan_radius(level(9, 4), 2U, 2U);
    const FlatClustering eom = hierarchy.select_excess_of_mass(4U);
    if (!dbscan.clusters_are_pairwise_disjoint ||
        !eom.clusters_are_pairwise_disjoint) {
      throw std::logic_error("a differential clustering is not disjoint");
    }

    std::cout << "schema morsehgp3d.point_hierarchy.sklearn.v1\n";
    std::cout << "dbscan_epsilon 1.5\n";
    std::cout << "dbscan_min_samples 2\n";
    std::cout << "hdbscan_min_cluster_size 4\n";
    std::cout << "hdbscan_min_samples 2\n";
    for (std::size_t index = 0U; index < points.size(); ++index) {
      std::cout << "point " << index << ' ' << points[index][0] << ' '
                << points[index][1] << '\n';
    }
    emit_labels("dbscan_labels", dbscan);
    emit_labels("eom_labels", eom);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "point-hierarchy sklearn dump failed: " << error.what() << '\n';
    return 1;
  }
}
