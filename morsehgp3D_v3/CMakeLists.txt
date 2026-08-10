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
find_package(Threads REQUIRED)

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

# LE JUGE GAMMA_k : la definition du manuscrit (theoreme 2, protocole de lot
# normatif de la reponse Q1.2) contre la chaine `flat_catalogue`+`build_forest`,
# comparee par PARTITIONS PAR NIVEAU — libres de toute convention de noeud. Sur
# les ordres sans evenement degenere ni censure de rang, l'accord est EXIGE ;
# sur les autres il est MESURE et publie : c'est la carte de la frontiere Q1.
# La geometrie exacte est PARTAGEE avec M1 (`exact_geometry.hpp`), jamais
# dupliquee : deux juges dupliques peuvent etre faux d'accord.
add_executable(mhgp3v_gamma_judge oracle/gamma_forest_judge.cpp)
target_include_directories(mhgp3v_gamma_judge
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oracle ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_gamma_judge PRIVATE mhgp)
target_compile_options(mhgp3v_gamma_judge PRIVATE -Wall -Wextra -Wpedantic)

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

# LE FRONT D'ONDE. Le meme corps est compile pour l'hote et, si CUDA est demande,
# pour sm_120. Sans CUDA le binaire qualifie le lot, le recu et la comparaison au
# chemin non borne ; avec CUDA il exige EN PLUS l'egalite bit a bit avec le kernel.
# Tout ce qui peut etre falsifie sans GPU l'est sans GPU.
add_executable(mhgp3v_device_wavefront prototype/device_wavefront_qualification.cpp)
target_include_directories(mhgp3v_device_wavefront PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_device_wavefront PRIVATE mhgp)
# Les avertissements C++ ne doivent pas fuir dans la compilation CUDA : nvcc
# recoit -Werror comme une option a valeur et refuse alors --fmad=false.
target_compile_options(mhgp3v_device_wavefront PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>)

# LA MESURE QUI DECIDE. A densite FIXE, le terrain croit-il ? La reponse est le
# seul chiffre qui dise si 100 ms est atteignable a 50 000 points.
add_executable(mhgp3v_scale_profile prototype/scale_profile.cpp)
target_include_directories(mhgp3v_scale_profile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_scale_profile PRIVATE mhgp)
target_compile_options(mhgp3v_scale_profile PRIVATE -Wall -Wextra -Werror)

# LA SELECTIVITE DU LEMME DU DEMI-BOULE. Le budget du generateur direct depend d'une
# quantite que sa propre specification declare NON BORNEE : la taille de l'univers
# admissible. Ce probe la mesure, et verifie que le lemme ne refute aucune paire reelle.
add_executable(mhgp3v_admissible_pair_probe prototype/admissible_pair_probe.cpp)
target_include_directories(mhgp3v_admissible_pair_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_admissible_pair_probe PRIVATE mhgp)
target_compile_options(mhgp3v_admissible_pair_probe PRIVATE -Wall -Wextra -Werror)

# PROTOTYPE DE SOURCE DIRECTE. Il n'enumere aucun sommet d'arrangement : il
# enumere les SUPPORTS depuis leur ancre, dans un voisinage dont la completude
# est un theoreme. Ce qu'il etablit est un ACCORD RELATIF aux primitives
# `mhgp::sphere_*` et `mhgp::miniball_of` qu'il partage avec la reference, sur
# les campagnes exercees — ni une certification, ni un SLO.
add_executable(mhgp3v_direct_source prototype/direct_source.cpp)
target_include_directories(mhgp3v_direct_source PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_direct_source PRIVATE mhgp)
target_compile_options(mhgp3v_direct_source PRIVATE -Wall -Wextra -Werror)

# LE PIPELINE SATURE CHRONOMETRE : nuage -> catalogue -> fold, temps par etage.
# Binaire de MESURE (les verdicts d'exactitude vivent dans le juge Gamma).
add_executable(mhgp3v_saturated_pipeline prototype/saturated_pipeline.cpp)
target_include_directories(mhgp3v_saturated_pipeline PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_saturated_pipeline PRIVATE mhgp)
target_compile_options(mhgp3v_saturated_pipeline PRIVATE -Wall -Wextra -Werror)

# LA PORTE DU JOIN PAR POSTINGS : differentiel bit a bit contre le fold de
# verite O(G^2), fixtures nommees de la note 50 k, identites de recu, mutants.
add_executable(mhgp3v_postings_join_gate prototype/postings_join_gate.cpp)
target_include_directories(mhgp3v_postings_join_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_postings_join_gate PRIVATE mhgp)
target_compile_options(mhgp3v_postings_join_gate PRIVATE -Wall -Wextra -Werror)

# LA QUALIFICATION DU KERNEL FACE-OWNER : differentiel natif arete par arete
# contre le chemin CPU collect_edges. Sans CUDA le binaire refuse proprement.
add_executable(mhgp3v_faceowner_device_qualification
  prototype/faceowner_device_qualification.cpp)
target_include_directories(mhgp3v_faceowner_device_qualification
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_faceowner_device_qualification PRIVATE mhgp)
target_compile_options(mhgp3v_faceowner_device_qualification PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>)
# LA VERIFICATION STRUCTURELLE A L'ECHELLE : k=1 == single-linkage par EMST
# exact (Prim i128), egalite de multisets de niveaux via sphere_cmp_beta —
# l'invariant qui tient aux tailles sans verite exhaustive, troncature comprise.
add_executable(mhgp3v_structural_scale_check prototype/structural_scale_check.cpp)
target_include_directories(mhgp3v_structural_scale_check PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_structural_scale_check PRIVATE mhgp)
target_compile_options(mhgp3v_structural_scale_check PRIVATE -Wall -Wextra -Werror)

# LA PORTE DU CATALOGUE PARALLELE : front d'onde CPU == sequentiel par la clef
# compacte (membres tries) + support canonique + rang, travail invariant entre
# 2 et 3 threads (insensible a l'horloge), mutant drop-last-root tue.
add_executable(mhgp3v_parallel_catalogue_gate prototype/parallel_catalogue_gate.cpp)
target_include_directories(mhgp3v_parallel_catalogue_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_parallel_catalogue_gate PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_parallel_catalogue_gate PRIVATE -Wall -Wextra -Werror)

