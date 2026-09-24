# Contre-audit indépendant : octets de la sortie FULL R20 et cible 100 ms

24 septembre 2026. Lecture seule du paquet R20, cas GPU `08/000000`, grille
1 mm/u18, sans sol, `s=8`, W48. Aucun GCP utilisé pour cet audit. Le paquet
R20 est construit au commit `48791e72` ; les définitions de sortie consultées
ci-dessous sont identiques entre ce commit et le présent checkout. La sortie
en mémoire est explicite ; le reçu publie ses **comptes** et condensés, pas une
sérialisation des tableaux. Le statut demeure `complete_relative`.

## Comptes mesurés et disposition réelle

Les [sondes R20 K5](../receipts/g4_tower_r20_20260924/vm/probe_0.stdout)
et [K10](../receipts/g4_tower_r20_20260924/vm/probe_2.stdout), champs
`orders`, donnent les sommes suivantes. Leurs temps sont également des
mesures, pas des estimations de bande passante.

| Kmax | Nœuds | Références parent | Cases successeur | Liens verticaux | Contributions datées | FULL / chaîne |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 1 541 750 | 1 541 745 | 1 541 750 | 1 541 750 | 897 776 | 421,527 / 1 108,630 ms |
| 10 | 7 426 215 | 7 426 205 | 7 426 215 | 7 426 215 | 4 414 230 | 1 954,913 / 3 873,050 ms |

Les cases successeur et liens verticaux valent exactement un par nœud :
[`FullCoverageCertificate::build_from`](../src/tower/forest/full_coverage_certificate.hpp)
ajoute ensemble `nodes_` et `successors_`, et
[`FullBallOrder::lower_nodes`](../src/tower/forest/full_ball_tower.hpp) est
vérifié de même taille à l'encodage, y compris à K1 où les liens sont absents
en valeur mais **présents en mémoire**. Les références parent sont des entrées
CSR ; `parents = nodes − Kmax` dans ces deux sorties, avec une racine finale
par ordre. Une continuation garde son nœud et n'ajoute aucune case.

Sur l'ABI x86_64 de la [VM R20](../receipts/g4_tower_r20_20260924/vm/lscpu.stdout),
les dispositions des [`FullNode` et `FullNodeId`](../src/tower/forest/full_certificate.hpp),
[`FullDatedContribution`, `FullCoveragePopulation` et leurs vecteurs](../src/tower/forest/full_coverage_certificate.hpp),
[`ExactLevel`](../src/tower/lanes/level.hpp) et
[`PointId`](../src/tower/core/types.hpp) donnent 64, 8, 80, 48, 48 et 4
octets respectivement. Les six assertions reproductibles se trouvent dans
[`layout_check_full_r20_20260924.cpp`](layout_check_full_r20_20260924.cpp) :
depuis la racine du dépôt, lancer
`g++ -std=c++20 -fsyntax-only morsehgp3D_v9/audits/layout_check_full_r20_20260924.cpp`.
Cette vérification passe ici sans produire d'exécutable ni lancer le moteur.
La machine locale et la VM R20 sont toutes deux x86_64, mais
`g++ 13.3` local et
[`g++ 11.4` sur la VM](../receipts/g4_tower_r20_20260924/vm/compiler.stdout)
ne sont pas le même compilateur exact : les nombres de ce tableau sont les
tailles de cette famille d'ABI, **pas une mesure `sizeof` exécutée dans la
VM R20** ni une preuve concernant l'ABI du GPU. Les tailles `sizeof`
incluent l'alignement/padding.

| Tableau publié, taille `size × sizeof` | K5 | K10 |
| --- | ---: | ---: |
| Nœuds, 64 o | 98 672 000 o | 475 277 760 o |
| Parents, 8 o | 12 333 960 o | 59 409 640 o |
| Successeurs, 8 o | 12 334 000 o | 59 409 720 o |
| Liens verticaux, 8 o | 12 334 000 o | 59 409 720 o |
| Contributions, 80 o | 71 822 080 o | 353 138 400 o |
| **Cinq tableaux, somme exacte des tailles utiles** | **207 496 040 o (197,9 Mio)** | **1 006 645 240 o (960,0 Mio)** |

La banque des populations est **partagée entre les K**, pas dupliquée dans
chaque forêt. Elle contient un `domain` de 39 885 `PointId`, une ligne de
48 o par population et les tableaux d'IDs intérieur/coquille. La fonction
[`assign_populations`](../src/tower/forest/full_ball_tower.hpp) crée 39 885
lignes singleton, puis une ligne par **ball distinct effectivement cité**
par une contribution non K1. R20 publie les incidences de contributions,
mais ni le nombre de ball distincts cités ni la somme des IDs de ces lignes.
Un ball peut servir plusieurs K : assimiler 897 776 contributions à 897 776
lignes serait une double comptabilisation possible.

