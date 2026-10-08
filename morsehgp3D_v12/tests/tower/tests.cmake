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

# Index des naissances et premieres sondes (T2-c) : tri par base stable et deterministe (1 et 8 fils), index contre une
# reference independante avec l'empreinte du produit et des masques faibles (collisions : dichotomie dans le seau,
# jamais de produit de deux groupes egaux), candidats de la jointure triee (G-L5) egaux a ceux d'une sonde par
# representant, resolution rejouee avec un index a collisions (G-L5 et file G-L7) : memes cibles et memes compteurs ;
# table S* -> boule du catalogue contre une table ordonnee ; population repetee refusee.
mhgp12_add_unit(mhgp12_tower_index SOURCES index_unit.cpp
                GROUPS radix_stable index_reference weak_key_resolution support_table duplicate_population
                LABELS fast TIMEOUT 300)

# Oracle borne : chaque cible egale a celle de resolve_v12 (politique v12_indices) et dans la composante attendue a la
# coupe ouverte de sa jonction (foret de l'etage B), sur la suite rapide (n <= 14) ; doublons refuses (D8).
mhgp12_python_gate(mhgp12_tower_oracle 0 g_oracle.py ${tower_probe}
                   LINE "g_oracle_ok nuages=342 doublons_refuses=34 cibles=21225 cibles_cellule=2359 cellules=7821 inertes=876"
                   LABELS oracle fast TIMEOUT 300)

# Determinisme et invariants globaux a l'echelle (jamais un juge exhaustif) : la sonde a 1 et 8 fils, empreinte de
# l'OBJET de la resolution (naissances, cellules, traces, cibles, compteurs de l'objet : CONTRAT_TOUR.md, paragraphe 8)
# et compteurs de l'objet identiques ; compteurs du travail identiques ligne a ligne, hors empreinte ; un controle de
# decroissance par plus petite boule et par succes de sonde ; une chaine par representant. Nuages synthetiques de la
# porte d'echelle du catalogue (--uniform=N,20261007,18), puis la trame ng00 (MHGP12_DATA_DIR). Empreintes de l'objet
# du 7 octobre (T2-c) : exports res.bin et cat.bin identiques a l'octet a ceux d'avant ; seule la definition de
# l'empreinte a change (anciennes, travail compris : 5304d1c8, fdd42b4e, 9fd0fd9e, 231d826b).
set(tower_scale_8000 "empreinte=a40f1b2ef8547269 naissances=376649 cellules=600630 representants=1780799 cibles_cellule=370896")
set(tower_scale_16000 "empreinte=cf7c7745fcb4ae6e naissances=772572 cellules=1238402 representants=3674998 cibles_cellule=774576")
set(tower_scale_32000 "empreinte=d1f08fd0dbdf48eb naissances=1580630 cellules=2540941 representants=7550507 cibles_cellule=1594813")
foreach(n 8000 16000 32000)
  mhgp12_python_gate(mhgp12_tower_scale${n} 0 g_determinism.py ${tower_probe} synth_u${n}_k5
                     --uniform=${n},20261007,18 --k=5 --threads=1,8
                     LINE "g_determinism_ok cas=synth_u${n}_k5 fils=1,8 ${tower_scale_${n}}" LABELS scale${n} TIMEOUT 900)
