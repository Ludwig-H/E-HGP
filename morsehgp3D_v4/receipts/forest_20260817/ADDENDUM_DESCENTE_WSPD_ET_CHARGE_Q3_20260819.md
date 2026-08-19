# Addendum — où va vraiment `t_gen` : la descente WSPD était séquentielle et pesait 72 %

Date : 19 août 2026 UTC. Traite l'item n° 1 de la feuille de route
corrigée (audit `dd0d4a6` § 3) : « scan q3 et construction des covers —
décider, **sur les seules ancres lourdes**, entre scan plat
parallèle/GPU et index exact par couches convexes », avec l'exigence
que l'assiette soit **justifiée par des compteurs de charge, pas
supposée ».

La décision est prise, et elle n'est aucune des deux options : **les
compteurs montrent que le scan q3 n'est pas le poste dominant, et que
le poste qui l'est n'était mesuré par personne.**

## 1. Le profil ne rendait pas compte du temps

Premier constat, avant toute optimisation : à n=8000 (uniform, s=8,
smax=11, 4 fils), la somme des postes profilés vaut **61,6 s de CPU**
réparti sur quatre fils — soit ~15 s de mur — pour un `t_gen` de
**72,8 s**. Il manquait donc l'essentiel, et tout classement de « poste
dominant » tiré de ce profil était sans fondement. Le reçu Jung du
18 août désignait ainsi le scan de profondeur q3 ; c'était faux.

Le poste manquant est `wspd_alive` : la descente par vagues qui produit
les rectangles vivants de chaque lane, en appelant
`count_universal_witnesses_234` sur chaque rectangle du front. Elle
n'était dans aucun chrono — et elle était **entièrement séquentielle**.

Chronométrée (n=8000, avant tout changement) :

| poste | temps | nature |
|---|---|---|
| descente WSPD q2 | 5 350,4 ms | **séquentiel** |
| descente WSPD q3 | 22 045,6 ms | **séquentiel** |
| descente WSPD q4 | 25 083,6 ms | **séquentiel** |
| **descente, total** | **52 479,6 ms** | **72 % de `t_gen`** |
| cover par ancre | 12 641,8 ms | CPU, 4 fils |
| complétion q4 | 22 454,9 ms | CPU, 4 fils |
| cœur de seed (Jung) | 9 825,1 ms | CPU, 4 fils |
| **scan de profondeur q3** | **7 590,0 ms** | CPU, 4 fils — **~2,6 % du mur** |
| profondeur q4 | 2 386,2 ms | CPU, 4 fils |
| cover par rectangle | 2 784,1 ms | CPU, 4 fils |
| histogrammes h_a/h_b | 294,5 ms | CPU, 4 fils |

## 2. Les couches convexes q3 sont réfutées par la mesure

Les compteurs de charge par ancre demandés par l'audit sont désormais
publiés (`q3_charge`, `q3_charge_par_seeds`, `q3_charge_par_travail`) :
par ancre, le travail **réellement scanné** (sortie anticipée
comprise), le nombre de seeds et la taille du cover — en histogrammes
log2, quelques additions par ancre et jamais par site.

n=8000, uniform :

```text
ancres_scannees=1 036 238   seeds=35 431 544   travail=364 151 470
cover_moyen=120,2   seeds_moyen=34,19   travail_moyen=351,4
```

Le chiffre qui tranche : **travail / seeds = 10,3 sites évalués par
seed**, pour un cover de 120. La sortie anticipée à `h_3 = 9`
intérieurs met fin au scan après une dizaine d'évaluations — le seed
meurt, ou survit (4,0 % : 1 425 847 candidats q3 sur 35,4 M de seeds)
et paie alors le cover entier.

Le rapport est stable en famille et en taille (n=800) :

| famille | cover moyen | seeds/ancre | travail/ancre | **sites/seed** |
|---|---|---|---|---|
| uniform | 97,5 | 27,7 | 292,3 | **10,5** |
| eight_clusters | 274,3 | 88,1 | 856,5 | **9,7** |
| terrain | 41,2 | 7,8 | 88,2 | **11,4** |
| uniform n=8000 | 120,2 | 34,2 | 351,4 | **10,3** |

