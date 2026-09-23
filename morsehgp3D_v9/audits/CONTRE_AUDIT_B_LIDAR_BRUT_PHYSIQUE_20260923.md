# Contrelecture B — première trame brute et découpes physiques

23 septembre 2026. Relecture indépendante du
[reçu A](lidar_raw_physical_scaling_20260923/README.md) publié en
`aa2528d2`. Aucun calcul HGP supplémentaire ni GCP par B. Cadre :
une seule trame SemanticKITTI **08/000000 avec sol**, 123 389 retours
distincts, grille entière **1 mm** (pas float32 natif), K1..5,
s8/W8 CPU local partagé `nice 19`, une exécution par cas.

## Réception et construction des entrées

Les **26/26** empreintes du reçu passent. Les sources v8 de la trame
brute et le binaire v12 sont présents localement avec leurs SHA
attendus. Un rejeu en lecture seule du préparateur reconstruit le
manifeste et les SHA des **63 payloads** (21 nuages × trois fichiers
coordonnées/IDs) ; le lecteur rejoué reproduit `SUMMARY.json`
**octet pour octet** à partir des neuf sorties brutes. Les deux
correspondances raw→full sont bijectives ici. Les demi-plans `x=0` et
`y=0` sont pris sur les signes des coordonnées float32 **avant** la
grille, conformément à la coupe physique demandée ; les trois IDs
qui changeraient de secteur après arrondi sont correctement préservés.
Les densités globales sont emboîtées par ID original
`30 847 ⊂ 61 694 ⊂ 123 389`, puis intersectées avec les secteurs.
La préparation Python reste **hors chrono**.

Les neuf sondes disent `complete_relative` : ce statut contrôle le
catalogue émis mais pas les clés jamais produites. Le rejeu du lecteur
et des entrées n'est pas une preuve indépendante de complétude HGP.

## Ce que montrent réellement les coûts

| sites de la trame brute | chaîne locale W8 | CPU·s | q3/q4 | formes du cœur effectivement chargées |
| ---: | ---: | ---: | ---: | ---: |
| 30 847 | 9,318 s | 49,920 | 7,005 s | 35,46 M |
| 61 694 | 20,486 s | 116,088 | 15,686 s | 125,48 M |
| 123 389 | 48,357 s | 285,329 | 38,942 s | 551,69 M |

Les pentes effectives des formes sont **1,823 puis 2,136** aux deux
doublements ; celles du CPU sont **1,218 puis 1,297**, du temps de
chaîne **1,137 puis 1,239**. Le deuxième doublement dépasse donc n²
pour une **opération réellement payée**, pas pour le temps total
mesuré. Le source v12 appelle `dead_.load` à chaque cœur et calcule
une forme par site du cœur hors les deux extrémités. Sur la trame
entière : **3,986 M cœurs**, **551,69 M formes**, soit **138,39
formes par cœur** en moyenne. La somme des deux moitiés physiques
garde 97,0 % des constructions de cœur du plein mais seulement
30,6 % des formes : le coût par cœur croît fortement avec l'étendue
de la scène. Le plein prend 49,174 s de mur externe et 1,93 GiB RSS.

Ce n'est **ni** une borne asymptotique superquadratique **ni** une
qualification sous-quadratique de la chaîne : un seul nuage, une
graine, un run par cas, CPU partagé et géométrie modifiée par les
coupes. Les trois densités ne sont pas les anciennes tailles
8k/16k/32k sans sol (autre sélection et autre régime) ; ne pas les
apparier. Aucun K10, autre scène/séquence, G4 ou GPU n'est testé ici.

## Conséquence pour le prochain port q3/q4

Ce reçu rend prioritaire une réduction **facturée en formes chargées,
CPU et coût aval**, pas simplement en nombre de cœurs ou de paires
réfutées. Le shadow de voisins **du cœur** ne prouve pas la performance
des voisins globaux désormais en WIP ; sur la trame brute, ceux-ci
seraient tentés avant le filtre ponctuel sur bien plus d'arêtes que les
seules 3,986 M arrivant au cœur. Comparer ON/OFF à sorties identiques
sur les mêmes octets, avec coût de construction des listes kNN,
visites, cellules/test, formes, covers, CPU/mur/RSS et premier échec.
Reprendre ensuite les densités 8k/16k/32k et plusieurs trames brutes
et sans sol ; aucune extrapolation vers des dizaines de millions de
points n'est encore recevable.
