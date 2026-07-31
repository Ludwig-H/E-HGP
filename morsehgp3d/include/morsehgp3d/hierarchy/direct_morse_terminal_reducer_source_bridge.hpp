#pragma once

#include "morsehgp3d/hierarchy/capped_distinct_point_coverage.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_source_authority.hpp"
#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_terminal_reducer_source_bridge_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_mode =
        "certified_bounded_terminal_sources_to_reducer_source";
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_terminal_reducer_source_bridge_proof_basis =
        "sealed_pair_and_higher_terminal_authorities_complete_direct_"
        "supports_arities_two_through_four_then_fresh_event_seed_and_"
        "bounded_reducer_source_replay_v1";

enum class ExactDirectMorseTerminalReducerSourceBridgeDecision :
    std::uint8_t {
  not_built,
  no_bridge_requested_order_not_five_or_ten,
  no_bridge_min_cluster_size_zero,
  no_bridge_pair_terminal_authority_not_sealed,
  no_bridge_higher_terminal_authority_not_sealed,
  no_bridge_direct_support_catalog_not_terminal,
  no_bridge_relevant_extra_shell_diagnostics,
  no_bridge_event_journal_not_certified,
  no_bridge_saddle_arm_seed_journal_not_certified,
  no_bridge_reducer_source_manifest_not_certified,
  complete_certified_bounded_reducer_source,
};

enum class ExactDirectMorseTerminalReducerSourceBridgeScope : std::uint8_t {
  unspecified,
  terminal_direct_supports_arities_two_through_four_to_reducer_source_with_downstream_cluster_size_view,
};

// This audit deliberately separates source completeness from the downstream
// min-cluster-size view.  The latter is never allowed to prune direct
// supports, saddle arms, silent incidences or an incomplete equality batch.
struct ExactDirectMorseTerminalReducerSourceBridgeAudit {
  static constexpr std::string_view backend =
      direct_morse_terminal_reducer_source_bridge_backend;
  static constexpr std::string_view profile =
      direct_morse_terminal_reducer_source_bridge_profile;
  static constexpr std::string_view mode =
      direct_morse_terminal_reducer_source_bridge_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_terminal_reducer_source_bridge_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_terminal_reducer_source_bridge_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_terminal_reducer_source_bridge_proof_basis;

  std::uint32_t schema_version{
      direct_morse_terminal_reducer_source_bridge_schema_version};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t min_cluster_size{};
  bool requested_order_is_five_or_ten{false};
  bool min_cluster_size_positive{false};
  bool pair_terminal_authority_sealed_on_entry{false};
  bool higher_terminal_authority_sealed_on_entry{false};
  bool source_authorities_match{false};
  bool terminal_authorities_consumed_once{false};
  bool all_support_arities_two_through_four_terminal{false};
  bool no_relevant_extra_shell_diagnostics{false};
  bool event_journal_certified{false};
  bool saddle_arm_seed_journal_certified{false};
  bool reducer_source_manifest_certified{false};
  bool synchronous_batch_provider_ready{false};
  bool min_cluster_size_is_downstream_view_only{false};
  bool complete_equal_level_batch_required_before_cluster_size_view{false};
  bool source_pruning_by_min_cluster_size_performed{false};
  bool forbidden_global_structure_materialized{false};
  bool hierarchy_reduction_performed{false};
  bool public_status_claimed{false};
  ExactDirectMorseTerminalReducerSourceBridgeDecision decision{
      ExactDirectMorseTerminalReducerSourceBridgeDecision::not_built};
  ExactDirectMorseTerminalReducerSourceBridgeScope scope{
      ExactDirectMorseTerminalReducerSourceBridgeScope::unspecified};

  [[nodiscard]] bool certified() const noexcept;

  friend bool operator==(
      const ExactDirectMorseTerminalReducerSourceBridgeAudit&,
      const ExactDirectMorseTerminalReducerSourceBridgeAudit&) = default;
};

struct ExactDirectMorseTerminalReducerSourceBridgeBuildResult;

