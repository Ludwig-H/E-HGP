# MorseHGP3D v11 : squelette de construction (docs/ARCHITECTURE.md, paragraphe 5).
# Port du CMakeLists.txt de la v10 du raccord R2 (commit 865f5e6) : options, sanitizers, runtime partage de Clang,
# portes a code exact. Ce qui est nouveau : les modules s'ajoutent par leurs seuls fichiers, sans editer celui-ci.
#
# ---------------------------------------------------------------------------------------------------------------
# MODE D'EMPLOI POUR L'AUTEUR D'UN MODULE (table des modules : cmake/modules.cmake, copie du paragraphe 2)
#
# Un module <m> possede src/<m>/ et tests/<m>/. Il ne modifie jamais ce fichier, cmake/, tests/support/ ni tools/.
#
# 1. src/<m>/module.cmake : les sources du module dans la bibliotheque statique mhgp11 (chemins relatifs a src/<m>/).
#    Un module tout en en-tetes appelle mhgp11_module_sources() sans argument. L'en-tete public est
#    src/<m>/<m>.hpp ; un autre module n'inclut que lui : #include "<m>/<m>.hpp". Exemple, src/sched/module.cmake :
#
#        mhgp11_module_sources(pool.cpp)
#
#    Le fichier est inclus dans la portee de ce CMakeLists.txt : pour une option propre a un fichier, les commandes
#    CMake habituelles restent permises sur la cible mhgp11 (set_source_files_properties, target_link_libraries).
#
# 2. tests/<m>/tests.cmake : les portes du module, par les aides de cmake/gates.cmake (mode d'emploi en tete de ce
#    fichier ; un add_test direct est refuse). Exemple, tests/sched/tests.cmake :
#
#        mhgp11_add_unit(mhgp11_sched_unit SOURCES pool_test.cpp sort_test.cpp
#                        GROUPS pool_cover pool_exceptions sort LABELS fast)
#        mhgp11_python_gate(mhgp11_sched_stress 0 test_stress.py ${PROJECT_BINARY_DIR} LABELS long TIMEOUT 1800)
#
#    Un test C++ inclut "test.hpp" (tests/support : MHGP11_TEST, CHECK, CHECK_EQ, REQUIRE, MHGP11_TEST_MAIN) et se lie
#    a mhgp11. Un script Python importe mhgp11_gate (tests/support). Les donnees LiDAR se trouvent par la variable
#    d'environnement MHGP11_DATA_DIR ; une porte de label lidar est sautee si elle manque.
#
# 3. tests/mutants/<m>.json : les mutants du module (format : en-tete de tests/mutants/run_mutants.py). La porte
#    mhgp11_mutants_<m> (labels mutant et long) est enregistree des que le fichier existe.
#
# 4. Construire un seul module et ses dependances pendant que les autres s'ecrivent :
#
#        cmake -S morsehgp3D_v11 -B /tmp/v11-build/<m> -DCMAKE_BUILD_TYPE=Release -DMHGP11_MODULES=<m>
#        cmake --build /tmp/v11-build/<m> -j2 && ctest --test-dir /tmp/v11-build/<m> -LE long --output-on-failure
#
#    -LE long ecarte les portes de plus d'une minute (campagnes de mutants, stress) : elles se jouent sur la VM G4.
#
#    MHGP11_MODULES liste les unites a construire ET a tester : des modules de la table, plus "cli" (cli/cli.cmake)
#    et "reference" (reference/tests.cmake). La bibliotheque recoit ces modules et toutes leurs dependances ; les
#    portes sont celles des unites listees seulement. Vide (defaut) : toutes les unites presentes. "cli" entraine
#    tous les modules presents. Variables lisibles par module.cmake, tests.cmake et cli.cmake : MHGP11_UNITS (unites
#    testees), MHGP11_LIBRARY_MODULES (modules de la bibliotheque), MHGP11_COORD_BITS, MHGP11_SANITIZE, MHGP11_TSAN,
#    MHGP11_POISON.
#
# 5. cli/cli.cmake declare l'executable mhgp11 (add_executable, lie a la bibliotheque mhgp11) ; ses portes vont dans
#    tests/cli/tests.cmake. reference/tests.cmake declare les portes de l'oracle Python.
# ---------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v11 LANGUAGES CXX)

