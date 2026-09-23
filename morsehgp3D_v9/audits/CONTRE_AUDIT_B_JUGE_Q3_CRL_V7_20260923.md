# Contrelecture B du juge q3 v7 : strate longue utile, portée encore bornée

23 septembre 2026. Lecture indépendante de
[`c_omission_20260923/results/gates_v7/`](c_omission_20260923/results/gates_v7/),
du juge C et de son lanceur, sans nouvelle exécution ni modification du
moteur. Code juge `9ffb871e6`, bibliothèques reconstruites depuis
`7565451fc` selon `PROVENANCE.txt` ; ne pas transférer ces résultats au
chemin GPU S2 ni à la tour complète.

## Ce qui est effectivement mieux jugé

La strate critique longue est maintenant définie de façon cohérente :
`p=Kmax−2`, coquille de trois sites, triangle aigu sans paire antipodale,
**plus longue arête** d'au moins 1 600 unités de grille. `drop-long`
emploie cette même arête, là où son ancienne définition par les deux
partenaires d'une ancre pouvait épargner le cas visé. La fixture de 33
sites autour de l'ancre 0 tue `drop-long` et `drop-crl` par le motif
`PRUNE_DISAGREES_CRL`. Sur huit ancres isolées de chacune des coupes
LiDAR s00/s01/s02 à 8k/K10, le juge brut trouve respectivement
**92/179/17 triangles CRL distincts**, et le parcours élagué sain ne
diverge pas. Les mutants sont refusés sur 5/8/3 ancres ; une clé CRL
retirée du catalogue est déclarée absente dans chaque cas.

Ce sont des portes causales utiles, **pas une preuve d'exhaustivité**.
`drop-crl` et `drop-long` modifient le parcours *du juge* ; leur rejet
prouve sa sensibilité à ces omissions internes, non l'existence d'une
injection identique dans le générateur produit. Le retrait ciblé d'une
clé du catalogue est la porte produit distincte, sur une clé par cas.
Les 92/179/17 clés ne sont qu'environ 0,061 %, 0,188 % et 0,016 % des
populations q3 de tête respectives **telles que comptées par le catalogue
produit** ; ce dénominateur n'est pas un oracle indépendant. Le juge ne
fait pas tourner FULL (`run_tower=false`), partage l'index et `BallData`
avec le produit, et ne couvre pas les ancres non tirées.

## Petite fermeture de confiance recommandée

Les deux juges (`q2_sample_judge.cpp:161–167`,
`q3_sample_judge.cpp:388–396`) contrôlent `n==points.size()`, puis
comparent `pos[u]` à `points[ix.point_id(u)]`. Ils **indexent avant de
vérifier** `point_id(u)<n`, et ne prouvent pas que les IDs forment une
permutation complète. Avec un index corrompu et des coordonnées répétées,
un doublon d'ID ou l'omission d'un retour pourrait conserver la simple
égalité de positions ; un ID hors bornes déclencherait un accès invalide
dans le juge. Cela n'invalide pas les entrées uniques vérifiées ni les
résultats observés ; cela limite l'indépendance de la garde.

Avant d'étendre la prétention du juge, vérifier pour chaque rang
`id<n`, puis unicité/couverture des IDs (bitset de `n` bits), et, si
possible, comparer le multiensemble coordonnées+ID directement à
l'entrée avant l'échantillonnage. Ajouter des mutants d'index
`duplicate_id`, `missing_id`, `out_of_range_id` dont le refus survient
avant le parcours géométrique. Archiver enfin stderr, journal de build
et sortie du selftest annoncés, absents du reçu v7 actuel, pour rendre
la porte rejouable depuis ses seules pièces.
