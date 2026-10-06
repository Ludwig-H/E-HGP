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
# Aller-retour publication de l'API -> lecteur officiel (audit abc30ed06) : une provenance coherente est relue sans
# refus par bench/mhgp11_formats.py ; tailles nulles, rapport faux, compte faux et budget nul sont refuses
# parameter_out_of_range avant toute creation (ni D ni D.pending).
add_executable(mhgp11_api_publish_probe ${CMAKE_CURRENT_LIST_DIR}/publish_probe.cpp)
target_link_libraries(mhgp11_api_publish_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_api_publish_reader 0 publish_reader.py --probe $<TARGET_FILE:mhgp11_api_publish_probe>
                   --bits ${MHGP11_COORD_BITS} LINE "api_publish_reader_verdict conforme cas5" LABELS fast)
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

# Deux voies de la sortie supports (livraison L2b, docs/SORTIES.md paragraphe 11) : arbre d'ordre K seul (build_order,
# masque 7035) et ordre K tire de FULL avec le journal des graines sur l'ordre K (build_order_full, masque 16379 :
# voie de compute). Sonde supports_route.cpp : api_detail::compute_supports par chaque voie dans une Session neuve,
# publication et fin d'appel, puis supports.mhgp11sp, manifeste et registres du journal identiques a l'octet.
add_executable(mhgp11_api_supports_route_probe ${CMAKE_CURRENT_LIST_DIR}/supports_route.cpp)
target_link_libraries(mhgp11_api_supports_route_probe PRIVATE mhgp11)
# Nuages et ordres de l'oracle borne S1 (ceux de mhgp11_supports_hierarchy_fraction), W1 dans l'ordre de l'oracle et
# W3 dans l'ordre inverse ; memes octets entre les W. Couverture gravee par profil (u18 : les deux cercles n = 1023
# exclus, 6 ordres, 20 boules publiees, 14 cellules et 36 graines de moins qu'en u21 et u24). 22 s en local (Release u21).
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_supports_route_line "supports_route_couverture bits=18 nuages=209 exclus=2 ordres=957 boules=12489 noeuds=12552 supports=12489 etendues=1610 multiples=0 cellules=8605 graines=26471 ordres_6plus=7 fils=1,3")
  set(mhgp11_supports_route_floors --min-clouds=209 --min-orders=957 --min-balls=12489 --min-cells=8605
      --min-extended=1610 --min-multiple=0 --min-high=7)
else()
  set(mhgp11_supports_route_line "supports_route_couverture bits=${MHGP11_COORD_BITS} nuages=211 exclus=0 ordres=963 boules=12509 noeuds=12576 supports=12509 etendues=1613 multiples=0 cellules=8619 graines=26507 ordres_6plus=7 fils=1,3")
  set(mhgp11_supports_route_floors --min-clouds=211 --min-orders=963 --min-balls=12509 --min-cells=8619
      --min-extended=1613 --min-multiple=0 --min-high=7)
endif()
mhgp11_python_gate(mhgp11_api_supports_route_oracle 0 supports_route_oracle.py
                   $<TARGET_FILE:mhgp11_api_supports_route_probe> --bits=${MHGP11_COORD_BITS}
                   --work=${CMAKE_BINARY_DIR}/supports_route_oracle --workers=1,3 ${mhgp11_supports_route_floors}
                   LINE "${mhgp11_supports_route_line}" LABELS oracle fast TIMEOUT 900)
# Echelle (8 000, 16 000, 32 000 sites, uniform18 des portes mhgp11_supports_hierarchy_scale*) et trames LiDAR entieres
# sans sol a K5 (MHGP11_DATA_DIR, jamais recopiees) : les deux voies a W1 puis W4, memes octets entre les voies et
# entre les W ; fichier egal a celui des portes de la sortie supports. Ligne gravee (comptes du produit, registres du
# journal, empreintes) ; planchers de boules et de cellules. Une ligne JSON de mesure par (W, voie), descriptive.
# Empreintes de MHGP11SP et de son manifeste par profil : ils portent coord_bits (qualification G4 du 5 octobre 2026,
# audits a40e9cc6d et 8682f4082) ; comptes et registres du journal sont communs aux trois profils.
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_route_scale8000_file 09a1101bc512394e)
  set(mhgp11_route_scale8000_manifest e10e6c8d117429e3)
  set(mhgp11_route_scale16000_file 62e74f1f5df26634)
  set(mhgp11_route_scale16000_manifest dc3a1b98925c658a)
  set(mhgp11_route_scale32000_file a91f226656b6cbd3)
  set(mhgp11_route_scale32000_manifest 3e29e3f5d1403abd)
  set(mhgp11_route_ng00_file 51b941795a2361c7)
  set(mhgp11_route_ng00_manifest bc8d49e588415eed)
  set(mhgp11_route_ng01_file b8449cfbc212f782)
  set(mhgp11_route_ng01_manifest f6b275a818df1d39)
  set(mhgp11_route_ng02_file a4642008f9145a97)
  set(mhgp11_route_ng02_manifest 247ffdc9a96d2891)
