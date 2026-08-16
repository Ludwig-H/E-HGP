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



# LA SELECTIVITE DU LEMME DU DEMI-BOULE. Le budget du generateur direct depend d'une
# quantite que sa propre specification declare NON BORNEE : la taille de l'univers
# admissible. Ce probe la mesure, et verifie que le lemme ne refute aucune paire reelle.
add_executable(mhgp3v_admissible_pair_probe prototype/admissible_pair_probe.cpp)
target_include_directories(mhgp3v_admissible_pair_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_admissible_pair_probe PRIVATE mhgp)
target_compile_options(mhgp3v_admissible_pair_probe PRIVATE -Wall -Wextra -Werror)


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

# LA PORTE DU CATALOGUE PARALLELE : le MULTIENSEMBLE COMPLET des sommets emis
# (coquille, interieur, niveau) == reverse_search_stream sequentiel, avec
# MULTIPLICITE UN (identite couronne+sous-arbres), puis le catalogue canonique
# (membres tries + support + rang), travail invariant entre threads et
# threads+1 (insensible a l'horloge). Mutants : drop-odd-roots meurt par le
# multiensemble, duplicate-root par la multiplicite. La porte tourne
# MULTITHREAD (>= 4) : la data race des compteurs mutable du CertifiedIndex
# partage est reparee (metriques par worker, arbre immuable en requete).
add_executable(mhgp3v_parallel_catalogue_gate prototype/parallel_catalogue_gate.cpp)
target_include_directories(mhgp3v_parallel_catalogue_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_parallel_catalogue_gate PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_parallel_catalogue_gate PRIVATE -Wall -Wextra -Werror)
# ThreadSanitizer sur le chemin CPU multithread (OFF par defaut). Incompatible
# CUDA, comme MORSEHGP3D_ENABLE_SANITIZERS cote produit. Portee : la cible de
# la porte parallele — le seul chemin multithread que cette porte recoit.
option(MHGP3V_ENABLE_TSAN "Instrumenter la porte parallele avec ThreadSanitizer" OFF)
if(MHGP3V_ENABLE_TSAN)
  if(MHGP3V_ENABLE_CUDA)
    message(FATAL_ERROR "MHGP3V_ENABLE_TSAN est incompatible avec MHGP3V_ENABLE_CUDA")
  endif()
  target_compile_options(mhgp3v_parallel_catalogue_gate PRIVATE -fsanitize=thread -g)
  target_link_options(mhgp3v_parallel_catalogue_gate PRIVATE -fsanitize=thread)
endif()

# LA PORTE DU ROUTEUR MEDIAN (note dendrogramme, Th. 5.1 et §14) : la descente
# a majorite stricte == l'enumeration de TOUS les terminaux (perte minimale ET
# noeud le plus haut du segment de medianes), fixtures 5/6/7 de la note gravees,
# quatre mutants nommes de la note tues. Combinatoire pure : aucun lien mhgp.
add_executable(mhgp3v_tree_median_gate prototype/tree_median_gate.cpp)
target_include_directories(mhgp3v_tree_median_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(mhgp3v_tree_median_gate PRIVATE -Wall -Wextra -Werror)



# LA FACTORY ValidatedHybridSidecar (contrat 20260810, porte 2 de l'etat
# courant) : frontiere de confiance typee — la fermeture des carriers est une
# capability construite et liee par digests, jamais un booleen CLI.
add_executable(mhgp3v_sidecar_factory_gate prototype/sidecar_factory_gate.cpp)
target_include_directories(mhgp3v_sidecar_factory_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_sidecar_factory_gate PRIVATE mhgp)
target_compile_options(mhgp3v_sidecar_factory_gate PRIVATE -Wall -Wextra -Werror)

# LA SONDE SELF-JOIN q2 DES PAIRES (PROPOSITION §6.2 ; l'ancien nom « P1a »
# designait le center-cover retire) : machine
# de blocs, lane q2 par temoins d'antichaine, identite du ledger fermee.
add_executable(mhgp3v_pair_selfjoin_probe prototype/pair_selfjoin_probe.cpp)
target_include_directories(mhgp3v_pair_selfjoin_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_pair_selfjoin_probe PRIVATE mhgp)
target_compile_options(mhgp3v_pair_selfjoin_probe PRIVATE -Wall -Wextra -Werror)

# LE FALSIFICATEUR D'ANCRES q3/q4 par coeur universel de Jung (porte en cinq
# points de NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4) : count-only, oracle
# exhaustif n<=32, fate par lane.
add_executable(mhgp3v_pair_anchor_probe prototype/pair_anchor_probe.cpp)
target_include_directories(mhgp3v_pair_anchor_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_pair_anchor_probe PRIVATE mhgp)
target_compile_options(mhgp3v_pair_anchor_probe PRIVATE -Wall -Wextra -Werror)

# LE DOUBLE SANITIZER DU NOYAU ANGULAIRE (audit etat courant, profondeur
# durcissement 2 : « ajouter une fixture extreme non colineaire et un build
# sanitizer cible ») : la meme source du falsificateur d'ancres, compilee
# ASan+UBSan sans recouvrement — le debordement signe (i64 comme __int128)
# y est fatal, pas silencieux. Cible CPU seule ; les sanitizers restent
# incompatibles CUDA, comme MHGP3V_ENABLE_TSAN et le produit.
if(NOT MHGP3V_ENABLE_CUDA)
  add_executable(mhgp3v_pair_anchor_probe_san prototype/pair_anchor_probe.cpp)
  target_include_directories(mhgp3v_pair_anchor_probe_san PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(mhgp3v_pair_anchor_probe_san PRIVATE mhgp)
  target_compile_options(mhgp3v_pair_anchor_probe_san PRIVATE -Wall -Wextra -Werror
    -fsanitize=address,undefined -fno-sanitize-recover=all -g)
  target_link_options(mhgp3v_pair_anchor_probe_san PRIVATE
    -fsanitize=address,undefined)
endif()

# LA PORTE DIRECTE DU GENERATEUR PARTAGE (audit etat courant, porte 1) : le
# contrat « exactement n, ou refus ferme » sur make_family_cloud nu, quatre
# familles, contre-exemple 12500/707 rejoue, insuffisance visible.
add_executable(mhgp3v_cloud_family_gate prototype/cloud_family_gate.cpp)
target_include_directories(mhgp3v_cloud_family_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_cloud_family_gate PRIVATE mhgp)
target_compile_options(mhgp3v_cloud_family_gate PRIVATE -Wall -Wextra -Werror)

# LA PORTE DU DEGRE GABRIEL (audit degre/kissing/smax=11 du 11/08) : deux
# fixtures u16 gravees a treize voisins, rangs fermes exacts 2 et 11 — la
# refutation executable du cap 12 et de toute ligne CSR a largeur fixe.
add_executable(mhgp3v_gabriel_degree_gate prototype/gabriel_degree_gate.cpp)
target_include_directories(mhgp3v_gabriel_degree_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_gabriel_degree_gate PRIVATE mhgp)
target_compile_options(mhgp3v_gabriel_degree_gate PRIVATE -Wall -Wextra -Werror)

# LA SOURCE q2 CANDIDATE PRODUIT (NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16,
# ordre d'implementation points 2--3 de l'audit etat courant) : Morton/LBVH
# radix, coupe Yao48 stricte fail-open en banques 48x10, classification
# terminale, census ferme, ledger complet et juge quadratique independant.
add_executable(mhgp3v_pair_yao48_source prototype/pair_yao48_source.cpp)
target_include_directories(mhgp3v_pair_yao48_source PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_pair_yao48_source PRIVATE mhgp)
target_compile_options(mhgp3v_pair_yao48_source PRIVATE -Wall -Wextra -Werror)

# LA LANE k=1 (PROPOSITION §6.1) : l'EMST exact par Boruvka sur le LBVH
# partage — niveaux 4*niveau = d^2 exacts en i64, departage canonique
# (d^2, min id, max id), purete par noeud recalculee a chaque ronde. Le
# probe porte le juge Prim O(n^2) independant (multiensemble des niveaux +
# partitions canoniques a chaque coupe stricte et fermee), quatre fixtures
# gravees verifiees hors bande, le replay canonique multi-feuilles et
# l'equivariance par permutation.
add_executable(mhgp3v_emst_boruvka_probe prototype/emst_boruvka_probe.cpp)
target_include_directories(mhgp3v_emst_boruvka_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_emst_boruvka_probe PRIVATE mhgp)
target_compile_options(mhgp3v_emst_boruvka_probe PRIVATE -Wall -Wextra -Werror)

# LE FALSIFICATEUR DE MASSE P1a (NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY,
# note AUDITEE) : center-cover q4 au seuil huit, 64 patchs rationnels a
# l'echelle seize, microtuile AVANT cover, ledger transactionnel et juge
# determinantal independant.
add_executable(mhgp3v_center_cover_mass_probe prototype/center_cover_mass_probe.cpp)
target_include_directories(mhgp3v_center_cover_mass_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_center_cover_mass_probe PRIVATE mhgp)
target_compile_options(mhgp3v_center_cover_mass_probe PRIVATE -Wall -Wextra -Werror)

# LE HARNAIS DIAGNOSTIQUE NOMME warm_e2e_h0_v3_diagnostic : LBVH + EMST k=1 +
# q2 shardee, nuage frais par repetition, ledger rejoue, p50/p95 par phase.
# Il DIT ce qu'il ne couvre pas et ne ferme aucun SLO officiel.
add_executable(mhgp3v_warm_e2e_h0_diagnostic prototype/warm_e2e_h0_diagnostic.cpp)
target_include_directories(mhgp3v_warm_e2e_h0_diagnostic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_warm_e2e_h0_diagnostic PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_warm_e2e_h0_diagnostic PRIVATE -Wall -Wextra -Werror)



option(MHGP3V_ENABLE_CUDA "Compiler le kernel de front d'onde pour sm_120" OFF)
if(MHGP3V_ENABLE_CUDA)
  enable_language(CUDA)
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120-real)
  endif()
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
  message(STATUS "mhgp3v : kernel de front d'onde active pour ${CMAKE_CUDA_ARCHITECTURES}")
  # SOURCE PAR ANCRE MAXIMALE, PARITE HOTE/DEVICE.
  # Le noyau n'implemente aucune geometrie : il appelle `run_anchor_point` de
  # `prototype/anchor_pipeline.hpp`, exactement la fonction que l'hote compile.
  add_executable(mhgp3v_anchor_device
    prototype/anchor_source_device_qualification.cpp
    prototype/anchor_source_kernel.cu)
  target_include_directories(mhgp3v_anchor_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(mhgp3v_anchor_device PRIVATE mhgp Threads::Threads)
  target_compile_options(mhgp3v_anchor_device PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>)
  set_target_properties(mhgp3v_anchor_device PROPERTIES
    CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_EXTENSIONS OFF
    CUDA_SEPARABLE_COMPILATION OFF)
  # Le chemin est entier : la discipline flottante ne sert qu'a garder
  # l'estimation initiale de la racine entiere reproductible d'un hote a l'autre.
  target_compile_options(mhgp3v_anchor_device PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false;--ftz=false;--prec-div=true;--prec-sqrt=true>)
  add_test(NAME mhgp3v_anchor_device_parite COMMAND mhgp3v_anchor_device
           --points=4000 --family=uniform --seed=1 --threads=4 --slots=8192)
  set_tests_properties(mhgp3v_anchor_device_parite PROPERTIES
    PASS_REGULAR_EXPRESSION "parite supports=OUI compteurs=OUI")
  add_test(NAME mhgp3v_anchor_device_parite_terrain COMMAND mhgp3v_anchor_device
           --points=4000 --family=terrain --seed=3 --threads=4 --slots=8192)
  set_tests_properties(mhgp3v_anchor_device_parite_terrain PROPERTIES
    PASS_REGULAR_EXPRESSION "parite supports=OUI compteurs=OUI")
  message(STATUS "mhgp3v : source par ancre device active pour ${CMAKE_CUDA_ARCHITECTURES}")
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
# theoremes 1--2 du predicat q_min (enonce intemporel, l'unique autorite est
# la porte elle-meme) sont RECUS contre la
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
# LES MUTANTS DU FALLBACK PREFIXE--PREFIXE (note index, portes §Recu) : des
# prefixes trop courts perdent une paire aux communs tardifs ; le dernier
# point de prefixe non indexe perd la paire qui ne se voyait que par lui ; les
# candidats crus sur-fusionnent ; la projection DSU avant recertification
# certifie a tort des membres issus de plusieurs generateurs d'une racine.
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_length "mutant tue" 4
  --hybrid-mutant prefix-length-minus-one)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_drop_last "mutant tue" 4
  --hybrid-mutant drop-last-posting)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_skip_recert "mutant tue" 4
  --hybrid-mutant skip-recertification)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_project_root "mutant tue" 4
  --hybrid-mutant project-root-first)
# LES MUTANTS DE POSSESSION ET DE CALENDRIER au niveau du FOLD REEL (audit
# 8df7ac8 + reponse Q2/Q3) : un ordre par requete perd des paires ; les lots
# futurs stages d'avance unissent trop tot ; le staging sequentiel casse le
# preflight fige a la cloture du lot ; la possession desactivee duplique la
# paire dans le ledger pre-DSU ; le posting duplique casse l'identite L(r,K).
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_order_per_query "mutant tue" 4
  --hybrid-mutant order-per-query)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_future_visible "mutant tue" 4
  --hybrid-mutant future-visible)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_stage_sequentially "mutant tue" 4
  --hybrid-mutant stage-query-sequentially)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_double_query "mutant tue" 4
  --hybrid-mutant double-query-pair)
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_prefix_kill_duplicate_posting "mutant tue" 4
  --hybrid-mutant duplicate-posting)
# LA GARDE STRICTE PRE-LOT du fast path (reponse 20260811 §5) : un carrier au
# niveau egal est une contradiction de sidecar — le mutant qui force le
# niveau egal doit mourir par le refus atomique, jamais etre route.
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_kill_equal_level_lookup "mutant tue" 4
  --hybrid-mutant equal-level-lookup)
# LA FENETRE D'EVENEMENT q <= k+1 du fast multi-lot : le defaut du premier
# delta live (q=k+2 attache zero face), grave en mutant permanent.
mhgp3v_add_expected_code_test_for(mhgp3v_postings_join_gate
  mhgp3v_hybrid_kill_fast_window_off "mutant tue" 4
  --hybrid-mutant fast-window-off)
# LA SONDE SELF-JOIN q2 (portes 4-5 de l'audit delta cbac109) : ledger de fate par
# paire (partition, multiplicite un, inclusion non compensable des non-inertes
# dans les microtuiles), planchers anti vert-par-vacuite, fixtures gravees et
# sept mutants a code exact (partition croisee, interne, dernier bloc, seuil,
# contact, duplication compensee, overshoot du generateur) ; CLI hostile
# refusee.
add_test(NAME mhgp3v_pair_selfjoin_q2_terrain COMMAND mhgp3v_pair_selfjoin_probe
         --points 400 --seed 20260810 --family terrain --verify-bruteforce 1
         --min-pruned-pairs 40000 --min-states 1000)
set_tests_properties(mhgp3v_pair_selfjoin_q2_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION "ledger C\\(n,2\\)=79800 FERME")
add_test(NAME mhgp3v_pair_selfjoin_q2_scanline COMMAND mhgp3v_pair_selfjoin_probe
         --points 400 --seed 20260810 --family scanline_overlap_multiecho
         --verify-bruteforce 1)
set_tests_properties(mhgp3v_pair_selfjoin_q2_scanline PROPERTIES
  PASS_REGULAR_EXPRESSION "FERME")
# Le double a code exact exige par l'audit etat courant : la regex seule peut
# ignorer un code non nul apres avoir vu FERME.
add_test(NAME mhgp3v_pair_selfjoin_q2_scanline_code COMMAND mhgp3v_pair_selfjoin_probe
         --points 400 --seed 20260810 --family scanline_overlap_multiecho
         --verify-bruteforce 1)
# Doubles a code exact des regex : un add_test SANS regex exige le code 0
# nativement (expect_failure.cmake refuse par construction les codes nuls).
add_test(NAME mhgp3v_pair_selfjoin_terrain_code COMMAND mhgp3v_pair_selfjoin_probe
         --points 400 --seed 20260810 --family terrain --verify-bruteforce 1
         --min-pruned-pairs 40000 --min-states 1000)
# Les fixtures gravees : regex sur la ligne OK + double a code 0.
add_test(NAME mhgp3v_pair_selfjoin_fixture_contact COMMAND
         mhgp3v_pair_selfjoin_probe --fixture contact)
set_tests_properties(mhgp3v_pair_selfjoin_fixture_contact PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture contact recue")
add_test(NAME mhgp3v_pair_selfjoin_fixture_contact_code COMMAND
         mhgp3v_pair_selfjoin_probe --fixture contact)
add_test(NAME mhgp3v_pair_selfjoin_fixture_tenth COMMAND
         mhgp3v_pair_selfjoin_probe --fixture tenth-witness)
set_tests_properties(mhgp3v_pair_selfjoin_fixture_tenth PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture tenth-witness recue")
add_test(NAME mhgp3v_pair_selfjoin_fixture_tenth_code COMMAND
         mhgp3v_pair_selfjoin_probe --fixture tenth-witness)
add_test(NAME mhgp3v_pair_selfjoin_fixture_duplicate COMMAND
         mhgp3v_pair_selfjoin_probe --fixture duplicate)
set_tests_properties(mhgp3v_pair_selfjoin_fixture_duplicate PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture duplicate recue")
add_test(NAME mhgp3v_pair_selfjoin_fixture_duplicate_code COMMAND
         mhgp3v_pair_selfjoin_probe --fixture duplicate)
add_test(NAME mhgp3v_pair_selfjoin_fixture_scope COMMAND
         mhgp3v_pair_selfjoin_probe --fixture q2-vs-q3-scope)
set_tests_properties(mhgp3v_pair_selfjoin_fixture_scope PROPERTIES
  PASS_REGULAR_EXPRESSION "ancres superieures")
add_test(NAME mhgp3v_pair_selfjoin_fixture_scope_code COMMAND
         mhgp3v_pair_selfjoin_probe --fixture q2-vs-q3-scope)
# Les mutants : partition croisee, partition interne, dernier bloc (identite
# du ledger), dixieme temoin et contact (ledger de fate par paire — un compte
# agrege les aurait compenses).
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_skip_half_block "mutant tue" 4 --points 400
  --seed 20260810 --family terrain --inject skip-half-block)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_drop_rr "mutant tue" 4 --points 400
  --seed 20260810 --family terrain --inject drop-rr)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_drop_last_microtile "mutant tue" 4 --points 400
  --seed 20260810 --family terrain --inject drop-last-microtile)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_threshold_nine "mutant tue" 4
  --fixture tenth-witness --inject threshold-nine)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_count_shell "mutant tue" 4
  --fixture contact --inject count-shell)
# LA RECEPTION L4/HERITAGE (porte 1 de l'audit etat courant) : differentiel
# bi-mode — la baseline recue de 40050c4 (ni L4 ni heritage, sans les
# injections du sujet) contre le mode optimise, egalite de TOUS les sorts et
# masses exigee ; plancher 9+1 pour armer le mutant du dixieme recherche ;
# cinq mutants cibles ; fixture aux extremes u16 (magnitudes maximales).
add_test(NAME mhgp3v_pair_selfjoin_differential_terrain COMMAND
         mhgp3v_pair_selfjoin_probe --points 400 --seed 20260810 --family terrain
         --differential 1 --min-nine-plus-one 10)
set_tests_properties(mhgp3v_pair_selfjoin_differential_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION "sorts et masses IDENTIQUES")
add_test(NAME mhgp3v_pair_selfjoin_differential_terrain_code COMMAND
         mhgp3v_pair_selfjoin_probe --points 400 --seed 20260810 --family terrain
         --differential 1 --min-nine-plus-one 10)
# uniform a n=1000 : a 400 la part de microtuiles (62 %) declenche le NO-GO
# de parcimonie, orthogonal au differentiel — 1000 reste sous les 50 %.
add_test(NAME mhgp3v_pair_selfjoin_differential_uniform_code COMMAND
         mhgp3v_pair_selfjoin_probe --points 1000 --seed 20260810 --family uniform
         --differential 1 --min-nine-plus-one 10)
foreach(l4_mutant l4-sign inherit-shift inherit-double-count inherit-stale-sibling
        stop-at-inherited)
  mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
    mhgp3v_pair_selfjoin_kill_${l4_mutant} "mutant tue" 4 --points 400
    --seed 20260810 --family terrain --differential 1 --inject ${l4_mutant})
endforeach()
add_test(NAME mhgp3v_pair_selfjoin_fixture_u16_extremes COMMAND
         mhgp3v_pair_selfjoin_probe --fixture u16-extremes)
set_tests_properties(mhgp3v_pair_selfjoin_fixture_u16_extremes PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture u16-extremes recue")
add_test(NAME mhgp3v_pair_selfjoin_fixture_u16_extremes_code COMMAND
         mhgp3v_pair_selfjoin_probe --fixture u16-extremes)
# LE CONTRAT DE CARDINALITE du generateur (contre-exemple multi-echo de
# l'audit etat courant) : a n=1100 le mutant qui retablit l'ancienne garde
# avant-pixel rend 1101 points — le driver exige l'egalite et le tue ; le
# nominal a exactement 1100 points est la regression gravee.
add_test(NAME mhgp3v_pair_selfjoin_cardinality_exact COMMAND
         mhgp3v_pair_selfjoin_probe --points 1100 --seed 20260810
         --family scanline_overlap_multiecho)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_generator_overshoot "mutant tue" 4 --points 1100
  --seed 20260810 --family scanline_overlap_multiecho
  --inject generator-overshoot)
# Le mutant COMPENSE : duplication d'un fils croise + omission de l'autre a
# masses egales — l'identite agregee FERME, seul le sort par paire mord.
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_kill_duplicate_compensated "mutant tue" 4 --points 400
  --seed 20260810 --family terrain --verify-bruteforce 1
  --inject duplicate-compensated)
# Budget-1 : la sonde refuse (code 3), elle ne tronque pas — et la frontiere
# exacte du budget est gravee : 1846 etats passent, 1845 refusent.
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_budget_refuses "budget d'etats depasse" 3 --points 400
  --seed 20260810 --family terrain --max-states 1)
add_test(NAME mhgp3v_pair_selfjoin_budget_exact_pass COMMAND
         mhgp3v_pair_selfjoin_probe --points 400 --seed 20260810 --family terrain
         --max-states 1846)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_budget_minus_one "budget d'etats depasse" 3 --points 400
  --seed 20260810 --family terrain --max-states 1845)
# Plancher viole : une sonde sous le plancher echoue a code 1.
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_floor_bites "plancher" 1 --points 400
  --seed 20260810 --family terrain --min-pruned-pairs 79800)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_selfjoin_probe
  mhgp3v_pair_selfjoin_reject_unknown "argument inconnu" 2 --inexistant 1)

# LE FALSIFICATEUR D'ANCRES q3/q4 : fixtures gravees (code 0 + regex), oracle
# exhaustif sur un petit nuage, mutants a code exact. Deux mutants sont
# MORT-NES et documentes dans le source, sans porte tueuse possible : « plage
# recouvrant une extremite » (g = 0 a l'extremite, le prédicat strict la
# refuse deja) et « oracle-accept-nonpositive » (toute paire prunee place, par
# monotonie axiale et la borne de Jung sur la miniboule d'un tuple d'arete
# maximale prunee, ses 9/8 temoins strictement dans cette miniboule — le juge
# elargi aux tuples non propres ne peut jamais mordre ; preuve au champ de
# l'injection).
foreach(anchor_fixture thin-acute contact-q3 contact-q4 nine-axial eight-axial
        seven-axial q4-tetra depth-rays scope-depth q4-colocated
        q4-rational-equality u16-extremes-anchor diametral-contact
        confounded-arc u16-extremes-skew)
  add_test(NAME mhgp3v_pair_anchor_fixture_${anchor_fixture} COMMAND
           mhgp3v_pair_anchor_probe --fixture ${anchor_fixture})
  set_tests_properties(mhgp3v_pair_anchor_fixture_${anchor_fixture} PROPERTIES
    PASS_REGULAR_EXPRESSION "OK : fixture ${anchor_fixture} recue")
  add_test(NAME mhgp3v_pair_anchor_fixture_${anchor_fixture}_code COMMAND
           mhgp3v_pair_anchor_probe --fixture ${anchor_fixture})
endforeach()
add_test(NAME mhgp3v_pair_anchor_oracle_uniform COMMAND mhgp3v_pair_anchor_probe
         --points 24 --seed 20260810 --family uniform --oracle 1)
set_tests_properties(mhgp3v_pair_anchor_oracle_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION "aucun desaccord")
add_test(NAME mhgp3v_pair_anchor_oracle_uniform_code COMMAND mhgp3v_pair_anchor_probe
         --points 24 --seed 20260810 --family uniform --oracle 1)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_gt_to_ge "mutant tue" 4 --fixture contact-q3
  --inject gt-to-ge)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_gt_to_ge_q4 "mutant tue" 4 --fixture contact-q4
  --inject gt-to-ge)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_threshold_q3 "mutant tue" 4 --fixture eight-axial
  --inject threshold-minus-one)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_threshold_q4 "mutant tue" 4 --fixture seven-axial
  --inject threshold-minus-one)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_witness_duplicated "mutant tue" 4 --fixture eight-axial
  --inject witness-duplicated)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_last_block "mutant tue" 4 --points 400
  --seed 20260810 --family terrain --inject last-block-omitted)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_duplicate_compensated "mutant tue" 4 --points 28
  --seed 20260810 --family uniform --oracle 1 --inject duplicate-compensated)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_anchor_non_maximal "mutant tue" 4 --fixture thin-acute
  --inject anchor-non-maximal)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_depth_open_boundary "mutant tue" 4 --fixture depth-rays
  --inject depth-open-boundary)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_depth_always_dropped "mutant tue" 4 --fixture depth-rays
  --inject depth-always-dropped)
add_test(NAME mhgp3v_pair_anchor_oracle_depth_code COMMAND mhgp3v_pair_anchor_probe
         --points 24 --seed 20260810 --family uniform --oracle 1 --mode depth)
add_test(NAME mhgp3v_pair_anchor_oracle_combined_code COMMAND mhgp3v_pair_anchor_probe
         --points 24 --seed 20260810 --family uniform --oracle 1 --mode combined)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_reject_unknown_mode "mode inconnu" 2 --mode inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_reject_unknown "argument inconnu" 2 --inexistant 1)
# LA CAMPAGNE DE BLOCS NON VACUE (audit etat courant, coeur point 5 et porte
# 1) : terrain n=32 graine 13 feuilles 2 — l'oracle rejoue des certificats de
# BLOC dans chaque lane (2 en q3, 1 en q4) et les planchers separes coins/
# terminaux refusent le vert-par-vacuite. La campagne uniform n=24, elle,
# n'a AUCUN bloc : le meme plancher y refuse a code 3, gravure de la vacuite.
add_test(NAME mhgp3v_pair_anchor_blocks_terrain32 COMMAND mhgp3v_pair_anchor_probe
         --points 32 --seed 13 --family terrain --leaf-size 2 --oracle 1
         --min-block-prunes 1 --min-corner-accepts 1 --min-terminal-prunes 1)
set_tests_properties(mhgp3v_pair_anchor_blocks_terrain32 PROPERTIES
  PASS_REGULAR_EXPRESSION "aucun desaccord")
add_test(NAME mhgp3v_pair_anchor_blocks_terrain32_code COMMAND mhgp3v_pair_anchor_probe
         --points 32 --seed 13 --family terrain --leaf-size 2 --oracle 1
         --min-block-prunes 1 --min-corner-accepts 1 --min-terminal-prunes 1)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_floor_bites_uniform24 "plancher viole" 3 --points 24
  --seed 20260810 --family uniform --oracle 1 --min-block-prunes 1)
# LES DECISIONS DE PROFONDEUR ARMEES (audit, profondeur point 1) : les
# fixtures axiales en mode depth forment de vrais certificats delta>=seuil ;
# le mutant qui debranche le prune meurt ici.
add_test(NAME mhgp3v_pair_anchor_nine_axial_depth COMMAND mhgp3v_pair_anchor_probe
         --fixture nine-axial --mode depth --lane q3)
set_tests_properties(mhgp3v_pair_anchor_nine_axial_depth PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture nine-axial recue")
add_test(NAME mhgp3v_pair_anchor_nine_axial_depth_code COMMAND mhgp3v_pair_anchor_probe
         --fixture nine-axial --mode depth --lane q3)
add_test(NAME mhgp3v_pair_anchor_eight_axial_depth COMMAND mhgp3v_pair_anchor_probe
         --fixture eight-axial --mode depth --lane q4)
set_tests_properties(mhgp3v_pair_anchor_eight_axial_depth PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : fixture eight-axial recue")
add_test(NAME mhgp3v_pair_anchor_eight_axial_depth_code COMMAND mhgp3v_pair_anchor_probe
         --fixture eight-axial --mode depth --lane q4)
add_test(NAME mhgp3v_pair_anchor_u16_extremes_depth_code COMMAND mhgp3v_pair_anchor_probe
         --fixture u16-extremes-anchor --mode depth --lane q3)
# LA FIXTURE EXTREME NON COLINEAIRE (audit etat courant, profondeur
# durcissement 2) : neuf paires antipodales exactes aux magnitudes u16
# maximales, delta = 9 derive a la main et rejoue par le juge n <= 32 —
# grands w1/w2, cross et dot non nuls, la ou u16-extremes-anchor n'exercait
# que des projections nulles. Le double a code exact accompagne la regex.
add_test(NAME mhgp3v_pair_anchor_u16_extremes_skew_code COMMAND mhgp3v_pair_anchor_probe
         --fixture u16-extremes-skew)
# LES REJEUX SANITIZES DU SWEEP : les fixtures qui exercent le tri, les
# groupes confondus, les antipodes et les magnitudes extremes tournent aussi
# sous ASan+UBSan a code exact ; un debordement signe du noyau ou de ses
# adaptateurs y meurt au lieu de se compenser.
if(NOT MHGP3V_ENABLE_CUDA)
  foreach(san_fixture depth-rays confounded-arc diametral-contact
          u16-extremes-anchor u16-extremes-skew)
    add_test(NAME mhgp3v_pair_anchor_san_${san_fixture} COMMAND
             mhgp3v_pair_anchor_probe_san --fixture ${san_fixture})
  endforeach()
  add_test(NAME mhgp3v_pair_anchor_san_depth_campaign COMMAND
           mhgp3v_pair_anchor_probe_san --points 32 --seed 13 --family terrain
           --leaf-size 2 --mode depth --oracle 1 --min-depth-pairs 400)
endif()
# LES MUTANTS DES PORTES 1 ET 3 : garde degeneree q4, egalite rationnelle
# sure, prune de profondeur debranche, rayons confondus otes de l'arc,
# contacts diametraux credites — cinq codes 4 exacts.
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_q4_degenerate "mutant tue" 4 --fixture q4-colocated
  --inject q4-degenerate-accept)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_q4_large_to_strict "mutant tue" 4
  --fixture q4-rational-equality --inject q4-large-to-strict)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_depth_disarmed "mutant tue" 4 --fixture nine-axial
  --mode depth --lane q3 --inject depth-prune-disarmed)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_confounded_dropped "mutant tue" 4
  --fixture confounded-arc --inject depth-confounded-dropped)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_kill_diametral_closed "mutant tue" 4
  --fixture diametral-contact --inject diametral-open-to-closed)
# LA RECEPTION L4/HERITAGE DU COEUR (audit etat courant, porte 3 ; miroir de
# la gate q2 d705bcde) : differentiel bi-mode — baseline sans L4 ni heritage,
# sans les injections du sujet, sort baseline totalement assigne ; egalite de
# TOUS les sorts et masses ; plancher plein+1 arme (1533 cas q3 a terrain
# 400) ; cinq mutants a code 4 exact ; le plancher hors differentiel REFUSE.
add_test(NAME mhgp3v_pair_anchor_differential_terrain COMMAND mhgp3v_pair_anchor_probe
         --points 400 --seed 20260810 --family terrain --differential 1
         --min-full-plus-one 100)
set_tests_properties(mhgp3v_pair_anchor_differential_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION "sorts et masses IDENTIQUES")
add_test(NAME mhgp3v_pair_anchor_differential_terrain_code COMMAND mhgp3v_pair_anchor_probe
         --points 400 --seed 20260810 --family terrain --differential 1
         --min-full-plus-one 100)
add_test(NAME mhgp3v_pair_anchor_differential_multiecho_code COMMAND
         mhgp3v_pair_anchor_probe --points 400 --seed 20260810
         --family scanline_overlap_multiecho --differential 1)
foreach(core_mutant l4-sign inherit-shift inherit-double-count inherit-stale-sibling
        stop-at-inherited)
  mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
    mhgp3v_pair_anchor_kill_core_${core_mutant} "mutant tue" 4 --points 400
    --seed 20260810 --family terrain --differential 1 --min-full-plus-one 10
    --inject ${core_mutant})
endforeach()
mhgp3v_add_expected_code_test_for(mhgp3v_pair_anchor_probe
  mhgp3v_pair_anchor_floor_without_differential "plancher" 2 --points 400
  --seed 20260810 --family terrain --min-full-plus-one 10)
# LA PORTE DIRECTE DU GENERATEUR (audit, porte 1) : nominal code 0 + regex,
# mutant overshoot code 4, CLI hostile code 2.
add_test(NAME mhgp3v_cloud_family_cardinality COMMAND mhgp3v_cloud_family_gate)
set_tests_properties(mhgp3v_cloud_family_cardinality PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : contrat de cardinalite du generateur partage")
add_test(NAME mhgp3v_cloud_family_cardinality_code COMMAND mhgp3v_cloud_family_gate)
mhgp3v_add_expected_code_test_for(mhgp3v_cloud_family_gate
  mhgp3v_cloud_family_kill_overshoot "mutant tue" 4 --mutant generator-overshoot)
mhgp3v_add_expected_code_test_for(mhgp3v_cloud_family_gate
  mhgp3v_cloud_family_reject_unknown_mutant "mutant inconnu" 2 --mutant inexistant)

# LA PORTE DU DEGRE GABRIEL : nominal code 0 + regex, mutant cap-12 code 4,
# CLI hostile code 2. Le PASS_REGULAR_EXPRESSION ignore le code de retour :
# la porte a code double le nominal.
add_test(NAME mhgp3v_gabriel_degree COMMAND mhgp3v_gabriel_degree_gate)
set_tests_properties(mhgp3v_gabriel_degree PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : gate du degre Gabriel recue")
add_test(NAME mhgp3v_gabriel_degree_code COMMAND mhgp3v_gabriel_degree_gate)
mhgp3v_add_expected_code_test_for(mhgp3v_gabriel_degree_gate
  mhgp3v_gabriel_degree_kill_cap12 "mutant tue" 4 --inject degree-cap-12)
mhgp3v_add_expected_code_test_for(mhgp3v_gabriel_degree_gate
  mhgp3v_gabriel_degree_reject_unknown_arg "argument inconnu" 2 --zzz)
mhgp3v_add_expected_code_test_for(mhgp3v_gabriel_degree_gate
  mhgp3v_gabriel_degree_reject_unknown_inject "injection inconnue" 2 --inject inexistant)

# LA SOURCE q2 YAO48/LBVH : six fixtures gravees (regex + doubles a code),
# campagnes oracle avec juge quadratique independant, differentiel bi-mode,
# equivariance par renversement, sept mutants a code exact et planchers de
# non-vacuite. Le detail des contrats est en tete de pair_yao48_source.cpp.
foreach(yao_fixture directional-equality region-prune underfull nonconverse
        u16-extremes colocated radial-straddle)
  add_test(NAME mhgp3v_yao48_fixture_${yao_fixture} COMMAND
           mhgp3v_pair_yao48_source --fixture ${yao_fixture})
  set_tests_properties(mhgp3v_yao48_fixture_${yao_fixture} PROPERTIES
    PASS_REGULAR_EXPRESSION "OK : fixture ${yao_fixture} recue")
  add_test(NAME mhgp3v_yao48_fixture_${yao_fixture}_code COMMAND
           mhgp3v_pair_yao48_source --fixture ${yao_fixture})
endforeach()
add_test(NAME mhgp3v_yao48_oracle_terrain200 COMMAND mhgp3v_pair_yao48_source
         --points 200 --seed 20260810 --family terrain --oracle 1 --permute 1
         --min-census-records 1 --min-underfull 1 --min-radial-prunes 1)
set_tests_properties(mhgp3v_yao48_oracle_terrain200 PROPERTIES
  PASS_REGULAR_EXPRESSION "aucun desaccord")
add_test(NAME mhgp3v_yao48_oracle_terrain200_code COMMAND mhgp3v_pair_yao48_source
         --points 200 --seed 20260810 --family terrain --oracle 1 --permute 1
         --min-census-records 1 --min-underfull 1)
add_test(NAME mhgp3v_yao48_oracle_uniform128_code COMMAND mhgp3v_pair_yao48_source
         --points 128 --seed 7 --family uniform --oracle 1 --permute 1)
add_test(NAME mhgp3v_yao48_oracle_multiecho256_code COMMAND mhgp3v_pair_yao48_source
         --points 256 --seed 20260810 --family scanline_overlap_multiecho --oracle 1
         --permute 1)
add_test(NAME mhgp3v_yao48_differential_terrain400_code COMMAND mhgp3v_pair_yao48_source
         --points 400 --seed 20260810 --family terrain --differential 1)
# L'INVARIANCE DES POLITIQUES DE TRAVAIL (exigence d'audit) : budgets minimal
# et ample rendent les memes sorts et census — la coupe accelere, elle ne
# decide jamais.
add_test(NAME mhgp3v_yao48_policy_differential_code COMMAND mhgp3v_pair_yao48_source
         --points 400 --seed 20260810 --family scanline_overlap_multiecho
         --policy-differential 1)
# L'INVARIANCE PAR SHARDS (exigence d'audit) : les reçus fusionnes de 1, 2 et
# 4 threads sont identiques champ par champ, fusion EXHAUSTIVE comprise.
add_test(NAME mhgp3v_yao48_shard_invariance COMMAND mhgp3v_pair_yao48_source
         --points 400 --seed 20260810 --family terrain --shard-check 1)
set_tests_properties(mhgp3v_yao48_shard_invariance PROPERTIES
  PASS_REGULAR_EXPRESSION "reçus fusionnes IDENTIQUES champ par champ")
add_test(NAME mhgp3v_yao48_shard_invariance_code COMMAND mhgp3v_pair_yao48_source
         --points 400 --seed 20260810 --family terrain --shard-check 1)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_strict_to_large "mutant tue" 4 --fixture directional-equality
  --inject strict-to-large)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_d_understated "mutant tue" 4 --fixture directional-equality
  --inject d-understated)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_threshold_minus_one "mutant tue" 4 --fixture directional-equality
  --inject threshold-minus-one)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_chamber_perm_swapped "mutant tue" 4 --fixture region-prune
  --inject chamber-perm-swapped)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_last_region_omitted "mutant tue" 4 --fixture region-prune
  --inject last-region-omitted)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_ownership_doubled "mutant tue" 4 --fixture region-prune
  --inject ownership-doubled)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_census_skips "mutant tue" 4 --fixture colocated
  --inject census-skips-inf-zero)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_kill_radial_forgets "mutant tue" 4 --points 200 --seed 20260810
  --family terrain --oracle 1 --inject radial-forgets-chamber)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_reject_unknown "argument inconnu" 2 --inexistant 1)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_reject_absurd "campagne absurde" 2 --points 2)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_reject_oracle_large "oracle exhaustif exige" 2 --points 300 --oracle 1)
