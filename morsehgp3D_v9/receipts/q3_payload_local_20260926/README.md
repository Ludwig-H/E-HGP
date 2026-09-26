# Port q3 payload — capture locale FULL du 26 septembre 2026

Capture close : **10 paires ON/OFF, 20 tours FULL K=1..5 relatives**, 29 commandes
d'export/sonde, aucun refus ni écart d'objet. Les trois condensés, tous les
ordres, les compteurs du générateur, le registre géométrique et le travail de
la tour sont identiques dans chaque paire. Au total, les passages ON importent
5 890 091 clés q3 et 11 642 678 IDs intérieurs. La référence OFF refait le census.

Les 115 empreintes des sources, binaires et recettes de compilation sont
identiques avant/après. Le lecteur normal, puis le même lecteur sous Python
`-O`, passent ; les mutations de condensé, cardinalité du payload et travail
géométrique sont refusées. Il s'agit de contrôles du lecteur, pas de trois
nouveaux mutants compilés du moteur. Les portes natives indépendantes du port
appartiennent à la qualification parente, pas à cette capture.

## Périmètre exact

- HEAD au départ : `9751bae69115b878f288633ae1216ff1ac13da73` ; les empreintes
  du manifeste, et non ce seul commit, désignent les fichiers exécutés.
- CPU local partagé, K5, s8, quatre workers et quatre fils statiques. Ce n'est
  pas une mesure sur quatre cœurs exclusifs. Une paire par cas, ordre alterné.
- Filtre/certificats/voies q3-q4 par lots **hôtes** ; aucun levier GPU et aucune
  session CUDA. Catalogue scellé, census q2 anticipé, regroupement haché, pool
  persistant, queue de tour et bassin de records activés.
- **L15 fusionné est activé ici (`q34_lanes_fused=1`)**, alors que la campagne
  G4 préparée en parallèle conserve `q34_lanes_fused=0`. Cette capture exerce
  notamment le transport fusionné. Ne pas comparer ses gains aux gains G4.
- Le bassin dit pinned est ici le jumeau hôte : ces résultats ne mesurent
  aucun transfert CUDA épinglé.
- Les neuf nuages synthétiques utilisent les recettes entières u16 gravées,
  graine 3, exportées explicitement en `u32le`. Leur étiquette est
  `synthetic_u16_recipe`, **pas 1 mm**. La densité augmente dans un domaine
  borné fixe ; ce ne sont pas des coupes spatiales LiDAR.
- Le bonus est la trame **08/000000 entière après retrait du sol**, 39 885
  sites, grille 1 mm et masque historiquement épinglés. Ce n'est ni toute la
  trame brute, ni plusieurs séquences, ni une nouvelle mesure de segmentation.

Le chrono `chain_total` comprend le calcul et la matérialisation de la tour
explicite. Lecture, export des synthétiques, calcul des condensés, segmentation
et préparation hors ligne n'y entrent pas. Le mur externe de chaque sonde est
conservé séparément et vérifié. Aucun contrat G4 de 1 s ou 100 ms n'est acquis
par cette capture locale.

## Temps observés

Secondes de chaîne ; une seule observation par case, pas un gain stable.

| Régime | n | OFF | ON |
|---|---:|---:|---:|
| Uniforme | 8 000 | 4,306 | 4,172 |
| Uniforme | 16 000 | 9,577 | 9,327 |
| Uniforme | 32 000 | 20,786 | 22,399 |
| Terrain | 8 000 | 0,909 | 0,896 |
| Terrain | 16 000 | 2,129 | 1,912 |
| Terrain | 32 000 | 3,674 | 3,562 |
| Huit amas | 8 000 | 21,878 | 21,141 |
| Huit amas | 16 000 | 83,393 | 79,206 |
| Huit amas | 32 000 | 306,684 | 299,911 |
| LiDAR sans sol 08/000000 | 39 885 | 27,010 | 26,528 |

La régression uniforme 32k est conservée ; aucun essai favorable n'a remplacé
une observation. Le bénéfice discret certain est la suppression de visites
du census : environ 52,5–56,5 % sur les synthétiques. Sur la trame sans sol,
les visites passent de 100 689 614 à 45 695 806 ; les tests de feuilles de
6 097 121 à 2 671 618. Son census mesuré passe de 746,509 à 447,227 ms, avec
691 282 clés q3 importées et une clé q3 de repli. Les voies hôtes prennent
5 991,435 / 6 081,223 ms OFF/ON. Aucun de ces temps n'est un temps GPU.

Les trois condensés de cette trame sont : tour `67450c64611075b1`, catalogue
`5ad1fe09354411ba`, présentations `a2aa4b20ca392dfe`. Tour et catalogue sont
également comparés aux épingles CPU historiques du protocole v30.

