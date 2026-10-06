# MEB exacte bornee et raccord au census global : oracle Gram, proprietes et refus transactionnels.
mhgp11_add_unit(mhgp11_tower_unit SOURCES unit.cpp
                GROUPS geometry local_support refusals ownership wrapper capacity shell concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_fault SOURCES fault.cpp GROUPS starvation LABELS fast)
mhgp11_add_unit(mhgp11_tower_diameter SOURCES diameter.cpp GROUPS canonical_pairs fallback LABELS fast)
add_executable(mhgp11_tower_probe ${CMAKE_CURRENT_LIST_DIR}/probe.cpp)
target_link_libraries(mhgp11_tower_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_fraction 0 fraction_oracle.py $<TARGET_FILE:mhgp11_tower_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_judge 0 model_test.py LABELS oracle fast TIMEOUT 60)
add_executable(mhgp11_meb_bench ${PROJECT_SOURCE_DIR}/bench/meb_probe.cpp)
target_link_libraries(mhgp11_meb_bench PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_bench_collector 0 ${PROJECT_SOURCE_DIR}/bench/meb_collector_test.py
                    LINE "meb_collector_verdict conforme attempts11 corruptions36 provenance20 schedules6 native0"
                    LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_tower_bench_io 0 ${PROJECT_SOURCE_DIR}/bench/meb_io_test.py
                    $<TARGET_FILE:mhgp11_meb_bench> ${MHGP11_COORD_BITS}
                    LINE "meb_io_verdict conforme attempts8 queries144 refusals5"
                    LABELS fast TIMEOUT 90)
# Domaine ferme pour le futur FULL ; aucune descente ni foret qualifiee par ces portes.
mhgp11_add_unit(mhgp11_tower_domain SOURCES domain.cpp
                GROUPS context lookup global_support ownership refusals capacity concurrency permutation LABELS fast)
mhgp11_add_unit(mhgp11_tower_domain_fault SOURCES domain_fault.cpp GROUPS starvation LABELS fast)
mhgp11_add_unit(mhgp11_tower_domain_parallel SOURCES domain_parallel.cpp
                GROUPS equivalence refusals lookup_refusal LABELS fast)
mhgp11_add_unit(mhgp11_tower_cells SOURCES cells.cpp
                GROUPS regular extended capacity ownership refusals extreme concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_cells_fault SOURCES cells_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_cells_probe ${CMAKE_CURRENT_LIST_DIR}/cells_probe.cpp)
target_link_libraries(mhgp11_tower_cells_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_cells_fraction 0 cells_oracle.py $<TARGET_FILE:mhgp11_tower_cells_probe>
                    LABELS oracle fast TIMEOUT 240)
mhgp11_python_gate(mhgp11_tower_cells_model 0 cells_model_test.py LABELS oracle fast TIMEOUT 180)
mhgp11_add_unit(mhgp11_tower_locate SOURCES locate_test.cpp
                GROUPS lookup global_identity saturated outside_catalogue LABELS fast)
mhgp11_add_unit(mhgp11_tower_descent SOURCES descent_test.cpp
                GROUPS interiors outside traces boundaries refusals capacity ownership concurrency singleton singleton_refusals LABELS fast)
mhgp11_add_unit(mhgp11_tower_descent_fault SOURCES descent_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_descent_probe ${CMAKE_CURRENT_LIST_DIR}/descent_probe.cpp)
target_link_libraries(mhgp11_tower_descent_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_descent_fraction 0 descent_oracle.py $<TARGET_FILE:mhgp11_tower_descent_probe>
                    LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_descent_model 0 descent_model_test.py LABELS oracle fast TIMEOUT 60)
mhgp11_add_unit(mhgp11_tower_forest SOURCES forest_test.cpp
                GROUPS plateau multigroup verticals canonical ownership refusals concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_forest_fault SOURCES forest_fault.cpp GROUPS starvation direct_timings LABELS fast)
add_executable(mhgp11_tower_forest_probe ${CMAKE_CURRENT_LIST_DIR}/forest_probe.cpp)
mhgp11_add_unit(mhgp11_tower_forest_sweep SOURCES forest_sweep_test.cpp
                GROUPS reference depth_and_memory timings concurrency LABELS fast)