mhgp3v_add_expected_code_test_for(mhgp3v_pair_yao48_source
  mhgp3v_yao48_floor_bites "plancher viole" 3 --fixture underfull
  --min-region-prunes 1)

# Les quatre fixtures gravees de la lane k=1 (feuilles 2, oracle Prim force,
# equivariance) ; tie-lattice arme le plancher des lots d'egalite,
# two-clusters celui des rondes.
add_test(NAME mhgp3v_emst_fixture_tie_lattice COMMAND mhgp3v_emst_boruvka_probe
         --fixture tie-lattice --permute 1 --min-tie-batches 1)
add_test(NAME mhgp3v_emst_fixture_colocated_batch COMMAND mhgp3v_emst_boruvka_probe
         --fixture colocated-batch --permute 1)
add_test(NAME mhgp3v_emst_fixture_u16_extremes COMMAND mhgp3v_emst_boruvka_probe
         --fixture u16-extremes --permute 1)
add_test(NAME mhgp3v_emst_fixture_two_clusters COMMAND mhgp3v_emst_boruvka_probe
         --fixture two-clusters --permute 1 --min-rounds 2)
# La campagne juge + equivariance, planchers armes contre le vert-par-vacuite
# (mesures a cette graine : 5 rondes, 53 niveaux distincts, 40 lots).
add_test(NAME mhgp3v_emst_oracle_campaign COMMAND mhgp3v_emst_boruvka_probe
         --points 512 --seed 20260810 --family terrain --oracle 1 --permute 1
         --min-rounds 3 --min-distinct-levels 40 --min-tie-batches 8)
# Les cinq mutants meurent au code contractuel 4.
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_mutant_reconnect "mutant tue par le juge Prim" 4
  --points 512 --seed 20260810 --family terrain --oracle 1
  --inject reconnect-wrong-components)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_mutant_level_off_by_one "mutant tue par le juge Prim" 4
  --points 512 --seed 20260810 --family terrain --oracle 1
  --inject level-off-by-one)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_mutant_purity_stale "mutant tue par le juge Prim" 4
  --points 512 --seed 20260810 --family terrain --oracle 1
  --inject purity-stale)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_mutant_skip_last_round "arbre couvrant viole" 4
  --fixture tie-lattice --inject skip-last-round)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_mutant_tie_nondeterministic "jeu canonique" 4
  --fixture tie-lattice --inject tie-nondeterministic)
# CLI hostile (2) et planchers (3).
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_cli_junk "sans valeur" 2 --points 1x2)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_cli_absurde "campagne absurde" 2 --points 2)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_cli_oracle_trop_grand "exige n <= 2048" 2 --points 4000 --oracle 1)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_floor_distinct_levels "plancher viole" 3
  --fixture tie-lattice --min-distinct-levels 2)
mhgp3v_add_expected_code_test_for(mhgp3v_emst_boruvka_probe
  mhgp3v_emst_floor_nuage_non_genere "nuage non genere" 3
  --points 1000 --coord 4 --family uniform)

# LES CINQ FIXTURES GRAVEES DE P1a (verifiees hors bande en arithmetique
# exacte) : la racine entiere et l'egalite de Jung de la note §2, un bloc
# reellement prunable, la politique microtuile-avant-cover et les extremes u16.
foreach(p1a_fixture jung-root jung-equality prunable-block microtile-first
        u16-extremes)
  add_test(NAME mhgp3v_p1a_fixture_${p1a_fixture} COMMAND
           mhgp3v_center_cover_mass_probe --fixture ${p1a_fixture})
  set_tests_properties(mhgp3v_p1a_fixture_${p1a_fixture} PROPERTIES
    PASS_REGULAR_EXPRESSION "OK : fixture ${p1a_fixture} recue")
  add_test(NAME mhgp3v_p1a_fixture_${p1a_fixture}_code COMMAND
           mhgp3v_center_cover_mass_probe --fixture ${p1a_fixture})
endforeach()
# LES CAMPAGNES DU JUGE DETERMINANTAL (n <= 32) : reçus rejoues temoin par
# temoin contre CHAQUE point reel du cote engage, bijection des sorts et
# enumeration exhaustive des supports q4 propres positifs. Les planchers
# refusent le vert-par-vacuite.
add_test(NAME mhgp3v_p1a_oracle_terrain32 COMMAND mhgp3v_center_cover_mass_probe
         --points 32 --seed 13 --family terrain --leaf-size 2 --microtile 4
         --oracle 1 --permute 1 --min-block-prunes 1 --min-microtiles 1
         --min-splits 1 --min-patch-surviving 1 --min-accepted-nodes 1
         --min-witness-point-tests 1)
set_tests_properties(mhgp3v_p1a_oracle_terrain32 PROPERTIES
  PASS_REGULAR_EXPRESSION "aucun desaccord")
add_test(NAME mhgp3v_p1a_oracle_terrain32_code COMMAND mhgp3v_center_cover_mass_probe
         --points 32 --seed 13 --family terrain --leaf-size 2 --microtile 4
         --oracle 1 --permute 1 --min-block-prunes 1 --min-microtiles 1
         --min-splits 1 --min-patch-surviving 1 --min-accepted-nodes 1
         --min-witness-point-tests 1)
add_test(NAME mhgp3v_p1a_oracle_scanline32_code COMMAND mhgp3v_center_cover_mass_probe
         --points 32 --seed 3 --family scanline_single_pass --leaf-size 2
         --microtile 4 --oracle 1 --permute 1)
add_test(NAME mhgp3v_p1a_oracle_uniform28_code COMMAND mhgp3v_center_cover_mass_probe
         --points 28 --seed 7 --family uniform --leaf-size 2 --microtile 4
         --oracle 1 --permute 1)
add_test(NAME mhgp3v_p1a_oracle_multiecho32_code COMMAND mhgp3v_center_cover_mass_probe
         --points 32 --seed 5 --family scanline_overlap_multiecho --leaf-size 2
         --microtile 4 --oracle 1 --permute 1)
# LES QUINZE MUTANTS DE LA NOTE §8, chacun a code 4 exact sur la fixture ou la
# campagne qui le tue.
foreach(p1a_mutant clip-scale-4 ceil-root-truncated patch-omitted
        feasible-declared-infeasible witness-duplicated witness-subtrees-overlap
        endpoint-overlap-subtracted stale-inherited-credit
        q4-radius-replaced-by-q3 microtile-truncated terminal-compensated)
  string(REPLACE "-" "_" p1a_mutant_id "${p1a_mutant}")
  mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
    mhgp3v_p1a_kill_${p1a_mutant_id} "mutant tue" 4 --fixture prunable-block
    --inject ${p1a_mutant})
endforeach()
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_kill_strict_to_large "mutant tue" 4 --fixture jung-equality
  --inject strict-to-large)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_kill_cover_before_terminal "mutant tue" 4 --fixture microtile-first
  --inject cover-before-terminal)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_kill_corner_skipped "mutant tue" 4 --points 32 --seed 13
  --family terrain --leaf-size 2 --microtile 4 --oracle 1 --inject corner-skipped)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_kill_witness_strict_to_large "mutant tue" 4 --points 32 --seed 3
  --family scanline_single_pass --leaf-size 2 --microtile 4 --oracle 1
  --inject witness-strict-to-large)
# CLI hostile (2), preflight du seuil (2) et plancher (3).
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_reject_unknown "argument inconnu" 2 --inexistant 1)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_reject_preflight "preflight viole" 2 --points 64 --leaf-size 8
  --microtile 16)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_reject_oracle_large "juge exhaustif exige" 2 --points 64 --oracle 1)
mhgp3v_add_expected_code_test_for(mhgp3v_center_cover_mass_probe
  mhgp3v_p1a_floor_bites "plancher viole" 3 --fixture microtile-first
  --min-block-prunes 1)

# Le harnais diagnostique : fumee bornee (n=2000, 2 reps, 2 threads) avec la
# serie NOMMEE et le refus explicite de tout SLO ; CLI hostile a code 2.
add_test(NAME mhgp3v_warm_e2e_smoke COMMAND mhgp3v_warm_e2e_h0_diagnostic
         --points 2000 --reps 2 --threads 2 --seed 20260811)
set_tests_properties(mhgp3v_warm_e2e_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "serie diagnostique warm_e2e_h0_v3_diagnostic")
add_test(NAME mhgp3v_warm_e2e_smoke_code COMMAND mhgp3v_warm_e2e_h0_diagnostic
         --points 2000 --reps 2 --threads 2 --seed 20260811)
mhgp3v_add_expected_code_test_for(mhgp3v_warm_e2e_h0_diagnostic
  mhgp3v_warm_e2e_reject_unknown "argument inconnu" 2 --inexistant 1)


# LA PORTE DE LA FACTORY : sept fixtures du contrat, trois mutants tues.
add_test(NAME mhgp3v_sidecar_factory_fixtures COMMAND mhgp3v_sidecar_factory_gate)
set_tests_properties(mhgp3v_sidecar_factory_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : factory ValidatedHybridSidecar")
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_kill_skip_last_removal "mutant tue" 4 --mutant skip-last-removal)
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_kill_strict_leq "mutant tue" 4 --mutant strict-leq)
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_kill_witness_by_position "mutant tue" 4 --mutant witness-by-position)
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_kill_skip_canonical_check "mutant tue" 4 --mutant skip-canonical-check)
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_kill_sha_fault "selftest SHA-256" 4 --mutant sha-fault)
mhgp3v_add_expected_code_test_for(mhgp3v_sidecar_factory_gate
  mhgp3v_sidecar_reject_unknown_mutant "mutant inconnu" 2 --mutant inexistant)
add_test(NAME mhgp3v_structural_scale_k1_emst COMMAND mhgp3v_structural_scale_check
         --points 200 --smax 11 --seed 20260810)
mhgp3v_add_expected_code_test_for(mhgp3v_structural_scale_check
  mhgp3v_structural_scale_kill_shift "REFUTATION" 1
  --points 100 --smax 11 --seed 7 --force-shift-fusion 1)
# LE RENFORCEMENT PAR PARTITIONS (reponse 20260811) : deux dendrogrammes de
# memes poids sont indistinguables par les seuls niveaux — la partition
# canonique des PointId est comparee en strict et en ferme a chaque niveau
# exact, et une union un lot trop tot doit rougir.
mhgp3v_add_expected_code_test_for(mhgp3v_structural_scale_check
  mhgp3v_structural_scale_kill_merge_early "REFUTATION : partition" 1
  --points 100 --smax 11 --seed 7 --force-merge-early 1)
# LA PORTE PARALLELE DURCIE, a QUATRE threads et plus (la data race du
# CertifiedIndex partage est reparee : les metriques de descente sont par
# worker). Les planchers (racines de frontiere, sommets de sous-arbres) sont
# calibres a environ la moitie de la campagne mesuree (1155 racines, 311 481
# sommets) : une frontiere vide ou des sous-arbres muets ne rendent plus OK.
add_test(NAME mhgp3v_parallel_catalogue_differential COMMAND mhgp3v_parallel_catalogue_gate
         --points 120 --smax 11 --clouds 3 --seed 20260810 --threads 4
         --min-roots 600 --min-subtree-vertices 150000)
# drop-odd-roots meurt par le MULTIENSEMBLE (des sommets manquent meme si leurs
# boules sont redecouvertes) ; duplicate-root meurt par la MULTIPLICITE (le
# catalogue deduplique, lui, resterait identique — c'est le certificat que la
# porte precedente n'avait pas).
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_kill_drop_roots "mutant tue par le multiensemble" 4
  --points 120 --smax 11 --clouds 3 --threads 4 --mutant drop-odd-roots)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_kill_duplicate_root "mutant tue par la multiplicite" 4
  --points 120 --smax 11 --clouds 3 --threads 4 --mutant duplicate-root)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_reject_unknown_mutant "mutant inconnu" 2
  --mutant inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_reject_low_threads "threads >= 4 exiges" 2
  --points 60 --smax 8 --clouds 1 --threads 2)
mhgp3v_add_expected_code_test_for(mhgp3v_parallel_catalogue_gate
  mhgp3v_parallel_catalogue_reject_floor "plancher de couverture" 3
  --points 60 --smax 8 --clouds 1 --seed 20260810 --threads 4
  --min-roots 100000000)
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
# Terrain mesure : 825 racines, 155 701 sommets de sous-arbres — planchers a
# environ la moitie, memes threads que la porte uniforme.
add_test(NAME mhgp3v_parallel_catalogue_terrain COMMAND mhgp3v_parallel_catalogue_gate
         --points 100 --smax 11 --clouds 2 --family terrain --seed 20260810 --threads 4
         --min-roots 400 --min-subtree-vertices 75000)
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
add_test(NAME mhgp3v_saturated_pipeline_compare_joins_hybrid_prefix
         COMMAND mhgp3v_saturated_pipeline
         --points 10 --smax 11 --max-order 3 --seed 20260810 --join hybrid-prefix
         --compare-joins 1)
mhgp3v_add_expected_code_test_for(mhgp3v_saturated_pipeline
  mhgp3v_saturated_pipeline_reject_hybrid_prefix_partial "famille complete" 2
  --points 32 --smax 11 --max-order 3 --join hybrid-prefix)
# LE MODE PREFIX-ALL (reponse 20260811, ordre recommande §2) : le join prefixe
# RELATIF a toute table recue — il ne refuse PAS la famille tronquee, c'est
# son objet : separer la correction du join de la completude de la source.
# `--compare-joins 1` grave le DIFFERENTIEL contre la verite G^2 sur la meme
# famille partielle (reception live 20260811) : la provenance seule ne recoit
# rien.
add_test(NAME mhgp3v_saturated_pipeline_prefix_all_partial
         COMMAND mhgp3v_saturated_pipeline
         --points 32 --smax 11 --max-order 3 --seed 20260810 --join prefix-all
         --compare-joins 1)
set_tests_properties(mhgp3v_saturated_pipeline_prefix_all_partial PROPERTIES
  PASS_REGULAR_EXPRESSION "prefix-all, relatif a la table recue")
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








# ---------------------------------------------------------------------------
# FRONT DE BOULES VIDES : le SECOND TEMOIN de l'arrangement shallow.
#
# `order_k_flats.hpp` reste le sujet mathematique de reference — transitions par
# lots, reverse search, theoreme de proprietaire. Ce temoin ne le remplace pas :
# il enumere les MEMES sommets avec une autre representation (forme liftee
# separable, elagage de boite exact par parabole entiere) et d'autres
# transitions, puis se juge contre un oracle exhaustif ecrit dans le meme
# fichier mais sur un determinant 5x5 developpe. Deux defauts communs ne se
# compensent donc pas.
#
# PORTEE DECLAREE : univers a shell d'au moins quatre points. Les supports q2 et
# q3 a shell de deux ou trois elements vivent sur les faces et les aretes de
# l'arrangement; ils sont hors de ce temoin ET hors de son juge, donc
# `incomplete`, jamais absents.
add_executable(mhgp3v_ball_front prototype/ball_front_probe.cpp)
target_include_directories(mhgp3v_ball_front PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_ball_front PRIVATE mhgp)
target_compile_options(mhgp3v_ball_front PRIVATE -Wall -Wextra -Werror)

# LES QUATRE FAMILLES, JUGEES SUR LES IDENTITES. Le juge compare shell par shell
# et interieur par interieur, pas des cardinalites : une omission compensee par
# un doublon ne peut pas rester verte.
add_test(NAME mhgp3v_ball_front_uniform COMMAND mhgp3v_ball_front
         --points=70 --k=3 --family=uniform --seed=3 --judge)
add_test(NAME mhgp3v_ball_front_terrain COMMAND mhgp3v_ball_front
         --points=70 --k=3 --family=terrain --seed=3 --judge)
add_test(NAME mhgp3v_ball_front_scanline COMMAND mhgp3v_ball_front
         --points=70 --k=3 --family=scanline_single_pass --seed=3 --judge)
# LE REGIME QUANTIFIE MULTI-ECHO est celui qui porte les cospheries : c'est la
# ou un parcours qui suppose un shell de quatre points exactement se casse.
add_test(NAME mhgp3v_ball_front_multiecho COMMAND mhgp3v_ball_front
         --points=70 --k=3 --family=scanline_overlap_multiecho --seed=3 --judge)
set_tests_properties(mhgp3v_ball_front_uniform mhgp3v_ball_front_terrain
                     mhgp3v_ball_front_scanline mhgp3v_ball_front_multiecho
                     PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD juge")

# LES DEUX SENS DE L'ARETE SONT NECESSAIRES. Couper l'un des deux laisse un
# parcours qui trouve encore la grande majorite des sommets : seul un juge par
# identites le tue. Code 4 = mutant tue.
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_kill_drop_entering "MUTANT TUE" 4
  --points=70 --k=2 --family=terrain --seed=3 --judge --inject=drop-entering)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_kill_drop_leaving "MUTANT TUE" 4
  --points=70 --k=2 --family=terrain --seed=3 --judge --inject=drop-leaving)


# REFUS AVANT CALCUL : code 2.
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_unknown_argument "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_bad_family "famille inconnue" 2 --family=inexistante)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_judge_too_large "juge exhaustif" 2
  --points=500 --k=1 --judge)



find_package(Threads REQUIRED)









# LES FIXTURES DE PROJECTION du front : la projection q2/q3 depuis le shell d'un
# sommet ne conserve PAS son niveau. Elles gravent la refutation de la regle que
# j'avais proposee et que l'auditeur a cassee.
add_test(NAME mhgp3v_ball_front_fixtures COMMAND mhgp3v_ball_front --fixtures)
set_tests_properties(mhgp3v_ball_front_fixtures
                     PROPERTIES PASS_REGULAR_EXPRESSION "ne conserve pas le niveau")

# ---------------------------------------------------------------------------
# SQUELETTE DU PAYLOAD OFFICIEL `BenchmarkOutputContract-v1`.
#
# Point un de l'ordre de travail de l'audit courant. La section 14.4 du plan de
# tests chronometre `warm_e2e` jusqu'au retour hote du payload — dix forets,
# verticales, lots et certificat minimal en memoire epinglee. Tant que ce
# payload n'a pas de structure declaree, aucune mesure n'est comparable au seuil
# et aucun audit ne peut verifier ce qui manque.
#
# Ce target ne calcule RIEN. Il declare l'inventaire des etages, leur statut,
# leur producteur, leur porte et leur appartenance au chemin critique, puis
# refuse l'eligibilite. C'est un instrument de comptabilite, pas un benchmark.
add_executable(mhgp3v_payload_skeleton prototype/benchmark_payload_harness.cpp)
target_compile_options(mhgp3v_payload_skeleton PRIVATE -Wall -Wextra -Werror)

# L'INVENTAIRE DOIT DIRE NON. Zero etage du chemin critique n'est complet : le
# squelette ne peut pas rendre le contrat eligible, et cette porte garde cette
# verite visible tant qu'elle dure.
add_test(NAME mhgp3v_payload_inventaire COMMAND mhgp3v_payload_skeleton --points=50000)
set_tests_properties(mhgp3v_payload_inventaire
                     PROPERTIES PASS_REGULAR_EXPRESSION "slo_eligible=non")

# L'EXIGENCE DE COMPLETUDE echoue par construction aujourd'hui. Le jour ou elle
# passera, le chemin critique sera reellement produit — et ce jour-la cette
# porte devra etre inversee sciemment, pas silencieusement.
mhgp3v_add_expected_code_test_for(mhgp3v_payload_skeleton
  mhgp3v_payload_exigence "EXIGENCE non satisfaite" 3 --require-complete)

# LE MUTANT DE FRAUDE : promouvoir tous les etages a `complete` sans producteur
# ni porte. Le controle de coherence doit le voir.
mhgp3v_add_expected_code_test_for(mhgp3v_payload_skeleton
  mhgp3v_payload_kill_claim "MUTANT TUE" 4 --inject=claim-complete)

mhgp3v_add_expected_code_test_for(mhgp3v_payload_skeleton
  mhgp3v_payload_reject_unknown "argument inconnu" 2 --sujet-inexistant)



if(Python3_Interpreter_FOUND OR Python3_EXECUTABLE)
  set(MHGP3V_CC_JUDGE_DRIVER
      ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_centre_cell_independent_judge.py)
endif()

# LA LANE `k=1` SANS YAO-1, ET SON JUGE EXHAUSTIF INDEPENDANT.
#
# Un support q2 a zero interieur EST une arete de Gabriel : sa boule diametrale
# est vide. Tout EMST etant contenu dans le Gabriel, la lane `k=1` se lit
# directement dans cette source — ni transcript de chambres, ni banque, ni
# enumeration Yao. Elle filtre, deduplique par le Kruskal creux, et groupe les
# niveaux egaux en lots atomiques.
#
# Le juge calcule l'EMST par un Prim EXHAUSTIF sur toutes les paires, en
# distances carrees entieres. Il ne partage aucun predicat avec le sujet. Le MST
# n'est pas unique en presence d'ex aequo, mais le multiensemble trie de ses
# poids l'est : c'est lui qu'on compare. L'accord verifie simultanement le
# theoreme et la completude de l'extraction.

# L'INVARIANCE DU RECU AU NOMBRE DE WORKERS.
#
# Le parallelisme decoupe l'arbre en sous-arbres independants a une profondeur
# de recolte. La propriete half-open garantit qu'un support n'est possede que
# par une seule cellule, donc par une seule tache; les compteurs cumulatifs
# s'additionnent, les compteurs de pic prennent le maximum. Le recu doit donc
# etre IDENTIQUE ligne a ligne, la seule ligne exclue etant celle qui annonce le
# nombre de workers. C'est la propriete qu'un portage device devra conserver, et
# c'est ce que cette porte exige litteralement.

# ---------------------------------------------------------------------------
# SOURCE PAR ANCRE MAXIMALE ET ENVELOPPE MOBILE.
# Specification : audits/NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md
# Ce sujet produit Source S en emettant chaque support depuis sa SEULE arete
# maximale canonique. Les portes ci-dessous ne recoivent ni complexite, ni SLO.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_anchor_source prototype/anchor_source.cpp)
target_include_directories(mhgp3v_anchor_source PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_anchor_source PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_anchor_source PRIVATE -Wall -Wextra -Werror)

# ---- Portes du producteur par ancre maximale -------------------------------
# Le differentiel `--verify` rejoue l'extension sur TOUTES les paires sans le
# moindre certificat, puis compare cle par cle et p par p. Il recoit donc d'un
# seul coup le certificat de front, les boules de milieu, le disque de Jung et
# l'enveloppe mobile : n'importe lequel qui couperait a tort ferait chuter le
# compte. Les planchers interdisent le vert par vacuite.
add_test(NAME mhgp3v_anchor_uniform COMMAND mhgp3v_anchor_source
         --points=120 --family=uniform --seed=1 --verify --min-supports=20000
         --min-anchors=7000)
add_test(NAME mhgp3v_anchor_terrain COMMAND mhgp3v_anchor_source
         --points=120 --family=terrain --seed=5 --verify --min-supports=5000
         --min-anchors=7000)
add_test(NAME mhgp3v_anchor_scanline COMMAND mhgp3v_anchor_source
         --points=110 --family=scanline_single_pass --seed=2 --verify
         --min-supports=4000 --min-anchors=5000)
add_test(NAME mhgp3v_anchor_multiecho COMMAND mhgp3v_anchor_source
         --points=110 --family=scanline_overlap_multiecho --seed=2 --verify
         --min-supports=3000 --min-anchors=5000)
# A cette taille le certificat de front coupe reellement : le plancher de
# prunes empeche que le differentiel passe sur un certificat jamais exerce.
add_test(NAME mhgp3v_anchor_front_exerce COMMAND mhgp3v_anchor_source
         --points=500 --family=uniform --seed=4 --verify --min-prunes=1000
         --min-supports=100000)
# La fixture gravee `ties` est un tetraedre regulier de cote carre huit, dont
# les six aretes sont maximales et dont les quatre faces sont equilaterales :
# c'est le cas ou la regle canonique de tie-break decide seule de l'unicite.
add_test(NAME mhgp3v_anchor_fixture_ties COMMAND mhgp3v_anchor_source
         --fixture=ties --verify --min-supports=70)
set_tests_properties(mhgp3v_anchor_uniform mhgp3v_anchor_terrain
                     mhgp3v_anchor_scanline mhgp3v_anchor_multiecho
                     mhgp3v_anchor_front_exerce mhgp3v_anchor_fixture_ties
                     PROPERTIES PASS_REGULAR_EXPRESSION "accord=OUI")

# REDONDANCE DE `theta`, DEMONTREE PUIS GRAVEE.
# Sur une ancre restee vivante, moins de `smax-2` sites verifient `Llow>0`; la
# `(smax-2)`-ieme plus grande borne inferieure est donc <= 0, et `Uhigh<theta`
# implique `Uhigh<0`, deja couvert par `always_outside`. Le filtre ne retire
# donc AUCUN site d'une ancre vivante. Il n'est plus sur le chemin par defaut;
# `--theta-audit` le rearme pour ces portes. Le plancher `--min-theta-active`
# interdit le vert par vacuite : sans lui, un filtre jamais atteint donnerait
# le meme zero qu'un filtre demontre redondant.
add_test(NAME mhgp3v_anchor_theta_redondant_uniform COMMAND mhgp3v_anchor_source
         --points=120 --family=uniform --seed=1 --verify --theta-audit
         --min-theta-active=5000 --min-supports=20000)
add_test(NAME mhgp3v_anchor_theta_redondant_terrain COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=5 --verify --theta-audit
         --min-theta-active=3000 --min-supports=5000)
add_test(NAME mhgp3v_anchor_theta_redondant_clusters COMMAND mhgp3v_anchor_source
         --points=80 --family=eight_clusters --seed=3 --verify --theta-audit
         --min-theta-active=2000 --min-supports=6000)
set_tests_properties(mhgp3v_anchor_theta_redondant_uniform
                     mhgp3v_anchor_theta_redondant_terrain
                     mhgp3v_anchor_theta_redondant_clusters PROPERTIES
  PASS_REGULAR_EXPRESSION "prunes_en_plus_sur_ancre_vivante=0\n")
# Le regex ci-dessus ignore le code de retour : la porte a code le double. Si
# le theoreme etait faux, le sujet rendrait 3 avec ce diagnostic.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_theta_plancher_vacuite "plancher d'ancres theta" 3
  --points=120 --family=uniform --seed=1 --theta-audit
  --min-theta-active=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_theta_reject_plancher_sans_audit "--min-theta-active exige --theta-audit" 2
  --points=120 --min-theta-active=10)

# Mutants : chacun casse UNE decision exacte et doit sortir en code 4.
# Le mutant theta exige le filtre ARME : casser un filtre desarme ne prouve
# rien. Le diagnostic exige en outre un compteur NON NUL, ce qui interdit que
# la porte de redondance ci-dessus passe sur un compteur mort.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_theta "MUTANT TUE" 4
  --points=500 --seed=4 --verify --theta-audit --inject=theta-no-fail-open)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_theta_compteur_causal "prunes_en_plus_sur_ancre_vivante=[1-9]" 4
  --points=500 --seed=4 --verify --theta-audit --inject=theta-no-fail-open)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_mutant_theta_desarme
  "le mutant theta exige --theta-audit" 2
  --points=500 --seed=4 --verify --inject=theta-no-fail-open)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_arete_min "MUTANT TUE" 4
  --points=500 --seed=4 --verify --inject=owner-min-edge)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_lentille "MUTANT TUE" 4
  --points=500 --seed=4 --verify --inject=lens-strict)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_census "MUTANT TUE" 4
  --points=500 --seed=4 --verify --inject=census-no-always-inside)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_positivite "MUTANT TUE" 4
  --points=500 --seed=4 --verify --inject=positivity-loose)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_tiebreak "MUTANT TUE" 4
  --fixture=ties --verify --inject=owner-no-tiebreak)
# Le terme `ext/4` ne se voit que sur des boites ALLONGEES : sur `uniform` les
# noeuds sont presque cubiques et le mutant survit. Ces deux familles
# anisotropes sont donc la porte, pas un choix de confort.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_front_ext "MUTANT TUE" 4
  --points=300 --family=scanline_overlap_multiecho --seed=3 --verify
  --inject=front-no-ext)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_front_ext_terrain "MUTANT TUE" 4
  --points=300 --family=terrain --seed=3 --verify --inject=front-no-ext)
# La fixture `q4only` place huit points au milieu exact d'une longue paire :
# la lane q4 y meurt alors que le support q2 reste pertinent avec p=8. Fermer
# un produit sur la seule lane q4 le detruit.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_front_q4_seule "MUTANT TUE" 4
  --fixture=q4only --verify --inject=front-q4-only)
add_test(NAME mhgp3v_anchor_fixture_q4only COMMAND mhgp3v_anchor_source
         --fixture=q4only --verify --min-supports=55)
set_tests_properties(mhgp3v_anchor_fixture_q4only PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")

# ---- FAMILLE ADVERSAIRE `eight_clusters` -----------------------------------
# Elle etait versionnee mais qu'aucune porte n'exercait. Ces deux portes
# recoivent son EXACTITUDE dans les deux moteurs.
add_test(NAME mhgp3v_anchor_eight_clusters COMMAND mhgp3v_anchor_source
         --points=80 --family=eight_clusters --seed=3 --verify
         --min-supports=6000 --min-anchors=3000)
add_test(NAME mhgp3v_anchor_eight_clusters_pipeline COMMAND mhgp3v_anchor_source
         --points=80 --family=eight_clusters --seed=3 --engine=pipeline --verify
         --min-supports=6000 --min-anchors=3000)
set_tests_properties(mhgp3v_anchor_eight_clusters
                     mhgp3v_anchor_eight_clusters_pipeline PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")
# MUR GRAVE, PAS UN VERT. Sur cette famille le certificat de front ne ferme
# AUCUN noeud : la boule temoin d'une ancre inter-amas tombe dans le vide qui
# separe les amas, ou aucun PointId ne peut la peupler. `candidate_pairs` vaut
# donc exactement C(n,2) et le producteur est cubique : mesure du 12 aout 2026
# sur l'ELF pince, n=100/200/500 -> 2 446 467 / 17 892 952 / 191 538 784 paires
# q4 parcourues, pentes successives 2,65 a 3,03, wall 0,685 / 6,855 / 92,458 s.
# Cette porte EXIGE que le plancher d'un seul prune soit refuse. Elle deviendra
# rouge le jour ou une route ferme reellement ces ancres — et c'est exactement
# le signal attendu. Ne pas la supprimer sans reecrire cette mesure.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_eight_clusters_front_inerte "plancher de prunes" 3
  --points=100 --family=eight_clusters --seed=3 --no-store --min-prunes=1)
# Le moteur `pipeline` est EXACTEMENT le code que nvcc compile. Le comparer a
# l'exhaustif ferme la chaine hote -> pipeline -> device.
add_test(NAME mhgp3v_anchor_pipeline_uniform COMMAND mhgp3v_anchor_source
         --points=140 --family=uniform --seed=2 --engine=pipeline --verify
         --min-supports=25000)
add_test(NAME mhgp3v_anchor_pipeline_terrain COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=5 --engine=pipeline --verify
         --min-supports=5000)
add_test(NAME mhgp3v_anchor_pipeline_multiecho COMMAND mhgp3v_anchor_source
         --points=120 --family=scanline_overlap_multiecho --seed=2
         --engine=pipeline --verify --min-supports=3000)
add_test(NAME mhgp3v_anchor_pipeline_fixtures COMMAND mhgp3v_anchor_source
         --fixture=ties --engine=pipeline --verify --min-supports=70)
set_tests_properties(mhgp3v_anchor_pipeline_uniform mhgp3v_anchor_pipeline_terrain
                     mhgp3v_anchor_pipeline_multiecho mhgp3v_anchor_pipeline_fixtures
                     PROPERTIES PASS_REGULAR_EXPRESSION "accord=OUI")
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_mutant_pipeline "les mutants visent le moteur de reference" 2
  --points=90 --verify --engine=pipeline --inject=lens-strict)

# ---- PARITE DES LEDGERS ENTRE LES DEUX MOTEURS ------------------------------
# `--verify` ne compare que des SUPPORTS, et cela a laissé passer deux défauts
# réels : le moteur `pipeline` — celui que nvcc compile — n'avait AUCUN
# compteur de rejet et publiait `rejets ... = 0` là où la référence en comptait
# `1 169 095`; et sa garde de densité n'existait pas côté référence, de sorte
# que les deux ledgers de front n'étaient pas comparables. Un compteur absent
# n'est pas un compteur nul. Ces portes comparent les TRENTE-SIX compteurs
# nommément, dans les deux états de la garde. Le nombre `compteurs=36` est dans
# le regex : une liste de comparaison silencieusement raccourcie serait rouge.
add_test(NAME mhgp3v_anchor_parite_ledger_uniform COMMAND mhgp3v_anchor_source
         --points=100 --family=uniform --seed=3 --compare-engines --min-supports=15000)
add_test(NAME mhgp3v_anchor_parite_ledger_terrain COMMAND mhgp3v_anchor_source
         --points=100 --family=terrain --seed=3 --compare-engines --min-supports=4000)
add_test(NAME mhgp3v_anchor_parite_ledger_clusters COMMAND mhgp3v_anchor_source
         --points=100 --family=eight_clusters --seed=3 --compare-engines --min-supports=9000)
add_test(NAME mhgp3v_anchor_parite_ledger_multiecho COMMAND mhgp3v_anchor_source
         --points=100 --family=scanline_overlap_multiecho --seed=3 --compare-engines
         --min-supports=4000)
add_test(NAME mhgp3v_anchor_parite_ledger_theta COMMAND mhgp3v_anchor_source
         --points=100 --family=uniform --seed=3 --compare-engines --theta-audit
         --min-theta-active=2000 --min-supports=15000)
set_tests_properties(mhgp3v_anchor_parite_ledger_uniform mhgp3v_anchor_parite_ledger_terrain
                     mhgp3v_anchor_parite_ledger_clusters mhgp3v_anchor_parite_ledger_multiecho
                     mhgp3v_anchor_parite_ledger_theta
                     PROPERTIES
  PASS_REGULAR_EXPRESSION "parite_moteurs compteurs=36 ecarts_compteurs=0[^\n]*accord=OUI")
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_compare_engines_pipeline
  "--compare-engines lance lui-meme les deux moteurs" 2
  --points=90 --compare-engines --engine=pipeline)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_compare_engines_mutant
  "--compare-engines compare deux moteurs sains" 2
  --points=90 --compare-engines --verify --inject=lens-strict)

# ---- MORT PAR BUDGET, AU PLUS TOT -------------------------------------------
# `Llow>0` certifie qu'un site est strictement intérieur à TOUTE boule
# admissible de l'ancre; un sous-ensemble de tels sites est donc un minorant
# exact de `p`. Dès qu'il dépasse le budget de la dernière lane vivante,
# l'ancre est morte et la décision ne peut plus changer. La boucle de marges
# s'arrête là. La sortie est identique — c'est la MEME décision, prise plus
# tôt : à `eight_clusters n=500`, `q4_paires_parcourues=191 538 784` et
# `interior_tests=334 430 649` sont inchangés au chiffre près, tandis que
# `site_evaluations` tombe de `33 870 356` à `13 983 250` et le mur de
# `92,46` à `33,53` s. Ces portes exigent l'accord ET un plancher de morts
# anticipées : sans le plancher, un chemin jamais emprunté passerait aussi.
add_test(NAME mhgp3v_anchor_budget_precoce_uniform COMMAND mhgp3v_anchor_source
         --points=120 --family=uniform --seed=3 --verify --min-budget-early=700
         --min-supports=15000)
