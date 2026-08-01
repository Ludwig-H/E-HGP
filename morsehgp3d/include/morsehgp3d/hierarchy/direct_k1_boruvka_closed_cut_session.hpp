#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/boruvka.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_k1_boruvka_closed_cut_session_schema_version = 1U;
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_session_backend = "reference_cpu";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_session_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_session_mode =
        "certified_exact_boruvka_monotone_closed_cut_sealed";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_session_public_status = "not_claimed";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_session_proof_basis =
        "fresh_exact_lbvh_boruvka_replay_compact_equal_level_k1_forest_"
        "monotone_closed_prefix_private_session_capability_v1";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_restore_mode =
        "certified_semantic_restore_by_fresh_exact_replay_no_file_codec";
inline constexpr std::string_view
    direct_k1_boruvka_closed_cut_restore_proof_basis =
        "allocation_free_preflight_fresh_exact_lbvh_boruvka_replay_"
        "fresh_closed_cut_prefix_replay_full_semantic_checkpoint_equality_"
        "process_local_capability_remint_v1";

// The conservative structural requirements are derived from point_count
// before the fresh Boruvka replay or any session-owned arena is allocated.
// The producer/verifier may use transient O(n) work after that gate; it never
// materializes the complete graph, Gamma, a Delaunay mosaic, cells or cofaces.
struct ExactDirectK1BoruvkaClosedCutSessionBudget {
  std::size_t maximum_point_count{};
  std::size_t maximum_theoretical_boruvka_round_count{};
  std::size_t maximum_source_edge_count{};
  std::size_t maximum_distinct_level_count{};
  std::size_t maximum_merge_node_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_parent_state_entry_count{};
  std::size_t maximum_component_size_state_entry_count{};
  std::size_t maximum_component_minimum_state_entry_count{};
  std::size_t maximum_component_root_state_entry_count{};
  std::size_t maximum_history_digest_count{};
  std::size_t maximum_checkpoint_state_entry_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutSessionBudget&,
      const ExactDirectK1BoruvkaClosedCutSessionBudget&) = default;
};

struct ExactDirectK1BoruvkaClosedCutSessionRequirements {
  std::size_t point_count{};
  std::size_t theoretical_boruvka_round_count{};
  std::size_t source_edge_count{};
  std::size_t distinct_level_count_upper_bound{};
  std::size_t merge_node_count_upper_bound{};
  std::size_t child_reference_count_upper_bound{};
  std::size_t parent_state_entry_count{};
  std::size_t component_size_state_entry_count{};
  std::size_t component_minimum_state_entry_count{};
  std::size_t component_root_state_entry_count{};
  std::size_t history_digest_count_upper_bound{};
  std::size_t checkpoint_state_entry_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutSessionRequirements&,
      const ExactDirectK1BoruvkaClosedCutSessionRequirements&) = default;
};

struct ExactDirectK1BoruvkaClosedCutSessionStamp {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  std::uint64_t session_instance_id{};
  std::size_t committed_level_cursor{};
  exact::ExactLevel closed_squared_level{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId committed_history_digest{};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutSessionStamp&,
      const ExactDirectK1BoruvkaClosedCutSessionStamp&) = default;
};

enum class ExactDirectK1BoruvkaClosedCutInitializationDecision
    : std::uint8_t {
  not_certified,
  no_preflight_capacity_overflow,
  no_preflight_budget_exhausted,
  no_boruvka_authority_rejected,
  no_compact_k1_forest_rejected,
  no_allocation_failed,
  no_session_instance_id_exhausted,
  complete_certified_sealed_session,
};

enum class ExactDirectK1BoruvkaClosedCutAdvanceDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_nonmonotone_closed_cut,
  no_exact_level_bit_budget_exhausted,
  no_exact_comparison_failed,
  no_internal_source_contradiction,
  complete_certified_monotone_closed_cut,
};

enum class ExactDirectK1BoruvkaClosedCutRootQueryDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_stale_or_foreign_stamp,
  no_singleton_out_of_range,
  complete_certified_singleton_closed_root,
};

enum class ExactDirectK1BoruvkaClosedCutCheckpointDecision : std::uint8_t {
  not_certified,
  no_session_not_ready,
  no_checkpoint_budget_exhausted,
  no_allocation_failed,
  complete_checkpoint_ready_state,
};

enum class ExactDirectK1BoruvkaClosedCutRestoreDecision : std::uint8_t {
  not_certified,
  no_checkpoint_shape_rejected,
  no_preflight_capacity_overflow,
  no_preflight_budget_exhausted,
  no_boruvka_authority_rejected,
  no_compact_k1_forest_rejected,
  no_allocation_failed,
  no_session_instance_id_exhausted,
  no_session_instance_id_not_distinct,
  no_closed_cut_prefix_replay_rejected,
  no_expected_checkpoint_export_rejected,
  no_semantic_checkpoint_mismatch,
  complete_certified_fresh_semantic_replay_restore,
};

