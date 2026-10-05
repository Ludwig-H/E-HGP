# Portes du module supports (tranche S6a de la sortie parametree : Q_b et comptes du lemme G ; l'assemblage attend le
# rattachement de la tour, tranche S3). Attendus des fixtures calcules par la definition en Fraction.
mhgp11_add_unit(mhgp11_supports_unit SOURCES supports_test.cpp witness_test.cpp
                GROUPS square right_triangle growth cube octahedron circle lines mixed shell_bound refusals constants
                       concurrency sphere5 sphere5_k12 square_k10 sphere9_refus impossible_arity
                LABELS fast)

# Sonde JSON canonique (Q_b, fermeture, comptes par boule ; agregats sur W_K ; registres de la foret d'ordre K).
add_executable(mhgp11_supports_probe ${CMAKE_CURRENT_LIST_DIR}/supports_probe.cpp)
target_link_libraries(mhgp11_supports_probe PRIVATE mhgp11)
# Fixture 13 (sphere50) : les 84 points entiers de x^2 + y^2 + z^2 = 50 translates de +10 forment une coquille de 84
# sites au-dela du plafond kMaxShell = 24 : refus support_shell_capacity de l'appel entier, aucune ligne de boule.
mhgp11_expect_code(mhgp11_supports_shell_capacity 2 mhgp11_supports_probe --sphere=50,10 --k=2 --all
                   LINE "supports_probe_verdict refus support_shell_capacity" LABELS unit fast)

# Differentiel contre l'oracle borne S1 (reference/hgp11_ref/supports.py, Fraction, etage A) sur les nuages de sa suite
# (fixtures de la specification et des audits, nuages d'Euler, fixtures historiques, familles a graine 31) et a ses
# ordres : memes boules de W_K, memes (p, m, qmin), memes comptes du lemme G, memes comptes par support (apport des
# auditeurs du 5 octobre 2026). Un nuage hors du domaine du profil est exclu et compte (u18 : les deux cercles
# n = 1023 de la specification) ; couverture gravee par profil.
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_supports_fraction_line "supports_fraction_couverture bits=18 nuages=208 exclus=2 ordres=945 boules=15038 supports=16913 etendues=2545 multiples=524 tetraedres=1145")
  set(mhgp11_supports_fraction_floors --min-clouds=208 --min-orders=945 --min-balls=15038 --min-supports=16913
      --min-extended=2545 --min-multiple=524 --min-tetra=1145)
else()
  set(mhgp11_supports_fraction_line "supports_fraction_couverture bits=${MHGP11_COORD_BITS} nuages=210 exclus=0 ordres=951 boules=15062 supports=16943 etendues=2551 multiples=530 tetraedres=1145")
  set(mhgp11_supports_fraction_floors --min-clouds=210 --min-orders=951 --min-balls=15062 --min-supports=16943
      --min-extended=2551 --min-multiple=530 --min-tetra=1145)
endif()
mhgp11_python_gate(mhgp11_supports_fraction 0 fraction_diff.py $<TARGET_FILE:mhgp11_supports_probe>
                   ${PROJECT_BINARY_DIR}/supports_fraction --bits=${MHGP11_COORD_BITS} ${mhgp11_supports_fraction_floors}
                   LINE "${mhgp11_supports_fraction_line}" LABELS oracle fast TIMEOUT 600)

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

# Assemblage de la hierarchie des supports (tranche S6b, build_support_hierarchy) : juge complet des petits nuages,
# determinisme (sans Pool, Pools de 1, 2 et 4 fils, arbre seriel ou par lots), permutation et reetiquetage, admission
# a 24 sites (sphere5), refus de l'appel entier a 25 sites (sphere9), formule d'admission exacte et refus avant toute
# allocation ; panne de chaque allocation.
mhgp11_add_unit(mhgp11_supports_hierarchy SOURCES hierarchy_test.cpp
                GROUPS fixtures determinism permutation sphere5 sphere9 admission LABELS fast)
mhgp11_add_unit(mhgp11_supports_hierarchy_fault SOURCES hierarchy_fault.cpp GROUPS starvation LABELS fast)

