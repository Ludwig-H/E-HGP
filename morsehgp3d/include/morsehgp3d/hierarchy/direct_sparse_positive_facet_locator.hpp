#pragma once

#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_positive_facet_locator_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_mode = "certified";
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_positive_facet_locator_proof_basis =
        "caller_asserted_external_authority_relative_sparse_positive_full_"
        "facet_key_"
        "pre_batch_state_sequential_atomic_commit_v1";

inline constexpr std::size_t
    direct_sparse_positive_facet_maximum_point_count = 10U;

// Canonical facet identity.  The used PointIds are strictly increasing, the
// point count is in [1, 10], and every unused tail entry is zero.  The fixed
// input form makes validation allocation-free; committed keys are packed in
// the locator's durable flat PointId arena.
struct ExactDirectSparseFacetKey {
  std::array<
      spatial::PointId,
      direct_sparse_positive_facet_maximum_point_count>
      point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectSparseFacetKey&,
      const ExactDirectSparseFacetKey&) = default;
};

// The locator never establishes external geometric authority.  A non-zero
// replay token names one record in the caller-owned authority identified at
// locator construction.  The caller is responsible for retaining that
// authority and replaying the named record when auditing a positive result.
struct ExactDirectSparseFacetWitness {
  std::uint64_t external_authority_id{};
  std::uint64_t replay_token{};

  friend bool operator==(
      const ExactDirectSparseFacetWitness&,
      const ExactDirectSparseFacetWitness&) = default;
};

using ExactDirectSparseComponentHandle = std::size_t;

struct ExactDirectSparseFacetQuery {
  std::size_t query_index{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseFacetWitness witness{};

  friend bool operator==(
      const ExactDirectSparseFacetQuery&,
      const ExactDirectSparseFacetQuery&) = default;
};

struct ExactDirectSparseComponentUnion {
  std::size_t union_index{};
  ExactDirectSparseComponentHandle left_handle{};
  ExactDirectSparseComponentHandle right_handle{};
  ExactDirectSparseFacetWitness witness{};

  friend bool operator==(
      const ExactDirectSparseComponentUnion&,
      const ExactDirectSparseComponentUnion&) = default;
};

struct ExactDirectSparseFacetBinding {
  std::size_t binding_index{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness witness{};

  friend bool operator==(
      const ExactDirectSparseFacetBinding&,
      const ExactDirectSparseFacetBinding&) = default;
};

// All capacities are checked before constructing scratch or durable arenas.
// The table and batch-scratch slot caps must cover 2M+1 and 2B+1 slots,
// respectively, so linear probing always has an empty terminator below 1/2
// load.  A zero fingerprint mask is supported only to force test collisions;
// correctness always comes from the complete key comparison.
struct ExactDirectSparsePositiveFacetLocatorBudget {
  std::size_t maximum_component_handle_count{};
  std::size_t maximum_committed_binding_count{};
  std::size_t maximum_committed_key_point_count{};
  std::size_t maximum_committed_union_count{};
  std::size_t maximum_committed_batch_count{};
  std::size_t maximum_batch_query_count{};
  std::size_t maximum_batch_union_count{};
  std::size_t maximum_batch_binding_count{};
  std::size_t maximum_batch_key_point_count{};
  std::size_t maximum_table_slot_count{};
  std::size_t maximum_batch_scratch_slot_count{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetLocatorBudget&,
      const ExactDirectSparsePositiveFacetLocatorBudget&) = default;
};

struct ExactDirectSparsePositiveFacetLocatorConfig {
  std::uint64_t external_authority_id{};
  std::uint64_t fingerprint_mask{~std::uint64_t{0U}};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetLocatorConfig&,
      const ExactDirectSparsePositiveFacetLocatorConfig&) = default;
};

enum class ExactDirectSparseFacetLookupDisposition : std::uint8_t {
  unresolved,
  positive,
};

struct ExactDirectSparseFacetLookupResult {
  std::size_t query_index{};
  ExactDirectSparseFacetLookupDisposition disposition{
      ExactDirectSparseFacetLookupDisposition::unresolved};
  ExactDirectSparseComponentHandle pre_batch_component_handle{};
  ExactDirectSparseFacetWitness query_witness{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool component_handle_present{false};
  bool source_binding_witness_present{false};

  friend bool operator==(
      const ExactDirectSparseFacetLookupResult&,
      const ExactDirectSparseFacetLookupResult&) = default;
};

// Durable table slot.  component_handle is the original dense DSU handle and
// is intentionally never rewritten after a union.  Lookups resolve it through
// the current parent arena.  The key slice lives in the separate flat arena.
struct ExactDirectSparsePositiveFacetSlot {
  std::uint64_t fingerprint{};
  std::size_t committed_binding_index{};
  std::size_t key_point_offset{};
  std::size_t key_point_count{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness binding_witness{};
  bool occupied{false};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetSlot&,
      const ExactDirectSparsePositiveFacetSlot&) = default;
};

struct ExactDirectSparseCommittedUnionRecord {
  std::size_t committed_union_index{};
  ExactDirectSparseComponentHandle left_handle{};
  ExactDirectSparseComponentHandle right_handle{};
  ExactDirectSparseFacetWitness witness{};

