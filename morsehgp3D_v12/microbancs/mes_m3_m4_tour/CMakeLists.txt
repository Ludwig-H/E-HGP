# Microbancs de la tour v12 (MES-M3, MES-M4) et outil de vidage de la v11, hors produit.
# CMake 3.22 (VM G4) ; C++20 sans extensions ; GCC 11.4 vise ; -Wall -Wextra -Wpedantic -Werror.
#   cmake -S v12_tour -B <build> -DCMAKE_BUILD_TYPE=Release \
#         -DMHGP11_SOURCE=<depot>/morsehgp3D_v11 -DMHGP11_BUILD=<construction Release u21 de la v11>
cmake_minimum_required(VERSION 3.22)
project(mhgp12_tour_microbancs CXX)

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

# Vidage de la v11 (lie a libmhgp11.a ; lit bench/whole_input.hpp de la v11).
add_executable(mhgp12_vidage vidage/vidage_v11.cpp)
target_include_directories(mhgp12_vidage PRIVATE "${MHGP11_SOURCE}/bench")
target_link_libraries(mhgp12_vidage PRIVATE mhgp11_lie)
target_compile_options(mhgp12_vidage PRIVATE ${mhgp12_warnings})

# MES-M3 : plus petite boule proposee (flottant) puis certifiee (LEM-T1 ou certificat exact), contre bounded_meb.
add_executable(mhgp12_mes_m3 mes_m3/mes_m3.cpp)
target_link_libraries(mhgp12_mes_m3 PRIVATE mhgp11_lie)
target_compile_options(mhgp12_mes_m3 PRIVATE ${mhgp12_warnings})
# Mutant causal (copie compilee a part, jamais une branche du chemin mesure) : LEM-T1 sans le test S dans F.
add_executable(mhgp12_mes_m3_mutant_sans_s_dans_f mes_m3/mes_m3.cpp)
target_link_libraries(mhgp12_mes_m3_mutant_sans_s_dans_f PRIVATE mhgp11_lie)
target_compile_options(mhgp12_mes_m3_mutant_sans_s_dans_f PRIVATE ${mhgp12_warnings})
target_compile_definitions(mhgp12_mes_m3_mutant_sans_s_dans_f PRIVATE MHGP12_MUTANT_T1_SANS_S_DANS_F)

# MES-M4 : foret sans lots (noyau union-find par taille, contraction des plateaux, numerotation canonique).
add_executable(mhgp12_mes_m4 mes_m4/mes_m4.cpp)
target_compile_options(mhgp12_mes_m4 PRIVATE ${mhgp12_warnings})
add_executable(mhgp12_mes_m4_mutant_sans_contraction mes_m4/mes_m4.cpp)
target_compile_options(mhgp12_mes_m4_mutant_sans_contraction PRIVATE ${mhgp12_warnings})
target_compile_definitions(mhgp12_mes_m4_mutant_sans_contraction PRIVATE MHGP12_MUTANT_SANS_CONTRACTION)

# Porte de l'admission du lecteur MHGP12DP (CST-0225) : neuf fichiers synthetiques, dont le temoin de 88 octets de
# l'auditeur ; independante de la v11. Usage : mhgp12_format_test <dossier de travail> (code 0 conforme, 1 ecart).
add_executable(mhgp12_format_test tests/format_reader_test.cpp)
target_include_directories(mhgp12_format_test PRIVATE common)
target_compile_options(mhgp12_format_test PRIVATE ${mhgp12_warnings})
