# Proposition de reprise ciblée des 41 occurrences ordinaires

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Cette proposition n'a pas été exécutée. Elle reprend les noms sans résultat de fina2 au commit `38b76701b9b0198fc1c37afe16e1480e638e513c` ; aucun verdict nouveau n'est acquis.

| Configuration | Profil | Occurrences | Fils conservés |
| --- | --- | ---: | ---: |
| gcc_release | u18 | 8 | 16 |
| bits21 | u21 | 11 | 4 |
| bits24 | u24 | 11 | 4 |
| poison | u21, MHGP11_POISON=ON | 11 | 4 |

Les 41 occurrences représentent 11 noms distincts de portes CLI `_opt`, listés exactement par configuration dans [scope.json](scope.json). Les options de compilation, exclusions initiales et délais restent ceux de la matrice source. Un `-R` ancré sélectionne seulement ces noms. Les planchers deviennent 8/11/11/11 et les labels exigés sont ceux réellement présents : `lidar`, `scale8000`, `scale16000`, `scale32000`. Les anciennes exigences `unit`/`fast` et la sonde sched, étrangères à cette reprise, ne sont pas rejouées.

La chaîne restante L → P10 → P9 → mesure ne sélectionne aucune de ces 41 occurrences. L a terminé les 28 portes non mutantes sélectionnées au même commit ; ses six absences sont des campagnes mutants. Aucune identité K10 n'est ajoutée ici. Les preuves sanitizer de finb/fins conservent leurs profils et ne deviennent pas des verdicts ordinaires `_opt` pour ces quatre configurations.

## Intégration indispensable

Le contrôleur refuse `python3 -c` dans un plan et le paquet exclut `receipts/`. Le plan utilise donc le script existant `tools/g4_matrix.py` avec une seule nouvelle matrice suivie. Aucune modification du validateur ni nouveau script produit n'est nécessaire.

1. Intégrer [g4_reprise_41.json](g4_reprise_41.json) sous `morsehgp3D_v11/tools/g4_reprise_41_20261005.json` dans le commit corrigé et poussé sur `main` retenu par le développeur.
2. Vérifier la présence de ce chemin au commit retenu, puis fournir [plan_reprise_41.json](plan_reprise_41.json) au contrôleur avec `--commit` et `--data build/v11-persist/qual_sorties/data_complet`.
3. Refaire le préflight gardé et choisir la durée de session selon ses gardes. La proposition conserve 2100 s pour la matrice et 2200 s pour la commande ; elle ne garantit pas le temps d'exécution et ne recommande aucune taille de VM.
4. Après fermeture certifiée de la session précédente, exécuter une session gardée sur la seule VM autorisée ; conserver les inventaires, verdicts par nom et arrêt ciblé certifié.

**Le pin actuel ne contient pas cette matrice : le plan n'y est pas exécutable sans l'intégration.** `validate_plan` accepte la forme mais ne vérifie pas l'existence du chemin auxiliaire `--matrix`. La preuve de forme ne constitue pas un préflight GCP ni une autorisation nouvelle.

## Reprises séparées après correction

Les six noms `api_supports_route` sont détaillés dans `scope.json` : douze occurrences ordinaires en u18/u24 et six occurrences ASan/UBSan u24 ont des attentes de hashes à corriger selon le profil. Garder les assertions métier et les comparaisons calcul/FULL/ordre, puis rejouer ces cas dans leurs profils ; ils ne font pas partie des 41 absences.

La voie M u18 doit ensuite rejouer seulement `mhgp11_mutants_api` et `mhgp11_mutants_cli` après correction de leur porte : 23 mutants API n'ont pas été jugés, car le témoin est refusé ; `sp_masque_16379` était inactif sur la voie supports après L2b et requiert une porte sensible à `order_params`, telle que `mhgp11_cli_points`. Exiger d'abord un témoin conforme puis les verdicts individuels. Il n'est pas nécessaire de relancer les autres campagnes déjà achevées pour cette reprise. Les résultats M u18 ne qualifient pas les campagnes L u21 ; une future voie L destinée aux identités K10 peut exclure le label `mutant` explicitement.

## Relecture portable bornée

Depuis ce dossier, `python3 -B replay.py --repo /workspaces/E-HGP` puis `python3 -O -B replay.py --repo /workspaces/E-HGP` appellent seulement `load_matrix` et `validate_plan` des sources Git épinglées, puis vérifient les sélecteurs sur les inventaires et verdicts fina2. Aucun calcul produit, test natif ou accès cloud n'est exécuté. Les archives locales de `/workspaces/.ehgp-sessions/v11.20261005.claudefina2` et l'objet Git épinglé sont nécessaires ; aucune donnée LiDAR ou sortie binaire n'est copiée dans ce dossier. Les sources, inventaires, reçu, archive et plans examinés sont identifiés par SHA-256 dans `scope.json`.

Les deux relectures ont terminé avec code 0 : forme du plan et de la matrice `PASS`, sélection exactement 8/11/11/11, union de 11 noms, intégration toujours requise. Une [contrelecture indépendante du second auditeur](independent_review.json) a confirmé ces mêmes ensembles et contraintes.
