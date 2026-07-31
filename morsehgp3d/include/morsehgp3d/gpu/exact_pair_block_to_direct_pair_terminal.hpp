#pragma once

#include "morsehgp3d/gpu/exact_pair_block_transactional_frontier_resident_cuda.hpp"
#include "morsehgp3d/hierarchy/exact_direct_pair_terminal_authority.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_pair_block_to_direct_pair_terminal_schema_version = 1U;
inline constexpr std::string_view
    exact_pair_block_to_direct_pair_terminal_backend =
        "cuda_g4_plus_reference_cpu_classifier";
inline constexpr std::string_view
    exact_pair_block_to_direct_pair_terminal_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_pair_block_to_direct_pair_terminal_mode =
        "bounded_atomic_terminal_pair_classification";
inline constexpr std::string_view
    exact_pair_block_to_direct_pair_terminal_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_pair_block_to_direct_pair_terminal_public_status = "not_claimed";

struct ExactPairBlockToDirectPairTerminalBudget {
  std::size_t maximum_classification_node_visit_count{};
  std::size_t maximum_emitted_record_count{};
  std::size_t maximum_emitted_point_id_reference_count{};

  [[nodiscard]] static constexpr ExactPairBlockToDirectPairTerminalBudget
  unlimited() noexcept {
    return {
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max(),
        std::numeric_limits<std::size_t>::max()};
  }

  friend bool operator==(
      const ExactPairBlockToDirectPairTerminalBudget&,
      const ExactPairBlockToDirectPairTerminalBudget&) = default;
};

enum class ExactPairBlockToDirectPairTerminalStatus : std::uint8_t {
  complete,
  source_not_terminal,
  source_authority_mismatch,
  classification_node_visit_capacity_exhausted,
  output_record_capacity_exhausted,
  output_point_id_reference_capacity_exhausted,
  operational_allocation_failure,
};

struct ExactPairBlockToDirectPairTerminalAttempt {
  ExactPairBlockToDirectPairTerminalStatus status{
      ExactPairBlockToDirectPairTerminalStatus::source_not_terminal};
  ExactPairBlockToDirectPairTerminalBudget requested_budget{};
  hierarchy::ExactDirectPairTerminalAudit audit{};
  std::unique_ptr<hierarchy::ExactDirectPairTerminalAuthority> authority;

  [[nodiscard]] bool complete() const noexcept {
    return status == ExactPairBlockToDirectPairTerminalStatus::complete &&
        authority != nullptr &&
        authority->sealed_in_process_terminal_authority() &&
        authority->audit() == audit;
  }
};

// Converts a fully authenticated unordered pair-block cut into the neutral
// support-2 terminal authority consumed downstream.  Only terminal leaf pairs
// are classified; certified prune blocks remain scalar mass.  No pair matrix,
// Delaunay mosaic, coface table or hierarchy arena is constructed.
class ExactPairBlockToDirectPairTerminalBuilder final {
 public:
  static constexpr std::string_view backend =
      exact_pair_block_to_direct_pair_terminal_backend;
  static constexpr std::string_view profile =
      exact_pair_block_to_direct_pair_terminal_profile;
  static constexpr std::string_view mode =
      exact_pair_block_to_direct_pair_terminal_mode;
  static constexpr std::string_view deployment_status =
      exact_pair_block_to_direct_pair_terminal_deployment_status;
  static constexpr std::string_view public_status =
      exact_pair_block_to_direct_pair_terminal_public_status;

  [[nodiscard]] static ExactPairBlockToDirectPairTerminalAttempt build(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairBlockToDirectPairTerminalBudget budget,
      ExactPairBlockTransactionalFrontierResidentCudaResult source);
};

[[nodiscard]] ExactPairBlockToDirectPairTerminalAttempt
build_exact_pair_block_to_direct_pair_terminal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactPairBlockToDirectPairTerminalBudget budget,
    ExactPairBlockTransactionalFrontierResidentCudaResult source);

}  // namespace morsehgp3d::gpu
