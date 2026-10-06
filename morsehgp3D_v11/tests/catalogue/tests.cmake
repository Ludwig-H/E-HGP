# Catalogue sequentiel : juges geometriques independants et frontiere transactionnelle.
mhgp11_add_unit(mhgp11_catalogue_unit SOURCES unit.cpp
                GROUPS fixtures refusals transaction obtuse_prefix q4_deferred LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_catalogue_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_catalogue_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_catalogue_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_catalogue_probe>
                    LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_judge 0 model_test.py LABELS oracle fast)
add_executable(mhgp11_catalogue_bench ${PROJECT_SOURCE_DIR}/bench/catalogue_probe.cpp)
target_link_libraries(mhgp11_catalogue_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_catalogue_bench_io 0 bench_test.py $<TARGET_FILE:mhgp11_catalogue_bench>
                    LABELS fast TIMEOUT 90)
mhgp11_python_gate(mhgp11_catalogue_bench_collector 0 bench_collector_test.py
                    LINE "catalogue_collector_verdict conforme attempts20 persisted6 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_semantic 0 bench_semantic_test.py
                    LINE "catalogue_semantic_verdict conforme fixtures8 corruptions17 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_profiles_io 0 bench_profiles_test.py
                    $<TARGET_FILE:mhgp11_catalogue_bench> ${MHGP11_COORD_BITS}
                    LINE "catalogue_profiles_io_verdict conforme fixtures4"
                    LABELS fast TIMEOUT 100)
mhgp11_python_gate(mhgp11_catalogue_profiles_collector 0 bench_profiles_collector_test.py
                    LINE "catalogue_profiles_collector_verdict conforme attempts25 corruptions12 schedules7 interrupted1 supplement10 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_semantic_cache 0 bench_semantic_cache_test.py
                    LINE "semantic_cache_verdict conforme helper16 keys12 transactions30 dependencies13 attempts22 campaigns2 native0"
                    LABELS fast TIMEOUT 30)

mhgp11_add_unit(mhgp11_catalogue_region SOURCES center_region.cpp
                GROUPS pair_region line_region obtuse_region median_envelope LABELS fast)

# Compteurs locaux de feuille (R1) : vidage controle et grandes feuilles.
mhgp11_add_unit(mhgp11_catalogue_leaf_counts SOURCES leaf_counts_test.cpp
                GROUPS flush_overflow large_leaves engraved LABELS fast)

mhgp11_add_unit(mhgp11_catalogue_parallel SOURCES parallel.cpp
                GROUPS equivalence frontier_overlap frontier_edges frontier_deep global_limits memory_and_refusals timings LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_parallel_fault SOURCES parallel_fault.cpp GROUPS starvation LABELS fast)
mhgp11_python_gate(mhgp11_catalogue_parallel_fraction 0 fraction_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> --parallel LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_parallel_judge 0 fraction_oracle.py --parallel-model
                    LABELS oracle fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_parallel_collector 0 bench_parallel_collector_test.py
                    LINE "catalogue_parallel_collector_verdict conforme attempts34 comparisons17 schedules11 checkpoints2 native0"
                    LABELS fast TIMEOUT 30)

mhgp11_add_unit(mhgp11_catalogue_leaf_coop SOURCES leaf_coop_test.cpp
                GROUPS witness_q3_obtuse_q4 cache_hits sizes near_max LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_cache SOURCES center_line_cache.cpp
                GROUPS ranks relations_reset capacity_fallback memory equivalence LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_cache_fault SOURCES center_line_cache_fault.cpp GROUPS allocation LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_sort SOURCES sort_indices_test.cpp
                GROUPS boundaries wide_levels refusals LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_sort_fault SOURCES sort_indices_fault.cpp GROUPS allocations LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_sort_fenv SOURCES sort_indices_fenv_test.cpp
                GROUPS key_modes permutations LABELS fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_catalogue_cache_fraction 0 optimization_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> --cache LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_sort_fraction 0 optimization_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> --sort LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_sort_cache_fraction 0 optimization_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> --sort --cache LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_optimization_model 0 optimization_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_optimizations_collector 0 bench_optimizations_test.py
                    LINE "optimizations_verdict conforme attempts30 comparisons8 schedules9 interrupted2 checks581 native0"
                    LABELS fast TIMEOUT 60)

