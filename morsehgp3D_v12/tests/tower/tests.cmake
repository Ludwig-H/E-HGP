# Portes de l'etage G de la tour (tranche T2, docs/CONTRAT_TOUR.md, paragraphe 9). Raisons emises par le module et
# leurs portes : tower_capacity (unit.cpp, targets_capacity : bornes 2^31 - 1 et 2^32 - 1, sans allocation),
# cell_capacity (unit.cpp, capacity_refusals : 30 sites cospheriques, C(30, 4) au-dela du plafond), tower_invariant
# (unit.cpp, witness_memo : date initiale controlee ; resolve_part sous une jonction trop basse). catalogue_missing_ball
# et census_mismatch (statut invariant_violated) ne sont rendus par aucune entree valide ((H2) tenue par le catalogue de
# T1, census et catalogue coherents) : les mutants table_s_etoile_muette et controle_croise_decale de
# tests/mutants/tower.json les provoquent sur le cercle du carre (witness_t1_square, F = BD a l'ordre 2).
add_executable(mhgp12_tower_probe ${PROJECT_SOURCE_DIR}/bench/tower_probe.cpp)
target_include_directories(mhgp12_tower_probe PRIVATE ${PROJECT_SOURCE_DIR}/bench)
target_link_libraries(mhgp12_tower_probe PRIVATE mhgp12)
set(tower_probe $<TARGET_FILE:mhgp12_tower_probe>)
mhgp12_expect_code(mhgp12_tower_probe_usage 2 mhgp12_tower_probe --k=5 LABELS fast)

# Temoins graves (WIT-D2, WIT-MEMO, WIT-T1-CARRE cote tour, faits du census complet et du census sature, pas inerte
# sous la fenetre), cibles et capacite, plafonds declares, budget, determinisme a 1 et 8 fils.
mhgp12_add_unit(mhgp12_tower_unit SOURCES unit.cpp
                GROUPS targets_capacity witness_d2 witness_memo witness_t1_square fact_saturated inert_below_window
                       capacity_refusals budget determinism
                LABELS fast TIMEOUT 300)

# Oracle borne : chaque cible egale a celle de resolve_v12 (politique v12_indices) et dans la composante attendue a la
# coupe ouverte de sa jonction (foret de l'etage B), sur la suite rapide (n <= 14) ; doublons refuses (D8).
mhgp12_python_gate(mhgp12_tower_oracle 0 g_oracle.py ${tower_probe}
                   LINE "g_oracle_ok nuages=342 doublons_refuses=34 cibles=21225 cibles_cellule=2359 cellules=7821 inertes=876"
                   LABELS oracle fast TIMEOUT 300)

# Determinisme et invariants globaux a l'echelle (jamais un juge exhaustif) : la sonde a 1 et 8 fils, empreinte de la
# resolution (naissances, cellules, traces, cibles, compteurs de l'objet et du travail) et lignes par ordre identiques ;
# un controle de decroissance par plus petite boule et par succes de sonde ; une chaine par representant. Nuages
# synthetiques de la porte d'echelle du catalogue (--uniform=N,20261007,18), puis la trame ng00 (MHGP12_DATA_DIR).
set(tower_scale_8000 "empreinte=5304d1c8fe7c25ff naissances=376649 cellules=600630 representants=1780799 cibles_cellule=370896")
set(tower_scale_16000 "empreinte=fdd42b4e0fba5d08 naissances=772572 cellules=1238402 representants=3674998 cibles_cellule=774576")
set(tower_scale_32000 "empreinte=9fd0fd9e98fa5305 naissances=1580630 cellules=2540941 representants=7550507 cibles_cellule=1594813")
foreach(n 8000 16000 32000)
  mhgp12_python_gate(mhgp12_tower_scale${n} 0 g_determinism.py ${tower_probe} synth_u${n}_k5
                     --uniform=${n},20261007,18 --k=5 --threads=1,8
                     LINE "g_determinism_ok cas=synth_u${n}_k5 fils=1,8 ${tower_scale_${n}}" LABELS scale${n} TIMEOUT 900)