target_link_libraries(mhgp11_tower_forest_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_forest_fraction 0 forest_oracle.py $<TARGET_FILE:mhgp11_tower_forest_probe>
                    LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_model 0 forest_model_test.py LABELS oracle fast TIMEOUT 90)
mhgp11_add_unit(mhgp11_tower_classification SOURCES classification_test.cpp
                GROUPS analytic prefix refusals ownership_concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_classification_fault SOURCES classification_fault.cpp GROUPS no_allocation LABELS fast)
add_executable(mhgp11_tower_classification_probe ${CMAKE_CURRENT_LIST_DIR}/classification_probe.cpp)
target_link_libraries(mhgp11_tower_classification_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_classification_fraction 0 classification_oracle.py
                    $<TARGET_FILE:mhgp11_tower_classification_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_classification_oracle_model 0 classification_oracle.py --selftest
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_classification_model 0 classification_model.py LABELS oracle fast TIMEOUT 60)
add_executable(mhgp11_full_bench ${PROJECT_SOURCE_DIR}/bench/full_probe.cpp)
target_link_libraries(mhgp11_full_bench PRIVATE mhgp11)
# Export FULL -> points (banc) : la regle de pendaison et l'evaluation sont dans bench/points_hierarchy.py.
add_executable(mhgp11_points_export ${PROJECT_SOURCE_DIR}/bench/points_export.cpp)
target_link_libraries(mhgp11_points_export PRIVATE mhgp11)
# Largeur de l'export POINTS (audit P2 du 4 octobre 2026) : tetraedre regulier au bord du profil, version du format et
# mots par niveau (u24 : quatre mots, niveau non reduit de 196/148 bits).
# Regles de revendication E1 (Holm, H_L1, H_L2 avec borne d'IC) : bibliotheque standard seule.
mhgp11_python_gate(mhgp11_tower_points_flat_claims 0 ${PROJECT_SOURCE_DIR}/bench/points_flat_claims.py --selftest
                   LINE "points_flat_claims_verdict conforme checks10" LABELS fast TIMEOUT 30)
mhgp11_python_gate(mhgp11_tower_points_export_width 0 ${PROJECT_SOURCE_DIR}/bench/points_export_width_gate.py
                   --export $<TARGET_FILE:mhgp11_points_export> --work ${CMAKE_BINARY_DIR}/points_export_width
                   LINE "points_export_width_verdict conforme" LABELS fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_full_bench_io 0 full_bench_io.py $<TARGET_FILE:mhgp11_full_bench> ${MHGP11_COORD_BITS}
                    LINE "full_io_verdict conforme attempts427 successes413 refusals14" LABELS fast TIMEOUT 120)
# Voies de feuille (CPU, feuille source unique sur l'hote, lot sur le Pool) sur 3 000 sites uniformes : meme dump,
# meme registre, lot plus nombreux que les sites (les feuilles se recouvrent ; garde fausse du 4 octobre 2026).
mhgp11_python_gate(mhgp11_tower_full_leaf_lanes 0 full_leaf_lanes.py $<TARGET_FILE:mhgp11_full_bench> ${MHGP11_COORD_BITS}
                    LINE "full_leaf_lanes_verdict conforme sites3000 k5 boules212695 lot27048 non_resolues0 rejouees0 copiees23766 k10 boules1075008 lot35957 non_resolues0 rejouees0 copiees35131 rejouees_sans_reservoir555"
                    LABELS fast TIMEOUT 300)
mhgp11_python_gate(mhgp11_tower_full_bench_semantic 0 full_bench_semantic_test.py
                    LINE "full_semantic_verdict conforme positives21 corruptions48 checks168 native0"
                    LABELS fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_campaign 0 full_campaign_test.py
                    LINE "full_campaign_verdict conforme attempts483 schedules412 interrupted1 checks39532 native0"
                    LABELS fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_full_v10_model 0 full_v10_model.py
                    LINE "full_v10_model_verdict conforme positives42 corruptions19 native0"
                    LABELS fast TIMEOUT 60)

# Memo avant MEB : oracle Definition conserve, nouvelles portes natives sans compilation locale.
mhgp11_add_unit(mhgp11_tower_memo SOURCES memo.cpp
                GROUPS dates collisions_refusals differential capacity LABELS fast)