# Sonde de la hierarchie : requetes JSON (differentiel contre l'oracle S1) et mode echelle (I5, I6, I11, rattachement,
# empreinte identique a W1, W2, W4 et sous permutation de l'entree).
add_executable(mhgp11_supports_hierarchy_probe ${CMAKE_CURRENT_LIST_DIR}/hierarchy_probe.cpp)
target_link_libraries(mhgp11_supports_hierarchy_probe PRIVATE mhgp11)

# Differentiel COMPLET de la hierarchie contre l'oracle borne S1 (hierarchy_fraction.py) : noeuds, rattachements,
# roles, branches, Q_b et comptes, ordre natif (postordre, rang, BallIdx ; arite, SiteIdx = rangs de Morton), sur les
# nuages et ordres de mhgp11_tower_attach_fraction ; voie serielle sans Pool (W1) et voie par lots sur 3 fils (W3).
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_hierarchy_fraction_line "hierarchy_fraction_couverture bits=18 nuages=209 exclus=2 ordres=957 boules=15246 noeuds=12552 naissances=6641 fusions=5848 internes=2757 supports=17167 etendues=2639 multiples=570 tetraedres=1145 ordres_6plus=7 fils=1,3")
  set(mhgp11_hierarchy_fraction_floors --min-clouds=209 --min-orders=957 --min-balls=15246 --min-nodes=12552
      --min-supports=17167 --min-extended=2639 --min-multiple=570)
else()
  set(mhgp11_hierarchy_fraction_line "hierarchy_fraction_couverture bits=${MHGP11_COORD_BITS} nuages=211 exclus=0 ordres=963 boules=15270 noeuds=12576 naissances=6651 fusions=5858 internes=2761 supports=17197 etendues=2645 multiples=576 tetraedres=1145 ordres_6plus=7 fils=1,3")
  set(mhgp11_hierarchy_fraction_floors --min-clouds=211 --min-orders=963 --min-balls=15270 --min-nodes=12576
      --min-supports=17197 --min-extended=2645 --min-multiple=576)
endif()
mhgp11_python_gate(mhgp11_supports_hierarchy_fraction 0 hierarchy_fraction.py
                   $<TARGET_FILE:mhgp11_supports_hierarchy_probe> --bits=${MHGP11_COORD_BITS} --workers=1,3
                   ${mhgp11_hierarchy_fraction_floors} --min-births=6641 --min-merges=5848 --min-internals=2757
                   --min-tetra=1145 --min-high=7 LINE "${mhgp11_hierarchy_fraction_line}" LABELS oracle fast TIMEOUT 600)

# Invariants a l'echelle (specification 8.4) : I5, I6, I11 et rattachement juges sur toute W_5 (fermeture brute de
# chaque coquille), empreinte identique sans Pool et sur 2 et 4 fils (W1, W2, W4), et sous permutation de l'entree
# (meme empreinte que l'entree non permutee). Arbre par la voie par lots sur 4 fils. uniform18 : memes entrees que les
# portes mhgp11_tower_attach_scale* (empreinte entree=) ; grid8000 : grille cospherique propre a cette sonde (coquilles
# etendues jusqu'a 16 sites, boules a plusieurs supports).
set(mhgp11_hierarchy_8000 "k=5 n=8000 boules=395667 noeuds=273655 naissances=164842 fusions=108813 internes=122012 supports=395667 etendues=0 multiples=0 tetraedres=109772 branches=273654 fermetures=395667 kparties=1549792 cofaces=230825 incidences=230825 coquille_max=4 empreinte=8f153ae85de3dc6e entree=3be1324202d28360")
set(mhgp11_hierarchy_16000 "k=5 n=16000 boules=819004 noeuds=565098 naissances=340057 fusions=225041 internes=253906 supports=819004 etendues=0 multiples=0 tetraedres=227995 branches=565097 fermetures=819004 kparties=3213739 cofaces=478947 incidences=478947 coquille_max=4 empreinte=db71b4702625e23c entree=3469c29b4c34b7e3")
set(mhgp11_hierarchy_32000 "k=5 n=32000 boules=1690045 noeuds=1163756 naissances=700184 fusions=463577 internes=526284 supports=1690045 etendues=0 multiples=0 tetraedres=473506 branches=1163760 fermetures=1690045 kparties=6639350 cofaces=989861 incidences=989861 coquille_max=4 empreinte=d619483bb2bd7fe0 entree=aea3dec129cef911")
set(mhgp11_hierarchy_grid "k=5 n=8000 boules=340017 noeuds=123153 naissances=113503 fusions=223827 internes=2687 supports=506343 etendues=125169 multiples=55795 tetraedres=198234 branches=348945 fermetures=340017 kparties=10111750 cofaces=4594487 incidences=6735333 coquille_max=16 empreinte=ae41bbf6304ab905 entree=0ce2e238750a0e2c")
foreach(n 8000 16000 32000)
  mhgp11_expect_code(mhgp11_supports_hierarchy_scale${n} 0 mhgp11_supports_hierarchy_probe --uniform18=${n},20261002
                     --k=5 --workers=1,2,4 --min-balls=100000
                     LINE "hierarchy_probe_verdict conforme ${mhgp11_hierarchy_${n}}" LABELS scale${n} TIMEOUT 1800)
