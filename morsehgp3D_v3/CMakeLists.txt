cmake_minimum_required(VERSION 3.22)
project(morsehgp3d_v3 LANGUAGES CXX)

# MorseHGP3D v3 — pour l'instant, uniquement M1 : le juge.
#
# Le sujet juge est `morsehgp3D_v2`, qui fournit une SEMANTIQUE CANDIDATE et des
# fixtures, jamais une autorite. L'oracle n'emprunte a la production aucune
# primitive : son arithmetique, sa geometrie et sa reconstruction de la foret
# sont ecrites ici.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(../morsehgp3D_v2 morsehgp3D_v2_build)

add_executable(mhgp3v_arith_selftest oracle/bigint_selftest.cpp)
target_include_directories(mhgp3v_arith_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oracle)
target_compile_options(mhgp3v_arith_selftest PRIVATE -Wall -Wextra -Wpedantic)

# GMP n'est PAS une dependance de l'oracle : il n'intervient que comme second
# temoin de la validation arithmetique, et seulement s'il est present.
find_path(MHGP3V_GMPXX_INCLUDE gmpxx.h)
find_library(MHGP3V_GMPXX_LIBRARY gmpxx)
find_library(MHGP3V_GMP_LIBRARY gmp)
if(MHGP3V_GMPXX_INCLUDE AND MHGP3V_GMPXX_LIBRARY AND MHGP3V_GMP_LIBRARY)
  target_compile_definitions(mhgp3v_arith_selftest PRIVATE MHGP3V_HAVE_GMP)
  target_include_directories(mhgp3v_arith_selftest PRIVATE ${MHGP3V_GMPXX_INCLUDE})
  target_link_libraries(mhgp3v_arith_selftest
    PRIVATE ${MHGP3V_GMPXX_LIBRARY} ${MHGP3V_GMP_LIBRARY})
  message(STATUS "mhgp3v : temoin GMP present, validation large activee")
else()
  message(WARNING "mhgp3v : GMP absent, la validation arithmetique perd son temoin large")
endif()

add_executable(mhgp3v_oracle oracle/oracle_main.cpp)
target_include_directories(mhgp3v_oracle
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oracle ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_oracle PRIVATE mhgp)
target_compile_options(mhgp3v_oracle PRIVATE -Wall -Wextra -Wpedantic)

# M3' — le juge differentiel du parcours MULTIPLICITAIRE. Il n'appelle ni le
# germe, ni les predicats de pinceau, ni le transport du sujet, mais il PARTAGE
# avec lui `mhgp::sphere_side`, `mhgp::sphere4` et `mhgp::miniball_of` : ce qu'il
# etablit est un accord RELATIF a ces primitives, pas un catalogue critique
# exact. L'autorite independante manquante est une reference rationnelle
# multiplicitaire dans l'oracle M1.
add_executable(mhgp3v_flats_differential prototype/flats_differential.cpp)
target_include_directories(mhgp3v_flats_differential PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_flats_differential PRIVATE mhgp)
target_compile_options(mhgp3v_flats_differential PRIVATE -Wall -Wextra -Werror)

# La DICHOTOMIE des premieres incidences du coeur. Elle ne remplace pas l'oracle
# general : elle mesure les volumes que le tri externe devra porter, et se juge
# contre une verite exhaustive ecrite dans le meme fichier.
add_executable(mhgp3v_first_incidence prototype/first_incidence_dichotomy.cpp)
target_include_directories(mhgp3v_first_incidence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_first_incidence PRIVATE mhgp)
target_compile_options(mhgp3v_first_incidence PRIVATE -Wall -Wextra -Werror)

enable_testing()

function(mhgp3v_add_expected_failure_test name expected)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
           "-DPROGRAM=$<TARGET_FILE:mhgp3v_oracle>"
           "-DEXPECTED=${expected}"
           "-DARGUMENTS=${ARGN}"
           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
