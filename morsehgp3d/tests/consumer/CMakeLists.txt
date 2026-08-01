cmake_minimum_required(VERSION 3.25)

project(MorseHGP3DConsumer LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MorseHGP3D CONFIG REQUIRED)

add_executable(morsehgp3d_consumer main.cpp)
target_link_libraries(
  morsehgp3d_consumer
  PRIVATE
    morsehgp3d::contract
    morsehgp3d::spatial
    morsehgp3d::capped_distinct_point_coverage
    morsehgp3d::silent_pair_gateway
    morsehgp3d::anchored_pair_candidate_classifier
    morsehgp3d::anchored_pair_witness_bank
    morsehgp3d::sparse_anchored_pair_session
    morsehgp3d::direct_support_terminal
    morsehgp3d::direct_morse_gamma_carrier_conformance
    morsehgp3d::direct_sparse_facet_descent_batch_executor
    morsehgp3d::direct_sparse_facet_descent_batch_plan
    morsehgp3d::direct_sparse_facet_top_k_proposal_transcript
    morsehgp3d::direct_sparse_gateway_historical_quotient
    morsehgp3d::direct_sparse_positive_facet_prefix_sweep
    morsehgp3d::direct_sparse_successive_incidence
    morsehgp3d::direct_sparse_successive_incidence_star_journal
    morsehgp3d::direct_sparse_unified_level_plan
    morsehgp3d::direct_frozen_incidence_quotient
    morsehgp3d::direct_frozen_incidence_hgp_action_plan
    morsehgp3d::direct_frozen_unified_incidence_batch
    morsehgp3d::direct_morse_forest_root_coverage_index
)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME morsehgp3d.consumer COMMAND morsehgp3d_consumer)
endif()
