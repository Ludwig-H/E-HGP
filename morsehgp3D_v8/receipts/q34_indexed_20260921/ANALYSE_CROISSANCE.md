# Tranche 32 — croissance du travail global q3/q4 sur LiDAR scan 0

## Conclusion

Le critère empirique fort « tous les postes importants augmentent de moins de quatre lorsque n double » **n'est pas satisfait**. Les principales anomalies sont la recherche supplémentaire de témoins **par paire**, puis, à K5 avec Local28, les visites répétées de la carte des centres. En revanche, les sorties, les scans utiles q4 et le census q3 progressent moins vite que le quadruplement sur les deux doublements mesurés.

Window30 réduit fortement le nombre de seeds q4, mais sa préparation des couches duales et ses deux scans restent coûteux. À 32k/K10, il paie 22,038 milliards de comparaisons/orientations de sélection, puis 3,698 milliards de lectures de sites dans les familles. Le petit tri final ne résume donc pas son coût.

Ces observations ne prouvent **ni** une borne générale sous-quadratique **ni** une borne inférieure superquadratique. Elles localisent du travail à réduire, pour un scan et deux valeurs de K. Il ne s'agit pas de la tour FULL, d'un contrat 50k, ni d'une mesure GPU.

## Provenance et clôture

Autorité principale : [lidar_86twby55/MANIFEST.json](lidar/lidar_86twby55/MANIFEST.json) et sa [fermeture](lidar/lidar_86twby55/COMPLETION.json), close `passed` le 21 septembre 2026 à 09:04:45 UTC, sans erreur de fermeture. Douze commandes, douze mesures ; 206 sources épinglées, quatre fichiers d'entrée. Build `build/v8_q34_indexed_20260921`. Le commit indiqué par le manifeste est un contexte de travail ; les hashes des sources, du build et des entrées sont l'autorité effective.

SHA-256 du manifeste : `2af1415b588f0466174ed5bdd3facc9a630afb70bba0fbca6dec1aab6483b293`. SHA-256 de la fermeture : `c26f59dfb948ad207d387463fcd6d0cc7b5f7a84c1d6288de152700727314759`.

Configuration : scan 0 préparé `single_000000`, n=8k/16k/32k, K=5/10, s=8, masque q3/q4=6, quatre workers, front `samples`, recherche `rectangle-pair`, census q3 `boxes`, callback `digest`. L'affinité disponible couvre les CPU logiques 0 à 7 ; elle ne constitue pas une preuve de quatre cœurs physiques exclusifs. Local28 emploie les options par défaut de cette version. Les fichiers préparés d'audit sont consommés en lecture seule, avec leurs propres hashes ; leurs qualifications antérieures ne sont pas transférées.

| n | K5 Local28 | K5 Window30 | K10 Local28 | K10 Window30 |
|---|---|---|---|---|
| 8k | [0000](lidar/lidar_86twby55/record_0000.json) | [0001](lidar/lidar_86twby55/record_0001.json) | [0002](lidar/lidar_86twby55/record_0002.json) | [0003](lidar/lidar_86twby55/record_0003.json) |
| 16k | [0004](lidar/lidar_86twby55/record_0004.json) | [0005](lidar/lidar_86twby55/record_0005.json) | [0006](lidar/lidar_86twby55/record_0006.json) | [0007](lidar/lidar_86twby55/record_0007.json) |
| 32k | [0008](lidar/lidar_86twby55/record_0008.json) | [0009](lidar/lidar_86twby55/record_0009.json) | [0010](lidar/lidar_86twby55/record_0010.json) | [0011](lidar/lidar_86twby55/record_0011.json) |

La lecture suivante a été exécutée après clôture, avec puis sans `-O` : les deux sorties JSON sont identiques et `passed`. Aucune exécution native supplémentaire n'a été lancée pour cette analyse.

La clôture générale des lectures est conservée séparément dans [FINAL_READBACK.json](FINAL_READBACK.json), également `passed`, sans modification des 206 sources, des entrées ni des artefacts qu'elle contrôle.