add_test(NAME mhgp3v_anchor_budget_precoce_clusters COMMAND mhgp3v_anchor_source
         --points=120 --family=eight_clusters --seed=3 --verify
         --min-budget-early=1000 --min-supports=9000)
add_test(NAME mhgp3v_anchor_budget_precoce_pipeline COMMAND mhgp3v_anchor_source
         --points=120 --family=terrain --seed=3 --engine=pipeline --verify
         --min-budget-early=1000 --min-supports=4000)
set_tests_properties(mhgp3v_anchor_budget_precoce_uniform
                     mhgp3v_anchor_budget_precoce_clusters
                     mhgp3v_anchor_budget_precoce_pipeline PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_budget_precoce_plancher "plancher de morts anticipees" 3
  --points=120 --family=uniform --seed=3 --min-budget-early=100000000)

# GARDE DE DENSITE : HEURISTIQUE ABLATEE, PAS UN CERTIFICAT. Elle estime la
# population de la boule témoin par la densité du nœud partenaire, qui ne borne
# pas cette population ailleurs. L'ablation du 12 août 2026 à `n=1 000`,
# moteur `pipeline`, la réfute : elle ne gagne AUCUN prune — `47 382` sur
# `uniform`, `14 231` sur `eight_clusters`, `53 401` sur `terrain`, identiques
# armée ou non — retire `17 %` de visites témoins, et coûte plus de temps
# qu'elle n'en économise (`19,60` contre `25,76` s sur `uniform`). Elle reste
# donc désarmée par défaut. Cette porte reçoit son exactitude quand elle est
# armée : sauter une tentative ne doit jamais changer Source S.
# LA GARDE A QUITTE LE CHEMIN PARTAGE HOTE/DEVICE. `run_anchor_point` est la
# SEULE fonction que l'hote et le device executent en commun, et l'ajout du
# parametre `density_guard` n'avait ete propage ni a `anchor_source_kernel.cu`
# ni a la qualification device : la cible CUDA opt-in ne compilait plus. La
# reparation est une SUPPRESSION, plus sure qu'une propagation d'une capacite
# que l'ablation pincee disqualifie deja — aucun prune gagne sur les trois
# familles, deux temps sur trois degrades, et une densite locale qui ne borne
# pas la population ailleurs.
#
# Le harnais de diagnostic survit dans le seul moteur CPU `reference`. Le
# combiner au moteur `pipeline` en ferait une option SILENCIEUSEMENT INERTE,
# ce qui est pire qu'une option absente : la porte suivante exige le refus.
add_test(NAME mhgp3v_anchor_garde_densite_exacte COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=5 --verify --density-guard
         --min-supports=5000)
set_tests_properties(mhgp3v_anchor_garde_densite_exacte PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")
# LA PARITE AVEC LA GARDE ARMEE N'A PLUS D'OBJET, ET SON REFUS PREND SA PLACE.
#
# La garde ayant quitte le chemin partage hote/device, seul le moteur de
# reference la porte. Comparer les trente-six compteurs des deux moteurs avec
# elle armee produirait un ecart LEGITIME — ni un accord, ni un defaut, donc le
# pire des deux mondes. L'ancienne porte `mhgp3v_anchor_parite_ledger_garde`
# echouait exactement ainsi : `witness_skipped=1150` d'un cote, zero de l'autre.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_garde_densite_hors_parite
  "incompatibles : la garde n'existe plus" 2
  --points=100 --family=terrain --seed=3 --compare-engines --density-guard)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_garde_densite_hors_pipeline
  "n'existe plus dans le moteur pipeline" 2
  --points=140 --family=terrain --seed=5 --engine=pipeline --density-guard)

# DOMAINE `smax`. Les seuils 10/9/8 et l'enveloppe au neuvieme rang ne sont
# ceux que de `smax=11`. Le contre-exemple recu de l'auditeur publiait 53
# supports faux a `smax=24`; ces portes le gravent aux deux bouts du domaine.
add_test(NAME mhgp3v_anchor_smax24 COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=3 --smax=24 --verify
         --min-supports=24000)
add_test(NAME mhgp3v_anchor_smax24_pipeline COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=3 --smax=24 --engine=pipeline
         --verify --min-supports=24000)
add_test(NAME mhgp3v_anchor_smax20 COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=3 --smax=20 --verify
         --min-supports=15000)
add_test(NAME mhgp3v_anchor_smax_bas COMMAND mhgp3v_anchor_source
         --points=200 --family=uniform --seed=6 --smax=5 --verify
         --min-supports=500)
set_tests_properties(mhgp3v_anchor_smax24 mhgp3v_anchor_smax24_pipeline
                     mhgp3v_anchor_smax20 mhgp3v_anchor_smax_bas
                     PROPERTIES PASS_REGULAR_EXPRESSION "accord=OUI")
# Le mutant qui fige les seuils a leurs valeurs de `smax=11` doit mourir hors
# de ce point, et il SURVIT legitimement a `smax=11` : la porte le tue la ou
# le figement est faux.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_mutant_smax_fige "MUTANT TUE" 4
  --points=140 --family=terrain --seed=3 --smax=24 --verify
  --inject=smax-fixed-thresholds)

# Refus contractuels, tous AVANT le moindre calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_mutant_sans_juge "un mutant sans juge ne prouve rien" 2
  --points=90 --inject=lens-strict)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_suffixe "--points invalide" 2 --points=90junk)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_inconnu "argument inconnu" 2 --points=90 --turbo)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_hors_profil "hors du profil dense u16" 2 --points=70000)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_reject_verify_sans_stockage "--verify exige le stockage" 2
  --points=90 --verify --no-store)
# Planchers : un plancher non atteint est un code 3, jamais un vert silencieux.
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_plancher_supports "plancher de supports" 3
  --points=90 --seed=1 --min-supports=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_anchor_source
  mhgp3v_anchor_plancher_prunes "plancher de prunes" 3
  --points=90 --seed=1 --min-prunes=100000000)

# ---------------------------------------------------------------------------
# CONE CIBLE EXACT : FERMETURE D'ANCRES AVANT `PairId`.
# Specification : audits/AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md
# sections 3 et 7. Ce sujet REPARE le mur que la porte `midball_legacy` grave :
# le front historique cherche ses temoins dans des boules concentriques au
# milieu, alors que le spindle exact en contient aussi pres des extremites.
# Aucune de ces portes ne recoit une complexite ni un SLO.
# ---------------------------------------------------------------------------
# Le juge est une UNITE DE TRADUCTION SEPAREE. Elle n'inclut ni
# `spindle_cone.hpp` ni `anchor_envelope.hpp` : le compilateur garantit ainsi
# qu'aucune ligne du sujet ne s'execute dans le juge. C'est la reparation du
# faux vert `smax`, ou les deux chemins partageaient la meme conversion fautive.
add_executable(mhgp3v_spindle_cone_probe
  prototype/spindle_cone_probe.cpp
  prototype/spindle_cone_oracle.cpp)
target_include_directories(mhgp3v_spindle_cone_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_spindle_cone_probe PRIVATE mhgp)
target_compile_options(mhgp3v_spindle_cone_probe PRIVATE -Wall -Wextra -Werror)

# Les fixtures sont gravees aux coordonnees exactes. Deux d'entre elles sont
# des FRONTIERES (egalite stricte) et deux sont des MUTANTS DETERMINISTES : le
# centre de la boite credite une lane que ses coins refusent, et le produit
# `E2 X2` deborde i64 la ou `4H^2` ne deborde pas.
add_test(NAME mhgp3v_cone_fixtures COMMAND mhgp3v_spindle_cone_probe --fixtures)
set_tests_properties(mhgp3v_cone_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures accord=OUI refutees=0")
# Le selftest juge le juge : les TROIS ecritures (H,E2X2), (H,R) et (g,Q) sont
# comparees sur 400 000 triples, en petites coordonnees puis en pleine largeur
# u16. La porte `ALL` par huit coins est en outre confrontee a l'enumeration du
# RESEAU ENTIER de chaque boite : c'est une equivalence, pas une suffisance.
add_test(NAME mhgp3v_cone_selftest COMMAND mhgp3v_spindle_cone_probe --selftest --seed=1)
set_tests_properties(mhgp3v_cone_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION "selftest accord=OUI")

# Le juge ponctuel enumere toutes les paires ORDONNEES et tous les temoins avec
# la representation (g,Q), qui ne partage aucune quantite intermediaire avec le
# chemin produit. Les planchers interdisent le vert par vacuite : sans eux, un
# certificat qui ne ferme jamais rien passerait.
#
# CES PORTES SONT DES TESTS A CODE, PAS DES REGEX. Un CTest a
# `PASS_REGULAR_EXPRESSION` IGNORE le code de sortie : la porte reste verte
# meme quand le binaire rend 3. Ces quatre portes portaient
# `PASS_REGULAR_EXPRESSION "accord=OUI"`, et leurs cinq planchers etaient donc
# INOPERANTS — la ligne `accord=OUI` du juge est imprimee AVANT que les
# planchers ne soient evalues. L'anti-vacuite que le regex tenait (« le juge
# a-t-il seulement tourne ? ») est desormais dans `--min-accord-juge`, qui
# REFUSE en code 2 si `--verify` est absent.
#
# LES TROIS LANES SONT JUGEES SEPAREMENT. Le sujet ne remplissait qu'un seul
# bitset, et seulement quand q2, q3 ET q4 etaient toutes mortes ; le juge
# faisait le meme ET. Une fermeture q3 seule ou q4 seule pouvait donc etre
# fausse sans etre observee, alors que l'aval consomme les lanes separement.
# Chaque porte porte desormais un plancher NON NUL par lane : la conjonction ne
# les remplace pas, puisqu'elle est vide des qu'une seule lane ne ferme rien.
add_test(NAME mhgp3v_cone_juge_uniform COMMAND mhgp3v_spindle_cone_probe
         --points=300 --family=uniform --seed=3 --bank=48 --verify
         # `--min-none-q4` valait 200 000 quand `witness_none_q4` comptait les
         # HITS : la meme refutation etait recomptee a chaque visite du
         # sous-arbre, et le plancher pouvait etre franchi sans qu'une seule
         # refutation NOUVELLE ait eu lieu. En transitions la mesure vaut
         # 170 160 ; le plancher descend donc a 150 000, qui est un vrai
         # plancher et non la mesure elle-meme.
         --min-prunes=1200 --min-mass-closed=8000 --min-bank=48
         --min-none-q4=150000 --min-credits-herites=100000 --min-accord-juge=8000
         --min-accord-q2=40000 --min-accord-q3=10000 --min-accord-q4=8000)
add_test(NAME mhgp3v_cone_juge_clusters COMMAND mhgp3v_spindle_cone_probe
         --points=300 --family=eight_clusters --seed=3 --bank=48 --verify
         --min-prunes=1500 --min-mass-closed=18000 --min-accord-juge=18000
         --min-accord-q2=50000 --min-accord-q3=20000 --min-accord-q4=18000)
add_test(NAME mhgp3v_cone_juge_terrain COMMAND mhgp3v_spindle_cone_probe
         --points=300 --family=terrain --seed=5 --bank=48 --verify
         --min-prunes=3000 --min-mass-closed=40000 --min-accord-juge=40000
         --min-accord-q2=60000 --min-accord-q3=45000 --min-accord-q4=40000)
add_test(NAME mhgp3v_cone_juge_multiecho COMMAND mhgp3v_spindle_cone_probe
         --points=300 --family=scanline_overlap_multiecho --seed=2 --bank=48
         --verify --min-prunes=2500 --min-mass-closed=50000 --min-accord-juge=50000
         --min-accord-q2=65000 --min-accord-q3=55000 --min-accord-q4=50000)
# Le plancher de verdict sans son verdict est refuse AVANT tout calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_plancher_juge_sans_juge "plancher d'accord du juge exige --verify" 2
  --points=200 --min-accord-juge=1)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_plancher_lane_sans_juge "plancher d'accord par lane exige --verify" 2
  --points=200 --min-accord-q3=1)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_plancher_lane_q4 "plancher d'accord q4" 3
  --points=300 --family=uniform --seed=3 --bank=48 --verify --min-accord-q4=100000000)

#
# LE DOMAINE `smax` EST FERME AVANT TOUT CAST.
#
# `--smax=9223372036854775807` fermait `380/380` paires ordonnees sans un seul
# test temoin, avec code zero et `accord=OUI` du juge comme de la permutation :
# le sujet ET le juge castaient la valeur en `int`, les seuils devenaient
# negatifs des deux cotes, et l'accord etait donc parfait sur une contradiction.
# C'etait une contradiction mathematique exacte, pas une question de coût.
#
# Deux reparations independantes sont gravees ici. D'abord ces cinq refus, tous
# anterieurs au moindre calcul. Ensuite, et surtout, le juge a change d'unite de
# traduction : il n'inclut plus `anchor_envelope.hpp` et recalcule ses seuils en
# `long long`. Un defaut de conversion du sujet ne peut donc plus s'y produire a
# l'identique.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_smax_llong_max "--smax hors du domaine d'enveloppe" 2
  --points=20 --family=uniform --seed=3 --bank=4 --verify --smax=9223372036854775807)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_smax_int_max_plus_un "--smax hors du domaine d'enveloppe" 2
  --points=20 --smax=2147483648)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_smax_borne_plus_un "--smax hors du domaine d'enveloppe" 2
  --points=20 --smax=35)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_smax_trois "--smax hors du domaine d'enveloppe" 2
  --points=20 --smax=3)
# `strtoll` SATURE a LLONG_MAX et ne signale que par `errno`. Sans ce test, une
# valeur a vingt chiffres serait devenue LLONG_MAX, puis aurait pu franchir une
# borne par le seul hasard de la saturation.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_smax_erange "--smax invalide" 2
  --points=20 --smax=99999999999999999999)

# LA CARDINALITE DEMANDEE N'ETAIT PAS GARANTIE. `--points=100 --coord=2` rendait
# huit points et code zero : toutes les identites etaient alors verifiees sur le
# mauvais univers, et le reçu annoncait `n=8` sans que rien ne le refuse. Le
# reçu publie desormais aussi un digest du nuage.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_cardinalite "cardinalite demandee" 2
  --points=100 --coord=2 --verify)

# EQUIVARIANCE PAR PERMUTATION, sur l'ENSEMBLE DES PAIRES FERMEES et non sur un
# compteur. Elle a effectivement refute une premiere banque ordonnee par
# (d2, PointId) : 60 paires divergentes sur `uniform`, 19 sur `terrain`. La
# banque est desormais ordonnee par (d2, cle Morton), donc fonction du seul
# nuage. Ne pas affaiblir cette porte en un test de compteurs.
# Meme remarque : le code de sortie fait foi. Le binaire rend 1 des qu'une
# paire fermee depend de la numerotation ; `--min-accord-permutation` exige en
# plus qu'une MASSE de paires ait effectivement ete comparee des deux cotes, et
# refuse en code 2 si `--permute` est absent.
add_test(NAME mhgp3v_cone_permutation_uniform COMMAND mhgp3v_spindle_cone_probe
         --points=400 --family=uniform --seed=3 --bank=64 --permute
         --min-prunes=3000 --min-accord-permutation=25000)
add_test(NAME mhgp3v_cone_permutation_terrain COMMAND mhgp3v_spindle_cone_probe
         --points=400 --family=terrain --seed=3 --bank=64 --permute
         --min-prunes=5000 --min-accord-permutation=100000)
add_test(NAME mhgp3v_cone_permutation_multiecho COMMAND mhgp3v_spindle_cone_probe
         --points=400 --family=scanline_overlap_multiecho --seed=3 --bank=64
         --permute --min-prunes=4000 --min-accord-permutation=100000)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_plancher_permutation "plancher d'accord de permutation exige --permute" 2
  --points=200 --min-accord-permutation=1)

# LE MUR AMAS, VU DE L'AUTRE COTE. La porte `midball_legacy` ci-dessus exige
# que le certificat historique ne ferme RIEN sur `eight_clusters`. Celle-ci
# exige que le cone exact y ferme beaucoup, sur la meme famille et la meme
# graine. Les deux ensemble sont la mesure du gain ; aucune des deux seule ne
# l'est.
#
# CETTE PORTE ETAIT MORTE. Elle exigeait `PASS_REGULAR_EXPRESSION
# "prunes=2[0-9]{4}"`, or le moteur de regex de CMake N'IMPLEMENTE PAS le
# quantificateur `{n}` : les trois caracteres `{`, `4`, `}` y sont litteraux.
# La porte ne pouvait donc JAMAIS passer, quelle que soit la valeur mesuree
# (`prunes=23803` au pin). Elle imposait de surcroit une borne SUPERIEURE, que
# toute amelioration du certificat aurait cassee. Les deux planchers ci-dessous
# disent la meme chose dans le bon sens, et le code de sortie les fait respecter.
add_test(NAME mhgp3v_cone_mur_amas_ferme COMMAND mhgp3v_spindle_cone_probe
         --points=500 --family=eight_clusters --seed=3 --bank=96 --leaf=2
         --min-prunes=20000 --min-mass-closed=120000)

# Mutants : chacun casse UNE decision exacte. Un mutant qui ne change RIEN est
# refuse en code 3 — une porte vacueuse ne prouve pas plus qu'un mutant absent.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_positivite "MUTANT TUE .juge." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-no-positivity)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_coefficients "MUTANT TUE .juge." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-swap-coeff)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_centre_seul "MUTANT TUE .juge." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-center-only)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_slot_double "MUTANT TUE .juge." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-duplicate-slot)
# L'egalite ne se rencontre pas dans un nuage generique : elle exige les
# coplanarites massives de `terrain`. C'est la famille qui est la porte.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_egalite "MUTANT TUE .juge." 4
  --points=400 --family=terrain --seed=5 --bank=48 --verify
  --inject=cone-accept-equality)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_coin_omis "MUTANT TUE .juge." 4
  --points=400 --family=scanline_overlap_multiecho --seed=5 --bank=48 --verify
  --inject=cone-seven-corners)
# La largeur ne se voit qu'a pleine echelle u16 : `E2 X2` ne depasse 2^63 que
# si les distances approchent 65535. Le mutant SURVIT legitimement a petites
# coordonnees, et la porte suivante exige ce refus en code 3.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_largeur_i64 "MUTANT TUE" 4
  --points=400 --coord=65535 --family=uniform --seed=7 --bank=48 --verify
  --inject=cone-narrow-i64)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_vacueux "MUTANT SURVIVANT" 3
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-narrow-i64)
# La refutation NONE est fail-open : un MAJORANT de R a la place du minorant ne
# ferme jamais une paire vivante, il PERD du travail utile. Le juge ne peut
# donc pas le tuer — seul le ledger le peut, et c'est la forme correcte de
# cette porte.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_none_majorant "MUTANT TUE .ledger." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-none-uses-rmax)
# L'HERITAGE SANS RECREDIT EST L'INVARIANT CENTRAL DU PARCOURS : un cone qui
# contient la boite du parent contient celle de tous ses enfants, donc un credit
# ne se reteste ni ne se recompte jamais. Le mutant existait et tuait, mais
# aucune porte ne l'exercait — un mutant sans porte ne prouve rien.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_mutant_heritage "MUTANT TUE .juge." 4
  --points=300 --family=uniform --seed=3 --bank=48 --verify
  --inject=cone-ignore-inherited)

# Refus contractuels, tous AVANT le moindre calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_mutant_sans_juge "un mutant sans juge ne prouve rien" 2
  --points=200 --inject=cone-center-only)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_inconnu "argument inconnu" 2 --points=200 --turbo)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_banque "--bank hors bornes" 2 --points=200 --bank=300)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_juge_non_borne "le juge ponctuel est borne" 2
  --points=800 --verify)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_reject_suffixe "--points invalide" 2 --points=200junk)
# Planchers : un plancher non atteint est un code 3, jamais un vert silencieux.
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_plancher_prunes "plancher de prunes" 3
  --points=200 --family=uniform --seed=1 --min-prunes=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_plancher_masse "plancher de masse fermee" 3
  --points=200 --family=uniform --seed=1 --min-mass-closed=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_plancher_credits "plancher de credits herites" 3
  --points=200 --family=uniform --seed=1 --min-credits-herites=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_spindle_cone_probe
  mhgp3v_cone_plancher_refutations "plancher de refutations q4" 3
  --points=200 --family=uniform --seed=1 --min-none-q4=100000000)

# ---------------------------------------------------------------------------
# `SOC64` / `CORNER512` : LE CERTIFICAT CORRELE D'UN RECTANGLE.
#
# Specification : PROPOSITION.md section 6.2 ; ordre impose par
# audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md section 4.1.
#
# Le classifieur `JungSpindleRect-v0` combine des EXTREMA SEPARES de D, V et T
# sur le meme rectangle et ne gagne que trois centiemes de point. La question
# ouverte etait de savoir si cette perte vient de la DECORRELATION des extrema
# ou d'une absence reelle de coeur commun. Ces portes jugent le premier terme.
#
# L'autorite est l'ENUMERATION DU RESEAU ENTIER de A x B x C, bornee par un cap
# explicite. La chaine exigee est un ORDRE, jamais une egalite :
#   soc64 <= corner512 <= exhaustif.
# Une porte qui exigerait l'egalite refuserait le fail-open, qui est la
# propriete voulue.
add_executable(mhgp3v_soc64_probe prototype/soc64_probe.cpp)
target_include_directories(mhgp3v_soc64_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_soc64_probe PRIVATE mhgp)
target_compile_options(mhgp3v_soc64_probe PRIVATE -Wall -Wextra -Werror)

# Les huit fixtures sont gravees aux coordonnees exactes. Deux d'entre elles
# portent le resultat mathematique lui-meme : `soc64-axial` ferme q4 la ou les
# extrema scalaires n'atteignent meme pas q3, et `soc64-faux-echec-relaxe`
# montre le produit relaxe SOUS-ESTIMANT un rectangle reellement q4 — donc
# UNKNOWN, jamais NONE. L'equivariance par les 48 isometries du reseau est
# verifiee sur chacune.
add_test(NAME mhgp3v_soc64_fixtures COMMAND mhgp3v_soc64_probe --fixtures)
set_tests_properties(mhgp3v_soc64_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures accord=OUI refutees=0")

# Le selftest tire des rectangles dans deux regimes : petites boites jugees par
# enumeration complete du reseau, grandes boites jugees par echantillonnage de
# triples interieurs. Les planchers interdisent le vert par vacuite, et
# `--min-perte-relaxe` exige d'avoir OBSERVE le fail-open — donc d'avoir mesure
# le prix de la relaxation, et pas seulement ses succes.
add_test(NAME mhgp3v_soc64_selftest COMMAND mhgp3v_soc64_probe
         --selftest --seed=1 --rounds=20000
         --min-soc64-all=500 --min-corner512-all=500 --min-exhaustifs=5000
         --min-gain-scalaire=20 --min-perte-relaxe=1)
add_test(NAME mhgp3v_soc64_selftest_serre COMMAND mhgp3v_soc64_probe
         --selftest --seed=7 --rounds=20000 --span=4096
         --min-soc64-all=500 --min-gain-scalaire=100 --min-perte-relaxe=20)

# LES SEPT MUTANTS, ET LA FIXTURE QUI TUE CHACUN.
#
# Le code 4 signifie « mutant SURVIVANT » : la porte est donc a code ZERO, et
# c'est le binaire qui refuse quand la faute passe inapercue. `soc-narrow-i64`
# merite une mention : sa premiere fixture large etait COLINEAIRE, donc H^2 et
# E X y enroulaient du meme cote et l'ordre survivait par accident. Il a fallu
# un couple presque orthogonal — E X = 3,69e19 contre 3H^2 = 1,29e10 — pour que
# l'enroulement change le verdict. Une fixture large ne suffit pas : il faut
# une fixture large ou l'enroulement DECIDE.
foreach(_soc_mutant IN ITEMS
        soc-no-positivity soc-accept-equality soc-swap-coeff soc-narrow-i64
        soc-diagonal-only soc-seven-targets soc-minkowski-same)
  string(REPLACE "-" "_" _soc_nom "${_soc_mutant}")
  add_test(NAME mhgp3v_soc64_mutant_${_soc_nom}
           COMMAND mhgp3v_soc64_probe --fixtures --inject=${_soc_mutant})
  set_tests_properties(mhgp3v_soc64_mutant_${_soc_nom} PROPERTIES
    PASS_REGULAR_EXPRESSION "mutant ${_soc_mutant} mort sur")
endforeach()

# Refus avant tout calcul, et planchers a code exact.
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_reject_sans_mode "choisir --fixtures ou --selftest" 2 --seed=1)
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_reject_span "--span hors du profil u16" 2 --selftest --span=70000)
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_reject_inject "--inject inconnu" 2 --fixtures --inject=soc-inconnu)
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_reject_plancher_sans_campagne
  "les planchers de couverture exigent --selftest" 2 --fixtures --min-soc64-all=1)
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_plancher_perte "plancher perte_relaxe non atteint" 3
  --selftest --seed=1 --rounds=200 --min-perte-relaxe=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_soc64_probe
  mhgp3v_soc64_plancher_gain "plancher gain_sur_scalaire non atteint" 3
  --selftest --seed=1 --rounds=200 --min-gain-scalaire=100000000)

# ---------------------------------------------------------------------------
# ANALYSEUR DE RAMPE : LA PORTE DE CROISSANCE, EN ARITHMETIQUE EXACTE.
#
# La rampe G4 du pin `35fcea8` ne calculait aucune pente : `--max-slope=9`
# etait inerte parce que chaque processus ne recevait qu'UNE taille, et le
# verdict `1,7` a ete pose a la main dans une note posterieure. Cet analyseur
# decide `pente <= p/q` par la comparaison ENTIERE `m2^q n1^p <= m1^q n2^p` :
# aucun logarithme flottant n'est evalue, et une pente exactement au seuil est
# decidee sans ambiguite.
if(Python3_FOUND)
  add_test(NAME mhgp3v_rampe_fixtures
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_rampe_pentes.py --fixtures)
  set_tests_properties(mhgp3v_rampe_fixtures PROPERTIES
    PASS_REGULAR_EXPRESSION "fixtures accord=OUI refutees=0")
  # LE RECU REEL EST UNE FIXTURE. Le seuil `1,7` de la note de route est
  # applique au recu grave du pin `35fcea8` : la porte DOIT rendre 3, parce que
  # `eight_clusters` la depasse sur ses trois intervalles et que trois fenetres
  # `terrain` ont du `pending`. Si un jour ce test devient vert, c'est que le
  # recu a change, et il faudra le dire.
  add_test(NAME mhgp3v_rampe_recu_35fcea8_refuse
           COMMAND ${CMAKE_COMMAND}
                   "-DPROGRAM=${Python3_EXECUTABLE}"
                   "-DEXPECTED=eight_clusters/r4"
                   "-DEXPECTED_CODE=3"
                   "-DARGUMENTS=${CMAKE_CURRENT_SOURCE_DIR}/audits/check_rampe_pentes.py;--repertoire=${CMAKE_CURRENT_SOURCE_DIR}/receipts/rampe_raffinement_g4_20260813;--lane=4;--metrique=sum_E;--max-pente=17/10;--exige-code-zero;--exige-pending-zero"
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
  # Une matrice incomplete est un refus AVANT tout calcul, pas une moyenne sur
  # ce qui est present.
  add_test(NAME mhgp3v_rampe_matrice_incomplete
           COMMAND ${CMAKE_COMMAND}
                   "-DPROGRAM=${Python3_EXECUTABLE}"
                   "-DEXPECTED=matrice incomplete"
                   "-DEXPECTED_CODE=2"
                   "-DARGUMENTS=${CMAKE_CURRENT_SOURCE_DIR}/audits/check_rampe_pentes.py;--repertoire=${CMAKE_CURRENT_SOURCE_DIR}/receipts/rampe_raffinement_g4_20260813;--familles=uniform,terrain;--profondeurs=0,4"
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
endif()

# ---------------------------------------------------------------------------
# GARDE-FOU DE CONFIGURATION : PAS DE QUANTIFICATEUR `{n}` DANS UNE REGEX CTEST.
#
# Le moteur de regex de CMake (`cmsys::RegularExpression`) implemente `^ $ . []
# * + ? | ()` et RIEN D'AUTRE. Les accolades y sont des caracteres litteraux.
# Une porte ecrite `PASS_REGULAR_EXPRESSION "prunes=2[0-9]{4}"` ne peut donc
# JAMAIS passer : elle cherche litteralement la chaine `{4}`. Le defaut est
# muet — la porte existe, elle est comptee dans l'inventaire des CTests, et
# elle echoue pour une raison qui n'a aucun rapport avec la mesure.
#
# Cette contradiction est desormais une fixture permanente : elle est refusee a
# la CONFIGURATION, avant qu'un seul test ne soit enregistre.
file(READ "${CMAKE_CURRENT_LIST_FILE}" _mhgp3v_self)
string(REGEX MATCHALL "[^\n]*_REGULAR_EXPRESSION[^\n]*" _mhgp3v_regex_lines "${_mhgp3v_self}")
set(_mhgp3v_bad_regex "")
foreach(_line IN LISTS _mhgp3v_regex_lines)
  string(STRIP "${_line}" _line)
  # Les lignes de commentaire sont exclues : ce bloc-ci cite le motif fautif.
  if(NOT _line MATCHES "^#" AND _line MATCHES "{[0-9]")
    list(APPEND _mhgp3v_bad_regex "${_line}")
  endif()
endforeach()
if(_mhgp3v_bad_regex)
  string(REPLACE ";" "\n  " _mhgp3v_bad_regex_txt "${_mhgp3v_bad_regex}")
  message(FATAL_ERROR
    "quantificateur `{n}` dans une regex CTest : CMake le lit LITTERALEMENT et "
    "la porte ne peut jamais passer. Developper la repetition, ou porter la "
    "mesure dans un plancher `--min-*` du binaire.\n  ${_mhgp3v_bad_regex_txt}")
endif()
unset(_mhgp3v_self)
unset(_mhgp3v_regex_lines)
unset(_mhgp3v_bad_regex)

# ---------------------------------------------------------------------------
# FENETRE k-NN CERTIFIEE : LE CERTIFICAT, ET SEULEMENT LUI.
#
# Specification : audits/NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md section
# 4.1, corrigee par audits/AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md
# section 1, et ordre de reprise section 8 points 2 a 4.
#
# CE SUJET N'EST PAS UN PRODUCTEUR. Son enumerateur local forme tous les
# sous-ensembles de taille 2, 3 et 4 de la fenetre, soit M + C(M,2) + C(M,3)
# propositions par ancre — 349 632 a M=128, donc plus de dix-sept milliards a
# 50 000 points. L'audit du deblocage collectif chiffre exactement ce mur. Aucun
# temps, aucune pente et aucun compteur de travail ne sont donc revendiques ici.
#
# CE QUI EST REVENDIQUE EST UNE IDENTITE, verifiee dans LES DEUX SENS contre un
# juge rationnel exhaustif, sur les identites completes et non sur des comptes :
#
#   { S certifie par une fenetre }  ==  { S global tel que 4R^2 < delta_out(a)^2 }
#
# Le juge emploie l'elimination de Gauss rationnelle de `oracle/exact_geometry.hpp`;
# le sujet emploie des determinants entiers de Cramer et `mhgp::BigInt<4>`. Les
# deux representations ne partagent aucune quantite intermediaire.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_window_source_probe prototype/window_source_probe.cpp)
target_include_directories(mhgp3v_window_source_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_window_source_probe PRIVATE mhgp)
target_compile_options(mhgp3v_window_source_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_window_fixtures COMMAND mhgp3v_window_source_probe --fixtures)
set_tests_properties(mhgp3v_window_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures accord=OUI refutees=0")

# LES QUATRE FAMILLES CONTRACTUELLES. Les planchers `--min-jamais-proposes`
# sont les plus importants : ils exigent que le juge trouve des supports globaux
# qu'AUCUNE fenetre n'a jamais proposes. Sans eux, la porte laisserait croire que
# « residuel = candidats refuses », ce que l'audit refuse explicitement. Sur
# `eight_clusters` le rapport est ecrasant — 1 277 jamais proposes contre 156
# certifiables — et c'est precisement le mur des amas, vu du bon cote.
add_test(NAME mhgp3v_window_juge_uniform COMMAND mhgp3v_window_source_probe
         --points=24 --window=8 --family=uniform --seed=3 --judge
         --min-certifies=300 --min-non-certifies=30 --min-jamais-proposes=700
         --min-shell=3 --min-jung=150 --min-chemin-exact=200)
add_test(NAME mhgp3v_window_juge_terrain COMMAND mhgp3v_window_source_probe
         --points=24 --window=8 --family=terrain --seed=3 --judge
         --min-certifies=300 --min-jamais-proposes=250 --min-shell=3 --min-jung=150)
add_test(NAME mhgp3v_window_juge_clusters COMMAND mhgp3v_window_source_probe
         --points=24 --window=8 --family=eight_clusters --seed=3 --judge
         --min-certifies=200 --min-jamais-proposes=1000 --min-chemin-exact=200)
add_test(NAME mhgp3v_window_juge_multiecho COMMAND mhgp3v_window_source_probe
         --points=24 --window=8 --family=scanline_overlap_multiecho --seed=3 --judge
         --min-certifies=250 --min-shell=5 --min-jamais-proposes=100 --min-jung=200)

# MUTANTS. Chacun casse UNE decision exacte et doit perdre une identite.
#
# `window-accept-equality` a EFFECTIVEMENT trouve un trou dans une premiere
# version de ce juge : il certifiait quatre supports au lieu d'un, tous
# globalement valides, donc sans aucun faux positif ni identite fausse. Le juge
# ne comparait alors que deux inclusions sur trois. La troisieme —
# `certifie inclus dans certifiable` — a ete ajoutee pour lui : sans elle, une
# SUR-certification passe inapercue des que le census tombe juste par hasard, et
# c'est le cas le plus dangereux, puisque rien ne prouve alors que la fenetre
# contenait la boule.
#
# L'egalite `4R^2 = delta_out^2` est de mesure nulle sur un nuage generique : le
# mutant y survit legitimement. Le nuage grave `--cloud-egalite` la force aux
# DEUX endpoints a la fois, sans quoi l'autre endpoint certifierait le support et
# la divergence disparaitrait.
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_egalite "MUTANT TUE" 4
  --cloud-egalite --window=1 --judge --inject=window-accept-equality)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_coupure_incluse "MUTANT TUE" 4
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --inject=window-cut-at-included)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_positivite "MUTANT TUE" 4
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --inject=window-skip-positivity)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_shell_interieur "MUTANT TUE" 4
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --inject=window-shell-as-interior)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_jung_seul "MUTANT TUE" 4
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --inject=window-jung-only)
# LA LARGEUR NE SE VOIT QU'A PLEINE ECHELLE u16. A `coord=27` le mutant i64
# survit legitimement : rien ne deborde. La porte exige donc `--coord=65535`.
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_largeur_i64 "MUTANT TUE" 4
  --points=20 --window=8 --family=uniform --coord=65535 --seed=3 --judge
  --inject=window-narrow-i64)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_mutant_vacueux "MUTANT SURVIVANT" 3
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --inject=window-narrow-i64)

# Refus contractuels, tous AVANT le moindre calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_mutant_sans_juge "un mutant sans juge ne prouve rien" 2
  --points=20 --window=8 --inject=window-skip-positivity)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_juge_non_borne "le juge rationnel exhaustif est borne" 2
  --points=60 --window=8 --judge)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_smax "--smax hors du domaine d'enveloppe" 2
  --points=20 --window=8 --smax=9223372036854775807)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_fenetre "--window hors bornes" 2 --points=20 --window=0)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_inconnu "argument inconnu" 2 --points=20 --turbo)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_cardinalite "cardinalite demandee" 2 --points=100 --coord=2 --window=8)
# Le plancher de supports jamais proposes sans juge est refuse : c'est le juge
# seul qui les voit.
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_reject_plancher_sans_juge "supports jamais proposes exige --judge" 2
  --points=20 --window=8 --min-jamais-proposes=1)
# Planchers : un plancher non atteint est un code 3, jamais un vert silencieux.
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_plancher_certifies "plancher de supports certifies" 3
  --points=20 --window=8 --family=uniform --seed=3 --min-certifies=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_window_source_probe
  mhgp3v_window_plancher_jamais_proposes "plancher de supports jamais proposes" 3
  --points=20 --window=8 --family=uniform --seed=3 --judge
  --min-jamais-proposes=100000000)




# LE MUTANT `cible-temoin` EST RETIRE, ET C'EST UNE RETRACTATION.
#
# Il paraissait tuer — 189 desaccords sur un differentiel cible. Le contre-audit
# a montre qu'il lisait une case HORS du prefixe top-h, laissee par une ancre
# precedente : il modelisait un temoin gratuit de hauteur arbitraire, pas la
# cible. Ses desaccords venaient de la memoire, pas de la faute visee.
#
# Correctement modelise — en INSERANT `tau_d` dans la liste triee — il survit,
# et c'est demontrable : la fermeture exige `tau_d >= c tau_h` avec `c >= 3/2`,
# donc si `tau_d` etait le `h`-ieme il faudrait `tau_d >= c tau_d`. La cible ne
# peut jamais etre creditee. La faute est SAINE, et un mutant sain n'est pas un
# mutant.
#
# C'est la quatrieme fois dans ce chantier qu'une porte de mutant visait un
# non-defaut. Un mutant qui meurt pour la mauvaise raison est pire qu'un mutant
# absent : il donne une confiance sans objet.