elseif(MHGP11_COORD_BITS EQUAL 21)
  set(mhgp11_route_scale8000_file 042faefbdb28cdde)
  set(mhgp11_route_scale8000_manifest 893caa565df996ca)
  set(mhgp11_route_scale16000_file c14680fc7204a04b)
  set(mhgp11_route_scale16000_manifest cbc698ae8ac232f9)
  set(mhgp11_route_scale32000_file 2b8486fb37005a7a)
  set(mhgp11_route_scale32000_manifest 7fa9ea6a785d263b)
  set(mhgp11_route_ng00_file 921e19d469b6acfa)
  set(mhgp11_route_ng00_manifest 71676fdf16b2b2df)
  set(mhgp11_route_ng01_file 74e30730244bc28e)
  set(mhgp11_route_ng01_manifest c3914090b40d7c04)
  set(mhgp11_route_ng02_file 5b4ad69eb41a6742)
  set(mhgp11_route_ng02_manifest 5cbe0264b7df31fb)
elseif(MHGP11_COORD_BITS EQUAL 24)
  set(mhgp11_route_scale8000_file 35583fa59294790a)
  set(mhgp11_route_scale8000_manifest 0cde764601503047)
  set(mhgp11_route_scale16000_file 9a5e6fbc0e8041d3)
  set(mhgp11_route_scale16000_manifest a1e38d5f5af7d348)
  set(mhgp11_route_scale32000_file 0a32bf4343a14b5b)
  set(mhgp11_route_scale32000_manifest 782c22ad18e48558)
  set(mhgp11_route_ng00_file 4f574587dd6678e2)
  set(mhgp11_route_ng00_manifest d0a948b58b58ec7d)
  set(mhgp11_route_ng01_file 08ae13d1e28c660a)
  set(mhgp11_route_ng01_manifest 3158caeb8e86d7b4)
  set(mhgp11_route_ng02_file a60e5078a9cc81f4)
  set(mhgp11_route_ng02_manifest 0a89af314e99fe49)
endif()
foreach(case "scale8000;8000;uniform18=8000,20261002;noeuds=273655 boules=273655 supports=273655 etendues=0 multiples=0 cellules=230825 graines=737362 fichier=${mhgp11_route_scale8000_file} manifeste=${mhgp11_route_scale8000_manifest} journal=a6c3b688a7fe1b4a;273655;230825"
             "scale16000;16000;uniform18=16000,20261002;noeuds=565098 boules=565098 supports=565098 etendues=0 multiples=0 cellules=478947 graines=1530768 fichier=${mhgp11_route_scale16000_file} manifeste=${mhgp11_route_scale16000_manifest} journal=436d1354ae183486;565098;478947"
             "scale32000;32000;uniform18=32000,20261002;noeuds=1163756 boules=1163761 supports=1163761 etendues=0 multiples=0 cellules=989861 graines=3165977 fichier=${mhgp11_route_scale32000_file} manifeste=${mhgp11_route_scale32000_manifest} journal=c371678559b144d7;1163761;989861"
             "lidar;39885;data=lidar_ng00;noeuds=576371 boules=576482 supports=576482 etendues=62 multiples=0 cellules=448805 graines=1350288 fichier=${mhgp11_route_ng00_file} manifeste=${mhgp11_route_ng00_manifest} journal=c189d5cd3ec3de66;576482;448805"
             "lidar;35551;data=lidar_ng01;noeuds=478265 boules=478380 supports=478380 etendues=38 multiples=0 cellules=369751 graines=1102505 fichier=${mhgp11_route_ng01_file} manifeste=${mhgp11_route_ng01_manifest} journal=5e36f8ddbd2fb94c;478380;369751"
             "lidar;45845;data=lidar_ng02;noeuds=609376 boules=610002 supports=610002 etendues=171 multiples=0 cellules=471060 graines=1393952 fichier=${mhgp11_route_ng02_file} manifeste=${mhgp11_route_ng02_manifest} journal=ec87f14da7adff77;610002;471060")
  list(GET case 0 label)
  list(GET case 1 n)
  list(GET case 2 input)
  list(GET case 3 counts)
  list(GET case 4 balls)
  list(GET case 5 cells)
  if(label STREQUAL "lidar")
    string(REPLACE "data=lidar_" "" frame "${input}")
    set(name mhgp11_api_supports_route_lidar_${frame}_k5)
  else()
    set(name mhgp11_api_supports_route_${label})
  endif()
  mhgp11_expect_code(${name} 0 mhgp11_api_supports_route_probe --work=${CMAKE_BINARY_DIR}/${name} --k=5
                     --workers=1,4 --${input} --min-balls=${balls} --min-cells=${cells}
                     LINE "supports_route_verdict conforme k=5 n=${n} ${counts} fils=1,4"
                     LABELS ${label} TIMEOUT 1800)
endforeach()