```sh
python3 -B morsehgp3D_v8/bench/run_q34_indexed_checks.py read morsehgp3D_v8/receipts/q34_indexed_20260921/lidar/lidar_86twby55 --check-live --compact
python3 -B -O morsehgp3D_v8/bench/run_q34_indexed_checks.py read morsehgp3D_v8/receipts/q34_indexed_20260921/lidar/lidar_86twby55 --check-live --compact
```

La première série [lidar_2x8pm0nw](lidar/lidar_2x8pm0nw/COMPLETION.json), close à 08:47:20 UTC, constitue une répétition distincte de K5/Local28. Ses trois sorties, comptes du front, covers, census q3 par blocs et comptes Local28 sont exactement ceux de la matrice principale. Ses temps restent des observations distinctes. Les séries `lidar_tgo9mbax` et `lidar_b7fo1a0i`, limitées à 8k, ne permettent pas de calculer une croissance en n et ne sont pas mélangées aux tableaux suivants.

## Ce qui est compté — et ce qui ne doit pas être additionné

On conserve un **vecteur de travail**, pas un total artificiel d'« opérations CPU ». Une visite, une comparaison entière, une orientation et une copie d'ID n'ont pas le même prix. Les quelques sommes ci-dessous réunissent des appels réellement exécutés dans des étapes distinctes ; les tableaux ne sont pas destinés à être additionnés entre eux.

| Poste | Compteurs retenus et précautions |
|---|---|
| Préparation commune | `cloud_work.uniqueness_comparisons`, `index_work.point_visits` ; copies, validation et stockage linéaires gardés séparés. |
| Front historique | `front.work.product_visits` et `witness_box_distance_tests` ; ne pas y ajouter les populations de paires représentées comme si elles avaient été énumérées. |
| Recherches supplémentaires | Pour `work.witness.rectangles` et `.pairs`, addition des H, Xi, tests de voie et tests d'ordre **entre les deux étages seulement**. `node_visits == h_bound_tests` : un seul des deux est compté. `point_tests` est un sous-ensemble de ces visites. |
| Cover | `work.cover.node_visits == bound_tests + point_tests`. `admitted_sites` et `rejected_sites` sont des populations certifiées, pas des lectures scalaires. |
| Seeds q3 | `work.q3.seed_node_visits` et `seeds == ball_builds`, sans addition de ces deux alias. Les tests de propriété sont un coût séparé. |
| Census q3 | `work.q3_blocks.count_bounds_prepared + shell_bounds_prepared`. Cette somme inclut les enfants préparés mais non visités après saturation : leur calcul a réellement été payé. Les populations `count_inside_sites` et `count_nonnegative_sites` ne s'y ajoutent pas. |
| Local28, préparation | Visites de décomposition du cover et de domaine positif, projections, tri/orientations du domaine, tests de domaine des cellules. |
| Local28, partition | `atlas.partition.block_bound_tests + point_tests` compte les évaluations géométriques ; `frontier_ids_copied` compte les copies effectives de nœuds. Les nœuds conservés sans test sont un coût structurel distinct. `input_sites`/`active_sites` de la partition sont des populations représentées. |
| Local28, par seed | `sweep.query_visits`, `line_tests`, puis `sweep.active_sites`, qui désigne ici de **vraies lectures scalaires**, contrairement au champ homonyme de partition. `root_locations` et comparaisons de tri/groupement sont des étapes supplémentaires. |
| Window30, couches | `selection.form_tests` scanne le cover ; somme `lex_comparisons + orientation_tests + retained_id_sort_comparisons`. `layer_input_groups`, `hull_index_copies` et `compaction_moves` décrivent des boucles/copies réelles. Ne pas transformer `layer_input_ids`, une somme des poids des groupes, en visites individuelles. |
| Window30, familles | `sweep.family.sites + window.second_pass_sites` : deux passes distinctes, leur somme est légitime. Le tri final seul omet les tas et les comparaisons aux bornes. |
| Window30, racines | Somme `window.heap_comparisons + heap_sort_comparisons + window_comparisons + sweep.family.sort_comparisons + group_comparisons` ; aucun de ces cinq compteurs n'inclut les autres. |
| Sorties | `output.callbacks`, `support_ids`, `shell_ids` comptent le payload réellement parcouru. Les coquilles sont complètes, mais le mode `digest` ne stocke pas un catalogue global ni ses incidences pour FULL. |

