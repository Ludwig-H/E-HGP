# Carte de l'étage TREE (forêts 1..K) : ce qui est parallèle, ce qui est série, ce qui irait sur GPU

Rédigé le 2026-10-06 vers 01 h 30 UTC (heure lue par `date -u`). Lecture seule : aucun build, aucun test, aucun
commit, aucune branche. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 évoqué seulement comme piste
profile=quantized_u21_input_only
public_status=not_claimed
```

Code lu au commit qualifié `df904711a` (worktree `build/v11-impl-l3`, identique à `origin/main` pour `src/`) :
`src/tower/forest_pipeline.cpp`, `forest_parallel.cpp`, `forest_concurrent.cpp`, `forest_plateau.cpp`,
`forest_vertical.cpp`, `forest_ancestor_sweep.hpp`, `descent.cpp`, `locate.cpp`, `population_lookup.cpp`,
`src/sched/pool.cpp`, `src/api/compute.cpp`.

Étiquettes utilisées dans ce rapport :

- **M** : mesure G4 publiée, avec sa source ;
- **M-loc** : mesure locale ;
- **E** : estimation raisonnée, jamais présentée comme une mesure.

## 0. En bref

1. **Le chemin critique de l'étage tree à W48 n'est pas seulement la résolution.** Le mur vaut environ
   20 ms (contextes, classification, naissances) + R (fin de la dernière résolution) + une queue de publication.
   R est très stable d'une prise à l'autre : 114 à 120 ms (ng00), 83 à 89 ms (ng01), 94 à 99 ms (ng02). La queue,
   elle, varie de 2 à 59 ms. Ces chiffres viennent de 60 prises W48 de `claudediag1`, au moteur de tour inchangé
   depuis (**M**). Les prises lentes sont celles dont la queue est longue.
2. **Cause mesurée de la queue : le débit du publieur de l'ordre 5, pas son démarrage.** Le publieur démarre à
   3,7 ms, n'attend presque pas (0 à 15 ms), et consomme 81 à 151 ms de CPU. À un fil, la même publication en
   consomme 35 à 46 ms. L'écart est donc de **×2,2 à ×3,3** (**M**). Somme des cinq publieurs : ×2,5 à ×3,0 par
   rapport à W1. Somme des quatre balayages verticaux : ×3,5 à ×4,8. En étage séparé à W48, c'est-à-dire sans
   résolveurs actifs, la publication de l'ordre 5 ne coûtait que 51,5 ms contre 42,7 à W1 (×1,2 ; c40 contre
   e49, **M**). L'inflation vient donc de la **cohabitation avec les 39 résolveurs**. Ses parts respectives (SMT,
   L3/DRAM, réveils) ne sont pas encore mesurées.
3. **R est borné par le CPU des descentes exactes.** À W1, la résolution régulière coûte 2,54 à 3,38 s, soit
   **92 % du CPU de l'étage**. Le coût par pas est de 596 à 704 ns, contre 188 ns pour la v10 (audit des
   transpositions). À W48, 39 résolveurs consomment 3,1 à 4,5 CPU·s, soit ×1,3 par rapport à W1 (effet SMT). Ils
   sont parfaitement équilibrés : la première fin arrive à 113,5 ms et la dernière à 115,7 ms sur ng00 (**M**).
4. **La partie massivement parallèle et sans arithmétique est la publication.** La forêt d'ordre k est l'arbre de
   fusion de Kruskal sur des rangs entiers denses, avec les plateaux contractés en fusions n-aires. Les
   verticales sont des requêtes d'ancêtre de niveau. Les deux se portent sur GPU sans prédicat géométrique ni
   flottant. La numérotation canonique rend la sortie identique à l'octet. Mais ce levier ne paie qu'**après** que
   R soit passé sous 45 à 50 ms (**E**).
5. **La résolution est la partie lourde, et elle ne se porte pas bien sur GPU.** Ses deux tiers au moins sont
   des succès de table bon marché. Le coût est dans le tiers restant : MEB exactes, recherche de S*, census sur
   l'index, comparaisons de Level en Wide. C'est le profil divergent qui a borné la voie GPU des feuilles
   (3,2 fils actifs par warp). Le premier levier sur R est donc **CPU** : le coût par pas.
6. **Levier CPU immédiat et bon marché : l'ordonnancement.** Il s'agit de donner aux publieurs et suiveurs lourds
   (P5, P4, V5, V4) des cœurs physiques sans résolveur sur le frère SMT. Gain attendu : −15 à −50 ms sur les
   médianes de l'étage (**E**). Le gain est surtout sur ng02, dont la queue médiane dépasse 40 ms. Aucune
   décision ne change.
7. **Sorties `points` et `plat`.** Elles passent encore par `build_order` au masque 7 035, sans ordres
   concurrents. Leur étage tree vaut 138 à 184 ms à W48 pour deux fois moins de travail que FULL (**M**, QUAL). Un
   pipeline à un ordre (tranche S11) les ramènerait vers 65 à 90 ms (**E**). Cela ne touche pas le contrat FULL.

Ces leviers combinés ne ferment pas 100 ms sur FULL. L'étage `domain` vaut à lui seul 175 à 252 ms à W48. Même
avec une queue nulle, l'étage tree reste à 105–140 ms tant que le coût par pas des descentes n'est pas divisé
par 2 à 3.

## 1. L'algorithme et sa structure parallèle

Voie qualifiée : masque 16 379, K = 5, W48. Le pipeline s'active quand W ≥ 2K
(`forest_pipeline.cpp`, `pipeline_lanes`) : 48 tâches = **39 résolveurs + 5 publieurs + 4 suiveurs**.

| Étape | Ce qu'elle fait | Données (ng00, K = 5) | Parallélisme actuel | Nature |
| --- | --- | --- | --- | --- |
| Contextes (résidu de `forest_ns`) | table de populations I∪U → boule (CAS), espaces census, graines verticales, `ForestParallel` | 857 891 entrées, 44,2 Mo | partiel | allocation et remplissage |
| A Classification | type de chaque boule à chaque ordre | 1,31 M boules × 5 ordres | blocs du Pool | indépendante par boule |
| B Naissances | naissances par blocs, cohortes de même rang triées par **centre exact**, états DSU, liste des jobs, liaison de la table | 897 776 naissances | 3 distributions ; cohortes : une tâche par ordre | tri exact (i128, 4 limbes) |
| C Résolution | chaque face d'une cellule régulière (q = 2..4 faces) → graine : `bound` sur la table liée, sinon `descend_each_step` (MEB bornée, `find_support`, census sur l'index, trace stricte) jusqu'à un succès de table | 3,62 M traces, 4,80 M pas, 291 515 census, 21,15 M tests de points, 3,79 M présentations MEB | 39 lanes, blocs de 256 cellules réclamés dans l'ordre global des boules | indépendante par trace ; chaîne de pas dépendants ; exacte |
| D Publication | par plateau de rang : `find` des graines, `touch`, `unite_roots`, puis `close` : tri des racines touchées et fusion n-aire | ordre 5 : 448 805 cellules, 1,35 M graines, 438 011 plateaux, 341 081 naissances | **une tâche séquentielle par ordre**, qui suit les blocs publiés (époque et futex) | union-find séquentiel, sans arithmétique géométrique |
| E Verticales | image basse de chaque naissance haute (graine régulière réemployée : 857 771 sur 857 891 ; 120 descentes), puis balayage fermé par DSU et contrôle des enfants | 1,46 M nœuds hauts | une tâche suiveuse par ordre haut | requêtes d'ancêtre de niveau |

Points de structure tirés du code :

- **Les descentes ne lisent jamais le DSU**, seulement le domaine immobile (`forest_parallel.cpp`, ligne 1). La
  résolution est donc entièrement anticipable, et transférable vers un autre exécuteur.
- **La publication ne fait aucune géométrie.** `regular_cell` compare seulement des rangs entiers et des
  identifiants. Les cellules étendues, résolues en série dans le publieur, sont **négligeables** : 107 cellules
  et 0,76 ms à l'ordre 5 (c40, `full_paired.json`, **M**).
- La forêt d'ordre k est entièrement déterminée par deux données. D'abord les naissances avec leur rang. Ensuite
  les arêtes « graines d'une même cellule », au rang de la cellule. Les composantes à la coupe fermée de chaque
  rang définissent les nœuds. La numérotation est canonique : naissances par (niveau, centre), fusions par
  (niveau, plus petite naissance descendante). **Tout algorithme correct rend donc les mêmes octets.**
- Chaque forêt d'ordre 5 finit en **un seul arbre** : 576 371 nœuds pour 576 370 arêtes (**M**). Un découpage
  par composantes finales ne parallélise rien.

## 2. Faits mesurés

### 2.1 Étage tree, chaîne qualifiée (`claudefinmesure`, source `38b76701b`, moteur identique à `df904711a`)

Source : `morsehgp3D_v11/receipts/developpement_20261005/qualification_finale/claudefinmesure/sorties_g4.json`,
lue par `git show origin/main:…`. Mur de l'étage `tree` en ms, K = 5.

| Trame (sites) | W1 FULL, 4 prises | W48 FULL, 4 prises | W48 `supports` | `domain` W48 |
| --- | --- | --- | --- | --- |
| ng00 (39 885) | 3 757 à 3 811 | 161,8 / 163,6 / 172,9 / 185,9 (froid) | 168,1 à 173,2 | 211 à 251 |
| ng01 (35 551) | 2 766 à 2 800 | 118,4 / 125,4 / 139,2 / 141,4 | 119,3 à 142,2 | 175 à 206 |
| ng02 (45 845) | 3 221 à 3 275 | 150,4 / 150,7 / 164,8 / 165,6 | 149,7 à 174,1 | 215 à 245 |
| ng00, K = 10, W48 | — | 1 623 et 1 655 | 1 655 et 1 665 | 1 718 à 1 741 |

Identité : `tree_k_sha256` est commun à toutes les prises de chaque trame. FULL ng00 compte 1 541 750 nœuds et
897 776 naissances (**M**).

### 2.2 Phases de l'étage, prises W48 de `claudediag1` (binaire e49ea4690, tour identique ; quatre variantes de catalogue × trois trames × cinq prises = 60 prises)

Source : archives de session conservées dans
`build/v11-persist/diag1/results/results/cmd/000_ab/files/ab/t_*_w48_r*.stdout`, champs `phases` et
`pipeline_tasks`, recalculés pour ce rapport. Les dumps de ces prises sont identiques à la référence de leur
trame (reçu `developpement_20261004/mesures_g4_ab8_diag1`). Valeurs en ms : médiane (min–max) sur la variante
`new`, cinq prises.

| | ng00 | ng01 | ng02 |
| --- | --- | --- | --- |
| `forest_ns` | 159,5 (139,5–192,6) | 120,3 (107,2–121,9) | 173,5 (143,6–175,3) |
| contextes (résidu) | 9,3 | 7,8 | 9,7 |
| classification | 2,6 | 2,3 | 2,8 |
| naissances et liaison | 8,3 | 6,7 | 11,2 |
| **R** = fin de la dernière résolution | **117,4** (115,7–120,4) | **85,0** (84,2–86,9) | **96,6** (94,5–97,6) |
| queue publication + verticales | 23,2 (3,5–51,7) | 18,9 (6,1–20,0) | 51,1 (25,7–55,1) |
| CPU des 39 résolveurs (s) | 4,4 (4,2–4,5) | 3,2 (3,1–3,3) | 3,7 (3,6–3,7) |
| CPU du publieur de l'ordre 5 | 105,2 (102,7–142,3) | 84,6 (80,6–100,8) | 148,1 (98,9–151,3) |
| CPU des 5 publieurs | 390,8 (356–443) | 326,9 (280–347) | 431,8 (403–442) |
| CPU des 4 balayages verticaux | 318,1 (240–362) | 233,3 (197–273) | 359,3 (320–364) |
| départ du publieur de l'ordre 5 | 3,7 | ≈ 3,7 | ≈ 3,7 |
| attente du publieur de l'ordre 5 | 0 à 15 | 0,7 à 6,9 | 0,3 à 0,5 |

Sur les 60 prises, R reste entre 83,4 et 120,4 ms. La queue varie de 2,0 à 58,7 ms. Les médianes de queue par
trame et par variante vont de 5,5 à 52,4 ms : **la dispersion de l'étage est la queue** (**M**).

### 2.3 À un fil (étages complets, même source `t_new_*_w1_r0.stdout`), ms

| | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| forêts | 3 717,5 | 2 827,0 | 3 304,9 |
| résolution régulière | **3 378,7** | **2 538,1** | **2 916,4** |
| publication, 5 ordres | 133,5 | 110,6 | 149,1 |
| dont publication de l'ordre 5 | **42,7** | **35,0** | **46,0** |
| verticales, 4 balayages | 66,3 | 55,7 | 75,6 |
| dont balayage de l'ordre 5 | 34,4 | 28,3 | 39,5 |
| naissances / classification | 55,9 / 37,7 | 42,8 / 32,1 | 67,5 / 40,5 |
| pas de descente (Mi) ; ns par pas | 4,80 ; **704** | 3,92 ; **647** | 4,89 ; **596** |

En étage séparé à W48 (c40, `CARTE_V11.md` § 2.3, **M**), sans résolveurs simultanés :

- publication de l'ordre 5 : 51,5 / 35,7 / 47,2 ms ;
- balayage vertical de l'ordre 5 : 39,5 / 31,2 / 25,7 ms.

### 2.4 Travail logique par ordre (ng00, identique à tout W, **M**)

| k | traces | pas | census | tests de points | présentations MEB | cellules rejouées | plateaux |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 202 326 | 202 326 | 0 | 0 | 0 | 101 138 | 57 690 |
| 2 | 410 941 | 475 662 | 2 515 | 94 479 | 64 721 | 166 266 | 144 417 |
| 3 | 673 325 | 861 267 | 14 000 | 668 574 | 249 043 | 249 527 | 231 626 |
| 4 | 985 378 | 1 338 957 | 57 674 | 3 331 040 | 833 989 | 341 136 | 329 265 |
| 5 | 1 350 288 | 1 919 262 | 217 326 | 17 055 923 | 2 639 192 | 448 805 | 438 011 |

Il y a au plus 4,80 − 3,62 = 1,18 M pas au-delà du premier, donc **au moins 67 % des traces se résolvent en un
pas de table** (dérivé de **M**). L'ordre 5 porte 75 % des census, 81 % des tests de points et 70 % des
présentations MEB.

### 2.5 Autres faits utiles

- **Profil `perf` W1 (b872, ng00, `CARTE_V11.md` § 2.4, M).** Fonctions de la tour, en part du CPU du
  processus :
  - `PopulationLookup::bound` 2,9 %, `find_support` 2,7, `resolve_job` 2,6, `PopulationLookup::find` 2,3 ;
  - `CensusWorkspace::query` 2,1, `descend_each_step` 2,1, `strict_trace` 1,3, `hit` 1,1 ;
  - `birth_node` 1,0, `visit_located_part` 0,9, `bounded_meb` 0,9, `regular_cell` 0,8.

  `num::side`, `power_bound_signs` et `bound_terms` sont partagés avec le catalogue.
- **Voie `build_order`** (sorties `points` et `plat`, masque 7 035 sans ordres concurrents ; QUAL avant L2b,
  `carte_profil_etages.md` § 1.6, M). Étage tree :
  - W1 : 1 988 / 1 410 / 1 584 ms ;
  - W48 : 182 / 138 / 184 ms ;
  - accélération ×8,6 à ×10,9 seulement.
- **Matériel G4** (reçu `developpement_20261004/gpu_g4`) : 24 cœurs × 2 SMT ; RTX PRO 6000 Blackwell Server
  Edition, 97 887 Mio, `sm_120`, pilote 580.178.04, CUDA 12.9.41, GCC 11.4, CMake 3.22.1.
- **Voie GPU existante** (feuilles du catalogue seulement, même reçu, **M**) :
  - contexte de 78 ms (`cudaFree(0)`), recouvert en partie à froid ;
  - 3,2 à 3,4 fils actifs par warp ;
  - pile locale de 3,2 Kio par fil ;
  - retour à 4 Go/s, plus de 10 Go/s avec des pages pré-touchées.
- **Le Pool n'a ni affinité ni priorité** (`src/sched/pool.cpp`). Il réclame les tâches par indice croissant, par
  CAS de grain 1.

## 3. Diagnostic

On écrit l'étage tree à W48 sous la forme
`tree ≈ contextes + classification + naissances + max(R, fin du publieur de l'ordre 5) + reste`.
Les termes valent :

- contextes, classification et naissances : 17 à 24 ms (**M**) ;
- R : 84 à 120 ms (**M**).

Dans la cohabitation actuelle, le publieur de l'ordre 5 a besoin de 81 à 151 ms de CPU. Il finit souvent
**après** R, d'où la queue. Isolé, il en aurait besoin de 35 à 51 ms, bien en dessous de R : la queue
disparaîtrait (**E** appuyée sur la publication en étage séparé, 51,5 ms à W48).

Le plancher de l'étage, sans changer le travail, est donc d'environ 20 + R : 137 / 105 / 117 ms (**E**). Le
contrat demande davantage, et seul R peut le donner. Or R est au plafond matériel : 39 résolveurs sur 24 cœurs,
avec une inflation SMT de ×1,3. Seul le **travail par pas** le raccourcit. Une fois R sous 45–50 ms, la
publication série de l'ordre 5, même non contendue (35 à 51 ms), redevient le chemin critique. C'est le moment
où une forêt parallèle, CPU ou GPU, devient utile.

## 4. Opportunités chiffrées

Ordre : rapport gain/risque décroissant. Tous les gains sont des **estimations** (E) tant que le protocole
indiqué n'a pas été joué sur G4.

### O1 — Ordonnancer le pipeline : cœurs dédiés et priorité pour P5, P4, V5, V4 (CPU)

- **Étage visé.** Pipeline C+D+E (`forest_pipeline.cpp`, `sched/pool.cpp`).
- **Idée.** Placer les tâches série lourdes (publieurs des ordres 4 et 5, suiveurs des ordres 4 et 5) sur des
  cœurs physiques dont le frère SMT porte une tâche légère et souvent bloquée. Candidates : P1, P2, V2, qui
  attendent 8 à 98 ms par prise (P1 : 53 à 94 ms). Variante sans affinité : nice +k pour les résolveurs. Les indices de tâche sont fixes
  (`Pipeline::run`), donc l'affinité peut s'appliquer par tâche.
- **Coût actuel.** Queue de 2 à 59 ms ; médianes de 5,5 à 52,4 ms selon la trame et la variante. CPU de P5 :
  81 à 151 ms, contre 35 à 46 ms à W1 (**M**, § 2.2–2.3).
- **Gain attendu (E).** Si P5 tourne au rythme de l'étage séparé (≤ 51 ms), il reste sous R sur les trois
  trames, et la queue tombe à quelques ms. Médianes de l'étage : ng00 159 → environ 140, ng01 120 → environ 105,
  ng02 173 → environ 120 ms. Il faut retrancher la perte de capacité des résolveurs : environ 2 cœurs sur 24,
  soit +0 à +10 ms sur R. **Net : −15 à −50 ms** (Amdahl : le gain est borné par la queue, pas par R).
- **Exactitude.** Seul l'ordonnancement change, aucune décision ne lit l'horloge. Les forêts, verticales,
  journaux et registres sont identiques : les portes `mhgp11_tower_pipeline_equivalence` couvrent déjà W1, W4 et
  W48. L'absence d'interblocage ne change pas, car les tâches et leurs attentes restent les mêmes.
- **Risque.** Faible. L'affinité dépend de la topologie de l'hôte (paires SMT à lire dans
  `/sys/devices/system/cpu/cpu*/topology/thread_siblings_list`). Bruit d'ordonnanceur. Option désactivée par
  défaut hors G4.
- **Effort.** 0,5 à 1 jour (affinité par tâche dans `Pipeline::run`, plus une porte de non-régression).
- **Protocole G4.**
  1. `lscpu` et la topologie, consignés dans le reçu.
  2. A/B/A apparié en ordre de Williams, avec un bras A/A, ≥ 7 prises par trame, W48, froid (processus neuf)
     puis chaud.
  3. Quatre variantes : base ; affinité P5/P4/V5/V4 ; résolveurs −2 ; nice +5 sur les résolveurs.
  4. Exporter les champs déjà disponibles depuis `d5b1d0179` : `publish_start/end/cpu/wait`,
     `vertical_*`, `lanes_*`.
  5. Si `perf` est permis : `perf stat -e LLC-load-misses,cycles` par fil, pour attribuer l'inflation entre SMT
     et L3.
  6. Exiger l'identité à l'octet (dump `MHGP11FUL1` et `tree_k_sha256`) à chaque prise.
  7. Décider sur la médiane des rapports appariés.

### O2 — Publieur à flux compact, avec préchargement de ses états DSU (CPU)

- **Étage visé.** D, `ForestBuilder::publish`, `regular_cell` et `find`.
- **Idée.** Aujourd'hui, chaque publieur relit les 1,31 M boules (`kinds[b]`, `balls[b].rank`, `m`, `qmin`) pour
  n'en garder que 101 k à 449 k : cinq parcours concurrents des fiches de boules, qui se disputent L3 avec les
  résolveurs. Trois changements :
  1. La classification par blocs émet un flux compact par ordre : (boule, rang, indice de job, régulière ou
     étendue).
  2. Le publieur précharge `states[graine]` et `states[parent]` des cellules j + 8 : les graines sont connues
     dès que le bloc est publié.
  3. `parent` (u32) est séparé des champs de chaîne (`head`, `tail`, `next`, `top`, `touched`), pour que `find`
     tienne mieux en cache : 1,4 Mo au lieu de 8 Mo à l'ordre 5.
- **Coût actuel.** 35 à 46 ms à W1 et 81 à 151 ms à W48 pour l'ordre 5 ; 280 à 443 ms de CPU pour les cinq
  publieurs (**M**).
- **Gain attendu (E).** −20 à −40 % du CPU des publieurs. Le gain est plus fort en cohabitation, où chaque défaut
  de cache coûte un accès DRAM. Cumulé avec O1, la queue disparaît même sans affinité parfaite. À R constant,
  l'effet sur le mur se limite à la queue. Le levier sert surtout de prérequis quand R baissera.
- **Exactitude.** Même ordre des unions et mêmes racines (plus petite naissance). Les fusions n-aires, les
  registres `unions`, `touched_components`, `continuations` et `plateaus`, et le journal des graines sont
  inchangés.
- **Risque.** Faible. Mémoire supplémentaire : flux compact d'environ 12 octets par cellule rejouée, soit
  5,4 Mo à l'ordre 5. Il est à admettre au MemoryBudget, en coexistence avec les naissances.
- **Effort.** 1 à 2 jours.
- **Protocole G4.**
  1. W1 apparié : publication par ordre (`plateaus_ns` en voie par étages).
  2. W48 en pipeline : CPU de P5 et queue.
  3. Identité à l'octet et égalité des registres. Mutants : graine préchargée non relue, flux compact qui saute
     une cellule étendue.

### O3 — Diviser le coût par pas des descentes (CPU) : le seul levier qui raccourcit R

- **Étage visé.** C, `resolve_job`, `descend_each_step`, `visit_located_part`, `CensusWorkspace::query`,
  `bounded_meb` et `find_support`.
- **Idée.** Fermer l'écart avec la v10 : 596 à 704 ns par pas contre 188 ns. Les sous-leviers sont déjà
  instruits dans l'audit des transpositions :
  - V3 : borne exacte sur réseau et arbre radix de Morton pour le census. Simulé : bornes ×0,35–0,38 et tests
    ×0,26–0,28, mêmes listes et mêmes descentes.
  - V7 : mémo de cellule daté, typé comme certificat (contrat R5 accepté).
  - Instrumenter d'abord la répartition succès de table / MEB / `find_support` / census par pas, aucune n'étant
    mesurée séparément aujourd'hui.
- **Coût actuel.** R vaut 84 à 120 ms à W48 ; résolveurs 3,1 à 4,5 CPU·s ; 2,54 à 3,38 s à W1 (**M**).
- **Gain attendu (E).**
  - V3 : −5 à −25 ms ;
  - V7 : −5 à −12 ms ;
  - un coût par pas ramené près de la v10 donnerait R ≈ 30 à 40 ms.

  Par Amdahl, R descend à proportion de la part ôtée au CPU des résolveurs (ils sont équilibrés à 2 ms près).
  Mais sous 45–50 ms de R, c'est la publication de l'ordre 5 (35 à 51 ms) qui borne : O2 puis O7 deviennent
  nécessaires.
- **Exactitude.** V3 donne les mêmes listes I/U dans l'ordre de Morton, donc les mêmes descentes. V7 est un
  certificat typé (R ⊆ P_b complet, λ_b < λ strict au plateau). Rien de flottant.
- **Risque.** Moyen : assistant de bornes distinct de `power_bound_signs`, voie Wide conservée.
- **Effort.** V3 : 1 à 2 jours. V7 : 2 à 3 jours.
- **Protocole G4.** Compteurs déterministes d'abord (nœuds, bornes et tests du census, pas évités), en local
  puis sur G4. Ensuite W1 apparié sur la résolution seule, puis W48 sur R et le mur, avec identité à l'octet.

### O4 — Utiliser les fils matériels laissés libres par les tâches bloquées (CPU)

- **Étage visé.** C, avec la taille du Pool et le nombre de résolveurs.
- **Idée.** Pendant R, P1, P2, V2 et souvent V3 sont bloqués entre 8 et 98 ms par prise (`*_wait_ns`) : 4 à 6
  fils matériels restent inoccupés. Créer quelques fils de plus que de fils matériels (par exemple un Pool de 52
  à 54 et 43 à 45 résolveurs) laisse l'ordonnanceur du noyau les remplir. **À n'essayer qu'avec O1**, sinon
  le publieur de l'ordre 5 perd encore du débit.
- **Coût actuel.** R, comme en O3.
- **Gain attendu (E).** −3 à −8 % de R (−3 à −9 ms), si les tâches bloquées dorment vraiment dans le futex. Le
  CPU des balayages suggère une part d'attente active, à vérifier.
- **Exactitude.** Les lanes sont des ordinaux de blocs : le registre est sommé dans l'ordre fixe des tâches
  (`pipeline_orders`). Les sorties et registres hors census sont identiques. Les espaces census suivent le
  nombre de tâches, à admettre au budget (+4n octets par tâche possédée).
- **Risque.** Faible à moyen : bruit et inversion de priorité. L'absence d'interblocage reste acquise, car chaque
  tâche a son fil.
- **Effort.** 0,5 jour.
- **Protocole G4.** Celui d'O1, avec un Pool de 48, 52 et 56 fils, chaque fois avec et sans O1.

### O5 — Contextes et naissances : environ 20 ms avant la première résolution (CPU)

- **Étage visé.** Contextes (résidu de 7,7 à 10,8 ms), naissances (6,6 à 11,4 ms, accélération ×5 à ×7
  seulement), classification (2,2 à 2,9 ms).
- **Idée.**
  1. Trier les cohortes par centre exact en parallèle **entre cohortes**, et non plus une tâche par ordre.
  2. Recouvrir la construction de la table de populations et des espaces census avec la classification.
  3. Lancer les résolveurs de l'ordre k dès que ses naissances et sa liaison existent, au lieu d'attendre tous
     les ordres.
- **Coût actuel.** 17 à 24 ms sur le chemin critique (**M**, § 2.2).
- **Gain attendu (E).** −5 à −10 ms.
- **Exactitude.** Mêmes structures et même liaison contrôlée (naissance, clé, rang).
- **Risque.** Faible à moyen : la liaison ordre par ordre change l'argument « bind une fois après toutes les
  naissances ».
- **Effort.** 1 à 2 jours.
- **Protocole G4.** Phases `classify`, `births` et résidu, appariées, W48, identité à l'octet.

### O6 — Pipeline à un ordre pour `build_order` (tranche S11) : sorties `points`, `plat`, voire `supports` (CPU)

- **Étage visé.** Tree des sorties à un ordre (`order_tree`, masque 7 035).
- **Idée.** Livrer S11 : L ≈ W − 2 résolveurs sur les seuls jobs de l'ordre K, plus un publieur, sans verticales.
- **Coût actuel.** 182 / 138 / 184 ms à W48, pour 1 988 / 1 410 / 1 584 ms à W1 (**M**, QUAL).
- **Gain attendu (E).** R_K ≈ 1,4–2,0 s × 1,3 / 46 ≈ 40 à 57 ms. Avec environ 20 ms de préparation et une queue
  minime, l'étage serait à **65 à 90 ms**. Pour `supports`, ce serait un retour vers une voie deux fois moins
  coûteuse que FULL.
- **Exactitude.** Identité à `build_full(...).order(K)` (porte I10) et journal écrit par l'unique publieur.
  C'est déjà le contrat de L2b.
- **Risque.** Moyen : argument d'interblocage avec un seul publieur, journal, règle de L2 à rejouer.
- **Effort.** 2 à 3 jours.
- **Protocole G4.** Règle du § 11 de `SORTIES.md` rejouée par `sorties_g4.py`, étendu à `points` et `plat` (P1
  de `carte_profil_etages.md`). Identité `MHGP11PT` et `MHGP11SP`.

### O7 — Forêt exacte parallèle (GPU, ou CPU parallèle) et verticales par requêtes d'ancêtre de niveau

- **Étage visé.** D et E.
- **Idée.** Une fois R achevé (ou bloc par bloc), construire chaque forêt d'ordre k comme l'arbre de fusion de
  Kruskal des naissances (sommets, au rang de naissance) et des arêtes intra-cellule (graine₀, graineⱼ), au rang
  de la cellule. Les nœuds de même rang adjacents sont ensuite contractés, ce qui donne exactement les fusions
  n-aires du plateau atomique (composantes à la coupe fermée).
  - Sur GPU : arbre couvrant minimal de type Borůvka (ECL-MST), clé (rang, ordinal de cellule) pour le
    déterminisme ; dendrogramme parallèle, ou Kruskal série sur les seules n − 1 arêtes retenues (341 k au lieu
    de 1,35 M graines) ; contraction des égalités ; renumérotation canonique.
  - Verticales : requêtes indépendantes « plus haut ancêtre de rang ≤ λ » sur une forêt close, par
    sauts binaires, plus le contrôle des images des enfants. Tout est en entiers u32 : ni géométrie ni
    flottant.
- **Coût actuel.**
  - Publication de l'ordre 5 : 35 à 46 ms à W1 et 36 à 52 ms en étage séparé à W48 ; 81 à 151 ms de CPU en
    pipeline.
  - Balayage vertical de l'ordre 5 : 28 à 40 ms à W1.
  - À K = 10, l'audit estimait la publication de l'ordre 10 à 119–146 ms en série (**E**).
- **Gain attendu (E).**
  - Aujourd'hui : 0 à −(queue), donc pas mieux qu'O1.
  - Une fois R sous 45–50 ms (O3), il retire le plancher série de 35 à 50 ms et le remplace par environ 5 à
    10 ms : noyau, plus 14 Mo de graines à l'aller et 16 à 40 Mo de nœuds au retour à plus de 10 Go/s
    pré-touchés, contexte chaud.
  - À K = 10 : −80 à −120 ms (audit V4).
- **Exactitude.**
  - Rangs denses entiers et numérotation canonique : sortie identique à l'octet.
  - Les registres `unions`, `touched_components`, `continuations`, `plateaus` et `vertical_checks` se
    recalculent indépendamment de l'ordre de traitement.
  - En revanche, `ancestor_unions`, `ancestor_find_steps` et `ancestor_activations` dépendent du chemin
    suivi : il faut un contrat de registre (compte logique contre compte physique), comme pour J3.
  - Au registre des preuves, « contraction des plateaux par composantes fortement connexes » est une
    `proof_obligation`, et « traitement séquentiel de niveaux égaux » est `false_in_general` (risque de
    binariser une multifusion). L'égalité « contraction des rangs égaux = plateau atomique » doit y être
    inscrite **avant** le port, avec les fixtures existantes : témoin (0,2,4) ternaire, diamant K2, E5.
  - Voie GPU : exacte sur l'appareil (entiers u32), sinon `unresolved` reprise sur CPU, ce qui est trivial ici.
- **Risque.** Élevé. Deux voies CPU proches ont été mesurées négatives :
  - Borůvka et étiquette minimale de la v9 : 1 264,7 ms à K = 5 sur ng00 ;
  - réduction segmentée par blocs : 31 à 45 % des fusions à la couture, 5 à 10 fois le travail
    (`PISTES_DE_RUPTURE.md` R2.6).

  Par ailleurs, la conception par barrière perd le recouvrement actuel, et le contexte CUDA (78 ms) doit être
  chaud ou ouvert en parallèle.
- **Effort.** 1 à 2 semaines, microbanc compris.
- **Protocole G4.**
  1. **Microbanc hors moteur d'abord** : vider sur G4 les naissances (rang) et les graines par cellule de chaque
     ordre (ng00 à ng02, K = 5 et 10), puis construire la forêt par un programme CUDA autonome.
  2. Identité exigée : forêt canonique égale à l'octet à la section de `MHGP11FUL1`.
  3. Temps à froid et à chaud, transferts inclus, contexte séparé.
  4. Critère d'entrée dans le moteur : forêt K = 5 ≤ 10 ms et K = 10 ≤ 50 ms, transferts compris, **et** R
     déjà sous 50 ms.
  5. Ensuite seulement : intégration derrière un bit, portes TSan, mutants (multifusion binarisée, image
     ouverte, ex aequo non contractés).

### O8 — Descentes par fronts d'onde sur GPU (seule voie GPU qui attaque R)

- **Étage visé.** C.
- **Idée.** Traiter les traces par vagues :
  1. vague 0 : succès de table (`bound`, hachage de K sites) sur l'appareil ;
  2. vagues suivantes, pour les échecs : MEB exacte de k ≤ 5 points (énumération uniforme des supports, peu
     divergente), recherche de S*, census sur l'index sur l'appareil, trace stricte. Chaque vague est un lot.
  3. Repli : tout ce qui sort du domaine certifié (Level en Wide, débordement i128) part en `unresolved` et est
     repris sur CPU avant admission.
- **Coût actuel.** R vaut 84 à 120 ms. Au plus 1,18 M pas au-delà du premier (ng00), 291 515 census,
  21,15 M tests de points, 3,79 M présentations MEB (**M**).
- **Gain attendu (E, faible confiance).** R → 15 à 40 ms plus 3 à 5 ms de transferts : table de 44 Mo,
  populations, index, sauf si le domaine est déjà résident. N'a d'effet sur le mur qu'avec O7 : sinon la
  publication non contendue (35 à 51 ms) borne. Les succès de table seuls ne représentent qu'environ 12 % de la
  résolution au profil W1 (`bound` 2,9 % et une part de `resolve_job`, sur 41 % de forêts) : c'est trop peu
  pour porter cette vague seule.
- **Exactitude.** Prédicats exacts sur l'appareil, ou `unresolved` vers le CPU (contrat R7). Graines et dates
  contrôlées comme aujourd'hui (date initiale < niveau du plateau).
- **Risque.** Très élevé :
  - divergence et pile locale, comme pour les feuilles ;
  - Level jusqu'à 180/204 bits ;
  - le census sur l'appareil n'a jamais payé de bout en bout (v6 : noyaux 154 ms pour un étage de 7,7 s ; v7 :
    29,8 ms pour 846 ms).
- **Effort.** Plusieurs semaines.
- **Protocole G4.**
  1. Exporter l'histogramme des pas par trace et la part de chaque route par pas (diagnostic CPU).
  2. Microbancs sur requêtes vidées : MEB k ≤ 5 et census par lot, identité de chaque résultat contre le CPU, puis
     ns par pas sur l'appareil contre CPU.
  3. Seuil d'entrée : moins de 150 ns par pas, transferts compris, et taux de repli borné par une porte.

## 5. Pistes GPU écartées, et pourquoi

| Piste | Raison | Source |
| --- | --- | --- |
| Classification sur GPU | 2,2 à 2,9 ms à W48 ; les transferts et la synchronisation coûteraient autant | § 2.2 (**M**) |
| Naissances et tri des cohortes par centre exact sur GPU | 6,6 à 11,4 ms ; comparaisons rationnelles à 4 limbes ; un parallélisme CPU entre cohortes suffit (O5) | § 2.2 (**M**) |
| Union-find synchrone par plateau sur GPU (un noyau ou une barrière par rang) | 438 011 plateaux à l'ordre 5 : autant de barrières que de rangs ; il faut une voie hors ordre (O7) ou rien | § 2.4 (**M**) |
| Succès de table seuls sur GPU | environ 12 % de la résolution au profil (**E**) ; orchestration et transferts pour −10 à −14 ms au mieux | `CARTE_V11.md` § 2.4 (**M**) |
| Cellules étendues hors du publieur | 107 cellules et 0,76 ms à l'ordre 5 | c40 `full_paired.json` (**M**) |
| Changer la taille des blocs de résolution (256) | résolveurs équilibrés : première fin à 113,5 ms, dernière à 115,7 ms | § 2.2 (**M**) |
| Proposition flottante sur GPU puis recertification CPU | détruit l'étage (phase 15) ; contraire à F1–F6 | audit des transpositions § 5.4 |
| Mémo de lane | 2,6 % de succès, incompatible avec le pipeline ; déjà retiré | `AUDIT_TRANSPOSITIONS_V11.md` § 2.3 |
| Plus de fils que de fils matériels sans priorité ni affinité | ralentit encore le publieur de l'ordre 5, qui est le chemin critique | § 3 |
| Borůvka CPU, réduction segmentée par blocs, forêt par diviser pour régner | mesurés ou simulés négatifs (v9 1,26 s ; couture de 31 à 45 %) ; à ne rouvrir qu'avec un microbanc qui les bat (O7) | `AUDIT_TRANSPOSITIONS_V11.md` § 5.3 |

## 6. Questions ouvertes

1. **Attribution de l'inflation ×2,2 à ×3,3 du publieur et ×3,5 à ×4,8 des balayages** en cohabitation : SMT, L3
   ou DRAM, réveils futex ? La part d'attente active éventuelle de `std::atomic::wait` (libstdc++ de GCC 11)
   dans le CPU des tâches qui attendent est aussi à mesurer. C'est la mesure qui décide O1, O2 et O4.
2. **Pourquoi un pas de descente coûte-t-il 596 à 704 ns en v11 contre 188 ns en v10 ?** Il manque une
   répartition par route et par pas (table, MEB, `find_support`, census, trace stricte).
3. **Froid ou chaud.** Le contexte CUDA (78 ms) ne laisse une place à O7 ou O8 que dans un processus résident,
   ou avec une ouverture anticipée recouverte. Décision de l'utilisateur (déjà posée au § 6.1 de l'audit des
   transpositions).
