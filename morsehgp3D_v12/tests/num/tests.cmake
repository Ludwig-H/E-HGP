# Portes exactes : la meme batterie s'applique au profil compile (21, 24 ou 32 bits).
mhgp12_add_unit(mhgp12_num_unit SOURCES integer_test.cpp geometry_test.cpp power_test.cpp candidate_test.cpp q3_candidate_test.cpp bounds_test.cpp
                                     center_region_test.cpp centers_test.cpp distance_test.cpp checked_power_test.cpp power_certificate_test.cpp
                                     orientation_certificate_test.cpp
                GROUPS wide budgets levels domain geometry extremes power_paths candidate q3_candidate bounds
                       region_domain region_pair region_line region_cubic_width centers distance checked_limits checked_public
                       certificate_limits certificate_public certificate_owners
                       orientation_limits orientation_public orientation_owners LABELS fast)
# Repere local, paliers et garde (docs/CONTRAT_NUMERIQUE.md, paragraphe 7) : etendues certifiees, portes de palier
# (temoin a s* et a s*+1, compteur de voies), boules certifiees, pave, certificats lies a leur domaine.
mhgp12_add_unit(mhgp12_num_local SOURCES frame_test.cpp lanes_test.cpp translation_test.cpp guard_test.cpp
                GROUPS frame_extents frame_refusals frame_tiers lane_side lane_orientation lane_midpoint lane_distance
                       lane_reservoir lane_center lane_levels lane_centers_order translation certify guard_sites
                       guard_boxes guard_certificate
                LABELS fast TIMEOUT 120)
# Bornes de census sur sites entiers (levier V3) : enumeration exacte, monotonie contre la borne continue.
mhgp12_add_unit(mhgp12_num_lattice SOURCES lattice_bounds_test.cpp GROUPS lattice_fixtures lattice_random
                LABELS fast TIMEOUT 120)
add_executable(mhgp12_num_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp12_num_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp12_num_probe>
                    LABELS oracle fast)
add_executable(mhgp12_num_bounds_probe ${CMAKE_CURRENT_LIST_DIR}/bounds_probe.cpp)
target_link_libraries(mhgp12_num_bounds_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_bounds_fraction 0 bounds_oracle.py $<TARGET_FILE:mhgp12_num_bounds_probe>
                    LABELS oracle fast)
mhgp12_python_gate(mhgp12_num_bounds_model 0 bounds_oracle.py --selftest LABELS oracle fast)
add_executable(mhgp12_num_center_region_probe ${CMAKE_CURRENT_LIST_DIR}/center_region_probe.cpp)
target_link_libraries(mhgp12_num_center_region_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_center_region_fraction 0 center_region_oracle.py
                    $<TARGET_FILE:mhgp12_num_center_region_probe> LABELS oracle fast)
mhgp12_python_gate(mhgp12_num_center_region_model 0 center_region_oracle.py --selftest LABELS oracle fast)
mhgp12_expect_compile_failure(mhgp12_num_budget_refusal SOURCE refusal_probe.cpp
                              TOKEN num_budget_invalide OPTIONS -DMHGP12_NUM_NEGATIVE=1 LABELS unit fast)

add_executable(mhgp12_num_centers_probe ${CMAKE_CURRENT_LIST_DIR}/centers_probe.cpp)
target_link_libraries(mhgp12_num_centers_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_centers_fraction 0 centers_oracle.py $<TARGET_FILE:mhgp12_num_centers_probe>
                    LABELS oracle fast)
mhgp12_python_gate(mhgp12_num_centers_model 0 centers_oracle.py --selftest LABELS oracle fast)

add_executable(mhgp12_num_distance_probe ${CMAKE_CURRENT_LIST_DIR}/distance_probe.cpp)
target_link_libraries(mhgp12_num_distance_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_distance_fraction 0 distance_oracle.py $<TARGET_FILE:mhgp12_num_distance_probe>
                    LABELS oracle fast TIMEOUT 60)
mhgp12_python_gate(mhgp12_num_distance_model 0 distance_oracle.py --selftest LABELS oracle fast TIMEOUT 30)

add_executable(mhgp12_num_checked_power_probe ${CMAKE_CURRENT_LIST_DIR}/checked_power_probe.cpp)
target_link_libraries(mhgp12_num_checked_power_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_checked_power_fraction 0 checked_power_oracle.py
                    $<TARGET_FILE:mhgp12_num_checked_power_probe> LABELS oracle fast TIMEOUT 60)
mhgp12_python_gate(mhgp12_num_checked_power_model 0 checked_power_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)

add_executable(mhgp12_num_power_certificate_probe ${CMAKE_CURRENT_LIST_DIR}/power_certificate_probe.cpp)
target_link_libraries(mhgp12_num_power_certificate_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_power_certificate_fraction 0 power_certificate_oracle.py
                    $<TARGET_FILE:mhgp12_num_power_certificate_probe> LABELS oracle fast TIMEOUT 60)
mhgp12_python_gate(mhgp12_num_power_certificate_model 0 power_certificate_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)
add_executable(mhgp12_num_orientation_certificate_probe ${CMAKE_CURRENT_LIST_DIR}/orientation_certificate_probe.cpp)
target_link_libraries(mhgp12_num_orientation_certificate_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_orientation_certificate_fraction 0 orientation_certificate_oracle.py
                    $<TARGET_FILE:mhgp12_num_orientation_certificate_probe> LABELS oracle fast TIMEOUT 90)
