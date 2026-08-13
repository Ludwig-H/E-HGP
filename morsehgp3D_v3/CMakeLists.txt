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

# LA PORTE ENSEMBLISTE DE L'INDEX PREFIXE--PREFIXE (note index exact) : le
# theoreme juge contre le join quadratique pour TOUT k, fixtures des communs
# en dernieres positions et du faux candidat, identite de masse L(r,K),
# planchers de paires ET de faux candidats. Combinatoire pure : aucun lien.
add_executable(mhgp3v_prefix_index_gate prototype/prefix_index_gate.cpp)
target_include_directories(mhgp3v_prefix_index_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(mhgp3v_prefix_index_gate PRIVATE -Wall -Wextra -Werror)

# LA SONDE DE MASSE de l'index prefixe--prefixe : diagnostic tout-requete sur
# un catalogue reel (bornes hautes du fallback), jamais un verdict.
add_executable(mhgp3v_prefix_mass_probe prototype/prefix_mass_probe.cpp)
target_include_directories(mhgp3v_prefix_mass_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_prefix_mass_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_prefix_mass_probe PRIVATE -Wall -Wextra -Werror)

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

# LE PRUNE CONVEXE CERTIFIE (reponse pont/q4 20260811 §6) : le predicat
# partage axes + plan du barycentre, verification entiere seule decisive.
add_executable(mhgp3v_cell_prune_gate prototype/cell_prune_gate.cpp)
target_include_directories(mhgp3v_cell_prune_gate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_cell_prune_gate PRIVATE mhgp)
target_compile_options(mhgp3v_cell_prune_gate PRIVATE -Wall -Wextra -Werror)

# LA SONDE DE MASSE DE LA SOURCE PAR CELLULES DE CENTRES (reponse 20260811) :
# theoreme cellule--candidat, temoins top-t_q par anneaux, dilations exactes —
# le prealable de decision 50 k, sans former un seul tuple.
add_executable(mhgp3v_cell_source_mass_probe prototype/cell_source_mass_probe.cpp)
target_include_directories(mhgp3v_cell_source_mass_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_cell_source_mass_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_cell_source_mass_probe PRIVATE -Wall -Wextra -Werror)

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
# LA PORTE MULTIENSEMBLISTE de l'index : autorite combinatoire du theoreme,
# possession canonique par lots, preflight fige, permutation globale commune.
add_test(NAME mhgp3v_prefix_index_differential COMMAND mhgp3v_prefix_index_gate
         --families 400 --sets 24 --universe 30 --max-order 8 --seed 20260810)
# LA TERMINAISON HOSTILE (audit 8df7ac8 P2) : universe=8 < 11 bouclait a
# l'infini ; le rang est desormais borne par min(11, universe) et la campagne
# TERMINE — planchers reduits a la non-vacuite de paires.
add_test(NAME mhgp3v_prefix_index_universe8 COMMAND mhgp3v_prefix_index_gate
         --families 20 --sets 8 --universe 8 --max-order 3 --seed 20260810
         --min-pairs 1 --min-false 0 --min-qq 0 --min-prior 0 --min-later 0 --min-rr 0)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_length "mutant tue" 4
  --families 50 --mutant prefix-length-minus-one)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_drop_last "mutant tue" 4
  --families 50 --mutant drop-last-posting)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_skip_recert "mutant tue" 4
  --families 50 --mutant skip-recertification)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_duplicate_posting "mutant tue" 4
  --families 50 --mutant duplicate-posting)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_double_query "mutant tue" 4
  --families 50 --mutant double-query-pair)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_order_per_query "mutant tue" 4
  --families 50 --mutant order-per-query)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_stage_sequentially "mutant tue" 4
  --families 50 --mutant stage-query-sequentially)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_kill_future_visible "mutant tue" 4
  --families 50 --mutant future-visible)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_index_gate
  mhgp3v_prefix_index_reject_unknown_mutant "mutant inconnu" 2 --mutant inexistant)
add_test(NAME mhgp3v_prefix_mass_probe_diagnostic COMMAND mhgp3v_prefix_mass_probe
         --points 48 --smax 11 --max-order 3 --seed 20260810)
