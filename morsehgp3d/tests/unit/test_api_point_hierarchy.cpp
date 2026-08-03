#include "morsehgp3d/morsehgp3d.hpp"

#include <algorithm>
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
using morsehgp3d::api::ExactCertificationError;
using morsehgp3d::api::FlatSelectionMethod;
using morsehgp3d::api::PointHierarchy;
using morsehgp3d::api::PointHierarchyOptions;
using morsehgp3d::api::PositiveRationalExponent;
using morsehgp3d::api::OrderWeight;
using morsehgp3d::api::SimplexPointWeighting;
using morsehgp3d::api::TowerEdgeKind;
using morsehgp3d::api::build_exact_point_hierarchy;
using morsehgp3d::api::compare_density_levels_exact;
using morsehgp3d::api::compute_tower_payload_id;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    function();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator, std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] CanonicalId receipt(char digit) {
  return CanonicalId::from_lower_hex(std::string(64U, digit));
}

void certify(CertifiedTowerInput& input) {
  input.receipts.all_orders_forest_receipt_id = receipt('1');
  input.receipts.vertical_maps_receipt_id = receipt('2');
  input.receipts.projectable_incidences_receipt_id = receipt('3');
  input.receipts.all_orders_complete = true;
  input.receipts.vertical_maps_complete = true;
  input.receipts.projectable_incidences_complete = true;
  input.receipts.exact_source_certified = true;
  input.receipts.surrogate_source_used = false;
  input.receipts.tower_payload_id = compute_tower_payload_id(input);
}

[[nodiscard]] CertifiedTowerInput two_order_tower() {
  CertifiedTowerInput input;
  input.point_count = 5U;
  input.maximum_order = 2U;
  input.nodes = {
      CertifiedTowerNode{10U, 1U, level(1)},
      CertifiedTowerNode{11U, 1U, level(4)},
      CertifiedTowerNode{12U, 1U, level(1)},
      CertifiedTowerNode{20U, 2U, level(2)},
      CertifiedTowerNode{21U, 2U, level(8)}};
  input.edges = {
      CertifiedTowerEdge{
          10U, 11U, TowerEdgeKind::horizontal_forest, {1U, level(4)}},
      CertifiedTowerEdge{
          11U, 12U, TowerEdgeKind::horizontal_forest, {1U, level(100)}},
      CertifiedTowerEdge{
          20U, 21U, TowerEdgeKind::horizontal_forest, {2U, level(8)}},
      CertifiedTowerEdge{
          20U, 10U, TowerEdgeKind::adjacent_order_vertical, {1U, level(2)}},
      CertifiedTowerEdge{
          21U, 11U, TowerEdgeKind::adjacent_order_vertical, {1U, level(8)}}};
  input.projectable_simplices = {
      CertifiedProjectableSimplex{
          100U, 1U, 10U, {0U}, {level(1)}, {1U, level(1)}},
      CertifiedProjectableSimplex{
          101U, 1U, 10U, {1U}, {level(1)}, {1U, level(1)}},
      CertifiedProjectableSimplex{
          102U, 1U, 11U, {2U}, {level(4)}, {1U, level(4)}},
      CertifiedProjectableSimplex{
          103U, 1U, 11U, {3U}, {level(4)}, {1U, level(4)}},
      CertifiedProjectableSimplex{
          104U, 1U, 12U, {4U}, {level(1)}, {1U, level(1)}},
      CertifiedProjectableSimplex{
          200U, 2U, 20U, {0U, 1U}, {level(2)}, {2U, level(2)}},
      CertifiedProjectableSimplex{
          201U, 2U, 21U, {2U, 3U}, {level(8)}, {2U, level(8)}}};
  certify(input);
  return input;
}

[[nodiscard]] PointHierarchyOptions exponent_two_options() {
  PointHierarchyOptions options;
  options.exp_z = PositiveRationalExponent{2U, 1U};
  options.simplex_point_weighting = SimplexPointWeighting::inverse_radius;
  return options;
}

