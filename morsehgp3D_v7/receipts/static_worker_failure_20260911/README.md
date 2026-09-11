# Allocation scratch après admission : qualification O2, SAN non qualifié

11 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Complément du
[paquet de mutants statiques](../static_resolver_mutants_20260911/README.md),
sans modification des sources actives, des fichiers d'auditeur ou de GCP.

**La panne après admission est correctement propagée en O2 : aucun ordre
publié, tous les workers joints et travail déjà payé conservé.** Ce résultat
porte sur le prototype CPU `33e7d05e…`, avec une instrumentation privée décrite
ci-dessous. L'exécution ASan/UBSan n'est **pas qualifiée** : sa capture conserve
un échec fatal de LeakSanitizer sous ptrace. Le rejeu hors sandbox a demandé une
approbation puis a été interrompu sans autorisation exploitable ; aucune
exécution hors sandbox n'est revendiquée.

## Injection et vérifications

Un [hook test-only](capture/test_only_hook.diff) est placé juste avant le premier
`push_back` allouant de la pile scratch d'un worker. Il arme un drapeau local au
thread ; le véritable `operator new` suivant échoue avec `std::bad_alloc`.
L'allocation refusée est celle d'un `NodeRef`, **4 octets**, et survient après au
moins une MEB effectivement payée. L'injection n'est ni une allocation géante
ni un nouveau plafond de travail. Le
[driver](capture/driver.cpp) utilise des atomiques pour le choix unique de la
panne et un drapeau thread-local pour l'allocation ; aucune écriture partagée
non synchronisée n'a été introduite dans cette instrumentation.

Deux passes injectent la panne, sur des pools de deux puis quatre workers.
Chacune impose un échec `resource_exhausted`, la raison
`full_ball_allocation_failed`, `orders.empty()`, la jointure de tous les workers
et la conservation des MEB déjà payées. Le nominal O2 conserve au total deux
MEB pour ces deux pannes. Après désarmement, la même bibliothèque reconstruit
le corpus rationnel léger : **2 524 signatures/coupes exactes et 1 506 images
verticales** concordent avec Gamma. Les census et réponses attendues sont les
mêmes octets que dans le paquet précédent, pas recalculés depuis ce moteur.

Le [mutant supprimant la réduction des stats dans le catch](capture/drop_paid_work.diff)
compile, puis échoue avec la raison exacte
`paid_worker_work_retained_after_failure`. La garde sur le travail payé est
donc non vacue. Ce mutant ne signale pas un défaut nominal connu : il réfute
l'omission hypothétique du traitement d'exception déjà présent.

## Captures et limites

| Voie | Compilation | Exécution | Statut conservé |
| --- | ---: | ---: | --- |
| O2, injection après admission et réutilisation | 0 | 0 | Réussite |
| O2, mutant sans conservation des stats | 0 | 1 | Mutant réfuté |
| ASan/UBSan, même injection | 0 | 1 | Échec fatal LSan/ptrace, aucune qualification SAN |

Les [quatre commandes O2](capture/o2_commands.json) et les
[deux commandes SAN](capture/san_commands.json) sont fermées. Aucun échec de
compilation n'est compté comme mutant tué. La source de l'instrumentation est
identique entre O2 et SAN, mais cela ne transfère pas le résultat O2 aux
sanitizers. La capture SAN n'a pas de `sources_after.json`, le recorder s'étant
arrêté à l'échec ; cette absence historique est conservée, jamais comblée par
un témoin fabriqué après coup.

La vérification des races repose ici sur la contrelecture des accès et la
jointure du pool ; **aucun résultat ThreadSanitizer n'est rapporté**. Le reçu
ne prétend pas couvrir toutes les allocations, toutes les pannes de thread,
ni la complétude géométrique du producteur. Il n'établit aucun contrat de temps.

## Lecture portable

Les copies répétées sont dédupliquées via [storage_map.json](storage_map.json),
avec empreintes et mapping de chaque fichier logique. Sources, sorties et
raisons sont conservées octet pour octet. Aucun ELF n'est distribué ; leurs
empreintes sont conservées. Les sources historiques et scripts non destinés
à être exécutés portent `.source` lorsqu'ils sont exposés directement.

```bash
python3 -B morsehgp3D_v7/receipts/static_worker_failure_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/static_worker_failure_20260911/verify.py
```

Le lecteur ne compile ni n'exécute le moteur. Il vérifie l'O2 réussi, le mutant
causal et **l'échec SAN conservé** ; il ne relance pas la demande d'approbation
ni un sanitizer. GCP non utilisé.
