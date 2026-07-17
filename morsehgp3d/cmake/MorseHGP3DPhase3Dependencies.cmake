include_guard(GLOBAL)

include(FetchContent)

set(
  FETCHCONTENT_UPDATES_DISCONNECTED
  ON
  CACHE BOOL
  "Do not update the pinned MorseHGP3D Phase 3 dependencies"
  FORCE
)

set(
  MORSEHGP3D_DLPACK_GIT_COMMIT
  "84d107bf416c6bab9ae68ad285876600d230490d"
)
set(
  MORSEHGP3D_NANOBIND_GIT_COMMIT
  "2a61ad2494d09fecb2e13322c1383342c299900d"
)
set(
  MORSEHGP3D_NANOBIND_ROBIN_MAP_GIT_COMMIT
  "4ec1bf19c6a96125ea22062f38c2cf5b958e448e"
)

function(
  _morsehgp3d_phase3_require_git_revision
  checkout
  expected_revision
  dependency_name
)
  find_package(Git REQUIRED)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${checkout}" rev-parse HEAD
    RESULT_VARIABLE revision_status
    OUTPUT_VARIABLE actual_revision
    ERROR_VARIABLE revision_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT revision_status EQUAL 0)
    message(
      FATAL_ERROR
      "Cannot verify ${dependency_name} revision in ${checkout}: ${revision_error}"
    )
  endif()
  if(NOT actual_revision STREQUAL expected_revision)
    message(
      FATAL_ERROR
      "${dependency_name} revision mismatch: expected ${expected_revision}, got ${actual_revision}"
    )
  endif()
  execute_process(
    COMMAND
      "${GIT_EXECUTABLE}" -C "${checkout}" status --porcelain=v1
      --untracked-files=all --ignore-submodules=none
    RESULT_VARIABLE worktree_status
    OUTPUT_VARIABLE worktree_changes
    ERROR_VARIABLE worktree_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT worktree_status EQUAL 0)
    message(
      FATAL_ERROR
      "Cannot verify ${dependency_name} worktree in ${checkout}: ${worktree_error}"
    )
  endif()
  if(NOT worktree_changes STREQUAL "")
    message(
      FATAL_ERROR
      "${dependency_name} checkout is dirty despite pinned revision "
      "${expected_revision}: ${worktree_changes}"
    )
  endif()
endfunction()

# DLPack 1.3 sets CMAKE_CXX_STANDARD=11 in its top-level project. Fetch its
# pinned headers without adding that project, then expose only a project-owned
# interface target. SOURCE_SUBDIR deliberately names no CMake project.
FetchContent_Declare(
  morsehgp3d_dlpack_content
  GIT_REPOSITORY https://github.com/dmlc/dlpack.git
  GIT_TAG "${MORSEHGP3D_DLPACK_GIT_COMMIT}"
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
  UPDATE_DISCONNECTED TRUE
  SOURCE_SUBDIR morsehgp3d-no-add-subdirectory
)
FetchContent_MakeAvailable(morsehgp3d_dlpack_content)
_morsehgp3d_phase3_require_git_revision(
  "${morsehgp3d_dlpack_content_SOURCE_DIR}"
  "${MORSEHGP3D_DLPACK_GIT_COMMIT}"
  "DLPack"
)

add_library(morsehgp3d_phase3_dlpack INTERFACE)
add_library(
  morsehgp3d::phase3_dlpack
  ALIAS morsehgp3d_phase3_dlpack
)
target_compile_features(morsehgp3d_phase3_dlpack INTERFACE cxx_std_20)
target_include_directories(
  morsehgp3d_phase3_dlpack
  SYSTEM INTERFACE
    "${morsehgp3d_dlpack_content_SOURCE_DIR}/include"
)
target_compile_definitions(
  morsehgp3d_phase3_dlpack
  INTERFACE
    MORSEHGP3D_DLPACK_GIT_COMMIT="${MORSEHGP3D_DLPACK_GIT_COMMIT}"
)

# nanobind's sole git submodule is part of the pinned source tree. FetchContent
# initializes exactly that gitlink and the checks below reject any mismatch.
set(NB_CREATE_INSTALL_RULES OFF CACHE BOOL "" FORCE)
set(NB_USE_SUBMODULE_DEPS ON CACHE BOOL "" FORCE)
set(NB_TEST OFF CACHE BOOL "" FORCE)
set(NB_TEST_STABLE_ABI OFF CACHE BOOL "" FORCE)
set(NB_TEST_SHARED_BUILD OFF CACHE BOOL "" FORCE)
set(NB_TEST_CUDA OFF CACHE BOOL "" FORCE)
set(NB_TEST_FREE_THREADED OFF CACHE BOOL "" FORCE)

