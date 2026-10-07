# Microbanc MES-M2 de la v12 : feuille du catalogue data-parallele (hors produit).
#
# Cibles :
#   mhgp12_leaf_identity  identite sur l'hote des deux formes (warp simule) contre les vidages de leaf.cpp (C++ seul)
#   mhgp12_mes_s          histogrammes d'etendue locale (contrat numerique, MES-S) sur les vidages (C++ seul)
#   mhgp12_arena_selftest auto-test de la verification d'arene du banc (warp simule, arrivees melangees, mutants)
#   mhgp12_dump_admission_selftest  porte d'admission des vidages (CST-0215) : vidage valide admis, mutants refuses
#   mhgp12_leaf_dump      vidage des feuilles de la v11 ; exige MHGP11_SOURCE_DIR et MHGP11_LIBRARY (libmhgp11.a, u21)
#   mhgp12_leaf_timing    chrono local indicatif a un fil : leaf.cpp, feuille v11 sur l'hote, formes v12 simulees
#   mhgp12_leaf_bench     banc CUDA (sm_120) : temoin v11 un fil par feuille, formes j3 et coherente ; exige
#                         MHGP12_FEUILLE_CUDA=ON et MHGP11_SOURCE_DIR (en-tetes leaf_device.hpp et leaf_batch.hpp)
#
# Exemples :
#   cmake -S . -B b -DCMAKE_BUILD_TYPE=Release -DMHGP11_SOURCE_DIR=<depot>/morsehgp3D_v11 \
#         -DMHGP11_LIBRARY=<construction v11>/libmhgp11.a
#   cmake -S . -B b_cuda -DCMAKE_BUILD_TYPE=Release -DMHGP11_SOURCE_DIR=<depot>/morsehgp3D_v11 \
#         -DMHGP12_FEUILLE_CUDA=ON -DCMAKE_CUDA_COMPILER=<nvcc>
cmake_minimum_required(VERSION 3.20)
project(mhgp12_feuille LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(MHGP12_COORD_BITS 21 CACHE STRING "Profil de coordonnees (21 ; celui de la bibliotheque v11 liee)")
set(MHGP11_SOURCE_DIR "" CACHE PATH "Racine de morsehgp3D_v11 (en-tetes du temoin et de l'outil de vidage)")
set(MHGP11_LIBRARY "" CACHE FILEPATH "libmhgp11.a construite au meme profil (outil de vidage)")
option(MHGP12_FEUILLE_CUDA "Banc CUDA (sm_120)" OFF)
set(MHGP12_MARCH "" CACHE STRING "Jeu d'instructions de l'hote (-march), vide : aucun")

set(mhgp12_warnings -Wall -Wextra -Wpedantic -Werror)

if(MHGP12_FEUILLE_CUDA)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120)
  endif()
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
  # CMake 3.22 (VM G4) ne connait pas l'option du dialecte CUDA20 et refuse la generation ; nvcc 12 la comprend
  # (meme contournement que morsehgp3D_v11/CMakeLists.txt).
  if(NOT CMAKE_CUDA20_STANDARD_COMPILE_OPTION)
    set(CMAKE_CUDA20_STANDARD_COMPILE_OPTION "-std=c++20")
    set(CMAKE_CUDA20_EXTENSION_COMPILE_OPTION "-std=c++20")
  endif()
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

function(mhgp12_host_target name)
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(${name} PRIVATE MHGP12_COORD_BITS=${MHGP12_COORD_BITS})
  target_compile_options(${name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${mhgp12_warnings}>)
  if(MHGP12_MARCH)
    target_compile_options(${name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-march=${MHGP12_MARCH}>)
  endif()
endfunction()

find_package(Threads REQUIRED)

add_executable(mhgp12_leaf_identity host/leaf_identity.cpp)
mhgp12_host_target(mhgp12_leaf_identity)
target_link_libraries(mhgp12_leaf_identity PRIVATE Threads::Threads)

add_executable(mhgp12_mes_s host/mes_s.cpp)
mhgp12_host_target(mhgp12_mes_s)

add_executable(mhgp12_arena_selftest host/arena_selftest.cpp)
mhgp12_host_target(mhgp12_arena_selftest)

# Porte d'admission des vidages (CST-0215) : vidage synthetique valide et mutants a empreinte juste.
add_executable(mhgp12_dump_admission_selftest host/dump_admission_selftest.cpp)
mhgp12_host_target(mhgp12_dump_admission_selftest)

if(MHGP11_SOURCE_DIR AND MHGP11_LIBRARY)
  add_executable(mhgp12_leaf_dump vidage/leaf_dump.cpp)
  mhgp12_host_target(mhgp12_leaf_dump)
  target_include_directories(mhgp12_leaf_dump PRIVATE ${MHGP11_SOURCE_DIR}/src ${MHGP11_SOURCE_DIR}/bench)
  target_compile_definitions(mhgp12_leaf_dump PRIVATE MHGP11_COORD_BITS=${MHGP12_COORD_BITS})
  target_link_libraries(mhgp12_leaf_dump PRIVATE ${MHGP11_LIBRARY} Threads::Threads)
  add_executable(mhgp12_leaf_timing vidage/leaf_timing.cpp)
  mhgp12_host_target(mhgp12_leaf_timing)
  target_include_directories(mhgp12_leaf_timing PRIVATE ${MHGP11_SOURCE_DIR}/src)
  target_compile_definitions(mhgp12_leaf_timing PRIVATE MHGP11_COORD_BITS=${MHGP12_COORD_BITS})
  target_link_libraries(mhgp12_leaf_timing PRIVATE ${MHGP11_LIBRARY} Threads::Threads)
endif()

if(MHGP12_FEUILLE_CUDA)
  if(NOT MHGP11_SOURCE_DIR)
    message(FATAL_ERROR "mhgp12_leaf_bench : MHGP11_SOURCE_DIR requis (temoin v11)")
  endif()
  add_executable(mhgp12_leaf_bench cuda/leaf_bench.cu)
  target_include_directories(mhgp12_leaf_bench PRIVATE ${PROJECT_SOURCE_DIR}/include ${MHGP11_SOURCE_DIR}/src)
  target_compile_definitions(mhgp12_leaf_bench PRIVATE MHGP12_COORD_BITS=${MHGP12_COORD_BITS}
                                                       MHGP11_COORD_BITS=${MHGP12_COORD_BITS})
  target_compile_options(mhgp12_leaf_bench PRIVATE
                         $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr -lineinfo -Xptxas=-v>)
  target_link_libraries(mhgp12_leaf_bench PRIVATE CUDA::cudart_static)
endif()
