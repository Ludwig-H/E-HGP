# Segments S2 sur la trame brute, les quatre quarts et trois densités

23 septembre 2026. Le [premier reçu sur la trame entière dense](../s2_segment_mass_20260923/README.md)
montrait que les segments S2 d'au moins 16 arêtes portent une forte part du
travail du cœur q3/q4. Ce panel contrôle si ce signal tient après les
coupes `x=0`, puis `y=0`, passant par le capteur, et après sélection
emboîtée de 1/2 et 1/4 des retours **avant** la grille commune 1 mm.
Il couvre **15 cas** : plein et quatre quarts aux trois densités, sur
l'unique trame brute SemanticKITTI 08/000000, K5/s8, q3+q4 CPU. Aucun
alignement des points ou des passages n'est supposé.

Le [sidecar C++](measure.cpp) rejoue le front WSPD `MidpointSamples` et le
filtre de rectangle `Affine`, puis joint par **IDs bruts** toutes les arêtes
du [reçu S2 épinglé](../edge_matched_core_20260923/README.md) à un seul
rectangle. Il ne rejoue ni le filtre de paire, ni le cœur, ni FULL. Les
masques tracés restent inclus dans les masques du rectangle. Le
[runner LIVE](run_panel.py) vérifie les SHA des entrées, des huit parties
de trace par cas et du stdout batch, puis recoupe pour **chacun** des 15 cas
cinq comptes front/filtre : rectangles d'entrée, masse d'entrée,
rectangles ouverts, masse ouverte, visites de nœuds. Il recoupe aussi
les charges, la somme des tailles de cœur et les deux comptes de masques
q3/q4. Le [reçu compact](PANEL.json) garde ces comptes et leurs
histogrammes ; le [lecteur statique](verify.py) les recontrôle en Python
normal et `-O`. Le plein dense concorde avec le premier sidecar indépendant.

Dans le tableau, `F` compte les formes réellement calculées par `load`,
**deux extrémités incluses par arête**. `R16` est le nombre de rectangles
avec au moins 16 arêtes S2 survivantes ; `F16/F` est la part des formes
qu'ils portent. `B=27S16/F16` est le ratio de termes paire-sommet pour
une première grille de huit sous-cellules (27 sommets partagés) sur ces
arêtes ; `B₂=27(S16,q3+S16,q4)/F16` sépare les deux voies selon leurs
masques **avant cœur**. Ces budgets ne comptent ni recherche de gardes,
ni couverture, ni repli exact ; ils ne sont pas des gains.

| Densité | Région | Sites | F | R16 | F16/F | B | B₂ | Sidecar s |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1/4 | Plein | 30 847 | 37 009 904 | 873 | 67,64 % | 4,00 % | 6,42 % | 8,1 |
| 1/4 | x− y− | 7 649 | 4 310 796 | 305 | 29,17 % | 18,73 % | 31,87 % | 1,9 |
| 1/4 | x− y≥0 | 7 788 | 4 325 933 | 136 | 19,14 % | 12,07 % | 18,63 % | 3,4 |
| 1/4 | x≥0 y− | 7 692 | 3 548 514 | 86 | 51,86 % | 4,78 % | 7,23 % | 1,1 |
| 1/4 | x≥0 y≥0 | 7 718 | 2 229 643 | 101 | 7,20 % | 39,41 % | 68,36 % | 1,3 |
| 1/2 | Plein | 61 694 | 128 852 821 | 2 861 | 76,52 % | 2,96 % | 4,86 % | 17,5 |
| 1/2 | x− y− | 15 217 | 13 106 100 | 802 | 38,27 % | 12,96 % | 22,01 % | 4,3 |
| 1/2 | x− y≥0 | 15 427 | 13 260 212 | 621 | 31,19 % | 10,50 % | 17,08 % | 5,9 |
| 1/2 | x≥0 y− | 15 619 | 14 655 072 | 425 | 69,19 % | 3,85 % | 6,16 % | 2,5 |
| 1/2 | x≥0 y≥0 | 15 431 | 7 024 220 | 557 | 17,66 % | 28,02 % | 48,45 % | 2,9 |
| 1 | Plein | 123 389 | 559 661 741 | 11 174 | 85,52 % | 2,24 % | 3,69 % | 42,3 |
| 1 | x− y− | 30 265 | 46 146 150 | 3 051 | 53,04 % | 10,37 % | 17,63 % | 10,3 |
| 1 | x− y≥0 | 30 780 | 47 832 906 | 2 619 | 50,15 % | 8,19 % | 13,54 % | 13,7 |
| 1 | x≥0 y− | 31 391 | 52 302 311 | 1 562 | 74,40 % | 3,35 % | 5,49 % | 5,9 |
| 1 | x≥0 y≥0 | 30 953 | 26 739 169 | 2 684 | 35,50 % | 19,73 % | 33,34 % | 6,5 |

