# Le résidu q2 des amas du front v8 tombe de trente-huit fois avec les crédits Pool déjà écrits

14 septembre 2026, après **e3af11a7**. Auditeur indépendant B.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

> **Statut au 21 septembre 2026 (auditeur B)** : note historique (e3af11a7), chiffres corrigés sur son reçu [credits_terminaux_20260914/](credits_terminaux_20260914/CREDITS_TERMINAUX_CHECKS.json). Le filtre Pool terminal a été porté par le constructeur à ba11e3ab et contre-vérifié de bout en bout ([chaine_q2_20260914/CHAINE_Q2_POOL_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_POOL_CHECKS.json)).

Depuis le raccord front → census (huitième tranche), la famille « huit
amas » du constructeur reste quadratique : 11,4 / 43,8 / 173,5 s à
8k/16k/32k, visites ×4,1 puis ×4,2, résidu q2 de 460 millions de paires
à 32k. Ce régime est exactement celui des gros facteurs pour lequel les
tranches P0 (Pool, DualBlocks, Tubes) ont été construites et qualifiées,
et que le front actuel n'appelle plus : il rejette par témoins extérieurs
seulement, or les produits inter-amas n'en ont aucun (la lentille est
vide de sites). Cette note mesure, sur les sources épinglées de da366f7f
et la fixture `clusters` du constructeur, ce que rapportent ces crédits
appliqués aux seuls rectangles terminaux à gros facteurs, et diagnostique
pourquoi les tubes n'y servent à rien. Le harnais additionne les
candidates des plans ; **il n'exécute ni census ni collecte après les
crédits** : le gain est mesuré sur le résidu que le census aurait à
payer, pas sur le temps de la chaîne. Reçu rejouable :
[credits_terminaux_20260914/](credits_terminaux_20260914/CREDITS_TERMINAUX_CHECKS.json).

## 1. Un produit inter-amas isolé

Fixture `clusters` à 32k (huit amas de 4 000 sites aux sommets d'un cube,
jitter 1 024, séparés de 30 000), produit des amas 0 et 1 (4 048 × 4 012
sites, 16,24 millions de paires), Kmax 10, s 8, via l'ancienne factory
`prepare_rectangle` et `make_credit_batch` :

| Stratégie | Temps | Résidu q2 | Résidu q3 | Résidu q4 |
| --- | ---: | ---: | ---: | ---: |
| Tubes | 1,4 ms | 16 240 576 | 16 240 576 | 16 240 576 |
| Pool | 7,3 ms | 8 447 | 2 516 804 | 3 797 212 |
| DualBlocks | 177 ms | 2 665 | 133 641 | 159 525 |

Les produits 0×3 (diagonale de face) et 0×7 (grande diagonale) donnent
95 et 60 paires q2 avec Pool. Les sites de A « en avant » de l'ancre vers
B sont des témoins universels de toute la boîte B : c'est le crédit h_a
du plan P0, et il suffit ici presque toujours.

## 2. Pourquoi les tubes ne créditent rien sur un nuage irrégulier

La largeur de cellule des tubes est fixée à `4·max|d_i|` en unités
transverses `u = d × p`, avec `d = 2(c_B − c_A)`. Pour deux amas distants
de 30 000, un déplacement transverse réel de δ vaut `|d|·δ ≈ 60 000·δ` :
la cellule ne fait que quatre unités de large en coordonnées réelles.
Sur 8 060 sites irréguliers, on obtient 7 818 cellules, donc un site par
cellule et aucun suffixe : crédits nuls dans les trois voies, sans repli
déclaré (`fallbacks = 0`). Un balayage de la largeur (copie d'audit,
multiplicateur seul changé) montre le compromis : ×16 laisse 4,1 millions
de paires q2, ×64 en laisse 377 475 (32 cellules), ×256 et au-delà (deux
cellules) 3,7 millions, avec q3/q4 de nouveau à 16,2 millions parce que
`Q_C` devient trop grand. Même à sa meilleure largeur, le certificat
reste quarante fois moins sélectif que Pool.