mhgp11_add_unit(mhgp11_tower_memo_full SOURCES memo_full.cpp
                GROUPS equivalence refusals concurrency LABELS fast)
mhgp11_add_unit(mhgp11_tower_memo_fault SOURCES memo_fault.cpp GROUPS starvation LABELS fast)
# Parapluie public tower/tower.hpp : noms publics de la tour FULL sans tower_detail (tranche S2, aucun octet change).
mhgp11_add_unit(mhgp11_tower_public_header SOURCES public_header_test.cpp GROUPS umbrella order_tree LABELS fast)
mhgp11_python_gate(mhgp11_tower_forest_memo_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_probe> --memo 64 LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_full_memo_collector 0 full_memo_collector_test.py
                    LINE "full_memo_collector_verdict conforme schedules7 interruptions1 inventories2 checks167 native0"
                    LABELS fast TIMEOUT 60)

mhgp11_python_gate(mhgp11_tower_full_reuse_collector 0 full_reuse_collector_test.py
                    LINE "full_reuse_collector_verdict conforme attempts10 decodes6 interruptions1 checks23 native0"
                    LABELS fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_parallel_collector 0 full_parallel_collector_test.py
                    LINE "full_parallel_collector_verdict conforme attempts119 corruptions39 decodes37 schedules7 interruptions2 comparisons12 checks825 native0"
                    LABELS fast TIMEOUT 60)

# Descentes regulieres par lanes logiques ; Definition exhaustive reste l'autorite geometrique.
mhgp11_add_unit(mhgp11_tower_forest_parallel SOURCES forest_parallel_test.cpp
                GROUPS equivalence plateaus extended_merge mixed_plateau lanes48 refusals pool_busy LABELS fast)
mhgp11_add_unit(mhgp11_tower_forest_parallel_fault SOURCES forest_parallel_fault.cpp GROUPS starvation LABELS fast)
add_executable(mhgp11_tower_forest_parallel_probe ${CMAKE_CURRENT_LIST_DIR}/forest_parallel_probe.cpp)
target_link_libraries(mhgp11_tower_forest_parallel_probe PRIVATE mhgp11)
mhgp11_python_gate(mhgp11_tower_forest_parallel_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_parallel_memo_fraction 0 forest_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> --memo 64 LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_forest_parallel_model 0 forest_parallel_model.py
                    LINE "forest_parallel_model_verdict conforme facts27 native0" LABELS oracle fast TIMEOUT 30)

# Raccord options catalogue FULL : faux enfants, artefacts Definition reels, pas de natif local.
mhgp11_python_gate(mhgp11_tower_full_catalogue_collector 0 full_catalogue_collector_test.py
                    LINE "full_catalogue_collector_verdict conforme attempts147 corruptions69 decodes18 schedules7 interruptions2 comparisons9 checks938 native0"
                    LABELS fast TIMEOUT 60)

# Naissances verticales independantes du DSU, puis balayage ferme inchange.
mhgp11_add_unit(mhgp11_tower_vertical_parallel SOURCES forest_vertical_parallel_test.cpp
                GROUPS equivalence closed_dates lanes48 refusals pool_busy LABELS fast)
mhgp11_add_unit(mhgp11_tower_vertical_parallel_fault SOURCES forest_vertical_parallel_fault.cpp
                GROUPS starvation vertical_census_failure LABELS fast)
mhgp11_python_gate(mhgp11_tower_vertical_parallel_fraction 0 forest_vertical_parallel_oracle.py
                    $<TARGET_FILE:mhgp11_tower_forest_parallel_probe> LABELS oracle fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_vertical_parallel_model 0 forest_vertical_parallel_model.py
                    LINE "vertical_parallel_model_verdict conforme cases324 checks3618 corruptions20 equal_dates1 strict_dates29 native0"
                    LABELS oracle fast TIMEOUT 30)

# Rangs deja ordonnes : seules les cohortes avec plusieurs centres construisent des Sphere.
mhgp11_add_unit(mhgp11_tower_birth_runs SOURCES forest_birth_runs_test.cpp
                GROUPS points cohorts extended memory parallel_memo composition LABELS fast)
