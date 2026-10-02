# Aides des portes CTest de la v11 (docs/ARCHITECTURE.md, paragraphe 5). Port de morsehgp3D_v10/cmake/gates.cmake du
# raccord R2 (commit 865f5e6, fonction mhgp10_gate), etendu a ce que le CMakeLists.txt de la v10 ecrivait a la main
# pour chaque porte (portes Python sous run_expect.cmake, jumelle sous python3 -O, PYTHONDONTWRITEBYTECODE, refus a
# la compilation).
#
# Toute porte passe par cmake/run_expect.cmake : code de sortie EXACT, arret par signal toujours en echec.
# Codes : 0 conforme, 1 desaccord d'un juge, 2 refus avant calcul, 3 plancher ou invariant viole, 4 mutant tue.
#
#   mhgp11_add_unit(<cible> SOURCES <fichier>... [GROUPS <test>...] [LABELS <label>...] [TIMEOUT <s>] [LIBS <lib>...])
#       Construit l'executable de test <cible> (lie a mhgp11, en-tetes de tests/support) et enregistre ses portes,
#       code 0, label unit plus LABELS. Sans GROUPS : une porte <cible> qui joue tous les tests. Avec GROUPS : une
#       porte <cible>_<test> par test nomme, et une porte <cible>_inventaire qui echoue (code 3) si la liste GROUPS
#       n'est pas exactement celle des tests de l'executable (aucun test oublie par les portes).
#   mhgp11_expect_code(<nom> <code> <cible> [<argument>...] LABELS <label>... [LINE <ligne>] [TIMEOUT <s>]
#                      [ENV <VAR=valeur>...])
#       Porte <nom> : execute <cible> (cible executable du projet, ou chemin absolu d'un programme) et exige le code
#       exact <code> ; LINE exige en plus cette ligne exacte sur la sortie standard de la meme execution.
#   mhgp11_python_gate(<nom> <code> <script> [<argument>...] LABELS <label>... [LINE <ligne>] [TIMEOUT <s>]
#                      [ENV <VAR=valeur>...])
#       Porte Python (interprete du build, Python >= 3.10). Sauf label long, une jumelle <nom>_opt rejoue la porte sous
#       python3 -O (PYTHONOPTIMIZE=1, herite par les interpretes lances) : une porte ne repose jamais sur assert.
#       Les portes long ne sont pas doublees ; la porte mhgp11_style interdit assert dans tout script Python.
#   mhgp11_expect_refusal(<nom> TOKEN <jeton> COMMAND <programme> [<argument>...] REFUSAL <option>...
#                         LABELS <label>... [SCRATCH <dossier>] [TIMEOUT <s>])
#       Porte de refus (cmake/expect_refusal.cmake) : la commande reussit, puis echoue avec le jeton une fois suivie
#       des options REFUSAL.
#   mhgp11_expect_compile_failure(<nom> SOURCE <fichier> TOKEN <jeton> OPTIONS <option>... LABELS <label>...)
#       Refus a la compilation : <fichier> compile (analyse seule, options du projet), puis ne compile plus avec
#       OPTIONS, et le compilateur ecrit le jeton (static_assert, #error). Pour changer une definition du projet :
#       -UMHGP11_COORD_BITS -DMHGP11_COORD_BITS=20.
#
# Chemins relatifs (SOURCES, <script>, SOURCE) : relatifs au fichier tests.cmake appelant.
# Labels admis : unit oracle diff_v10 scale8000 scale16000 scale32000 lidar mutant fast long ; au moins un par porte ;
# fast et long s'excluent. long : porte de plus d'une minute. Delai par defaut : 300 s (3600 s sous long).
# Les portes scale*, lidar et mutant sont jouees seules (RUN_SERIAL) : elles mesurent ou occupent tous les coeurs.
# Une porte lidar exige le dossier nomme par la variable d'environnement MHGP11_DATA_DIR ; s'il manque, elle est
# sautee (Skipped) et non jouee.
set(MHGP11_GATE_LABELS unit oracle diff_v10 scale8000 scale16000 scale32000 lidar mutant fast long)
set(MHGP11_GATE_SERIAL_LABELS scale8000 scale16000 scale32000 lidar mutant)
set(MHGP11_DATA_DIR_ENV MHGP11_DATA_DIR)
set(MHGP11_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

# Chemin absolu d'un fichier donne relativement au fichier appelant.
function(_mhgp11_absolute out path)
  if(IS_ABSOLUTE "${path}")
    set(${out} "${path}" PARENT_SCOPE)
  else()
    set(${out} "${CMAKE_CURRENT_LIST_DIR}/${path}" PARENT_SCOPE)
  endif()
endfunction()

# Ajoute a la liste nommee par `mhgp11_list_var` (dans la portee de l'appelant) les definitions <prefix>0 .. des mots
# de la liste nommee par `mhgp11_words_var`, puis <count_name> ; un point-virgule dans un mot est echappe pour rester
# dans le mot. Les parametres portent un prefixe : un parametre masquerait une variable de meme nom de l'appelant.
function(_mhgp11_word_definitions mhgp11_list_var mhgp11_count_name mhgp11_prefix mhgp11_words_var)
  set(mhgp11_result "${${mhgp11_list_var}}")
  set(mhgp11_i 0)
  foreach(mhgp11_word IN LISTS ${mhgp11_words_var})
    string(REPLACE ";" "\;" mhgp11_word "${mhgp11_word}")
    list(APPEND mhgp11_result "-D${mhgp11_prefix}${mhgp11_i}=${mhgp11_word}")
    math(EXPR mhgp11_i "${mhgp11_i} + 1")
  endforeach()
  list(APPEND mhgp11_result "-D${mhgp11_count_name}=${mhgp11_i}")
  set(${mhgp11_list_var} "${mhgp11_result}" PARENT_SCOPE)
endfunction()

# Enregistre la porte <name> : <program> doit rendre <expected>. Lit dans la portee de l'appelant les listes
# gate_args, gate_labels, gate_env et les valeurs gate_line, gate_timeout (vides si sans objet).
function(_mhgp11_register name expected program)
  if(NOT name MATCHES "^mhgp11_[a-z0-9_]+$")
    message(FATAL_ERROR "mhgp11_porte_nom_invalide : '${name}' ; forme attendue mhgp11_<minuscules, chiffres, _>")
  endif()
  if(NOT expected MATCHES "^[0-4]$")
    message(FATAL_ERROR "mhgp11_porte_code_invalide : porte ${name}, code attendu '${expected}' hors de 0..4")
  endif()
  if(NOT gate_labels)
    message(FATAL_ERROR "mhgp11_porte_sans_label : porte ${name}, au moins un label (LABELS)")
  endif()
  foreach(label IN LISTS gate_labels)
    if(NOT label IN_LIST MHGP11_GATE_LABELS)
      message(FATAL_ERROR "mhgp11_porte_label_inconnu : porte ${name}, label '${label}' "
                          "(admis : ${MHGP11_GATE_LABELS})")
    endif()
  endforeach()
  if("fast" IN_LIST gate_labels AND "long" IN_LIST gate_labels)
    message(FATAL_ERROR "mhgp11_porte_fast_et_long : porte ${name}, les labels fast et long s'excluent")
  endif()
  get_property(known GLOBAL PROPERTY MHGP11_GATES)
  if(name IN_LIST known)
    message(FATAL_ERROR "mhgp11_porte_nom_en_double : porte ${name} deja enregistree")
  endif()

  set(defs "-DCMD=${program}" "-DEXPECTED=${expected}")
  _mhgp11_word_definitions(defs NARGS ARG gate_args)
  if(NOT "${gate_line}" STREQUAL "")
    string(REPLACE ";" "\;" line "${gate_line}")
    list(APPEND defs "-DEXPECT_LINE=${line}")
  endif()
  if("lidar" IN_LIST gate_labels)
    list(APPEND defs "-DREQUIRE_DIR_ENV=${MHGP11_DATA_DIR_ENV}")
  endif()
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} ${defs} -P ${MHGP11_CMAKE_DIR}/run_expect.cmake)

  set(timeout 300)
  if("long" IN_LIST gate_labels)
    set(timeout 3600)
  endif()
  if(NOT "${gate_timeout}" STREQUAL "")
    if(NOT gate_timeout MATCHES "^[1-9][0-9]*$")
      message(FATAL_ERROR "mhgp11_porte_delai_invalide : porte ${name}, TIMEOUT '${gate_timeout}' n'est pas un "
                          "nombre de secondes")
    endif()
    set(timeout ${gate_timeout})
  endif()
  # Aucune porte n'ecrit de bytecode Python dans les sources (raccord R2 de la v10, prealable P4).
  set_property(TEST ${name} PROPERTY ENVIRONMENT PYTHONDONTWRITEBYTECODE=1 ${gate_env})
  set_property(TEST ${name} PROPERTY LABELS ${gate_labels})
  set_property(TEST ${name} PROPERTY TIMEOUT ${timeout})
  foreach(label IN LISTS gate_labels)
    if(label IN_LIST MHGP11_GATE_SERIAL_LABELS)
      set_property(TEST ${name} PROPERTY RUN_SERIAL TRUE)
    endif()
  endforeach()
  if("lidar" IN_LIST gate_labels)
    set_property(TEST ${name} PROPERTY SKIP_REGULAR_EXPRESSION "mhgp11_porte_sautee ${MHGP11_DATA_DIR_ENV}")
  endif()
  set_property(GLOBAL APPEND PROPERTY MHGP11_GATES ${name})
