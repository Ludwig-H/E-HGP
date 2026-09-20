# Clôture des lectures — fenêtre q4, tranche 30

La qualification corrigée est close : **52 mesures appariées PASS et 91 CTests Release PASS**. Les dix commandes de lecture, normales et sous `python3 -O`, passent et donnent les mêmes résultats. Cette note distingue la reprise de la première qualification, dont l'échec est conservé.

## Captures finales

| Capture | Portée | Résultat |
| --- | --- | --- |
| [smoke_ohuhz56s](smoke_ohuhz56s/COMPLETION.json) | Release, 9 gates et 10 mesures | PASS, 19 commandes |
| [smoke_radsfh8d](smoke_radsfh8d/COMPLETION.json) | Clang ASan/UBSan, 9 gates et 10 mesures | PASS, 19 commandes |
| [scale_qv0aggg3](scale_qv0aggg3/COMPLETION.json) | Release, CPU 0, 9 gates et 32 mesures | PASS, 41 commandes |
| [regression_2xx82lty](regression_2xx82lty/COMPLETION.json) | Suite Release complète, `ctest --parallel 2` | PASS, 91 tests |

La clôture [readers_h76zdu5c](readers_h76zdu5c/COMPLETION.json), terminée le 20 septembre 2026 à 22:05:53 UTC, contient quatre lectures avec `--check-live` et l'autotest du lecteur, exécutés d'abord normalement puis avec `-O`. L'autotest rejette **46 corruptions** de reçus ; ce sont des contrôles du lecteur, pas 46 mutations géométriques. Les **189 sources, 90 fichiers d'entrée et 77 artefacts** restent identiques avant et après ; `closing_errors` est vide.

Commande exécutée depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v8/receipts/q4_window_20260920/close_reads.py \
 morsehgp3D_v8/receipts/q4_window_20260920/smoke_ohuhz56s \
 morsehgp3D_v8/receipts/q4_window_20260920/smoke_radsfh8d \
 morsehgp3D_v8/receipts/q4_window_20260920/scale_qv0aggg3 \
 morsehgp3D_v8/receipts/q4_window_20260920/regression_2xx82lty \
 --selftest morsehgp3D_v8/receipts/q4_window_20260920/smoke_ohuhz56s