# C++20 sans extension, avertissements traites en erreurs, aucune dependance externe dans le produit.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(MHGP11_MODULES "" CACHE STRING "Unites a construire et a tester (modules, cli, reference) ; vide : toutes")
set(MHGP11_COORD_BITS 21 CACHE STRING "Bits par coordonnee : 18, 21 (defaut) ou 24 ; qualifications par capture")
option(MHGP11_SANITIZE "ASan + UBSan" OFF)
option(MHGP11_TSAN "ThreadSanitizer" OFF)
option(MHGP11_POISON "Empoisonne les tampons non initialises (0xA5)" OFF)
set(MHGP11_MARCH "" CACHE STRING "Jeu d'instructions cible (-march), par exemple x86-64-v3 ; vide : aucun")
set(MHGP11_MUTANT_JOBS 0 CACHE STRING "Mutants traites en parallele par une porte de mutants ; 0 : d'apres la machine")

if(NOT MHGP11_COORD_BITS MATCHES "^(18|21|24)$")
  message(FATAL_ERROR "mhgp11_coord_bits_invalide : MHGP11_COORD_BITS=${MHGP11_COORD_BITS}, attendu 18, 21 ou 24")
endif()
if(MHGP11_SANITIZE AND MHGP11_TSAN)
  message(FATAL_ERROR "MHGP11_SANITIZE et MHGP11_TSAN s'excluent (ASan et TSan ne se combinent pas)")
endif()

set(MHGP11_WARNING_OPTIONS -Wall -Wextra -Wpedantic -Werror)
add_compile_options(${MHGP11_WARNING_OPTIONS})

# Garde simple contre -ffast-math (doctrine flottante, docs/ARCHITECTURE.md, paragraphe 4).
# La v10 s'appuyait sur un contrat de compilation (IEEE-754 strict, ni contraction, ni reassociation) et a du refuser
# une a une les options qui l'affaiblissent : cmake/fp_flags.cmake du raccord R2 fait 486 lignes et declare encore
# quatre canaux hors contrat. La v11 ne porte PAS ce fichier, parce qu'elle n'a pas ce contrat :
#   F1  aucune decision n'est prise en flottant ;
#   F2  les noyaux entiers portes en binaire64 restent des entiers de valeur absolue < 2^53, donc exacts sous tout
#       mode d'arrondi et tout ordre de sommation ;
#   F3  une cle approchee n'est faite que de conversions, produits, quotients et sommes de termes de meme signe,
#       avec une borne d'erreur propagee par expression ;
#   F4  deux cles ne sont declarees ordonnees que si leur ecart depasse la marge prouvee, sinon la comparaison est
#       rejouee en exact ;
#   F5  defense en profondeur, sans role dans les preuves : refus de __FAST_MATH__ et auto-test au demarrage.
# Ce qui suit est la part de F5 qui revient a la construction : on demande au compilateur lui-meme si les options de
# la configuration active definissent __FAST_MATH__ (-ffast-math, -Ofast, quelle que soit leur orthographe ou la
# variable qui les porte), au lieu de reconnaitre des jetons. Ce n'est pas un contrat : une option posee ailleurs
# (cible, source) echappe a cette sonde ; le #error de src/core/types.hpp, que toute unite inclut, la refuse alors a
# la compilation. La sonde juge les options EFFECTIVES : "-Ofast -O3" (CMAKE_CXX_FLAGS=-Ofast, type Release) ne
# definit pas __FAST_MATH__, -O3 l'emporte, et la configuration est admise a raison.
# Portes : mhgp11_support_configure_fast_math_cxx_flags, mhgp11_support_configure_fast_math_release_ofast,
# mhgp11_core_fast_math_refusal, mhgp11_core_ofast_refusal.
string(TOUPPER "${CMAKE_BUILD_TYPE}" mhgp11_config)
separate_arguments(mhgp11_probe_flags UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${mhgp11_config}}")
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${mhgp11_probe_flags} -dM -E -x c++ /dev/null
                RESULT_VARIABLE mhgp11_probe_rc OUTPUT_VARIABLE mhgp11_probe_macros ERROR_VARIABLE mhgp11_probe_err)