endfunction()

function(mhgp11_expect_code name expected target)
  cmake_parse_arguments(PARSE_ARGV 3 E "" "LINE;TIMEOUT" "LABELS;ENV")
  if(TARGET ${target})
    set(program "$<TARGET_FILE:${target}>")
  elseif(IS_ABSOLUTE "${target}")
    set(program "${target}")
  else()
    message(FATAL_ERROR "mhgp11_porte_programme_invalide : porte ${name}, '${target}' n'est ni une cible du "
                        "projet ni un chemin absolu")
  endif()
  set(gate_args "${E_UNPARSED_ARGUMENTS}")
  set(gate_labels "${E_LABELS}")
  set(gate_env "${E_ENV}")
  set(gate_line "${E_LINE}")
  set(gate_timeout "${E_TIMEOUT}")
  _mhgp11_register(${name} ${expected} "${program}")
endfunction()

function(mhgp11_python_gate name expected script)
  cmake_parse_arguments(PARSE_ARGV 3 P "" "LINE;TIMEOUT" "LABELS;ENV")
  if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "porte ${name} : interprete Python absent (find_package(Python3) dans CMakeLists.txt)")
  endif()
  _mhgp11_absolute(script_path "${script}")
  if(NOT EXISTS "${script_path}")
    message(FATAL_ERROR "mhgp11_porte_script_absent : porte ${name}, ${script_path}")
  endif()
  string(REPLACE ";" "\;" script_word "${script_path}")
  set(gate_args "${script_word}" "${P_UNPARSED_ARGUMENTS}")
  if("${P_UNPARSED_ARGUMENTS}" STREQUAL "")
    set(gate_args "${script_word}")
  endif()
  set(gate_labels "${P_LABELS}")
  set(gate_env "${P_ENV}")
  set(gate_line "${P_LINE}")
  set(gate_timeout "${P_TIMEOUT}")
  _mhgp11_register(${name} ${expected} "${Python3_EXECUTABLE}")
  if(NOT "long" IN_LIST gate_labels)
    list(APPEND gate_env PYTHONOPTIMIZE=1)
    _mhgp11_register(${name}_opt ${expected} "${Python3_EXECUTABLE}")
  endif()
