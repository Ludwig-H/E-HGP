# Portes du module supports (tranche S6a de la sortie parametree : Q_b et comptes du lemme G ; l'assemblage attend le
# rattachement de la tour, tranche S3). Attendus des fixtures calcules par la definition en Fraction.
mhgp11_add_unit(mhgp11_supports_unit SOURCES supports_test.cpp
                GROUPS square right_triangle growth cube octahedron circle lines mixed shell_bound refusals constants
                       concurrency
                LABELS fast)

# Sonde JSON canonique (Q_b, fermeture, comptes par boule ; agregats sur W_K ; registres de la foret d'ordre K).
add_executable(mhgp11_supports_probe ${CMAKE_CURRENT_LIST_DIR}/supports_probe.cpp)
target_link_libraries(mhgp11_supports_probe PRIVATE mhgp11)
# Fixture 13 (sphere50) : les 84 points entiers de x^2 + y^2 + z^2 = 50 translates de +10 forment une coquille de 84
# sites au-dela du plafond kMaxShell = 24 : refus support_shell_capacity de l'appel entier, aucune ligne de boule.
mhgp11_expect_code(mhgp11_supports_shell_capacity 2 mhgp11_supports_probe --sphere=50,10 --k=2 --all
                   LINE "supports_probe_verdict refus support_shell_capacity" LABELS unit fast)

# Juge Fraction (sample_judge.py, bibliotheque standard) sur TOUTES les boules de Cat_3 de 40 petits nuages de grille
# (12 a 40 points, cotes 3 a 5) : coquilles etendues, supports multiples et tetraedres nombreux ; couverture gravee.
mhgp11_python_gate(mhgp11_supports_judge_small 0 sample_judge.py $<TARGET_FILE:mhgp11_supports_probe>
                   ${PROJECT_BINARY_DIR}/supports_judge_small --small=20261004,40 --k=3
                   --min-balls=9067 --min-extended=4520 --min-multiple=1545 --min-tetra=1554 --min-brute=9066
                   LINE "supports_sample_judge_couverture boules=9067 etendues=4520 multiples=1545 tetraedres=1554 brutes=9066"
                   LABELS oracle fast TIMEOUT 300)

# Juge d'echantillon a l'echelle (paragraphe 8.3 de la specification) : 200 boules de Cat_5 tirees a graine fixe, plus
# des coquilles etendues tirees de meme ; U_b par force brute sur les n sites, Q_b, N_j et comptes recalcules,
# denombrement brut si C(p+m, K+1) <= 20000. uniform18 n'a aucune coquille etendue dans Cat_5 (comme Cat_7, porte
# mhgp11_catalogue_euler_scale8000) : la grille cospherique de 8 000 points et la trame ng00 portent ce plancher.
mhgp11_python_gate(mhgp11_supports_sample_judge_scale8000 0 sample_judge.py $<TARGET_FILE:mhgp11_supports_probe>
                   ${PROJECT_BINARY_DIR}/supports_judge_u8000 --uniform18=8000,20261002 --k=5 --sample=200,20261004
                   --extended=100 --workers=3 --min-balls=200 --min-tetra=43 --min-brute=200
                   LINE "supports_sample_judge_couverture boules=200 etendues=0 multiples=0 tetraedres=43 brutes=200"
                   LABELS scale8000 TIMEOUT 1800)
mhgp11_python_gate(mhgp11_supports_sample_judge_grid8000 0 sample_judge.py $<TARGET_FILE:mhgp11_supports_probe>
                   ${PROJECT_BINARY_DIR}/supports_judge_g8000 --grid=8000,32,1000,20261004 --k=5 --sample=200,20261004
                   --extended=100 --workers=3 --min-balls=300 --min-extended=149 --min-multiple=49 --min-tetra=178
                   --min-brute=300
                   LINE "supports_sample_judge_couverture boules=300 etendues=149 multiples=49 tetraedres=178 brutes=300"
                   LABELS scale8000 TIMEOUT 1800)
mhgp11_python_gate(mhgp11_supports_sample_judge_lidar_ng00_k5 0 sample_judge.py $<TARGET_FILE:mhgp11_supports_probe>
                   ${PROJECT_BINARY_DIR}/supports_judge_ng00 --data=lidar_ng00 --k=5 --sample=200,20261004
                   --extended=200 --workers=3 --min-balls=400 --min-extended=200 --min-multiple=3 --min-tetra=28
                   --min-brute=400
                   LINE "supports_sample_judge_couverture boules=400 etendues=200 multiples=3 tetraedres=28 brutes=400"
                   LABELS lidar TIMEOUT 1800)

