#pragma once

#include "morsehgp3d/hierarchy/grouped_anchored_pair_certificate.hpp"

#include <cstddef>
#include <string_view>

namespace morsehgp3d::hierarchy {

struct ExactSymmetricGroupedAnchoredPairPruneReceiptAudit {
  std::size_t anchor_count{};
  std::size_t query_count{};
  bool source_certificate_recertified{false};
  bool anchor_morton_range_authenticated{false};
  bool query_morton_range_authenticated{false};
  bool morton_ranges_disjoint{false};
  bool unordered_mass_overflow_checked{false};
  bool directed_mass_overflow_checked{false};
  bool process_local_source_authority_retained{false};

  friend bool operator==(
      const ExactSymmetricGroupedAnchoredPairPruneReceiptAudit&,
      const ExactSymmetricGroupedAnchoredPairPruneReceiptAudit&) = default;
};

class ExactSymmetricGroupedAnchoredPairPruneReceipt;

// Replays the existing P8g authority against the caller's expected node and
// rank.  The anchor ids are reconstructed from the authentic Morton interval,
// rather than accepted from the caller.  A and Q must be nonempty disjoint
// half-open leaf ranges.  Failure, including either mass overflow, throws
// before a receipt exists.
[[nodiscard]] ExactSymmetricGroupedAnchoredPairPruneReceipt
recertify_exact_symmetric_grouped_anchored_pair_prune(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactGroupedAnchoredPairPruneCertificate& source_certificate,
    std::size_t expected_anchor_leaf_begin,
    std::size_t expected_anchor_leaf_end,
    std::size_t expected_query_lbvh_node_index,
    std::size_t expected_maximum_closed_rank);

// A move-only process-local receipt.  Its directed authority follows from the
// symmetry of (x-p).(x-q), after the source P8g certificate has proved every
// pair in A x Q and the authentic Morton ranges have proved A and Q disjoint.
// It has deliberately no default constructor or serialization surface.
class ExactSymmetricGroupedAnchoredPairPruneReceipt {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "symmetric_grouped_anchored_pair_prune_receipt";
  static constexpr std::string_view deployment_status = "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view authority_lifetime = "process_local";
  static constexpr std::string_view proof_basis =
      "recertified_P8g_symmetric_diametral_predicate_disjoint_authentic_"
      "morton_ranges_v1";

  ExactSymmetricGroupedAnchoredPairPruneReceipt(
      const ExactSymmetricGroupedAnchoredPairPruneReceipt&) = delete;
  ExactSymmetricGroupedAnchoredPairPruneReceipt& operator=(
      const ExactSymmetricGroupedAnchoredPairPruneReceipt&) = delete;
  ExactSymmetricGroupedAnchoredPairPruneReceipt(
      ExactSymmetricGroupedAnchoredPairPruneReceipt&&) noexcept = default;
  ExactSymmetricGroupedAnchoredPairPruneReceipt& operator=(
      ExactSymmetricGroupedAnchoredPairPruneReceipt&&) noexcept = default;
  ~ExactSymmetricGroupedAnchoredPairPruneReceipt() = default;

  [[nodiscard]] std::size_t anchor_leaf_begin() const noexcept {
    return anchor_leaf_begin_;
  }

  [[nodiscard]] std::size_t anchor_leaf_end() const noexcept {
    return anchor_leaf_end_;
  }

  [[nodiscard]] std::size_t query_leaf_begin() const noexcept {
    return query_leaf_begin_;
  }

  [[nodiscard]] std::size_t query_leaf_end() const noexcept {
    return query_leaf_end_;
  }

  [[nodiscard]] std::size_t query_lbvh_node_index() const noexcept {
    return query_lbvh_node_index_;
  }

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }

  [[nodiscard]] std::size_t unordered_mass() const noexcept {
    return unordered_mass_;
  }

  [[nodiscard]] std::size_t directed_mass() const noexcept {
    return directed_mass_;
  }

  [[nodiscard]] const ExactSymmetricGroupedAnchoredPairPruneReceiptAudit&
  audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactSymmetricGroupedAnchoredPairPruneReceiptAudit&
  audit() const && = delete;

  [[nodiscard]] const ExactGroupedAnchoredPairPruneCertificate&
  source_certificate() const & noexcept {
    return source_certificate_;
  }
  [[nodiscard]] const ExactGroupedAnchoredPairPruneCertificate&
  source_certificate() const && = delete;

  // Replays both the process-local certificate identity and the two Morton
  // ranges.  A receipt cannot be rebound to a rebuilt cloud or LBVH.
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;

 private:
  ExactSymmetricGroupedAnchoredPairPruneReceipt(
      const ExactGroupedAnchoredPairPruneCertificate& source_certificate,
      std::size_t anchor_leaf_begin,
      std::size_t anchor_leaf_end,
      std::size_t query_leaf_begin,
      std::size_t query_leaf_end,
      std::size_t query_lbvh_node_index,
      std::size_t maximum_closed_rank,
      std::size_t unordered_mass,
      std::size_t directed_mass,
      ExactSymmetricGroupedAnchoredPairPruneReceiptAudit audit);

  ExactGroupedAnchoredPairPruneCertificate source_certificate_;
  std::size_t anchor_leaf_begin_{};
  std::size_t anchor_leaf_end_{};
  std::size_t query_leaf_begin_{};
  std::size_t query_leaf_end_{};
  std::size_t query_lbvh_node_index_{};
  std::size_t maximum_closed_rank_{};
  std::size_t unordered_mass_{};
  std::size_t directed_mass_{};
  ExactSymmetricGroupedAnchoredPairPruneReceiptAudit audit_{};

  friend ExactSymmetricGroupedAnchoredPairPruneReceipt
  recertify_exact_symmetric_grouped_anchored_pair_prune(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const ExactGroupedAnchoredPairPruneCertificate& source_certificate,
      std::size_t expected_anchor_leaf_begin,
      std::size_t expected_anchor_leaf_end,
      std::size_t expected_query_lbvh_node_index,
      std::size_t expected_maximum_closed_rank);
};

}  // namespace morsehgp3d::hierarchy