Ces significations ont été recoupées dans [q34_witness_search.cpp](../../src/lanes/q34_witness_search.cpp), [q3_ball_census.cpp](../../src/lanes/q3_ball_census.cpp), [q4_local.cpp](../../src/lanes/q4_local.cpp), [q4_local_partition.cpp](../../src/lanes/q4_local_partition.cpp), [q4_shallow_set.cpp](../../src/lanes/q4_shallow_set.cpp) et [q4_window.cpp](../../src/lanes/q4_window.cpp), épinglés par le manifeste principal.

## Préparation et travail commun aux deux backends

La préparation partagée reste modérée sur ces entrées : comparaisons d'unicité 122134/266299/560576, soit ×2,180/×2,105 ; visites de points pendant construction de l'index 274802/584840/1238658, soit ×2,128/×2,118. Le nombre de nœuds de l'index vaut 15999/31999/63999. Cela ne borne pas les recherches ultérieures.

Toutes les valeurs des tableaux suivants sont en **millions**, sauf les facteurs de croissance. Les ratios sont calculés sur les entiers non arrondis ; les deux facteurs correspondent à 8k→16k puis 16k→32k.

### K5

| Travail commun | 8k | 16k | 32k | Ratios |
|---|---:|---:|---:|---:|
| Front : produits | 1,104 | 2,310 | 4,632 | 2,093 / 2,005 |
| Front : distances de boîtes | 35,774 | 79,946 | 170,440 | 2,235 / 2,132 |
| Paires réellement développées | 0,345 | 0,901 | 2,453 | 2,612 / 2,724 |
| Recherches supplémentaires : H | 61,494 | 161,833 | 701,165 | 2,632 / **4,333** |
| Recherches supplémentaires : Xi | 24,653 | 70,121 | 443,262 | 2,844 / **6,321** |
| Recherches supplémentaires : tests de voie | 38,899 | 106,792 | 574,403 | 2,745 / **5,379** |
| Recherches supplémentaires : ordre des enfants | 66,366 | 175,253 | 739,165 | 2,641 / **4,218** |
| Cover : visites | 18,621 | 46,776 | 117,231 | 2,512 / 2,506 |
| q3 : parcours des seeds | 16,402 | 42,171 | 113,462 | 2,571 / 2,691 |
| q3 : seeds/boules | 1,911 | 6,033 | 20,363 | 3,156 / 3,375 |
| q3 : bornes préparées, compte+coquille | 72,936 | 225,238 | 760,044 | 3,088 / 3,374 |
| q3 : comparaisons de tri de coquille | 0,328 | 0,663 | 1,336 | 2,023 / 2,016 |
| Sorties : callbacks | 0,105 | 0,212 | 0,427 | 2,024 / 2,014 |
| Sorties : IDs de coquille | 0,325 | 0,658 | 1,328 | 2,025 / 2,019 |

### K10

| Travail commun | 8k | 16k | 32k | Ratios |
|---|---:|---:|---:|---:|
| Front : produits | 1,583 | 3,306 | 6,498 | 2,088 / 1,965 |
| Front : distances de boîtes | 51,598 | 115,194 | 240,810 | 2,233 / 2,090 |
| Paires réellement développées | 0,718 | 1,814 | 4,394 | 2,527 / 2,422 |
| Recherches supplémentaires : H | 154,309 | 434,629 | 1490,427 | 2,817 / 3,429 |
| Recherches supplémentaires : Xi | 69,580 | 225,655 | 969,582 | 3,243 / **4,297** |
| Recherches supplémentaires : tests de voie | 116,362 | 351,095 | 1473,228 | 3,017 / **4,196** |
| Recherches supplémentaires : ordre des enfants | 162,734 | 458,053 | 1548,404 | 2,815 / 3,380 |
| Cover : visites | 53,289 | 127,678 | 307,157 | 2,396 / 2,406 |
| q3 : parcours des seeds | 49,681 | 117,090 | 283,821 | 2,357 / 2,424 |
| q3 : seeds/boules | 8,099 | 20,258 | 53,125 | 2,501 / 2,622 |
| q3 : bornes préparées, compte+coquille | 342,892 | 861,715 | 2253,761 | 2,513 / 2,615 |
| q3 : comparaisons de tri de coquille | 1,429 | 2,952 | 5,993 | 2,065 / 2,030 |
| Sorties : callbacks | 0,526 | 1,096 | 2,234 | 2,083 / 2,038 |
| Sorties : IDs de coquille | 1,696 | 3,542 | 7,233 | 2,089 / 2,042 |