mhgp11_python_gate(mhgp11_tower_birth_runs_model 0 forest_birth_runs_model.py
                    LINE "birth_runs_model_verdict conforme cases72 checks612 facts156 corruptions300 native0"
                    LABELS oracle fast TIMEOUT 30)

# Diagnostics verticaux controles avant toute reutilisation semantique.
mhgp11_python_gate(mhgp11_tower_full_vertical_collector 0 full_vertical_collector_test.py
                    LINE "full_vertical_collector_verdict conforme attempts131 corruptions71 decodes31 schedules7 interruptions2 comparisons10 checks757 native0"
                    LABELS fast TIMEOUT 60)

# Census physique partage entre descentes successives, jamais entre callbacks simultanes.
mhgp11_add_unit(mhgp11_tower_census_reuse SOURCES census_reuse_test.cpp
                GROUPS descents memo_identity full admission LABELS fast)
mhgp11_add_unit(mhgp11_tower_census_reuse_fault SOURCES census_reuse_fault.cpp
                GROUPS allocation_free starvation LABELS fast)
mhgp11_python_gate(mhgp11_tower_census_reuse_fraction 0 descent_oracle.py
                    $<TARGET_FILE:mhgp11_tower_descent_probe> --workspace LABELS oracle fast TIMEOUT 120)
mhgp11_python_gate(mhgp11_tower_census_reuse_model 0 census_reuse_model.py
                    LINE "{\"checks\": 65808, \"corruptions\": 45, \"native\": 0, \"positives\": 171, \"routing\": 3720, \"saturation_facts\": 81, \"verdict\": \"conforme\"}"
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_census_collector 0 full_census_collector_test.py
                    LINE "full_census_collector_verdict conforme attempts201 corruptions390 decodes108 schedules7 interruptions2 comparisons11 cross_route11 checks1914 native0"
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_regular_vertical_reuse_model 0 regular_vertical_reuse_model.py
                    LABELS oracle fast TIMEOUT 30)
mhgp11_add_unit(mhgp11_tower_regular_vertical_reuse SOURCES regular_vertical_reuse_test.cpp
                GROUPS equivalence closed_dates extended cache_contract LABELS fast TIMEOUT 180)
mhgp11_add_unit(mhgp11_tower_regular_vertical_reuse_fault SOURCES regular_vertical_reuse_fault.cpp
                GROUPS factory all_k_memory starvation inactive LABELS fast TIMEOUT 180)
mhgp11_add_unit(mhgp11_tower_dense_lookup_test SOURCES dense_lookup_test.cpp
                GROUPS lookup holes full prefix memory LABELS fast TIMEOUT 120)
mhgp11_add_unit(mhgp11_tower_dense_lookup_fault SOURCES dense_lookup_fault.cpp
                GROUPS allocation_free direct_timings starvation LABELS fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_dense_lookup_model 0 dense_lookup_model.py
                    LINE "dense_lookup_model_verdict conforme cases42 orders138 checks2358 corruptions810 absent504 nonidentity246 all_k_bytes2952 native0"
                    LABELS oracle fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_dense_collector 0 full_dense_collector_test.py
                    LINE "full_dense_collector_verdict conforme attempts136 corruptions57 decodes41 schedules7 interruptions2 comparisons9 checks263 native0"
                    LABELS fast TIMEOUT 60)
mhgp11_python_gate(mhgp11_tower_full_regular_vertical_collector 0 full_regular_vertical_collector_test.py
                    LINE "full_regular_vertical_collector_verdict conforme attempts141 corruptions59 decodes38 schedules7 interruptions2 comparisons13 windows2550 cli11 checks7781 native0"
                    LABELS fast TIMEOUT 60)

# La classification reguliere derive directement les deux ordres actifs du certificat catalogue.
mhgp11_add_unit(mhgp11_tower_regular_classification SOURCES regular_classification_test.cpp
                GROUPS classification full_tables options LABELS fast TIMEOUT 180)
mhgp11_python_gate(mhgp11_tower_regular_classification_model 0 regular_classification_model.py
                    LINE "regular_classification_model_verdict conforme orders168 regular672 extended156 checks7728 corruptions1413 native0"
                    LABELS oracle fast TIMEOUT 30)