mhgp11_add_unit(mhgp11_catalogue_adaptive SOURCES adaptive_frontier.cpp
                GROUPS equivalence round_memory plan_limits diagnostics_transaction LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_adaptive_fault SOURCES adaptive_fault.cpp GROUPS allocations LABELS fast)
mhgp11_python_gate(mhgp11_catalogue_adaptive_fraction 0 adaptive_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_adaptive_combined_fraction 0 adaptive_oracle.py
                    $<TARGET_FILE:mhgp11_catalogue_probe> --combined LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_adaptive_judge 0 adaptive_oracle.py --selftest
                    LINE "adaptive_oracle_model conforme positives3 corruptions36 native0"
                    LABELS oracle fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_catalogue_adaptive_model 0 adaptive_frontier_model.py
                    LABELS oracle fast TIMEOUT 30)

# Diagnostics possedes et ablation adaptative : protocole natif sur G4, collecteur Python simule ici.
mhgp11_python_gate(mhgp11_catalogue_adaptive_bench_io 0 bench_adaptive_io_test.py
                    $<TARGET_FILE:mhgp11_catalogue_bench> ${MHGP11_COORD_BITS}
                    LINE "catalogue_adaptive_io_verdict conforme attempts43 refusals9"
                    LABELS fast TIMEOUT 90)
mhgp11_python_gate(mhgp11_catalogue_adaptive_collector 0 bench_adaptive_collector_test.py
                    LINE "catalogue_adaptive_collector_verdict conforme positives13 corruptions43 attempts19 comparisons10 schedules9 interrupted2 options44 gzip13 reuse36 decoded12 reused24 native0"
                    LABELS fast TIMEOUT 60)

mhgp11_add_unit(mhgp11_catalogue_assembly SOURCES assembly_parallel_test.cpp
                GROUPS boundaries wide_levels refusals memory_and_equivalence pool_and_limits LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_assembly_fault SOURCES assembly_parallel_fault.cpp GROUPS allocations LABELS fast)
mhgp11_python_gate(mhgp11_catalogue_assembly_collector 0 bench_assembly_collector_test.py
                    LINE "assembly_collector_verdict conforme" LABELS fast TIMEOUT 60)

mhgp11_add_unit(mhgp11_catalogue_single SOURCES single_pass_test.cpp GROUPS pages equivalence refusals pool_busy LABELS fast)
mhgp11_add_unit(mhgp11_catalogue_single_fault SOURCES single_pass_fault.cpp
                GROUPS append_atomic allocations late_budget LABELS fast)
mhgp11_python_gate(mhgp11_catalogue_single_fraction 0 single_pass_oracle.py $<TARGET_FILE:mhgp11_catalogue_probe>
                    LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_single_model 0 single_pass_oracle.py --selftest
                    LINE "single_pass_model_verdict conforme positives3 corruptions36 native0" LABELS oracle fast TIMEOUT 30)

# Graphe J2 ferme jusqu'a 32 sites ; repli historique exact au-dela.
mhgp11_add_unit(mhgp11_catalogue_pair_graph SOURCES small_pair_graph_test.cpp
                GROUPS masks leaf_order contacts_obtuse equivalence mixed_fallback live_rows frontier_identity LABELS fast TIMEOUT 120)
mhgp11_add_unit(mhgp11_catalogue_pair_graph_fault SOURCES small_pair_graph_fault.cpp
                GROUPS memory all_slots pair_allocation starvation LABELS fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_catalogue_pair_graph_model 0 small_pair_graph_model.py
                    LABELS oracle fast TIMEOUT 60)

mhgp11_python_gate(mhgp11_catalogue_support_contact_model 0 support_contact_model.py
                    LABELS oracle fast TIMEOUT 30
                    LINE "support_contact_model_verdict conforme presentations504 checks2772 contacts5208 corruptions1875 native0")

