# Shadow B — paires de gardes sur les rectangles WSPD réels

23 septembre 2026. Expérience **audit-only**, sans port dans le moteur.
Elle mesure la sélectivité du [certificat exact à 64 couples de
coins](../CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md) au niveau
`A×B`, **avant** l'expansion S2. Le cas est la trame brute entière
SemanticKITTI **08/000000**, grille 1 mm/u18, **123 389 sites**, K5/s8.
Une seule trame, un seul `s` et un seul `K` ne qualifient ni croissance
sous-quadratique ni contrat G4.

## Résultat utile

Le front réel produit 6 175 011 rectangles ; le filtre de rectangle
laisse 2 548 453 ouverts, représentant 22 034 426 paires `A×B` à
examiner. Les **1 747** rectangles ouverts avec produit au moins 1 024
concentrent 397 354 920 des 559 661 741 formes du cœur de la trame
(71,0 %). Ils sont donc une cible pertinente **en masse**, mais seuls
299 d'entre eux portent une arête survivant à S2.

| Parmi les 1 747 grands rectangles | Segments | Produit `A×B` | Arêtes après S2 | Formes `F` après S2 |
| --- | ---: | ---: | ---: | ---: |
| Avec au moins une survivante S2 | 299 | 888 779 | 81 089 | 397 354 920 |
| Sans survivante S2 | 1 448 | 4 926 252 | 0 | 0 |
| Fermés complètement par 64 coins + appariement maximum, avec survivantes | **72** | 193 443 | **2 175** | **5 059 809** |
| Fermés complètement, mais déjà vides après S2 | 979 | 4 033 983 | 0 | 0 |

Ainsi, **0,904 %** de tout `F` (1,274 % du `F` des grands
rectangles) serait *éligible* à un saut du cœur si ce certificat était
inséré au bon endroit. Les 979 rectangles vides fermés pourraient
éviter une partie de la boucle S2, mais n'évitent **aucune** forme de
cœur. Leur produit de 4 033 983 représente 18,3 % des 22 034 426
paires ouvertes globales, pas un gain de durée mesuré. Sur les seuls
202 grands segments de longueur S2 au moins 16, la preuve ferme 31
segments et 4 655 445 des 396 309 030 formes correspondantes.

Quarante-six rectangles positifs supplémentaires ferment **q3
seulement** : 2 884 arêtes et 11 290 428 formes. Comme q4 reste
ouverte et que les voies partagent le cœur diamétral, ces formes ne
s'ajoutent **pas** aux 5 059 809 formes évitables avant le cœur.
La même palette testée avec les témoins singletons `box_witness`
ne ferme **aucun** segment positif complet. L'appariement maximum
ajoute seulement trois segments positifs (919 271 `F`) au glouton
déterministe sur cette palette ; il ne change pas le verdict.

## Méthode et travail payé

Le [code source archivé](probe.cpp) rejoue `MidpointSamples` et le
filtre `Affine` du front sur l'index global du moteur. Il conserve
**tous** les rectangles ouverts de produit ≥1 024 ; 24 contrôles
plus petits sont tirés par hash de l'ordinal avant lecture de la
trace. Pour chaque rectangle, la première arête de ses rangs spatiaux
oriente quatre recherches indexées, chacune limitée à 128 nœuds et
huit sites dans le cœur diamétral. Les sites proposés sont hors
`A∪B`. Ce représentant ne décide jamais du rejet : chaque paire de
gardes positive au représentant subit les coins virtuels exacts, et
les paires disjointes sont comptées séparément pour q3/q4. Un échec
de recherche, de coin ou de matching signifie **repli exact**, jamais
survie ou rejet prouvé.

Le shadow teste toutes les paires proposées et les deux voies,
même lorsqu'un arrêt anticipé suffirait. Son coût n'est donc pas celui
d'un port optimisé : **864 392** nœuds index extraits,
**1 341 508** bornes de boîtes, **186 023** sites examinés,
**6 188** recherches arrêtées au budget, **752 757** paires de
gardes proposées et **26 845 077** couples de coins effectivement
évalués. Le run final mono-CPU sur hôte partagé somme **75,7 ms**
de sélection et **614,0 ms** de classification + matching exact sur
ces 1 747 rectangles ; front/reconstruction de trace et préparation
sont **exclus**. Ces durées ne sont pas des gains nets de chaîne et ne
se comparent pas directement à un noyau G4.

