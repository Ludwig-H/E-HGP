# Levier A : réservoir chaîné sans appel au chemin froid (6 octobre 2026)

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. GCP non utilisé. Aucun commit, aucune branche. Base : 905fad2e1 (origin/main). Worktree : `/tmp/v11-wf-A`. Construction : `/tmp/v11-wf-A-build`. Correctif : `/workspaces/E-HGP/build/v11-persist/wf_gpu/A.patch`.

Statut : **prêt pour G4**. Le noyau de comptage revient au SASS de j2memo : 10 352 instructions, 0 CALL et 164 registres, avec 112 BSSY contre 107. L'écriture reste à ~0 : aucune feuille rejouée tant que le réservoir suffit. Le temps GPU n'est **pas mesuré**, faute de GPU sur le codespace.

## 1. Ce que dit le SASS de la base (diagnostic)

J'ai compilé avec `-lineinfo` puis attribué chaque instruction à sa ligne source avec `nvdisasm --print-line-info`. Les 1 480 instructions de plus de la base par rapport à j2memo (11 832 contre 10 352) ne viennent presque pas du puits. Elles viennent pour l'essentiel de l'épilogue du noyau :

| fichier source (instructions statiques du noyau count) | j2memo | base 905fad2e1 | reservoir2 (59509bbc8) | A |
|---|---|---|---|---|
| leaf_device_predicates.hpp | 6211 | 6322 | 6492 | 6200 |
| leaf_device.hpp | 2805 | 2800 | 2746 | 2819 |
| leaf_batch.hpp (puits) | 834 | 1099 | 568 | 835 |
| leaf_batch_cuda.cu | 304 | 379 | 362 | 292 |
| sm_30_intrinsics.hpp (échanges de l'épilogue) | 168 | 1170 | 1202 | 170 |
| total | 10352 | 11832 | 11440 | 10352 |

Dès que le parcours contient une opération atomique (appelée ou intégrée), ptxas ne peut plus prouver que le warp a reconvergé au moment de la réduction `__shfl_down_sync`. Il génère alors un repli `WARPSYNC.COLLECTIVE … ENDCOLLECTIVE` pour chacun des 150 échanges. Cela fait environ 1 000 instructions et 14 à 15 BSSY de la base, que le compteur statique attribuait à tort au parcours. Ce repli n'est pas joué quand le warp a convergé, ce qui est le cas ici puisque les 32 fils arrivent à la réduction. Le chemin rapide reste un `SHFL` nu.

Corollaire : les comptes « instr » et « BSSY » bruts de la base (11 832 / 134) surestimaient l'écart réel. Dans le parcours, l'écart de la base avec j2memo tient en trois points :
- l'appel `cold` hors ligne (3 CALL, cadre de pile 4 144 octets) ;
- environ 260 instructions de puits en plus ;
- environ 110 instructions de prédicats en plus, dues au réarrangement des registres autour des appels.

Il faut aussi rappeler que le compte statique de BSSY ne prédit pas le temps : reservoir2 n'a que 100 BSSY hors épilogue (moins que j2memo) mais comptait en 43 ms. La cause était dynamique : une boucle octet par octet avec un test de bloc à chaque octet.

## 2. Conception retenue (piste iii raffinée)

**Blocs entrelacés à un seul curseur.** Chaque case reste un bloc de 2 048 octets par feuille, comme aujourd'hui (128 × 8 + 1 024). Chaque émission y range son enregistrement de 8 octets (`encode_bytes`, même disposition que `LeafRecord`) suivi de sa population (`p + m` octets).
- **Marge** : une émission ne commence dans le bloc courant que si `used <= kBlockLimit = 2048 - 8 - 64`. Elle y tient alors entière : il n'y a **jamais de chevauchement** entre deux blocs, donc aucune boucle octet par octet ni de test par octet.
- **État du puits** : `block` et `used` (deux u32), en plus de `balls` et `incidences`. Ils remplacent deux pointeurs, deux places restantes, deux blocs courants et `fits`. `fits()` vaut `block != kNoChunk`.
- **Chemin chaud** : un seul test `used > kBlockLimit`, puis une écriture directe. C'est le même code que la case de j2memo : 835 instructions de puits contre 834.
- **Changement de bloc intégré**, sans appel ni boucle : `taken = claim_chunk(arena); arena.next[block] = taken; block = taken; used = 0;`.
  - `claim_chunk` n'a plus de branche `spare == 0` : avec `spare = 0`, `taken < 0` est faux et le résultat est `kNoChunk`.
  - Une fois `block = kNoChunk`, `used` reste nul et l'on ne repasse jamais par le changement de bloc. Le lien du bloc plein reçoit alors `kNoChunk`, un lien qui n'est jamais suivi puisque la feuille sera rejouée.
  - Au SASS, cela donne un BSSY court par site d'émission, avec un corps prédiqué : un ATOM, un SEL et trois stores.
- **Un seul genre de bloc** : un seul tableau `next` et un seul curseur. La mémoire est inchangée (2 Kio par case et par bloc du réservoir) ; on économise `4 × (count + spare)` octets de liens.
- **Copie** (`copy_scratch`) : elle rejoue la même règle de changement de bloc en lisant les enregistrements de la chaîne, et elle renvoie maintenant un booléen. Elle refuse explicitement dans trois cas : bloc hors de l'arène, `need > 64`, ou comptes qui ne concordent pas. L'hôte rend alors `catalogue_invariant`. Sur CUDA, `copy_kernel` incrémente `errors`, ce qui produit le même refus au retour.
- **Épilogue** : un `__syncwarp()` avant la réduction. Ptxas retire alors les 150 replis collectifs. Le coût est d'un WARPSYNC par fil en fin de noyau.

### Pistes écartées, et pourquoi

| piste | verdict |
|---|---|
| (i) drapeau seul, puis passe dédiée qui ré-émet les feuilles débordées dans le réservoir | C'est le rejeu, qui retraverse les feuilles lourdes : 77 ms à K10 sur G4 (4 196 feuilles), latence d'une feuille lourde sur un fil. Le SASS de A atteint déjà celui de j2memo **sans** rejeu, donc (i) n'apporte rien. |
| (ii) capacité variable par feuille fixée avant le noyau | Le chemin chaud serait celui de j2memo, mais avec un rejeu résiduel, parce que m prédit mal la lourdeur (top 1 % des feuilles = 5,4 % du travail). A obtient le même SASS sans prédicteur et sans rejeu : piste non construite. |
| (iii) chemin froid intégré tel quel (reservoir2) | Il ralentissait déjà de 30 à 43 ms à K5 (boucle octet par octet). C'est l'entrelacement avec marge qui permet de l'intégrer sans boucle. |
| `blocks` copié dans le puits (une indirection de moins) | Mesuré : 11 512 contre 11 520 instructions, LD 175 contre 178. Gain négligeable, non retenu. |
| population écrite par un pointeur avancé (`at += p`) | Mesuré : +32 instructions, BSSY inchangés. Non retenu. |
| `claim_chunk` avec garde `spare == 0` | Mesuré : BSSY 116 au lieu de 112. Non retenu. |

## 3. Preuve d'exactitude

1. **Décisions** : `leaf_device.hpp` n'est pas touché, et le puits ne fait que ranger. Statuts, compteurs, `balls` et `incidences` sont calculés exactement comme avant.
2. **Rangement** : on définit pour une feuille la suite déterministe des émissions `e_1, …, e_n`, chacune avec `need_i = p_i + m_i <= 64`.
   - Le puits place `e_i` au couple (bloc ordinal `b_i`, décalage `u_i`) défini ainsi : `u_1 = 0`, puis `u_{i+1} = u_i + 8 + need_i`. Si `u_{i+1} > L`, la position passe au bloc ordinal suivant avec décalage 0 juste avant `e_{i+1}`.
   - Comme `u_i <= L = 2048 - 72`, on a `u_i + 8 + need_i <= 2048` : l'émission tient dans son bloc.
   - La copie recalcule la même suite à partir des seuls octets lus (`p`, `m` aux octets 4 et 5 de l'enregistrement, écrits par le même puits). Elle suit les mêmes changements de bloc au même indice `i`, puis le lien `next[bloc]` écrit **avant** toute écriture dans le nouveau bloc.
   - Elle écrit l'enregistrement `i` à `record_begin + i` et sa population à `population_begin + Σ_{t<i} need_t`. Ce sont exactement les places de `FillSink`, qui émet dans le même ordre.
3. **Blocs distincts** : chaque bloc du réservoir est pris une seule fois (curseur atomique, indice `< spare`), et seul son preneur écrit dans le bloc et dans son lien. La copie ne lit qu'après la fin du comptage (`cudaDeviceSynchronize` sur l'appareil, fin du `parallel_for` sur l'hôte). L'ordre des blocs pris dépend des fils, jamais les sorties.
4. **Épuisement** : si `claim_chunk` rend `kNoChunk`, la feuille ne range plus rien. Elle a `stored = 0`, elle est rejouée par `FillSink` aux places des préfixes, et elle reste vérifiée par `record_end` et `population_end`.
5. **Bornes** :
   - `used <= 2048` en u32 ;
   - `block < count + spare < 2^32` (garde inchangée des exécuteurs) ;
   - le curseur ne dépasse pas `count + spare`, car chaque feuille échoue au plus une fois et les réussites sont au plus `spare` : il ne peut pas reboucler.
6. **Mémoire** : l'hôte admet `chunks × 2048 + 4 × (chunks + 1)` octets au `MemoryBudget` avant d'allouer. Sur CUDA, `DeviceArray::allocate` réserve au budget avant `cudaMallocAsync`.
7. **Voie sans réservoir (131072)** : la sémantique est intacte (toute feuille qui déborde de sa case est rejouée). Le **nombre** de feuilles qui débordent change, parce que la capacité d'une case entrelacée diffère de 128 enregistrements + 1 024 rangs :
   - gate de 3 000 sites, K10/24 : 555 → 532 (LINE mise à jour dans `tests/tower/tests.cmake`) ;
   - ng00 K10/24 : 4 196 → 4 095 ;
   - ng00 K5/16 : 14 → 1.

## 4. Portes jouées (local, Release u21, hôte seul)

| porte | résultat exact |
|---|---|
| `ctest -R "full_leaf_lanes\|catalogue_(cache\|single_pass\|leaf\|small_pair\|parallel)"` | `100% tests passed, 0 tests failed out of 32` |
| `full_leaf_lanes.py` (sous `python3 -S -O` aussi) | `full_leaf_lanes_verdict conforme sites3000 k5 boules212695 lot27048 non_resolues0 rejouees0 copiees23766 k10 boules1075008 lot35957 non_resolues0 rejouees0 copiees35131 rejouees_sans_reservoir532` |
| `tools/check_style.py` | `style_ok fichiers=531` |
| `run_mutants.py --check` | `manifeste_ok module=tower mutants=151 plancher=151` |
| ng00 K5/16, masques 16379, 49147 et 180219, 2 fils, A et base | dumps tous `3a2bfb4f9f48…`, `catalogue_work` identique au CPU |
| ng00 K10/24, mêmes masques | dumps tous `61a4245b91d9…`, `catalogue_work` identique au CPU |
| 3 répétitions appariées supplémentaires (12 exécutions par variante) | mêmes dumps, un seul registre par configuration |

## 5. Mutants (`tests/mutants/tower.json`, plancher 148 → 151)

J'ai remplacé les quatre mutants du puits de la base, dont les motifs ont disparu, et j'en ai ajouté trois. Commande : `run_mutants.py --only … --floor 7 --jobs 2 --build-jobs 2`, Release, u21. Résultat : `mutants_ok module=tower mutants=7 tues=7 dont_signal=0 dont_delai=0 dont_construction=0 plancher=7`.

| id | cible | verdict |
|---|---|---|
| rang_local_interieur_faux | rangs de l'intérieur dans la case | TUÉ (code) |
| rang_local_coquille_faux | rangs de la coquille dans la case (nouveau) | TUÉ (code) |
| bloc_entrelace_sans_entete | place suivante sans les 8 octets d'en-tête (nouveau) | TUÉ (code) |
| bloc_entrelace_marge_debordee | marge du puits dépassée, déborde dans le bloc voisin (nouveau) | TUÉ (code) |
| reservoir_chaine_perdue | bloc pris non chaîné | TUÉ (code) |
| reservoir_pas_de_chaine | règle de changement de bloc de la copie différente de celle du puits | TUÉ (code) |
| reservoir_epuisement_ignore | un bloc de plus que `spare` (hors arène sans réservoir) (nouveau) | TUÉ (code) |

Aucun mutant ne vise les refus défensifs de `copy_scratch`, qui ne sont atteignables que sur corruption. Les mutants de corruption ci-dessus les traversent ; je n'ai pas cherché à savoir s'ils sont tués par le dump ou par le refus.

## 6. SASS (nvcc 12.9, sm_120, Release, u21)

| noyau count | instr | BSSY | CALL | registres | pile | LDL | STL | LD | ST | ATOM | YIELD |
|---|---|---|---|---|---|---|---|---|---|---|---|
| j2memo 34a8a561d | 10352 | 107 | 0 | 166 | 4040 | 344 | 290 | 241 | 113 | 0 | 0 |
| base 905fad2e1 | 11832 | 134 | 3 | 164 | 4064 | 358 | 311 | 240 | 145 | 3 | 0 |
| base + `__syncwarp` seul | 10736 | 118 | 3 | 164 | 4144 | 358 | 311 | 240 | 145 | 2 | 0 |
| A sans `__syncwarp` (a2) | 11520 | 128 | 0 | 164 | 4064 | 344 | 293 | 178 | 122 | 3 | 2 |
| **A (livré)** | **10352** | **112** | **0** | **164** | **4064** | 344 | 293 | 178 | 122 | 3 | 2 |

Le noyau fill est inchangé (9 920 instructions, 93 BSSY, 168 registres). `copy_kernel` utilise 40 registres.

BSSY de A par rapport à j2memo, ligne par ligne :
- +3 pour le changement de bloc (un par site) ;
- +3 pour le test `block == kNoChunk` (contre `fits && …`) ;
- +3 pour la boucle de coquille ;
- +2 pour `local_rank` ;
- −9 sur d'autres lignes.

## 7. Mesures hôte appariées (descriptives)

Machine partagée par quatre agents, charge 7 à 12 sur 8 cœurs. Les temps murs sont donc dispersés du simple au double. Valeurs en ms (count_ns du lot), 3 répétitions en ordre alterné, 2 fils, ng00 :

| configuration | base (r1, r2, r3) | A (r1, r2, r3) |
|---|---|---|
| K5/16 49147 | 7456, 3744, 4927 | 4988, 6184, 6061 |
| K5/16 180219 | 6026, 7009, 5383 | 5645, 3742, 5435 |
| K10/24 49147 | 22341, 14839, 22415 | 28849, 18865, 25798 |
| K10/24 180219 | 26494, 18006, 26619 | 24966, 18275, 26858 |

Ces temps murs ne permettent aucune conclusion. Mesure plus stable : le compte d'instructions callgrind de `HostBatch::body` (comptage et copie, 1 fil), sur le nuage de la porte (3 000 sites), K10/24, 49147. Base : 21 608 362 878. A : 21 503 862 409, soit **−0,48 %**. Les dumps sont identiques (`238737c5a3a7`).

Volumes du lot ng00 :

| configuration | base | A |
|---|---|---|
| K5/16 avec réservoir | rejouées 0, copiées 138 490, blocs 14 | rejouées 0, copiées 138 490, blocs 1 |
| K10/24 avec réservoir | rejouées 0, copiées 228 759, blocs 4 088 | rejouées 0, copiées 228 759, blocs 4 386 (sur 66 346 de réservoir) |

## 8. Effet attendu sur G4 : ESTIMATION, non mesurée

Le SASS du parcours de A est celui de j2memo à +5 BSSY près, plus un ATOM prédiqué par site d'émission (joué seulement au changement de bloc) et 2 YIELD. Si l'écart de la base à j2memo vient bien de l'appel et du réarrangement qu'il impose (seule différence structurelle restante), alors on peut attendre :
- **comptage** : K5/16 d'environ 39 à 30–33 ms ; K10/24 d'environ 226 à 172–185 ms ;
- **écriture** : ~0,3 ms (copie seule, aucune feuille rejouée), comme la base ;
- **domain GPU** : K5/16 environ 210 ms (contre 218), K10/24 environ 570 ms (contre 618).

Ce chiffrage reste incertain : le compte statique s'est déjà montré trompeur, puisque reservoir2 avait moins de BSSY et était plus lent.

Masques pour la session G4 (base 905fad2e1 contre A, appariés, ordre de Williams) :
- ng00, ng01, ng02 ;
- `81915` (lot GPU avec réservoir) et `212987` (= 81915 + 131072, sans réservoir) ;
- K5/16, K5/24, K10/24 ;
- référence CPU `16379`, mêmes dumps exigés.

Comparer `leaf_batch.count_ns`, `fill_ns`, `fill_jobs`, `copied_jobs`, `spare_record_chunks` et `domain_ns`. Critère de succès : comptage A ≤ 1,05 × j2memo, avec `fill_jobs = 0` à K10/24 en 81915.

## 9. Risques

- **Non mesuré sur GPU.** Les 2 YIELD sont insérés par ptxas parce qu'il y a un atomique dans la boucle du parcours. Ils sont absents de j2memo et de la base (où l'atomique est dans la fonction appelée). Leur effet sur l'ordonnancement divergent est inconnu, a priori faible.
- **Sémantique des champs de sonde** : `spare_record_chunks` et `spare_population_chunks` portent désormais le même compte (un seul genre de bloc), documenté dans `LeafBatchTimings`. Les reçus qui les additionneraient seraient faussés. `gpu_sanitizer.py` n'exige que `spare_record_chunks > 0` et reste valide.
- **Capacité d'une case** :
  - à K10, environ 123 émissions de 16 octets contre 128 ; plus de blocs pris (4 386 contre 4 088 sur ng00), sans conséquence ;
  - à K5, la capacité augmente (environ 160 contre 128).
- **Voie 131072** : le nombre de feuilles rejouées change (porte mise à jour 555 → 532). Les reçus antérieurs ne sont pas comparables feuille à feuille sur ce point.
- **`__syncwarp()`** suppose que les 32 fils du bloc atteignent la réduction. C'est déjà exigé par `__shfl_down_sync(0xffffffff)` et c'est vrai aujourd'hui : aucun retour anticipé dans `count_kernel`. Un retour anticipé ajouté plus tard serait une faute dans les deux cas.
- **Écritures non alignées** : les enregistrements sont écrits octet par octet (`encode_bytes`), comme `encode` le faisait déjà au SASS (`ST.E.U8`).
- **Hors levier, noté** : `encode` refait 4 recherches linéaires `local_rank` (4 BSSY et jusqu'à 4·m chargements par émission). Or le support émis vaut toujours `sites[prefix[0..q-1]]` avec `qmin = q` (`census_and_emit` rejette tout support différent du générateur). Les rangs locaux sont donc `prefix[i]`, et les passer au puits retirerait ces recherches, au comptage comme à l'écriture. C'est à évaluer séparément, car cela touche l'interface de `leaf_device.hpp`.
