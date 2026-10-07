# MorseHGP3D v12 : squelette de construction (ARCHITECTURE.md de la v11, paragraphe 5).
# Port du CMakeLists.txt de la v11 (commit ac081a06f), lui-meme port de celui de la v10 du raccord R2 (commit 865f5e6) :
# options, sanitizers, runtime partage de Clang, portes a code exact ; les modules s'ajoutent par leurs seuls fichiers,
# sans editer celui-ci. Ce que la v12 change : prefixes mhgp12 et MHGP12_, profils 21 (defaut) et 24 seulement (18
# abandonne, decision D6 ; 32 differe), option CUDA retiree tant qu'aucun module ne compile pour le GPU.
#
# ---------------------------------------------------------------------------------------------------------------
# MODE D'EMPLOI POUR L'AUTEUR D'UN MODULE (table des modules : cmake/modules.cmake, copie du paragraphe 5 de
# docs/ARCHITECTURE.md)
#
# Un module <m> possede src/<m>/ et tests/<m>/. Il ne modifie jamais ce fichier, cmake/, tests/support/ ni tools/.
#
# 1. src/<m>/module.cmake : les sources du module dans la bibliotheque statique mhgp12 (chemins relatifs a src/<m>/).
#    Un module tout en en-tetes appelle mhgp12_module_sources() sans argument. L'en-tete public est
#    src/<m>/<m>.hpp ; un autre module n'inclut que lui : #include "<m>/<m>.hpp". Exemple, src/sched/module.cmake :
#
#        mhgp12_module_sources(pool.cpp)
#
#    Le fichier est inclus dans la portee de ce CMakeLists.txt : pour une option propre a un fichier, les commandes
#    CMake habituelles restent permises sur la cible mhgp12 (set_source_files_properties, target_link_libraries).
#
# 2. tests/<m>/tests.cmake : les portes du module, par les aides de cmake/gates.cmake (mode d'emploi en tete de ce
#    fichier ; un add_test direct est refuse). Exemple, tests/sched/tests.cmake :
#
#        mhgp12_add_unit(mhgp12_sched_unit SOURCES pool_test.cpp sort_test.cpp
#                        GROUPS pool_cover pool_exceptions sort LABELS fast)
#        mhgp12_python_gate(mhgp12_sched_stress 0 test_stress.py ${PROJECT_BINARY_DIR} LABELS long TIMEOUT 1800)
#
#    Un test C++ inclut "test.hpp" (tests/support : MHGP12_TEST, CHECK, CHECK_EQ, REQUIRE, MHGP12_TEST_MAIN) et se lie
#    a mhgp12. Un script Python importe mhgp12_gate (tests/support). Les donnees LiDAR se trouvent par la variable
#    d'environnement MHGP12_DATA_DIR ; une porte de label lidar est sautee si elle manque.
#
# 3. tests/mutants/<m>.json : les mutants du module (format : en-tete de tests/mutants/run_mutants.py). La porte
#    mhgp12_mutants_<m> (labels mutant et long) est enregistree des que le fichier existe.
#
# 4. Construire un seul module et ses dependances pendant que les autres s'ecrivent :
#
#        cmake -S morsehgp3D_v12 -B /tmp/v12-build/<m> -DCMAKE_BUILD_TYPE=Release -DMHGP12_MODULES=<m>
#        cmake --build /tmp/v12-build/<m> -j2 && ctest --test-dir /tmp/v12-build/<m> -LE long --output-on-failure
#
#    -LE long ecarte les portes de plus d'une minute (campagnes de mutants, stress) : elles se jouent sur la VM G4.
#
#    MHGP12_MODULES liste les unites a construire ET a tester : des modules de la table, plus "cli" (cli/cli.cmake)
#    et "reference" (reference/tests.cmake). La bibliotheque recoit ces modules et toutes leurs dependances ; les
#    portes sont celles des unites listees seulement. Vide (defaut) : toutes les unites presentes. "cli" entraine
#    tous les modules presents. Variables lisibles par module.cmake, tests.cmake et cli.cmake : MHGP12_UNITS (unites
#    testees), MHGP12_LIBRARY_MODULES (modules de la bibliotheque), MHGP12_COORD_BITS, MHGP12_SANITIZE, MHGP12_TSAN,
#    MHGP12_POISON.
#
# 5. cli/cli.cmake declare l'executable mhgp12 (add_executable, lie a la bibliotheque mhgp12) ; ses portes vont dans
#    tests/cli/tests.cmake. reference/tests.cmake declare les portes de l'oracle Python.
# ---------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(morsehgp3d_v12 LANGUAGES CXX)

