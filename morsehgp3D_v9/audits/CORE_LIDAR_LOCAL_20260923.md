# Audit local — noyau diamétral q3/q4 sur deux coupes LiDAR sans sol

23 septembre 2026. **Diagnostic local, hors registre, sans G4 ni tour FULL.**
Toutes les commandes ci-dessous partent de
`/workspaces/E-HGP/build/v9-open-worktree` ; les deux fichiers d'entrée
sont pris dans le répertoire du reçu v8 indiqué plus bas.
Produit Release : arbres Git `morsehgp3D_v9/src`
`96a4bb5fe7cf7754684fc5f45974369fcd6b8d85` et `bench`
`0efb28b5671a524508a245f3d998240eb04e8dad`, identiques aux
snapshots `f599aed7` et `a5872918` malgré les commits de protocole et
audit intervenus pendant la lecture. Le binaire
`build/v9-dev/mhgp9_tower_probe` vaut SHA-256
`8325847b1d6cf38c50f88edcdad4d627472cd7693b3f30cc6cd41181db508a83` ;
`libmhgp9_gen.a` vaut
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`.
Le build est `Release` avec `/usr/bin/c++`.

Deux entrées autonomes, figées dans le reçu v8
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/`
(grille u18/1 mm, masque sans sol de **08/000000**, une seule séquence) :

| Fichier `.u32le` | Sites | SHA-256 |
| --- | ---: | --- |
| `quarter_x_nonneg_y_neg` | 8 225 | `e9c5ba569ef8b8293025c848786d9d50ba33c68b5ab5d3f06ff8670de799584a` |
| `quarter_x_neg_y_nonneg` | 13 055 | `6020856e9c6f6d6ad596afd945ae970ed4062ffd4670a73486bc2a57cb2bf3d6` |

La sonde a été appelée, séquentiellement, dans l'ordre **8 225/K5 OFF,
ON ; 8 225/K10 OFF, ON ; 13 055/K10 OFF, ON**, une seule fois par
configuration, avec
`build/v9-dev/mhgp9_tower_probe INPUT.u32le K 8 --s=8 --no-tower
--lever=q34_dead_core=0|1` et les quatre autres leviers à leur défaut
ON. `--no-tower` exécute la génération, fusion et recensus du catalogue,
puis s'arrête : les temps de chaîne ne sont **pas** des temps FULL. L'hôte
était partagé ; les temps mur ci-dessous ne sont pas une ablation G4.

| Sites/K | Cœur | q3/q4 mur ms | Chaîne mur ms | Chaîne CPU s | Paires développées | Covers complets | Cores clos |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 225/K5 | OFF | 3 153,857 | 3 526,318 | 17,779 | 2 054 188 | 331 218 | 0 |
| 8 225/K5 | ON | 2 661,174 | 3 048,893 | 16,757 | 2 054 188 | 155 078 | 176 140 |
| 8 225/K10 | OFF | 8 963,779 | 10 202,532 | 53,275 | 2 702 021 | 717 196 | 0 |
| 8 225/K10 | ON | 9 899,579 | 11 166,164 | 50,272 | 2 702 021 | 334 720 | 382 476 |
| 13 055/K10 | OFF | 23 218,492 | 27 173,514 | 163,357 | 4 921 345 | 1 333 723 | 0 |
| 13 055/K10 | ON | 20 994,754 | 24 804,248 | 142,978 | 4 921 345 | 664 906 | 668 817 |

Le cœur ferme 50,1–53,3 % des arêtes qui arrivent au cover. Son effet de
coût change avec la coupe : à 8 225/K10, les visites de nœuds index
**augmentent de 83,1 %** et le mur q3/q4 régresse ; à 13 055/K10,
les visites **baissent de 10,1 %** et le mur q3/q4 baisse. Les tests de
formes uniformes baissent dans les deux cas. Ce sont deux géométries
d'une même trame, pas une loi de taille ni une conclusion de croissance.

