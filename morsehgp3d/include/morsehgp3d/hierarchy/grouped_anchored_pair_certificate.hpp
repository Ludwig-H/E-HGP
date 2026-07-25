#pragma once

#include "morsehgp3d/spatial/aabb.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string_view>

namespace morsehgp3d::spatial {
class MortonLbvhIndex;
}

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_anchor_count = 32U;
inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_witness_pool_size = 64U;
inline constexpr std::size_t
    exact_grouped_anchored_pair_maximum_closed_rank = 11U;
static_assert(
    exact_grouped_anchored_pair_maximum_witness_pool_size <=
    std::numeric_limits<std::uint64_t>::digits);
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_backend = "reference_cpu";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_mode =
        "grouped_exact_certificate";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_public_status = "not_claimed";
inline constexpr std::string_view
    exact_grouped_anchored_pair_certificate_proof_basis =
        "bounded_group_witness_pool_and_exact_anchor_aabb_query_aabb_"
        "strict_diametral_phi_maximum_v1";

enum class ExactGroupedAnchoredPairPruneDecision : std::uint8_t {
  certified,
  inconclusive,
  budget_exhausted,
};

enum class ExactGroupedAnchoredPairPruneStopReason : std::uint8_t {
  none,
  anchor_count_limit,
  witness_pool_entry_limit,
  exact_predicate_limit,
};

// The built-in G <= 32 and W <= 64 limits are structural.  These caller caps
// may be smaller and make an otherwise valid request fail open without
// publishing partial witness authority.
struct ExactGroupedAnchoredPairPruneBudget {
  std::size_t maximum_anchor_count{};
  std::size_t maximum_witness_pool_entry_count{};
  std::size_t maximum_exact_predicate_count{};

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneBudget&,
      const ExactGroupedAnchoredPairPruneBudget&) = default;
};

struct ExactGroupedAnchoredPairPruneAudit {
  std::size_t anchor_count{};
  std::size_t witness_pool_entry_count{};
  std::size_t exact_predicate_count{};
  std::size_t strict_group_witness_count{};
  bool input_canonical{false};
  bool anchor_bounds_constructed{false};
  bool lbvh_node_authority_verified{false};
  bool complete{false};

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneAudit&,
      const ExactGroupedAnchoredPairPruneAudit&) = default;
};

// A certified result proves one shared prune for exactly the bound anchor
// group and immutable LBVH node.  certified() reports the local decision;
// certifies() must be used before replay so process-local cloud/LBVH identity,
// node range and anchor provenance are checked together.  An inconclusive or
// exhausted result carries no witness authority.
class ExactGroupedAnchoredPairPruneCertificate {
 public:
  static constexpr std::string_view backend =
      exact_grouped_anchored_pair_certificate_backend;
  static constexpr std::string_view profile =
      exact_grouped_anchored_pair_certificate_profile;
  static constexpr std::string_view mode =
      exact_grouped_anchored_pair_certificate_mode;
  static constexpr std::string_view deployment_status =
      exact_grouped_anchored_pair_certificate_deployment_status;
  static constexpr std::string_view public_status =
      exact_grouped_anchored_pair_certificate_public_status;
  static constexpr std::string_view proof_basis =
      exact_grouped_anchored_pair_certificate_proof_basis;

  ExactGroupedAnchoredPairPruneCertificate(
      const ExactGroupedAnchoredPairPruneCertificate&) = default;
  ExactGroupedAnchoredPairPruneCertificate(
      ExactGroupedAnchoredPairPruneCertificate&&) noexcept = default;
  ExactGroupedAnchoredPairPruneCertificate& operator=(
      const ExactGroupedAnchoredPairPruneCertificate&) = default;
  ExactGroupedAnchoredPairPruneCertificate& operator=(
      ExactGroupedAnchoredPairPruneCertificate&&) noexcept = default;
  ~ExactGroupedAnchoredPairPruneCertificate() = default;

  [[nodiscard]] ExactGroupedAnchoredPairPruneDecision decision() const
      noexcept {
    return decision_;
  }