**Conséquence.** Un index par ancre — couches convexes ou autre — se
construit une fois sur le cover et s'amortit sur les seeds. Sa
construction coûte au moins `O(|cover| log |cover|)`, soit ~830
opérations à n=8000, contre **351 sites scannés au total pour l'ancre
entière**. L'index coûte donc ~2,4 fois ce qu'il remplace, avant même
d'avoir répondu à une seule requête. Il ne peut pas payer.

**Et il n'y a pas d'assiette.** La part cumulée du travail portée par
les ancres à beaucoup de seeds (n=8000) :

```text
s>=8 : 97,0 %    s>=16 : 86,5 %    s>=32 : 66,1 %
s>=64 : 30,2 %   s>=128 : 4,0 % (10 541 ancres)
```

Le travail est réparti sur des centaines de milliers d'ancres
moyennes ; les 10 541 ancres « lourdes » (≥ 128 seeds) n'en portent que
4 %. L'assiette que l'audit demandait de justifier **n'existe pas**.

Le scan plat gagne donc par défaut — et il est déjà ce que le code
fait. Sa forme est en outre exactement celle qu'un port GPU veut : ~10
itérations régulières par seed, warp-par-seed, sans structure de
données à construire.

## 3. Ce qui a été fait à la place : paralléliser la descente

Le traitement d'un rectangle du front est **pur** — lecture seule de
l'index, pile locale dans `count_universal_witnesses_234`. La vague se
découpe donc sans aucune synchronisation.

Découpage en **tranches ordonnées** : chaque tranche écrit ses propres
tampons (rectangles vivants et front suivant), et la concaténation se
fait **en ordre de tranche**, jamais en ordre d'achèvement. La sortie
est ainsi **bit-identique** au chemin séquentiel quel que soit le
nombre de fils ; le tirage dynamique ne porte que sur l'attribution des
tranches. Les premières vagues (< 256 rectangles) restent
séquentielles : les paralléliser coûterait plus que leur travail.

### Mesure, n=8000, uniform, s=8, smax=11, 4 fils

| | avant | après |
|---|---|---|
| descente q2 | 5 350,4 ms | 1 581,8 ms |
| descente q3 | 22 045,6 ms | 6 329,8 ms |
| descente q4 | 25 083,6 ms | 7 481,0 ms |
| **descente totale** | **52 479,6 ms** | **15 392,6 ms** (**×3,41**) |
| **`t_gen` (mur)** | **72 827,5 ms** | **35 295,0 ms** (**×2,06**) |

Sorties **identiques** : `boules_uniques=3 134 427`,
`evenements=3 126 158`, les dix cardinalités par K inchangées, et les
rectangles visités identiques au rectangle près
(1 255 851 / 4 522 891 / 5 050 767 avant comme après).

Contrôle apparié à n=800, même binaire, un fil contre quatre :
2 402,4 ms → 685,1 ms, soit **×3,51** — 88 % d'efficacité sur quatre
cœurs.

## 4. Portes

`ctest --test-dir build/v4` : **135 tests, tous verts**.

