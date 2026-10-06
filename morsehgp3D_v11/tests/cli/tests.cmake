# Portes de l'executable mhgp11 (tranche S5 de la sortie parametree : --sortie=full).
# Sonde de reference : mhgp11_full_bench (bench/full_probe.cpp) si les portes de la tour l'ont deja declaree ; sinon
# (construction -DMHGP11_MODULES=cli des mutants) la meme source sous un nom propre, memes octets.
if(TARGET mhgp11_full_bench)
  set(mhgp11_cli_reference $<TARGET_FILE:mhgp11_full_bench>)
else()
  add_executable(mhgp11_cli_full_reference ${PROJECT_SOURCE_DIR}/bench/full_probe.cpp)
  target_link_libraries(mhgp11_cli_full_reference PRIVATE mhgp11)
  set(mhgp11_cli_reference $<TARGET_FILE:mhgp11_cli_full_reference>)
endif()
set(mhgp11_cli $<TARGET_FILE:mhgp11_cli>)

# Contrat : au moins 20 refus dans l'ordre du paragraphe 3 de docs/SORTIES.md, codes exacts (2 et 3), ligne de refus
# avec l'etat du dossier (publication, manifest_sha256), ni dossier ni D.pending hors des doubles echecs, entrees
# intactes, retrait du dossier publie quand la sortie standard echoue apres la publication, published_complete quand
# ce retrait echoue aussi (tube plein, puis D.pending cree des que D apparait : sans crochet). Deux bibliotheques
# prechargees, hors produit : exception flottante demasquee avant main (environment_selftest, code 3), et
# synchronisation du parent de D puis renommages de D refuses (commit en double echec, retrait reussi ou refuse). Sous
# ASan ou TSan, un prechargement precederait le runtime du sanitizer : ces trois cas n'y sont pas joues et la ligne
# attendue le dit.
if(MHGP11_SANITIZE OR MHGP11_TSAN)
  set(mhgp11_cli_preload)
  set(mhgp11_cli_contract_line "cli_contract_verdict conforme refus65 temoins3")
else()
  add_library(mhgp11_cli_fenv_preload SHARED ${CMAKE_CURRENT_LIST_DIR}/fenv_preload.cpp)
  add_library(mhgp11_cli_io_fault_preload SHARED ${CMAKE_CURRENT_LIST_DIR}/io_fault_preload.cpp)
  target_link_libraries(mhgp11_cli_io_fault_preload PRIVATE ${CMAKE_DL_LIBS})
  set(mhgp11_cli_preload --preload $<TARGET_FILE:mhgp11_cli_fenv_preload>
      --fault-preload $<TARGET_FILE:mhgp11_cli_io_fault_preload>)
  set(mhgp11_cli_contract_line "cli_contract_verdict conforme refus68 temoins3")
endif()
mhgp11_python_gate(mhgp11_cli_contract 0 cli_contract.py --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS}
                   ${mhgp11_cli_preload} LINE "${mhgp11_cli_contract_line}" LABELS fast TIMEOUT 300)
# Champ tree_k_sha256 publie par l'executable contre une serialisation independante de la signature version 2, en
# bibliotheque standard (structures ecrites a la main, valeurs de l'auditeur), a K = 1 et K = 2, entree permutee et
# reetiquetee (point 3 des auditeurs, 9cbf805c6).
mhgp11_python_gate(mhgp11_cli_tree_signature 0 cli_tree_signature.py --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS}
                   LINE "cli_tree_signature_verdict conforme cas3 appels6" LABELS fast TIMEOUT 300)
# Identite : sha256 brut de full.mhgp11ful1 egal au dump de la sonde, petits nuages, K = 1..5, deux ordres, W1/W4.
mhgp11_python_gate(mhgp11_cli_full_identity 0 cli_full_identity.py --cli ${mhgp11_cli} --bench ${mhgp11_cli_reference}
                   --bits ${MHGP11_COORD_BITS} LINE "cli_full_identity_verdict conforme attempts503 refusals7"
                   LABELS fast TIMEOUT 600)
# Determinisme (W1, W2, W4 et W48 de docs/SORTIES.md, paragraphe 10, repetition, permutation) et reetiquetage (0 et
# 0xFFFFFFFF compris).
mhgp11_python_gate(mhgp11_cli_full_determinism 0 cli_full_invariance.py --mode=determinism --cli ${mhgp11_cli}
                   --bits ${MHGP11_COORD_BITS} --fils=1,2,4,48 LINE "cli_full_determinism_verdict conforme runs36"
                   LABELS fast TIMEOUT 600)