set_tests_properties(mhgp3v_prefix_mass_probe_diagnostic PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse terminee")
# LE MASQUE HYBRIDE de la sonde (audit 8df7ac8 P0) : staging par lot, requetes
# = les fallback reels du fold, preflight fige, percentiles H_Q — le recu qui
# dimensionne les chunks device. Les deux familles scanline (reponse Q5) sont
# le regime declare proche LiDAR : ex aequo massifs, stries, multi-echos.
add_test(NAME mhgp3v_prefix_mass_probe_hybrid_mask COMMAND mhgp3v_prefix_mass_probe
         --points 32 --smax 11 --max-order 3 --seed 20260810 --mask hybrid)
set_tests_properties(mhgp3v_prefix_mass_probe_hybrid_mask PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse terminee \\(masque hybride")
add_test(NAME mhgp3v_prefix_mass_probe_scanline COMMAND mhgp3v_prefix_mass_probe
         --points 32 --smax 11 --max-order 3 --seed 20260810
         --family scanline_single_pass --mask hybrid)
set_tests_properties(mhgp3v_prefix_mass_probe_scanline PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse terminee \\(masque hybride")
add_test(NAME mhgp3v_prefix_mass_probe_scanline_overlap COMMAND mhgp3v_prefix_mass_probe
         --points 32 --smax 11 --max-order 3 --seed 20260810
         --family scanline_overlap_multiecho --mask hybrid)
set_tests_properties(mhgp3v_prefix_mass_probe_scanline_overlap PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse terminee \\(masque hybride")
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_mass_probe
  mhgp3v_prefix_mass_probe_reject_unknown "argument inconnu" 2 --inexistant 1)
mhgp3v_add_expected_code_test_for(mhgp3v_prefix_mass_probe
  mhgp3v_prefix_mass_probe_reject_hybrid_threshold "baseline t=1" 2
  --mask hybrid --threshold 2)
# LA SONDE CELLULES : l'identite de dilation (marche par anneaux == balayage
# complet) est verifiee cellule par cellule sur un petit nuage de chaque
# famille ; la CLI hostile est refusee avant tout calcul.
add_test(NAME mhgp3v_cell_source_mass_terrain COMMAND mhgp3v_cell_source_mass_probe
         --points 400 --smax 11 --seed 20260810 --family terrain --verify-bruteforce 1)
set_tests_properties(mhgp3v_cell_source_mass_terrain PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse de la source par cellules terminee")
add_test(NAME mhgp3v_cell_source_mass_scanline COMMAND mhgp3v_cell_source_mass_probe
         --points 400 --smax 11 --seed 20260810 --family scanline_overlap_multiecho
         --verify-bruteforce 1)
set_tests_properties(mhgp3v_cell_source_mass_scanline PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : sonde de masse de la source par cellules terminee")
mhgp3v_add_expected_code_test_for(mhgp3v_cell_source_mass_probe
  mhgp3v_cell_source_mass_reject_unknown "argument inconnu" 2 --inexistant 1)
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

# LA PORTE DU PRUNE : contact conserve, pics lointains separes en local,
# nappe inclinee separee par le plan general ; deux mutants tues.
add_test(NAME mhgp3v_cell_prune_fixtures COMMAND mhgp3v_cell_prune_gate)
set_tests_properties(mhgp3v_cell_prune_fixtures PROPERTIES
  PASS_REGULAR_EXPRESSION "OK : prune convexe certifie")
mhgp3v_add_expected_code_test_for(mhgp3v_cell_prune_gate
  mhgp3v_cell_prune_kill_contact "mutant tue" 4 --mutant contact-as-separation)
mhgp3v_add_expected_code_test_for(mhgp3v_cell_prune_gate
  mhgp3v_cell_prune_kill_from_cloud "mutant tue" 4 --mutant prune-from-cloud)
mhgp3v_add_expected_code_test_for(mhgp3v_cell_prune_gate
  mhgp3v_cell_prune_reject_unknown_mutant "mutant inconnu" 2 --mutant inexistant)

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

# ---------------------------------------------------------------------------
# LA LOCALITE CERTIFIEE PAR INVERSION (note de solution du 12 aout 2026).
#
# Le sujet remplace l'enumeration de C(n,2) par une generation par ancre : la
# calotte exacte C_z(D) = { u : u.v > d/D } au lieu de la chambre de Yao48, un
# rayon PAR CELLULE de direction, et le prédicat entier a trois sommets. Deux
# juges independants le tiennent : le juge A (toute paire lointaine d'une ancre
# certifiee doit avoir K interieurs stricts) et le juge du census (le census
# directionnel doit egaler l'enumeration exhaustive de C(n,2)).
# ---------------------------------------------------------------------------
add_executable(mhgp3v_certified_locality_probe prototype/certified_locality_probe.cpp)
target_include_directories(mhgp3v_certified_locality_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_certified_locality_probe PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_certified_locality_probe PRIVATE -Wall -Wextra -Werror)

# Le theoreme sur les TROIS arites, juge exhaustif a support propre positif.
add_test(NAME mhgp3v_locality_fixture_trois_arites COMMAND mhgp3v_certified_locality_probe
         --mode=fixture --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
         --judge-arity=4 --min-activations=1700 --min-q4=150)
# Le juge A : 48 435 paires lointaines a cette graine, plancher arme a 40 000.
add_test(NAME mhgp3v_locality_juge_paires_lointaines COMMAND mhgp3v_certified_locality_probe
         --mode=fixture --points=600 --family=uniform --grid=4 --seed=1 --kmin=10
         --judge-arity=1 --min-far-pairs=40000)
# Le census directionnel EXACT contre l'enumeration exhaustive, deux familles.
add_test(NAME mhgp3v_locality_census_exact_terrain COMMAND mhgp3v_certified_locality_probe
         --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
         --judge-census=1 --min-activations=15000)
add_test(NAME mhgp3v_locality_census_exact_scanline COMMAND mhgp3v_certified_locality_probe
         --mode=directional --points=900 --family=scanline_overlap_multiecho --grid=4
         --seed=3 --kmin=10 --judge-census=1 --min-activations=15000)
# INVARIANCE DE GRILLE : le filtre etant conservateur, le census ne depend pas
# de la finesse de la discretisation des directions. Deux grilles extremes.
add_test(NAME mhgp3v_locality_census_grille_3 COMMAND mhgp3v_certified_locality_probe
         --mode=directional --points=900 --family=terrain --grid=3 --seed=3 --kmin=10
         --judge-census=1 --min-activations=15000)
add_test(NAME mhgp3v_locality_census_grille_8 COMMAND mhgp3v_certified_locality_probe
         --mode=directional --points=900 --family=terrain --grid=8 --seed=3 --kmin=10
         --judge-census=1 --min-activations=15000)
# INVARIANCE DE FENETRE : la lane close ne depend pas du nombre de voisins lus.
add_test(NAME mhgp3v_locality_census_fenetre_courte COMMAND mhgp3v_certified_locality_probe
         --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
         --max-neighbours=64 --judge-census=1 --min-activations=15000)

# LES CINQ MUTANTS, au code contractuel 4.
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_rho_min_corner "mutant tue" 4
  --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1 --inject=rho-min-corner)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_rho_first_corner "mutant tue" 4
  --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1 --inject=rho-first-corner)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_rho_kth_short "mutant tue" 4
  --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1 --inject=rho-kth-short)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_locate_drop_boundary "mutant tue" 4
  --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1 --inject=locate-drop-boundary)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_cap_corner_any "mutant tue" 4
  --mode=fixture --points=80 --family=uniform --grid=4 --seed=1 --kmin=4
  --inject=cap-corner-any)

# CLI hostile et injections hors mode : code 2, avant tout calcul.
# LE SIGNE InSphere q4. Livre une fois a l'envers : q4 tombait de 202 a 3
# activations a n=70. La fixture gravee (tetraedre de circumcentre l'origine,
# centre interieur, point lointain exterieur) est exercee a chaque execution et
# tue son mutant au code 4 ; le plancher --min-q4 double la garde.
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_mutant_insphere_sign "mutant tue par la fixture d'orientation q4" 4
  --mode=fixture --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --inject=insphere-sign-flip)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_plancher_q4 "plancher q4" 3
  --mode=fixture --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --judge-arity=4 --min-q4=999999999)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_cli_inconnu "argument inconnu" 2 --mode=directional --points=200 --bidon)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_cli_mode_inconnu "mode inconnu" 2 --mode=nimporte --points=200)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_injection_hors_mode_cap "sans effet en mode directionnel" 2
  --mode=directional --points=600 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1 --inject=cap-nine)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_injection_hors_mode_rho "sans effet en mode fixture" 2
  --mode=fixture --points=80 --family=uniform --grid=4 --seed=1 --kmin=4
  --inject=rho-min-corner)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_juge_census_borne "borne a 4000 points" 2
  --mode=directional --points=5000 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_injection_sans_juge "exige --judge-census=1" 2
  --mode=directional --points=600 --family=terrain --grid=4 --seed=3 --kmin=10
  --inject=rho-min-corner)
