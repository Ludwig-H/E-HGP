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

mhgp11_add_unit(mhgp11_catalogue_region SOURCES center_region.cpp
                GROUPS pair_region line_region obtuse_region LABELS fast)