Les tranches P0 ont mesuré les tubes sur des grilles et des nappes
exactement alignées, où les sites d'une même colonne ont un transverse
nul : c'est là qu'ils gagnaient. Sur l'échantillonnage irrégulier que
l'utilisateur impose comme régime de référence, ce certificat exige une
largeur adaptée à la géométrie du facteur, et même alors il ne remplace
pas Pool ou DualBlocks. Il ne doit pas porter le chemin général.

## 3. Sur le front réel : 28 rectangles portent presque tout le résidu

Front `MidpointSamples` du constructeur, toutes voies, Kmax 10, s 8, puis
crédits locaux appliqués dans le callback aux seuls rectangles terminaux
dont un facteur atteint 64 sites (copie des points des deux facteurs
incluse dans le temps) :

| Famille, n | Rectangles émis | dont facteur ≥ 64 | Résidu q2 du front | Résidu q2 après Pool | Temps des crédits | q3 après Pool | q4 après Pool |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Amas 8k | 1 839 366 | 28 | 29,73 M | 1,74 M (×0,059) | 60 ms | 5,03 M (×0,16) | 6,01 M (×0,19) |
| Amas 16k | 5 250 240 | 28 | 116,75 M | 4,79 M (×0,041) | 122 ms | 17,46 M (×0,14) | 20,77 M (×0,17) |
| Amas 32k | 13 271 306 | 28 | 460,08 M | 12,18 M (×0,026) | 241 ms | 58,85 M (×0,12) | 72,49 M (×0,15) |

Sur ces 28 rectangles, Pool ramène 28,0 millions de paires q2 à 11 329 à
8k, 112,0 millions à 29 878 à 16k et 448,0 millions à 102 993 à 32k ; ce
qui reste dans le résidu total (1,74 / 4,79 / 12,18 millions, soit ×2,75
puis ×2,54 par doublement au lieu de ×3,9) vient des petits rectangles
intra-amas, que Pool ne réduit pas (seuil 2 à 8k : même résidu q2, 1,04 s
de crédits de plus d'après le reçu). Les temps de crédits incluent la
copie des facteurs et la préparation du rectangle, pas leur destruction.
DualBlocks sur les mêmes 28 rectangles coûte 1,1 / 2,6 / 5,5 s et
laisse à 32k 31 419 / 1,68 M / 2,04 M paires q2/q3/q4 (contre 102 993 /
32,0 M / 47,8 M avec Pool) : préférable pour q3/q4 quand le census de ces
voies existera.
Sur l'uniforme 8k, aucun rectangle n'atteint 64 sites et Pool ne change
rien (×1,000) ; le terrain mince est dans le même cas.

## 3 bis. Ce que les survivantes contiennent réellement

Ajout du 14 septembre, après relecture du constructeur. Le harnais
[front_pool_truth.cpp](credits_terminaux_20260914/front_pool_truth.cpp)
reprend le même front et les mêmes 28 rectangles, ne construit que le
plan q2, puis compte par force brute (arrêtée à Kmax) les sites
strictement intérieurs de chaque paire survivante contre tous les sites
du nuage ; il contrôle aussi la sûreté des rejets sur 560 000 paires
rejetées tirées de façon déterministe par rectangle. Reçu :
[SURVIVANTS_CHECKS.json](credits_terminaux_20260914/SURVIVANTS_CHECKS.json).

| Amas, n | Survivantes Pool | Survivantes DualBlocks | Vrais supports q2 (p < Kmax) | Plan q2 seul, Pool | Plan q2 seul, DualBlocks | Rejets non sûrs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8k | 11 329 | 7 718 | 2 140 | 7,4 ms | 115 ms | 0 / 559 790 |
| 16k | 29 878 | 14 019 | 3 085 | 14,6 ms | 247 ms | 0 / 559 870 |
| 32k | 102 993 | 31 419 | 4 690 | 31,7 ms | 553 ms | 0 / 559 866 |

Trois faits en sortent. Les produits inter-amas ne sont **pas** une pure
certification : ils portent 2 140 / 3 085 / 4 690 supports q2 réellement
retenus, presque tous dans les douze produits d'arêtes du cube (140 à 183
par produit à 8k, 309 à 458 à 32k ; diagonales de face 8 à 45, grandes
diagonales 0 à 5), et cette population croît de ×1,44 puis ×1,52 par
doublement, comme une surface. Les survivantes de Pool croissent bien
plus vite (×2,6 puis ×3,4) : à 32k elles valent vingt-deux fois la
vérité, celles de DualBlocks 6,7 fois (×1,8 puis ×2,2). Enfin, le plan
q2 seul coûte 7 à 32 ms sur les 28 rectangles ; les 60 à 241 ms du § 3
comprenaient les trois voies. Les histogrammes de profondeur des
survivantes vivantes (de 94 à 311 paires par valeur de p à 8k) sont dans
le reçu.

