# Apparié : composition des trois correctifs

8 octobre 2026. Proposition sur Git `0e62a7232` ; reçus précédents immuables.

Application sur une extraction propre de ce Git :

```sh
git -C EXTRACTION apply /CHEMIN_RECUS/apparie_livraison/composition.patch
git -C EXTRACTION apply /CHEMIN_RECUS/apparie_composition_triple/fermeture.patch
```

Le premier patch ferme identité et cohorte ; le second ajoute la fermeture du binaire.
SHA final du pilote : `1e2f1ea8c2ab8b98ac602b78ae9454a06034a8244b945292be61832586e81d70`.
Les deux `git apply --check` passent dans cet ordre.

L'AST vérifie les fonctions inchangées, le `main` exactement issu du correctif de fermeture,
et le juge de cohorte augmenté seulement des deux instructions de fermeture déjà proposées.
Les gardes de configuration passent d'abord ; les hashes initial et final sont comparés avant
les statistiques. Le hash final est acquis après la Session informative éventuelle.

Avec LF renforcé et les fixtures V=6 : nouveau positif conservé ; sonde modifiée entre appels,
hash final absent, cohorte vide et 12 journaux d'identité supprimés refusés. Deux campagnes Python
simulées seulement, sans répéter les quatre autres portes. Normal/−O identiques.

```sh
python3 -B check.py DEPOT
python3 -B -O check.py DEPOT
```

Sortie : `results.json`. Dépendances voisines épinglées ; aucun moteur, GCP ou donnée réelle.
La fermeture compare deux relevés ; elle ne prouve pas l'immuabilité continue.