# LA PORTE DU ROUTEUR MEDIAN (note dendrogramme, Th. 5.1 et §14) : la descente
# a majorite stricte == l'enumeration de TOUS les terminaux (perte minimale ET
# noeud le plus haut du segment de medianes), fixtures 5/6/7 de la note gravees,
# quatre mutants nommes de la note tues. Combinatoire pure : aucun lien mhgp.
add_executable(mhgp3v_tree_median_gate prototype/tree_median_gate.cpp)
target_include_directories(mhgp3v_tree_median_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(mhgp3v_tree_median_gate PRIVATE -Wall -Wextra -Werror)

option(MHGP3V_ENABLE_CUDA "Compiler le kernel de front d'onde pour sm_120" OFF)
if(MHGP3V_ENABLE_CUDA)
  enable_language(CUDA)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120-real)
  endif()
  target_sources(mhgp3v_device_wavefront PRIVATE prototype/device_wavefront_kernel.cu)
  target_compile_definitions(mhgp3v_device_wavefront PRIVATE MHGP3V_WAVEFRONT_CUDA)
  target_sources(mhgp3v_faceowner_device_qualification PRIVATE
    prototype/faceowner_device_kernel.cu)
  target_compile_definitions(mhgp3v_faceowner_device_qualification PRIVATE
    MHGP3V_FACEOWNER_CUDA)
  set_target_properties(mhgp3v_faceowner_device_qualification PROPERTIES
    CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_EXTENSIONS OFF
    CUDA_SEPARABLE_COMPILATION OFF)
  # Meme hygiene numerique que le front d'onde : le chemin est entier, la
  # discipline le garde reproductible face a l'oracle CPU.
  target_compile_options(mhgp3v_faceowner_device_qualification PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false;--ftz=false;--prec-div=true;--prec-sqrt=true>)
  set_target_properties(mhgp3v_device_wavefront PROPERTIES
    CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_EXTENSIONS OFF
    CUDA_SEPARABLE_COMPILATION OFF)
  # Meme hygiene numerique que morsehgp3d : aucun FMA contracte, aucun
  # flush-to-zero, division et racine exactes. Le chemin est entier, mais la
  # discipline reste la meme et le rend reproductible face a l'oracle CPU.
  target_compile_options(mhgp3v_device_wavefront PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false;--ftz=false;--prec-div=true;--prec-sqrt=true>)
  message(STATUS "mhgp3v : kernel de front d'onde active pour ${CMAKE_CUDA_ARCHITECTURES}")
endif()

enable_testing()

# QUAND LE CONTRAT FIXE LE CODE DE SORTIE, LA PORTE L'EXIGE. 2 = campagne
# refusee avant tout calcul, 3 = plancher ou invariant viole, 1 = desaccords du
# juge. L'audit live a montre que le harness acceptait un crash par signal comme
# un rejet : le harness refuse desormais tout resultat non entier, et cette
# variante refuse en plus tout code different du contrat.
function(mhgp3v_add_expected_code_test_for program name expected code)
  add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
           "-DPROGRAM=$<TARGET_FILE:${program}>"
           "-DEXPECTED=${expected}"
           "-DEXPECTED_CODE=${code}"
           "-DARGUMENTS=${ARGN}"
           -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
endfunction()

add_test(NAME mhgp3v_arith_selftest COMMAND mhgp3v_arith_selftest 20000)
# LE PARSEUR DU SELF-TEST EST STRICT, ET C'EST GARDE. `atoi` acceptait `1junk`
# — vingt verifications, OK imprime — et aucun argument excedentaire n'etait
# refuse (audit live `f37341d`).
mhgp3v_add_expected_code_test_for(mhgp3v_arith_selftest
  mhgp3v_arith_selftest_reject_junk "tours invalide" 2 1junk)
