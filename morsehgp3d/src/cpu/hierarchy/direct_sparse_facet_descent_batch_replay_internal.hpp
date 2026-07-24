#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_batch_executor.hpp"

namespace morsehgp3d::hierarchy::detail {

// Internal immutable seam used by Phase 15B.  The caller owns the freshly
// reconstructed 14C plan and derives these cursors once; this function never
// advances an executor session and never accepts a process-local 14H ticket.
struct ExactDirectSparseFacetDescentBatchReplayCursor {
  std::size_t source_batch_index{};
  std::size_t source_chunk_index{};
  std::size_t source_lane_index{};
  std::size_t source_family_index{};
  std::size_t source_arm_seed_index{};
};

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult
replay_exact_direct_sparse_facet_descent_batch_at_cursor(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan,
    const ExactDirectSparseFacetDescentBatchReplayCursor& cursor,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& execution_budget,
    const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order);

}  // namespace morsehgp3d::hierarchy::detail
