# Arêtes appariées du cœur q3/q4 : LiDAR brut, plans du capteur et densité

23 septembre 2026. Audit du port S2 au commit source
`c265a5dae4dd92059fc78acc0a1d7f52de9c1435`, en **CPU Release**,
K5/s8/W8/static8, filtre q3/q4 par lots CPU (`q34_batch_filter=1`,
`q34_gpu_filter=0`). Entrée : trame **brute avec sol** SemanticKITTI
08/000000, 123 389 retours distincts, grille commune 1 mm/u18. Les
plans `x=0` et `y=0` passent par le capteur dans les coordonnées float32
physiques **avant** grille ; aucun recalage ni hypothèse d'alignement
entre captations. Le [manifeste des entrées](../lidar_raw_physical_scaling_20260923/MANIFEST.json)
(`6e64125a…`) lie les IDs originaux aux payloads u18 et prouve les
sélections globales emboîtées 1/4 ⊂ 1/2 ⊂ pleine. Les mêmes sites ont
exactement les mêmes coordonnées u18 dans le plein et leur quart.

Ce reçu comprend **15 couples** moteur/lot CPU : plein et quatre quarts
aux trois densités, puis un deuxième couple plein qui trace le masque
après la preuve du cœur. Chaque couple a le même binaire, le même
catalogue, les mêmes ordres et le même digest de tour. Les 16 cas lot
tracés vérifient tous `charges = arêtes distinctes = core_builds` et
`Σ taille du cœur = core_sites = dead_core_form_sites + 2 × charges`.
Les traces emploient les **IDs des retours originaux** ; elles sont
appariées par paire non orientée. Aucun doublon d'arête n'a été trouvé.
Le statut est `complete_relative` : il recoupe les clés émises, sans
certifier les clés que les deux voies pourraient omettre. Le flux FULL
CPU est exécuté, mais il n'y a ni mesure GPU/G4, ni contrat 1 s acquis.

## Résultat : les arêtes traversantes portent la croissance du cœur

`F` compte **toutes les formes réellement calculées**, y compris celles
des deux extrémités à chaque `Q34DeadLaneProver::load`. « Traversantes »
signifie que les extrémités sont dans deux quarts physiques différents.
Les quatre quarts sont recalculés séparément : leur somme n'est pas la
tour de la trame entière.

| Densité | Sites plein | Charges plein / Σ quarts | F plein / Σ quarts | Traversantes : charges / F | F des mêmes arêtes, plein − quarts |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1/4 | 30 847 | 774 494 / 736 307 | 37 009 904 / 14 414 886 | 38 732 / **22 570 300** | 35 917 |
| 1/2 | 61 694 | 1 684 675 / 1 614 880 | 128 852 821 / 48 045 604 | 70 429 / **80 744 719** | 103 409 |
| Pleine | 123 389 | 3 986 433 / 3 831 169 | 559 661 741 / 173 020 536 | 157 012 / **386 517 780** | 317 334 |

À pleine densité, les traversantes représentent **3,94 % des charges**
mais **69,06 % des formes**. Les 3 829 421 arêtes communes au plein et
à leur quart coûtent 173 143 961 formes dans le plein contre
172 826 627 dans les quarts : la présence des sites extérieurs ne leur
ajoute que 317 334 formes. Aucune arête intra-quart n'est propre au plein
dans ces trois jeux ; 1 748 arêtes propres aux quarts coûtent 193 909
formes à pleine densité. L'identité contrôlée est :

`559 661 741 − 173 020 536 = 317 334 + 386 517 780 − 193 909`.

Pour chaque arête **commune**, le cœur diamétral fermé dépend seulement
de ses deux extrémités, donc `D_quart(e)=D_plein(e)∩quart`. Les 24 arêtes
échantillonnées par densité sont recomptées avec le prédicat entier
`|2z−a−b|²≤|b−a|²` ; pour les communes échantillonnées, les ensembles
d'IDs eux-mêmes vérifient l'intersection. Les 15 traces entières vérifient les sommes,
les appartenenances des arêtes aux quarts et la monotonie des tailles
communes. Les causes exactes des quelques arêtes propres aux quarts
(front, filtre, cœur) ne sont pas isolées par cette trace.

La pente finie de `F` sur le plein vaut **1,800**, puis **2,119** aux
deux doublements. Celle des formes des traversantes vaut **1,839** puis
**2,259** ; celle des arêtes communes du plein **1,736** puis **1,848**.
La croissance plus rapide que le carré du dernier lien est ainsi
localisée, pour cette trame et ce port, dans la masse des traversantes,
pas dans le gonflement des cœurs des mêmes arêtes intra-quart. Ce sont
trois tailles d'une seule scène, pas une borne asymptotique.

Le plan `x=0` explique presque toute cette masse sur **cette** scène :
les arêtes traversant ce plan portent 18,697 → 77,549 → **383,619 M**
formes (`p=2,052/2,306`), tandis que les arêtes franchissant seulement
`y=0` à l'intérieur d'une même moitié x portent 3,873 → 3,195 →
2,899 M. Cette classification n'est qu'un diagnostic physique ; une
optimisation produit ne peut supposer cet axe ni l'alignement des points.
À densité pleine, les 1 % plus gros cœurs traversants portent seulement
3,0 % de leur masse, et les 10 % en portent 27,7 % : ce n'est pas une
poignée isolée de cas extrêmes.

## Le masque après preuve précise le verrou

