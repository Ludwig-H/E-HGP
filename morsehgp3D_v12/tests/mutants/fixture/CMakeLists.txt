# Projet factice (hors produit, sans compilateur) du lanceur de mutants : tests/mutants/test_run_mutants.py y joue le
# lanceur avec les vrais cmake et ctest, pour les issues qu'une doublure ne prouve pas (porte temoin sautee, erreur de
# chargement de CTest, programme impossible a lancer, interprete d'un script introuvable). Le "produit" est
# src/value.txt ; la porte le compare a src/expected.txt par les aides de la v12 (code exact, verdict de
# run_expect.cmake).
#
#   cmake -S tests/mutants/fixture -B <dossier> -DMHGP12_ROOT=<racine v12>
cmake_minimum_required(VERSION 3.20)
project(mhgp12_mutant_fixture NONE)
include(CTest)
include(${MHGP12_ROOT}/cmake/gates.cmake)
set(judge ${CMAKE_COMMAND})
# point d'insertion des mutants de chargement
mhgp12_expect_code(mhgp12_fixture_value 0 ${judge} -E compare_files ${PROJECT_SOURCE_DIR}/src/value.txt
                   ${PROJECT_SOURCE_DIR}/src/expected.txt LABELS unit fast)
# la meme porte sous le label lidar : sautee quand MHGP12_DATA_DIR ne nomme pas un dossier
mhgp12_expect_code(mhgp12_fixture_lidar_value 0 ${judge} -E compare_files ${PROJECT_SOURCE_DIR}/src/value.txt
                   ${PROJECT_SOURCE_DIR}/src/expected.txt LABELS lidar)
# un script juge, rendu executable dans le dossier de construction (aucun droit d'execution n'est demande au depot)
file(READ ${PROJECT_SOURCE_DIR}/src/judge.sh.in judge_script)
file(WRITE ${PROJECT_BINARY_DIR}/judge.sh "${judge_script}")
file(CHMOD ${PROJECT_BINARY_DIR}/judge.sh PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
mhgp12_expect_code(mhgp12_fixture_script 0 ${PROJECT_BINARY_DIR}/judge.sh LABELS unit fast)
mhgp12_check_gate_registry()