Le plein passe de **67,64 % → 76,52 % → 85,52 %** des formes dans les
segments ≥16 quand sa densité double deux fois. Sa masse `F16` croît
25 034 263 → 98 597 450 → 478 635 662, avec exposants finis
`log₂(F16₂/F16₁)` de **1,978 puis 2,279**. Ces exposants dépassent ceux
de F total (1,800 puis 2,119) sur cette trame : les grands segments
concentrent la croissance observée. Ils ne constituent pas une borne
asymptotique. Les entrées sont emboîtées par IDs, mais rectangles et
segments ≥16 sont **reconstruits** à chaque densité : `F16` compare des
charges sélectionnées différentes, pas une cohorte emboîtée. La somme
des quatre quarts `F16` vaut 4 086 368 →
20 532 036 → 96 871 607, différente du plein calculé séparément ;
les arêtes traversantes y changent la tâche.

La variation spatiale empêche de figer **16** comme seuil produit. Le
quart `x≥0,y≥0` passe seulement de 7,20 % à 17,66 %, puis 35,50 % de F ;
son budget `B` vaut 39,41 %, 28,02 %, puis 19,73 %, avant même les
gardes. Au plein dense, les 11 174 segments ≥16 portent 396 481 arêtes,
avec 271 207 masques q3 et 382 641 masques q4 : séparer les réductions
porte le budget de 2,24 % à 3,69 % de F16. Le sélecteur ≥16 est donc un
**diagnostic pour l'ablation**, pas une règle universelle. Un ordonnanceur
peut conserver le chemin direct S3 lorsque le coût borné de la tentative
de certificat paraît défavorable ; toute tentative doit garder les
masques, les seuils distincts et un repli exact, et mesurer les sorties
et le coût de toute la chaîne.

La colonne `Sidecar s` est le mur de chaque exécution auxiliaire
(front + filtre de rectangle + jointure + tri), **hors** vérification Python
des SHA et des traces, et sans cœur ni FULL. Ces temps d'un hôte partagé
ne mesurent ni le bénéfice du certificat proposé, ni GCP G4. Les traces
binaires restent en `/tmp` et ne survivront pas à l'arrêt du codespace ;
leurs SHA et les agrégats sont gardés dans `PANEL.json` et les reçus
parents. Le commit `c265a5d` est celui **de la trace** ; le sidecar est
lié à l'archive distincte `libmhgp9_gen.a` de SHA `208aabb…`. Les
15 jointures, comptes et masques concordent sur ces cas ; ce n'est pas
une identité binaire générale entre constructions.

```sh
python3 -B morsehgp3D_v9/audits/s2_segment_panel_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/s2_segment_panel_20260923/verify.py
(cd morsehgp3D_v9/audits/s2_segment_panel_20260923 && sha256sum -c SHA256SUMS)
```

Pour refaire le panel tant que les octets éphémères existent, compiler
`measure.cpp` en C++20 `-O2` avec l'archive épinglée, puis exécuter depuis
la racine du dépôt :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/s2_segment_panel_20260923/measure.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-s2-segment-panel-20260923
python3 -B morsehgp3D_v9/audits/s2_segment_panel_20260923/run_panel.py \
  --inputs /tmp/mhgp9-s2-scaling-20260923-inputs \
  --traces /tmp/mhgp9-edge-core-audit-20260923 \
  --archive build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a \
  --binary /tmp/mhgp9-s2-segment-panel-20260923 \
  --out /tmp/mhgp9-s2-segment-panel-replay.json
```

Le runner refuse d'écraser un reçu existant.
Les moitiés spatiales sont dans le
[panel S2 plein/demi](../s2_half_density_k5_20260923/README.md) ;
le présent appariement de segments concerne les quarts. Il faut encore
tester plusieurs scènes, K10, le régime sans sol et la réalisation
CPU/GPU complète. Ni les exposants finis ni ce ratio arithmétique ne
prouvent une croissance sous-quadratique globale.
