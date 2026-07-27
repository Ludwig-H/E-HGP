#pragma once

#include "morsehgp3d/spatial/lbvh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::size_t exact_block_rank_prune_maximum_closed_rank = 11U;
inline constexpr std::size_t exact_block_rank_prune_maximum_witness_node_count =
    exact_block_rank_prune_maximum_closed_rank - 1U;
inline constexpr std::size_t
    exact_block_rank_prune_maximum_proposed_witness_node_count = 64U;

struct ExactBlockRankNodeAuthority {
  std::size_t node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};

  [[nodiscard]] std::size_t leaf_count() const noexcept {
    return leaf_end - leaf_begin;
  }

  friend bool operator==(
      const ExactBlockRankNodeAuthority&,
      const ExactBlockRankNodeAuthority&) = default;
};

enum class ExactBlockRankPruneStatus : std::uint8_t {
  certified_above_rank,
  rejected_witness_node_count_limit,
  rejected_invalid_node_authority,
  rejected_support_overlap,
  rejected_witness_support_overlap,
  rejected_witness_antichain_overlap,
  insufficient_witness_mass,
  inconclusive_nonnegative_phi,
  rejected_arithmetic_overflow,
};

struct ExactBlockRankPruneAudit {
  std::array<ExactBlockRankNodeAuthority, 2U> support_nodes{};
  std::size_t proposed_witness_node_count{};
  std::size_t canonical_witness_node_count{};
  std::size_t examined_witness_node_count{};
  std::size_t strict_witness_node_count{};
  std::size_t nonnegative_witness_node_count{};
  std::size_t required_witness_point_count{};
  std::size_t proposed_witness_point_count{};
  std::size_t certified_witness_point_count{};
  std::size_t exact_phi_aabb_maximum_evaluation_count{};
  std::size_t unordered_pair_mass{};
  std::size_t directed_pair_mass{};
  bool support_nodes_authenticated{false};
  bool proposal_node_count_limit_checked{false};
  bool support_ranges_disjoint{false};
  bool witness_nodes_authenticated{false};
  bool witness_support_ranges_disjoint{false};
  bool witness_antichain_validated{false};
  bool canonical_witness_order_enforced{false};
  bool witness_mass_overflow_checked{false};
  bool pair_mass_overflow_checked{false};
  bool strict_witness_mass_sufficient{false};
  bool process_local_authority_retained{false};
  bool no_pair_catalog_materialized{true};
  bool no_facet_coface_or_incidence_materialized{true};
  bool no_gamma_or_delaunay_structure_materialized{true};
  bool global_hierarchy_exactness_claimed{false};
  bool public_morse_exactness_claimed{false};

  friend bool operator==(
      const ExactBlockRankPruneAudit&,
      const ExactBlockRankPruneAudit&) = default;
};

class ExactBlockRankPruneReceipt;
class ExactBlockRankPruneResult;
class ExactBlockRankPruneReceiptCertifier;

// Certifies one local Cartesian block A x B only.  The witness nodes retained
// in a receipt form an authenticated, support-disjoint LBVH antichain whose
// total leaf mass is at least maximum_closed_rank - 1.  Every retained box C
// satisfies max_{p in A, q in B, x in C} (x-p).(x-q) < 0 exactly.
[[nodiscard]] ExactBlockRankPruneResult
certify_exact_block_rank_prune_receipt(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t first_support_node_index,
    std::size_t second_support_node_index,
    std::span<const std::size_t> witness_node_indices,
    std::size_t maximum_closed_rank);

// Move-only, process-local authority.  It proves that every unordered pair in
// the two support subtrees is above maximum_closed_rank.  It deliberately
// makes no assertion about a complete hierarchy or any Morse output.
class ExactBlockRankPruneReceipt {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "exact_block_rank_prune_receipt";
  static constexpr std::string_view deployment_status = "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view authority_lifetime = "process_local";
  static constexpr std::string_view proof_basis =
      "exact_diametral_AABB_maximum_strict_witness_LBVH_antichain_v1";

  ExactBlockRankPruneReceipt(const ExactBlockRankPruneReceipt&) = delete;
  ExactBlockRankPruneReceipt& operator=(
      const ExactBlockRankPruneReceipt&) = delete;
  ExactBlockRankPruneReceipt(ExactBlockRankPruneReceipt&&) noexcept = default;
  ExactBlockRankPruneReceipt& operator=(
      ExactBlockRankPruneReceipt&&) noexcept = default;
  ~ExactBlockRankPruneReceipt() = default;

