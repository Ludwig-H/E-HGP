# Microbanc MES-G1 de la v12 (levier G-L3 : saut certifie sans census), hors produit.
# CMake 3.22 (VM G4) ; C++20 sans extensions ; GCC 11.4 vise ; -Wall -Wextra -Wpedantic -Werror.
# Lie a la v11 gelee (libmhgp11.a, profil u21, moteur ac081a06f), comme mes_m3_m4_tour dont il lit le format des
# vidages (common/format.hpp) et le support canonique (mes_m3/meb_cert.hpp) :
#   cmake -S mes_g1_saut -B <build> -DCMAKE_BUILD_TYPE=Release \
#         -DMHGP11_SOURCE=<depot>/morsehgp3D_v11 -DMHGP11_BUILD=<construction Release u21 de la v11>
#   ctest --test-dir <build> --output-on-failure        (porte et mutant, codes exacts)
cmake_minimum_required(VERSION 3.22)
project(mhgp12_mes_g1 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(MHGP11_SOURCE "" CACHE PATH "sources morsehgp3D_v11 (moteur ac081a06f)")
set(MHGP11_BUILD "" CACHE PATH "construction Release de la v11 contenant libmhgp11.a")
set(MHGP11_COORD_BITS 21 CACHE STRING "profil de la construction v11 liee (21 = produit)")
if(NOT EXISTS "${MHGP11_SOURCE}/src/tower/meb.hpp")
  message(FATAL_ERROR "MHGP11_SOURCE invalide : ${MHGP11_SOURCE}")
endif()
if(NOT EXISTS "${MHGP11_BUILD}/libmhgp11.a")
  message(FATAL_ERROR "MHGP11_BUILD invalide (libmhgp11.a absent) : ${MHGP11_BUILD}")
endif()

find_package(Threads REQUIRED)
add_library(mhgp11_lie STATIC IMPORTED)
set_target_properties(mhgp11_lie PROPERTIES
  IMPORTED_LOCATION "${MHGP11_BUILD}/libmhgp11.a"
  INTERFACE_INCLUDE_DIRECTORIES "${MHGP11_SOURCE}/src"
  INTERFACE_COMPILE_DEFINITIONS "MHGP11_COORD_BITS=${MHGP11_COORD_BITS}"
  INTERFACE_LINK_LIBRARIES Threads::Threads)

set(mhgp12_warnings -Wall -Wextra -Wpedantic -Werror)

# Banc et porte : voisins exacts, fenetres de Morton, tests exacts contre la plus petite boule (v11), census de la v11.
add_executable(mhgp12_mes_g1 mes_g1.cpp)
target_link_libraries(mhgp12_mes_g1 PRIVATE mhgp11_lie)
target_compile_options(mhgp12_mes_g1 PRIVATE ${mhgp12_warnings})
# Mutant causal (copie compilee a part, jamais une branche du chemin mesure) : un site sur la sphere (cote nul) est
# admis comme interieur.
add_executable(mhgp12_mes_g1_mutant_cote_nul mes_g1.cpp)
target_link_libraries(mhgp12_mes_g1_mutant_cote_nul PRIVATE mhgp11_lie)
target_compile_options(mhgp12_mes_g1_mutant_cote_nul PRIVATE ${mhgp12_warnings})
target_compile_definitions(mhgp12_mes_g1_mutant_cote_nul PRIVATE MHGP12_MUTANT_G1_COTE_NUL)

# Seconde moitie de G-L3 : le vidage de la v11 (meme source que mes_m3_m4_tour, option --bras-saut : quatrieme bras
# replique_v12_saut de la mesure de resolution), construit ici pour que le pilote de ce microbanc ne depende que de sa
# propre construction ; et son mutant causal du controle de foret (cibles decalees d'un representant : code 1 exige).
add_executable(mhgp12_vidage ../mes_m3_m4_tour/vidage/vidage_v11.cpp)
target_include_directories(mhgp12_vidage PRIVATE "${MHGP11_SOURCE}/bench")
target_link_libraries(mhgp12_vidage PRIVATE mhgp11_lie)
target_compile_options(mhgp12_vidage PRIVATE ${mhgp12_warnings})
add_executable(mhgp12_vidage_mutant_cibles_decalees ../mes_m3_m4_tour/vidage/vidage_v11.cpp)
target_include_directories(mhgp12_vidage_mutant_cibles_decalees PRIVATE "${MHGP11_SOURCE}/bench")
target_link_libraries(mhgp12_vidage_mutant_cibles_decalees PRIVATE mhgp11_lie)
target_compile_options(mhgp12_vidage_mutant_cibles_decalees PRIVATE ${mhgp12_warnings})
target_compile_definitions(mhgp12_vidage_mutant_cibles_decalees PRIVATE MHGP12_MUTANT_SAUT_CIBLES_DECALEES)

# Portes a code de sortie exact (../../cmake/run_expect.cmake de la v12) : porte conforme (0), mutant tue (1).
enable_testing()
set(mhgp12_run_expect "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/run_expect.cmake")
add_test(NAME mhgp12_mes_g1_porte
         COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:mhgp12_mes_g1> -DNARGS=1 -DARG0=--porte -DEXPECTED=0
                 "-DEXPECT_LINE={\"porte\":\"mes_g1\",\"mutant_cote_nul\":false,\"temoins\":4,\"ecarts\":0}"
                 -P ${mhgp12_run_expect})
add_test(NAME mhgp12_mes_g1_porte_mutant_cote_nul
         COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:mhgp12_mes_g1_mutant_cote_nul> -DNARGS=1 -DARG0=--porte
                 -DEXPECTED=1 -P ${mhgp12_run_expect})
add_test(NAME mhgp12_mes_g1_usage
         COMMAND ${CMAKE_COMMAND} -DCMD=$<TARGET_FILE:mhgp12_mes_g1> -DNARGS=1 -DARG0=--inconnue -DEXPECTED=2
                 -P ${mhgp12_run_expect})
# Porte du pilote et du juge (REGLE_G1) sur sorties synthetiques, sous python3 -S -O (aucune garde par assert).
find_program(MHGP12_PYTHON3 python3)
if(MHGP12_PYTHON3)
  add_test(NAME mhgp12_mes_g1_pilote_juge
           COMMAND ${CMAKE_COMMAND} -DCMD=${MHGP12_PYTHON3} -DNARGS=3 -DARG0=-S -DARG1=-O
                   -DARG2=${CMAKE_CURRENT_SOURCE_DIR}/test_pilote_g1.py -DEXPECTED=0 -P ${mhgp12_run_expect})
  set_tests_properties(mhgp12_mes_g1_pilote_juge PROPERTIES LABELS "gate;mes_g1" TIMEOUT 120
                       ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
endif()
set_tests_properties(mhgp12_mes_g1_porte mhgp12_mes_g1_porte_mutant_cote_nul mhgp12_mes_g1_usage
                     PROPERTIES LABELS "gate;mes_g1" TIMEOUT 60)
