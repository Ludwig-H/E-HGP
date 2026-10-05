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
  set(mhgp11_cli_contract_line "cli_contract_verdict conforme refus61 temoins3")
else()
  add_library(mhgp11_cli_fenv_preload SHARED ${CMAKE_CURRENT_LIST_DIR}/fenv_preload.cpp)
  add_library(mhgp11_cli_io_fault_preload SHARED ${CMAKE_CURRENT_LIST_DIR}/io_fault_preload.cpp)
  target_link_libraries(mhgp11_cli_io_fault_preload PRIVATE ${CMAKE_DL_LIBS})
  set(mhgp11_cli_preload --preload $<TARGET_FILE:mhgp11_cli_fenv_preload>
      --fault-preload $<TARGET_FILE:mhgp11_cli_io_fault_preload>)
  set(mhgp11_cli_contract_line "cli_contract_verdict conforme refus64 temoins3")
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
