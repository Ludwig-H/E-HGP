# Portes du module core.
mhgp12_add_unit(mhgp12_core_unit SOURCES status_test.cpp buffer_test.cpp ledger_test.cpp
                GROUPS types reasons outcome macros result guarded budget budget_threads reservation buffer csr ledger
                       block_cache block_cache_limit
                LABELS fast)

# Penurie de memoire injectee : operator new remplace dans cet executable seulement.
mhgp12_add_unit(mhgp12_core_fault SOURCES alloc_fault.cpp GROUPS alloc_fault alloc_fault_cache eviction_admise refusal
                       ledger
                LABELS fast)

# Acces verifies de Result : value() et take() sur un refus terminent le processus. La porte exige l'arret anormal
# de la sonde ; le temoin value_ok montre que la sonde finit sinon par le code 0, que ces portes refuseraient.
add_executable(mhgp12_core_misuse_probe ${CMAKE_CURRENT_LIST_DIR}/misuse_probe.cpp)
target_link_libraries(mhgp12_core_misuse_probe PRIVATE mhgp12)
mhgp12_expect_code(mhgp12_core_result_value_ok 0 mhgp12_core_misuse_probe value_ok
                   LINE "valeur lue 5" LABELS unit fast)
foreach(misuse value_on_refusal const_value_on_refusal take_on_refusal)
  mhgp12_expect_abnormal_stop(mhgp12_core_result_${misuse} mhgp12_core_misuse_probe ${misuse} LABELS unit fast)
endforeach()

# Empoisonnement des tampons : la sonde est toujours construite, la porte n'existe que sous MHGP12_POISON.
add_executable(mhgp12_core_poison_probe ${CMAKE_CURRENT_LIST_DIR}/poison_test.cpp)
target_link_libraries(mhgp12_core_poison_probe PRIVATE mhgp12)
target_include_directories(mhgp12_core_poison_probe PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)
if(MHGP12_POISON)
  mhgp12_expect_code(mhgp12_core_poison 0 mhgp12_core_poison_probe
                     LINE "mhgp12_test_ok tests=1 controles=11" LABELS unit fast)
endif()

# Empoisonnement ASan du cache de blocs (CST-0019) : sous ASan, GCC ou Clang, lire un bloc rendu au cache ou la queue
# d'un bloc de classe arrete le processus (SIGABRT par abort_on_error) ; le temoin garde finit par le code 0. La sonde
# est toujours construite, les portes n'existent que sous MHGP12_SANITIZE.
add_executable(mhgp12_core_cache_poison_probe ${CMAKE_CURRENT_LIST_DIR}/cache_poison_probe.cpp)
target_link_libraries(mhgp12_core_cache_poison_probe PRIVATE mhgp12)
if(MHGP12_SANITIZE)
  mhgp12_expect_code(mhgp12_core_cache_poison_garde 0 mhgp12_core_cache_poison_probe garde
                     LINE "sonde_cache_ok garde" LABELS unit fast)
  foreach(mode lecture_apres_restitution lecture_hors_taille)
    mhgp12_expect_abnormal_stop(mhgp12_core_cache_poison_${mode} mhgp12_core_cache_poison_probe ${mode} LABELS unit fast)
  endforeach()
endif()

# Refus a la compilation. Gardes de core/types.hpp : flottant (F5 ; -ffast-math par l'en-tete interne, -Ofast par
# l'en-tete public que toute unite inclut) et profil de coordonnees. Puis : refus ignore, identifiants forts, Buffer.
mhgp12_expect_compile_failure(mhgp12_core_fast_math_refusal SOURCE guard_probe.cpp
                              TOKEN mhgp12_fast_math_interdit OPTIONS -ffast-math LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_ofast_refusal SOURCE refusal_probe.cpp
                              TOKEN mhgp12_fast_math_interdit OPTIONS -Ofast LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_coord_bits_refusal SOURCE guard_probe.cpp
                              TOKEN mhgp12_coord_bits_invalide
                              OPTIONS -UMHGP12_COORD_BITS -DMHGP12_COORD_BITS=20 LABELS unit fast)
# Profils refuses (decision D6 de la v12) : 18 bits, admis par la v11, et 33 bits, au-dela des coordonnees u32 (32 est
# admis depuis l'arithmetique en repere local). Meme refus a la configuration : tests/support/tests.cmake.
foreach(bits 18 33)
  mhgp12_expect_compile_failure(mhgp12_core_coord_bits_${bits}_refusal SOURCE guard_probe.cpp
                                TOKEN mhgp12_coord_bits_invalide
                                OPTIONS -UMHGP12_COORD_BITS -DMHGP12_COORD_BITS=${bits} LABELS unit fast)
endforeach()
mhgp12_expect_compile_failure(mhgp12_core_coord_bits_absent SOURCE guard_probe.cpp
                              TOKEN mhgp12_coord_bits_absent OPTIONS -UMHGP12_COORD_BITS LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_refusal_ignored SOURCE refusal_probe.cpp
                              TOKEN nodiscard OPTIONS -DMHGP12_NEGATIVE=1 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_strong_id_naked SOURCE refusal_probe.cpp
                              TOKEN "no matching function" OPTIONS -DMHGP12_NEGATIVE=2 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_buffer_trivial_only SOURCE refusal_probe.cpp
                              TOKEN "types triviaux seulement" OPTIONS -DMHGP12_NEGATIVE=3 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_strong_id_mixed SOURCE refusal_probe.cpp
                              TOKEN "SiteIdx" OPTIONS -DMHGP12_NEGATIVE=4 LABELS unit fast)
mhgp12_expect_compile_failure(mhgp12_core_result_throwing_move SOURCE refusal_probe.cpp
                              TOKEN mhgp12_result_deplacement OPTIONS -DMHGP12_NEGATIVE=5 LABELS unit fast)