La recherche **par paire** est responsable des sauts du filtre à 32k. À K5, ses bornes H font ×5,364, Xi ×7,440, tests q3 ×9,972 et tests q4 ×4,341 sur le second doublement. Les mêmes postes de la recherche par rectangle restent sous ×2,57 sur ce doublement. À K10, les recherches par paire font encore ×4,693 sur Xi et ×5,214 sur la voie q4. Les recherches ne sont donc pas rendues sous-quadratiques par leur seule saturation à petit K.

Le code actuel peut éliminer un nœud de témoins par Hmax≤0, ou créditer un nœud entièrement dans le citron. Mais, lorsque le nœud rencontre H>0 sans fournir assez de témoins, l'absence d'un rejet certifié par une borne inférieure de Xi peut obliger à descendre très loin. C'est une explication architecturale compatible avec ces comptes, pas une attribution profilée de chaque cas lent. De nouvelles bornes Xi restent une proposition à tester, non un résultat de cette capture.

Le census q3 par boîtes a supprimé les scans scalaires systématiques, pas le nombre de boules proposées : 20 362 738 seeds pour 381 123 sorties q3 à 32k/K5, et 53 124 709 pour 1 709 820 à K10. À K5/32k, **302 141 450** bornes du compte ont été préparées puis laissées non visitées après saturation ; elles figurent bien dans les 760 043 929 bornes payées, et ne sont pas additionnées une seconde fois.

## Local28 : forte réduction du scan, mais parcours répétés

Pour limiter la largeur, chaque cellule numérique contient les valeurs 8k / 16k / 32k, toujours en millions. Les ratios restent séparés.

| Travail Local28 | K5 : 8k / 16k / 32k | Ratios K5 | K10 : 8k / 16k / 32k | Ratios K10 |
|---|---:|---:|---:|---:|
| Seeds | 2,312 / 7,931 / 28,257 | 3,431 / 3,563 | 10,712 / 27,913 / 83,663 | 2,606 / 2,997 |
| Génération : visites | 17,160 / 47,256 / 136,638 | 2,754 / 2,891 | 60,466 / 147,261 / 390,002 | 2,435 / 2,648 |
| Domaine positif : visites | 13,725 / 34,615 / 88,904 | 2,522 / 2,568 | 43,422 / 101,767 / 248,776 | 2,344 / 2,445 |
| Partition : évaluations bloc+point | 80,895 / 247,266 / 866,771 | 3,057 / 3,505 | 410,133 / 1062,195 / 3082,095 | 2,590 / 2,902 |
| Partition : copies de frontière | 47,286 / 144,648 / 506,628 | 3,059 / 3,502 | 235,916 / 614,160 / 1782,590 | 2,603 / 2,902 |
| Atlas : tests disque+facettes | 4,990 / 12,346 / 31,395 | 2,474 / 2,543 | 22,670 / 52,369 / 122,804 | 2,310 / 2,345 |
| Parcours de carte par seed | 24,982 / 100,372 / 562,408 | **4,018 / 5,603** | 173,603 / 482,887 / 1741,815 | 2,782 / 3,607 |
| Scans de sites actifs | 12,960 / 27,234 / 57,555 | 2,101 / 2,113 | 64,118 / 138,488 / 291,593 | 2,160 / 2,106 |
| Localisations de racines | 15,173 / 31,574 / 65,824 | 2,081 / 2,085 | 71,007 / 152,690 / 319,089 | 2,150 / 2,090 |
| Tri racines+coquilles+groupes | 19,076 / 38,706 / 78,909 | 2,029 / 2,039 | 69,093 / 147,219 / 305,410 | 2,131 / 2,075 |
| Présentations de supports | 0,240 / 0,503 / 1,047 | 2,094 / 2,082 | 2,630 / 5,587 / 11,595 | 2,124 / 2,076 |

