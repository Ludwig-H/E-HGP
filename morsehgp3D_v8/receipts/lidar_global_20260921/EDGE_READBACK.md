# Arêtes LiDAR : 144 mesures constructeur closes

Capture [edge_scale_gom3hgqp](edge_scale_gom3hgqp/COMPLETION.json), sous `taskset -c 0` : **144 mesures PASS**, 196 sources et 16 fichiers d'entrée épinglés avant/après. Neuf arêtes originales, scans 0/100/200 transformés dans le repère `LiDAR_scan_000000`, préfixes 8k/16k/32k/50k, K5/10, ordres 28→29→30 et 30→29→28. Chaque voie est réellement appelée et paie sa propre préparation ; nuage/index/cover communs restent mesurés séparément.

Les 72 paires d'ordre ont **tous leurs champs hors temps identiques**, sorties complètes comprises. Le constructeur reconstitue rationnellement les neuf boules-cibles, vérifie leur propriété d'arête avec les IDs originaux, puis effectue 954 000 tests de census sur les 36 préfixes-cibles. Cela donne 76 observations de cible conservée et 68 de cible retirée, exactement comme prédit par leur profondeur ; 60 mesures n'émettent aucun q4, sans échec ni résultat silencieusement supprimé.

Commandes exécutées, chacune normalement et avec `python3 -B -O` :

```sh
python3 -B morsehgp3D_v8/bench/run_q34_lidar_checks.py read morsehgp3D_v8/receipts/lidar_global_20260921/edge_scale_gom3hgqp --check-live --compact
python3 -B morsehgp3D_v8/bench/run_q34_lidar_checks.py selftest morsehgp3D_v8/receipts/lidar_global_20260921/edge_scale_gom3hgqp
```

Les quatre commandes sortent avec le code 0 : lectures concordantes, **26 corruptions de reçus refusées** dans chaque mode. Ce nombre ne désigne pas des mutations du moteur géométrique. Les manifestes, sorties brutes, empreintes et fermetures sont ceux de la capture ; ces lectures ne recapturent pas les exécutions natives.

## Résultat et limites

Les covers vont jusqu'à 147 sites seulement. Sous la charge concurrente présente pendant cette campagne, la fenêtre30 est plus rapide que les blocs28 sur 55/144 mesures ; le rapport médian temps30/temps28 vaut 1,153. Les maxima des intervalles propres par arête sont 0,292 ms (28), 1,217 ms (29) et 0,590 ms (30). Ce sont des observations, pas un gain de vitesse stable.

Les blocs restent utiles : pour la fixture8 à 50k/K10, ils ne visitent aucun site dans les balayages locaux, contre 3 132 visites pour29 et30, sans sortie q4 dans les trois cas. Inversement certains petits covers coûtent davantage à préparer en28. Il n'est pas justifié de remplacer universellement28 par30.

La croissance n'est pas uniformément sous-quadratique : pour la fixture2/K10, le premier balayage29/30 compte 60 / 363 / 972 visites à 8k/16k/32k, soit ×6,05 puis ×2,678. Le lecteur publie tous les compteurs, capacités et temps, avec les rapports et le seuil quadratique adapté à chaque pas (50k/32k n'est pas un doublement). Une telle série sur une arête fixée n'est ni une preuve asymptotique ni la mesure du producteur global.

Le juge C++ vérifie chaque support publié et effectue un census exact global une fois par boule distincte après égalité complète des trois listes. Cela prouve la validité des émissions contrôlées, pas la complétude indépendante sur ces grands nuages. Les cas exhaustifs restent séparés. Aucun contrat de tour 50k/G4, catalogue, FULL ou résultat GPU n'est acquis ici ; GCP non utilisé.

Le [préflight195](preflight/READBACK.md) est conservé séparément, sans promotion après changement de son instantané de sources.
