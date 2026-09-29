# Reçu : frontière du catalogue pilotée par la charge (29 septembre 2026)

`public_status=not_claimed`. Changement d'implémentation sans changement d'objet (étape J1 du plan de performance,
`gpu_design/juge/PLAN.md` hors dépôt).

## Motif

Sur G4, le catalogue de la trame LiDAR 02 à K = 5 passait de 1,19 s à 24 fils à 1,11 s à 48 fils. La frontière de
l'arbre des boîtes s'arrêtait dès 64 tâches par fil, sans regarder leur charge. Or le coût d'une tâche suit le nombre
de sites de sa boîte (corrélation 0,99 avec les boules jugées), et la densité LiDAR est très concentrée : une
cellule de 2 m porte 6,3 % des sites de la trame 02. La tâche la plus lourde fixait donc le temps à 48 fils.

## Ce qui change

- Après la cible de 64 tâches par fil, la frontière ne développe plus que les tâches dont la boîte contient plus de
  n / (64 P) sites.
- Les tâches finales partent par nombre de sites décroissant (tri stable, ordre approché de la plus longue tâche
  d'abord).
- `Local` est aligné sur 64 octets.
- `catalogue_stages` publie le nombre de tâches et les sites de la plus chargée.

L'arbre ne change pas, car la frontière n'en est qu'une coupe, et la sortie suit l'ordre canonique : catalogue et
grand livre sont indépendants de la répartition.

## Contrôles

- `differentiel.txt` : dumps canoniques complets, binaire `c764e121a` contre le nouveau à 3 fils et à 1 fil, sur 10
  entrées (deux trames LiDAR entières, un quart de trame, deux synthétiques, K = 5 et 10). Tous identiques, grand
  livre compris.
- Le dixième cas (`syn_filaments_space_x4`, K = 10, 1 fil) apparaît « différent » dans `differentiel.txt` : le disque
  était plein et son dump a été tronqué (JSON vide). Rejoué dans `/tmp`, il est identique (`b9719e2166bb7c92` aux trois
  exécutions).
- Plus lourde tâche après J1 : 238 sites sur 45 845 pour la trame 02 (0,5 %), contre environ 6 % avant.
- Portes v10 vertes, 8 sur 8.
- La mesure de l'accélération sur 48 fils se fera en session G4 : sur ce codespace chargé, le temps mural n'est pas
  une mesure.