endforeach()
mhgp12_python_gate(mhgp12_tower_determinism_lidar_ng00_k5 0 g_determinism.py ${tower_probe} lidar_ng00_k5
                   --data=lidar_ng00 --k=5 --threads=1,8
                   LINE "g_determinism_ok cas=lidar_ng00_k5 fils=1,8 empreinte=231d826bb0d4fe57 naissances=897776 cellules=1306872 representants=3622258 cibles_cellule=650932"
                   LABELS lidar long TIMEOUT 1800)

# Juge du determinisme contre ses faux succes (contre-lecture de l'auditeur Codex du 7 octobre, CST-0018) : la fausse
# sonde g_fausse_sonde.py (hors produit) rejoue une sortie tronquee apres l'ordre 1 (ligne CTest imitee), un ordre
# double, une empreinte courte et une sortie sans ligne exit ; le juge les refuse (code 2), le temoin conforme passe.
# Le juge d'avant cette contre-lecture les admettait toutes (code 0).
set(tower_fausse_sonde ${CMAKE_CURRENT_LIST_DIR}/g_fausse_sonde.py)
mhgp12_python_gate(mhgp12_tower_juge_temoin 0 g_determinism.py ${tower_fausse_sonde} fausse --uniform=10,1,18 --k=5
                   --threads=1,8 LABELS fast
                   LINE "g_determinism_ok cas=fausse fils=1,8 empreinte=abababababababab naissances=22 cellules=5 representants=10 cibles_cellule=0"
                   ENV MHGP12_FAUSSE_SONDE=ok)
foreach(mode k1_seul ordre_double empreinte_courte sans_sortie sortie_ordre_booleen sortie_ordre_flottant)
  mhgp12_python_gate(mhgp12_tower_juge_refus_${mode} 2 g_determinism.py ${tower_fausse_sonde} fausse
                     --uniform=10,1,18 --k=5 --threads=1,8 LABELS fast ENV MHGP12_FAUSSE_SONDE=${mode})
endforeach()

# Differentiel contre la v11 gelee (MHGP12_V11_TOWER_DIR : <cas>_k5/ avec cat.bin, ordre_<k>.bin et foret_<k>.bin de
# mhgp12_vidage, feuille 16, microbancs/mes_m3_m4_tour ; donnees MHGP12_DATA_DIR) : la sonde exporte l'etage G, le juge
# g_diff_v11.py exige pour chaque representant la composante de la graine v11 a la coupe ouverte de sa jonction
# (memes naissances, cellules et traces). Sans ce dossier, les portes ne sont pas enregistrees.
set(MHGP12_V11_TOWER_DIR "$ENV{MHGP12_V11_TOWER_DIR}" CACHE PATH
    "Vidages de la tour v11 gelee par cas (ng00_k5, ng01_k5, ng02_k5) ; vide : portes diff_v11 de la tour absentes")
set(tower_diff_ng00 "representants=3622258 composantes_egales=3622258 graines_identiques=3622258 cibles_cellule=650932")
set(tower_diff_ng01 "representants=3012810 composantes_egales=3012810 graines_identiques=3012810 cibles_cellule=521513")
set(tower_diff_ng02 "representants=3850037 composantes_egales=3850037 graines_identiques=3850037 cibles_cellule=650599")
if(MHGP12_V11_TOWER_DIR AND EXISTS "${MHGP12_V11_TOWER_DIR}/ng00_k5/foret_5.bin")
  foreach(case ng00 ng01 ng02)
    mhgp12_python_gate(mhgp12_tower_diff_v11_${case}_k5 0 g_diff_v11.py ${MHGP12_V11_TOWER_DIR}/${case}_k5
                       --probe=${tower_probe} --data=lidar_${case} --k=5 --threads=3 --case=${case}_k5
                       LINE "g_diff_v11_ok cas=${case}_k5 ordres=5 ${tower_diff_${case}}" LABELS lidar long TIMEOUT 1800)
  endforeach()
else()
  message(STATUS "mhgp12 : portes diff_v11 de la tour absentes (MHGP12_V11_TOWER_DIR='${MHGP12_V11_TOWER_DIR}')")
endif()
