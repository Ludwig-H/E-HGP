# Census global : oracle Gram/Fraction, ownership explicite et refus transactionnels.
mhgp12_add_unit(mhgp12_index_unit SOURCES unit.cpp
                GROUPS fixtures structure ownership budget blocks lattice concurrency LABELS fast)
mhgp12_add_unit(mhgp12_index_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp12_index_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp12_index_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_index_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp12_index_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp12_python_gate(mhgp12_index_judge 0 model_test.py LABELS oracle fast TIMEOUT 30)
# Sonde de banc de l'index (bench/index_probe.cpp) et sa porte d'entrees-sorties. Le pilote G4 de la v11
# (bench/index_g4.py) et sa porte mhgp11_index_bench_collector ne sont pas portes : ils exigeaient les constructions
# qualifiees aux profils 18, 21 et 24 et le complement ASan 18 bits (index_asan18_matrix.json).
add_executable(mhgp12_index_bench ${PROJECT_SOURCE_DIR}/bench/index_probe.cpp)
target_link_libraries(mhgp12_index_bench PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_index_bench_io 0 ${PROJECT_SOURCE_DIR}/bench/index_io_test.py
                    $<TARGET_FILE:mhgp12_index_bench> ${MHGP12_COORD_BITS}
                    LINE "index_io_verdict conforme attempts8 queries192 refusals5"
                    LABELS fast TIMEOUT 90)

# Vue empruntee : une passe exacte, capacite fixe n et callbacks synchrones exclusifs.
mhgp12_add_unit(mhgp12_index_borrowed SOURCES borrowed_test.cpp
                GROUPS fixtures lattice ownership callbacks concurrency LABELS fast)
mhgp12_add_unit(mhgp12_index_borrowed_fault SOURCES borrowed_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp12_index_borrowed_probe ${CMAKE_CURRENT_LIST_DIR}/borrowed_probe.cpp)
target_link_libraries(mhgp12_index_borrowed_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_index_borrowed_fraction 0 borrowed_oracle.py
                    $<TARGET_FILE:mhgp12_index_borrowed_probe> $<TARGET_FILE:mhgp12_index_probe>
                    LABELS oracle fast TIMEOUT 180)
mhgp12_python_gate(mhgp12_index_borrowed_model 0 borrowed_oracle.py --selftest
                    LINE "borrowed_model_verdict conforme positives2020 corruptions24 native0"
                    LABELS oracle fast TIMEOUT 60)
