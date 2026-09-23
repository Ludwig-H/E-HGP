# Contrelecture B — première trame brute et découpes physiques

**Erratum de lecture ajouté par A, 23 septembre.** Les valeurs appelées ci-dessous
« formes » sont le champ `dead_core_form_sites`, qui **exclut les deux
extrémités calculées à chaque charge**. Le total de formes réellement
calculées est `core_sites = dead_core_form_sites + 2×dead_core_loads` :
**37,010→128,853→559,662 M** sur les trois densités, avec pentes
**1,800/2,119**. Les tableaux historiques ci-dessous et leurs pentes
**1,823/2,136** restent exacts pour le seul sous-total hors extrémités ;
voir la [synthèse corrigée](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md).

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

| sites de la trame brute | chaîne locale W8 | CPU·s | q3/q4 | formes du cœur hors extrémités |
| ---: | ---: | ---: | ---: | ---: |
| 30 847 | 9,318 s | 49,920 | 7,005 s | 35,46 M |
| 61 694 | 20,486 s | 116,088 | 15,686 s | 125,48 M |
| 123 389 | 48,357 s | 285,329 | 38,942 s | 551,69 M |

Les pentes effectives de ce sous-total sont **1,823 puis 2,136** aux deux
doublements ; celles du CPU sont **1,218 puis 1,297**, du temps de
chaîne **1,137 puis 1,239**. Le deuxième doublement dépasse donc n²
pour une **opération réellement payée**, pas pour le temps total
mesuré. Le source v12 appelle `dead_.load` à chaque cœur et calcule
une forme par site du cœur, **extrémités comprises**. Sur la trame
entière : **3,986 M cœurs**, **551,69 M formes hors extrémités**
(138,39 par cœur), soit **559,66 M formes réellement calculées**
(140,39 par cœur). La somme des deux moitiés physiques
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

## Extension du reçu : 21 cas, contrelecture B du 23 septembre

Le commit `0421a2a0` ajoute les douze combinaisons manquantes : sept
secteurs physiques × trois densités globales emboîtées, toujours une
seule trame **avec sol**, K5/s8/W8 local. Les neuf cas originaux et
leurs sorties restent inchangés. J'ai contrôlé **50/50 SHA** ; un
rejeu indépendant des deux correspondances raw→full, des signes x/y
float32 et du rang global `splitmix64` retrouve les 21 ensembles d'IDs
et leurs payloads u18. Avec les 21 sorties archivées, le manifeste,
`SUMMARY.json` (37 157 octets) et `summarize.stdout` sont reconstruits
**octet pour octet** en interceptant les écritures du lecteur ; aucun
fichier n'a été modifié pendant cette contrelecture. Le lecteur refuse
logiquement le manifeste temporaire limité aux neuf premiers cas.

Les chiffres de l'extension sont cohérents pour le sous-total
hors extrémités : **2/14** pentes
au moins quadratiques entre densités d'un même secteur — plein
1/2→1 : **2,136333** ; quart `x≥0,y<0` 1/4→1/2 : **2,059549**.
Les quatorze pentes CPU sont **1,1995–1,3283**, les paires développées
**1,2869–1,7691**. Parmi les dix-huit liens spatiaux, **7** pentes de
formes atteignent 2, maximum **2,879826** ; elles changent cependant
la géométrie et les frontières. Les ratios moitiés/plein des formes
aux densités 1/4, 1/2, entière sont **0,549 / 0,421 / 0,306**.

Verdict inchangé : le reçu prouve un signal de masse de travail local,
pas une complexité asymptotique ni un contrat de tour. Une scène,
une séquence, une répétition, CPU partagé et `complete_relative`
seulement ; la contention rend les nouveaux murs peu comparables.
