cmake_minimum_required(VERSION 3.22)
project(morsehgp3d_v10 LANGUAGES CXX)

# MorseHGP3D v10 : tour FULL de la hierarchie K-NN, hierarchies de points et tetes de clustering.
# C++20 sans extensions, avertissements traites en erreurs, aucune dependance externe dans le produit.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# compile_commands.json : commandes exactes relues par la campagne de mutants des filtres flottants
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(MHGP10_SANITIZE "ASan + UBSan" OFF)
option(MHGP10_TSAN "ThreadSanitizer" OFF)
option(MHGP10_POISON "Empoisonne les tampons non initialises (0xA5)" OFF)
set(MHGP10_MARCH "" CACHE STRING "Jeu d'instructions cible (-march), par exemple x86-64-v3 ou x86-64-v4 ; vide : aucun")

add_compile_options(-Wall -Wextra -Wpedantic -Werror)
# Filtres flottants certifies (SiteTree, tour) : IEEE-754 binaire64 strict, arrondi au plus proche, sans reassociation,
# sans division remplacee par un produit par l'inverse, sans contraction FMA (verification adverse du raccord R2,
# § 3.3 : -Ofast et CMAKE_CXX_FLAGS_RELEASE=-ffast-math passaient la configuration). Refus de -Ofast, -ffast-math et de
# ses composantes qui reassocient ou remplacent une division, dans chaque variable CMAKE_CXX_FLAGS* definie a ce point
# (cache compris : tous les types de build, actif ou non, et les valeurs _INIT venues de CXXFLAGS ou d'une chaine
# d'outils) ; site_tree.cpp et tower.cpp les refusent aussi a la compilation (#error). La contraction n'a pas de macro :
# -ffp-contract=off pour toutes les unites, quel que soit -march (sans effet sur le x86-64 de base, qui n'a pas de FMA).
# Portes : mhgp10_regression_fp_flags_configure, mhgp10_regression_site_tree_fast_math,
# mhgp10_regression_tower_fast_math.
set(mhgp10_fp_flags_forbidden "-Ofast|-ffast-math|-funsafe-math-optimizations|-fassociative-math|-freciprocal-math")
get_cmake_property(mhgp10_fp_flag_vars VARIABLES)
list(FILTER mhgp10_fp_flag_vars INCLUDE REGEX "^CMAKE_CXX_FLAGS")
list(SORT mhgp10_fp_flag_vars)
foreach(var IN LISTS mhgp10_fp_flag_vars)
  if(" ${${var}} " MATCHES "[ \t](${mhgp10_fp_flags_forbidden})[ \t]")
    message(FATAL_ERROR "${var} contient ${CMAKE_MATCH_1} : interdit, les filtres flottants certifies supposent "
                        "IEEE-754 strict (mhgp10_fp_flags_interdits)")
  endif()
endforeach()
set(MHGP10_FP_OPTIONS -ffp-contract=off)
add_compile_options(${MHGP10_FP_OPTIONS})
if(MHGP10_MARCH)
  add_compile_options(-march=${MHGP10_MARCH})
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