Un second build d'audit déplace la trace **après** `dead_.prove`, sans
changer la preuve. Ses 3 986 433 arêtes, tailles de cœur, masques
**avant** cœur, catalogue, ordres et digest sont identiques au premier
build. Le nombre de masques post-cœur nuls reproduit
`core_closed_edges` du moteur.

| Classe, densité pleine | Charges | F calculées | Charges closes au cœur | F calculées sur voies closes |
| --- | ---: | ---: | ---: | ---: |
| Traversantes des quatre quarts | 157 012 | 386 517 780 | 129 153 | **382 405 649** (98,94 % de F de la classe) |
| Dont traversantes de `x=0` | 124 423 | 383 619 073 | 107 853 | **380 106 252** (99,08 %) |
| Intra-quart | 3 829 421 | 173 143 961 | 1 677 744 | 144 267 295 |

Globalement, **526 672 944 / 559 661 741** formes (94,11 %) sont
matérialisées sur des arêtes dont les deux voies sont ensuite fermées
au cœur. Le masque post-cœur **ne constitue pas** un certificat de rejet
disponible avant `load` : le construire sans ces formes et compter son
coût restent des travaux distincts.

Un sélecteur intrinsèque exploratoire, `|a−b|≥4 m` calculé sur les
coordonnées entières, cible 339 428 arêtes (8,51 %) et 450 005 733
formes (80,41 %) au plein ; 441 336 015 de ces formes sont sur des voies
closes après cœur. Les seuils 2/4/8 m et leurs comptes sont dans
[`POST_CORE_FULL.json`](POST_CORE_FULL.json). Ce seuil n'est **ni** une
preuve géométrique **ni** une règle universelle : la prochaine ablation
doit essayer sur ces arêtes un certificat exact **avant** chargement du
cœur, avec repli, et mesurer son coût total, les sorties et les clés.
Une autre voie serait de calculer les formes seulement à la première
consultation par `prove`. Une cellule du premier niveau balayé (profondeur 2)
qui descend a déjà consulté tous les sites : la fraction de formes sur voies
closes ne prédit donc pas le gain. Mesurer d'abord le plus grand ordinal
consulté divisé par la taille du cœur, les retests et l'adressage ; l'ancien
[port global de crédit par nœuds](../ETAT_COURANT.md)
sur 16k sans sol réduisait les formes et **augmentait** le CPU. La
sélection par longueur peut servir à borner l'ablation, sans présumer
qu'elle sera gagnante. Tester ensuite K10, brut et sans sol, plusieurs
séquences et changements de densité, sans codage de l'axe x.

Les CPU·s de chaîne du bras moteur non tracé sur les trois **pleins**
valent 50,633 → 118,024 → 290,346 (`p=1,221/1,299`) ; ceux du bras
lot **tracé** 52,721 → 124,465 → 309,467 (`p=1,239/1,314`).
Une exécution par cas sur hôte partagé, avec coût de trace dans le bras
lot : ces temps décrivent le reçu, pas un gain ou une borne de S2. Les
[reçus bruts v12](../lidar_raw_physical_scaling_20260923/README.md)
et leur [complément K10](../lidar_raw_k10_sectors_20260923/README.md)
contiennent **les deux demi-scènes et les quatre quarts à chacune des
trois densités**. Le présent appariement d'arêtes S2 concerne les quarts
seulement ; il ne transforme pas les sommes des quarts en sondes des
demi-scènes.

## Reçu et reproduction

[`RESULTS.json`](RESULTS.json) garde les 15 SHA d'entrée, digests,
compteurs, temps descriptifs, SHA des sorties et des parties de trace,
ainsi que les SHA de la source, des deux patchs et des binaires. Les
[`DECOMPOSITION_QUARTER.json`](DECOMPOSITION_QUARTER.json),
[`DECOMPOSITION_HALF.json`](DECOMPOSITION_HALF.json) et
[`DECOMPOSITION_FULL.json`](DECOMPOSITION_FULL.json) gardent l'identité
par classe, la classification x/y, l'histogramme des tailles et les
échantillons géométriques. Le [reçu post-cœur](POST_CORE_FULL.json)
garde les masques agrégés et le sélecteur de longueur. Aucun chemin
`/tmp` volatil n'est dans ces JSON ; seuls les **petits résumés** sont
versionnés. Les traces binaires et les stdout détaillés sont dans `/tmp`
et seront perdus à l'arrêt du codespace. Leur lecteur LIVE
(`analyze.py`, `analyze_after.py`) exige ces fichiers ; les SHA gardés
permettent d'identifier une reprise, pas de reconstruire des traces
absentes. Le reçu compact seul conserve les résultats dérivés et se
contrôle par `SHA256SUMS`.

Rejouer dans un nouveau dossier `/tmp` depuis la racine du dépôt :

```sh
bash morsehgp3D_v9/audits/edge_matched_core_20260923/replay.sh /tmp/mhgp9-edge-core-replay-new
```

Le script archive le commit source épinglé dans `/tmp`, régénère les
payloads depuis les données v8 versionnées, construit deux binaires CPU
Release, exécute les 15 couples puis le couple post-cœur, recoupe les
sorties et écrit un nouveau reçu compact. Il faut CMake, un compilateur
C++20 et Boost ; si Boost n'est pas dans l'emplacement local déjà
utilisé, fournir `MHGP9_AUDIT_BOOST_ROOT`. Les 15 comparaisons exécutent
FULL mais restent `complete_relative` : une preuve indépendante des
clés absentes est une porte séparée.
