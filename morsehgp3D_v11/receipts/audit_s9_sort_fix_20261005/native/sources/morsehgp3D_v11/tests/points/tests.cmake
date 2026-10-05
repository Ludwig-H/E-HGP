# Portes du module points (tranche S9 de la sortie parametree : hierarchie de points H^r_{K+1}).
add_executable(mhgp11_points_probe ${CMAKE_CURRENT_LIST_DIR}/points_probe.cpp)
target_link_libraries(mhgp11_points_probe PRIVATE mhgp11)
target_include_directories(mhgp11_points_probe PRIVATE ${PROJECT_SOURCE_DIR}/bench)
# Exportateur MHGP11PH de reference (bench/points_export.cpp) : celui des portes de la tour s'il est declare, sinon la
# meme source sous un nom propre (construction -DMHGP11_MODULES=points des mutants), memes octets.
if(TARGET mhgp11_points_export)
  set(mhgp11_points_reference $<TARGET_FILE:mhgp11_points_export>)
else()
  add_executable(mhgp11_points_export_reference ${PROJECT_SOURCE_DIR}/bench/points_export.cpp)
  target_link_libraries(mhgp11_points_export_reference PRIVATE mhgp11)
  target_include_directories(mhgp11_points_export_reference PRIVATE ${PROJECT_SOURCE_DIR}/bench)
  set(mhgp11_points_reference $<TARGET_FILE:mhgp11_points_export_reference>)
endif()

# Unitaires : index d'ancetres contre la remontee naive, determinisme (sans Pool, 1, 2 et 4 fils), refus (K = n,
# m hors domaine, budget), budget rendu.
mhgp11_add_unit(mhgp11_points_unit SOURCES points_test.cpp GROUPS ancestors determinism sort_refusal refusals budget LABELS fast)
target_include_directories(mhgp11_points_unit PRIVATE ${PROJECT_SOURCE_DIR}/tests/supports)

# Les douze fixtures de bench/points_gate.py rejouees en bibliotheque standard sur la sonde (dont trois sous la regle
# en rayon, attendus recalcules), plus K = 1 (liaison simple, m(1) = 1). Tue qualification_decalee, sans_marge,
# coupe_ouverte, m_k1_deux.
mhgp11_python_gate(mhgp11_points_fixtures 0 points_fixtures.py --probe $<TARGET_FILE:mhgp11_points_probe>
                   --work ${PROJECT_BINARY_DIR}/points_fixtures
                   LINE "points_fixtures_verdict conforme fixtures=13/13 nuages_k1=30 comparaisons=7000"
                   LABELS fast TIMEOUT 300)
# Oracle de la definition sans numpy (extrait de bench/points_reference.py) : dates et proprietaires exacts site par
# site, blocs de l'arbre de points a chaque plateau, K = 1..4, m(K) et m = 1 ; planchers 150 nuages, 4 000
# comparaisons, 500 sites retardes (specification, paragraphe 8.7) ; au moins 10 decisions natives tranchees par le
# repli exact (egalites de rangs distincts : le reste est decide par la table des racines). Tue marge_carree.
mhgp11_python_gate(mhgp11_points_oracle 0 points_oracle_stdlib.py --probe $<TARGET_FILE:mhgp11_points_probe>
                   --work ${PROJECT_BINARY_DIR}/points_oracle --clouds 160 --min-clouds 150 --min-comparisons 4000
                   --min-delayed 500 --min-exact 10
                   LINE "points_oracle_verdict conforme nuages=164 ordres=1086 comparaisons=7088 retardes=3022 plateaux=5720 replis=12"
                   LABELS oracle fast TIMEOUT 600)
# Identite exacte site par site et arbre de points contre la chaine Python qualifiee (bench/points_radius.py
# hang_margin_radius(order, m(K)), points_flat.tower_point_tree) sur le meme MHGP11PH : numpy, label long.
mhgp11_python_gate(mhgp11_points_vs_python 0 points_vs_python.py --probe $<TARGET_FILE:mhgp11_points_probe>
                   --export ${mhgp11_points_reference} --work ${PROJECT_BINARY_DIR}/points_vs_python --clouds 400
                   --m-all --uniform=300,2000,8000 --k=1,2,3,4,5 --min-clouds 407 --min-sites 100000
                   --min-delayed 50000 LABELS long TIMEOUT 3600)
foreach(frame ng00 ng01 ng02)
  mhgp11_python_gate(mhgp11_points_vs_python_lidar_${frame}_k5 0 points_vs_python.py
                     --probe $<TARGET_FILE:mhgp11_points_probe> --export ${mhgp11_points_reference}
                     --work ${PROJECT_BINARY_DIR}/points_vs_python_${frame} --clouds 0 --data=lidar_${frame} --k=5
                     --workers 8 --min-sites 30000 --min-delayed 20000 LABELS lidar long TIMEOUT 3600)
endforeach()

# Echelle et trames (K5) par l'executable : lecteur officiel (structure sur tout le fichier, lecture exacte de 500
# sites et plateaux tires), fichiers identiques a W1 et W4 et sous permutation, reetiquetage, tree_k_sha256 egal a
# --sortie=supports. Invariants : laminarite (blocs), proprietaire vivant au plancher, t <= Q < M.
if(TARGET mhgp11_cli)
  # Comptes graves (retardes, plateaux) : memes entrees, meme Cat_5 aux trois profils.
  foreach(case "8000;7000;retardes=7801 plateaux=7871" "16000;14000;retardes=15676 plateaux=15751"
               "32000;28000;retardes=31491 plateaux=31458")
    list(GET case 0 n)
    list(GET case 1 delayed)
    list(GET case 2 counts)
    mhgp11_python_gate(mhgp11_points_scale${n} 0 ${PROJECT_SOURCE_DIR}/tests/cli/cli_points.py
                       --cli $<TARGET_FILE:mhgp11_cli> --bits ${MHGP11_COORD_BITS} --uniform=${n} --k=5
                       --min-delayed=${delayed} LINE "cli_points_verdict conforme cas=1 appels=5 refus=0 ${counts}"
                       LABELS scale${n} TIMEOUT 3600)
  endforeach()
  foreach(case "ng00;retardes=34509 plateaux=37684" "ng01;retardes=31286 plateaux=33496"
               "ng02;retardes=41792 plateaux=43148")
    list(GET case 0 frame)
    list(GET case 1 counts)
    mhgp11_python_gate(mhgp11_points_lidar_${frame}_k5 0 ${PROJECT_SOURCE_DIR}/tests/cli/cli_points.py
                       --cli $<TARGET_FILE:mhgp11_cli> --bits ${MHGP11_COORD_BITS} --data=lidar_${frame} --k=5
                       --min-delayed=20000 LINE "cli_points_verdict conforme cas=1 appels=5 refus=0 ${counts}"
                       LABELS lidar TIMEOUT 3600)
  endforeach()
endif()
