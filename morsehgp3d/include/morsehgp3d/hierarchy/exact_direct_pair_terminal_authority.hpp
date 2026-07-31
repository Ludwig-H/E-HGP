#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace morsehgp3d::gpu {
class ExactPairBlockToDirectPairTerminalBuilder;
}

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    exact_direct_pair_terminal_authority_schema_version = 1U;
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_backend =
        "cuda_g4_plus_reference_cpu_classifier";
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_mode =
        "bounded_transactional_pair_cut_plus_exact_terminal_classifier";
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_public_status = "not_claimed";
inline constexpr std::string_view
    exact_direct_pair_terminal_authority_proof_basis =
        "authenticated_unordered_pair_block_partition_plus_exact_closed_ball_"
        "classification_of_every_terminal_leaf_pair_v1";

enum class ExactDirectPairTerminalSourceKind : std::uint8_t {
  unspecified,
  reference_host_fake_transactional_pair_cut,
  qualified_cuda_transactional_pair_cut,
};

using ExactDirectPairTerminalRecord =
    std::variant<ExactPairSupportEvent,
                 ExactPairSupportExtraShellDiagnostic>;

struct ExactDirectPairTerminalAudit {
  std::uint32_t schema_version{
      exact_direct_pair_terminal_authority_schema_version};
  ExactDirectPairTerminalSourceKind source_kind{
      ExactDirectPairTerminalSourceKind::unspecified};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t maximum_closed_rank{};
  std::size_t unordered_pair_universe_size{};
  std::size_t source_prune_receipt_count{};
  std::size_t pruned_unordered_pair_mass{};
  std::size_t source_terminal_pair_count{};
  std::uint64_t source_final_cut_digest{};
  std::size_t classification_started_count{};
  std::size_t classification_terminal_count{};
  std::size_t classification_node_visit_count{};
  std::size_t classification_budget_exhaustion_count{};
  std::size_t above_rank_count{};
  std::size_t accepted_event_count{};
  std::size_t relevant_extra_shell_diagnostic_count{};
  std::size_t emitted_record_count{};
  std::size_t emitted_point_id_reference_count{};
  bool source_partition_validated{false};
  bool every_prune_recertified{false};
  bool unordered_pair_coverage_certified{false};
  bool terminal_pair_classification_partition_certified{false};
  bool output_partition_certified{false};
  bool retained_records_certified{false};
  bool qualified_cuda_execution{false};
  bool host_fake_execution{false};
  bool no_forbidden_global_structure_materialized{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool global_facet_coface_or_incidence_arena_materialized{false};
  bool hierarchy_reduction_performed{false};
  bool public_status_claimed{false};
  bool complete{false};
  bool poisoned{false};

  [[nodiscard]] bool terminal_identities_hold() const noexcept;

  friend bool operator==(
      const ExactDirectPairTerminalAudit&,
      const ExactDirectPairTerminalAudit&) = default;
};

// GPU-free, move-only process-local authority for the complete support-2
// terminal catalogue.  It owns only rank-relevant records; pair blocks that
// were certified above rank and terminal pairs classified above rank remain
// scalar mass in the audit.  It is neither a pair matrix nor a hierarchy.
class ExactDirectPairTerminalAuthority final {
 public:
  static constexpr std::string_view backend =
      exact_direct_pair_terminal_authority_backend;
  static constexpr std::string_view profile =
      exact_direct_pair_terminal_authority_profile;
  static constexpr std::string_view mode =
      exact_direct_pair_terminal_authority_mode;
  static constexpr std::string_view deployment_status =
      exact_direct_pair_terminal_authority_deployment_status;
  static constexpr std::string_view public_status =
      exact_direct_pair_terminal_authority_public_status;
  static constexpr std::string_view proof_basis =
      exact_direct_pair_terminal_authority_proof_basis;

  ExactDirectPairTerminalAuthority(
      const ExactDirectPairTerminalAuthority&) = delete;
  ExactDirectPairTerminalAuthority& operator=(
      const ExactDirectPairTerminalAuthority&) = delete;
  ExactDirectPairTerminalAuthority(
      ExactDirectPairTerminalAuthority&& other) noexcept;
  ExactDirectPairTerminalAuthority& operator=(
      ExactDirectPairTerminalAuthority&&) = delete;
  ~ExactDirectPairTerminalAuthority() = default;

  [[nodiscard]] bool bound_to(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order) const noexcept;
  [[nodiscard]] bool sealed_in_process_terminal_authority() const noexcept;
  [[nodiscard]] const ExactPairSupportCheckpointManifest& manifest() const
      & noexcept {
    return manifest_;
  }
  [[nodiscard]] const ExactPairSupportCheckpointManifest& manifest() const
      && = delete;
  [[nodiscard]] const ExactDirectPairTerminalAudit& audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactDirectPairTerminalAudit& audit() const && = delete;
  [[nodiscard]] std::span<const ExactDirectPairTerminalRecord> records() const
      & noexcept {
    return records_;
  }
  [[nodiscard]] std::span<const ExactDirectPairTerminalRecord> records() const
      && = delete;

  [[nodiscard]] std::vector<ExactDirectPairTerminalRecord> release_records()
      &&;
  std::vector<ExactDirectPairTerminalRecord> release_records() & = delete;

 private:
  ExactDirectPairTerminalAuthority(
      ExactPairSupportCheckpointManifest manifest,
      ExactDirectPairTerminalAudit audit,
      std::vector<ExactDirectPairTerminalRecord>&& records) noexcept;

  ExactPairSupportCheckpointManifest manifest_{};
  ExactDirectPairTerminalAudit audit_{};
  std::vector<ExactDirectPairTerminalRecord> records_;
  bool sealed_{false};

  friend class gpu::ExactPairBlockToDirectPairTerminalBuilder;
};

}  // namespace morsehgp3d::hierarchy