struct ExactDirectK1BoruvkaClosedCutAdvanceReceipt {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  ExactDirectK1BoruvkaClosedCutSessionStamp pre_stamp{};
  ExactDirectK1BoruvkaClosedCutSessionStamp post_stamp{};
  std::size_t consumed_intermediate_level_count{};
  std::size_t consumed_edge_count{};
  std::size_t consumed_merge_node_count{};
  bool state_mutated{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1BoruvkaClosedCutAdvanceDecision decision{
      ExactDirectK1BoruvkaClosedCutAdvanceDecision::not_certified};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutAdvanceReceipt&,
      const ExactDirectK1BoruvkaClosedCutAdvanceReceipt&) = default;
};

struct ExactDirectK1BoruvkaClosedCutRootQueryResult {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  ExactDirectK1BoruvkaClosedCutSessionStamp stamp{};
  spatial::PointId singleton_point_id{};
  std::optional<K1NodeId> closed_root_node_id;
  std::size_t parent_hop_count{};
  ExactDirectK1BoruvkaClosedCutRootQueryDecision decision{
      ExactDirectK1BoruvkaClosedCutRootQueryDecision::not_certified};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutRootQueryResult&,
      const ExactDirectK1BoruvkaClosedCutRootQueryResult&) = default;
};

// A checkpoint stores the semantic partition, not the implementation's DSU
// topology.  canonical_component_minimum_by_point and
// closed_root_node_id_by_point therefore permit deterministic semantic
// recertification without persisting path-compression artifacts.  The public
// record never carries authority: restore freshly replays exact LBVH Boruvka,
// replays the committed closed prefix, exports the expected checkpoint and
// requires full semantic equality.  This is a semantic restore entry point;
// serialization and a durable file codec remain outside this component.
struct ExactDirectK1BoruvkaClosedCutCheckpoint {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  ExactDirectK1BoruvkaClosedCutSessionStamp stamp{};
  contract::CanonicalId source_forest_digest{};
  std::vector<spatial::PointId> canonical_component_minimum_by_point;
  std::vector<K1NodeId> closed_root_node_id_by_point;
  contract::CanonicalId state_digest{};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutCheckpoint&,
      const ExactDirectK1BoruvkaClosedCutCheckpoint&) = default;
};

struct ExactDirectK1BoruvkaClosedCutCheckpointExportResult {
  ExactDirectK1BoruvkaClosedCutCheckpoint checkpoint{};
  ExactDirectK1BoruvkaClosedCutCheckpointDecision decision{
      ExactDirectK1BoruvkaClosedCutCheckpointDecision::not_certified};
};

struct ExactDirectK1BoruvkaClosedCutRestoreReceipt {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  ExactDirectK1BoruvkaClosedCutSessionStamp checkpoint_stamp{};
  ExactDirectK1BoruvkaClosedCutSessionStamp restored_stamp{};
  contract::CanonicalId checkpoint_source_forest_digest{};
  contract::CanonicalId restored_source_forest_digest{};
  contract::CanonicalId checkpoint_state_digest{};
  contract::CanonicalId restored_state_digest{};
  std::size_t replayed_boruvka_round_count{};
  std::size_t replayed_boruvka_component_minimum_count{};
  std::size_t replayed_closed_cut_level_count{};
  std::size_t replayed_closed_cut_edge_count{};
  std::size_t replayed_closed_cut_merge_node_count{};
  bool allocation_free_structural_preflight_certified{false};
  bool boruvka_authority_freshly_replayed{false};
  bool compact_k1_equal_level_forest_certified{false};
  bool closed_cut_prefix_freshly_replayed{false};
  bool expected_checkpoint_freshly_exported{false};
  bool semantic_checkpoint_equality_certified{false};
  bool session_instance_ids_distinct{false};
  bool scientific_digests_identical{false};
  bool observed_boruvka_authority_mutated{false};
  bool durable_checkpoint_file_codec_claimed{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1BoruvkaClosedCutInitializationDecision
      initialization_decision{
          ExactDirectK1BoruvkaClosedCutInitializationDecision::
              not_certified};
  ExactDirectK1BoruvkaClosedCutAdvanceDecision advance_decision{
      ExactDirectK1BoruvkaClosedCutAdvanceDecision::not_certified};
  ExactDirectK1BoruvkaClosedCutCheckpointDecision checkpoint_decision{
      ExactDirectK1BoruvkaClosedCutCheckpointDecision::not_certified};
  ExactDirectK1BoruvkaClosedCutRestoreDecision decision{
      ExactDirectK1BoruvkaClosedCutRestoreDecision::not_certified};

  friend bool operator==(
      const ExactDirectK1BoruvkaClosedCutRestoreReceipt&,
      const ExactDirectK1BoruvkaClosedCutRestoreReceipt&) = default;
};

class ExactDirectK1BoruvkaClosedCutSession;
struct ExactDirectK1BoruvkaClosedCutSessionInitialization;

// Scientific authority is the live object, not a public receipt boolean.
// Construction is private, the object is move-only, and every stamp/root/
// checkpoint is accepted only through replay against this sealed source.
class ExactDirectK1BoruvkaClosedCutSession {
 public:
  struct Impl;

