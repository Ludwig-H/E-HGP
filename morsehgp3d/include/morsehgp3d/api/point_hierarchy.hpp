#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace morsehgp3d::api {

using PointId = std::uint64_t;
using SourceNodeId = std::uint64_t;
using SourceSimplexId = std::uint64_t;
using PointHierarchyNodeId = std::uint64_t;

// The public exact API accepts a rational exponent instead of a binary64
// approximation.  The value is normalized by the builder before use.
struct PositiveRationalExponent {
  std::uint32_t numerator{3U};
  std::uint32_t denominator{1U};

  [[nodiscard]] static PositiveRationalExponent ambient_dimension_3() noexcept {
    return PositiveRationalExponent{3U, 1U};
  }
};

struct DensityLevel {
  std::uint32_t order{0U};
  // A zero squared radius denotes the single exact symbolic top-density batch
  // Lambda_infinity.  It is never replaced by a finite radius or epsilon.
  exact::ExactLevel squared_radius{};

  friend bool operator==(const DensityLevel&, const DensityLevel&) = default;
};

enum class TowerEdgeKind : std::uint8_t {
  horizontal_forest = 0U,
  adjacent_order_vertical = 1U,
};

struct CertifiedTowerNode {
  SourceNodeId source_node_id{0U};
  std::uint32_t order{0U};
  exact::ExactLevel squared_radius{};
};

struct CertifiedTowerEdge {
  SourceNodeId first_source_node_id{0U};
  SourceNodeId second_source_node_id{0U};
  TowerEdgeKind kind{TowerEdgeKind::horizontal_forest};
  // Exact filtration level at which this incidence becomes active.  For a
  // vertical edge its order is the lower of the two adjacent source orders.
  DensityLevel activation_level{};
};

// One projectable (order - 1)-simplex.  Its coface radii are the atomic
// inverse-radius terms used by the Chapter-9-style simplex-to-point transfer.
// point_ids must be strictly increasing and contain exactly `order` entries.
struct CertifiedProjectableSimplex {
  SourceSimplexId source_simplex_id{0U};
  std::uint32_t order{0U};
  SourceNodeId carrier_source_node_id{0U};
  std::vector<PointId> point_ids;
  std::vector<exact::ExactLevel> incident_coface_squared_radii;
  // V1 requires this density to equal the carrier activation density.  At
  // squared radius zero this is the common symbolic Lambda_infinity batch;
  // EOM cancels equal zero-level mass exactly before comparing finite terms.
  // A distinct extended-lifetime value would require an explicit
  // pseudo-terminal and is rejected rather than ignored by flat cuts.
  DensityLevel terminal_exit_level{};
};

// These receipts are produced by the upstream certified forest, vertical-map
// and incidence authorities.  An all-zero identifier is rejected.  This
// component verifies their presence and the structural invariants it can
// replay; it does not manufacture an upstream exactness claim.
struct TowerSourceReceipts {
  contract::CanonicalId all_orders_forest_receipt_id{};
  contract::CanonicalId vertical_maps_receipt_id{};
  contract::CanonicalId projectable_incidences_receipt_id{};
  // Content hash of the complete canonical tower payload.  This binds the
  // three upstream authority identifiers to the nodes, edges and incidences
  // consumed by this reduction.
  contract::CanonicalId tower_payload_id{};
  bool all_orders_complete{false};
  bool vertical_maps_complete{false};
  bool projectable_incidences_complete{false};
  bool exact_source_certified{false};
  bool surrogate_source_used{true};
};

struct CertifiedTowerInput {
  std::uint64_t point_count{0U};
  std::uint32_t maximum_order{0U};
  std::vector<CertifiedTowerNode> nodes;
  std::vector<CertifiedTowerEdge> edges;
  std::vector<CertifiedProjectableSimplex> projectable_simplices;
  TowerSourceReceipts receipts{};
};

enum class SimplexPointWeighting : std::uint8_t {
  inverse_radius = 0U,
  uniform = 1U,
};

struct OrderWeight {
  std::uint32_t order{0U};
  exact::ExactRational weight{exact::BigInt{1}};
};

struct PointHierarchyBudget {
  std::uint64_t maximum_points{2'000'000U};
  std::uint64_t maximum_source_nodes{2'000'000U};
  std::uint64_t maximum_source_edges{4'000'000U};
  std::uint64_t maximum_projectable_simplices{8'000'000U};
  std::uint64_t maximum_point_simplex_incidences{64'000'000U};
  std::uint64_t maximum_coface_radius_atoms{64'000'000U};
  std::uint64_t maximum_hierarchy_nodes{6'000'001U};
  std::uint32_t maximum_exact_integer_bits{4096U};
  std::uint32_t initial_certification_bits{32U};
  std::uint32_t maximum_certification_bits{512U};

  // Explicit opt-in envelope for resident experiments through 30 million
  // points.  It is a fail-closed allocation cap, not a scale qualification or
  // a promise that an arbitrary multi-order source fits memory.
  [[nodiscard]] static PointHierarchyBudget large_resident_30m() noexcept;
};

struct PointHierarchyOptions {
  PositiveRationalExponent exp_z{PositiveRationalExponent::ambient_dimension_3()};
  SimplexPointWeighting simplex_point_weighting{
      SimplexPointWeighting::inverse_radius};
  std::vector<OrderWeight> order_weights;
  PointHierarchyBudget budget{};
};

class ExactCertificationError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

