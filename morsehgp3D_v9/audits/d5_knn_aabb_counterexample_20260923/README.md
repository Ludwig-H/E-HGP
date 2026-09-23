# D5 : le nombre de nœuds du k-NN ne se borne pas par les sites fermés

23 septembre 2026. Audit de la proposition de saut au centre
[`PLAN_ETAPE1_SAUT.md`](../../docs/d5_conception_20260923/PLAN_ETAPE1_SAUT.md),
§ 1.4. Cadre `exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, `not_claimed`. Aucun changement du moteur.

La formule « pire cas O(|D̄∩P|) sur une coquille cosphérique massive »
n'est pas une borne du `CloudIndex` à boîtes AABB : les boîtes peuvent
intersecter la boule alors que leurs sites n'y sont pas. La
contre-épreuve entière u18 ci-jointe garde **7 sites dans D̄** et
augmente le nombre de visites avec les sites extérieurs :

| Sites extérieurs | Sites fermés | Nœuds internes survivants | Nœuds visités / total |
| ---: | ---: | ---: | ---: |
| 8 | 7 | 14 | 29 / 29 |
| 16 | 7 | 22 | 45 / 45 |
| 32 | 7 | 38 | 77 / 77 |
| 64 | 7 | 70 | 141 / 141 |

Ces quatre cas finis illustrent le défaut de la borne et ne prouvent
pas seuls un exposant asymptotique. La famille géométrique ci-dessous
donne l'argument général. Ni l'exactitude ni l'intérêt possible du saut
D5 ne sont réfutés.

Construction : centre (100000,100000,100000), rayon r=32044. Deux sites
diamétraux (±r,0,0) forcent D=MEB(s) pour toute facette de cinq sites
qui les contient. Cinq autres sites sont strictement intérieurs, près
de (0,r−1,0). Ainsi `p=5`, `q_min=2`, `K=5` et la fenêtre de D commence
à K=6 : D est une entrée géométriquement **non terminale** de la
sous-routine de saut à K5. La sonde ne démontre pas que ce quintuplet
est effectivement une facette émise par la tour FULL.
Les 64 autres sites entiers sont sur le cercle extérieur de rayon
R=32045 dans le premier quadrant (z central). La boule fermée contient
les sept premiers sites seulement. Les cinq plus proches du centre sont
les cinq intérieurs ; leur puissance maximale est τ=−64086.

Pour toute boîte de l'arbre radix contenant au moins deux sites de
l'arc extérieur, le minimum coordonnée par coordonnée donne une borne
inférieure strictement inférieure à τ, même si chaque site extérieur a
une puissance positive. La sonde appelle le vrai `build_cloud_index` et
`AxisBounds` ; avec le **seuil final**, donc le plus fort du parcours,
tous les nœuds internes des quatre préfixes survivent. Les feuilles
extérieures et les deux supports sont élagués, mais il faut visiter
tous les nœuds avant de les élaguer. Le seuil courant commence à 0 et
ne descend que vers τ : l'ordre DFS ou près-d'abord ne répare pas ce
contre-exemple.

Pour une famille de taille arbitraire quand le domaine des coordonnées
grandit, prendre n points rationnels distincts sur un court arc du
premier quadrant d'un cercle de rayon R. Pour deux points distincts de
cet arc, le coin inférieur de leur boîte a une norme **strictement**
inférieure à R. Comme le sous-ensemble est fini, choisir `r<R` assez
près de R pour que tous ces coins soient dans la boule de rayon r,
puis placer cinq intérieurs assez près de son bord pour que leur
puissance maximale τ dépasse toutes ces bornes de boîtes. Mettre l'arc
dans une cellule Morton séparée des sept sites fermés : son sous-arbre
contient n feuilles et n−1 nœuds internes, tous survivants. Il faut
donc Ω(n) visites alors que |D̄∩P|=7. Les coordonnées rationnelles se
mettent à l'échelle vers une grille entière ; la famille asymptotique
suppose que cette grille grandit. Le profil u18 fixe ne fournit ici
que les quatre contre-épreuves finies ci-dessus.

La borne utilisable est en nombre de nœuds dont la borne AABB survit au
seuil, avec un pire cas O(n), indépendamment de |D̄∩P|. Mesurer les
`tree_nodes` et tests de feuilles séparément sur les demi-scènes,
quarts de scènes et amincissements globaux 1/2 et 1/4 des LiDAR ; les
108 nœuds moyens du sidecar 8k/K10 ne constituent pas une borne de
croissance.

Reproduction : `python3 -B morsehgp3D_v9/audits/d5_knn_aabb_counterexample_20260923/run_and_check.py`
depuis le dépôt. Le script extrait en répertoire temporaire le
`src/tower` du commit produit `a6d08f05f47378b1483e630e2c75865571bc1ec0`
(arbre Git `38895277e70bb18a16cb29883d2d1edd1e5ef5be`), compile
[`probe.cpp`](probe.cpp) avec `g++ -std=c++20 -O2 -Wall -Wextra -Werror`,
et compare tous les champs à [`RESULT.json`](RESULT.json). Source
directement impliquée : `tree/cloud_index.hpp` SHA-256
`2426a0cfa42689aed25b8a9c6630d92c54aa3cf4704273d58173ccad29275c5f` ;
`pipeline/census.hpp` SHA-256
`f35bdf0d87a2e667fb54743790a83f329428c5e3f7a12f61ca1f0012a361352c`.
Le reçu archive le résultat, le code du test et leurs SHA-256 ; aucun
résultat de tour FULL, de LiDAR ni de G4 n'en découle.
