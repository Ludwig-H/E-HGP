# Rapports détaillés de l'audit général de la v8

22 septembre 2026. Douze lentilles, puis quatre lectures complémentaires, en
lecture seule sur `origin/main` **12294241**. Les rapports 1 à 12 sont les
**versions contre-vérifiées** :
un second agent a rouvert chaque affirmation principale et chaque chiffre à la
source (fichier et ligne, reçu JSON, commit), a tenté de les réfuter, puis a
intégré ses corrections ; la dernière section de chaque rapport les liste.
Les rapports d'origine ne sont pas conservés dans le dépôt : leurs corrections
sont visibles dans ces sections. La synthèse est dans
[AUDIT_V8_SYNTHESE.md](../AUDIT_V8_SYNTHESE.md).

Ces rapports sont des lectures, pas des qualifications : aucun n'a compilé ni
exécuté le moteur (la suite CTest a été rejouée à part, voir le
[reçu](../../receipts/audit_v8_20260922/README.md)). Les chiffres marqués
« non vérifiable » n'ont pas de reçu dans le dépôt. Les éléments marqués
« non commis » viennent du worktree partagé et ne font pas partie de la v8
publiée.

| # | lentille | contenu |
| --- | --- | --- |
| 1 | [trajectoire et contrats](01_trajectoire_contrats.md) | chronologie des 169 commits, contrats successifs, gouvernance, incidents |
| 2 | [chaîne q2](02_chaine_q2.md) | tranches 1 à 21 : front, census, Pool, parallélisme, ce qui se porte |
| 3 | [voies q3/q4](03_voies_q3_q4.md) | tranches 22 à 34, flux global, atlas, phases 1-2 de la reprise |
| 4 | [float32](04_float32.md) | voie sans perte : primitives qualifiées, brouillon non commis, sort |
| 5 | [18 bits et tranche en suspens](05_u18_et_tranche_en_suspens.md) | port `a74e90f2`, bornes recalculées, tranche indexée non commise |
| 6 | [portes et qualité](06_portes_tests_qualite.md) | CMake, oracles, mutants, sanitizers, dette de code et de reçus |
| 7 | [mesures et reçus](07_mesures_recus.md) | inventaire des 48 reçus, chiffres, écart au contrat |
| 8 | [LiDAR et G4](08_lidar_entrees_g4.md) | entrées, sans-sol, licences, données personnelles, protocole G4 |
| 9 | [audits indépendants](09_audits_independants.md) | constats des auditeurs A, B, complémentaire et externe, questions restées ouvertes |
| 10 | [héritage v7](10_heritage_v7.md) | objet FULL, constructeur, mesures de tour, GPU, ce que la v8 a laissé |
| 11 | [objet et preuves](11_objet_et_preuves.md) | conformité mathématique, preuves refaites, registre |
| 12 | [parallélisme, GPU, performance](12_parallelisme_gpu_perf.md) | occupation, file de tâches, budget par arête et par opération |
| 13 | [canal de coordination v8](13_canal_coordination_v8.md) | les 3 850 lignes du canal : questions, engagements, obligations de l'aval, constats non clos |
| 14 | [verrous et plan de refonte](14_verrous_et_plan_de_refonte.md) | état de P0 et B1–B5, chantiers § 1–10, oublis du plan v9 |
| 15 | [entrées v8 non lues](15_entrees_v8_non_lues.md) | passation, README et fausses pistes v8 en entier ; garde-fous d'exactitude à relire |
| 16 | [carte du code](16_carte_du_code_v8.md) | les 78 sources avec sha256 et disposition (porter, différentiel, référencer, abandonner) |

Les rapports 13 à 16 ont été ajoutés après la critique de complétude pour
couvrir les sources qu'aucune lentille n'avait lues. Ce sont des lectures
simples, **non contre-vérifiées** ; leurs constats principaux ont été vérifiés
à la source avant d'entrer dans la synthèse, le plan et l'héritage.

Le cadre annoncé en tête de chaque rapport est celui de l'objet audité (la
v8) ; l'audit lui-même relève de `exploration_v9_hors_registre`. Il est mené
par la lignée de session du développeur sortant de la v8 : les auditeurs de
la v9 doivent le rejuger.

## Contradictions entre rapports, tranchées par la critique de complétude

Les rapports ont été contre-vérifiés un par un ; une critique finale les a
confrontés entre eux. Quand deux rapports divergent, c'est la ligne
ci-dessous qui fait foi, vérifiée à la source.

