# Portes du module api (tranche S5 de la sortie parametree) : Session, calcul, publication, fin d'appel et etat publie,
# signature tree_k_sha256 version 2, parametres du moteur, auto-test F5.
mhgp11_add_unit(mhgp11_api_session SOURCES session_test.cpp publish_test.cpp
                GROUPS released product_alive refusals equivalence tree_digest provenance after_publish
                       session_identity engine
                LABELS fast TIMEOUT 300)
# Destruction d'une Session (docs/ARCHITECTURE.md, paragraphe 7.1, et regle 4 ; audit general a65903a7b) : un budget
# non revenu a zero (produit encore vivant) termine le processus, arret anormal attendu ; temoin : produit rendu avant
# la destruction, code 0.
add_executable(mhgp11_api_session_misuse_probe ${CMAKE_CURRENT_LIST_DIR}/session_misuse.cpp)
target_link_libraries(mhgp11_api_session_misuse_probe PRIVATE mhgp11)
mhgp11_expect_abnormal_stop(mhgp11_api_session_destroyed_live mhgp11_api_session_misuse_probe vivant LABELS unit fast)
mhgp11_expect_code(mhgp11_api_session_destroyed_released 0 mhgp11_api_session_misuse_probe rendu
                   LINE "session_misuse rendu" LABELS unit fast)
# Pannes injectees : operator new et pthread_create remplaces dans cet executable seulement.
mhgp11_add_unit(mhgp11_api_session_fault SOURCES session_fault.cpp GROUPS session starvation publication LABELS fast)
target_link_libraries(mhgp11_api_session_fault PRIVATE ${CMAKE_DL_LIBS})
# Auto-test F5 (raison environment_selftest) : quatre modes d'arrondi avec et sans FTZ/DAZ, mesures faussees refusees.
mhgp11_add_unit(mhgp11_api_selftest SOURCES selftest_test.cpp GROUPS modes judge LABELS fast)
# Environnement flottant reel du processus : modes et FTZ/DAZ admis (code 0), exception demasquee refusee par
# environment_selftest (invariant viole, code 3) avant toute Session.
add_executable(mhgp11_api_selftest_probe ${CMAKE_CURRENT_LIST_DIR}/selftest_fault.cpp)
target_link_libraries(mhgp11_api_selftest_probe PRIVATE mhgp11)
foreach(case "none;0;none" "upward;0;none" "ftz_daz;0;none" "inexact;3;environment_selftest"
             "underflow;3;environment_selftest" "denormal;3;environment_selftest")
  list(GET case 0 name)
  list(GET case 1 code)
  list(GET case 2 reason)
  mhgp11_expect_code(mhgp11_api_selftest_fault_${name} ${code} mhgp11_api_selftest_probe ${name}
                     LINE "selftest_probe ${reason}" LABELS fast)
endforeach()