endfunction()

# Meme porte, mais pour un autre binaire que l'oracle. La version precedente
# codait la cible en dur, si bien qu'un test negatif ecrit pour un autre juge
# interrogeait silencieusement l'oracle et passait pour la mauvaise raison.
function(mhgp3v_add_expected_failure_test_for program name expected)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
           "-DPROGRAM=$<TARGET_FILE:${program}>"
           "-DEXPECTED=${expected}"
           "-DARGUMENTS=${ARGN}"
           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
endfunction()

add_test(NAME mhgp3v_arith_selftest COMMAND mhgp3v_arith_selftest 20000)
# La porte par defaut tire ses coordonnees sur la GRILLE DECLAREE, et impose des
# planchers : une campagne vide ou censuree ne peut pas rendre OK.
add_test(NAME mhgp3v_oracle COMMAND mhgp3v_oracle
         --clouds 12 --seed 4242 --min-points 8 --max-points 10 --max-order 2
         --min-decided 10 --min-nodes 200)
# Le sujet ancre doit etre juge lui aussi, sur des nuages PLUS GRANDS que la
# fenetre : c'est ce que l'ancienne porte n'exercait jamais.
add_test(NAME mhgp3v_oracle_anchored COMMAND mhgp3v_oracle --subject anchored
         --regime exhaustive --clouds 6 --seed 4242 --min-points 20 --max-points 22
         --max-order 1 --min-decided 5 --min-nodes 30)
# Et sur les arites TROIS et QUATRE, qui dominent la cascade : l'ordre 1 seul
# n'exerce que les singletons et les paires.
add_test(NAME mhgp3v_oracle_anchored_high_arity COMMAND mhgp3v_oracle --subject anchored
         --regime exhaustive --clouds 8 --seed 90210 --min-points 9 --max-points 12
         --max-order 3 --min-decided 6 --min-nodes 60)

# Non-regression PERMANENTE du faux certificat local : sous une fenetre supposee
# de 16 voisins sur 22 points, le generateur doit se declarer incomplet. La
# version fautive annonçait les 22 ancres certifiees et omettait un support.
add_test(NAME mhgp3v_false_certificate_regression COMMAND mhgp3v_oracle --subject anchored
         --regime assumed_window --seed-neighbours 16 --clouds 1 --seed 4242
         --min-points 22 --max-points 22 --max-order 1 --min-decided 1 --min-nodes 1
         --require-incomplete-anchors 22)

# Bornes semantiques du CLI : une valeur syntaxiquement valide mais hors contrat
# doit rendre 2 AVANT toute arithmetique.
mhgp3v_add_expected_failure_test(mhgp3v_cli_order_overflow "campagne absurde"
  --subject anchored --measure-only --regime exhaustive --points 1 --clouds 1
  --max-order 2147483647)

# M2.2 — le rang est calcule PAR LA PROFONDEUR, jamais en comptant la boule.
# Le vert de ce test EST la verification du dictionnaire rang = 4 + c_e + delta_e
# contre la verite exhaustive.
# --max-order 3 seul ne suffisait PAS : l'ordre est tire uniformement, donc la
# plupart des nuages avaient s_max <= 3, et tout support quatre accepte y force
# profondeur nulle. Le vert ne prouvait alors que rang = arite, le cas trivial.
# On impose donc un ordre eleve ET un plancher d'emissions a profondeur
# strictement positive.
add_test(NAME mhgp3v_edge_shallow_depth_dictionary COMMAND mhgp3v_oracle
         --subject edge_shallow --clouds 12 --seed 4242 --min-points 10 --max-points 12
         --min-order 5 --max-order 6 --min-decided 10 --min-nodes 100
         --min-positive-depth 500)

