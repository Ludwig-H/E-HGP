# Qualification ciblée du port MEB différé — 4/5 septembre 2026

`public_status=not_claimed`. **32/32 CTests intégrés passent en Release et 32/32 sous ASan/UBSan**, sur deux builds CPU neufs et isolés. Aucun benchmark, résultat GCP, mesure SLO ou certificat global n'est déduit de ces portes. Les contre-exemples attendus sont jugés par leurs codes exacts et préfixes causaux, pas comptés comme des exécutions nominales.

## Port et liaison à la source

Le GO porte exactement quatre fichiers : `src/forest/silent_incidence.hpp`, `src/core/mutants.hpp`, `tests/meb_lazy_gate.cpp`, puis sept enregistrements en fin de CMake. Le patch approuvé est la révision2 `d5f273e37d506daf3894c8d7fd271e3b8b877522119863d73d629344ab499bed` ; source réelle et test permanent sont octet-identiques à cette révision, respectivement `5214a9a7f2b6f53b1c59c803d414e109c9a660f15ab9448d88aec90300160c71` et `122807a3fe431bd9658262f8061bcb7e2258a7832516ceff918da52d08ac3a55`.

Chaque lane q3/q4 utilise une closure de matérialisation commune aux builds produit et test. Seuls compteurs et appel eager sont instrumentés. Le test permanent utilise le registre réel ; aucun adaptateur overlay n'est porté. Les compteurs sont logiques, pas une mesure des instructions exécutées. [La revue indépendante](../meb_lazy_review_20260904/README.md) reste un reçu antérieur séparé, avec ses propres pins ; ses résultats ne sont pas réattribués au port intégré.

Les sources, tests, oracle, CLI, CMake et fichiers contractuels présents dans `receipts/conformite_v5` sont épinglés avant/après chaque campagne ; cette dernière empreinte ne transfère aucune autorité v5 et ne signifie pas que chaque fichier inventorié est consommé par les32 portes. Neuf binaires privés sont épinglés après compilation, avant les tests et après ceux-ci. Les listes sont identiques aux deux extrémités de chaque run. Ce build neuf n'est pas présenté comme hermétique : toutes les bibliothèques et tous les en-têtes système ne sont pas archivés.

## Exécutions fraîches et dates

| Configuration | Début configuration UTC | Début CTest UTC | Résultat | Durée CTest rapportée |
| --- | --- | --- | --- | --- |
| Release, C++20, O3, NDEBUG | 2026-09-04 23:56:58 | 2026-09-04 23:59:09 | 32/32 | 3,95s |
| ASan/UBSan, C++20, O1, g, NDEBUG | 2026-09-04 23:59:25 | 2026-09-05 00:04:55 | 32/32 | 31,39s |

Ces durées sont celles de la qualification sur hôte partagé, pas des performances pipeline. Les constructions dédiées prennent130,62s et329,13s et ne réutilisent pas les exécutables historiques. Les commandes détaillées, limites de temps, sorties et statuts sont dans les JSON originaux. Les JUnit ont exactement les32 noms attendus, tous uniques et `status=run`, sans failure/error/skipped. Le juge JUnit est lui-même éprouvé sur un cas positif et huit rejets en Python normal et `-O`.

