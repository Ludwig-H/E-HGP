# Portes exactes : la meme batterie s'applique au profil compile (18, 21 ou 24 bits).
mhgp11_add_unit(mhgp11_num_unit SOURCES integer_test.cpp geometry_test.cpp power_test.cpp candidate_test.cpp bounds_test.cpp
                GROUPS wide budgets levels domain geometry extremes power_paths candidate bounds LABELS fast)
add_executable(mhgp11_num_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_num_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_num_probe>
                    LABELS oracle fast)
add_executable(mhgp11_num_bounds_probe ${CMAKE_CURRENT_LIST_DIR}/bounds_probe.cpp)
target_link_libraries(mhgp11_num_bounds_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_bounds_fraction 0 bounds_oracle.py $<TARGET_FILE:mhgp11_num_bounds_probe>
                    LABELS oracle fast)
mhgp11_python_gate(mhgp11_num_bounds_model 0 bounds_oracle.py --selftest LABELS oracle fast)
mhgp11_expect_compile_failure(mhgp11_num_budget_refusal SOURCE refusal_probe.cpp
                              TOKEN num_budget_invalide OPTIONS -DMHGP11_NUM_NEGATIVE=1 LABELS unit fast)
