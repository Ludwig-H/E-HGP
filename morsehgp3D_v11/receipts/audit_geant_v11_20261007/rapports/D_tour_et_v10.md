# Rapport D — Tour FULL de la v11 : forêts, graines, publieurs, verticales ; comparaison à la v10

Lecteur D (un des huit lecteurs indépendants). 7 octobre 2026. Lecture seule sur `/workspaces/E-HGP` (HEAD `e968aba8d`,
moteur gelé `ac081a06f`). Aucune compilation, aucune mesure, **GCP non utilisé**.

```text
phase=exploration_v11_hors_registre (close)   backend=cpu_reference   profile=quantized_u21_input_only   public_status=not_claimed
```

**Conventions.** **[V]** = vérifié dans le code (chemin:ligne) ou dans un reçu (chemin) ; **[I]** = inférence de ma part.
Les chemins sans préfixe sont relatifs à `morsehgp3D_v11/`. « v10 » désigne `morsehgp3D_v10/src/tower/tower.cpp` au HEAD :
son chemin de descente est identique à celui de `777406b82` (mesuré en session G4 4) et de `8b8d66f6e` (session 1,
seule session dont les compteurs de descente par ordre sont publiés ; `git diff 8b8d66f6e 777406b82 -- tower.cpp` ne
touche que l'entrée `cover` et le dendrogramme de points) [V].

---

## 0. Résumé (douze constats)

1. **L'algorithme au gel est conforme à ce que décrivent l'audit final (§ 7.1) et le rapport C**, à quelques nuances
   près (§ 6). Les octets sont identiques à tout W grâce à cinq invariants : fonctions pures pour classer et résoudre,
   écritures à positions fixées par ordinal, un seul écrivain par ordre qui suit l'ordre canonique des boules,
   racine DSU = plus petite naissance, clôtures triées (§ 2.6) [V].
2. **Chemin critique K5 confirmé** (`claudeg1`, base, voie CPU, 6 processus froids) : préambule 12,8 à 18,4 ms, puis
   max(R = 82,8 / 63,2 / 74,3 ms ; fin de P5 = 102,3 / 81,8 / 97,5 ms), V5 collé à P5 (0 à 2 ms) [V].
3. **K10** : R = 1 214 / 894 / 988 ms ; 35,2 / 25,9 / 28,6 CPU·s sur 29 voies [V]. **Omis par l'audit** : un préambule
   de 61 à 78 ms à K10 [V], dont une table de populations que j'estime à environ 362 Mo [I].
4. **Question n° 1 (×5 des résolveurs à K10).** Ce n'est **pas** le nombre de pas : à l'ordre 10, la v11 fait
   6,18 M pas contre 5,26 M (×1,18), 2,35 M MEB contre 1,97 M (×1,20) et 1,16 M census contre 0,95 M (×1,22), pour
   exactement les mêmes 3,83 M graines [V]. L'écart vient du **coût du pas** : ×2,65 en temps de fil à W48 [V calcul]
   (×3,2–3,7 à W1 dès K5, d'après les cartes [V]). Le fait le mieux établi est la **MEB exacte exhaustive** de la v11 :
   74,2 présentations de supports par MEB à l'ordre 10, contre 4,6 à l'ordre 5 [V]. La v10, elle, propose la MEB en
   flottant puis la certifie en exact (0 repli sur 7,66 M) [V].
5. Les causes avancées par l'audit (§ 3.2, [I]) sont **surestimées ou fausses**. Le choix des k plus proches ne vaut
   que +3 % de MEB : mutant `JUMP_ANY` de l'audit L06 de la v10, dump identique [V]. Le mémo daté vaut 7 à 9 % des
   pas [V]. La partition T6 concerne les **boîtes du catalogue**, pas les descentes [V]. Les 29 voies ne coûtent que
   ×1,2 à ×1,3 en cœurs physiques [I].
6. **Constat majeur absent de l'audit.** La conception de la tour v11 du 2 octobre
   (`build/v11-persist/conception/CONCEPTION_TOUR.md`, sha256 `01e217f0…`, **non versionnée**) prévoyait déjà :
   - D-G3 : MEB proposée en flottant et certifiée par le catalogue (lemme T1, sans arithmétique) ;
   - D-G4 : census borné aux k plus proches ;
   - D-F1 : noyau union-find sans lots et contraction parallèle des plateaux (théorème T4). Son prototype, mesuré sur
     les entrées réelles de la v10, est ×2,6 plus rapide que le Kruskal de la v10, avec des forêts identiques ;
   - D-F3 et D-V1 : historique d'attache et verticales en O(1).

   **Rien de cela n'a été implanté.** Ce document doit être versionné avant toute perte de `build/`.
7. **Forêt comme arbre couvrant minimal (V4/O7).** Le lemme de l'auditeur (composantes du MST par rang, puis
   contraction des chaînes de même rang) n'est pas au registre [V]. En revanche, le registre contient **déjà** :
   - Borůvka sur l'expansion étoilée, `proved_here` (l. 237) ;
   - le contre-exemple de la contraction d'hyperarête entière, `false_in_general` (l. 238) ;
   - l'hyper-Kruskal par lot, `proved_here` (l. 245).

   La `proof_obligation` de la ligne 243 (contraction des plateaux) a une preuve candidate : T4 de `CONCEPTION_TOUR`,
   annexe A.4 [V].
8. **Les « précédents négatifs » sont faibles.** Les 1 264,7 ms du Borůvka v9 sont une mesure **locale** d'un
   constructeur par étiquette minimale. La couture de 31 à 45 % vient d'un découpage **spatial** [V]. Aucun ne teste
   un Borůvka sur l'expansion étoilée, ni le noyau D-F1.
9. **Compteurs logiques.** Le contre-exemple du triangle (0,0), (3,0), (1,2) est confirmé [V]. Les compteurs
   `plateaus`, `continuations` et `touched_components` sont des propriétés des cellules, pas du dendrogramme.
10. **Les tableaux du § 7 de l'audit sont exacts pour l'essentiel** : tous les chiffres des leviers ont été recoupés
    avec leurs reçus. Six corrections et quatre omissions sont relevées au § 6.
11. **v12, la tour à 40–50 ms à K5 est plausible** [I], à trois conditions :
    - un pas de descente au coût de la v10 : D-G3, plus les filtres F6 et le mémo déterministe T3 ;
    - le noyau D-F1 recouvert par la résolution, puis contraction et verticales parallèles ;
    - un préambule inférieur ou égal à 5 ms.

    La forêt par MST parallèle n'est qu'un second recours.
12. **Mesure décisive** : profil par route à W1 sur G4, même session, v11 contre v10 R2, à K10 sur ng00, avec une
    variante v11 « MEB proposée et certifiée » à vidage identique (§ 4.5).

---

## 1. Ce qu'est une cellule, une graine, une descente (vocabulaire géométrique)

Notations de `docs/MATHEMATIQUES.md` §§ 1–6 [V] :
- $\beta(F)$ est le rayon carré de la plus petite boule englobante (MEB) de $F$ ;
- pour une boule critique $b$, $I_b$ est l'intérieur strict ($p$ sites), $U_b$ la coquille ($m$ sites) et $q$ le
  cardinal minimal d'un support ;
- $L_k(a)$ est la région couverte par au moins $k$ boules de rayon $\sqrt a$.

**Cellule $(b,k)$.** Une boule critique $b$ examinée à l'ordre $k$, dans sa fenêtre $p+q-1\le k\le p+m$ (T3,
`MATHEMATIQUES.md` l. 188–198). Au niveau $\lambda_b$, le centre $c_b$ est couvert par au moins $k$ boules : c'est là
que $L_k$ change localement.
- **Naissance** si $k=p+m$ : la $k$-partie $P_b$ apparaît isolément.
- **Jonction** sinon. Pour une coquille régulière ($m=q$), la jonction est à $k=p+q-1$. Les $q$ faces
  $I_b\cup(U_b\setminus\{u\})$ sont des $k$-parties de MEB strictement plus petite (traces strictes, T2). Elles
  existaient donc avant $\lambda_b$, dans des composantes que $c_b$ réunit à $\lambda_b$.
