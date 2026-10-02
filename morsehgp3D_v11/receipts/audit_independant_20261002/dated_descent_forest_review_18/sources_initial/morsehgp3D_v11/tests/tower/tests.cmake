# MEB exacte bornee et raccord au census global : oracle Gram, proprietes et refus transactionnels.
mhgp11_add_unit(mhgp11_tower_unit SOURCES unit.cpp
                GROUPS geometry local_support refusals ownership wrapper capacity shell concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_tower_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_tower_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_judge 0 model_test.py LABELS oracle fast TIMEOUT 60)
add_executable(mhgp11_meb_bench ${PROJECT_SOURCE_DIR}/bench/meb_probe.cpp)
target_link_libraries(mhgp11_meb_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_bench_collector 0 ${PROJECT_SOURCE_DIR}/bench/meb_collector_test.py
                    LINE "meb_collector_verdict conforme attempts11 corruptions32 provenance20 schedules6 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_tower_bench_io 0 ${PROJECT_SOURCE_DIR}/bench/meb_io_test.py
                    $<TARGET_FILE:mhgp11_meb_bench> ${MHGP11_COORD_BITS}
                    LINE "meb_io_verdict conforme attempts8 queries144 refusals5"
                    LABELS fast TIMEOUT 90)
# Domaine ferme pour le futur FULL ; aucune descente ni foret qualifiee par ces portes.
mhgp11_add_unit(mhgp11_tower_domain SOURCES domain.cpp
                GROUPS context lookup global_support ownership refusals capacity concurrency permutation LABELS fast)
mhgp11_add_unit(mhgp11_tower_domain_fault SOURCES domain_fault.cpp GROUPS starvation LABELS fast)
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