4. **Contrat de registre** d'une forêt ou de verticales parallèles : les compteurs `ancestor_*` dépendent du
   chemin. Faut-il les déclarer physiques, comme pour J3 ?
5. **Registre des preuves** : faut-il inscrire « contraction des rangs égaux de l'arbre de Kruskal = plateau
   atomique » (lien avec la ligne `proof_obligation` des plateaux) avant tout prototype O7 ?
6. **K = 10** : l'étage tree vaut 1,62 à 1,66 s à W48 (une prise par sortie, **M**). O7 et O8 y pèseraient
   davantage qu'à K = 5. Est-ce une cible ?
7. **Topologie G4** : paires SMT, nœuds NUMA, taille de L3. À consigner par `lscpu` dans le prochain reçu avant
   O1.
8. **`points` et `plat`** : restent-elles sur `build_order` au masque 7 035, ou passent-elles par S11 (O6) ou
   par la voie FULL comme `supports` ?

## 7. Protocole commun de mesure sur G4

- Session gardée `gcp-migration/v11_session.py`, arrêt `TERMINATED` certifié.
- Trames ng00, ng01 et ng02 entières sans sol, grille 1 mm, u21, K = 5 (puis K = 10 pour O7).
- Binaire Release de référence et variante derrière un bit d'optimisation inactif par défaut.
- A/B/A en ordre de Williams, avec bras A/A ; au moins 7 prises par trame à W48, une à trois à W1. Froid :
  processus neuf, caches non vidés, déclarés. Chaud : passes successives.
- À chaque prise : `MHGP11FUL1` (ou `tree_k_sha256`) identique à la référence de la trame, registres logiques
  égaux. Un écart d'octet est un refus, jamais un compromis.
- Grandeurs publiées : `forest_ns` ou étage `tree`, R, queues, départ, fin, CPU et attente de chaque tâche du
  pipeline ; pour le GPU, contexte, aller, noyau, retour et repli séparés.
- Décision sur la médiane des rapports appariés et le test des signes. Cinq paires ne tranchent pas sous environ
  30–40 ms à W48 (bras A/A jusqu'à 9 %).
- Un reçu par session, immuable, ancré au commit, sans coordonnée LiDAR.