- **Coquille étendue** ($m>q$) : les morceaux sont les composantes des $t$-parties séparables.

**Graine.** Pour chaque trace stricte $F$ d'une jonction, c'est la **naissance** de la forêt d'ordre $k$ dont la
composante, à la coupe fermée $\beta(F)$, contient $F$. Le publieur réunit les racines DSU des graines d'une même
cellule.

**Descente (T5).** Glissement vers le bas de la filtration, à partir de $F$ :
- si $p\ge k$ dans $B(F)$, prendre $k$ sites intérieurs : leur MEB est strictement plus petite ;
- sinon prendre $I\cup A$, avec $A$ séparable dans la coquille ;
- s'arrêter quand $F=P_b$ avec $|P_b|=k$, c'est-à-dire sur une naissance.

Le niveau décroît strictement et on reste dans la composante de $F$ à la coupe fermée $\beta(F)$. Le terminal dépend
de la politique, mais pas sa classe pour $a\ge\beta(F)$.

**Date.** Le résultat n'est valable qu'à partir de la date **initiale** $\beta(F)$, jamais de la terminale (témoin
$X=\{0,2,4,6\}$, `DESCENT_MEMO.md`). D'où la garde : la date initiale doit être strictement inférieure au niveau de
la cellule.

---

## 2. L'algorithme exact au gel (livrable 1)

### 2.1 Domaine et classification (étape A)

- `FullDomain` immuable : index radix de Morton, catalogue trié par rang, table support → boule
  (`full_domain.hpp:15-61`) [V].
- `classify_range` (`forest_build.cpp:156-185`) [V] :
  - si $m=q_{\min}$ (cellule régulière), la cellule est analytique : naissance si $k=p+q$, sinon `kinds=2` et une
    tâche régulière ;
  - sinon `classify_cell` (`cells_classify.cpp:30-60`) cherche le premier témoin strict.
- Voie concurrente : `classify_parallel` (`forest_concurrent.cpp:139-155`), 256 blocs par ordre, compteurs sommés
  dans l'ordre fixe des blocs [V].

### 2.2 Naissances (étape B)

- `parallel_births` (`forest_build.cpp:416-464`) [V], en trois distributions :
  1. naissances et listes de cellules régulières par blocs (`birth_block`, l. 314-335), positions données par des
     sommes préfixes ;
  2. tri des cohortes de même rang par **centre exact** (`sort_cohort_range`, l. 72-101, avec
     `num::compare_centers`), en $(K-1)\times32$ tranches alignées sur les cohortes ;
  3. table dense clé → nœud (l. 366-374).
- L'ordre 1 trie les sites par xyz (l. 58-68).
- Puis la table de populations est **liée** : chaque entrée $|I\cup U|=h$ reçoit sa naissance dans la forêt d'ordre
  $h$ (`population_lookup.cpp:140-174`), après contrôle (forêt $h$, clé, rang) [V].

### 2.3 Résolution des graines (étape C)

`ForestParallel::resolve_job` (`forest_parallel.cpp:51-116`) [V]. Pour chacune des $q$ faces d'une cellule
régulière, sommet omis décroissant (ordre lexicographique de `build_cell`) :

1. **Voie liée** (l. 73-84). `PopulationLookup::bound` (`population_lookup.cpp:176-192`) hache la partie triée, sonde,
   compare toute la ligne et rend (nœud lié, rang). Il contrôle rang(graine) < rang(cellule) et compte un pas.
2. **Sinon `descend_each_step`** (`population_lookup.cpp:258-292`, avec `first_missed=true`). À chaque pas : un
   `hit` de table (l. 211-232), sinon `descent_step` (`descent.cpp:165-180`) puis `visit_located_part`
   (`locate.cpp:59-87`) :
   - `bounded_meb` (`meb.cpp:157-175`) : diamètre exact sur C(k,2) paires, puis **tous** les q3 puis les q4 en ordre
     lexicographique, jusqu'au premier support strict contenant. On obtient le support **local** canonique ;
   - `find_support` du support local. En cas de succès, $I$ et $U$ sont lus au catalogue ;
   - sinon census sur l'index (`census_workspace.cpp:47-63`), **saturant au seuil $k$**. Il rend les $k$ premiers
     intérieurs dans l'ordre de parcours, c'est-à-dire l'ordre de Morton (`index.hpp:89-95` : « ni K plus
     proches »). Puis `global_support` (`canonical.cpp:52-69`) et `find_support` ;
   - `DescentBuilder::run` (`descent.cpp:132-148`) : si $p\ge k$, la partie suivante est `interior().first(k)`
     (l. 138-142). Sinon `strict_trace` (l. 103-130) ou terminal (l. 88-101). Chaque transition est contrôlée
     strictement décroissante par comparaison **exacte** de `Level` (l. 289 de `population_lookup.cpp`).
3. **Garde de date** (l. 102-107). La date initiale doit être strictement inférieure au niveau de la cellule. La
   naissance rendue doit exister et avoir un rang strictement inférieur.

Les graines sont écrites dans les cases `4·job` de leur ordre (`forest_pipeline.cpp:73`). La graine de la face omettant
$U[q-1]$ est mémorisée pour la verticale de la naissance de la même boule à l'ordre $p+q$
(`regular_vertical_seeds.hpp:25-37`) [V].

Les cellules étendues sont résolues **dans le publieur** par `ForestBuilder::cell` (`forest_plateau.cpp:40-70`) :
`build_cell` énumère les traces strictes. Elles sont négligeables : 107 cellules, 0,76 ms à l'ordre 5
(`carte_forets.md` § 1) [V].

### 2.4 Publication par plateau de rang (étape D)

