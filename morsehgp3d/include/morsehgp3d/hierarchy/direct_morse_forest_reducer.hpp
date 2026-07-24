#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_forest_reducer_schema_version =
    1U;
inline constexpr std::string_view direct_morse_forest_reducer_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_reducer_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_reducer_mode =
    "budgeted";
inline constexpr std::string_view
    direct_morse_forest_reducer_deployment_status = "architecture_only";
inline constexpr std::string_view direct_morse_forest_reducer_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_forest_reducer_proof_basis =
    "strict_batch_stream_frozen_r_or_l_carrier_hypergraph_complete_"
    "transitive_quotient_qr_then_atomic_locator_and_scientific_commit_v1";

// This is the complete scientific projection consumed by one reducer fold.
// source_chunk_index is operational provenance only: it is deliberately
// excluded from every hierarchy identity and result record.
struct ExactDirectMorseForestReducerBatch {
  std::size_t source_batch_index{};
  std::optional<std::size_t> source_chunk_index;
  std::size_t source_family_begin_index{};
  std::size_t source_family_end_index{};
  std::size_t source_arm_seed_begin_index{};
  std::size_t source_arm_seed_end_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  ExactDirectSparseFacetWitness locator_query_witness{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      strict_pre_batch_locator_stamp{};
  ExactDirectSparseFacetDescentClosureBudget requested_closure_budget{};
  std::size_t transient_closure_node_count{};
  std::size_t transient_closure_step_call_count{};
  std::size_t shared_closure_build_count{};
  // Synchronous non-owning views.  Their producer must remain alive and
  // immutable until fold() returns; the reducer never retains either span.
  std::span<const ExactDirectSparseFacetDescentBatchResolvedKey> resolved_keys;
  std::span<const ExactDirectSparseFacetDescentBatchArmJoin> arm_joins;
};

// Projects non-owning views of the compact 14D scientific delta.  Transient
// closure nodes, proposal records and execution-session state never enter the
// reducer.  source must remain alive and immutable until fold() returns.
// Throws std::invalid_argument unless source is a complete, non-mutating 14D
// architecture execution.
[[nodiscard]] ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult& source);
ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    ExactDirectSparseFacetDescentBatchExecutionResult&& source) = delete;
ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult&& source) =
    delete;

enum class ExactDirectMorseForestReducerFoldDecision : std::uint8_t {
  not_folded,
  no_reducer_finished,
  no_reducer_batch_out_of_order,
  no_reducer_batch_inconsistent,
  no_reducer_budget_exhausted,
  no_reducer_operational_allocation_failed,
  no_reducer_frozen_carrier_quotient_rejected,
  no_reducer_locator_commit_rejected,
  complete_reducer_batch_commit,
};

struct ExactDirectMorseForestReducerFoldResult {
  std::uint32_t schema_version{direct_morse_forest_reducer_schema_version};
  std::size_t source_batch_index{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp pre_fold_locator_stamp{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp post_fold_locator_stamp{};
  bool complete_batch_staged_before_mutation{false};
  bool full_equal_level_quotient_resolved_before_mutation{false};
  bool locator_committed_before_scientific_state{false};
  bool scientific_state_committed{false};
  bool reducer_state_mutated{false};
  bool source_chunk_index_has_scientific_authority{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestReducerFoldDecision decision{
      ExactDirectMorseForestReducerFoldDecision::not_folded};

  [[nodiscard]] bool certified_committed_batch() const noexcept;
  [[nodiscard]] bool certified_atomic_rejection() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestReducerFoldResult&,
      const ExactDirectMorseForestReducerFoldResult&) = default;
};

// Incremental Phase-15C reduction.  The source authorities must outlive the
// reducer.  Its persistent scientific state is one sparse positive locator,
// one dense carrier DSU with a canonical-minimum attribute, per-order scalar
// counts and the final forest output arenas.  It retains no input batch
// deltas, closure graph, cells, cofaces, Gamma structure or Delaunay mosaic.
class ExactDirectMorseForestReducer {
 public:
  ExactDirectMorseForestReducer(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first);
  ~ExactDirectMorseForestReducer();

  ExactDirectMorseForestReducer(
      const ExactDirectMorseForestReducer&) = delete;
  ExactDirectMorseForestReducer& operator=(
      const ExactDirectMorseForestReducer&) = delete;
  ExactDirectMorseForestReducer(
      ExactDirectMorseForestReducer&&) noexcept;
  ExactDirectMorseForestReducer& operator=(
      ExactDirectMorseForestReducer&&) noexcept;

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold(
      const ExactDirectMorseForestReducerBatch& batch);

  // The returned locator is a short-lived strict-state view for the 14D
  // executor.  No fold may overlap such a read, and the reducer must not move
  // while an executor retains this address.
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& strict_locator()
      const noexcept;

  [[nodiscard]] std::size_t next_source_batch_index() const noexcept;
  [[nodiscard]] bool complete() const noexcept;

  // Consumes the reducer after every source batch has committed.  Calling
  // finish early throws std::logic_error.
  [[nodiscard]] ExactDirectMorseForestJournalResult finish();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