# LA MARCHE DE JARVIS EST RETIREE ET REMPLACEE PAR ANDREW EXACT.
#
# Le contre-audit l'a refutee sur quatre points, tous par fixtures
# deterministes, et le plus grave produisait une FAUSSE FERMETURE DANS LE CHEMIN
# SAIN. Son departage de colineaires employait `det(a x i, a, next)`, un
# predicat de DEGRE QUATRE : a `M=65535` les determinants valent environ
# `7,38e19` et le calcul `i64` deborde en rendant `-4503496549203964`, donc le
# verdict est INVERSE sous le profil contractuel. Sa branche `h==2` acceptait la
# seule coplanarite, sa marche pouvait ne jamais revenir au pivot, et une garde
# de capacite authentifiait alors un faux polygone.
#
# Mon selftest aleatoire de 4 000 pools passait ; l'exhaustif borne de l'audit
# trouvait `51` faux positifs et `294` faux negatifs. Le differentiel est donc
# desormais EXHAUSTIF — `37 752` pools enumeres contre l'oracle de Caratheodory,
# zero desaccord — et quatre contre-fixtures deterministes sont gravees :
# inclusion de rang deux sur la seule coplanarite, directions projectives
# dupliquees prises pour un segment, couverture legitime par les rayons, et la
# fausse fermeture nominale a seize directions identiques.







# --- Front de rectangles : intervalle exact de H, verdicts ALL/NONE/MIXED,
# recherche de temoins au meilleur d'abord a budget borne.
add_executable(mhgp3v_rect_front_probe
  prototype/rect_front_probe.cpp
)
target_include_directories(mhgp3v_rect_front_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_rect_front_probe PRIVATE mhgp)
target_compile_options(mhgp3v_rect_front_probe PRIVATE -Wall -Wextra -Werror)

# Le juge exhaustif doit valider l'intervalle, et la campagne doit exercer les
# trois verdicts : planchers contre le vert-par-vacuite.
add_test(NAME mhgp3v_rect_front_intervalle
  COMMAND mhgp3v_rect_front_probe --family=uniform --points=2000,8000 --lane=0
          --budget=24 --selftest=20000 --min-all=1000 --min-none=1000 --min-mixed=1000
          --min-closed-pct=70)

# FIXTURES GRAVEES aux coordonnees exactes : sommet interieur, sommet hors
# boite, reseau entier contre continu, milieu impair, extremes u16. Et le
# MUTANT COLINEAIRE : un nuage porte par une droite n'a aucun support q3/q4,
# donc aucune issue ne peut y etre etiquetee POSITIVE hors q2.
# Les fixtures portent aussi le JUGE EXHAUSTIF DE L'INTERVALLE DU SCORE `S`, dont
# depend entierement le `NONE` du certificat central : un `Smin` surestime
# prononcerait un faux `NONE`.
add_test(NAME mhgp3v_rect_front_fixtures COMMAND mhgp3v_rect_front_probe --fixtures)
set_tests_properties(mhgp3v_rect_front_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures accord=OUI")

# Les deux fautes que j'ai reellement ecrites doivent mourir devant le juge.
mhgp3v_add_expected_code_test_for(mhgp3v_rect_front_probe
  mhgp3v_rect_front_mutant_max_coins "mutant_killed=1" 4
  --family=uniform --points=2000 --selftest=20000 --inject=max-par-coins)
mhgp3v_add_expected_code_test_for(mhgp3v_rect_front_probe
  mhgp3v_rect_front_mutant_sommet_non_ecrete "mutant_killed=1" 4
  --family=uniform --points=2000 --selftest=20000 --inject=sommet-non-ecrete)

# LE COEUR COMMUN EST JUGE POINT PAR POINT. Chaque point compte dans le coeur
# doit satisfaire le predicat EXACT du rectangle, teste par une autre ecriture
# — boite degeneree et intervalle exact — et non par la sphere qui vient de
# decider. Plancher de mille points juges contre le vert-par-vacuite.
add_test(NAME mhgp3v_rect_front_coeur_juge
  COMMAND mhgp3v_rect_front_probe --family=terrain --points=4000
          --budget-depth=4 --selftest=0 --verify-core)
add_test(NAME mhgp3v_rect_front_coeur_juge_q4
  COMMAND mhgp3v_rect_front_probe --family=terrain --points=4000 --lane=2
          --budget-depth=4 --selftest=0 --verify-core)
set_tests_properties(mhgp3v_rect_front_coeur_juge_q4 PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0")
set_tests_properties(mhgp3v_rect_front_coeur_juge PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0")

# ORACLE EXHAUSTIF DES VERDICTS `ALL`. Un echantillonnage peut FALSIFIER une
# faute frequente ; il ne prouve jamais un universel. Sur un petit nuage, TOUS
# les couples de nœuds disjoints sont classes et la TOTALITE des triples de
# chaque verdict `ALL` est enumeree, dans une ecriture qui n'emprunte ni
# `Lambda`, ni le coeur central, ni les bornes de boites.
add_test(NAME mhgp3v_rect_front_oracle_all COMMAND mhgp3v_rect_front_probe --oracle-all=320)
set_tests_properties(mhgp3v_rect_front_oracle_all PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle_all accord=OUI")

# L'echantillonnage RESTE, mais comme diagnostic de grande taille seulement.
# LES VERDICTS `ALL` SONT JUGES PAR ECHANTILLONNAGE DE TRIPLES REELS. Un noeud
# declare ALL affirme que TOUT z de C est temoin universel de TOUTE paire de
# A x B ; le juge tire des triples de points EXISTANTS et evalue le predicat
# ponctuel exact, sans emprunter ni Lambda, ni le coeur central, ni les bornes
# de boites. Les trois lanes sont exercees.
add_test(NAME mhgp3v_rect_front_all_juge_q2
  COMMAND mhgp3v_rect_front_probe --family=terrain --points=8000 --lane=0
          --budget-depth=4 --selftest=0 --verify-all)
set_tests_properties(mhgp3v_rect_front_all_juge_q2 PROPERTIES
  PASS_REGULAR_EXPRESSION "juge_all triples=[0-9]+ desaccords=0")
add_test(NAME mhgp3v_rect_front_all_juge_q3
  COMMAND mhgp3v_rect_front_probe --family=terrain --points=8000 --lane=1
          --budget-depth=4 --selftest=0 --verify-all)
set_tests_properties(mhgp3v_rect_front_all_juge_q3 PROPERTIES
  PASS_REGULAR_EXPRESSION "juge_all triples=[0-9]+ desaccords=0")
add_test(NAME mhgp3v_rect_front_all_juge_q4
  COMMAND mhgp3v_rect_front_probe --family=terrain --points=8000 --lane=2
          --budget-depth=4 --selftest=0 --verify-all)
set_tests_properties(mhgp3v_rect_front_all_juge_q4 PROPERTIES
  PASS_REGULAR_EXPRESSION "juge_all triples=[0-9]+ desaccords=0")

# Le rayon double — la faute que j'avais ecrite — doit mourir devant ce juge.
mhgp3v_add_expected_code_test_for(mhgp3v_rect_front_probe
  mhgp3v_rect_front_mutant_coeur_trop_grand "DESACCORD DU JUGE" 1
  --family=terrain --points=4000 --budget-depth=4 --selftest=0 --inject=coeur-trop-grand)

# Une campagne aux parametres hors domaine est refusee AVANT tout calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_rect_front_probe
  mhgp3v_rect_front_refus_domaine "REFUS: lane" 2
  --family=uniform --points=2000 --lane=9)

# Le PLANCHER doit mordre : une campagne qui ne ferme pas assez est refusee en
# code 3, sinon la porte serait verte par vacuite.
mhgp3v_add_expected_code_test_for(mhgp3v_rect_front_probe
  mhgp3v_rect_front_plancher_mord "PLANCHER" 3
  --family=uniform --points=2000 --lane=0 --budget=1 --selftest=0 --min-closed-pct=90)

# --- WspdFrontLowerBound-v1 : la partition entiere construite AVANT toute
# geometrie, puis une seule classification par RectId terminal avec un masque
# de lanes commun aux trois seuils.
add_executable(mhgp3v_wspd_front_probe prototype/wspd_front_probe.cpp)
target_include_directories(mhgp3v_wspd_front_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_wspd_front_probe PRIVATE mhgp)
target_compile_options(mhgp3v_wspd_front_probe PRIVATE -Wall -Wextra -Werror)

# LA MULTIPLICITE EST LA PREMIERE PORTE. La somme des masses ne prouve rien —
# une omission et un doublon de meme masse se compensent. L'oracle developpe
# tous les records a n <= 64 et exige multiplicite EXACTEMENT UN par PairId.
add_test(NAME mhgp3v_wspd_front_multiplicite
  COMMAND mhgp3v_wspd_front_probe --family=uniform --points=48,64 --coord=512 --oracle)
set_tests_properties(mhgp3v_wspd_front_multiplicite PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle accord=OUI")

# UNE OMISSION ET UN DOUBLON DE MASSES EGALES SE COMPENSENT dans toute somme ;
# seuls le cardinal des cles et leur multiplicite les distinguent. Les deux
# doivent mourir SEPAREMENT.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_front_probe
  mhgp3v_wspd_front_mutant_omission "INVARIANT VIOLE" 3
  --family=uniform --points=64 --coord=512 --oracle --inject=omission)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_front_probe
  mhgp3v_wspd_front_mutant_doublon "INVARIANT VIOLE" 3
  --family=uniform --points=64 --coord=512 --oracle --inject=doublon)

# `leaf > 1` omet les self-blocs feuille : refuse AVANT tout calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_front_probe
  mhgp3v_wspd_front_refus_leaf "leaf doit valoir 1" 2
  --family=uniform --points=2000 --leaf=4)

mhgp3v_add_expected_code_test_for(mhgp3v_wspd_front_probe
  mhgp3v_wspd_front_refus_oracle_trop_grand "l'oracle de multiplicite exige" 2
  --family=uniform --points=4000 --oracle)

# --- WSPD par vagues sur octree comprime de Morton : le prototype dont le
# kernel GPU sera la transcription directe. Aucune recursion, aucune pile.
add_executable(mhgp3v_wspd_wavefront_probe prototype/wspd_wavefront_probe.cpp
  oracle/jung_dual_judge.cpp)
target_include_directories(mhgp3v_wspd_wavefront_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_wspd_wavefront_probe PRIVATE mhgp)
target_compile_options(mhgp3v_wspd_wavefront_probe PRIVATE -Wall -Wextra -Werror)

# La vague doit PARTITIONNER les paires exactement comme la recursion, avec la
# cellule de Morton COMME avec la boite serree.
add_test(NAME mhgp3v_wspd_wavefront_multiplicite
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=48,64 --coord=512 --oracle)
set_tests_properties(mhgp3v_wspd_wavefront_multiplicite PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle accord=OUI")
add_test(NAME mhgp3v_wspd_wavefront_multiplicite_serree
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=48,64 --coord=512 --oracle --tight)
set_tests_properties(mhgp3v_wspd_wavefront_multiplicite_serree PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle accord=OUI")

# LA VAGUE CENTRALE EST JUGEE PAR BALAYAGE EXHAUSTIF. Toute fermeture affirme
# qu'il existe dix `PointId` DISTINCTS satisfaisant le masque central sur tout
# `A x B` ; le juge le verifie sur le nuage entier, sans emprunter l'intervalle
# du score ni l'antichaine.
add_test(NAME mhgp3v_wspd_wavefront_juge_vague
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1200
          --sep-euclid=2/1 --tight --judge-vwave --window=256)
set_tests_properties(mhgp3v_wspd_wavefront_juge_vague PROPERTIES
  PASS_REGULAR_EXPRESSION "juge_vague accord=OUI")

# LE MASQUE GLOBAL est la faute que j'avais ecrite : une lane deja `ALL`
# redescend dans les enfants et sa population est creditee DEUX fois. Le juge
# doit la tuer.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_mutant_masque_global "mutant_killed=1" 4
  --family=uniform --points=1200 --sep-euclid=2/1 --tight
  --inject=masque-global --window=256)

# Le travail doit valoir DEUX fois la sortie : chaque terminal a exactement un
# ancetre non separe. C'est la propriete qui rend la vague GPU-viable.
add_test(NAME mhgp3v_wspd_wavefront_travail
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=8000 --sep-euclid=2/1 --tight)
set_tests_properties(mhgp3v_wspd_wavefront_travail PROPERTIES
  PASS_REGULAR_EXPRESSION "tests/front=1\\.9[0-9]")

# --- `EdgeWindowRangeAdd-v0` : LA FENETRE D'ARETES, EXACTE, EN `O(F+n)`.
#
# J'avais ecrit que le maximum de `E_q(a)` exigeait de developper `|A||B|` par
# terminal. C'est faux : les nœuds du radix tree sont des intervalles DISJOINTS
# de `GenerationRank`, donc tout terminal satisfait un ordre TOTAL entre ses
# deux plages, et l'orientation canonique en fait un range-add — deux ecritures
# et un scan prefixe. Ce ledger rend `sum_a E_4(a)`, le nombre exact d'aretes
# candidates que le moteur shallow devrait traiter, sans jamais developper un
# `PairId`.

# L'ORACLE DEVELOPPE CHAQUE `PairId` EXACTEMENT UNE FOIS et compare TOUT le
# vecteur de degres. Une identite de somme ne suffit pas : une plage de mauvaise
# longueur peut la satisfaire. Les planchers interdisent le vert par vacuite.
add_test(NAME mhgp3v_wspd_wavefront_fenetre_oracle
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=600
          --sep-euclid=2/1 --tight --window-ledger --oracle-window=800
          --min-ouverts=10000 --min-orientations=10000)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_oracle PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle_fenetre accord=OUI")

# LE MEME ORACLE SUR UNE FAMILLE STRUCTUREE : `uniform` seule ne prouve pas que
# le range-add survit a des plages tres desequilibrees.
add_test(NAME mhgp3v_wspd_wavefront_fenetre_oracle_amas
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=500
          --sep-euclid=2/1 --tight --window-ledger --oracle-window=800
          --min-ouverts=5000 --min-orientations=5000)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_oracle_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle_fenetre accord=OUI")

# MUTANT `orientation-pointid` : le range-add sur des intervalles de `PointId`.
# Le contre-audit autorise un SCATTER par `spid[rang]` APRES le scan ; il
# n'autorise pas le range-add sur des plages de `PointId`, qui ne sont pas
# contigues. Trois portes independantes doivent le mordre — domaine des degres,
# identite de somme, oracle exhaustif — et le recu dit laquelle a vu quoi.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_fenetre_mutant_orientation "mutant_killed=1" 4
  --family=uniform --points=600 --sep-euclid=2/1 --tight
  --inject=orientation-pointid --oracle-window=800)

# MUTANT `orientation-cote` : crediter toujours le cote stocke en premier sans
# tester lequel est reellement inferieur. L'identite de somme y SURVIT — les
# deux plages ont la meme masse — et c'est precisement pourquoi l'equivariance
# par echange des cotes est une porte separee, non un doublon.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_fenetre_mutant_cote "ordre de stockage" 3
  --family=uniform --points=600 --sep-euclid=2/1 --tight
  --inject=orientation-cote)

# LE PLANCHER DOIT MORDRE. Sans cette porte, `--min-ouverts` pourrait etre une
# option decorative que personne n'a jamais vue refuser.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_fenetre_plancher_mord "PLANCHER" 3
  --family=uniform --points=600 --sep-euclid=2/1 --tight --window-ledger
  --min-ouverts=999999999)

# UN MUTANT SANS SON JUGE EST UN MUTANT NON JUGE. Le refus est AVANT calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_fenetre_refus_mutant_sans_oracle "REFUS" 2
  --family=uniform --points=600 --sep-euclid=2/1 --tight
  --inject=orientation-pointid)

# L'ORACLE SUR UNE RELATION NON TRIVIALE. Sans banque, tous les terminaux sont
# ouverts et l'oracle ne compare le range-add qu'a la relation complete
# `C(n,2)` : il ne juge alors aucune selection par fate. Ce nominal exige des
# sorts FERMES ET OUVERTS non vides en q4, sur les deux orientations.
add_test(NAME mhgp3v_wspd_wavefront_fenetre_oracle_banque
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1200
          --sep-euclid=6/1 --tight --vwave --window=256 --window-ledger
          --oracle-window=1500 --min-fermes=10000 --min-ouverts=10000
          --min-orientations=10000)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_oracle_banque PROPERTIES
  PASS_REGULAR_EXPRESSION "oracle_fenetre accord=OUI")

# LA PENTE DE `sum_E4` EST UNE PORTE, PAS UNE DECORATION — la lecon de
# `0eb65f1`. Le contre-audit a montre que cette porte n'etait jamais ATTEINTE :
# l'ancienne porte `front_records` rendait `3` avant l'impression. Les deux
# tests desarment donc explicitement les portes anterieures par `--max-slope=9`,
# et le refus porte desormais un motif DISTINCT que le test doit nommer.
add_test(NAME mhgp3v_wspd_wavefront_fenetre_pente_uniform
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1000,2000,4000
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --max-slope=9 --max-slope-e4=1.45
          --min-fermes=10000 --min-ouverts=10000 --min-orientations=10000)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_pente_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION "pente sum_E4=1\\.[0-3]")

# LA MEME PORTE DOIT REFUSER LA OU LE RESIDUEL N'EST PAS LINEAIRE. A `s=3`, la
# separation seule ne garantit plus aucune boule inscrite de rayon positif en
# q4 — il faut `s > sqrt(209/14) = 3,86375` pour le certificat CODE — donc la
# banque ne ferme presque rien et `sum_E4` reste quadratique. Le motif attendu
# est celui de la porte `sum_E4`, pas le generique `REFUS DE PENTE`.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_fenetre_pente_mord "REFUS DE PENTE sum_E4" 3
  --family=uniform --points=1000,2000,4000 --sep-euclid=3/1 --tight --vwave
  --window=512 --window-ledger --max-slope=9 --max-slope-e4=1.45)

# --- `JungSpindleSingleton-v0` : le cœur ANISOTROPE et ses deux fixtures.
#
# Le certificat en production teste la BOULE INSCRITE `209 V2 <= 56 D2`, qui est
# le pire cas directionnel du vrai cœur. La fixture A est celle de l'audit
# `b96751c` §1.2 : dix points strictement dans la boule diametrale et
# strictement HORS de la circumsphere q4, qu'aucun certificat ne doit crediter.
# La fixture B est un point de l'AXE, que la boule inscrite rejette et que le
# spindle credite — sans elle, un spindle qui n'accepte jamais rien passerait A.
# Le juge du balayage de Jung n'emploie aucune des deux algebres.
add_test(NAME mhgp3v_wspd_wavefront_fixtures_spindle
  COMMAND mhgp3v_wspd_wavefront_probe --fixtures-spindle)
set_tests_properties(mhgp3v_wspd_wavefront_fixtures_spindle PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures_spindle accord=OUI")

# LA COUPURE PAR RANG EST REFUTEE, DEFINITIVEMENT. C'est ma propre proposition
# que cette fixture ferme : l'audit `b96751c` §2 rend un support q4 POSITIF, de
# profondeur ZERO et d'arete maximale UNIQUE, dont le second endpoint est
# au-dela du rang `4383`. Aucun seuil de rang ne peut donc etre exact.
#
# CORRECTION `92d0c0f` : `x` et `y` sont EUX AUSSI plus proches de `a` que ne
# l est `b`, donc le rang 1-indexe du partenaire vaut `4384` et non `4382`.
# J avais oublie les deux autres sommets du support lui-meme.
add_test(NAME mhgp3v_wspd_wavefront_fixtures_rang
  COMMAND mhgp3v_wspd_wavefront_probe --fixtures-rang)
set_tests_properties(mhgp3v_wspd_wavefront_fixtures_rang PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures_rang accord=OUI rang_du_partenaire=4384")

# LE TERME DIRECTIONNEL SUR UN RECTANGLE, ET L'AUDIT DE SA PRESCRIPTION.
#
# L'audit `b96751c` §6 debloque le passage du spindle au rectangle par
# l'identite `T = d.v = ||z-a||^2 - ||z-b||^2`, separable par axe. Je ne la
# recois pas sur parole : la fixture compare l'intervalle prescrit a une
# enumeration EXHAUSTIVE de tout `A x B x C` sur trois mille boites tirees, et
# reproduit le contre-exemple qui montre que tester les seuls coins est faux.
add_test(NAME mhgp3v_wspd_wavefront_fixtures_terme_t
  COMMAND mhgp3v_wspd_wavefront_probe --fixtures-terme-t)
set_tests_properties(mhgp3v_wspd_wavefront_fixtures_terme_t PROPERTIES
  PASS_REGULAR_EXPRESSION "fixtures_terme_t accord=OUI cas=3000")

# LA FINALITE DE LA FENETRE DOIT ETRE JUGEE, PAS SEULEMENT IMPRIMEE.
#
# Le contre-audit live releve que les modes non-VWave annonçaient
# `fenetre_finale=OUI` APRES un abandon. La fenetre Morton, en particulier,
# n'examine jamais tout le nuage : c'est une proposition bornee autour d'une
# cle, jamais une preuve d'absence. Les deux portes ci-dessous exigent le bon
# verdict dans les deux sens, sinon le drapeau ne juge rien.
add_test(NAME mhgp3v_wspd_wavefront_fenetre_finale_non
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1200
          --sep-euclid=6/1 --tight --bank --window=256 --window-ledger)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_finale_non PROPERTIES
  PASS_REGULAR_EXPRESSION "fenetre_finale=NON raison=continuations_pendantes")

add_test(NAME mhgp3v_wspd_wavefront_fenetre_finale_oui
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1200
          --sep-euclid=6/1 --tight --vwave --window=512 --window-ledger
          --min-fermes=10000 --min-ouverts=10000)
set_tests_properties(mhgp3v_wspd_wavefront_fenetre_finale_oui PROPERTIES
  PASS_REGULAR_EXPRESSION "fenetre_finale=OUI")

# --- `Q3AcuteCarrierWave-v0` : LE SECOND NIVEAU, SUR LES PORTEURS.
#
# q3 n'est pas ternaire. Canonicalise par son arete maximale, un support q3 est
# la relation BINAIRE `(arete) x (porteur)`, et l'acuite s'y reduit au signe
# oppose du test du cœur q2 sur la MEME quantite entiere. La vague de niveau
# deux enumere les blocs porteurs ; elle ne ferme rien.
#
# LA MULTIPLICITE EST LA PREMIERE PORTE, comme pour la WSPD de paires. L'oracle
# enumere tous les triples, retient ceux dont `ab` est l'arete maximale ET dont
# l'angle en `x` est strictement aigu, puis exige que chacun tombe dans
# EXACTEMENT UN bloc emis. Un prune qui en mange un est refute.
add_test(NAME mhgp3v_wspd_wavefront_q3_multiplicite
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=120 --coord=512
          --sep-euclid=2/1 --tight --q3-sep=4 --q3-oracle=200 --min-blocs=10000)
set_tests_properties(mhgp3v_wspd_wavefront_q3_multiplicite PROPERTIES
  PASS_REGULAR_EXPRESSION "q3_oracle accord=OUI")

# LA MEME PORTE A UNE AUTRE SEPARATION : si le prune dependait de `s`, une seule
# valeur ne le verrait pas.
add_test(NAME mhgp3v_wspd_wavefront_q3_multiplicite_serree
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=120 --coord=512
          --sep-euclid=2/1 --tight --q3-sep=8 --q3-oracle=200 --min-blocs=10000)
set_tests_properties(mhgp3v_wspd_wavefront_q3_multiplicite_serree PROPERTIES
  PASS_REGULAR_EXPRESSION "q3_oracle accord=OUI")

# ET SUR UNE FAMILLE STRUCTUREE, ou la lentille et la boule diametrale sont
# tres inegalement peuplees.
add_test(NAME mhgp3v_wspd_wavefront_q3_multiplicite_amas
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=120 --coord=512
          --sep-euclid=2/1 --tight --q3-sep=4 --q3-oracle=200 --min-blocs=5000)
set_tests_properties(mhgp3v_wspd_wavefront_q3_multiplicite_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "q3_oracle accord=OUI")

# LE PLANCHER DE BLOCS DOIT MORDRE.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_q3_plancher_mord "PLANCHER" 3
  --family=uniform --points=120 --coord=512 --sep-euclid=2/1 --tight
  --q3-sep=4 --min-blocs=999999999)

# L'OWNER DOIT ETRE GLOBAL, SURTOUT SUR LES EGALITES. Choisir « une arete
# maximale faible » ne rend pas l'emission unique : un isocele en a deux, un
# equilateral trois. L'owner exact maximise la longueur carree puis, a egalite,
# prend la plus petite `EdgeKey` de `PointId` tries. La fixture est
# l'equilateral ENTIER de `Z^3` de l'audit `92d0c0f` §2.
add_test(NAME mhgp3v_wspd_wavefront_fixtures_owner
  COMMAND mhgp3v_wspd_wavefront_probe --fixtures-owner)
set_tests_properties(mhgp3v_wspd_wavefront_fixtures_owner PROPERTIES
  PASS_REGULAR_EXPRESSION "relabelings=3 owner_pointid=\\(0,1\\)")

# L'OWNER SE DECIDE SUR LES `PointId`, JAMAIS SUR LES POSITIONS MORTON. L'audit
# `1aa487d` prend le defaut en flagrant delit : mon comparateur recevait des
# positions dans `sp`, et surtout le SUJET et la VERITE appelaient la meme
# fonction, donc pouvaient s'accorder sur le meme mauvais tie-break. Le juge
# compare desormais l'ARETE qui possede, pas seulement la multiplicite — une
# multiplicite de un survit a un tie-break faux, puisque les trois aretes d'un
# equilateral sont toutes emises.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_wavefront_q3_mutant_owner_rang "mutant_killed=1" 4
  --family=uniform --points=100 --coord=16 --sep-euclid=2/1 --tight
  --q3-sep=4 --q3-oracle=200 --inject=owner-generationrank)

# ET LE NOMINAL SUR LE MEME NUAGE RICHE EN EGALITES : sans lui, le mutant
# pourrait mourir d'autre chose que de son tie-break.
add_test(NAME mhgp3v_wspd_wavefront_q3_owner_egalites
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=100 --coord=16
          --sep-euclid=2/1 --tight --q3-sep=4 --q3-oracle=200)
set_tests_properties(mhgp3v_wspd_wavefront_q3_owner_egalites PROPERTIES
  PASS_REGULAR_EXPRESSION "owners_faux=0")

# --- ETAPE 0A : `BallFormToBallEvent-v0`, LA TRANCHE VERTICALE QUI PRODUIT.
#
# La chaine v3 comptait des paires depuis des jours sans jamais produire un
# objet. Cette sonde prend un petit nuage, enumere EXHAUSTIVEMENT ses formes
# q2/q3/q4, et rend `PrimitiveSphereKey`, `SupportKey`, `I_B`, `U_B`, owner et
# disposition. Elle n'est pas le chemin produit : c'est l'autorite que la source
# sparse devra reproduire exactement.
add_executable(mhgp3v_ball_event_probe prototype/ball_event_probe.cpp)
target_include_directories(mhgp3v_ball_event_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/prototype)
target_compile_options(mhgp3v_ball_event_probe PRIVATE -Wall -Wextra -Werror)

# LE JUGE EST INDEPENDANT PAR SA ROUTE : il resout le systeme de GRAM dans la
# base du support, forme centre et rayon comme RATIONNELS, et compare par
# produit croise. Le sujet emploie la forme close `W` a l'arite trois, un Cramer
# en coordonnees a l'arite quatre, et ne forme jamais de centre.
add_test(NAME mhgp3v_ball_event_juge
  COMMAND mhgp3v_ball_event_probe --family=uniform --points=14 --coord=32
          --min-runs=200 --min-supports=300 --min-arite4=50)
set_tests_properties(mhgp3v_ball_event_juge PROPERTIES
  PASS_REGULAR_EXPRESSION "ball_event accord=OUI fold=OUI")

# ETAPE 0B : LE FOLD. Il consomme les `SphereRun` par niveau croissant sans
# jamais FORMER un niveau — `be_level_cmp` compare par produit croise. C'est la
# couture `SphereIdentity` que l'audit `1aa487d` §3 demande de preparer
# maintenant, pour qu'un profil `binary64` futur ne force pas a reecrire le fold.
#
# LE JUGE EST UNE AUTRE ALGORITHMIQUE : le sujet trie puis unit — Kruskal — et
# le juge ferme la matrice des niveaux d'arete par FLOYD-WARSHALL en min-max.
# Les deux doivent rendre le meme niveau de goulot pour CHAQUE paire.
add_test(NAME mhgp3v_ball_event_fold_grid
  COMMAND mhgp3v_ball_event_probe --family=grid --points=16 --coord=32 --min-supports=200)
set_tests_properties(mhgp3v_ball_event_fold_grid PROPERTIES
  PASS_REGULAR_EXPRESSION "fold fusions=[0-9]+ paires_connectees=[0-9]+ desaccords=0")

add_test(NAME mhgp3v_ball_event_fold_amas
  COMMAND mhgp3v_ball_event_probe --family=clusters --points=16 --coord=32 --min-supports=100)
set_tests_properties(mhgp3v_ball_event_fold_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0 domaine=OK")

# LA FAMILLE `grid` EST LE REGIME DUR : elle porte des cospheres, des formes
# degenerees et des refus de domaine, que `uniform` n'exerce presque pas.
add_test(NAME mhgp3v_ball_event_juge_cospheres
  COMMAND mhgp3v_ball_event_probe --family=grid --points=14 --coord=32
          --min-runs=200 --min-supports=200 --min-refus=20)
set_tests_properties(mhgp3v_ball_event_juge_cospheres PROPERTIES
  PASS_REGULAR_EXPRESSION "ball_event accord=OUI")

add_test(NAME mhgp3v_ball_event_juge_amas
  COMMAND mhgp3v_ball_event_probe --family=clusters --points=14 --coord=32 --min-supports=100)
set_tests_properties(mhgp3v_ball_event_juge_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "ball_event accord=OUI")

# TROIS MUTANTS, TROIS PROPRIETES DISTINCTES : l'owner decide sur l'index du
# tuple et non sur le `PointId` ; la coquille est comptee comme interieur ; la
# cle primitive n'est pas reduite par son pgcd, ce qui eclate les cospheres.
mhgp3v_add_expected_code_test_for(mhgp3v_ball_event_probe
  mhgp3v_ball_event_mutant_owner_index "mutant_killed=1" 4
  --family=grid --points=14 --coord=32 --inject=owner-index)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_event_probe
  mhgp3v_ball_event_mutant_coquille "mutant_killed=1" 4
  --family=grid --points=14 --coord=32 --inject=coquille-interieure)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_event_probe
  mhgp3v_ball_event_mutant_cle "mutant_killed=1" 4
  --family=grid --points=14 --coord=32 --inject=cle-non-reduite)

# LE PLANCHER DOIT MORDRE, ET LE JUGE DOIT REFUSER SON DOMAINE.
mhgp3v_add_expected_code_test_for(mhgp3v_ball_event_probe
  mhgp3v_ball_event_plancher_mord "PLANCHER" 3
  --family=uniform --points=14 --coord=32 --min-arite4=999999)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_event_probe
  mhgp3v_ball_event_refus_domaine "REFUS" 2
  --family=uniform --points=14 --coord=128)

# `JungSpindleRect-v0` : le certificat anisotrope sur rectangle. Il est EXACT et
# garde par `fixtures_terme_t`, mais son gain mesure est NUL — trois centiemes de
# point — parce que `Tabs` est le MINIMUM de `|T|` sur le bloc alors que le gain
# par paire vient des sites ou `|T|` est MAXIMAL. La porte conserve la
# disjonction, qui ne peut jamais perdre un support, et enregistre son taux.
add_test(NAME mhgp3v_wspd_wavefront_spindle_rect
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=2000
          --sep-euclid=8/1 --tight --vwave --window=512 --spindle --window-ledger)
set_tests_properties(mhgp3v_wspd_wavefront_spindle_rect PROPERTIES
  PASS_REGULAR_EXPRESSION "spindle_ALL=[0-9]+ essais=[0-9]+ tabs_non_nul=[0-9]+")

# ---------------------------------------------------------------------------
# `SOC64-shadow-q4` DANS LE CERTIFICATEUR CENTRAL : LE REPLAY VIRTUEL COMBINE.
#
# Les deux contre-audits du 14 aout ont trouve independamment la meme faute dans
# la premiere version : elle additionnait `cred[2]` et un credit SOC64 sur des
# ensembles NON DISJOINTS, parce qu'un descendant `central-ALL` recredite des
# `PointId` deja comptes par un ancetre `SOC64-ALL`. Le shadow tient desormais
# DEUX ledgers sur un seul parcours, et le `flip` les COMPARE au lieu de les
# sommer.
#
# Le juge est une seconde arithmetique : quand SOC64 rend `ALL`, le probe
# enumere les vrais `PointId` des trois nœuds et rejoue chaque triple dans
# l'ecriture `(g,Q)` de `spindle_cone.hpp`, qui ne partage ni `H`, ni `E`, ni
# `X`, ni aucune difference de Minkowski. Un seul triple refutant rend 1 : un
# verdict universel n'a pas de taux d'erreur admissible.
add_test(NAME mhgp3v_wspd_soc64_shadow_juge
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=800
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --judge-soc64 --min-soc-taches=100000)
set_tests_properties(mhgp3v_wspd_soc64_shadow_juge PROPERTIES
  PASS_REGULAR_EXPRESSION "soc64_juge accord=OUI verdicts_juges=[0-9]+ sautes=[0-9]+ triples=[0-9]+ faux=0")

# LE TEMOIN DE LA FAUTE, GARDE. Le ledger refute est calcule a cote et publie
# sur sa propre ligne ; la porte exige qu'il SURCOMPTE strictement, donc que
# l'ecart mesure ne retombe pas silencieusement a zero. Si un jour il devient
# nul, c'est que le parcours a change et il faudra le dire.
add_test(NAME mhgp3v_wspd_soc64_somme_brute_surcompte
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1200
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --raffine=4 --soc64-shadow)
set_tests_properties(mhgp3v_wspd_soc64_somme_brute_surcompte PROPERTIES
  PASS_REGULAR_EXPRESSION "soc64_somme_brute q4 : fermetures=[0-9]+ masse_fermee=[0-9]+ \\| surcompte fermetures=[1-9][0-9]* masse=[1-9][0-9]*")

# REFUS AVANT CALCUL. Sans `--vwave` le shadow ne verrait jamais une boite
# temoin et publierait zero tache avec un code zero : un vert par vacuite.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_soc64_reject_sans_vwave "--soc64-shadow exige --vwave" 2
  --family=uniform --points=200 --soc64-shadow)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_soc64_reject_plancher_sans_shadow "--min-soc-taches exige --soc64-shadow" 2
  --family=uniform --points=200 --min-soc-taches=1)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_wspd_soc64_plancher_taches "PLANCHER" 3
  --family=uniform --points=400 --sep-euclid=8/1 --tight --vwave --window=512
  --window-ledger --soc64-shadow --min-soc-taches=1000000000)

# ---------------------------------------------------------------------------
# `EdgeAcuteCarrierSample-v0` : LE PREMIER CHIFFRE DE `M3`/`M4`.
#
# Les deux contre-audits du 14 aout exigent la meme chose : une fenetre d'aretes
# sparse ne borne NI `M3`, NI `M4`. Tant que ce nombre est inconnu, aucune porte
# de cout n'est decidable, et comparer un temps de vague a une masse `E4` evitee
# n'a pas de sens. L'echantillon est SCELLE : tirage systematique sur la masse
# cumulee, pondere par `|A||B|`, sans generateur pseudo-aleatoire.
add_test(NAME mhgp3v_porteurs_uniform
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --porteurs=2048 --min-masse-porteurs=100000)
set_tests_properties(mhgp3v_porteurs_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION "porteurs q4 : population_finale=[0-9]+ masse_pending=[0-9]+ tires=2048 graine=[0-9]+ repetitions_consecutives=[0-9]+ digest=[0-9a-f]+ . A_e moyen=[0-9]+.* C4_carrier_quadrature=[0-9.]+ Hoeffding")

# LA CONTRE-FAMILLE A DEUX DROITES, GRAVEE.
#
# Section 3 de `AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`.
# Avec `A_i=(i,0,0)` et `B_j=(0,j,H)`, chaque paire croisee possede une sphere
# vide : les puissances exterieures des autres sites valent exactement
# `(k-i)^2` et `(l-j)^2`. AUCUN certificat universel sound ne les ferme.
# Pourtant aucun triangle n'est aigu, donc la vraie source q3/q4 est VIDE.
#
# Ces deux portes mesurent les deux nombres cote a cote sur le meme nuage. La
# premiere exige que la masse universelle ouverte soit bien quadratique — sans
# quoi la refutation serait vide. La seconde exige ZERO porteur sur la totalite
# de l'echantillon. Un jour ou la seconde deviendrait non nulle, c'est le
# compteur de porteurs qui serait faux, pas la geometrie.
add_test(NAME mhgp3v_two_lines_masse_universelle_quadratique
  COMMAND mhgp3v_wspd_wavefront_probe --family=two_lines --points=400
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --porteurs=4096 --min-ouverts=1000)
set_tests_properties(mhgp3v_two_lines_masse_universelle_quadratique PROPERTIES
  PASS_REGULAR_EXPRESSION "fenetre q4 : terminaux_ouverts=[0-9]+ masse_ouverte=4[0-9][0-9][0-9][0-9] ")