`ForestBuilder::publish` (`forest_concurrent.cpp:42-78`) parcourt les boules dans l'ordre (donc le rang) et ne garde
que `kinds==2` [V]. Pour chaque cellule régulière :
- `await_job` (l. 90-107) : lecture de l'époque, puis de l'état du bloc, puis attente futex sur l'époque ;
- `regular_cell` (`forest_plateau.cpp:103-125`) : pour chaque graine, `find` (compression complète, l. 9-16), puis
  `touch` (inscription de l'ancienne composante dans `touched` et dans une chaîne singleton, l. 18-25), puis
  `unite_roots` (l. 29-38). La racine est la **plus petite** naissance et les chaînes sont concaténées.

Au changement de rang, `close` (l. 72-101) :
- trie les racines touchées ;
- pour chaque racine survivante, parcourt sa chaîne d'anciennes composantes, avec une garde de cycle ;
- au moins deux éléments donnent **une** multifusion N-aire, dont les enfants sont les `top` triés et le nœud est
  au rang du plateau ; un seul élément donne une continuation, sans nœud ;
- remet `touched` à zéro.

Toutes les 32 clôtures, `announce` publie `nodes` puis `closed` en *release* et notifie (l. 111-117). `finish` exige
une racine unique et arêtes + 1 = nœuds (`forest_build.cpp:474-481`) [V].

### 2.5 Verticales (étape E)

Le suiveur de l'ordre haut $h$ (`forest_vertical.cpp:175-199`) fusionne deux flux déjà triés par rang : naissances
puis fusions publiées de l'ordre $h$, à rang égal la naissance d'abord (`follow_step`, `forest_internal.hpp:59-66`). Il
attend que l'ordre bas ait clos le niveau (`await_lower`, l. 97-104, abandon revérifié après chaque attente : correctif
`3bd4d734e`). Puis :
- `ClosedAncestorSweep::advance` (`forest_ancestor_sweep.hpp:40-58`) active **toutes** les fusions basses de rang
  inférieur ou égal au niveau. C'est un DSU par taille, avec le sommet géométrique `top_` tenu à part ;
- image d'une naissance : graine régulière réemployée (`birth_image`, l. 132-146), sinon descente des $k-1$ premiers
  sites de $I\cup U$ (`forest_vertical_seed.hpp:6-31`, garde large initiale inférieure ou égale au niveau) ;
- image d'une fusion : l'image commune de **tous** les enfants (`visit`, l. 90-111), sinon `tower_invariant`.

À K5 sur ng00, la graine réemployée évite 857 771 descentes sur 857 891 [V] (`full_regular_vertical_20261003/reuse1`).
L'ordre de mémoire tient : le résolveur écrit la graine, puis publie le bloc en *release* ; le publieur bas l'acquiert,
clôt le rang et publie `closed` en *release* ; le suiveur l'acquiert. La chaîne *happens-before* est donc complète
(`forest_vertical.cpp:130-131`) [V].

### 2.6 Pourquoi les octets sont identiques à tout W [V]

1. La classification et la résolution sont des **fonctions pures** du domaine immuable : les descentes ne lisent
   jamais le DSU (`forest_parallel.cpp:1`). La politique est fixe : premiers intérieurs de Morton, support local
   lexicographique.
2. **Positions fixes** : graines aux cases `4·ordinal`, naissances aux sommes préfixes des blocs, registres sommés dans
   l'ordre (tâche, ordre) après le join (`forest_pipeline.cpp:273-275`).
3. **Un seul écrivain par ordre**, qui suit l'ordre canonique des boules. Les attentes changent le moment des
   opérations du DSU, jamais leur ordre.
4. Union par **minimum** (racine = plus petite naissance), clôture qui **trie** les racines touchées, enfants triés.
   La numérotation est canonique : naissances par (rang, centre exact, ordre total puisque deux boules distinctes de
   même niveau ont des centres distincts), fusions par (rang, plus petite naissance).
5. Le balayage vertical suit un flux trié fixe et active **toutes** les fusions de rang égal avant la première requête
   du rang. Le DSU par taille n'affecte pas la réponse (`top_`).

Portes : `pipeline_decisions` (4 000 flux scriptés, `tests/tower/forest_pipeline_test.cpp:49`) et
`pipeline_equivalence` (quatre nuages de 70 à 160 sites, W1 contre W48 répété, l. 133-180). Sur les trames, les
empreintes des vidages sont identiques à chaque prise W48 et W1 des reçus. 163 mutants dans `tests/mutants/tower.json`
[V].

---

## 3. Le parallélisme (livrable 2)

### 3.1 Mécanique [V]

- **Pool synchrone** (`sched/pool.cpp:55-110`) : une invocation à la fois, réclamation de tranches par CAS et par
  indice croissant, sans vol de travail ni affinité.
- **Pipeline** (`forest_pipeline.cpp:186-278`) : une seule distribution de $W$ tâches.
  - $L=W-(2K-1)$ **résolveurs** (`pipeline_lanes`, l. 180-184) réclament des blocs de 256 cellules (l. 21), triés
    selon l'ordre global de la première boule, tous ordres confondus (l. 241-244) ;
  - $K$ **publieurs**, un par ordre ;
  - $K-1$ **suiveurs**.
- **Absence d'interblocage** : une tâche n'attend que des tâches d'indice inférieur, déjà réclamées. Cela vaut à tout
  W, W1 compris.
- **Synchronisation** :
  - un état d'un octet par bloc (`atomic_ref`) et une époque par ordre (`fetch_add`, release, puis `notify_all`) ;
  - `closed` et `nodes` publiés toutes les 32 clôtures ;
  - fin et abandon par `ForestProgress::finish` (`forest_internal.hpp:37-46`).
- **Répartition à W48** : 39 / 5 / 4 tâches à K5, 29 / 10 / 9 à K10.
- **Placement O1** (`forest_placement.cpp:90-115`, en-tête `.hpp:1-8`) :
  - chaque tâche lourde (P_K, P_{K−1}, V_K, V_{K−1}) reçoit **le premier fil** d'un des 4 derniers cœurs ;
  - les **fils frères** de ces cœurs hébergent les tâches légères (P1..P_{K−2}, V2..V_{K−2}) ;
  - les résolveurs prennent les 40 fils des 20 autres cœurs ;
  - le plan est inactif si K > 5, et donc à K10.

### 3.2 Chemin critique mesuré [V]

Source : `receipts/developpement_20261007/filtre_g1_avx2/claudeg1/gpu_ab_report_ab_*.json`, bras `base`, médianes
à froid. Préambule = `forest_ms` − fin de P_K.

| | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| K5 CPU 802811 : `forest_ms` | 117,2 | 94,6 | 115,9 |
| classification / naissances (ms) | 2,2 / 4,5 | 1,9 / 4,0 | 2,3 / 5,3 |
| préambule (`forest` − fin P5) | 14,9 | 12,8 | 18,4 |
| R, fin du dernier résolveur | 82,8 | 63,2 | 74,3 |
| fin de P5 / de V5 | 102,3 / 102,3 | 81,8 / 81,8 | 97,5 / 99,6 |
| CPU de P5 / de V5 (ms) | 98,9 / 87,9 | 80,7 / 68,2 | 94,6 / 68,2 |
| P5 échantillonné : cellules / clôtures (ms) | 64,7 / 34,7 | 50,3 / 21,2 | 63,7 / 22,5 |
| CPU des 39 résolveurs | 3,08 s | 2,35 s | 2,77 s |
| K10 GPU 868347 : `forest_ms` | 1 294,2 | 957,8 | 1 068,8 |
| classification / naissances (ms) | 14,6 / 21,5 | 11,6 / 17,8 | 14,5 / 22,0 |
| **préambule (`forest` − fin P10)** | **77,6** | **61,4** | **78,3** |
| R | 1 214,4 | 894,5 | 988,2 |
| CPU des 29 résolveurs | 35,2 s | 25,9 s | 28,6 s |
| CPU de P10 / de V10 (ms) | 241,9 / 182,0 | 164,8 / 123,8 | 196,2 / 167,0 |
| attente de chaque publieur | 0,94 à 1,18 s | | |