void test_exact_density_comparator() {
  const PositiveRationalExponent exp_two{2U, 1U};
  check(
      compare_density_levels_exact(
          DensityLevel{1U, level(1)}, DensityLevel{2U, level(2)}, exp_two) == 0,
      "k/r^exp_z compares equal across orders without binary64 rounding");
  check(
      compare_density_levels_exact(
          DensityLevel{1U, level(1)},
          DensityLevel{2U, level(8)},
          PositiveRationalExponent{3U, 1U}) > 0,
      "the rational odd exponent comparison uses exact cross-powers");
  check(
      compare_density_levels_exact(
          DensityLevel{1U, level(0)},
          DensityLevel{2U, level(0)},
          PositiveRationalExponent{7U, 3U}) == 0,
      "all zero-radius density levels form one atomic infinite batch");
}

void test_multi_order_topology_partition_and_rendering() {
  const PointHierarchy hierarchy =
      build_exact_point_hierarchy(two_order_tower(), exponent_two_options());
  check(hierarchy.validate_partition_invariants(),
        "the complete tower reduction is a point partition at every node");
  check(hierarchy.point_count() == 5U && hierarchy.maximum_order() == 2U,
        "the public hierarchy preserves point-count and all-orders metadata");
  check(hierarchy.nodes().size() == 7U,
        "the two cross-order batches, late root and virtual root remain multifurcated");
  check(
      hierarchy.receipt().exact_reduction_of_bound_payload &&
          !hierarchy.receipt().upstream_source_authority_replayed &&
          !hierarchy.receipt().surrogate_source_used,
      "the receipt states exact downstream reduction without source promotion");

  const auto child_cut = hierarchy.select_dbscan_radius(level(2), 2U, 2U);
  check(
      child_cut.method == FlatSelectionMethod::dbscan_radius &&
          child_cut.selected_node_ids.size() == 1U &&
          child_cut.labels[0] == 0 && child_cut.labels[1] == 0 &&
          child_cut.labels[2] == -1 && child_cut.labels[3] == -1 &&
          child_cut.labels[4] == -1 &&
          child_cut.noise_point_count == 3U &&
          child_cut.clusters_are_pairwise_disjoint,
      "the exact DBSCAN-radius cut selects the high-density point child once");

  const auto root_cut = hierarchy.select_dbscan_radius(level(8), 2U, 2U);
  check(
      root_cut.selected_node_ids.size() == 1U &&
          root_cut.labels[0] == 0 && root_cut.labels[1] == 0 &&
          root_cut.labels[2] == 0 && root_cut.labels[3] == 0 &&
          root_cut.labels[4] == -1 && root_cut.noise_point_count == 1U,
      "the lower-density radius cut selects the natural root but not evidence-free noise");

  const auto eom = hierarchy.select_excess_of_mass(2U);
  check(
      eom.method == FlatSelectionMethod::excess_of_mass &&
          eom.selected_node_ids.empty() && eom.labels[0] == -1 &&
          eom.labels[1] == -1 && eom.labels[2] == -1 &&
          eom.labels[3] == -1 && eom.labels[4] == -1 &&
          eom.noise_point_count == 5U &&
          eom.clusters_are_pairwise_disjoint,
      "default EOM excludes a sole natural root like HDBSCAN");

  const auto allowed_root = hierarchy.select_excess_of_mass(2U, true);
  check(
      allowed_root.selected_node_ids.size() == 1U &&
          allowed_root.labels[0] == 0 && allowed_root.labels[1] == 0 &&
          allowed_root.labels[2] == 0 && allowed_root.labels[3] == 0 &&
          allowed_root.labels[4] == 0 &&
          allowed_root.noise_point_count == 0U &&
          allowed_root.selection_receipt_id != eom.selection_receipt_id,
      "allow_single_cluster is explicit and bound to the EOM receipt");
}

