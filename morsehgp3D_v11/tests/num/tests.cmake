# Portes exactes : la meme batterie s'applique au profil compile (18, 21 ou 24 bits).
mhgp11_add_unit(mhgp11_num_unit SOURCES integer_test.cpp geometry_test.cpp power_test.cpp candidate_test.cpp bounds_test.cpp
                                     center_region_test.cpp centers_test.cpp
                GROUPS wide budgets levels domain geometry extremes power_paths candidate bounds
                       region_domain region_pair region_line region_cubic_width centers LABELS fast)
add_executable(mhgp11_num_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_num_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_num_probe>
                    LABELS oracle fast)
add_executable(mhgp11_num_bounds_probe ${CMAKE_CURRENT_LIST_DIR}/bounds_probe.cpp)
target_link_libraries(mhgp11_num_bounds_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_bounds_fraction 0 bounds_oracle.py $<TARGET_FILE:mhgp11_num_bounds_probe>
                    LABELS oracle fast)
mhgp11_python_gate(mhgp11_num_bounds_model 0 bounds_oracle.py --selftest LABELS oracle fast)
add_executable(mhgp11_num_center_region_probe ${CMAKE_CURRENT_LIST_DIR}/center_region_probe.cpp)
target_link_libraries(mhgp11_num_center_region_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_center_region_fraction 0 center_region_oracle.py
                    $<TARGET_FILE:mhgp11_num_center_region_probe> LABELS oracle fast)
mhgp11_python_gate(mhgp11_num_center_region_model 0 center_region_oracle.py --selftest LABELS oracle fast)
mhgp11_expect_compile_failure(mhgp11_num_budget_refusal SOURCE refusal_probe.cpp
                              TOKEN num_budget_invalide OPTIONS -DMHGP11_NUM_NEGATIVE=1 LABELS unit fast)

add_executable(mhgp11_num_centers_probe ${CMAKE_CURRENT_LIST_DIR}/centers_probe.cpp)
target_link_libraries(mhgp11_num_centers_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_centers_fraction 0 centers_oracle.py $<TARGET_FILE:mhgp11_num_centers_probe>
                    LABELS oracle fast)
mhgp11_python_gate(mhgp11_num_centers_model 0 centers_oracle.py --selftest LABELS oracle fast)