**Lecture.**
- À K5, P5 est critique de 15 à 23 ms après R. Son coût est l'effet de la **cohabitation** : 35 à 46 ms à W1
  (`claudediag1`), 81 à 99 ms au gel [V].
- À K10, R domine. Une fois R corrigé, deux nouveaux murs apparaîtraient : P10 (165 à 242 ms de CPU en cohabitation)
  et le préambule (61 à 78 ms) [I].

### 3.3 Plateaux presque singletons [V]

- **Ordre 5** : 438 011 clôtures pour 448 698 cellules régulières, soit 1,024 cellule par plateau. Parmi ces
  plateaux, 212 858 (48,6 %) sont des **continuations** sans nœud (`claudegpu3/prof_k5`).
- **Ordre 10** : 1 133 328 clôtures pour 1 138 258 cellules, dont 478 746 continuations, soit 42,2 %
  (`receipts/developpement_20261004/gpu_g4/sessions/claudegpu5/prof_k10/gpu_profile.json`).
- Le prototype D-F1 (§ 5.4) compte **au plus 4 événements binaires par rang** aux ordres 4, 5 et 10.

Une barrière par plateau est donc sans espoir. Une contraction après coup est, elle, trivialement parallèle.

### 3.4 Rigidité du Pool [V/I]

Une tâche par fil. À K10, 19 fils attendent environ 1 s, mais leurs CPU cumulés (environ 2,75 CPU·s) occupent tout de
même environ 2,2 fils matériels [V calcul]. Les 29 résolveurs couvrent déjà les 24 cœurs : passer à 48 résolveurs ne
rendrait qu'environ ×1,2 (gain SMT) [I]. Le conseil de l'audit (graphe de tâches, vol de travail) est juste, mais son
gain est plafonné.

---

## 4. Question ouverte n° 1 : pourquoi environ ×5 sur les descentes à K10 ? (livrable 3)

### 4.1 Travail logique, ordre 10, ng00, K10 : même objet, même nombre de graines

| Quantité | v10 [V]¹ | v11 [V]² | Rapport |
|---|---:|---:|---:|
| traces (graines) résolues | 3 831 401 | 3 831 491 | 1,00 |
| pas | 5 258 976 | 6 183 796 | **1,18** |
| pas par trace | 1,37 | 1,61 | 1,18 |
| MEB calculées | 1 966 948 | 2 352 191³ | **1,20** |
| présentations de supports MEB | (Welzl flottant, 0 repli) | **174 443 897** | — |
| présentations par MEB | — | **74,2** | — |
| census par l'index (ou l'arbre) | 947 896 | 1 158 367 | 1,22 |
| identifications au catalogue sans census | 1 051 920 | 1 193 881⁴ | 1,13 |
| arrêts sur mémo de cellule | 539 165 | 0 | — |
| réemplois verticaux / descentes verticales | 979 293 / 57 | 979 293 / 57 | 1,00 |

1. `morsehgp3D_v10/receipts/g4_session1_20260929/results/cmd/004_c0_lidar00_k10_w48/stdout`.
2. `receipts/developpement_20261004/gpu_g4/sessions/claudegpu5/prof_k10/gpu_profile.json` (`16b482169`, 4 octobre).
   Depuis, la MEB et la descente n'ont reçu que des changements sans effet sur ces compteurs : `257aabb92` (un
   `#include` de `meb.cpp`), `751686868` (somme sans branche des registres, « même vidage »), et V3 qui ne change que
   les tests de points du census (`git log 16b482169..ac081a06f`) [V].