endfunction()

function(mhgp11_expect_refusal name)
  cmake_parse_arguments(PARSE_ARGV 1 R "" "TOKEN;SCRATCH;TIMEOUT" "COMMAND;REFUSAL;LABELS")
  if(R_UNPARSED_ARGUMENTS OR NOT R_TOKEN OR NOT R_COMMAND OR NOT R_REFUSAL)
    message(FATAL_ERROR "mhgp11_expect_refusal ${name} : exige TOKEN, COMMAND et REFUSAL")
  endif()
  set(command_words "${R_COMMAND}")
  list(POP_FRONT command_words program)
  set(inner "-DCMD=${program}")
  _mhgp11_word_definitions(inner NARGS ARG command_words)
  _mhgp11_word_definitions(inner NREFUSAL REFUSAL R_REFUSAL)
  list(APPEND inner "-DTOKEN=${R_TOKEN}")
  if(R_SCRATCH)
    list(APPEND inner "-DSCRATCH=${R_SCRATCH}")
  endif()
  list(APPEND inner -P "${MHGP11_CMAKE_DIR}/expect_refusal.cmake")
  set(gate_args "${inner}")
  set(gate_labels "${R_LABELS}")
  set(gate_env "")
  set(gate_line "expect_refusal_verdict conforme ${R_TOKEN}")
  set(gate_timeout "${R_TIMEOUT}")
  _mhgp11_register(${name} 0 "${CMAKE_COMMAND}")