# Campagne NEGATIVE : chaque faute injectee doit etre attrapee. Un garde qui ne
# se declenche jamais ne vaut rien.
foreach(fault member_unsorted rotated_centre sentinel n_children roots)
  add_test(NAME mhgp3v_oracle_inject_${fault} COMMAND mhgp3v_oracle
           --inject ${fault} --clouds 4 --seed 4242 --min-points 8 --max-points 10
           --max-order 2 --min-decided 1 --min-nodes 1)
  set_tests_properties(mhgp3v_oracle_inject_${fault} PROPERTIES
    PASS_REGULAR_EXPRESSION "appliquee exactement une fois.*garde vise declenche exactement une fois")
endforeach()

# Deux vraies sources de rang 2 et de meme niveau exact, mais contributrices a
# deux composantes disjointes : ce test isole le garde de contribution sans que
# le rang, le niveau ou la monotonie puissent le faire rougir a sa place.
add_test(NAME mhgp3v_oracle_inject_merge_source_foreign COMMAND mhgp3v_oracle
         --inject merge_source_foreign --fixture foreign_source_same_level
         --subject v2 --clouds 1 --min-points 4 --max-points 4 --max-order 1
         --coord-max 2 --min-decided 1 --min-nodes 7)
set_tests_properties(mhgp3v_oracle_inject_merge_source_foreign PROPERTIES
  PASS_REGULAR_EXPRESSION "appliquee exactement une fois.*garde vise declenche exactement une fois")

# Trois arêtes contributrices au meme lot : remplacer la source minimale par
# une autre contributrice conserve rang, niveau et incidence, mais viole le
# contrat public de serialisation de la v2.
add_test(NAME mhgp3v_oracle_inject_merge_source_nonminimal COMMAND mhgp3v_oracle
         --inject merge_source_nonminimal_contributor
         --fixture nonminimal_contributor_same_level --subject v2 --clouds 1
         --min-points 4 --max-points 4 --max-order 1 --coord-max 10
         --min-decided 1 --min-nodes 6)
set_tests_properties(mhgp3v_oracle_inject_merge_source_nonminimal PROPERTIES
  PASS_REGULAR_EXPRESSION "appliquee exactement une fois.*garde vise declenche exactement une fois")

# Une campagne ou aucun sujet n'est admissible ne doit jamais devenir verte par
# les seuls echecs de plancher.
#
mhgp3v_add_expected_failure_test(
  mhgp3v_oracle_inject_vacuity_guard
  "campagne SUPPORT doit etre verte avant tout probe hostile"
  --inject member_unsorted --clouds 1 --seed 4242 --min-points 8 --max-points 8
  --max-order 1 --coord-max 1 --min-decided 1 --min-nodes 1)

# Une cocyclicite portee uniquement par des triangles non bien centres ne met
# pas le domaine des sphères critiques hors RelevantGP. L'ancienne mesure de
# census censurait a tort tout le nuage ancre.
add_test(NAME mhgp3v_oracle_noncritical_shell_tie COMMAND mhgp3v_oracle
         --fixture noncritical_shell_tie --subject anchored --regime exhaustive
         --clouds 1 --min-points 4 --max-points 4 --max-order 2 --coord-max 2000
         --min-decided 1 --min-nodes 1)

# REFUTATION PERMANENTE de l'epluchage en couches convexes. Le sommet de
# profondeur 1 y est porte par une DIAGONALE du quadrilatere dual : ses deux
# extremites sont sur la couche 0, que l'epluchage retire. Une implementation
# fondee sur les couches manquerait le support de rang ferme 5.
add_test(NAME mhgp3v_oracle_convex_layer_refutation COMMAND mhgp3v_oracle
         --fixture convex_layer_refutation --subject edge_shallow
         --clouds 1 --min-decided 1 --min-nodes 1)

# Exerce le terme c_e : un point SUR la droite de l'ancre est une forme
# constante interieure. Sans lui, un vert ne prouve que rang = arite.
add_test(NAME mhgp3v_oracle_constant_inside_witness COMMAND mhgp3v_oracle
         --fixture constant_inside_witness --subject edge_shallow
         --clouds 1 --min-decided 1 --min-nodes 1)