# Planchers de couverture : code 3, jamais un vert par vacuite.
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_plancher_activations "plancher d'activations" 3
  --mode=directional --points=900 --family=terrain --grid=4 --seed=3 --kmin=10
  --min-activations=999999999)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_plancher_paires_lointaines "plancher de paires lointaines" 3
  --mode=fixture --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --judge-arity=1 --min-far-pairs=999999999)

# LA GENERATION LOCALE DES TROIS ARITES. La fenetre de support est explicite :
# la boule diametrale d'une corde N'EST PAS incluse dans la boule (triangle
# (0,0,0) (120,0,0) (60,100,0), rayon circonscrit 68, portee diametrale 92),
# donc un support q3/q4 peut avoir une arete tombstonee en q2. La mesure n'est
# recevable que si elle sature ; le juge exhaustif des trois arites l'exige.
add_test(NAME mhgp3v_locality_arites_exactes COMMAND mhgp3v_certified_locality_probe
         --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
         --max-neighbours=69 --support-window=40 --judge-census=1 --min-q4=200)
add_test(NAME mhgp3v_locality_arites_terrain COMMAND mhgp3v_certified_locality_probe
         --mode=arity --points=120 --family=terrain --grid=4 --seed=5 --kmin=5
         --max-neighbours=119 --support-window=60 --judge-census=1)
# Fenetre non saturee : la generation manque des supports et le juge le dit.
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_fenetre_non_saturee "la fenetre de support n'a pas sature" 1
  --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --max-neighbours=69 --support-window=20 --judge-census=1)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_arites_mutant_rho "mutant tue par le juge des arites" 4
  --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --max-neighbours=69 --support-window=40 --judge-census=1 --inject=rho-min-corner)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_arites_mutant_locate "mutant tue par le juge des arites" 4
  --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --max-neighbours=69 --support-window=40 --judge-census=1
  --inject=locate-drop-boundary)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_arites_juge_borne "borne a 400 points" 2
  --mode=arity --points=500 --family=terrain --grid=4 --seed=3 --kmin=10
  --judge-census=1)

# LE JUGE INDEPENDANT DU CENSUS. Le juge interne du probe partage les
# constructeurs de sphere et les predicats du sujet : il compare des
# cardinalites obtenues par la meme arithmetique, ce que l'audit du 12 aout a
# refuse comme reception. Celui-ci recalcule le meme census en rationnels
# multiprecision, elimination de Gauss et centre explicite. Seul le generateur
# de nuages est partage.
add_executable(mhgp3v_locality_census_judge oracle/locality_census_judge.cpp)
target_include_directories(mhgp3v_locality_census_judge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_locality_census_judge PRIVATE mhgp)
target_compile_options(mhgp3v_locality_census_judge PRIVATE -Wall -Wextra -Werror)

