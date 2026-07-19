add_executable(morsehgp3d_exact_types_tests test_exact_types.cpp)
target_link_libraries(morsehgp3d_exact_types_tests PRIVATE morsehgp3d::exact)

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
  morsehgp3d_exact_types_tests
  morsehgp3d_exact_types_dump
  morsehgp3d_predicates_tests
  morsehgp3d_affine_tests
  morsehgp3d_centers_tests
  morsehgp3d_support_tests
  morsehgp3d_level_order_tests
  morsehgp3d_expansion_tests
  morsehgp3d_spatial_tests
  morsehgp3d_spatial_query_dump
  morsehgp3d_spatial_lbvh_tests
  morsehgp3d_spatial_lbvh_query_dump
  morsehgp3d_hierarchy_emst_tests
  morsehgp3d_hierarchy_k1_forest_tests
  morsehgp3d_hierarchy_miniball_tests
  morsehgp3d_hierarchy_boruvka_tests
  morsehgp3d_hierarchy_gabriel_tests
  morsehgp3d_hierarchy_k1_dump
  morsehgp3d_predicate_filter_context_tests
  morsehgp3d_warm_context_benchmark_host_test
  morsehgp3d_gpu_spatial_reference_context_tests
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
