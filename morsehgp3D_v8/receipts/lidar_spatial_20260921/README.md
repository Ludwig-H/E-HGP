# Préparation LiDAR spatiale — 21 septembre 2026

Demande utilisateur : scène complète, deux moitiés, puis quatre quarts,
à densité conservée. [Protocole](../../docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md).
Ces reçus valident des **entrées**, pas un temps de moteur, une croissance
algorithmique, ni une tour FULL/G4. Aucun coût GCP.

La capture [spatial_tbmhj_zx](spatial_tbmhj_zx/COMPLETION.json) est close :
11 commandes, toutes réussies ; quatre sources et trois scans bruts
inchangés ; 78 artefacts vérifiés et aucune erreur de fermeture. Les scans
bruts sont consommés en lecture seule depuis les données locales de l'audit
LiDAR08. Cela ne transfère aucun verdict de cet audit au constructeur.

## Objets préparés

Grille globale fixe20mm, repère propre de chaque capteur, sites distincts
ordonnés lexicographiquement, puis coupes x/y à l'origine. Ordre des quarts :
x−y−, x−y+, x+y−, x+y+ ; zéro appartient au côté non négatif.

| Scan | Retours bruts | Scène quantifiée | Deux moitiés | Quatre quarts |
| --- | ---: | ---: | --- | --- |
| 0 | 123 389 | 119 142 | 59 189 / 59 953 | 29 128 / 30 061 / 30 027 / 29 926 |
| 100 | 124 479 | 119 942 | 59 059 / 60 883 | 28 829 / 30 230 / 30 645 / 30 238 |
| 200 | 125 526 | 120 725 | 59 366 / 61 359 | 29 187 / 30 179 / 31 079 / 30 280 |

Chaque scène conserve17 fichiers : manifeste, clôture, correspondance
raw→site global, puis sept nuages et leurs sept correspondances local→global.
Les deux moitiés et les quatre quarts recomposent exactement la scène,
sans recouvrement. Chaque paire de quarts recompose sa moitié.

La grille fusionne respectivement4247/4537/4801 retours ; tous restent
adressables dans la correspondance. Elle change le quadrant brut de
71/65/87 retours et fusionne11/7/9 sites contenant des retours de quadrants
bruts différents. Ces valeurs sont publiées dans chaque manifeste, pas
assimilées à une conservation exacte de la géométrie float32 brute.
Les effectifs100/200 diffèrent des anciennes préparations recalées sur0 :
ici les scans restent dans leur propre repère capteur.

## Vérifications exécutées

La [porte](../../tests/lidar_spatial_gate.py) passe11 tests en normal et−O,
avec des sous-cas pour les frontières d'arrondi et les entrées non finies :
arrondi contre rationnels indépendants, frontières et zéro signé, tous
les mappings, morceaux vides, absence d'écrasement, corruption de chacun
des15 payloads binaires, source/manifeste altérés, hashes falsifiés,
échec d'écriture tardif et fermetures de hashes. La détection des erreurs
ne dépend pas des assertions Python désactivables.

Les trois scènes sont ensuite préparées et relues deux fois, normal/−O.
Chaque lecture reconstruit le nuage et les sept partitions depuis tous
les retours bruts et compare les octets, pas seulement les hashes. Les
deux résultats par scène sont identiques. La capture finale lie encore
chaque artefact aux hashes de ces lectures et inclut les trois fichiers
de clôture de scène. Sources et commandes exactes sont dans le reçu.

Le lanceur réutilise explicitement le collecteur de processus P0 pour
annuler et joindre une commande interrompue ; ce reçu n'est pas une
nouvelle qualification exhaustive de toutes ses branches d'interruption.
Les11 tests supplémentaires sont une porte Python autonome : aucun
build C++ épinglé n'a été reconfiguré, les96CTests34 restent leur capture
distincte. Prochaine étape : raccord des mesures moteur à ces manifestes,
puis six comparaisons parent/enfant par scène avec leurs effectifs réels.