endforeach()
mhgp11_expect_code(mhgp11_supports_hierarchy_permutation_scale8000 0 mhgp11_supports_hierarchy_probe
                   --uniform18=8000,20261002 --k=5 --workers=4 --permute=20261005 --min-balls=100000
                   LINE "hierarchy_probe_verdict conforme ${mhgp11_hierarchy_8000}" LABELS scale8000 TIMEOUT 1800)
mhgp11_expect_code(mhgp11_supports_hierarchy_grid8000 0 mhgp11_supports_hierarchy_probe --grid=8000,32,1000,20261004
                   --k=5 --workers=1,2,4 --min-balls=100000 --min-extended=100000 --min-multiple=50000
                   LINE "hierarchy_probe_verdict conforme ${mhgp11_hierarchy_grid}" LABELS scale8000 TIMEOUT 1800)
mhgp11_expect_code(mhgp11_supports_hierarchy_permutation_grid8000 0 mhgp11_supports_hierarchy_probe
                   --grid=8000,32,1000,20261004 --k=5 --workers=4 --permute=7 --min-balls=100000
                   LINE "hierarchy_probe_verdict conforme ${mhgp11_hierarchy_grid}" LABELS scale8000 TIMEOUT 1800)
# Trames LiDAR entieres sans sol (MHGP11_DATA_DIR, jamais recopiees) a K5 : memes |W_5|, naissances, fusions,
# internes, branches et noeuds que mhgp11_tower_attach_e1e2_lidar_*_k5 ; K10 sur ng00 en label long (mesure).
foreach(case "ng00;k=5 n=39885 boules=789886 noeuds=576371 naissances=341081 fusions=235401 internes=213404 supports=789889 etendues=141 multiples=3 tetraedres=141332 branches=576483 fermetures=789886 kparties=3034691 cofaces=449011 incidences=449016 coquille_max=5 empreinte=9679af706d4ebcc7 entree=975c390e5912fabe"
             "ng01;k=5 n=35551 boules=652958 noeuds=478265 naissances=283207 fusions=195173 internes=174578 supports=652959 etendues=81 multiples=1 tetraedres=107851 branches=478385 fermetures=652958 kparties=2502103 cofaces=369857 incidences=369860 coquille_max=4 empreinte=3ebfb3cb792297db entree=6b918ef47e9ae56e"
             "ng02;k=5 n=45845 boules=832386 noeuds=609376 naissances=361326 fusions=248676 internes=222384 supports=832394 etendues=354 multiples=8 tetraedres=127461 branches=610022 fermetures=832386 kparties=3189636 cofaces=471577 incidences=471589 coquille_max=5 empreinte=8ca05a63c81b3e6e entree=6e11fa8bc5ee6432")
  list(GET case 0 frame)
  list(GET case 1 counts)
  mhgp11_expect_code(mhgp11_supports_hierarchy_lidar_${frame}_k5 0 mhgp11_supports_hierarchy_probe
                     --data=lidar_${frame} --k=5 --workers=1,2,4 --min-balls=500000 --min-extended=50
                     LINE "hierarchy_probe_verdict conforme ${counts}" LABELS lidar TIMEOUT 1800)
endforeach()
mhgp11_expect_code(mhgp11_supports_hierarchy_lidar_ng00_k10 0 mhgp11_supports_hierarchy_probe --data=lidar_ng00
                   --k=10 --workers=1,4,48 --tree-workers=48 --min-balls=1000000 LABELS lidar long TIMEOUT 3600)
