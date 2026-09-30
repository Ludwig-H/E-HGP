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
set(MHGP10_MARCH "" CACHE STRING "Jeu d'instructions cible (-march), par exemple x86-64-v3 ou x86-64-v4 ; vide : aucun")

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
if(CMAKE_CXX_FLAGS MATCHES "fast-math")
  message(FATAL_ERROR "-ffast-math est interdit : les filtres flottants certifies supposent IEEE-754 strict")
endif()
if(MHGP10_MARCH)
  # sans contraction en FMA : memes arrondis flottants que le build de base
  add_compile_options(-march=${MHGP10_MARCH} -ffp-contract=off)
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

add_executable(mhgp10_cluster cli/mhgp10_cluster.cpp)
target_link_libraries(mhgp10_cluster PRIVATE mhgp10_core)

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
  add_test(NAME mhgp10_regression_level_collision
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/regression/test_level_collision.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_level_collision PROPERTIES LABELS "gate;regression" TIMEOUT 900)
  add_test(NAME mhgp10_points_cover
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/points/test_cover_entry.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_points_cover PROPERTIES LABELS "gate;points" TIMEOUT 900)
  # Faits mathematiques de la projection des points (registre racine, section V10) : couvertures recouvrantes,
  # discontinuite de cover, croisement multi-K, egalite de la borne de stabilite 2 epsilon de core.
  add_test(NAME mhgp10_regression_projection_facts
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=-O ${PROJECT_SOURCE_DIR}/tests/regression/test_projection_facts.py ${PROJECT_BINARY_DIR}"
                   "-DEXPECT_LINE=projection_facts_ok faits=4/4 mutants_tues=5/5 equivalents_acceptes=1/1"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_projection_facts PROPERTIES LABELS "gate;regression;fast" TIMEOUT 300)
  add_test(NAME mhgp10_regression_batch_equivalence
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/regression/test_batch_equivalence.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_batch_equivalence PROPERTIES LABELS "gate;regression" TIMEOUT 900)
  add_test(NAME mhgp10_regression_mreach_border
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/regression/test_mreach_border.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_mreach_border PROPERTIES LABELS "gate;regression" TIMEOUT 900)
  add_test(NAME mhgp10_regression_multiplicity_refusal
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/regression/test_multiplicity_refusal.py ${PROJECT_BINARY_DIR}"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_multiplicity_refusal PROPERTIES LABELS "gate;regression;fast" TIMEOUT 300)
endif()