endforeach()
mhgp12_python_gate(mhgp12_tower_determinism_lidar_ng00_k5 0 g_determinism.py ${tower_probe} lidar_ng00_k5
                   --data=lidar_ng00 --k=5 --threads=1,8
                   LINE "g_determinism_ok cas=lidar_ng00_k5 fils=1,8 empreinte=e5a81154fb1b15f1 naissances=897776 cellules=1306872 representants=3622258 cibles_cellule=650932"
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
# L'empreinte ne porte que l'objet (T2-c) : un ecart du travail ou de l'objet entre 1 et 8 fils, a empreinte egale,
# reste un ecart (code 1).
foreach(mode travail_8 objet_8)
  mhgp12_python_gate(mhgp12_tower_juge_ecart_${mode} 1 g_determinism.py ${tower_fausse_sonde} fausse
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

# ---- Etages T, M, V, R et export FUL1 (docs/CONTRAT_TOUR.md, paragraphe 9) ----
mhgp12_add_unit(mhgp12_tower_forest SOURCES forest_unit.cpp
                GROUPS admission attache branches catalogue determinisme domaine hypergraphes refus requetes temoins
                       verticales
                LABELS fast)
# Session recouverte (build_tower, decision D-F2 ; src/tower/pipeline.hpp) : graphe des etapes contre les lectures de
# chacune, identite avec la voie sequentielle (resolve_tower puis build_forests : empreinte FUL1, registres, compteurs
# de l'objet et du travail, cibles) a 1, 2, 3 et 8 fils sur nuages aleatoires, grilles et droites, determinisme sur
# 1 500 sites, admission unique de la region (pic au plus les octets admis ; limite exacte suffisante) et refus
# transactionnel un octet en dessous.
mhgp12_add_unit(mhgp12_tower_pipeline SOURCES pipeline_unit.cpp
                GROUPS graphe identite determinisme admission refus LABELS fast TIMEOUT 600)
# Session recouverte sous penurie injectee (operators new remplaces dans ce seul executable ; prelecture de l'auditeur
# Codex du 8 octobre) : allocation, les trois new qui levent de build_tower (SessionRun, puis BuildState et Pipeline dans
# open_session, qui n'est donc pas noexcept) echouent tour a tour : refus memory_budget, budget rendu, aucune
# terminaison, reprise a l'identique ; penurie, chaque new sans exception de build_tower echoue tour a tour (un fil :
# toutes ; trois fils : un echantillon) : refus memory_budget ou succes, budget rendu, ni terminaison ni attente.
mhgp12_add_unit(mhgp12_tower_pipeline_fault SOURCES pipeline_fault.cpp GROUPS allocation penurie LABELS fast)
# Adaptateur de test des vidages MHGP12DP de la v11 (MES-M0, determinisme, JUG-EMST) : outil joue par la porte
# MES-M0 ci-dessous et par le pilote du developpeur. Il lit les vidages par le lecteur strict du format, source unique
# des microbancs (microbancs/mes_m3_m4_tour/common/format.hpp) : construit seulement si ce dossier est present (les
# copies des campagnes de mutants ne le contiennent pas).
set(tower_dump_format ${PROJECT_SOURCE_DIR}/microbancs/mes_m3_m4_tour/common/format.hpp)
if(EXISTS ${tower_dump_format})
  add_executable(mhgp12_tower_dumps ${CMAKE_CURRENT_LIST_DIR}/tower_dumps.cpp)
  target_link_libraries(mhgp12_tower_dumps PRIVATE mhgp12)
endif()
# Oracle borne (contrat, paragraphe 9.2) : suite rapide de reference/ sans doublon et temoins (WIT-TRI-EQ, WIT-SIX,
# carre K1..4), trois politiques de saut ; registre, coupes et vidage FUL1 relu par le lecteur strict.
add_executable(mhgp12_tower_forest_oracle ${CMAKE_CURRENT_LIST_DIR}/tower_oracle.cpp)
target_link_libraries(mhgp12_tower_forest_oracle PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_tower_forest_oracle_gate 0 oracle_tour.py $<TARGET_FILE:mhgp12_tower_forest_oracle>
                   ${MHGP12_COORD_BITS}
                   LABELS oracle fast TIMEOUT 600)
# Temoin WIT-FORME-NIVEAU par l'export reel (contrat, paragraphe 1 ; fixture reference/test_witness_forme.py) : meme
# registre, table des niveaux dans l'ordre de Morton (v11) puis des positions (T1) : octets differents, empreinte
# semantique identique.
mhgp12_python_gate(mhgp12_tower_forme_niveau 0 forme_niveau.py $<TARGET_FILE:mhgp12_tower_forest_oracle>
                   ${MHGP12_COORD_BITS}
                   LABELS oracle fast TIMEOUT 120)
# MES-M0 sur les graines de la v11 (contrat, paragraphe 9.1 ; mes_m0.py) : octets egaux aux empreintes de MESURE.md
# au profil 21, empreinte semantique aux profils 24 et 32, 1 fil contre 8 fils, et JUG-EMST sur l'ordre un si son
# binaire est donne. Enregistree seulement si les vidages MHGP12DP de la v11 sont donnes a la configuration
# (-DMHGP12_TOWER_DUMPS=<dossier>, un sous-dossier par cas, outil mhgp12_vidage des microbancs) ; les trames viennent
# de MHGP12_DATA_DIR (porte lidar, sautee sans elles).
set(MHGP12_TOWER_DUMPS "" CACHE PATH "vidages MHGP12DP de la v11 pour MES-M0 (vide : porte non enregistree)")
set(MHGP12_JUG_EMST "" CACHE FILEPATH "binaire mhgp12_jug_emst (juges/emst), joue sur l'ordre un des vidages")
if(MHGP12_TOWER_DUMPS AND TARGET mhgp12_tower_dumps)
  set(tower_mes_m0_judge)
  if(MHGP12_JUG_EMST)
    set(tower_mes_m0_judge --juge-emst ${MHGP12_JUG_EMST})
  endif()
  mhgp12_python_gate(mhgp12_tower_mes_m0 0 mes_m0.py $<TARGET_FILE:mhgp12_tower_dumps> ${MHGP12_COORD_BITS} -
                     ${MHGP12_TOWER_DUMPS} ${tower_mes_m0_judge} LABELS lidar long TIMEOUT 7200)
endif()
# Chaine G -> T (mhgp12_tower_chain : index, catalogue de T1, resolve_tower, T, M, V, R, export) : porte MES-M0
# SEMANTIQUE avec le catalogue de T1 dans la chaine (contrat, paragraphe 9.1, sortie de T2) : ng00-02 a K5 et K10 et
# uniformes a K5, empreinte semantique egale a celle des vidages de la v11, 1 fil contre 8 fils ; trames dans
# MHGP12_DATA_DIR (porte lidar), sans vidage de la v11.
add_executable(mhgp12_tower_chain ${CMAKE_CURRENT_LIST_DIR}/tower_chain.cpp)
target_link_libraries(mhgp12_tower_chain PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_tower_chain_m0 0 mes_m0.py --chaine $<TARGET_FILE:mhgp12_tower_chain> ${MHGP12_COORD_BITS}
                   - LABELS lidar long TIMEOUT 7200)

# Sonde FULL residente (bench/full_probe.cpp ; frontiere du mur proposee par l'auditeur Codex le 8 octobre) : Session
# ouverte une fois, passes du nuage a la tour complete en memoire, trames successives, voie appareil avec --device.
# Porte de la voie CPU : empreinte FUL1 identique d'une passe a l'autre et egale a celle de mhgp12_tower_chain, trames
# alternees, etages du mur disjoints ; usage faux : code 2. La voie appareil se juge sur G4.
add_executable(mhgp12_full_probe ${PROJECT_SOURCE_DIR}/bench/full_probe.cpp)
target_link_libraries(mhgp12_full_probe PRIVATE mhgp12)
mhgp12_expect_code(mhgp12_full_probe_usage 2 mhgp12_full_probe --k=5 LABELS fast)
mhgp12_python_gate(mhgp12_full_probe_cpu 0 full_probe_check.py $<TARGET_FILE:mhgp12_full_probe>
                   $<TARGET_FILE:mhgp12_tower_chain> LINE "full_probe_ok passes=7 trames=2 identite_chaine=oui"
                   LABELS fast TIMEOUT 600)
# Meme porte sur la Session recouverte (--recouvert, build_tower, decision D-F2) : schema "recouvert" (partition murale,
# fenetres murales des taches, memoire P, C, tour, recouvrement et fins par ordre coherents) et empreinte FUL1 EGALE a
# celle de mhgp12_tower_chain (voie sequentielle).
mhgp12_python_gate(mhgp12_full_probe_cpu_recouvert 0 full_probe_check.py $<TARGET_FILE:mhgp12_full_probe>
                   $<TARGET_FILE:mhgp12_tower_chain> --recouvert
                   LINE "full_probe_ok passes=7 trames=2 identite_chaine=oui schema=recouvert" LABELS fast TIMEOUT 600)
