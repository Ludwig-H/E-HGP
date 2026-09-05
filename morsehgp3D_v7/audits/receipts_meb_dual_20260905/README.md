# Reçus du raccord local MEB privé à deux budgets

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Les écritures et compilations de l'auditeur sont isolées sous `audits/`. Aucun calcul de tour, benchmark de performance ou GCP pendant cette qualification.

La [note courante](../MEB_DOUBLE_BUDGET_COURANT.md) porte la preuve et les obligations de port. La référence produit reste F ; les [deux headers privés](inputs/manifest.json) sont conservés octet pour octet. Le helper ancien fournit les formes, le rang et la matérialisation ; son ancienne voie sans second budget n'est pas appelée. Le [snapshot F](inputs/source_F.json) épingle séparément ses 143 sources et références.

## Sonde rationnelle propre à l'auditeur

Le [bridge](../meb_dual_bridge.cpp) appelle la vraie référence F et le prototype privé avec un observateur passif. Le [juge](../meb_dual_oracle.py) fournit une autre autorité géométrique : élimination de Gram en fractions rationnelles, barycentriques strictes et coquille complète, depuis le corpus permanent de 89 nuages. F ne fournit ni support ni cap au juge. Chaque nuage passe dans l'ordre d'entrée et son renversement ; les caps L sont 0, R−1, R et R+1, dédoublonnés, puis croisés avec P=0/1/4/5/401.

Les [résultats normal](geometry/normal.json) et [Python optimisé](geometry/optimized.json) relisent les mêmes sorties des deux builds indépendants O2 et O1/UBSan. Par build :

| Contrôle | Observations |
| --- | ---: |
| Ordres locaux / appels MEB | 178 / 3 430 |
| Succès rapides q2 / q3 / q4 | 416 / 310 / 64 |
| Refus legacy / refus shell | 1 650 / 40 |
| Certificats trouvés puis refusés par L | 291 |
| Replis F / formes proposées effectivement chargées | 1 459 / 8 509 |
| Ordinaux comparés à une énumération indépendante | 1 507 |

La clé entière, les quatre slots du support, les trois limbs du niveau et son dénominateur sont contrôlés, ainsi que les statistiques non nulles et le statut/raison initiaux. Les événements restent vides dans cette sonde. Les refus avant accept conservent la boule sentinelle ; les refus scientifiques peuvent conserver une boule écrite, comme F. Un `Work` frais est créé par commande M : le cumul par ordre a une autre preuve.

Trois copies privées fautives sont compilées en O2. Le juge détecte la suppression du shell régulier (`oracle.terminal.121`), l'ordinal majoré de un (`oracle.terminal.6`) et le doublement du numérateur/dénominateur q4 (`oracle.q4_raw_level.84`). Le dernier conserve le rayon et viole son écriture littérale. Les patches restent conservés. Ce sont des mutants de copies privées, pas des portes déjà intégrées au produit.

