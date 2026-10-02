# Portes du socle lui-meme : cmake/run_expect.cmake, cmake/expect_refusal.cmake, le cadre de test C++ (test.hpp),
# l'aide Python (mhgp11_gate.py), le controle de style et le lanceur de mutants. Jouees avec les portes de core.
#
# Pour juger qu'une porte ECHOUE quand il le faut, le script juge est lance a l'interieur d'une autre porte : le script
# interieur doit finir par le code 1 (FATAL_ERROR) et ecrire son verdict exact.
add_executable(mhgp11_support_exit_probe ${CMAKE_CURRENT_LIST_DIR}/exit_probe.cpp)
add_executable(mhgp11_support_framework_probe ${CMAKE_CURRENT_LIST_DIR}/framework_probe.cpp)
target_include_directories(mhgp11_support_framework_probe PRIVATE ${CMAKE_CURRENT_LIST_DIR})

set(mhgp11_probe $<TARGET_FILE:mhgp11_support_exit_probe>)
set(mhgp11_run_expect ${PROJECT_SOURCE_DIR}/cmake/run_expect.cmake)
set(mhgp11_expect_refusal ${PROJECT_SOURCE_DIR}/cmake/expect_refusal.cmake)

# ---- run_expect.cmake : code exact, ligne exacte, signal, donnees absentes -------------------------------------
mhgp11_expect_code(mhgp11_support_code_exact 3 mhgp11_support_exit_probe code 3 LABELS unit fast)
mhgp11_expect_code(mhgp11_support_line_present 0 mhgp11_support_exit_probe say "controle ok" code 0
                   LINE "controle ok" LABELS unit fast)
# arguments conserves tels quels : espaces, argument vide, point-virgule
mhgp11_expect_code(mhgp11_support_arguments 0 mhgp11_support_exit_probe say "a b" say "" say "x;y" code 0
                   LINE "x;y" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_code_mismatch 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=2 -DARG0=code -DARG1=3 -DEXPECTED=0 -P ${mhgp11_run_expect}
                   LINE "run_expect_verdict code" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_signal_refused 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=1 -DARG0=kill -DEXPECTED=0 -P ${mhgp11_run_expect}
                   LINE "run_expect_verdict arret_anormal" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_program_absent 1 ${CMAKE_COMMAND}
                   -DCMD=${PROJECT_BINARY_DIR}/mhgp11_programme_absent -DNARGS=0 -DEXPECTED=0 -P ${mhgp11_run_expect}
                   LINE "run_expect_verdict arret_anormal" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_line_absent 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=4 -DARG0=say -DARG1=abd -DARG2=code -DARG3=0 -DEXPECTED=0
                   -DEXPECT_LINE=abc -P ${mhgp11_run_expect}
                   LINE "run_expect_verdict ligne_absente" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_expected_out_of_range 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=2 -DARG0=code -DARG1=5 -DEXPECTED=5 -P ${mhgp11_run_expect}
                   LABELS unit fast)
# dossier de donnees absent : jeton de porte sautee, la commande n'est pas lancee (elle rendrait 3, attendu 0)
mhgp11_expect_code(mhgp11_support_skip_token 0 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=2 -DARG0=code -DARG1=3 -DEXPECTED=0
                   -DREQUIRE_DIR_ENV=MHGP11_SUPPORT_DOSSIER_ABSENT -P ${mhgp11_run_expect}
                   LINE "mhgp11_porte_sautee MHGP11_SUPPORT_DOSSIER_ABSENT" LABELS unit fast)
# dossier de donnees present : la commande est jouee et jugee
mhgp11_expect_code(mhgp11_support_data_present 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=2 -DARG0=code -DARG1=3 -DEXPECTED=0
                   -DREQUIRE_DIR_ENV=MHGP11_SUPPORT_DOSSIER -P ${mhgp11_run_expect}
                   LINE "run_expect_verdict code" LABELS unit fast ENV MHGP11_SUPPORT_DOSSIER=${PROJECT_SOURCE_DIR})

