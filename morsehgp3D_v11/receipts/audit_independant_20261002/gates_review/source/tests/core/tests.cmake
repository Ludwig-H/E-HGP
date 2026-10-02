# Portes du module core.
mhgp11_add_unit(mhgp11_core_unit SOURCES status_test.cpp buffer_test.cpp ledger_test.cpp
                GROUPS types reasons outcome macros result guarded budget budget_threads buffer csr ledger
                LABELS fast)

# Echec de l'allocation apres la reservation : operator new remplace dans cet executable seulement.
mhgp11_add_unit(mhgp11_core_alloc_fault SOURCES alloc_fault.cpp LABELS fast)

# Empoisonnement des tampons : la sonde est toujours construite, la porte n'existe que sous MHGP11_POISON.
add_executable(mhgp11_core_poison_probe ${CMAKE_CURRENT_LIST_DIR}/poison_test.cpp)
target_link_libraries(mhgp11_core_poison_probe PRIVATE mhgp11)
target_include_directories(mhgp11_core_poison_probe PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)
if(MHGP11_POISON)
  mhgp11_expect_code(mhgp11_core_poison 0 mhgp11_core_poison_probe
                     LINE "mhgp11_test_ok tests=1 controles=9" LABELS unit fast)
endif()

# Refus a la compilation : garde du flottant (F5), profil de coordonnees, refus ignore, identifiants forts, Buffer.
mhgp11_expect_compile_failure(mhgp11_core_fast_math_refusal SOURCE refusal_probe.cpp
                              TOKEN mhgp11_fast_math_interdit OPTIONS -ffast-math LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_ofast_refusal SOURCE refusal_probe.cpp
                              TOKEN mhgp11_fast_math_interdit OPTIONS -Ofast LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_coord_bits_refusal SOURCE refusal_probe.cpp
                              TOKEN mhgp11_coord_bits_invalide
                              OPTIONS -UMHGP11_COORD_BITS -DMHGP11_COORD_BITS=20 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_coord_bits_absent SOURCE refusal_probe.cpp
                              TOKEN mhgp11_coord_bits_absent OPTIONS -UMHGP11_COORD_BITS LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_refusal_ignored SOURCE refusal_probe.cpp
                              TOKEN nodiscard OPTIONS -DMHGP11_NEGATIVE=1 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_strong_id_naked SOURCE refusal_probe.cpp
                              TOKEN "no matching function" OPTIONS -DMHGP11_NEGATIVE=2 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_buffer_trivial_only SOURCE refusal_probe.cpp
                              TOKEN "types triviaux seulement" OPTIONS -DMHGP11_NEGATIVE=3 LABELS unit fast)
mhgp11_expect_compile_failure(mhgp11_core_strong_id_mixed SOURCE refusal_probe.cpp
                              TOKEN "SiteIdx" OPTIONS -DMHGP11_NEGATIVE=4 LABELS unit fast)