mhgp3v_add_expected_code_test_for(mhgp3v_arith_selftest
  mhgp3v_arith_selftest_reject_excess "argument excedentaire" 2 20000 20000)
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
mhgp3v_add_expected_code_test_for(mhgp3v_oracle mhgp3v_cli_order_overflow "campagne absurde" 2
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
mhgp3v_add_expected_code_test_for(mhgp3v_oracle 
  mhgp3v_oracle_inject_vacuity_guard
  "campagne SUPPORT doit etre verte avant tout probe hostile" 1
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
         --min-multiple-shells 20 --min-quotiented 20
         --min-reverse 1500 --min-reverse-depth 6 --min-internal-nodes 100 --min-owner 2000)
# NOTE. `--min-reverse-depth` porte desormais aussi sur les DECISIONS de filiation et
# sur les refus en O(m) par le couple de retour : sans ces deux planchers, un mutant
# « prefiltre toujours vrai » laisserait l'optimisation morte et la porte verte.

# Nuages generiques : c'est la porte de completude ordinaire.
add_test(NAME mhgp3v_flats_generic COMMAND mhgp3v_flats_differential
         --clouds 150 --points 11 --coord 24 --smax 6 --seed 4242 --min-cases 1000
         --min-navigated 700 --min-vertices 50000
         --min-reverse 50000 --min-reverse-depth 8 --min-internal-nodes 1000 --min-owner 40000)

# NUAGE ASSEZ GRAND POUR QUE L'INDEX AIT UN ARBRE. Tous les autres nuages
# permanents ont au plus treize points : avec des feuilles de quatre ils ont deja
# des noeuds internes, mais vingt points et un ordre bas donnent un arbre profond
# et un rejeu indexe qui elague vraiment. Sans cette porte, le chemin indexe
# n'etait qualifie que sur une feuille unique.
add_test(NAME mhgp3v_flats_indexed_tree COMMAND mhgp3v_flats_differential
         --clouds 2 --points 20 --coord 40 --smax 4 --seed 20260809 --min-cases 200
         --min-navigated 100 --min-vertices 3000 --min-reverse 3000 --min-reverse-depth 15
         --min-internal-nodes 100)

# Grille SATUREE : dix points dans une boite de cote cinq. Presque tous les
# nuages y portent des cospheries, des coplanarites et des alignements. C'est le
# regime que l'ancien parcours censurait, et celui que celui-ci doit traverser.
add_test(NAME mhgp3v_flats_degenerate COMMAND mhgp3v_flats_differential
         --clouds 120 --points 10 --coord 5 --smax 8 --seed 31337 --min-cases 1000
         --min-navigated 500 --min-multiple-shells 300 --min-quotiented 300
         --min-reverse 30000 --min-reverse-depth 8 --min-internal-nodes 1000 --min-owner 30000)

# LA GRILLE u16 ENTIERE, ET PAS SEULEMENT DE PETITES COORDONNEES.
#
# Toutes les portes ci-dessus tiennent dans une boite de cote au plus quarante.
# Le determinant d'orientation y reste tres en dessous de 2^31, si bien qu'une
# TRONCATURE 32 bits du signe owner ne s'y voyait pas : le catalogue restait
# juste par accident d'echelle. Le defaut n'apparaissait qu'a coordonnee pleine.
#
# Cette porte navigue donc sur la grille declaree, la seule ou le contrat u16 est
# reellement exerce. Elle est le pendant en campagne de la fixture
# `[frontiere u16 owner]`, qui grave les deux frontieres exactes 1290/1291 et
# 1023/1024/1025.
add_test(NAME mhgp3v_flats_u16_owner COMMAND mhgp3v_flats_differential
         --clouds 60 --points 10 --coord 65536 --smax 4 --seed 20260809 --min-cases 400
         --min-navigated 200 --min-vertices 10000
         --min-reverse 10000 --min-reverse-depth 8 --min-internal-nodes 500 --min-owner 8000)

# LA SOURCE DIRECTE, JUGEE CONTRE LE CATALOGUE FERME.
#
# La porte exige l'accord RELATIF complet, clef par coquille : meme ensemble de
# spheres, meme support canonique, meme rang, meme niveau rationnel et meme
# LISTE DE MEMBRES PAR COQUILLE. C'est un quotient semantique : le pool public
# concatene et ses offsets ne sont PAS compares ici (audit chrono `e406e1f`).
# Sans les listes par coquille, deux payloads de meme rang et d'interieurs
# differents etaient indiscernables.
#
# Elle exige aussi l'UNICITE. La map de sortie ecrasait ses doublons : un mutant
# retirant la restriction `z > p` emettait 126 fois au lieu de 56 sans qu'aucun
# compteur ne bouge. Les emissions sont maintenant comptees a part.
add_test(NAME mhgp3v_direct_source_generic COMMAND mhgp3v_direct_source
         --clouds 6 --points 40 --coord 160 --smax 8 --seed 20260810 --judge 1
         --min-clouds 6 --min-emitted 10000 --min-candidates 500000 --min-windowed 8000)
# La grille SATUREE : cospheries, coplanarites et alignements partout. C'est la
# ou la convention de support canonique decide, et ou une source directe naive
# emettrait plusieurs fois la meme sphere. Le cover y tombe en repli racine, ce
# qui est correct et doit rester exact : aucun plancher de fenetre ici.
add_test(NAME mhgp3v_direct_source_degenerate COMMAND mhgp3v_direct_source
         --clouds 12 --points 12 --coord 5 --smax 8 --seed 31337 --judge 1
         --min-clouds 12 --min-emitted 1800 --min-candidates 8000)
# `n < t_q` : la banque ne peut jamais conclure, elle est OTEE et le voisinage
# racine est exact. C'etait une sortie generique; c'est un statut, et il doit
# rester juste.
add_test(NAME mhgp3v_direct_source_small_cloud COMMAND mhgp3v_direct_source
         --clouds 6 --points 5 --coord 10 --smax 32 --seed 7 --judge 1
         --min-clouds 6 --min-emitted 30)
# LA FORET DES K ARBRES, BOUT EN BOUT.
#
# Le catalogue n'est que la moitie du contrat. Cette porte construit la foret
# depuis LA SOURCE et depuis LA REFERENCE pour k = 1..5, puis compare des
# signatures RECURSIVES canoniques.
#
# Elle ne compare surtout pas des indices : `ForestNode::source` d'une
# multifusion est « la plus petite PAR INDEX des spheres du lot », donc dependant
# du generateur. Une naissance est identifiee par les membres de son minimum, une
# multifusion par le RANG EXACT de son niveau dans l'ordre `sphere_cmp_beta`.
add_test(NAME mhgp3v_direct_source_forest COMMAND mhgp3v_direct_source
         --clouds 6 --points 14 --coord 12 --smax 6 --seed 20260810 --judge 1 --forest 5
         --min-clouds 6 --min-emitted 1000 --min-forest-nodes 1000)
# Une foret d'ordre k lit les rangs k et k+1 : demander k = s_max rendrait des
# ordres tronques en les qualifiant.
# LA COMPARAISON A PAYLOAD PUBLIC CANONIQUE, en porte permanente. Chaque cote
# produit son payload — catalogue serialise dans l'ordre lexicographique du
# support, pool reconstruit, puis K forets depuis CE catalogue — sous SON
# chrono; le juge (differentiel, empreintes, comparaisons champ a champ) a une
# horloge separee, fermee avant l'impression. Le payload public est compare EN
# ENTIER : spheres avec representation exacte, pool, offsets, forets et indices
# `ForestNode::source` — la sonde d'audit qui trouvait 3 762 positions
# divergentes sous quotient vert est devenue le mutant `--force-shell-order`.
# La porte garde l'EXACTITUDE du couple, pas sa vitesse.
add_test(NAME mhgp3v_direct_source_same_payload COMMAND mhgp3v_direct_source
         --clouds 4 --points 40 --coord 34 --smax 6 --seed 20260810 --judge 1 --forest 5
         --min-clouds 4 --min-emitted 2000 --min-forest-nodes 2000)
# LES LIBELLES TEMPORELS SONT DU CONTRAT, PAS DU DECOR. L'audit a montre qu'un
# mutant remplacant stdout par des claims de payload public identique et de juge
# certifie hors timer laissait 14/14 portes vertes : cette porte exige le
# libelle exact — payload public canonique, juge sous sa propre horloge — sur la
# meme commande. PASS_REGULAR_EXPRESSION ignore le code de retour; la porte
# precedente le garde.
add_test(NAME mhgp3v_direct_source_same_payload_labels COMMAND mhgp3v_direct_source
         --clouds 4 --points 40 --coord 34 --smax 6 --seed 20260810 --judge 1 --forest 5
         --min-clouds 4 --min-emitted 2000 --min-forest-nodes 2000)
set_tests_properties(mhgp3v_direct_source_same_payload_labels PROPERTIES
  PASS_REGULAR_EXPRESSION
  "PAYLOAD PUBLIC CANONIQUE .* juge [0-9.]+ s .[0-9]+ comparaisons. HORS des deux chronos")
# LE VALIDATEUR DE FORET EST TOTAL, ET SES DEUX FAUTES D'AUDIT SONT DES
# FIXTURES : cycle de freres (l'ancien controle bouclait) et enfant hors plage
# (il lisait hors du tableau, overflow ASan). L'auto-test exige la terminaison,
# le compte nomme de chaque faute, l'empreinte STRUCTURE_INVALIDE sans recursion
# et l'acceptation du temoin sain.
add_test(NAME mhgp3v_direct_source_forest_validator COMMAND mhgp3v_direct_source
         --selftest-forest-validator)
# LE MUTANT D'ORDRE DE COQUILLE DOIT ROUGIR PAR LA PORTE DE PAYLOAD, PAS PAR LE
# QUOTIENT. Il assemble le catalogue source dans l'ordre d'iteration de la map
# par coquille — exactement la faute que la sonde d'audit observait — et le
# quotient semantique reste vert : seul le comparateur champ a champ le voit.
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_shell_order "PAYLOAD PUBLIC DIFFERENT" 1
  --clouds 2 --points 40 --coord 34 --smax 6 --seed 20260810 --judge 1 --forest 5
  --force-shell-order 1)
# L'ORDRE DE CONSTRUCTION EST UN PARAMETRE DE CAMPAGNE : --build-order 1
# construit la source d'abord, pour que les campagnes chrono alternent la
# position sans soustraire deux modes CLI (l'audit a montre que cette
# soustraction est confondue par la position et l'etat initial). La porte garde
# l'exactitude sous cet ordre, pas la vitesse.
add_test(NAME mhgp3v_direct_source_source_first COMMAND mhgp3v_direct_source
         --clouds 4 --points 40 --coord 34 --smax 6 --seed 20260810 --judge 1 --forest 5
         --build-order 1 --min-clouds 4 --min-emitted 2000 --min-forest-nodes 2000)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_build_order "qu'une seule construction" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --judge 0 --build-order 1)
