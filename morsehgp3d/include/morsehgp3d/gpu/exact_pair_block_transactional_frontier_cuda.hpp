#pragma once

#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_transactional_frontier_cuda_schema_version = 1U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_cuda_maximum_closed_rank = 11U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_cuda_maximum_witness_node_count =
        exact_pair_block_transactional_frontier_cuda_maximum_closed_rank - 1U;
inline constexpr std::size_t
    exact_pair_block_transactional_frontier_cuda_maximum_capacity_per_point =
        64U;
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_cuda_backend = "reference_cpu";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_cuda_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_cuda_mode =
        "bounded_host_contract_double_buffered_transactional_pair_block_"
        "frontier";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_cuda_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_pair_block_transactional_frontier_cuda_public_status =
        "not_claimed";

// Every capacity is a fixed multiple of the input point count.  A terminal
// output may still be quadratic on an adversarial cloud; in that case this
// bounded contract rolls the complete wave back and reports capacity
// exhaustion instead of allocating a disguised dense pair universe.
struct ExactPairBlockTransactionalFrontierCudaConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t pending_block_capacity_per_point{16U};
  std::size_t prune_receipt_capacity_per_point{8U};
  std::size_t terminal_pair_capacity_per_point{16U};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaConfig&,
      const ExactPairBlockTransactionalFrontierCudaConfig&) = default;
};

enum class ExactPairBlockTransactionalFrontierCudaProposalKind :
    std::uint8_t {
  // Derive the unique native split, or the terminal pair, without attempting
  // a geometric prune.
  open,
  // Recompute the exact block receipt.  Any inconclusive or rejected receipt
  // fails open to the same native split/terminal transition.
  try_exact_prune_else_open,
};

// One proposal corresponds positionally to one block in the open wave.  The
// echoed source and unique transaction id prevent accidental batch mixing;
// neither is accepted as authority for a split or a pair mass.
struct ExactPairBlockTransactionalFrontierCudaProposal {
  std::uint64_t transaction_id{};
  ExactPairBlockAuthority source{};
  ExactPairBlockTransactionalFrontierCudaProposalKind kind{
      ExactPairBlockTransactionalFrontierCudaProposalKind::open};
  std::array<
      std::size_t,
      exact_pair_block_transactional_frontier_cuda_maximum_witness_node_count>
      witness_node_indices{};
  std::size_t witness_node_count{};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaProposal&,
      const ExactPairBlockTransactionalFrontierCudaProposal&) = default;
};

// Compact process-local replay material for one pruned Cartesian product.
// The exact Q signs are deliberately recomputed from the immutable cloud and
// LBVH when a terminal authority is validated.
struct ExactPairBlockTransactionalFrontierCudaPruneReceipt {
  std::uint64_t transaction_id{};
  ExactPairBlockAuthority support_block{};
  std::array<
      ExactPairBlockNodeAuthority,
      exact_pair_block_transactional_frontier_cuda_maximum_witness_node_count>
      witness_nodes{};
  std::size_t witness_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t certified_witness_point_count{};
  std::size_t unordered_pair_mass{};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt&,
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt&) = default;
};

struct ExactPairBlockTransactionalFrontierCudaTerminalPair {
  std::uint64_t transaction_id{};
  ExactPairBlockNodeAuthority first_node{};
  ExactPairBlockNodeAuthority second_node{};
  std::array<spatial::PointId, 2U> point_ids{};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaTerminalPair&,
      const ExactPairBlockTransactionalFrontierCudaTerminalPair&) = default;
};

enum class ExactPairBlockTransactionalFrontierCudaWaveStart : std::uint8_t {
  wave_ready,
  frontier_complete,
};

enum class ExactPairBlockTransactionalFrontierCudaWaveDecision :
    std::uint8_t {
  complete_wave_commit,
  no_wave_open,
  rolled_back_invalid_proposal,
  rolled_back_capacity_exhausted,
  rolled_back_arithmetic_overflow,
  rolled_back_operational_allocation_failure,
};

