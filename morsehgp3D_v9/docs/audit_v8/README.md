# Rapports détaillés de l'audit général de la v8

22 septembre 2026. Douze lentilles en lecture seule sur `origin/main`
**12294241**. Chaque rapport ci-dessous est la **version contre-vérifiée** :
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

Le cadre annoncé en tête de chaque rapport est celui de l'objet audité (la
v8) ; l'audit lui-même relève de `exploration_v9_hors_registre`. Il est mené
par la lignée de session du développeur sortant de la v8 : les auditeurs de
la v9 doivent le rejuger.
