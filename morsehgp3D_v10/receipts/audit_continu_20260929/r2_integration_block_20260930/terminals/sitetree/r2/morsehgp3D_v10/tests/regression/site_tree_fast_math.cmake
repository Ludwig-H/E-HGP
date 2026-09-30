# Porte : site_tree.cpp refuse -ffast-math a la compilation (doctrine v4 : le filtre flottant certifie de SiteTree
# exige IEEE-754 strict ; second tour de correction du 30 septembre 2026).
#
# 1. Controle positif : la meme compilation sans -ffast-math doit reussir (code 0), sinon un refus pour une autre
#    cause (inclusion absente, drapeau inconnu) rendrait la porte verte par vacuite.
# 2. Sous -ffast-math : code 1 exact et message grave du #error sur la sortie d'erreur.
# Ligne finale sur la sortie standard : site_tree_fast_math_refuse. Toute autre issue : FATAL_ERROR (code 1).
#
#   cmake -DCXX=<compilateur> -DSRC=<src/cloud/site_tree.cpp> -DINC=<src> -P site_tree_fast_math.cmake
foreach(v CXX SRC INC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "site_tree_fast_math : variable ${v} absente")
  endif()
endforeach()
set(base_args -std=c++20 -fsyntax-only "-I${INC}" "${SRC}")
execute_process(COMMAND "${CXX}" ${base_args} RESULT_VARIABLE rc_ctrl OUTPUT_VARIABLE out_ctrl ERROR_VARIABLE err_ctrl)
if(NOT "${rc_ctrl}" STREQUAL "0")
  message(FATAL_ERROR "controle positif : compilation sans -ffast-math, code ${rc_ctrl}, attendu 0 :\n${err_ctrl}")
endif()
execute_process(COMMAND "${CXX}" -ffast-math ${base_args} RESULT_VARIABLE rc_fm OUTPUT_VARIABLE out_fm
                ERROR_VARIABLE err_fm)
if(NOT "${rc_fm}" STREQUAL "1")
  message(FATAL_ERROR "compilation sous -ffast-math : code ${rc_fm}, attendu 1 (refus par #error)")
endif()
string(FIND "${err_fm}" "mhgp10_site_tree_fast_math_interdit" pos)
if(pos EQUAL -1)
  message(FATAL_ERROR "compilation sous -ffast-math refusee sans le message grave :\n${err_fm}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E echo site_tree_fast_math_refuse)
