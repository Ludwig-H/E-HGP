cmake_minimum_required(VERSION 3.22)
project(morsehgp3d_v10 LANGUAGES CXX)

# MorseHGP3D v10 : tour FULL de la hierarchie K-NN, hierarchies de points et tetes de clustering.
# C++20 sans extensions, avertissements traites en erreurs, aucune dependance externe dans le produit.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(MHGP10_SANITIZE "ASan + UBSan" OFF)
option(MHGP10_TSAN "ThreadSanitizer" OFF)
option(MHGP10_POISON "Empoisonne les tampons non initialises (0xA5)" OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
if(CMAKE_CXX_FLAGS MATCHES "fast-math")
  message(FATAL_ERROR "-ffast-math est interdit : les filtres flottants certifies supposent IEEE-754 strict")
endif()
if(MHGP10_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()
if(MHGP10_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

find_package(Threads REQUIRED)

add_library(mhgp10_core STATIC
  src/core/buffer.cpp
  src/sched/pool.cpp
  src/cloud/cloud.cpp
  src/cloud/site_tree.cpp
  src/arith/geometry.cpp
  src/catalogue/generator.cpp
  src/tower/tower.cpp
  src/points/dendrogram.cpp
  src/head/head.cpp)
target_include_directories(mhgp10_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(mhgp10_core PUBLIC Threads::Threads)
if(MHGP10_POISON)
  target_compile_definitions(mhgp10_core PUBLIC MHGP10_POISON)
endif()

include(CTest)
include(cmake/gates.cmake)

add_executable(mhgp10_catalogue cli/mhgp10_catalogue.cpp)
target_link_libraries(mhgp10_catalogue PRIVATE mhgp10_core)

add_executable(mhgp10_tower cli/mhgp10_tower.cpp)
target_link_libraries(mhgp10_tower PRIVATE mhgp10_core)

add_executable(mhgp10_unit tests/unit/unit_main.cpp)
target_link_libraries(mhgp10_unit PRIVATE mhgp10_core)
mhgp10_gate(NAME mhgp10_unit COMMAND mhgp10_unit CODE 0 LINE "unit_ok" LABELS gate fast)

# Temoin de test (hors produit) : atteignabilite mutuelle + tete v10, pour juger la condensation contre
# sklearn.cluster.HDBSCAN la ou les deux objets coincident (K = 1, 2). HDBSCAN lui-meme n'est jamais
# reimplemente : les bancs l'appellent via scikit-learn.
add_executable(mhgp10_mreach_cluster tests/head/mreach_cluster.cpp tests/head/mreach.cpp)
target_include_directories(mhgp10_mreach_cluster PRIVATE ${PROJECT_SOURCE_DIR}/tests/head)
target_link_libraries(mhgp10_mreach_cluster PRIVATE mhgp10_core)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME mhgp10_catalogue_oracle
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/oracle/test_catalogue_oracle.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_catalogue_oracle PROPERTIES LABELS "gate;oracle" TIMEOUT 1800)
  add_test(NAME mhgp10_tower_oracle
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/oracle/test_tower_oracle.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_tower_oracle PROPERTIES LABELS "gate;oracle" TIMEOUT 1800)
  add_test(NAME mhgp10_head_condensation_vs_sklearn
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/head/test_condensation_vs_sklearn.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_head_condensation_vs_sklearn PROPERTIES LABELS "gate;head" TIMEOUT 900)
endif()
