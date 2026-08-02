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
  morsehgp3d_hierarchy_yao48_emst_tests
  test_hierarchy_yao48_emst.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_yao48_emst_tests
  PRIVATE morsehgp3d::hierarchy
)
morsehgp3d_apply_sanitizers(morsehgp3d_hierarchy_yao48_emst_tests)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_yao48_emst_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_yao48_emst_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
  test_hierarchy_exact_lbvh_yao48_emst.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
  PRIVATE morsehgp3d::hierarchy
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
  test_hierarchy_yao48_ranked_pair_candidates.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
  PRIVATE morsehgp3d::yao48_ranked_pair_candidates_reference
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
  test_hierarchy_exact_ranked_diametral_pair_catalog_reference.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
  PRIVATE morsehgp3d::exact_ranked_diametral_pair_catalog_reference
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_capped_distinct_point_coverage_tests
  test_hierarchy_capped_distinct_point_coverage.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_capped_distinct_point_coverage_tests
  PRIVATE morsehgp3d::capped_distinct_point_coverage
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_capped_distinct_point_coverage_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_capped_distinct_point_coverage_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_at_least20_stream_view_tests
  test_hierarchy_direct_at_least20_stream_view.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_at_least20_stream_view_tests
  PRIVATE morsehgp3d::direct_at_least20_stream_view
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_at_least20_stream_view_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_at_least20_stream_view_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests
  test_hierarchy_exact_q3_gabriel_triangle_oracle.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests
  PRIVATE morsehgp3d::exact_q3_gabriel_triangle_oracle
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_silent_pair_gateway_tests
  test_hierarchy_silent_pair_gateway.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_silent_pair_gateway_tests
  PRIVATE morsehgp3d::silent_pair_gateway
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_silent_pair_gateway_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_silent_pair_gateway_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
  test_hierarchy_silent_pair_gateway_gamma2_differential.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
  PRIVATE morsehgp3d::silent_pair_gateway_gamma2_differential
)
if(MSVC)
else()
  target_compile_options(
    morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

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
  morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests
  test_hierarchy_exact_k2_k1_hierarchy.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests
  PRIVATE morsehgp3d::hierarchy
)

add_executable(
  morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
  test_hierarchy_direct_morse_k2_k1_target_authority.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
  PRIVATE morsehgp3d::direct_morse_k2_k1_target_authority
)

add_executable(
  morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
  test_hierarchy_direct_morse_m1_o5_death_accounting.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
  PRIVATE morsehgp3d::direct_morse_m1_o5_death_accounting
)

add_executable(
  morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
  test_hierarchy_direct_morse_gamma_carrier_conformance.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
  PRIVATE morsehgp3d::direct_morse_gamma_carrier_conformance
)

