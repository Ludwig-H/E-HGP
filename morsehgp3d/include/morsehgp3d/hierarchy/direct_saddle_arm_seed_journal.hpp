#pragma once

#include "morsehgp3d/hierarchy/direct_morse_event_journal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_saddle_arm_seed_journal_schema_version = 1U;
inline constexpr std::string_view direct_saddle_arm_seed_journal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_saddle_arm_seed_journal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_saddle_arm_seed_journal_mode =
    "certified";
inline constexpr std::string_view
    direct_saddle_arm_seed_journal_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_saddle_arm_seed_journal_public_status = "not_claimed";
inline constexpr std::string_view
    direct_saddle_arm_seed_journal_proof_basis =
        "source_relative_terminal_positive_minimal_support_deleted_facet_"
        "strict_miniball_drop_factorized_arm_seeds_v1";

// The budget is checked before the source journal is freshly replayed and
// before any output vector is reserved.  The replay-entry requirement is the
// proved 3n+5E upper bound of the source journal, not a mutable observed
// counter.
struct ExactDirectSaddleArmSeedBudget {
  std::size_t maximum_source_journal_replay_entry_count{};
  std::size_t maximum_role_scan_count{};
  std::size_t maximum_saddle_family_count{};
  std::size_t maximum_arm_seed_count{};

  friend bool operator==(
      const ExactDirectSaddleArmSeedBudget&,
      const ExactDirectSaddleArmSeedBudget&) = default;
};

// One family corresponds to one saddle role in the exact (order, level)
// journal.  Interior ids, shell ids and the critical center remain owned by
// the Phase-9 terminal facade and are not copied.
struct ExactDirectSaddleArmFamilyRecord {
  std::size_t family_index{};
  std::size_t source_event_index{};
  std::size_t journal_event_projection_index{};
  std::size_t journal_role_record_index{};
  std::size_t journal_batch_index{};
  std::size_t order{};
  exact::ExactLevel critical_squared_level{};
  std::size_t arm_seed_offset{};
  std::size_t arm_seed_count{};
  contract::CanonicalId source_event_arm_identity_digest{};

  friend bool operator==(
      const ExactDirectSaddleArmFamilyRecord&,
      const ExactDirectSaddleArmFamilyRecord&) = default;
};

// A seed is the factorized identity of F_u = (I union U) minus {u}.  The
// facet is intentionally reconstructed on demand in a fixed ten-id buffer.
// Equal facets produced by distinct events remain distinct seeds.
struct ExactDirectSaddleArmSeedRecord {
  std::size_t arm_seed_index{};
  std::size_t family_index{};
  spatial::PointId removed_support_point_id{};

  friend bool operator==(
      const ExactDirectSaddleArmSeedRecord&,
      const ExactDirectSaddleArmSeedRecord&) = default;
};

struct ExactDirectSaddleArmFacet {
  static constexpr std::size_t maximum_point_count = 10U;

  std::array<spatial::PointId, maximum_point_count> point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectSaddleArmFacet&,
      const ExactDirectSaddleArmFacet&) = default;
};

enum class ExactDirectSaddleArmSeedDecision : std::uint8_t {
  not_certified,
  no_seed_journal_capacity_overflow,
  no_seed_journal_budget_exhausted,
  no_seed_journal_source_not_certified,
  no_seed_journal_source_join_inconsistent,
  complete_certified_factorized_arm_seeds,
};

enum class ExactDirectSaddleArmSeedScope : std::uint8_t {
  unspecified,
  terminal_direct_saddle_deleted_facets_only,
};

struct ExactDirectSaddleArmSeedJournalResult {
  static constexpr std::string_view backend =
      direct_saddle_arm_seed_journal_backend;
  static constexpr std::string_view profile =
      direct_saddle_arm_seed_journal_profile;
  static constexpr std::string_view mode =
      direct_saddle_arm_seed_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_saddle_arm_seed_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_saddle_arm_seed_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_saddle_arm_seed_journal_proof_basis;

  std::uint32_t schema_version{
      direct_saddle_arm_seed_journal_schema_version};
  ExactDirectSaddleArmSeedBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t required_source_journal_replay_entry_count{};
  std::size_t required_role_scan_count{};
  std::size_t required_saddle_family_count{};
  std::size_t required_arm_seed_count{};
  std::size_t logical_added_storage_entry_count{};
  std::size_t logical_added_storage_entry_limit{};
  std::size_t combined_logical_storage_entry_count{};
  std::size_t combined_logical_storage_entry_limit{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  std::vector<ExactDirectSaddleArmFamilyRecord> families;
  std::vector<ExactDirectSaddleArmSeedRecord> arm_seeds;
  bool budget_preflight_certified{false};
  bool source_journal_freshly_replayed{false};
  bool source_facade_join_certified{false};
  bool every_saddle_role_projected_once{false};
  bool every_support_point_has_one_arm_seed{false};
  bool arm_seed_order_is_canonical{false};
  bool factorized_facets_reconstruct_exactly{false};
  bool source_relative_strict_miniball_drop_theorem_applies{false};
  bool output_linear_in_direct_events{false};
  bool no_forbidden_global_structure_materialized{false};
  bool facets_materialized_in_journal{false};
  bool miniballs_or_global_partitions_computed{false};
  bool hierarchy_reduction_performed{false};
  bool forest_or_gateway_attach_performed{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSaddleArmSeedDecision decision{
      ExactDirectSaddleArmSeedDecision::not_certified};
  ExactDirectSaddleArmSeedScope scope{
      ExactDirectSaddleArmSeedScope::unspecified};

  [[nodiscard]] bool certified_partial_refinement() const;

  friend bool operator==(
      const ExactDirectSaddleArmSeedJournalResult&,
      const ExactDirectSaddleArmSeedJournalResult&) = default;
};

// The Phase-9 facade is required because the Phase-10 event journal
// deliberately does not copy interior ids.  A complete result stores only
// one family per saddle and one seed per support point.
[[nodiscard]] ExactDirectSaddleArmSeedJournalResult
build_exact_direct_saddle_arm_seed_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& budget);

// Reconstructs F_u in sorted PointId order without a heap allocation.  This
// is an identity projection only: it computes no miniball, component, root or
// attachment.
[[nodiscard]] ExactDirectSaddleArmFacet
reconstruct_exact_direct_saddle_arm_facet(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal,
    std::size_t arm_seed_index);

struct ExactDirectSaddleArmSeedVerification {
  bool source_journal_certified{false};
  bool requirements_certified{false};
  bool family_records_certified{false};
  bool arm_seed_records_certified{false};
  bool factorized_facets_certified{false};
  bool result_facts_certified{false};
  bool decision_and_scope_certified{false};
  bool fresh_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSaddleArmSeedVerification&,
      const ExactDirectSaddleArmSeedVerification&) = default;
};

// Rebuilds the complete factorized journal from the external authorities.
// No field of the observed seed journal steers replay.
[[nodiscard]] ExactDirectSaddleArmSeedVerification
verify_exact_direct_saddle_arm_seed_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& budget,
    const ExactDirectSaddleArmSeedJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
