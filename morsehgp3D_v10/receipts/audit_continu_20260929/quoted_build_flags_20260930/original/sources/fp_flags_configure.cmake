# Porte : la configuration refuse -Ofast, -ffast-math et ses composantes qui reassocient ou remplacent une division
# (-funsafe-math-optimizations, -fassociative-math, -freciprocal-math) dans chaque variable CMAKE_CXX_FLAGS*, et impose
# -ffp-contract=off a toutes les unites (raccord R2 du 30 septembre 2026 ; verification adverse de la lentille
# raccord_r2, § 3.3 : -DCMAKE_CXX_FLAGS=-Ofast et -DCMAKE_CXX_FLAGS_RELEASE=-ffast-math configuraient avec le code 0).
#
# Chaque cas configure le projet dans un dossier neuf (WORK/<cas>, efface ensuite).
#  Refus : code non nul, jeton grave mhgp10_fp_flags_interdits et nom de la variable fautive sur la sortie d'erreur ;
#          CMAKE_CXX_FLAGS (-Ofast), CMAKE_CXX_FLAGS_RELEASE, CMAKE_CXX_FLAGS_DEBUG sous le type Debug,
#          CMAKE_CXX_FLAGS_RELWITHDEBINFO et CMAKE_CXX_FLAGS_MINSIZEREL sous le type Release, type personnalise actif
#          (CMAKE_CXX_FLAGS_PROFIL sous le type Profil) ou inactif (sous le type Release), variable d'environnement
#          CXXFLAGS.
#  Temoins : configuration par defaut et CMAKE_CXX_FLAGS=-fno-fast-math (qui annule) : code 0 ; dans le premier,
#          chaque commande de compile_commands.json porte -ffp-contract=off (contraction coupee sans MHGP10_MARCH).
# Ligne finale sur la sortie standard : fp_flags_configure_ok refus=8 temoins=2. Toute autre issue : FATAL_ERROR
# (code 1).
#
#   cmake -DSRC_DIR=<morsehgp3D_v10> -DWORK=<dossier> -DCXX=<compilateur> -P fp_flags_configure.cmake
cmake_minimum_required(VERSION 3.22)
foreach(v SRC_DIR WORK CXX)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "fp_flags_configure : variable ${v} absente")
  endif()
endforeach()
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")

# "nom|variable fautive attendue|environnement|arguments" ; « - » : champ vide ; arguments separes par des virgules.
set(refusals
    "cxx_flags_ofast|CMAKE_CXX_FLAGS|-|-DCMAKE_CXX_FLAGS=-Ofast"
    "release_fast_math|CMAKE_CXX_FLAGS_RELEASE|-|-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -ffast-math"
    "debug_ofast|CMAKE_CXX_FLAGS_DEBUG|-|-DCMAKE_BUILD_TYPE=Debug,-DCMAKE_CXX_FLAGS_DEBUG=-g -Ofast"
    "relwithdebinfo|CMAKE_CXX_FLAGS_RELWITHDEBINFO|-|-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-funsafe-math-optimizations"
    "minsizerel_associative|CMAKE_CXX_FLAGS_MINSIZEREL|-|-DCMAKE_CXX_FLAGS_MINSIZEREL=-Os -fassociative-math"
    "type_personnalise|CMAKE_CXX_FLAGS_PROFIL|-|-DCMAKE_BUILD_TYPE=Profil,-DCMAKE_CXX_FLAGS_PROFIL=-freciprocal-math"
    "variante_inactive|CMAKE_CXX_FLAGS_PROFIL|-|-DCMAKE_BUILD_TYPE=Release,-DCMAKE_CXX_FLAGS_PROFIL=-O2 -Ofast"
    "env_cxxflags|CMAKE_CXX_FLAGS|CXXFLAGS=-O2 -ffast-math|-")
set(refused 0)
foreach(case IN LISTS refusals)
  string(REPLACE "|" ";" parts "${case}")
  list(GET parts 0 name)
  list(GET parts 1 var)
  list(GET parts 2 envset)
  list(GET parts 3 cfg)
  set(args)
  if(NOT "${cfg}" STREQUAL "-")
    string(REPLACE "," ";" args "${cfg}")
  endif()
  set(launcher "${CMAKE_COMMAND}" -E env)
  if(NOT "${envset}" STREQUAL "-")
    list(APPEND launcher "${envset}")
  endif()
  execute_process(COMMAND ${launcher} "${CMAKE_COMMAND}" -S "${SRC_DIR}" -B "${WORK}/${name}"
                          "-DCMAKE_CXX_COMPILER=${CXX}" ${args}
                  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if("${rc}" STREQUAL "0")
    message(FATAL_ERROR "cas ${name} : configuration acceptee (code 0), refus attendu")
  endif()
  string(REGEX REPLACE "[ \t\r\n]+" " " err_flat "${err}")  # CMake replie les longs messages
  string(FIND "${err_flat}" "mhgp10_fp_flags_interdits" pos_tok)
  string(FIND "${err_flat}" "${var} contient " pos_var)
  if(pos_tok EQUAL -1 OR pos_var EQUAL -1)
    message(FATAL_ERROR "cas ${name} : refus sans le jeton grave ou sans la variable ${var} :\n${err}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "refus ${name} variable=${var} code=${rc}")
  file(REMOVE_RECURSE "${WORK}/${name}")
  math(EXPR refused "${refused} + 1")
endforeach()

set(witnesses 0)
foreach(name defaut no_fast_math)
  set(args)
  if(name STREQUAL "no_fast_math")
    set(args "-DCMAKE_CXX_FLAGS=-fno-fast-math")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SRC_DIR}" -B "${WORK}/${name}" "-DCMAKE_CXX_COMPILER=${CXX}" ${args}
                  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT "${rc}" STREQUAL "0")
    message(FATAL_ERROR "temoin ${name} : configuration refusee (code ${rc}) :\n${err}")
  endif()
  if(name STREQUAL "defaut")
    file(READ "${WORK}/${name}/compile_commands.json" commands)
    string(REGEX MATCHALL "\"command\": \"[^\"]*\"" entries "${commands}")
    list(LENGTH entries n_entries)
    if(n_entries EQUAL 0)
      message(FATAL_ERROR "temoin ${name} : compile_commands.json sans commande")
    endif()
    set(n_contract 0)
    foreach(e IN LISTS entries)
      if(e MATCHES " -ffp-contract=off")
        math(EXPR n_contract "${n_contract} + 1")
      endif()
    endforeach()
    if(NOT n_contract EQUAL n_entries)
      message(FATAL_ERROR "temoin ${name} : ${n_contract} commande(s) sur ${n_entries} avec -ffp-contract=off")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo
                            "temoin ${name} code=0 commandes=${n_entries} contraction_coupee=${n_contract}")
  else()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "temoin ${name} code=0")
  endif()
  file(REMOVE_RECURSE "${WORK}/${name}")
  math(EXPR witnesses "${witnesses} + 1")
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "fp_flags_configure_ok refus=${refused} temoins=${witnesses}")