mhgp11_python_gate(mhgp11_tower_full_pair_graph_collector 0 full_pair_graph_collector_test.py
                    LINE "full_pair_graph_collector_verdict conforme {\"attempts\": 147, \"checks\": 152, \"cli\": 18, \"comparisons\": 13, \"corruptions\": 62, \"interruptions\": 2, \"native\": 0, \"schedules\": 8}"
                    LABELS fast TIMEOUT 120)

# Meme enumeration et compteurs logiques ; Level q3 et q4 differes.
mhgp11_add_unit(mhgp11_tower_meb_deferred SOURCES meb_deferred_test.cpp
                GROUPS eager_parity q3_witness LABELS fast)
mhgp11_python_gate(mhgp11_tower_meb_deferred_model 0 meb_deferred_model.py
                    LABELS oracle fast TIMEOUT 30
                    LINE "meb_deferred_model_verdict conforme requests54 checks655 corruptions540 native0")

# Qualification de cibles compilateur : collector dans io, sans module natif io requis.
mhgp11_python_gate(mhgp11_tower_full_march_collector 0 ${PROJECT_SOURCE_DIR}/tests/io/full_march_collector.py
                    LABELS fast TIMEOUT 60
                    LINE "full_march_collector_verdict conforme campaigns9 children146 decodes73 corruptions58 provenance4 interruptions2 checks562 native0")
# Table de populations (lemme contre la descente de reference) et ordres concurrents (memes forets, W1/W4/W48).
mhgp11_add_unit(mhgp11_tower_population_concurrent SOURCES population_concurrent_test.cpp
                GROUPS lemma equivalence refusals LABELS fast TIMEOUT 600)
mhgp11_add_unit(mhgp11_tower_population_contract SOURCES population_contract_test.cpp
                GROUPS contexts each_step owned_admission ledger LABELS fast)
mhgp11_python_gate(mhgp11_tower_full_paired_protocol 0 full_paired_protocol_test.py
                    LINE "full_paired_protocol_verdict conforme checks72 native0" LABELS fast TIMEOUT 60)
# Lecteur du diagnostic de pipeline : bornes par le mur des forets, aucun ordre entre departs et fins des voies
# (audit du 4 octobre 2026, pin 66372e621) ; le temoin d'une voie finie avant le dernier depart est accepte.
mhgp11_python_gate(mhgp11_tower_full_pipeline_reader 0 full_pipeline_reader_test.py
                    LINE "full_pipeline_reader_verdict conforme checks17" LABELS fast TIMEOUT 30)
# Pipeline des ordres concurrents : decisions du balayage suivi contre l'ordre sequentiel, puis memes forets,
# verticales et compteurs que la voie par etages (W48 repete contre W1) ; abandon de l'ordre bas pendant l'attente.
mhgp11_add_unit(mhgp11_tower_pipeline SOURCES forest_pipeline_test.cpp GROUPS decisions equivalence abandon
                LABELS fast TIMEOUT 600)

# Arbre d'ordre K seul et rattachement de W_K (tranche S3 de la sortie parametree, build_order) : identite I10 avec
# l'ordre K de build_full, parametres de cout honores (table de populations, memo, lookup dense), fixtures 1, 4, 7 et
# 8 du paragraphe 2.9 de la specification et temoins D2 et E5 du contrat (attendus de l'oracle borne S1, recalcules
# en Fraction), garde de capacite des traces publiees (valeurs synthetiques), petit temoin a K eleve de l'auditeur
# (square_k10, integration L1 : 12 sites, K9 a K12 sur le domaine etroit, attendus de l'oracle S1), E1 = E2 (descente
# d'une K-partie puis ancetre ferme : port de ball_nodes comme juge de test) sur Cat_kmax et sur Cat_K (domaine etroit
# de la facade, seul a porter le contre-cas D2), branches recomptees par descentes neuves, refus, pannes d'allocation
# sans resultat partiel. Delai de 900 s pour les deux executables : order_identity a pris 536 s seule sous ASan + UBSan
# (Debug, machine chargee, contre-lecture S3), et e1e2 juge desormais deux domaines.
mhgp11_add_unit(mhgp11_tower_order SOURCES order_tree_test.cpp GROUPS identity same_params refusals
                LABELS fast TIMEOUT 900)
