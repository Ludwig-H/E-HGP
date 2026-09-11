# Journal incrémental privé : anciennes coupes et lots mixtes

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le lemme de stabilité a maintenant une gate indépendante réussie O2/SAN.**
Elle qualifie uniquement le prototype structurel privé : journal `b526b895…`,
propriétaire `76885ecd…`, issus de la variante constructeur sur `ad7ffd28`.
Les [dix headers effectivement consommés](source_pins.json) sont conservés
dans une [archive autonome](source_snapshot.tar.gz). Aucun raccord au
producteur FULL, verticale calculée, gain mémoire ou latence n’est qualifié.

## Résultat et indépendance

[prefix_gate.cpp](prefix_gate.cpp) reprend les lots `structural_mixed` du
[corpus historique](../receipts_coverage_cpp_20260910/corpus.py) : cinq
naissances, deux fusions ternaires, une naissance + continuation + fusion
dans le même lot, recouvrements et contributions répétées. Les niveaux sont
petits ou larges, avec deux encodages rationnels équivalents ; les coupes
emploient encore un autre encodage.

Les attentes sont des tableaux explicites de racines, couvertures, dates,
parents et contributions, indépendants du code candidat. Chaque préfixe
est construit puis scellé par un propriétaire neuf ; aucune vue mutable,
aucun accès privé et aucune publication intermédiaire ne sont ajoutés.
La comparaison à la façade partageant le même noyau est un contrôle
supplémentaire d’API, **pas l’oracle**.

Les sorties [O2](o2_mutants.json) et [ASan/UBSan](san_replay.json) sont
identiques : **13 905 contrôles**, quatre encodages, 20 préfixes scellés,
5 616 requêtes racine/couverture et 260 paires de coupes historiques.
24 transitions autorisées de successeurs sont observées entre préfixes ; 16 coupes
fermées changent effectivement, tandis que le dernier lot, redondant,
conserve sa couverture. Les 16 suffixes invalides empoisonnent tout le
propriétaire, y compris après fermeture d’un ordre précédent : sortie vide
et reprise refusée.

Trois mutants du snapshot sont compilés et refusés avec le code 1 :

| Mutation | Diagnostic de la gate |
| --- | --- |
| Suivre un successeur futur sans tester sa date | `prefix.old.cut.immutable` |
| Dater la contribution au niveau initial de son segment | `contribution.level` |
| Rendre immédiatement réutilisable le parent d’une continuation | `late.parent.or.level.rejection` |

Le premier [essai SAN](san.json) conserve son refus LeakSanitizer sous
ptrace ; le rejeu hors sandbox passe avec détection des fuites activée.
Les mauvais arguments donnent le code 2. Sources et gate sont stables
pendant chaque capture. Aucun test de panne d’allocation nouveau ici :
les refus exercés sont sémantiques.

## Reproduction et suite utile

```bash
python3 -B morsehgp3D_v7/audits/receipts_incremental_prefix_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_incremental_prefix_20260911/verify.py
```

Le lecteur vérifie les empreintes, les sources archivées, les commandes,
les options de compilation et de sanitizers, les compteurs non vacuants,
les diagnostics causaux et l’identité O2/SAN.
Il ne consomme pas le moteur actif. [record.py](record.py) permet un rejeu
avec `--name` inédit et `--mutants` ou `--san` ; les commandes exactes et
l’environnement du compilateur sont dans les captures. Les binaires et
sources extraites sont des fichiers de travail ignorés, non distribués.

La [preuve précédente](../receipts_incremental_review_20260911/README.md)
et son calcul de résidence restent historiques et inchangés. Cette gate
ferme sa demande de régression sur le prototype épinglé. Lors du raccord
FULL, il restera à vérifier la même histoire avec les ancres verticales et
les journaux effectivement produits ; aucun préfixe public n’est requis.
GCP non utilisé.
