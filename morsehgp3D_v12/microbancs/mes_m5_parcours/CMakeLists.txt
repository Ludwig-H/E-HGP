# Microbanc MES-M5 de la v12 : parcours des boites du catalogue en largeur sur le GPU (hors produit).
#
# Cibles :
#   mhgp12_traversal_identity  identite sur l'hote (warp simule) du parcours en largeur contre les vidages MHGP12TR,
#                              mutants, fixtures (C++ seul)
#   mhgp12_traversal_dump      vidage du parcours de la v11 ; exige MHGP11_SOURCE_DIR et MHGP11_LIBRARY (libmhgp11.a, u21)
#   mhgp12_v11_traversal_timing chrono de la frontiere et de la passe unique de la v11 (voie lot, W fils) et du walk
#                              sequentiel ; memes exigences
#   mhgp12_traversal_bench     banc CUDA (sm_120) : parcours sur l'appareil, chrono, identite, mutants ; exige
#                              MHGP12_PARCOURS_CUDA=ON
#
# Le warp en source unique (simt.hpp) et les primitives de vidage (dump_format.hpp) sont ceux de MES-M2 :
# MHGP12_M2_DIR pointe sur microbancs/mes_m2_feuille (par defaut le dossier frere, une fois verse dans le depot).
#
# Exemples :
#   cmake -S . -B b -DCMAKE_BUILD_TYPE=Release -DMHGP11_SOURCE_DIR=<depot>/morsehgp3D_v11 \
#         -DMHGP11_LIBRARY=<construction v11>/libmhgp11.a
#   cmake -S . -B b_cuda -DCMAKE_BUILD_TYPE=Release -DMHGP12_PARCOURS_CUDA=ON -DCMAKE_CUDA_COMPILER=<nvcc>
cmake_minimum_required(VERSION 3.20)
project(mhgp12_parcours LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(MHGP12_M2_DIR "${PROJECT_SOURCE_DIR}/../mes_m2_feuille" CACHE PATH "Racine du microbanc MES-M2 (simt.hpp, dump_format.hpp)")
set(MHGP11_SOURCE_DIR "" CACHE PATH "Racine de morsehgp3D_v11 (en-tetes de l'outil de vidage et du chrono v11)")
set(MHGP11_LIBRARY "" CACHE FILEPATH "libmhgp11.a construite au profil u21 (outil de vidage, chrono v11)")
option(MHGP12_PARCOURS_CUDA "Banc CUDA (sm_120)" OFF)

if(NOT EXISTS "${MHGP12_M2_DIR}/include/mhgp12/leaf/simt.hpp")
  message(FATAL_ERROR "MHGP12_M2_DIR=${MHGP12_M2_DIR} : include/mhgp12/leaf/simt.hpp introuvable (microbanc MES-M2)")
endif()

set(mhgp12_warnings -Wall -Wextra -Wpedantic -Werror)

if(MHGP12_PARCOURS_CUDA)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120)
  endif()
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
  # CMake 3.22 (VM G4) ne connait pas l'option du dialecte CUDA20 ; nvcc 12 la comprend (meme contournement que MES-M2).
  if(NOT CMAKE_CUDA20_STANDARD_COMPILE_OPTION)
    set(CMAKE_CUDA20_STANDARD_COMPILE_OPTION "-std=c++20")
    set(CMAKE_CUDA20_EXTENSION_COMPILE_OPTION "-std=c++20")
  endif()
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

function(mhgp12_host_target name)
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/include ${MHGP12_M2_DIR}/include)
  target_compile_options(${name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${mhgp12_warnings}>)
endfunction()

find_package(Threads REQUIRED)

add_executable(mhgp12_traversal_identity host/traversal_identity.cpp)
mhgp12_host_target(mhgp12_traversal_identity)
target_link_libraries(mhgp12_traversal_identity PRIVATE Threads::Threads)

if(MHGP11_SOURCE_DIR AND MHGP11_LIBRARY)
  add_executable(mhgp12_traversal_dump vidage/traversal_dump.cpp)
  mhgp12_host_target(mhgp12_traversal_dump)
  target_include_directories(mhgp12_traversal_dump PRIVATE ${MHGP11_SOURCE_DIR}/src ${MHGP11_SOURCE_DIR}/bench)
  target_compile_definitions(mhgp12_traversal_dump PRIVATE MHGP11_COORD_BITS=21)
  target_link_libraries(mhgp12_traversal_dump PRIVATE ${MHGP11_LIBRARY} Threads::Threads)
  add_executable(mhgp12_v11_traversal_timing vidage/v11_timing.cpp)
  mhgp12_host_target(mhgp12_v11_traversal_timing)
  target_include_directories(mhgp12_v11_traversal_timing PRIVATE ${MHGP11_SOURCE_DIR}/src ${MHGP11_SOURCE_DIR}/bench)
  target_compile_definitions(mhgp12_v11_traversal_timing PRIVATE MHGP11_COORD_BITS=21)
  target_link_libraries(mhgp12_v11_traversal_timing PRIVATE ${MHGP11_LIBRARY} Threads::Threads)
endif()

if(MHGP12_PARCOURS_CUDA)
  add_executable(mhgp12_traversal_bench cuda/traversal_bench.cu)
  target_include_directories(mhgp12_traversal_bench PRIVATE ${PROJECT_SOURCE_DIR}/include ${MHGP12_M2_DIR}/include)
  target_compile_options(mhgp12_traversal_bench PRIVATE
                         $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr -lineinfo -Xptxas=-v>)
  target_link_libraries(mhgp12_traversal_bench PRIVATE CUDA::cudart_static)
endif()
