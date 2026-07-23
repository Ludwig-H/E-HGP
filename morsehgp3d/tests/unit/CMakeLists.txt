add_executable(morsehgp3d_exact_types_tests test_exact_types.cpp)
target_link_libraries(morsehgp3d_exact_types_tests PRIVATE morsehgp3d::exact)

add_executable(
  morsehgp3d_contract_canonical_id_tests
  test_contract_canonical_id.cpp
)
target_link_libraries(
  morsehgp3d_contract_canonical_id_tests
  PRIVATE morsehgp3d::contract
)

add_executable(morsehgp3d_exact_types_dump exact_types_dump.cpp)
target_link_libraries(morsehgp3d_exact_types_dump PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_predicates_tests test_predicates.cpp)
target_link_libraries(morsehgp3d_predicates_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_affine_tests test_affine.cpp)
target_link_libraries(morsehgp3d_affine_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_centers_tests test_centers.cpp)
target_link_libraries(morsehgp3d_centers_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_support_tests test_support.cpp)
target_link_libraries(morsehgp3d_support_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_level_order_tests test_level_order.cpp)
target_link_libraries(morsehgp3d_level_order_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_expansion_tests test_expansion.cpp)
target_link_libraries(morsehgp3d_expansion_tests PRIVATE morsehgp3d::exact)

add_executable(morsehgp3d_spatial_tests test_spatial_brute_force.cpp)
target_link_libraries(morsehgp3d_spatial_tests PRIVATE morsehgp3d::spatial)

