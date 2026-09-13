#pragma once

#include <iostream>

#include "pipeline/local_credits.hpp"

namespace mhgp8::bench {

// Shared wire spelling only: no interpretation or summation of unlike work.
inline void print_work(const Work& work) {
  std::cout << "{\"validation_points\":" << work.validation_points
            << ",\"uniqueness_comparisons\":" << work.uniqueness_comparisons
            << ",\"pool_selection_tests\":" << work.pool_selection_tests
            << ",\"pool_selected\":" << work.pool_selected
            << ",\"tree_nodes\":" << work.tree_nodes
            << ",\"tree_point_visits\":" << work.tree_point_visits
            << ",\"dual_tasks\":" << work.dual_tasks
            << ",\"credited_blocks\":" << work.credited_blocks
            << ",\"noncredit_blocks\":" << work.noncredit_blocks
            << ",\"leaf_pairs\":" << work.leaf_pairs
            << ",\"saturated_tasks\":" << work.saturated_tasks
            << ",\"max_tree_depth\":" << work.max_tree_depth
            << ",\"max_task_depth\":" << work.max_task_depth
            << ",\"tube_records\":" << work.tube_records
            << ",\"tube_cells\":" << work.tube_cells
            << ",\"tube_sort_comparisons\":" << work.tube_sort_comparisons
            << ",\"tube_sweep_tests\":" << work.tube_sweep_tests
            << ",\"tube_credited_sites\":" << work.tube_credited_sites
            << ",\"tube_separation_fallbacks\":" << work.tube_separation_fallbacks;
  const auto& predicates = work.predicates;
  std::cout << ",\"predicates\":{\"point_tests\":" << predicates.point_tests
            << ",\"universal_queries\":" << predicates.universal_queries
            << ",\"q2_axis_terms\":" << predicates.q2_axis_terms
            << ",\"corner_tests\":" << predicates.corner_tests
            << ",\"block_bound_tests\":" << predicates.block_bound_tests
            << ",\"negative_probes\":" << predicates.negative_probes << "}}";
}

}  // namespace mhgp8::bench