# LES VALEURS GRAVEES DU JUGE RATIONNEL. Produites hors bande par
# `mhgp3v_locality_census_judge` — rationnels multiprecision, elimination de
# Gauss, centre explicite — elles ne dependent d'aucun predicat du sujet. Le
# juge est relance ici a n=50, et le sujet doit les reproduire a n=50 et n=70.
add_test(NAME mhgp3v_locality_juge_rationnel COMMAND mhgp3v_locality_census_judge
         --points=50 --family=uniform --seed=1 --kmin=4
         --expect-q2=456 --expect-q3=592 --expect-q4=137)
add_test(NAME mhgp3v_locality_arites_gravees_50 COMMAND mhgp3v_certified_locality_probe
         --mode=arity --points=50 --family=uniform --grid=4 --seed=1 --kmin=4
         --max-neighbours=49 --support-window=30
         --expect-q2=456 --expect-q3=592 --expect-q4=137)
add_test(NAME mhgp3v_locality_arites_gravees_70 COMMAND mhgp3v_certified_locality_probe
         --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
         --max-neighbours=69 --support-window=40
         --expect-q2=681 --expect-q3=884 --expect-q4=202)
# CETTE PORTE ETAIT MORTE, ET SON DEFAUT EST DE LA MEME CLASSE QUE LA REGEX
# `{n}` : elle exigeait le diagnostic « mutant tue par les valeurs gravees »
# pour l'injection `insphere-sign-flip`, alors que ce mutant est tue BIEN PLUS
# TOT, par la fixture d'orientation q4, qui rend le code 4 avec un tout autre
# message. Le diagnostic attendu etait donc inatteignable, quelle que soit la
# qualite du sujet. L'injection `insphere-sign-flip` possede par ailleurs deja
# sa propre porte sur la fixture d'orientation : celle-ci n'en etait qu'un
# doublon mal nomme.
#
# `rho-kth-short` traverse au contraire tout le mode `arity` et meurt bien sur
# les valeurs gravees `681/884/202`, avec le diagnostic exact. Trois autres
# injections — `census-drop-ties`, `cap-nine`, `cap-drop-strictness` — SURVIVENT
# a ce chemin : elles ne changent aucun des trois comptes sur cette fixture, et
# elles ont leurs portes ailleurs. La couverture des valeurs gravees repose donc
# sur cette injection-ci et sur `locate-drop-boundary`.
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_gravees_mutant_rho "mutant tue par les valeurs gravees" 4
  --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --max-neighbours=69 --support-window=40
  --expect-q2=681 --expect-q3=884 --expect-q4=202 --inject=rho-kth-short)
mhgp3v_add_expected_code_test_for(mhgp3v_certified_locality_probe
  mhgp3v_locality_gravees_fenetre_courte "desaccord avec le juge rationnel independant" 1
  --mode=arity --points=70 --family=uniform --grid=4 --seed=1 --kmin=4
  --max-neighbours=69 --support-window=20
  --expect-q2=681 --expect-q3=884 --expect-q4=202)
mhgp3v_add_expected_code_test_for(mhgp3v_locality_census_judge
  mhgp3v_locality_juge_rationnel_borne "borne a 400 points" 2 --points=500)

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

# L'ABLATION EST UN MODE DE MESURE, PAS UN MODE DE SORTIE. Elle produit une
# sortie fausse par construction : elle doit donc etre refusee des qu'un juge ou
# un mutant l'accompagne, et s'annoncer en clair.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_ablation_jugee "l'ablation ne peut ni etre jugee" 2
  --points=60 --smax=6 --family=terrain --seed=3 --judge --ablate=2)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_ablation_mutant "l'ablation ne peut ni etre jugee" 2
  --points=60 --smax=6 --family=terrain --seed=3 --ablate=2 --inject=drop-ties)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_ablation_differee "ne partagent pas le meme flot" 2
  --points=60 --smax=6 --family=terrain --seed=3 --ablate=2 --deferred-lift)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_ablation_plancher "aucun plancher sur une sortie ablatee" 2
  --points=60 --smax=6 --family=terrain --seed=3 --ablate=2 --min-supports=1)
add_test(NAME mhgp3v_centre_cell_ablation_annoncee COMMAND mhgp3v_centre_cell
         --points=200 --smax=11 --family=terrain --seed=11 --ablate=3)
set_tests_properties(mhgp3v_centre_cell_ablation_annoncee
                     PROPERTIES PASS_REGULAR_EXPRESSION "SORTIE FAUSSE PAR CONSTRUCTION")

# REFUS AVANT CALCUL : code 2.
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_unknown_argument "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_bad_family "famille inconnue" 2 --family=inexistante)
mhgp3v_add_expected_code_test_for(mhgp3v_ball_front
  mhgp3v_ball_front_reject_judge_too_large "juge exhaustif" 2
  --points=500 --k=1 --judge)

# ---------------------------------------------------------------------------
# SONDE D'ECHELLE de l'arrangement shallow : le VOLUME, que le differentiel
# exhaustif ne peut pas mesurer. Elle instrumente `reverse_search_stream` sur
# des nuages realistes et publie sommets, densite, rang ferme, high-water de la
# pile et travail d'index. Elle ne juge rien et ne mesure aucun SLO.
add_executable(mhgp3v_flats_scale prototype/flats_scale_probe.cpp)
target_include_directories(mhgp3v_flats_scale PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_flats_scale PRIVATE mhgp)
target_compile_options(mhgp3v_flats_scale PRIVATE -Wall -Wextra -Werror)