# Porte de label lidar : sautee (Skipped) si la variable MHGP11_DATA_DIR ne nomme pas un dossier, jouee sinon. Elle
# sert de sentinelle : si elle est sautee, toutes les portes lidar le sont, faute de donnees.
mhgp11_expect_code(mhgp11_support_lidar_sentinel 0 mhgp11_support_exit_probe say "donnees lidar visibles" code 0
                   LINE "donnees lidar visibles" LABELS lidar fast)

# ---- expect_refusal.cmake : temoin, refus, jeton ----------------------------------------------------------------
mhgp11_expect_refusal(mhgp11_support_refusal_ok TOKEN jeton_grave
                      COMMAND ${mhgp11_probe} say jeton_grave code 0 REFUSAL 2 LABELS unit fast)
mhgp11_expect_code(mhgp11_support_refusal_control_red 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=4 -DARG0=say -DARG1=jeton_grave -DARG2=code -DARG3=1
                   -DNREFUSAL=1 -DREFUSAL0=2 -DTOKEN=jeton_grave -P ${mhgp11_expect_refusal}
                   LINE "expect_refusal_verdict temoin_en_echec" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_refusal_absent 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=4 -DARG0=say -DARG1=jeton_grave -DARG2=code -DARG3=0
                   -DNREFUSAL=1 -DREFUSAL0=0 -DTOKEN=jeton_grave -P ${mhgp11_expect_refusal}
                   LINE "expect_refusal_verdict refus_absent" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_refusal_token_absent 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=4 -DARG0=say -DARG1=autre_texte -DARG2=code -DARG3=0
                   -DNREFUSAL=1 -DREFUSAL0=2 -DTOKEN=jeton_grave -P ${mhgp11_expect_refusal}
                   LINE "expect_refusal_verdict jeton_absent" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_refusal_signal 1 ${CMAKE_COMMAND}
                   -DCMD=${mhgp11_probe} -DNARGS=2 -DARG0=say -DARG1=jeton_grave
                   -DNREFUSAL=1 -DREFUSAL0=kill -DTOKEN=jeton_grave -P ${mhgp11_expect_refusal}
                   LABELS unit fast)

# ---- cadre de test C++ : codes 0, 1, 2, 3 -----------------------------------------------------------------------
mhgp11_expect_code(mhgp11_support_framework_pass 0 mhgp11_support_framework_probe pass sign texts
                   LINE "mhgp11_test_ok tests=3 controles=16" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_fail 1 mhgp11_support_framework_probe fail
                   LINE "ECHECS 1" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_require 1 mhgp11_support_framework_probe require_stops
                   LINE "test require_stops controles=1 echecs=1 plancher=1" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_floor 3 mhgp11_support_framework_probe below_floor
                   LINE "PLANCHER non atteint : below_floor" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_throw 1 mhgp11_support_framework_probe throws
                   LINE "ECHEC throws : exception sortie du test : exception de la sonde" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_unknown 2 mhgp11_support_framework_probe absent LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_repeated 2 mhgp11_support_framework_probe pass pass LABELS unit fast)
# sans argument, tous les tests sont joues : les echecs voulus de la sonde donnent le code 1
mhgp11_expect_code(mhgp11_support_framework_all 1 mhgp11_support_framework_probe LINE "ECHECS 3" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_inventory 0 mhgp11_support_framework_probe --inventaire
                   below_floor fail pass require_stops sign texts throws
                   LINE "inventaire_ok tests=7" LABELS unit fast)
mhgp11_expect_code(mhgp11_support_framework_inventory_missing 3 mhgp11_support_framework_probe --inventaire
                   below_floor fail pass require_stops sign texts
                   LINE "INVENTAIRE test sans porte : throws" LABELS unit fast)

