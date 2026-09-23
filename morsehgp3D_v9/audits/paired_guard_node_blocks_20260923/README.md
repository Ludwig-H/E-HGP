# Paires de nœuds avant le cœur : gain local sous budget, rendement global ouvert

23 septembre 2026. Shadow CPU **hors produit et hors registre** sur les 120
arêtes S2 de deux réservoirs épinglés dans
[`paired_guards_precore_20260923`](../paired_guards_precore_20260923/README.md).
Une seule trame SemanticKITTI brute 08/000000, grille commune 1 mm/u18,
K5/s8. Le tirage est stratifié selon `F`, taille du cœur diamétral **connue
après construction de ce cœur** : les taux de ce panel ne décrivent pas le
flux S2 complet, et aucun déclencheur pré-cœur économique n'est acquis.

Le [sidecar](node_blocks.cpp) prépare le nuage et construit **une fois**
l'index spatial natif immuable de 246 777 nœuds pour les 120 arêtes. Quatre
recherches prioritaires, une par quadrant du plan perpendiculaire à `ab`,
explorent les boîtes par distance minimale au milieu. Un nœud candidat
contient au plus `cap=1/2/4/8` sites, appartient entièrement au quadrant
et au cœur diamétral. Un nœud contenant `a` ou `b` est subdivisé, jusqu'à
écarter ces feuilles. Le budget de 64 ou 256 **nœuds dépilés par quadrant**
arrête une recherche incomplète ; une telle interruption ne rejette aucune
arête. Il reste jusqu'à 16 nœuds par quadrant, puis toutes leurs paires de
nœuds sont testées. Le glouton trie les paires certifiées par population
appariable, puis marge ; les nœuds retenus sont disjoints pour **chaque**
voie. Les voies q3/q4 peuvent réutiliser les mêmes sites entre elles, sans
addition de crédits. La sélection est une heuristique de propositions,
pas une preuve de complétude des échecs.

Pour les boîtes `G,H`, `s=a+b`, `d=b−a`, `D=|d|²`, on prend les intervalles
`W_G,i=[2loG_i−s_i,2hiG_i−s_i]` et `W_H,i` de même. `Qmax` est la somme des
plus grands carrés de leurs extrémités ; `Hmin=2D−QmaxG−QmaxH`.
L'arithmétique d'intervalles signés sur `d×(W_G+W_H)` donne `Xmax` comme
somme des plus grands carrés de chaque composante. `Hmin>0` et
`3Hmin²>4Xmax` certifient **chaque** paire transversale en q3 ;
`Hmin²>2Xmax` fait de même en q4. Les égalités ne créditent rien.
Deux nœuds disjoints apportent `min(|G|,|H|)` paires de sites distincts.
Les produits sont promus avant multiplication en `i128` signé : sous u18,
`|Hmin|<2^41`, `Xmax<2^80`, `3Hmin²<2^83`, `4Xmax<2^82`.

| Méthode / budget | Fermées / 120 | F fermable / 520 631 | Nœuds dépilés | Boîtes à l'enfilement | Tests de paires |
| --- | ---: | ---: | ---: | ---: | ---: |
| Top-B points B16, 64 | 21 | 64 429 | 30 334 | 58 444 | 11 751 |
| Nœuds cap4, 64 | **32** | **110 327** | 30 314 | 57 794 | 14 624 |
| Nœuds cap8, 64 | **34** | **121 765** | 30 314 | 57 486 | 16 365 |
| Top-B points B16, 256 | 67 | 280 728 | 48 553 | 83 768 | 195 514 |
| Nœuds cap4, 256 | 67 | 285 918 | 47 593 | 82 160 | 184 736 |
| Nœuds cap8, 256 | 65 | 273 822 | 46 624 | 80 268 | 183 266 |

**Erratum du ledger de bornes.** Dans ce sidecar, chaque nœud dépilé
recalcule sa borne après celle de l'enfilement, sans incrémenter la colonne
« Boîtes à l'enfilement ». Les **calculs effectifs de bornes** valent donc
`boîtes + nœuds dépilés` : pour cap4, **88 108** à budget64 et
**129 753** à budget256, contre **58 444** et **83 768** pour Top-B,
qui transporte sa borne dans la file. La
[contrelecture indépendante](../CONTRE_AUDIT_B_COUT_NOEUDS_PAIRES_20260923.md)
donne le contrôle de source. Les preuves géométriques et les fermetures
ne changent pas.