mhgp12_python_gate(mhgp12_num_orientation_certificate_model 0 orientation_certificate_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30)

# Propriete q4 de presentation, distincte du predicat generique sur un autre tetraedre.
mhgp12_add_unit(mhgp12_num_q4_presentation SOURCES q4_presentation_test.cpp
                GROUPS presentation boundaries foreign_and_owners LABELS fast)
add_executable(mhgp12_num_q4_presentation_probe ${CMAKE_CURRENT_LIST_DIR}/q4_presentation_probe.cpp)
target_link_libraries(mhgp12_num_q4_presentation_probe PRIVATE mhgp12)
# Comptes graves aux profils 21 et 24 (la v11 gravait aussi ceux de son profil 18 : 491 et 14213).
set(q4_presentation_requests 635)
set(q4_presentation_checks 19829)
mhgp12_python_gate(mhgp12_num_q4_presentation_fraction 0 q4_presentation_oracle.py
                    $<TARGET_FILE:mhgp12_num_q4_presentation_probe> LABELS oracle fast TIMEOUT 120
                    LINE "q4_presentation_fraction_verdict conforme bits${MHGP12_COORD_BITS} requests${q4_presentation_requests} checks${q4_presentation_checks} malformed2")
mhgp12_python_gate(mhgp12_num_q4_presentation_model 0 q4_presentation_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 60
                    LINE "q4_presentation_model_verdict conforme requests1270 checks39662 corruptions76 native0")
mhgp12_python_gate(mhgp12_num_q4_presentation_process 0 q4_presentation_process_test.py
                    LABELS oracle fast TIMEOUT 30
                    LINE "q4_presentation_process_verdict conforme scenarios13 checks58 native0")

# Classification q3 avant materialisation ; oracle Gram independant.
mhgp12_add_unit(mhgp12_num_triangle_kind SOURCES triangle_kind_test.cpp
                GROUPS kinds extremes LABELS fast)
add_executable(mhgp12_num_triangle_kind_probe ${CMAKE_CURRENT_LIST_DIR}/triangle_kind_probe.cpp)
target_link_libraries(mhgp12_num_triangle_kind_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_triangle_kind_fraction 0 triangle_kind_oracle.py
                    $<TARGET_FILE:mhgp12_num_triangle_kind_probe> LABELS oracle fast TIMEOUT 60
                    LINE "triangle_kind_fraction_verdict conforme bits${MHGP12_COORD_BITS} requests284 checks568 malformed2")
mhgp12_python_gate(mhgp12_num_triangle_kind_model 0 triangle_kind_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 30
                    LINE "triangle_kind_model_verdict conforme requests568 checks1148 corruptions48 processes5 native0")

# Tranche S8 : entiers a longueur utile, table des racines, sommes de radicaux (specification paragraphes 7.8 et 8.7).
mhgp12_add_unit(mhgp12_num_s8_unit SOURCES big_test.cpp
                GROUPS capacity aliasing division rational_form square_signature radical_budget roots_limits LABELS fast)
add_executable(mhgp12_num_big_probe ${CMAKE_CURRENT_LIST_DIR}/big_probe.cpp)
target_link_libraries(mhgp12_num_big_probe PRIVATE mhgp12)
mhgp12_python_gate(mhgp12_num_big 0 big_gate.py $<TARGET_FILE:mhgp12_num_big_probe> LABELS oracle fast TIMEOUT 300
                    LINE "num_big_verdict conforme operations=23445 refus_capacite=1516 knuth=2500")
add_executable(mhgp12_num_radical_probe ${CMAKE_CURRENT_LIST_DIR}/radical_probe.cpp)
target_link_libraries(mhgp12_num_radical_probe PRIVATE mhgp12)
# La copie Python des decisions exactes (radical_port.py) est jugee contre l'empreinte epinglee de sa source, le banc
# bench/points_radius.py de la v11 (ac081a06f), que la v12 ne porte pas : la porte ne prend plus son chemin.
mhgp12_python_gate(mhgp12_num_radical 0 radical_gate.py $<TARGET_FILE:mhgp12_num_radical_probe>
                    LABELS oracle fast TIMEOUT 300
                    LINE "num_radical_verdict conforme temoins=19 decisions=5900 egalites=1981 raffinees=1526 refus=0")
add_executable(mhgp12_num_roots_probe ${CMAKE_CURRENT_LIST_DIR}/roots_probe.cpp)
target_link_libraries(mhgp12_num_roots_probe PRIVATE mhgp12)
# Ligne gravee au profil 21 ; au profil 24 les planchers seuls s'appliquent (ligne a graver sur G4).
set(num_roots_line)
if(MHGP12_COORD_BITS EQUAL 21)
  set(num_roots_line LINE "num_roots_verdict conforme bits=21 racines=3314 sommes=4062 replis=692 egalites=512 raffinees=180")
endif()
mhgp12_python_gate(mhgp12_num_roots 0 roots_gate.py $<TARGET_FILE:mhgp12_num_roots_probe>
                    LABELS oracle fast TIMEOUT 300 ${num_roots_line})
# Le cout de la table des racines sur le catalogue d'ordre K (bench/roots_cost.cpp de la v11, portes lidar, scale* et
# long) n'est pas porte : il exige le module catalogue, reecrit par la v12, et ses donnees uniformes etaient nommees au
# profil 18 (uniform_u18_n*). Il reviendra avec le catalogue.