```

Les commandes individuelles, sorties brutes et empreintes avant/après sont dans cette capture. SHA-256 de sa clôture : `ba25546dac211ae8002d54e96cc80fb78ffad4bdb56e9e65f9b32c81b5c7b7dc` ; de son résumé : `0b94fa1798b7f623595de299f83f0e9bd934dcdbd1a6f0a8f8a0a119d1a2514d`.

## Échec initial conservé, correction et reprise distincte

Les premières captures `smoke_t17k5a2a`, `smoke_7e8pmosr` et `scale_5ph5gh2r` contiennent elles aussi 52 mesures PASS. Mais [regression_9a8cwnny](regression_9a8cwnny/COMPLETION.json) a **échoué : 90/91**, sur `mhgp8_wspd_q2_dynamic_receipts_gate_optimized`, avec `mutant survived: worker_digest`. Remplacer arbitrairement un digest par zéro ne le modifiait pas lorsque sa valeur initiale était déjà zéro. Ce constat ne démontre pas qu'un worker particulier était inactif.

La première clôture [readers_fvjcjci_](readers_fvjcjci_/COMPLETION.json) est elle aussi conservée en échec : trois lectures normales passent, la quatrième rejette correctement cette régression ; ni les lectures optimisées ni l'autotest ne sont alors exécutés. Son lancement après un précontrôle déjà négatif était une erreur d'orchestration, pas un défaut du lecteur. Ses fichiers n'ont été ni écrasés ni promus en succès.

Le [correctif documenté](preflight/WORKER_DIGEST_FIX.md) rend la corruption du digest réellement différente de sa valeur initiale et vérifie les cas zéro, un et entier non signé maximal. L'unique source modifiée dans l'inventaire de 189 fichiers est `tests/wspd_q2_dynamic_receipts_gate.py` :

- ancienne empreinte : `a4b0428cbd66d03f70b316946498646f4e81fd40272526dd4b6fecc0ea9532fd` ; [source archivée](preflight/wspd_q2_dynamic_receipts_gate_before_worker_digest_fix.py) ;
- nouvelle empreinte : `83b231df83432fed6bee6a11d7319ca5272bc6f7fa08b0094230501257116250`.

Aucun moteur, binaire ou build épinglé n'a changé pendant cette reprise. Les quatre nouvelles captures ci-dessus qualifient le nouvel instantané de sources ; elles ne réécrivent pas le résultat initial 90/91.

## Comparaison des deux campagnes de croissance

[SCALE_REPRISE_COMPARISON.json](SCALE_REPRISE_COMPARISON.json) ferme la comparaison des 32 configurations : **tous les champs sauf `timings` sont identiques**, notamment travail, sorties, digests, capacités et validations. Les artefacts sont identiques ; la seule différence de source est le correctif Python décrit ci-dessus. La comparaison conserve les empreintes des 86 fichiers d'entrée et des 189 sources avant/après.

Les heures et chronométrages restent distincts : la première campagne commence à 21:52:07 UTC, la reprise à 22:02:26 UTC. Exemple, `dense_permuted`, K10, temps du moteur incluant son callback, en millisecondes :

| n | Fenêtre 30, initial | Fenêtre 30, reprise | Référence 29, reprise |
| --- | ---: | ---: | ---: |
| 8 000 | 14,271316 | 14,429521 | 52,632224 |
| 16 000 | 36,485009 | 33,669607 | 167,793609 |
| 32 000 | 84,301650 | 84,220839 | 565,258910 |

Ce sont des observations locales sur hôte partagé, pas un gain stable garanti. À 32k, la reprise mesure 100,808295 ms pour la somme préparation commune + moteur fenêtre, contre 581,846366 ms pour celle de la référence ; ces deux sommes partagent la même préparation, ce ne sont pas deux temps mur indépendants. Le temps total de la sonde paie les deux moteurs et le juge.

## Ce que ces lectures ne prouvent pas

La gate géométrique propre à la fenêtre passe **6 206 contrôles** en Release et sous ASan/UBSan. Les grandes sondes comparent réellement les sorties q4 normalisées avec le moteur 29 et contrôlent indépendamment les boules émises par calcul rationnel et census une fois par boule distincte. Cela ne transforme pas le juge des émissions en oracle exhaustif de complétude sur les grands nuages.

Sur `dense_permuted` K10, les visites des deux passes valent 208 488 / 596 136 / 1 829 992 et les comparaisons de racines, tas et fenêtre compris, 352 569 / 884 540 / 2 446 344. Mais le premier balayage reste proportionnel au produit graines × sites retenus. Le contre-régime `adversarial` K10 conserve 960 / 3 968 / 16 128 / 65 024 premières visites pour n32/64/128/256, soit des rapports **×4,133 / ×4,065 / ×4,032**. Le lecteur publie aussi les autres rapports supérieurs à quatre ; aucune borne générale sous-quadratique n'est acquise.

Les six buffers du balayage fenêtre occupent 352 / 416 / 320 octets dans la série dense permutée K10, contre 4 128 / 8 224 / 16 416 pour le balayage 29. Toutefois le pic interne couplé reste 513 454 / 995 363 / 1 861 533 octets, dominé par la sélection commune. Ces mesures de capacités ne sont ni le RSS ni une somme de pics indépendants.

Enfin, cette qualification porte sur **q4 pour une arête fournie**, pas sur toutes les arêtes, q3, le catalogue global ou la tour des hiérarchies. Elle ne clôt ni le contrat 50k sur G4 ni le passage aux dizaines de millions de points. Aucun test GCP n'a été réalisé dans cette tranche.