3. Déduit des paires de diamètre : 105 850 647 = 45 × 2 352 191 + 36 × 57 (verticales à l'ordre 9).
4. `catalogue_hits − population_hits`.

À K5 (somme des ordres, ng00) :
- 4,80 M pas en v11 contre 4,40 M en v10 (×1,09) ;
- 1,175 M MEB contre 1,085 M (×1,08) ;
- 291 515 census contre 285 539 (×1,02).

Sources : `audit_deep_20261004/performance/derived_normal.json` et `004…/001_c0_lidar00_k5_w48/stdout` [V].

**Conclusion [V].** Le **travail compté** de la v11 n'excède celui de la v10 que de 2 à 22 %.

### 4.2 Décomposition du ×5

| Facteur | K5 | K10 | Source |
|---|---:|---:|---|
| pas (v11/v10) | 1,09 | ≈ 1,15 (26,2 M / 22,9 M)ᵃ | [V] ordres 1–5 et 10 ; [I] ordres 6–9 interpolés |
| temps de fil par pas à W48 (v11 / v10) | 642 / 312 ns = **2,06** | ≈ 1 343 / 509 ns = **2,65** | [V] CPU des voies, `t_resolve`×48 |
| fils (48 / L) | 1,23 | 1,66 | [V] |
| produit | 2,76 (mesuré : 82,8 / 28,6 = 2,9) | 5,06 (mesuré : 1 214 / 243 = 5,0) | |

ᵃ Ordres 6 à 9 estimés par le rapport v11/v10 interpolé entre 1,12 (ordre 5) et 1,18 (ordre 10).

**Nuance SMT [I].**
- À K10, 29 résolveurs occupent 24 cœurs presque seuls. Les 48 fils de la v10 partagent tous un cœur à deux.
- Le temps de fil par pas de la v11 est donc **flatteur** : en travail solo, l'écart par pas est plutôt de ×3,5 à ×4.
- Le facteur « fils » ne vaut que ×1,2 à ×1,3 en capacité physique.
- À W1, les cartes donnent déjà 596 à 704 ns par pas en v11 (avant V3) contre 188 ns en v10, soit ×3,2 à ×3,7
  (`notes_hors_depot_20261007/gpu_optim/carte_forets.md` § 2.3 ; `audit_transpositions/cartes/CARTE_V10_VITESSE.md`
  l. 62).

En résumé, **l'écart est un coût par pas de ×3 à ×4, multiplié par environ ×1,2 de pas et environ ×1,25 de
parallélisme effectif**.

### 4.3 Comparaison des descentes, côte à côte

| Poste | v10 (`tower.cpp`) [V] | v11 [V] |
|---|---|---|
| table d'arrêt | semis par ordre, avant **chaque** MEB (l. 820-833) : `FlatIndex` 8 o/case + populations k mots | `PopulationLookup` unique tous ordres, sondée à chaque pas (`population_lookup.cpp:275`) ; 8C + 4E(K+3) o (`.hpp:25-29`) |
| **MEB** | proposition `DWelzl` en double (l. 250-440 : paire éloignée puis pire point, ≤ 8 tours), certificat exact `verify_meb` (l. 468-514 : centre exact, aigu ou orientation filtrée, côtés filtrés à marge), repli Welzl exact ; **0 repli sur 7,66 M** ; niveau calculé à la demande (l. 517-526) | `bounded_meb` (`meb.cpp:157-175`) : C(k,2) distances exactes, puis **tous** les q3 puis q4 lexicographiques, avec construction exacte (Q3/Q4Candidate), positivité et inclusion exactes, puis matérialisation du `Level`. « Aucun … flottant » (`docs/MEB.md:45-47`). Jusqu'à 716 candidats |
| garde de décroissance | filtre double `r2a` à marge ; exact 7 fois sur toute la tour K10 (l. 838-844) | `num::compare` exact de `Level` à chaque pas |
| identification | `find_ball` du support certifié (l. 865-896), (p,q,m) dans `BallInfo`, une ligne de cache ; juge de recensement sur 1 boule sur 32 | `find_support` du support local canonique (`locate.cpp:79-84`) |
| census hors catalogue | `SiteTree::closed_ball` k-d à filtre flottant, **boule entière** (moyenne 16,4 sites, maximum 1 258 à K10 ; L06) | index radix, bornes **exactes** sur sites entiers (V3), arrêt au seuil k (`census_workspace.cpp:47-63`) ; puis `global_support` exact (`canonical.cpp`) |
| saut si p ≥ k | k plus proches en double à marge, exact sinon (l. 905-934) | k premiers intérieurs de Morton (`descent.cpp:138-142`) |
| mémo | par cellule (b,k), daté par λ_b, atomique *relaxed* (l. 951-973, 991) | aucun dans les modes de référence |
| comptabilité | compteurs u64 simples par fil | `DescentStep` et `DescentResult` portent le registre complet (37 registres), `add_descent` transactionnelle à chaque pas (`descent.cpp:62-69`, 190) |
| parallélisme | `parallel_for` à grain 32 sur 48 fils, lots de 32 avec empreintes et préchargement (l. 1441-1494) | 29 ou 39 voies, blocs de 256, préchargement en trois étages (`forest_parallel.cpp:147-170`) |

### 4.4 Hypothèse la mieux étayée, et hiérarchie des causes

**(1) La MEB exacte exhaustive, cause principale à K10** [V pour les comptes, I pour la part de temps].

Présentations par MEB, ordres 2 à 5 (`developpement_20261003/ecart_v10_v11/records.json`, mode 16379) et ordre 10
[V] :

| Ordre | 2 | 3 | 4 | 5 | 10 |
|---|---:|---:|---:|---:|---:|
| présentations par MEB | 1,00 | 1,33 | 2,36 | 4,64 | 74,2 |

- Elles suivent C(k,3) + C(k,4).
- J'estime environ 346 M présentations sur toute la tour K10, contre 3,79 M à K5, soit ×91, alors que les pas
  croissent de ×5,5 [I : ordres 6 à 9 interpolés à 22–29 % du maximum, comme aux ordres 5 et 10].
- Même dans la v10, la part de la MEB passe de 28,5 % à 37,4 % des cycles de `resolve` entre K5 et K10, avec un
  Welzl en O(k) (`build/v11-persist/audit_v10/L06_CODE_TOUR.md` l. 400-411, profil `rdtsc`) [V].
- À K5, après V3, la MEB fait encore 25 % des instructions de la résolution (`v3_census/README.md` l. 94) [V].

**(2) Le census exact sur l'index** [V/I].
- V3 a divisé par 3 les appels aux bornes de boîte (44,6 M → 14,5 M, ng00 K5). Pourtant ces bornes font encore
  29 % des instructions de la résolution à K5, autant que la MEB, car les décisions de nœud restent entières
  (`LatticeSphere`) (`v3_census/README.md` l. 70–94) [V].
- À l'ordre 10 : 1,16 M census et 127,9 M tests de points avant V3, soit 110 par census.
- La conception D-I2 prévoyait un élagage par borne flottante certifiée à sens unique, avec décisions exactes. Il
  n'a pas été fait.

**(3) Surcoûts fixes par pas** [I] :
- comparaison exacte de `Level` (jusqu'à 180–204 bits pour q3/q4) ;
- `Point::make` contrôlés ;
- copies de registres de 37 champs ;
- sonde de table à chaque pas : à K10, la table fait environ 362 Mo [I], contre 44,2 Mo à K5 [V], d'où des défauts
  de cache DRAM ;
- le levier de constantes a déjà retiré 9,4 % des instructions, pour ×0,94–0,975 à K10 (`constantes_pas_descente`)
  [V].

**(4) Le nombre de pas** (+18 %) : explicable par l'absence de mémo (7 % et 9 % des pas de la v10 finissent sur le
mémo [V]) et par la politique « premiers de Morton ». Le mutant `JUMP_ANY` de la v10 mesure **+3 % de MEB et +7 % de
sauts**, dump identique (L06 l. 319) [V]. La v10 est elle-même à 7–8 % du minimum de pas
(`PISTES_DE_RUPTURE.md` § 0.3) [V].

**(5) Moins de voies** : ×1,2 à ×1,3 en cœurs [I].

**À écarter.** La sous-maille T6 : c'est un paramètre des boîtes du **catalogue** v10 (« u18, T = 6 bits
sous-unitaires pour les boîtes », `CARTE_V10_VITESSE.md` § 1.1). Une ablation T = 6/3/0 donne ±0,3 % de travail et
des vidages identiques (`AUDIT_TRANSPOSITIONS_V11.md` l. 503) [V]. Elle n'entre pas dans les descentes.

### 4.5 La mesure qui tranche

Session G4 gardée unique, trames ng00–ng02, K5 et K10.

1. **W1**, même binaire Release que la référence :
   - v11 `ac081a06f` et v10 R2 figée (`build/v10-integration-r2`) ;
   - temps de résolution par ordre et histogramme des pas par trace ;
   - pour la v11, un profil `rdtsc` par segment calqué sur L06 (table, MEB en diamètre / q2 / q3 / q4, `find_support`,
     census en nœuds / bornes / points, `global_support`, `strict_trace`, comparaison de `Level`, registres), avec
     échantillonnage 1/64 comme le profil des publieurs (`0ff64512a`).
2. **Variante v11 « MEB proposée et certifiée »**, derrière un bit inactif par défaut :
   - port de `DWelzl` (proposition, ne décide rien) ;
   - certificat T1 : si le support proposé S est le $S^*$ d'une boule b et $F\subseteq P_b$, alors B(F) = b, par
     simple comparaison d'identifiants ;
   - sinon certificat exact du support proposé (centre exact, signes barycentriques, côtés, filtres F6), puis
     canonisation **parmi les seuls points de F sur la sphère**, et repli Welzl exact ;
   - vidages `MHGP11FUL1` **identiques** exigés : la boule identifiée est unique, donc la descente aussi.
3. **Règle écrite d'avance** : si la variante réduit le CPU de résolution K10 de 40 % ou plus à W1, la cause (1)
   est confirmée. Sinon le profil désigne le census ou les surcoûts fixes.

Coût estimé : une session de moins de 2 h.

---

## 5. La forêt comme arbre couvrant minimal parallèle (V4/O7) (livrable 4)

### 5.1 Le lemme de l'auditeur [V]

Source : `receipts/audit_plan_gpu_20261006/mathematics/REPORT.md` l. 7-32.