  friend bool operator==(
      const ExactDirectSparseCommittedUnionRecord&,
      const ExactDirectSparseCommittedUnionRecord&) = default;
};

struct ExactDirectSparsePositiveFacetLocatorCounters {
  std::size_t committed_batch_count{};
  std::size_t query_count{};
  std::size_t positive_lookup_count{};
  std::size_t unresolved_lookup_count{};
  std::size_t union_request_count{};
  std::size_t binding_request_count{};
  std::size_t inserted_binding_count{};
  std::size_t compatible_duplicate_binding_count{};
  std::size_t committed_key_point_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetLocatorCounters&,
      const ExactDirectSparsePositiveFacetLocatorCounters&) = default;
};

enum class ExactDirectSparsePositiveFacetLocatorInitializationDecision
    : std::uint8_t {
  not_certified,
  no_locator_capacity_overflow,
  no_locator_budget_exhausted,
  no_locator_external_authority_rejected,
  complete_certified_empty_sparse_positive_locator,
};

enum class ExactDirectSparsePositiveFacetBatchDecision : std::uint8_t {
  not_certified,
  no_positive_locator_not_initialized,
  no_positive_locator_capacity_overflow,
  no_positive_locator_budget_exhausted,
  no_positive_locator_input_shape_rejected,
  no_positive_locator_external_witness_rejected,
  contradiction_incompatible_exact_facet_binding,
  complete_certified_sparse_positive_batch_commit,
};

enum class ExactDirectSparsePositiveFacetLocatorScope : std::uint8_t {
  unspecified,
  positive_bindings_relative_to_caller_asserted_external_authority_only,
};

struct ExactDirectSparsePositiveFacetBatchCounters {
  std::size_t query_count{};
  std::size_t positive_lookup_count{};
  std::size_t unresolved_lookup_count{};
  std::size_t union_request_count{};
  std::size_t binding_request_count{};
  std::size_t inserted_binding_count{};
  std::size_t compatible_duplicate_binding_count{};
  std::size_t batch_input_key_point_count{};
  std::size_t inserted_key_point_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetBatchCounters&,
      const ExactDirectSparsePositiveFacetBatchCounters&) = default;
};

// One small durable record per accepted transaction makes aggregate counters
// structurally checkable without retaining historical query payloads.  Its
// fact booleans are assertions of the original call, not freshly replayable
// facts.
struct ExactDirectSparseCommittedBatchRecord {
  std::size_t committed_batch_index{};
  ExactDirectSparsePositiveFacetBatchCounters counters{};
  bool input_shape_certified{false};
  bool input_witness_structure_certified{false};
  bool strict_pre_batch_snapshot_certified{false};
  bool sequential_atomic_commit_certified{false};

  friend bool operator==(
      const ExactDirectSparseCommittedBatchRecord&,
      const ExactDirectSparseCommittedBatchRecord&) = default;
};

struct ExactDirectSparsePositiveFacetBatchResult {
  std::uint32_t schema_version{
      direct_sparse_positive_facet_locator_schema_version};
  std::size_t candidate_batch_index{};
  std::vector<ExactDirectSparseFacetLookupResult> lookups;
  ExactDirectSparsePositiveFacetBatchCounters counters{};
  bool budget_preflight_certified{false};
  bool input_shape_certified{false};
  bool every_input_witness_non_null_and_authority_matched{false};
  bool lookups_use_strict_pre_batch_snapshot{false};
  bool current_batch_bindings_hidden_from_lookups{false};
  bool every_positive_lookup_has_non_null_external_witness_tokens{false};
  bool every_fingerprint_candidate_compared_by_full_key{false};
  bool explicit_unions_applied_before_binding_compatibility{false};
  bool exact_duplicate_bindings_compatible_after_explicit_unions{false};
  bool atomic_commit_performed{false};
  bool locator_state_mutated{false};
  bool contradiction_detected{false};
  bool missing_facet_means_isolated{false};
  bool total_facet_authority_claimed{false};
  ExactDirectSparsePositiveFacetBatchDecision decision{
      ExactDirectSparsePositiveFacetBatchDecision::not_certified};
  ExactDirectSparsePositiveFacetLocatorScope scope{
      ExactDirectSparsePositiveFacetLocatorScope::unspecified};