# SiteTree (audits des 29 et 30 septembre 2026) : requetes a centre rationnel pour des centres lointains, sous les
# quatre modes d'arrondi (-frounding-math : la porte change le mode a l'execution), fixture de l'auditeur gravee,
# balayage juge en entiers larges, domaine complet de filtered(), chemin pris par appel (SiteTree::path_counts).
# Aucune porte SiteTree n'a le label fast : les plans de la VM ne construisent que les cibles qu'ils listent, et une
# porte fast doit viser l'une d'elles ou etre en Python (porte mhgp10_fast_targets du groupe entrees_cli).
# Hypothese de la preuve de kMargin : aucune contraction FMA dans site_tree.cpp, quel que soit -march (y compris un
# -march passe hors de MHGP10_MARCH, dans CMAKE_CXX_FLAGS) ; controlee par la porte du refus de -ffast-math.
set_source_files_properties(src/cloud/site_tree.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
get_source_file_property(site_tree_fp_options src/cloud/site_tree.cpp COMPILE_OPTIONS)
add_executable(mhgp10_site_tree_far_center tests/regression/site_tree_far_center.cpp)
target_link_libraries(mhgp10_site_tree_far_center PRIVATE mhgp10_core)
target_compile_options(mhgp10_site_tree_far_center PRIVATE -frounding-math)
mhgp10_gate(NAME mhgp10_regression_site_tree_far_center COMMAND mhgp10_site_tree_far_center CODE 0
            LINE "site_tree_far_center_ok" LABELS gate regression)
set(site_tree_fast_math_args -DCXX=${CMAKE_CXX_COMPILER} -DCXX_ID=${CMAKE_CXX_COMPILER_ID}
    -DARCH=${CMAKE_SYSTEM_PROCESSOR} -DSRC_OPTIONS=${site_tree_fp_options}
    -DSRC=${PROJECT_SOURCE_DIR}/src/cloud/site_tree.cpp -DINC=${PROJECT_SOURCE_DIR}/src
    -P ${PROJECT_SOURCE_DIR}/tests/regression/site_tree_fast_math.cmake)
string(REPLACE ";" " " site_tree_fast_math_args "${site_tree_fast_math_args}")
add_test(NAME mhgp10_regression_site_tree_fast_math
         COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_COMMAND} "-DARGS=${site_tree_fast_math_args}" -DEXPECTED=0
                 -DEXPECT_LINE=site_tree_fast_math_refuse -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp10_regression_site_tree_fast_math PROPERTIES LABELS "gate;regression" TIMEOUT 300)
# Mode de rejeu documente de la porte (-DMHGP10_GATE_OLD_API, contre une bibliotheque d'avant le tour 1) : la source
# compile avec les avertissements du projet et ne reference plus filtered() ni path_counts().
set(site_tree_old_api_args -DCXX=${CMAKE_CXX_COMPILER} -DNM=${CMAKE_NM}
    -DSRC=${PROJECT_SOURCE_DIR}/tests/regression/site_tree_far_center.cpp -DINC=${PROJECT_SOURCE_DIR}/src
    -DWORK=${PROJECT_BINARY_DIR}/site_tree_old_api -P ${PROJECT_SOURCE_DIR}/tests/regression/site_tree_old_api.cmake)
string(REPLACE ";" " " site_tree_old_api_args "${site_tree_old_api_args}")
add_test(NAME mhgp10_regression_site_tree_old_api
         COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_COMMAND} "-DARGS=${site_tree_old_api_args}" -DEXPECTED=0
                 -DEXPECT_LINE=site_tree_old_api_ok -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp10_regression_site_tree_old_api PROPERTIES LABELS "gate;regression" TIMEOUT 300)
# Campagne gravee des mutants des filtres flottants (raccord R2 du 30 septembre 2026) : chaque mutant est recompile avec
# la commande exacte du build (compile_commands.json), son objet remplace celui de la bibliotheque dans une copie, la
# porte est liee a cette copie puis jugee ; le temoin doit reproduire l'objet de la bibliotheque octet pour octet.
# Build sans sanitizer seulement (edition de liens simple) ; generateurs Makefile et Ninja. Pas de label fast.
# Filtres flottants de la tour (raccord R2 ; verification adverse du raccord, § 3.3) : sorties identiques sous les
# quatre modes d'arrondi, filtres coupes hors FE_TONEAREST et observes par les compteurs fp_* (un fil, quatre fils,
# fils de modes differents) ; refus a la compilation et contraction coupee (meme script que SiteTree, UNIT=tower,
# options globales MHGP10_FP_OPTIONS) ; refus a la configuration dans chaque CMAKE_CXX_FLAGS* et -ffp-contract=off
# partout.
add_executable(mhgp10_tower_rounding tests/regression/tower_rounding.cpp)
target_link_libraries(mhgp10_tower_rounding PRIVATE mhgp10_core)
target_compile_options(mhgp10_tower_rounding PRIVATE -frounding-math)
mhgp10_gate(NAME mhgp10_regression_tower_rounding COMMAND mhgp10_tower_rounding CODE 0 LINE "tower_rounding_ok"
            LABELS gate regression TIMEOUT 900)
