# P0 — affectation de plan interrompue par une allocation

13 septembre 2026. Audit complémentaire du commit **3589a2c9**,
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`. Le développeur
modifie séparément le partage entre voies ; ce constat vise les cinq
sources du commit indiqué, extraites dans un répertoire privé.

**Défaut conditionnel : après `std::bad_alloc` pendant l’affectation par
copie d’un `CreditPlan`, réutiliser la cible peut mélanger propriétaire
et crédits.** Il ne s’agit pas d’une régression géométrique des exécutions
normales. L’appelant qui abandonne la cible après l’exception n’exerce pas
ce défaut. Le risque devient une perte de cohérence, voire un accès hors
limites, lorsque la cible est réutilisée après interception de l’exception.

## Ce que R3 corrige effectivement

La lecture de [la factory](../../morsehgp3D_v8/src/pipeline/local_credits.cpp)
confirme la copie privée des coordonnées et propositions avant validation.
Les bornes, l’unicité et les crédits du cœur sont calculés sur ce stockage.
Les plans partagent ensuite son propriétaire, sans refaire la validation
du nuage. Les quatre opérations de copie/déplacement de `PreparedRectangle`
sont supprimées. Les anciens défauts de copie du propriétaire et d’alias
du vecteur d’entrée restent sous l’autorité de
[l’autre auditeur](../../morsehgp3D_v8/audits/DIALOGUE_COURANT.md) ; cette note
ne duplique pas leurs contre-fixtures et ne les considère pas encore ouverts
dans les octets R3 examinés.

## Déclencheur nouveau et reproduction

`CreditPlan` reste affectable par copie implicitement. L’ordre des membres
dans [le header](../../morsehgp3D_v8/src/pipeline/local_credits.hpp) fait
affecter `rectangle_`, la voie et le seuil avant les vecteurs de crédits.
Une allocation échouée dans `a_` laisse donc les membres déjà affectés
attachés au nouveau propriétaire et les autres issus de l’ancien plan.

Le [juge](plan_assignment_probe.cpp) utilise une cible q2/K1 à deux sites
par facteur, puis une source q4/K10 à trois sites par facteur. L’affectation
normale est d’abord vérifiée. Il injecte ensuite une seule panne au premier
`operator new` de `target = source`, puis intercepte `std::bad_alloc`.

| Après l’exception | Version publiée | Réparation temporaire |
| --- | ---: | ---: |
| Propriétaire remplacé | oui | non |
| Tailles des facteurs du propriétaire | 3 / 3 | 2 / 2 |
| Tailles des tableaux de crédits conservés | 2 / 2 | 2 / 2 |
| Paires annoncées | 1 | 1 |
| Émissions hors des plages du propriétaire | 1 | 0 |

Dans la version publiée, le consommateur reçoit une paire dont le second
ID n’appartient plus au facteur B du propriétaire annoncé. Le juge ne
déclenche aucun accès hors limites : il compare les tailles et les IDs.
Un appel ultérieur `keeps(2, 3)` passerait le contrôle des plages du nouveau
propriétaire, puis indexerait un tableau de crédits resté de taille deux.

## Correction constructive, portée bornée

Si l’affectation par copie est utile, construire d’abord une copie temporaire,
puis échanger **tous** les membres sans exception donne une garantie forte :
soit la copie aboutit, soit la cible reste inchangée. Le runner applique ce
`copy-and-swap` uniquement à une copie temporaire du header et conserve les
opérations de déplacement explicites. Supprimer l’affectation par copie est
une autre option si l’API n’en a pas besoin. Les futurs conteneurs de plans
doivent garder la même garantie lors de leurs propres affectations.

Le [reproducteur](plan_assignment_checks.py) épingle le commit, les cinq
sources, le juge, le runner, le compilateur et les exécutables. Deux builds
C++20 stricts sous UBSan confrontent le défaut à sa réparation temporaire,
avec contrôle positif et panne réellement exercée. Les sorties et codes
sont dans [PLAN_ASSIGNMENT_CHECKS](PLAN_ASSIGNMENT_CHECKS.json).

```bash
python3 -B audits/morsehgp3D_v8_complementaire/plan_assignment_checks.py
python3 -B -O audits/morsehgp3D_v8_complementaire/plan_assignment_checks.py
```

Le reçu concerne cette garantie d’exception ; il ne qualifie ni le nouveau
partage entre voies, ni WSPD, ni le coût aval ou la tour FULL. Aucun fichier
du développeur ou de l’autre auditeur n’est modifié. GCP non utilisé.
