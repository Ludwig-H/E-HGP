# Juge d'echelle JUG-EMST de la v12 (hors produit) : ordre un de la tour FULL = lien simple sur l'arbre couvrant
# euclidien minimal exact, avec plateaux. Construction propre, sans dependance au produit ni a la v11 ; CMake 3.22
# (VM G4) ; C++20 sans extensions ; -Wall -Wextra -Wpedantic -Werror.
#   cmake -S morsehgp3D_v12/juges/emst -B <build> -DCMAKE_BUILD_TYPE=Release
#   cmake --build <build> -j3 && ctest --test-dir <build> --output-on-failure -j3
# Portes : codes exacts (0 conforme, 1 desaccord, 2 usage, 3 entree ou invariant, 4 mutant tue) ; la porte des
# vidages de la v11 (label lidar) est sautee sans la variable d'environnement MHGP12_EMST_VIDAGES.
cmake_minimum_required(VERSION 3.22)
project(mhgp12_jug_emst CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(jug_emst_warnings -Wall -Wextra -Wpedantic -Werror)

add_executable(mhgp12_jug_emst src/jug_emst.cpp)
target_compile_options(mhgp12_jug_emst PRIVATE ${jug_emst_warnings})

add_executable(mhgp12_jug_emst_autotest src/autotest.cpp)
target_compile_options(mhgp12_jug_emst_autotest PRIVATE ${jug_emst_warnings})

# Mutants causaux : copies compilees a part, jamais une branche du juge.
add_executable(mhgp12_jug_emst_mutant_binaire src/jug_emst.cpp)
target_compile_options(mhgp12_jug_emst_mutant_binaire PRIVATE ${jug_emst_warnings})
target_compile_definitions(mhgp12_jug_emst_mutant_binaire PRIVATE MHGP12_MUTANT_BINAIRE)
add_executable(mhgp12_jug_emst_mutant_departage src/jug_emst.cpp)
target_compile_options(mhgp12_jug_emst_mutant_departage PRIVATE ${jug_emst_warnings})
target_compile_definitions(mhgp12_jug_emst_mutant_departage PRIVATE MHGP12_MUTANT_DEPARTAGE)

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
  set(jug_emst_tests ${PROJECT_SOURCE_DIR}/tests)
  set(jug_emst_reference ${PROJECT_SOURCE_DIR}/../../reference)
  set(jug_emst_work ${PROJECT_BINARY_DIR}/portes)

  # jug_emst_gate(<nom> <code attendu> <commande...>) : porte a code de sortie exact (un crash n'est jamais admis).
  function(jug_emst_gate name expected)
    add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DEXPECTED=${expected} -P
             ${PROJECT_SOURCE_DIR}/tests/run_expect.cmake -- ${ARGN})
  endfunction()

  jug_emst_gate(mhgp12_jug_emst_autotest 0 $<TARGET_FILE:mhgp12_jug_emst_autotest>)
  foreach(opt "" "-O")
    set(suffix "")
    if(opt STREQUAL "-O")
      set(suffix "_O")
    endif()
    jug_emst_gate(mhgp12_jug_emst_temoins${suffix} 0 ${Python3_EXECUTABLE} -S ${opt}
                  ${jug_emst_tests}/temoins.py --juge $<TARGET_FILE:mhgp12_jug_emst>
                  --travail ${jug_emst_work}/temoins${suffix})
    jug_emst_gate(mhgp12_jug_emst_oracle_rapide${suffix} 0 ${Python3_EXECUTABLE} -S ${opt}
                  ${jug_emst_tests}/oracle_rapide.py --juge $<TARGET_FILE:mhgp12_jug_emst>
                  --reference ${jug_emst_reference} --travail ${jug_emst_work}/oracle${suffix})
  endforeach()
  jug_emst_gate(mhgp12_jug_emst_mutant_binaire 4 ${Python3_EXECUTABLE} -S ${jug_emst_tests}/temoins.py
                --juge $<TARGET_FILE:mhgp12_jug_emst_mutant_binaire> --mutant binaire
                --travail ${jug_emst_work}/mutant_binaire)
  jug_emst_gate(mhgp12_jug_emst_mutant_departage 4 ${Python3_EXECUTABLE} -S ${jug_emst_tests}/temoins.py
                --juge $<TARGET_FILE:mhgp12_jug_emst_mutant_departage> --mutant departage
                --travail ${jug_emst_work}/mutant_departage)
  # Vidages MHGP11FUL1 de la v11 gelee (empreintes de MESURE.md, paragraphe 4) : sautee (code 77) sans donnees.
  add_test(NAME mhgp12_jug_emst_vidages_v11
           COMMAND ${Python3_EXECUTABLE} -S ${jug_emst_tests}/vidages_v11.py --juge $<TARGET_FILE:mhgp12_jug_emst>)
  set_tests_properties(mhgp12_jug_emst_vidages_v11 PROPERTIES SKIP_RETURN_CODE 77 LABELS lidar TIMEOUT 1800)
endif()