# Le generateur par NAVIGATION dans l'arrangement, juge par la verite exhaustive :
# catalogue, forets et niveaux exacts, toutes arites, avec cospheries traitees.
add_test(NAME mhgp3v_oracle_order_k COMMAND mhgp3v_oracle
         --subject order_k --clouds 30 --seed 777
         --min-points 8 --max-points 14 --max-order 6
         --min-decided 10 --min-nodes 100)

# ---------------------------------------------------------------------------
# M3' — le parcours MULTIPLICITAIRE contre la verite exhaustive.
#
# Trois portes dans le meme binaire, et il faut les trois : le SOMMET
# (coquille ET niveau strict), le CATALOGUE (supports canoniques et rangs) et
# l'EQUIVARIANCE par permutation. La troisieme n'est pas cosmetique : sans elle,
# la convention de support canonique « plus petit sous-ensemble par identifiant »
# passe les deux premieres et publie pourtant un support different selon la
# numerotation des points, des qu'une miniboule a plusieurs supports minimaux.
# ---------------------------------------------------------------------------

# Les fixtures aux coordonnees exactes des audits, a tous les ordres 2..8, plus
# leur equivariance. Le plancher interdit une campagne vide.
add_test(NAME mhgp3v_flats_fixtures COMMAND mhgp3v_flats_differential
         --clouds 0 --min-cases 150 --min-navigated 100 --min-vertices 500
         --min-multiple-shells 20 --min-quotiented 20)

# Nuages generiques : c'est la porte de completude ordinaire.
add_test(NAME mhgp3v_flats_generic COMMAND mhgp3v_flats_differential
         --clouds 150 --points 11 --coord 24 --smax 6 --seed 4242 --min-cases 1000
         --min-navigated 700 --min-vertices 50000)

# Grille SATUREE : dix points dans une boite de cote cinq. Presque tous les
# nuages y portent des cospheries, des coplanarites et des alignements. C'est le
# regime que l'ancien parcours censurait, et celui que celui-ci doit traverser.
add_test(NAME mhgp3v_flats_degenerate COMMAND mhgp3v_flats_differential
         --clouds 120 --points 10 --coord 5 --smax 8 --seed 31337 --min-cases 1000
         --min-navigated 500 --min-multiple-shells 300 --min-quotiented 300)

# GATE D est PARTIELLEMENT verifiee dans les quatre portes positives : parent
# unique hors racine, inclusion des ensembles interieurs, absence de cycle,
# racine unique et couverture identique au parcours. Ne sont PAS encore
# verifies : le rang trois de la fermeture C(d), l'identite S(next) = C(d) union
# A, la finitude de l'extremite et la stricte variation du potentiel. Ces quatre
# assertions doivent preceder tout remplacement du parcours.

# Nuages enrichis en points d'une meme sphere entiere : coquilles de taille
# arbitraire et lots entrants multiples.
add_test(NAME mhgp3v_flats_cospherical COMMAND mhgp3v_flats_differential
         --clouds 240 --points 8 --coord 40 --smax 7 --seed 90210 --min-cases 1400
         --min-navigated 1000 --min-multiple-shells 200)

# La dichotomie doit reproduire l'UNIVERS DES COFACES, l'UNIVERS DES FACETTES,
# lambda(F) et M(F). La verite enumere elle-meme toutes les cofaces de cardinal
# k+1 a vacuite OUVERTE : une coface omise par le sujet est donc un desaccord, et
# non une facette qui n'est jamais soumise au juge.
#
# Trois regimes, avec planchers : une grille de cote deux ou la branche fermee
# est totale et les extra-shells partout ; un nuage assez grand pour exercer les
# noeuds internes de l'index ; un ordre plus haut.
add_test(NAME mhgp3v_first_incidence_extra_shell COMMAND mhgp3v_first_incidence
         --clouds 20 --points 8 --coord 2 --k 3 --seed 7
         --min-closed 1000 --min-internal-nodes 15)