# LE REFUS FINAL DES STATUTS NON kOk EST UNE PORTE, PAS UNE POLITESSE. L'audit a
# montre qu'un mutant le neutralisant laissait 14/14 vertes. Cette commande — la
# reproduction exacte de l'audit — porte vingt tentatives dont un refus de
# dimension affine, et DOIT sortir en echec avec sa raison; le refus est compte
# et chronometre A PART, hors des deux chronos.
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_refused_status "statut non kOk" 3
  --clouds 20 --points 5 --coord 4 --smax 3 --seed 13 --judge 1 --forest 2
  --min-clouds 19 --min-emitted 1 --min-forest-nodes 1)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_forest_order "exige s_max" 2
  --clouds 1 --points 12 --coord 5 --smax 4 --seed 31337 --judge 1 --forest 4)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_forest_without_judge "ne se juge que sous" 2
  --clouds 1 --points 12 --coord 5 --smax 8 --seed 31337 --judge 0 --forest 3)
# LES ORDRES BAS ETAIENT UN DOMAINE CONTRADICTOIRE : la CLI acceptait `s_max=2`
# puis la lane q=3 echouait sur `t_q < 1` avec un message generique. Une lane
# d'arite q > s_max est VIDE par contrat — un support d'arite q a un rang au
# moins q — et c'est maintenant ce qui se passe.
add_test(NAME mhgp3v_direct_source_low_order COMMAND mhgp3v_direct_source
         --clouds 8 --points 10 --coord 8 --smax 2 --seed 5 --judge 1
         --min-clouds 8 --min-emitted 80)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_unknown "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_floor "plancher" 3
  --clouds 1 --points 12 --coord 5 --smax 8 --seed 31337 --min-emitted 100000000)
# LES TROIS MODES SONT EXCLUSIFS. Demander un jugement en sautant l'enumeration
# imprimait des spheres manquantes, ignorait les planchers et sortait zero.
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_cover_and_judge "ne peut pas juger" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --cover-only 1 --judge 1)
# LE MODE COVER AUTONOME A SA PORTE POSITIVE. La regression auditee a `84adbcc` :
# le mode sautait l'enumeration puis la garde finale exigeait quand meme
# `candidats == C_q`, comparait 0 a C_q et sortait 3 — le disclaimer MODE COVER
# n'etait jamais atteint, et le seul CTest cover ne testait que le conflit
# cover+juge. Cette porte exerce le chemin positif, avec des planchers a
# semantique cover : nuages decides et tests de cover.
add_test(NAME mhgp3v_direct_source_cover_mode COMMAND mhgp3v_direct_source
         --clouds 2 --points 100 --coord 50 --smax 11 --seed 7 --leaf 8
         --judge 0 --cover-only 1 --min-clouds 2 --min-cover-tests 100000
         --min-lane-cover-tests 60000)
# La meme commande, jugee cette fois sur ses LIBELLES : la ligne temporelle ET
# le disclaimer final — l'audit live a montre que viser le seul disclaimer
# laissait revenir l'ancien libelle « catalogue seul — reference=0.000 ».
# PASS_REGULAR_EXPRESSION ignore le code de retour, c'est pourquoi la porte
# precedente existe a part.
add_test(NAME mhgp3v_direct_source_cover_mode_disclaimer COMMAND mhgp3v_direct_source
         --clouds 2 --points 100 --coord 50 --smax 11 --seed 7 --leaf 8
         --judge 0 --cover-only 1 --min-clouds 2 --min-cover-tests 100000
         --min-lane-cover-tests 60000)
set_tests_properties(mhgp3v_direct_source_cover_mode_disclaimer PROPERTIES
  PASS_REGULAR_EXPRESSION
  "temps      : MODE COVER(.|\n)*MODE COVER : cover, voisinages et masses combinadiques seulement")
# Un plancher d'enumeration en mode cover est STRUCTURELLEMENT inatteignable :
# la combinaison est refusee fail-closed, jamais laissee rougir plus tard. Les
# QUATRE branches sont gardees — l'audit live a releve que seule la premiere
# l'etait, les trois autres pouvant regresser sans rougir.
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_cover_floor "incompatibles avec --cover-only" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --cover-only 1 --judge 0
  --min-emitted 1)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_cover_floor_windowed "incompatibles avec --cover-only" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --cover-only 1 --judge 0
  --min-windowed 1)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_cover_floor_candidates "incompatibles avec --cover-only" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --cover-only 1 --judge 0
  --min-candidates 1)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_cover_floor_forest "incompatibles avec --cover-only" 2
  --clouds 1 --points 10 --coord 10 --smax 4 --seed 7 --cover-only 1 --judge 0
  --min-forest-nodes 1)
# LE MUTANT DE DOUBLE EMISSION DOIT ROUGIR. Il est instructif : le differentiel
# par coquille reste vert, parce que la map ecrase; c'est le compteur d'unicite
# qui le tue.
# LE COMPARATEUR DE MEMBRES DOIT ETRE VIVANT. Sans ce mutant, rien ne prouve
# qu'il compare autre chose que des listes vides : il retire un membre a chaque
# sphere emise, et la porte doit rougir.
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_dropped_member_with_forest "ne peut pas etre combine" 2
  --clouds 1 --points 14 --coord 12 --smax 6 --seed 1 --judge 1 --force-drop-member 1
  --forest 3)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_dropped_member "desaccords" 1
  --clouds 2 --points 14 --coord 12 --smax 6 --seed 20260810 --judge 1
  --force-drop-member 1)
mhgp3v_add_expected_code_test_for(mhgp3v_direct_source
  mhgp3v_direct_source_reject_double_emission "en double" 3
  --clouds 2 --points 12 --coord 5 --smax 8 --seed 31337 --force-both-directions 1)

# LE JUGE GAMMA_k, EN PORTES PERMANENTES. La porte generique exige l'ACCORD
# EXACT des partitions par niveau sur les ordres sans cosphericite (Def. 26 du
# manuscrit) ni censure de rang — c'est le domaine ou la chaine pretend
# l'exactitude, et le juge est INDEPENDANT (arithmetique de l'oracle, protocole
# de lot normatif de la reponse Q1.2). La porte saturee MESURE la frontiere :
# grille 4^3, cosphericites partout ; ses planchers garantissent que le regime
# degenere est bien exerce, sans figer le compte de divergences — une future
# lecture elargie du fold pourrait legitimement les faire tomber a zero.
add_test(NAME mhgp3v_gamma_judge_generic COMMAND mhgp3v_gamma_judge
         --clouds 30 --points 8 --coord 40 --smax 11 --max-order 3 --seed 7
         --min-decided 30 --min-nondegenerate 20 --min-levels 3000 --min-judged 80)
add_test(NAME mhgp3v_gamma_judge_saturated COMMAND mhgp3v_gamma_judge
         --clouds 20 --points 9 --coord 4 --smax 11 --max-order 3 --seed 31337
         --min-decided 20 --min-degenerate 15 --min-levels 200 --min-judged 10)
# LE MUTANT DE NIVEAU DEPLACE : une fusion sujet poussee vers un niveau
# ETRANGER n'est visible qu'en comparant sur l'UNION des niveaux, aux deux
# coupes — le trou releve par l'audit live (echantillonnage aux seuls niveaux
# de la verite, coupe fermee seule).
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_shifted_level "desaccords hors degenerescence" 1
  --clouds 10 --points 8 --coord 40 --smax 11 --max-order 3 --seed 7
  --force-shift-level 1)
