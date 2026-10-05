# Finm : mutant CLI survivant devenu inactif pour sa porte supports

Pin produit lu : `38b76701b9b0198fc1c37afe16e1480e638e513c`. Archive finm close, SHA-256 `3be572c878600e88c27410269a75aef068e6b702651f85ef50db6ed08c7639cb` ; arrêt ciblé déclaré par le root à 21:15:46 UTC. Aucune compilation, exécution native ou action cloud par cet auditeur.

`tests/mutants/cli.json:225–231` définit `sp_masque_16379` : supprimer `params.concurrent_orders = false` dans `src/api/compute.cpp:54`, puis jouer `mhgp11_cli_supports_oracle`. Finm constate 27 mutants CLI tués et celui-ci survivant ; cet extrait réel est conservé dans `cli_failure_excerpt.txt`. Ce statut est un échec du lot de mutants, pas une preuve d'erreur géométrique.

Depuis L2b, la mutation n'est plus atteinte par cette porte :

- `src/api/compute.cpp:212` transmet `kSupportsRoute` pour un `SupportsRequest` public ; `src/api/internal.hpp:37` fixe ce choix à `full_tower`.
- `src/api/compute.cpp:129` sélectionne alors `build_order_full(..., full_params(), ...)`. La mutation ne touche que `order_params()` ; les paramétrages FULL restent identiques.
- `tests/cli/cli_supports_oracle.py:210,212,236,248` publie uniquement supports et full. Sa comparaison à Fraction, ses refus de coquille et ses signatures restent utiles, mais ne peuvent observer ce changement d'une fonction devenue inactive sur leurs chemins.

Le mutant est donc équivalent pour sa porte actuelle, pas pour toute l'API. Points et plat passent par `order_tree` sans argument de voie (`compute.cpp:164,184`) : la voie par défaut est `order_tree` (:119), elle appelle `build_order(..., order_params(), ...)` (:131). Après la suppression, `concurrent_orders` hérite de `true` dans `full_params`. `src/tower/order_tree.cpp:100` refuse explicitement ce paramètre avec `parameter_out_of_range`.

## Raccord minimal recommandé

Conserver la mutation, mais la renommer par exemple `points_ordres_concurrents`, lui donner une note correspondant au refus de la sortie points et remplacer sa porte par **`mhgp11_cli_points`**. Cette cible existe dans `tests/cli/tests.cmake:126–128`, et la construction du lot inclut déjà `cli`. `cli_points.py:58` exige code 0 et succès pour les appels admis ; sa petite suite impose au moins 40 cas admis (:185). Le mutant doit donc rencontrer un refus contraire à une attente de succès sur un chemin effectivement muté.

Le moteur et l'oracle supports restent inchangés, le nombre/plancher de mutants CLI reste 28. Ce raccord proposé n'a pas été exécuté par l'auditeur : le témoin non muté puis la copie mutée restent à juger dans le harnais existant après correction. Le mutant API distinct `voie_supports_order_tree` vérifie déjà le choix public de la voie supports ; finm ne l'a pas jugé, car son témoin de calibration u18 échoue sur les SHA u21 gravés (constat séparé du lot API).

## Preuve bornée

```sh
python3 replay.py
python3 -O replay.py
```

Sorties identiques à `static_proof.json`, SHA-256 `0772ed7dc41cbf094c8ee3a09d35c859cb693542cb5b034a5b801b3bbcd0300a`. Ce script stdlib contrôle les empreintes des sources figées, la suppression unique, les branches pertinentes, l'inventaire des quatre publications de l'oracle supports via AST Python, la cible points enregistrée et le survivant réellement consigné. Il ne compile ni n'importe les scripts du dépôt. Le modèle de chemins est borné à ces façades et à ce mutant ; aucune qualification native du nouveau raccord n'en découle. Aucun nouveau défaut moteur ni porte de correction supports manquante n'est établi.
