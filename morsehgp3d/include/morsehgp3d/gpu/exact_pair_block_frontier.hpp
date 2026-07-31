#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::size_t
    exact_pair_block_frontier_maximum_closed_rank = 6U;
inline constexpr std::size_t
    exact_pair_block_frontier_maximum_witness_antichain_node_count =
        exact_pair_block_frontier_maximum_closed_rank - 1U;
inline constexpr std::size_t
    exact_pair_block_frontier_maximum_lbvh_depth =
        3U * spatial::MortonLbvhIndex::morton_bits_per_axis +
        std::numeric_limits<std::size_t>::digits;
inline constexpr std::size_t
    exact_pair_block_frontier_maximum_pending_authority_count =
        3U * exact_pair_block_frontier_maximum_lbvh_depth + 1U;

struct ExactPairBlockNodeAuthority {
  std::size_t node_index{};
  std::size_t leaf_begin{};
  std::size_t leaf_end{};

  [[nodiscard]] std::size_t leaf_count() const noexcept {
    return leaf_end - leaf_begin;
  }

  friend bool operator==(
      const ExactPairBlockNodeAuthority&,
      const ExactPairBlockNodeAuthority&) = default;
};

enum class ExactPairBlockAuthorityKind : std::uint8_t {
  diagonal,
  cross,
};

struct ExactPairBlockAuthority {
  ExactPairBlockAuthorityKind kind{
      ExactPairBlockAuthorityKind::diagonal};
  ExactPairBlockNodeAuthority first{};
  ExactPairBlockNodeAuthority second{};
  std::size_t unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockAuthority&,
      const ExactPairBlockAuthority&) = default;
};

struct ExactPairBlockFrontierConfig {
  std::size_t maximum_closed_rank{};
  bool prioritize_cross_blocks{false};

  friend bool operator==(
      const ExactPairBlockFrontierConfig&,
      const ExactPairBlockFrontierConfig&) = default;
};

struct ExactPairBlockFrontierCapacity {
  // Runtime retention cap for pending plus active authorities.  The storage
  // itself remains fixed and independent of the point count.
  std::size_t maximum_retained_authority_count{
      exact_pair_block_frontier_maximum_pending_authority_count};

  friend bool operator==(
      const ExactPairBlockFrontierCapacity&,
      const ExactPairBlockFrontierCapacity&) = default;
};

enum class ExactPairBlockFrontierStepKind : std::uint8_t {
  cross_block,
  complete,
  inconclusive_capacity,
  inconclusive_arithmetic_overflow,
};

struct ExactPairBlockFrontierStep {
  ExactPairBlockFrontierStepKind kind{
      ExactPairBlockFrontierStepKind::inconclusive_capacity};
  std::optional<ExactPairBlockAuthority> cross_block;
  std::size_t retained_authority_count{};
  std::size_t pending_unordered_pair_mass{};
  std::size_t awaiting_unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockFrontierStep&,
      const ExactPairBlockFrontierStep&) = default;
};

enum class ExactPairBlockOpenKind : std::uint8_t {
  first_support_split,
  second_support_split,
  terminal_pair,
  inconclusive_capacity,
  inconclusive_arithmetic_overflow,
};

struct ExactPairBlockOpenResult {
  ExactPairBlockOpenKind kind{
      ExactPairBlockOpenKind::inconclusive_capacity};
  ExactPairBlockAuthority source{};
  std::optional<std::array<spatial::PointId, 2U>> terminal_pair;
  std::size_t opened_unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockOpenResult&,
      const ExactPairBlockOpenResult&) = default;
};

enum class ExactPairBlockQProposalState : std::uint8_t {
  usable,
  ambiguous,
  arithmetic_overflow,
};

// A proposal is never authoritative.  Its sign is only a routing/audit hint;
// every pruning decision below is replayed by the exact host predicate.
struct ExactPairBlockQProposal {
  ExactPairBlockQProposalState state{
      ExactPairBlockQProposalState::ambiguous};
  int proposed_maximum_sign{};

  friend bool operator==(
      const ExactPairBlockQProposal&,
      const ExactPairBlockQProposal&) = default;
};

struct ExactPairBlockWitnessBudget {
  std::size_t maximum_exact_q_evaluation_count{1U};

  friend bool operator==(
      const ExactPairBlockWitnessBudget&,
      const ExactPairBlockWitnessBudget&) = default;
};