mhgp11_python_gate(mhgp11_cli_full_relabel 0 cli_full_invariance.py --mode=relabel --cli ${mhgp11_cli}
                   --bits ${MHGP11_COORD_BITS} LINE "cli_full_relabel_verdict conforme runs12" LABELS fast TIMEOUT 600)

# Echelle et LiDAR (G4) : un nuage, deux ordres d'entree, meme dump que la sonde. Aucune mesure de temps ici.
foreach(case "8000;5;scale8000;" "16000;5;scale16000;" "32000;5;scale32000;" "32000;10;scale32000;long")
  list(GET case 0 n)
  list(GET case 1 k)
  list(GET case 2 label)
  list(GET case 3 extra)
  set(suffix "")
  if(NOT k EQUAL 5)
    set(suffix "_k${k}")
  endif()
  mhgp11_python_gate(mhgp11_cli_full_identity_${label}${suffix} 0 cli_full_identity.py --cli ${mhgp11_cli}
                     --bench ${mhgp11_cli_reference} --bits ${MHGP11_COORD_BITS} --uniform=${n} --k=${k} --fils=8
                     --min-attempts=2 LINE "cli_full_identity_verdict conforme attempts2 refusals0"
                     LABELS ${label} ${extra} TIMEOUT 3600)
endforeach()
mhgp11_python_gate(mhgp11_cli_full_determinism_scale8000 0 cli_full_invariance.py --mode=determinism
                   --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS} --uniform=8000 --k=5 --fils=1,8,48
                   LINE "cli_full_determinism_verdict conforme runs10" LABELS scale8000 TIMEOUT 3600)
foreach(case "ng00;5;" "ng01;5;" "ng02;5;" "ng00;10;long")
  list(GET case 0 frame)
  list(GET case 1 k)
  list(GET case 2 extra)
  mhgp11_python_gate(mhgp11_cli_full_identity_lidar_${frame}_k${k} 0 cli_full_identity.py --cli ${mhgp11_cli}
                     --bench ${mhgp11_cli_reference} --bits ${MHGP11_COORD_BITS} --data=lidar_${frame} --k=${k}
                     --fils=8 --min-attempts=2 LINE "cli_full_identity_verdict conforme attempts2 refusals0"
                     LABELS lidar ${extra} TIMEOUT 3600)
endforeach()

# ---- Sortie supports (tranche S7) : supports.mhgp11sp (MHGP11SP version 1) relu par le lecteur officiel
# (bench/mhgp11_formats.py, read_supports et check_directory : decodage strict, controles du paragraphe 6 de
# docs/SORTIES.md, agregats du manifeste recomptes, tree_k_sha256 recalcule depuis le fichier).
# Oracle : le fichier lu egale le vidage canonique de l'oracle borne S1 (canonical(k, ids)), sur les nuages et ordres
# de mhgp11_supports_hierarchy_fraction (W1 dans l'ordre de l'oracle, W4 dans l'ordre inverse, en alternance) ;
# tree_k_sha256 egal entre --sortie=full et --sortie=supports a K1..4 ; refus de l'appel entier a 25 sites (code 2,
# support_shell_capacity, ni D ni D.pending ; FULL conforme), sphere5 a 24 sites admise (828 supports).
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_cli_supports_oracle_line "cli_supports_oracle_couverture bits=18 nuages=209 exclus=2 ordres=957 boules=15246 etendues=2639 multiples=570 tetraedres=1145 ordres_6plus=7 signatures=804 larges=4")
  set(mhgp11_cli_supports_oracle_floors --min-clouds=209 --min-orders=957 --min-balls=15246 --min-extended=2639
      --min-multiple=570 --min-signatures=804)
else()
  set(mhgp11_cli_supports_oracle_line "cli_supports_oracle_couverture bits=${MHGP11_COORD_BITS} nuages=211 exclus=0 ordres=963 boules=15270 etendues=2645 multiples=576 tetraedres=1145 ordres_6plus=7 signatures=810 larges=4")
  set(mhgp11_cli_supports_oracle_floors --min-clouds=211 --min-orders=963 --min-balls=15270 --min-extended=2645
      --min-multiple=576 --min-signatures=810)
endif()
mhgp11_python_gate(mhgp11_cli_supports_oracle 0 cli_supports_oracle.py --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS}
                   ${mhgp11_cli_supports_oracle_floors} LINE "${mhgp11_cli_supports_oracle_line}"
                   LABELS oracle fast TIMEOUT 900)