  [[nodiscard]] bool certified_committed_batch() const noexcept;

  friend bool operator==(
      const ExactDirectSparsePositiveFacetBatchResult&,
      const ExactDirectSparsePositiveFacetBatchResult&) = default;
};

// Non-owning observation used by the fresh verifier.  It can also describe a
// decoded durable image; verification does not trust the producer object.
struct ExactDirectSparsePositiveFacetLocatorStateView {
  std::uint32_t schema_version{};
  ExactDirectSparsePositiveFacetLocatorBudget budget{};
  ExactDirectSparsePositiveFacetLocatorConfig config{};
  std::size_t required_component_handle_capacity{};
  std::size_t required_table_slot_capacity{};
  std::size_t required_batch_scratch_slot_capacity{};
  std::span<const ExactDirectSparsePositiveFacetSlot> slots;
  std::span<const spatial::PointId> key_point_arena;
  std::span<const ExactDirectSparseComponentHandle> component_parents;
  std::span<const ExactDirectSparseCommittedUnionRecord> committed_unions;
  std::span<const ExactDirectSparseCommittedBatchRecord> committed_batches;
  ExactDirectSparsePositiveFacetLocatorCounters counters{};
  bool budget_preflight_certified{false};
  bool empty_table_initialized{false};
  bool dense_component_handles_initialized{false};
  bool flat_durable_key_arena_initialized{false};
  bool positive_bindings_only{false};
  bool full_key_comparison_required{false};
  bool missing_facet_means_isolated{false};
  bool total_facet_authority_claimed{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectSparsePositiveFacetLocatorInitializationDecision
      initialization_decision{
          ExactDirectSparsePositiveFacetLocatorInitializationDecision::
              not_certified};
  ExactDirectSparsePositiveFacetLocatorScope scope{
      ExactDirectSparsePositiveFacetLocatorScope::unspecified};
};

struct ExactDirectSparsePositiveFacetLocatorStructuralVerification {
  std::size_t table_slot_scan_count{};
  std::size_t key_point_scan_count{};
  std::size_t union_record_scan_count{};
  std::size_t batch_record_scan_count{};
  bool trusted_construction_parameters_certified{false};
  bool capacity_requirements_certified{false};
  bool flat_table_and_key_arena_certified{false};
  bool every_fingerprint_recomputed_and_full_key_located{false};
  bool dense_handle_dsu_replay_certified{false};
  bool union_witness_structure_certified{false};
  bool historical_batch_assertions_and_counters_well_formed{false};
  bool internal_fact_fields_match_contract{false};
  bool decision_and_scope_certified{false};
  bool external_authority_replayed_by_locator{false};
  bool bounded_temporary_scratch_without_second_durable_output{false};
  bool fresh_durable_structure_verification_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectSparsePositiveFacetLocatorStructuralVerification&,
      const ExactDirectSparsePositiveFacetLocatorStructuralVerification&) =
      default;
};

class ExactDirectSparsePositiveFacetLocator {
 public:
  static constexpr std::string_view backend =
      direct_sparse_positive_facet_locator_backend;
  static constexpr std::string_view profile =
      direct_sparse_positive_facet_locator_profile;
  static constexpr std::string_view mode =
      direct_sparse_positive_facet_locator_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_positive_facet_locator_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_positive_facet_locator_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_positive_facet_locator_proof_basis;

  [[nodiscard]] bool certified_positive_locator() const noexcept;

  [[nodiscard]] ExactDirectSparsePositiveFacetBatchResult apply_batch(
      std::span<const ExactDirectSparseFacetQuery> queries,
      std::span<const ExactDirectSparseComponentUnion> unions,
      std::span<const ExactDirectSparseFacetBinding> bindings);

