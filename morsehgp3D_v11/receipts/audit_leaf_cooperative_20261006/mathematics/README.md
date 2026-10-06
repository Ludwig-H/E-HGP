# Feuille coopérative : ordre et témoin q3→q4

Contrelecture mathématique de la section O au pin `3b76a3fcf0ca14dd08f005e1e1ae8e3418dd247e`. Aucun code natif, build, GPU ou GCP exécuté ; aucune qualification de la nouvelle implémentation coopérative en cours n'est revendiquée.

Le découpage par paire est compatible avec le parcours existant si chaque branche conserve ses coupes, son curseur après `j`, l'ordre de ses faces J2, son recensement croissant et l'ordre de ses descendants. Les paires peuvent être exécutées dans un ordre arbitraire ; concaténer leurs sorties en ordre `(i,j)` restitue le DFS complet. Les bits J2 servent uniquement aux compteurs dans la source épinglée.

Témoin discriminant proposé : quatre sites en ordre Morton `(5,2,1), (10,5,5), (9,8,5), (1,5,8)`, boîte `[0,16)^3`, `K=3`. Le préfixe q3 `(0,1,2)` est obtus, mais q4 `(0,1,2,3)` est le support positif minimal de centre `(5,5,5)`, rayon carré `25`. La descente q4 doit subsister après le retour sans émission de q3.

Rejeu depuis tout dépôt contenant le pin :

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux exécutions archivées ont le code 0 et des sorties identiques : 213 contrôles, dont 96 comparaisons de parcours borné. Aucun essai en échec. Voir `REPORT.md` pour la portée : les événements du modèle sont des tentatives q2/q3/q4, pas des boules émises par le produit ; le témoin q4 est, séparément, établi par calcul rationnel exact.