On peut néanmoins borner le volume **logique** des lignes. La boucle des
[`programs[k]`](../src/tower/forest/full_ball_tower.hpp) ne présente chaque
ball qu'une fois par ordre, entre `n_interior+arity−1` et
`n_interior+n_shell`. Un ball régulier (`n_shell=arity`) a au plus deux
contributions d'ordres distincts. Les [comptes de catalogue R20 K5](../receipts/g4_tower_r20_20260924/vm/probe_0.stdout)
et [K10](../receipts/g4_tower_r20_20260924/vm/probe_2.stdout) donnent
227/444 balls à coquille supplémentaire, `max_shell=5`, et
`max_interior=4/9`. Même en donnant quatre contributions K2..5 à chaque
ball extra K5, ou cinq K2..10 à chaque extra K10, il faut au moins
`ceil((857891−2×227)/2)=428719` / `ceil((4374345−3×444)/2)=2186507`
balls distincts en plus des singletons. Chaque ligne non singleton porte
au moins deux IDs ; les maxima individuels sont 9/14 IDs dans ces reçus.

| Banque partagée, calcul dérivé des comptes R20 | K5 | K10 |
| --- | ---: | ---: |
| Lignes de population possibles | 468 604 à 897 776 | 2 226 392 à 4 414 230 |
| IDs intérieur/coquille possibles | 897 323 à 7 760 904 | 4 412 899 à 61 280 715 |
| `48×lignes + 4×IDs + 4×domain` | 26 241 824 à 74 296 404 o | 124 677 952 à 457 165 440 o |
| Cinq tableaux + banque, volume logique | **233 737 864 à 281 792 444 o** | **1 131 323 192 à 1 463 810 680 o** |

Ces intervalles bornent les **tailles d'éléments** avec les informations
publiées ; ils ne bornent pas l'allocation physique supérieure. Les capacités
des deux `std::vector` de chaque ligne, métadonnées d'allocateur, copies
temporaires, index et catalogue ne sont pas comprises. Le `peak_rss_kb` du
reçu (1 179 876 / 4 808 740 KiB) concerne tout le processus, donc ne mesure
pas la sortie FULL seule. Les quatre tableaux du certificat sont réservés
à leur taille structurelle exacte avant l'encodage ; `lower_nodes` l'est
pendant la phase des images. `reserve` n'est pas une promesse standard
d'absence de capacité supplémentaire.

## Lecture de la contrainte 100 ms

Écrire le seul volume logique K5 en 100 ms demanderait un débit **effectif**
de 2,34 à 2,82 Go/s ; K10 demanderait 11,31 à 14,64 Go/s. Ce sont des
quotients octets/temps, ni des mesures de trafic DRAM, ni une borne inférieure
de temps : des données peuvent traverser les caches et les tailles incluent
du padding. R20 ne fournit pas de microbenchmark de bande passante isolant
l'expansion finale. La décision utilisateur d'inclure toutes ces cases dans
les 100 ms reste impérative.

Le [code d'encodage](../src/tower/forest/full_ball_tower.hpp) construit les
nœuds, parents, successeurs et contributions des K en parallèle ; le cas K5
mesure **32,233 ms** pour cette phase (195 162 040 o de taille utile de ces
quatre tableaux). La banque prend 17,216 ms, les populations 19,571 ms et
les images verticales 26,862 ms. Leur somme, **95,882 ms**, inclut calcul,
validation, tris et allocations ; elle n'isole pas les écritures. Elle ne
constitue donc pas un plancher de 95,882 ms pour un autre algorithme.

Le verrou démontré pour **ce chemin R20** est d'abord le travail : à K5,
la phase statique (198,500 ms) et les lots (56,518 ms) font déjà 255,018 ms ;
le reste de la chaîne prend `1 108,630−421,527=687,103 ms`, même si FULL
était gratuit. Le reçu compte aussi 3 621 785 représentants, 1 289 447
appels MEB et 31 708 173 visites d'index pour FULL K5. On ne peut donc
attribuer son écart à 100 ms à une barrière d'écriture de la sortie. La
sortie complète ajoute un coût réel que toute refonte doit mesurer, mais
R20 ne prouve aucun plancher de bande passante au-dessus de 100 ms ; sa
mesure pointe surtout vers la réduction du travail géométrique, de la
résolution des cibles et du travail amont de la chaîne.

Pour publier un nombre exact d'octets de la banque lors d'une prochaine
capture, il faut relever après `assign_populations` : `rows.size()`, somme
des `interior.size()+shell.size()`, et séparément leurs `capacity()` ;
la même capture peut isoler le temps de l'expansion explicite et ses octets
effectivement écrits. Aucun de ces nombres n'est présent dans R20.