set(tower_fast_math_args -DCXX=${CMAKE_CXX_COMPILER} -DCXX_ID=${CMAKE_CXX_COMPILER_ID}
    -DARCH=${CMAKE_SYSTEM_PROCESSOR} -DSRC_OPTIONS=${MHGP10_FP_OPTIONS} -DUNIT=tower
    -DSRC=${PROJECT_SOURCE_DIR}/src/tower/tower.cpp -DINC=${PROJECT_SOURCE_DIR}/src
    -P ${PROJECT_SOURCE_DIR}/tests/regression/site_tree_fast_math.cmake)
string(REPLACE ";" " " tower_fast_math_args "${tower_fast_math_args}")
add_test(NAME mhgp10_regression_tower_fast_math
         COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_COMMAND} "-DARGS=${tower_fast_math_args}" -DEXPECTED=0
                 -DEXPECT_LINE=tower_fast_math_refuse -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp10_regression_tower_fast_math PROPERTIES LABELS "gate;regression" TIMEOUT 600)
set(fp_flags_configure_args -DSRC_DIR=${PROJECT_SOURCE_DIR} -DWORK=${PROJECT_BINARY_DIR}/fp_flags_configure
    -DCXX=${CMAKE_CXX_COMPILER} -P ${PROJECT_SOURCE_DIR}/tests/regression/fp_flags_configure.cmake)
string(REPLACE ";" " " fp_flags_configure_args "${fp_flags_configure_args}")
add_test(NAME mhgp10_regression_fp_flags_configure
         COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_COMMAND} "-DARGS=${fp_flags_configure_args}" -DEXPECTED=0
                 "-DEXPECT_LINE=fp_flags_configure_ok refus=8 temoins=2"
                 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
set_tests_properties(mhgp10_regression_fp_flags_configure PROPERTIES LABELS "gate;regression" TIMEOUT 600)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND AND NOT MHGP10_SANITIZE AND NOT MHGP10_TSAN AND CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
  set(float_filter_mutants_args ${PROJECT_SOURCE_DIR}/tests/regression/float_filter_mutants.py ${PROJECT_BINARY_DIR}
      --lib=$<TARGET_FILE:mhgp10_core> --cxx-id=${CMAKE_CXX_COMPILER_ID} --arch=${CMAKE_SYSTEM_PROCESSOR}
      --src-options=${site_tree_fp_options} --fp-options=${MHGP10_FP_OPTIONS} --jobs=2)
  string(REPLACE ";" " " float_filter_mutants_args "${float_filter_mutants_args}")
  add_test(NAME mhgp10_regression_float_filter_mutants
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE} "-DARGS=${float_filter_mutants_args}"
                   "-DEXPECT_LINE=float_filter_mutants_ok familles=3/3 temoins=3/3 mutants_tues=80/80 equivalents=1/1"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_float_filter_mutants PROPERTIES LABELS "gate;regression;mutation"
                       TIMEOUT 3600 ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
endif()

add_executable(mhgp10_unit tests/unit/unit_main.cpp)
target_link_libraries(mhgp10_unit PRIVATE mhgp10_core)
mhgp10_gate(NAME mhgp10_unit COMMAND mhgp10_unit CODE 0 LINE "unit_ok" LABELS gate fast)

add_executable(mhgp10_rank_search tests/unit/rank_search.cpp)
target_include_directories(mhgp10_rank_search PRIVATE ${PROJECT_SOURCE_DIR}/src)
mhgp10_gate(NAME mhgp10_rank_search COMMAND mhgp10_rank_search CODE 0 LINE "rank_search_ok"
            LABELS gate regression)