Le [journal d'exécution](geometry/run.json) conserve 13 commandes closes, les hashes avant/après et cinq binaires observés stables, dans une échéance cumulée de 120 s. Les commandes ont consommé environ 15,15 s, compilation incluse ; ce chiffre n'est pas une mesure de gain MEB. Les sorties complètes stdout/stderr sont conservées en gzip déterministe. Les [20 dépendances locales de chaque build](geometry/dependency_review.json) correspondent à F, aux headers scellés ou au patch annoncé. Les hash stdin lient les bruts aux commandes reconstruites.

Le [premier rejet du juge](geometry/initial_judge_rejection.json) est conservé avec [son driver](geometry/initial_driver.py.txt). Il attendait un écart de compteur pour la faute ordinal, alors que le cap L=R expose d'abord un refus terminal incorrect. Le [correctif de classement et de rejeu](geometry/judge_correction.patch) conserve ce premier échec et réutilise les mêmes binaires et sorties ; aucun résultat moteur n'est réécrit. Le driver courant permet aussi une destination de reproduction neuve sous les audits.

## Contrelecture du triangle constructeur

La [revue indépendante](qualification/review.json) vérifie six commandes closes, leurs codes 0/0/0/4/2/2, 38 artefacts et 61 pins source/autorité. Le [manifeste de captures](qualification/capture_manifest.json) référence les preuves déjà conservées lorsque leurs octets sont identiques ; aucun binaire n'est versionné. La lecture live confirme 62 pins et 20 dépendances locales.

La porte triangle teste huit couples P/L, quatre appels cumulatifs et une frontière MAX. Son mutant `ChargeAfter` conserve les terminaux mais provoque 28 violations causales contre zéro en nominal, avec le code 4 requis. Les constantes imprimées des cas cumulatifs sont soutenues par les conditions exécutées dans le gate, pas par une trace exhaustive de chaque appel. Ce reçu ne qualifie aucun q4.

Le [lecteur portable](qualification/verify_triangle.py) passe normalement et sous `-O`, avec quatre positifs et quatorze rejets ciblés. Les anciennes mentions privées « non compilé » sont des métadonnées de préparation, antérieures au reçu clos ; elles ne sont pas des statuts actifs du présent audit.

## Porte géométrique constructeur, close séparément

Le [lecteur du reçu géométrique](geometry_constructor/verify_geometry_receipt.py) contre-vérifie les [sorties et compteurs capturés](geometry_constructor/captured_optimized.json) : 9 339 comparaisons à F, 1 507 ordinaux et 384 appels F de mesure de R. Ce corpus de 176 scènes/384 ordres est distinct des 89 nuages de l'oracle rationnel. Il choisit ses caps depuis F ; il constitue un différentiel, pas une seconde autorité Gram. Le mutant causal produit 46 437 violations et le code 4, sans divergence des autres terminaux.

Les frontières de compteur près de MAX, les caps abaissés, cinq appels avec `pivot_cap=0` et la scène de support final d'ordinal 550 sont réellement appelés. La [note courante, §7](../MEB_DOUBLE_BUDGET_COURANT.md#7-reçus-privés-et-obligations-de-qualification-actualisées) distingue les observations agrégées des déductions sur la voie rapide à partir du code et des scènes. Elle retire les anciennes demandes satisfaites sans inventer de compteur par scène. Le [manifeste de capture](geometry_constructor/capture_manifest.json) et les [contrôles normal/-O](geometry_constructor/inspector_checks.json) rendent cette lecture portable.

Le constructeur qualifie son instanciation `Trace`. Le bridge indépendant emploie un autre `Observer` passif. Aucune de ces campagnes n'est réattribuée à `NoObserver` : le [reçu natif v2 désormais clos](next_native_review.json) attend sa contrelecture propre. Il n'est pas demandé de refaire ce run. Aucun de ces résultats ne modifie le produit F.

## Modèle des transferts de budget

Le [contrôle entier indépendant](budget/budget_transition_probe.py) compare 128 transferts à des charges unitaires, vérifie quatre appels persistants, cinq états près de MAX et deux corruptions d'audit. La [revue](budget/review.json) distingue ces attendus mathématiques des appels C++ effectivement exécutés. Depuis des compteurs frais, le nombre de candidats physiques des replis et propositions est borné par la somme des deux charges ; les sommes de plafonds sont mathématiques et ne doivent pas être évaluées en u64 au port.

## Reproduction

Rejouer les preuves conservées, sans compilation ni moteur :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/meb_dual_oracle.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/meb_dual_oracle.py
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_meb_dual_20260905/qualification/verify_triangle.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_meb_dual_20260905/qualification/verify_triangle.py --self-test
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_meb_dual_20260905/budget/budget_transition_probe.py --check-only
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_meb_dual_20260905/geometry_constructor/verify_geometry_receipt.py
```

Le juge géométrique régénère seulement son résumé normal/optimisé ; les bruts scellés restent identiques. Le lecteur triangle écrit sur stdout. Le mode `--check-only` du modèle de budgets est purement arithmétique. Pour reconstruire les cinq sondes, après remise en place des sources F épinglées, choisir une destination qui n'a aucun `run.json` :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/meb_dual_oracle.py --run --destination .work_meb_dual/reproduction_neuve
```

Les sources produit et les anciens reçus restent inchangés. Le [manifeste courant](../validation_current.json) et les [contrôles de publication](publication_checks.json) lient les preuves communes à la variante F reconnue ; ils ne changent pas ce prototype privé en produit intégré.