  [[nodiscard]] const std::array<ExactBlockRankNodeAuthority, 2U>&
  support_nodes() const & noexcept {
    return support_nodes_;
  }
  [[nodiscard]] const std::array<ExactBlockRankNodeAuthority, 2U>&
  support_nodes() const && = delete;

  [[nodiscard]] std::span<const ExactBlockRankNodeAuthority> witness_nodes()
      const & noexcept {
    return {witness_nodes_.data(), witness_node_count_};
  }
  [[nodiscard]] std::span<const ExactBlockRankNodeAuthority> witness_nodes()
      const && = delete;

  [[nodiscard]] std::size_t maximum_closed_rank() const noexcept {
    return maximum_closed_rank_;
  }
  [[nodiscard]] std::size_t required_witness_point_count() const noexcept {
    return required_witness_point_count_;
  }
  [[nodiscard]] std::size_t certified_witness_point_count() const noexcept {
    return certified_witness_point_count_;
  }
  [[nodiscard]] std::size_t unordered_pair_mass() const noexcept {
    return unordered_pair_mass_;
  }
  [[nodiscard]] std::size_t directed_pair_mass() const noexcept {
    return directed_pair_mass_;
  }
  [[nodiscard]] const ExactBlockRankPruneAudit& audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactBlockRankPruneAudit& audit() const && = delete;

  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;
  [[nodiscard]] bool certifies(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t first_support_node_index,
      std::size_t second_support_node_index,
      std::size_t maximum_closed_rank) const noexcept;

 private:
  ExactBlockRankPruneReceipt(
      std::array<ExactBlockRankNodeAuthority, 2U> support_nodes,
      std::array<ExactBlockRankNodeAuthority,
                 exact_block_rank_prune_maximum_witness_node_count>
          witness_nodes,
      std::size_t witness_node_count,
      std::size_t maximum_closed_rank,
      std::size_t required_witness_point_count,
      std::size_t certified_witness_point_count,
      std::size_t unordered_pair_mass,
      std::size_t directed_pair_mass,
      ExactBlockRankPruneAudit audit,
      std::shared_ptr<const void> cloud_identity,
      std::shared_ptr<const void> lbvh_identity);

  std::array<ExactBlockRankNodeAuthority, 2U> support_nodes_{};
  std::array<ExactBlockRankNodeAuthority,
             exact_block_rank_prune_maximum_witness_node_count>
      witness_nodes_{};
  std::size_t witness_node_count_{};
  std::size_t maximum_closed_rank_{};
  std::size_t required_witness_point_count_{};
  std::size_t certified_witness_point_count_{};
  std::size_t unordered_pair_mass_{};
  std::size_t directed_pair_mass_{};
  ExactBlockRankPruneAudit audit_{};
  std::shared_ptr<const void> cloud_identity_;
  std::shared_ptr<const void> lbvh_identity_;

  friend class ExactBlockRankPruneReceiptCertifier;
};

class ExactBlockRankPruneResult {
 public:
  ExactBlockRankPruneResult(const ExactBlockRankPruneResult&) = delete;
  ExactBlockRankPruneResult& operator=(
      const ExactBlockRankPruneResult&) = delete;
  ExactBlockRankPruneResult(ExactBlockRankPruneResult&&) noexcept = default;
  ExactBlockRankPruneResult& operator=(
      ExactBlockRankPruneResult&&) noexcept = default;
  ~ExactBlockRankPruneResult() = default;

  [[nodiscard]] ExactBlockRankPruneStatus status() const noexcept {
    return status_;
  }
  [[nodiscard]] bool certified() const noexcept {
    return status_ == ExactBlockRankPruneStatus::certified_above_rank &&
        receipt_.has_value();
  }
  [[nodiscard]] const ExactBlockRankPruneAudit& audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactBlockRankPruneAudit& audit() const && = delete;
  [[nodiscard]] const ExactBlockRankPruneReceipt* receipt() const & noexcept {
    return receipt_ ? &*receipt_ : nullptr;
  }
  [[nodiscard]] const ExactBlockRankPruneReceipt* receipt() const && = delete;

 private:
  ExactBlockRankPruneResult(
      ExactBlockRankPruneStatus status,
      ExactBlockRankPruneAudit audit,
      std::optional<ExactBlockRankPruneReceipt> receipt);

  ExactBlockRankPruneStatus status_{};
  ExactBlockRankPruneAudit audit_{};
  std::optional<ExactBlockRankPruneReceipt> receipt_;

  friend class ExactBlockRankPruneReceiptCertifier;
};

}  // namespace morsehgp3d::hierarchy