# Echelle et LiDAR (K5) : lecteur et invariants sur tout le fichier, fichiers et manifeste identiques a W1 et W4 (et
# a la repetition), permutation (fichier identique), reetiquetage non dense avec 0xFFFFFFFF (seule la colonne
# SITES.point_id change), tree_k_sha256 egal a celui de --sortie=full ; plus un petit nuage de boite riche en
# cospheriques. Comptes graves : MHGP11SP 2, arbre couvrant d'ordre K (6 octobre 2026). W48 : sur G4 (--fils=1,4,48).
set(mhgp11_cli_supports_8000 "sites=8000 k=5 noeuds=273655 boules=273655 supports=273655 etendues=0 fusions=108813 branches=273654 appels=12")
set(mhgp11_cli_supports_16000 "sites=16000 k=5 noeuds=565098 boules=565098 supports=565098 etendues=0 fusions=225041 branches=565097 appels=12")
set(mhgp11_cli_supports_32000 "sites=32000 k=5 noeuds=1163756 boules=1163761 supports=1163761 etendues=0 fusions=463577 branches=1163760 appels=12")
foreach(n 8000 16000 32000)
  mhgp11_python_gate(mhgp11_cli_supports_scale${n} 0 cli_supports_scale.py --cli ${mhgp11_cli}
                     --bits ${MHGP11_COORD_BITS} --uniform=${n} --k=5 --fils=1,4 --min-balls=100000
                     LINE "cli_supports_scale_verdict conforme ${mhgp11_cli_supports_${n}}" LABELS scale${n}
                     TIMEOUT 3600)
endforeach()
foreach(case "ng00;sites=39885 k=5 noeuds=576371 boules=576482 supports=576482 etendues=62 fusions=235401 branches=576483 appels=12"
             "ng01;sites=35551 k=5 noeuds=478265 boules=478380 supports=478380 etendues=38 fusions=195173 branches=478385 appels=12"
             "ng02;sites=45845 k=5 noeuds=609376 boules=610002 supports=610002 etendues=171 fusions=248676 branches=610022 appels=12")
  list(GET case 0 frame)
  list(GET case 1 counts)
  mhgp11_python_gate(mhgp11_cli_supports_lidar_${frame}_k5 0 cli_supports_scale.py --cli ${mhgp11_cli}
                     --bits ${MHGP11_COORD_BITS} --data=lidar_${frame} --k=5 --fils=1,4 --min-balls=400000
                     --min-extended=30 LINE "cli_supports_scale_verdict conforme ${counts}" LABELS lidar TIMEOUT 3600)
endforeach()

# ---- Sortie points (tranche S9) : points.mhgp11pt (MHGP11PT version 1) relu par le lecteur officiel en lecture exacte
# (read_points : plancher et drapeau strict certifies, plateaux strictement croissants, entree de chaque site a un
# plateau de sa date, comptes du manifeste recomptes) ; tree_k_sha256 egal a --sortie=supports (et full sur les
# temoins) a meme entree et meme K ; W1 et W4, permutation, reetiquetage ; refus K = n a K >= 2 (n = 2, 5, 8 :
# parameter_out_of_range a l'etape compute, ni D ni D.pending), K = 1 admis a n = 1 et 2.
mhgp11_python_gate(mhgp11_cli_points 0 cli_points.py --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS}
                   LINE "cli_points_verdict conforme cas=52 appels=266 refus=3 retardes=105 plateaux=242"
                   LABELS fast TIMEOUT 600)

# ---- Sortie plat (tranche S10) : etiquettes.mhgp11et (MHGP11ET version 1) relu par le lecteur officiel ; temoins de
# bench/points_flat_gate.py (F1, F4, F4b, F5, F6, F8 : groupes graves, egalites certifiees lues dans le manifeste) ;
# nuages en amas a PointId non denses, K = 1..4 : etiquette = plus petit PointId, W1 = W4, permutation, tree_k_sha256
# egal a --sortie=points ; refus K = n a K >= 2, K = 1 admis a n = 1.
mhgp11_python_gate(mhgp11_cli_plat 0 cli_plat.py --cli ${mhgp11_cli} --bits ${MHGP11_COORD_BITS}
                   LINE "cli_plat_verdict conforme cas=40 appels=112 refus=2 egalites=3 retenus=318"
                   LABELS fast TIMEOUT 600)