# PLANCHER CONTRE LE VERT PAR VACUITE : une sonde qui n'enumere rien passerait
# sans lui. La valeur est tenue tres en dessous de la mesure observee pour ne
# pas graver une densite qui n'est pas un theoreme.
add_test(NAME mhgp3v_flats_scale_uniform COMMAND mhgp3v_flats_scale
         --points=120 --smax=6 --family=uniform --seed=101 --min-vertices=5000)
add_test(NAME mhgp3v_flats_scale_multiecho COMMAND mhgp3v_flats_scale
         --points=120 --smax=6 --family=scanline_overlap_multiecho --seed=101
         --min-vertices=2000)
mhgp3v_add_expected_code_test_for(mhgp3v_flats_scale
  mhgp3v_flats_scale_reject_unknown_argument "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_flats_scale
  mhgp3v_flats_scale_reject_floor "PLANCHER non atteint" 3
  --points=40 --smax=4 --min-vertices=100000000)

# ---------------------------------------------------------------------------
# SOURCE PAR LISTES IMBRIQUEES DE CELLULES DE CENTRES.
#
# Architecture proposee par l'auditeur dans
# `audits/NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`. Elle
# subdivise l'espace des CENTRES au lieu de parcourir les sommets de
# l'arrangement, dont la mesure montre qu'ils sont majoritairement des transits.
#
# La propriete decisive est que la liste d'une cellule est a la fois le domaine
# de generation ET le certificat de census : aucun scan global apres la racine.
# C'est ce qui la rend transposable en `count/scan/fill`.
add_executable(mhgp3v_centre_cell prototype/centre_cell_source.cpp)
find_package(Threads REQUIRED)
target_include_directories(mhgp3v_centre_cell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_centre_cell PRIVATE mhgp Threads::Threads)
target_compile_options(mhgp3v_centre_cell PRIVATE -Wall -Wextra -Werror)

# LES CINQ NUAGES, JUGES SUR LES IDENTITES DE SUPPORT, avec `I_B` et `U_B`.
# Le juge exhaustif enumere tous les sous-ensembles et fait son census sur TOUT
# le nuage : une omission compensee par un doublon ne peut pas rester verte.
add_test(NAME mhgp3v_centre_cell_uniform COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=uniform --seed=3 --judge --min-supports=1)
add_test(NAME mhgp3v_centre_cell_terrain COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=terrain --seed=3 --judge --min-supports=1)
add_test(NAME mhgp3v_centre_cell_scanline COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=scanline_single_pass --seed=3 --judge
         --min-supports=1)
add_test(NAME mhgp3v_centre_cell_multiecho COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=scanline_overlap_multiecho --seed=3 --judge
         --min-supports=1)