mhgp11_add_unit(mhgp11_tower_attach SOURCES attach_test.cpp GROUPS fixtures capacity square_k10 e1e2
                LABELS fast TIMEOUT 900)
mhgp11_add_unit(mhgp11_tower_order_fault SOURCES order_tree_fault.cpp LABELS fast TIMEOUT 300)
# Arbre d'ordre K tire de FULL (livraison L2b, build_order_full) : journal des graines pose sur le constructeur de
# l'ordre K, voie non concurrente (serielle, lots, verticales paralleles et reemploi, memo) et voie concurrente (par
# etages a W3, pipeline a W12, masque 16379 de la facade), puis extraction de la seule foret d'ordre K ; foret,
# rattachement et registres du journal identiques a build_order, sur Cat_kmax et Cat_K ; aucune verticale gardee ;
# refus avant tout effet et budgets trop courts sans reservation restante. TSan sur G4 (mhgp11_tower_pipeline et
# cette porte).
mhgp11_add_unit(mhgp11_tower_order_full SOURCES order_full_test.cpp GROUPS identity refusals
                LABELS fast TIMEOUT 900)
add_executable(mhgp11_tower_attach_probe ${CMAKE_CURRENT_LIST_DIR}/attach_probe.cpp)
target_link_libraries(mhgp11_tower_attach_probe PRIVATE mhgp11)
target_include_directories(mhgp11_tower_attach_probe PRIVATE ${PROJECT_SOURCE_DIR}/bench)
add_executable(mhgp11_tower_attach_judge ${CMAKE_CURRENT_LIST_DIR}/attach_judge.cpp)
target_link_libraries(mhgp11_tower_attach_judge PRIVATE mhgp11)
target_include_directories(mhgp11_tower_attach_judge PRIVATE ${PROJECT_SOURCE_DIR}/bench)
# Incidences fortes tirees de WindowAttachment = bloc d'incidences de MHGP11PH (mhgp11_points_export, kmax = K,
# ordres = K), a l'octet, avec le bloc de foret (I10 entre l'export 16379 et l'ordre K seul) : 60 cas bornes K = 1..5,
# temoins D2 et E5 compris. Delai 900 s, comme mhgp11_tower_order : 284 s jouee seule sous ASan + UBSan (Debug,
# machine chargee), au-dela de 300 s une fois (contre-lecture S3).
mhgp11_python_gate(mhgp11_tower_attach_export 0 attach_export_gate.py --probe $<TARGET_FILE:mhgp11_tower_attach_probe>
                   --export $<TARGET_FILE:mhgp11_points_export> --work ${CMAKE_BINARY_DIR}/attach_export
                   --min-cases 60 --min-incidences 647622
                   LINE "attach_export_verdict conforme cas=60 noeuds=270799 incidences=647622 octets=9687200"
                   LABELS fast TIMEOUT 900)
# Differentiel contre l'oracle borne S1 (reference/hgp11_ref/supports.py, Fraction, etage A ; apport des auditeurs du
# 5 octobre 2026, integration L1) : la ligne JSON de la sonde (mode requetes, Cat_K prepare a l'ordre K, arbre d'ordre K
# seul par build_order) egale la projection de Supports.canonical(k, ids) sur les champs de S3 (arbre et niveaux,
# att(b), ant(b), roles, traces strictes), et S* est un support de l'oracle d'arite qmin. Nuages de la suite de
# l'oracle a ses ordres (210 nuages, 951 ordres, dont D2 et E5 sur le domaine etroit) et petit temoin a K eleve de
# l'auditeur a K1..K12 ; voie serielle W1 (points dans l'ordre de l'oracle) et voie par lots W3 (ordre inverse). Un
# nuage hors du domaine du profil est exclu et compte (u18 : les deux cercles n = 1023 de la specification) ; couverture
# gravee par profil, planchers aux valeurs de u18.
if(MHGP11_COORD_BITS EQUAL 18)
  set(mhgp11_attach_fraction_line "attach_fraction_couverture bits=18 nuages=209 exclus=2 ordres=957 boules=15246 noeuds=12552 naissances=6641 fusions=5848 internes=2757 passageres=277 faibles=7553 fusions_3plus=1911 ordres_6plus=7 fils=1,3")
  set(mhgp11_attach_fraction_floors --min-clouds=209 --min-orders=957 --min-balls=15246 --min-nodes=12552)