if(NOT mhgp11_probe_rc STREQUAL "0")
  message(FATAL_ERROR "sonde des macros du compilateur illisible (code ${mhgp11_probe_rc}) : ${mhgp11_probe_err}")
endif()
if(mhgp11_probe_macros MATCHES "#define __FAST_MATH__")
  message(FATAL_ERROR "mhgp11_fast_math_interdit : les options de compilation definissent __FAST_MATH__ "
                      "(-ffast-math, -Ofast) ; docs/ARCHITECTURE.md, paragraphe 4, F5")
endif()

if(MHGP11_MARCH)
  add_compile_options(-march=${MHGP11_MARCH})
endif()
if(MHGP11_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()
if(MHGP11_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
# Clang avec sanitizer : runtime partage (-shared-libsan), ce que GCC fait par defaut. Reparation du raccord R2 de la
# v10 (constat B1) : avec le runtime statique de Clang, un executable de test qui remplace operator new ne se lie pas
# (definitions multiples) et une bibliotheque de prechargement passe apres le runtime ; les harnais d'injection de
# fautes etaient alors inoperants. Runtime partage absent : refus explicite a la configuration, jamais un harnais muet.
if((MHGP11_SANITIZE OR MHGP11_TSAN) AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-runtime-dir RESULT_VARIABLE mhgp11_rt_rc
                  OUTPUT_VARIABLE mhgp11_rt_dir ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(mhgp11_rt_sanitizer thread)
  if(MHGP11_SANITIZE)
    set(mhgp11_rt_sanitizer address,undefined)
  endif()
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_FLAGS -fsanitize=${mhgp11_rt_sanitizer})
  set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=${mhgp11_rt_sanitizer} -shared-libsan)
  check_cxx_source_compiles("int main() { return 0; }" MHGP11_CLANG_SHARED_SANITIZER_RUNTIME)
  unset(CMAKE_REQUIRED_FLAGS)
  unset(CMAKE_REQUIRED_LINK_OPTIONS)
  if(NOT mhgp11_rt_rc STREQUAL "0" OR NOT IS_DIRECTORY "${mhgp11_rt_dir}"
     OR NOT MHGP11_CLANG_SHARED_SANITIZER_RUNTIME)
    message(FATAL_ERROR "mhgp11_runtime_partage_absent : Clang avec sanitizer exige le runtime partage "
                        "(-shared-libsan, dossier [${mhgp11_rt_dir}]) ; l'installer, ou construire les variantes a "
                        "sanitizer avec GCC")
  endif()
  add_link_options(-shared-libsan "LINKER:-rpath,${mhgp11_rt_dir}")
endif()

find_package(Threads REQUIRED)

# ---- Unites et modules -----------------------------------------------------------------------------------------
include(cmake/modules.cmake)

set(mhgp11_present_modules)
foreach(module IN LISTS MHGP11_ALL_MODULES)
  if(EXISTS ${PROJECT_SOURCE_DIR}/src/${module}/module.cmake)
    list(APPEND mhgp11_present_modules ${module})
  endif()
endforeach()
set(mhgp11_has_cli FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/cli/cli.cmake)
  set(mhgp11_has_cli TRUE)
endif()
set(mhgp11_has_reference FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/reference/tests.cmake)
  set(mhgp11_has_reference TRUE)
endif()

if(MHGP11_MODULES)
  set(MHGP11_UNITS ${MHGP11_MODULES})
  list(REMOVE_DUPLICATES MHGP11_UNITS)
else()
  set(MHGP11_UNITS ${mhgp11_present_modules})
  if(mhgp11_has_cli)
    list(APPEND MHGP11_UNITS cli)
  endif()
  if(mhgp11_has_reference)
    list(APPEND MHGP11_UNITS reference)
  endif()
endif()

set(mhgp11_requested_modules)
foreach(unit IN LISTS MHGP11_UNITS)
  if(unit STREQUAL "cli")
    if(NOT mhgp11_has_cli)
      message(FATAL_ERROR "mhgp11 : unite cli demandee, cli/cli.cmake absent")
    endif()
    list(APPEND mhgp11_requested_modules ${mhgp11_present_modules})
  elseif(unit STREQUAL "reference")
    if(NOT mhgp11_has_reference)
      message(FATAL_ERROR "mhgp11 : unite reference demandee, reference/tests.cmake absent")
    endif()
  elseif(unit IN_LIST MHGP11_ALL_MODULES)
    list(APPEND mhgp11_requested_modules ${unit})
  else()
    message(FATAL_ERROR "mhgp11 : unite inconnue '${unit}' dans MHGP11_MODULES "
                        "(admis : ${MHGP11_ALL_MODULES} cli reference)")
  endif()
endforeach()
if(NOT mhgp11_requested_modules AND NOT MHGP11_UNITS)
  message(FATAL_ERROR "mhgp11 : aucun module present (src/<module>/module.cmake)")
endif()
mhgp11_module_closure(MHGP11_LIBRARY_MODULES ${mhgp11_requested_modules})
foreach(module IN LISTS MHGP11_LIBRARY_MODULES)
  if(NOT module IN_LIST mhgp11_present_modules)
    message(FATAL_ERROR "mhgp11 : le module '${module}' est requis (unites : ${MHGP11_UNITS}) mais "
                        "src/${module}/module.cmake est absent ; limiter la construction par -DMHGP11_MODULES=...")
  endif()
endforeach()
message(STATUS "mhgp11 : unites testees = ${MHGP11_UNITS} ; modules de la bibliotheque = ${MHGP11_LIBRARY_MODULES} ; "
               "bits = ${MHGP11_COORD_BITS}")

# ---- Bibliotheque ----------------------------------------------------------------------------------------------
# mhgp11_module_sources(<fichier>...) : sources du module en cours (chemins relatifs a son dossier).
function(mhgp11_module_sources)
  foreach(source IN LISTS ARGN)
    if(NOT EXISTS ${MHGP11_MODULE_DIR}/${source})
      message(FATAL_ERROR "module ${MHGP11_MODULE} : source absente ${MHGP11_MODULE_DIR}/${source}")
    endif()
    target_sources(mhgp11 PRIVATE ${MHGP11_MODULE_DIR}/${source})
  endforeach()
endfunction()

if(MHGP11_LIBRARY_MODULES)
  add_library(mhgp11 STATIC)
  target_include_directories(mhgp11 PUBLIC ${PROJECT_SOURCE_DIR}/src)
  target_compile_definitions(mhgp11 PUBLIC MHGP11_COORD_BITS=${MHGP11_COORD_BITS})
  target_link_libraries(mhgp11 PUBLIC Threads::Threads)
  if(MHGP11_POISON)
    # PRIVATE : l'empoisonnement vit dans src/core/buffer.cpp seulement ; aucune unite cliente n'en depend.
    target_compile_definitions(mhgp11 PRIVATE MHGP11_POISON)
  endif()
  foreach(module IN LISTS MHGP11_LIBRARY_MODULES)
    set(MHGP11_MODULE ${module})
    set(MHGP11_MODULE_DIR ${PROJECT_SOURCE_DIR}/src/${module})
    if(NOT EXISTS ${MHGP11_MODULE_DIR}/${module}.hpp)
      message(FATAL_ERROR "module ${module} : en-tete public absent (src/${module}/${module}.hpp)")
    endif()
    include(${MHGP11_MODULE_DIR}/module.cmake)
  endforeach()
  unset(MHGP11_MODULE)
  unset(MHGP11_MODULE_DIR)
endif()

if("cli" IN_LIST MHGP11_UNITS)
  include(cli/cli.cmake)
endif()

# ---- Portes ----------------------------------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
  include(cmake/gates.cmake)

  # Portes du socle lui-meme (cadre de test, run_expect.cmake, outils) : jouees avec celles de core.
  if("core" IN_LIST MHGP11_UNITS)
    include(tests/support/tests.cmake)
  endif()
  foreach(unit IN LISTS MHGP11_UNITS)
    if(unit STREQUAL "reference")
      include(reference/tests.cmake)
    elseif(EXISTS ${PROJECT_SOURCE_DIR}/tests/${unit}/tests.cmake)
      include(tests/${unit}/tests.cmake)
    endif()
  endforeach()

  # Regles de proprete lisibles dans le texte (docs/ARCHITECTURE.md, paragraphe 1), sur les unites construites.
  # Construction complete (MHGP11_MODULES vide) : tout l'arbre est juge.
  set(mhgp11_style_units)
  if(MHGP11_MODULES)
    set(mhgp11_style_units --units ${MHGP11_UNITS})
  endif()
  mhgp11_python_gate(mhgp11_style 0 ${PROJECT_SOURCE_DIR}/tools/check_style.py --root ${PROJECT_SOURCE_DIR}
                     ${mhgp11_style_units} LABELS fast)

  # Mutants : une porte par manifeste tests/mutants/<unite>.json present. La copie mutee est construite avec le
  # compilateur et le profil de ce build, sans sanitizer : sous MHGP11_SANITIZE ou MHGP11_TSAN les portes de mutants
  # ne sont pas enregistrees (une campagne par jeu de builds suffit). Le compte rendu de la campagne (empreintes du
  # manifeste et des sources, verdict et cause de chaque mutant) est ecrit dans <build>/mutants_<unite>.json.
  set(mhgp11_mutant_profile -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DMHGP11_COORD_BITS=${MHGP11_COORD_BITS} -DMHGP11_POISON=${MHGP11_POISON} -DMHGP11_MARCH=${MHGP11_MARCH})
  foreach(unit IN LISTS MHGP11_UNITS)
    set(manifest ${PROJECT_SOURCE_DIR}/tests/mutants/${unit}.json)
    if(EXISTS ${manifest})
      # Manifeste a jour (schema, motifs presents une seule fois) : porte rapide, sans construction.
      mhgp11_python_gate(mhgp11_mutants_${unit}_manifest 0 ${PROJECT_SOURCE_DIR}/tests/mutants/run_mutants.py
                         --manifest ${manifest} --source ${PROJECT_SOURCE_DIR} --check LABELS mutant fast)
      if(NOT MHGP11_SANITIZE AND NOT MHGP11_TSAN)
        set(mutant_args --manifest ${manifest} --source ${PROJECT_SOURCE_DIR}
            --work ${PROJECT_BINARY_DIR}/mutants/${unit} --report ${PROJECT_BINARY_DIR}/mutants_${unit}.json
            --jobs ${MHGP11_MUTANT_JOBS}
            --cmake ${CMAKE_COMMAND} --ctest ${CMAKE_CTEST_COMMAND} --generator ${CMAKE_GENERATOR})
        foreach(definition IN LISTS mhgp11_mutant_profile)
          list(APPEND mutant_args --cmake-arg=${definition})
        endforeach()
        mhgp11_python_gate(mhgp11_mutants_${unit} 0 ${PROJECT_SOURCE_DIR}/tests/mutants/run_mutants.py
                           ${mutant_args} LABELS mutant long TIMEOUT 7200)
      endif()
    endif()
  endforeach()

  mhgp11_check_gate_registry()
endif()