# C++20 sans extension, avertissements traites en erreurs, aucune dependance externe dans le produit.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(MHGP12_MODULES "" CACHE STRING "Unites a construire et a tester (modules, cli, reference) ; vide : toutes")
set(MHGP12_COORD_BITS 21 CACHE STRING "Bits par coordonnee : 21 (defaut) ou 24 ; chaque profil a ses portes")
option(MHGP12_SANITIZE "ASan + UBSan" OFF)
option(MHGP12_TSAN "ThreadSanitizer" OFF)
option(MHGP12_POISON "Empoisonne les tampons non initialises (0xA5)" OFF)
set(MHGP12_MARCH "" CACHE STRING "Jeu d'instructions cible (-march), par exemple x86-64-v3 ; vide : aucun")
set(MHGP12_MUTANT_JOBS 0 CACHE STRING "Mutants traites en parallele par une porte de mutants ; 0 : d'apres la machine")

# Profil de coordonnees (decision D6 de la v12) : 21 bits (defaut) ou 24. Le profil 18 bits est abandonne ; 32 bits
# attend l'arithmetique en repere local (docs/CONTRAT_NUMERIQUE.md), qui arrive dans une tranche ulterieure. Chaque
# refus a son jeton et sa porte (tests/support/tests.cmake) ; core/types.hpp refuse les memes valeurs a la compilation.
if(MHGP12_COORD_BITS STREQUAL "18")
  message(FATAL_ERROR "mhgp12_coord_bits_18_abandonne : MHGP12_COORD_BITS=18 ; le profil 18 bits est abandonne "
                      "(decision D6 de la v12), attendu 21 (defaut) ou 24")
elseif(MHGP12_COORD_BITS STREQUAL "32")
  message(FATAL_ERROR "mhgp12_coord_bits_32_differe : MHGP12_COORD_BITS=32 est refuse pour l'instant ; "
                      "l'arithmetique en repere local (B <= 32, docs/CONTRAT_NUMERIQUE.md) arrive dans une tranche "
                      "ulterieure, attendu 21 (defaut) ou 24")
elseif(NOT MHGP12_COORD_BITS MATCHES "^(21|24)$")
  message(FATAL_ERROR "mhgp12_coord_bits_invalide : MHGP12_COORD_BITS=${MHGP12_COORD_BITS}, attendu 21 ou 24")
endif()
if(MHGP12_SANITIZE AND MHGP12_TSAN)
  message(FATAL_ERROR "MHGP12_SANITIZE et MHGP12_TSAN s'excluent (ASan et TSan ne se combinent pas)")
endif()

# Option CUDA de la v11 non reprise : aucun module du socle ne compile pour le GPU, et une option n'existe que si une
# porte l'exerce. Le module catalogue la ramenera, avec le dialecte CUDA20 declare a la main pour CMake 3.22 (VM G4),
# qui ne le connait pas (CMakeLists.txt de la v11, commit ac081a06f).
set(MHGP12_WARNING_OPTIONS -Wall -Wextra -Wpedantic -Werror)
add_compile_options(${MHGP12_WARNING_OPTIONS})

