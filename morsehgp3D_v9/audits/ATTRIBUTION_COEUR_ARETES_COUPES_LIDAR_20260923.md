# Attribuer le coût du cœur aux arêtes des coupes LiDAR

**Protocole exécuté** sur le port S2 CPU à K5, aux trois densités :
[reçu par arête et masque après cœur](edge_matched_core_20260923/README.md).
Cette note conserve la méthode et ses hypothèses ; le reçu lié porte les
résultats et les limites actuelles.

Auditeur A, 23 septembre 2026. Diagnostic proposé pour le port S2 courant ;
les chiffres ci-dessous sont les [reçus bruts v12
K5](lidar_raw_physical_scaling_20260923/README.md) et
[K10](lidar_raw_k10_sectors_20260923/README.md), **pas** une mesure S2 ou
G4. Les plans physiques du capteur `x=0`, `y=0` donnent quatre quarts
disjoints, sans déplacement des coordonnées u18 issues de la trame
entière. La sélection 1/4 ⊂ 1/2 ⊂ entière reste celle des IDs originaux.

| 08/000000 brut, densité entière | charges du cœur : plein / Σ quarts | formes réellement écrites : plein / Σ quarts | formes par charge : plein / Σ quarts |
| --- | ---: | ---: | ---: |
| K5 | 3 986 433 / 3 831 169 | 559 661 741 / 173 020 536 | 140,39 / 45,16 |
| K10 | 7 811 827 / 7 481 564 | 1 254 254 109 / 478 489 365 | 160,56 / 63,96 |

Les formes incluent les deux extrémités de chaque arête :
`F=dead_core_form_sites+2·dead_core_loads`. Les quatre quarts paient
presque autant de charges que le plein, mais beaucoup moins de formes.
**Ces agrégats ne disent pas pourquoi** : une coupe modifie aussi
l'ensemble des arêtes traitées, le front et les rejets. Attribuer tout
l'écart au grossissement des mêmes cœurs serait injustifié.

Pour une arête canonique `e=(a,b)` traitée dans le plein `X` et dans son
quart `P`, le cœur fermé est l'intersection des sites avec la **même**
boule diamétrale, déterminée uniquement par `a,b`. Donc, exactement,

`D_P(e)=D_X(e)∩P` et `|D_X(e)|−|D_P(e)|=|D_X(e)∩(X\P)|≥0`.

Cette identité donne une expérience qui sépare les mécanismes. Émettre
hors chrono, à chaque `dead_.load` du **S2 CPU**, les IDs de retours
originaux des deux extrémités, le secteur et `core.site_count()`. Les
rangs/IDs locaux u18 ne suffisent pas : la préparation les renumérote,
et trois retours changent même de secteur si l'on coupe après arrondi.
Avant de joindre les traces, vérifier qu'il n'y a qu'une charge par
arête canonique dans chaque run ; sinon conserver et expliquer les
multiplicités au lieu de les écraser.

Classer ensuite les arêtes du plein en (i) mêmes arêtes présentes dans
leur quart, (ii) arêtes intra-quart propres au plein, (iii) arêtes dont
les extrémités sont dans deux quarts. Garder séparément (iv) les arêtes
propres aux calculs des quarts. Avec ces quatre classes, l'écart exact
des formes s'écrit, **après contrôle d'unicité des charges par arête** :

`F_plein−ΣF_quarts = Σ_communes (|D_plein|−|D_quart|)
 + Σ_plein_seul_intra |D_plein| + Σ_traversantes |D_plein|
 − Σ_quart_seul |D_quart|`.

La première somme mesure **exactement** les sites extérieurs ajoutés
aux cœurs des mêmes arêtes ; les autres termes mesurent le changement
d'arêtes. Contrôler que les totaux de la trace reconstruisent les
compteurs du reçu, et vérifier sur quelques arêtes les ensembles d'IDs
du cœur, pas seulement leurs tailles. Une première discrimination demande
cinq exécutions CPU S2 (plein + quatre quarts) sur 08/000000 brut/K5 à
densité entière ; refaire aux densités 1/2 et 1/4 puis K10 permet de
relier le mécanisme aux pentes. Mesurer le coût de la trace à part.

Si la première somme domine, chercher un certificat exact par blocs ou
un partage des preuves entre arêtes voisines **avant** la
matérialisation d'une forme par site du cœur. Si les arêtes nouvelles ou
traversantes dominent, concentrer l'effort sur le front, les rectangles
et le filtrage avant cœur. Les secteurs restent un diagnostic : sommer
leurs tours ne reconstruit pas la tour du plein.