Les options du second run sont explicites : `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. [Le lancement](sanitizer_launch.json) et [les deux valeurs effectivement observées](sanitizer_effective_options.json) sont conservés. Elles sont héritées par les sous-processus, notamment les scènes d'archive. Aucun diagnostic sanitizer n'a été neutralisé ni reclassé en succès ; l'absence d'erreur vaut seulement pour les exécutions testées, pas comme preuve générale de sûreté mémoire.

## Périmètre et non-vacuité

La sélection est l'union ancrée de32 noms littéraux, sans label global ni porte de benchmark : sept nouvelles MEB ; six Gamma/complétion ; quatre archive/API/nettoyage ; quatre vrai mono ; quatre census direct ; quatre refus d'allocation ; trois échecs de lancement. L'inventaire détaillé et la regex réellement exécutée sont dans `expected_names.json`, `inventory.stdout` et les commandes CTest de chaque sous-dossier.

Le nominal MEB compare11805 cas, dont664 succès,8 dégénérescences et11133 refus cap. Les supports finaux non vacus sont q2=53, q3=79 et q4=34. Il observe329615/26952 rejets bruts q3/q4 et zéro matérialisation logique sur rejet. Le mutant eager conserve les11805 comparaisons d'objets/statuts/comptes et est tué uniquement par329615/26952 matérialisations rejetées ; les deux faux négatifs sont isolés à leur support minimal q3/q4.

Gamma exerce26 scènes,1492 coupes,1543928 paires du cœur,8 incidences silencieuses et371 transitions normalisées. Les archives sont rejouées sur24 scènes, en Python normal et `-O`, avec relecture indépendante des digests/deltas et refus des corruptions. Le vrai mono vérifie quatre tours complètes, huit rejets et30 créations de threads du contrôle positif ; les trois injections tardives/précoces ont chacune deux scènes. Les logs entiers conservent les autres planchers, pas seulement ce résumé.

Les binaires de CLI utilisés pour ces fixtures sont privés à `build/v7_meb_integrated_tests/{release,sanitized}` : SHA Release `127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f`, sanitizer `21b982716bedd5ee7acd4be4ed8fac4bcf4b649cefe48ab06d329babce92ef7a`. Ils ne constituent pas les reçus du futur chronométrage C/D, qui utilise un build et un protocole séparés. Les deux exécutables C protégés, `build/v7/mhgp7` et `build/v7_c_qualification/mhgp7`, restent exactement `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`.

## Conservation et publication

Les textes sous `release/` et `sanitized/` sont des copies octet-identiques des reçus clos sous `build/v7_meb_integrated_tests/*_receipts`, **sauf les deux projections d'inventaire détaillées ci-dessous**. Les JSON de source/build restent historiques et ne sont pas réécrits pour ressembler à un worktree ultérieur. Le mapping d'export indique tous les chemins et hashes originaux/publics. Le fichier original `receipt_manifest.json` est nommé `receipt_manifest.original.json` ici : il décrit le dossier d'exécution original, pas les noms publics renommés.

Les logs complets `LastTest.log` sont publiés sous `LastTest.stdout`, sans modification des octets : Release `395e438d25febc6c0818f1ee5110f5f2e0fc595524d7b152cfbaf3f41d186453`, sanitizer `28c04f8d4042f29d9d9126dcd83d82651afe48ac44621d25b78f5738a0db3f9a`.

Seuls `release/inventory.stdout` et `sanitized/inventory.stdout` sont des **projections `append_LF`**, pas des copies brutes : `apply_patch` ajoute un unique LF terminal à leurs originaux CTest sans LF, conservés intacts sous build. L'égalité exacte « publié = source + un LF » et l'égalité JSON ont été vérifiées ; aucun contenu JSON n'est changé. Le manifeste fournit les pins complets :

| Projection publique | Octets source → publiés | SHA256 source | SHA256 publié |
| --- | --- | --- | --- |
| `release/inventory.stdout` | 34605 → 34606 | `257d3f7c588e0909d2baab8b77248e9c0fc510ebe76a506877a04467882df7b9` | `15e8353150cb80f2a2f5d45bb06de9241ecd8642b4300253feb729e8b3852a40` |
| `sanitized/inventory.stdout` | 34733 → 34734 | `53522018571c649af91979e41277815a2516936e4a0eada92c670bb2dc9a04d6` | `b1ec84b00ad89e80b5854c134bb4c2cee6e3914d70104cb488d31202b4c46153` |

`run_targeted.original.py` est le runner tel qu'exécuté, hashé, sans réécriture de ses chemins relatifs. Pour reproduire ses commandes, le remettre au chemin déclaré `build/v7_meb_integrated_tests/run_targeted.py` dans un workspace où les deux dossiers de build/reçus sont absents ; il refuse d'écraser une tentative. Il charge depuis des octets épinglés le helper déjà archivé `receipts/release_20260904/run_release.py`, jamais les anciens résultats.

`SHA256SUMS` couvre tous les fichiers publics sauf lui-même. Les blobs effectivement indexés doivent être revérifiés au staging par le responsable de publication ; aucun binaire n'est versionné ici. Aucun fichier v6, branche Git, commit ou ressource GCP n'a été créé ou modifié par ce sous-chantier. La suite CPU complète323 et les mesures C/D restent des étapes distinctes à autoriser et qualifier séparément.