La jointure avec la trace S2 se fait **après** le test pré-S2 : chaque
paire d'IDs bruts d'un rectangle échantillonné est unique, et une
arête tracée est affectée une fois. Le nombre `F` contient les deux
extrémités de chaque arête. Les masses de front, filtre, arêtes S2
(3 986 433) et `F` (559 661 741) rejoignent les reçus épinglés.
Le [lecteur de lignes](verify.awk) recalcule les totaux, seuils de
voies, invariants de travail et résultats glouton/**maximum** ;
le [test de matching](match_test.cpp) confronte l'algorithme à une
programmation dynamique exhaustive sur 27 000 graphes déterministes
de 2 à 10 sommets et à la fixture géométrique du certificat. Ce
n'est pas une preuve exhaustive de tous les graphes à 32 sommets.
La sortie agrégée **ne sérialise pas les IDs des gardes ni le matching
par rectangle** : un lecteur indépendant ne peut pas refaire les
64 tests de chaque fermeture positive à partir du seul reçu. Les
72 fermetures sont donc un **shadow de source et d'agrégats**, pas
une qualification par certificat individuel archivé.

## Provenance et rejeu

SHA-256 : source `probe.cpp`
`7f4f2c4e50d0b9e4e3261288ad582abaffa3d9e971466d30a9878bb56c6c589c` ;
binaire local `probe_max`
`ebd96a881c1c67e0ca5de1c858c19bd8f31161433a4e67d9db85fe84b608fc87` ;
sortie locale complète `exhaustive_max.stdout`
`70024f66452a592d8b7dada7b4e696ab7d3386e154cbb626c3fb47530f5762d4` ;
archive du générateur `libmhgp9_gen.a`
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`.
Les points `/tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le`
ont SHA
`233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172` ;
leurs IDs bruts adjacents ont SHA
`796e9814dd2dff7778ad8d1bb55f5113ab54206ff8b246e6432200f9a6833f7f`.
Les huit traces `part_*.bin` sont sous
`/tmp/mhgp9-edge-core-audit-20260923/full/trace` et épinglées par
le [reçu S2](../s2_segment_mass_20260923/README.md). Les données,
traces, archive et binaire sous `/tmp`/`build` sont des dépendances
**LIVE non versionnées** ; ce dossier archive la source et les
agrégats, pas une preuve autonome réexécutable sans ces dépendances.

Depuis la racine du dépôt avec ces dépendances présentes :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/rect_pair_shadow_b_20260923/probe.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-rect-pair-shadow-b
timeout 150s /tmp/mhgp9-rect-pair-shadow-b \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le \
  /tmp/mhgp9-edge-core-audit-20260923/full/trace \
  > /tmp/mhgp9-rect-pair-shadow-b.stdout
awk -f morsehgp3D_v9/audits/rect_pair_shadow_b_20260923/verify.awk \
  /tmp/mhgp9-rect-pair-shadow-b.stdout
```

Le run original est sorti avec code 0 ; un rebuild a reproduit les
octets du binaire. La reconstruction front+filtre du sidecar a pris
45,3 s de mur local sur un cœur partagé, **hors** des 689,7 ms de tentatives ;
elle n'est pas un chrono du front intégré ni de G4.

## Décision pour la refonte

Ne pas porter ce crible uniforme `A×B`/palette 32 comme solution au
cœur q3/q4 : il laisse au moins **99,096 %** des formes du cœur de
cette trame sous le chemin courant, avant même le coût de sélection.
Le test des 64 coins est déjà exact pour la relaxation en boîtes
continues et une paire fixe ; améliorer seulement son *calcul* peut
réduire son coût, pas augmenter ses fermetures. Les essais prioritaires
sont un **partage adaptatif des grands rectangles** avec certificats
héritables, des palettes de gardes mieux choisies ou des blocs de
gardes certifiés, puis des tuiles d'arêtes qui amortissent la recherche
sans exiger une preuve sur toute la boîte. Mesurer ces essais sur les
**segments positifs**, avec la sélection payée sur tous les rectangles
tentés, et comparer `F` et covers réellement évités, sortie clé par
clé, FULL, CPU/mur/RSS. Répéter 8k/16k/32k, brut/sans-sol,
K5/K10, s8/10/12 et plusieurs séquences avant toute conclusion de
passage à l'échelle ou de contrat G4.
