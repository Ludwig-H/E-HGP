# q4 à profondeur non nulle, sans face q3 admissible

Cette porte complète la [fixture à douze sites et K3](../q4_global_12sites_20260923/README.md). Quatre sommets réguliers ont centre `O=(20,20,20)` et rayon carré `300`. Pour chaque face q3, deux témoins distincts sont extérieurs à la boule q4 mais strictement intérieurs à la boule de cette face. On ajoute successivement `p=1,2,7` sites distincts proches de `O`, strictement intérieurs aux cinq boules. L'[oracle Fraction](run.py) vérifie les signes et les profondeurs exactes :

| Sites | K | Profondeur q4 | Profondeur de chacune des quatre faces q3 |
|---:|---:|---:|---:|
| 13 | 4 | 1 | 3 |
| 14 | 5 | 2 | 4 |
| 19 | 10 | 7 | 9 |

Dans chaque cas, `p=K−3<K−2` rend la présentation q4 admissible, tandis que les faces ont `p+2=K−1` sites intérieurs et sont rejetées. À `K−1`, cette même clé q4 n'est pas encore admissible. Les six arêtes du support ont la même longueur ; deux ordres complets d'IDs exercent le départage du propriétaire. Toutes les coordonnées sont des entiers u18 distincts.

[`check.cpp`](check.cpp) compare **tout le flux** q3/q4 — clé, support, profondeur et coquille — à l'énumération rationnelle indépendante du gate produit `tests/gen/wspd_q34_gate.cpp`, dont seul le symbole `main` est renommé dans une copie temporaire. Les 76 flux couvrent les trois valeurs de `p`, les deux ordres d'IDs, `s=8/10/12` sur Local28 et sur le preset optimisé (filtre de rectangles, cœur diamétral, cache de témoins, atlas q3 et voie morte), Window30 à `s=8`, le seuil précédent à `s=8`, et W1/W4 parallèles pour l'optimisé à K5/K10 (W4 aussi à K4). Il n'y a pas de modification du code produit.

Le [reçu Release](receipt.json) est `PASS` à la source `5011653d0` : **11 184** tétraèdres parcourus, **1 798** boules rationnelles distinctes, **8 628** assertions et **76** flux égaux. Un rejeu sous `python3 -O`, sans réécrire le reçu, donne les mêmes valeurs et hashes. Le lanceur contrôle la racine source du build CMake, reconstruit `mhgp9_gen`, vérifie la propreté des sources du générateur et de son gate avant/après, et fixe les hashes du cache CMake, du gate, du sidecar, du lanceur, de la bibliothèque avant/après et du binaire. Le premier essai contre une bibliothèque locale compilée avant les dernières modifications a échoué sur l'ABI des options parallèles ; ce n'était pas une divergence géométrique. La reconstruction propre dans `/tmp/mhgp9-q4-depth-build` a levé cet échec. Le reçu dépend encore de ce build et de ses en-têtes **LIVE** : il ne constitue pas une archive binaire autonome.

Pour rejouer, configurer un build Release CMake de la même source avec les en-têtes Boost requis, puis lancer :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v9/audits/q4_depth_ladder_20260923/run.py \
  --source-root build/v9-open-worktree/morsehgp3D_v9 \
  --build /tmp/mhgp9-q4-depth-build
```

Ce gate ferme la lacune ciblée des **profondeurs q4 non nulles au seuil**, y compris K10, sur de petits nuages adversariaux. Il ne prouve pas la complétude universelle de l'induction q4, du catalogue, de la tour FULL, ni le contrat LiDAR/G4.