# FetchContent configures nanobind in a child directory, while
# nanobind_add_module is called later from the project root.  Resolve Python in
# that root scope so the directory-scoped Python_INCLUDE_DIRS remains visible
# when nanobind creates its support library and extension target.
find_package(
  Python 3.9
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule
)
if(
  NOT TARGET Python::Interpreter
  OR NOT TARGET Python::Module
  OR NOT Python_INCLUDE_DIRS
)
  message(FATAL_ERROR "Phase 3 requires visible Python development headers")
endif()

FetchContent_Declare(
  morsehgp3d_nanobind_content
  GIT_REPOSITORY https://github.com/wjakob/nanobind.git
  GIT_TAG "${MORSEHGP3D_NANOBIND_GIT_COMMIT}"
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
  GIT_SUBMODULES ext/robin_map
  GIT_SUBMODULES_RECURSE TRUE
  UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(morsehgp3d_nanobind_content)
_morsehgp3d_phase3_require_git_revision(
  "${morsehgp3d_nanobind_content_SOURCE_DIR}"
  "${MORSEHGP3D_NANOBIND_GIT_COMMIT}"
  "nanobind"
)
_morsehgp3d_phase3_require_git_revision(
  "${morsehgp3d_nanobind_content_SOURCE_DIR}/ext/robin_map"
  "${MORSEHGP3D_NANOBIND_ROBIN_MAP_GIT_COMMIT}"
  "nanobind robin-map submodule"
)
if(NOT COMMAND nanobind_add_module)
  message(FATAL_ERROR "Pinned nanobind did not provide nanobind_add_module")
endif()

# CCCL/CUB and NVTX3 are supplied only by the selected CUDA 12.9 toolkit. No
# second FetchContent source is allowed for either dependency.
find_package(CUDAToolkit 12.9 REQUIRED)
if(NOT CUDAToolkit_VERSION MATCHES "^12\\.9(\\.|$)")
  message(
    FATAL_ERROR
    "Phase 3 requires CUDA Toolkit 12.9.x, got ${CUDAToolkit_VERSION}"
  )
endif()
if(NOT TARGET CUDA::cudart)
  message(FATAL_ERROR "CUDA Toolkit 12.9 did not provide CUDA::cudart")
endif()
if(NOT TARGET CUDA::nvtx3)
  message(FATAL_ERROR "CUDA Toolkit 12.9 did not provide CUDA::nvtx3")
endif()

find_path(
  MORSEHGP3D_PHASE3_CUB_INCLUDE_DIR
  NAMES cub/version.cuh
  PATHS ${CUDAToolkit_INCLUDE_DIRS}
  NO_DEFAULT_PATH
  REQUIRED
)
find_path(
  MORSEHGP3D_PHASE3_NVTX_INCLUDE_DIR
  NAMES nvtx3/nvToolsExt.h
  PATHS ${CUDAToolkit_INCLUDE_DIRS}
  NO_DEFAULT_PATH
  REQUIRED
)

add_library(morsehgp3d_phase3_cuda_dependencies INTERFACE)
add_library(
  morsehgp3d::phase3_cuda_dependencies
  ALIAS morsehgp3d_phase3_cuda_dependencies
)
target_compile_features(
  morsehgp3d_phase3_cuda_dependencies
  INTERFACE cxx_std_20
)
target_include_directories(
  morsehgp3d_phase3_cuda_dependencies
  SYSTEM INTERFACE
    "${MORSEHGP3D_PHASE3_CUB_INCLUDE_DIR}"
    "${MORSEHGP3D_PHASE3_NVTX_INCLUDE_DIR}"
)
target_link_libraries(
  morsehgp3d_phase3_cuda_dependencies
  INTERFACE CUDA::cudart CUDA::nvtx3
)
target_compile_definitions(
  morsehgp3d_phase3_cuda_dependencies
  INTERFACE
    MORSEHGP3D_NANOBIND_GIT_COMMIT="${MORSEHGP3D_NANOBIND_GIT_COMMIT}"
    MORSEHGP3D_NANOBIND_ROBIN_MAP_GIT_COMMIT="${MORSEHGP3D_NANOBIND_ROBIN_MAP_GIT_COMMIT}"
)

add_library(morsehgp3d_phase3_dependencies INTERFACE)
add_library(
  morsehgp3d::phase3_dependencies
  ALIAS morsehgp3d_phase3_dependencies
)
target_compile_features(morsehgp3d_phase3_dependencies INTERFACE cxx_std_20)
target_link_libraries(
  morsehgp3d_phase3_dependencies
  INTERFACE
    morsehgp3d::phase3_dlpack
    morsehgp3d::phase3_cuda_dependencies
)
