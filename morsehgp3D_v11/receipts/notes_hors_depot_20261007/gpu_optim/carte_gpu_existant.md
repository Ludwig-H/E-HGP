# Carte de la voie GPU existante des feuilles du catalogue (v11)

Rédigé le 6 octobre 2026, vers 01 h 25 UTC (`date -u`). Lecture seule, sources lues sur `origin/main` = `df904711a`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) / cuda_g4 (voie mesurée)
profile=quantized_u21_input_only (reçus gpu_g4)
public_status=not_claimed
GCP non utilisé
```

Sources : `src/catalogue/leaf_device.hpp`, `leaf_batch.hpp`, `leaf_batch_cuda.cu`, `leaf_batch_cuda_context.cu`,
`single_pass_batch.cpp`, `single_pass.cpp`, `leaf.cpp`, `center_line_cache.hpp`, `docs/CATALOGUE.md` (section « Voie
GPU des feuilles »), `bench/gpu_ab.py`, `bench/gpu_profile.py`, reçu `receipts/developpement_20261004/gpu_g4` (rapports
JSON des sessions 3 à 6, `nsys_stats.csv`, `ncu_details.csv`, `gpu_profile.json`), audit
`receipts/audit_gpu6_receipt_20261004`, `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (§ D à F),
`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (réponses D et F),
`receipts/developpement_20261005/qualification_finale` (mesure `claudefinmesure`).

Depuis la session `claudegpu6` (`22a6af6aa`), seul `57dd21be1` a touché `src/catalogue` (observabilité). Le dump K = 5
de ng00 de la qualification finale (`3a2bfb4f…`) est celui des sessions GPU : **les mesures GPU du 4 octobre
s'appliquent au code courant**.

Légende : **[G4]** = mesure G4 d'un reçu ; **[dérivé G4]** = différence ou rapport de mesures G4 ; **[estimation]** =
raisonnement, jamais une mesure.

## 1. Ce que fait la voie aujourd'hui

1. La passe unique CPU (48 fils) parcourt l'arbre et **met en file** les feuilles (graphe de paires, m ≤ 32) au lieu de
   les jouer (`TaskLeafQueue`).
2. **Après la fin du parcours**, `process_leaf_batch` rassemble les files, puis appelle `run_leaf_batch_cuda` :
   attente du contexte, allocations (`cudaMallocAsync`, pool gardé), envoi en mémoire pageable, `count_kernel` (un fil
   par feuille, blocs d'un warp, feuilles triées par m décroissant, case de 2 Kio par feuille), préfixes CUB,
   `copy_kernel`, sélection CUB des feuilles qui débordent, `fill_kernel` (rejeu entier de ces feuilles, un fil
   chacune), retour vers des pages pré-touchées.
3. L'hôte calcule les Level (`Sphere::through`), rejoue par `leaf.cpp` les feuilles `unresolved` (zéro sur les trois
   trames), puis rend le bloc à l'assemblage.

Le parcours CPU et l'exécuteur GPU sont **strictement en série** : le GPU est inactif pendant le parcours, les 48 fils
CPU attendent pendant l'exécuteur. La voie n'existe que dans la sonde `full_probe` (bit 65536) ; ni `src/api` ni la
CLI ne l'exposent, et la qualification finale ne la couvre pas.

## 2. Faits mesurés

### 2.1 Mur et étages, ng00, K = 5, feuille 16 (session 6, `22a6af6aa`)

| Grandeur | CPU | GPU | Source |
| --- | ---: | ---: | --- |
| mur FULL, médiane chaude passes 2..P | 346,8 ms | 419,6 ms | [G4] audit gpu6, DERIVED |
| domain (passe chaude typique) | 206–210 ms | 243–246 ms | [G4] `gpu_k5_report.json` warm |
| single_pass (parcours, + feuilles en CPU) | 150,2–150,9 ms | 108,9–112,8 ms | [G4] idem |
| exécuteur GPU (chaud) | — | 58,3–59,0 ms | [G4] idem |
| dont `count_kernel` | — | 35,4–35,9 ms (nsys : 35,43 / 35,53 / 35,60 ms) | [G4] report + `nsys_stats.csv` |
| dont écriture (`fill_ns`) | — | 17,5–17,6 ms (nsys `fill_kernel` : 17,20 ms) | [G4] idem |
| dont `copy_kernel` | — | 0,27 ms | [G4] nsys |
| envoi / retour | — | ≈ 1,9 ms / 2,3 ms | [G4] froid, médianes |
| Level sur l'hôte | — | 5,4–5,9 ms | [G4] warm |
| rassemblement des files | — | 2,1–2,5 ms | [G4] froid |
| mémoire device cumulée | — | 798 Mo | [G4] `device_bytes` |

Coût des feuilles en CPU (single_pass CPU moins single_pass GPU, passes chaudes, S6) : **≈ 39 ms (ng00), ≈ 26 ms
(ng01), ≈ 37 ms (ng02)** [dérivé G4]. Ce que la voie GPU ajoute en série : exécuteur 51–59 ms + Level 4,4–5,9 ms +
rassemblement 2,1–2,5 ms + environ 9 ms de reste de domain (domain GPU − single_pass GPU = 133 ms, contre 57 ms en CPU)
[dérivé G4]. **Bilan net K = 5 : +30 à +50 ms pour le GPU** (perte), à froid comme à chaud.

### 2.2 K = 10, feuille 24 (session 6)

CPU : single_pass 633–666 ms, domain 823–860 ms. GPU : single_pass 151–156 ms, exécuteur 342–344 ms (count 216–218 ms,
fill 112–113 ms), Level 21,7–22,7 ms, domain 739–746 ms [G4, passes chaudes ng00]. Feuilles en CPU ≈ 486 ms
[dérivé G4]. Gain GPU de 2 à 6 % sur le mur, qui reste dominé par les forêts (1,17–1,66 s) [G4].

### 2.3 Nsight Compute, `count_kernel`, K = 5, ng00 (S6, horloge verrouillée 1,85 GHz)

43,77 ms ; 168 registres → 25 % d'occupation théorique, 21,25 % atteinte (10,2 warps/SM) ; **3,29 fils actifs sur 32**
(3,14 non prédiqués) ; IPC 1,32 ; créneaux d'émission 33 % ; 7,72 cycles par instruction, dont **40,5 % d'attente sur
une dépendance L1TEX** (mémoire locale ou globale) ; 0,45 warp éligible par ordonnanceur ; pile locale de 3 296 octets
par fil, **2,4 octets utiles sur 32** par secteur en lecture locale et 2,2 en écriture ; 14 % de secteurs globaux en
trop ; 4 vagues pleines et une vague partielle de 2 023 blocs ; DRAM 1,2 %, L2 99 % de réussite. À K = 10 :
266 ms, 3,18 fils actifs, même profil [G4 `ncu_details.csv`, `gpu_profile.json`]. Les lignes chaudes (S3) sont la
barrière, désormais retirée, `center_line_meets` (≈ 20 % des échantillons d'attente) et le cache J2 simulé (5 à 7 %)
[G4, README du reçu].

### 2.4 `fill_kernel` : la queue est sérialisée **dans un seul warp**

- K = 5, S6 : 14 feuilles rejouées, **grille d'un bloc de 32 fils**, 17,2 ms (nsys), occupation atteinte 2,1 %, IPC
  0,21 ; un seul SM sur 188 travaille [G4].
- K = 10, S6 : 4 196 feuilles, 132 blocs de 32, 136 ms sous ncu (112 ms en mur), occupation 2,08 %, un warp par SM
  actif [G4].

Dans un warp, des fils divergents s'exécutent à tour de rôle. Les 14 feuilles lourdes de K = 5 coûtent donc à peu près
la **somme** de leurs chemins, et non le maximum : la feuille la plus lourde prend entre 17,2/14 ≈ 1,2 ms et 17,2 ms.
Les reçus ne permettent pas de trancher, faute de chrono par feuille.

### 2.5 Le noyau de feuille fait plus de travail que `leaf.cpp`

- Le cache J2 du device est **simulé** : `lines_possible` calcule `center_line_meets` à chaque demande et ne garde que
  le bit « déjà vu » pour les compteurs (`leaf_device.hpp`, `seen`). `leaf.cpp` mémorise la relation
  (`CenterLineCache::lookup`). Les droites calculées par le GPU sont les demandes : **75,0 M** contre 31,3 M en CPU à
  K = 5 (×2,4), **498 M** contre 146 M à K = 10 (×3,4) (registres ng00, champs `region_line_tests` et
  `region_line_evaluations`) [G4, registres].
- Avec le même code `leaf_device` sur les 48 fils de l'hôte (mode `lot`), le comptage prend 115 ms, contre 32–35 ms sur
  le GPU et ≈ 39 ms pour toutes les feuilles en `leaf.cpp` [G4 S4/S5, dérivé]. Le GPU exécute donc ce code ≈ 3,4 fois
  plus vite que 48 cœurs, mais ce code est ≈ 3 fois plus lourd que `leaf.cpp`.

### 2.6 Contexte, chargement, transferts

- Le fil d'ouverture anticipée met **136 à 183 ms** à froid (`prefetch_ns`, S6). Dans le profil nsys de S6,
  `cudaFree(0)` ne prend que 69,4 ms (78 ms en S3) : l'ouverture est **ralentie d'un facteur ≈ 2 quand elle concurrence
  les 48 fils du parcours** [G4 ; la cause est une hypothèse].
- Attente résiduelle à froid (`device_init_ns`, médianes S6) : 32 ms (ng00), 56 ms (ng01), 13 ms (ng02). La première
  prise après démarrage de la VM a attendu 237 ms. À chaud, 0 [G4].
- Le premier lancement de noyau coûte 2,95 ms (chargement paresseux du module, `cuModuleGetLoadingMode`). Les lancements
  suivants coûtent quelques µs [G4 nsys].
- Envoi en mémoire **pageable** de 44 Mo par passe (sites 19,7 Mo, `LeafJob` 22,6 Mo) à 23–30 Go/s. Retour de 22,5 Mo à
  27–37 Go/s vers des pages pré-touchées. CUB : moins de 20 µs [G4 nsys].

### 2.7 La taille de feuille déplace l'optimum (session 4, cases de 32, `b74f9ea3a`)

| K, feuille | single_pass GPU (parcours seul) | count GPU | fill GPU | domain CPU |
| --- | ---: | ---: | ---: | ---: |
| 5, 16 | 94–110 ms | 28–34 ms | 18–19 ms | 183–237 ms |
| 5, 24 | **29–38 ms** | 67–69 ms | 69–78 ms | 209–253 ms |
| 10, 24 | 118–137 ms | — | — | 687–879 ms |
| 10, 32 | **47–57 ms** | 234–279 ms | 241–260 ms | 741–959 ms |

Médianes à froid [G4]. Quand les feuilles partent au GPU, le parcours CPU d'une feuille de 24 coûte trois fois moins
qu'avec une feuille de 16 (filter_tests 379 M → 248 M, nœuds 783 k → 272 k). Avec un fil par feuille, ce gain est mangé
par la lenteur des feuilles lourdes.

## 3. Diagnostic : pourquoi le GPU perd à K = 5

1. **Amdahl d'abord.** Les feuilles ne pèsent que 26 à 39 ms des ≈ 207 ms de domain [dérivé G4]. Même instantanée et
   recouverte, la voie des feuilles ne peut rendre plus que cela à feuille 16. Aujourd'hui, elle coûte en série
   ≈ 70–76 ms : exécuteur, Level, rassemblement, reste.
2. **Divergence.** Un fil par feuille, et des coûts très inégaux : la moitié des feuilles n'émet rien, d'autres émettent
   jusqu'à 189 boules. Résultat : 3,3 fils utiles sur 32, soit environ 10 fois de calcul perdu. Trier les feuilles par m
   n'a rien changé [G4].
3. **Latence mémoire locale.** 3,3 Kio de pile par fil, accédés de façon divergente (2,4 octets utiles sur 32 par
   secteur). C'est 40 % du temps d'attente. Avec seulement 25 % d'occupation (168 registres), trop peu de warps restent
   éligibles pour cacher cette latence. L'ALU n'est pas la borne.
4. **Queue sérialisée.** La seconde passe met toutes les feuilles qui débordent dans un seul warp, ou dans 132 warps à
   K = 10. Coût : 17,5 ms à K = 5, ≈ 30 % de l'exécuteur, et 112 ms à K = 10, un tiers.
5. **Travail en trop.** Le device calcule 2,4 à 3,4 fois plus de droites J2 que `leaf.cpp`, parce que le cache est
   simulé et non réel.
6. **Aucun recouvrement.** Le parcours (≈ 110 ms, GPU inactif) précède l'exécuteur (≈ 59 ms, CPU inactif).
7. **À froid seulement**, le contexte : de 13 à 56 ms d'attente résiduelle, parce que l'ouverture, ralentie par la
   contention, dure plus longtemps que le parcours qui devait la masquer.

Les transferts (≈ 4 ms), CUB (< 0,1 ms) et les lancements ne sont **pas** la cause.

## 4. Opportunités chiffrées

Tous les gains sont des **[estimation]** fondées sur les mesures de la section 2. Une seule règle d'exactitude :
même code de feuille, même registre, mêmes dumps, et `unresolved` rejoué par `leaf.cpp`.

### O1. Écriture sans rejeu sérialisé (priorité 1)

- **Idée.** (a) Variante immédiate : lancer `fill_kernel` avec **une feuille par warp** (`<<<fill_jobs, 32>>>`, voie 0
  seule, ou blocs d'un fil). (b) Variante complète : une **arène de débordement** sur le device. Au comptage, une
  feuille dont la case déborde réserve sa place par `atomicAdd` sur un curseur (après sa propre réduction) et y écrit
  ses émissions dans l'ordre d'émission. Elle note son couple (début, longueur). `copy_kernel` copie ensuite cases et
  arène à leurs places, dans l'ordre des feuilles. Si l'arène est pleine, la feuille est marquée `fill` et rejouée comme
  aujourd'hui.
- **Coût actuel.** `fill_ns` de 12,7 à 19,8 ms à K = 5, de 105 à 117 ms à K = 10 ; cases de 2 Kio par feuille, soit
  724 Mo à K = 5 et 1,09 Go à K = 10 [G4 / dérivé].
- **Gain attendu.** Avec (b), il ne reste qu'une copie : **−12 à −19 ms par trame à K = 5** (3 à 5 % du mur GPU),
  **−100 à −115 ms à K = 10**. Avec (a), la durée tombe entre le maximum d'une feuille et la valeur actuelle : de −0 à
  −16 ms à K = 5, à mesurer. (b) permet aussi de réduire les cases à quelques centaines d'octets : une case de 32
  enregistrements et 256 incidences couvrait déjà 97,5 à 98,3 % des feuilles (S4). La mémoire device baisserait d'un
  facteur ≈ 4.
- **Exactitude.** L'ordre d'émission dans une feuille est inchangé, l'ordre entre feuilles reste fixé par les préfixes.
  Le curseur atomique ne décide que d'un emplacement intermédiaire, jamais de l'ordre publié. L'arène est réservée dans
  le `MemoryBudget` (`DeviceArray`), avec repli exact si elle est pleine.
- **Risque.** Faible : un compteur atomique de plus. La porte `mhgp11_tower_full_leaf_lanes` doit couvrir les trois
  chemins : case, arène, rejeu. Le mode `lot` sur l'hôte doit garder la même logique pour valider.
- **Effort.** (a) une heure ; (b) un jour, porte comprise.
- **Protocole G4.** `gpu_ab.py --modes cpu=16379,gpu=81915,gpuO1=<binaire O1>`, K = 5 feuille 16 et K = 10 feuille 24,
  7 prises froides (ordre de Williams) et 8 passes chaudes, identité dump et registre à chaque prise. Plus
  `gpu_profile.py` : le noyau `fill_kernel` doit disparaître ou se réduire à quelques warps. Publier `fill_jobs`, les
  feuilles en arène, `copied_jobs` et le pic du pool.

### O2. Cache J2 réel sur le device, pile dimensionnée à la feuille (priorité 2)

- **Idée.** Remplacer `seen` (1 bit par triple) par un état sur 2 bits (inconnu, ou relation), comme
  `CenterLineCache`, et ne calculer `center_line_meets` qu'au premier appel. Instancier aussi `Leaf` sur la borne
  réelle de la feuille (16, 24 ou 32) au lieu de `kMaxSites = 32`, pour que P, dom/domby/nbr, live, interior/shell et le
  cache tiennent dans la pile utile. À feuille 16, environ 1,4 Kio au lieu de 3,3 Kio [estimation par décompte des
  tableaux].
- **Coût actuel.** `count_kernel` 35,5 ms à K = 5, 217 ms à K = 10 ; droites calculées : 75,0 M contre 31,3 M utiles, et
  498 M contre 146 M ; attente L1TEX : 40 % [G4].
- **Gain attendu.** `center_line_meets` vaut ≈ 20 % des échantillons d'attente. En retirer 58 % des appels (K = 5) ou
  71 % (K = 10) rendrait ≈ 10 à 14 % du comptage. La pile réduite améliore le taux de réussite L1 (68 % aujourd'hui).
  Total : −15 à −30 % du comptage, soit **−5 à −10 ms à K = 5 et −35 à −65 ms à K = 10**.
- **Exactitude.** On mémorise une fonction pure : relations et décisions identiques. Les compteurs `evaluations` et
  `cache_hits` sont déjà tenus comme sur le CPU et restent identiques. Les bornes R1 (`kCountBound`) ne peuvent que
  diminuer avec une borne de feuille plus petite. Les feuilles de plus de la borne gardent leur repli.
- **Risque.** Faible. Le seul est de laisser une instance du gabarit non couverte par les portes : il faut tester les
  instances 16, 24 et 32.
- **Effort.** 1 à 2 jours.
- **Protocole G4.** Comme O1 (modes `gpuO1` et `gpuO1O2`), plus ncu sur `count_kernel` : pile, octets utiles par
  secteur local, part L1TEX. Identité exigée à chaque prise.

### O3. Feuille coopérative par warp, J3 (priorité 3, le levier structurel)

- **Idée.** Une feuille par warp. Sites, dom/domby/nbr, live et table H des droites en **mémoire partagée** (≈ 1,5 Kio
  par feuille de 32). Phases : paires (une voie par paire), triplets (une voie par triplet vivant), quadruplets (ET de
  trois lignes de H), census de m sites en parallèle (ballot), émission par préfixe de warp. Rangs locaux portés
  directement, sans recherche (demande de l'audit, réponse F). Feuilles larges : le DFS reste la référence.
- **Coût actuel.** Comptage 35,5 ms / 217 ms et queue (O1). Occupation de la voie de calcul ≈ 10 % (3,3 sur 32) [G4].
- **Gain attendu.** Une occupation des voies de 50 à 70 % au lieu de 10 %, et la pile locale remplacée par la mémoire
  partagée, donneraient un comptage **3 à 6 fois plus rapide** : K = 5 ≈ 6 à 12 ms, K = 10 ≈ 40 à 70 ms. La queue
  disparaît, car une feuille lourde est jouée par 32 voies. Cette estimation n'est étayée par aucune mesure : c'est
  pourquoi O1 et O2 passent d'abord, et l'instrumentation Q1 avant J3. **À feuille 16 et sans O4**, le gain reste
  borné par Amdahl : l'exécuteur passerait de ≈ 59 à ≈ 15–20 ms, soit **−0 à −20 ms** par rapport au CPU à K = 5. À
  K = 10 : **−250 à −300 ms** de domain par rapport au GPU actuel, donc ≈ −10 % du mur.
- **Exactitude.** Contrat J3 accepté par l'auditeur (réponse D). Les compteurs logiques sont identiques ;
  `evaluations = demandes`, aucun hit ni repli sous J3 ; les nouveaux comptes vont dans un diagnostic séparé. Il faut
  garder les certificats, qmin, contacts, propriétaire, la récursion q4 après rejet q3, et le rejeu entier de toute
  feuille non certifiée. **L'ordre d'émission dans une feuille changera** : il faut vérifier que rien en aval du bloc
  ne dépend de cet ordre avant le tri canonique (niveau, S*), puis exiger l'identité des dumps. `gpu_ab.py` compare le
  registre entier : il faudra le comparer hors des champs `region_line_evaluations` et `region_line_cache_hits`, ou
  recalculer ces champs à la façon du DFS.
- **Risque.** Élevé : nouvelle feuille, synchronisations de warp, mémoire partagée. Il faut aussi les portes natives
  encore dues (q3 extrême, q4 à 2^20 et 2^20 + 1, préfixe obtus, coquille à qmin = 2).
- **Effort.** 1 à 2 semaines, par tranches.
- **Protocole G4.** Tranches mesurées une à une (paires, puis triplets, puis quadruplets) contre O1+O2 :
  `gpu_ab.py` à froid et à chaud, ncu (fils actifs par warp, mémoire partagée et locale, part d'attente), chrono
  par feuille (Q1) pour vérifier que la queue a disparu.

### O4. Recouvrement du parcours et du lot : flux multiples, mémoire épinglée (priorité 4)

- **Idée.** Chaque tâche de la passe unique pousse ses feuilles par **tranches** (par exemple 8 192 feuilles) dans un
  tampon hôte épinglé (`cudaHostAlloc`, compté dans le budget). Un fil soumet chaque tranche sur un flux parmi 2 à 4 :
  envoi asynchrone, comptage, copie, retour asynchrone. Les préfixes globaux viennent à la fin, dans l'ordre
  (tâche, tranche) ; les Level de la tranche i sont calculés pendant le noyau de la tranche i+1.
- **Coût actuel.** Série stricte. Parcours de 95–112 ms pendant lequel le GPU est inactif, puis exécuteur de 51–59 ms et
  Level de 4,4–5,9 ms pendant lesquels le CPU attend. À K = 10 : 152 ms puis 343 ms [G4].
- **Gain attendu.** On masque le plus court des deux. À K = 5 et dans l'état actuel du noyau : −45 à −55 ms sur le
  domain GPU, donc **−10 à −20 ms par rapport au CPU** (la voie GPU passe enfin devant). À K = 10 : ≈ −150 ms. Avec O1 à
  O3, presque tout l'exécuteur est masqué.
- **Exactitude.** La disposition reste fixée par l'ordre (tâche, tranche, feuille), et les files d'une tâche sont
  déterministes. Il faut l'établir par une porte qui fait varier le nombre de fils et la taille des tranches à dump
  identique. Rien ne décide sur le device hors de `leaf_device`.
- **Risque.** Moyen : concurrence entre l'hôte et le device, budget de la mémoire épinglée, ordre des fins.
- **Effort.** 3 à 5 jours.
- **Protocole G4.** Modes `gpuO1O2` et `gpuO1O2O4`. nsys pour vérifier sur la ligne de temps le chevauchement des
  noyaux et du parcours. Identité à W1, W24 et W48 et pour deux tailles de tranche.

### O5. Taille de feuille co-optimisée pour le GPU (après O3, le plus gros gain du catalogue)

- **Idée.** Avec J3 (une voie par site), passer à la feuille 24 à K = 5, et 32 à K = 10.
- **Coût actuel.** Parcours de 94–110 ms (feuille 16) contre **29–38 ms** (feuille 24) à K = 5, et 118–137 ms contre
  **47–57 ms** à K = 10 [G4 S4]. Avec un fil par feuille, le comptage double et la queue explose, ce qui rend la feuille
  24 perdante (GPU K = 5 : 542 ms contre 477 ms, S4).
- **Gain attendu.** Si J3 rend le comptage en feuille 24 à 15–25 ms, le domain K = 5 serait d'environ 38 (parcours)
  + 57 (assemblage et tri) + 15–25 + ≈ 10 (Level et transferts), soit **≈ 120 à 130 ms au lieu de ≈ 207 ms**, et environ
  −80 ms sur le mur (≈ −25 %). C'est une estimation à confirmer tranche par tranche. **Même ainsi, le contrat de 100 ms
  reste hors d'atteinte par le catalogue seul** : le domain resterait au-dessus de 100 ms, et l'étage `tree` vaut
  139–171 ms sur CPU (qualification finale).
- **Exactitude.** La taille de feuille ne change pas l'objet. Les dumps sont identiques à la référence CPU en feuille
  16 ; c'est le cas aujourd'hui pour les feuilles 16, 24 et 32 (sessions 4 à 6, identité « conforme »).
- **Risque.** Moyen : les feuilles plus larges remplissent davantage la mémoire partagée.
- **Effort.** Faible une fois O3 fait (paramètre et mesure).
- **Protocole G4.** Matrice feuille {16, 24, 32} × K {5, 10} × {CPU, GPU J3}, sur les trois trames, à froid et à chaud.

### O6. Contexte ouvert une fois et sans contention (priorité basse, froid seulement)

- **Idée.** Ouvrir le contexte **au début du processus**, avant le chargement du nuage et avant le démarrage du Pool, et
  dans l'API à la construction de la Session. Précharger les modules (`cudaFuncGetAttributes` sur les noyaux dans le fil
  d'ouverture). Vérifier le mode persistance du pilote sur la VM, qui n'est consigné dans aucun reçu. Exposer la voie
  dans `src/api` si elle doit servir au produit.
- **Coût actuel.** Attente résiduelle à froid de 13 à 56 ms (S6), jusqu'à 113 ms (S4, feuille 24), plus 2,95 ms de
  premier lancement. Rien à chaud [G4].
- **Gain attendu.** Supprimer l'attente à froid, donc −13 à −56 ms sur un processus neuf. Rien sur un service
  long : une Session garde son contexte, que la voie n'est pas encore exposée à recevoir.
- **Exactitude.** Aucun effet sur le calcul.
- **Risque.** Très faible.
- **Effort.** Une demi-journée.
- **Protocole G4.** Prises à froid seulement : `prefetch_ns`, `device_init_ns`, 1er lancement, avec et sans `taskset`
  du fil d'ouverture. Relever `nvidia-smi -q | grep -i persistence` dans l'inventaire de `gpu_profile.py`.

### O7. Mémoire épinglée et transferts (marginal seul ; prérequis de O4)

- **Coût actuel.** Envoi de 1,5 à 2,0 ms (44 Mo pageables, 23 à 30 Go/s) ; retour de 1,9 à 2,3 ms à K = 5 et de 6,4 à
  7,8 ms à K = 10 [G4].
- **Gain attendu.** Avec des tampons épinglés réutilisés : −1 à −2 ms à K = 5, −3 à −5 ms à K = 10. Réduire `LeafJob` de
  64 à 32 octets (boîte en i32 si la borne d'entrée le prouve) retirerait 11 Mo d'envoi, soit −0,4 ms.
- **Exactitude.** Neutre. Le budget doit compter l'épinglé (MemoryBudget, groupe `reservation`).
- **Effort / risque.** Faibles.
- **Protocole.** nsys (débits `[CUDA memcpy]`, type de mémoire Pinned contre Pageable), plus `gpu_ab.py`.

### O8. Registres et occupation (essai bon marché, résultat incertain)

- **Idée.** Ajouter `__launch_bounds__(32, 16)` ou plafonner à 128 registres : occupation théorique de 33 % au lieu de
  25 %.
- **Coût actuel.** 168 registres, 25 % d'occupation, 0,45 warp éligible [G4].
- **Gain attendu.** Entre −10 % et +10 % du comptage. Un débordement des registres vers la mémoire locale
  aggraverait la borne actuelle. À ne tenter qu'après O2.
- **Exactitude.** Neutre. **Effort.** Une heure. **Protocole.** ncu : registres, débordements (spill), occupation
  atteinte ; puis `gpu_ab.py`.

### Ordre conseillé et cumul

D'abord Q1 (instrumentation par feuille, ci-dessous), puis O1 (b), O2, O6 et O7, puis O4, puis O3, puis O5.
Estimation cumulée à K = 5 : O1+O2 ramènent l'exécuteur de ≈ 59 à ≈ 35 ms, ce qui ne suffit pas encore à battre le CPU
en série ; avec O4, le GPU passe devant de 15 à 30 ms ; avec O3+O5, le domain descendrait vers 120 à 130 ms. À K = 10 :
O1+O2 rendent ≈ −150 ms, O3+O4 ≈ −250 ms de plus, sur un mur de 1,8 à 2,4 s dominé par les forêts.

## 5. Pistes écartées

- **Réécrire l'arithmétique `i128`** (ou poser des filtres flottants) : l'ALU entière est à 24 % et l'attente est
  dominée par la latence mémoire et la divergence. Une décision flottante est de toute façon interdite.
- **Retrier les feuilles par m** ou par taille : déjà mesuré, sans effet sur les fils actifs (3,29 à 3,43). La taille
  ne prédit pas le travail.
- **Blocs plus gros avec barrière** : mesuré en S3 puis S5, le retrait de la barrière ne rend que 3 %.
- **Format compact en rangs locaux** : mesuré en S6, à peu près neutre sur le mur. Ne pas redemander l'ablation
  (audit, réponse F).
- **Agrandir les cases fixes** pour supprimer le rejeu : elles coûtent déjà 2 Kio par feuille (724 Mo à 1,09 Go). C'est
  l'arène (O1 b) qui règle le débordement.
- **Mémoire unifiée ou gérée** : elle réintroduit les fautes de page de retour (4 Go/s mesurés avant les pages
  pré-touchées).
- **Graphes CUDA, réduction des lancements** : 27 lancements par passe, quelques µs chacun, hors le premier (2,95 ms,
  traité par O6).
- **Flux multiples sans découpage du lot** : la chaîne count → scan → fill → retour est dépendante. Il n'y a rien à
  recouvrir au-delà de ≈ 2 ms. Les flux ne servent qu'avec O4.
- **Feuille 24 ou 32 avec un fil par feuille** : mesuré perdant (S4).
- **Croire que la voie des feuilles tiendra les 100 ms** : à feuille 16, elle ne peut rendre que 26 à 39 ms. Même avec
  O3+O5, le domain resterait au-dessus de 100 ms, et l'étage `tree` (139 à 171 ms, CPU) n'est pas touché.

## 6. Questions ouvertes

1. **Q1, le préalable à J3.** Quelle est la distribution du coût par feuille sur le device ? Proposition : chronos
   `clock64()` par feuille, et par warp le maximum et la somme, dans un tableau de diagnostic hors registre.
   `count_kernel` est-il borné par sa queue ou par son débit ? Quelle est la durée de la feuille la plus lourde ? Un
   indicateur bon marché (lignes `live`, voisins `nbr`) prédit-il le travail, ce qui permettrait un tri par coût ?
2. La durée d'ouverture du contexte (136–183 ms dans le fil, contre 69–78 ms sous nsys) vient-elle de la contention
   avec les 48 fils ? Le mode persistance était-il actif sur la VM ?
3. J3 : quelque chose dépend-il de l'ordre des émissions **dans** une feuille avant le tri canonique (niveau, S*) ?
   Comment `gpu_ab.py` doit-il comparer les registres sous le contrat D (champs `evaluations` et `cache_hits`) ?
4. La voie GPU doit-elle entrer dans l'API et la CLI ? Aujourd'hui elle n'existe que dans `full_probe`, et la
   qualification finale ne la couvre pas.
5. `docs/CATALOGUE.md` annonce des cases de « 32 enregistrements, 256 incidences », alors que le code en a 128 et 1 024
   (2 Kio, `kScratchRecords` et `kScratchPopulation`). La documentation est restée à la session 4 et doit être
   corrigée.
6. À K = 10, une fois O1 à O4 faits, les forêts (1,2 à 1,7 s) dominent : la voie des feuilles ne peut plus rien pour le
   mur. Quelle part du budget GPU faut-il alors consacrer aux forêts plutôt qu'au catalogue ? Cette question relève
   d'un autre angle de la carte.