**Graphe.** Ordre k fixé :
- sommets : les naissances, avec leur rang ;
- arêtes : pour chaque cellule, l'étoile de ses graines (graine₀, graineⱼ), au rang de la cellule. Les graines ont
  une date initiale strictement inférieure au rang de la cellule.

**Énoncé.**
- Pour tout rang λ, tout MST a les mêmes composantes que le graphe restreint aux arêtes de rang inférieur ou égal à
  λ. C'est la propriété de cycle. Les étoiles remplacent exactement les hyperarêtes.
- Kruskal produit des fusions binaires au même rang. Contracter les chaînes parent/enfant de **nœuds de fusion de
  même rang** rend exactement les fusions du plateau atomique.
- On garde les naissances, on supprime les continuations, puis on renumérote par (rang, plus petite naissance) et on
  trie les enfants : on obtient la même forêt canonique.
- Contracter entre rangs distincts change la filtration.
- Le lemme ne s'applique ni à un catalogue incomplet, ni à un mélange d'ordres.

### 5.2 État au registre (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`) [V]

| Ligne | Énoncé | Statut |
|---|---|---|
| 243 | contraction des plateaux par composantes fortement connexes | `proof_obligation` |
| 244 | traitement séquentiel de niveaux égaux | `false_in_general` (mutant `forest_plateau_sequentiel`) |
| 237 | Borůvka sur l'expansion étoilée canonique, puis base de selles, préserve toutes les partitions de seuil (strictes et fermées), avec R ≤ ⌈log₂ B⌉ rondes | `proved_here` |
| 238 | contracter toute l'hyperarête d'une selle parce qu'une racine l'a choisie | `false_in_general`, fixture `relative_boruvka_p0_whole_hyperedge_contraction` |
| 245 | hyper-Kruskal par lot préserve les composantes | `proved_here` |
| 242 | le pointer-jumping préserve la racine | `proved_here` |

**L'audit final dit seulement que le lemme de l'auditeur n'est pas inscrit.** C'est vrai. Il omet que les lignes 237,
238 et 245, issues d'autres chantiers, en sont le cadre direct : un hypergraphe de Morse pondéré, dont les terminaux de
selle sont nos graines. La ligne 238 est **le piège** d'un Borůvka naïf sur hyperarêtes.

Il omet aussi une **preuve candidate de la ligne 243**. Le théorème T4 (`CONCEPTION_TOUR.md` annexe A.4, l. 740-746)
démontre que, pour tout ordre interne à un rang et toute règle d'union, les classes d'événements binaires « liés » de
même rang sont exactement les multifusions. Il a été contrôlé sur 6 000 hypergraphes aléatoires à égalités
(`preuves_tour/foret_check.py`) et sur six ordres réels (forêts identiques à la v10, `preuves_tour/noyau_v11.log`)
[V].

### 5.3 Contre-exemple des compteurs [V]

Témoin K1 : A = (0,0), B = (3,0), C = (1,2). Niveaux AC = 5/4, BC = 2, AB = 9/4.
- Modèle complet : `unions=2, touched_components=5, continuations=1, plateaus=3`.
- MST seul : `2, 4, 0, 2`. Le MST supprime AB, qui ne touche qu'une composante déjà fusionnée.

Conséquences :
- `ancestor_activations` (nombre de fusions de rang ≤ μ) et `ancestor_unions` (somme des arités) se calculent sans
  simuler de DSU ;
- `ancestor_find_steps` dépend du chemin parcouru.

Les registres `plateaus`, `continuations` et `touched_components` sont des propriétés de la **liste des cellules**. Il
faut les calculer par une passe parallèle sur les cellules, une fois la forêt close. Par exemple, `touched` = nombre
d'ancêtres distincts des graines à la coupe fermée de rang − 1, obtenu par requêtes d'ancêtre [I]. Ou bien on les
déclare physiques.

### 5.4 Précédents : plus faibles que ne le dit l'audit [V]

- **« Borůvka v9 à 1 264,7 ms (K5, ng00) »** :
  - c'est une mesure **locale** (« M-loc ») d'un constructeur par étiquette minimale (`fouille/v9.md` l. 288) ;
  - le parallélisme intra-plateau y est rejeté à « 1,05 bloc par lot » ;
  - ce n'est pas un Borůvka sur l'expansion étoilée.
- **« 31–45 % des fusions à la couture »** :
  - c'est un Kruskal local par **blocs spatiaux de Morton** (≈ 52 blocs de 880 sites), puis une couture
    séquentielle (`PISTES_DE_RUPTURE.md` § 3.7) ;
  - c'est une autre méthode, et elle borne le gain à ×2,2–3,2 sur un noyau déjà petit.
- **Précédent positif omis** : le noyau D-F1 (§ 4.1 et 4.5 de `CONCEPTION_TOUR`). Union par taille, événements
  binaires de 20 octets, aucun nœud écrit. Mesuré localement sur les entrées réelles du Kruskal v10 de ng00 :

| | ordre 5 | ordre 10 |
|---|---:|---:|
| noyau D-F1 | 18,2 ms | 54,2 ms |
| Kruskal par lots v10 | 47,6 ms | 140,9 ms |
| rapport | ×2,61 | ×2,60 |

Forêts et numérotations identiques, 2 444 942 images de jonctions concordantes. Estimation G4 : 8 à 12 ms (K5) et
20 à 35 ms (K10) pour le noyau séquentiel, plus 1,5 à 3 ms de matérialisation parallèle [V pour le local, I pour
G4].

### 5.5 Ce qu'il faudrait pour une forêt parallèle viable et déterministe [I]

1. **Ordre total des arêtes** (rang de la cellule, `BallIdx`, indice de graine). Le MST est alors unique : Kruskal à
   réservations déterministes et Borůvka à départage canonique rendent le même. Respecter la ligne 238 : contracter
   le **lien choisi**, jamais toute l'étoile.
