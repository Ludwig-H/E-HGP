#pragma once

#include "morsehgp3d/hierarchy/direct_k1_boruvka_closed_cut_session.hpp"
#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_resident_k2_k1_closed_cut_bridge_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_resident_k2_k1_closed_cut_bridge_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_resident_k2_k1_closed_cut_bridge_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_resident_k2_k1_closed_cut_bridge_mode =
        "certified_conditional_resident_k2_to_sealed_k1_closed_cut_stream_v1";
inline constexpr std::string_view
    direct_morse_resident_k2_k1_closed_cut_bridge_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_resident_k2_k1_closed_cut_bridge_proof_basis =
        "private_move_only_bridge_and_resident_tickets_live_sealed_k1_"
        "monotone_closed_cut_replay_exhaustive_prior_root_latent_carrier_"
        "and_exact_delta_group_images_live_singleton_root_queries_"
        "preallocated_post_resident_commit_publication_v1";

// The bridge never materializes a point-pair matrix, Star, Gamma, cells,
// cofaces or a Delaunay mosaic.  One K2 preparation retains only a flat
// PointId scratch arena and one compact record per frozen quotient group.
// The persistent arena retains one batch record per committed K2 batch.
struct ExactDirectMorseResidentK2K1ClosedCutBridgeBudget {
  std::size_t maximum_committed_k2_batch_count{};
  std::size_t maximum_committed_k2_group_count{};
  std::size_t maximum_prepared_k2_group_count{};
  std::size_t maximum_group_coverage_point_reference_scan_count{};
  std::size_t maximum_group_point_scratch_count{};
  std::size_t maximum_distinct_group_point_count{};
  std::size_t maximum_singleton_root_query_count{};

  friend bool operator==(
      const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget&,
      const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget&) = default;
};

struct ExactDirectMorseResidentK2K1ClosedCutBridgeRequirements {
  std::size_t resident_batch_count{};
  std::size_t resident_k2_batch_count{};
  std::size_t prepared_group_count{};
  std::size_t group_coverage_point_reference_scan_count{};
  std::size_t distinct_group_point_count{};
  std::size_t singleton_root_query_count{};

  friend bool operator==(
      const ExactDirectMorseResidentK2K1ClosedCutBridgeRequirements&,
      const ExactDirectMorseResidentK2K1ClosedCutBridgeRequirements&) =
      default;
};

struct ExactDirectMorseResidentK2K1ClosedCutBridgeStamp {
  std::uint32_t schema_version{
      direct_morse_resident_k2_k1_closed_cut_bridge_schema_version};
  std::uint64_t bridge_session_instance_id{};
  std::uint64_t resident_session_authority_id{};
  std::uint64_t resident_locator_instance_id{};
  std::size_t resident_batch_cursor{};
  std::size_t resident_epoch{};
  std::size_t committed_k2_batch_count{};
  std::size_t committed_k2_group_count{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId resident_higher_canonical_cloud_digest{};
  ExactDirectK1BoruvkaClosedCutSessionStamp committed_k1_stamp{};

  friend bool operator==(
      const ExactDirectMorseResidentK2K1ClosedCutBridgeStamp&,
      const ExactDirectMorseResidentK2K1ClosedCutBridgeStamp&) = default;
};

struct ExactDirectMorseResidentK2K1ClosedCutGroupImage {
  std::size_t owner_group_index{};
  std::size_t exhaustive_distinct_point_count{};
  spatial::PointId canonical_representative_point_id{};
  K1NodeId closed_k1_root_node_id{};
  ExactFrozenIncidencePriorRootId resident_resultant_root_id{};
  std::size_t singleton_root_query_count{};
  bool prior_root_coverage_csr_consumed{false};
  bool latent_carrier_coverage_csr_consumed{false};
  bool exact_coverage_delta_consumed{false};
  bool exhaustive_group_image_certified{false};
  bool every_singleton_query_live_verified{false};
  bool unique_closed_k1_root_certified{false};

  [[nodiscard]] bool certified_conditional_group_image() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentK2K1ClosedCutGroupImage&,
      const ExactDirectMorseResidentK2K1ClosedCutGroupImage&) = default;
};