# ---- aide Python, controle de style, lanceur de mutants ---------------------------------------------------------
mhgp11_python_gate(mhgp11_support_gate_helper 0 test_gate_helper.py
                   LINE "gate_helper_ok controles=21" LABELS unit fast)
mhgp11_python_gate(mhgp11_support_check_style 0 test_check_style.py ${PROJECT_SOURCE_DIR}/tools/check_style.py
                   LABELS unit fast)
mhgp11_python_gate(mhgp11_support_run_mutants 0 ${PROJECT_SOURCE_DIR}/tests/mutants/test_run_mutants.py
                   ${PROJECT_SOURCE_DIR}/tests/mutants/run_mutants.py LABELS unit fast)

# ---- aides de cmake/gates.cmake : proprietes des portes creees, refus a la configuration ------------------------
# Projet factice sans compilateur (tests/support/gate_fixture) : configure en une fraction de seconde.
mhgp11_python_gate(mhgp11_support_gate_properties 0 test_gate_properties.py ${CMAKE_COMMAND} ${CMAKE_CTEST_COMMAND}
                   ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/gates/properties
                   LINE "gate_properties_ok controles=49" LABELS unit fast)
foreach(fault label no_label fast_long name code duplicate program timeout script direct)
  set(token_label mhgp11_porte_label_inconnu)
  set(token_no_label mhgp11_porte_sans_label)
  set(token_fast_long mhgp11_porte_fast_et_long)
  set(token_name mhgp11_porte_nom_invalide)
  set(token_code mhgp11_porte_code_invalide)
  set(token_duplicate mhgp11_porte_nom_en_double)
  set(token_program mhgp11_porte_programme_invalide)
  set(token_timeout mhgp11_porte_delai_invalide)
  set(token_script mhgp11_porte_script_absent)
  set(token_direct mhgp11_porte_hors_aides)
  mhgp11_expect_refusal(mhgp11_support_gates_refuse_${fault} TOKEN ${token_${fault}}
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_LIST_DIR}/gate_fixture -B ${PROJECT_BINARY_DIR}/gates/fault_${fault}
            -DMHGP11_ROOT=${PROJECT_SOURCE_DIR}
    REFUSAL -DMHGP11_FIXTURE_FAULT=${fault}
    SCRATCH ${PROJECT_BINARY_DIR}/gates/fault_${fault} LABELS unit fast)
endforeach()

# ---- garde simple contre -ffast-math a la configuration (CMakeLists.txt) ----------------------------------------
# Temoin : ce projet se configure dans un dossier de travail. Refus : -ffast-math dans CMAKE_CXX_FLAGS, puis -Ofast
# dans la variable du type de build (cas que la garde par jetons de la v10 publiee laissait passer : audit L04).
# La sonde interroge le compilateur sur les options effectives : -DCMAKE_CXX_FLAGS=-Ofast avec le type Release donne
# "-Ofast -O3", ou -O3 l'emporte et __FAST_MATH__ n'est pas defini ; cette configuration est admise a raison.
foreach(case cxx_flags release_ofast)
  set(refusal -DCMAKE_CXX_FLAGS=-ffast-math)
  if(case STREQUAL "release_ofast")
    set(refusal "-DCMAKE_CXX_FLAGS_RELEASE=-Ofast -DNDEBUG")
  endif()
  mhgp11_expect_refusal(mhgp11_support_configure_fast_math_${case} TOKEN mhgp11_fast_math_interdit
    COMMAND ${CMAKE_COMMAND} -S ${PROJECT_SOURCE_DIR} -B ${PROJECT_BINARY_DIR}/gates/configure_${case}
            -G ${CMAKE_GENERATOR} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_BUILD_TYPE=Release
            -DMHGP11_MODULES=core -DBUILD_TESTING=OFF
    REFUSAL ${refusal}
    SCRATCH ${PROJECT_BINARY_DIR}/gates/configure_${case} LABELS unit fast)
endforeach()