add_test(NAME mhgp3v_two_lines_zero_porteur_aigu
  COMMAND mhgp3v_wspd_wavefront_probe --family=two_lines --points=400
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --porteurs=4096)
# Le libelle est `C4`, et c'est le fond de la correction : ce compteur mesure la
# relation q3 geometrique PRE-RANG, pas la masse quaternaire. Cette porte a
# echoue au moment du renommage, et c'est ainsi qu'on sait que le renommage a
# bien eu lieu.
set_tests_properties(mhgp3v_two_lines_zero_porteur_aigu PROPERTIES
  PASS_REGULAR_EXPRESSION "porteurs q4 : .* A_e moyen=0.000 max=0 sans=4096 .100.000%. . C4_carrier_quadrature=0 ")
# Le meme nuage juge aussi SOC64 : il ferme quelques paires intra-droite, jamais
# les croisees. Le juge `(g,Q)` doit rester d'accord sur cette famille aussi.
add_test(NAME mhgp3v_two_lines_soc64_juge
  COMMAND mhgp3v_wspd_wavefront_probe --family=two_lines --points=400
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --judge-soc64 --min-soc-taches=10000)
set_tests_properties(mhgp3v_two_lines_soc64_juge PROPERTIES
  PASS_REGULAR_EXPRESSION "soc64_juge accord=OUI verdicts_juges=[0-9]+ sautes=[0-9]+ triples=[0-9]+ faux=0")

# L'ORACLE EXHAUSTIF DE `C4` ET `M4`. Il ne passe NI par la masse cumulee, NI
# par le decodage `cible -> (terminal, ra, rb)` de l'echantillonneur : il
# parcourt les terminaux ouverts et leurs deux plages directement. Quand
# `--porteurs` depasse la masse, le tirage devient exhaustif et les deux chemins
# doivent rendre le MEME entier. C'est la seule facon de savoir que
# l'arithmetique d'index de l'echantillonneur est juste — le defaut le plus
# silencieux possible de ce compteur.
add_test(NAME mhgp3v_porteurs_oracle_uniform
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=120
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --porteurs=20000 --apex=4096 --porteurs-oracle=200)
set_tests_properties(mhgp3v_porteurs_oracle_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION "porteurs_controle q4 : population_finale=6917 C4_carrier=119669 M4_apex=4676447")
# UN JUGE SAUTE NE VAUT JAMAIS ACCORD. Le contre-audit a reproduit la vacuite :
# `points=100 --porteurs-oracle=4` sortait zero sans ligne de controle ni
# mutant. Le refus est desormais anterieur au calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_porteurs_reject_controle_saute "le controle serait saute" 2
  --family=uniform --points=100 --sep-euclid=8/1 --tight --vwave --window=512
  --window-ledger --porteurs=1000 --porteurs-oracle=4)
# LE MUTANT QUE LE CONTRE-AUDIT A EXIGE. La version `v0` de ce compteur
# imprimait `C4` sous le libelle `M4` — la relation q3 geometrique pre-rang
# confondue avec la masse quaternaire. Le mutant reintroduit exactement cette
# faute, et l'oracle la tue : `C4=113858` mais `M4=4305996`.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_porteurs_kill_c4_comme_m4 "mutant_killed=1 porteurs-c4-comme-m4" 4
  --family=uniform --points=120 --sep-euclid=8/1 --tight --vwave --window=512
  --window-ledger --porteurs=2000 --porteurs-oracle=200
  --inject=porteurs-c4-comme-m4)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_porteurs_reject_mutant_sans_oracle
  "--inject=porteurs-c4-comme-m4 exige --porteurs-oracle" 2
  --family=uniform --points=120 --sep-euclid=8/1 --tight --vwave --window=512
  --window-ledger --porteurs=1000 --inject=porteurs-c4-comme-m4)
# La contre-famille doit rendre ZERO des deux cotes, et par les deux chemins.
add_test(NAME mhgp3v_two_lines_oracle_c4_m4_nuls
  COMMAND mhgp3v_wspd_wavefront_probe --family=two_lines --points=120
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --porteurs=20000 --apex=4096 --porteurs-oracle=200)
set_tests_properties(mhgp3v_two_lines_oracle_c4_m4_nuls PROPERTIES
  PASS_REGULAR_EXPRESSION "porteurs_controle q4 : population_finale=5742 C4_carrier=0 M4_apex=0")
# LE CAP DETERMINISTE DU SHADOW. Au cap, le ledger cesse d'etre final et le dit.
add_test(NAME mhgp3v_soc64_cap_minorant
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=800
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --soc-cap=4096)
set_tests_properties(mhgp3v_soc64_cap_minorant PROPERTIES
  PASS_REGULAR_EXPRESSION "cap_refuses=[1-9][0-9]* ledger=MINORANT_CAP")

mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_porteurs_reject_plancher_sans_echantillon "--min-masse-porteurs exige --porteurs" 2
  --family=uniform --points=200 --min-masse-porteurs=1)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_porteurs_plancher_masse "PLANCHER" 3
  --family=uniform --points=400 --sep-euclid=8/1 --tight --vwave --window=512
  --window-ledger --porteurs=64 --min-masse-porteurs=1000000000)

# ---------------------------------------------------------------------------
# ORACLE DE FORCE BRUTE q4 : LA VERITE TERRAIN, SANS RIEN PARTAGER AVEC LE SUJET.
#
# Le contre-audit `AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`
# section 11 reproche au controle interne du probe de partager les lambdas
# geometriques, les terminaux et les fates. Cette cible ne partage rien : autre
# unite de traduction, indice = PointId, ni tri Morton, ni fenetre, ni fate.
#
# Elle enumere TOUS les 4-sous-ensembles et separe trois nombres que le projet
# confondait : les candidats `M4`, les bien centres `W4`, et la sortie `H4` a
# rang au plus `smax-4`. Ses valeurs sont gravees.
add_executable(mhgp3v_q4_brute_oracle prototype/q4_brute_oracle.cpp)
target_include_directories(mhgp3v_q4_brute_oracle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_q4_brute_oracle PRIVATE mhgp)
target_compile_options(mhgp3v_q4_brute_oracle PRIVATE -Wall -Wextra -Werror)

# `M4` EST QUARTIQUE PAR CONSTRUCTION, ET C'EST LA LE POINT. Chaque
# 4-sous-ensemble possede exactement une arete owner ; sommer `|Q_e|` sur toutes
# les aretes compte donc les 4-sous-ensembles eux-memes. Le rapport mesure
# `0,6429`, et la porte le grave : un jour ou il s'effondrerait, ce serait la
# definition de l'owner qui aurait change.
add_test(NAME mhgp3v_q4_brute_uniform_60
  COMMAND mhgp3v_q4_brute_oracle --family=uniform --points=60)
set_tests_properties(mhgp3v_q4_brute_uniform_60 PROPERTIES
  PASS_REGULAR_EXPRESSION "4-sous-ensembles=487635 M4_total=313521 W4=37823 H4.rang<=7.=2387")
add_test(NAME mhgp3v_q4_brute_clusters_60
  COMMAND mhgp3v_q4_brute_oracle --family=eight_clusters --points=60)
set_tests_properties(mhgp3v_q4_brute_clusters_60 PROPERTIES
  PASS_REGULAR_EXPRESSION "4-sous-ensembles=487635 M4_total=322320 W4=48616 H4.rang<=7.=1609")
# LA SORTIE EST LINEAIRE, LES CANDIDATS NE LE SONT PAS. A `n=90`, `H4/n` vaut
# encore `27,3` sur les amas alors que `M4/C(n,4)` reste a `0,66` : l'ecart
# entre candidats et sortie est le vrai verrou, et il n'est pas dans `E4`.
add_test(NAME mhgp3v_q4_brute_clusters_90
  COMMAND mhgp3v_q4_brute_oracle --family=eight_clusters --points=90)
set_tests_properties(mhgp3v_q4_brute_clusters_90 PROPERTIES
  PASS_REGULAR_EXPRESSION "M4_total=1685886 W4=244956 H4.rang<=7.=2458")
# LA CONTRE-FAMILLE, VUE PAR UN CHEMIN TOTALEMENT DISJOINT : zero candidat, donc
# zero sortie, alors que le residuel universel y est quadratique.
add_test(NAME mhgp3v_q4_brute_two_lines
  COMMAND mhgp3v_q4_brute_oracle --family=two_lines --points=60)
set_tests_properties(mhgp3v_q4_brute_two_lines PROPERTIES
  PASS_REGULAR_EXPRESSION "M4_total=0 W4=0 H4.rang<=7.=0")

# LA MARGE DU REJET, ET POURQUOI ELLE OUVRE UNE VOIE. Un candidat q4 rejete ne
# l'est pas de justesse : la profondeur moyenne de sa sphere croit lineairement
# en `n` — 22,5 puis 34,4 puis 46,4 pour n = 60, 90, 120 — alors que le seuil
# reste a sept. Un certificat de BLOC n'a donc qu'a certifier « au moins huit »
# quand la valeur typique est deja six fois plus grande, et l'ecart s'ouvre avec
# la taille. C'est cette marge qui rend `TetraDepthBlock` plausible.
add_test(NAME mhgp3v_q4_brute_marge_rejet
  COMMAND mhgp3v_q4_brute_oracle --family=eight_clusters --points=120)
set_tests_properties(mhgp3v_q4_brute_marge_rejet PROPERTIES
  PASS_REGULAR_EXPRESSION "interieurs_moyen=46.40 max=116 seuil=7 marge=6.6x")

# ---------------------------------------------------------------------------
# `JungDual` : UN GROUPE DE TEMOINS FERME UNE PAIRE.
#
# Forme entiere de la condition minimax donnee par la section 5 de
# `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`. Le certificat
# central demande a UN temoin d'etre interieur a TOUTE sphere admissible ;
# `JungDual` demande seulement qu'au moins un membre du groupe le soit. Huit
# groupes a identites disjointes ferment q4.
#
# LE JUGE LE MOINS CHER ET LE PLUS SEVERE. Pour `k=1` et poids un, le predicat
# dual DOIT coincider avec l'ecriture historique `(g,Q)` de `spindle_cone.hpp`.
# Ce sont deux derivations independantes ; leur accord sur cent mille triples en
# pleine largeur u16 est la seule preuve disponible que la forme entiere du
# minimax est juste.
# LE JUGE EST UNE UNITE DE TRADUCTION SEPAREE, ET UN AUTRE OBJET MATHEMATIQUE.
# Le sujet decide par la voie DUALE — il choisit des poids et teste `A^2 > 2R`.
# Le juge prend la voie PRIMALE : il ne choisit aucun poids, il minimise
# `||s||^2` sur l'intersection des demi-plans mauvais et compare au rayon du
# disque de Jung. Son arithmetique est `mhgp3v::BigInt`, signe et magnitude —
# `K` atteint 70 bits sous u16, donc `Delta` et `N` environ 140 : `i128` ne
# suffit pas.
add_executable(mhgp3v_jung_dual_probe prototype/jung_dual_probe.cpp
  oracle/jung_dual_judge.cpp)
target_include_directories(mhgp3v_jung_dual_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/oracle)
target_link_libraries(mhgp3v_jung_dual_probe PRIVATE mhgp)
target_compile_options(mhgp3v_jung_dual_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_jung_dual_selftest
  COMMAND mhgp3v_jung_dual_probe --selftest --seed=1 --rounds=100000)
add_test(NAME mhgp3v_jung_dual_selftest_petit
  COMMAND mhgp3v_jung_dual_probe --selftest --seed=7 --rounds=100000 --span=64)
set_tests_properties(mhgp3v_jung_dual_selftest_petit PROPERTIES
  PASS_REGULAR_EXPRESSION "dual_selftest accord=OUI")

# LES CINQ MUTANTS. Deux d'entre eux ont d'abord SURVECU, et c'est instructif :
# `dual-ignore-weights` est un no-op a `k=1` puisque le poids y vaut deja un, et
# `dual-accept-equality` exige une egalite EXACTE `A^2 = 2R` que des coordonnees
# tirees ne produisent jamais. Il a fallu une fixture gravee pour chacun — la
# frontiere `a=(0,0,0)`, `z=(1,1,1)`, `b=(2,1,1)`, et un balayage a deux poids
# distincts.
foreach(_dual_mutant IN ITEMS
        dual-no-positivity dual-accept-equality dual-swap-coeff dual-narrow-i64
        dual-ignore-weights)
  string(REPLACE "-" "_" _dual_nom "${_dual_mutant}")
  mhgp3v_add_expected_code_test_for(mhgp3v_jung_dual_probe
    mhgp3v_jung_dual_kill_${_dual_nom} "MUTANT TUE" 4
    --selftest --seed=1 --rounds=100000 --inject=${_dual_mutant})
endforeach()

# L'ABLATION. Elle mesure le seul chiffre qui compte : combien de paires un
# groupe ferme que le temoin unique ne ferme pas. L'invariant d'inclusion est
# arme dans le binaire — un groupe de taille un EST un temoin unique, donc
# l'extraction ne peut jamais fermer moins.
add_test(NAME mhgp3v_jung_dual_ablation_amas
  COMMAND mhgp3v_jung_dual_probe --ablation --family=eight_clusters --points=600
          --echantillon=200 --voisins=16 --seed=1 --min-gain=1)
set_tests_properties(mhgp3v_jung_dual_ablation_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "dual_ablation famille=eight_clusters .* gain=[1-9]")
mhgp3v_add_expected_code_test_for(mhgp3v_jung_dual_probe
  mhgp3v_jung_dual_reject_sans_mode "choisir --selftest, --ablation ou --fixtures" 2 --seed=1)
mhgp3v_add_expected_code_test_for(mhgp3v_jung_dual_probe
  mhgp3v_jung_dual_reject_inject "--inject inconnu" 2 --selftest --inject=dual-inconnu)
mhgp3v_add_expected_code_test_for(mhgp3v_jung_dual_probe
  mhgp3v_jung_dual_plancher_gain "PLANCHER" 3
  --ablation --family=terrain --points=600 --echantillon=20 --min-gain=100000)

# ---------------------------------------------------------------------------
# LA DISSECTION DE LA PERTE : BOITE OU BUDGET ?
#
# `--profondeur-exacte` compte, pour un echantillon de PAIRES q4 ouvertes, les
# temoins universels exacts de la paire. `--profondeur-rect` fait de meme pour
# le RECTANGLE entier. L'ecart separe ce que la geometrie interdit de ce que la
# relaxation de boite et le budget coutent.
#
# L'ECHANTILLON DE RECTANGLES EST REPARTI PAR HACHAGE, et ce n'est pas un
# detail : prendre les premiers terminaux ouverts de la vague donnait 0 % de
# fermables, parce que ce sont les feuilles d'indices Morton adjacents, donc les
# aretes les PLUS COURTES, dont le spindle q4 est legitimement vide.
add_test(NAME mhgp3v_dissection_perte_amas
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --profondeur-rect=200 --rect-stride=307 --porteurs=200 --profondeur-exacte)

# ---------------------------------------------------------------------------
# OU LE CREDIT SE PERD-IL ? LE DIAGNOSTIC DE FEUILLE.
#
# La descente n'est PAS bornee par le budget : `tronques=0` et `pending=0`
# partout, donc elle atteint toutes ses feuilles. Reordonner la descente par
# proximite au milieu (`--ordre-proche`) ne change RIEN — douze configurations,
# masses identiques au bit pres. L'hypothese du budget est donc refutee.
#
# La cause est le PREDICAT. Sur les seules feuilles temoins et pour q4, trois
# verdicts sont compares sur le meme objet : la boule inscrite `56/209`
# actuellement branchee, le certificat correle `SOC64`, et l'universalite
# exacte `(g,Q)` point par point. La boule inscrite ne capte que les deux tiers
# de ce qui est atteignable ; `SOC64` en capte `96 %`.
#
# Ce test grave les trois taux ensemble : leur ORDRE est ce qui compte, et il
# est structurel — `central <= soc64 <= exact`, la premiere inegalite etant
# large et la seconde serree.
add_test(NAME mhgp3v_diag_feuille_amas
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --diag-feuille=200000)
add_test(NAME mhgp3v_diag_feuille_uniform
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --diag-feuille=200000)
# L'ORDRE DE DESCENTE NE CHANGE RIEN, ET C'EST GRAVE COMME TEL. Un jour ou ce
# test deviendrait rouge, c'est que le budget serait redevenu contraignant.
add_test(NAME mhgp3v_ordre_proche_sans_effet
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --ordre-proche)

# LA FIXTURE `u < p < d` : LE CONTRAT DEMANDE LA PROFONDEUR, PAS L'UNIVERSALITE.
#
# Section 5.5 de `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`.
# Elle separe trois quantites que tout le reste du travail confondait :
#   u = temoins interieurs a TOUTE sphere admissible ;
#   p = groupes couvrants a identites disjointes ;
#   d = min sur les centres du nombre de temoins interieurs.
# On a `u <= p <= d`, et le contrat demande `d >= 8`. Sur la configuration
# `a=(0,100,100)`, `b=(40,100,100)` avec six temoins de corde et trois gadgets
# aux regions mauvaises non vides et deux a deux disjointes dans le disque de
# Jung, on mesure exactement `u=6 < p=7 < d=8`.
#
# Elle refute donc DEUX affirmations d'un coup : « le certificat singleton est
# presque optimal » — il rate deux unites sur huit — et « huit groupes disjoints
# suffisent comme decision complete » — le packing plafonne a sept alors que la
# profondeur vaut huit. Une porte qui assimile echec du packing et profondeur
# insuffisante perd des fermetures VRAIES.
# LES TROIS FIXTURES DU JUGE PRIMAL, EN BINAIRE AUTONOME. Elles viennent de la
# section 5.1 du contre-audit et exercent ce que le selftest dual ne pouvait
# pas : la couverture COLLECTIVE — `a=(0,0,0)`, `b=(100,0,0)`, `z1=(42,26,0)`,
# `z2=(42,0,26)`, ou aucun singleton ne passe q4 mais le groupe couvre —, la
# dependance aux poids, et la frontiere stricte `g^2 = 2R`.
add_executable(mhgp3v_jung_dual_judge oracle/jung_dual_judge_fixtures.cpp
  oracle/jung_dual_judge.cpp)
target_include_directories(mhgp3v_jung_dual_judge PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/oracle)
target_compile_options(mhgp3v_jung_dual_judge PRIVATE -Wall -Wextra -Werror)
add_test(NAME mhgp3v_jung_dual_judge_fixtures COMMAND mhgp3v_jung_dual_judge --fixtures)
set_tests_properties(mhgp3v_jung_dual_judge_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "juge_primal accord=OUI fixtures=3 refutees=0")

# L'IMPLICATION EST DIRECTIONNELLE, ET C'EST TOUT CE QU'ON EXIGE. Le predicat
# dual est SUFFISANT, donc `dual couvre => primal couvre`. La reciproque est
# fausse : la banque de poids est finie et rate environ la moitie des groupes
# couvrants sur des tirages uniformes. Cette incompletude est MESUREE, pas
# refusee — et elle se revele sans effet sur les groupes que le proposer
# choisit reellement, qui sont les plus proches du milieu.
set_tests_properties(mhgp3v_jung_dual_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION "dual_collectif accord=OUI groupes=[0-9]+ dual=[0-9]+ primal=[0-9]+ . non_sound=0")

add_test(NAME mhgp3v_jung_dual_fixture_u_p_d COMMAND mhgp3v_jung_dual_probe --fixtures)
set_tests_properties(mhgp3v_jung_dual_fixture_u_p_d PROPERTIES
  PASS_REGULAR_EXPRESSION "dual_fixture u_p_d : u=6 p=7 d=8")

# ---------------------------------------------------------------------------
# `SOC64` EST ESSAYE AUSSI SUR UN `NONE` CENTRAL, ET C'EST GRATUIT.
#
# Le test central q4 est `s <= (56/209) D`, et `56/209 = 0,267943` approche
# `2 - sqrt(3) = 0,267949`, qui est exactement la forme du spindle dans le PIRE
# cas `U perpendiculaire a d`. Un `NONE` central ne signifie donc pas qu'aucun
# point n'est temoin universel : il signifie qu'aucun n'est dans la boule
# INSCRITE. Essayer `SOC64` la-dessus n'ajoute aucune descente — un `NONE` n'en
# declenchait deja aucune — et double la masse fermee sur `uniform`.
add_test(NAME mhgp3v_soc64_sur_none_central
  COMMAND mhgp3v_wspd_wavefront_probe --family=uniform --points=1500
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --soc64-shadow)
set_tests_properties(mhgp3v_soc64_sur_none_central PROPERTIES
  PASS_REGULAR_EXPRESSION "terminaux fermetures=39218 masse_fermee=173575")

# ---------------------------------------------------------------------------
# `BlockJungDual64` : UN GROUPE FERME UN RECTANGLE ENTIER, EN 64 COINS.
#
# Section 5.9 de `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`.
# C'est l'ABI qui manquait entre le proposer de poids et la tuile CK : pour une
# base et des poids FIXES, elle decide tout le produit AABB sans developper un
# seul `PairId`.
#
# Ce n'est pas une relaxation mais une EQUIVALENCE. Avec `L=somme w`,
# `Z=somme w z`, `Q=somme w ||z||^2`, on pose `A0 = -L(a.b)+(a+b).Z-Q` et
# `C0 = L(a x b) - a x Z - Z x b`. `A0` et `C0` sont AFFINES en `a` a `b` fixe,
# et reciproquement ; le cone de Lorentz strict est convexe ; les 64 couples de
# coins sont donc necessaires et suffisants.
#
# L'identite avec la forme `A/P/R` se verifie a la main — `A = 4 A0` et
# `R = 4 ||C0||^2`, donc `A^2 > 2R` equivaut a `2 A0^2 > ||C0||^2` — et le
# selftest l'exige sur vingt mille tirages, puis exige que les 64 coins
# redonnent le verdict ponctuel sur des boites reduites a des points.
set_tests_properties(mhgp3v_jung_dual_selftest_petit PROPERTIES
  PASS_REGULAR_EXPRESSION "bjd_selftest accord=OUI couples=[0-9]+ desaccords_point=0 desaccords_boite=0")

# ---------------------------------------------------------------------------
# `BlockJungDual64` BRANCHE DANS LA VAGUE : LE CREDIT DE GROUPE, ET SA
# CONDITION DE VALIDITE.
#
# Un groupe couvrant vaut UNE unite de profondeur. Elle ne s'ajoute a celles
# des temoins universels QUE si les identites sont disjointes — sinon le meme
# `PointId` porte deux unites. C'est la faute `somme-au-lieu-union` deja tuee
# deux fois sur le shadow `SOC64`, et le premier raccord des groupes la
# reproduisait : il puisait ses membres dans TOUTES les feuilles visitees, y
# compris celles que le certificat central venait de crediter.
#
# Deux juges independants gardent ce credit :
#   - le juge PRIMAL `BigInt` de `oracle/jung_dual_judge.cpp` verifie que
#     chaque groupe credite couvre TOUTES les paires de son rectangle. Il ne
#     choisit aucun poids : il minimise `||s||^2` sur les demi-plans mauvais ;
#   - le juge de FERMETURE recompose la famille disjointe complete — groupes
#     couvrants plus universels hors groupes, en `(g,Q)` — et exige `need`
#     unites. Il mord la ou le juge des flips ne peut rien : les groupes
#     changent le fate de la BASELINE, donc leurs fermetures ne sont pas des
#     flips.
add_test(NAME mhgp3v_bjd_credit_disjoint
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=200
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --soc64-actif --bjd-groupes=8 --juge-bjd=64
          --min-bjd-groupes=200 --min-bjd-fermetures=10)
set_tests_properties(mhgp3v_bjd_credit_disjoint PROPERTIES
  PASS_REGULAR_EXPRESSION
  "juge_bjd q4 : statut=COMPLET groupes=256 paires=442 faux=0 unknown=0 sautes=0 \\| fermetures juges=17 faux=0 sautes=0")

# UN REFUS DE JUGER N'EST PAS UN ACCORD. Sous un cap trop petit, la premiere
# ecriture publiait `OK`, `fenetre_finale=OUI` et le code zero avec quatre-vingt
# dix-huit groupes non juges. Le statut est desormais `PARTIEL` et la gate le
# refuse par un code de plancher.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_refus_juge_partiel "PARTIEL: " 3
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --soc64-actif --bjd-groupes=8 --juge-bjd=1)

# LES MODES VACUAIRES SONT REFUSES AVANT TOUT CALCUL, pas verts a zero essai.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_refus_sans_vwave "--bjd-groupes exige --vwave" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight
  --window=512 --bjd-groupes=8)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_refus_mutant_sans_juge "un mutant bjd exige --juge-bjd" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --bjd-groupes=8 --inject-bjd-reutilise)

# ---------------------------------------------------------------------------
# LA FIXTURE GRAVEE `seven_collinear_plus_reused_pair`.
#
# Section 2 de `AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`. Avec
# `a=(0,0,0)`, `b=(10,0,0)` et `z_i=(i,0,0)` pour `1<=i<=7`, la difference
# entre rayon carre et distance carree vaut exactement `i(10-i)>0` pour TOUT
# centre d'une sphere par `a,b`. Les sept temoins sont donc universels et la
# profondeur du pool vaut exactement SEPT, alors que le seuil q4 en demande
# huit : aucun terminal ne doit fermer.
#
# Toute paire de ces memes temoins reste pourtant une base couvrante. Un credit
# qui les reutilise publie donc un huitieme credit sans huitieme identite. La
# fixture est deterministe et ne depend d'aucune graine ; les portes
# statistiques sur `eight_clusters` ne la remplacent pas.
add_test(NAME mhgp3v_bjd_fixture_collineaire
  COMMAND mhgp3v_wspd_wavefront_probe --family=collinear_seven --points=9
          --sep-euclid=32/1 --tight --vwave --window=512 --window-ledger
          --soc64-actif --bjd-groupes=8 --juge-bjd=64 --exige-q4-ouvert)
set_tests_properties(mhgp3v_bjd_fixture_collineaire PROPERTIES
  PASS_REGULAR_EXPRESSION
  "bjd_groupes q4 : essais=20 couvrants=0 \\(0.000%\\) couples=20 couples/essai=1.00 rejetes_credite=55")
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_fixture_collineaire_mutant "mutant_killed=1 bjd-reutilise-temoin" 4
  --family=collinear_seven --points=9 --sep-euclid=32/1 --tight --vwave
  --window=512 --window-ledger --soc64-actif --bjd-groupes=8 --juge-bjd=64
  --exige-q4-ouvert --inject-bjd-reutilise)

# LES DEUX MUTANTS DE DISJONCTION. Le premier est exactement l'ecriture du
# raccord initial ; le second recredite le meme groupe jusqu'au seuil. Tous
# deux ferment des rectangles ou une seule identite interieure porte plusieurs
# unites de profondeur, et le juge de fermeture doit les tuer.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_mutant_reutilise_temoin "mutant_killed=1 bjd-reutilise-temoin" 4
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --soc64-actif --bjd-groupes=8 --juge-bjd=64
  --inject-bjd-reutilise)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_bjd_mutant_groupes_chevauchants "mutant_killed=1 bjd-groupes-chevauchants" 4
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --soc64-actif --bjd-groupes=8 --juge-bjd=64
  --inject-bjd-chevauche)

# ---------------------------------------------------------------------------
# `MidballBlockDepth` : LE PREDICAT q2 EXACT, SUR UN BLOC, AVANT LA DESCENTE.
#
# Section 4 et reponse Q5 de
# `AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`.
#
# Un support q2 n'a qu'UN centre — le milieu — donc qu'UNE boule. Il n'y a
# aucun continuum a quantifier, et le predicat exact est celui de Thales
# `H(a,b,z) = (z-a).(b-z) > 0`. Sa somme se separe par axe, chaque terme est
# lineaire en `a` et `b` et CONCAVE en `z` : le minimum sur le produit de trois
# AABB est donc la somme des trois minima aux sommets — la valeur exacte, pas
# une relaxation. Le maximum, lui, exige le point stationnaire `(a+b)/2`
# rabattu : le calculer aux seuls sommets est le mutant `midball-max-aux-coins`
# et produirait de faux `NONE`.
#
# Le juge n'emprunte rien a la primitive : il enumere tous les triplets entiers
# des trois boites. Les planchers exigent les trois verdicts.
add_executable(mhgp3v_midball_probe prototype/midball_probe.cpp)
target_include_directories(mhgp3v_midball_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_midball_probe PRIVATE mhgp)
target_compile_options(mhgp3v_midball_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_midball_selftest COMMAND mhgp3v_midball_probe --selftest=4000)
set_tests_properties(mhgp3v_midball_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION
  "midball_selftest accord=OUI tours=4000 all=[0-9]+ none=[0-9]+ mixed=[0-9]+ desaccords=0 bornes_fausses=0")

foreach(_mb_mutant IN ITEMS
    midball-max-aux-coins midball-accept-equality midball-drop-axis
    midball-min-au-milieu)
  string(REPLACE "-" "_" _mb_nom "${_mb_mutant}")
  mhgp3v_add_expected_code_test_for(mhgp3v_midball_probe
    mhgp3v_${_mb_nom} "mutant_killed=1 ${_mb_mutant}" 4
    --selftest=4000 --inject=${_mb_mutant})
endforeach()

# LA NON-HEREDITE, GRAVEE. Un q2 profond — dix temoins dans la boule
# diametrale — laisse subsister un q3 de rang trois et un q4 de rang quatre,
# car leurs centres uniques sont AILLEURS. Propager la profondeur q2 vers les
# lanes superieures est le mutant `midball-prune-q3q4`.
add_test(NAME mhgp3v_midball_non_heredite COMMAND mhgp3v_midball_probe --fixtures)
set_tests_properties(mhgp3v_midball_non_heredite PROPERTIES
  PASS_REGULAR_EXPRESSION
  "midball_fixtures : q2_interieurs=10 thales=10 q3_interieurs=0 q4_interieurs=0 cospherique_q3=1 cospherique_q4=1")
mhgp3v_add_expected_code_test_for(mhgp3v_midball_probe
  mhgp3v_midball_mutant_prune_q3q4 "mutant_killed=1 midball-prune-q3q4" 4
  --fixtures --inject=midball-prune-q3q4)

# Refus avant calcul : un mode vide ou un mutant sans son juge ne peut pas etre
# vert.
mhgp3v_add_expected_code_test_for(mhgp3v_midball_probe
  mhgp3v_midball_refus_sans_mode "--selftest, --selftest-hc ou --fixtures est exige" 2)
mhgp3v_add_expected_code_test_for(mhgp3v_midball_probe
  mhgp3v_midball_refus_mutant_sans_juge "un mutant de primitive exige --selftest" 2
  --fixtures --inject=midball-drop-axis)

# ---------------------------------------------------------------------------
# `MidballBlockDepth` BRANCHE DANS LA VAGUE, EN DISJONCTION.
#
# Le verdict central q2 compare `max(s)` a `min(D)`, deux extrema pris
# SEPAREMENT sur le rectangle. Le minimum exact de `H` sur le meme produit de
# boites decide la meme lane sans cette perte.
#
# EN DISJONCTION, JAMAIS EN REMPLACEMENT. Les deux `NONE` different : le
# central elague des qu'aucun `z` n'est UNIVERSEL, alors que `max H <= 0` exige
# que toute paire exclue tout `z`. Substituer le second detruit l'elagage,
# consomme le budget de descente et fait PERDRE des fermetures — mesure a
# `n=3000` sur huit amas : recertifications `31 538 327 -> 46 889 303` et masse
# q2 ouverte `262 529 -> 685 040`. En disjonction, les memes mesures donnent
# `29 542 758` recertifications et `188 460` de masse : c'est le premier
# certificat de cette exploration qui reduit A LA FOIS la fenetre et le travail.
#
# Le juge n'emprunte rien au sujet : il enumere les vrais `PointId` et exige
# `need[0]` temoins universels sur TOUTE paire du rectangle, dans l'ecriture
# `(g,Q)`. Ni `H`, ni la separation par axe, ni aucun extremum de boite n'y
# apparaissent.
add_test(NAME mhgp3v_midball_vwave_juge
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=200
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
          --midball --juge-midball=400000)
set_tests_properties(mhgp3v_midball_vwave_juge PROPERTIES
  PASS_REGULAR_EXPRESSION
  "midball q2 : all=5540 gains=5540 \\| juge=3122 faux=0 sautes=0")

mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_midball_refus_sans_vwave "--midball exige --vwave" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --window=512
  --midball)
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_midball_refus_juge_sans_sujet "--juge-midball exige --midball" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --juge-midball=400000)
# Le juge de vague ne recompose que les temoins du masque CENTRAL : avec un
# disjonctif exact il declarerait faux des fermetures legitimes.
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_midball_refus_judge_vwave "--judge-vwave ne recompose pas le disjonctif midball" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave
  --window=512 --window-ledger --midball --judge-vwave)

# ---------------------------------------------------------------------------
# `HCBlockDepth` : LA MEME IDEE POUSSEE AUX LANES q3 ET q4 — ET SON PRIX.
#
# Dans la representation `(H,C)`, `q3` vaut `3H^2 > ||C||^2` et `q4`
# `2H^2 > ||C||^2`. `H` reste exact par separation d'axes ; chaque composante de
# `C` est bornee par intervalle, ce qui ne perd que la correlation entre ses deux
# produits — donc ne cree que des `MIXED`, jamais un faux `ALL`.
#
# LE GAIN EST REEL ET LE PRIX AUSSI. A `eight_clusters,n=3000` le disjonctif
# retire `12,6 %` des recertifications et `17,6 %` de la fenetre q3, mais la
# mediane de trois temps CPU passe de `8,3 s` a `13,7 s`. Le predicat coute plus
# que les visites qu'il economise, exactement comme `SOC64` et `BJD` avant lui.
# Il est donc grave comme DIAGNOSTIC mesure, pas comme chemin chaud : seul
# `--midball`, dont le surcout est dans le bruit, reste candidat.
add_test(NAME mhgp3v_hc_selftest COMMAND mhgp3v_midball_probe --selftest-hc=3000)
set_tests_properties(mhgp3v_hc_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION
  "hc_selftest accord=OUI tours=3000 none=[0-9]+ q2=[0-9]+ q3=[0-9]+ q4=[0-9]+ desaccords=0")

foreach(_hc_mutant IN ITEMS hc-swap-coeff hc-correlation-ignoree)
  string(REPLACE "-" "_" _hc_nom "${_hc_mutant}")
  mhgp3v_add_expected_code_test_for(mhgp3v_midball_probe
    mhgp3v_${_hc_nom} "mutant_killed=1 ${_hc_mutant}" 4
    --selftest-hc=3000 --inject=${_hc_mutant})
endforeach()

add_test(NAME mhgp3v_hc_vwave
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=3000
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --hc)
set_tests_properties(mhgp3v_hc_vwave PROPERTIES
  PASS_REGULAR_EXPRESSION
  "hc : all_q2=439603 all_q3=431194 all_q4=358618")

# ---------------------------------------------------------------------------
# BORNE SUPERIEURE DES CREDITS : REFUTATION MESUREE.
#
# `cred + reste` majore exactement le credit final atteignable — un `NONE`
# retire sa population, un `ALL` la transfere, un `MIXED` la conserve puisque
# ses deux enfants la partitionnent. Abandonner une lane des que cette somme
# passe sous le seuil est donc exact et sans risque.
#
# Le gain est NUL : `31 538 327 -> 31 535 026` recertifications, soit un
# centieme de pour cent. La raison est structurelle : le seuil vaut huit a dix
# points, une misere devant les populations empilees, si bien que la lane ne
# meurt qu'une fois la pile presque vide. La coupure par le haut ne peut pas
# fonctionner sur cette quantite ; il faudrait majorer les temoins ENCORE
# ADMISSIBLES, ce qui demande precisement la visite qu'on cherche a eviter.
add_test(NAME mhgp3v_borne_sup_refutation
  COMMAND mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=3000
          --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --borne-sup)
set_tests_properties(mhgp3v_borne_sup_refutation PROPERTIES
  PASS_REGULAR_EXPRESSION
  "borne_sup : lanes_mortes=648746 visites_evitees=525")
mhgp3v_add_expected_code_test_for(mhgp3v_wspd_wavefront_probe
  mhgp3v_hc_refus_sans_vwave "--hc exige --vwave" 2
  --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --window=512 --hc)

