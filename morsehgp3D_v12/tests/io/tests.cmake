# Portes du module io. tests/io/full_march_collector.py est enregistre par tests/tower/tests.cmake (collecteur des
# recus de la tour) : il n'est pas reenregistre ici.
mhgp12_add_unit(mhgp12_io_unit SOURCES sha256_test.cpp input_test.cpp
                GROUPS sha256 sha256_voies hex input input_sizes input_unreadable input_budget LABELS fast)
mhgp12_add_unit(mhgp12_io_transaction SOURCES plan_test.cpp transaction_test.cpp
                GROUPS plan_syntax plan_parent plan_conflicts plan_inputs create_names parent_unwritable
                       commit noreplace orphan discard retract write_failure
                LABELS fast)

# Penurie de memoire injectee : operator new remplace dans cet executable seulement.
mhgp12_add_unit(mhgp12_io_fault SOURCES fault.cpp GROUPS starvation LABELS fast)

# Differentiel du SHA-256 contre hashlib (bibliotheque standard de Python).
add_executable(mhgp12_io_sha256_probe ${CMAKE_CURRENT_LIST_DIR}/sha256_probe.cpp)
target_link_libraries(mhgp12_io_sha256_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_io_sha256 0 sha256_oracle.py $<TARGET_FILE:mhgp12_io_sha256_probe> LABELS oracle fast)