add_executable(
  morsehgp3d_hierarchy_size_condensed_hierarchy_tests
  test_hierarchy_size_condensed_hierarchy.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_size_condensed_hierarchy_tests
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
  morsehgp3d_anchored_pair_witness_bank_tests
  test_anchored_pair_witness_bank.cpp
)
target_link_libraries(
  morsehgp3d_anchored_pair_witness_bank_tests
  PRIVATE morsehgp3d::anchored_pair_witness_bank
)
if(MSVC)
  target_compile_options(
    morsehgp3d_anchored_pair_witness_bank_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_anchored_pair_witness_bank_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_grouped_anchored_pair_certificate_tests
  test_grouped_anchored_pair_certificate.cpp
)
target_link_libraries(
  morsehgp3d_grouped_anchored_pair_certificate_tests
  PRIVATE morsehgp3d::sparse_anchored_pair_session
)
if(MSVC)
  target_compile_options(
    morsehgp3d_grouped_anchored_pair_certificate_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_grouped_anchored_pair_certificate_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_exact_block_rank_prune_receipt_tests
  test_exact_block_rank_prune_receipt.cpp
)
target_link_libraries(
  morsehgp3d_exact_block_rank_prune_receipt_tests
  PRIVATE morsehgp3d::pair_support
)
if(MSVC)
  target_compile_options(
    morsehgp3d_exact_block_rank_prune_receipt_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_exact_block_rank_prune_receipt_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_morton_grouped_anchored_pair_schedule_tests
  test_morton_grouped_anchored_pair_schedule.cpp
)
target_link_libraries(
  morsehgp3d_morton_grouped_anchored_pair_schedule_tests
  PRIVATE
    morsehgp3d::anchored_pair_witness_bank
    morsehgp3d::anchored_pair_candidate_classifier
)
if(MSVC)
  target_compile_options(
    morsehgp3d_morton_grouped_anchored_pair_schedule_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_morton_grouped_anchored_pair_schedule_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_morton_triangular_block_pair_schedule_tests
  test_morton_triangular_block_pair_schedule.cpp
)
target_link_libraries(
  morsehgp3d_morton_triangular_block_pair_schedule_tests
  PRIVATE morsehgp3d::morton_triangular_block_pair_schedule
)
if(MSVC)
  target_compile_options(
    morsehgp3d_morton_triangular_block_pair_schedule_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_morton_triangular_block_pair_schedule_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
  test_gpu_exact_pair_block_frontier.cpp
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
  PRIVATE morsehgp3d::gpu_exact_pair_block_frontier_host_contract
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
  test_gpu_exact_pair_block_witness_cuda.cpp
  fake_gpu_exact_pair_block_witness_cuda_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_witness_cuda_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
  test_gpu_exact_pair_block_antichain_cuda.cpp
  fake_gpu_exact_pair_block_witness_cuda_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_antichain_cuda_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
  test_gpu_exact_pair_block_transactional_frontier_cuda.cpp
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_transactional_frontier_cuda_host_contract
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
  test_gpu_exact_pair_block_transactional_page_plan.cpp
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_transactional_frontier_cuda_host_contract
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
  test_gpu_exact_pair_block_transactional_paged_host_scheduler.cpp
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
  test_gpu_exact_pair_block_transactional_frontier_resident_cuda.cpp
  fake_gpu_exact_pair_block_transactional_frontier_resident_cuda_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
  PRIVATE
    morsehgp3d::gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
  test_gpu_exact_pair_block_automatic_prune_recipe_catalog.cpp
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
  PRIVATE morsehgp3d::gpu_exact_pair_block_automatic_prune_recipe_catalog
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
  test_gpu_exact_pair_block_to_direct_pair_terminal.cpp
  fake_gpu_exact_pair_block_transactional_frontier_resident_cuda_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
  PRIVATE
    morsehgp3d::exact_pair_block_to_direct_pair_terminal
    morsehgp3d::gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract
    morsehgp3d::anchored_pair_candidate_classifier
    morsehgp3d::direct_morse_forest_source_authority
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_anchored_pair_candidate_classifier_tests
  test_anchored_pair_candidate_classifier.cpp
)
target_link_libraries(
  morsehgp3d_anchored_pair_candidate_classifier_tests
  PRIVATE morsehgp3d::anchored_pair_candidate_classifier
)
if(MSVC)
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_classifier_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_classifier_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_sparse_anchored_pair_session_tests
  test_sparse_anchored_pair_session.cpp
)
target_link_libraries(
  morsehgp3d_sparse_anchored_pair_session_tests
  PRIVATE morsehgp3d::sparse_anchored_pair_session
)
if(MSVC)
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_session_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_session_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_anchored_pair_candidate_recertifier_tests
  test_anchored_pair_candidate_recertifier.cpp
  "${PROJECT_SOURCE_DIR}/src/cpu/hierarchy/anchored_pair_candidate_recertifier.cpp"
)
target_link_libraries(
  morsehgp3d_anchored_pair_candidate_recertifier_tests
  PRIVATE morsehgp3d::anchored_pair_witness_bank
)
target_compile_features(
  morsehgp3d_anchored_pair_candidate_recertifier_tests
  PRIVATE cxx_std_20
)
if(MSVC)
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_recertifier_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_anchored_pair_candidate_recertifier_tests
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
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
  test_gpu_exact_higher_support_product_cuda.cpp
  fake_gpu_exact_higher_support_product_cuda_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
  PRIVATE
    morsehgp3d::gpu_exact_higher_support_product_cuda_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()