2. **Dendrogramme** : Kruskal séquentiel sur les V−1 arêtes du MST (341 k à l'ordre 5 au lieu de 1,35 M graines), ou
   dendrogramme parallèle. Puis contraction des chaînes de même rang (T4) et renumérotation par tri parallèle
   (rang, plus petite naissance).
3. **Contrat des compteurs** écrit avant le port (logiques par passe sur les cellules, ou physiques).
4. **Registre** : inscrire le lemme MST/contraction et T4 avec leurs fixtures, puis clore la ligne 243. Fixtures :
   ternaire (0,2,4), diamant K2, E5, triangle des compteurs, contraction d'hyperarête entière.
5. **Microbanc** sur graines vidées : ≤ 10 ms à K5 et ≤ 50 ms à K10 (transferts compris si GPU), identité à l'octet
   avec la section `MHGP11FUL1`.
6. **Pertinence** : seulement si R descend sous le temps du noyau D-F1. Sinon le noyau séquentiel recouvert suffit.

---

## 6. Vérification de l'audit final § 7 et du rapport C (livrable 5)

### 6.1 Confirmés [V]

- **Algorithme** (§ 7.1) : étapes A–E, garde de date, plateau atomique, numérotation, contrôles, portes et
  163 mutants.
- **Chronologie de l'étage**, recoupée reçu par reçu :
  - memo1 : FULL ×1,282–1,322 ;
  - forest3 : 14,79 → 6,16 s ;
  - census2 : 750,5 ms ;
  - reuse1 : 658,0 ms ;
  - c40 : 240,8 / 163,6 / 197,2 ms ;
  - tranche 3 : 209 → 171,0 / 133,0 / 157,7 ms, et −25,8 / −29,2 / −30,0 % à W1 ;
  - V3 : 0,925 ;
  - constantes : 0,966 ;
  - O2a : 0,904 ;
  - cohortes : 0,529 et 0,942 ;
  - claudeg1 : 117,2 / 94,6 / 115,9 ms et 1 294 / 958 / 1 069 ms.
- **Leviers retirés** :
  - annonces : 0,966 pour un seuil de 0,90 ;
  - préchargement des graines : 0,865 pour 0,85 ;
  - préchargements combinés : 0,950 pour 0,90, clôtures 31,9 → 31,3 ms ;
  - THP : 1,064, et 100 → 126 ms sur ng00 ;
  - O1 : A/A involontaire à ±12 % / ±9 %, puis critère manqué (1,038), puis adoption (0,917 ; pire 0,967).
- **Divers** :
  - plateaux presque singletons ;
  - 19 tâches en attente d'environ 1 s à K10 ;
  - triangle des compteurs ;
  - date initiale et témoins ;
  - défaut `ForestProgress::finish` corrigé en `3bd4d734e`.

### 6.2 À corriger

| Où | Affirmation | Correction |
|---|---|---|
| Audit § 3.2 et § 14.2, causes [I] du ×5 | k plus proches, mémo V7, partition T6, 29 voies | T6 est hors sujet (catalogue). Les k plus proches valent +3 % de MEB (L06 `JUMP_ANY`). Le mémo vaut −7/−9 % de pas. Les voies valent ×1,2–1,3 en cœurs. La cause principale manque : le coût du pas (×3–4), avec la MEB exhaustive exacte en tête (§ 4) |
| PASSATION § 3, « à porter de la v10 » | k plus proches, mémo cellulaire daté, T6, J3, M(K) | Il manque **la MEB proposée en flottant puis certifiée** (`DWelzl` + `verify_meb`, version R2 gardée FE), de loin la plus importante pour la tour. Le mémo est à porter sous la forme **déterministe** T3 (pointeurs de cellule), pas sous la forme atomique *relaxed* de la v10, dont la graine dépend de la course des fils (`CONCEPTION_TOUR` § 3.2) |
| Audit § 7.1 et rapport C | placement « sur des cœurs physiques dédiés » | premier fil d'un cœur dédié ; le frère SMT héberge les tâches légères ; résolveurs sur les 20 autres cœurs |
| Audit § 7.4 et rapport C § 4 | P5 « encore 95–99 ms après O1/O2 » | session O2 : 78,4–94,9 ms (`o2_publieur` l. 60–65) ; au gel : 98,9 / 80,7 / 94,6 ms, soit 81–99 ms |
| Rapport C § 5, audit § 7.1 | « préambule ~13–16 ms » | 14,9 / 12,8 / 16,3–18,4 ms (ng02 au-delà de 16) |
| Audit § 7.5 et rapport C § 5 | « chaque publieur balaie les 1,31 M fiches de boules » | il balaie le tableau `kinds` (1,31 M octets) et ne lit la fiche de boule que pour `kinds==2` (`forest_concurrent.cpp:47-58`). Le flux compact garde son intérêt, mais moindre |
| Audit § 7.3 et rapport C § 3 | « précédents CPU négatifs : Borůvka v9 1 264,7 ms ; couture 31–45 % » | mesure **locale** d'un constructeur min-label, et découpage **spatial**. Ni l'un ni l'autre ne teste un MST sur l'expansion étoilée. Le précédent positif D-F1 (×2,6) est omis |

### 6.3 Omissions

1. **`CONCEPTION_TOUR.md` et `preuves_tour/`** (2 octobre, 01e217f0…, `noyau_v11.cpp` 0d712a53…) : la conception
   d'origine de la tour v11.
   - Décisions D-G1/G3/G4/G5, D-I2, D-F1/F2/F3, D-V1, lemmes T1, T3–T7, budget estimé de 22 à 32 ms à K5.
   - **Divergence entre conception et implantation** non documentée : MEB exhaustive au lieu de T1, Morton au lieu de
     k plus proches, publieur à plateaux au lieu du noyau puis contraction, balayage DSU au lieu de l'historique
     d'attache.
   - **Non versionnée** : le reçu `notes_hors_depot_20261007` n'en a repris que `PISTES_DE_RUPTURE.md`.
2. **Préambule K10** de 61 à 78 ms (§ 3.2). La table de populations est d'environ 362 Mo à K10 [I].
   - Formule (`population_lookup.hpp:25-29`) : 4,37 M entrées, égales à la somme des naissances des ordres 2 à 10.
     À K5, 857 891 = somme des naissances des ordres 2 à 5, ce qui recoupe exactement les 44,2 Mo [V].
   - Lignes : 4,37 M × 13 × 4 octets = 227,5 Mo. Cases : 2²⁴ × 8 octets = 134,2 Mo.
   - Elle est reconstruite à chaque appel.
3. **Le mémo de descente sériel** (clé exacte de la partie) avait **41,5 % de succès d'origine et 13,8 % de suffixes**
   (memo1, ng00). Les 2,6 % ne concernent que les mémos de **lane**, à cause de la répartition circulaire. La leçon
   porte sur la distribution, pas sur la mémorisation.
4. **Les compteurs v11 de l'ordre 10** sont au dépôt (`claudegpu5/prof_k10`), mais aucun reçu ne les compare à la
   v10. C'est pourtant la comparaison qui localise l'écart (§ 4.1).

---

## 7. Recommandations pour la tour de la v12 (≤ 40–50 ms à K5) (livrable 6)

### 7.1 À porter d'emblée

**De la v10 (`tower.cpp`), requalifiés :**
1. **MEB proposée en flottant, puis certifiée** (D-G3) :
   - certificat T1 sans arithmétique, quand le support proposé est un $S^*$ et que $F\subseteq P_b$. Il couvrirait
     75 % des pas MEB à K5 et 51 % à l'ordre 10 [V : `catalogue_hits − population_hits` sur `census` +
     identifications] ;
   - sinon certificat exact du support proposé, avec filtres F6 sans marges u18 figées (PO-I1, constat R2) ;
   - repli Welzl exact, puis canonisation parmi $F\cap\partial B$.

   F1 est respecté : la proposition ne décide rien. Les compteurs qui dépendent de la proposition (route T1 ou route
   exacte) sont **physiques**, ce qu'il faut déclarer dans le contrat des compteurs.
2. **Garde de décroissance filtrée** : clés F3/F4, exact seulement en cas d'ambiguïté.
3. **Mémo de cellule daté, sous forme déterministe** (D-G1 et lemme T3) : arrêt sur la première cellule de fenêtre non
   naissance, cible suivie après coup par pointeurs strictement descendants. Pas d'atomique partagé, graines
   identiques à tout W.