Les lignes Top-B proviennent du
[shadow indexé voisin](../paired_guard_index_shadow_20260923/SUMMARY.json) ;
notre cap1 reproduit ses fermetures et `F` aux deux budgets. À budget64,
cap4 prouve 11 arêtes supplémentaires et 45 898 sites de cœur cumulés par rapport
aux points seuls, au prix de 2 873 tests de paires et **29 664 calculs de
bornes effectifs** supplémentaires. Les
quatre quadrants ne sont complets pour aucune des 120 arêtes Top-B à ce
budget (450/480 interruptions) ; ce gain est donc un **gain de preuve
positive sous budget**, pas une différence de vérité géométrique. Pour
cap4 à budget64, 88 arêtes restent ouvertes après **22 274 visites,
42 974 boîtes à l'enfilement, donc 65 248 bornes effectives, et
6 050 tests de paires**. À budget256, les 53 ouvertes
consomment encore **21 037 visites, 36 514 boîtes à l'enfilement,
donc 57 551 bornes effectives, et 75 332 tests de
paires**. Le dernier budget ferme autant d'arêtes que Top-B : cap4 en
gagne deux et en perd deux, soit seulement +5 190 `F` nets (+1,85 % des
`F` fermables par Top-B). Cap8 perd deux fermetures nettes. Tous les
budgets 256/1024/4096 produisent les mêmes comptes ; 256 suffit ici à
remplir les 16 nœuds par quadrant. Le temps mesuré du sidecar varie avec
la charge de l'hôte et exclut la lecture, le contrôle indépendant de `F`,
la construction de l'index, le cœur, le cover et le catalogue ; comparer
les bornes effectives, les visites **et** les tests de paires. La préparation/index du
rejeu local vaut 54,3 ms et conserve 19,2 Mo pour l'index, partagés par
les 120 arêtes.

Un témoin où les blocs aident réellement au budget64 est `a=47537`,
`b=62756`, q4 seule : un bloc de quatre sites de boîte
`[78538,72851,27105]..[78593,72893,27198]` et un autre de quatre
sites de boîte `[77432,73765,26176]..[77495,73806,26189]` satisfont
`Hmin=6 068 880`, `Xmax=11 231 878 571 020`, donc
`Hmin²>2Xmax`. Les 16 paires transversales et les identités de sites
sont recontrôlées à partir du [reçu](RESULT.stdout) ; le
singleton à ce budget ne ferme pas cette arête. Le
[lecteur indépendant](verify.py) contrôle les **758 certificats de
paires de blocs enregistrés**, les boîtes, toutes leurs paires ponctuelles,
les seuils, les IDs disjoints et les égalités strictes. Son mode LIVE
contrôle la correspondance de ces IDs/coordonnées aux fichiers u18 de SHA
épinglés. Clang ASan/UBSan a rejoué les 1 920 lignes de mesure et produit
les mêmes décisions, compteurs et preuves que Release.

Le [diagnostic de seuil D](../paired_guard_index_shadow_20260923/TRIGGER_D.json)
donne un déclencheur calculable avant `make_diametral`, mais `D≥2^22`
conserve encore 909 278 des 3 986 433 arêtes S2 (22,8 %) tout en couvrant
ce panel ; `D≥2^23` n'en conserve que 63/67 fermables B16. Ce crible seul
ne justifie pas d'appeler quatre recherches et un appariement sur le flux
entier. Les `F` fermables du tableau sont un potentiel avant `load`, jamais
une baisse mesurée du temps de chaîne, du catalogue, de FULL ou du GPU.

Relecture statique :

```sh
python3 -B morsehgp3D_v9/audits/paired_guard_node_blocks_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/paired_guard_node_blocks_20260923/verify.py
```

Le mode LIVE ajoute `--points` et `--raw-ids` avec les fichiers
`s00_full_full.*.u32le` régénérés par
[`lidar_raw_physical_scaling_20260923/generate.py`](../lidar_raw_physical_scaling_20260923/generate.py).
Le [manifeste](MANIFEST.json) donne les SHA, la commande de compilation,
les dépendances et la portée de ce rejeu.