# LE FOLD SATURE (S.4/S.5, prototype/saturated_fold.hpp), JUGE LA OU v2
# CENSURE. Sur la grille 4^3 la chaine build_forest declarait 40/60 ordres non
# autoritatifs ; le fold sature juge 60/60 et l'accord est EXIGE jusque SUR le
# regime degenere (--require-degenerate-agreement) : c'est la semantique
# industrielle multiplicitaire, demontree sur son juge. La famille d'entree est
# complete (s_max > n) — sous famille tronquee le fold n'est qu'un raffinement
# S.6 et ne se qualifie pas ici.
add_test(NAME mhgp3v_gamma_judge_fold_generic COMMAND mhgp3v_gamma_judge
         --clouds 30 --points 8 --coord 40 --smax 11 --max-order 3 --seed 7
         --subject-fold 1 --require-degenerate-agreement 1
         --min-decided 30 --min-nondegenerate 20 --min-levels 3000 --min-judged 85)
add_test(NAME mhgp3v_gamma_judge_fold_saturated COMMAND mhgp3v_gamma_judge
         --clouds 20 --points 9 --coord 4 --smax 11 --max-order 3 --seed 31337
         --subject-fold 1 --require-degenerate-agreement 1
         --min-decided 20 --min-degenerate 15 --min-levels 1500 --min-judged 55)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_fold_mutant "sens que pour le sujet chaine" 2
  --clouds 1 --points 8 --coord 40 --smax 11 --max-order 2 --seed 7
  --subject-fold 1 --force-shift-level 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_unknown "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_absurd "campagne absurde" 2 --points 3)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_floor "plancher" 3
  --clouds 2 --points 6 --coord 10 --smax 6 --max-order 2 --seed 7
  --min-decided 1000)

# LE PREDICAT STRUCTUREL D'EVENEMENT GAMMA, EN PORTES PERMANENTES. Les
# theoremes 1--2 de NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN sont RECUS contre la
# verite exhaustive : un niveau du sature est un vrai niveau Gamma_k ssi son
# lot contient un generateur avec |M| >= k et q_min(B) <= k+1. Le juge calcule
# q_min par enumeration INDEPENDANTE (miniboule rationnelle de l'oracle),
# jamais en lisant n_support du produit — qui coinciderait sur ce chemin et
# ferait une porte morte ; le mutant demande par l'audit 621ee80 est le
# decalage q_min+1, tue par la reception. La campagne saturee exige l'accord
# JUSQUE sur le regime degenere.
add_test(NAME mhgp3v_gamma_judge_event_predicate_generic COMMAND mhgp3v_gamma_judge
         --clouds 10 --points 8 --coord 40 --smax 11 --max-order 3 --seed 7
         --subject-fold 1 --check-event-predicate 1 --min-event-levels 1000
         --min-records 1000)
add_test(NAME mhgp3v_gamma_judge_event_predicate_saturated COMMAND mhgp3v_gamma_judge
         --clouds 20 --points 9 --coord 4 --smax 11 --max-order 3 --seed 31337
         --subject-fold 1 --require-degenerate-agreement 1 --check-event-predicate 1
         --min-event-levels 1400 --min-records 1800)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_qmin_shift "desaccords" 1
  --clouds 2 --points 8 --coord 40 --smax 11 --max-order 2 --seed 7
  --subject-fold 1 --check-event-predicate 1 --force-qmin-shift 1)
# LES RECORDS PAR TEMOIN (note temoins de l'auditeur) : la bijection des
# composantes — temoin ferme omega_k, temoins stricts absorbes, type — est
# recue record a record contre l'oracle, qui construit ses temoins depuis ses
# propres k-faces. La fixture des continuations compensees (triangle q_min=3
# redondant et paire q_min=2 au MEME niveau exact 25) exerce le cas que les
# triples par niveau ne peuvent pas voir ; les quatre mutants du chemin sujet
# meurent, drop-strict-witness et stale-witness par les RECORDS SEULS.
add_test(NAME mhgp3v_gamma_judge_witness_fixture COMMAND mhgp3v_gamma_judge
         --witness-fixture 1 --max-order 3 --subject-fold 1 --check-event-predicate 1
         --require-degenerate-agreement 1 --min-records 30)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_skip_marker "REFUTE" 1
  --witness-fixture 1 --max-order 3 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-skip-event-marker 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_mark_redundant "REFUTE" 1
  --witness-fixture 1 --max-order 3 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-mark-redundant 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_drop_strict_witness "RECORDS REFUTES" 1
  --witness-fixture 1 --max-order 3 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-drop-strict-witness 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_stale_witness "RECORDS REFUTES" 1
  --witness-fixture 1 --max-order 3 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-stale-witness 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_compensated_marking "RECORDS REFUTES" 1
  --witness-fixture 1 --max-order 1 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-skip-event-marker 1 --force-mark-redundant 1)
# LE MUTANT COMPENSE EXACT (audit live) : echange atomique dans un MEME lot
# entre une racine marquee et une racine distincte de meme compte strict
# portant un generateur redondant — les triples restent inchanges PAR
# CONSTRUCTION, seuls les records mordent (temoin ferme divergent).
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_swap_marking "temoin ferme divergent" 1
  --witness-fixture 1 --max-order 1 --subject-fold 1 --check-event-predicate 1
  --require-degenerate-agreement 1 --force-swap-marking 1)
# LE MARQUEUR SUPERFLU dans une racine deja marquee : ni type ni temoin ne
# bougent — seules les BOULES MARQUANTES le voient, sur la grille saturee ou
# les niveaux egaux donnent des racines multi-marquees.
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_kill_extra_marker "boules marquantes divergentes" 1
  --clouds 5 --points 9 --coord 4 --smax 11 --max-order 3 --seed 31337
  --subject-fold 1 --check-event-predicate 1 --require-degenerate-agreement 1
  --force-extra-marker 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_predicate_without_fold "exige --subject-fold" 2
  --check-event-predicate 1)
mhgp3v_add_expected_code_test_for(mhgp3v_gamma_judge
  mhgp3v_gamma_judge_reject_shift_without_predicate "exige --check-event-predicate" 2
  --subject-fold 1 --force-qmin-shift 1)

# LA PORTE DU JOIN PAR POSTINGS, EN PORTES PERMANENTES. Les fixtures nommees de
# la note 50 k ont leur transcript GRAVE (une derive de la semantique Q1.2
# contredit la porte elle-meme) ; les campagnes generique et saturee font le
# differentiel COMPLET (partitions comprises) contre le fold de verite O(G^2),
# l'invariance par permutation du catalogue, et exigent par planchers que le
# regime multiplicitaire (grille 4^3) et les lots silencieux soient exerces.
add_test(NAME mhgp3v_postings_gate_fixtures COMMAND mhgp3v_postings_join_gate)
# Planchers calibres sur la campagne mesuree (1950/4916/3311/4782 generique,
# 1623/4200/1889/2386 saturee) avec marge : ils prouvent que la porte mord,
# sans figer les comptes exacts.
add_test(NAME mhgp3v_postings_gate_generic COMMAND mhgp3v_postings_join_gate
         --clouds 30 --points 8 --coord 40 --smax 11 --max-order 3 --seed 7
         --min-generators 1500 --min-unions 4000 --min-silent 2500 --min-levels 4000)
