# Portes du catalogue (tranche T1, docs/CONTRAT_CATALOGUE.md, paragraphe 6). Raisons emises par le module et leurs
# portes : kmax_out_of_range, multiplicity_unsupported, wide_leaf (unit.cpp, refusals ; oracle.py pour les doublons),
# shell_capacity (unit.cpp, witness_sphere50), catalogue_counter_overflow (unit.cpp, counter_overflow : somme
# controlee a 2^64 - 1). catalogue_invariant (statut invariant_violated) n'est rendu par aucune entree valide : les
# mutants qui cassent l'ordre, le census ou la canonisation le provoquent (sonde : code 3).
add_executable(mhgp12_catalogue_probe ${PROJECT_SOURCE_DIR}/bench/catalogue_probe.cpp)
target_link_libraries(mhgp12_catalogue_probe PRIVATE mhgp12)
set(catalogue_probe $<TARGET_FILE:mhgp12_catalogue_probe>)

# Temoins graves (WIT-T1-CARRE, rectangle des deux conventions de S*, WIT-SPHERE50, WIT-FEUILLES, profondeurs 60 et
# 63 de CST-0205 avec leurs grands livres de la v11), refus, somme des compteurs, paliers de la feuille (homothetie
# jusqu'aux deux bords du domaine du profil : racine fermee a 2^32 au profil 32, CST-0204), determinisme a 1, 4 et 8
# fils.
mhgp12_add_unit(mhgp12_catalogue_unit SOURCES unit.cpp
                GROUPS witness_square witness_rectangle witness_transl witness_long_triangle witness_sphere50 witness_leaves depth_witnesses refusals
                       counter_overflow tiers wide_traversal determinism
                LABELS fast TIMEOUT 300)

# Fin d'etage partagee et voie appareil (tranche T1-b), jouees sur l'hote par l'executeur Pool, meme code que
# l'appareil : niveaux a mots contre num, tri par base et sommes prefixes, fin d'etage sur enregistrements construits
# (egalites et quasi-egalites de niveaux, repli exact) contre une reference independante, voie appareil complete contre
# la voie CPU (temoins, feuilles non resolues rejouees sur l'hote, refus, etat resident) ; device_open : sans GPU,
# device_unavailable ; avec GPU (G4), la voie appareil reelle contre la voie CPU. Raisons : device_unavailable,
# device_fault. Tranche T2-d : chaine longue du repli exact (fenetres de cles doublees), premier toucher des pages
# (intervalle respecte), sorties en flux sur un transit simule (sequence des tranches de l'executeur CUDA), refus sous
# budget serre de l'hote et de l'appareil (transit simule ; device_open_budget : la vraie voie appareil sur G4).
# Tranche T1-d (catalogue en flux) : fin d'etage par tranches de cles (identite contre la voie complete a 1, 2, 7 et
# beaucoup de tranches, plan, jeux construits), voie CPU et voie appareil sous budgets serres (arene en flux).
mhgp12_add_unit(mhgp12_catalogue_device_unit SOURCES device_unit.cpp device_finish_test.cpp device_pipeline_test.cpp
                device_budget_test.cpp slices_test.cpp
                GROUPS level_words radix_and_scan stream_staging device_reasons finish_random finish_ties finish_plateau
                       finish_long_chain finish_budget finish_sliced prefault_pages pipeline_witnesses pipeline_staged
                       pipeline_budget pipeline_budget_reuse pipeline_resident slices_identity slices_plan
                       slices_cpu_budget slices_device_budget slices_device_reuse device_open
                       device_open_budget
                LABELS fast TIMEOUT 300)

# Arene en flux sur plusieurs lots de feuilles (tranche T1-d) : 150 000 sites a K5, six lots, porte longue.
mhgp12_add_unit(mhgp12_catalogue_slices_long SOURCES slices_long_test.cpp GROUPS slices_streaming LABELS long
                TIMEOUT 1800)

# Session G4 de la tranche T1-b (bench/g4_catalogue_device.py) : juge a regle ecrite d'avance, auto-test par injections
# (adopte, rejete, refuse) ; motifs des mutants appareil (bench/g4_catalogue_mutants.json) presents une seule fois.
mhgp12_python_gate(mhgp12_catalogue_g4_judge 0 ${PROJECT_SOURCE_DIR}/bench/g4_catalogue_device.py --selftest-judge
                   LINE "juge_g4_t1b_ok injections=18" LABELS fast)
mhgp12_python_gate(mhgp12_catalogue_g4_mutants 0 ${PROJECT_SOURCE_DIR}/bench/g4_catalogue_device.py --check-mutants
                   ${PROJECT_SOURCE_DIR} LINE "mutants_appareil_ok mutants=3" LABELS fast)

