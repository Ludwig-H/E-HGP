#pragma once

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

enum class ExactFacetMiniballStatus : std::uint8_t {
  not_certified,
  exact_facet_miniball_certified,
};

enum class ExactFacetMiniballScope : std::uint8_t {
  unspecified,
  local_facet_miniball_only,
};

struct ExactFacetMiniballCounters {
  std::size_t facet_point_count{};
  std::array<std::size_t, 4> enumerated_support_count_by_size{};
  std::size_t enumerated_support_count{};
  std::size_t affinely_dependent_support_count{};
  std::size_t boundary_reduced_support_count{};
  std::size_t exterior_circumcenter_support_count{};
  std::size_t minimal_support_candidate_count{};
  std::size_t candidate_point_classification_count{};
  std::size_t candidate_strictly_inside_classification_count{};
  std::size_t candidate_boundary_classification_count{};
  std::size_t candidate_outside_classification_count{};
  std::size_t enclosing_support_count{};
  std::size_t optimal_support_count{};
  std::size_t selected_support_size{};

  friend bool operator==(
      const ExactFacetMiniballCounters&,
      const ExactFacetMiniballCounters&) = default;
};

// This result certifies one bounded local facet only. The enclosing ball is
// unique, but its exact support need not be. Among all positive supports of
// minimum radius, the adapter chooses minimum cardinality and then the
// lexicographically smallest PointId vector. That deterministic choice is not
// a claim that the support is the unique essential support of a later descent.
struct ExactFacetMiniballResult {
  static constexpr std::size_t maximum_facet_point_count = 10U;
  static constexpr std::size_t maximum_support_point_count = 4U;
  static constexpr std::size_t maximum_enumerated_support_count = 385U;
  static constexpr const char* proof_basis =
      "exhaustive_exact_supports_up_to_four_facet_miniball_v1";

  std::vector<spatial::PointId> facet_point_ids;
  std::vector<spatial::PointId> support_point_ids;
  std::vector<spatial::PointId> strictly_inside_point_ids;
  std::vector<spatial::PointId> boundary_point_ids;
  exact::ExactCenter3 center;
  exact::ExactLevel squared_radius;
  ExactFacetMiniballCounters counters{};
  ExactFacetMiniballStatus status{
      ExactFacetMiniballStatus::not_certified};
  ExactFacetMiniballScope scope{ExactFacetMiniballScope::unspecified};

  friend bool operator==(
      const ExactFacetMiniballResult&,
      const ExactFacetMiniballResult&) = default;
};

struct ExactFacetMiniballVerification {
  bool facet_identity_certified{false};
  bool exhaustive_support_enumeration_certified{false};
  bool exact_center_and_radius_certified{false};
  bool enclosing_partition_certified{false};
  bool canonical_support_certified{false};
  bool counters_certified{false};
  bool status_certified{false};
  bool local_scope_certified{false};
  bool fresh_replay_certified{false};
  bool local_exact_facet_miniball_certified{false};
};

// Enumerates every subset of one to four facet points. For a ten-point facet,
// this is exactly 385 supports. Every relatively interior circumcenter is
// classified against every facet point with exact rationals before the least
// enclosing radius and canonical support are selected.
[[nodiscard]] ExactFacetMiniballResult build_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids);

// Repeats the complete bounded enumeration without trusting any result field.
// It is a fresh execution of the same proved exhaustive algorithm, not an
// independent software oracle and not a full_pi0 public-status certificate.
[[nodiscard]] ExactFacetMiniballVerification verify_exact_facet_miniball(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> facet_point_ids,
    const ExactFacetMiniballResult& result);

}  // namespace morsehgp3d::hierarchy