La préparation géométrique n'est pas gratuite : les visites de décomposition du cover valent 13,509/33,907/85,659 millions à K5 et 42,835/101,077/244,002 à K10. Les comparaisons de tri et orientations du domaine positif ajoutent 6,195/13,352/28,746 millions à K5 et 16,278/34,669/73,851 à K10 ; elles ne sont pas incluses dans les évaluations de partition du tableau.

Il faut aussi garder les sous-postes défavorables visibles. À K5, les seules bornes de **blocs** de partition font ×4,112 au second doublement, contre ×3,505 pour bloc+point ; les nœuds conservés sans examen font ×5,108/×5,849, jusqu'à 55 937 747. Ces derniers coûtent une visite et une conservation de descripteur, pas une évaluation géométrique. Les tests de droite dans la carte font ×5,492 au second doublement. `visit()` visite effectivement les quatre enfants d'une cellule traversée et compte aussi les états Deep/Outside immédiatement abandonnés : les 562 millions de visites ne sont pas une population fictive de sites.

Les budgets de construction de carte sont fixes dans cette capture et n'interrompent pas la recherche exacte. Ils n'autorisent pas à déduire une borne générale sur le nombre total de seeds, de cartes ou de visites.

## Window30 : moins de seeds, préparation et scans encore lourds

| Travail Window30 | K5 : 8k / 16k / 32k | Ratios K5 | K10 : 8k / 16k / 32k | Ratios K10 |
|---|---:|---:|---:|---:|
| Seeds | 0,830 / 2,007 / 4,750 | 2,418 / 2,367 | 5,163 / 12,395 / 29,877 | 2,401 / 2,410 |
| Préparation : formes de sites | 14,420 / 48,406 / 169,469 | 3,357 / 3,501 | 65,654 / 173,738 / 522,094 | 2,646 / 3,005 |
| Sélection : comparaisons/orientations | 310,863 / 1112,923 / 4121,191 | 3,580 / 3,703 | 2558,195 / 7015,460 / 22038,384 | 2,742 / 3,141 |
| Sélection : groupes visités par couche | 38,696 / 135,101 / 485,885 | 3,491 / 3,596 | 426,444 / 1172,553 / 3696,925 | 2,750 / 3,153 |
| Sélection : copies indices de contour | 76,535 / 268,317 / 967,587 | 3,506 / 3,606 | 848,665 / 2335,888 / 7373,560 | 2,752 / 3,157 |
| Sélection : déplacements de compaction | 35,432 / 127,696 / 469,117 | 3,604 / 3,674 | 403,226 / 1120,455 / 3579,655 | 2,779 / 3,195 |
| Premier scan de famille | 27,716 / 69,870 / 170,320 | 2,521 / 2,438 | 523,756 / 1322,901 / 3326,665 | 2,526 / 2,515 |
| Second scan de famille | 4,423 / 9,000 / 18,392 | 2,035 / 2,044 | 85,910 / 182,646 / 371,395 | 2,126 / 2,033 |
| Deux scans cumulés | 32,140 / 78,870 / 188,712 | 2,454 / 2,393 | 609,665 / 1505,548 / 3698,060 | 2,469 / 2,456 |
| Comparaisons tas+fenêtre+tri+groupes | 51,335 / 124,862 / 296,480 | 2,432 / 2,374 | 1591,726 / 3877,237 / 9439,393 | 2,436 / 2,435 |
| Présentations de supports | 0,476 / 0,963 / 1,954 | 2,022 / 2,028 | 6,077 / 12,466 / 25,061 | 2,051 / 2,010 |

