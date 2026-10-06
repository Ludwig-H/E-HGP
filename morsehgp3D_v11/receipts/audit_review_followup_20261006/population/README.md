# Revue du seuil IoU de population

Au pin `ddb8d4ea9d485e6c0936f4f1101759e8afa13fb4`, `points_flat_study.one()` arrondit les meilleurs IoU hiérarchiques à quatre décimales avant que `summarize()` applique le critère strict `>0.5`. `10001/20001 > 1/2` devient `0.5` et est exclu à tort de la population. Le patch minimal retire ces deux arrondis ; il conserve le seuil strict et tous les autres calculs.

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux exécutions passent 39 contrôles, code 0, stderr vide et stdout identique. Les sources réelles sont extraites du pin Git et vérifiées par SHA256 ; le patch est vérifié puis appliqué uniquement dans un dossier temporaire. Aucun source développeur n'est modifié.

La preuve exécute les AST réels de `Evaluator`, `best_blocks`, des deux affectations causales de `one()` et de `summarize()`, avec de petits adaptateurs de bibliothèque standard pour `tolist()` et les moyennes scalaires. Elle couvre dessus/dessous/égalité au seuil, un IoU éloigné de la bande d'arrondi et le passage JSON. Les résultats sont synthétiques ; aucun impact sur une scène réelle, aucune qualification native ou nouvelle mesure n'est revendiqué. Voir `REPORT.md`.
