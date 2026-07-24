#pragma once

#include "morsehgp3d/hierarchy/direct_morse_chunk_run.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"

#include <stdexcept>

namespace morsehgp3d::hierarchy {

// Zero-copy Phase-15C handoff from one freshly recertified 15B batch to the
// incremental reducer.  The source projection must remain alive and immutable
// until fold() returns; neither vector span is retained by the reducer.
[[nodiscard]] inline ExactDirectMorseForestReducerBatch
project_exact_direct_morse_recertified_forest_reducer_batch(
    const ExactDirectMorseRecertifiedBatchProjection& source) {
  if (!source.closure_summary.complete_relative_positive ||
      source.closure_summary.graph_payload_persisted ||
      source.execution_counters.resolved_key_projection_count !=
          source.resolved_keys.size() ||
      source.execution_counters.arm_join_count !=
          source.arm_joins.size() ||
      source.execution_counters.shared_closure_build_count !=
          (source.resolved_keys.empty() ? 0U : 1U)) {
    throw std::invalid_argument(
        "a recertified forest handoff requires one complete compact 14D "
        "projection");
  }
  ExactDirectMorseForestReducerBatch projected;
  projected.source_batch_index = source.source_batch_index;
  projected.source_chunk_index = source.source_chunk_index;
  projected.source_family_begin_index =
      source.source_family_begin_index;
  projected.source_family_end_index = source.source_family_end_index;
  projected.source_arm_seed_begin_index =
      source.source_arm_seed_begin_index;
  projected.source_arm_seed_end_index =
      source.source_arm_seed_end_index;
  projected.order = source.order;
  projected.squared_level = source.closed_batch_squared_level;
  projected.traversal_order = source.traversal_order;
  projected.locator_query_witness = source.locator_query_witness;
  projected.strict_pre_batch_locator_stamp =
      source.strict_pre_batch_locator_stamp;
  projected.requested_closure_budget =
      source.requested_closure_budget;
  projected.transient_closure_node_count =
      source.closure_summary.transient_node_count;
  projected.transient_closure_step_call_count =
      source.closure_summary.counters.evaluated_step_source_count;
  projected.shared_closure_build_count =
      source.execution_counters.shared_closure_build_count;
  projected.resolved_keys = source.resolved_keys;
  projected.arm_joins = source.arm_joins;
  return projected;
}

[[nodiscard]] ExactDirectMorseForestReducerBatch
project_exact_direct_morse_recertified_forest_reducer_batch(
    ExactDirectMorseRecertifiedBatchProjection&&) = delete;
[[nodiscard]] ExactDirectMorseForestReducerBatch
project_exact_direct_morse_recertified_forest_reducer_batch(
    const ExactDirectMorseRecertifiedBatchProjection&&) = delete;

}  // namespace morsehgp3d::hierarchy