# ---------------------------------------------------------------------------
# `Corner8BallDepth` : LE SUPPORT COMPLET N'A QU'UNE BOULE.
#
# Tant qu'on ferme une PAIRE, le centre reste une variable libre et tout le
# certificat consiste a quantifier sur le disque de Jung. Une fois le SUPPORT
# complet, il n'y a plus rien a quantifier : `{a,b,c,d}` determine sa
# circumsphere, et il suffit d'en compter les interieurs.
#
# Le predicat ne forme ni centre ni division : avec `O = det3(u,v,w)` et `J` le
# determinant in-sphere translate, `z` est strictement interieur si et seulement
# si `O*J < 0`. Sur un bloc, les deux signes sont bornes SEPAREMENT — le produit
# n'est jamais forme.
#
# La convexite fait le reste. A signe d'orientation fixe, le coefficient de
# `||z||^2` dans `sigma*J` vaut `|O| > 0` : la forme est strictement convexe en
# `z`, donc son maximum sur une boite temoin est a l'un des huit coins. Les
# coins sont COMPLETS pour prouver l'interieur — et seulement dans ce sens.
add_executable(mhgp3v_corner8_probe prototype/corner8_probe.cpp)
target_include_directories(mhgp3v_corner8_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_corner8_probe PRIVATE mhgp)
target_compile_options(mhgp3v_corner8_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_corner8_selftest COMMAND mhgp3v_corner8_probe --selftest=2000)
set_tests_properties(mhgp3v_corner8_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION
  "corner8_selftest accord=OUI tours=2000 all=[0-9]+ mixed=[0-9]+ desaccords=0")

# L'IMPLICATION NE S'INVERSE PAS. Le tetraedre `(3,2,2),(1,2,2),(2,3,2),(2,2,3)`
# et la boite `[1,3]^3` ont leurs huit coins EXTERIEURS et leur centre
# INTERIEUR : une fonction convexe peut etre negative au centre et positive
# partout aux coins.
add_test(NAME mhgp3v_corner8_convexite COMMAND mhgp3v_corner8_probe --convexite)
set_tests_properties(mhgp3v_corner8_convexite PROPERTIES
  PASS_REGULAR_EXPRESSION "corner8_convexite : coins_exterieurs=8 centre_interieur=1")

# LA FIXTURE u16 MISE A L'ECHELLE. Cinq spans de huit points, quarante
# `PointId` distincts, `4096` supports tous bien centres dont les huit temoins
# de `Z` sont uniformement interieurs. UN SEUL bloc doit fermer, avant tout fill.
add_test(NAME mhgp3v_corner8_bloc COMMAND mhgp3v_corner8_probe --bloc)
set_tests_properties(mhgp3v_corner8_bloc PROPERTIES
  PASS_REGULAR_EXPRESSION
  "corner8_bloc : verdict=ALL_INTERIOR supports=4096 temoins=32768 interieurs=32768")

# DEUX MUTANTS MEURENT SUR LE TIRAGE, DEUX NON — ET C'EST DIT.
#
# `corners-outside-implies-none` tombe massivement (23 236 desaccords sur
# 60 000 tirages) et `c8-accept-equality` sur trois cas de shell. En revanche
# `c8-drop-corner` n'a d'effet que si le huitieme coin est le SEUL a echouer, et
# `c8-norme-aux-coins` que si la boite temoin chevauche un facteur support. Ces
# deux-la ne sont PAS exerces par un tirage uniforme : ils resteront hors porte
# tant qu'une fixture deterministe ne les vise pas. Les enregistrer comme verts
# serait un vert par vacuite.
foreach(_c8_mutant IN ITEMS corners-outside-implies-none c8-accept-equality)
  string(REPLACE "-" "_" _c8_nom "${_c8_mutant}")
  mhgp3v_add_expected_code_test_for(mhgp3v_corner8_probe
    mhgp3v_${_c8_nom} "mutant_killed=1 ${_c8_mutant}" 4
    --selftest=60000 --inject=${_c8_mutant})
endforeach()

mhgp3v_add_expected_code_test_for(mhgp3v_corner8_probe
  mhgp3v_corner8_refus_sans_mode "--selftest, --bloc ou --convexite est exige" 2)

# ---------------------------------------------------------------------------
# `OwnedCK-WST3` : LA SOURCE D'ORDRE TROIS, COUNTER-ONLY.
#
# Une WSPD decompose les `C(n,2)` paires en `O(s^3 n)` rectangles. L'etendre aux
# TRIPLETS sans developper un seul triangle est la vraie source q3.
#
# Chaque triangle est attribue a son arete MAXIMALE. Si `ab` l'est, alors
# `||x-a|| <= ||b-a||` et `||x-b|| <= ||b-a||` : le troisieme sommet vit dans la
# LENTILLE des deux boules de rayon `||b-a||`. Sur un rectangle entier, majorer
# par `Dmax(A,B)` et remplacer les endpoints par leurs boites donne une
# condition NECESSAIRE, donc sure pour la couverture — et sans aucune racine
# carree : la distance a une boite se separe par axe et tout se compare au carre.
#
# Le juge enumere les `C(n,3)` triangles a petit `n`, determine l'owner de
# chacun et exige que son troisieme sommet appartienne a EXACTEMENT UN bloc du
# rectangle owner. Couverture manquante et doublon sont deux fautes distinctes.
add_executable(mhgp3v_wst3_probe prototype/wst3_probe.cpp)
target_include_directories(mhgp3v_wst3_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_wst3_probe PRIVATE mhgp)
target_compile_options(mhgp3v_wst3_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_wst3_couverture
  COMMAND mhgp3v_wst3_probe --family=uniform --points=120 --sep-euclid=2/1 --juge
          --min-blocs=100)
set_tests_properties(mhgp3v_wst3_couverture PROPERTIES
  PASS_REGULAR_EXPRESSION "wst3_juge : triangles=[0-9]+ manquants=0 doublons=0")

add_test(NAME mhgp3v_wst3_couverture_amas
  COMMAND mhgp3v_wst3_probe --family=eight_clusters --points=120 --sep-euclid=2/1
          --juge --min-blocs=100)
set_tests_properties(mhgp3v_wst3_couverture_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "wst3_juge : triangles=[0-9]+ manquants=0 doublons=0")

# UN SEUL MUTANT MORD LA COUVERTURE, ET C'EST DIT. `wst3-rayon-min` retrecit la
# lentille et laisse `145 236` triangles non couverts sur `n=120`. Les deux
# autres — `wst3-pas-de-descente` et `wst3-une-seule-boule` — SUR-couvrent : ils
# degradent le cout, jamais la couverture, et le juge de couverture ne peut donc
# pas les tuer. Les enregistrer ici serait un vert par vacuite ; il leur faut une
# porte de COUT, pas de correction.
mhgp3v_add_expected_code_test_for(mhgp3v_wst3_probe
  mhgp3v_wst3_rayon_min "mutant_killed=1 wst3-rayon-min" 4
  --family=uniform --points=120 --sep-euclid=2/1 --juge --inject=wst3-rayon-min)

# L'ECHELLE LIEE AU RECTANGLE. Exiger l'inclusion complete force a raffiner
# jusqu'a la frontiere de la lentille : le cout suit une SURFACE et le nombre de
# blocs redevient quadratique. En arretant la descente des que la cellule est
# petite devant le rayon du domaine, la couverture reste exact-once — une
# antichaine est disjointe — et le domaine est seulement SUR-couvert, ce qui est
# sur puisque la lentille n'est qu'une condition necessaire.
#
# Mesure sur `uniform`, `s=2`, `n = 1000..16000` :
#   inclusion complete : pentes 1,95 / 1,56 / 1,70 / 1,76
#   echelle=16         : pentes 1,50 / 1,64 / 1,66 / 1,23
#   echelle=1          : pentes 1,28 / 1,47 / 1,48 / 1,09
# et les rectangles WSPD eux-memes suivent 1,07 / 1,18 / 1,24 / 1,01. A echelle
# grossiere, le nombre de blocs d'ordre TROIS suit donc celui de l'ordre DEUX a
# une constante pres — environ trente-deux blocs par rectangle.
add_test(NAME mhgp3v_wst3_echelle_couverture
  COMMAND mhgp3v_wst3_probe --family=eight_clusters --points=140 --sep-euclid=2/1
          --echelle=1 --juge --min-blocs=1000)
set_tests_properties(mhgp3v_wst3_echelle_couverture PROPERTIES
  PASS_REGULAR_EXPRESSION "wst3_juge : triangles=447580 manquants=0 doublons=0")

mhgp3v_add_expected_code_test_for(mhgp3v_wst3_probe
  mhgp3v_wst3_refus_mutant_sans_juge "un mutant exige --juge" 2
  --family=uniform --points=120 --inject=wst3-rayon-min)

# `OwnedCK-WST4` : L'ORDRE QUATRE EST UN PRODUIT, PAS UNE NOUVELLE RECHERCHE.
#
# Pour un tetraedre `{a,b,x,y}` d'arete maximale `ab`, les DEUX autres sommets
# satisfont la meme contrainte que le troisieme sommet d'un triangle : ils sont
# dans la lentille. Les blocs `WST4` d'un rectangle sont donc exactement les
# COUPLES NON ORDONNES de ses blocs `WST3`, diagonale comprise — `{C,C}` porte
# les paires internes a une meme cellule.
#
# L'exact-once suit sans preuve supplementaire : les blocs `WST3` sont
# disjoints, donc `x` tombe dans au plus un `C` et `y` dans au plus un `D`, et
# le couple non ordonne est unique. Le juge le verifie tout de meme sur les
# `C(n,4)` quadruplets, owner par owner.
add_test(NAME mhgp3v_wst4_couverture
  COMMAND mhgp3v_wst3_probe --family=uniform --points=60 --sep-euclid=2/1
          --echelle=1 --ordre=4 --juge --min-blocs=1000)
set_tests_properties(mhgp3v_wst4_couverture PROPERTIES
  PASS_REGULAR_EXPRESSION
  "wst4_juge : quadruplets=487635 manquants=0 doublons=0")

# LE SIGNE DE L'ACUITE, GRAVE PAR LE TETRAEDRE REGULIER.
#
# `H = (x-a).(b-x)` et l'angle au sommet `x` vaut `(a-x).(b-x) = -H` : la face
# est aigue quand `H < 0`, pas `H > 0`. L'identite `E+X-D = -2H` le confirme, et
# c'est Thales dans le bon sens — a l'INTERIEUR de la boule diametrale l'angle
# est OBTUS.
#
# `p0=(0,0,0)`, `p1=(0,1,1)`, `p2=(1,0,1)`, `p3=(1,1,0)` : six aretes de
# longueur carree deux, quatre poids `1/4`, et `H = -1` pour les deux carriers.
# Ma premiere ecriture les classait non aigus et retirait leur couple, donc
# perdait un q4 bien centre.
add_test(NAME mhgp3v_wst3_tetra_regulier COMMAND mhgp3v_wst3_probe --fixture-tetra)
set_tests_properties(mhgp3v_wst3_tetra_regulier PROPERTIES
  PASS_REGULAR_EXPRESSION
  "wst3_tetra_regulier : H2=-1 H3=-1 carriers_aigus=2 aretes_egales=6")

# L'ECHELLE DES CELLULES EST LE LEVIER, ET IL EST QUADRATIQUE.
#
# La descente s'arrete des que `diag^2 <= (num/den) rayon^2`. Un rapport plus
# grand donne des cellules plus grosses, donc moins de blocs — et comme l'ordre
# quatre est un PRODUIT, le gain y est au carre. Mesure sur `uniform,n=8000` :
#
#   echelle=1/1   : 23 420 blocs4 par point, masse 8,55e14
#   echelle=1/64  :     76 blocs4 par point, masse 1,01e15
#   echelle=1/256 :     42 blocs4 par point, masse 1,02e15
#
# La sur-couverture ne coute que dix-neuf pour cent de masse pour un facteur
# cinq cent cinquante-six sur les enregistrements. La couverture reste jugee
# exact-once par projection owner jusqu'a `1/4096`.
add_test(NAME mhgp3v_wst4_echelle_fine
  COMMAND mhgp3v_wst3_probe --family=eight_clusters --points=60 --sep-euclid=2/1
          --echelle=1/256 --ordre=4 --juge --min-blocs=100)
set_tests_properties(mhgp3v_wst4_echelle_fine PROPERTIES
  PASS_REGULAR_EXPRESSION
  "wst4_juge : quadruplets=487635 manquants=0 doublons=0")

# LA CONTRE-FIXTURE PLATE, EXIGEE PAR LA REPONSE Q10/Q11.
#
# `o=(2000,2000,2000)`, `A=o+(1000,0,1)`, `B=o+(0,1000,-1)`,
# `C=o+(-1000,0,1)`, `D=o+(0,-1000,-1)` : quatre poids `1/4`, `R^2=1000001` et
# une orientation NON nulle, dont l'aplatissement normalise tend vers zero quand
# on etire l'echelle horizontale a hauteur fixee.
#
# Elle refute la premisse « presque coplanaire donc grand circumrayon » : le
# rayon reste comparable a l'echelle horizontale. Un grand rayon ne garantirait
# de toute facon aucun `PointId` interieur dans un nuage arbitraire — la
# reparation sans signe uniforme est le certificat BISIGNE, pas une heuristique
# de rayon.
add_test(NAME mhgp3v_wst3_fixture_plate COMMAND mhgp3v_wst3_probe --fixture-plate)
set_tests_properties(mhgp3v_wst3_fixture_plate PROPERTIES
  PASS_REGULAR_EXPRESSION
  "wst3_fixture_plate : R2=1000001 cospherique=1 orientation_nulle=0")

# CE QUE CONTIENT LA MASSE, ET NON CE QUE CONTIENNENT LES BLOCS.
#
# Tous les diagnostics precedents echantillonnaient les BLOCS uniformement.
# C'est le biais qui cachait le mur : les blocs sont domines en nombre par les
# petits, et la masse par les gros. `--masse` tire un quadruplet uniformement
# DANS LA MASSE, puis le disseque exactement.
#
# A `n=2000`, `s=2`, trois mille tirages, sur les deux familles :
#   - `82,7 %` ont leur arete maximale HORS du rectangle : ils appartiennent au
#     bloc d'un autre owner. C'est la « projection owner » que l'audit
#     distinguait du flux emis ;
#   - les autres ont en moyenne `915` interieurs sur `2000` points, soit
#     `46 %` du nuage dans leur circumsphere ;
#   - ZERO tirage sur trois mille a `I <= 7`, le seuil du contrat.
#
# La masse candidate est donc quasi integralement du dechet, et la sortie reelle
# est trop rare pour qu'un echantillon de cette taille la rencontre. Le filtre
# discriminant n'est ni l'owner ni l'acuite : c'est la TAILLE de la circumsphere.
add_test(NAME mhgp3v_wst4_masse_dissequee
  COMMAND mhgp3v_wst3_probe --family=uniform --points=2000 --sep-euclid=2/1
          --echelle=1/256 --ordre=4 --masse=3000)
set_tests_properties(mhgp3v_wst4_masse_dissequee PROPERTIES
  PASS_REGULAR_EXPRESSION
  "masse_dissequee : tires=3000 ids_repetes=12 \\(0.40%\\) owner_hors=2481 \\(82.70%\\) degeneres=0 interieurs_moyen=915.0 retenus=0")

# LA LOCALITE DES SUPPORTS REELLEMENT RETENUS.
#
# La masse ne contient presque aucun support retenu et un tirage ne les
# rencontre pas ; on les enumere donc EXHAUSTIVEMENT a petit `n`. Ce que la
# mesure cherche n'est pas leur nombre mais leur LOCALITE : le rang maximal d'un
# sommet vis-a-vis d'un autre. S'il est borne, une construction par voisinage
# borne les capture tous.
#
# ATTENTION AU PERIMETRE. Ce compteur retient les quadruplets d'orientation non
# nulle dont la circumsphere a au plus sept interieurs. Il n'applique PAS encore
# la stricte positivite des quatre barycentriques, qui est l'autorite q4 : ses
# nombres majorent donc `W4_positive` et ne sont pas `H4_rank`.
add_test(NAME mhgp3v_supports_retenus_localite
  COMMAND mhgp3v_wst3_probe --family=uniform --points=120 --sep-euclid=2/1
          --supports-retenus)
set_tests_properties(mhgp3v_supports_retenus_localite PROPERTIES
  PASS_REGULAR_EXPRESSION
  "supports_retenus : n=120 non_degeneres=8212806 bien_centres=600311 \\(7.31%\\) retenus=7344 retenus/n=61.20 ratio_arete_espacement moyen=5.94 max=12.04 rang_max moyen=43.5 pire=99")

# LA BONNE CENTRALITE : L'AUTORITE q4, SANS FORMER LE CENTRE.
#
# Un quadruplet a circumsphere peu remplie n'est un support que si son
# circumcentre est STRICTEMENT interieur au tetraedre ; sinon sa miniboule se
# reduit a une arite inferieure. Par Cramer, `c = N/(2 O)`, et le signe de
# `orient3d(face, c)` vaut celui de `det3(e1, e2, N - 2 O q0)` fois celui de
# `O` : aucune division, aucun flottant, tout sous `2^109` donc dans `i128`.
#
# Quatre cas la separent : le tetraedre regulier et le tetraedre PLAT du
# contre-audit sont bien centres — l'aplatissement n'a rien a voir avec la
# centralite —, tandis que la fixture de shell, ou `ab` est un diametre et les
# poids de `c,d` nuls, et un tetraedre tres obtus sont refuses.
add_test(NAME mhgp3v_corner8_centre COMMAND mhgp3v_corner8_probe --centre)
set_tests_properties(mhgp3v_corner8_centre PROPERTIES
  PASS_REGULAR_EXPRESSION "corner8_centre : regulier=1 plat=1 shell=0 obtus=0")

# ---------------------------------------------------------------------------
# `Q4SeedAxisTopR4` : LES COMPLETIONS q4 D'UN `Q4Seed3`, SANS PRODUIT.
#
# Specification : PROPOSITION.md section 7.2. Reception demandee par
# audits/AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md, et contrat de source
# dans audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md.
#
# L'entree est un `Q4Seed3` : un prefixe ternaire CREE ET POSSEDE par `Lane4`.
# Ce n'est ni un support q3, ni un record lu depuis `Lane3`. q2, q3 et q4 sont
# TROIS ENUMERATIONS INDEPENDANTES et le rang n'est pas hereditaire ; la porte
# `--exact-once` publie `seeds_rang_q3_mort`, le nombre de prefixes deja morts
# pour q3 qui produisent pourtant un q4 pertinent, et exige qu'il soit non nul.
#
# QUATRE CHOSES SONT JUGEES : la selection, les IDENTITES `I_B/U_B` contre un
# juge `insphere_j` a trois classes, la MORT PAR GAPS typee, et l'EXACT-ONCE
# global contre le brute force `C(n,4)`.
#
# CHAQUE PORTE A REGEX EST DOUBLEE D'UNE PORTE A CODE : un
# `PASS_REGULAR_EXPRESSION` masque le code de retour.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_q4seed_axis_topr4_probe prototype/q4seed_axis_topr4_probe.cpp)
target_include_directories(mhgp3v_q4seed_axis_topr4_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_q4seed_axis_topr4_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_q4seed_axis_topr4_probe PRIVATE -Wall -Wextra -Werror)

# FIXTURE SHARP 1 : seize completions, deux a chaque profondeur de zero a sept.
# La borne ne descend pas a quinze.
add_test(NAME mhgp3v_q4axis_fixture16_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-16)
add_test(NAME mhgp3v_q4axis_fixture16
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-16)
set_tests_properties(mhgp3v_q4axis_fixture16 PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture16 : r4=8 owner=EdgeKey\\(1,2\\) q4=16 candidats=16 groupes=16 manquants=0 identites_fausses=0 gaps_faux=0 degeneres=0 hist=2,2,2,2,2,2,2,2")

# FIXTURE SHARP 2 : la mort par gaps est TYPEE et sharp. Profondeur minimale
# huit sur tout `J_f`, sept des qu'on retire un seul des seize temoins.
add_test(NAME mhgp3v_q4axis_mort16_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-mort-16)
add_test(NAME mhgp3v_q4axis_mort16
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-mort-16)
set_tests_properties(mhgp3v_q4axis_mort16 PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture_mort16 : r4=8 profondeur_min_Jf=8 retrait_un_min=7 verdict=MORT_GAP minoree=8 shallow=0 gaps_faux=0 fate_apex=HORS_DOMAINE")

# FIXTURE DE FRONTIERE : `J_f` est FERME. Le tetraedre regulier sature Jung, son
# apex tombe exactement sur le bout, et le temoin `(98,100,104)` cospherique du
# meme bout reste un SHELL. Les deux orientations `B>0` et `B<0` sont exercees.
add_test(NAME mhgp3v_q4axis_jung_tendu_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-jung-tendu)
add_test(NAME mhgp3v_q4axis_jung_tendu
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-jung-tendu)
set_tests_properties(mhgp3v_q4axis_jung_tendu PROPERTIES PASS_REGULAR_EXPRESSION
  "bout_apex=0 s1=1 m1=0 i1=0 s2=1 m2=0 i2=0")

# THEOREME : un `Q4Seed3` owner STRICTEMENT AIGU a toujours `T2>0`, car l'angle
# oppose a l'arete maximale est le plus grand, donc dans `[60,90)` degres, donc
# `sin^2 C >= 3/4 > 2/3`. La branche `T2<=0` est INATTEIGNABLE sous acuite
# owner ; elle reste implementee et exercee sur un prefixe obtus.
add_test(NAME mhgp3v_q4axis_t2_code COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-t2)
add_test(NAME mhgp3v_q4axis_t2 COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-t2)
set_tests_properties(mhgp3v_q4axis_t2 PROPERTIES PASS_REGULAR_EXPRESSION
  "obtus=1 T2_obtus_negatif=1 aigu=1 T2_aigu_positif=1 verdict_obtus_mort_T2=1 candidats=0")

# CAMPAGNE DIFFERENTIELLE. Chaque famille porte SES PROPRES planchers : un vert
# par vacuite rend 3.
add_test(NAME mhgp3v_q4axis_sweep_uniform
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=uniform
                 --points=60 --seed=1 --threads=2 --min-seeds=10000 --min-shallow=3000)
add_test(NAME mhgp3v_q4axis_sweep_amas
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=eight_clusters
                 --points=60 --seed=1 --threads=2 --min-seeds=10000 --min-shallow=2000)
add_test(NAME mhgp3v_q4axis_sweep_terrain
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=terrain
                 --points=60 --seed=1 --threads=2 --min-seeds=5000 --min-shallow=200)
# Les familles SCANLINE portent les coplanarites massives : c'est la seule
# ou le shell persistant existe, donc la seule qui morde
# `a8-shell-compte-interieur`.
add_test(NAME mhgp3v_q4axis_sweep_scanline
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=scanline_single_pass
                 --points=60 --seed=1 --threads=2 --min-seeds=3000 --min-shallow=200)
add_test(NAME mhgp3v_q4axis_sweep_multiecho
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=scanline_overlap_multiecho
                 --points=60 --seed=1 --threads=2 --min-seeds=500 --min-shallow=100)

# UN SEUIL DIFFERENT DE SEPT. Sans lui, l'ABI parametrique n'est pas testee :
# `r4=smax-3` doit gouverner `p>=r4`, `k=r4-p` et la borne `2(r4-p)`.
add_test(NAME mhgp3v_q4axis_sweep_smax_14
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=uniform
                 --points=60 --seed=1 --threads=2 --smax=14
                 --min-seeds=10000 --min-shallow=5000)
add_test(NAME mhgp3v_q4axis_sweep_smax_7
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=uniform
                 --points=60 --seed=1 --threads=2 --smax=7
                 --min-seeds=10000 --min-shallow=500)

# LA CONTRE-FAMILLE `two_lines` : aucun triangle aigu, donc AUCUN `Q4Seed3`
# owner. La source q4 y est vide, et c'est la seule porte a plancher nul.
add_test(NAME mhgp3v_q4axis_two_lines_vide
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=two_lines
                 --points=60 --threads=2 --min-seeds=0 --min-shallow=0)
set_tests_properties(mhgp3v_q4axis_two_lines_vide PROPERTIES PASS_REGULAR_EXPRESSION
  "famille=two_lines n=60 r4=8 seeds=0 ")
add_test(NAME mhgp3v_q4axis_two_lines_vide_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=two_lines
                 --points=60 --threads=2 --min-seeds=0 --min-shallow=0)

# L'EXACT-ONCE GLOBAL, CONTRE LE BRUTE FORCE `C(n,4)`. Zero manque, zero
# doublon, zero surplus. `--min-q3-morts` EXIGE que l'independance des lanes
# soit reellement exercee : des prefixes deja morts pour q3 doivent produire des
# q4 pertinents, sinon la porte est verte par vacuite.
add_test(NAME mhgp3v_q4axis_exact_once_uniform
         COMMAND mhgp3v_q4seed_axis_topr4_probe --exact-once --family=uniform
                 --points=50 --seed=1 --min-q4=2000 --min-q3-morts=10)
set_tests_properties(mhgp3v_q4axis_exact_once_uniform PROPERTIES PASS_REGULAR_EXPRESSION
  "manque=0 doublon=0 surplus=0")
add_test(NAME mhgp3v_q4axis_exact_once_uniform_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --exact-once --family=uniform
                 --points=50 --seed=1 --min-q4=2000 --min-q3-morts=10)
add_test(NAME mhgp3v_q4axis_exact_once_amas
         COMMAND mhgp3v_q4seed_axis_topr4_probe --exact-once --family=eight_clusters
                 --points=50 --seed=1 --min-q4=1500 --min-q3-morts=10)
add_test(NAME mhgp3v_q4axis_exact_once_terrain
         COMMAND mhgp3v_q4seed_axis_topr4_probe --exact-once --family=terrain
                 --points=50 --seed=1 --min-q4=100 --min-q3-morts=1)

# REFUS ET PLANCHERS, A CODE EXACT.
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_refus_sans_mode
  "--fixture-16, --fixture-mort-16, --fixture-jung-tendu, --fixture-t2, --fixture-plateau, --sweep ou --exact-once est exige" 2)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_refus_cap "la campagne exhaustive est bornee a 400 points" 2
  --sweep --family=uniform --points=500)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_refus_cap_exact_once "l'exact-once exhaustif est borne a 200 points" 2
  --exact-once --family=uniform --points=300)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_refus_smax "--smax hors domaine" 2
  --sweep --family=uniform --points=60 --smax=99)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_plancher_vide "PLANCHER: 0 Q4Seed3 aigus owner" 3
  --sweep --family=two_lines --points=60 --threads=2 --min-seeds=1)
# LE PLANCHER QUI GARDE L'INDEPENDANCE DES LANES : si aucun prefixe q3-mort ne
# produit de q4, la fixture d'independance n'est pas exercee et la campagne est
# refusee, meme si tout le reste est vert.
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_plancher_independance "prefixes q3-morts utiles" 3
  --exact-once --family=terrain --points=50 --min-q4=100 --min-q3-morts=100)

# LES NEUF MUTANTS, A CODE DE SORTIE 4. Chacun est nomme avec la porte QUI SEULE
# le mord : les deux frontieres de `J_f` n'existent que sur le tetraedre
# regulier, et le shell persistant que sur une famille scanline.
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_cap "mutant_killed=1 a8-cap-moins-un" 4
  --fixture-16 --inject=a8-cap-moins-un)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_k_fixe "mutant_killed=1 a8-k-fixe-r4-moins-un" 4
  --fixture-16 --inject=a8-k-fixe-r4-moins-un)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_bouts_ouverts "mutant_killed=1 a8-bouts-ouverts" 4
  --fixture-jung-tendu --inject=a8-bouts-ouverts)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_permanence_large "mutant_killed=1 a8-permanence-large" 4
  --fixture-jung-tendu --inject=a8-permanence-large)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_gap_large "mutant_killed=1 a8-gap-large" 4
  --fixture-mort-16 --inject=a8-gap-large)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_signe_b "mutant_killed=1 a8-signe-b-inverse" 4
  --sweep --family=uniform --points=60 --threads=2 --inject=a8-signe-b-inverse)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_b_zero "mutant_killed=1 a8-oublie-b-zero" 4
  --sweep --family=uniform --points=60 --threads=2 --inject=a8-oublie-b-zero)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_abs_tri "mutant_killed=1 a8-abs-avant-tri" 4
  --sweep --family=uniform --points=60 --threads=2 --inject=a8-abs-avant-tri)
mhgp3v_add_expected_code_test_for(mhgp3v_q4seed_axis_topr4_probe
  mhgp3v_q4axis_mutant_shell "mutant_killed=1 a8-shell-compte-interieur" 4
  --sweep --family=scanline_single_pass --points=60 --threads=2 --profil=plateau
  --inject=a8-shell-compte-interieur)
# Le profil PLATEAU est aussi exerce en campagne honnete : sans lui, aucune
# comparaison d'identites n'a lieu la ou un shell existe.
add_test(NAME mhgp3v_q4axis_sweep_scanline_plateau
         COMMAND mhgp3v_q4seed_axis_topr4_probe --sweep --family=scanline_single_pass
                 --points=60 --seed=1 --threads=2 --profil=plateau
                 --min-seeds=3000 --min-shallow=200)

# LA CONTRE-FIXTURE DE CAPACITE : 97 IDs de MEME racine axiale. L'ancien buffer
# valait `32+64+3=99` et cessait SILENCIEUSEMENT d'ecrire au 99e slot ; il
# rendait une liste fausse sans overflow ni compte requis. La capacite prouvee
# vaut `3+32+2*64=163`, le compte VOULU est publie meme tronque, et le PROFIL
# decide : `RelevantGP` refuse la degenerescence, un plateau declare la publie
# entiere. Un compteur n'est pas un fate.
add_test(NAME mhgp3v_q4axis_plateau_code
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-plateau)
add_test(NAME mhgp3v_q4axis_plateau
         COMMAND mhgp3v_q4seed_axis_topr4_probe --fixture-plateau)