add_test(NAME mhgp3v_postings_gate_saturated COMMAND mhgp3v_postings_join_gate
         --clouds 20 --points 9 --coord 4 --smax 11 --max-order 3 --seed 31337
         --min-generators 1300 --min-unions 3500 --min-silent 1500 --min-levels 2000)
# LES SIX MUTANTS NOMMES par la note §6 : chacun doit mourir par une fixture,
# le differentiel ou une identite de recu — code 4 exige, message « mutant tue ».
# Un mutant survivant rend 0 et le test a code attendu rougit.
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_strict_threshold "mutant tue" 4 --mutant strict-threshold)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_skip_new_new "mutant tue" 4 --mutant skip-new-new)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_omit_last_posting "mutant tue" 4 --mutant omit-last-posting)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_truncate_membership "mutant tue" 4 --mutant truncate-membership)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_collect_silent "mutant tue" 4 --mutant collect-silent)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_kill_sequential_equal_levels "mutant tue" 4
  --mutant sequential-equal-levels)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_reject_unknown "argument inconnu" 2 --option-inexistante)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_reject_absurd "campagne absurde" 2 --points 3)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_reject_floor "plancher" 3
  --clouds 2 --points 6 --coord 10 --smax 6 --max-order 2 --seed 7
  --min-generators 100000000)
# LE PIPELINE REFUSE AUSSI SES ARGUMENTS ABSURDES, jointure comprise.
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_join "jointure inconnue" 2 --join magique)
# LES MUTANTS DU FOLD FACE-OWNER (porte minimale §7.4 de la reponse masse,
# plus le mutant de la refutation cofaces) : chacun meurt par le differentiel
# des quatre formes, la cosphere gravee ou l'identite incidences==binomiales.
foreach(faceowner_mutant
        owner-not-minimal publish-batch-early omit-signature duplicate-signature
        shift-k qmin-filter-partial)
  string(REPLACE "-" "_" faceowner_mutant_name ${faceowner_mutant})
  mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
    mhgp3v_faceowner_kill_${faceowner_mutant_name} "mutant tue" 4
    --faceowner-mutant ${faceowner_mutant})
endforeach()
# Le mutant de la refutation est CIBLE sur l'ordre refute : il ne s'applique
# qu'a k=6 et doit mourir SUR LA COSPHERE avec un desaccord a cet ordre — pas
# par un premier ecart sans rapport (audit 8d6516c §portes specifiques).
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_faceowner_kill_support_facet_filter "ordre k=6" 4
  --faceowner-mutant support-facet-filter)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_reject_unknown_faceowner "mutant face-owner inconnu" 2
  --faceowner-mutant inexistant)
# LES MUTANTS DU FOLD HYBRIDE (portes §7 de la note hybride) : le fast path
# force malgre le certificat meurt par une fusion perdue SUR LA COSPHERE ; la
# cle brute base/num/den meurt par le lookup manquant sous pretention complete.
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_kill_force_principal "mutant tue" 4 --hybrid-mutant force-principal)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_kill_raw_ball_key "lookup manquant" 4 --hybrid-mutant raw-ball-key)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_postings_gate_reject_unknown_hybrid "mutant hybride inconnu" 2
  --hybrid-mutant inexistant)
add_test(NAME mhgp3v_structural_scale_k1_emst COMMAND mhgp3v_structural_scale_check
         --points 200 --smax 11 --seed 20260810)
mhgp3v_add_expected_code_test_for(mhgp3v_structural_scale_check
  mhgp3v_structural_scale_kill_shift "REFUTATION" 1
  --points 100 --smax 11 --seed 7 --force-shift-fusion 1)
add_test(NAME mhgp3v_parallel_catalogue_differential COMMAND mhgp3v_parallel_catalogue_gate
         --points 120 --smax 11 --clouds 3 --seed 20260810)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_kill_drop_roots "mutant tue" 4
  --points 120 --smax 11 --clouds 3 --mutant drop-odd-roots)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_reject_unknown_mutant "mutant inconnu" 2
  --mutant inexistant)
# LE ROUTEUR MEDIAN : le differentiel contre l'enumeration, puis les quatre
# mutants de la note (glouton renomme mediane, seuil inclusif, stay structurel
# interdit, descente sur egalite 1/2--1/2) — chacun meurt par une fixture
# gravee, donc de facon deterministe, code 4 exige.
add_test(NAME mhgp3v_tree_median_differential COMMAND mhgp3v_tree_median_gate
         --trees 300 --nodes 24 --seed 20260810)
mhgp3v_add_expected_code_test_for(mhgp3v_tree_median_gate
  mhgp3v_tree_median_kill_greedy_plurality "mutant tue" 4
  --trees 50 --nodes 16 --mutant greedy-plurality)
mhgp3v_add_expected_code_test_for(mhgp3v_tree_median_gate
  mhgp3v_tree_median_kill_half_inclusive "mutant tue" 4
  --trees 50 --nodes 16 --mutant half-inclusive)
mhgp3v_add_expected_code_test_for(mhgp3v_tree_median_gate
  mhgp3v_tree_median_kill_forbid_zero_stay "mutant tue" 4
  --trees 50 --nodes 16 --mutant forbid-zero-stay)
mhgp3v_add_expected_code_test_for(mhgp3v_tree_median_gate
  mhgp3v_tree_median_kill_descend_on_tie "mutant tue" 4
  --trees 50 --nodes 16 --mutant descend-on-tie)
mhgp3v_add_expected_code_test_for(mhgp3v_tree_median_gate
  mhgp3v_tree_median_reject_unknown_mutant "mutant inconnu" 2
  --mutant inexistant)
# Sans CUDA, la qualification device refuse proprement — jamais un chemin
# device pretendu ; SOUS CUDA ce refus serait un faux rouge : les portes
# deviennent le nominal a masses reelles et le mutant drop-edge (label cuda,
# rejouables en session G4).
# Le mode chrono ne peut pas porter le mutant drop-edge : sans differentiel il
# survivrait par construction — la combinaison est refusee AVANT le #ifdef
# CUDA, donc cette porte vaut pour les deux builds.
mhgp3v_add_expected_code_test_for(mhgp3v_faceowner_device_qualification
  mhgp3v_faceowner_device_reject_timing_drop_edge "exige le differentiel" 2
  --points 10 --timing-only 1 --force-drop-edge 1)
if(NOT MHGP3V_ENABLE_CUDA)
  mhgp3v_add_expected_code_test_for(mhgp3v_faceowner_device_qualification
    mhgp3v_faceowner_device_reject_absent "kernel face-owner absent" 2 --points 10)
else()
  add_test(NAME mhgp3v_faceowner_device_nominal COMMAND
           mhgp3v_faceowner_device_qualification --points 64 --smax 11 --max-order 5)
  mhgp3v_add_expected_code_test_for(mhgp3v_faceowner_device_qualification
    mhgp3v_faceowner_device_kill_drop_edge "DIVERGENT" 1
    --points 64 --smax 11 --max-order 3 --force-drop-edge 1)
  set_tests_properties(mhgp3v_faceowner_device_nominal
    mhgp3v_faceowner_device_kill_drop_edge PROPERTIES LABELS cuda)