struct ExactDirectMorseResidentK2K1ClosedCutBatchRecord {
  std::uint32_t schema_version{
      direct_morse_resident_k2_k1_closed_cut_bridge_schema_version};
  ExactDirectMorseUnifiedSnapshotIdentity resident_pre_batch_identity{};
  std::size_t source_batch_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  ExactDirectK1BoruvkaClosedCutSessionStamp published_pre_k1_stamp{};
  ExactDirectK1BoruvkaClosedCutSessionStamp live_post_k1_stamp{};
  std::size_t consumed_intermediate_k1_level_count{};
  std::size_t group_coverage_point_reference_scan_count{};
  std::size_t distinct_group_point_count{};
  std::size_t singleton_root_query_count{};
  std::vector<ExactDirectMorseResidentK2K1ClosedCutGroupImage> group_images;
  bool k1_cut_advanced_before_resident_commit{false};
  bool every_intermediate_k1_level_consumed{false};
  bool group_images_exhaustive_from_frozen_csr{false};
  bool every_group_has_one_live_closed_k1_root{false};
  bool resident_batch_committed{false};
  bool post_commit_publication_allocation_free{false};
  bool incidence_complete_reduction{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_conditional_k2_to_k1_batch() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentK2K1ClosedCutBatchRecord&,
      const ExactDirectMorseResidentK2K1ClosedCutBatchRecord&) = default;
};

enum class ExactDirectMorseResidentK2K1ClosedCutInitializationDecision
    : std::uint8_t {
  not_certified,
  no_resident_session_rejected,
  no_k1_session_rejected,
  no_cloud_or_point_namespace_mismatch,
  no_budget_rejected,
  no_session_instance_id_exhausted,
  no_allocation_failed,
  complete_certified_bounded_conditional_bridge,
};

enum class ExactDirectMorseResidentK2K1ClosedCutPreparationDecision
    : std::uint8_t {
  not_certified,
  no_bridge_not_ready,
  no_resident_preparation_rejected,
  no_resident_pre_batch_identity_rejected,
  no_preparation_budget_exhausted,
  no_frozen_k2_group_image_rejected,
  no_k1_cut_advance_rejected,
  no_k1_group_root_query_rejected,
  no_k1_group_image_inconsistent,
  no_allocation_failed,
  complete_prepared_non_k2_transit_batch,
  complete_prepared_conditional_k2_to_k1_batch,
};

enum class ExactDirectMorseResidentK2K1ClosedCutCommitDecision
    : std::uint8_t {
  not_certified,
  no_ticket_already_consumed,
  no_foreign_ticket_rejected,
  no_resident_commit_rejected_without_vertical_mutation,
  complete_committed_non_k2_transit_batch,
  complete_committed_conditional_k2_to_k1_batch,
};

class ExactDirectMorseResidentK2K1ClosedCutPreparedBatch {
 public:
  ExactDirectMorseResidentK2K1ClosedCutPreparedBatch() noexcept;
  ~ExactDirectMorseResidentK2K1ClosedCutPreparedBatch();
  ExactDirectMorseResidentK2K1ClosedCutPreparedBatch(
      ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&&) noexcept;
  ExactDirectMorseResidentK2K1ClosedCutPreparedBatch& operator=(
      ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&&) noexcept;
  ExactDirectMorseResidentK2K1ClosedCutPreparedBatch(
      const ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&) = delete;
  ExactDirectMorseResidentK2K1ClosedCutPreparedBatch& operator=(
      const ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool k2_vertical_batch() const noexcept;
  [[nodiscard]] const ExactDirectMorseUnifiedSnapshotIdentity&
  resident_pre_batch_identity() const noexcept;
  // The returned resident bundle is the authentic bundle owned by the
  // private resident ticket.  The reference is valid only while this ticket
  // remains alive and before it is moved into commit.
  [[nodiscard]] const ExactDirectMorseUnifiedResidentAuthorityBundle&
  resident_authority_bundle() const noexcept;
  [[nodiscard]] const ExactDirectMorseResidentK2K1ClosedCutBatchRecord*
  conditional_batch_record() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseResidentK2K1ClosedCutPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseResidentK2K1ClosedCutBridge;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutPreparedBatch>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutPreparedBatch>);

struct ExactDirectMorseResidentK2K1ClosedCutPreparationResult {
  std::optional<ExactDirectMorseResidentK2K1ClosedCutPreparedBatch> ticket;
  ExactDirectMorseResidentK2K1ClosedCutBridgeRequirements requirements{};
  // Preparation never mutates the resident or publishes a vertical record.
  // The live K1 cut is a monotone target-authority cache and may advance;
  // k1_lower_cut_may_have_advanced_without_vertical_publication reports that
  // separately, so this flag must not be read as "all owned state unchanged".
  bool no_resident_or_published_vertical_state_mutated_on_failure{false};
  bool k1_lower_cut_may_have_advanced_without_vertical_publication{false};
  bool non_k2_transit_only{false};
  bool conditional_k2_to_k1_group_images_prepared{false};
  ExactDirectMorseResidentK2K1ClosedCutPreparationDecision decision{
      ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::not_certified};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectMorseResidentK2K1ClosedCutCommitResult {
  ExactDirectMorseUnifiedResidentCommitResult resident_commit{};
  std::size_t committed_resident_batch_cursor{};
  std::size_t committed_k2_batch_count{};
  std::size_t committed_k2_group_count{};
  bool ticket_consumed{false};
  bool vertical_state_mutated{false};
  bool no_vertical_state_mutated_on_resident_rejection{false};
  bool post_resident_commit_publication_allocation_free{false};
  ExactDirectMorseResidentK2K1ClosedCutCommitDecision decision{
      ExactDirectMorseResidentK2K1ClosedCutCommitDecision::not_certified};

  [[nodiscard]] bool certified_committed_batch() const noexcept;
};

class ExactDirectMorseResidentK2K1ClosedCutBridge;
struct ExactDirectMorseResidentK2K1ClosedCutInitialization;

// The bridge owns both mutable sessions and is deliberately single-threaded.
// A prepared ticket and its parent bridge must be externally serialized.
// Public stamps and records are audit payloads only; commit authority is the
// private shared seal inside the move-only prepared ticket.
class ExactDirectMorseResidentK2K1ClosedCutBridge {
 public:
  struct Impl;

