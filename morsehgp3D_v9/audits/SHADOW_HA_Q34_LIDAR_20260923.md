# SHADOW `H_a` sur rectangles q3/q4 LiDAR sans sol, grille 1 mm

23 septembre 2026. Observation CPU hors registre ; aucun rejet activé dans le produit, aucun résultat FULL/G4. La proposition et sa preuve sont dans [la piste B](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md). Le [sidecar exact](shadow_ha_q34_u18_20260923.cpp), sa [sortie K10](shadow_ha_scene00_k10_s8_replay_20260923.txt) et sa [mesure de ressources](shadow_ha_scene00_k10_s8_replay_20260923.time.txt) sont archivés ici.

## Méthode et portée

Source moteur v9 `610264dad26427f3c15e1850910b67cf6d5ab4f6`, checkout privé propre, GCC 13.3, Release `-O2`. Entrée v8 `receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le`, 39 885 **sites** sans sol, quantifiés à 1 mm ; `K10/s8`, front `MidpointSamples`, masque q3/q4 `6`. Le sidecar filtre chaque vrai rectangle WSPD avec les bornes `Affine` du produit, puis conserve son vrai `b_node`. Pour chaque ligne résiduelle `a×B_node` avec `|B|≥8`, il propose les `2K` sites les plus proches de `a` parmi les `8K` rangs spatiaux adjacents. Cette petite palette est heuristique : seuls les appels publics exacts `universal_witness(Q3/Q4,a,box(B_node),z)` créditent un ID distinct, avec égalité refusée et ID de B sauté. La voie q4 certifiée pour un même `z` implique q3. Les seuils sont `K−1` et `K−2`; les crédits des voies ne s'additionnent pas.

L'ordre des mesures est index → préparation de palette → front → filtre de rectangles → toutes les lignes avec l'API publique → toutes les mêmes lignes avec un prédicat fusionné expérimental (masque comparé **ligne par ligne**) → échantillon déterministe `1/512` des lignes avec ancien DFS, cache et toutes leurs paires → replay mono de **toutes** les paires sans palette → replay mono avec lignes fermées sautées et masques partiels transmis. Les deux replays gardent des caches indépendants et comparent le masque final de chacune des 30 777 213 paires ; pour une ligne sautée, le replay initial prouve que le OU de tous ses masques vaut zéro. `covers` compte seulement les paires dont au moins une voie passe le filtre ; aucun cover n'est construit par ce sidecar. Le test de paire `Affine` singleton×singleton est exact, alors que le DFS singleton×boîte peut rester indécis même si un témoin ponctuel passe les huit coins. Sur l'échantillon, chaque voie fermée par `H_a` est aussi fermée par le filtre de paire ; le DFS de ligne manque 11 unités de masse q4 certifiées par la palette.

## K10/s8, 08/000000

| Étape / masse | Compte exact |
| --- | ---: |
| Rectangles émis / masse du front | 4 782 714 / 153 941 650 |
| Masse retirée au filtre de rectangle | 123 164 437 |
| Rectangles / paires résiduels | 2 034 440 / 30 777 213 |
| Lignes `|B|≥8` / masse éligible | 632 560 / 25 345 687 |
| Paires dont q3 est fermée par `H_a` | 2 747 214 |
| Paires dont q4 est fermée par `H_a` | 3 764 127 |
| **Paires dont les deux voies présentes sont fermées** | **3 499 305**, soit 11,37 % du résidu |

Les masses par voie se recouvrent. La masse entièrement fermée par classes de `|B|` vaut : `8–15: 236 652 / 2 118 096`, `16–31: 526 379 / 4 179 219`, `32–63: 855 152 / 6 134 070`, `64–127: 1 167 998 / 8 113 786`, `128–255: 532 100 / 3 597 058`, `≥256: 181 024 / 1 203 458`. Les `|B|<8` portent 5 431 526 paires et ne paient pas cette palette. `H_a` propose 12 323 754 IDs sur les lignes éligibles, dont 667 sautés parce que dans B ; 536 331 lignes n'atteignent le seuil d'aucune voie. Les appels publics paient 19 876 621 requêtes et 36 896 355 tests de coins. Le prédicat fusionné expérimental rend le **même masque sur chaque ligne** avec 12 323 087 appels et 29 010 773 coins ; il n'a pas été porté au produit.

Le replay conserve **tous les masques de paires identiques**. Il donne 4 507 278 paires pouvant construire un cover dans les deux chemins : `H_a` n'économise donc **aucun cover ni chargement de formes** sur ces rejets. Les 3 499 305 paires sautées comprenaient surtout des rejets peu coûteux du cache. Les compteurs du chemin mono sont :

