# Recherche des trames difficiles

Comment les cinq scènes de [`../`](../README.md) ont été trouvées, et ce qui
a été écarté en chemin.

## Point de départ : aucune trame publiée

Les figures qui montrent ce type d'échec ne donnent jamais le numéro de leur
trame : ALPINE (fig. 1, 2 et 4), ElC-OIS (fig. 5), 3DUIS (fig. 6),
Divide-and-Merge (animation, séquence 19 de test). Nous avons vérifié les
articles, les dépôts officiels, leurs tickets et les métadonnées des images.
Les dépôts ne livrent qu'une seule trame réelle, 08/000000, dans 3DUIS. Sur
cette trame sans sol, chacune des sept instances d'au moins 40 points a un
nœud de l'arbre HDBSCAN d'IoU supérieur ou égal à 0,92, quel que soit K (1,
5 ou 10). Ce n'est donc pas une trame difficile, et les scènes ont été
**cherchées**.

## Critère

Pour chaque instance « thing » de la vérité terrain, d'au moins 40 points
après retrait du sol, on calcule le **meilleur IoU atteignable par un nœud**
de l'arbre HDBSCAN complet (`min_samples = K`, point compté ;
`min_cluster_size = 1`), pour K = 1, 5 et 10. L'arbre est calculé sur la
trame entière et de façon exacte : `tools/hierarchy.py` traite en bloc les
arêtes de même poids et est vérifié contre Prim dense et contre
l'énumération de tous les nœuds.

L'IoU suit l'évaluation panoptique de SemanticKITTI : les points « void »
(classes non étiqueté, aberrant, autre structure et autre objet, que la
table d'apprentissage envoie sur 0) sont retirés avant l'appariement. Ils
restent dans le nuage et dans la hiérarchie, où ils peuvent servir de pont.
Toute extraction HDBSCAN rend des nœuds de cet arbre. Si le meilleur IoU
est ≤ 0,5, aucune ne peut donc compter l'objet comme vrai positif au sens
de la qualité panoptique.

Présélection par les seuls labels : trames de la séquence 08 (validation)
qui ont au moins un vélo, une moto, un piéton, un cycliste ou un motard, et
deux voitures, de 60 points chacun ou plus. Il y en a 2 388 ; on en garde
une sur huit, soit 299. On y ajoute 117 trames voisines des candidates
(fenêtres 000038–000064, 000512–000530, 000640–000666, 000868–000892,
001164–001200), pour choisir la meilleure vue de chaque scène.

## Fichier

[`criblage_08.jsonl`](criblage_08.jsonl) contient une ligne par trame (416
trames, 4 558 instances). Chaque ligne donne `origin` (`criblage_1_sur_8`
ou `voisinage_dense`), l'empreinte sha256 du `.bin`, les effectifs avec et
sans sol, puis, pour chaque instance : sa classe, `sem`, `inst`, son nombre
de points, sa distance au capteur, les classes voisines à moins de 0,3 m,
la distance à l'instance la plus proche et `best_iou` pour K = 1, 5 et 10.
Aucune coordonnée de point n'y figure. Il est produit par
`tools/search_frames.py` ; seul le champ `origin` est ajouté ensuite.

## Ce que dit le criblage

Sur les 299 trames du criblage 1 sur 8, pour les instances d'au moins
50 points (le seuil `min_points` de la qualité panoptique SemanticKITTI),
à toutes distances :

<!-- stats:début -->
| classe | instances | aucun nœud > 0,5 à K = 5 | à K = 10 | pour K = 1, 5 et 10 |
| --- | ---: | ---: | ---: | ---: |
| voiture | 2 188 | 0 | 0 | 0 |
| piéton | 231 | 7 | 6 | 5 |
| autre véhicule | 140 | 2 | 2 | 2 |
| vélo | 138 | 22 | 36 | 17 |
| moto | 96 | 1 | 2 | 1 |
| cycliste | 94 | 1 | 1 | 0 |
| camion | 23 | 0 | 0 | 0 |
| motard | 8 | 0 | 0 | 0 |
<!-- stats:fin -->

Les voitures proches du capteur, qui semblaient échouer quand l'IoU
comptait les points void, passent toutes : elles n'étaient « noyées » que
dans des points non étiquetés.

## Candidats retenus et écartés

| trame | objets | verdict |
| --- | --- | --- |
| 08/001176 | vélos 43, 57, 56 (et 55) garés en deux paires | **retenu** (démos 01 et 04) : le vélo 57 plafonne à 0,40–0,42 pour K = 1 à 10 ; le vélo 55, non suivi, échoue aussi (0,49 ; 0,5 exactement, donc non apparié ; 0,30) |
| 08/000882 | vélos 38, 58, 59 contre une façade | **retenu** (démo 02) : les vélos 38 et 58 restent sous 0,37 pour K = 1 à 10 ; le vélo 59 est apparié (0,60 à K = 5) |
| 08/000048 | piéton 9 près d'une façade, piéton 15, vélo 24 | **retenu** (démo 03) : le piéton 9 plafonne à 0,44 pour K = 1 à 10 ; à K = 5, il échoue sur 23 des 28 trames criblées entre 000032 et 000064 |
| 08/002554 | voitures 169, 170, 171 en file | **retenu comme témoin** (démo 05) |
| 08/000058 | la même scène que 000048 | **abandonné** : avec l'IoU au sens de la PQ, le piéton 9 y est apparié de justesse (0,51 à K = 5), car ce qui l'absorbe est surtout du mobilier « autre objet », void |
| 08/000055 | piétons 2 (mobile), 9, 15 | **écarté** : la boîte du piéton 2 contient un second piéton annoté sans identifiant d'instance (sem 254, inst 0, 100 points) ; le meilleur nœud HDBSCAN recouvre presque exactement leur union. Le cas mêle un vrai rapprochement et une annotation incomplète |
| 08/000649 | autre véhicule 16, vélos 37 et 61 | **écarté pour la lisibilité** : le vélo 37 (66 points) est pris dans une haie (IoU 0,18 à K = 5, 0,12 à K = 10). C'est le mécanisme de la démo 02, dans une scène moins lisible |
| 08/003570–003578 | piéton mobile 28 longeant un bâtiment | candidat de réserve : IoU 0,15 à 0,20 pour K = 1, 5, 10 en 003578, mais 0,63 à K = 10 en 003570 |
| 08/000038–000063 | piéton 1 contre la voiture 195 (cas « cycliste contre voiture » d'ElC-OIS) | pas un échec : les deux restent au-dessus de 0,5 (au moins 0,51) sur toutes les trames criblées |

## Méthodes non réimplémentées

- **ElC-OIS** (clustering ellipsoïdal) : ses gains sur HDBSCAN, le
  clustering euclidien et CVC n'ont été mesurés que sous un masque
  sémantique de premier plan, qui retire aussi la végétation. Le transposer
  à un simple retrait géométrique du sol serait une autre méthode.
- **CVC** (voxels courbes) : sa hiérarchie n'est pas exactement emboîtée,
  et son algorithme dépend de l'ordre des points.
- **Divide-and-Merge** : il dépend de la sémantique, et ALPINE le bat à
  sémantique égale.
- **3DUIS** : sa proposition est HDBSCAN avec `min_cluster_size = 20`,
  soit K = 21 dans notre convention, sur un MST approché. Ce réglage n'est
  pas évalué ici. Son raffinement exige des poids appris.