struct ExactPairBlockTransactionalFrontierCudaWaveResult {
  ExactPairBlockTransactionalFrontierCudaWaveDecision decision{
      ExactPairBlockTransactionalFrontierCudaWaveDecision::no_wave_open};
  std::size_t submitted_block_count{};
  std::size_t certified_prune_count{};
  std::size_t fail_open_count{};
  std::size_t diagonal_split_count{};
  std::size_t cross_split_count{};
  std::size_t terminal_pair_count{};
  std::size_t successor_block_count{};
  std::size_t committed_pruned_unordered_pair_mass{};
  std::size_t committed_terminal_unordered_pair_mass{};
  std::size_t committed_pending_unordered_pair_mass{};
  bool scientific_state_mutated{false};
  bool source_wave_restored_on_rejection{false};

  [[nodiscard]] bool committed() const noexcept {
    return decision == ExactPairBlockTransactionalFrontierCudaWaveDecision::
                           complete_wave_commit &&
        scientific_state_mutated;
  }

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaWaveResult&,
      const ExactPairBlockTransactionalFrontierCudaWaveResult&) = default;
};

enum class ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus :
    std::uint8_t {
  ready,
  invalid_proposal,
  arithmetic_overflow,
};

// Exact, non-mutating count/mass classification for one source transition.
// The resident host/fake launcher uses this as the count stage before its
// bounded page scan.  The subsequent commit replays the same predicates and
// remains the only scientific mutation.
struct ExactPairBlockTransactionalFrontierCudaTransitionPreview {
  ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus status{
      ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::
          invalid_proposal};
  std::size_t successor_block_count{};
  std::size_t prune_receipt_count{};
  std::size_t terminal_pair_count{};
  std::size_t successor_unordered_pair_mass{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};

  [[nodiscard]] bool ready() const noexcept {
    return status ==
        ExactPairBlockTransactionalFrontierCudaTransitionPreviewStatus::ready;
  }

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaTransitionPreview&,
      const ExactPairBlockTransactionalFrontierCudaTransitionPreview&) =
      default;
};

struct ExactPairBlockTransactionalFrontierCudaAudit {
  std::uint32_t schema_version{
      exact_pair_block_transactional_frontier_cuda_schema_version};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_closed_rank{};
  std::size_t required_witness_point_count{};
  std::size_t pending_block_capacity{};
  std::size_t prune_receipt_capacity{};
  std::size_t terminal_pair_capacity{};
  std::size_t unordered_pair_universe_mass{};
  std::size_t pending_unordered_pair_mass{};
  std::size_t inflight_unordered_pair_mass{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t terminal_unordered_pair_mass{};
  std::size_t pending_block_count{};
  std::size_t inflight_block_count{};
  std::size_t prune_receipt_count{};
  std::size_t terminal_pair_count{};
  std::size_t wave_begin_count{};
  std::size_t wave_commit_count{};
  std::size_t wave_rollback_count{};
  std::size_t explicit_rollback_count{};
  std::size_t exact_prune_attempt_count{};
  std::size_t certified_prune_count{};
  std::size_t exact_prune_fail_open_count{};
  std::size_t diagonal_split_count{};
  std::size_t cross_split_count{};
  std::size_t double_buffer_swap_count{};
  std::size_t maximum_pending_block_count{};
  std::size_t maximum_inflight_block_count{};
  std::uint64_t final_cut_digest{};
  bool fixed_linear_capacities_validated{false};
  bool compact_rank_and_witness_bounds_validated{false};
  bool every_wave_source_matched_positionally{false};
  bool unique_transaction_ids_validated{false};
  bool native_split_partition_validated{false};
  bool pairwise_disjoint_support_products_validated{false};
  bool transactional_mass_conservation_validated{false};
  bool rejected_wave_restored_without_scientific_mutation{false};
  bool logical_double_buffer_frontier_exercised{false};
  bool global_pair_coverage_closed{false};
  bool terminal_authority_sealed{false};
  bool cuda_execution_performed{false};
  bool pair_catalog_complete_claimed{false};
  bool hierarchy_or_tree_claimed{false};
  bool slo_claimed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_facet_coface_or_incidence_arena_materialized{false};
  bool durable_receipt_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactPairBlockTransactionalFrontierCudaAudit&,
      const ExactPairBlockTransactionalFrontierCudaAudit&) = default;
};

class ExactPairBlockTransactionalFrontierCudaContext;