enum class ExactPairBlockWitnessStatus : std::uint8_t {
  certified_closed,
  inconclusive_nonnegative_q,
  inconclusive_insufficient_witness_mass,
  inconclusive_invalid_witness_authority,
  inconclusive_witness_support_overlap,
  inconclusive_witness_antichain_overlap,
  inconclusive_witness_antichain_capacity,
  inconclusive_ambiguous_proposal,
  inconclusive_proposal_arithmetic_overflow,
  inconclusive_exact_replay_limit,
  inconclusive_arithmetic_overflow,
};

struct ExactPairBlockQReplay {
  std::array<spatial::ExactDyadicAabb3, 3U> boxes{};
  hierarchy::ExactDiametralPhiAabbMaximum maximum{};
  std::size_t exact_axis_endpoint_product_count{24U};
  bool existing_exact_fallback_used{true};

  friend bool operator==(
      const ExactPairBlockQReplay&,
      const ExactPairBlockQReplay&) = default;
};

struct ExactPairBlockWitnessResult {
  ExactPairBlockWitnessStatus status{
      ExactPairBlockWitnessStatus::inconclusive_exact_replay_limit};
  ExactPairBlockAuthority support_block{};
  std::optional<ExactPairBlockNodeAuthority> witness;
  std::optional<ExactPairBlockQReplay> exact_q_replay;
  ExactPairBlockQProposal proposal{};
  std::size_t required_witness_point_count{};
  std::size_t authenticated_witness_point_count{};
  std::size_t certified_unordered_pair_mass{};
  bool frontier_mutated{false};
  bool proposal_agreed_with_exact{false};

  friend bool operator==(
      const ExactPairBlockWitnessResult&,
      const ExactPairBlockWitnessResult&) = default;
};

// At most R-1 nonempty, pairwise-disjoint native nodes are ever necessary:
// once their disjoint leaf mass reaches R-1, further witnesses cannot
// strengthen the rank decision.  Each node is nevertheless recertified by an
// independent exact Q maximum before any mass is committed.
struct ExactPairBlockWitnessAntichainResult {
  ExactPairBlockWitnessStatus status{
      ExactPairBlockWitnessStatus::inconclusive_exact_replay_limit};
  ExactPairBlockAuthority support_block{};
  std::array<
      ExactPairBlockNodeAuthority,
      exact_pair_block_frontier_maximum_witness_antichain_node_count>
      witnesses{};
  std::array<
      ExactPairBlockQReplay,
      exact_pair_block_frontier_maximum_witness_antichain_node_count>
      exact_q_replays{};
  std::array<
      ExactPairBlockQProposal,
      exact_pair_block_frontier_maximum_witness_antichain_node_count>
      proposals{};
  std::size_t submitted_witness_node_count{};
  std::size_t authenticated_witness_node_count{};
  std::size_t exact_q_replay_count{};
  std::size_t required_witness_point_count{};
  std::size_t authenticated_witness_point_count{};
  std::size_t proposal_agreement_count{};
  std::size_t proposal_mismatch_count{};
  std::size_t certified_unordered_pair_mass{};
  bool frontier_mutated{false};

  friend bool operator==(
      const ExactPairBlockWitnessAntichainResult&,
      const ExactPairBlockWitnessAntichainResult&) = default;
};

struct ExactPairBlockFrontierAudit {
  std::size_t total_unordered_pair_mass{};
  std::size_t pending_unordered_pair_mass{};
  std::size_t awaiting_unordered_pair_mass{};
  std::size_t certified_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};
  std::size_t diagonal_split_count{};
  std::size_t cross_split_count{};
  std::size_t terminal_pair_count{};
  std::size_t certified_cross_block_count{};
  std::size_t exact_q_evaluation_count{};
  std::size_t exact_q_proposal_mismatch_count{};
  std::size_t maximum_retained_authority_count{};
  bool exact_mass_conservation_verified{false};
  bool complete{false};
  bool disjoint_native_lbvh_authorities_only{true};
  bool no_global_pair_matrix_materialized{true};
  bool no_delaunay_cell_coface_or_incidence_materialized{true};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};

  friend bool operator==(
      const ExactPairBlockFrontierAudit&,
      const ExactPairBlockFrontierAudit&) = default;
};

// First host contract for a future A x B x W device frontier.  It partitions
// T(N) natively as T(L) union C(L,R) union T(R), retains a fixed-size stack,
// and closes C(A,B) only after exact host replay of one support-disjoint
// witness block W.  Inconclusive outcomes retain the active authority; opening
// it is an explicit, transactional operation.
class ExactPairBlockFrontierContext {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "budgeted_component_only_exact_pair_block_frontier";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr std::string_view phase_marker = "phase15_AxBxW_host";
  static constexpr std::string_view proof_basis =
      "native_lbvh_TL_CLR_TR_exact_mass_disjoint_witness_antichain_"
      "exact_dyadic_Q_8_corners_per_axis_v2";