struct PointHierarchyNode {
  PointHierarchyNodeId node_id{0U};
  std::optional<PointHierarchyNodeId> parent_node_id;
  std::vector<PointHierarchyNodeId> child_node_ids;
  std::optional<DensityLevel> event_level;
  std::vector<SourceNodeId> direct_source_node_ids;
  std::uint64_t direct_point_count{0U};
  std::uint64_t subtree_point_count{0U};
  std::uint64_t point_dfs_begin{0U};
  std::uint64_t point_dfs_end{0U};
};

enum class FlatSelectionMethod : std::uint8_t {
  lambda_cut = 0U,
  dbscan_radius = 1U,
  excess_of_mass = 2U,
};

struct FlatClustering {
  FlatSelectionMethod method{FlatSelectionMethod::lambda_cut};
  std::vector<std::int64_t> labels;
  std::vector<PointHierarchyNodeId> selected_node_ids;
  std::uint64_t noise_point_count{0U};
  bool clusters_are_pairwise_disjoint{false};
  std::uint64_t minimum_cluster_size{0U};
  std::optional<DensityLevel> lambda_threshold;
  std::optional<exact::ExactLevel> dbscan_epsilon_squared;
  std::uint32_t dbscan_reference_order{0U};
  bool eom_allow_single_cluster{false};
  contract::CanonicalId hierarchy_reduction_receipt_id{};
  contract::CanonicalId selection_receipt_id{};
};

struct ExactPointHierarchyReceipt {
  contract::CanonicalId source_forest_receipt_id{};
  contract::CanonicalId source_vertical_maps_receipt_id{};
  contract::CanonicalId source_projectable_incidences_receipt_id{};
  contract::CanonicalId source_tower_payload_id{};
  contract::CanonicalId reduction_receipt_id{};
  PositiveRationalExponent exp_z{};
  SimplexPointWeighting simplex_point_weighting{
      SimplexPointWeighting::inverse_radius};
  std::vector<OrderWeight> normalized_order_weights;
  std::uint32_t initial_certification_bits{0U};
  std::uint32_t maximum_certification_bits{0U};
  bool exact_reduction_of_bound_payload{false};
  bool upstream_source_authority_replayed{false};
  bool surrogate_source_used{true};
  bool source_payload_binding_verified{false};
  bool public_exact_status_claimed{false};
};

class PointHierarchy {
 public:
  PointHierarchy(const PointHierarchy&) noexcept = default;
  PointHierarchy& operator=(const PointHierarchy&) noexcept = default;
  PointHierarchy(PointHierarchy&& other) noexcept;
  PointHierarchy& operator=(PointHierarchy&& other) noexcept;

  [[nodiscard]] std::uint64_t point_count() const noexcept;
  [[nodiscard]] std::uint32_t maximum_order() const noexcept;
  [[nodiscard]] PointHierarchyNodeId virtual_root_node_id() const noexcept;
  [[nodiscard]] const std::vector<PointHierarchyNode>& nodes() const noexcept;
  [[nodiscard]] const std::vector<PointHierarchyNodeId>&
  terminal_node_by_point() const noexcept;
  [[nodiscard]] const std::vector<PointId>& point_ids_in_dfs_order() const noexcept;
  [[nodiscard]] const ExactPointHierarchyReceipt& receipt() const noexcept;

  // `threshold` represents k / r^exp_z; the common n*omega_3 factor is omitted.
  [[nodiscard]] FlatClustering select_lambda_cut(
      const DensityLevel& threshold,
      std::uint64_t minimum_cluster_size = 1U) const;

  // Exact DBSCAN-style rendering of the hierarchy at epsilon.  The reference
  // order is the K-NN/min_samples density order and defaults to K when zero;
  // minimum_cluster_size remains an independent output-size filter.
  [[nodiscard]] FlatClustering select_dbscan_radius(
      const exact::ExactLevel& epsilon_squared,
      std::uint32_t reference_order = 0U,
      std::uint64_t minimum_cluster_size = 1U) const;

  // HDBSCAN-style condensed-tree EOM selection.  The default matches
  // allow_single_cluster=false: a sole natural root cannot be the final
  // cluster.  Every finite score comparison is certified with rational
  // enclosures.  A zero-radius batch is compared as the exact leading symbol
  // Lambda_infinity; equal symbolic mass cancels, and any unresolved finite
  // remainder fails closed.
  [[nodiscard]] FlatClustering select_excess_of_mass(
      std::uint64_t minimum_cluster_size,
      bool allow_single_cluster = false) const;

  [[nodiscard]] bool validate_partition_invariants() const;

 private:
  struct Implementation;
  explicit PointHierarchy(std::shared_ptr<const Implementation> implementation);

  std::shared_ptr<const Implementation> implementation_;

  friend PointHierarchy build_exact_point_hierarchy(
      const CertifiedTowerInput&, const PointHierarchyOptions&);
};

// Builds the canonical merge tree of the complete certified tower T_1,...,T_K,
// then performs one irreversible top-down point routing.  No Delaunay mosaic,
// higher-order cell catalogue, point MST or geometric surrogate is built.
[[nodiscard]] PointHierarchy build_exact_point_hierarchy(
    const CertifiedTowerInput& input,
    const PointHierarchyOptions& options = {});

// Public exact comparator used by callers that stream already sorted events.
// Returns -1, 0 or +1 for left < right, left == right or left > right.
[[nodiscard]] int compare_density_levels_exact(
    const DensityLevel& left,
    const DensityLevel& right,
    PositiveRationalExponent exp_z);

// Computes the permutation-invariant content identifier expected in
// TowerSourceReceipts::tower_payload_id.  It binds data, not geometric truth;
// the upstream authority receipts remain the trust root for source exactness.
[[nodiscard]] contract::CanonicalId compute_tower_payload_id(
    const CertifiedTowerInput& input);

}  // namespace morsehgp3d::api