# LA GRILLE GRAVEE porte les egalites entieres `l_C(x) == R_p(C)` que AUCUN nuage
# aleatoire ne produit. C'est le seul nuage qui exerce la decision « conserver
# l'egalite », et le seul qui tue le mutant `drop-ties`.
add_test(NAME mhgp3v_centre_cell_grille_gravee COMMAND mhgp3v_centre_cell
         --points=125 --smax=11 --family=grave_grid --judge --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_uniform mhgp3v_centre_cell_terrain
                     mhgp3v_centre_cell_scanline mhgp3v_centre_cell_multiecho
                     mhgp3v_centre_cell_grille_gravee
                     PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD juge")

# LA VARIANTE A/B DU LIFT DIFFERE. Le RLE `SupportKey` avant lift divise les
# lifts par cinq, mais coute treize pour cent de plus sur ce CPU : le lift n'est
# pas le goulot, l'enumeration l'est. La variante reste donc desactivee par
# defaut et JUGEE, pour qu'un futur A/B device compare des identites, pas des
# impressions.
add_test(NAME mhgp3v_centre_cell_differe_uniform COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=uniform --seed=3 --judge --deferred-lift
         --min-supports=1)
add_test(NAME mhgp3v_centre_cell_differe_terrain COMMAND mhgp3v_centre_cell
         --points=60 --smax=6 --family=terrain --seed=3 --judge --deferred-lift
         --min-supports=1)
add_test(NAME mhgp3v_centre_cell_differe_grille COMMAND mhgp3v_centre_cell
         --points=125 --smax=11 --family=grave_grid --judge --deferred-lift
         --min-supports=1)
# UN LOT MINUSCULE force de nombreuses vidanges : la sortie doit etre identique.
add_test(NAME mhgp3v_centre_cell_differe_lot_minuscule COMMAND mhgp3v_centre_cell
         --points=125 --smax=11 --family=grave_grid --judge --deferred-lift
         --batch-records=1024 --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_differe_uniform
                     mhgp3v_centre_cell_differe_terrain
                     mhgp3v_centre_cell_differe_grille
                     mhgp3v_centre_cell_differe_lot_minuscule
                     PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD juge")

# LES QUATRE FIXTURES GRAVEES.
#
# `egalite` : un nuage aleatoire ne trouve PAS l'egalite `beta == R_p(C)`; sans
# elle le mutant `drop-ties` survit partout.
# `proprietaire` : la face partagee de deux cellules soeurs appartient a UNE
# seule d'entre elles; le pave ferme casse l'exact-once.
# `arite3` et `arite4` : les deux contre-fixtures de l'auditeur, section 4 de
# `audits/AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`. Elles REFUTENT la
# cascade d'arites : un support q3 pertinent peut n'avoir aucune arete q2
# pertinente, et un q4 pertinent aucune facette q3 pertinente. Les lanes doivent
# donc etre generativement independantes.
add_test(NAME mhgp3v_centre_cell_fixtures COMMAND mhgp3v_centre_cell --fixtures=toutes)
set_tests_properties(mhgp3v_centre_cell_fixtures
                     PROPERTIES PASS_REGULAR_EXPRESSION "FIXTURE arite4")
add_test(NAME mhgp3v_centre_cell_fixture_equality COMMAND mhgp3v_centre_cell
         --fixtures=egalite)
set_tests_properties(mhgp3v_centre_cell_fixture_equality
                     PROPERTIES PASS_REGULAR_EXPRESSION "FIXTURE egalite")
add_test(NAME mhgp3v_centre_cell_fixture_arite3 COMMAND mhgp3v_centre_cell
         --fixtures=arite3)
add_test(NAME mhgp3v_centre_cell_fixture_arite4 COMMAND mhgp3v_centre_cell
         --fixtures=arite4)
# LA COQUILLE NON BORNEE. Trente points de rayon cinq autour de (10,10,10) :
# six permutations signees de (5,0,0) et vingt-quatre de (4,3,0). La paire
# antipodale a `p=0, q=2`, donc elle est PERTINENTE, et `|U_B|=30`. Tout cap de
# coquille la tronque. La stratification par profondeur ne sauve rien : `A_0`
# contient deja les trente egalites.
add_test(NAME mhgp3v_centre_cell_fixture_coquille COMMAND mhgp3v_centre_cell
         --fixtures=coquille)
set_tests_properties(mhgp3v_centre_cell_fixture_coquille
                     PROPERTIES PASS_REGULAR_EXPRESSION "shell_high_water=30")
set_tests_properties(mhgp3v_centre_cell_fixture_arite3
                     PROPERTIES PASS_REGULAR_EXPRESSION "ABC=1 AB=0 AC=0 BC=0")
set_tests_properties(mhgp3v_centre_cell_fixture_arite4
                     PROPERTIES PASS_REGULAR_EXPRESSION "TET=1 faces=0000")

# LA SEPARATION PAR LA NORMALE LOCALE.
#
# Le k-DOP a directions fixes ne voit pas le cas dominant des nuages
# surfaciques : une cellule hors du plan local, dont les dix plus proches sites
# sont presque cospheriques vus de loin. Le critere de travail la decoupait
# indefiniment alors qu'aucun support POSITIF ne peut y avoir son centre. La
# direction est choisie dans les donnees — normale du plan des moindres carres,
# via l'adjugee de la covariance — mais le test reste entier et exact, donc le
# prune l'est aussi. Le plancher interdit le vert par vacuite; l'accord du juge
# sur les quatre nuages interdit qu'il retire un support.
add_test(NAME mhgp3v_centre_cell_normale_plancher COMMAND mhgp3v_centre_cell
         --points=400 --smax=11 --family=terrain --seed=11 --min-normal-pruned=1
         --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_normale_plancher
                     PROPERTIES PASS_REGULAR_EXPRESSION "normal_pruned=[1-9]")
add_test(NAME mhgp3v_centre_cell_normale_juge COMMAND mhgp3v_centre_cell
         --points=125 --smax=11 --family=terrain --seed=11 --judge --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_normale_juge
                     PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD juge")
# LA VARIANTE SANS NORMALE doit rendre EXACTEMENT la meme sortie : le prune est
# une decision de cout, jamais de semantique.
add_test(NAME mhgp3v_centre_cell_sans_normale COMMAND mhgp3v_centre_cell
         --points=125 --smax=11 --family=terrain --seed=11 --judge
         --no-normal-separation --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_sans_normale
                     PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD juge")

# LE SONDAGE DU VRAI GRAPHE ET SA BORNE D'INCIDENCE.
#
# Le critere de split employait le potentiel d'intervalles, majorant grossier
# d'un SURGRAPHE. Les compteurs sont maintenant exacts, chacun sur sa coupe de
# lane, et la borne d'incidence `4 Q4 <= (m4-3) T4` de l'auditeur les controle :
# chaque K4 fournit quatre triangles, un triangle a au plus `m4-3` apex. Le
# plancher `--min-probes` interdit le vert par vacuite; le mutant emule la faute
# de comptage realiste — un K4 compte une fois par face — et la borne le voit.
add_test(NAME mhgp3v_centre_cell_sonde_exacte COMMAND mhgp3v_centre_cell
         --points=200 --smax=11 --family=terrain --seed=11 --probe-factor=64
         --min-probes=1 --min-supports=1)
set_tests_properties(mhgp3v_centre_cell_sonde_exacte
                     PROPERTIES PASS_REGULAR_EXPRESSION "incidence_checks=[1-9]")
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_incidence "INVARIANT viole" 3
  --points=200 --smax=11 --family=terrain --seed=11 --probe-factor=64
  --min-probes=1 --inject=incidence-off-by-one)

# LES MUTANTS. Code 4 = mutant tue. Chacun casse UNE decision.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_shrink_list "MUTANT TUE" 4
  --points=60 --smax=6 --family=terrain --seed=3 --judge --inject=shrink-list)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_drop_ties "MUTANT TUE" 4
  --points=125 --smax=11 --family=grave_grid --judge --inject=drop-ties)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_owner_closed "MUTANT TUE" 4
  --points=125 --smax=11 --family=grave_grid --judge --inject=owner-closed)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_rank_closed "MUTANT TUE" 4
  --points=125 --smax=11 --family=grave_grid --judge --inject=rank-closed)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_tight_threshold "MUTANT TUE" 4
  --points=60 --smax=6 --family=terrain --seed=3 --judge --inject=tight-threshold)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_bisector_strict "MUTANT TUE" 4
  --points=125 --smax=11 --family=grave_grid --judge --inject=bisector-strict)
