# Portes exactes : la meme batterie s'applique au profil compile (18, 21 ou 24 bits).
mhgp11_add_unit(mhgp11_num_unit SOURCES integer_test.cpp geometry_test.cpp power_test.cpp candidate_test.cpp q3_candidate_test.cpp bounds_test.cpp
                                     center_region_test.cpp centers_test.cpp distance_test.cpp checked_power_test.cpp power_certificate_test.cpp
                                     orientation_certificate_test.cpp
                GROUPS wide budgets levels domain geometry extremes power_paths candidate q3_candidate bounds
                       region_domain region_pair region_line region_cubic_width centers distance checked_limits checked_public
                       certificate_limits certificate_public certificate_owners
                       orientation_limits orientation_public orientation_owners LABELS fast)
# Bornes de census sur sites entiers (levier V3) : enumeration exacte, monotonie contre la borne continue.
mhgp11_add_unit(mhgp11_num_lattice SOURCES lattice_bounds_test.cpp GROUPS lattice_fixtures lattice_random
                LABELS fast TIMEOUT 120)
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

add_executable(mhgp11_num_distance_probe ${CMAKE_CURRENT_LIST_DIR}/distance_probe.cpp)
target_link_libraries(mhgp11_num_distance_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_distance_fraction 0 distance_oracle.py $<TARGET_FILE:mhgp11_num_distance_probe>
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_num_distance_model 0 distance_oracle.py --selftest LABELS oracle fast TIMEOUT 30)

add_executable(mhgp11_num_checked_power_probe ${CMAKE_CURRENT_LIST_DIR}/checked_power_probe.cpp)
target_link_libraries(mhgp11_num_checked_power_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_checked_power_fraction 0 checked_power_oracle.py
                    $<TARGET_FILE:mhgp11_num_checked_power_probe> LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_num_checked_power_model 0 checked_power_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)

add_executable(mhgp11_num_power_certificate_probe ${CMAKE_CURRENT_LIST_DIR}/power_certificate_probe.cpp)
target_link_libraries(mhgp11_num_power_certificate_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_power_certificate_fraction 0 power_certificate_oracle.py
                    $<TARGET_FILE:mhgp11_num_power_certificate_probe> LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_num_power_certificate_model 0 power_certificate_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)
add_executable(mhgp11_num_orientation_certificate_probe ${CMAKE_CURRENT_LIST_DIR}/orientation_certificate_probe.cpp)
target_link_libraries(mhgp11_num_orientation_certificate_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_orientation_certificate_fraction 0 orientation_certificate_oracle.py
                    $<TARGET_FILE:mhgp11_num_orientation_certificate_probe> LABELS oracle fast TIMEOUT 90)
mhgp11_python_gate(mhgp11_num_orientation_certificate_model 0 orientation_certificate_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)

# Propriete q4 de presentation, distincte du predicat generique sur un autre tetraedre.
mhgp11_add_unit(mhgp11_num_q4_presentation SOURCES q4_presentation_test.cpp
                GROUPS presentation boundaries foreign_and_owners LABELS fast)
add_executable(mhgp11_num_q4_presentation_probe ${CMAKE_CURRENT_LIST_DIR}/q4_presentation_probe.cpp)
target_link_libraries(mhgp11_num_q4_presentation_probe PRIVATE mhgp11)
if(MHGP11_COORD_BITS EQUAL 18)
  set(q4_presentation_requests 491)
  set(q4_presentation_checks 14213)
else()
  set(q4_presentation_requests 635)
  set(q4_presentation_checks 19829)
endif()
mhgp11_python_gate(mhgp11_num_q4_presentation_fraction 0 q4_presentation_oracle.py
                    $<TARGET_FILE:mhgp11_num_q4_presentation_probe> LABELS oracle fast TIMEOUT 120
                    LINE "q4_presentation_fraction_verdict conforme bits${MHGP11_COORD_BITS} requests${q4_presentation_requests} checks${q4_presentation_checks} malformed2")
mhgp11_python_gate(mhgp11_num_q4_presentation_model 0 q4_presentation_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 60
                    LINE "q4_presentation_model_verdict conforme requests1761 checks53877 corruptions114 native0")
mhgp11_python_gate(mhgp11_num_q4_presentation_process 0 q4_presentation_process_test.py
                    LABELS oracle fast TIMEOUT 30
                    LINE "q4_presentation_process_verdict conforme scenarios13 checks58 native0")

# Classification q3 avant materialisation ; oracle Gram independant.
mhgp11_add_unit(mhgp11_num_triangle_kind SOURCES triangle_kind_test.cpp
                GROUPS kinds extremes LABELS fast)