# Garde simple contre -ffast-math (doctrine flottante, ARCHITECTURE.md de la v11, paragraphe 4).
# La v10 s'appuyait sur un contrat de compilation (IEEE-754 strict, ni contraction, ni reassociation) et a du refuser
# une a une les options qui l'affaiblissent : cmake/fp_flags.cmake du raccord R2 fait 486 lignes et declare encore
# quatre canaux hors contrat. Ni la v11 ni la v12 ne portent ce fichier, parce qu'elles n'ont pas ce contrat :
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
# Portes : mhgp12_support_configure_fast_math_cxx_flags, mhgp12_support_configure_fast_math_release_ofast,
# mhgp12_core_fast_math_refusal, mhgp12_core_ofast_refusal.
string(TOUPPER "${CMAKE_BUILD_TYPE}" mhgp12_config)
separate_arguments(mhgp12_probe_flags UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${mhgp12_config}}")
execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${mhgp12_probe_flags} -dM -E -x c++ /dev/null
                RESULT_VARIABLE mhgp12_probe_rc OUTPUT_VARIABLE mhgp12_probe_macros ERROR_VARIABLE mhgp12_probe_err)
if(NOT mhgp12_probe_rc STREQUAL "0")
  message(FATAL_ERROR "sonde des macros du compilateur illisible (code ${mhgp12_probe_rc}) : ${mhgp12_probe_err}")
endif()
if(mhgp12_probe_macros MATCHES "#define __FAST_MATH__")
  message(FATAL_ERROR "mhgp12_fast_math_interdit : les options de compilation definissent __FAST_MATH__ "
                      "(-ffast-math, -Ofast) ; ARCHITECTURE.md de la v11, paragraphe 4, F5")
endif()

if(MHGP12_MARCH)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=${MHGP12_MARCH}>)
endif()
if(MHGP12_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()
if(MHGP12_TSAN)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
# Clang avec sanitizer : runtime partage (-shared-libsan), ce que GCC fait par defaut. Reparation du raccord R2 de la
# v10 (constat B1) : avec le runtime statique de Clang, un executable de test qui remplace operator new ne se lie pas
# (definitions multiples) et une bibliotheque de prechargement passe apres le runtime ; les harnais d'injection de
# fautes etaient alors inoperants. Runtime partage absent : refus explicite a la configuration, jamais un harnais muet.
if((MHGP12_SANITIZE OR MHGP12_TSAN) AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-runtime-dir RESULT_VARIABLE mhgp12_rt_rc
                  OUTPUT_VARIABLE mhgp12_rt_dir ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(mhgp12_rt_sanitizer thread)
  if(MHGP12_SANITIZE)
    set(mhgp12_rt_sanitizer address,undefined)
  endif()
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_FLAGS -fsanitize=${mhgp12_rt_sanitizer})
  set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=${mhgp12_rt_sanitizer} -shared-libsan)
  check_cxx_source_compiles("int main() { return 0; }" MHGP12_CLANG_SHARED_SANITIZER_RUNTIME)
  unset(CMAKE_REQUIRED_FLAGS)
  unset(CMAKE_REQUIRED_LINK_OPTIONS)
  if(NOT mhgp12_rt_rc STREQUAL "0" OR NOT IS_DIRECTORY "${mhgp12_rt_dir}"
     OR NOT MHGP12_CLANG_SHARED_SANITIZER_RUNTIME)
    message(FATAL_ERROR "mhgp12_runtime_partage_absent : Clang avec sanitizer exige le runtime partage "
                        "(-shared-libsan, dossier [${mhgp12_rt_dir}]) ; l'installer, ou construire les variantes a "
                        "sanitizer avec GCC")
  endif()
  add_link_options(-shared-libsan "LINKER:-rpath,${mhgp12_rt_dir}")
endif()

find_package(Threads REQUIRED)

# ---- Unites et modules -----------------------------------------------------------------------------------------
include(cmake/modules.cmake)

set(mhgp12_present_modules)
foreach(module IN LISTS MHGP12_ALL_MODULES)
  if(EXISTS ${PROJECT_SOURCE_DIR}/src/${module}/module.cmake)
    list(APPEND mhgp12_present_modules ${module})
  endif()
endforeach()
set(mhgp12_has_cli FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/cli/cli.cmake)
  set(mhgp12_has_cli TRUE)