endif()
# LE MODE COMPARE-JOINS : meme catalogue, deux joins, transcript et digest
# compares in-process — le differentiel de l'audit au-dela des nuages du juge.
add_test(NAME mhgp3v_saturated_pipeline_compare_joins COMMAND mhgp3v_saturated_pipeline
         --points 32 --smax 11 --max-order 3 --seed 20260810 --compare-joins 1)
add_test(NAME mhgp3v_saturated_pipeline_compare_joins_global COMMAND mhgp3v_saturated_pipeline
         --points 32 --smax 11 --max-order 3 --seed 20260810 --join postings-global
         --threads 2 --compare-joins 1)
add_test(NAME mhgp3v_saturated_pipeline_compare_joins_faceowner COMMAND mhgp3v_saturated_pipeline
         --points 32 --smax 11 --max-order 3 --seed 20260810 --join faceowner
         --compare-joins 1)
# LA FAMILLE TERRAIN (regime type LiDAR : surfacique, sol a jitter fin donc
# coplanarites massives) passe par le MEME differentiel des joins ET par le
# differentiel du catalogue parallele — l'accord des formes est exige sur ce
# regime degenere, pas seulement sur la famille volumique uniforme.
add_test(NAME mhgp3v_saturated_pipeline_compare_joins_terrain COMMAND mhgp3v_saturated_pipeline
         --points 48 --smax 11 --max-order 3 --seed 20260810 --family terrain
         --compare-joins 1)
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_unknown_family "famille inconnue" 2
  --family martienne)
add_test(NAME mhgp3v_parallel_catalogue_terrain COMMAND mhgp3v_parallel_catalogue_gate
         --points 100 --smax 11 --clouds 2 --family terrain --seed 20260810)
# LE PREFLIGHT REFUSE UN BUDGET INSUFFISANT AVANT TOUTE EMISSION, et publie le
# manifeste du refus (P_post predit, pic conservateur, plus gros lot).
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_budget "preflight" 3
  --points 32 --smax 11 --max-order 3 --seed 20260810 --join postings
  --memory-budget-mb 1)
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_budget_global "preflight" 3
  --points 32 --smax 11 --max-order 3 --seed 20260810 --join postings-global
  --memory-budget-mb 1)
add_test(NAME mhgp3v_saturated_pipeline_compare_joins_hybrid COMMAND mhgp3v_saturated_pipeline
         --points 10 --smax 11 --max-order 3 --seed 20260810 --join hybrid
         --compare-joins 1)
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_hybrid_partial "famille complete" 2
  --points 32 --smax 11 --max-order 3 --join hybrid)
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_budget_faceowner "preflight" 3
  --points 32 --smax 11 --max-order 3 --seed 20260810 --join faceowner
  --memory-budget-mb 1)

# LE NOYAU F0 DU FOLD HORIZONTAL, EN PORTE PERMANENTE.
#
# Le script n'etait lance qu'a la main : rien n'empechait une regression de
# passer inapercue entre deux audits. Il est desormais un CTest, sous
# `python3 -O` en plus du mode normal — c'est precisement ce drapeau qui
# effacait les vingt-sept obligations portees par `assert` et rendait un `PASS`
# vide. Elles passent maintenant par un controle explicite.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_test(NAME mhgp3v_gate_d_fold_f0 COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_gate_d_fold_f0.py)
  set_tests_properties(mhgp3v_gate_d_fold_f0 PROPERTIES
                       PASS_REGULAR_EXPRESSION "Gate_D_F0_kernel=PASS")
  add_test(NAME mhgp3v_gate_d_fold_f0_optimised COMMAND ${Python3_EXECUTABLE} -O
           ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_gate_d_fold_f0.py)
  set_tests_properties(mhgp3v_gate_d_fold_f0_optimised PROPERTIES
                       PASS_REGULAR_EXPRESSION "Gate_D_F0_kernel=PASS")
else()
  message(STATUS "mhgp3v : interprete Python absent, porte F0 non enregistree")
endif()

# LE BALAYAGE EN DEMI-PLAN DU FILTRE DE PAIRES, EN PORTE PERMANENTE.
#
# Le probe n'avait aucun CTest : sa conclusion d'echelle a donc pu etre publiee
# alors que son oracle angulaire etait faux. Il en a maintenant un, avec ses
# PLANCHERS — sans `--min-true`, un catalogue vide rendait `vraies=0,
# refutees=0, OK`, c'est-a-dire vert precisement quand le juge etait mort.
#
# Les six fixtures du balayage tournent AVANT toute campagne : le contre-exemple
# entier de l'audit (exact 2 contre 5 par directions vives), la separation des
# trois contrats ouvert/ferme/vif sur 3-4-5, les antipodes, les rayons
# confondus, les points de la droite, et l'ordre `ouvert <= ferme <= vif` sur un
# nuage entier avec au moins un ecart strict.
#
# La porte couvre aussi le LEMME DE TRIPLE et les cliques : un support d'arite
# trois est un triangle du graphe admissible, un support d'arite quatre un K4.
# Le lemme de triple doit refuter ZERO triple reel — il est necessaire, ou il est
# faux.
add_test(NAME mhgp3v_admissible_pair_sweep COMMAND mhgp3v_admissible_pair_probe
         --points 30 --smax 8 --repeats 3 --seed 424242 --min-true 900 --min-admitted 1200
         --min-triangles 3000 --min-k4 20000 --min-triples-rejected 500 --min-true-arity4 200)
# LE MUTANT DE SELECTIVITE. `zero triple vrai refute` etait la seule obligation
# nouvelle : un filtre qui accepte TOUT la satisfait sans rien filtrer, et une
# regression qui n'enumere aucun triangle aussi. Le plancher de triples REJETES
# est ce qui les tue.
mhgp3v_add_expected_code_test_for(mhgp3v_admissible_pair_probe
  mhgp3v_admissible_pair_reject_accept_all "triples rejetes" 3
  --points 30 --smax 8 --repeats 3 --seed 424242 --min-triangles 3000 --min-k4 20000
  --min-triples-rejected 500 --min-true-arity4 200 --force-triple-accept 1)

# La REVERSE SEARCH est comparee au BFS dans les quatre portes positives : meme
# ensemble de sommets, memes coquilles, memes ensembles interieurs, sans `seen`,
# `frontier` ni `visited`. C'est la porte qui autorise a retirer les tables
# globales ; un parent legerement faux produirait un sous-arbre tronque que seule
# cette comparaison verrait.

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
         --min-navigated 1000 --min-multiple-shells 200
         --min-reverse 100000 --min-reverse-depth 12 --min-internal-nodes 2000 --min-owner 50000)

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
         --min-closed 800 --min-empty 400 --min-internal-nodes 80 --min-attachments 20)
add_test(NAME mhgp3v_first_incidence_high_order COMMAND mhgp3v_first_incidence
         --clouds 12 --points 9 --coord 3 --k 4 --seed 13
         --min-closed 400 --min-empty 80 --min-internal-nodes 20)