add_executable(
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
  test_gpu_exact_higher_support_product_fixed.cpp
)
target_include_directories(
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
  PRIVATE morsehgp3d::higher_support
)
target_compile_features(
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_higher_support_product_fixed_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_higher_support_product_fixed_tests
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
  morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
  test_hierarchy_exact_low_order_gabriel_skeleton.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
  PRIVATE morsehgp3d::direct_support_terminal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
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
  morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
  test_hierarchy_direct_normalized_h0_source_plan.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
  PRIVATE
    morsehgp3d::direct_normalized_h0_incidence_reduction_authority
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
  test_hierarchy_direct_normalized_vertical_square_receipt.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
  PRIVATE morsehgp3d::direct_normalized_vertical_square_receipt
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
  test_hierarchy_direct_sparse_successive_incidence.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
  PRIVATE morsehgp3d::direct_sparse_successive_incidence
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
  test_hierarchy_direct_sparse_successive_incidence_star_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
  PRIVATE morsehgp3d::direct_sparse_successive_incidence_star_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
  test_hierarchy_direct_sparse_unified_level_plan.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
  PRIVATE morsehgp3d::direct_sparse_unified_level_plan
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
  test_hierarchy_direct_star_h0_retraction_differential.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
  PRIVATE morsehgp3d::direct_star_h0_retraction_differential
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
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
  morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
  test_hierarchy_direct_frozen_incidence_quotient.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
  PRIVATE morsehgp3d::direct_frozen_incidence_quotient
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
  test_hierarchy_direct_frozen_incidence_hgp_action_plan.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
  PRIVATE morsehgp3d::direct_frozen_incidence_hgp_action_plan
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
  test_hierarchy_direct_frozen_unified_incidence_batch.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
  PRIVATE morsehgp3d::direct_frozen_unified_incidence_batch
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
  test_hierarchy_direct_morse_unified_resident_session.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
  PRIVATE morsehgp3d::direct_morse_unified_resident_session
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
  test_hierarchy_direct_morse_resident_at_least20_adapter.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
  PRIVATE morsehgp3d::direct_morse_resident_at_least20_adapter
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
  test_hierarchy_normalized_exact_hartigan_level_manifest.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
  PRIVATE morsehgp3d::normalized_exact_hartigan_level_manifest
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
  test_hierarchy_direct_k1_boruvka_closed_cut_session.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
  PRIVATE morsehgp3d::direct_k1_boruvka_closed_cut_session
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
  test_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
  PRIVATE morsehgp3d::direct_morse_resident_k2_k1_closed_cut_bridge
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
  test_hierarchy_direct_morse_resident_all_orders_vertical_bridge.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
  PRIVATE morsehgp3d::direct_morse_resident_all_orders_vertical_bridge
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_relative_morse_boruvka_tests
  test_hierarchy_relative_morse_boruvka.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_relative_morse_boruvka_tests
  PRIVATE
    morsehgp3d::hierarchy
    morsehgp3d::relative_morse_boruvka
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_relative_morse_boruvka_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_relative_morse_boruvka_tests
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
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
  test_hierarchy_direct_morse_forest_carrier_cut_index.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
  PRIVATE morsehgp3d::direct_morse_forest_carrier_cut_index
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
  test_hierarchy_direct_morse_forest_root_coverage_index.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
  PRIVATE morsehgp3d::direct_morse_forest_root_coverage_index
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
  test_hierarchy_direct_morse_forest_carrier_cut_replay_session.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
  PRIVATE morsehgp3d::direct_morse_forest_carrier_cut_replay_session
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
  test_hierarchy_direct_morse_forest_carrier_cut_closure_adapter.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
  PRIVATE morsehgp3d::direct_morse_forest_carrier_cut_closure_adapter
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
  test_hierarchy_direct_morse_forest_reducer.cpp
  fake_gpu_phase14_facet_top_k_proposal_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/direct_sparse_facet_top_k_proposal.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/direct_sparse_facet_top_k_integrated_adapter.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
  PRIVATE
    morsehgp3d::direct_morse_forest_reducer
    morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
    morsehgp3d::spatial
    Threads::Threads
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_reducer_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
  test_hierarchy_direct_morse_terminal_reducer_source_bridge.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
  PRIVATE
    morsehgp3d::direct_morse_forest_source_authority
    morsehgp3d::direct_morse_terminal_forest_reduction
    morsehgp3d::direct_morse_vertical_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_vertical_journal_tests
  test_hierarchy_direct_morse_vertical_journal.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_vertical_journal_tests
  PRIVATE morsehgp3d::direct_morse_vertical_journal
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_journal_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_journal_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
  test_hierarchy_direct_morse_vertical_target_proposal_adapter.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
  PRIVATE morsehgp3d::direct_morse_vertical_target_proposal_adapter
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
  test_hierarchy_direct_morse_vertical_target_proposal_pipeline.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
  PRIVATE morsehgp3d::direct_morse_vertical_target_proposal_pipeline
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

add_executable(
  morsehgp3d_hierarchy_direct_morse_budget_tests
  test_hierarchy_direct_morse_budget.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_direct_morse_budget_tests
  PRIVATE morsehgp3d::direct_morse_budget
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_budget_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_budget_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )
endif()

if(UNIX)
  add_executable(
    morsehgp3d_hierarchy_atomic_linear_run_store_tests
    test_hierarchy_atomic_linear_run_store.cpp
  )
  target_link_libraries(
    morsehgp3d_hierarchy_atomic_linear_run_store_tests
    PRIVATE morsehgp3d::atomic_linear_run_store
  )
  target_compile_options(
    morsehgp3d_hierarchy_atomic_linear_run_store_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_sparse_anchored_pair_chunk_run_tests
    test_sparse_anchored_pair_chunk_run.cpp
  )
  target_link_libraries(
    morsehgp3d_sparse_anchored_pair_chunk_run_tests
    PRIVATE morsehgp3d::sparse_anchored_pair_chunk_run
  )
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_chunk_run_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_sparse_anchored_pair_h0_run_tests
    test_sparse_anchored_pair_h0_run.cpp
  )
  target_link_libraries(
    morsehgp3d_sparse_anchored_pair_h0_run_tests
    PRIVATE morsehgp3d::sparse_anchored_pair_h0_run
  )
  target_compile_options(
    morsehgp3d_sparse_anchored_pair_h0_run_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_sparse_direct_h0_candidate_merge_tests
    test_sparse_direct_h0_candidate_merge.cpp
  )
  target_link_libraries(
    morsehgp3d_sparse_direct_h0_candidate_merge_tests
    PRIVATE
      morsehgp3d::direct_support_terminal
      morsehgp3d::sparse_direct_h0_candidate_merge
  )
  target_compile_options(
    morsehgp3d_sparse_direct_h0_candidate_merge_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_sparse_higher_support_h0_chunk_run_tests
    test_sparse_higher_support_h0_chunk_run.cpp
  )
  target_link_libraries(
    morsehgp3d_sparse_higher_support_h0_chunk_run_tests
    PRIVATE morsehgp3d::sparse_higher_support_h0_chunk_run
  )
  target_compile_options(
    morsehgp3d_sparse_higher_support_h0_chunk_run_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_sparse_direct_h0_atomic_run_reader_tests
    test_sparse_direct_h0_atomic_run_reader.cpp
  )
  target_link_libraries(
    morsehgp3d_sparse_direct_h0_atomic_run_reader_tests
    PRIVATE morsehgp3d::sparse_direct_h0_atomic_run_reader
  )
  target_compile_options(
    morsehgp3d_sparse_direct_h0_atomic_run_reader_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
  )

  add_executable(
    morsehgp3d_hierarchy_direct_morse_chunk_run_tests
    test_hierarchy_direct_morse_chunk_run.cpp
  )
  target_link_libraries(
    morsehgp3d_hierarchy_direct_morse_chunk_run_tests
    PRIVATE
      morsehgp3d::direct_morse_chunk_run
      morsehgp3d::direct_morse_forest_reducer
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_chunk_run_tests
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

add_executable(
  morsehgp3d_hierarchy_hartigan_all_orders_tests
  test_hierarchy_hartigan_all_orders.cpp
)
target_link_libraries(
  morsehgp3d_hierarchy_hartigan_all_orders_tests
  PRIVATE morsehgp3d::hierarchy
)
if(MSVC)
  target_compile_options(
    morsehgp3d_hierarchy_hartigan_all_orders_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_hierarchy_hartigan_all_orders_tests
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
add_executable(
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
  test_gpu_direct_sparse_facet_top_k_proposal_context.cpp
  fake_gpu_phase14_facet_top_k_proposal_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/direct_sparse_facet_top_k_proposal.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/direct_sparse_facet_top_k_integrated_adapter.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_morton_lbvh_build_context_tests
  test_gpu_morton_lbvh_build_context.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
  test_gpu_morton_yao48_device_tiled_pair_frontier.cpp
  fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
  test_gpu_morton_yao48_ranked_pair_tile_classifier.cpp
  fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.cpp
  fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
  test_gpu_morton_yao48_ranked_pair_h0_projection.cpp
  fake_gpu_morton_yao48_ranked_pair_h0_projection_launchers.cpp
  fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.cpp
  fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_exact_pair_level_fixed_tests
  test_gpu_exact_pair_level_fixed.cpp
)
add_executable(
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
  test_gpu_exact_closed_rank23_pair_terminal_catalog.cpp
  fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.cpp
  fake_gpu_morton_yao48_device_tiled_pair_frontier_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
  test_gpu_anchored_pair_candidate_proposal_context.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/anchored_pair_candidate_proposal.cpp"
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
add_executable(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
  test_gpu_ranked_diametral_pair_catalog_context.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
  PRIVATE
    morsehgp3d::gpu_ranked_diametral_pair_catalog_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_exact_diametral_phi_context_tests
  test_gpu_exact_diametral_phi_context.cpp
  fake_gpu_exact_diametral_phi_launchers.cpp
)
target_include_directories(
  morsehgp3d_gpu_exact_diametral_phi_context_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_exact_diametral_phi_context_tests
  PRIVATE
    morsehgp3d::gpu_exact_diametral_phi_host_contract
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_diametral_phi_context_tests PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_diametral_phi_context_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_diametral_phi_context_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_diametral_phi_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
  test_gpu_morton_yao48_pair_frontier.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_pair_frontier_host_reference
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_pair_frontier_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_pair_frontier_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
  test_gpu_morton_yao48_tiled_pair_frontier.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_tiled_pair_frontier_host_reference
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
  test_gpu_morton_yao48_radial_subtree_filter.cpp
  fake_gpu_morton_yao48_radial_subtree_filter_launchers.cpp
  fake_gpu_phase14_morton_lbvh_build_launchers.cpp
  "${PROJECT_SOURCE_DIR}/src/gpu/morton_lbvh_build.cpp"
)
target_include_directories(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_radial_subtree_filter_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
add_executable(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
  test_gpu_morton_yao48_seed_work_profile.cpp
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
  PRIVATE morsehgp3d::gpu_morton_yao48_seed_work_profile_support
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_tests PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_seed_work_profile_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_morton_lbvh_build_context_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_morton_lbvh_build_context_tests
  PRIVATE morsehgp3d::spatial Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_lbvh_build_context_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_lbvh_build_context_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_lbvh_build_context_tests PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_lbvh_build_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
)
if(MSVC)
target_compile_options(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_ranked_pair_tile_classifier_host_contract
    morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
  PRIVATE
    morsehgp3d::gpu_morton_yao48_ranked_pair_h0_projection_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_exact_pair_level_fixed_tests
  PRIVATE "${PROJECT_SOURCE_DIR}/src/cuda"
)
target_link_libraries(
  morsehgp3d_gpu_exact_pair_level_fixed_tests
  PRIVATE morsehgp3d::exact
)
target_compile_features(
  morsehgp3d_gpu_exact_pair_level_fixed_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_pair_level_fixed_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_pair_level_fixed_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_pair_level_fixed_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
  PRIVATE
    morsehgp3d::gpu_exact_closed_rank23_pair_terminal_catalog_host_contract
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
target_include_directories(
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/cuda"
    "${PROJECT_SOURCE_DIR}/src/gpu"
)
target_link_libraries(
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
  PRIVATE
    morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
    morsehgp3d::spatial
    Threads::Threads
)
target_compile_features(
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
  PRIVATE cxx_std_20
)
morsehgp3d_apply_sanitizers(
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
)
if(MSVC)
  target_compile_options(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
    PRIVATE /W4 /WX
  )
else()
  target_compile_options(
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
endif()
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
  morsehgp3d_hierarchy_yao48_emst_tests
  morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
  morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
  morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
  morsehgp3d_hierarchy_capped_distinct_point_coverage_tests
  morsehgp3d_hierarchy_direct_at_least20_stream_view_tests
  morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests
  morsehgp3d_hierarchy_silent_pair_gateway_tests
  morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
  morsehgp3d_hierarchy_k1_forest_tests
  morsehgp3d_hierarchy_miniball_tests
  morsehgp3d_hierarchy_gamma_tests
  morsehgp3d_hierarchy_critical_arm_gamma_tests
  morsehgp3d_hierarchy_gamma_transition_tests
  morsehgp3d_hierarchy_reduced_gamma_batch_tests
  morsehgp3d_hierarchy_reduced_gamma_history_tests
  morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests
  morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
  morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
  morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
  morsehgp3d_hierarchy_size_condensed_hierarchy_tests
  morsehgp3d_hierarchy_reduced_gamma_cut_tests
  morsehgp3d_hierarchy_critical_event_gamma_overlay_tests
  morsehgp3d_hierarchy_critical_catalog_tests
  morsehgp3d_depth_zero_natural_support_tests
  morsehgp3d_hierarchy_pair_support_stream_tests
  morsehgp3d_anchored_pair_witness_bank_tests
  morsehgp3d_grouped_anchored_pair_certificate_tests
  morsehgp3d_exact_block_rank_prune_receipt_tests
  morsehgp3d_morton_grouped_anchored_pair_schedule_tests
  morsehgp3d_morton_triangular_block_pair_schedule_tests
  morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
  morsehgp3d_anchored_pair_candidate_classifier_tests
  morsehgp3d_sparse_anchored_pair_session_tests
  morsehgp3d_hierarchy_higher_support_product_tests
  morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
  morsehgp3d_gpu_exact_higher_support_product_fixed_tests
  morsehgp3d_hierarchy_higher_support_stream_tests
  morsehgp3d_hierarchy_direct_support_differential_tests
  morsehgp3d_hierarchy_direct_support_terminal_tests
  morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
  morsehgp3d_hierarchy_direct_morse_event_journal_tests
  morsehgp3d_hierarchy_direct_saddle_arm_seed_journal_tests
  morsehgp3d_hierarchy_direct_closed_saddle_incidence_journal_tests
  morsehgp3d_hierarchy_direct_sparse_positive_facet_locator_tests
  morsehgp3d_hierarchy_direct_sparse_positive_facet_prefix_sweep_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_step_tests
  morsehgp3d_hierarchy_direct_sparse_facet_descent_closure_tests
  morsehgp3d_hierarchy_direct_sparse_first_incidence_tests
  morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
  morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
  morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
  morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
  morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_journal_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_candidate_localization_tests
  morsehgp3d_hierarchy_direct_sparse_gateway_clock_tests
  morsehgp3d_hierarchy_direct_frozen_root_quotient_tests
  morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
  morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
  morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
  morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
  morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
  morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
  morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
  morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
  morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
  morsehgp3d_hierarchy_relative_morse_boruvka_tests
  morsehgp3d_hierarchy_direct_k1_forest_journal_tests
  morsehgp3d_hierarchy_direct_morse_forest_journal_tests
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
  morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
  morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
  morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
  morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
  morsehgp3d_hierarchy_direct_morse_vertical_journal_tests
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
  morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
  morsehgp3d_hierarchy_direct_morse_budget_tests
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
  morsehgp3d_hierarchy_hartigan_all_orders_tests
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
  morsehgp3d_gpu_exact_diametral_phi_context_tests
  morsehgp3d_gpu_spatial_reference_replay_host
  morsehgp3d_gpu_spatial_bounds_context_tests
  morsehgp3d_gpu_spatial_bounds_replay_host
  morsehgp3d_gpu_spatial_lbvh_context_tests
  morsehgp3d_gpu_spatial_lbvh_replay_host
  morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
  morsehgp3d_gpu_k1_boruvka_context_tests
  morsehgp3d_gpu_k1_boruvka_exact_search_tests
  morsehgp3d_gpu_k1_boruvka_seeded_chain_tests
  morsehgp3d_gpu_k1_boruvka_hierarchy_tests
  morsehgp3d_gpu_k1_boruvka_exact_search_work_profile_host
  morsehgp3d_gpu_k1_boruvka_replay_host
  morsehgp3d_gpu_k1_boruvka_full_replay_host
  morsehgp3d_gpu_k1_boruvka_morton_work_profile_host
  morsehgp3d_anchored_pair_candidate_recertifier_tests
  morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
  morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
  morsehgp3d_gpu_morton_yao48_pair_frontier_tests
  morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
  morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
  morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
  morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
  morsehgp3d_gpu_exact_pair_level_fixed_tests
  morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
  morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
  morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
  morsehgp3d_gpu_morton_lbvh_build_context_tests
)
if(UNIX)
  list(
    APPEND _morsehgp3d_cpu_test_targets
    morsehgp3d_hierarchy_atomic_linear_run_store_tests
    morsehgp3d_sparse_anchored_pair_chunk_run_tests
    morsehgp3d_sparse_anchored_pair_h0_run_tests
    morsehgp3d_sparse_direct_h0_candidate_merge_tests
    morsehgp3d_sparse_higher_support_h0_chunk_run_tests
    morsehgp3d_sparse_direct_h0_atomic_run_reader_tests
    morsehgp3d_hierarchy_direct_morse_chunk_run_tests
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
    morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
    PRIVATE /W4 /WX
  )
  target_compile_options(
    morsehgp3d_hierarchy_size_condensed_hierarchy_tests PRIVATE /W4 /WX
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
    morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
    PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Werror
  )
  target_compile_options(
    morsehgp3d_hierarchy_size_condensed_hierarchy_tests
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
  NAME morsehgp3d.hierarchy_yao48_emst
  COMMAND morsehgp3d_hierarchy_yao48_emst_tests
)
add_test(
  NAME morsehgp3d.hierarchy_exact_lbvh_yao48_emst
  COMMAND morsehgp3d_hierarchy_exact_lbvh_yao48_emst_tests
)
add_test(
  NAME morsehgp3d.hierarchy_yao48_ranked_pair_candidates
  COMMAND morsehgp3d_hierarchy_yao48_ranked_pair_candidates_tests
)
add_test(
  NAME morsehgp3d.hierarchy_exact_ranked_diametral_pair_catalog_reference
  COMMAND
    morsehgp3d_hierarchy_exact_ranked_diametral_pair_catalog_reference_tests
)
add_test(
  NAME morsehgp3d.hierarchy_capped_distinct_point_coverage
  COMMAND morsehgp3d_hierarchy_capped_distinct_point_coverage_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_at_least20_stream_view
  COMMAND morsehgp3d_hierarchy_direct_at_least20_stream_view_tests
)
add_test(
  NAME morsehgp3d.hierarchy_exact_q3_gabriel_triangle_oracle
  COMMAND morsehgp3d_hierarchy_exact_q3_gabriel_triangle_oracle_tests
)
add_test(
  NAME morsehgp3d.hierarchy_silent_pair_gateway
  COMMAND morsehgp3d_hierarchy_silent_pair_gateway_tests
)
add_test(
  NAME morsehgp3d.hierarchy_silent_pair_gateway_gamma2_differential
  COMMAND
    morsehgp3d_hierarchy_silent_pair_gateway_gamma2_differential_tests
)
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
  NAME morsehgp3d.hierarchy_exact_k2_k1_hierarchy
  COMMAND morsehgp3d_hierarchy_exact_k2_k1_hierarchy_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_k2_k1_target_authority
  COMMAND morsehgp3d_hierarchy_direct_morse_k2_k1_target_authority_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_m1_o5_death_accounting
  COMMAND morsehgp3d_hierarchy_direct_morse_m1_o5_death_accounting_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_gamma_carrier_conformance
  COMMAND morsehgp3d_hierarchy_direct_morse_gamma_carrier_conformance_tests
)
add_test(
  NAME morsehgp3d.hierarchy_size_condensed_hierarchy
  COMMAND morsehgp3d_hierarchy_size_condensed_hierarchy_tests
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
  NAME morsehgp3d.anchored_pair_witness_bank
  COMMAND morsehgp3d_anchored_pair_witness_bank_tests
)
add_test(
  NAME morsehgp3d.grouped_anchored_pair_certificate
  COMMAND morsehgp3d_grouped_anchored_pair_certificate_tests
)
add_test(
  NAME morsehgp3d.exact_block_rank_prune_receipt
  COMMAND morsehgp3d_exact_block_rank_prune_receipt_tests
)
add_test(
  NAME morsehgp3d.morton_grouped_anchored_pair_schedule
  COMMAND morsehgp3d_morton_grouped_anchored_pair_schedule_tests
)
add_test(
  NAME morsehgp3d.morton_triangular_block_pair_schedule
  COMMAND morsehgp3d_morton_triangular_block_pair_schedule_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_frontier_host_contract
  COMMAND morsehgp3d_gpu_exact_pair_block_frontier_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_witness_cuda_host_contract
  COMMAND morsehgp3d_gpu_exact_pair_block_witness_cuda_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_antichain_cuda_host_contract
  COMMAND morsehgp3d_gpu_exact_pair_block_antichain_cuda_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_transactional_frontier_cuda_host_contract
  COMMAND morsehgp3d_gpu_exact_pair_block_transactional_frontier_cuda_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_transactional_page_plan
  COMMAND morsehgp3d_gpu_exact_pair_block_transactional_page_plan_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_transactional_paged_host_scheduler
  COMMAND morsehgp3d_gpu_exact_pair_block_transactional_paged_host_scheduler_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract
  COMMAND morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_automatic_prune_recipe_catalog
  COMMAND morsehgp3d_gpu_exact_pair_block_automatic_prune_recipe_catalog_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_pair_block_to_direct_pair_terminal
  COMMAND morsehgp3d_gpu_exact_pair_block_to_direct_pair_terminal_tests
)
add_test(
  NAME morsehgp3d.anchored_pair_candidate_classifier
  COMMAND morsehgp3d_anchored_pair_candidate_classifier_tests
)
add_test(
  NAME morsehgp3d.sparse_anchored_pair_session
  COMMAND morsehgp3d_sparse_anchored_pair_session_tests
)
add_test(
  NAME morsehgp3d.hierarchy_higher_support_product
  COMMAND morsehgp3d_hierarchy_higher_support_product_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_higher_support_product_cuda_host_contract
  COMMAND morsehgp3d_gpu_exact_higher_support_product_cuda_host_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_higher_support_product_fixed
  COMMAND morsehgp3d_gpu_exact_higher_support_product_fixed_tests
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
  NAME morsehgp3d.hierarchy_exact_low_order_gabriel_skeleton
  COMMAND morsehgp3d_hierarchy_exact_low_order_gabriel_skeleton_tests
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
  NAME morsehgp3d.hierarchy_direct_normalized_h0_source_plan
  COMMAND morsehgp3d_hierarchy_direct_normalized_h0_source_plan_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_normalized_vertical_square_receipt
  COMMAND morsehgp3d_hierarchy_direct_normalized_vertical_square_receipt_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_successive_incidence
  COMMAND morsehgp3d_hierarchy_direct_sparse_successive_incidence_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_successive_incidence_star_journal
  COMMAND morsehgp3d_hierarchy_direct_sparse_successive_incidence_star_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_sparse_unified_level_plan
  COMMAND morsehgp3d_hierarchy_direct_sparse_unified_level_plan_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_star_h0_retraction_differential
  COMMAND morsehgp3d_hierarchy_direct_star_h0_retraction_differential_tests
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
  NAME morsehgp3d.hierarchy_direct_frozen_incidence_quotient
  COMMAND morsehgp3d_hierarchy_direct_frozen_incidence_quotient_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_frozen_incidence_hgp_action_plan
  COMMAND morsehgp3d_hierarchy_direct_frozen_incidence_hgp_action_plan_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_frozen_unified_incidence_batch
  COMMAND morsehgp3d_hierarchy_direct_frozen_unified_incidence_batch_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_unified_resident_session
  COMMAND morsehgp3d_hierarchy_direct_morse_unified_resident_session_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_resident_at_least20_adapter
  COMMAND morsehgp3d_hierarchy_direct_morse_resident_at_least20_adapter_tests
)
add_test(
  NAME morsehgp3d.hierarchy_normalized_exact_hartigan_level_manifest
  COMMAND morsehgp3d_hierarchy_normalized_exact_hartigan_level_manifest_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_k1_boruvka_closed_cut_session
  COMMAND morsehgp3d_hierarchy_direct_k1_boruvka_closed_cut_session_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge
  COMMAND morsehgp3d_hierarchy_direct_morse_resident_k2_k1_closed_cut_bridge_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_resident_all_orders_vertical_bridge
  COMMAND morsehgp3d_hierarchy_direct_morse_resident_all_orders_vertical_bridge_tests
)
add_test(
  NAME morsehgp3d.hierarchy_relative_morse_boruvka
  COMMAND morsehgp3d_hierarchy_relative_morse_boruvka_tests
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
  NAME morsehgp3d.hierarchy_direct_morse_forest_carrier_cut_index
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_index_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_forest_root_coverage_index
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_root_coverage_index_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_forest_carrier_cut_replay_session
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_replay_session_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_forest_carrier_cut_closure_adapter
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_carrier_cut_closure_adapter_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_forest_reducer
  COMMAND morsehgp3d_hierarchy_direct_morse_forest_reducer_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_terminal_reducer_source_bridge
  COMMAND morsehgp3d_hierarchy_direct_morse_terminal_reducer_source_bridge_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_vertical_journal
  COMMAND morsehgp3d_hierarchy_direct_morse_vertical_journal_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_vertical_target_proposal_adapter
  COMMAND morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_adapter_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_vertical_target_proposal_pipeline
  COMMAND morsehgp3d_hierarchy_direct_morse_vertical_target_proposal_pipeline_tests
)
add_test(
  NAME morsehgp3d.hierarchy_direct_morse_budget
  COMMAND morsehgp3d_hierarchy_direct_morse_budget_tests
)
if(UNIX)
  add_test(
    NAME morsehgp3d.hierarchy_atomic_linear_run_store
    COMMAND morsehgp3d_hierarchy_atomic_linear_run_store_tests
  )
  add_test(
    NAME morsehgp3d.sparse_anchored_pair_chunk_run
    COMMAND morsehgp3d_sparse_anchored_pair_chunk_run_tests
  )
  add_test(
    NAME morsehgp3d.sparse_anchored_pair_h0_run
    COMMAND morsehgp3d_sparse_anchored_pair_h0_run_tests
  )
  add_test(
    NAME morsehgp3d.sparse_direct_h0_candidate_merge
    COMMAND morsehgp3d_sparse_direct_h0_candidate_merge_tests
  )
  add_test(
    NAME morsehgp3d.sparse_higher_support_h0_chunk_run
    COMMAND morsehgp3d_sparse_higher_support_h0_chunk_run_tests
  )
  add_test(
    NAME morsehgp3d.sparse_direct_h0_atomic_run_reader
    COMMAND morsehgp3d_sparse_direct_h0_atomic_run_reader_tests
  )
  add_test(
    NAME morsehgp3d.hierarchy_direct_morse_chunk_run
    COMMAND morsehgp3d_hierarchy_direct_morse_chunk_run_tests
  )
endif()
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
  NAME morsehgp3d.hierarchy_hartigan_all_orders
  COMMAND morsehgp3d_hierarchy_hartigan_all_orders_tests
)
set_tests_properties(
  morsehgp3d.hierarchy_hartigan_all_orders
  PROPERTIES TIMEOUT 180
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
  NAME morsehgp3d.gpu_direct_sparse_facet_top_k_proposal_context
  COMMAND
    morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_context_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_lbvh_build_context
  COMMAND morsehgp3d_gpu_morton_lbvh_build_context_tests
)
add_test(
  NAME morsehgp3d.gpu_anchored_pair_candidate_proposal_context
  COMMAND
    morsehgp3d_gpu_anchored_pair_candidate_proposal_context_tests
)
add_test(
  NAME morsehgp3d.gpu_ranked_diametral_pair_catalog_contract
  COMMAND morsehgp3d_gpu_ranked_diametral_pair_catalog_contract_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_diametral_phi_context
  COMMAND morsehgp3d_gpu_exact_diametral_phi_context_tests
)
if(MORSEHGP3D_ENABLE_CUDA)
  add_test(
    NAME morsehgp3d.gpu_exact_diametral_phi_qualification
    COMMAND morsehgp3d_gpu_exact_diametral_phi_qualification
  )
  add_test(
    NAME morsehgp3d.gpu_exact_pair_block_witness_cuda_qualification
    COMMAND morsehgp3d_gpu_exact_pair_block_witness_cuda_qualification
  )
  add_test(
    NAME morsehgp3d.gpu_exact_pair_block_antichain_cuda_qualification
    COMMAND morsehgp3d_gpu_exact_pair_block_antichain_cuda_qualification
  )
endif()
add_test(
  NAME morsehgp3d.gpu_morton_yao48_pair_frontier
  COMMAND morsehgp3d_gpu_morton_yao48_pair_frontier_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_yao48_tiled_pair_frontier
  COMMAND morsehgp3d_gpu_morton_yao48_tiled_pair_frontier_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_yao48_device_tiled_pair_frontier
  COMMAND morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_yao48_ranked_pair_tile_classifier
  COMMAND morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_yao48_ranked_pair_h0_projection
  COMMAND morsehgp3d_gpu_morton_yao48_ranked_pair_h0_projection_tests
)
if(MORSEHGP3D_ENABLE_CUDA)
  add_test(
    NAME morsehgp3d.gpu_morton_yao48_ranked_pair_tile_classifier_qualification
    COMMAND
      morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification
  )
endif()
add_test(
  NAME morsehgp3d.gpu_exact_pair_level_fixed
  COMMAND morsehgp3d_gpu_exact_pair_level_fixed_tests
)
add_test(
  NAME morsehgp3d.gpu_exact_closed_rank23_pair_terminal_catalog
  COMMAND morsehgp3d_gpu_exact_closed_rank23_pair_terminal_catalog_tests
)
add_test(
  NAME morsehgp3d.gpu_morton_yao48_radial_subtree_filter
  COMMAND morsehgp3d_gpu_morton_yao48_radial_subtree_filter_tests
)
if(MORSEHGP3D_ENABLE_CUDA)
  add_test(
    NAME morsehgp3d.gpu_morton_yao48_radial_subtree_filter_qualification
    COMMAND morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification
  )
endif()
add_test(
  NAME morsehgp3d.gpu_morton_yao48_seed_work_profile
  COMMAND morsehgp3d_gpu_morton_yao48_seed_work_profile_tests
)
add_test(
  NAME morsehgp3d.anchored_pair_candidate_recertifier
  COMMAND morsehgp3d_anchored_pair_candidate_recertifier_tests
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
  NAME morsehgp3d.phase15_pair_first_scale_campaign_contract
  COMMAND
    "${CMAKE_COMMAND}"
    -E env
    "PYTHONDONTWRITEBYTECODE=1"
    "${Python3_EXECUTABLE}"
    -B
    "${CMAKE_CURRENT_SOURCE_DIR}/../profiling/test_phase15_pair_first_scale_campaign.py"
)
set_tests_properties(
  morsehgp3d.phase15_pair_first_scale_campaign_contract
  PROPERTIES TIMEOUT 60
)
add_test(
  NAME morsehgp3d.phase15_true_hgp_scale_campaign_contract
  COMMAND
    "${CMAKE_COMMAND}"
    -E env
    "PYTHONDONTWRITEBYTECODE=1"
    "${Python3_EXECUTABLE}"
    -B
    "${CMAKE_CURRENT_SOURCE_DIR}/../profiling/test_phase15_true_hgp_scale_campaign.py"
)
set_tests_properties(
  morsehgp3d.phase15_true_hgp_scale_campaign_contract
  PROPERTIES TIMEOUT 60
)
add_test(
  NAME morsehgp3d.phase15_true_hgp_scale_campaign_v3_contract
  COMMAND
    "${CMAKE_COMMAND}"
    -E env
    "PYTHONDONTWRITEBYTECODE=1"
    "${Python3_EXECUTABLE}"
    -B
    "${CMAKE_CURRENT_SOURCE_DIR}/../profiling/test_phase15_true_hgp_scale_campaign_v3.py"
)
set_tests_properties(
  morsehgp3d.phase15_true_hgp_scale_campaign_v3_contract
  PROPERTIES TIMEOUT 60
)
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
if(TARGET morsehgp3d_direct_morse_product_runner)
  add_test(
    NAME morsehgp3d.direct_morse_sparse_pair_runner
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_direct_morse_sparse_pair_runner.py"
      "${PROJECT_SOURCE_DIR}"
      "$<TARGET_FILE:morsehgp3d_direct_morse_product_runner>"
  )
  set_tests_properties(
    morsehgp3d.direct_morse_sparse_pair_runner
    PROPERTIES TIMEOUT 90
  )
endif()
if(TARGET morsehgp3d_massive_sparse_pair_prefix_smoke)
  add_test(
    NAME morsehgp3d.massive_sparse_pair_prefix_smoke
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../differential/check_massive_sparse_pair_prefix_smoke.py"
      "$<TARGET_FILE:morsehgp3d_massive_sparse_pair_prefix_smoke>"
  )
  set_tests_properties(
    morsehgp3d.massive_sparse_pair_prefix_smoke
    PROPERTIES TIMEOUT 15
  )
endif()
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
if(TARGET
   morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification)
  add_test(
    NAME morsehgp3d.gpu_direct_sparse_facet_top_k_proposal_qualification
    COMMAND
      morsehgp3d_gpu_direct_sparse_facet_top_k_proposal_qualification
  )
  set_tests_properties(
    morsehgp3d.gpu_direct_sparse_facet_top_k_proposal_qualification
    PROPERTIES TIMEOUT 60
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
