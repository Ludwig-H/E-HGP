# Levier D : queue du noyau de comptage due aux feuilles lourdes (GPU) — ABANDONNÉ, sonde diagnostique livrée

6 octobre 2026, worktree `/tmp/v11-wf-D` à la base `905fad2e1` (origin/main), modifications non commitées.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / public_status=not_claimed`.
GCP non utilisé. Aucun GPU sur le codespace : tout chiffre G4 ci-dessous est une **ESTIMATION** par modèle.

## 1. Verdict

**Statut : abandonné.** Ni le découpage des feuilles lourdes en tâches de paires (piste ii) ni l'ordre des feuilles par
travail estimé décroissant (piste i) ne sont rentables d'après un modèle du noyau de comptage calé sur deux mesures
Nsight indépendantes. Le diagnostic de départ (« la latence d'une feuille lourde sur un fil fait la queue du noyau »)
n'est pas compatible avec ces mesures :

- les warps ne reconvergent pas entre feuilles. Nsight donne 3,1 à 3,4 fils actifs sur 32, y compris dans le warp de
  la seconde passe qui ne contenait que 7 à 15 feuilles lourdes (4,10). La durée d'un warp suit donc à peu près
  **la somme** du travail de ses 32 feuilles divisée par environ 3, et non le maximum ;
- une feuille lourde (environ 20 fois la moyenne) allonge son warp d'environ 1,6 fois seulement ;
- la queue du noyau vient surtout de la granularité des vagues (5 vagues à K5, 7,35 à K10) et de la variance de la
  somme par warp, et pas d'une feuille isolée.

Le patch ne livre donc **pas** le découpage. Il livre une **sonde diagnostique** à coût nul sur le chemin chaud : deux
ordres de fils de l'exécuteur CUDA. Ils permettent à G4 de réfuter ou de confirmer le modèle en une session. Les SASS
de tous les noyaux sont identiques à la base. L'émulation exacte du découpage reste disponible en annexe, avec
0 écart sur 1 237 171 feuilles de ng00, si G4 contredisait le modèle.

## 2. Travail par feuille mesuré (ng00, hôte, un fil, `rdtsc`, minimum de 3 prises)

Instrumentation temporaire de l'exécuteur hôte (retirée du patch, conservée dans l'annexe). Cycles de `run_leaf`
(`CountSink`) par feuille, compteurs, poids candidats.

| | K5, feuilles de 16 | K10, feuilles de 24 |
| --- | ---: | ---: |
| feuilles | 353 456 | 530 259 |
| feuille la plus lourde / moyenne | 19,9 | 22,7 |
| part du travail du top 1 % | 6,7 % | 6,7 % |
| part du travail du top 0,1 % | 1,1 % | 1,1 % |
| Spearman (cycles, m) | 0,55 | 0,45 |
| Spearman (cycles, paires de profondeur 1) | 0,83 | 0,87 |
| Spearman (cycles, triplets candidats de profondeur 2) | 0,91 | 0,93 |

Les mêmes ordres de grandeur se retrouvent dans une première prise, plus bruitée (machine chargée, charge 6 à 11 sur
8 cœurs) : top 1 % = 6,3 et 6,8 %, Spearman triplets = 0,96 et 0,92. Une régression déterministe de ces cycles sur
les compteurs (préfixes, recensement, évaluations J2, candidats q4, jugés) donne R² = 0,84 à K5 et 0,92 à K10. Elle
sert de troisième jeu, sans bruit.

## 3. Modèle du noyau, calé sur Nsight

Modèle :
- 188 SM × 12 warps résidents (164 registres, blocs d'un warp) ;
- blocs affectés gloutonnement dans l'ordre des fils ;
- durée d'un warp = max(feuille la plus longue, somme des 32 feuilles / k).

Deux variantes :
- **modèle somme** : k = 3,14, la valeur Nsight des fils actifs ;
- **modèle max** : reconvergence parfaite, la durée vaut la feuille la plus longue.

| Observable Nsight | Mesure G4 | Modèle somme | Modèle max |
| --- | --- | --- | --- |
| fils actifs par warp, ordre par m | 3,1 à 3,4 | 3,14 (par construction) | 9,5 |
| warps actifs moyens, K5/16 (session gpu 5 : 21,6 % de 25 %) | 10,4 sur 12 | 10,5 à 11,1 | 6,3 à 9,0 |
| warps actifs moyens, K10/24 (coop2 : 21,06 % de 25 %) | 10,1 sur 12 | 10,3 à 11,2 | 4,2 à 8,7 |
| seconde passe de 7 à 15 feuilles lourdes, un warp seul (K5) | 13 à 21 ms | environ 30 ms en charge, moins sur un SM libre | environ 17 ms |

Les deux premières lignes réfutent le modèle max. La troisième est compatible avec le modèle somme dès qu'un warp seul
sur son SM va 1,5 à 2 fois plus vite que dans le noyau chargé (7,96 cycles par instruction émise mesurés en charge).

**Borne du gain d'équilibrage parfait** (somme des durées de warp / emplacements, modèle somme) :
- K5 : 0,87 à 0,92 × le comptage actuel ;
- K10 : 0,86 à 0,93 ×.

C'est au mieux 7 à 14 % du noyau de comptage, quel que soit le levier.

## 4. Pistes jugées

### (i) Ordre des feuilles par travail estimé décroissant

Rapport au comptage actuel (ordre par m) ; une colonne par jeu de mesures (prise 1, prise 2, régression).

| Ordre | Modèle somme, K5 | Modèle somme, K10 | Modèle max, K5 | Modèle max, K10 |
| --- | --- | --- | --- | --- |
| triplets décroissants (feuille) | 1,80 / 1,80 / 1,72 | 1,42 / 1,60 / 1,51 | 0,76 / 0,79 / 0,67 | 0,89 / 0,65 / 0,53 |
| warps de l'ordre par m, lancés par somme des triplets (LPT) | 0,97 / 0,93 / 0,96 | 1,07 / 1,03 / 1,03 | 0,90 / 1,04 / 1,10 | 0,97 / 1,05 / 1,05 |
| LPT des warps sur le travail réel (oracle, irréalisable) | 0,92 / 0,89 / 0,95 | 0,87 / 0,93 / 0,95 | – | – |
| lot sans tri | 1,31 / 1,23 / 1,19 | 1,16 / 1,20 / 1,19 | 1,21 / 1,30 / 1,35 | 1,05 / 1,10 / 1,11 |

Trier les feuilles lourdes en tête les regroupe dans les mêmes warps. Dans le modèle somme, ces warps deviennent
énormes, d'où le facteur 1,4 à 1,8. Le LPT par warps garde la composition de l'ordre par m, mais le gain reste au mieux
de 3 à 7 % à K5 et devient une perte à K10.

Surtout, l'estimation elle-même coûte trop cher : poids = préparation, lignes vivantes et triplets, en séquentiel sur
ng00.
- K5 : 1,32 s, contre 6,75 s pour les feuilles entières, soit **19,6 %**.
- K10 : 4,02 s contre 34,8 s, soit **11,6 %**.

Sur GPU, un noyau d'estimation préalable referait la préparation (`prepare` + `live_rows`) de toutes les feuilles, soit
au moins autant que le gain espéré. Sur l'hôte W48, l'estimation coûterait environ 28 ms à K5 et 85 ms à K10
(ESTIMATION), de l'ordre du comptage GPU lui-même.

### (ii) Découpage des feuilles lourdes en tâches de paires

L'**émulation hôte exacte** a été écrite (annexe, `leaf_device_split.hpp`). Elle comprend :
- `run_leaf_or_defer` (poids = paires de profondeur 1) ;
- `split_prepare` (tables et profondeur 0 avec les compteurs d'`extend<0>`) ;
- `run_pair` (drapeau `One` d'`extend`, qui garde la boucle de 830473218 pour la voie un fil) ;
- l'union des mémoires J2 des tâches (évaluations = rangs distincts, succès = tests − évaluations) ;
- le cas sans cache.

Résultat : statut, 15 compteurs, boules et incidences sont égaux à `run_leaf` sur toutes les feuilles de ng00, soit
353 456 feuilles à K5 avec cache, 530 259 à K10 avec cache et 353 456 à K5 sans cache. Les tâches y sont comptées
dans un ordre permuté (pas 7). **0 écart.**

Mais le surcoût par feuille découpée est de **× 1,22 à 1,47** (cycles hôte des tâches / feuille entière : mémoire J2
perdue entre tâches, tables relues), et le gain dépend de l'emplacement des tâches. Rapports au comptage actuel, modèle
somme, tâches au coût mesuré :

| Seuil de paires | K5 : feuilles découpées (part du travail) | K5 : noyau séparé | K5 : même noyau, tâches en queue | K10 : noyau séparé | K10 : même noyau |
| --- | --- | --- | --- | --- | --- |
| > 96 | 7 928 (9,4 %) | 1,06 à 1,07 | 0,96 à 0,97 | 1,20 à 1,38 | 1,15 à 1,32 |
| > 160 (K10) | – | – | – | 1,15 | 0,94 à 1,05 |
| > 192 (K10) | – | – | – | 1,05 à 1,09 | 0,94 à 0,95 |

- **Noyau séparé**, lancé après le comptage, seule forme possible si le report est décidé dans le noyau de comptage :
  toujours une perte (1,01 à 1,38), avant même la seconde passe d'écriture des tâches (rejeu de 6 à 10 % du travail)
  et les 5 à 6 lancements supplémentaires.
- **Même noyau, tâches en queue** : les tâches comblent la queue des vagues. Le gain atteint 3 à 6 %, mais exige de
  connaître les feuilles à découper avant le lancement, donc l'estimation de la piste (i), qui coûte 12 à 20 % du
  travail. Perte nette.
- Le découpage idéal sans surcoût (top 1 %) ne donne lui-même que 0,94 à 0,97.

## 5. Ce que livre le patch : ordres diagnostiques des fils (sonde pour G4)

**Conception.**
- `thread_order` (dans `leaf_batch.cpp`, compilé sur l'hôte) remplace `size_order` de `leaf_batch_cuda.cu`, déplacé
  sans changement.
- Trois ordres :
  - `kOrderSize`, défaut : m décroissant, stable ;
  - `kOrderWeight` : poids décroissant, puis rang dans le lot ;
  - `kOrderWarpWeight` : warps complets de l'ordre par m, lancés par somme des poids décroissante puis rang du warp ;
    le warp incomplet reste en queue pour que la composition des autres ne glisse pas.
- `leaf_weight` = triplets candidats de profondeur 2, mêmes coupes G3 et lignes vivantes qu'`extend<0>` et
  `extend<1>`.
- Les poids sont calculés sur le Pool, hors du chrono de comptage ; la durée est publiée dans
  `leaf_batch.order_ns`.
- Plomberie : `CatalogueParams::thread_order`, `LeafBatchView::order`, `LeafBatchTimings::order_ns`,
  `CatalogueTimings::batch_order_ns`.
- Bits de sonde : **262144** pour l'ordre par poids, **524288** pour les warps par poids. Ils ne sont admis qu'avec
  65536 (lot CUDA) et un seul à la fois, sinon la sonde rend le code 2. Le maximum de la sonde passe de 262143 à
  1048575.

**Preuve d'exactitude.**
- Seule l'affectation fil → feuille change (`DeviceView::order`, et l'ordre de la liste de la seconde passe). Statuts,
  comptes, préfixes, cases, chaînes et écritures restent indexés par feuille du lot ; les préfixes sont calculés dans
  l'ordre du lot.
- Les sorties ne dépendent donc pas de l'ordre, tant que celui-ci est une permutation. Les ordres diagnostiques le
  contrôlent explicitement (refus `catalogue_invariant` sinon).
- L'ordre par défaut est le tri par comptage d'origine, au caractère près.
- Toute allocation nouvelle (poids, sommes et rangs des warps, ordre rangé, contrôle) est admise au `MemoryBudget`
  avant allocation. Un ordre inconnu est refusé (`parameter_out_of_range`), un lot de 2^32 feuilles ou plus aussi
  (`index_overflow_u32`).
- Sorties déterministes : comparateurs totaux (clé puis rang) ; les poids ne dépendent pas du nombre de fils.
- Les noyaux sont inchangés (§ 7).

## 6. Portes jouées (codespace, Release, u21, `/tmp/v11-wf-D-build`)

- `ctest -R "full_leaf_lanes|catalogue_(cache|single_pass|leaf|small_pair|parallel)"` : **36/36 conformes**. Elles
  comprennent `mhgp11_tower_full_leaf_lanes` et `_opt`, avec la ligne gravée inchangée, et la nouvelle porte
  `mhgp11_catalogue_leaf_order` :
  - `weight_direct` : 4 001 contrôles, contre un décompte direct par triple boucle, K = 1, 2, 5 et 10, plancher de
    50 feuilles de poids > 100 ;
  - `orders` : 12 196 contrôles (permutation, clé monotone, départage par rang, warps entiers et somme décroissante,
    warp incomplet en queue) ;
  - `refusals` : 5 contrôles (ordre inconnu ; budget juste suffisant pour l'ordre par m, puis refus `memory_budget`
    avant toute allocation de l'ordre par poids) ;
  - `inventaire`.
- Sonde ng00, 2 fils :

  | Configuration | Mode | Dump | Empreinte du registre |
  | --- | --- | --- | --- |
  | K5/16 | 16379 | `3a2bfb4f9f48…` | `383375b91d75` |
  | K5/16 | 49147 | `3a2bfb4f9f48…` | `383375b91d75` |
  | K10/24 | 16379 | `61a4245b91d9…` | `e05f5f2e818b` |
  | K10/24 | 49147 | `61a4245b91d9…` | `e05f5f2e818b` |

  Les dumps sont les dumps de référence, le registre est identique entre voies et `order_ns` vaut 0 sur l'hôte.
- Refus de la sonde : 311291 (49147 + 262144) → code 2 ; 573435 → code 2 ; 868347 (deux bits) → code 2 ; 344059
  (81915 + 262144) sans CUDA → `invalid_input / parameter_out_of_range`, code 2.
- `python3 tools/check_style.py` : `style_ok fichiers=532`, après sortie de `timed_order` pour garder
  `run_leaf_batch_cuda` à 100 lignes.
- `run_mutants.py --check` : `manifeste_ok module=catalogue mutants=75 plancher=75`.

## 7. SASS (nvcc 12.9, sm_120, Release, u21)

| Noyau | Base 905fad2e1 | Patch D |
| --- | --- | --- |
| count | 11 832 instructions, 134 BSSY, 3 CALL, 164 registres, LDL 358, STL 311 | identique |
| fill | 9 920 instructions, 93 BSSY, 0 CALL, 168 registres | identique |

Le désassemblage complet de l'objet (`cuobjdump -sass`, adresses et empreinte de l'espace de noms anonyme normalisées)
est **identique octet pour octet** pour tous les noyaux. Le diagnostic ne touche que du code hôte.

## 8. Mutants

Sept mutants nouveaux dans `tests/mutants/catalogue.json` (plancher 68 → 75). Tous **TUÉS** (code de porte) :
- `ordre_poids_croissant` ;
- `ordre_poids_departage_inverse` ;
- `ordre_warps_somme_croissante` ;
- `ordre_warps_rangs_confondus` : permutation refusée ;
- `ordre_warps_non_applique` ;
- `poids_sans_vivant_j` ;
- `poids_seuil_profondeur_un`.

Une première écriture de ce dernier ne compilait plus (variable inutilisée, `-Werror`) et a été jugée INVALIDE par le
lanceur. Il a été réécrit (`> second + 1`), puis rejoué : `mutants_ok module=catalogue mutants=1 tues=1`. Pour les six autres, la
campagne `--only` affichait `TUE code` pour chacun.

Non couverts localement, faute de CUDA : le câblage de `timed_order` dans l'exécuteur CUDA et le choix de l'ordre par
les bits de la sonde. Ils sont couverts sur G4 par l'identité des dumps et des registres de `gpu_ab`.

## 9. Mesures hôte appariées (descriptives)

Le patch ne modifie aucun chemin de l'hôte (exécuteur Pool, feuilles, lot).

Mesure appariée et alternée base/D, ng00, K5/16, mode 49147 (lot sur le Pool), W2, dernière de 3 passes à chaud :

| Paire | `domain` base → D (ms) | `count` base → D (ms) | Rapport `domain` |
| ---: | --- | --- | ---: |
| 1 | 4 035 → 5 462 | 2 496 → 3 571 | 1,35 |
| 2 | 7 842 → 6 655 | 5 065 → 4 118 | 0,85 |
| 3 | 9 570 → 9 606 | 5 911 → 6 024 | 1,00 |

Le rapport médian vaut 1,00. La machine était saturée par les autres agents (charge 10,8 à 13,6 sur 8 cœurs) : ces
chiffres sont du bruit à ± 35 %. Ils ne mesurent rien d'autre que l'absence de chemin hôte modifié, ce que montrent
déjà le code et l'identité des dumps et des registres. Aucune mesure GPU n'est possible ici.

## 10. Effet attendu sur G4 (ESTIMATION, écrite avant toute mesure)

Commande : `bench/gpu_ab.py --modes cpu=16379,gpu=81915,gpu_poids=344059,gpu_warps=606203` à K5/16 et K10/24, sur
ng00, ng01 et ng02, W48. Dumps et registres identiques exigés.

| | Modèle somme (retenu) | Modèle max (réfuté par Nsight) |
| --- | --- | --- |
| `count_ns`, `gpu_poids` / `gpu`, K5 | **× 1,7 à 1,8** (environ 39 → 67 à 70 ms) | × 0,67 à 0,79 |
| `count_ns`, `gpu_poids` / `gpu`, K10 | **× 1,4 à 1,6** (environ 226 → 320 à 360 ms) | × 0,53 à 0,89 |
| `count_ns`, `gpu_warps` / `gpu`, K5 | × 0,93 à 0,97 | × 0,90 à 1,10 |
| `count_ns`, `gpu_warps` / `gpu`, K10 | × 1,03 à 1,07 | × 0,97 à 1,05 |

En plus, sur `order_ns` (hors comptage, dans l'exécuteur), avec W48 : environ 15 à 30 ms à K5 et 40 à 90 ms à K10
pour les deux ordres diagnostiques, contre moins de 1 ms pour l'ordre par m. Le `domain` des modes diagnostiques sera
donc plus lent ; seul `count_ns` juge le modèle.

**Lecture.**
- Si `gpu_poids` ralentit le comptage d'au moins 1,3 fois, le modèle somme est confirmé : l'abandon du levier D est
  définitif, et le seul levier de queue restant est de rendre les warps homogènes en somme sans estimation (borne
  7 à 14 %).
- Si `gpu_poids` accélère le comptage, le modèle est faux : l'émulation du découpage (annexe) devient le point de
  départ, avec la forme « même noyau, tâches en queue ».

**Production : aucun effet** (ordre par défaut inchangé, SASS identique).

## 11. Risques

- Le modèle repose sur les cycles hôte comme proxy du travail d'un fil GPU. La répartition hôte (J2 environ 32 %) est
  proche de celle de Nsight (J2 38 %), mais l'i128 et la mémoire locale pèsent différemment sur GPU. Les trois jeux
  de mesures donnent les mêmes signes.
- Les cycles hôte ont été pris sur une machine chargée (8 cœurs, charge 6 à 11). C'est la raison de la troisième
  prise par régression déterministe.
- Le diagnostic coûte du temps hôte (W48 : 15 à 90 ms par lot) et quelques Mo (poids u64, ordre rangé, contrôle) : à
  réserver aux sessions de mesure.
- La seconde passe d'écriture suit aussi l'ordre diagnostique (`select_emitting` sur `order`). Elle est presque vide
  avec le réservoir et sans effet sur les sorties.
- Aucun risque d'exactitude identifié : la sortie est indexée par feuille et la permutation est contrôlée.

## 12. Annexes (dans `build/v11-persist/wf_gpu/`)

- `D.patch` : le patch livré.
- `D_annexe_emulation_decoupe.patch` : émulation hôte du découpage, avec drapeau `One` d'`extend`, `split_prepare`,
  `run_pair`, union J2 et instrumentation des statistiques. **Non livrée au produit.** Le contrôle « SPLIT_CHECK … bad
  0 » sur 1 237 171 feuilles de ng00 est la preuve d'égalité exécutée.
- `D_annexe/` : scripts de simulation (`simulate*.py`, `orders.py`, `bounds.py`, `separate.py`, `fit.py`) et
  sorties. Les fichiers CSV par feuille (environ 30 Mo chacun) restent dans `/tmp/v11-wf-D-run`, non copiés : le
  disque `/workspaces` est plein.