# Une campagne DANS le domaine regulier : zero egalite exterieure (comptee pour
# TOUT point exterieur sur la coquille, intrus present ou non), zero support
# essentiel ambigu tous cardinaux confondus, zero point de F sur la coquille hors
# du support, zero panne de primitive. C'est la seule ou les candidats a une
# attache sont des attaches AUTORISEES — et la cible brute y echoue quand meme.
#
# Les planchers de DESCENTE sont indispensables : sans un plancher d'ETAPES et un
# plancher sur les DEUX branches terminales, supprimer le bloc de descente laisse
# les CTests verts.
add_test(NAME mhgp3v_first_incidence_regular COMMAND mhgp3v_first_incidence
         --clouds 12 --points 14 --coord 4000 --k 3 --seed 11
         --require-regular --min-attachments 15 --min-closed 400 --min-empty 200
         --min-descent 20 --min-descent-steps 3 --min-terminal-empty 5
         --min-terminal-single 10)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_irregular "--require-regular exige zero egalite" 3
  --clouds 12 --points 20 --coord 20 --k 3 --seed 11 --require-regular)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_absurd "campagne absurde" 2 --clouds 0)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_unknown "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_suffix "valeur entiere invalide" 2 --clouds 1junk)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_floor "plancher de couverture non atteint" 3
  --clouds 2 --points 8 --coord 20 --k 3 --min-closed 1000000)
mhgp3v_add_expected_code_test_for(mhgp3v_first_incidence
  mhgp3v_first_incidence_reject_descent_floor "plancher de descente non atteint" 3
  --clouds 2 --points 8 --coord 20 --k 3 --min-descent-steps 1000000)

# LE LOT DU FRONT D'ONDE, qualifie sans GPU. Le corps evalue par le noyau borne doit
# rendre exactement les verdicts du chemin CPU non borne, sommet par sommet et bit
# par bit. C'est la porte qui rend la session GPU courte : elle n'aura plus qu'a
# montrer que le MEME corps donne les MEMES bits sur sm_120.
#
# LES PLANCHERS SONT SEPARES, ET C'EST LA CORRECTION DU P0.
#
# Un plancher agrege se satisfait d'un seul chiffre. La porte precedente n'avait
# que `--min-refused`, qui prouvait que la BRANCHE de refus avait ete prise, et
# jamais que son resultat avait ete conserve : un mutant refusant TOUS les
# sommets rendait zero flat, zero couple, zero desaccord et `OK`.
add_test(NAME mhgp3v_device_wavefront_host COMMAND mhgp3v_device_wavefront
         --clouds 6 --points 24 --coord 4000 --smax 8 --seed 20260809
         --min-clouds 6 --min-accepted 15000 --min-flats 60000 --min-couples 40000
         --min-kernels 6)
add_test(NAME mhgp3v_device_wavefront_degenerate COMMAND mhgp3v_device_wavefront
         --clouds 6 --points 14 --coord 5 --smax 8 --seed 31337
         --min-clouds 6 --min-accepted 3000 --min-flats 12000 --min-couples 7000
         --min-kernels 6)
# Le chemin de REFUS doit etre exerce ET REJOUE. Le plancher de rejeu est le
# nouveau : sans lui, un refus reste une statistique et non un jugement.
add_test(NAME mhgp3v_device_wavefront_refusal COMMAND mhgp3v_device_wavefront
         --clouds 4 --points 20 --coord 3 --smax 12 --seed 11
         --min-refused 10 --min-replayed 10 --min-clouds 4 --min-accepted 2000
         --min-flats 12000 --min-couples 5000 --min-kernels 4)
mhgp3v_add_expected_code_test_for(mhgp3v_device_wavefront
  mhgp3v_device_wavefront_reject_absurd "campagne absurde" 2 --points 2)
# LE MUTANT VIT DANS LE BINAIRE, ET IL DOIT ROUGIR. `--force-refuse-all` refuse
# chaque sommet a l'admission : le ledger balance encore et la masse est encore
# conservee — rejouer TOUT est correct, simplement inutile — mais les planchers
# d'acceptation, de flats et de couples tombent a zero.
mhgp3v_add_expected_code_test_for(mhgp3v_device_wavefront
  mhgp3v_device_wavefront_reject_all_refused "sommets acceptes" 3
  --clouds 4 --points 20 --coord 3 --smax 12 --seed 11 --force-refuse-all 1
  --min-refused 10 --min-replayed 10 --min-clouds 4 --min-accepted 2000
  --min-flats 12000 --min-couples 5000 --min-kernels 4)

# Interface fail-closed du juge lui-meme.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_unknown_argument "argument inconnu" 2 --clouds 1 --sujet-inexistant)
# Chaque option porte desormais sa plage semantique, verifiee AVANT le cast :
# le diagnostic est celui de l'option fautive, pas un « campagne absurde » global.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_smax_below_contract "valeur entiere invalide" 2
  --clouds 1 --smax 1)
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_empty_campaign "plancher non atteint" 3 --clouds 0 --min-cases 100000)
# `atoi` acceptait « 0junk » et rendait zero : la campagne devenait vide et verte.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_integer_suffix "valeur entiere invalide" 2 --clouds 0junk --min-cases 1)
# La grille declaree est u16 ; hors d'elle les bornes des predicats ne tiennent plus.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_coord_outside_grid "valeur entiere invalide" 2
  --clouds 1 --points 11 --coord 2147483647 --smax 2 --min-cases 1)
# Une campagne qui ne navigue jamais ne peut pas rendre OK.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_no_navigation "plancher de couverture non atteint" 3
  --clouds 0 --min-cases 1 --min-navigated 100000)
# Le token etait lu en entier, mais le `long long` etait ensuite tronque en
# `int` sans controle de plage : `--clouds 4294967296` rendait zero nuage.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_clouds_narrowing "valeur entiere invalide" 2
  --clouds 4294967296 --min-cases 1)
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_coord_narrowing "valeur entiere invalide" 2
  --clouds 0 --coord 4295032832 --min-cases 1)
# Neuf points DISTINCTS dans huit positions : la demande est impossible et ne
# doit pas etre censuree en silence par le tirage.
mhgp3v_add_expected_code_test_for(mhgp3v_flats_differential
  mhgp3v_flats_reject_impossible_campaign "campagne impossible" 2
  --clouds 1 --points 9 --coord 2 --smax 2 --min-cases 1)

# Interface fail-closed : token entier partiel, reçu non ecrit et option ignoree
# doivent tous fournir un code non nul ET le diagnostic attendu.
mhgp3v_add_expected_code_test_for(mhgp3v_oracle mhgp3v_cli_reject_integer_suffix
  "valeur entiere invalide" 2 --clouds 1junk)
mhgp3v_add_expected_code_test_for(mhgp3v_oracle mhgp3v_receipt_write_failure
  "ecriture du recu incomplete" 1
  --fixture foreign_source_same_level --subject v2 --clouds 1 --min-points 4
  --max-points 4 --max-order 1 --coord-max 2 --min-decided 1 --min-nodes 1
  --receipt /dev/full)
mhgp3v_add_expected_code_test_for(mhgp3v_oracle mhgp3v_measure_receipt_rejected
  "measure-only ne prend encore ni --receipt ni --inject" 2
  --subject anchored --measure-only --regime exhaustive --points 4 --clouds 1
  --max-order 1 --receipt ignored.json)