add_test(NAME mhgp3v_first_incidence_internal_nodes COMMAND mhgp3v_first_incidence
         --clouds 12 --points 20 --coord 20 --k 3 --seed 11
         --min-closed 800 --min-empty 400 --min-internal-nodes 80)
add_test(NAME mhgp3v_first_incidence_high_order COMMAND mhgp3v_first_incidence
         --clouds 12 --points 9 --coord 3 --k 4 --seed 13
         --min-closed 400 --min-empty 80 --min-internal-nodes 20)
mhgp3v_add_expected_failure_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_absurd "campagne absurde" --clouds 0)
mhgp3v_add_expected_failure_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_unknown "argument inconnu" --sujet-inexistant)
mhgp3v_add_expected_failure_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_suffix "valeur entiere invalide" --clouds 1junk)
mhgp3v_add_expected_failure_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_floor "plancher de couverture non atteint"
  --clouds 2 --points 8 --coord 20 --k 3 --min-closed 1000000)

# Interface fail-closed du juge lui-meme.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_unknown_argument "argument inconnu" --clouds 1 --sujet-inexistant)
# Chaque option porte desormais sa plage semantique, verifiee AVANT le cast :
# le diagnostic est celui de l'option fautive, pas un « campagne absurde » global.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_smax_below_contract "valeur entiere invalide"
  --clouds 1 --smax 1)
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_empty_campaign "plancher non atteint" --clouds 0 --min-cases 100000)
# `atoi` acceptait « 0junk » et rendait zero : la campagne devenait vide et verte.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_integer_suffix "valeur entiere invalide" --clouds 0junk --min-cases 1)
# La grille declaree est u16 ; hors d'elle les bornes des predicats ne tiennent plus.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_coord_outside_grid "valeur entiere invalide"
  --clouds 1 --points 11 --coord 2147483647 --smax 2 --min-cases 1)
# Une campagne qui ne navigue jamais ne peut pas rendre OK.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_no_navigation "plancher de couverture non atteint"
  --clouds 0 --min-cases 1 --min-navigated 100000)
# Le token etait lu en entier, mais le `long long` etait ensuite tronque en
# `int` sans controle de plage : `--clouds 4294967296` rendait zero nuage.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_clouds_narrowing "valeur entiere invalide"
  --clouds 4294967296 --min-cases 1)
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_coord_narrowing "valeur entiere invalide"
  --clouds 0 --coord 4295032832 --min-cases 1)
# Neuf points DISTINCTS dans huit positions : la demande est impossible et ne
# doit pas etre censuree en silence par le tirage.
mhgp3v_add_expected_failure_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_impossible_campaign "campagne impossible"
  --clouds 1 --points 9 --coord 2 --smax 2 --min-cases 1)

# Interface fail-closed : token entier partiel, reçu non ecrit et option ignoree
# doivent tous fournir un code non nul ET le diagnostic attendu.
mhgp3v_add_expected_failure_test(mhgp3v_cli_reject_integer_suffix
  "valeur entiere invalide" --clouds 1junk)
mhgp3v_add_expected_failure_test(mhgp3v_receipt_write_failure
  "ecriture du recu incomplete"
  --fixture foreign_source_same_level --subject v2 --clouds 1 --min-points 4
  --max-points 4 --max-order 1 --coord-max 2 --min-decided 1 --min-nodes 1
  --receipt /dev/full)
mhgp3v_add_expected_failure_test(mhgp3v_measure_receipt_rejected
  "measure-only ne prend encore ni --receipt ni --inject"
  --subject anchored --measure-only --regime exhaustive --points 4 --clouds 1
  --max-order 1 --receipt ignored.json)