endif()
set(mhgp12_has_reference FALSE)
if(EXISTS ${PROJECT_SOURCE_DIR}/reference/tests.cmake)
  set(mhgp12_has_reference TRUE)
endif()

if(MHGP12_MODULES)
  set(MHGP12_UNITS ${MHGP12_MODULES})
  list(REMOVE_DUPLICATES MHGP12_UNITS)
else()
  set(MHGP12_UNITS ${mhgp12_present_modules})
  if(mhgp12_has_cli)
    list(APPEND MHGP12_UNITS cli)
  endif()
  if(mhgp12_has_reference)
    list(APPEND MHGP12_UNITS reference)
  endif()
endif()

set(mhgp12_requested_modules)
foreach(unit IN LISTS MHGP12_UNITS)
  if(unit STREQUAL "cli")
    if(NOT mhgp12_has_cli)
      message(FATAL_ERROR "mhgp12 : unite cli demandee, cli/cli.cmake absent")
    endif()
    list(APPEND mhgp12_requested_modules ${mhgp12_present_modules})
  elseif(unit STREQUAL "reference")
    if(NOT mhgp12_has_reference)
      message(FATAL_ERROR "mhgp12 : unite reference demandee, reference/tests.cmake absent")
    endif()
  elseif(unit IN_LIST MHGP12_ALL_MODULES)
    list(APPEND mhgp12_requested_modules ${unit})
  else()
    message(FATAL_ERROR "mhgp12 : unite inconnue '${unit}' dans MHGP12_MODULES "
                        "(admis : ${MHGP12_ALL_MODULES} cli reference)")
  endif()
endforeach()
if(NOT mhgp12_requested_modules AND NOT MHGP12_UNITS)
  message(FATAL_ERROR "mhgp12 : aucun module present (src/<module>/module.cmake)")
endif()
mhgp12_module_closure(MHGP12_LIBRARY_MODULES ${mhgp12_requested_modules})
foreach(module IN LISTS MHGP12_LIBRARY_MODULES)
  if(NOT module IN_LIST mhgp12_present_modules)
    message(FATAL_ERROR "mhgp12 : le module '${module}' est requis (unites : ${MHGP12_UNITS}) mais "
                        "src/${module}/module.cmake est absent ; limiter la construction par -DMHGP12_MODULES=...")
  endif()
endforeach()
message(STATUS "mhgp12 : unites testees = ${MHGP12_UNITS} ; modules de la bibliotheque = ${MHGP12_LIBRARY_MODULES} ; "
               "bits = ${MHGP12_COORD_BITS}")

# ---- Bibliotheque ----------------------------------------------------------------------------------------------
# mhgp12_module_sources(<fichier>...) : sources du module en cours (chemins relatifs a son dossier).
function(mhgp12_module_sources)
  foreach(source IN LISTS ARGN)
    if(NOT EXISTS ${MHGP12_MODULE_DIR}/${source})
      message(FATAL_ERROR "module ${MHGP12_MODULE} : source absente ${MHGP12_MODULE_DIR}/${source}")
    endif()
    target_sources(mhgp12 PRIVATE ${MHGP12_MODULE_DIR}/${source})
  endforeach()
endfunction()

if(MHGP12_LIBRARY_MODULES)
  add_library(mhgp12 STATIC)
  target_include_directories(mhgp12 PUBLIC ${PROJECT_SOURCE_DIR}/src)
  target_compile_definitions(mhgp12 PUBLIC MHGP12_COORD_BITS=${MHGP12_COORD_BITS})
  target_link_libraries(mhgp12 PUBLIC Threads::Threads)
  if(MHGP12_POISON)
    # PRIVATE : l'empoisonnement vit dans src/core/buffer.cpp seulement ; aucune unite cliente n'en depend.
    target_compile_definitions(mhgp12 PRIVATE MHGP12_POISON)
  endif()
  foreach(module IN LISTS MHGP12_LIBRARY_MODULES)
    set(MHGP12_MODULE ${module})
    set(MHGP12_MODULE_DIR ${PROJECT_SOURCE_DIR}/src/${module})
    if(NOT EXISTS ${MHGP12_MODULE_DIR}/${module}.hpp)
      message(FATAL_ERROR "module ${module} : en-tete public absent (src/${module}/${module}.hpp)")
    endif()
    include(${MHGP12_MODULE_DIR}/module.cmake)
  endforeach()
  unset(MHGP12_MODULE)
  unset(MHGP12_MODULE_DIR)
