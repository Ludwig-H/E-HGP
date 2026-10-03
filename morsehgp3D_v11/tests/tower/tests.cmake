# MEB exacte bornee et raccord au census global : oracle Gram, proprietes et refus transactionnels.
mhgp11_add_unit(mhgp11_tower_unit SOURCES unit.cpp
                GROUPS geometry local_support refusals ownership wrapper capacity shell concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
mhgp11_add_unit(mhgp11_tower_diameter SOURCES diameter.cpp GROUPS canonical_pairs fallback LABELS fast)
add_executable(mhgp11_tower_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_tower_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_tower_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_judge 0 model_test.py LABELS oracle fast TIMEOUT 60)
add_executable(mhgp11_meb_bench ${PROJECT_SOURCE_DIR}/bench/meb_probe.cpp)
target_link_libraries(mhgp11_meb_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_bench_collector 0 ${PROJECT_SOURCE_DIR}/bench/meb_collector_test.py
                    LINE "meb_collector_verdict conforme attempts11 corruptions36 provenance20 schedules6 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_tower_bench_io 0 ${PROJECT_SOURCE_DIR}/bench/meb_io_test.py
                    $<TARGET_FILE:mhgp11_meb_bench> ${MHGP11_COORD_BITS}
                    LINE "meb_io_verdict conforme attempts8 queries144 refusals5"
                    LABELS fast TIMEOUT 90)
# Domaine ferme pour le futur FULL ; aucune descente ni foret qualifiee par ces portes.
mhgp11_add_unit(mhgp11_tower_domain SOURCES domain.cpp
                GROUPS context lookup global_support ownership refusals capacity concurrency permutation LABELS fast)
mhgp11_add_unit(mhgp11_tower_domain_fault SOURCES domain_fault.cpp GROUPS starvation LABELS fast)
mhgp11_add_unit(mhgp11_tower_domain_parallel SOURCES domain_parallel.cpp
                GROUPS equivalence refusals lookup_refusal LABELS fast)
mhgp11_add_unit(mhgp11_tower_cells SOURCES cells.cpp
                GROUPS regular extended capacity ownership refusals extreme concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_cells_fault SOURCES cells_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_cells_probe ${CMAKE_CURRENT_LIST_DIR}/cells_probe.cpp)
target_link_libraries(mhgp11_tower_cells_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_cells_fraction 0 cells_oracle.py $<TARGET_FILE:mhgp11_tower_cells_probe>
                    LABELS oracle fast TIMEOUT 240)
mhgp11_python_gate(mhgp11_tower_cells_model 0 cells_model_test.py LABELS oracle fast TIMEOUT 180)
mhgp11_add_unit(mhgp11_tower_locate SOURCES locate_test.cpp
                GROUPS lookup global_identity saturated outside_catalogue LABELS fast)
mhgp11_add_unit(mhgp11_tower_descent SOURCES descent_test.cpp
                GROUPS interiors outside traces boundaries refusals capacity ownership concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_descent_fault SOURCES descent_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_descent_probe ${CMAKE_CURRENT_LIST_DIR}/descent_probe.cpp)
target_link_libraries(mhgp11_tower_descent_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_descent_fraction 0 descent_oracle.py $<TARGET_FILE:mhgp11_tower_descent_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_descent_model 0 descent_model_test.py LABELS oracle fast TIMEOUT 60)
mhgp11_add_unit(mhgp11_tower_forest SOURCES forest_test.cpp
                GROUPS plateau multigroup verticals canonical ownership refusals concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_forest_fault SOURCES forest_fault.cpp GROUPS starvation direct_timings LABELS fast)
add_executable(mhgp11_tower_forest_probe ${CMAKE_CURRENT_LIST_DIR}/forest_probe.cpp)
mhgp11_add_unit(mhgp11_tower_forest_sweep SOURCES forest_sweep_test.cpp
                GROUPS reference depth_and_memory timings concurrency LABELS fast)
target_link_libraries(mhgp11_tower_forest_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_forest_fraction 0 forest_oracle.py $<TARGET_FILE:mhgp11_tower_forest_probe>
                    LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_model 0 forest_model_test.py LABELS oracle fast TIMEOUT 90)
mhgp11_add_unit(mhgp11_tower_classification SOURCES classification_test.cpp
                GROUPS analytic prefix refusals ownership_concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_classification_fault SOURCES classification_fault.cpp GROUPS no_allocation LABELS fast)
add_executable(mhgp11_tower_classification_probe ${CMAKE_CURRENT_LIST_DIR}/classification_probe.cpp)
target_link_libraries(mhgp11_tower_classification_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_classification_fraction 0 classification_oracle.py
                    $<TARGET_FILE:mhgp11_tower_classification_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_classification_oracle_model 0 classification_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_classification_model 0 classification_model.py LABELS oracle fast TIMEOUT 60)
add_executable(mhgp11_full_bench ${PROJECT_SOURCE_DIR}/bench/full_probe.cpp)
target_link_libraries(mhgp11_full_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_full_bench_io 0 full_bench_io.py $<TARGET_FILE:mhgp11_full_bench> ${MHGP11_COORD_BITS}
                    LINE "full_io_verdict conforme attempts400 successes387 refusals13" LABELS fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_full_bench_semantic 0 full_bench_semantic_test.py
                    LINE "full_semantic_verdict conforme positives21 corruptions48 checks168 native0"
                    LABELS fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_campaign 0 full_campaign_test.py
                    LINE "full_campaign_verdict conforme attempts462 schedules391 interrupted1 checks36704 native0"
                    LABELS fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_full_v10_model 0 full_v10_model.py
                    LINE "full_v10_model_verdict conforme positives42 corruptions19 native0"
                    LABELS fast TIMEOUT 60)

# Memo avant MEB : oracle Definition conserve, nouvelles portes natives sans compilation locale.
mhgp11_add_unit(mhgp11_tower_memo SOURCES memo.cpp
                GROUPS dates collisions_refusals differential capacity LABELS fast)
mhgp11_add_unit(mhgp11_tower_memo_full SOURCES memo_full.cpp
                GROUPS equivalence refusals concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_memo_fault SOURCES memo_fault.cpp GROUPS starvation LABELS fast)
mhgp11_python_gate(mhgp11_tower_forest_memo_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_probe> --memo 64 LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_full_memo_collector 0 full_memo_collector_test.py
                    LINE "full_memo_collector_verdict conforme schedules7 interruptions1 inventories2 checks167 native0"
                    LABELS fast TIMEOUT 60)

mhgp11_python_gate(mhgp11_tower_full_reuse_collector 0 full_reuse_collector_test.py
                    LINE "full_reuse_collector_verdict conforme attempts10 decodes6 interruptions1 checks23 native0"
                    LABELS fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_parallel_collector 0 full_parallel_collector_test.py
                    LINE "full_parallel_collector_verdict conforme attempts119 corruptions39 decodes37 schedules7 interruptions2 comparisons12 checks825 native0"
                    LABELS fast TIMEOUT 60)

# Descentes regulieres par lanes logiques ; Definition exhaustive reste l'autorite geometrique.
mhgp11_add_unit(mhgp11_tower_forest_parallel SOURCES forest_parallel_test.cpp
                GROUPS equivalence plateaus extended_merge mixed_plateau lanes48 refusals pool_busy LABELS fast)
mhgp11_add_unit(mhgp11_tower_forest_parallel_fault SOURCES forest_parallel_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_forest_parallel_probe ${CMAKE_CURRENT_LIST_DIR}/forest_parallel_probe.cpp)
target_link_libraries(mhgp11_tower_forest_parallel_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_forest_parallel_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_parallel_memo_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> --memo 64 LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_parallel_model 0 forest_parallel_model.py
                    LINE "forest_parallel_model_verdict conforme facts27 native0" LABELS oracle fast TIMEOUT 30)

# Raccord options catalogue FULL : faux enfants, artefacts Definition reels, pas de natif local.
mhgp11_python_gate(mhgp11_tower_full_catalogue_collector 0 full_catalogue_collector_test.py
                    LINE "full_catalogue_collector_verdict conforme attempts147 corruptions69 decodes18 schedules7 interruptions2 comparisons9 checks938 native0"
                    LABELS fast TIMEOUT 60)

# Naissances verticales independantes du DSU, puis balayage ferme inchange.
mhgp11_add_unit(mhgp11_tower_vertical_parallel SOURCES forest_vertical_parallel_test.cpp
                GROUPS equivalence closed_dates lanes48 refusals pool_busy LABELS fast)
mhgp11_add_unit(mhgp11_tower_vertical_parallel_fault SOURCES forest_vertical_parallel_fault.cpp
                GROUPS starvation vertical_census_failure LABELS fast)
mhgp11_python_gate(mhgp11_tower_vertical_parallel_fraction 0 forest_vertical_parallel_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_vertical_parallel_model 0 forest_vertical_parallel_model.py
                    LINE "vertical_parallel_model_verdict conforme cases324 checks3618 corruptions20 equal_dates1 strict_dates29 native0"
                    LABELS oracle fast TIMEOUT 30)

# Rangs deja ordonnes : seules les cohortes avec plusieurs centres construisent des Sphere.
mhgp11_add_unit(mhgp11_tower_birth_runs SOURCES forest_birth_runs_test.cpp
                GROUPS points cohorts extended memory parallel_memo LABELS fast)
mhgp11_python_gate(mhgp11_tower_birth_runs_model 0 forest_birth_runs_model.py
                    LINE "birth_runs_model_verdict conforme cases72 checks612 facts156 corruptions300 native0"
                    LABELS oracle fast TIMEOUT 30)

# Diagnostics verticaux controles avant toute reutilisation semantique.
mhgp11_python_gate(mhgp11_tower_full_vertical_collector 0 full_vertical_collector_test.py
                    LINE "full_vertical_collector_verdict conforme attempts131 corruptions71 decodes31 schedules7 interruptions2 comparisons10 checks685 native0"
                    LABELS fast TIMEOUT 60)

# Census physique partage entre descentes successives, jamais entre callbacks simultanes.
mhgp11_add_unit(mhgp11_tower_census_reuse SOURCES census_reuse_test.cpp
                GROUPS descents memo_identity full admission LABELS fast)
mhgp11_add_unit(mhgp11_tower_census_reuse_fault SOURCES census_reuse_fault.cpp
                GROUPS allocation_free starvation LABELS fast)
mhgp11_python_gate(mhgp11_tower_census_reuse_fraction 0 descent_oracle.py
                    $<TARGET_FILE:mhgp11_tower_descent_probe> --workspace LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_census_reuse_model 0 census_reuse_model.py
                    LINE "{\"checks\": 65511, \"corruptions\": 45, \"native\": 0, \"positives\": 171, \"routing\": 3720, \"saturation_facts\": 81, \"verdict\": \"conforme\"}"
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_census_collector 0 full_census_collector_test.py
                    LINE "full_census_collector_verdict conforme attempts201 corruptions390 decodes108 schedules7 interruptions2 comparisons11 cross_route11 checks1914 native0"
                    LABELS oracle fast TIMEOUT 60)