La préparation du cover géométrique est également payée ; ses visites sont celles données pour Local28 ci-dessus. En revanche, Window30 n'a pas de construction du domaine positif ni d'atlas Local28 cachés.

Les principaux postes propres à Window30 restent sous ×4 sur ces entrées, mais cela n'est vrai ni de tous ses compteurs ni du front de filtrage partagé. Par exemple, les constantes intérieures/extérieures, les familles rejetées par constantes, les fenêtres ponctuelles et les groupes de coordonnées dupliquées ont des ratios supérieurs à quatre. Ce sont des sous-classes déjà incluses dans les scans ou regroupements, pas des scans supplémentaires à additionner. Les fenêtres ponctuelles passent de 22/138/833 à K5 et 12/129/1112 à K10 ; elles restent traitées exactement, sans suppression des contacts.

À 32k/K10, Window30 parcourt 3,698 milliards de sites dans ses deux scans, contre 291,593 millions de sites actifs pour Local28, mais ce rapprochement **omet volontairement les 3,082 milliards de tests de partition Local28** et sa préparation. Il serait donc faux d'en faire, seul, un facteur d'accélération. De même, comparer seulement les nombres de seeds favoriserait Window30 en masquant les couches duales.

## Capture séparée : s8, s10 et s12 à K5/Window30

Les six mesures s10/s12 proviennent d'une **autre capture**, [separation/lidar_2w21ewdx](separation/lidar_2w21ewdx/MANIFEST.json), [close `passed`](separation/lidar_2w21ewdx/COMPLETION.json) à 08:55:55 UTC sans erreur de fermeture et relue dans `FINAL_READBACK.json`. Ses hashes de sources, artefacts et entrées sont exactement ceux de la matrice principale. Les paramètres communs sont scan 0, K5, Window30, W4, `rectangle-pair`, census `boxes` et callback `digest`. Le s8 ci-dessous vient des records 0001/0005/0009 de la matrice principale ; il n'est pas une septième mesure de la capture séparée.

Provenance s10 : [8k/0000](separation/lidar_2w21ewdx/record_0000.json), [16k/0002](separation/lidar_2w21ewdx/record_0002.json), [32k/0004](separation/lidar_2w21ewdx/record_0004.json). Provenance s12 : [8k/0001](separation/lidar_2w21ewdx/record_0001.json), [16k/0003](separation/lidar_2w21ewdx/record_0003.json), [32k/0005](separation/lidar_2w21ewdx/record_0005.json). SHA-256 de ce manifeste : `17cf58732bd396d8e1821df899e324a15db3e8f731761a213f058389c80aa138`.

Les H et Xi ci-dessous additionnent toujours les recherches supplémentaires par **rectangle et par paire**, pas le front historique. Valeurs en millions ; ratios 8k→16k puis 16k→32k.

| s | Travail | 8k | 16k | 32k | Ratios |
|---:|---|---:|---:|---:|---:|
| 8 | Produits du front | 1,104 | 2,310 | 4,632 | 2,093 / 2,005 |
| 10 | Produits du front | 1,258 | 2,647 | 5,402 | 2,104 / 2,040 |
| 12 | Produits du front | 1,389 | 2,935 | 6,082 | 2,113 / 2,072 |
| 8 | Bornes H, rectangle+paire | 61,494 | 161,833 | 701,165 | 2,632 / **4,333** |
| 10 | Bornes H, rectangle+paire | 58,284 | 151,404 | 562,138 | 2,598 / 3,713 |
| 12 | Bornes H, rectangle+paire | 56,028 | 146,081 | 506,614 | 2,607 / 3,468 |
| 8 | Bornes Xi, rectangle+paire | 24,653 | 70,121 | 443,262 | 2,844 / **6,321** |
| 10 | Bornes Xi, rectangle+paire | 22,299 | 63,070 | 326,487 | 2,828 / **5,177** |
| 12 | Bornes Xi, rectangle+paire | 20,342 | 58,957 | 278,831 | 2,898 / **4,729** |