## 4. Conséquence pour le raccord

Une politique par taille suffit : rectangles à facteurs d'au plus
quelques dizaines de sites, chemin scalaire actuel ; rectangles à gros
facteurs, plan local Pool (ou DualBlocks pour q3/q4) avant le census,
sur les nœuds du front sans copie ni tri du nuage. À 32k, les 28 produits
inter-amas portent 448 des 460 millions de paires q2 du résidu : Pool y
coûte 241 ms (copie des facteurs comprise) et laisse 102 993 paires, ce
qui divise par 38 le résidu q2 total que le census des amas aurait à
payer. Ce n'est pas encore un temps de chaîne : la comparaison q2 seule,
génération, front, crédits, census et collecte compris, sur les mêmes
sources que son bras témoin, reste à faire par le constructeur ; les
173 s mesurés à e3af11a7 en sont le point de départ. Le seuil de taille
est une politique à mesurer, pas une hypothèse géométrique ; les crédits
restent sûrs quel que soit le seuil, puisque chaque plan P0 est un
minorant certifié (juges de la première tranche).

Cela ne rouvre pas les histogrammes quadratiques : Pool coûte
O(h(|A|+|B|)) par rectangle et n'est appelé que sur quelques dizaines de
produits ; ce sont les certificats de la première tranche, requalifiés
sur un vrai front. Le résidu q3/q4 des amas, lui, n'a pas encore de
consommateur ; ses 2 à 4 millions de paires par produit après Pool
tomberaient à quelques centaines de milliers avec DualBlocks.

## 5. Reproduction

```bash
mkdir -p /tmp/pinned && git archive da366f7f morsehgp3D_v8/src morsehgp3D_v8/bench morsehgp3D_v8/CMakeLists.txt morsehgp3D_v8/cmake | tar -x -C /tmp/pinned
cmake -S /tmp/pinned/morsehgp3D_v8 -B /tmp/pinned_build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && cmake --build /tmp/pinned_build --target mhgp8_p0
python3 -B morsehgp3D_v8/audits/credits_terminaux_20260914/run_credits_terminaux.py --lib /tmp/pinned_build/libmhgp8_p0.a --src-root /tmp/pinned/morsehgp3D_v8 --build-dir /tmp/credits_build --output /tmp/credits.json
python3 -B -O morsehgp3D_v8/audits/credits_terminaux_20260914/run_credits_terminaux.py --lib /tmp/pinned_build/libmhgp8_p0.a --src-root /tmp/pinned/morsehgp3D_v8 --build-dir /tmp/credits_build --quick --output /tmp/credits_O.json
```

Le reçu épingle les sources produit consommées (blobs de da366f7f), la
copie d'audit des tubes, les trois harnais et la bibliothèque liée. Le
second reçu (survivantes) se rejoue par
`python3 -B morsehgp3D_v8/audits/credits_terminaux_20260914/run_survivants.py`
avec les mêmes options `--lib`, `--src-root` et `--build-dir`.