  static constexpr std::string_view backend =
      direct_morse_resident_k2_k1_closed_cut_bridge_backend;
  static constexpr std::string_view profile =
      direct_morse_resident_k2_k1_closed_cut_bridge_profile;
  static constexpr std::string_view mode =
      direct_morse_resident_k2_k1_closed_cut_bridge_mode;
  static constexpr std::string_view public_status =
      direct_morse_resident_k2_k1_closed_cut_bridge_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_resident_k2_k1_closed_cut_bridge_proof_basis;

  ExactDirectMorseResidentK2K1ClosedCutBridge() noexcept;
  ~ExactDirectMorseResidentK2K1ClosedCutBridge();
  ExactDirectMorseResidentK2K1ClosedCutBridge(
      ExactDirectMorseResidentK2K1ClosedCutBridge&&) noexcept;
  ExactDirectMorseResidentK2K1ClosedCutBridge& operator=(
      ExactDirectMorseResidentK2K1ClosedCutBridge&&) noexcept;
  ExactDirectMorseResidentK2K1ClosedCutBridge(
      const ExactDirectMorseResidentK2K1ClosedCutBridge&) = delete;
  ExactDirectMorseResidentK2K1ClosedCutBridge& operator=(
      const ExactDirectMorseResidentK2K1ClosedCutBridge&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool resident_complete() const noexcept;
  [[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutBridgeStamp
  current_stamp() const;
  [[nodiscard]] bool verify_stamp(
      const ExactDirectMorseResidentK2K1ClosedCutBridgeStamp&) const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseResidentK2K1ClosedCutBatchRecord>&
  committed_k2_batches() const noexcept;
  // These forwarded resident views follow the resident session lifetime
  // contract: do not retain them across prepare, commit, move or destruction.
  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult& resident_plan()
      const noexcept;
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& resident_locator()
      const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseUnifiedResidentComponentState>&
  resident_component_states() const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseUnifiedResidentRootCoverage>&
  resident_root_coverages() const noexcept;
  [[nodiscard]] const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
  resident_group_records() const noexcept;

  [[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutPreparationResult
  prepare_next();
  [[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutCommitResult commit(
      ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&&) noexcept;

 private:
  explicit ExactDirectMorseResidentK2K1ClosedCutBridge(
      std::unique_ptr<Impl>) noexcept;

  friend struct ExactDirectMorseResidentK2K1ClosedCutInitialization;
  friend ExactDirectMorseResidentK2K1ClosedCutInitialization
  initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      ExactDirectMorseUnifiedResidentSession&&,
      ExactDirectK1BoruvkaClosedCutSession&&,
      const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget&);

  std::unique_ptr<Impl> impl_;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutBridge>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutBridge>);

struct ExactDirectMorseResidentK2K1ClosedCutInitialization {
  std::uint32_t schema_version{
      direct_morse_resident_k2_k1_closed_cut_bridge_schema_version};
  ExactDirectMorseResidentK2K1ClosedCutBridgeBudget requested_budget{};
  ExactDirectMorseResidentK2K1ClosedCutBridgeRequirements requirements{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId resident_higher_canonical_cloud_digest{};
  bool resident_live_session_consumed{false};
  bool k1_live_session_consumed{false};
  bool process_local_bridge_capability_issued{false};
  bool cloud_and_point_namespace_bound{false};
  bool persistent_k2_batch_arena_preallocated{false};
  bool incidence_complete_reduction{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentK2K1ClosedCutInitializationDecision decision{
      ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
          not_certified};
  ExactDirectMorseResidentK2K1ClosedCutBridge bridge{};

  [[nodiscard]] bool certified_ready_bridge() const noexcept;
};

[[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutInitialization
initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
    ExactDirectMorseUnifiedResidentSession&& resident_session,
    ExactDirectK1BoruvkaClosedCutSession&& k1_session,
    const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget& budget);

}  // namespace morsehgp3d::hierarchy
