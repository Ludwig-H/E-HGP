# Census global : oracle Gram/Fraction, ownership explicite et refus transactionnels.
mhgp11_add_unit(mhgp11_index_unit SOURCES unit.cpp
                GROUPS fixtures structure ownership budget blocks lattice concurrency LABELS fast)
mhgp11_add_unit(mhgp11_index_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_index_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_index_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_index_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_index_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_index_judge 0 model_test.py LABELS oracle fast TIMEOUT 30)
add_executable(mhgp11_index_bench ${PROJECT_SOURCE_DIR}/bench/index_probe.cpp)
target_link_libraries(mhgp11_index_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_index_bench_collector 0 ${PROJECT_SOURCE_DIR}/bench/index_collector_test.py
                    LINE "index_collector_verdict conforme attempts11 corruptions17 provenance20 schedules6 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_index_bench_io 0 ${PROJECT_SOURCE_DIR}/bench/index_io_test.py
                    $<TARGET_FILE:mhgp11_index_bench> ${MHGP11_COORD_BITS}
                    LINE "index_io_verdict conforme attempts8 queries192 refusals5"
                    LABELS fast TIMEOUT 90)

# Vue empruntee : une passe exacte, capacite fixe n et callbacks synchrones exclusifs.
mhgp11_add_unit(mhgp11_index_borrowed SOURCES borrowed_test.cpp
                GROUPS fixtures lattice ownership callbacks concurrency LABELS fast)
mhgp11_add_unit(mhgp11_index_borrowed_fault SOURCES borrowed_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_index_borrowed_probe ${CMAKE_CURRENT_LIST_DIR}/borrowed_probe.cpp)
target_link_libraries(mhgp11_index_borrowed_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_index_borrowed_fraction 0 borrowed_oracle.py
                    $<TARGET_FILE:mhgp11_index_borrowed_probe> $<TARGET_FILE:mhgp11_index_probe>
                    LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_index_borrowed_model 0 borrowed_oracle.py --selftest
                    LINE "borrowed_model_verdict conforme positives3030 corruptions36 native0"
                    LABELS oracle fast TIMEOUT 60)