# Session G4 de la tranche T2-d (bench/g4_catalogue_flux.py, transferts et publication) : juge a regle ecrite
# d'avance qui relit les lignes natives des sondes liees a leur commande (g4_catalogue_flux_lecteur.py), auto-test
# par injections dont les cas de la contre-lecture d'admission de l'auditeur (g4_catalogue_flux_selftest.py) ; motifs
# des bras d'ablation et du mutant appareil presents une seule fois.
mhgp12_python_gate(mhgp12_catalogue_g4_flux_judge 0 ${PROJECT_SOURCE_DIR}/bench/g4_catalogue_flux.py --selftest-judge
                   LINE "juge_g4_t2dc_ok injections=39 admission=25 cohorte=8" LABELS fast)
mhgp12_python_gate(mhgp12_catalogue_g4_flux_substitutions 0 ${PROJECT_SOURCE_DIR}/bench/g4_catalogue_flux.py
                   --check-substitutions ${PROJECT_SOURCE_DIR} LINE "substitutions_ok bras=4 mutant=1" LABELS fast)

# Session G4 de la tranche T1-d (bench/g4_catalogue_t1d.py, catalogue en flux) : juge a regle ecrite d'avance (A/A,
# A/B de la trame, voie en flux sous budgets de l'appareil), auto-test par injections.
mhgp12_python_gate(mhgp12_catalogue_g4_t1d_judge 0 ${PROJECT_SOURCE_DIR}/bench/g4_catalogue_t1d.py --selftest-judge
                   LINE "juge_g4_t1d_ok injections=15" LABELS fast)

# Oracle borne : egalite avec l'etage B de reference/hgp12_ref sur la suite rapide (n <= 14) ; doublons refuses (D8).
mhgp12_python_gate(mhgp12_catalogue_oracle 0 oracle.py ${catalogue_probe}
                   LINE "catalogue_oracle_ok nuages=342 boules=6988 doublons_refuses=34" LABELS oracle fast TIMEOUT 300)
mhgp12_expect_code(mhgp12_catalogue_probe_usage 2 mhgp12_catalogue_probe --k=5 LABELS fast)

# Echelle (invariants globaux, jamais un juge exhaustif) : nuages synthetiques de 8 000, 16 000 et 32 000 sites
# (--uniform=N,20261007,18) ; boules, incidences, niveaux et vingt compteurs logiques egaux a ceux de la v11 sur la
# meme entree (v11_counts.json), memes octets a 1, 4 et 8 fils.
foreach(n 8000 16000 32000)
  set(counts_8000 "boules=600630 incidences=2908188 niveaux=600613")
  set(counts_16000 "boules=1238402 incidences=6006903 niveaux=1238293")
  set(counts_32000 "boules=2540941 incidences=12338428 niveaux=2540177")
  mhgp12_python_gate(mhgp12_catalogue_scale${n} 0 ledger_gate.py ${catalogue_probe} synth_u${n}_k5_l24
                     --uniform=${n},20261007,18 --k=5 --leaf=24 --threads=1,4,8
                     LINE "catalogue_ledger_ok cas=synth_u${n}_k5_l24 ${counts_${n}} fils=1,4,8"
                     LABELS scale${n} TIMEOUT 900)
endforeach()

# Filets : identite d'Euler a K+2 (JUG-EULER) et restriction J1 ; petit nuage avec recensement exact de chaque site
# liste, puis l'uniforme de 8 000 sites.
mhgp12_python_gate(mhgp12_catalogue_euler_small 0 euler.py ${catalogue_probe} --uniform=3000,5,16 --k=5 --leaf=24
                   --census LINE "catalogue_euler_ok k=5 n=3000 ordres=5 boules_k=210786 boules_k2=454667 etendues=0"
                   LABELS oracle fast TIMEOUT 300)
mhgp12_python_gate(mhgp12_catalogue_euler_scale8000 0 euler.py ${catalogue_probe} --uniform=8000,20261007,18 --k=5
                   --leaf=24 LINE "catalogue_euler_ok k=5 n=8000 ordres=5 boules_k=600630 boules_k2=1307611 etendues=0" LABELS scale8000 TIMEOUT 900)