# Contre-epreuve globale des comptes par les registres de la foret d'ordre K (voie sequentielle de reference, aucune
# descente supplementaire) : arithmetiques distinctes (bounded_meb des traces contre predicats de num et fermeture
# zeta). Toute W_K : classified_cells = |W_K|, classification.combinations = somme C(m,t), replayed_cells = cellules,
# cells.combinations = somme C(m,t) des cellules, trace_resolutions = somme strict_traces, births = naissances.
# Ligne de verdict de la sonde gravee (|Cat_K|, |W_K|, coquilles etendues, supports, boules a plusieurs supports).
# Familles uniform18 des bancs du catalogue (empreintes entree= egales a celles des portes
# mhgp11_catalogue_euler_scale*) et grille cospherique de 8 000 points (125 064 coquilles etendues dans W_5, m <= 14).
foreach(case "scale8000;uniform8000;uniform18=8000,20261002;n=8000 boules=597998 choisies=395667 etendues=0 supports=395667 multiples=0 coquille_max=4 entree=3be1324202d28360"
             "scale8000;grid8000;grid=8000,32,1000,20261004;n=8000 boules=468514 choisies=337517 etendues=125064 supports=502847 multiples=55729 coquille_max=14 entree=5a5697d925f68c1e"
             "scale16000;uniform16000;uniform18=16000,20261002;n=16000 boules=1233046 choisies=819004 etendues=0 supports=819004 multiples=0 coquille_max=4 entree=3469c29b4c34b7e3"
             "scale32000;uniform32000;uniform18=32000,20261002;n=32000 boules=2536732 choisies=1690045 etendues=0 supports=1690045 multiples=0 coquille_max=4 entree=aea3dec129cef911")
  list(GET case 0 label)
  list(GET case 1 tag)
  list(GET case 2 input)
  list(GET case 3 counts)
  mhgp11_python_gate(mhgp11_supports_registers_${tag}_k5 0 sample_judge.py $<TARGET_FILE:mhgp11_supports_probe>
                     ${PROJECT_BINARY_DIR}/supports_registers_${tag} --${input} --k=5 --registers --workers=3
                     LINE "supports_probe_verdict conforme k=5 ${counts}"
                     LABELS ${label} TIMEOUT 1800)
endforeach()
# Trames LiDAR entieres sans sol (MHGP11_DATA_DIR, jamais recopiees) : K5 gravees (|Cat_5| egal a la porte
# mhgp11_catalogue_euler_lidar_*_k5, |W_5| egal au registre work.cells de l'ordre 5 du recu c40_paired) ; K10 en label
# long, verdict a graver apres la session G4.
foreach(case "ng00;n=39885 boules=1306696 choisies=789886 etendues=141 supports=789889 multiples=3 coquille_max=5 entree=975c390e5912fabe"
             "ng01;n=35551 boules=1095926 choisies=652958 etendues=81 supports=652959 multiples=1 coquille_max=4 entree=6b918ef47e9ae56e"
             "ng02;n=45845 boules=1407885 choisies=832386 etendues=354 supports=832394 multiples=8 coquille_max=5 entree=6e11fa8bc5ee6432")
  list(GET case 0 frame)
  list(GET case 1 counts)
  mhgp11_python_gate(mhgp11_supports_registers_lidar_${frame}_k5 0 sample_judge.py
                     $<TARGET_FILE:mhgp11_supports_probe> ${PROJECT_BINARY_DIR}/supports_registers_${frame}_k5
                     --data=lidar_${frame} --k=5 --registers --workers=3
                     LINE "supports_probe_verdict conforme k=5 ${counts}"
                     LABELS lidar TIMEOUT 1800)
endforeach()
foreach(frame ng00 ng01 ng02)
  mhgp11_python_gate(mhgp11_supports_registers_lidar_${frame}_k10 0 sample_judge.py
                     $<TARGET_FILE:mhgp11_supports_probe> ${PROJECT_BINARY_DIR}/supports_registers_${frame}_k10
                     --data=lidar_${frame} --k=10 --registers --workers=8
                     LABELS lidar long TIMEOUT 3600)
endforeach()
