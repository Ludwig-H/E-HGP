# Portes du module head (tranche S10 de la sortie parametree : sortie plate certifiee).

# Unitaires sur arbres abstraits : F14a a F14e (attendus manuels de clusters), egalites EOM certifiees (z = 1
# rationnelle et irrationnelle, z = 2), repli exact des dates (Delta != 0 et Delta = 0, trois racines), ordre d'entree,
# refus, budget rendu.
mhgp11_add_unit(mhgp11_head_unit SOURCES head_test.cpp GROUPS fixtures equalities dates order refusals budget
                LABELS fast)

if(TARGET mhgp11_cli)
  # Tete native contre la tete Python qualifiee (bench/points_flat.py : condense, select, labels) sur le MEME arbre de
  # points publie (MHGP11PT relu par le lecteur officiel) : meme partition des points (bijection des clusters, meme
  # bruit), etiquette = plus petit PointId du cluster, tree_k_sha256 egal a --sortie=points ; mcs 3, 5, 10, 20 et
  # EOM z = 1, 2, 3, feuilles. numpy : label long (joue en local, hors G4). Synthetique : 24 nuages en amas a PointId
  # non denses, K = 1..5 ; trames K5 : mcs 10 et 20.
  mhgp11_python_gate(mhgp11_head_vs_python 0 head_vs_python.py --cli $<TARGET_FILE:mhgp11_cli>
                     --bits ${MHGP11_COORD_BITS} --work ${PROJECT_BINARY_DIR}/head_vs_python --clouds 24
                     --k=1,2,3,4,5 --mcs=3,5,10,20 --min-clouds 120 --min-calls 1920 --min-clusters 16000
                     LINE "head_vs_python_verdict conforme nuages=120 appels=1920 clusters=16474 retenus=16474 bruit=49474"
                     LABELS long TIMEOUT 3600)
  foreach(case "ng00;clusters=6086 retenus=6086 bruit=52434" "ng01;clusters=4774 retenus=4774 bruit=42194"
               "ng02;clusters=4978 retenus=4978 bruit=52770")
    list(GET case 0 frame)
    list(GET case 1 counts)
    mhgp11_python_gate(mhgp11_head_vs_python_lidar_${frame}_k5 0 head_vs_python.py --cli $<TARGET_FILE:mhgp11_cli>
                       --bits ${MHGP11_COORD_BITS} --work ${PROJECT_BINARY_DIR}/head_vs_python_${frame}
                       --data=lidar_${frame} --k=5 --mcs=10,20 --min-calls 8 --min-clusters 1000
                       LINE "head_vs_python_verdict conforme nuages=1 appels=8 ${counts}" LABELS lidar long TIMEOUT 3600)
  endforeach()

  # Echelle et trames (K5, EOM z = 1 mcs 20, ligne LiDAR publiee) par l'executable : lecteur officiel, fichiers
  # identiques a W1 et W4, memes etiquettes par PointId sous permutation de l'entree, etiquette = plus petit PointId
  # du cluster. retenus : somme des trois appels (W1, W4, permute).
  foreach(case "8000;retenus=321" "16000;retenus=675" "32000;retenus=1425")
    list(GET case 0 n)
    list(GET case 1 counts)
    mhgp11_python_gate(mhgp11_plat_scale${n} 0 ${PROJECT_SOURCE_DIR}/tests/cli/cli_plat.py
                       --cli $<TARGET_FILE:mhgp11_cli> --bits ${MHGP11_COORD_BITS} --uniform=${n} --k=5
                       --min-selected=50 LINE "cli_plat_verdict conforme cas=1 appels=3 refus=0 egalites=0 ${counts}"
                       LABELS scale${n} TIMEOUT 3600)
  endforeach()
  foreach(case "ng00;retenus=996" "ng01;retenus=672" "ng02;retenus=648")
    list(GET case 0 frame)
    list(GET case 1 counts)
    mhgp11_python_gate(mhgp11_plat_lidar_${frame}_k5 0 ${PROJECT_SOURCE_DIR}/tests/cli/cli_plat.py
                       --cli $<TARGET_FILE:mhgp11_cli> --bits ${MHGP11_COORD_BITS} --data=lidar_${frame} --k=5
                       --min-selected=50 LINE "cli_plat_verdict conforme cas=1 appels=3 refus=0 egalites=0 ${counts}"
                       LABELS lidar TIMEOUT 3600)
  endforeach()
endif()