else()
  set(mhgp11_attach_fraction_line "attach_fraction_couverture bits=${MHGP11_COORD_BITS} nuages=211 exclus=0 ordres=963 boules=15270 noeuds=12576 naissances=6651 fusions=5858 internes=2761 passageres=277 faibles=7564 fusions_3plus=1915 ordres_6plus=7 fils=1,3")
  set(mhgp11_attach_fraction_floors --min-clouds=211 --min-orders=963 --min-balls=15270 --min-nodes=12576)
endif()
mhgp11_python_gate(mhgp11_tower_attach_fraction 0 attach_fraction.py $<TARGET_FILE:mhgp11_tower_attach_probe>
                   --bits=${MHGP11_COORD_BITS} --workers=1,3 ${mhgp11_attach_fraction_floors} --min-births=6641
                   --min-merges=5848 --min-internals=2757 --min-passing=277 --min-weak=7553 --min-wide=1911
                   --min-high=7 LINE "${mhgp11_attach_fraction_line}" LABELS oracle fast TIMEOUT 600)
# Juge a l'echelle (attach_judge.cpp), I1 a I4 recomptes a chaque execution, memes nuages que
# mhgp11_catalogue_euler_scale* : I10 contre build_full en mode 16379, l'ordre K seul etant construit par la voie par
# lots W3 (mhgp11_tower_order_identity_*) ou par la voie serielle sans Pool, avec la meme empreinte attache= de
# WindowAttachment (mhgp11_tower_attach_scale*) ; E1 = E2 (mhgp11_tower_attach_e1e2_* : toutes les boules a 8 000
# points, 2 000 tirees a graine fixe a 32 000).
set(mhgp11_s3_counts_8000 "boules=395667 naissances=164842 fusions=108813 internes=122012 branches=273654 traces=737362 noeuds=273655")
set(mhgp11_s3_counts_16000 "boules=819004 naissances=340057 fusions=225041 internes=253906 branches=565097 traces=1530768 noeuds=565098")
set(mhgp11_s3_counts_32000 "boules=1690045 naissances=700184 fusions=463577 internes=526284 branches=1163760 traces=3165977 noeuds=1163756")
set(mhgp11_s3_floor_8000 --min-balls=395667 --min-merges=108813 --min-internals=122012)
set(mhgp11_s3_floor_16000 --min-balls=819004 --min-merges=225041 --min-internals=253906)
set(mhgp11_s3_floor_32000 --min-balls=1690045 --min-merges=463577 --min-internals=526284)
set(mhgp11_s3_tail_8000 "attache=845bdf1deb8c7bb3 entree=3be1324202d28360")
set(mhgp11_s3_tail_16000 "attache=efb95876c7aa639b entree=3469c29b4c34b7e3")
set(mhgp11_s3_tail_32000 "attache=9b07979d0029104e entree=aea3dec129cef911")
foreach(n 8000 16000 32000)
  mhgp11_expect_code(mhgp11_tower_order_identity_scale${n} 0 mhgp11_tower_attach_judge --uniform18=${n},20261002
                     --k=5 --workers=3 --identity ${mhgp11_s3_floor_${n}}
                     LINE "attach_judge_verdict conforme k=5 n=${n} ${mhgp11_s3_counts_${n}} e2=0 identite=1 ${mhgp11_s3_tail_${n}}"
                     LABELS scale${n} TIMEOUT 1800)
  mhgp11_expect_code(mhgp11_tower_attach_scale${n} 0 mhgp11_tower_attach_judge --uniform18=${n},20261002
                     --k=5 --workers=3 --serial --identity ${mhgp11_s3_floor_${n}}
                     LINE "attach_judge_verdict conforme k=5 n=${n} ${mhgp11_s3_counts_${n}} e2=0 identite=1 ${mhgp11_s3_tail_${n}}"
                     LABELS scale${n} TIMEOUT 1800)