endif()

if("cli" IN_LIST MHGP12_UNITS)
  include(cli/cli.cmake)
endif()

# ---- Portes ----------------------------------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
  include(cmake/gates.cmake)

  # Portes du socle lui-meme (cadre de test, run_expect.cmake, outils) : jouees avec celles de core.
  if("core" IN_LIST MHGP12_UNITS)
    include(tests/support/tests.cmake)
  endif()
  foreach(unit IN LISTS MHGP12_UNITS)
    if(unit STREQUAL "reference")
      include(reference/tests.cmake)
    elseif(EXISTS ${PROJECT_SOURCE_DIR}/tests/${unit}/tests.cmake)
      include(tests/${unit}/tests.cmake)
    endif()
  endforeach()

  # Regles de proprete lisibles dans le texte (ARCHITECTURE.md de la v11, paragraphe 1), sur les unites construites.
  # Construction complete (MHGP12_MODULES vide) : tout l'arbre est juge.
  set(mhgp12_style_units)
  if(MHGP12_MODULES)
    set(mhgp12_style_units --units ${MHGP12_UNITS})
  endif()
  mhgp12_python_gate(mhgp12_style 0 ${PROJECT_SOURCE_DIR}/tools/check_style.py --root ${PROJECT_SOURCE_DIR}
                     ${mhgp12_style_units} LABELS fast)

  # Mutants : une porte par manifeste tests/mutants/<unite>.json present. La copie mutee est construite avec le
  # compilateur et le profil de ce build, sans sanitizer : sous MHGP12_SANITIZE ou MHGP12_TSAN les portes de mutants
  # ne sont pas enregistrees (une campagne par jeu de builds suffit). Le compte rendu de la campagne (empreintes du
  # manifeste et des sources, verdict et cause de chaque mutant) est ecrit dans <build>/mutants_<unite>.json.
  set(mhgp12_mutant_profile -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DMHGP12_COORD_BITS=${MHGP12_COORD_BITS} -DMHGP12_POISON=${MHGP12_POISON} -DMHGP12_MARCH=${MHGP12_MARCH})
  foreach(unit IN LISTS MHGP12_UNITS)
    set(manifest ${PROJECT_SOURCE_DIR}/tests/mutants/${unit}.json)
    if(EXISTS ${manifest})
      # Manifeste a jour (schema, motifs presents une seule fois) : porte rapide, sans construction.
      mhgp12_python_gate(mhgp12_mutants_${unit}_manifest 0 ${PROJECT_SOURCE_DIR}/tests/mutants/run_mutants.py
                         --manifest ${manifest} --source ${PROJECT_SOURCE_DIR} --check LABELS mutant fast)
      if(NOT MHGP12_SANITIZE AND NOT MHGP12_TSAN)
        set(mutant_args --manifest ${manifest} --source ${PROJECT_SOURCE_DIR}
            --work ${PROJECT_BINARY_DIR}/mutants/${unit} --report ${PROJECT_BINARY_DIR}/mutants_${unit}.json
            --jobs ${MHGP12_MUTANT_JOBS}
            --cmake ${CMAKE_COMMAND} --ctest ${CMAKE_CTEST_COMMAND} --generator ${CMAKE_GENERATOR})
        foreach(definition IN LISTS mhgp12_mutant_profile)
          list(APPEND mutant_args --cmake-arg=${definition})
        endforeach()
        mhgp12_python_gate(mhgp12_mutants_${unit} 0 ${PROJECT_SOURCE_DIR}/tests/mutants/run_mutants.py
                           ${mutant_args} LABELS mutant long TIMEOUT 7200)
      endif()
    endif()
  endforeach()

  mhgp12_check_gate_registry()
endif()