// Move-only terminal authority for the pair-block partition only.  It proves
// that every unordered pair belongs exactly once to a certified prune block
// or to the bounded terminal-pair stream.  It is neither a multi-rank pair
// catalogue nor a hierarchy.
class ExactPairBlockTransactionalFrontierCudaTerminalAuthority final {
 public:
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority(
      const ExactPairBlockTransactionalFrontierCudaTerminalAuthority&) =
      delete;
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority& operator=(
      const ExactPairBlockTransactionalFrontierCudaTerminalAuthority&) =
      delete;
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority(
      ExactPairBlockTransactionalFrontierCudaTerminalAuthority&&) noexcept =
      default;
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority& operator=(
      ExactPairBlockTransactionalFrontierCudaTerminalAuthority&&) noexcept =
      default;
  ~ExactPairBlockTransactionalFrontierCudaTerminalAuthority() = default;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaAudit& audit()
      const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  prune_receipts() const & noexcept {
    return prune_receipts_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  prune_receipts() const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  terminal_pairs() const & noexcept {
    return terminal_pairs_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  terminal_pairs() const && = delete;

 private:
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority(
      ExactPairBlockTransactionalFrontierCudaAudit audit,
      std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>&&
          prune_receipts,
      std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>&&
          terminal_pairs,
      std::shared_ptr<const void> lbvh_identity) noexcept;

  ExactPairBlockTransactionalFrontierCudaAudit audit_{};
  std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>
      prune_receipts_;
  std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>
      terminal_pairs_;
  std::shared_ptr<const void> lbvh_identity_;

  friend class ExactPairBlockTransactionalFrontierCudaContext;
};

// Host executable specification of the future resident CUDA scheduler.  A
// wave treats either the complete current buffer or one leading page as
// in-flight, retains an unselected suffix, stages outputs in the other buffer,
// and publishes them together.  Any invalid proposal, arithmetic failure, or
// capacity shortfall restores the source wave exactly.
class ExactPairBlockTransactionalFrontierCudaContext final {
 public:
  static constexpr std::string_view backend =
      exact_pair_block_transactional_frontier_cuda_backend;
  static constexpr std::string_view profile =
      exact_pair_block_transactional_frontier_cuda_profile;
  static constexpr std::string_view mode =
      exact_pair_block_transactional_frontier_cuda_mode;
  static constexpr std::string_view deployment_status =
      exact_pair_block_transactional_frontier_cuda_deployment_status;
  static constexpr std::string_view public_status =
      exact_pair_block_transactional_frontier_cuda_public_status;

  // The immutable cloud and LBVH are borrowed authorities.  They must outlive
  // the context and must not be moved while it is active.
  [[nodiscard]] static ExactPairBlockTransactionalFrontierCudaContext start(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockTransactionalFrontierCudaConfig config = {});
  [[nodiscard]] static ExactPairBlockTransactionalFrontierCudaContext start(
      spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      ExactPairBlockTransactionalFrontierCudaConfig = {}) = delete;
  [[nodiscard]] static ExactPairBlockTransactionalFrontierCudaContext start(
      const spatial::MortonLbvhIndex&&,
      const spatial::CanonicalPointCloud&,
      ExactPairBlockTransactionalFrontierCudaConfig = {}) = delete;
  [[nodiscard]] static ExactPairBlockTransactionalFrontierCudaContext start(
      const spatial::MortonLbvhIndex&,
      spatial::CanonicalPointCloud&&,
      ExactPairBlockTransactionalFrontierCudaConfig = {}) = delete;
  [[nodiscard]] static ExactPairBlockTransactionalFrontierCudaContext start(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&&,
      ExactPairBlockTransactionalFrontierCudaConfig = {}) = delete;

  ExactPairBlockTransactionalFrontierCudaContext(
      const ExactPairBlockTransactionalFrontierCudaContext&) = delete;
  ExactPairBlockTransactionalFrontierCudaContext& operator=(
      const ExactPairBlockTransactionalFrontierCudaContext&) = delete;
  ExactPairBlockTransactionalFrontierCudaContext(
      ExactPairBlockTransactionalFrontierCudaContext&& other) noexcept;
  ExactPairBlockTransactionalFrontierCudaContext& operator=(
      ExactPairBlockTransactionalFrontierCudaContext&&) = delete;
  ~ExactPairBlockTransactionalFrontierCudaContext() = default;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool wave_open() const noexcept {
    return ready() && wave_open_;
  }
  [[nodiscard]] bool validated_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud) const noexcept;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaConfig& config()
      const & noexcept {
    return config_;
  }
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaConfig& config()
      const && = delete;
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaAudit& audit()
      const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactPairBlockTransactionalFrontierCudaAudit& audit()
      const && = delete;
  [[nodiscard]] std::span<const ExactPairBlockAuthority> pending_blocks()
      const & noexcept;
  [[nodiscard]] std::span<const ExactPairBlockAuthority> pending_blocks()
      const && = delete;
  [[nodiscard]] std::span<const ExactPairBlockAuthority> inflight_blocks()
      const & noexcept;
  [[nodiscard]] std::span<const ExactPairBlockAuthority> inflight_blocks()
      const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  committed_prune_receipts() const & noexcept {
    return prune_receipts_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaPruneReceipt>
  committed_prune_receipts() const && = delete;
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  committed_terminal_pairs() const & noexcept {
    return terminal_pairs_;
  }
  [[nodiscard]] std::span<
      const ExactPairBlockTransactionalFrontierCudaTerminalPair>
  committed_terminal_pairs() const && = delete;

  [[nodiscard]] ExactPairBlockTransactionalFrontierCudaWaveStart begin_wave()
      &;
  // Opens at most maximum_source_count leading sources.  Unselected sources
  // stay pending and are copied ahead of emitted successors at commit.  This
  // is the host-contract injection point for deterministic resident pages.
  [[nodiscard]] ExactPairBlockTransactionalFrontierCudaWaveStart begin_wave(
      std::size_t maximum_source_count) &;
  [[nodiscard]]
      ExactPairBlockTransactionalFrontierCudaTransitionPreview
      preview_transition(
          const ExactPairBlockTransactionalFrontierCudaProposal& proposal)
          const &;
  [[nodiscard]]
      ExactPairBlockTransactionalFrontierCudaTransitionPreview
      preview_transition(
          const ExactPairBlockTransactionalFrontierCudaProposal&) const && =
          delete;
  [[nodiscard]] ExactPairBlockTransactionalFrontierCudaWaveResult commit_wave(
      std::span<const ExactPairBlockTransactionalFrontierCudaProposal>
          proposals) &;
  [[nodiscard]] bool rollback_wave() & noexcept;

  [[nodiscard]] ExactPairBlockTransactionalFrontierCudaTerminalAuthority seal()
      &&;
  ExactPairBlockTransactionalFrontierCudaTerminalAuthority seal() & = delete;

 private:
  struct PrivateConstructionTag {};

  ExactPairBlockTransactionalFrontierCudaContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      ExactPairBlockTransactionalFrontierCudaConfig config,
      PrivateConstructionTag);

  void refresh_audit();
  void verify_internal_mass() const;
  [[nodiscard]] ExactPairBlockTransactionalFrontierCudaWaveResult
  rollback_result(
      ExactPairBlockTransactionalFrontierCudaWaveDecision decision,
      std::size_t submitted_block_count) noexcept;

  ExactPairBlockTransactionalFrontierCudaConfig config_{};
  ExactPairBlockTransactionalFrontierCudaAudit audit_{};
  std::array<std::vector<ExactPairBlockAuthority>, 2U> block_buffers_;
  std::size_t active_buffer_index_{};
  std::vector<ExactPairBlockTransactionalFrontierCudaPruneReceipt>
      prune_receipts_;
  std::vector<ExactPairBlockTransactionalFrontierCudaTerminalPair>
      terminal_pairs_;
  const spatial::MortonLbvhIndex* index_{};
  const spatial::CanonicalPointCloud* cloud_{};
  std::shared_ptr<const void> lbvh_identity_;
  std::size_t pending_unordered_pair_mass_{};
  std::size_t inflight_unordered_pair_mass_{};
  std::size_t inflight_block_count_{};
  std::size_t pruned_unordered_pair_mass_{};
  std::size_t terminal_unordered_pair_mass_{};
  std::size_t pending_block_capacity_{};
  std::size_t prune_receipt_capacity_{};
  std::size_t terminal_pair_capacity_{};
  bool wave_open_{false};
  bool complete_{false};
  bool sealed_or_moved_{false};
};

}  // namespace morsehgp3d::gpu