# LE MUTANT DE STRATE : il arrete le census au premier bucket, sans promotion.
# Le census restreint est alors incomplet et les identites `I_B`/`U_B` faussent.
# La grille gravee ne le tue PAS — ses budgets d'entree y sont deja corrects —
# et c'est pourquoi la porte porte sur deux familles.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_strata_stop "MUTANT TUE" 4
  --points=125 --smax=11 --family=terrain --seed=11 --judge --inject=strata-stop)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_strata_stop_uniform "MUTANT TUE" 4
  --points=125 --smax=11 --family=uniform --seed=11 --judge --inject=strata-stop)

# LE MUTANT DE CASCADE D'ARITES : il conditionne la lane q3 au succes de la lane
# q2 et la lane q4 au succes de q3. Seule la contre-fixture d'arite trois le tue,
# et c'est exactement la faute que l'auditeur a exigee de graver.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_kill_arity_cascade "MUTANT TUE" 4
  --fixtures=arite3 --inject=arity-cascade)

# REFUS AVANT CALCUL : code 2.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_unknown_argument "argument inconnu" 2 --sujet-inexistant)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_suffix "valeur invalide" 2 --points=60junk)
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_judge_too_large "juge exhaustif" 2
  --points=300 --smax=6 --judge)
# LES GENERATEURS DU DEPOT DEDUPLIQUENT : aucune famille n'exerce ce refus. Ce
# nuage grave le fait, avec un unique site repete.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_doublons "unsupported_degeneracy" 2
  --points=64 --smax=11 --family=grave_doublon)
# PLANCHER DE COUVERTURE : contre le vert-par-vacuite.
mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell
  mhgp3v_centre_cell_reject_floor "PLANCHER viole" 3
  --points=60 --smax=6 --family=terrain --seed=3 --min-supports=100000000)

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

# ---------------------------------------------------------------------------
# LE JUGE ARITHMETIQUEMENT INDEPENDANT DE LA SOURCE PAR CELLULES.
#
# L'audit refusait le juge interne du probe : il partage `ball_front.hpp`, ses
# lifts et sa forme de puissance avec le sujet, si bien que deux fautes communes
# s'y compenseraient. Celui-ci ne partage que le generateur de nuages et une
# ligne de texte.
#
# Le sujet decide par determinants entiers `i128`. Le juge n'emploie AUCUN
# determinant : il resout le centre par elimination de Gauss en rationnels
# multiprecision, teste la positivite par les coordonnees barycentriques du meme
# systeme, et fait son census par comparaisons de rationnels. Son cout est
# `Theta(n^5)` en multiprecision, donc il est borne a quarante-cinq points et ne
# peut jamais devenir un backend.
add_executable(mhgp3v_centre_cell_judge oracle/centre_cell_judge.cpp)
target_include_directories(mhgp3v_centre_cell_judge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mhgp3v_centre_cell_judge PRIVATE mhgp)
target_compile_options(mhgp3v_centre_cell_judge PRIVATE -Wall -Wextra -Werror)

mhgp3v_add_expected_code_test_for(mhgp3v_centre_cell_judge
  mhgp3v_centre_cell_judge_borne "borne a 45 points" 2 --points=200)