add_executable(mhgp11_num_triangle_kind_probe ${CMAKE_CURRENT_LIST_DIR}/triangle_kind_probe.cpp)
target_link_libraries(mhgp11_num_triangle_kind_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_triangle_kind_fraction 0 triangle_kind_oracle.py
                    $<TARGET_FILE:mhgp11_num_triangle_kind_probe> LABELS oracle fast TIMEOUT 60
                    LINE "triangle_kind_fraction_verdict conforme bits${MHGP11_COORD_BITS} requests284 checks568 malformed2")
mhgp11_python_gate(mhgp11_num_triangle_kind_model 0 triangle_kind_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30
                    LINE "triangle_kind_model_verdict conforme requests852 checks1717 corruptions72 processes5 native0")

# Tranche S8 : entiers a longueur utile, table des racines, sommes de radicaux (specification paragraphes 7.8 et 8.7).
mhgp11_add_unit(mhgp11_num_s8_unit SOURCES big_test.cpp
                GROUPS capacity aliasing division rational_form square_signature radical_budget roots_limits LABELS fast)
add_executable(mhgp11_num_big_probe ${CMAKE_CURRENT_LIST_DIR}/big_probe.cpp)
target_link_libraries(mhgp11_num_big_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_big 0 big_gate.py $<TARGET_FILE:mhgp11_num_big_probe> LABELS oracle fast TIMEOUT 300
                    LINE "num_big_verdict conforme operations=23445 refus_capacite=1516 knuth=2500")
add_executable(mhgp11_num_radical_probe ${CMAKE_CURRENT_LIST_DIR}/radical_probe.cpp)
target_link_libraries(mhgp11_num_radical_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_num_radical 0 radical_gate.py $<TARGET_FILE:mhgp11_num_radical_probe>
                    ${PROJECT_SOURCE_DIR}/bench/points_radius.py LABELS oracle fast TIMEOUT 300
                    LINE "num_radical_verdict conforme temoins=19 decisions=5900 egalites=1981 raffinees=1526 refus=0")
add_executable(mhgp11_num_roots_probe ${CMAKE_CURRENT_LIST_DIR}/roots_probe.cpp)
target_link_libraries(mhgp11_num_roots_probe PRIVATE mhgp11)
# Ligne gravee au profil 21 ; aux profils 18 et 24 les planchers seuls s'appliquent (lignes a graver sur G4).
set(num_roots_line)
if(MHGP11_COORD_BITS EQUAL 21)
  set(num_roots_line LINE "num_roots_verdict conforme bits=21 racines=3314 sommes=4062 replis=692 egalites=512 raffinees=180")
endif()
mhgp11_python_gate(mhgp11_num_roots 0 roots_gate.py $<TARGET_FILE:mhgp11_num_roots_probe>
                    ${PROJECT_SOURCE_DIR}/bench/points_radius.py LABELS oracle fast TIMEOUT 300 ${num_roots_line})
# Cout de la table des racines sur le catalogue d'ordre K (mesure, hors produit) : construit si le catalogue l'est.
# Portes lidar (MHGP11_DATA_DIR : familles uniformes 8000, 16000, 32000 et trames sans sol), juge d'echantillon et
# identite des voies sequentielle et parallele ; temps et octets lus dans la sortie JSON (recu G4).
if("catalogue" IN_LIST MHGP11_LIBRARY_MODULES AND "sched" IN_LIST MHGP11_LIBRARY_MODULES)
  add_executable(mhgp11_num_roots_cost ${PROJECT_SOURCE_DIR}/bench/roots_cost.cpp)
  target_include_directories(mhgp11_num_roots_cost PRIVATE ${PROJECT_SOURCE_DIR}/bench)
  target_link_libraries(mhgp11_num_roots_cost PRIVATE mhgp11)
  foreach(case "uniform_u18_n8000;5;scale8000" "uniform_u18_n16000;5;scale16000" "uniform_u18_n32000;5;scale32000"
               "lidar_ng00;5;lidar" "lidar_ng01;5;lidar" "lidar_ng02;5;lidar" "lidar_ng00;10;long")
    list(GET case 0 data)
    list(GET case 1 k)
    list(GET case 2 extra)
    mhgp11_expect_code(mhgp11_num_roots_cost_${data}_k${k} 0 mhgp11_num_roots_cost --data=${data} --k=${k}
                       --workers=4 --min-levels=100000 LINE "roots_cost_verdict conforme k=${k}"
                       LABELS lidar ${extra} TIMEOUT 1800)
  endforeach()
endif()