## Croissance : résultats utiles, limites réelles

Les rapports de mur ON aux doublements sont 2,235 puis 2,402 (uniforme),
2,133 puis 1,863 (terrain), 3,746 puis 3,786 (amas). Le lecteur publie aussi
tous les rapports des compteurs non nuls, et signale ceux atteignant quatre.
Cela **ne qualifie pas globalement les algorithmes comme sous-quadratiques**.

Sur uniforme, aucun des compteurs du générateur, du registre géométrique et
des nœuds/parents/contributions contrôlés n'atteint le quadruplement. Sur
terrain, les sorties q4 valent 1 141 / 4 646 / 19 429 : rapports 4,07 puis 4,18,
malgré des gros travaux et un mur moins croissants. Une transition de densité
dans cette recette quasi plane est une explication possible, pas une preuve.

Sur les amas, le verrou est particulièrement clair :

| Compteur | 8k | 16k | 32k |
|---|---:|---:|---:|
| Paires q34 expansées | 28 352 433 | 112 770 168 | 449 652 733 |
| Visites de témoins par paire | 402 038 698 | 1 513 384 670 | 5 766 286 474 |
| Chunks du passage q4 des voies | 11 407 810 | 38 898 584 | 123 020 421 |
| Comparaisons q4 des voies | 678 196 | 894 069 | 1 457 005 |

Les expansions font ×3,977 puis ×3,987 : les appeler sous-quadratiques parce
que ces rapports sont légèrement inférieurs à quatre serait trompeur.
À 32k OFF, le filtre q34 prend 221,214 s sur 306,684 s de chaîne. Le tri ou
les comparaisons q4 ne constituent pas ce verrou principal.

Diagnostic supplémentaire, calculé directement sur les fichiers : séparer
les huit amas par les trois tests de coordonnées `x > 35000` donne des tailles
942/973/977/1008/1009/1016/1019/1056 à 8k,
1948/1979/1987/1998/2005/2016/2029/2038 à 16k et
3947/3984/3990/3992/4002/4012/4025/4048 à 32k. La masse exacte des paires
inter-amas, `(n² − somme des tailles²)/2`, vaut respectivement
27 995 740 / 111 997 058 / 447 996 847. Les expansions observées se trouvent
juste au-dessus, autour de 0,44 n². Cela suggère l'expansion presque complète
des produits entre amas ; les agrégats seuls ne prouvent pas l'identité de
chaque paire. Cette observation motive des certificats locaux par facteur
`h_a/h_b` avant expansion, à démontrer et mesurer séparément.

Les replis sont également à surveiller : entre 16k et 32k amas, les tests
q3 de feuilles passent de 3 825 938 à 18 122 937 (×4,74), les événements du
balayage q4 de 14 490 938 à 66 951 999 (×4,62), et ses sites actifs de
47 043 031 à 325 808 997 (×6,93). Ils étaient nuls à 8k. Une bonne croissance
des seules voies par lots ne qualifie donc pas la croissance de leurs replis.
Le payload q3 ne modifie aucun de ces travaux géométriques.

## Relecture et conservation

`manifest.json` conserve commandes, codes, murs, noms des sorties brutes et
empreintes avant/après ; `summary.json` est recalculé depuis ces sorties.
Chaque commande d'export/sonde a ses fichiers `.stdout` et `.stderr`, même
si ce dernier est vide. Les données générées restent hors dépôt dans
`/workspaces/E-HGP/build/v9-q3-payload-scaling-inputs-20260926/` ; les deux
binaires se trouvent dans `build/v9-q3-payload-integration-20260926/`.

Lecteur **LIVE**, dépendant des données, binaires et sources épinglés dans le
worktree `/tmp/mhgp9-audit-resume-20260926` ; pas une archive autonome :

```sh
python3 -B morsehgp3D_v9/bench/run_payload_scaling.py --readback morsehgp3D_v9/receipts/q3_payload_local_20260926
python3 -O -B morsehgp3D_v9/bench/run_payload_scaling.py --readback morsehgp3D_v9/receipts/q3_payload_local_20260926
```

Le lecteur normal est aussi exécuté automatiquement après fermeture de la
capture ; la seconde commande a été exécutée séparément, avec code 0. Tous
deux annoncent `PASS`, 10 paires et 20 tours. Toute dérive des sources/binaires
ou tout résultat partiel fait refuser la relecture ; les échecs ne sont pas
effacés. Le diagnostic textuel inter-amas ci-dessus est un complément calculé
sur les entrées, distinct des invariants automatiques du lecteur.