set_tests_properties(mhgp3v_q4axis_plateau PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture_plateau : aigu=1 verdict=OUVERT entrants=48 sortants=49 fate_relevant_gp=UNSUPPORTED_DEGENERACY fate_plateau=EXACT n_shell=100 requis_shell=100")
# `a8-shell-compte-interieur` n'est PAS double sur `--fixture-plateau` : ce
# plateau est un groupe de racines egales, pas un shell PERSISTANT (`B=0, A=0`).
# Le mutant n'y a rien a corrompre. Seule une famille scanline, avec ses
# coplanarites massives, le mord — et c'est la porte ci-dessus.

# ---------------------------------------------------------------------------
# J0 : LA TAILLE DE L'OBJET, AUX ECHELLES DU CONTRAT.
#
# Specification : jalon J0 de
# audits/NOTE_CLAUDE_PLAN_50K_PUIS_TRENTE_MILLIONS_20260814.md.
#
# Un seul chiffre gouverne la faisabilite : combien de supports l'objet
# contient-il ? Le brute force `C(n,4)` plafonne a `n ~ 400`, donc la sonde
# enumere par ANCRE D'ARETE DIAMETRALE — et son mode `--verifie` prouve que
# cette enumeration EGALE le brute force, sur q2 comme sur q4.
#
# Ce qu'elle publie reste un LEDGER DE CANDIDATS sous coupure `--dmax` declaree,
# pas un nombre certifie : la coupure n'est pas le rayon par calottes. La sonde
# publie le diametre reellement atteint et REFUSE a `0,75 dmax`.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_lane_source_scale_probe prototype/lane_source_scale_probe.cpp)
target_include_directories(mhgp3v_lane_source_scale_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_lane_source_scale_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_lane_source_scale_probe PRIVATE -Wall -Wextra -Werror)

# L'EXACTITUDE CONTRE LE BRUTE FORCE, SUR LES DEUX FAMILLES OBLIGATOIRES ET AUX
# DEUX PROFILS DE RANG. Sans elle, l'enumeration par ancre n'est qu'une
# heuristique rapide.
foreach(j0_fam uniform eight_clusters)
  foreach(j0_smax 11 6)
    add_test(NAME mhgp3v_lane_source_verifie_${j0_fam}_${j0_smax}
             COMMAND mhgp3v_lane_source_scale_probe --verifie --family=${j0_fam}
                     --points=100 --seed=1 --smax=${j0_smax} --threads=2)
    # CTest ne gere pas les references arriere : c'est la porte JUMELLE a code
    # qui decide l'egalite, la sonde rendant 1 des qu'un compte differe. Le regex
    # ne garde ici que la non-vacuite du diagnostic.
    set_tests_properties(mhgp3v_lane_source_verifie_${j0_fam}_${j0_smax} PROPERTIES
      PASS_REGULAR_EXPRESSION "verifie : brute_q2=[1-9][0-9]* ancre_q2=[0-9]+ brute_q4=[1-9][0-9]* ancre_q4=[0-9]+")
    add_test(NAME mhgp3v_lane_source_verifie_${j0_fam}_${j0_smax}_code
             COMMAND mhgp3v_lane_source_scale_probe --verifie --family=${j0_fam}
                     --points=100 --seed=1 --smax=${j0_smax} --threads=2)
  endforeach()
endforeach()

# LE MUTANT DE COUPURE : une coupure trop courte tronque la mesure, et la sonde
# doit le DIRE, pas rendre un nombre plus petit. `--dmax` minuscule sur les amas,
# ou la contre-famille a de longs supports traversant les vides.
mhgp3v_add_expected_code_test_for(mhgp3v_lane_source_scale_probe
  mhgp3v_lane_source_refus_coupure "mesure tronquee" 3
  --family=eight_clusters --points=400 --smax=11 --threads=2 --dmax=40)
mhgp3v_add_expected_code_test_for(mhgp3v_lane_source_scale_probe
  mhgp3v_lane_source_refus_domaine "--points hors domaine" 2
  --family=uniform --points=400000)
mhgp3v_add_expected_code_test_for(mhgp3v_lane_source_scale_probe
  mhgp3v_lane_source_refus_verifie_cap "--verifie est borne a 200 points" 2
  --verifie --family=uniform --points=1000)

# ---------------------------------------------------------------------------
# LE CERTIFICAT DE LOCALITE RESTREINT AUX DIRECTIONS ATTEIGNABLES.
#
# Specification : audits/NOTE_SOLUTION_CALOTTES_ADMISSIBLES_20260815.md.
#
# Le certificat par calottes existant echoue la ou le contrat vit — mesure au
# `certified_locality_probe` : `49,8 %` d'ancres NON certifiees sur `uniform`,
# `99,1 %` sur `terrain`. Une nappe n'a aucun point au-dela de sa normale, donc
# la cellule normale n'est jamais couverte.
#
# Le LEMME DU PARTENAIRE ANTIPODAL montre que cette exigence est trop forte :
# un support positif contenant `x` possede toujours un sommet AU-DELA de
# l'equateur de sa boule. Une direction sans point au-dela n'est donc jamais la
# direction d'un support, et exiger sa couverture est une exigence vide.
#
# Ce module est SEPARE de `certified_locality_probe.cpp`, contre-audite et long
# de 2 341 lignes, que je ne modifie pas. Sa grille, son arithmetique et sa
# selection sont reecrites, pour qu'un defaut commun ne se compense pas.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_caps_admissible_probe prototype/caps_admissible_probe.cpp)
target_include_directories(mhgp3v_caps_admissible_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_caps_admissible_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_caps_admissible_probe PRIVATE -Wall -Wextra -Werror)

# LES TROIS CONTRE-FIXTURES DE LA REFUTATION. Elles sont la porte PRINCIPALE de
# ce module : le theoreme des directions admissibles est FAUX, et ces trois cas
# le montrent — vacuite a l'echelle `r`, echange de quantificateurs au test par
# sommets, et arete confondue avec le diametre `2R`.
add_test(NAME mhgp3v_caps_refutation_code
         COMMAND mhgp3v_caps_admissible_probe --fixtures-refutation)
add_test(NAME mhgp3v_caps_refutation
         COMMAND mhgp3v_caps_admissible_probe --fixtures-refutation)
set_tests_properties(mhgp3v_caps_refutation PROPERTIES PASS_REGULAR_EXPRESSION
  "refutation_vacuite : certifie_x=1 certifie_v=1 D2=100 r2=25 refute=1")
add_test(NAME mhgp3v_caps_refutation_quantificateurs
         COMMAND mhgp3v_caps_admissible_probe --fixtures-refutation)
set_tests_properties(mhgp3v_caps_refutation_quantificateurs PROPERTIES
  PASS_REGULAR_EXPRESSION "refutation_quantificateurs : D2=102 test_sommets=0 direction_admissible=1 intersection_corrigee=1 refute=1")
# LE MUTANT `inclusion` EST TUE ICI, ET NULLE PART AILLEURS. Sur un nuage
# aleatoire dense il survit — la distinction inclusion/intersection ne s'y voit
# pas. Seule la cellule `e1,e2,e3` avec `s=(10,1,1)` la montre : le test par
# sommets rend faux, le classifieur exact rend vrai.
add_test(NAME mhgp3v_caps_refutation_arete
         COMMAND mhgp3v_caps_admissible_probe --fixtures-refutation)
set_tests_properties(mhgp3v_caps_refutation_arete PROPERTIES PASS_REGULAR_EXPRESSION
  "refutation_arete_vs_diametre : arete2=8 r2=9 arete_sous_r=1 quatre_R2_sur_r2=1 refute=1")

# LA FORME CORRIGEE : enveloppe `A_r`, marquage par INTERSECTION, partenaires
# NON restreints a `B(x,r)`. Ses chiffres sont bien plus modestes que ceux de la
# version refutee — `terrain` passe de `0` a `14 %` et non a `98 %` — et c'est
# exactement ce qui les rend croyables.
foreach(caps_fam uniform terrain scanline_single_pass eight_clusters)
  add_test(NAME mhgp3v_caps_corrige_falsifie_${caps_fam}
           COMMAND mhgp3v_caps_admissible_probe --corrige --falsifie
                   --family=${caps_fam} --points=150 --seed=1 --kmin=8 --threads=2)
  set_tests_properties(mhgp3v_caps_corrige_falsifie_${caps_fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "falsifie : supports_q2_testes=[1-9][0-9]* violations=0 supports_q4_testes=[1-9][0-9]* violations_q4=0")
  add_test(NAME mhgp3v_caps_corrige_falsifie_${caps_fam}_code
           COMMAND mhgp3v_caps_admissible_probe --corrige --falsifie
                   --family=${caps_fam} --points=150 --seed=1 --kmin=8 --threads=2)
endforeach()
add_test(NAME mhgp3v_caps_corrige_gain_uniform
         COMMAND mhgp3v_caps_admissible_probe --corrige --family=uniform --points=300
                 --seed=1 --kmin=8 --threads=2 --min-certifies-pct=80)
add_test(NAME mhgp3v_caps_corrige_gain_terrain
         COMMAND mhgp3v_caps_admissible_probe --corrige --family=terrain --points=300
                 --seed=1 --kmin=8 --threads=2 --min-certifies-pct=10)

# LES PORTES CI-DESSOUS NE VALIDENT AUCUN THEOREME. Elles mesurent seulement
# qu'aucune violation n'apparait sur des nuages ALEATOIRES denses — ce qui est
# vrai et ce qui est exactement pourquoi elles etaient trompeuses : la
# refutation vit dans une configuration a deux points, pas dans un nuage dense.
# Elles sont conservees comme diagnostic, sous ce nom.
foreach(caps_fam uniform terrain scanline_single_pass scanline_overlap_multiecho
                 eight_clusters)
  # `n=150` exerce AUSSI q4 : le brute force `C(n,4)` y est calculable, et q4
  # est le vrai objet. A `n=350` seul q2 serait teste.
  add_test(NAME mhgp3v_caps_admissible_falsifie_${caps_fam}
           COMMAND mhgp3v_caps_admissible_probe --falsifie --family=${caps_fam}
                   --points=150 --seed=1 --kmin=8 --threads=2)
  set_tests_properties(mhgp3v_caps_admissible_falsifie_${caps_fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "falsifie : supports_q2_testes=[1-9][0-9]* violations=0 supports_q4_testes=[1-9][0-9]* violations_q4=0")
  add_test(NAME mhgp3v_caps_admissible_falsifie_${caps_fam}_code
           COMMAND mhgp3v_caps_admissible_probe --falsifie --family=${caps_fam}
                   --points=150 --seed=1 --kmin=8 --threads=2)
  # Et une seconde taille, uniquement q2, pour que la couverture ne depende pas
  # d'un seul cardinal.
  add_test(NAME mhgp3v_caps_admissible_falsifie_${caps_fam}_q2
           COMMAND mhgp3v_caps_admissible_probe --falsifie --family=${caps_fam}
                   --points=350 --seed=2 --kmin=8 --threads=2)
endforeach()

# LE GAIN EST GRAVE FAMILLE PAR FAMILLE, avec un plancher : si la restriction
# cessait de payer, la porte le dirait au lieu de rester verte.
add_test(NAME mhgp3v_caps_admissible_gain_terrain
         COMMAND mhgp3v_caps_admissible_probe --family=terrain --points=600
                 --seed=1 --kmin=8 --threads=2 --min-certifies-pct=70)
add_test(NAME mhgp3v_caps_admissible_gain_scanline
         COMMAND mhgp3v_caps_admissible_probe --family=scanline_single_pass
                 --points=600 --seed=1 --kmin=8 --threads=2 --min-certifies-pct=70)
add_test(NAME mhgp3v_caps_admissible_gain_uniform
         COMMAND mhgp3v_caps_admissible_probe --family=uniform --points=600
                 --seed=1 --kmin=8 --threads=2 --min-certifies-pct=90)
mhgp3v_add_expected_code_test_for(mhgp3v_caps_admissible_probe
  mhgp3v_caps_admissible_plancher "d'ancres certifiees <" 3
  --family=terrain --points=600 --seed=1 --kmin=8 --threads=2
  --min-certifies-pct=99.9)
mhgp3v_add_expected_code_test_for(mhgp3v_caps_admissible_probe
  mhgp3v_caps_admissible_refus_falsifie "--falsifie est borne a 400 points" 2
  --falsifie --family=uniform --points=1000)

# ---------------------------------------------------------------------------
# QUALIFICATION HOTE/DEVICE DE `Q4SeedAxisTopR4`.
#
# Le kernel n'implemente AUCUNE geometrie : il appelle `evaluate_seed`, ecrite
# une seule fois dans `axis_device_job.hpp` et compilee pour les deux cibles.
# Sans cela, une comparaison bit a bit comparerait deux implementations et non
# deux executions.
#
# La cible se construit SANS CUDA : le chemin hote reste qualifiable seul, et le
# binaire dit alors `device : absent`. Avec `-DMHGP3V_ENABLE_CUDA=ON`, le kernel
# est ajoute et la parite de chaque champ de chaque verdict est exigee.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_axis_device prototype/axis_device_qualification.cpp)
target_include_directories(mhgp3v_axis_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_axis_device PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_axis_device PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>)
if(MHGP3V_ENABLE_CUDA)
  target_sources(mhgp3v_axis_device PRIVATE prototype/axis_device_kernel.cu)
  target_compile_definitions(mhgp3v_axis_device PRIVATE MHGP3V_AXIS_CUDA)
  set_target_properties(mhgp3v_axis_device PROPERTIES
    CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_EXTENSIONS OFF
    CUDA_SEPARABLE_COMPILATION OFF)
  # Meme hygiene numerique que les autres kernels : aucun FMA contracte, aucun
  # flush-to-zero. Le chemin est entier, la discipline le rend reproductible.
  target_compile_options(mhgp3v_axis_device PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false;--ftz=false;--prec-div=true;--prec-sqrt=true>)
endif()
# Le chemin hote seul, borne, avec plancher de non-vacuite.
add_test(NAME mhgp3v_axis_device_hote
         COMMAND mhgp3v_axis_device --family=uniform --points=600 --seed=1 --smax=6
                 --max-seeds=20000)
set_tests_properties(mhgp3v_axis_device_hote PROPERTIES PASS_REGULAR_EXPRESSION
  "axis_device : famille=uniform n=600 smax=6 r4=3 seeds=[1-9][0-9]*")
add_test(NAME mhgp3v_axis_device_hote_code
         COMMAND mhgp3v_axis_device --family=uniform --points=600 --seed=1 --smax=6
                 --max-seeds=20000)
mhgp3v_add_expected_code_test_for(mhgp3v_axis_device
  mhgp3v_axis_device_refus_domaine "--points hors domaine" 2 --points=10)

# ---------------------------------------------------------------------------
# DOMINANCE 432 : LE PREMIER CERTIFICAT DONT LA FERMETURE CROIT AVEC `n`.
#
# Specification : audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md
# sections 2, 3 et 7 — premiere des trois voies mises en concurrence.
#
# Le groupe octaedrique ramene toute direction a la chambre canonique, et neuf
# triangles de rayons entiers (3,i,j) la subdivisent : 48*9 = 432 sous-cones,
# avec min cos^2(gamma) = 9/11 entre deux rayons d'un meme sous-cone. Pour un
# couple (ancre, sous-cone), les huitieme, neuvieme et dixieme plus petites
# HAUTEURS suffisent : toute cible dont la hauteur atteint le seuil est fermee
# sans etre examinee individuellement. La decision porte sur un INTERVALLE de
# hauteurs, pas sur une paire — c'est la difference exacte avec les six
# ordonnances precedentes, qui partaient toutes de la paire.
#
# LE FACTEUR PUBLIE ETAIT TROP GROS DANS HUIT CAS SUR NEUF. Son trois vient de
# ||v||^2 <= 3 tau(v)^2, qui est l'inegalite de la CHAMBRE, saturee seulement
# sur la diagonale. Les sous-cones sont bien plus etroits, et le maximum de
# ||v||^2/v_x^2 sur un triangle est atteint a un SOMMET. Les tests deviennent
# `25 (9+i^2+j^2) tau(s)^2 <= 81 tau(d)^2` en q4 et `64 (...) <= 225 (...)` en
# q3, avec une constante par sous-cone. Sur le sous-cone axial le seuil passe de
# 3 a 1,843. Mesure a `n=8 000` : `uniform` ferme 25 fois plus, `eight_clusters`
# et `terrain` 1,3 fois plus, sans un seul desaccord du juge.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_directional_dominance_probe
  prototype/directional_dominance_probe.cpp
  prototype/spindle_cone_oracle.cpp)
target_include_directories(mhgp3v_directional_dominance_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_directional_dominance_probe PRIVATE mhgp)
target_compile_options(mhgp3v_directional_dominance_probe PRIVATE -Wall -Wextra -Werror)
# LA PARTITION EST LA PREMIERE PORTE. Les 432 sous-cones doivent partitionner
# les directions entieres, et surtout deux directions d'une meme cellule doivent
# verifier cos^2(gamma) >= 9/11 : sans cette borne le cutoff radial est FAUX.
# Le selftest l'exerce sur les 6 858 directions non nulles d'un cube 19^3, par
# produits croises entiers, et exige que les 432 cellules soient non vides.
add_test(NAME mhgp3v_dominance_selftest COMMAND mhgp3v_directional_dominance_probe --selftest)
set_tests_properties(mhgp3v_dominance_selftest PROPERTIES
  PASS_REGULAR_EXPRESSION "selftest accord=OUI")
# LE JUGE EST ECHANTILLONNE, ET C'EST ASSUME. Le juge exhaustif est en O(n^3) :
# il ne peut pas atteindre les tailles ou ce certificat commence a mordre — a
# `n=200` il ne ferme rien du tout, faute de huit temoins dans une meme cellule
# sur 432. L'echantillon tire les fermetures de PLUS PETITE MARGE, les seules ou
# une faute peut se loger, et chacune est rejouee en O(n) par l'unite oracle
# independante. UN ECHANTILLON NE PROUVE AUCUNE COMPLETUDE : il refute.
#
# LE CERTIFICAT DIRECT REMPLACE LA RELAXATION RADIALE, ET L'ABLATION MESURE LE
# GAIN. Le cutoff tabule composait deux relaxations : `||s|| <= kappa tau(s)`
# puis `r/D <= 3/5`. Le certificat direct emploie les deux rayons extremes de la
# cellule et sa frontiere EST celle du spindle. Le seuil de hauteur descend de
# pres de trois a `11/6` sur `U00` et a `3/2` sur `U22`. Mesure a `n=4 000`,
# fermetures q4 dirigees, radiale contre directe : `uniform` 450 066 -> 2 036 954
# (x4,53), `terrain` 7 700 953 -> 9 033 186 (x1,17), `multiecho` x1,11,
# `eight_clusters` x1,01. Zero desaccord du juge echantillonne.
add_test(NAME mhgp3v_dominance_uniform COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=uniform --seed=3 --judge-echantillon=800
         --min-closed-q4=6000 --min-residuel=3000000 --min-cellules=300000)
# ABLATION : la forme radiale tabulee, conservee pour que le gain de la table
# directe soit mesurable et non affirme.
add_test(NAME mhgp3v_dominance_ablation_radiale COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=uniform --seed=3 --ablation-radiale
         --judge-echantillon=400 --min-closed-q4=6000 --min-residuel=3000000)
add_test(NAME mhgp3v_dominance_clusters COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=eight_clusters --seed=3 --judge-echantillon=800
         --min-closed-q4=8000 --min-residuel=3000000)
add_test(NAME mhgp3v_dominance_terrain COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=terrain --seed=3 --judge-echantillon=800
         --min-closed-q4=1000000 --min-residuel=2000000)
add_test(NAME mhgp3v_dominance_multiecho COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=scanline_overlap_multiecho --seed=3 --judge-echantillon=800
         --min-closed-q4=1000000 --min-residuel=2000000)
# Juge EXHAUSTIF a petite taille : il ne ferme rien, mais il verifie que rien
# n'est ferme a tort et que les identites de masse tiennent.
add_test(NAME mhgp3v_dominance_juge_exhaustif COMMAND mhgp3v_directional_dominance_probe
         --points=200 --family=terrain --seed=3 --judge)
set_tests_properties(mhgp3v_dominance_juge_exhaustif PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")
# MUTANTS ARMES.
#
# Aucune famille generique n'arme le mutant du SEUIL : la ou le cutoff ferme, la
# paire possede en general bien plus que le seuil de temoins universels, et en
# retirer un ne change aucun verdict. Le nuage grave `--cloud-seuil` place sept
# temoins colineaires exactement — un de moins que la lane q4 n'en exige — et
# rend la cellule sous-pleine pour la reference.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_mutant_seuil "MUTANT TUE" 4
  --cloud-seuil --judge --inject=dom-h-moins-un)
# LE DOMAINE `smax` EST DERIVE, PLUS FIGE. La CLI accepte `4 <= smax <= 34`
# alors que le sujet employait `{10,9,8}`, qui ne vaut qu'a `smax=11` : a
# `smax=34` le juge refute une fermeture que le sujet prononce. Le seuil vaut
# desormais `h = smax+1-q`, et un `smax` exigeant plus que le tampon top-h
# REFUSE au lieu de tronquer.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_smax_hors_tampon "top-h superieur au tampon" 2
  --points=200 --smax=34)
# Refus contractuels.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_mutant_sans_juge "un mutant sans juge ne prouve rien" 2
  --points=200 --inject=dom-h-moins-un)
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_juge_non_borne "le juge ponctuel exhaustif est borne" 2
  --points=1000 --judge)
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_smax "--smax hors du domaine d'enveloppe" 2
  --points=200 --smax=9223372036854775807)
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_inconnu "argument inconnu" 2 --points=200 --turbo)
# UN CERTIFICAT QUI FERME TOUT EST FAUX. Ce plancher porte sur ce qui RESTE.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_plancher_residuel "plancher de residuel q4" 3
  --points=200 --family=terrain --seed=3 --min-residuel=100000000)
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_plancher_ferme "plancher de fermeture q4" 3
  --points=200 --family=uniform --seed=3 --min-closed-q4=1)

# ---------------------------------------------------------------------------
# PREFILTRE COMBINE : COEUR + DOMINANCE PAR EXTREMITE.
#
# Proposition de l'utilisateur du 15 aout 2026. Trois comptes de temoins,
# PROUVES DEUX A DEUX DISJOINTS — le coeur exclut `A` et `B`, `h_a` vit dans
# `A`, `h_b` dans `B`, et `A inter B` est vide par construction CK — dont la
# somme minore `|P inter W_q(a,b)|`. L'ancre meurt des que cette somme atteint
# `h_q`. La decision porte sur un histogramme de `h_b`, jamais sur une paire :
# le cout par rectangle est `O(|A|+|B|)` et non `O(|A||B|)`.
#
# Le juge borne verifie la seule chose qui compte : le minorant ne depasse
# JAMAIS le compte vrai. Un depassement serait une fermeture a tort, donc une
# perte de support.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_acute_owner_gateway_probe prototype/acute_owner_gateway_probe.cpp)
target_include_directories(mhgp3v_acute_owner_gateway_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_acute_owner_gateway_probe PRIVATE mhgp)

# ---------------------------------------------------------------------------
# LE GATEWAY `A x B x C` — la source ternaire, sans expansion de paires.
#
# `Phi = (a-x).(b-x)`, `Delta_E = D - E`, `Delta_X = D - X` sont SEPARABLES PAR
# AXE : leurs extrema exacts sur trois AABB sont la somme de trois extrema
# d'intervalles, donc `O(1)`, et la separabilite PRESERVE la correlation par
# `a_k` que des bornes independantes sur `D` et `E` detruiraient. Les six
# formules ont ete verifiees contre l'enumeration exhaustive de tous les entiers
# de six mille triplets d'intervalles, sans un ecart.
#
# CE QUE LES PORTES EXIGENT, ET DANS QUEL ORDRE :
#
#   `--oracle`        surete sur boites aleatoires : `dead_faux=0`, `all_faux=0`.
#   `--oracle-biais`  boites FABRIQUEES pour atteindre `ALL_STRICT`. Sans elles
#                     la branche n'etait jamais prise (`all_strict=0`) et le
#                     mutant `all-strict-lache` survivait par VACUITE.
#   `--brute`         le juge au niveau NUAGE : la recursion entiere contre
#                     `C(n,3)` sous owner canonique. C'est lui qui a trouve mes
#                     trois fautes — cellule Morton au lieu de boite serree,
#                     double comptage `(A,B)`/`(B,A)`, et filtre `pid` en trop.
#
# TROIS MUTANTS, DE DEUX ESPECES. `dead-large` et `all-strict-lache` rendent le
# classifieur FAUX et meurent au juge (code 4). `phi-large` ne ment pas — il
# rend `MIXED` la ou `DEAD` etait justifie — donc il reste SUR et se tue par
# PERTE DE COUVERTURE, sous le plancher de `dead`. Les confondre serait une
# erreur de categorie. Le plancher `--min-dead=400` les separe exactement :
# `416` blocs morts sans mutant, `351` avec — la graine etant fixee, ces deux
# nombres sont reproductibles a l'unite.
add_test(NAME mhgp3v_gateway_oracle_boites
  COMMAND mhgp3v_acute_owner_gateway_probe --oracle --tours=20000 --etendue=1 --min-dead=400)
set_tests_properties(mhgp3v_gateway_oracle_boites PROPERTIES
  PASS_REGULAR_EXPRESSION "dead_faux=0 all_faux=0(.|\n)*verdict=SUR planchers=OK")

add_test(NAME mhgp3v_gateway_oracle_all_strict_atteint
  COMMAND mhgp3v_acute_owner_gateway_probe --oracle-biais --tours=3000 --etendue=1
          --min-all-strict=1000 --min-dead=1)
# Le plancher `--min-all-strict=1000` est PORTE PAR LE BINAIRE, qui rend 3 s'il
# n'est pas tenu : une regex ne peut pas exprimer « au moins mille » sans
# quantificateur, et le quantificateur est interdit ici a juste titre.
set_tests_properties(mhgp3v_gateway_oracle_all_strict_atteint PROPERTIES
  PASS_REGULAR_EXPRESSION "dead_faux=0 all_faux=0(.|\n)*verdict=SUR planchers=OK")

foreach(fam two_lines terrain uniform eight_clusters)
  add_test(NAME mhgp3v_gateway_juge_${fam}
    COMMAND mhgp3v_acute_owner_gateway_probe --brute --family=${fam} --points=120)
  set_tests_properties(mhgp3v_gateway_juge_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "blocs_faux=0(.|\n)*ecart=0")
endforeach()

# `two_lines` : ZERO paire materialisee, et zero porteur. C'est l'exigence
# exacte de l'ordre d'execution de l'auditeur.
add_test(NAME mhgp3v_gateway_two_lines_sans_pairid
  COMMAND mhgp3v_acute_owner_gateway_probe --brute --family=two_lines --points=200
          --max-pairid=0)
set_tests_properties(mhgp3v_gateway_two_lines_sans_pairid PROPERTIES
  PASS_REGULAR_EXPRESSION "pairid_expanded=0 carriers=0 carriers_symboliques=0(.|\n)*ecart=0")

mhgp3v_add_expected_code_test_for(mhgp3v_acute_owner_gateway_probe
  mhgp3v_gateway_mutant_dead_large "MUTANT TUE" 4
  --brute --family=terrain --points=120 --inject=dead-large)
mhgp3v_add_expected_code_test_for(mhgp3v_acute_owner_gateway_probe
  mhgp3v_gateway_mutant_all_strict_lache "MUTANT TUE" 4
  --brute --family=terrain --points=120 --inject=all-strict-lache)
mhgp3v_add_expected_code_test_for(mhgp3v_acute_owner_gateway_probe
  mhgp3v_gateway_mutant_phi_large "MUTANT TUE : phi-large" 4
  --oracle --tours=20000 --etendue=1 --min-dead=400 --inject=phi-large)
mhgp3v_add_expected_code_test_for(mhgp3v_acute_owner_gateway_probe
  mhgp3v_gateway_refus_etendue "l'oracle serait non borne" 2
  --oracle --etendue=9)
mhgp3v_add_expected_code_test_for(mhgp3v_acute_owner_gateway_probe
  mhgp3v_gateway_refus_brute_non_bornee "--brute borne a 400 points" 2
  --brute --points=1000)

add_executable(mhgp3v_combined_prefilter_probe prototype/combined_prefilter_probe.cpp)
target_include_directories(mhgp3v_combined_prefilter_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_combined_prefilter_probe PRIVATE mhgp)
target_compile_options(mhgp3v_combined_prefilter_probe PRIVATE -Wall -Wextra -Werror)

# La partition est la premiere porte : sans elle, tout pourcentage est faux.
add_test(NAME mhgp3v_combined_partition
  COMMAND mhgp3v_combined_prefilter_probe --points=2000 --family=uniform --seed=3
          --separation=8 --min-rectangles=2000)
set_tests_properties(mhgp3v_combined_partition PROPERTIES
  PASS_REGULAR_EXPRESSION "ledger masse_attendue=1999000 ecart=0 recouvrements=0")

add_test(NAME mhgp3v_combined_juge_uniform
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=uniform --seed=3
          --separation=6 --juge=160)
set_tests_properties(mhgp3v_combined_juge_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0")

add_test(NAME mhgp3v_combined_juge_amas
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=eight_clusters --seed=5
          --separation=8 --juge=160)
set_tests_properties(mhgp3v_combined_juge_amas PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0")

add_test(NAME mhgp3v_combined_juge_terrain
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=terrain --seed=7
          --separation=6 --juge=160)
set_tests_properties(mhgp3v_combined_juge_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION "desaccords=0")

# PORTES NEGATIVES A CODE EXACT.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_famille "REFUS : famille inconnue" 2
  --points=100 --family=inexistante)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_inconnu "REFUS : argument inconnu" 2
  --points=100 --nawak)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_profil "REFUS : n hors profil u16" 2
  --points=70000)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_smax "REFUS : smax hors domaine" 2
  --points=100 --smax=3)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_juge_non_borne "--juge=500 hors de .0,200." 2
  --points=500 --juge=500)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_mutant_sans_juge "REFUS : un mutant sans juge ne prouve rien" 2
  --points=100 --inject=largeur-i64)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_plancher_rectangles "PLANCHER" 3
  --points=200 --family=uniform --seed=3 --separation=8 --min-rectangles=1000000)
# `coeur-intervalle-xi` n'est PAS une porte : l'enveloppe d'intervalles est
# sure, seulement plus lache. Elle reste accessible par `--inject` pour mesurer
# l'ecart entre les deux majorants, jamais pour prouver une mort.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_centre "MUTANT TUE" 4
  --points=160 --family=eight_clusters --seed=5 --separation=8 --juge=160
  --inject=coeur-centre-seul)

# ---------------------------------------------------------------------------
# REPARATION P0 DU CONTRE-AUDIT DU 15 AOUT.
#
# La fixture grave le defaut trouve par l'auditeur : `A` et `B` singletons, un
# sous-arbre de CINQ sites tous dans `W2`, `h_2 = 10`. Sans masque de lanes, la
# voie rapide creditait cinq puis les feuilles cinq de plus — `hcore2 = 10` — et
# l'ancre VIVANTE etait fermee.
#
# Trois planchers, parce qu'une porte qui ne prouve pas qu'elle exerce ce
# qu'elle teste est exactement ce qui a laisse passer le bug :
#   `bulk_credits >= 1`     la voie rapide s'est reellement declenchee ;
#   `oracle_ids_doubles`    aucun PointId credite deux fois ni pris dans A/B ;
#   `oracle_couverture_ko`  chaque paire vue exactement une fois.
# ---------------------------------------------------------------------------
add_test(NAME mhgp3v_combined_fixture_coeur5
  COMMAND mhgp3v_combined_prefilter_probe --fixture=coeur5)
set_tests_properties(mhgp3v_combined_fixture_coeur5 PROPERTIES
  PASS_REGULAR_EXPRESSION "nonvacuite bulk_credits=[1-9]")

mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_bulk_sans_masque "PointId credites deux fois" 1
  --fixture=coeur5 --inject=bulk-sans-masque --juge=7)

# ORACLE PAR PairId : la seule porte qui teste la DECISION et non ses
# ingredients. Elle exige `fermees` inclus dans `vraiment mortes`.
add_test(NAME mhgp3v_combined_oracle_uniform
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=uniform --seed=3
          --separation=6 --oracle=160)
set_tests_properties(mhgp3v_combined_oracle_uniform PROPERTIES
  PASS_REGULAR_EXPRESSION
  "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")

add_test(NAME mhgp3v_combined_oracle_amas
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=eight_clusters --seed=5
          --separation=8 --oracle=160)
set_tests_properties(mhgp3v_combined_oracle_amas PROPERTIES
  PASS_REGULAR_EXPRESSION
  "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")

add_test(NAME mhgp3v_combined_oracle_terrain
  COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=terrain --seed=7
          --separation=6 --oracle=160)
set_tests_properties(mhgp3v_combined_oracle_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION
  "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")

# ---------------------------------------------------------------------------
# L'ECART AU VRAI VIVANT — la mesure qui dit s'il reste quelque chose à prendre.
#
# Le préfiltre est fail-open, donc son résiduel CONTIENT les ancres vraiment
# vivantes. Tout resserrement de certificat est un pari tant qu'on ignore de
# combien. Deux modes, et deux rôles :
#
#   `--vrai-vivant`  EXACT, et sans `O(n^3)`. Le préfiltre étant fail-open, toute
#                    ancre vivante est PARMI les survivantes : il suffit de les
#                    décider. Coût `C(n,2)` tests de budget plus
#                    `survivantes x n` avec sortie anticipée — il suit donc le
#                    résiduel. Reproduit à l'unité l'ancienne énumération
#                    exhaustive (`n=600` : `17479 / 42294 / 45913`).
#   `--echantillon`  conservé, et réhabilité. Je l'avais écarté au motif d'une
#                    variance « inexpliquée » de trois à douze écarts-types : je
#                    comparais des écarts RELATIFS à un écart-type ABSOLU en
#                    points de proportion. Les écarts-types relatifs corrects
#                    valent `2,414 %` / `1,207 %` / `0,604 %` pour
#                    `K = 5000 / 20000 / 80000`, et les neuf écarts mesurés
#                    tiennent tous sous `1,52` sigma. L'estimateur est sain ; le
#                    compte exact reste préféré tant qu'il tient, parce qu'il ne
#                    demande aucun intervalle.
#
# L'invariant est la porte : une violation signifierait que le préfiltre ferme
# à tort, le défaut le plus grave possible.
# ---------------------------------------------------------------------------
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_vrai_vivant_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=300 --family=${fam} --seed=3
            --separation=8 --smax=11 --vrai-vivant --ha=dualtree --coeur=corner64)
  set_tests_properties(mhgp3v_combined_vrai_vivant_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "vraivivant V2_pair_walive=[1-9][0-9]*")
endforeach()

mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_vrai_vivant_non_borne "--vrai-vivant non borne" 2
  --points=50000 --vrai-vivant)

# ---------------------------------------------------------------------------
# LES DEUX BALAYAGES DU `W`-VIVANT — P0.5 du ré-audit `eb1b52a`.
#
# Le balayage a été réécrit : trois passes sur `z`, une par lane, sont devenues
# UNE passe alimentant trois compteurs, et `corner8_lane(a, Box(b), z)` — huit
# coins identiques quand `b` est ponctuel — est devenu `pair_lane(a, b, z)`.
# Les paires `D = 0` sont désormais exclues, `V_q` étant défini sur `D > 0`.
#
# Une réécriture de balayage et une réécriture de CE QUE LE BALAYAGE COMPTE se
# ressemblent dans un diff. L'ancien est donc conservé sous `--vivant=legacy`,
# et cette porte exige l'égalité À L'UNITÉ des trois comptes sur trois familles.
#
# CE QUE LA MESURE A CORRIGÉ DANS MON RAISONNEMENT. La section 6.4 du ré-audit
# annonce huit évaluations redondantes par visite. C'est vrai du pire cas et
# faux en régime : `corner8_lane` rend `0` dès le PREMIER coin dont `H <= 0`,
# donc la redondance réelle vaut `1 + 7 P(H > 0)`. Mesurée à `n=300` : `1,386`
# sur `uniform`, `1,245` sur `terrain`, `1,845` sur `eight_clusters` — jamais
# huit. Le gain total en évaluations vaut `3,0` à `4,0`, et il vient donc
# surtout de la fusion des trois passes, pas de la spécialisation ponctuelle.
#
# DEUX MUTANTS, DONT UN NEUTRE ASSUMÉ :
#   `vivant-lane-unique`      rabat toute lane sur q2 dans le seul balayage
#                             fusionné. La porte DOIT rendre 1.
#   `vivant-sans-extinction`  n'éteint pas une lane qui atteint son seuil. Le
#                             verdict étant `c < h_q` et `c` croissant, il est
#                             NEUTRE — la porte doit rester verte. Il n'est pas
#                             décoratif : il chiffre ce que l'extinction gagne
#                             (`7 525 096` visites contre `6 690 404`, soit
#                             `11,1 %` sur `uniform`, `13,1 %` sur `terrain`,
#                             `24,3 %` sur `eight_clusters`), et il atteste que
#                             la sortie anticipée ne change aucun compte.
# ---------------------------------------------------------------------------
if(Python3_FOUND)
  add_test(NAME mhgp3v_vivant_deux_balayages
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_vivant_balayages.py
                   --probe=$<TARGET_FILE:mhgp3v_combined_prefilter_probe>
                   --points=300 --min-vivantes=500 --min-paires=2000)
  set_tests_properties(mhgp3v_vivant_deux_balayages PROPERTIES
    PASS_REGULAR_EXPRESSION
    "balayages accord=OUI familles=3 desaccords=0 planchers_violes=0")

  # LE MUTANT NEUTRE RESTE VERT, et son reçu chiffre l'extinction.
  add_test(NAME mhgp3v_vivant_extinction_neutre
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_vivant_balayages.py
                   --probe=$<TARGET_FILE:mhgp3v_combined_prefilter_probe>
                   --points=300 --min-vivantes=500 --min-paires=2000
                   --inject=vivant-sans-extinction)
  set_tests_properties(mhgp3v_vivant_extinction_neutre PROPERTIES
    PASS_REGULAR_EXPRESSION
    "balayages accord=OUI familles=3 desaccords=0 planchers_violes=0")

  # LE MUTANT LÉTAL MEURT, code 1 exact — un désaccord entre les deux juges.
  add_test(NAME mhgp3v_vivant_mutant_lane_unique
           COMMAND ${CMAKE_COMMAND}
                   "-DPROGRAM=${Python3_EXECUTABLE}"
                   "-DEXPECTED=accord=NON"
                   "-DEXPECTED_CODE=1"
                   "-DARGUMENTS=${CMAKE_CURRENT_SOURCE_DIR}/audits/check_vivant_balayages.py;--probe=$<TARGET_FILE:mhgp3v_combined_prefilter_probe>;--points=300;--min-vivantes=500;--min-paires=2000;--inject=vivant-lane-unique"
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/expect_failure.cmake)
endif()

# Un mutant `vivant-*` injecté dans son PROPRE juge le rendrait invisible : le
# probe refuse avant tout calcul.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_vivant_mutant_dans_son_juge "injecte dans son propre juge" 2
  --points=200 --vivant=legacy --inject=vivant-lane-unique)

# Et hors du mode `--vrai-vivant` il n'altère rien du tout : refus, plutôt qu'un
# vert par vacuité qui laisserait croire le mutant survivant.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_vivant_mutant_sans_mode "sans --vrai-vivant" 2
  --points=200 --inject=vivant-lane-unique)

# `--cout-instruction` ne mesure la lentille que sur la lane q4, ce que le
# balayage legacy — une lane à la fois — ne peut pas alimenter.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_cout_instruction_legacy "exige le balayage fusionne" 2
  --points=200 --vivant=legacy --cout-instruction)

# ---------------------------------------------------------------------------
# LE CŒUR-BOULE DANS LA DESCENTE : mesurable, et NON adopté.
#
# `--coeur=boule` ajoute deux choses au cœur : un crédit en bloc `ALL` pour les
# TROIS lanes — jusqu'ici seule q2 en avait un — et un certificat `NONE` par
# boule extérieure. La mesure refuse les deux.
#
#   `ALL`  : la fermeture est IDENTIQUE et le travail baisse de moins d'un pour
#            cent, pour un temps de paroi plus mauvais. C'est ce que le § 5.1
#            du ré-audit annonçait : avec `sphere_of(box)` la sphère circonscrit
#            l'AABB, donc son certificat est dominé par Corner64.
#   `NONE` : placé APRÈS `h_any_upper`, il coupe EXACTEMENT ZÉRO sous-arbre de
#            plus. Les deux ne répondent pas à la même question — le cœur compte
#            les témoins UNIVERSELS, et `h_any_upper` décide finement « aucun
#            point de Z n'est universel », là où la boule exige « aucun point
#            n'est témoin d'une seule paire ».
#
# Les portes gravent ces deux réfutations, et vérifient que le chemin reste sûr
# — le ledger d'IDs est désormais PAR LANE, ce que le ré-audit exigeait avant
# tout crédit q3/q4 en bloc.
# ---------------------------------------------------------------------------
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_coeur_boule_sur_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=${fam} --seed=3
            --separation=6 --oracle=160 --coeur=boule --ha=dualtree)
  set_tests_properties(mhgp3v_combined_coeur_boule_sur_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION
    "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")
endforeach()

# LA REFUTATION, EXECUTABLE. Zéro coupe nette, et un plancher non nul sur les
# crédits en bloc pour que la porte ne soit pas verte par vacuité.
add_test(NAME mhgp3v_combined_boule_ext_dominee
  COMMAND mhgp3v_combined_prefilter_probe --points=1000 --family=terrain --seed=3
          --separation=6 --smax=11 --coeur=boule --ha=dualtree)
set_tests_properties(mhgp3v_combined_boule_ext_dominee PROPERTIES
  PASS_REGULAR_EXPRESSION "nonvacuite bulk_boule=[1-9][0-9]* elague_ext=0")

# ---------------------------------------------------------------------------
# AUTO-JOINTURE DUAL-TREE — la seconde branche de Q23, celle que le ré-audit
# conseillait et que je disais ne pas savoir obtenir.
#
# Elle calcule EXACTEMENT les mêmes `h_a` que la jointure ponctuelle à huit
# coins : la récursion ne s'arrête que sur un verdict `ALL` — qui implique le
# verdict ponctuel pour chaque couple des deux boîtes — ou sur un couple
# feuille-feuille, où le test EST le test ponctuel. Ce n'est donc pas un nouveau
# minorant, c'est la même valeur pour moins de travail.
#
# `--verifie-jointure` calcule les DEUX et les confronte point par point : c'est
# la porte métamorphique, et le mutant `dual-sans-masque` — littéralement le
# défaut du P0 q2 transposé au dual-tree — la fait mourir au code 3. Le cutoff
# est mis à `0` dans ces portes pour que les blocs soient réellement exercés :
# au cutoff de production, les petites cellules passent toutes par la branche
# ponctuelle et le masque ne serait jamais sollicité.
# ---------------------------------------------------------------------------
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_dualtree_equivaut_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=${fam} --seed=3
            --separation=6 --verifie-jointure --dual-cutoff=0)
  set_tests_properties(mhgp3v_combined_dualtree_equivaut_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "dualtree verifies=[1-9][0-9]* ecarts=0")
endforeach()

mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_dual_sans_masque "dual-tree != jointure" 3
  --points=400 --family=uniform --seed=3 --separation=6 --verifie-jointure
  --dual-cutoff=0 --inject=dual-sans-masque --juge=200)

# LA CONSEQUENCE, ET PAS SEULEMENT L'ECART. La porte métamorphique ci-dessus dit
# que les comptes divergent ; celle-ci dit ce que ça coûte : sans masque le
# filtre cesse d'être fail-open et ferme des ancres VIVANTES. Il faut un régime
# tendu pour le voir — `smax=32` donne `h_q2=31`, et `separation=1` des cellules
# larges — parce qu'à `s=6, smax=11` le sur-comptage ne suffit pas à franchir le
# seuil. Deux familles, `212` et `1 525` fausses morts.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_dual_ferme_a_tort_amas "ancres fermees a tort" 1
  --points=200 --family=eight_clusters --seed=3 --separation=1 --smax=32
  --oracle=200 --ha=dualtree --dual-cutoff=0 --inject=dual-sans-masque --juge=200)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_dual_ferme_a_tort_terrain "ancres fermees a tort" 1
  --points=200 --family=terrain --seed=3 --separation=1 --smax=32
  --oracle=200 --ha=dualtree --dual-cutoff=0 --inject=dual-sans-masque --juge=200)

# Et le témoin positif : la même configuration tendue, saine, ne ferme rien à
# tort. Sans lui, les deux portes ci-dessus pourraient passer sur un probe
# cassé qui refuserait tout.
foreach(fam eight_clusters terrain)
  add_test(NAME mhgp3v_combined_dualtree_tendu_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=200 --family=${fam} --seed=3
            --separation=1 --smax=32 --oracle=200 --ha=dualtree --dual-cutoff=0)
  set_tests_properties(mhgp3v_combined_dualtree_tendu_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION
    "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")
endforeach()

# Le cutoff de production doit lui aussi rendre les mêmes valeurs : c'est une
# optimisation de parcours, pas un second filtre.
add_test(NAME mhgp3v_combined_dualtree_equivaut_cutoff_production
  COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=eight_clusters --seed=5
          --separation=8 --verifie-jointure)