Les trois valeurs de s produisent exactement les mêmes compteurs de payload et les mêmes digests à chaque n. Une séparation plus forte augmente ici le nombre de produits du front, mais réduit les évaluations H/Xi des recherches ultérieures. Elle atténue le saut de Xi sans le ramener sous quatre sur 16k→32k, même à s12. Aucun gain temporel stable n'est établi : ce sont des observations séparées, sans répétitions isolées ni garantie d'exclusivité CPU. Cette comparaison ne couvre ni K10/s10/s12, ni Local28/s10/s12.

## Sorties, temps et limites de l'inférence

Les six couples Local28/Window30 ont exactement les mêmes compteurs de sortie, sommes et XOR de digests, ainsi que les mêmes fronts, filtres supplémentaires et comptes q3 par blocs. Ce différentiel fermé ne remplace pas l'oracle indépendant des petites instances et ne prouve pas l'absence théorique de collision d'un digest.

À 32k, les sorties q3/q4 sont respectivement 381123/45520 à K5 et 1709820/524458 à K10. Le nombre de callbacks croît environ comme n sur cette série, tandis que plusieurs postes de recherche croissent bien davantage. Cette difficulté n'est donc pas expliquée par les seules sorties actuellement émises. Ces flux ne sont pas encore le catalogue canonique et la tour complète des K hiérarchies.

Les temps mur, préparation partagée incluse, sont conservés ci-dessous uniquement comme observations de ces commandes. Des activités concurrentes et l'absence de répétitions isolées interdisent d'en tirer un classement robuste des backends ou un exposant de croissance temporelle.

| Configuration | 8k | 16k | 32k |
|---|---:|---:|---:|
| K5 Local28 | 3,849 s | 22,364 s | 45,000 s |
| K5 Window30 | 4,244 s | 32,776 s | 40,543 s |
| K10 Local28 | 18,373 s | 91,485 s | 153,418 s |
| K10 Window30 | 44,663 s | 130,914 s | 240,957 s |

La répétition K5/Local28 antérieure avait les **mêmes comptes discrets** mais des temps 4,960/18,139/60,661 s : cette dispersion est précisément la raison de privilégier ici le travail plutôt qu'un exposant de temps.

Ce que cette capture ne permet pas d'affirmer :

- Pas de sous-quadratique global : deux doublements d'un scan, à s et K fixés, ne bornent ni tous les nuages, ni les sommes de tailles des covers, ni les seeds ou les sorties.
- Pas non plus de preuve d'une complexité asymptotique superquadratique à partir d'un sous-poste ×5 ou ×10 : transitions de régime et constantes restent possibles. Le constat rigoureux est l'échec du critère empirique uniforme sur ces tailles.
- Pas d'accélération multi-CPU déduite de ces seuls W4. La matrice principale est à s8 ; la comparaison s8/s10/s12 utilise la capture séparée et reste limitée à K5/Window30. Aucune de ces deux captures ne couvre 50k, les dizaines de millions, le GPU ou FULL.
- Pas de coût « quasi nul du tri » établi : ses formes sont différentes dans les deux backends, et la sélection duale paie elle-même du tri et des orientations. À l'inverse, optimiser seulement ce tri ne supprimerait pas les recherches par paire, les seeds q3 et les parcours d'atlas.
- Pas de mémoire industrielle globale démontrée par les pics privés : les capacités de travail, les populations logiques, la somme des pics workers et le RSS ne sont pas interchangeables. Le mode digest n'alloue pas les futurs enregistrements complets.

Ordre de travail suggéré par ces mesures : réduire d'abord les recherches de témoins par paire qui visitent des zones hors citron ; diminuer les seeds q3 avant de reconstruire leur boule et leur census ; éviter les parcours répétés de carte Local28 ou la préparation excessive des couches Window30. Chacune de ces pistes doit conserver les voies q3/q4 indépendantes, les seuils stricts et les coquilles complètes, puis repayer et mesurer toute sa préparation. Aucune n'est qualifiée par la présente analyse.
