# Catalogue contributif et poids non Gabriel

27 septembre 2026. Petit contre-exemple rationnel, indépendant des moteurs
et des anciens estimateurs. Il complète la
[lecture de HGP-old et HGP-Clusterer3D](LECTURE_HGP_OLD_CLUSTERER3D_20260927.md)
et le [plan ponctuel actuel](../experiments/point_dendrogram_20260927/PLAN.md).
Aucun ancien code non commercial n'est importé ou porté ; pas de clustering,
nouvelle compilation, GPU ou GCP.

## Conclusion exacte

Une preuve de préservation de la connexité ne suffit pas à préserver les
scores d'incidence Sτ. Le **catalogue de boules FULL limité à l'ordre K**
ne contient pas nécessairement toutes les MEB des cofaces contributives du
catalogue d'ordre K+1 annoncé par l'ancien clusterer. Une simple agrégation
des contributions des seules boules présentes peut donc perdre des poids,
voire des facettes entières.

Ce n'est **pas une impossibilité informationnelle de reconstruction**.
Si les coordonnées originales restent accessibles, ou si une représentation
plus riche les encode, une nouvelle construction géométrique peut retrouver
le catalogue et ses incidences. Elle constitue un nouveau calcul, pas une
conséquence gratuite du quotient de connexité. La preuve ci-dessous porte
sur K fixé et sa fenêtre de catalogue, **pas sur toutes les MEB possibles
à tous les ordres**.

Le contrat mathématique documenté de la voie 3D retient les ensembles de
K+1 sites dont la cellule de Voronoï d'ordre K+1 est non vide, puis leur
rayon MEB : [hypergraph.py](../../HGP-Clusterer3D/src/hgp_clusterer/hypergraph.py),
lignes 56–78. L'export v9 employé impose au contraire
`|I| + q_min ≤ K+1`, puis inclut **tous** les intérieurs dans ses cofaces
Gabriel : [native_weighted_export.cpp](../experiments/weighted_clustering_20260927/native_weighted_export.cpp),
lignes 34–40, 55–79, 123–126. Le présent test compare ces deux univers idéaux,
pas une exécution du vieux backend perturbé ni ses arrondis/planchers.

## Quatre points, K=2

Prendre A=(0,1,0), B=(4,1,0), C=(1,2,0), D=(1,0,0).
C'est une configuration **planaire plongée dans R³**, non un jeu volumique.
Les IDs sont A=0, B=1, C=2, D=3. Les quatre triples possibles possèdent
chacun un témoin strict : toutes les distances des trois sites retenus
sont inférieures à celle du site exclu.

| triple | témoin y | distances carrées à A,B,C,D | marge exclu − maximum retenu |
| --- | --- | --- | --- |
| ABC | (1,11,0) | 101,109,81,121 | 12 |
| ABD | (1,−9,0) | 101,109,121,81 | 12 |
| ACD | (−1,1,0) | 1,25,5,5 | 20 |
| BCD | (5,1,0) | 25,1,17,17 | 8 |

Il n'existe que quatre triples : ces certificats prouvent donc exactement
le catalogue d'ordre3 de cette configuration, sans solveur flottant ni
hypothèse de perturbation.

### Certificats MEB inférieurs et supérieurs

| triple | centre c | β=r² | support ; coefficients convexes positifs |
| --- | --- | --- | --- |
| ABC, ABD | (2,1,0) | 4 | A,B ; 1/2,1/2 |
| ACD | (1,1,0) | 1 | C,D ; 1/2,1/2 |
| BCD | (7/3,1,0) | 25/9 | B,C,D ; 4/9,5/18,5/18 |

Les coefficients λi somment à1, leur barycentre est c et tous les points
du support sont à distance carrée β. Pour tout centre y,

`Σ λi ||pi−y||² = β + ||c−y||² ≥ β`.

Le maximum des distances d'une boule contenant le support est donc au
moins β : c'est une borne inférieure valable pour **tout** y. Tous les
sommets du triple sont dans la boule donnée : la borne supérieure β est
atteinte. Le test vérifie exactement les coefficients quadratiques,
linéaires et constants de cette identité ainsi que l'inclusion supérieure.

| boule | intérieurs stricts I | coquille complète S | q_min | card(I)+q_min |
| --- | --- | --- | --- | --- |
| β4, centre(2,1,0) | C,D | A,B | 2 | 4 |
| β1, centre(1,1,0) | aucun | A,C,D | 2 | 2 |
| β25/9, centre(7/3,1,0) | aucun | B,C,D | 3 | 3 |

La boule β4 est absente de la fenêtre K2 car 4>3. ABC omet D et ABD omet
C, pourtant strictement intérieurs : ces deux cofaces sont non Gabriel.
ACD et BCD sont Gabriel. Pour β25/9, aucun couple de coquille n'est
antipodal, donc q_min=3 ; une boule de rayon positif ne peut avoir q_min=1.
La frontière supplémentaire A sur la boule β1 est correctement conservée.