add_executable(mhgp10_grid32_primitives tests/unit/grid32_primitives.cpp)
target_include_directories(mhgp10_grid32_primitives PRIVATE ${PROJECT_SOURCE_DIR}/src)
mhgp10_gate(NAME mhgp10_grid32_primitives COMMAND mhgp10_grid32_primitives CODE 0 LINE "grille32_ok"
            LABELS gate regression)

# Temoin de test (hors produit) : atteignabilite mutuelle + tete v10, pour juger la condensation contre
# sklearn.cluster.HDBSCAN la ou les deux objets coincident (K = 1, 2). HDBSCAN lui-meme n'est jamais
# reimplemente : les bancs l'appellent via scikit-learn.
add_executable(mhgp10_mreach_cluster tests/head/mreach_cluster.cpp tests/head/mreach.cpp)
target_include_directories(mhgp10_mreach_cluster PRIVATE ${PROJECT_SOURCE_DIR}/tests/head)
target_link_libraries(mhgp10_mreach_cluster PRIVATE mhgp10_core)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME mhgp10_cover_band_structural
           COMMAND ${Python3_EXECUTABLE} -B ${PROJECT_SOURCE_DIR}/tests/points/test_cover_band.py)
  set_tests_properties(mhgp10_cover_band_structural PROPERTIES LABELS "gate;points;fast" TIMEOUT 30)
  add_test(NAME mhgp10_dev_quotas
           COMMAND ${Python3_EXECUTABLE} -B ${PROJECT_SOURCE_DIR}/tests/points/test_dev_quotas.py)
  set_tests_properties(mhgp10_dev_quotas PROPERTIES LABELS "gate;points;fast" TIMEOUT 30)
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
  # discontinuite de cover, croisement multi-K, egalite de la borne de stabilite 2 epsilon de core, vote par coupe
  # non laminaire ; coupes de F1 et F3 aussi a r + delta, en conventions fermee et ouverte. Porte fast : ses ARGS
  # commencent par le script (forme des portes fast rejouees hors du build, raccord R2) ; -O passe par
  # l'environnement (PYTHONOPTIMIZE=1, equivalent de -O), la porte rend le meme code dans les deux modes.
  add_test(NAME mhgp10_regression_projection_facts
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=${PROJECT_SOURCE_DIR}/tests/regression/test_projection_facts.py ${PROJECT_BINARY_DIR}"
                   "-DEXPECT_LINE=projection_facts_ok faits=5/5 mutants_tues=14/14 equivalents_acceptes=2/2"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_projection_facts PROPERTIES LABELS "gate;regression;fast" TIMEOUT 300
                       ENVIRONMENT "PYTHONOPTIMIZE=1;PYTHONDONTWRITEBYTECODE=1")
  # Coherence documentaire de ces faits : statuts du registre racine (V10-P1 a P8), obligations OP16 a OP20 de
  # CLUSTER_v2, chemins cites, convention, errata. Lit docs/math a la racine du depot, absent du paquet G4 (git
  # archive de morsehgp3D_v10 seul) : pas de label fast.
  set(mhgp10_registry_gate ${PROJECT_SOURCE_DIR}/tests/regression/test_projection_registry.py)
  add_test(NAME mhgp10_regression_projection_registry
           COMMAND ${CMAKE_COMMAND} -DCMD=${Python3_EXECUTABLE}
                   "-DARGS=-O ${mhgp10_registry_gate} ${PROJECT_SOURCE_DIR}/.."
                   "-DEXPECT_LINE=projection_registry_ok familles=5/5 mutants_tues=14/14 equivalents_acceptes=2/2"
                   -DEXPECTED=0 -P ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
  set_tests_properties(mhgp10_regression_projection_registry PROPERTIES LABELS "gate;regression" TIMEOUT 120)
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