add_executable(
  morsehgp3d_power_cell_reference_tests
  test_power_cell_reference.cpp
)
target_link_libraries(
  morsehgp3d_power_cell_reference_tests
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(morsehgp3d_power_cell_reference_tests PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_power_cell_reference_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_h_polytope_reference_tests
  test_h_polytope_reference.cpp
)
target_link_libraries(
  morsehgp3d_h_polytope_reference_tests
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(morsehgp3d_h_polytope_reference_tests PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_h_polytope_reference_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_point_cloud_aabb_tests
  test_point_cloud_aabb.cpp
)
target_link_libraries(
  morsehgp3d_point_cloud_aabb_tests
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(morsehgp3d_point_cloud_aabb_tests PRIVATE /W4 /WX)
else()
  target_compile_options(
    morsehgp3d_point_cloud_aabb_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_ordinary_cell_closure_tests
  test_ordinary_cell_closure.cpp
)
target_link_libraries(
  morsehgp3d_ordinary_cell_closure_tests
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(
    morsehgp3d_ordinary_cell_closure_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_ordinary_cell_closure_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_ordinary_diagram_closure_tests
  test_ordinary_diagram_closure.cpp
)
target_link_libraries(
  morsehgp3d_ordinary_diagram_closure_tests
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(
    morsehgp3d_ordinary_diagram_closure_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_ordinary_diagram_closure_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_ordinary_diagram_closure_dump
  ordinary_diagram_closure_dump.cpp
)
target_link_libraries(
  morsehgp3d_ordinary_diagram_closure_dump
  PRIVATE morsehgp3d::spatial
)
if(MSVC)
  target_compile_options(
    morsehgp3d_ordinary_diagram_closure_dump PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_ordinary_diagram_closure_dump
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(morsehgp3d_spatial_query_dump spatial_query_dump.cpp)
target_link_libraries(morsehgp3d_spatial_query_dump PRIVATE morsehgp3d::spatial)

add_executable(morsehgp3d_spatial_lbvh_tests test_spatial_lbvh.cpp)
target_link_libraries(morsehgp3d_spatial_lbvh_tests PRIVATE morsehgp3d::spatial)

add_executable(morsehgp3d_spatial_lbvh_query_dump spatial_lbvh_query_dump.cpp)
target_link_libraries(
  morsehgp3d_spatial_lbvh_query_dump
  PRIVATE morsehgp3d::spatial
)

add_executable(morsehgp3d_hierarchy_emst_tests test_hierarchy_emst.cpp)
target_link_libraries(
  morsehgp3d_hierarchy_emst_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_k1_forest_tests
  test_hierarchy_k1_forest.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_k1_forest_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_miniball_tests
  test_hierarchy_miniball.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_miniball_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_gamma_tests
  test_hierarchy_gamma.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_gamma_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_arm_gamma_tests
  test_hierarchy_critical_arm_gamma.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_arm_gamma_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_gamma_transition_tests
  test_hierarchy_gamma_transition.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_gamma_transition_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_reduced_gamma_batch_tests
  test_hierarchy_reduced_gamma_batch.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_reduced_gamma_batch_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_reduced_gamma_history_tests
  test_hierarchy_reduced_gamma_history.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_reduced_gamma_history_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_reduced_gamma_cut_tests
  test_hierarchy_reduced_gamma_cut.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_reduced_gamma_cut_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
  test_hierarchy_critical_event_gamma_overlay.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_tests
  test_hierarchy_critical_catalog.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_depth_zero_natural_support_tests
  test_depth_zero_natural_support.cpp
)
target_link_libraries(
  morsehgp3d_depth_zero_natural_support_tests
  PRIVATE morsehgp3d::hierarchy
)
if(MSVC)
  target_compile_options(
    morsehgp3d_depth_zero_natural_support_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_depth_zero_natural_support_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_pair_support_stream_tests
  test_hierarchy_pair_support_stream.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_pair_support_stream_tests
  PRIVATE morsehgp3d::pair_support
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_pair_support_stream_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_pair_support_stream_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_higher_support_product_tests
  test_higher_support_product.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_higher_support_product_tests
  PRIVATE morsehgp3d::higher_support
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_higher_support_product_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_higher_support_product_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_higher_support_stream_tests
  test_hierarchy_higher_support_stream.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_higher_support_stream_tests
  PRIVATE morsehgp3d::higher_support
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_higher_support_stream_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_higher_support_stream_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_support_differential_tests
  test_hierarchy_direct_support_differential.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_support_differential_tests
  PRIVATE
    morsehgp3d::pair_support
    morsehgp3d::higher_support
    morsehgp3d::hierarchy
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_support_differential_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_support_differential_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_support_terminal_tests
  test_hierarchy_direct_support_terminal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_support_terminal_tests
  PRIVATE morsehgp3d::direct_support_terminal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_support_terminal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_support_terminal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_event_journal_tests
  test_hierarchy_direct_morse_event_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_event_journal_tests
  PRIVATE morsehgp3d::direct_morse_event_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_event_journal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_event_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
  test_hierarchy_direct_saddle_arm_seed_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
  PRIVATE morsehgp3d::direct_saddle_arm_seed_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
  test_hierarchy_direct_closed_saddle_incidence_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
  PRIVATE morsehgp3d::direct_closed_saddle_incidence_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
  test_hierarchy_direct_sparse_positive_facet_locator.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
  PRIVATE morsehgp3d::direct_sparse_positive_facet_locator
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
  test_hierarchy_direct_sparse_positive_facet_prefix_sweep.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
  PRIVATE morsehgp3d::direct_sparse_positive_facet_prefix_sweep
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
  test_hierarchy_direct_sparse_facet_descent_step.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
  PRIVATE morsehgp3d::direct_sparse_facet_descent_step
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
  test_hierarchy_direct_sparse_facet_descent_closure.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
  PRIVATE morsehgp3d::direct_sparse_facet_descent_closure
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
  test_hierarchy_direct_sparse_first_incidence.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
  PRIVATE morsehgp3d::direct_sparse_first_incidence
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_first_incidence_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
  test_hierarchy_direct_sparse_gateway_candidate_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
  PRIVATE morsehgp3d::direct_sparse_gateway_candidate_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
  test_hierarchy_direct_sparse_gateway_candidate_localization.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
  PRIVATE morsehgp3d::direct_sparse_gateway_candidate_localization
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
  test_hierarchy_direct_sparse_gateway_clock.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
  PRIVATE morsehgp3d::direct_sparse_gateway_historical_quotient
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
  test_hierarchy_direct_frozen_root_quotient.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
  PRIVATE morsehgp3d::direct_frozen_root_quotient
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_root_quotient_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_k1_forest_journal_tests
  test_hierarchy_direct_k1_forest_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_k1_forest_journal_tests
  PRIVATE morsehgp3d::direct_k1_forest_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_k1_forest_journal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_k1_forest_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_forest_journal_tests
  test_hierarchy_direct_morse_forest_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_journal_tests
  PRIVATE
    morsehgp3d::direct_morse_forest_journal
    morsehgp3d::direct_morse_industrial_plan
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_journal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
  test_hierarchy_direct_sparse_facet_descent_batch_plan.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
  PRIVATE morsehgp3d::direct_sparse_facet_descent_batch_plan
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
  test_hierarchy_direct_sparse_facet_descent_batch_executor.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
  PRIVATE
    morsehgp3d::direct_sparse_facet_descent_batch_executor
    morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
  test_hierarchy_direct_sparse_facet_top_k_proposal_transcript.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
  PRIVATE morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_pair_support_stream_codec_tests
  test_hierarchy_pair_support_stream_codec.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_pair_support_stream_codec_tests
  PRIVATE morsehgp3d::pair_support_codec
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_pair_support_stream_codec_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_pair_support_stream_codec_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(UNIX)
  add_executable(
    morsehgp3d_hierarchy_pair_support_stream_durable_tests
    test_hierarchy_pair_support_stream_durable.cpp
  )
  target_link_libraries(
    morsehgp3d_hierarchy_pair_support_stream_durable_tests
    PRIVATE morsehgp3d::pair_support_durable
  )
  target_compile_options(
    morsehgp3d_hierarchy_pair_support_stream_durable_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
  test_hierarchy_critical_catalog_reduced_gamma_overlay.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
  test_hierarchy_critical_catalog_arm_gamma_overlay.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
  test_hierarchy_critical_catalog_typed_gamma_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
  test_hierarchy_critical_catalog_typed_gamma_root_overlay.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
  test_hierarchy_critical_catalog_typed_gamma_arm_root_composition.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
  test_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
  test_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests
  test_hierarchy_morse_gamma_partition_sweep.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests
  PRIVATE morsehgp3d::hierarchy
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(MSVC)
  target_compile_options(
    morsehgp3d_contract_canonical_id_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_contract_canonical_id_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(morsehgp3d_hierarchy_gamma_dump hierarchy_gamma_dump.cpp)
target_link_libraries(
  morsehgp3d_hierarchy_gamma_dump
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump
  hierarchy_critical_catalog_arm_gamma_overlay_dump.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_boruvka_tests
  test_hierarchy_boruvka.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_boruvka_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(morsehgp3d_hierarchy_gabriel_tests test_hierarchy_gabriel.cpp)
target_link_libraries(
  morsehgp3d_hierarchy_gabriel_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(morsehgp3d_hierarchy_k1_dump hierarchy_k1_dump.cpp)
target_link_libraries(
  morsehgp3d_hierarchy_k1_dump
  PRIVATE morsehgp3d::hierarchy
)

find_package(Threads REQUIRED)
add_executable(
  morsehgp3d_predicate_filter_context_tests
  test_predicate_filter_context.cpp
  fake_gpu_predicate_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/predicate_filter.cpp"
)
add_executable(
  morsehgp3d_warm_context_benchmark_host_test
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_predicate_context_harness.cpp"
  fake_gpu_predicate_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/predicate_filter.cpp"
)
add_executable(
  morsehgp3d_gpu_spatial_reference_context_tests
  test_gpu_spatial_reference_context.cpp
  fake_gpu_spatial_reference_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_reference.cpp"
)
add_executable(
  morsehgp3d_gpu_h_polytope_proposal_context_tests
  test_gpu_h_polytope_proposal_context.cpp
  fake_gpu_h_polytope_proposal_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/h_polytope_proposal.cpp"
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_h_polytope_proposal_context_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_h_polytope_proposal_context_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_pair_support_phi_context_tests
  test_gpu_pair_support_phi_context.cpp
  fake_gpu_pair_support_phi_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/pair_support_phi.cpp"
)
target_include_directories(
  morsehgp3d_gpu_pair_support_phi_context_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_pair_support_phi_context_tests
  PRIVATE morsehgp3d::pair_support Threads::Threads
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_pair_support_phi_context_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_pair_support_phi_context_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_spatial_reference_replay_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_spatial_reference_replay.cpp"
  fake_gpu_spatial_reference_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_reference.cpp"
)
add_executable(
  morsehgp3d_gpu_spatial_bounds_context_tests
  test_gpu_spatial_bounds_context.cpp
  fake_gpu_spatial_bounds_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_bounds.cpp"
)
add_executable(
  morsehgp3d_gpu_spatial_bounds_replay_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_spatial_bounds_replay.cpp"
  fake_gpu_spatial_bounds_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_bounds.cpp"
)
add_executable(
  morsehgp3d_gpu_spatial_lbvh_context_tests
  test_gpu_spatial_lbvh_context.cpp
  fake_gpu_spatial_bounds_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_lbvh.cpp"
)
add_executable(
  morsehgp3d_gpu_spatial_lbvh_replay_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_spatial_lbvh_replay.cpp"
  fake_gpu_spatial_bounds_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/spatial_lbvh.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_context_tests
  test_gpu_k1_boruvka_context.cpp
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_hybrid.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_exact_search_tests
  test_gpu_k1_boruvka_exact_search.cpp
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
  test_gpu_k1_boruvka_seeded_chain.cpp
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_seeded_exact.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_hierarchy_tests
  test_gpu_k1_boruvka_hierarchy.cpp
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_seeded_exact.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_hierarchy.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_k1_boruvka_exact_search_work_profile.cpp"
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_seeded_exact.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_replay_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_k1_boruvka_replay.cpp"
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_full_replay_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_k1_boruvka_full_replay.cpp"
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka_hybrid.cpp"
)
add_executable(
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
  "${PROJECT_SOURCE_DIR}/src/tools/gpu_k1_boruvka_morton_work_profile.cpp"
  fake_gpu_k1_boruvka_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/k1_boruvka.cpp"
)
target_include_directories(
  morsehgp3d_predicate_filter_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_warm_context_benchmark_host_test
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_reference_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_h_polytope_proposal_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_reference_replay_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_bounds_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_bounds_replay_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_lbvh_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_spatial_lbvh_replay_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_exact_search_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_hierarchy_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_replay_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_full_replay_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_include_directories(
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_predicate_filter_context_tests
  PRIVATE morsehgp3d::exact Threads::Threads
)
target_link_libraries(
  morsehgp3d_warm_context_benchmark_host_test
  PRIVATE morsehgp3d::exact Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_reference_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_h_polytope_proposal_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_reference_replay_host
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_bounds_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_bounds_replay_host
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_lbvh_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_spatial_lbvh_replay_host
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_context_tests
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_exact_search_tests
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_hierarchy_tests
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_compile_definitions(
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  PRIVATE MORSEHGP3D_EXACT_SEARCH_PROFILE_BACKEND="fake_gpu"
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_replay_host
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_full_replay_host
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_link_libraries(
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
  PRIVATE morsehgp3d::hierarchy Threads::Threads
)
target_compile_definitions(
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
  PRIVATE MORSEHGP3D_WORK_PROFILE_BACKEND=\"fake_gpu\"
)
target_compile_definitions(
  morsehgp3d_warm_context_benchmark_host_test
  PRIVATE
    MORSEHGP3D_WARM_CONTEXT_CASE_COUNT=9
    MORSEHGP3D_WARM_CONTEXT_REPEAT_COUNT=3
)

set(
  _morsehgp3d_cpu_test_targets
  morsehgp3d_contract_canonical_id_tests
  morsehgp3d_exact_types_tests
  morsehgp3d_exact_types_dump
  morsehgp3d_predicates_tests
  morsehgp3d_affine_tests
  morsehgp3d_centers_tests
  morsehgp3d_support_tests
  morsehgp3d_level_order_tests
  morsehgp3d_expansion_tests
  morsehgp3d_spatial_tests
  morsehgp3d_power_cell_reference_tests
  morsehgp3d_h_polytope_reference_tests
  morsehgp3d_point_cloud_aabb_tests
  morsehgp3d_ordinary_cell_closure_tests
  morsehgp3d_ordinary_diagram_closure_tests
  morsehgp3d_ordinary_diagram_closure_dump
  morsehgp3d_spatial_query_dump
  morsehgp3d_spatial_lbvh_tests
  morsehgp3d_spatial_lbvh_query_dump
  morsehgp3d_hierarchy_emst_tests
  morsehgp3d_hierarchy_k1_forest_tests
  morsehgp3d_hierarchy_miniball_tests
  morsehgp3d_hierarchy_gamma_tests
  morsehgp3d_hierarchy_critical_arm_gamma_tests
  morsehgp3d_hierarchy_gamma_transition_tests
  morsehgp3d_hierarchy_reduced_gamma_batch_tests
  morsehgp3d_hierarchy_reduced_gamma_history_tests
  morsehgp3d_hierarchy_reduced_gamma_cut_tests
  morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_tests
  morsehgp3d_depth_zero_natural_support_tests
  morsehgp3d_hierarchy_pair_support_stream_tests
  morsehgp3d_hierarchy_higher_support_product_tests
  morsehgp3d_hierarchy_higher_support_stream_tests
  morsehgp3d_hierarchy_direct_support_differential_tests
  morsehgp3d_hierarchy_direct_support_terminal_tests
  morsehgp3d_hierarchy_direct_morse_event_journal_tests
  morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
  morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
  morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
  morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
  morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
  morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
  morsehgp3d_hierarchy_direct_k1_forest_journal_tests
  morsehgp3d_hierarchy_direct_morse_forest_journal_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
  morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
  morsehgp3d_hierarchy_pair_support_stream_codec_tests
  morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
  morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests
  morsehgp3d_hierarchy_gamma_dump
  morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump
  morsehgp3d_hierarchy_boruvka_tests
  morsehgp3d_hierarchy_gabriel_tests
  morsehgp3d_hierarchy_k1_dump
  morsehgp3d_predicate_filter_context_tests
  morsehgp3d_warm_context_benchmark_host_test
  morsehgp3d_gpu_spatial_reference_context_tests
  morsehgp3d_gpu_h_polytope_proposal_context_tests
  morsehgp3d_gpu_pair_support_phi_context_tests
  morsehgp3d_gpu_spatial_reference_replay_host
  morsehgp3d_gpu_spatial_bounds_context_tests
  morsehgp3d_gpu_spatial_bounds_replay_host
  morsehgp3d_gpu_spatial_lbvh_context_tests
  morsehgp3d_gpu_spatial_lbvh_replay_host
  morsehgp3d_gpu_k1_boruvka_context_tests
  morsehgp3d_gpu_k1_boruvka_exact_search_tests
  morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
  morsehgp3d_gpu_k1_boruvka_hierarchy_tests
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  morsehgp3d_gpu_k1_boruvka_replay_host
  morsehgp3d_gpu_k1_boruvka_full_replay_host
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
)
if(UNIX)
  list(
    APPEND _morsehgp3d_cpu_test_targets
    morsehgp3d_hierarchy_pair_support_stream_durable_tests
  )
endif()
foreach(_morsehgp3d_cpu_test_target IN LISTS _morsehgp3d_cpu_test_targets)
  morsehgp3d_apply_sanitizers("${_morsehgp3d_cpu_test_target}")
endforeach()
unset(_morsehgp3d_cpu_test_target)
unset(_morsehgp3d_cpu_test_targets)

if(MSVC)
  target_compile_options(morsehgp3d_exact_types_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_exact_types_dump PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_predicates_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_affine_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_centers_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_support_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_level_order_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_expansion_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_spatial_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_spatial_query_dump PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_spatial_lbvh_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_spatial_lbvh_query_dump PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_hierarchy_emst_tests PRIVATE /W4 /WX)
  target_compile_options(
    morsehgp3d_hierarchy_k1_forest_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_miniball_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_gamma_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_arm_gamma_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_gamma_transition_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_batch_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_history_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_cut_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_event_gamma_overlay_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(morsehgp3d_hierarchy_gamma_dump PRIVATE /W4 /WX)
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump
    PRIVATE /W4 /WX
  )
  target_compile_options(morsehgp3d_hierarchy_boruvka_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_hierarchy_gabriel_tests PRIVATE /W4 /WX)
  target_compile_options(morsehgp3d_hierarchy_k1_dump PRIVATE /W4 /WX)
  target_compile_options(
    morsehgp3d_predicate_filter_context_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_warm_context_benchmark_host_test PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_reference_context_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_reference_replay_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_bounds_context_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_bounds_replay_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_lbvh_context_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_lbvh_replay_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_context_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_exact_search_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_seeded_chain_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_hierarchy_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_replay_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_full_replay_host PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_morton_work_profile_host PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_exact_types_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_exact_types_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_predicates_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_affine_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_centers_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_support_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_level_order_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_expansion_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_spatial_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_spatial_query_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_spatial_lbvh_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_spatial_lbvh_query_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_emst_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_k1_forest_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_miniball_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_gamma_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_arm_gamma_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_gamma_transition_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_batch_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_history_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_reduced_gamma_cut_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_gamma_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_boruvka_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_gabriel_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_k1_dump
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_predicate_filter_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_warm_context_benchmark_host_test
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_reference_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_reference_replay_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_bounds_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_bounds_replay_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_lbvh_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_spatial_lbvh_replay_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_exact_search_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_hierarchy_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_replay_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_full_replay_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_test(NAME morsehgp3d.exact_types COMMAND morsehgp3d_exact_types_tests)
add_test(NAME morsehgp3d.predicates COMMAND morsehgp3d_predicates_tests)
add_test(NAME morsehgp3d.affine COMMAND morsehgp3d_affine_tests)
add_test(NAME morsehgp3d.centers COMMAND morsehgp3d_centers_tests)
add_test(NAME morsehgp3d.support COMMAND morsehgp3d_support_tests)
add_test(NAME morsehgp3d.level_order COMMAND morsehgp3d_level_order_tests)
add_test(NAME morsehgp3d.expansion COMMAND morsehgp3d_expansion_tests)
add_test(NAME morsehgp3d.spatial_brute_force COMMAND morsehgp3d_spatial_tests)
add_test(
  NAME morsehgp3d.power_cell_reference
  COMMAND morsehgp3d_power_cell_reference_tests
)
add_test(
  NAME morsehgp3d.h_polytope_reference
  COMMAND morsehgp3d_h_polytope_reference_tests
)
add_test(
  NAME morsehgp3d.point_cloud_aabb
  COMMAND morsehgp3d_point_cloud_aabb_tests
)
add_test(
  NAME morsehgp3d.ordinary_cell_closure
  COMMAND morsehgp3d_ordinary_cell_closure_tests
)
add_test(
  NAME morsehgp3d.ordinary_diagram_closure
  COMMAND morsehgp3d_ordinary_diagram_closure_tests
)
add_test(NAME morsehgp3d.spatial_lbvh COMMAND morsehgp3d_spatial_lbvh_tests)
add_test(NAME morsehgp3d.hierarchy_emst COMMAND morsehgp3d_hierarchy_emst_tests)
add_test(
  NAME morsehgp3d.hierarchy_k1_forest
  COMMAND morsehgp3d_hierarchy_k1_forest_tests
)
add_test(
  NAME morsehgp3d.hierarchy_miniball
  COMMAND morsehgp3d_hierarchy_miniball_tests
)
add_test(
  NAME morsehgp3d.hierarchy_gamma
  COMMAND morsehgp3d_hierarchy_gamma_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_arm_gamma
  COMMAND morsehgp3d_hierarchy_critical_arm_gamma_tests
)
add_test(
  NAME morsehgp3d.hierarchy_gamma_transition
  COMMAND morsehgp3d_hierarchy_gamma_transition_tests
)
add_test(
  NAME morsehgp3d.hierarchy_reduced_gamma_batch
  COMMAND morsehgp3d_hierarchy_reduced_gamma_batch_tests
)
add_test(
  NAME morsehgp3d.hierarchy_reduced_gamma_history
  COMMAND morsehgp3d_hierarchy_reduced_gamma_history_tests
)
add_test(
  NAME morsehgp3d.hierarchy_reduced_gamma_cut
  COMMAND morsehgp3d_hierarchy_reduced_gamma_cut_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_event_gamma_overlay
  COMMAND morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog
  COMMAND morsehgp3d_hierarchy_critical_catalog_tests
)
add_test(
  NAME morsehgp3d.depth_zero_natural_support
  COMMAND morsehgp3d_depth_zero_natural_support_tests
)
add_test(
  NAME morsehgp3d.hierarchy_pair_support_stream
  COMMAND morsehgp3d_hierarchy_pair_support_stream_tests
)
add_test(
  NAME morsehgp3d.hierarchy_higher_support_product
  COMMAND morsehgp3d_hierarchy_higher_support_product_tests
)
add_test(
  NAME morsehgp3d.hierarchy_higher_support_stream
  COMMAND morsehgp3d_hierarchy_higher_support_stream_tests
)

add_test(
  NAME morsehgp3d.hierarchy_direct_support_differential
  COMMAND morsehgp3d_hierarchy_direct_support_differential_tests
)

add_test(
  NAME morsehgp3d.hierarchy_direct_support_terminal
  COMMAND morsehgp3d_hierarchy_direct_support_terminal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_event_journal
  COMMAND morsehgp3d_hierarchy_direct_morse_event_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_saddle_arm_seed_journal
  COMMAND morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_closed_saddle_incidence_journal
  COMMAND morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_positive_facet_locator
  COMMAND morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_positive_facet_prefix_sweep
  COMMAND morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_facet_descent_step
  COMMAND morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_facet_descent_closure
  COMMAND morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_first_incidence
  COMMAND morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_gateway_candidate_journal
  COMMAND morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_gateway_candidate_localization
  COMMAND morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_gateway_clock
  COMMAND morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_frozen_root_quotient
  COMMAND morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_k1_forest_journal
  COMMAND morsehgp3d_hierarchy_direct_k1_forest_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_forest_journal
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_facet_descent_batch_plan
  COMMAND morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_plan_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_facet_descent_batch_executor
  COMMAND morsehgp3d_hierarchy_direct_sparse_facet_descent_batch_executor_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_facet_top_k_proposal_transcript
  COMMAND
    morsehgp3d_hierarchy_direct_sparse_facet_top_k_proposal_transcript_tests
)
add_test(
  NAME morsehgp3d.hierarchy_pair_support_stream_codec
  COMMAND morsehgp3d_hierarchy_pair_support_stream_codec_tests
)
if(UNIX)
  add_test(
    NAME morsehgp3d.hierarchy_pair_support_stream_durable
    COMMAND morsehgp3d_hierarchy_pair_support_stream_durable_tests
  )
endif()
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog_reduced_gamma_overlay
  COMMAND morsehgp3d_hierarchy_critical_catalog_reduced_gamma_overlay_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog_arm_gamma_overlay
  COMMAND morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog_typed_gamma_journal
  COMMAND morsehgp3d_hierarchy_critical_catalog_typed_gamma_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog_typed_gamma_root_overlay
  COMMAND
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_root_overlay_tests
)
add_test(
  NAME
    morsehgp3d.hierarchy_critical_catalog_typed_gamma_arm_root_composition
  COMMAND
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_composition_tests
)
add_test(
  NAME
    morsehgp3d.hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay
  COMMAND
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_arm_root_path_overlay_tests
)
add_test(
  NAME morsehgp3d.contract_canonical_id
  COMMAND morsehgp3d_contract_canonical_id_tests
)
add_test(
  NAME
    morsehgp3d.hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog
  COMMAND
    morsehgp3d_hierarchy_critical_catalog_typed_gamma_durable_arm_key_catalog_tests
)
add_test(
  NAME morsehgp3d.hierarchy_morse_gamma_partition_sweep
  COMMAND morsehgp3d_hierarchy_morse_gamma_partition_sweep_tests
)
add_test(
  NAME morsehgp3d.hierarchy_boruvka
  COMMAND morsehgp3d_hierarchy_boruvka_tests
)
add_test(
  NAME morsehgp3d.hierarchy_gabriel
  COMMAND morsehgp3d_hierarchy_gabriel_tests
)
add_test(
  NAME morsehgp3d.predicate_filter_context
  COMMAND morsehgp3d_predicate_filter_context_tests
)
add_test(
  NAME morsehgp3d.gpu_spatial_reference_context
  COMMAND morsehgp3d_gpu_spatial_reference_context_tests
)
add_test(
  NAME morsehgp3d.gpu_h_polytope_proposal_context
  COMMAND morsehgp3d_gpu_h_polytope_proposal_context_tests
)
add_test(
  NAME morsehgp3d.gpu_pair_support_phi_context
  COMMAND morsehgp3d_gpu_pair_support_phi_context_tests
)
add_test(
  NAME morsehgp3d.gpu_spatial_bounds_context
  COMMAND morsehgp3d_gpu_spatial_bounds_context_tests
)
add_test(
  NAME morsehgp3d.gpu_spatial_lbvh_context
  COMMAND morsehgp3d_gpu_spatial_lbvh_context_tests
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_context
  COMMAND morsehgp3d_gpu_k1_boruvka_context_tests
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_exact_search
  COMMAND morsehgp3d_gpu_k1_boruvka_exact_search_tests
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_seeded_chain
  COMMAND morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_hierarchy
  COMMAND morsehgp3d_gpu_k1_boruvka_hierarchy_tests
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_exact_search_work_profile_host
  COMMAND
    morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
    --family lattice
    --point-count 16
    --window-radius 4
    --seed 1
    --git-sha 0000000000000000000000000000000000000000
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_replay_host
  COMMAND morsehgp3d_gpu_k1_boruvka_replay_host
)
add_test(
  NAME morsehgp3d.gpu_k1_boruvka_full_replay_host
  COMMAND morsehgp3d_gpu_k1_boruvka_full_replay_host
)

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(
  NAME morsehgp3d.ordinary_diagram_closure_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_ordinary_diagram_closure.py"
    "$<TARGET_FILE:morsehgp3d_ordinary_diagram_closure_dump>"
)
set_tests_properties(
  morsehgp3d.ordinary_diagram_closure_differential
  PROPERTIES TIMEOUT 45
)

add_test(
  NAME morsehgp3d.hierarchy_gamma_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_hierarchy_gamma.py"
    "$<TARGET_FILE:morsehgp3d_hierarchy_gamma_dump>"
)
set_tests_properties(
  morsehgp3d.hierarchy_gamma_differential
  PROPERTIES TIMEOUT 300
)

add_test(
  NAME morsehgp3d.hierarchy_gamma_transition_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_hierarchy_gamma_transition.py"
    "$<TARGET_FILE:morsehgp3d_hierarchy_gamma_dump>"
)
set_tests_properties(
  morsehgp3d.hierarchy_gamma_transition_differential
  PROPERTIES TIMEOUT 60
)
add_test(
  NAME morsehgp3d.hierarchy_critical_catalog_arm_gamma_overlay_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_hierarchy_critical_catalog_arm_gamma_overlay.py"
    "$<TARGET_FILE:morsehgp3d_hierarchy_critical_catalog_arm_gamma_overlay_dump>"
)
set_tests_properties(
  morsehgp3d.hierarchy_critical_catalog_arm_gamma_overlay_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.hierarchy_k1_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_hierarchy_k1.py"
    "$<TARGET_FILE:morsehgp3d_hierarchy_k1_dump>"
)
set_tests_properties(
  morsehgp3d.hierarchy_k1_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.spatial_brute_force_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_brute_force.py"
    "$<TARGET_FILE:morsehgp3d_spatial_query_dump>"
    --fixture
    "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/spatial/relevant_gp_extra_shell_above_smax.json"
)
set_tests_properties(
  morsehgp3d.spatial_brute_force_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.spatial_lbvh_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_lbvh.py"
    "$<TARGET_FILE:morsehgp3d_spatial_lbvh_query_dump>"
)
set_tests_properties(
  morsehgp3d.spatial_lbvh_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.warm_context_benchmark_host
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_warm_context_benchmark.py"
    "$<TARGET_FILE:morsehgp3d_warm_context_benchmark_host_test>"
)
add_test(
  NAME morsehgp3d.phase3_build_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase3_build.py"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/.."
)
add_test(
  NAME morsehgp3d.phase3_runtime_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase3_runtime.py"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/.."
)
add_test(
  NAME morsehgp3d.phase2b_predicate_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase2b_predicates.py"
    "${PROJECT_SOURCE_DIR}"
)
add_test(
  NAME morsehgp3d.phase4_spatial_reference_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase4_spatial_reference.py"
    "${PROJECT_SOURCE_DIR}"
)
add_test(
  NAME morsehgp3d.phase4_spatial_bounds_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase4_spatial_bounds.py"
    "${PROJECT_SOURCE_DIR}"
)
add_test(
  NAME morsehgp3d.phase4_spatial_lbvh_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase4_spatial_lbvh.py"
    "${PROJECT_SOURCE_DIR}"
)
add_test(
  NAME morsehgp3d.phase7_h_polytope_proposal_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase7_h_polytope_proposal.py"
    "${PROJECT_SOURCE_DIR}"
)
set(
  _morsehgp3d_phase9_pair_support_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase9_pair_support.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  if(UNIX)
    set(
      _morsehgp3d_phase9_pair_support_symbol_binary
      "$<TARGET_FILE:morsehgp3d_hierarchy_pair_support_stream_durable_tests>"
    )
  else()
    set(
      _morsehgp3d_phase9_pair_support_symbol_binary
      "$<TARGET_FILE:morsehgp3d_hierarchy_pair_support_stream_tests>"
    )
  endif()
  list(
    APPEND _morsehgp3d_phase9_pair_support_configuration_command
    --binary
    "${_morsehgp3d_phase9_pair_support_symbol_binary}"
    --nm
    "${CMAKE_NM}"
  )
  unset(_morsehgp3d_phase9_pair_support_symbol_binary)
endif()
add_test(
  NAME morsehgp3d.phase9_pair_support_configuration
  COMMAND ${_morsehgp3d_phase9_pair_support_configuration_command}
)
unset(_morsehgp3d_phase9_pair_support_configuration_command)
set(
  _morsehgp3d_phase10_direct_saddle_arm_seed_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_saddle_arm_seed.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_saddle_arm_seed_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_saddle_arm_seed_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_saddle_arm_seed_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_saddle_arm_seed_configuration_command
)
set(
  _morsehgp3d_phase10_direct_closed_saddle_incidence_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_closed_saddle_incidence.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_closed_saddle_incidence_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_closed_saddle_incidence_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_closed_saddle_incidence_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_closed_saddle_incidence_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_positive_facet_locator_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_positive_facet_locator.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_positive_facet_locator_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_positive_facet_locator>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_positive_facet_locator_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_positive_facet_locator_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_positive_facet_locator_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_positive_facet_prefix_sweep_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_positive_facet_prefix_sweep.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_positive_facet_prefix_sweep_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_positive_facet_prefix_sweep>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_positive_facet_prefix_sweep_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_positive_facet_prefix_sweep_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_positive_facet_prefix_sweep_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_facet_descent_step_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_facet_descent_step.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_facet_descent_step_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_facet_descent_step>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_facet_descent_step_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_facet_descent_step_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_facet_descent_step_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_facet_descent_closure_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_facet_descent_closure.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_facet_descent_closure_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_facet_descent_closure>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_facet_descent_closure_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_facet_descent_closure_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_facet_descent_closure_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_first_incidence_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_first_incidence.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_first_incidence_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_first_incidence>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_first_incidence_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_first_incidence_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_first_incidence_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_gateway_candidate_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_gateway_candidate.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_gateway_candidate_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_gateway_candidate_journal>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_gateway_candidate_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_gateway_candidate_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_gateway_candidate_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_gateway_candidate_localization_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_gateway_candidate_localization.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_gateway_candidate_localization_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_gateway_candidate_localization>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_gateway_candidate_localization_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_gateway_candidate_localization_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_gateway_candidate_localization_configuration_command
)
set(
  _morsehgp3d_phase10_direct_sparse_gateway_clock_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_sparse_gateway_clock.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_sparse_gateway_clock_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_direct_sparse_gateway_clock>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_sparse_gateway_clock_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_sparse_gateway_clock_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_sparse_gateway_clock_configuration_command
)
set(
  _morsehgp3d_phase10_direct_frozen_root_quotient_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_frozen_root_quotient.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_frozen_root_quotient_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_hierarchy_direct_frozen_root_quotient_tests>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_frozen_root_quotient_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_frozen_root_quotient_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_frozen_root_quotient_configuration_command
)
set(
  _morsehgp3d_phase10_direct_k1_forest_configuration_command
  "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase10_direct_k1_forest.py"
  "${PROJECT_SOURCE_DIR}"
)
if(CMAKE_NM AND NOT MSVC)
  list(
    APPEND
    _morsehgp3d_phase10_direct_k1_forest_configuration_command
    --binary
    "$<TARGET_FILE:morsehgp3d_hierarchy_direct_k1_forest_journal_tests>"
    --nm
    "${CMAKE_NM}"
  )
endif()
add_test(
  NAME morsehgp3d.phase10_direct_k1_forest_configuration
  COMMAND
    ${_morsehgp3d_phase10_direct_k1_forest_configuration_command}
)
unset(
  _morsehgp3d_phase10_direct_k1_forest_configuration_command
)
add_test(
  NAME morsehgp3d.phase9_pair_aabb_corner_correlation
  COMMAND
    "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/../tests/oracle/test_pair_aabb_corner_correlation.py"
)
add_test(
  NAME morsehgp3d.phase9_higher_support_aabb_corner_regression
  COMMAND
    "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/../tests/oracle/test_higher_support_aabb_corner_regression.py"
)
add_test(
  NAME morsehgp3d.phase5_k1_boruvka_qualification_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase5_k1_boruvka_qualification.py"
    "${PROJECT_SOURCE_DIR}"
)
add_test(
  NAME morsehgp3d.phase5_k1_boruvka_morton_work_profile_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase5_k1_boruvka_morton_work_profile.py"
    "${PROJECT_SOURCE_DIR}"
    --executable
    "$<TARGET_FILE:morsehgp3d_gpu_k1_boruvka_morton_work_profile_host>"
)
add_test(
  NAME morsehgp3d.phase5_k1_boruvka_exact_search_work_profile_configuration
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../configuration/check_phase5_k1_boruvka_exact_search_work_profile.py"
    "${PROJECT_SOURCE_DIR}"
    --executable
    "$<TARGET_FILE:morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host>"
)
add_test(
  NAME morsehgp3d.spatial_gpu_reference_host_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_reference.py"
    "$<TARGET_FILE:morsehgp3d_gpu_spatial_reference_replay_host>"
)
set_tests_properties(
  morsehgp3d.spatial_gpu_reference_host_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.spatial_gpu_bounds_host_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_bounds.py"
    "$<TARGET_FILE:morsehgp3d_gpu_spatial_bounds_replay_host>"
)
set_tests_properties(
  morsehgp3d.spatial_gpu_bounds_host_differential
  PROPERTIES TIMEOUT 300
)
add_test(
  NAME morsehgp3d.spatial_gpu_lbvh_host_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_lbvh.py"
    "$<TARGET_FILE:morsehgp3d_gpu_spatial_lbvh_replay_host>"
)
set_tests_properties(
  morsehgp3d.spatial_gpu_lbvh_host_differential
  PROPERTIES TIMEOUT 300
)
if(TARGET morsehgp3d_gpu_spatial_reference_replay)
  add_test(
    NAME morsehgp3d.spatial_gpu_reference_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_reference.py"
      "$<TARGET_FILE:morsehgp3d_gpu_spatial_reference_replay>"
  )
  set_tests_properties(
    morsehgp3d.spatial_gpu_reference_differential
    PROPERTIES TIMEOUT 300
  )
endif()
if(TARGET morsehgp3d_gpu_spatial_bounds_replay)
  add_test(
    NAME morsehgp3d.spatial_gpu_bounds_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_bounds.py"
      "$<TARGET_FILE:morsehgp3d_gpu_spatial_bounds_replay>"
  )
  set_tests_properties(
    morsehgp3d.spatial_gpu_bounds_differential
    PROPERTIES TIMEOUT 300
  )
endif()
if(TARGET morsehgp3d_gpu_spatial_lbvh_replay)
  add_test(
    NAME morsehgp3d.spatial_gpu_lbvh_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_spatial_gpu_lbvh.py"
      "$<TARGET_FILE:morsehgp3d_gpu_spatial_lbvh_replay>"
  )
  set_tests_properties(
    morsehgp3d.spatial_gpu_lbvh_differential
    PROPERTIES TIMEOUT 300
  )
endif()
if(TARGET morsehgp3d_gpu_k1_boruvka_replay)
  add_test(
    NAME morsehgp3d.gpu_k1_boruvka_replay
    COMMAND morsehgp3d_gpu_k1_boruvka_replay
  )
  set_tests_properties(
    morsehgp3d.gpu_k1_boruvka_replay
    PROPERTIES TIMEOUT 300
  )
endif()
if(TARGET morsehgp3d_gpu_k1_boruvka_full_replay)
  add_test(
    NAME morsehgp3d.gpu_k1_boruvka_full_replay
    COMMAND morsehgp3d_gpu_k1_boruvka_full_replay
  )
  set_tests_properties(
    morsehgp3d.gpu_k1_boruvka_full_replay
    PROPERTIES TIMEOUT 300
  )
endif()
add_test(
  NAME morsehgp3d.exact_contract_differential
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_exact_types.py"
    "$<TARGET_FILE:morsehgp3d_exact_types_dump>"
)

if(TARGET morsehgp3d_replay_predicate)
  add_test(
    NAME morsehgp3d.predicate_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.predicate_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.predicate_decision_batch_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_decision_batch.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.predicate_campaign_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_predicate_campaign.py"
      "${PROJECT_SOURCE_DIR}/tools/predicate_campaign.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../campaigns/phase2a_predicates_v1.json"
      "${PROJECT_SOURCE_DIR}/.."
  )
  add_test(
    NAME morsehgp3d.phase2b_predicate_campaign_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_phase2b_predicate_campaign.py"
      "${PROJECT_SOURCE_DIR}/tools/phase2b_predicate_campaign.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      --source
      "${PROJECT_SOURCE_DIR}/.."
  )
  add_test(
    NAME morsehgp3d.affine_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.affine_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.affine_adaptive_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_adaptive_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.affine_adaptive_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_affine_adaptive_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.center_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_center_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.center_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_center_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.support_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_adaptive_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_adaptive_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.support_adaptive_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_support_adaptive_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.level_replay_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_replay.py"
      "${PROJECT_SOURCE_DIR}/tools/replay_predicate.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
      "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/predicates"
  )
  add_test(
    NAME morsehgp3d.level_random_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_random.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
  add_test(
    NAME morsehgp3d.level_checkpoint_differential
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_level_checkpoint.py"
      "${PROJECT_SOURCE_DIR}/tools/level_run_checkpoint.py"
      "$<TARGET_FILE:morsehgp3d_replay_predicate>"
  )
endif()
