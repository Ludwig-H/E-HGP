# Héritage : ce que la v9 porte de la v8 et de la v7

22 septembre 2026, ouverture de la v9. Rien n'est hérité implicitement : chaque
élément ci-dessous est un **port explicite**, épinglé à son commit, puis
requalifié dans la v9 (portes, mutants, reçu). Les versions précédentes
restent des sujets différentiels et des sources de fixtures, jamais une
autorité. Pin commun de lecture : `origin/main` **12294241** (la v7 n'a plus
changé depuis `dc57ffd5`). Les sha256 de fichiers sont dans les rapports
détaillés de [audit_v8/](audit_v8/README.md).

Légende du statut à l'origine : **prouvé** (preuve écrite et fixture),
**testé** (porte bornée), **mesuré** (reçu épinglé), **hypothèse** (chiffre
d'audit non rejouable depuis le dépôt : à re-mesurer avant tout usage).

## 1. Objet et preuves (à porter en premier)

| élément | source | pin | statut | usage v9 |
| --- | --- | --- | --- | --- |
| Objet FULL régulier : minima Gabriel de cardinal K, multifusions K+1, parents pré-lot, portails silencieux internes | `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`, `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md`, registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (entrées FULL) | `dc57ffd5` | prouvé : théorème conditionnel **horizontal** sous prémisses régulières (registre l. 120-122, 131-134) ; verticales et objet public non prouvés | définition de la sortie |
| Extension non régulière : contributions datées, ancres inertes, verticales après fermeture de plateau | `morsehgp3D_v7/docs/PLATEAUX_FULL_ET_ANCRES.md`, `CONTRAT_COUVERTURES_DATEES.md`, `audits/CONTRAT_VERTICAL_COURANT.md` | `dc57ffd5` | prouvé conditionnel + testé | grilles u18 non régulières : quotient de plateau certifié (`exact_full_quotient_certified`) ou refus de domaine explicite |
| Borne de sortie Ω(N²) à K fixé | `morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md`, `tests/full_output_lower_bound_gate.py` | `dc57ffd5` | prouvé | contrats en surcoût au-delà de la sortie |
| Seuils de voie h_q = Kmax + 2 − q | `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` § 5.3.1 | 12294241 | prouvé (autorité racine) | front et filtres |
| Lemme du citron (α3 = 3, α4 = 2, arête maximale d'un support positif) et contre-exemple α3 sur q4 | `morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md` ; `morsehgp3D_v8/audits/DIALOGUE_AUDITEUR_B.md` (tétraèdre (0,0,0),(60,0,0),(20,42,0),(28,10,49), z=(28,−12,−12)) | 12294241 | prouvé | rejet amont des arêtes |
| Théorème H (témoins hérités par rangs), certificat frère, ordre ComplementFirst, bandes Pool | `morsehgp3D_v8/docs/P0_TEMOINS_HERITES_Q2.md`, `P0_CERTIFICAT_FRERE_Q2.md`, `P0_ORDRE_TEMOINS_Q2.md`, `P0_POOL_TERMINAL_Q2.md` | 12294241 | prouvé + testé | voie q2 |
| Certificat familial, corde resserrée, fenêtre q4 [L, U], couches duales | `morsehgp3D_v8/docs/Q3_Q4_REJET_FAMILIAL_20260920.md`, `Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md`, `Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md`, `Q4_COUCHES_DUALES_20260920.md` | 12294241 | prouvé + testé | preuves gardées ; leur promotion par défaut a été fermée ou différée en v8 ([fausses pistes](FAUSSES_PISTES.md)) : ne les rouvrir que sur mesure sans sol 1 mm |
| Certificat d'atlas pour les graines q3 (compte certifié ≥ K−1 sur cellule fermée) | `morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md` | `0948d2d0` + `a74e90f2` | prouvé (note) + testé (un mutant propre, seuil K−2) + mesuré (82,7 à 95 % de graines rejetées) ; contre-lu par l'auditeur A : correct, avec la preuve manquante de `root_lane_skips` (`audits/CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md` § 2) | à porter avec cette preuve, une fixture d'égalité et un juge ciblé de `q3_center` et `certified_inside_count` ; un nœud profond à K−2 ne fournit aucun fragment q3 |
| Certificat collectif d'arête (sommes H_S, V_S ; triangles = 2 crédits ; même groupe aux 64 coins) ; capacités par ID | `morsehgp3D_v8/audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md`, `audits/collective_edge_20260922/` ; `audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md` | 12294241 | prouvé + testé en prototype (0 faux minorant sur 8 736 requêtes) | rejet avant atlas, rentabilité à mesurer |

Toutes les preuves v8 ci-dessus sont absentes du registre
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` : la v9 les y inscrit
(`proved_here`, avec leurs fixtures d'égalité) avant de les invoquer.

## 2. Code à porter depuis la v8 (moteur entier 18 bits)

| brique | source v8 | pin | état | condition de port |
| --- | --- | --- | --- | --- |
| Type de coordonnée et largeur (`Coordinate = int32`, 18 bits, profondeurs et piles dérivées) | `src/core/types.hpp` | `a74e90f2` | testé | remplacer les gardes par un **type de point certifié** ; table de bornes vérifiée par `static_assert` |
| Nuage préparé : copie privée, refus de plage, clés 3×18 bits | `src/pipeline/prepared_cloud.*` | `a74e90f2` | testé | contre-fixture (0,1,0)/(0,0,65536) ; **sans** l'arbre de plages, dont le seul client est le prototype T1 abandonné et qui occupe 80 % de la mémoire du nuage ([carte](audit_v8/16_carte_du_code_v8.md)) |
| Index global à coupes au milieu, échappements DFS | `src/pipeline/q2_census.cpp` (index, l. 97-194) | `a74e90f2` | testé | découpler de `axis_q2` / `local_credits` ; réserver les 2n−1 nœuds (38 % de mémoire perdue en v8) |
| Prédicats q2 et bornes préparées (u64 depuis 18 bits) | `src/spindle/predicates.hpp`, `src/spindle/q2_prepared_bounds.hpp`, `src/pipeline/q2_joint_bounds.hpp` | `a74e90f2` | prouvé + testé | fixtures aux extrêmes 262 143 |
| Front WSPD `MidpointSamples`, fenêtre 2K et héritage de témoins | `src/wspd/front.cpp` l. 1-500 (sans le dispatcher `Donate`, l. 502-762) | `a74e90f2` | testé + mesuré (×0,35 à ×0,65 hors rangées) | fenêtre 2K et héritage **qualifiés pour q2 seul** : le code les refuse hors du masque 1 (`front.cpp:406-412`), trois mutants survivaient sous le masque 7. Soit deux fronts (q2 ; q3/q4) dans un même appel public, soit une extension par voie avec le juge des bornes Ξ et les trois gardes de l'auditeur A |
| Census q2 (Shared, Complement, frère, collecte complète) et Pool terminal par facteurs | `src/pipeline/q2_census.cpp` (moteur, l. 197-774), `src/pipeline/q2_node_pool.hpp` | `a74e90f2` | prouvé + testé + mesuré | une seule entrée ; défauts = configuration mesurée |
| Boules exactes et clé primitive commune aux arités | `src/lanes/exact_ball.*`, `src/lanes/q4_family.*` | `a74e90f2` | prouvé + testé | garde de domaine ; fixtures 18 bits |
| Recherche de témoins par rectangle et par paire (citron, bornes affines, exclusion) | `src/lanes/q34_witness_search.*`, `src/lanes/q34_pair_bounds.hpp` | `a74e90f2` | testé + mesuré | réactiver les mutants du citron (dont α4) ; réemploi singleton |
| Census q3 par boîtes en deux passes (`PreparedPower`) | `src/lanes/q3_ball_census.*` | `a74e90f2` | testé + mesuré | ne jamais carrer B ; filtre flottant certifié à étudier |
| Atlas q4 Local28 (partition exacte, i64 à Q = 2^20, LiveOnly, Deep ≥ K−2) et centre q3 par division longue | `src/lanes/q4_local*.{hpp,cpp}` | `748ec082`, `a74e90f2` | prouvé + testé + mesuré | poste dominant : réduire le nombre d'opérations |
| Flux global, file bornée de plages, chronos par worker, registres additifs | `src/pipeline/wspd_q34.*`, `src/parallel/*` | `5224ff4e`, `5fdda963` | testé + mesuré (W8 local) | remplacer par un plan plat et des files par worker ; TSan |
| Cover d'arête, domaine positif de l'atlas, API des cellules de graines, type de graine du flux | `src/lanes/edge_cover.*`, `src/lanes/q4_positive_domain.*`, `src/lanes/q4_seed_cells.hpp`, `src/lanes/q34_seed.hpp` | `a74e90f2` | testé + mesuré (chemin mesuré) | le domaine positif refait une traversée d'index par arête (481 M visites à K5, scène 0) : la fusionner avec celle du cover ; déplacer `Q4CenterDomainMode` hors de l'en-tête du prototype T27 |
| Entrées q2 du chemin mesuré | `src/pipeline/q2_census.hpp`, `wspd_q2_census.hpp`, `wspd_q2_parallel.hpp` | `a74e90f2` | testé + mesuré | une seule entrée publique ; les variantes coopérative, par plages et par lots restent en cible différentielle |
| Lecteur `.u32le` à empreinte sur les valeurs | `bench/q4_lidar_probe.cpp` | `a74e90f2` | testé par usage | profil déclaré par la grille, pas déduit des valeurs |
| Oracle entier q3/q4 de l'auditeur B | `morsehgp3D_v8/audits/oracle_q3q4_20260915/` | 12294241 | testé | bornes u16 à requalifier en 18 bits |
| Oracles rationnels et portes | `oracle/p0_oracle.hpp`, `oracle/q2_census_oracle.hpp`, `tests/exact_ball_oracle.hpp`, `tests/wspd_q34_gate.cpp`, `tests/q3_q4_owner_independence_gate.py` | 12294241 | testé | Boost obligatoire |
| Harnais indépendants de l'auditeur B (flux q3/q4, validité bilatérale q4) | `morsehgp3D_v8/audits/q3_stream_crosscheck_20260921/`, `q4_stream_crosscheck_20260921/`, `q34_stream_crosscheck_spatial_20260921/` | 12294241 | testé | juges d'échantillon, jamais des portes exhaustives |
| Injection déterministe d'échecs d'allocation (remplacement de `operator new` global) ; discrimination causale des mutants par la ligne d'erreur attendue | dix portes C++, p. ex. `tests/wspd_q34_gate.cpp:16-38` ; `tests/wspd_q34_mutations.py` | 12294241 | testé | à reprendre avec les codes de sortie exacts de la v7 |

Disposition fichier par fichier des 78 sources (porter, cible différentielle,
référencer, abandonner ; découpage des trois fichiers mixtes `q2_census.cpp`,
`front.cpp`, `q4_local.cpp`) : [carte du code](audit_v8/16_carte_du_code_v8.md).
Défauts relevés à la lecture, à corriger au port : `run_joined_workers` sans
crochet d'annulation et attente de file qui ignore l'annulation
(`wspd_q34.cpp:752,783-784`, jamais passé sous TSan) ; propriété des graines q4
testée deux fois (`q4_local.cpp:559-594`) ; feuilles vides comptées vivantes
en LiveOnly. Relevés par l'auditeur A : constructeur public de
`Q4LocalFragment::Key` (`q4_local_partition.hpp:158`, la passkey ne protège
rien) ; `clock_gettime` qui retombe silencieusement à zéro
(`wspd_q34.cpp:727-735`, publier `cpu_clock_valid`) ; deux commentaires de
borne périmés (`q4_local_partition.cpp:64` « Gram < 2^33 » au lieu de 2^37,
`q3_ball_census.hpp:54` « < 2^105 » au lieu de 2^117). Les 42 bornes simples
des commentaires sont vraies (`audits/check_u18_bounds_20260922.py`) ; la note
`ELARGISSEMENT_18_BITS_20260922.md` contient huit énoncés faux et ne doit pas
servir de source.

Les portes et validateurs Python de la v8 écrivent souvent le chemin
`morsehgp3D_v8` en dur (73 scripts sur 101) et s'importent en chaîne : les
copier tels quels en v9 les casse. Porter la logique, pas les fichiers.

Dans la tranche **non commise** du constructeur (voir la [synthèse](AUDIT_V8_SYNTHESE.md) § 8),
trois éléments méritent un port une fois qu'ils seront commis ou épinglés :
la porte de domaine numérique `tests/u18_numeric_domain_gate.cpp`, le type
résultat XOR « fragment exact ou certificat terminal » de `saturate_deep` avec
ses trois mutants, et le filtre logique v2 des compteurs de
`bench/run_ground_baseline.py` (qui inclut les compteurs `terminal_*` oubliés
par la v1).

## 3. Code et outils à porter depuis la v7

| élément | source v7 | statut | usage v9 |
| --- | --- | --- | --- |
| Constructeur de tour par boules (représentants pré-lot, fermeture atomique, ancres inertes, verticales à la coupe fermée) | `morsehgp3D_v7/src/forest/full_ball_tower.hpp` | testé borné | sémantique de référence ; gardes de clé u16 (A < 2^68, B < 2^87, C < 2^105) à remplacer avant `BallKey::power` par A < 2^76, B < 2^96, C < 2^116 pour q3 (borne q4 à établir) ; comparateur U192/U320 suffisant à 18 bits ; coquille ≤ 12 = refus de domaine, pas un théorème ; catalogue d'entrée exact avec `q_min` recalculé (contre-audit A § 1) |
| MEB à coquille libre, quotient local de coquille, journal daté v2, certificat v1, producteur horizontal | `src/forest/anchor_meb.hpp`, `local_plateau.hpp`, `full_coverage_certificate.hpp`, `full_certificate.hpp`, `full_gabriel.hpp` | testé | juges et différentiels |
| Juge T2 census→FULL (n ≤ 14) et oracles Gamma | `morsehgp3D_v7/tests/census_tower_oracle.hpp`, `census_tower_gate.cpp`, `oracle/full_gamma.hpp`, `oracle/obig.hpp` | testé | première porte de la tour v9 |
| Portes FULL et leurs mutants | `morsehgp3D_v7/tests/full_ball_tower_gate.cpp`, `full_certificate_gate.cpp`, `full_coverage_certificate_gate.cpp`, `anchor_meb_gate.cpp`, `local_plateau_gate.cpp` | testé | idem |
| Portes à code de sortie exact et mutants `--inject` | `morsehgp3D_v7/cmake/run_expect.cmake`, fonction `mhgp7_gate` | testé | 0 conforme, 1 juge, 2 refus, 3 invariant, 4 mutant tué |
| CI Boost obligatoire, `--no-tests=error` ; contrôle `SHA256SUMS` des reçus | `.github/workflows/morsehgp3d-v7.yml`, `tools/check_v7_receipt_publication.py` | testé | CI v9 dès le premier commit de code |
| Architecture des objets parallèles de la tour (atlas `cell(B,K)`, graphes datés sur naissances, forêt minimale, verticales hors ligne) | `morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md` ; sources privées scellées du reçu `receipts/rank_guard_streaming_20260911/MANIFEST.json` | testé + mesuré (CPU4), privé | plan de parallélisation de l'aval |
| Noyau MEB accéléré (paire diamétrale d'abord, extrêmes, canonisation sur la coquille) | `morsehgp3D_v7/audits/NOTE_CLAUDE_COEUR_MEB_20260911.md`, `audits/receipts_coeur_meb_20260911/` ; variante pivot4 de `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` | testé (198 000 cas) + mesuré hors moteur | avec la porte MEB K7 |
| Outillage G4 v7 (paire CPU/hybride, arrêt ciblé) et primitives GPU exécutées sur carte | `gcp-migration/full_ball_worker_v7.py`, `full_probe_session_v7.py` ; `morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md` | mesuré | protocole G4 v9 ; leçons de résidence |

## 4. Fixtures à graver dès les premières portes

| fixture | ce qu'elle tue | source |
| --- | --- | --- |
| E5 (`tests/fixtures/regressions/gabriel_point_set_counterexample.json`) | fold des seules cofaces Gabriel, K-MST | registre `false_in_general` |
| régression silencieuse A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0), Kmax = 2 | parents perdus sans portails silencieux | `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` |
| deux fixtures à quatre points du graphe induit : A=(1,1,7), B=(5,2,1), C=(7,2,2), D=(5,2,8) (fusion vraie 477/34) et celle de l'auditeur v7 (fusion 169/9 retardée à 41/2) | minima avec seules adjacences induites | `morsehgp3D_v7/docs/SQUELETTE_MINIMA_GABRIEL.md`, `tests/full_gabriel_minima_quotient_gate.py`, `audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md` |
| coquille à sept points, triangle rectangle, carré K2 à quatre parents, MEB K7 | filtre p+u ≤ smax, MEB non canonique | `morsehgp3D_v7/docs/FAUSSES_PISTES.md`, `NOTE_CLAUDE_COEUR_MEB_20260911.md` |
| tétraèdre du contre-exemple α3 en q4 ; contacts (32,34,28) en q3 et (32,32,32) en q4 | α faux, témoin non strict | `SUPPORT_ET_CITRON.md`, `DIALOGUE_AUDITEUR_B.md` |
| indépendance des voies (six cas) | q3/q4 filtrés par q2, q4 filtré par q3 | `morsehgp3D_v8/tests/q3_q4_owner_independence_gate.py` |
| F1–F10 des bornes de bloc (sommet intérieur, λ = 24/25, population des témoins, égalité de propriété) | bornes aux coins, restriction de Z aux graines | `morsehgp3D_v8/audits/q3_bloc_float32_fixtures_20260921/` |
| {0,5,10,11} K2 et tangence Hmin = 0 ; cube à 8 sommets ; rails n2718 ; A={(0,0,0),(1,0,0)}, B={(100,0,0)} | certificat frère, doublons de diamètres, addition interdite | notes `morsehgp3D_v8/docs/P0_*.md` |
| maximum en z (A={(0,0,0)}, B={(3,0,0)}, Z=[0,2]×[0,3]×{0}, attendu h_max4 = 9, `Uncertain`) | lacune 2.2 jamais fermée | `morsehgp3D_v8/audits/PORTES_ET_TESTS_20260914.md` |
| extrêmes 18 bits (0 et 262 143) dans chaque porte de prédicat, refus à 262 144, collision (0,1,0)/(0,0,65536) | troncature u32, clés chevauchantes | port `a74e90f2` |
| centre q3 sur une frontière de cellule fermée ; site sur la coquille | localisation fausse de l'atlas | recommandé par l'audit (lentille 11) |
| AB/ABC et triangle rectangle (`tests/fixtures/regressions/delaunay_gabriel_unsupported_degeneracy_right_triangle.json`), matrice d'extra-shells (`gabriel_carrier_strict_interior_extra_shell_matrix.json`) | domaine non régulier : la tour v9 doit soit certifier le quotient de plateau (`exact_full_quotient_certified`), soit refuser explicitement ; jamais une acceptation silencieuse. Le triangle rectangle a une coquille supplémentaire sans aucun record q4 | fixtures racine existantes |
| déjà gravées en v8 : raffinement prématuré, `ComplementFirst`, census conjoint, tangence des rangées, point q4 isolé à huit contacts, boule commune aux trois arités | ordres de témoins, propriété, clé commune | `tests/q2_sibling_gate.cpp:262-275`, `q2_witness_order_gate.cpp:281`, `q2_joint_gate.cpp:241`, `q3_q4_owner_independence_gate.py:100-129`, `q4_seed_cells_gate.cpp:127`, `exact_ball_gate.cpp:207` |
| à graver : mauvais seuil T = K−2 ; exclusion locale par Ξ affine ; migration de crédit entre rectangles sur les sites [100,101,200,0] (le diamètre Gabriel (100,0) disparaît) ; deux rangées lointaines A_i=(1000,i,0), B_j=(60000,j,0) (m² paires q3/q4, contre-régime) | seuils, masque local, transfert de crédit | `morsehgp3D_v8/docs/Q34_PISTES_APRES_INDEXATION_20260921.md:113-114,205-219` ; canal v8 ([rapport 13](audit_v8/13_canal_coordination_v8.md)) ; notes non commises du complémentaire |

Obligations de l'aval posées pendant la v8 et jamais exécutées, à couvrir
par les portes de la tour ([rapport 13](audit_v8/13_canal_coordination_v8.md)) :
surplus à plusieurs seuils ; coquille répétée de 398 sites ; deux diamètres ne
font pas deux boules (recherche d'antipodes) ; la coquille contient le
support ; contact d'un support de poids nul ; ordre exact des rayons et
transport groupé des sorties ; un rejet de voie n'est pas l'inertie d'une
boule ; un rejet q3 n'efface jamais une boule q2 ; départage déterministe de
l'arête maximale ; les feuilles FULL ne suffisent pas aux poids du manuscrit
(profil pondéré séparé).

## 5. Propositions d'audit à re-mesurer (hypothèses)

| proposition | chiffre annoncé | pourquoi « hypothèse » |
| --- | --- | --- |
| Cascade de rectangles : présélection négative par paire représentante, plans h + h_a + h_b, réemploi singleton | ×2,55 à ×4,46 sur le filtrage échantillonné (960 contextes, mêmes paires) ; les plans et le réemploi en portent l'essentiel (×1,90 à ×3,98), la présélection seule ×0,94 à ×1,10 | sources dans une archive jointe à une conversation ; mêmes paires survivantes, donc aucun atlas supprimé ; le filtre pèse environ 7 % du profil de référence, d'où un gain total plafonné à quelques pour cent |
| Certificat collectif avant l'atlas sur LiDAR 1 mm | 221 rejets q4 sur 763 arêtes échantillonnées | même raison ; l'atlas sert aussi q3 |
| Saturation de l'atlas à K−1 (`saturate_deep`) | non mesurée sur trame | tranche non commise ; option non raccordée à une sonde |
| MEB pivot4 canonique | 0,262 à 0,271 du temps (microbenchmark) | hors constructeur, sources hors dépôt |

## 6. À référencer sans porter

- **Voie float32 sans perte** (prédicat q2, `FixedSigned` 1 728 bits, boules,
  clé, événements q4, bloc q3, census d'une arête, index natif) : qualifiée par
  reçus et 23 CTests au pin `a005f8aa` (sources identiques à 12294241). Elle
  reste **dormante** : consigne utilisateur du 22 septembre, aucun
  développement. La v9 la cite par pins et ne la compile pas dans son chemin
  actif ; ses fixtures F1–F10 servent aussi au moteur entier.
- **Protocole G4 v8** (`gcp-migration/q34_spatial_*_v8.py`) : primitives de
  cycle de vie à reprendre, mais le worker est figé sur une autorité de 216
  sources dont 87 ont changé ; un protocole v9 doit être réécrit (autorité au
  commit v9, entrées `.u32le`, sonde à jeton `atlas`, budget découpé).
- **Verrous et plan de refonte v8** (`morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md`,
  `PLAN_DE_REFONTE.md`) : historiques, périmés (contrat 50k, antérieurs aux
  phases 1-2 et au port 18 bits). Leur état au commit audité est résumé en § 10
  de la [synthèse](AUDIT_V8_SYNTHESE.md) ; leurs portes § 8 restent une liste de
  contrôle utile ([rapport 14](audit_v8/14_verrous_et_plan_de_refonte.md)).
- **Mesures de référence** : `ground_baseline_20260921` et
  `ground_phase1_20260921` (sans sol 2 cm) et la tour v7 50k : points de
  comparaison, jamais des qualifications v9.
