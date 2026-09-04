# Nettoyage d'archive — verdict courant

**A1 fermé sur la source examinée : le nettoyage conserve un retour contrôlé sous refus persistant d'allocation, y compris après les callbacks K1 et K2.** Vérification indépendante du 4 septembre 2026 sur `src/io/archive.hpp`, SHA-256 `cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430`.

Cadre : `exploration_v7_hors_registre` / `cpu_reference` / `quantized_u16_input_only` / `audit_independant_math_and_architecture` / `public_status=not_claimed`. [Reçu courant : sources, binaires, compilation, commandes et sorties complètes](receipts_20260904/archive_delta_current.json).

Le destructeur s'appuie désormais sur des descripteurs conservés, douze noms constants et des tampons de pile. Il supprime le provisoire sans allocation C++, possède aussi le répertoire lors d'un échec de construction et conserve l'archive après publication. Une erreur OS de suppression produit un diagnostic borné sans construire de chaîne dynamique. [Revue statique des chemins et limites](REVUE_NETTOYAGE_ARCHIVE_COURANT.md).

| Vérification indépendante | Résultat |
| --- | --- |
| [Probe minimale inchangée](archive_cleanup_probe.cpp), allocations disponibles | Code **0**, `destructor_returned`, aucun résidu |
| Même probe, allocations impossibles pendant tout le nettoyage | Code **0**, `destructor_returned`, aucun résidu |
| Nouvelle porte `mhgp7_archive_cleanup` | Code **0**, faute persistante exercée ; dix fichiers de forêt supprimés ; 9 défauts de construction, 1 après création du répertoire, 10 de commit ; 24 allocations effectivement refusées |
| Refus tardif inclus dans cette porte | Callbacks K1/K2 atteints (`callback_mask=6`) ; statut `resource_exhausted`, étage `fold`, payload provisoire invalidé, aucun fichier final ni provisoire restant |
| Publication et panne OS incluses dans cette porte | Échec de synchronisation du parent après publication sans retrait de l'archive ; échec de suppression diagnostiqué sans allocation ni terminaison |
| Porte `mhgp7_archive_api` | Code **0** : cycle de vie, abandon après suffixe échoué, synchronisations et sémantiques vérifiés |
| Porte Python `archive_gate.py`, normale puis `-O` | **24 scènes réussies par invocation**, relecture des digests/deltas et rejet des corruptions |

La configuration et la compilation Release ciblées réussissent avec GCC 13.3.0 et `-Wall -Wextra -Wpedantic -Werror`. Les quatre CTests sélectionnés passent. Le CLI reconstruit porte le SHA-256 `fa917eefd8198d8ee676585dd99401f74594dd33a4bf77e1265ef397f439e200` ; le reçu lie les commandes à la copie figée des sources.

Toutes les écritures sont confinées à `audits/.work_archive_delta/`. La nouvelle porte constructeur emploie `/tmp` en dur : **seul ce littéral a été remplacé dans sa copie d'audit** par un chemin sous `audits/`. Le patch et les deux hashes de cette porte figurent au reçu. La probe indépendante et `archive.hpp` sont inchangés. Les autres portes utilisent `TMPDIR` dans `audits/` et Python n'écrit pas de bytecode.

Le code, les portes effectivement consommées et les binaires sont stables avant/après. Un changement concurrent de `bench/compare_v6_v7.py`, non consommé par cette qualification, est conservé explicitement dans le reçu ; la stabilité du worktree entier n'est pas revendiquée. Les erreurs OS restent possibles et le nombre borné d'appels ne constitue pas une borne de latence ni une garantie après coupure électrique. Aucun claim géométrique ou GPU supplémentaire. GCP non utilisé.