// Owns the complete resident source chain needed by the Phase-15J provider.
// The pimpl keeps all borrowed adapter pointers stable across bridge moves.
// It is still only reducer input: no hierarchy reduction or public exactness
// is claimed here.
class ExactDirectMorseTerminalReducerSourceBridge {
 public:
  ExactDirectMorseTerminalReducerSourceBridge(
      const ExactDirectMorseTerminalReducerSourceBridge&) = delete;
  ExactDirectMorseTerminalReducerSourceBridge& operator=(
      const ExactDirectMorseTerminalReducerSourceBridge&) = delete;
  ExactDirectMorseTerminalReducerSourceBridge(
      ExactDirectMorseTerminalReducerSourceBridge&&) noexcept;
  ExactDirectMorseTerminalReducerSourceBridge& operator=(
      ExactDirectMorseTerminalReducerSourceBridge&&) noexcept;
  ~ExactDirectMorseTerminalReducerSourceBridge();

  [[nodiscard]] const ExactDirectMorseTerminalReducerSourceBridgeAudit&
  audit() const & noexcept {
    return audit_;
  }
  [[nodiscard]] const ExactDirectMorseTerminalReducerSourceBridgeAudit&
  audit() const && = delete;

  [[nodiscard]] const ExactDirectSupportTerminalFacade&
  direct_support_facade() const & noexcept;
  [[nodiscard]] const ExactDirectSupportTerminalFacade&
  direct_support_facade() const && = delete;
  [[nodiscard]] const ExactDirectMorseEventJournalResult&
  event_journal() const & noexcept;
  [[nodiscard]] const ExactDirectMorseEventJournalResult&
  event_journal() const && = delete;
  [[nodiscard]] const ExactDirectSaddleArmSeedJournalResult&
  saddle_arm_seed_journal() const & noexcept;
  [[nodiscard]] const ExactDirectSaddleArmSeedJournalResult&
  saddle_arm_seed_journal() const && = delete;
  [[nodiscard]] const ExactDirectSaddleArmSeedBudget&
  trusted_seed_budget() const & noexcept;
  [[nodiscard]] const ExactDirectSaddleArmSeedBudget&
  trusted_seed_budget() const && = delete;
  [[nodiscard]] const ExactDirectMorseForestSourceManifest&
  source_manifest() const & noexcept;
  [[nodiscard]] const ExactDirectMorseForestSourceManifest&
  source_manifest() const && = delete;

  // The bridge must outlive every reducer constructed from this view.
  [[nodiscard]] ExactDirectMorseForestSourceBatchProviderView
  source_provider() & noexcept;
  ExactDirectMorseForestSourceBatchProviderView source_provider() && = delete;

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision
  visit_source_batch(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) const;
  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer);

  [[nodiscard]] std::size_t min_cluster_size() const noexcept {
    return audit_.min_cluster_size;
  }

  // Callers may populate this exact capped summary only from an already
  // complete component after the reducer has committed the whole equality
  // batch.  It has no source- or hierarchy-authority role.
  [[nodiscard]] CappedDistinctPointCoverage
  make_downstream_complete_component_coverage() const;

 private:
  struct Impl;

  ExactDirectMorseTerminalReducerSourceBridge(
      std::unique_ptr<Impl> impl,
      ExactDirectMorseTerminalReducerSourceBridgeAudit audit) noexcept;

  std::unique_ptr<Impl> impl_;
  ExactDirectMorseTerminalReducerSourceBridgeAudit audit_{};

  friend ExactDirectMorseTerminalReducerSourceBridgeBuildResult
  build_exact_direct_morse_terminal_reducer_source_bridge(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      std::size_t,
      const ExactHigherSupportStreamBudget&,
      const ExactDirectSaddleArmSeedBudget&,
      std::size_t,
      ExactSparseAnchoredPairTerminalAuthority&&,
      ExactHigherSupportTerminalAuthority&&);
};

struct ExactDirectMorseTerminalReducerSourceBridgeBuildResult {
  ExactDirectMorseTerminalReducerSourceBridgeAudit audit{};
  std::unique_ptr<ExactDirectMorseTerminalReducerSourceBridge> bridge;

  [[nodiscard]] bool certified() const noexcept;
};

// A pair authority alone is intentionally not accepted: terminal higher
// supports of arities three and four are an independent required authority.
// Scalar policy errors are checked before either move-only authority is
// consumed.
[[nodiscard]] ExactDirectMorseTerminalReducerSourceBridgeBuildResult
build_exact_direct_morse_terminal_reducer_source_bridge(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    const ExactDirectSaddleArmSeedBudget& seed_budget,
    std::size_t min_cluster_size,
    ExactSparseAnchoredPairTerminalAuthority&& pair_authority,
    ExactHigherSupportTerminalAuthority&& higher_authority);

}  // namespace morsehgp3d::hierarchy