# Trames LiDAR (MHGP12_DATA_DIR) : grand livre de la v11 et determinisme, puis Euler sur ng00.
foreach(frame ng00 ng01 ng02)
  set(lidar_ng00 "boules=1306696 incidences=6097121 niveaux=1085776")
  set(lidar_ng01 "boules=1095926 incidences=5085683 niveaux=941217")
  set(lidar_ng02 "boules=1407885 incidences=6514697 niveaux=1099582")
  mhgp12_python_gate(mhgp12_catalogue_lidar_${frame}_k5 0 ledger_gate.py ${catalogue_probe} lidar_${frame}_k5_l24
                     --data=lidar_${frame} --k=5 --leaf=24 --threads=1,4,8
                     LINE "catalogue_ledger_ok cas=lidar_${frame}_k5_l24 ${lidar_${frame}} fils=1,4,8"
                     LABELS lidar long TIMEOUT 1800)
endforeach()
mhgp12_python_gate(mhgp12_catalogue_euler_lidar_ng00_k5 0 euler.py ${catalogue_probe} --data=lidar_ng00 --k=5
                   --leaf=24 LINE "catalogue_euler_ok k=5 n=39885 ordres=5 boules_k=1306696 boules_k2=2565656 etendues=320" LABELS lidar long TIMEOUT 1800)

# Differentiel contre les vidages de la v11 gelee (MHGP12_V11_CATALOGUE_DIR : <cas>_k5/cat.bin produits par
# mhgp12_vidage, feuille 24, trame <cas>) : la sonde exporte Cat_5, le lecteur de transition
# reference/transition_catalogue.py le juge (paragraphe 6.1 : bijection par centre exact et rayon carre, rangs, p, m,
# q_min, I, U, ordre publie, support minimal et convention de S* sur toute coquille etendue). Ligne attendue : la
# ligne de conformite du lecteur (comptes et ecarts de convention graves). Sans ce dossier, les portes ne sont pas
# enregistrees et la configuration le dit.
set(MHGP12_V11_CATALOGUE_DIR "$ENV{MHGP12_V11_CATALOGUE_DIR}" CACHE PATH
    "Vidages cat.bin de la v11 gelee par cas (ng00_k5, ..., u32000_k5) ; vide : portes diff_v11 du catalogue absentes")
set(transition_reader ${PROJECT_SOURCE_DIR}/reference/transition_catalogue.py)
set(diff_ng00 "boules=1306696 incidences=6097121 niveaux=1085776 coquilles_etendues=227 supports_multiples=3")
set(diff_ng01 "boules=1095926 incidences=5085683 niveaux=941217 coquilles_etendues=135 supports_multiples=1")
set(diff_ng02 "boules=1407885 incidences=6514697 niveaux=1099582 coquilles_etendues=572 supports_multiples=8")
set(diff_u8000 "boules=597998 incidences=2895136 niveaux=597987 coquilles_etendues=0 supports_multiples=0")
set(diff_u16000 "boules=1233046 incidences=5979160 niveaux=1232923 coquilles_etendues=0 supports_multiples=0")
set(diff_u32000 "boules=2536732 incidences=12316439 niveaux=2535983 coquilles_etendues=0 supports_multiples=0")
set(sstar_ng00 "sstar_selon_convention=0 sstar_differents=0 renumerotees=170938")
set(sstar_ng01 "sstar_selon_convention=0 sstar_differents=0 renumerotees=131082")
set(sstar_ng02 "sstar_selon_convention=2 sstar_differents=2 renumerotees=212365")
set(sstar_u8000 "sstar_selon_convention=0 sstar_differents=0 renumerotees=12")
set(sstar_u16000 "sstar_selon_convention=0 sstar_differents=0 renumerotees=136")
set(sstar_u32000 "sstar_selon_convention=0 sstar_differents=0 renumerotees=652")
set(file_ng00 lidar_ng00)
set(file_ng01 lidar_ng01)
set(file_ng02 lidar_ng02)
set(file_u8000 uniform_u18_n8000)
set(file_u16000 uniform_u18_n16000)
set(file_u32000 uniform_u18_n32000)
if(MHGP12_V11_CATALOGUE_DIR AND EXISTS "${MHGP12_V11_CATALOGUE_DIR}/ng00_k5/cat.bin")
  foreach(case ng00 ng01 ng02 u8000 u16000 u32000)
    mhgp12_python_gate(mhgp12_catalogue_diff_v11_${case}_k5 0 diff_case.py ${catalogue_probe} ${transition_reader}
                       ${MHGP12_V11_CATALOGUE_DIR}/${case}_k5/cat.bin --data=${file_${case}} --frame=${case} --k=5
                       --leaf=24 LINE "transition_catalogue_conforme ${diff_${case}} ${sstar_${case}}"
                       LABELS lidar long TIMEOUT 1800)
  endforeach()
else()
  message(STATUS "mhgp12 : portes diff_v11 du catalogue absentes (MHGP12_V11_CATALOGUE_DIR='${MHGP12_V11_CATALOGUE_DIR}')")
endif()