  [[nodiscard]] ExactGroupedAnchoredPairPruneStopReason stop_reason() const
      noexcept {
    return stop_reason_;
  }

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] std::size_t required_witness_count() const noexcept {
    return required_witness_count_;
  }

  [[nodiscard]] const ExactGroupedAnchoredPairPruneBudget& requested_budget()
      const & noexcept {
    return requested_budget_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneBudget& requested_budget()
      const && = delete;

  [[nodiscard]] const ExactGroupedAnchoredPairPruneAudit& audit() const
      & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneAudit& audit() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const
      & noexcept {
    return {anchor_point_ids_.data(), anchor_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> anchor_point_ids() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const & noexcept {
    return {witness_pool_point_ids_.data(), witness_pool_entry_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId> witness_pool_point_ids()
      const && = delete;

  [[nodiscard]] const spatial::ExactDyadicAabb3& anchor_bounds() const
      & noexcept {
    return anchor_bounds_;
  }
  [[nodiscard]] const spatial::ExactDyadicAabb3& anchor_bounds() const && =
      delete;

  [[nodiscard]] std::size_t lbvh_node_index() const noexcept {
    return lbvh_node_index_;
  }

  [[nodiscard]] std::size_t leaf_begin() const noexcept {
    return leaf_begin_;
  }

  [[nodiscard]] std::size_t leaf_end() const noexcept {
    return leaf_end_;
  }

  [[nodiscard]] const spatial::ExactDyadicAabb3& query_bounds() const
      & noexcept {
    return query_bounds_;
  }
  [[nodiscard]] const spatial::ExactDyadicAabb3& query_bounds() const && =
      delete;

  [[nodiscard]] std::span<const spatial::PointId>
  certified_witness_point_ids() const & noexcept {
    return {certified_witness_point_ids_.data(), certified_witness_count_};
  }
  [[nodiscard]] std::span<const spatial::PointId>
  certified_witness_point_ids() const && = delete;

  [[nodiscard]] std::uint64_t certified_witness_pool_mask() const noexcept {
    return certified_witness_pool_mask_;
  }

  [[nodiscard]] bool certified() const noexcept {
    return decision_ == ExactGroupedAnchoredPairPruneDecision::certified;
  }

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

  [[nodiscard]] bool certifies(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t expected_lbvh_node_index,
      std::size_t expected_maximum_closed_rank,
      std::span<const spatial::PointId> expected_anchor_point_ids) const
      noexcept;

  friend bool operator==(
      const ExactGroupedAnchoredPairPruneCertificate&,
      const ExactGroupedAnchoredPairPruneCertificate&) = default;

 private:
  ExactGroupedAnchoredPairPruneCertificate() = default;

  ExactGroupedAnchoredPairPruneDecision decision_{
      ExactGroupedAnchoredPairPruneDecision::inconclusive};
  ExactGroupedAnchoredPairPruneStopReason stop_reason_{
      ExactGroupedAnchoredPairPruneStopReason::none};
  std::size_t maximum_closed_rank_{};
  std::size_t required_witness_count_{};
  ExactGroupedAnchoredPairPruneBudget requested_budget_{};
  ExactGroupedAnchoredPairPruneAudit audit_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_anchor_count>
      anchor_point_ids_{};
  std::size_t anchor_count_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_witness_pool_size>
      witness_pool_point_ids_{};
  std::size_t witness_pool_entry_count_{};
  spatial::ExactDyadicAabb3 anchor_bounds_{};
  std::size_t lbvh_node_index_{};
  std::size_t leaf_begin_{};
  std::size_t leaf_end_{};
  spatial::ExactDyadicAabb3 query_bounds_{};
  std::array<spatial::PointId,
             exact_grouped_anchored_pair_maximum_closed_rank - 1U>
      certified_witness_point_ids_{};
  std::size_t certified_witness_count_{};
  std::uint64_t certified_witness_pool_mask_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;

  friend class ExactGroupedAnchoredPairPruneCertifier;
};

// The anchor ids and witness pool must each be strictly PointId-increasing.
// Every pool point must differ from every anchor.  The pool is only a bounded
// proposal and needs no recall guarantee or per-anchor bank membership.
//
// For anchor box A, the certified LBVH node box Q and a pool witness x, the
// exact predicate is
//
//   max_{a in A, q in Q} (x-a).(x-q) < 0.
//
// If maximum_closed_rank - 1 distinct witnesses pass, every actual pair has
// more than maximum_closed_rank closed-ball points.  No cloud-sized witness
// table, pair catalogue, cell, coface or higher-order Delaunay mosaic is
// built.  The existing exact dyadic predicate may use bounded transient BigInt
// fallback, but this primitive owns no dynamic arena proportional to n.
[[nodiscard]] ExactGroupedAnchoredPairPruneCertificate
certify_exact_grouped_anchored_pair_prune(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> anchor_point_ids,
    std::span<const spatial::PointId> witness_pool_point_ids,
    std::size_t lbvh_node_index,
    std::size_t maximum_closed_rank,
    ExactGroupedAnchoredPairPruneBudget budget);

}  // namespace morsehgp3d::hierarchy