set_tests_properties(mhgp3v_combined_dualtree_equivaut_cutoff_production PROPERTIES
  PASS_REGULAR_EXPRESSION "dualtree verifies=[1-9][0-9]* ecarts=0 cutoff=256")

# L'autorité exacte à huit coins pour `h_a` — elle ne peut que faire CROITRE le
# compte, donc la fermeture, et l'oracle par `PairId` vérifie qu'elle ne ferme
# jamais à tort.
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_ha_corner8_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=${fam} --seed=3
            --separation=6 --oracle=160 --ha=corner8)
  set_tests_properties(mhgp3v_combined_ha_corner8_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION
    "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")
endforeach()

# ---------------------------------------------------------------------------
# `h_a` PAR BOULE D'APEX — le chemin de la question Q23, gardé mais NON adopté.
#
# Il remplace l'auto-jointure ponctuelle par une descente du seul sous-arbre
# `A`, avec crédit en bloc et élagage. La boule étant inscrite dans le cône
# d'apex, elle MINORE `h_a` exact : le filtre reste fail-open, ce que l'oracle
# par `PairId` vérifie ici — `oracle_faux_morts=0` sur les trois familles.
#
# Il n'est pas la route par défaut, et la mesure dit pourquoi : voir la section
# Q23 de `audits/NOTE_CLAUDE_COEUR_BOULE_FORME_CLOSE_20260815.md`. Ces portes
# existent pour que le chemin reste compilé, exercé et sûr — pas pour le
# promouvoir.
# ---------------------------------------------------------------------------
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_ha_boule_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=160 --family=${fam} --seed=3
            --separation=6 --oracle=160 --ha=boule)
  set_tests_properties(mhgp3v_combined_ha_boule_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION
    "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")
endforeach()

mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_oracle_non_borne "--oracle=500 hors de .0,200." 2
  --points=500 --oracle=500)
# `--compare-corner512` active l'oracle SANS passer par `--oracle`, donc sans sa
# borne. Le ré-audit demandait qu'il reçoive le même cap ; c'est le seul chemin
# par lequel le filet interne est atteignable, et il est désormais exercé.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_compare512_cap "oracle borne a 200 points" 2
  --points=201 --compare-corner512)

# ---------------------------------------------------------------------------
# HARNAIS APPARIE `corner512` / `corner64` — questions Q21 et Q22 de l'auditeur.
#
# Il fait tourner les DEUX predicats sur les MEMES sites et publie trois
# comptes. Ce que chaque colonne prouve :
#
#   `perd=0/0/0`   la borne locale ne certifie JAMAIS ce que corner512 refuse :
#                  corner512 la DOMINE. Ce n'est pas une chance, c'est le
#                  theoreme de `soc64_rect.hpp` — l'evaluation aux coins decide
#                  EXACTEMENT l'enveloppe continue, qu'aucune borne tiree des
#                  seules AABB ne peut donc battre.
#   `faux=0/0/0`   corner512 n'a jamais certifie une lane que la force brute sur
#                  les VRAIES paires du rectangle refute. C'est la reponse
#                  empirique a Q21, cote surete.
#   `gagne=0/x/y`  il certifie STRICTEMENT plus en q3 et q4, jamais en q2 — et
#                  ce zero en q2 est lui-meme un resultat : `h_min_over_boxes`
#                  est exact pour `H`, donc rien a gagner sur cette lane.
#   `corner64_...` la specialisation ponctuelle rend la MEME valeur que la
#                  reference. Elle n'est pas un predicat de plus.
#
# Les planchers `sites=[1-9]` et `gagne=0/[1-9]` interdisent le vert par
# vacuite : une porte qui ne confronte rien, ou qui ne rencontre jamais le cas
# ou les deux predicats different, ne prouve rien.
# ---------------------------------------------------------------------------
foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_combined_corner512_apparie_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=120 --family=${fam} --seed=3
            --separation=6 --smax=11 --compare-corner512)
  set_tests_properties(mhgp3v_combined_corner512_apparie_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION
    "corner512 sites=[1-9][0-9]* gagne=0/[1-9][0-9]*/[1-9][0-9]* perd=0/0/0 faux=0/0/0 corner64_desaccords=0")
endforeach()

# La porte a code qui double les trois regex ci-dessus : sans elle, un code de
# sortie 3 passerait inapercu puisque `PASS_REGULAR_EXPRESSION` l'ignore.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_mutant_corner64_sept_coins "corner64 != corner512" 3
  --points=120 --family=uniform --seed=3 --separation=6 --smax=11
  --compare-corner512 --inject=corner64-sept-coins --juge=120)

# ---------------------------------------------------------------------------
# CŒUR-BOULE EN FORME CLOSE — `prototype/spindle_core_ball.hpp`.
#
# Spécification : audits/NOTE_CLAUDE_COEUR_BOULE_FORME_CLOSE_20260815.md.
# Ces portes gravent la preuve AVANT toute mesure de fermeture, comme la note
# s'y engage. Elles ne mesurent aucun résiduel et ne reçoivent aucune source.
#
# Trois familles de portes, et ce que chacune ne peut pas laisser passer :
#
#   `tangence`   les six points entiers qui encadrent `kappa_2`, `kappa_3` et
#                `kappa_4` à une unité près des DEUX côtés. Le juge y est le
#                fuseau exact en entiers, qui n'emploie aucune constante.
#   `derive`     le terme `-S2` n'est pas décoratif : sans lui la boule déborde
#                du côté de `A` et certifie ce que la paire extrême refute.
#   nuage        complétude — la descente avec élagage et crédit en bloc doit
#                compter EXACTEMENT ce que compte le balayage direct — et
#                sûreté, chaque témoin certifié étant reconfronté au fuseau.
#
# Les planchers `--min-coeurs`, `--min-bulk` et `--min-elagues` interdisent le
# vert par vacuité : une boule toujours vide passerait sinon toutes les portes
# de sûreté et de complétude ci-dessus sans rien exercer.
# ---------------------------------------------------------------------------
add_executable(mhgp3v_core_ball_probe prototype/core_ball_probe.cpp)
target_include_directories(mhgp3v_core_ball_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_core_ball_probe PRIVATE mhgp)
target_compile_options(mhgp3v_core_ball_probe PRIVATE -Wall -Wextra -Werror)

add_test(NAME mhgp3v_core_ball_tangence_code
         COMMAND mhgp3v_core_ball_probe --fixture=tangence)
add_test(NAME mhgp3v_core_ball_tangence
         COMMAND mhgp3v_core_ball_probe --fixture=tangence)
set_tests_properties(mhgp3v_core_ball_tangence PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture tangence juge_ok=6 boule_certifie=3 faux=0")

# LE SEUIL DE SEPARATION, ET UNE ERREUR CORRIGEE. La note annonçait d'abord
# `s > 2 + 1/kappa_q` en supposant la convention WSPD `d >= s max(r)` ; le
# dossier sépare en réalité sur l'ÉCART des boules, `d - r_A - r_B >= s max(r)`,
# donc le seuil est `s > 1/kappa_q` — 2,000 / 3,464 / 3,864. Le cœur naît bien
# plus tôt qu'écrit. Les dix lignes de la fixture encadrent les trois
# constantes, et deux d'entre elles (`s=3,4` en q3, `s=3,84` en q4) les serrent
# à environ un centième : sans elles les mutants de constante survivraient ici.
add_test(NAME mhgp3v_core_ball_seuil_code
         COMMAND mhgp3v_core_ball_probe --fixture=seuil)
add_test(NAME mhgp3v_core_ball_seuil
         COMMAND mhgp3v_core_ball_probe --fixture=seuil)
set_tests_properties(mhgp3v_core_ball_seuil PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture seuil verifies=10 mutant=none")

mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_seuil_mutant_kappa3 "encore sous 1/kappa_3" 3
  --fixture=seuil --inject=boule-kappa3-sur)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_seuil_mutant_kappa4 "encore sous 1/kappa_4" 3
  --fixture=seuil --inject=boule-kappa4-sur)

add_test(NAME mhgp3v_core_ball_derive_code
         COMMAND mhgp3v_core_ball_probe --fixture=derive)
add_test(NAME mhgp3v_core_ball_derive
         COMMAND mhgp3v_core_ball_probe --fixture=derive)
set_tests_properties(mhgp3v_core_ball_derive PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture derive certifies=[1-9][0-9]* faux=0")

foreach(fam uniform eight_clusters terrain)
  add_test(NAME mhgp3v_core_ball_nuage_${fam}
    COMMAND mhgp3v_core_ball_probe --points=400 --family=${fam} --seed=3
            --separation=8 --min-coeurs=200 --min-bulk=50 --min-elagues=500)
endforeach()

# LES MUTANTS. Sans eux les portes ci-dessus sont des affirmations, pas des
# épreuves. Trois cassent la sûreté (code 1), deux la complétude (code 3).
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_kappa3 "certifie" 1
  --fixture=tangence --inject=boule-kappa3-sur)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_kappa4 "certifie" 1
  --fixture=tangence --inject=boule-kappa4-sur)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_rayon_plus_deux "certifie" 1
  --fixture=tangence --inject=boule-rayon-plus-deux)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_inclus_large "certifie" 1
  --fixture=tangence --inject=boule-inclus-large)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_derive "hors du fuseau" 1
  --fixture=derive --inject=boule-oublie-derive)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_disjoint_centre "l'elagage ou le credit en bloc perd" 3
  --points=400 --family=uniform --seed=3 --separation=8 --inject=boule-disjoint-centre)

# ---------------------------------------------------------------------------
# LA BOULE D'APEX — `h_a` sans auto-jointure quadratique (question Q23).
#
# La région de `h_a` n'est PAS la boule du cœur : celle-ci est centrée à
# l'équateur du fuseau, loin de `A`. Les témoins de `A` sont près du PÔLE `a`,
# et la région y est un CÔNE d'apex `a`, de demi-ouverture
# `gamma_q = theta'_q - arcsin((r_B + 2 r_A)/D)`. Un cône contient des boules
# inscrites, et c'est celle-là que le sujet calcule.
#
# Les deux fixtures balaient un plan de `A` et confrontent chaque point au
# FUSEAU EXACT sur la paire `(a,b)` — `B` étant réduit à un point, la force
# brute est complète, sans aucune approximation côté juge.
#
#   `faux`  doit rester nul : c'est la sûreté.
#   `rates` mesure ce que la boule inscrite perd contre le cône. Il n'est PAS
#           nul, et c'est le prix documenté de Q23 : 95,0 % de couverture en
#           configuration éloignée, 81,1 % en configuration serrée.
# ---------------------------------------------------------------------------
add_test(NAME mhgp3v_core_ball_apex_code
         COMMAND mhgp3v_core_ball_probe --fixture=apex)
add_test(NAME mhgp3v_core_ball_apex
         COMMAND mhgp3v_core_ball_probe --fixture=apex)
set_tests_properties(mhgp3v_core_ball_apex PROPERTIES PASS_REGULAR_EXPRESSION
  "apex lane q4 certifies=7820 vrais=8342 couverture=93\\.743")

add_test(NAME mhgp3v_core_ball_apex_serre_code
         COMMAND mhgp3v_core_ball_probe --fixture=apex-serre)
add_test(NAME mhgp3v_core_ball_apex_serre
         COMMAND mhgp3v_core_ball_probe --fixture=apex-serre)
set_tests_properties(mhgp3v_core_ball_apex_serre PROPERTIES PASS_REGULAR_EXPRESSION
  "apex lane q4 certifies=5779 vrais=8110 couverture=71\\.258")

mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_apex_mutant_oublie_rayon "hors du fuseau" 1
  --fixture=apex --inject=apex-oublie-rayon)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_apex_mutant_racine_oubliee "hors du fuseau" 1
  --fixture=apex --inject=apex-racine-oubliee)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_apex_mutant_coeff "hors du fuseau" 1
  --fixture=apex --inject=apex-coeff-echange)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_apex_serre_mutant_rayon "hors du fuseau" 1
  --fixture=apex-serre --inject=apex-oublie-rayon)

# ---------------------------------------------------------------------------
# LA GARDE DE SIGNE SUR `gamma_q` — P0 du ré-audit du 15 août, et il était réel.
#
# `num` vaut `(sqrt(mult W) - N)^2` : un CARRÉ, donc positif des deux côtés de
# zéro. `W > 0` ne dit que `N < D`, jamais le signe de `gamma_q`. Quand celui-ci
# est négatif le cône est vide, et l'ancien code en tirait un rayon positif.
# La géométrie de l'auditeur, séparée dès `s=1`, le montre en une lane :
# `U=4000000`, `N=1999`, `W=3999`, `2W=7998` contre `N^2=3996001`.
#
# Le mutant `apex-sans-garde` restaure exactement l'ancien comportement et doit
# mourir : sans lui, cette porte ne prouverait pas que la garde sert.
# ---------------------------------------------------------------------------
add_test(NAME mhgp3v_core_ball_apex_signe_code
         COMMAND mhgp3v_core_ball_probe --fixture=apex-signe)
add_test(NAME mhgp3v_core_ball_apex_signe
         COMMAND mhgp3v_core_ball_probe --fixture=apex-signe)
set_tests_properties(mhgp3v_core_ball_apex_signe PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture apex-signe faux=0 refuses=2")

mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_apex_mutant_sans_garde "gamma negatif accepte" 1
  --fixture=apex-signe --inject=apex-sans-garde)

# ---------------------------------------------------------------------------
# LA BORNE COUPLÉE — section 3.3 du ré-audit.
#
# `R_dec` maximise séparément le déplacement du milieu et la perte de longueur,
# alors que les deux sont liés par UNE SEULE contrainte quadratique. La borne
# couplée conserve cette corrélation, et elle est SATURÉE : tout arrondi du
# mauvais côté crée de vrais faux crédits, ce n'est pas une marge théorique.
#
# `seuil-couple` grave les bascules RÉELLES de l'implémentation — `830 / 2003 /
# 2355` en `L2` — contre `828,43 / 2000,00 / 2350,66` en continu : les arrondis
# dirigés coûtent deux dixièmes de pour cent. À comparer aux seuils de `R_dec`,
# `2001 / 3466 / 3866` : la couplée naît à `39 %` de la séparation qu'exige la
# décorrélée.
#
# `couple-sature` construit l'adversaire exact où Cauchy est une égalité —
# `w` antiparallèle à l'axe, `p` orthogonale, dans le rapport `(2 kappa, 1)` —
# et vérifie que le point critique, exactement sur la sphère diamétrale donc
# `H = 0`, est REFUSÉ. C'est la porte de sûreté : le mutant qui sous-estime le
# facteur l'accepte et meurt au code 1.
# ---------------------------------------------------------------------------
add_test(NAME mhgp3v_core_ball_seuil_couple_code
         COMMAND mhgp3v_core_ball_probe --fixture=seuil-couple)
add_test(NAME mhgp3v_core_ball_seuil_couple
         COMMAND mhgp3v_core_ball_probe --fixture=seuil-couple)
set_tests_properties(mhgp3v_core_ball_seuil_couple PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture seuil-couple verifies=8 dec_q4=1105 coup_q4=1888")

# LE CONTRE-EXEMPLE DU RÉ-AUDIT : une unité de rayon en trop N'EST PAS absorbée
# par l'inégalité stricte. Le raisonnement fautif portait sur `R4`, alors que la
# comparaison porte sur `R4^2` — une unité y devient environ `2 R4` — et les
# distances de grille sont des racines, pas des entiers.
add_test(NAME mhgp3v_core_ball_rayon_plus_un_code
         COMMAND mhgp3v_core_ball_probe --fixture=rayon-plus-un)
add_test(NAME mhgp3v_core_ball_rayon_plus_un
         COMMAND mhgp3v_core_ball_probe --fixture=rayon-plus-un)
set_tests_properties(mhgp3v_core_ball_rayon_plus_un PROPERTIES PASS_REGULAR_EXPRESSION
  "rayon-plus-un R4=8 boule=0 fuseau=0")
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_rayon_plus_un "la stricte n.absorbe pas" 1
  --fixture=rayon-plus-un --inject=boule-rayon-plus-un)

# `--vrai-vivant` est FAUX dès qu'une masse est hors cap : les paires des
# rectangles capés sont ajoutées au résiduel sans jamais être testées, donc le
# compte n'est qu'un minorant. Contre-rejeu de l'auditeur : sur soixante points,
# cap 512 donne 1594 et cap 1 donne 1201, les deux sortant code zéro.
#
# `--cap=refus` est désormais EXPLICITE dans cette porte. Le défaut est passé à
# `--cap=scission`, qui raffine au lieu de rejeter et ne produit donc plus
# jamais de masse hors cap : sans ce drapeau la porte deviendrait verte par
# disparition de son objet, ce qui est la pire façon de perdre un test.
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_vrai_vivant_hors_cap "paires hors cap" 3
  --points=60 --family=uniform --seed=3 --separation=8 --smax=11
  --cap=refus --cap-cellule=1 --vrai-vivant)

# ---------------------------------------------------------------------------
# LA SCISSION RÉCURSIVE — P0.2 de l'audit positif du `00cf78c`.
#
# « Le plus simple est de subdiviser récursivement les rectangles dépassant le
# cap en coupant le plus gros nœud, jusqu'à obtenir une couverture complète. »
# C'est fait, et c'est le défaut. Trois choses à garder en tête :
#
#   * la séparation N'EST PAS monotone sous raffinement — la sphère
#     circonscrite à une AABB ne l'est pas — donc chaque sous-rectangle est
#     RE-TESTÉ, jamais hérité ;
#   * la récursion s'arrête au pire sur deux feuilles, qui sont des points,
#     donc elle termine et atteint toujours le cap ;
#   * `masse_non_decide` vaut alors zéro par construction, ce que la porte
#     ci-dessous exige à un cap volontairement minuscule.
#
# Le cap `4` sur `n=200` force la scission sur presque tout l'arbre : c'est le
# régime où un défaut de couverture se verrait.
foreach(fam uniform terrain eight_clusters two_lines)
  add_test(NAME mhgp3v_combined_scission_couverture_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=200 --family=${fam} --seed=3
            --separation=8 --smax=11 --cap=scission --cap-cellule=4)
  set_tests_properties(mhgp3v_combined_scission_couverture_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "non_decides=0 masse=19900 masse_non_decide=0 ")
endforeach()

# ---------------------------------------------------------------------------
# LA CONTRE-FAMILLE, ENFIN BRANCHÉE — et elle me réfute.
#
# `two_lines` porte une masse universelle QUADRATIQUE avec ZÉRO porteur aigu
# q3/q4 : aucun certificat universel sound ne peut fermer ses `m^2 = n^2/4`
# paires croisées, puisque chacune a une sphère vide. J'avais écrit que
# « l'ensemble `W`-vivant ne devient pas quadratique » sur la foi de trois
# familles ; l'audit du `00cf78c` a raison, et la mesure le confirme sur mon
# propre code :
#
#   n     q2       q3          q4          exposants q4
#   300   2 945    25 110      24 828
#   600   5 945    95 310      94 728      1,932
#   1200  11 945   370 710     369 528     1,964
#   2400  23 945   1 461 510   1 459 128   1,981
#
# `q4 / (n/2)^2` vaut `1,1035` puis `1,0525`, `1,0265`, `1,0133` : la
# convergence vers `m^2` PAR LE HAUT est la prédiction exacte de la famille.
#
# Le fait vraiment instructif est que **q2 reste linéaire** : le demi-espace
# `H>0` tue les paires croisées, les fuseaux plus étroits de q3/q4 ne les tuent
# pas. La quadratique est donc propre aux lanes aiguës, et seule la positivité
# aiguë peut la couper — pas un cinquième certificat géométrique.
#
# `mou=1,000` sur les trois lanes : le préfiltre y est déjà optimal.
add_test(NAME mhgp3v_combined_two_lines_quadratique
  COMMAND mhgp3v_combined_prefilter_probe --points=600 --family=two_lines --seed=3
          --separation=8 --smax=11 --vrai-vivant)
set_tests_properties(mhgp3v_combined_two_lines_quadratique PROPERTIES
  PASS_REGULAR_EXPRESSION
  "V4_pair_walive=94728 V4_survivantes_D0exclu=94728 V4_mou=1\\.000")

# Les deux familles de balayage, que ce probe refusait alors que le générateur
# les produit depuis toujours. Mes rampes ne disaient donc rien d'elles.
foreach(fam scanline_single_pass scanline_overlap_multiecho)
  add_test(NAME mhgp3v_combined_famille_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=300 --family=${fam} --seed=3
            --separation=8 --smax=11 --vrai-vivant)
  set_tests_properties(mhgp3v_combined_famille_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "V4_pair_walive=[1-9][0-9]* ")
endforeach()

mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_famille_inconnue "famille inconnue" 2
  --points=200 --family=poisson_homogene)

# ---------------------------------------------------------------------------
# LES ÉTAGES SÉPARÉS — `V4_pair_walive` contre `C4_carrier`.
#
# L'audit `eb42b57` a raison : le compteur que j'appelais `q4` faisait quatre
# métiers. Il ne mesure que des PAIRES-ANCRAGES `W4`-vivantes. Le porteur ternaire
# aigu est un autre étage, et c'est lui qui décide.
#
# LE LEMME, VÉRIFIÉ ET NON SUPPOSÉ. `(a,b)` étant l'arête maximale,
#
#     x est un porteur aigu de (a,b)   <=>   x dans L(a,b)  ET  H(a,x,b) < 0
#
# avec `H = (x-a).(b-x)`. Les angles en `a` et `b` sont gratuits : si l'angle en
# `a` valait `>= 90`, alors `|bx|^2 >= |ax|^2 + |ab|^2 > |ab|^2` et `x` sortirait
# de la lentille. Vérifié à `331 857` triplets sur quatre régimes, dont une
# grille `3^3` volontairement dégénérée, sans un écart. La stricte compte :
# `H = 0` est l'angle DROIT, et mon premier jet écrivait `H <= 0`, pour `185`
# écarts sur `8 005` ancres — tous ce cas-là.
#
# Équivalent à la condition de l'audit `|ax|^2 + |bx|^2 > |ab|^2`, puisque
# `|ax|^2 + |bx|^2 - |ab|^2 = -2H` : c'est bien la condition COUPLÉE, pas trois
# bornes indépendantes qui perdraient la corrélation.
#
# CE QUE `two_lines` IMPOSE. `Theta(n^2)` paires `W4`-vivantes, et **zéro**
# porteur. La contraction doit valoir exactement `1`.
# ---------------------------------------------------------------------------
# PORTEURS D'UNE ANCRE VIVANTE : `O(h)`, ET NON `O(n)`.
#
# Une ancre `W_4`-vivante a moins de `h_4 = 8` points dans son fuseau ; le
# rapport lentille/`W_4` valant `10,86`, elle ne peut porter que quelques
# dizaines de porteurs sous densité locale uniforme. Mesuré, `s=8` :
#
#   famille          n=200   n=400   n=800    max a n=800
#   terrain           4,99    5,39    5,86        99
#   uniform          16,10   18,58   21,49       136
#   eight_clusters   23,21   35,67   48,52       430
#
# et `C4` restreint aux ancres VIVANTES croît en `n^1,23` / `n^1,47` / `n^1,70`
# — SOUS-QUADRATIQUE sur les trois familles.
#
# CE QUE CELA CORRIGE. Le `n^3,01` mesuré par le gateway ternaire ne venait pas
# de ce que « les triangles aigus seraient cubiques en général », mais de ce que
# ce probe-la n'a AUCUN filtre d'ancre — ni exact, ni même le préfiltre. Avec le
# seul préfiltre, `C4` est déjà en `n^1,30`.
#
# CE QUI N'EST PAS BORNE, et qu'un kernel GPU doit traiter : la moyenne croît
# lentement (`23,2` à `48,5` sur `eight_clusters`) et le MAXIMUM croît à peu
# près linéairement (`110`, `222`, `430`). Un buffer dimensionné sur la moyenne
# déborderait ; il faut un cap déclaré par seed et un chemin de débordement.
add_test(NAME mhgp3v_porteurs_par_ancre_vivante
  COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=terrain --seed=3
          --separation=8 --smax=11 --seeds)
set_tests_properties(mhgp3v_porteurs_par_ancre_vivante PROPERTIES
  PASS_REGULAR_EXPRESSION
  "C4_sur_V4=65881 C4_sur_V4_moy=5\\.39 C4_sur_V4_max=51 C4_sur_morts=70073 C4_sur_morts_max=59")

foreach(fam two_lines uniform terrain eight_clusters)
  add_test(NAME mhgp3v_seed_elagage_exact_${fam}
    COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=${fam} --seed=3
            --separation=8 --smax=11 --verifie-seed)
  set_tests_properties(mhgp3v_seed_elagage_exact_${fam} PROPERTIES
    PASS_REGULAR_EXPRESSION "ecarts=0 .*certif_juges=[1-9][0-9]* certif_desaccords=0")
endforeach()

# LA PORTE QUE L'AUDIT RÉCLAME, avec ses deux étages sur la même ligne.
add_test(NAME mhgp3v_two_lines_etages_separes
  COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=two_lines --seed=3
          --separation=8 --smax=11 --verifie-seed)
set_tests_properties(mhgp3v_two_lines_etages_separes PROPERTIES
  PASS_REGULAR_EXPRESSION
  "S4_prefilter_survivor=43128 V4_pair_walive_exact=43128 V4_sans_carrier=43128 contraction_V4=1\\.0000 mou_S4_sur_V4=1\\.000(.|
)*C4_carrier=0 C4_sur_V4=0 C4_sur_V4_moy=0\\.00 C4_sur_V4_max=0 C4_sur_morts=0 C4_sur_morts_max=0 C4_carrier_weak_owner=0 C4_carrier_elag=0 ecarts=0")

# ---------------------------------------------------------------------------
# L'ÉLAGAGE PAR BLOC : deux disjoints, et je note ce que j'ai réfuté en route.
#
#   `bloc_hors_lentille`          la boîte est hors d'une des deux boules
#                                 `B(a,|ab|)`, `B(b,|ab|)` — élague le LOINTAIN,
#                                 et c'est lui qui rend la descente logarithmique ;
#   `bloc_dans_boule_diametrale`  la boîte est dans la boule diamétrale fermée,
#                                 soit `H >= 0` partout — c'est lui qui tue
#                                 `two_lines`, dont la lentille est pleine mais
#                                 entièrement non aiguë.
#
# DEUX RÉFUTATIONS MESURÉES, gardées ici parce qu'elles coûtent cher à redécouvrir :
#
#   * certificat au niveau du RECTANGLE `A x B` — la structure de `h_coeur`,
#     qui vaudrait pour toutes les ancres d'un coup : `gain=1,005`,
#     `90 324` blocs pour `99 210` points, soit `1,1` point par bloc. Le
#     quantificateur universel sur `A x B` est trop fort, il échoue dès qu'une
#     seule paire du rectangle rend un `x` aigu.
#   * `bloc_dans_boule_diametrale` SEUL, sans le disjoint de lentille :
#     `gain = 0,33` à `0,59` — PLUS LENT que le balayage complet. La boule
#     diamétrale est petite devant l'emprise, donc « boîte incluse » ne peut
#     jamais élaguer près de la racine, et la descente parcourait `657` nœuds
#     sur `799`.
#
# Avec les deux : `13,29x` sur `two_lines` (tous les points élagués, `~30`
# visites de nœud par ancre), `4,53x` sur `terrain`, `1,68x` sur `uniform`,
# `1,20x` sur `eight_clusters`.
#
# Le coût est compté en ÉVALUATIONS DU PRÉDICAT des deux côtés — une par nœud
# visité, une par point testé. Comparer des tests de feuilles à des tests de
# points aurait fabriqué un gain, exactement comme le dual-tree l'avait fait.
# ---------------------------------------------------------------------------
# LE FACTEUR DEUX DES FACES INCIDENTES, RÉFUTÉ EN COORDONNÉES ENTIÈRES.
#
# `NOTE_AUDITEUR_ORDRE_EXECUTION_APRES_5CE2634` donne le contre-exemple ; je l'ai
# vérifié plutôt que rapporté. `p0=(6,2,5) p1=(0,3,3) p2=(1,4,6) p3=(5,3,1)` :
# arêtes au carré `41 30 18 11 29 42`, owner `p2p3` de multiplicité **un** donc
# unique sans tie-break, circumcentre rationnel exact `(83/26, 81/26, 97/26)`
# équidistant à `7259/676`. Des deux faces incidentes à l'owner, `(p0,p2,p3)` est
# aiguë et `(p1,p2,p3)` ne l'est pas.
#
# La borne devient `N4_event <= R4_bundle`, sans le facteur deux.
#
# LE CONTRASTE FAIT LA NON-VACUITÉ : le tétraèdre régulier — quatre sommets
# alternés de `{0,2}^3`, arêtes toutes égales à `8` — a ses deux faces
# incidentes aiguës. Le total `1 + 2 = 3` est le plancher ; `4` signifierait que
# le facteur deux est revenu, `2` que la fixture ne voit plus le cas régulier.
# ---------------------------------------------------------------------------
# LES FRONTIÈRES EXACTES DE `pair_lane`, ET LES PAIRES `D = 0`.
#
# Priorité 2 de l'ordre d'exécution. Les trois lanes sont séparées par des
# ÉGALITÉS — `H = 0`, `4H^2 = ET`, `3H^2 = ET` — et une égalité ne se teste pas
# par un nuage aléatoire : il faut des coordonnées entières qui la réalisent.
# Dix cas gravés, décalés de `+1000` pour rester dans le profil u16 :
#
#   H_nul         `H = 0`, ET = 486        -> lane 0, non porteur (angle DROIT)
#   q4_frontiere  `3H^2 = ET = 243`        -> lane 3, q4 refusé à l'égalité
#   q3_frontiere  `4H^2 = ET = 324`        -> lane 2, q3 refusé à l'égalité
#   q4_strict     `3H^2 = 2187 > 729`      -> lane 4
#   q3_strict     `243 < 297 < 324`        -> lane 3
#   q2_strict     `3H^2 = 432 < ET = 594 < 4H^2 = 576`? non : `594 > 576` -> lane 2
#   z_egale_a / z_egale_b / a_egale_b      -> lane 0, aucun témoin
#   porteur       `H = -7`, dans la lentille -> porteur aigu
#
# Sans les intérieurs stricts la fixture ne distinguerait pas `>` de `>=` ; sans
# les `D = 0` elle laisserait passer les doublons quantifiés. Planchers : les
# quatre lanes `0/2/3/4` doivent toutes apparaître, et au moins un porteur.
add_test(NAME mhgp3v_frontieres_lanes_et_d0
  COMMAND mhgp3v_combined_prefilter_probe --fixture=frontieres)
set_tests_properties(mhgp3v_frontieres_lanes_et_d0 PROPERTIES
  PASS_REGULAR_EXPRESSION "frontiere cas=10 faux=0 lanes_couvertes=4 porteurs=1")

add_test(NAME mhgp3v_owner_facteur_deux_refute
  COMMAND mhgp3v_combined_prefilter_probe --fixture=owner-porteurs)
set_tests_properties(mhgp3v_owner_facteur_deux_refute PROPERTIES
  PASS_REGULAR_EXPRESSION
  "tetra=auditeur arete_max=42 owner=\\(2,3\\) multiplicite=1 faces_incidentes_aigues=1(.|\n)*tetra=regulier arete_max=8 owner=\\(0,1\\) multiplicite=6 faces_incidentes_aigues=2(.|\n)*facteur_deux=REFUTE")

# ---------------------------------------------------------------------------
# EXACT-ONCE DE LA SCISSION — priorité 1 de l'ordre d'exécution de l'auditeur.
#
# La scission récursive change la partition ; il faut donc prouver qu'elle
# couvre encore chaque `PairId` **exactement une fois**. Les caps minuscules
# forcent la récursion jusqu'au fond — `cap=2` descend `cellule_max` à `2` — et
# l'oracle vérifie la couverture par `PairId`, pas par masse agrégée.
foreach(c 512 8 2)
  foreach(fam terrain uniform two_lines)
    add_test(NAME mhgp3v_scission_exact_once_${fam}_cap${c}
      COMMAND mhgp3v_combined_prefilter_probe --points=120 --family=${fam} --seed=3
              --separation=8 --smax=11 --oracle=200 --cap=scission --cap-cellule=${c})
    set_tests_properties(mhgp3v_scission_exact_once_${fam}_cap${c} PROPERTIES
      PASS_REGULAR_EXPRESSION
      "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")
  endforeach()
endforeach()

add_test(NAME mhgp3v_seed_elagage_gain_two_lines
  COMMAND mhgp3v_combined_prefilter_probe --points=400 --family=two_lines --seed=3
          --separation=8 --smax=11 --seeds)
set_tests_properties(mhgp3v_seed_elagage_gain_two_lines PROPERTIES
  PASS_REGULAR_EXPRESSION "points_elagues=17251200 ")

add_test(NAME mhgp3v_core_ball_couple_sature_code
         COMMAND mhgp3v_core_ball_probe --fixture=couple-sature)
add_test(NAME mhgp3v_core_ball_couple_sature
         COMMAND mhgp3v_core_ball_probe --fixture=couple-sature)
set_tests_properties(mhgp3v_core_ball_couple_sature PROPERTIES PASS_REGULAR_EXPRESSION
  "fixture couple-sature faux=0 acceptes=1 refuses=1")

mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_coup_c "hors du fuseau" 1
  --fixture=couple-sature --inject=coup-sous-estime-c)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_mutant_coup_racine "attendu vide" 3
  --fixture=seuil-couple --inject=coup-plancher-racine)

# L'ablation : la borne couplée doit trouver STRICTEMENT plus de témoins que la
# décorrélée sur un vrai nuage, sans quoi elle ne servirait à rien. Et le mode
# `--rayon=dec` reste sûr, ce qui interdit de le confondre avec une régression.
add_test(NAME mhgp3v_core_ball_ablation_rayon_dec
  COMMAND mhgp3v_core_ball_probe --points=400 --family=terrain --seed=3
          --separation=6 --rayon=dec --min-coeurs=200 --min-bulk=50 --min-elagues=500)
set_tests_properties(mhgp3v_core_ball_ablation_rayon_dec PROPERTIES
  PASS_REGULAR_EXPRESSION "lane q4 coeurs_non_vides=[0-9]+ direct=50656 descente=50656 faux=0")
# Point 11 du plan : sphère englobant les POINTS plutôt que l'AABB. Deux usages
# à ne pas confondre — serrer la sphère de SEPARATION rend la séparation plus
# facile, donc produit des rectangles moins séparés et des cœurs plus PETITS.
# La partition reste sur la sphère de l'AABB ; seul le rayon du cœur est serré.
add_test(NAME mhgp3v_core_ball_sphere_points
  COMMAND mhgp3v_core_ball_probe --points=400 --family=terrain --seed=3
          --separation=6 --sphere=points --min-coeurs=200 --min-bulk=50 --min-elagues=500)
set_tests_properties(mhgp3v_core_ball_sphere_points PROPERTIES
  PASS_REGULAR_EXPRESSION "lane q4 coeurs_non_vides=[0-9]+ direct=62322 descente=62322 faux=0")

add_test(NAME mhgp3v_core_ball_ablation_rayon_max
  COMMAND mhgp3v_core_ball_probe --points=400 --family=terrain --seed=3
          --separation=6 --min-coeurs=200 --min-bulk=50 --min-elagues=500)
set_tests_properties(mhgp3v_core_ball_ablation_rayon_max PROPERTIES
  PASS_REGULAR_EXPRESSION "lane q4 coeurs_non_vides=[0-9]+ direct=61462 descente=61462 faux=0")

# ---------------------------------------------------------------------------
# LE PROFIL u16 EST ANNONCÉ, DONC IMPOSÉ. Le ré-audit a trouvé que
# `--coord=2147483647` était accepté, débordait sous UBSan et pouvait fermer à
# tort ; que `--points=201 --oracle=1` lançait l'oracle hors de sa borne
# annoncée ; que `--oracle=-1` passait ; que `--points=4294967298` se repliait
# sur `n=2` ; et que `--coord=12x` était lu comme `12`. Cinq refus négatifs, et
# une porte POSITIVE à `65535` pour que la borne ne soit pas un mur qui interdit
# le domaine légitime.
# ---------------------------------------------------------------------------
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_coord_hors_u16 "hors de .1,65535." 2
  --points=40 --coord=2147483647)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_oracle_borne_fausse "depasse la borne --oracle" 2
  --points=201 --oracle=1)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_oracle_negatif "hors de .0,200." 2
  --oracle=-1 --points=40)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_points_repliage "hors de .1,2000000." 2
  --points=4294967298)
mhgp3v_add_expected_code_test_for(mhgp3v_combined_prefilter_probe
  mhgp3v_combined_refus_coord_suffixe "valeur non entiere" 2
  --points=40 --coord=12x)
mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_refus_coord_hors_u16 "hors de .1,65535." 2
  --points=40 --coord=2147483647)

add_test(NAME mhgp3v_combined_domaine_u16_haut
  COMMAND mhgp3v_combined_prefilter_probe --points=40 --coord=65535 --oracle=40)
set_tests_properties(mhgp3v_combined_domaine_u16_haut PROPERTIES
  PASS_REGULAR_EXPRESSION
  "oracle_paires=[1-9][0-9]* oracle_faux_morts=0 oracle_ids_doubles=0 oracle_couverture_ko=0")

mhgp3v_add_expected_code_test_for(mhgp3v_core_ball_probe
  mhgp3v_core_ball_refus_non_borne "--points=9000 hors de .1,4000." 2
  --points=9000)
