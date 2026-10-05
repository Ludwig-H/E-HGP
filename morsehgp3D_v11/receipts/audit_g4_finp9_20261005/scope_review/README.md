# Portée P9

Lecture indépendante au pin `38b76701b9b0198fc1c37afe16e1480e638e513c` et contrôle de l'archive directe de `v11.20261005.claudefinp9`. Aucun build, programme natif ni appel cloud exécuté par cet audit.

Les quatre sorties directes montrent chacune un PASS CTest et un code 0. Elles ne conservent aucune ligne `points_vs_python_verdict` : les minima sont une conséquence du contrat du juge et de son code attendu, pas des compteurs métier directement archivés. Aucun total exact historique n'est réattribué à P9.

Le juge compare tous les sites (IDs dans l'ordre de Morton, dates exactes, propriétaire, plancher et strict), toute la forêt (parents/rangs), les plateaux et tous les blocs/entrées. Il compare les niveaux utilisés en Fraction. La chaîne Python utilise le MHGP11PH produit par le moteur natif : cette porte n'est pas un nouvel oracle indépendant de construction géométrique FULL. Les colonnes XYZ et les champs k/m du dump ne font pas l'objet d'une égalité explicite dans `compare` ; les deux programmes reçoivent cependant les mêmes fichiers et paramètres.

La porte synthétique fixe 400 nuages aléatoires, quatre témoins, trois uniformes (300/2000/8000), K=1..5 et m=1 ou K+1 à K≥2. Les portes LiDAR fixent K5, m6, W8 et lisent intégralement les trames sans sol 08/000000, 000100, 000200 (39885/35551/45845 sites, entrée entière après masque, u18 dans build u21). Elles ajoutent les quatre témoins fixes : les deux de 5 et 4 sites sont omis à K5 par la frontière K≥n ; les deux de 6 et 8 sites sont traités. Le compteur `clouds` compte également les témoins dont tous les ordres sont omis et ne doit pas être interprété comme un nombre de pendaisons réalisées.

Le seul saut CTest admis est le précontrôle d'un dossier LiDAR absent. Ici les sorties disent `Passed`, sans `Skipped`. Un fichier absent, un refus, un écart, un timeout ou un minimum non atteint ne donnent pas un code 0. Le code du juge n'utilise pas `assert`. Les portes `long` ne possèdent pas de jumelle optimisée : P9 ne prouve pas une exécution de ce différentiel sous `-O`.

`scope.json` sépare observations directes, conséquences du contrat et limites. `replay.py` vérifie les empreintes/ancrages Git et les quatre sorties de l'archive, sans rejouer les calculs :

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP --archive /workspaces/.ehgp-sessions/v11.20261005.claudefinp9/results/results.tar.gz
```