| Sites/K | Cœur | Visites core + cover | Sites core + cover | Tests uniformes core + cover |
| --- | --- | ---: | ---: | ---: |
| 8 225/K5 | OFF | 0 + 36 418 620 | 0 + 222 412 936 | 0 + 429 882 598 |
| 8 225/K5 | ON | 48 632 324 + 13 347 736 | 33 321 585 + 33 462 482 | 73 779 788 + 131 455 908 |
| 8 225/K10 | OFF | 0 + 84 476 660 | 0 + 573 882 591 | 0 + 1 338 059 598 |
| 8 225/K10 | ON | 121 305 018 + 33 365 206 | 95 610 039 + 106 704 696 | 261 920 416 + 451 286 834 |
| 13 055/K10 | OFF | 0 + 449 452 095 | 0 + 1 633 564 163 | 0 + 3 894 057 821 |
| 13 055/K10 | ON | 265 561 763 + 138 359 572 | 115 018 601 + 219 867 745 | 264 040 611 + 849 476 630 |

`sites` compte les deux extrémités par cover ; les formes chargées hors
extrémités valent donc `sites−2·builds`. Les paires développées sont
**inchangées** : le noyau intervient après leur expansion et leur filtre
de témoins. Son bénéfice local dépend du rapport entre nœuds supplémentaires
du core et covers/formes/tests évités ; il ne retire pas le verrou
structurel de l'énumération de paires ni le coût FULL.

Une variante « cover complet d'abord, puis filtre diamétral » n'est pas
recommandée sur ces coupes sans prototype favorable. À 13 055/K10, elle
devrait examiner les **1 633 564 163** incidences du cover complet pour
former le core, contre **115 018 601** incidences core dans l'ordre
actuel ; elle matérialiserait aussi le cover des **668 817** arêtes que
le core ferme. À 8 225/K10, les valeurs correspondantes sont
**573 882 591** contre **95 610 039**, avec **382 476** arêtes closes.
Un constructeur imbriqué en un seul DFS pourrait partager des visites,
mais toute matérialisation anticipée du full sur les arêtes closes et
ses tests supplémentaires doivent être payés. Les deux coupes ne
permettent pas de prédire son gain.

**Identité du flux.** La sonde publie les mêmes présentations q3/q4 et le
même nombre de clés de catalogue en ON/OFF : respectivement
`101 780/15 416/199 112` (8 225/K5),
`407 424/157 583/719 375` (8 225/K10) et
`1 195 544/837 167/2 354 410` (13 055/K10).
La comparaison indépendante
[`check_q34_core_stream_local_20260923.cpp`](check_q34_core_stream_local_20260923.cpp)
va plus loin : elle trie toutes les présentations par **clé exacte,
arité, support, profondeur et tous les IDs de coquille**, puis compare
les vecteurs entiers. Elle rapporte `equal=true` pour les trois paires,
sur **117 196**, **565 007** et **2 032 711** présentations chacune.
Compilation : `g++ -std=c++20 -O2 -Wall -Wextra -Werror -pthread
-Imorsehgp3D_v9/src/gen -Imorsehgp3D_v9/src
morsehgp3D_v9/audits/check_q34_core_stream_local_20260923.cpp
build/v9-dev/libmhgp9_gen.a -o /tmp/check_q34_core_stream_local_20260923`.
Empreinte SHA-256 de la source :
`4a180655bb6e942970dbd51f9ac0d5688e7d7f5b9820784d837665b108127610` ;
du binaire local :
`0d229d8f31de9ea4c6c2547da42fc282d391e57495f3cf0eb4b35f455daef6ed`.
Chaque comparaison exécute
`/tmp/check_q34_core_stream_local_20260923 INPUT.u32le K 8`.
Les portes produit `mhgp9_gen_q34_cover_gate --selftest`
(18 029 assertions) et `mhgp9_gen_wspd_q34_gate --selftest`
(27 674 assertions) passent aussi sur ce build. Le comparateur local
vérifie le flux q3/q4 ; il ne rejoue ni q2 ni la tour FULL sur ces coupes.