endfunction()

function(mhgp11_expect_compile_failure name)
  cmake_parse_arguments(PARSE_ARGV 1 C "" "SOURCE;TOKEN" "OPTIONS;LABELS")
  if(C_UNPARSED_ARGUMENTS OR NOT C_SOURCE OR NOT C_TOKEN OR NOT C_OPTIONS)
    message(FATAL_ERROR "mhgp11_expect_compile_failure ${name} : exige SOURCE, TOKEN et OPTIONS")
  endif()
  _mhgp11_absolute(source "${C_SOURCE}")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "mhgp11_expect_compile_failure ${name} : source absente ${source}")
  endif()
  mhgp11_expect_refusal(${name} TOKEN "${C_TOKEN}"
    COMMAND "${CMAKE_CXX_COMPILER}" ${CMAKE_CXX20_STANDARD_COMPILE_OPTION} ${MHGP11_WARNING_OPTIONS} -fsyntax-only
            "-I${PROJECT_SOURCE_DIR}/src" "-I${PROJECT_SOURCE_DIR}/tests/support"
            "-DMHGP11_COORD_BITS=${MHGP11_COORD_BITS}" "${source}"
    REFUSAL ${C_OPTIONS}
    LABELS ${C_LABELS})
endfunction()

function(mhgp11_add_unit target)
  cmake_parse_arguments(PARSE_ARGV 1 U "" "TIMEOUT" "SOURCES;GROUPS;LABELS;LIBS")
  if(U_UNPARSED_ARGUMENTS OR NOT U_SOURCES)
    message(FATAL_ERROR "mhgp11_add_unit ${target} : exige SOURCES (arguments non reconnus : ${U_UNPARSED_ARGUMENTS})")
  endif()
  if(NOT target MATCHES "^mhgp11_[a-z0-9_]+$")
    message(FATAL_ERROR "mhgp11_add_unit : la cible '${target}' doit etre de la forme mhgp11_<minuscules, chiffres, _>")
  endif()
  set(sources)
  foreach(source IN LISTS U_SOURCES)
    _mhgp11_absolute(source_path "${source}")
    list(APPEND sources "${source_path}")
  endforeach()
  add_executable(${target} ${sources})
  target_link_libraries(${target} PRIVATE mhgp11 ${U_LIBS})
  target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)

  set(gate_labels unit ${U_LABELS})
  list(REMOVE_DUPLICATES gate_labels)
  set(gate_env "")
  set(gate_line "")
  set(gate_timeout "${U_TIMEOUT}")
  if(NOT U_GROUPS)
    set(gate_args "")
    _mhgp11_register(${target} 0 "$<TARGET_FILE:${target}>")
    return()
  endif()
  foreach(group IN LISTS U_GROUPS)
    set(gate_args "${group}")
    _mhgp11_register(${target}_${group} 0 "$<TARGET_FILE:${target}>")
  endforeach()
  set(gate_args --inventaire ${U_GROUPS})
  _mhgp11_register(${target}_inventaire 0 "$<TARGET_FILE:${target}>")
endfunction()

# Fin de configuration : tout test du dossier doit avoir ete enregistre par les aides ci-dessus (code exact, labels
# admis). Un add_test direct est refuse.
function(mhgp11_check_gate_registry)
  get_property(gates GLOBAL PROPERTY MHGP11_GATES)
  get_property(tests DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY TESTS)
  foreach(test IN LISTS tests)
    if(NOT test IN_LIST gates)
      message(FATAL_ERROR "mhgp11_porte_hors_aides : test '${test}' enregistre sans les aides de cmake/gates.cmake "
                          "(add_test direct interdit)")
    endif()
  endforeach()
  list(LENGTH gates count)
  message(STATUS "mhgp11 : ${count} portes enregistrees")
endfunction()