## Les poids changent, pas seulement leur échelle

À z=2, utiliser ψσ=1/βσ, sans normalisation ni plancher. La normalisation
globale positive par la médiane des rayons n'effacerait pas les différences
ci-dessous en arithmétique exacte ; elle n'est pas une reproduction des
arrondis de l'ancien programme.

| facette τ | Sτ, cofaces Gabriel seulement | Sτ, quatre cofaces d'ordre3 |
| --- | --- | --- |
| AB | absente (0 pour comparaison) | 1/2 |
| AC, AD | 1 | 5/4 |
| BC, BD | 9/25 | 61/100 |
| CD | 34/25 | 34/25 |

CD ne change pas, les autres scores communs changent, et AB apparaît :
aucune constante globale ne transforme un vecteur en l'autre. Les totaux
Tx sont respectivement `(2,18/25,68/25,68/25)` et
`(3,43/25,161/50,161/50)`. Les masses changent aussi, par exemple
mCD=1 contre136/161. Dans les deux cas, la somme des masses vaut4 et chaque
masse de facette est au plus1. Les sorties conservent tous les S/T/m exacts.

Le test ne compare pas des partitions de connexité de ces graphes de
facettes aux univers différents. Il prouve le défaut de préservation des
**incidences pondérées** par la restriction considérée ; il ne démontre
aucun gain ou défaut d'ARI et n'exécute pas EOM sur quatre points à m20.

## Ce qu'une agrégation fidèle demanderait

Avec toutes les boules MEB pertinentes, un supplément suffisant serait

`N(τ,B) = #{σ admissible : τ⊂σ et MEB(σ)=B}`,

puis `Sτ(z) = ΣB N(τ,B) βB^(−z/2)`. Les multiplicités doivent être **par
facette**, pas seulement un compte global par boule. Les boules de la
fenêtre K n'apportent pas ces N ; notre exemple manque même une boule.
Pour une coface non Gabriel, les intérieurs globaux de sa MEB ne sont plus
tous obligatoires. Il faut en outre certifier l'admissibilité de sa cellule
d'ordre, non déduire celle-ci du seul rayon, de q_min ou de la coquille.

À la prochaine itération seulement, tester une petite ablation de catalogue :
ce quadruplet, E5 et quelques nuages fixés n≤10, K2/3, z2 rationnel. Un oracle
indépendant peut énumérer les petites cofaces, certifier leurs MEB et la
faisabilité exacte des inégalités linéaires de cellule d'ordre ; les cellules
fermées dégénérées doivent avoir une convention explicite. Comparer S/T/m,
facettes présentes, attaches, routage et coupes avant tout score statistique.

Garder le même objet géométrique FULL/Čech et les règles exclusives datées,
mais réattacher les facettes supplémentaires à leur vraie MEB. Chaque voie
route son univers de facettes **de score positif** : ne pas injecter des
scores nuls ou epsilon dans l'API actuelle. Conserver EOM pondéré→vote plat
comme contrôle séparé du catalogue, pour distinguer mesure et projection
unitaire ; aucune équivalence historique n'est présumée.

Si ce diagnostic révèle un effet matériel, envisager ultérieurement une
énumération sensible à la sortie des cellules admissibles avec agrégation
en flux des Sτ, puis abandon des cofaces brutes. Cela évite de stocker un
catalogue global de tous les sous-ensembles ; cela ne borne pas le nombre
de vraies cellules ni leur coût par une fonction sous-quadratique de n.
Ce n'est pas une proposition de grand port immédiat.

## Test autonome et reçu

[test_catalogue_counterexample.py](../experiments/point_dendrogram_20260927/test_catalogue_counterexample.py)
n'importe que `fractions`, `itertools`, `json` ; aucune fonction géométrique
ou statistique du dépôt n'est réutilisée. SHA256 de la source :
`19bf9eac9b81fb95ead20314551fe91bb7e61fdc9fa8a9f47d46126a9a57fb28`.

```sh
/home/codespace/.python/current/bin/python -B morsehgp3D_v9/experiments/point_dendrogram_20260927/test_catalogue_counterexample.py
/home/codespace/.python/current/bin/python -B -O morsehgp3D_v9/experiments/point_dendrogram_20260927/test_catalogue_counterexample.py
```

Capture privée **PASS**, deux commandes code0, stdout identiques,
stderr vides, six empreintes avant/après identiques, durée0,115s :
`/tmp/mhgp9-catalogue-counterexample-20260927-t_pedf1d/receipt.json`, SHA256
`5a3286708f7baf6d5df0633953d79f4e0374dc2bb4636eefb01f32bc84ce7260`.
Le reçu conserve argv/cwd/environnement, sorties et codes. Deux sources
anciennes y sont seulement hachées comme contexte documentaire, jamais
importées/exécutées. Les captures et sources des pilotes gelés restent
inchangées ; ce petit test est un ajout hors de leur inventaire.