  [[nodiscard]] const ExactDirectSparsePositiveFacetLocatorBudget& budget()
      const noexcept {
    return budget_;
  }
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocatorConfig& config()
      const noexcept {
    return config_;
  }
  [[nodiscard]] const std::vector<ExactDirectSparsePositiveFacetSlot>& slots()
      const noexcept {
    return slots_;
  }
  [[nodiscard]] const std::vector<spatial::PointId>& key_point_arena()
      const noexcept {
    return key_point_arena_;
  }
  [[nodiscard]] const std::vector<ExactDirectSparseComponentHandle>&
  component_parents() const noexcept {
    return component_parents_;
  }
  [[nodiscard]] const std::vector<ExactDirectSparseCommittedUnionRecord>&
  committed_unions() const noexcept {
    return committed_unions_;
  }
  [[nodiscard]] const std::vector<ExactDirectSparseCommittedBatchRecord>&
  committed_batches() const noexcept {
    return committed_batches_;
  }
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocatorCounters& counters()
      const noexcept {
    return counters_;
  }
  [[nodiscard]] std::size_t required_table_slot_capacity() const noexcept {
    return required_table_slot_capacity_;
  }
  [[nodiscard]] std::size_t required_batch_scratch_slot_capacity()
      const noexcept {
    return required_batch_scratch_slot_capacity_;
  }
  [[nodiscard]] ExactDirectSparsePositiveFacetLocatorInitializationDecision
  initialization_decision() const noexcept {
    return initialization_decision_;
  }
  [[nodiscard]] ExactDirectSparsePositiveFacetLocatorScope scope()
      const noexcept {
    return scope_;
  }

  [[nodiscard]] ExactDirectSparsePositiveFacetLocatorStateView state_view()
      const noexcept;

  friend bool operator==(
      const ExactDirectSparsePositiveFacetLocator&,
      const ExactDirectSparsePositiveFacetLocator&) = default;

 private:
  friend ExactDirectSparsePositiveFacetLocator
  build_exact_direct_sparse_positive_facet_locator(
      std::size_t,
      const ExactDirectSparsePositiveFacetLocatorBudget&,
      const ExactDirectSparsePositiveFacetLocatorConfig&);

  std::uint32_t schema_version_{
      direct_sparse_positive_facet_locator_schema_version};
  ExactDirectSparsePositiveFacetLocatorBudget budget_{};
  ExactDirectSparsePositiveFacetLocatorConfig config_{};
  std::size_t required_component_handle_capacity_{};
  std::size_t required_table_slot_capacity_{};
  std::size_t required_batch_scratch_slot_capacity_{};
  std::vector<ExactDirectSparsePositiveFacetSlot> slots_;
  std::vector<spatial::PointId> key_point_arena_;
  std::vector<ExactDirectSparseComponentHandle> component_parents_;
  std::vector<ExactDirectSparseCommittedUnionRecord> committed_unions_;
  std::vector<ExactDirectSparseCommittedBatchRecord> committed_batches_;
  ExactDirectSparsePositiveFacetLocatorCounters counters_{};
  bool budget_preflight_certified_{false};
  bool empty_table_initialized_{false};
  bool dense_component_handles_initialized_{false};
  bool flat_durable_key_arena_initialized_{false};
  bool positive_bindings_only_{false};
  bool full_key_comparison_required_{false};
  bool missing_facet_means_isolated_{false};
  bool total_facet_authority_claimed_{false};
  bool forbidden_global_structure_materialized_{false};
  bool public_status_claimed_{false};
  ExactDirectSparsePositiveFacetLocatorInitializationDecision
      initialization_decision_{
          ExactDirectSparsePositiveFacetLocatorInitializationDecision::
              not_certified};
  ExactDirectSparsePositiveFacetLocatorScope scope_{
      ExactDirectSparsePositiveFacetLocatorScope::unspecified};
};

// Builds one empty, capacity-bounded locator.  Handles are exactly the dense
// interval [0, component_handle_count); no facet binding is inferred.
[[nodiscard]] ExactDirectSparsePositiveFacetLocator
build_exact_direct_sparse_positive_facet_locator(
    std::size_t component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& config);

// Replays only the locator's internal durable contract.  Witness identifiers
// and tokens are checked structurally, but the caller-owned external authority
// is deliberately not replayed by this kernel.
// Uses O(committed bindings + committed key points + dense handles) temporary
// scratch.  It freshly verifies the durable structure, not historical query
// payloads, and therefore reports stored batch assertions only as well formed.
[[nodiscard]] ExactDirectSparsePositiveFacetLocatorStructuralVerification
verify_exact_direct_sparse_positive_facet_locator_structure(
    std::size_t trusted_component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& trusted_budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& trusted_config,
    const ExactDirectSparsePositiveFacetLocatorStateView& observed);

}  // namespace morsehgp3d::hierarchy