- `--par-gate` (l'objet post-RLE est indépendant du découpage) reste
  verte : la descente parallèle ne change aucun candidat.
- `--workers-gate` exige désormais **quatre ouvriers mesurés pour
  chacune des trois descentes**, publiés au point de création des
  `std::thread` (`wspd_q2/q3/q4`), en plus des étages existants.
- Mutant `wspd-one-worker` (nouveau, code 4) : la descente sérialise,
  CLI et digests inchangés — seule la mesure au point de création le
  trahit, les lanes restant à quatre. Les trois mutants d'ouvriers
  préexistants restent tués.

## 5. Ce que cela change pour la feuille de route

- **Item 1 est tranché** : ni index par couches convexes (réfuté par
  les compteurs), ni chantier sur le scan q3 (2,6 % du mur). La forme
  actuelle est la bonne, et c'est celle que le GPU veut.
- Le nouveau classement des postes de `t_gen`, après parallélisation
  (mur ~35,3 s, CPU 4 fils) : complétion q4 (22,5 s CPU), descente
  WSPD (15,4 s mur), cover par ancre (12,6 s CPU), cœur de seed
  (9,8 s CPU), scan q3 (8,0 s CPU).
- La **complétion q4** devient le premier poste CPU. C'est là qu'il
  faut regarder ensuite — après avoir mesuré ce qu'elle fait
  réellement, et non en supposant.

## 6. Le poste suivant, mesuré et non supposé : l'entonnoir de la complétion q4

Le premier poste CPU après la parallélisation est la complétion q4
(22,6 s). Avant d'y toucher, un entonnoir : où meurent les paires
(seed, y) énumérées sur la lentille de l'ancre ? Les compteurs sont
locaux à la boucle et versés une fois par seed — aucun coût par paire
au-delà d'un incrément de registre.

n=8000, uniform : **173 001 161 paires**.

| filtre | paires tuées | part | coût du prédicat |
|---|---|---|---|
| `y` confondu avec seed ou ancre | 15 515 943 | 9,0 % | comparaison |
| une des trois longueurs > D | 37 740 455 | 21,8 % | trois i64 |
| owner (ab arête maximale) | 272 904 | 0,2 % | six comparaisons |
| exact-once du seed | 31 972 100 | 18,5 % | une i64 |
| `det = 0` (coplanaires) | 29 688 | 0,02 % | déterminant i128 |
| **centre non strictement intérieur** | **70 989 328** | **41,0 %** | **`q4_form` + 4 signes i128** |
| atteignent le test de profondeur | 16 480 743 | 9,5 % | — |

L'entonnoir est **inversé** : le prédicat le plus cher est celui qui
tue le plus. Le même rapport se retrouve à n=800 (42,0 %), donc ce
n'est pas un effet de taille.

**Ce qui a été fait, et ce qui n'est pas mesurable.** Le test d'arité 4
stricte recalculait **quatre fois** le même déterminant : l'orientation
du sommet opposé à la face `s` vaut `(-V, +V, -V, +V)` pour
`V = det3(b-a, x-a, y-a)` — preuve complète en tête de
`q4_instruction.hpp` (permutation cyclique pour `s=1`, échange de
lignes pour `s=2`, multilinéarité pour `s=0`). Un seul déterminant
suffit, et il tient en **i64** (différences de u16 < 2^17, `det3` <
6·2^51) ; seules les quatre orientations du CENTRE dépendent vraiment
de la face et restent en i128. On passe donc de huit déterminants i128
à quatre i128 plus un i64.

**Aucun gain temporel n'est revendiqué** : `t_q4_completion` mesure
22 638,5 ms après contre 22 454,9 ms avant, soit un écart largement
sous le bruit du conteneur (±10 à 15 % entre processus). La sortie
anticipée sur la première face fautive fait qu'un rejet ne payait déjà
pas les huit déterminants. Le gain est **structurel** — une identité
prouvée à la place d'un calcul répété — et il faudrait un banc apparié
intra-processus, comme celui de l'internement, pour trancher la
constante. Il n'a pas été construit ici.

Porte : mutant `q4-center-parity` (parité inversée) tué par l'oracle
rationnel indépendant `mhgp4_q4_oracle`, code 4. C'est l'oracle, pas
une re-vérification exhaustive, qui reçoit l'identité.

**Ce que l'entonnoir désigne pour la suite** : 41 % des paires paient
`q4_form` (un déterminant i128) et les signes du centre pour rien. La
question ouverte est s'il existe un prédicat **nécessaire** et bon
marché — en longueurs carrées i64 — qui écarte une partie de ces
paires avant la forme de Cramer. Le manuscrit interdit le raccourci
évident : « tétraèdre bien centré » et « à faces aiguës » sont deux
notions distinctes, chacune réfutant l'autre (fixtures v3 gravées). La
recherche d'un tel prédicat est posée, pas résolue.

## 7. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure          # 135 tests
./build/v4/mhgp4_forest_probe --workers-gate
./build/v4/mhgp4_forest_probe --workers-gate --inject=wspd-one-worker  # code 4
./build/v4/mhgp4_q4_oracle --inject=q4-center-parity                   # code 4
./build/v4/mhgp4_forest_probe --family=uniform --n=8000 --s=8 --seed=3 \
    --smax=11 --threads=4     # profil_wspd, profil_gen, q3_charge*,
                              # q4_entonnoir
```