| Chemin de paire | Sans `H_a` | Avec `H_a` SHADOW |
| --- | ---: | ---: |
| Paires effectivement filtrées | 30 777 213 | 27 277 908 |
| Recherches exactes après cache | 11 799 271 | 10 971 449 |
| Rejets entiers par cache | 18 977 942 | 16 306 459 |
| Tests de nœuds du cache | 149 617 337 | 132 033 758 |
| Visites du filtre de paire | 959 838 242 | 918 510 247 |
| Paires passant vers un cover | 4 507 278 | 4 507 278 |

Sur cet hôte partagé, préparation de palette `0,0775 CPU·s`, appels publics sur toutes les lignes `0,6348 CPU·s` (fusion expérimentale `0,5366 CPU·s`). Le replay pair/cache donne `45,5732 → 43,5711 CPU·s`, soit **environ 1,29 CPU·s local net** après préparation et appels publics. Les phases ont été exécutées dans cet ordre, sans inversion de l'ordre des replays ; leurs chronos et le RSS de 223 352 KiB sont indicatifs. Ce RSS inclut les 30,78 M masques stockés pour la contre-vérification, pas une estimation de mémoire d'activation. Ni cover, formes, catalogue, tour FULL, travailleurs parallèles ou G4 ne sont chronométrés dans le replay.

## Contexte K5 et décision

Deux observations K5/s8 antérieures, avec la même géométrie `H_a` mais une version du sidecar **avant** l'ajout du replay, sont conservées : [08/000000](shadow_ha_scene00_k5_s8_context_20260923.txt) ferme `5 591 944 / 23 686 751` paires résiduelles (23,6 %), et [08/000100](shadow_ha_scene01_k5_s8_context_20260923.txt) `2 499 743 / 11 960 420` (20,9 %). Sur l'échantillon K5 de la première trame, les 10 577 paires de lignes fermées étaient toutes déjà rejetées par le filtre exact : 8 771 par le cache seul, 1 806 après recherche. Ces deux trames appartiennent à **la même séquence 08** ; aucun test s10/s12, brut avec sol ou autre séquence n'est inclus.

La contre-fixture de B dans [la piste](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md) place les `2K` voisins les plus proches de `a` dans la mauvaise direction alors que des témoins plus loin ferment la ligne. Notre palette de rangs spatiaux a la même limite de sélection ; un échec n'est jamais une preuve de survie. Le gain local K10 est réel dans ce replay, mais faible à l'échelle de la chaîne et ne réduit aucun cover. Garder `H_a` en SHADOW. La prochaine porte utile est une ablation **chaîne ON/OFF** avec préparation incluse, caches laissés évoluer par worker, identités exactes des supports/coquilles/catalogue/FULL, compteurs de paire/cover/formes et RSS, sur K5/K10, s8/10/12 et plusieurs séquences. Ne pas annoncer de gain G4 ni activer le rejet par défaut sur cette seule sonde.

## Provenance hachée

`sha256` : sidecar source archivé `f42d570bb31d95c2996b7fd958f1c32b62bfa50405438da0adbeb817e353b119` ; binaire local `/tmp/mhgp9_ha_shadow_20260923` `3759482d93f0a71ea1ac99dd55473e2d79545c2e6978fe895658d013b9da50b9` ; bibliothèque construite `/tmp/mhgp9-ha-build-20260923/libmhgp9_gen.a` `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a` ; entrée 08/000000 `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf` ; [sortie K10](shadow_ha_scene00_k10_s8_replay_20260923.txt) `7e7945f44bb90dab34d9de9b0b1fcc47644ba1267df2a1fe00b7b0b14980a56e` ; [temps K10](shadow_ha_scene00_k10_s8_replay_20260923.time.txt) `0ca187a80084e09918cca6571031c26f76eb13c9e7fc38bc797ff81809a112e7`. Binaire/build et entrée v8 sont des dépendances locales LIVE, pas une archive autonome. Les deux sorties K5 ont pour hashes `82017bf31217353d7137274e2132abb5ed91697054bb44366e7ae9d3fa42f948` et `9640fd70750cd02f0564335b05329b7801b071eda4f4350dbd52fccd596e54ce` ; leur sidecar antérieur avait les hashes source/binaire `645556e7b4d23ac80f3f2ddcf51524b95bb245be82da78c6931fc5a2370ee468` / `ed0f7728ed355ad9b57e78678b1b4287053e83f8fbc06fbf917f51304b709023`. Entrée 08/000100 : `ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f`.