# Filet de securite du catalogue a l'echelle (docs/CATALOGUE.md, juge d'Euler a K+2 et restriction J1) : sonde
# bench/catalogue_euler.cpp. Euler J3 aux ordres 1..min(K, n) sur Cat_{K+2}, restriction J1 cle par cle de Cat_K.
# Juge necessaire, jamais un certificat : ses limites sont gravees par mhgp11_catalogue_euler_limits.
add_executable(mhgp11_catalogue_euler ${PROJECT_SOURCE_DIR}/bench/catalogue_euler.cpp)
target_link_libraries(mhgp11_catalogue_euler PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_catalogue_euler_oracle 0 euler_oracle.py $<TARGET_FILE:mhgp11_catalogue_euler>
                    ${MHGP11_COORD_BITS} ${PROJECT_BINARY_DIR}
                    LINE "euler_oracle_ok controles=7270" LABELS oracle fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_catalogue_euler_limits 0 euler_limits.py $<TARGET_FILE:mhgp11_catalogue_euler>
                    ${PROJECT_BINARY_DIR} LINE "euler_limits_ok controles=139" LABELS oracle fast TIMEOUT 120)
# Echelle : familles uniform_u18_n8000/16000/32000 des entrees de banc du catalogue (schema
# mhgp11.catalogue_benchmark_inputs.v1 : random.Random(20261002).getrandbits(18), x puis y puis z), regenerees par la
# sonde ; l'empreinte entree= est celle des fichiers epingles par sha256. K = 5, donc Cat_5 et Cat_7, voie production
# a trois fils. Comptes graves par la ligne de verdict.
foreach(case "8000;597998;1301414;0;3be1324202d28360" "16000;1233046;2698867;0;3469c29b4c34b7e3"
             "32000;2536732;5578606;1;aea3dec129cef911")
  list(GET case 0 n)
  list(GET case 1 small)
  list(GET case 2 large)
  list(GET case 3 extended)
  list(GET case 4 fnv)
  mhgp11_expect_code(mhgp11_catalogue_euler_scale${n} 0 mhgp11_catalogue_euler --uniform18=${n},20261002 --k=5
                     --workers=3 --production --min-balls=${large} --min-orders=5 --min-compared=${small}
                     LINE "catalogue_euler_verdict conforme k=5 n=${n} ordres=5 boules_k=${small} boules_k2=${large} etendues=${extended} coquille_max=4 entree=${fnv}"
                     LABELS scale${n} TIMEOUT 1800)
endforeach()
# Trames LiDAR entieres sans sol (MHGP11_DATA_DIR, jamais copiees dans le depot), K = 5 (Cat_5, Cat_7) et K = 10
# (Cat_10, Cat_12 ; label long, pres de 4 Gio de memoire). Les nombres de boules et de coquilles etendues graves sont
# ceux du catalogue v10 sur les memes fichiers (audit L01 du 2 octobre 2026, catalogue_lidar_comptes.txt, sha256
# 79ff6a6570d5185688cb575d6d182bbcbddc3b152af20e87103fb651bd65517b) : egalite exacte, pas une tolerance.
foreach(case "ng00;39885;5;1306696;2565656;320;975c390e5912fabe;"
             "ng01;35551;5;1095926;2104698;204;6b918ef47e9ae56e;"
             "ng02;45845;5;1407885;2675990;865;6e11fa8bc5ee6432;"
             "ng00;39885;10;5512670;8314472;529;975c390e5912fabe;long"
             "ng01;35551;10;4383302;6492748;341;6b918ef47e9ae56e;long"
             "ng02;45845;10;5483320;8025829;1559;6e11fa8bc5ee6432;long")
  list(GET case 0 frame)
  list(GET case 1 n)
  list(GET case 2 k)
  list(GET case 3 small)
  list(GET case 4 large)
  list(GET case 5 extended)
  list(GET case 6 fnv)
  list(GET case 7 extra_label)
  mhgp11_expect_code(mhgp11_catalogue_euler_lidar_${frame}_k${k} 0 mhgp11_catalogue_euler --data=lidar_${frame}
                     --k=${k} --workers=3 --production --min-balls=${large} --min-orders=${k} --min-compared=${small}
                     --min-extended=${extended}
                     LINE "catalogue_euler_verdict conforme k=${k} n=${n} ordres=${k} boules_k=${small} boules_k2=${large} etendues=${extended} coquille_max=5 entree=${fnv}"
                     LABELS lidar ${extra_label} TIMEOUT 1800)
endforeach()