  static constexpr std::string_view backend =
      direct_k1_boruvka_closed_cut_session_backend;
  static constexpr std::string_view profile =
      direct_k1_boruvka_closed_cut_session_profile;
  static constexpr std::string_view mode =
      direct_k1_boruvka_closed_cut_session_mode;
  static constexpr std::string_view public_status =
      direct_k1_boruvka_closed_cut_session_public_status;
  static constexpr std::string_view proof_basis =
      direct_k1_boruvka_closed_cut_session_proof_basis;

  ExactDirectK1BoruvkaClosedCutSession() noexcept;
  ~ExactDirectK1BoruvkaClosedCutSession();
  ExactDirectK1BoruvkaClosedCutSession(
      ExactDirectK1BoruvkaClosedCutSession&&) noexcept;
  ExactDirectK1BoruvkaClosedCutSession& operator=(
      ExactDirectK1BoruvkaClosedCutSession&&) noexcept;
  ExactDirectK1BoruvkaClosedCutSession(
      const ExactDirectK1BoruvkaClosedCutSession&) = delete;
  ExactDirectK1BoruvkaClosedCutSession& operator=(
      const ExactDirectK1BoruvkaClosedCutSession&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionStamp current_stamp()
      const;

  [[nodiscard]] ExactDirectK1BoruvkaClosedCutAdvanceReceipt
  advance_closed_to(const exact::ExactLevel& closed_squared_level) &;

  [[nodiscard]] ExactDirectK1BoruvkaClosedCutRootQueryResult
  query_singleton_root(
      spatial::PointId singleton_point_id,
      const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const noexcept;

  [[nodiscard]] bool verify_stamp(
      const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const noexcept;
  [[nodiscard]] bool verify_advance_receipt(
      const ExactDirectK1BoruvkaClosedCutAdvanceReceipt& receipt) const;
  [[nodiscard]] bool verify_root_query_result(
      const ExactDirectK1BoruvkaClosedCutRootQueryResult& result) const
      noexcept;

  [[nodiscard]] ExactDirectK1BoruvkaClosedCutCheckpointExportResult
  export_checkpoint() const;
  [[nodiscard]] bool verify_checkpoint(
      const ExactDirectK1BoruvkaClosedCutCheckpoint& checkpoint) const;

 private:
  explicit ExactDirectK1BoruvkaClosedCutSession(
      std::unique_ptr<Impl> impl) noexcept;

  friend struct ExactDirectK1BoruvkaClosedCutSessionInitialization;
  friend ExactDirectK1BoruvkaClosedCutSessionInitialization
  build_exact_direct_k1_boruvka_closed_cut_session(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const K1ExactBoruvkaResult&,
      const ExactDirectK1BoruvkaClosedCutSessionBudget&);

  std::unique_ptr<Impl> impl_;
};

static_assert(
    !std::is_copy_constructible_v<ExactDirectK1BoruvkaClosedCutSession>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactDirectK1BoruvkaClosedCutSession>);

struct ExactDirectK1BoruvkaClosedCutSessionInitialization {
  std::uint32_t schema_version{
      direct_k1_boruvka_closed_cut_session_schema_version};
  ExactDirectK1BoruvkaClosedCutSessionBudget requested_budget{};
  ExactDirectK1BoruvkaClosedCutSessionRequirements requirements{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_forest_digest{};
  std::size_t replayed_boruvka_round_count{};
  std::size_t replayed_boruvka_component_minimum_count{};
  bool allocation_free_structural_preflight_certified{false};
  bool boruvka_authority_freshly_replayed{false};
  bool compact_k1_equal_level_forest_certified{false};
  bool checkpoint_semantic_state_ready{false};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectK1BoruvkaClosedCutInitializationDecision decision{
      ExactDirectK1BoruvkaClosedCutInitializationDecision::not_certified};
  ExactDirectK1BoruvkaClosedCutSession session{};

  [[nodiscard]] bool certified_ready_session() const noexcept;
};

struct ExactDirectK1BoruvkaClosedCutSessionRestore {
  ExactDirectK1BoruvkaClosedCutRestoreReceipt receipt{};
  ExactDirectK1BoruvkaClosedCutSession session{};

  [[nodiscard]] bool certified_restored_session() const noexcept;
};

// The observed public Boruvka record never carries authority by itself.  The
// factory first derives conservative sizes from the immutable cloud, then
// freshly replays the exact LBVH Boruvka producer against the same cloud and
// index before issuing the private session capability.
[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionInitialization
build_exact_direct_k1_boruvka_closed_cut_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& budget);

// This restores semantic state, not operational authority.  The checkpoint is
// read-only input and is never trusted: the same cloud/index/observed Boruvka
// tuple must mint a fresh sealed session, replay exactly the checkpoint's
// closed cut and reproduce every semantic checkpoint field.  Only the old
// process-local session_instance_id is excluded from semantic equality, and a
// distinct new instance id is required before success.  No file codec is
// provided here.
[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionRestore
restore_exact_direct_k1_boruvka_closed_cut_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1BoruvkaClosedCutCheckpoint& checkpoint,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& budget);

}  // namespace morsehgp3d::hierarchy