  [[nodiscard]] static ExactPairBlockFrontierContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockFrontierConfig config);

  ExactPairBlockFrontierContext(
      const ExactPairBlockFrontierContext&) = delete;
  ExactPairBlockFrontierContext& operator=(
      const ExactPairBlockFrontierContext&) = delete;
  ExactPairBlockFrontierContext(
      ExactPairBlockFrontierContext&&) noexcept = default;
  ExactPairBlockFrontierContext& operator=(
      ExactPairBlockFrontierContext&&) = delete;
  ~ExactPairBlockFrontierContext() = default;

  [[nodiscard]] bool ready() const noexcept {
    return lbvh_identity_ != nullptr;
  }
  [[nodiscard]] bool complete() const noexcept {
    return ready() && complete_;
  }
  [[nodiscard]] bool awaiting_cross_block_response() const noexcept {
    return ready() && awaiting_cross_block_.has_value();
  }
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;
  [[nodiscard]] const ExactPairBlockFrontierConfig& config() const &
      noexcept {
    return config_;
  }
  [[nodiscard]] const ExactPairBlockFrontierConfig& config() const && =
      delete;
  [[nodiscard]] const ExactPairBlockFrontierAudit& audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactPairBlockFrontierAudit& audit() const && = delete;
  [[nodiscard]] std::size_t pending_authority_count() const noexcept {
    return pending_authority_count_;
  }
  [[nodiscard]] std::optional<ExactPairBlockAuthority>
  awaiting_cross_block() const noexcept {
    return awaiting_cross_block_;
  }

  [[nodiscard]] ExactPairBlockFrontierStep advance(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockFrontierCapacity capacity) &;

  [[nodiscard]] ExactPairBlockWitnessResult try_certify_with_witness(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t witness_node_index,
      ExactPairBlockQProposal proposal,
      ExactPairBlockWitnessBudget budget) &;

  [[nodiscard]] ExactPairBlockWitnessAntichainResult
  try_certify_with_witness_antichain(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const std::size_t> witness_node_indices,
      std::span<const ExactPairBlockQProposal> proposals,
      ExactPairBlockWitnessBudget budget) &;

  [[nodiscard]] ExactPairBlockOpenResult open_cross_block(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockFrontierCapacity capacity) &;

 private:
  struct PrivateConstructionTag {};

  ExactPairBlockFrontierContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockFrontierConfig config,
      PrivateConstructionTag);

  [[nodiscard]] ExactPairBlockNodeAuthority node_authority(
      const spatial::MortonLbvhIndex& index,
      std::size_t node_index) const;
  [[nodiscard]] ExactPairBlockAuthority diagonal_authority(
      const spatial::MortonLbvhIndex& index,
      std::size_t node_index) const;
  [[nodiscard]] ExactPairBlockAuthority cross_authority(
      const spatial::MortonLbvhIndex& index,
      std::size_t first_node_index,
      std::size_t second_node_index) const;
  [[nodiscard]] ExactPairBlockWitnessAntichainResult
  try_certify_with_witness_antichain_impl(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const std::size_t> witness_node_indices,
      std::span<const ExactPairBlockQProposal> proposals,
      ExactPairBlockWitnessBudget budget,
      bool replay_before_insufficient_mass_decision) &;
  void validate_authority(
      const spatial::MortonLbvhIndex& index,
      const ExactPairBlockAuthority& authority) const;
  void push_authority(const ExactPairBlockAuthority& authority);
  void refresh_mass_audit();
  void verify_mass_conservation() const;
  void finish_if_complete();

  ExactPairBlockFrontierConfig config_{};
  std::size_t point_count_{};
  std::array<
      ExactPairBlockAuthority,
      exact_pair_block_frontier_maximum_pending_authority_count>
      pending_authorities_{};
  std::size_t pending_authority_count_{};
  std::optional<ExactPairBlockAuthority> awaiting_cross_block_;
  std::shared_ptr<const void> lbvh_identity_;
  std::size_t pending_unordered_pair_mass_{};
  std::size_t awaiting_unordered_pair_mass_{};
  std::size_t certified_unordered_pair_mass_{};
  std::size_t terminal_unordered_pair_mass_{};
  ExactPairBlockFrontierAudit audit_{};
  bool complete_{false};
};

}  // namespace morsehgp3d::gpu