endforeach()
foreach(case "8000;all;395667" "32000;2000,7;2000")
  list(GET case 0 n)
  list(GET case 1 e2)
  list(GET case 2 judged)
  mhgp11_expect_code(mhgp11_tower_attach_e1e2_scale${n} 0 mhgp11_tower_attach_judge --uniform18=${n},20261002 --k=5
                     --workers=3 --e2=${e2} ${mhgp11_s3_floor_${n}} --min-e2=${judged}
                     LINE "attach_judge_verdict conforme k=5 n=${n} ${mhgp11_s3_counts_${n}} e2=${judged} identite=0 ${mhgp11_s3_tail_${n}}"
                     LABELS scale${n} TIMEOUT 1800)
endforeach()
# Trames LiDAR entieres sans sol (MHGP11_DATA_DIR, jamais copiees dans le depot) a K5 : |W_5| = 789 886, 652 958 et
# 832 386 boules ; identite I10 (voie par lots W3), E1 = E2 sur 20 000 boules tirees, export a l'octet.
foreach(case "ng00;39885;789886;235401;213404;naissances=341081 fusions=235401 internes=213404 branches=576483 traces=1350288 noeuds=576371;attache=8d4ad62e754ba32d entree=975c390e5912fabe;576371;1705735;23186944"
             "ng01;35551;652958;195173;174578;naissances=283207 fusions=195173 internes=174578 branches=478385 traces=1102505 noeuds=478265;attache=72a8bf3900de9b61 entree=6b918ef47e9ae56e;478265;1416180;19266136"
             "ng02;45845;832386;248676;222384;naissances=361326 fusions=248676 internes=222384 branches=610022 traces=1393952 noeuds=609376;attache=04381e7f08032a30 entree=6e11fa8bc5ee6432;609376;1807322;24575400")
  list(GET case 0 frame)
  list(GET case 1 n)
  list(GET case 2 balls)
  list(GET case 3 merges)
  list(GET case 4 internals)
  list(GET case 5 counts)
  list(GET case 6 tail)
  list(GET case 7 nodes)
  list(GET case 8 incidences)
  list(GET case 9 bytes)
  set(floors --min-balls=${balls} --min-merges=${merges} --min-internals=${internals})
  mhgp11_expect_code(mhgp11_tower_order_identity_lidar_${frame}_k5 0 mhgp11_tower_attach_judge --data=lidar_${frame}
                     --k=5 --workers=3 --identity ${floors}
                     LINE "attach_judge_verdict conforme k=5 n=${n} boules=${balls} ${counts} e2=0 identite=1 ${tail}"
                     LABELS lidar TIMEOUT 1800)
  mhgp11_expect_code(mhgp11_tower_attach_e1e2_lidar_${frame}_k5 0 mhgp11_tower_attach_judge --data=lidar_${frame}
                     --k=5 --workers=3 --e2=20000,7 ${floors} --min-e2=20000
                     LINE "attach_judge_verdict conforme k=5 n=${n} boules=${balls} ${counts} e2=20000 identite=0 ${tail}"
                     LABELS lidar TIMEOUT 1800)
  mhgp11_python_gate(mhgp11_tower_attach_export_lidar_${frame}_k5 0 attach_export_gate.py
                     --probe $<TARGET_FILE:mhgp11_tower_attach_probe> --export $<TARGET_FILE:mhgp11_points_export>
                     --work ${CMAKE_BINARY_DIR}/attach_export_${frame} --data lidar_${frame} --k 5 --workers 3
                     --min-incidences ${incidences}
                     LINE "attach_export_verdict conforme cas=1 noeuds=${nodes} incidences=${incidences} octets=${bytes}"
                     LABELS lidar TIMEOUT 1800)
endforeach()
# K10 sur ng00 (Cat_10, pic de 3,4 Gio en local a W4) : identite contre build_full K1..10 et E1 = E2 sur 2 000 boules.
mhgp11_expect_code(mhgp11_tower_order_identity_lidar_ng00_k10 0 mhgp11_tower_attach_judge --data=lidar_ng00 --k=10
                   --workers=8 --identity --e2=2000,7 --min-balls=2117675 --min-merges=659261 --min-internals=479064
                   --min-e2=2000
                   LINE "attach_judge_verdict conforme k=10 n=39885 boules=2117675 naissances=979350 fusions=659261 internes=479064 branches=1638612 traces=3831491 noeuds=1638573 e2=2000 identite=1 attache=719d801e659bebe9 entree=975c390e5912fabe"
                   LABELS lidar long TIMEOUT 3600)