4. **Saut vers les k plus proches** : utile surtout pour borner le census (D-G4). Sa valeur en pas reste mince
   (+3 % de MEB).

**De la v11 :**
- domaine immuable ;
- descentes sans DSU ;
- table de populations : la rendre **compacte** (cases à étiquette, vérification contre la CSR du catalogue, sans
  recopier les lignes) et la **produire par le catalogue**, ou la garder dans la Session ;
- T4/T5, numérotation canonique, contrôles de naturalité ;
- réemploi vertical régulier ;
- cohortes par tranches ;
- index radix V3 ;
- portes `pipeline_*` ;
- mutants `forest_plateau_sequentiel`, `pipeline_abandon_apres_reveil`, `placement_perdu_au_deplacement`.

**De `CONCEPTION_TOUR` (à versionner d'abord) :**
- **D-F1** : noyau union-find par taille et événements binaires, un fil propriétaire par ordre, recouvert par R. Les
  propriétaires aident à résoudre quand ils attendent (D-F2) ;
- **matérialisation par contraction parallèle T4** ;
- **requêtes d'ancêtre par historique d'attache** (D-F3, au plus log₂ naissances) ;
- **verticales D-V1** : image d'une naissance en O(1) par le sommet de la jonction de la même boule, une requête par
  fusion, naturalité aux portes.

On supprime ainsi le balayage suivi et sa chorégraphie futex.

### 7.2 À ne pas reprendre

- La boule fermée entière de la v10 : Θ(n) au pire, 1 258 sites mesurés.
- Le juge 1/32 et la seconde recherche de support dans le chemin produit (D-G5).
- Le Kruskal par lots et les pointeurs de saut séquentiels de la v10.
- Les 37 registres transactionnels **par pas** de la v11 : cumuler en registres locaux par voie, avec une garde de
  débordement unique en fin de voie.
- L'énumération C(k,3) + C(k,4) dans le chemin produit : la garder pour l'oracle borné.

### 7.3 Budget K5 visé (ng00, W48) [I]

| Étage | v11 au gel | v12 visé | Moyens |
|---|---:|---:|---|
| préambule (table, classification, naissances, liaison) | 13–18 ms | ≤ 5 ms | table produite par le catalogue ou résidente ; classes de cellules émises par le catalogue |
| R (résolution) | 63–83 ms | ≤ 25–30 ms | coût du pas ÷2,5–3 (D-G3, filtres, registres légers, D-I2) ; −10 % de pas (T3) |
| noyau de l'ordre K | P5 : 81–99 ms en cohabitation | 8–12 ms, recouvert | D-F1 ; placement par CCD |
| contraction et verticales | V5 collé à P5 | ≤ 5 ms | T4 et D-V1 parallèles |
| **total forêts** | **91–116 ms** | **≈ 35–45 ms** | |

À K10, la meilleure estimation est de 200 à 300 ms : R d'environ 150 à 250 ms, noyau de 20 à 35 ms, préambule
inférieur ou égal à 20 ms. Le contrat de 100 ms à K10 reste hors de portée de la tour CPU, comme le disait déjà
`CONCEPTION_TOUR` § 10.

### 7.4 Ordre des mesures, avant tout port

1. **Profil par route à W1**, v11 contre v10 R2, même session, K5 et K10 (§ 4.5), avec l'histogramme des pas et des
   présentations par ordre.
2. **A/B « MEB proposée et certifiée »** dans la v11 gelée, à vidage identique. Si le gain est de 40 % ou plus à K10,
   D-G3 est le levier n° 1.
3. **Microbanc du noyau D-F1** sur graines vidées (naissances, rangs, graines par cellule), à W1 puis sous 47 fils de
   bruit qui imitent les résolveurs. On mesure l'inflation de cohabitation avec `perf stat` LLC/SMT par fil, ce qui
   répond à la question ouverte n° 1 de `carte_forets.md` § 6. Seuils : noyau ≤ 10 ms (K5) et ≤ 35 ms (K10) à W1 sur
   G4 ; contraction ≤ 3 ms.
4. **Coût et forme de la table de populations à K10** : construction, mémoire, défauts DRAM par sonde ; table
   compacte contre table actuelle.
5. **Seulement ensuite**, si R descend sous le noyau : microbanc du MST parallèle (§ 5.5).

### 7.5 Hygiène

Versionner `CONCEPTION_TOUR.md`, `A_CONTRE_LIRE_TOUR_20261002.md`, `preuves_tour/` et `audit_v10/L06_CODE_TOUR.md`
avec ses preuves dans un reçu, avant la perte de `build/v11-persist`. Ces pièces contiennent les seules mesures
par segment de la v10 et la seule preuve écrite de T4.

---

## 8. Sources principales

**Code v11**
- `src/tower/` : `forest_parallel.cpp`, `population_lookup.{hpp,cpp}`, `descent.cpp`, `locate.cpp`, `meb.cpp`,
  `canonical.cpp`, `forest_build.cpp`, `forest_plateau.cpp`, `forest_concurrent.cpp`, `forest_pipeline.cpp`,
  `forest_internal.hpp`, `forest_vertical.cpp`, `forest_vertical_seed.hpp`, `forest_ancestor_sweep.hpp`,
  `regular_vertical_seeds.hpp`, `forest_placement.{hpp,cpp}` ;
- `src/index/census_workspace.cpp`, `src/sched/pool.cpp`.

**Code v10** : `morsehgp3D_v10/src/tower/tower.cpp` ; R2 : `build/v10-integration-r2/src/morsehgp3D_v10/src/tower/tower.cpp`
(mêmes algorithmes, filtres gardés par FE et refus de fast-math).

**Reçus**
- v11 :
  - `developpement_20261007/filtre_g1_avx2/claudeg1/gpu_ab_report_ab_{k5_16_cpu,k10_24_gpu}.json` ;
  - `developpement_20261004/gpu_g4/sessions/claudegpu5/prof_k10/gpu_profile.json` ;
  - `developpement_20261003/ecart_v10_v11/records.json` ;
  - `audit_deep_20261004/performance/{derived_normal.json,context/TABLE_VERITE_G4.md}` ;
  - `developpement_20261006/{v3_census,constantes_pas_descente,o1_placement,o2_publieur}` ;
  - `developpement_20261007/{cohortes_tranches,annonces_publieurs,prechargement_graines,prechargements_publieurs,cache_blocs,thp_exploration}` ;
  - `full_memo_20261003/memo1`, `full_parallel_20261003/forest3`, `full_census_20261003/census2`,
    `full_regular_vertical_20261003/reuse1`, `developpement_20261003/pipeline_g4` ;
  - `audit_plan_gpu_20261006/mathematics/REPORT.md` ;
- v10 : `morsehgp3D_v10/receipts/g4_session1_20260929/results/cmd/00{1..6}_*/stdout`.

**Notes**
- `receipts/notes_hors_depot_20261007/{gpu_optim/carte_forets.md,conception/PISTES_DE_RUPTURE.md,audit_transpositions/…}` ;
- hors dépôt : `build/v11-persist/conception/CONCEPTION_TOUR.md` et `preuves_tour/`,
  `build/v11-persist/audit_v10/L06_CODE_TOUR.md`.

**Registre** : `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` l. 237–245.
