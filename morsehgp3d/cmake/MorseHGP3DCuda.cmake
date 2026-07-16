include_guard(GLOBAL)

if(NOT MORSEHGP3D_ENABLE_CUDA)
  message(FATAL_ERROR "MorseHGP3DCuda.cmake requires explicit CUDA opt-in")
endif()

function(_morsehgp3d_reject_cuda_options label value)
  if("${value}" MATCHES "use_fast_math")
    message(FATAL_ERROR "${label} contains forbidden use_fast_math")
  endif()
  if(
    "${value}"
    MATCHES
      "(^|[ \t;:>])(-gencode|--generate-code|-arch|--gpu-architecture|-code|--gpu-code|-ptx|--ptx)($|[=, \t;>])"
  )
    message(
      FATAL_ERROR
      "${label} contains a raw CUDA architecture or PTX selector; "
      "CMAKE_CUDA_ARCHITECTURES=120-real is the only authority"
    )
  endif()
  if(
    "${value}"
    MATCHES
      "(^|[ \t;:>])(@[^ \t;>]+|--options-file|-optf)($|[=, \t;>])"
  )
    message(FATAL_ERROR "${label} contains a forbidden CUDA options-file escape")
  endif()
endfunction()

function(_morsehgp3d_validate_global_cuda_flags)
  foreach(
    _morsehgp3d_cuda_flags_variable
    IN ITEMS
      CMAKE_CUDA_FLAGS
      CMAKE_CUDA_FLAGS_DEBUG
      CMAKE_CUDA_FLAGS_RELEASE
      CMAKE_CUDA_FLAGS_RELWITHDEBINFO
      CMAKE_CUDA_FLAGS_MINSIZEREL
  )
    _morsehgp3d_reject_cuda_options(
      "${_morsehgp3d_cuda_flags_variable}"
      "${${_morsehgp3d_cuda_flags_variable}}"
    )
  endforeach()
endfunction()

set(_morsehgp3d_cuda_architecture "120-real")
if(DEFINED CMAKE_CUDA_ARCHITECTURES)
  if(NOT "${CMAKE_CUDA_ARCHITECTURES}" STREQUAL "${_morsehgp3d_cuda_architecture}")
    message(
      FATAL_ERROR
      "Phase 3.1 requires exactly CMAKE_CUDA_ARCHITECTURES=120-real; "
      "got '${CMAKE_CUDA_ARCHITECTURES}'"
    )
  endif()
else()
  set(
    CMAKE_CUDA_ARCHITECTURES
    "${_morsehgp3d_cuda_architecture}"
    CACHE STRING
    "Closed Phase 3.1 real architecture"
  )
endif()

foreach(
  _morsehgp3d_nvcc_environment_variable
  IN ITEMS CUDAFLAGS NVCC_PREPEND_FLAGS NVCC_APPEND_FLAGS
)
  if(NOT "$ENV{${_morsehgp3d_nvcc_environment_variable}}" STREQUAL "")
    message(
      FATAL_ERROR
      "${_morsehgp3d_nvcc_environment_variable} must be empty for the closed AOT build"
    )
  endif()
endforeach()
_morsehgp3d_validate_global_cuda_flags()

include(CheckLanguage)
check_language(CUDA)
if(NOT CMAKE_CUDA_COMPILER)
  message(FATAL_ERROR "CUDA opt-in requires an available NVIDIA CUDA compiler")
endif()
enable_language(CUDA)
_morsehgp3d_validate_global_cuda_flags()

if(NOT CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  message(
    FATAL_ERROR
    "Phase 3.1 requires the NVIDIA CUDA compiler; got '${CMAKE_CUDA_COMPILER_ID}'"
  )
endif()
if(NOT CMAKE_CUDA_COMPILER_VERSION MATCHES "^12[.]9([.]|$)")
  message(
    FATAL_ERROR
    "Phase 3.1 requires CUDA 12.9.x; got '${CMAKE_CUDA_COMPILER_VERSION}'"
  )
endif()
if(NOT "${CMAKE_CUDA_ARCHITECTURES}" STREQUAL "${_morsehgp3d_cuda_architecture}")
  message(FATAL_ERROR "The CUDA compiler changed the closed sm_120 architecture")
endif()

function(morsehgp3d_configure_cuda_target target_name)
  if(NOT TARGET "${target_name}")
    message(FATAL_ERROR "Cannot configure missing CUDA target ${target_name}")
  endif()
  set_target_properties(
    "${target_name}"
    PROPERTIES
      CUDA_ARCHITECTURES "120-real"
      CUDA_EXTENSIONS OFF
      CUDA_COMPILER_LAUNCHER
        "${CMAKE_COMMAND};-E;env;--unset=CUDAFLAGS;--unset=NVCC_PREPEND_FLAGS;--unset=NVCC_APPEND_FLAGS"
      CUDA_SEPARABLE_COMPILATION OFF
      CUDA_STANDARD 20
      CUDA_STANDARD_REQUIRED ON
      POSITION_INDEPENDENT_CODE ON
  )
  target_compile_options(
    "${target_name}"
    PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>
      $<$<COMPILE_LANGUAGE:CUDA>:--ftz=false>
      $<$<COMPILE_LANGUAGE:CUDA>:--prec-div=true>
      $<$<COMPILE_LANGUAGE:CUDA>:--prec-sqrt=true>
  )
  if(MORSEHGP3D_CUDA_AUDIT)
    target_compile_options(
      "${target_name}"
      PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-lineinfo> $<$<COMPILE_LANGUAGE:CUDA>:--ptxas-options=-v>
    )
  endif()

  get_target_property(_morsehgp3d_compile_options "${target_name}" COMPILE_OPTIONS)
  get_target_property(
    _morsehgp3d_interface_compile_options
    "${target_name}"
    INTERFACE_COMPILE_OPTIONS
  )
  foreach(
    _morsehgp3d_target_options
    IN ITEMS
      _morsehgp3d_compile_options
      _morsehgp3d_interface_compile_options
  )
    _morsehgp3d_reject_cuda_options(
      "CUDA target ${target_name}"
      "${${_morsehgp3d_target_options}}"
    )
  endforeach()
endfunction()

unset(_morsehgp3d_cuda_architecture)
unset(_morsehgp3d_cuda_flags_variable)
unset(_morsehgp3d_nvcc_environment_variable)