if(Python3_Interpreter_FOUND OR Python3_EXECUTABLE)
  set(MHGP3V_CC_JUDGE_DRIVER
      ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_centre_cell_independent_judge.py)
  foreach(fam uniform terrain scanline_single_pass scanline_overlap_multiecho)
    add_test(NAME mhgp3v_centre_cell_independant_${fam}
             COMMAND ${Python3_EXECUTABLE} ${MHGP3V_CC_JUDGE_DRIVER}
                     --subject-binary $<TARGET_FILE:mhgp3v_centre_cell>
                     --judge-binary $<TARGET_FILE:mhgp3v_centre_cell_judge>
                     --points=32 --smax=7 --family=${fam} --seed=5 --min-supports=1)
    set_tests_properties(mhgp3v_centre_cell_independant_${fam}
                         PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD JUGE INDEPENDANT")
  endforeach()
  # LE JUGE DOIT VOIR UN SUJET FAUX. Sans cette porte, l'accord ne prouverait que
  # la capacite du juge a etre d'accord.
  add_test(NAME mhgp3v_centre_cell_independant_voit_le_mutant
           COMMAND ${Python3_EXECUTABLE} ${MHGP3V_CC_JUDGE_DRIVER}
                   --subject-binary $<TARGET_FILE:mhgp3v_centre_cell>
                   --judge-binary $<TARGET_FILE:mhgp3v_centre_cell_judge>
                   --points=32 --smax=7 --family=terrain --seed=5
                   --inject=rank-closed)
  set_tests_properties(mhgp3v_centre_cell_independant_voit_le_mutant
                       PROPERTIES WILL_FAIL TRUE)
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
if(Python3_Interpreter_FOUND OR Python3_EXECUTABLE)
  foreach(fam uniform terrain scanline_single_pass scanline_overlap_multiecho)
    add_test(NAME mhgp3v_centre_cell_k1_${fam}
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_centre_cell_independent_judge.py
                     --subject-binary $<TARGET_FILE:mhgp3v_centre_cell>
                     --judge-binary $<TARGET_FILE:mhgp3v_centre_cell_judge>
                     --points=400 --smax=11 --family=${fam} --seed=11 --k1)
    set_tests_properties(mhgp3v_centre_cell_k1_${fam}
                         PROPERTIES PASS_REGULAR_EXPRESSION "ACCORD JUGE INDEPENDANT k1")
  endforeach()
endif()
# LE GRAPHE DE GABRIEL DOIT ETRE CONNEXE sur des sites distincts. Une composante
# de plus signalerait une arete manquante, donc une source incomplete : c'est un
# invariant de completude, verifie a chaque execution de la lane.
add_test(NAME mhgp3v_centre_cell_k1_connexe COMMAND mhgp3v_centre_cell
         --points=600 --smax=11 --family=terrain --seed=3 --k1)
set_tests_properties(mhgp3v_centre_cell_k1_connexe
                     PROPERTIES PASS_REGULAR_EXPRESSION "composantes=1")

# L'INVARIANCE DU RECU AU NOMBRE DE WORKERS.
#
# Le parallelisme decoupe l'arbre en sous-arbres independants a une profondeur
# de recolte. La propriete half-open garantit qu'un support n'est possede que
# par une seule cellule, donc par une seule tache; les compteurs cumulatifs
# s'additionnent, les compteurs de pic prennent le maximum. Le recu doit donc
# etre IDENTIQUE ligne a ligne, la seule ligne exclue etant celle qui annonce le
# nombre de workers. C'est la propriete qu'un portage device devra conserver, et
# c'est ce que cette porte exige litteralement.
if(Python3_Interpreter_FOUND OR Python3_EXECUTABLE)
  foreach(fam uniform terrain scanline_overlap_multiecho)
    add_test(NAME mhgp3v_centre_cell_workers_${fam}
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_centre_cell_thread_invariance.py
                     --binary $<TARGET_FILE:mhgp3v_centre_cell>
                     --points=600 --smax=11 --family=${fam} --seed=11
                     --threads 1 2 5)
    set_tests_properties(mhgp3v_centre_cell_workers_${fam}
                         PROPERTIES PASS_REGULAR_EXPRESSION "RECU IDENTIQUE")
  endforeach()
  # LE LIFT DIFFERE change le calendrier des lots : son invariance doit etre
  # verifiee separement, car ses vidanges dependent de l'ordre d'arrivee.
  add_test(NAME mhgp3v_centre_cell_workers_differe
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/audits/check_centre_cell_thread_invariance.py
                   --binary $<TARGET_FILE:mhgp3v_centre_cell>
                   --points=600 --smax=11 --family=uniform --seed=11
                   --threads 1 3 --extra=--deferred-lift --semantic-only)
  set_tests_properties(mhgp3v_centre_cell_workers_differe
                       PROPERTIES PASS_REGULAR_EXPRESSION "SEMANTIQUE IDENTIQUE")
endif()

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
add_test(NAME mhgp3v_anchor_parite_ledger_garde COMMAND mhgp3v_anchor_source
         --points=100 --family=terrain --seed=3 --compare-engines --density-guard
         --min-supports=4000)
add_test(NAME mhgp3v_anchor_parite_ledger_theta COMMAND mhgp3v_anchor_source
         --points=100 --family=uniform --seed=3 --compare-engines --theta-audit
         --min-theta-active=2000 --min-supports=15000)
set_tests_properties(mhgp3v_anchor_parite_ledger_uniform mhgp3v_anchor_parite_ledger_terrain
                     mhgp3v_anchor_parite_ledger_clusters mhgp3v_anchor_parite_ledger_multiecho
                     mhgp3v_anchor_parite_ledger_garde mhgp3v_anchor_parite_ledger_theta
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
add_test(NAME mhgp3v_anchor_garde_densite_exacte COMMAND mhgp3v_anchor_source
         --points=140 --family=terrain --seed=5 --verify --density-guard
         --min-supports=5000)
set_tests_properties(mhgp3v_anchor_garde_densite_exacte PROPERTIES
  PASS_REGULAR_EXPRESSION "accord=OUI")

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
add_test(NAME mhgp3v_dominance_uniform COMMAND mhgp3v_directional_dominance_probe
         --points=2000 --family=uniform --seed=3 --judge-echantillon=800
         --min-closed-q4=6000 --min-residuel=3000000 --min-cellules=300000)
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
# La cible comptee comme temoin. Le differentiel `mutant ferme, reference non`
# est le seul ensemble ou une injection peut etre fausse ; un tirage uniforme
# sur des millions de fermetures ne le rencontre jamais.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_mutant_cible_temoin "MUTANT TUE" 4
  --points=3000 --family=terrain --seed=3 --judge-echantillon=1500
  --inject=dom-cible-temoin)

# Refus contractuels.
mhgp3v_add_expected_code_test_for(mhgp3v_directional_dominance_probe
  mhgp3v_dominance_reject_mutant_sans_juge "un mutant sans juge ne prouve rien" 2
  --points=200 --inject=dom-cible-temoin)
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