| sujet | rapports | ce qui est vrai |
| --- | --- | --- |
| nombre de commits v8 | 01 contre 06, 11 | 158 jusqu'à `a74e90f2` (`git log 2b658cbe^..a74e90f2 -- morsehgp3D_v8`), 169 jusqu'à 12294241 ; 200 ou 201 selon l'inclusion de la borne, tous chemins confondus |
| introduction du workflow `morsehgp3d-v8-lidar-audit.yml` | 08 contre 01, 06, 09 | `562d090c`, puis `ca73e96b` ; `7a121e44` n'ajoute que `probe.cpp` : la correction de 08 est fausse |
| builds locaux v8 | 07 contre 01, 06 | 150 dossiers `build/v8*` et 31 journaux, soit 181 entrées |
| sonde de la base sans sol | 01, 03, 08, 09, 12 contre 07 | le binaire est celui de la tranche 34 (`build/v8_q4_seed_cells_r2_20260921`, sources identiques à `70de84f2`) ; `92d74c13` n'est que le commit de contexte |
| part de l'atlas dans le gprof de base | 01 (40 %), 03 et 12, 09 (≈ 57 %) | `node_bounds_unchecked` seul 40,32 % ; famille atlas (fragments, `form`, `retain`, balayage) ≈ 57,6 % ; filtres de témoins ≈ 7 % |
| CPU de l'identité u16, scène 0, W8 | 01 contre 05, 07 | user 802,71 / 2 362,80 s, sys 0,64 / 2,32 s ; la référence de phase 1 est en user+sys, d'où +5,2 % et non +5,1 % |
| modèle du CPU local | 02 contre 12 | EPYC 9V74 consigné du 13 au 15 septembre, EPYC 7763 observé le 22 ; rien n'est consigné du 16 au 22 |
| q2 LiDAR 50k K5 « 1,45 s » | 02 contre 07 | traçable : `pipeline_wall_ms` 1 448,74 dans `lidar_growth/MEASURES.jsonl` |
| coût float32 « ×20 à ×70 par prédicat » | 12 contre 04, 07 | sans reçu ; ne ferme aucune piste |
| seuils h_q hors position générale | 09 contre 11 | `proved_here` au registre (l. 120) ; 09 sous-classe ce statut |
| écriture de la sortie v7 en 62,7 ms | 01 contre 09, 10 | sans reçu ; seule la taille de 1,75 Go est vérifiée |
| charge pendant la ligne de phase 1 S0 K10 W8 | 08 contre 03, 07 | `load_before` = 9,25, pas « < 3 » |
| complétude bilatérale q4 sur la trame entière | 03 contre 09, 11 | 61 (K5) et 52 (K10) paires conservées, 1 et 22 boules énumérées : 03 surévalue la couverture |
| mutants du moteur au commit audité | 01, 06, 11 | 8 portes de mutation exécutées et vertes dans le second build, `q34_indexed_witness` désactivée ([reçu](../../receipts/audit_v8_20260922/README.md)) |
| aval à porter | 03 contre 10, 11 | sémantique FULL de `full_ball_tower.hpp` au pin `dc57ffd5` ; pas le fold v4 |
| cadre du contrat | 05 contre 03, 07, 08, 12 | la trame sans sol est le régime prioritaire de la directive du 21 septembre ; le contrat du même jour sur trame brute n'est pas retiré ([synthèse](../AUDIT_V8_SYNTHESE.md) § 9, point 1) |
| fenêtre horaire de la tranche non commise | 01, 04, 05, 06, 09 | développeur 06:29–06:53 UTC, constructeur 09:58–10:56 UTC ; l'horaire de `sanitize_r2` de 06 est incompatible avec les 994,77 s de CTest de 05 |
| volume de la tranche non commise | 03, 05, 06 | 89 fichiers indexés (+7 962), dont la coordination racine ; 45 fichiers de test, 323 occurrences de la borne 18 bits |
| fusions de retours à 2 cm | 02 contre 11 | deux jeux de trames 100 et 200 coexistent (repère du scan 0 ou repère propre) ; les chiffres ne se comparent qu'à jeu égal |
| volumes et unités | plusieurs | reçus v8 : 979 311 698 octets, soit 979,3 Mo ou 933,9 Mio ; `du -sh` affiche 974 M (occupation disque, blocs compris) ; les dérivés KITTI font 114,08 Mo, soit environ 109 Mio |
| compteurs d'identité | 05, 07, 12 | 444 (message de `a74e90f2`) et 449 (filtre v2) ; les schémas de sonde ne sont pas réconciliés |
| taux de rejet des graines q3 par l'atlas | 01 contre 03, 07 | 82,7 à 95,0 % dans les reçus épinglés ; 73,8–90,7 % est un diagnostic sans reçu |
| découpe de la tour v7 | 10 § 3.2 | ordre réel prologue / géométrie / calendrier / épilogue = 16,0 / 62,7 / 16,2 / 4,4 % : la géométrie vaut 62,7 %, le calendrier 16,2 % |
| « localisation Outside perdue » pour les centres q3 | 03 § 5.17 contre 16 | par lecture, un centre q3 ne tombe jamais dans une cellule `Outside` de disque ou de facette ; `outside_domain` compte les graines d'arêtes à une seule complétion (`q4_local_partition.cpp:218`), d'où son invariance pour K ≥ 4 : le défaut de 03 est vide |
| taille de la voie q2 | 02 contre 06 | périmètre non défini ; la carte des sources ([rapport 16](16_carte_du_code_v8.md)) le fixe fichier par fichier |