void test_permutation_invariance_and_parameter_binding() {
  CertifiedTowerInput canonical = two_order_tower();
  CertifiedTowerInput permuted = canonical;
  std::reverse(permuted.nodes.begin(), permuted.nodes.end());
  std::reverse(permuted.edges.begin(), permuted.edges.end());
  std::reverse(
      permuted.projectable_simplices.begin(),
      permuted.projectable_simplices.end());
  const PointHierarchyOptions options = exponent_two_options();
  const PointHierarchy first = build_exact_point_hierarchy(canonical, options);
  const PointHierarchy second = build_exact_point_hierarchy(permuted, options);
  check(
      first.terminal_node_by_point() == second.terminal_node_by_point(),
      "canonical routing is invariant under source record permutations");
  check(
      first.receipt().reduction_receipt_id ==
          second.receipt().reduction_receipt_id,
      "the canonical reduction receipt is invariant under source record permutations");

  PointHierarchyOptions reweighted = options;
  reweighted.order_weights = {
      OrderWeight{1U, morsehgp3d::exact::ExactRational{BigInt{3}}},
      OrderWeight{2U, morsehgp3d::exact::ExactRational{BigInt{1}}}};
  const PointHierarchy changed =
      build_exact_point_hierarchy(canonical, reweighted);
  check(
      changed.receipt().reduction_receipt_id !=
          first.receipt().reduction_receipt_id,
      "the reduction receipt binds every normalized order weight");
}

void test_fail_closed_contract() {
  CertifiedTowerInput surrogate = two_order_tower();
  surrogate.receipts.surrogate_source_used = true;
  check_throws<ExactCertificationError>(
      [&] {
        static_cast<void>(
            build_exact_point_hierarchy(surrogate, exponent_two_options()));
      },
      "the exact public entry point rejects a surrogate source");

  CertifiedTowerInput missing_order = two_order_tower();
  missing_order.nodes.erase(missing_order.nodes.begin() + 2,
                            missing_order.nodes.end());
  missing_order.edges.clear();
  missing_order.projectable_simplices.erase(
      missing_order.projectable_simplices.begin() + 1,
      missing_order.projectable_simplices.end());
  check_throws<ExactCertificationError>(
      [&] {
        static_cast<void>(
            build_exact_point_hierarchy(missing_order, exponent_two_options()));
      },
      "the complete-forest contract rejects an omitted order");

  CertifiedTowerInput cyclic = two_order_tower();
  cyclic.edges.push_back(
      CertifiedTowerEdge{
          10U, 12U, TowerEdgeKind::horizontal_forest, {1U, level(1)}});
  cyclic.receipts.tower_payload_id = compute_tower_payload_id(cyclic);
  check_throws<ExactCertificationError>(
      [&] {
        static_cast<void>(
            build_exact_point_hierarchy(cyclic, exponent_two_options()));
      },
      "a cyclic horizontal source is rejected before reduction");

  CertifiedTowerInput missing_receipt = two_order_tower();
  missing_receipt.receipts.vertical_maps_receipt_id = {};
  check_throws<ExactCertificationError>(
      [&] {
        static_cast<void>(
            build_exact_point_hierarchy(missing_receipt, exponent_two_options()));
      },
      "a missing upstream receipt fails closed");

  CertifiedTowerInput extended_terminal = two_order_tower();
  extended_terminal.projectable_simplices.front().terminal_exit_level =
      DensityLevel{1U, level(1, 4)};
  extended_terminal.receipts.tower_payload_id =
      compute_tower_payload_id(extended_terminal);
  check_throws<ExactCertificationError>(
      [&] {
        static_cast<void>(build_exact_point_hierarchy(
            extended_terminal, exponent_two_options()));
      },
      "V1 rejects an unmaterialized extended terminal lifetime");
}

}  // namespace

int main() {
  test_exact_density_comparator();
  test_multi_order_topology_partition_and_rendering();
  test_permutation_invariance_and_parameter_binding();
  test_fail_closed_contract();
  if (failures != 0) {
    std::cerr << failures << " point-hierarchy test(s) failed\n";
    return 1;
  }
  std::cout << "all exact multi-order point-hierarchy tests passed\n";
  return 0;
}
