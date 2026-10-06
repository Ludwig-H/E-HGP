# Plan final d'optimisation GPU et CPU de la tour FULL v11

Rédigé le 6 octobre 2026, de 01 h 46 à 02 h 05 UTC (`date -u`). Lecture seule au commit `cf5da0e91` (origin/main,
moteur identique à `df904711a`). Aucun build, aucun commit, aucune branche. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 (voie de banc mesurée)
profile=quantized_u21_input_only
public_status=not_claimed
```

Ce plan remplace `PLAN_GPU.md`. Il tient compte de `CRITIQUE_PLAN_GPU.md` et de données relues dans les reçus
`gpu_g4` (champs `cpu_seconds` par prise et par passe, non exploités jusqu'ici).

Étiquettes des chiffres :

- **M-G4** : mesure lue dans un reçu G4 ;
- **D-G4** : valeur dérivée de mesures G4 par un calcul que j'écris ;
- **M-loc** : mesure sur le codespace (8 cœurs) ;
- **E** : estimation, jamais une mesure.

## 0. Verdict

1. **100 ms n'est pas atteignable avec les leviers identifiés.** Le meilleur chemin chiffré donne environ 160 à
   240 ms (E) pour `domain + tree` à K5, W48, en régime résident. Aujourd'hui : 386 / 319 / 382 ms (M-G4, FIN). Il
   ne passe sous 100 ms que si deux inconnues se lèvent :
   - le coût par pas des descentes est divisé par 3 (R ≤ 35–40 ms) ;
   - le domaine tombe sous 55 ms.

   Aucune des deux n'a de mécanisme identifié. Je les écris donc comme des **conditions**, pas comme des
   estimations. Sur ce point, je rejoins la critique.
2. **Le plancher de travail domine.** À W1, ng00 coûte 9,05 s de CPU (M-G4, AB). La machine donne au plus 24 à 31
   cœurs-équivalents (domaine ×23–24, passe unique ×30,5 de W1 à W48, M-G4). Le CPU seul ne descend donc pas sous
   290 à 380 ms sans retirer du travail. Le GPU sert à **délester du travail**, pas à gagner du parallélisme. Cela
   vaut en latence comme en débit à 10 Hz.
3. **Le premier levier est CPU et exact : le surcoût par nœud du parcours des boîtes.** C'est un fait nouveau, tiré
   des reçus GPU (§ 1.2). À W48, chaque nœud du parcours coûte environ 6 à 7 µs de CPU, contre environ 1 µs à W1.
   Le temps du parcours suit le nombre de nœuds, pas les tests G1. La critique a raison sur ce mécanisme.
4. **L2 (parcours G1 sur GPU) quitte le rang 1.** Il ne revient que si, une fois le surcoût par nœud retiré, le
   parcours dépasse encore 50 ms à W48 et que le filtre G1 en fait la majorité.
5. **Le levier GPU utile est le délestage recouvert des feuilles (L4).** Il vient avec trois choses :
   - les portes manquantes (L0) ;
   - un noyau `fill` réparti sur plusieurs warps ;
   - à terme, la feuille coopérative J3.

   À K10, c'est le levier principal (feuilles de 415 à 535 ms sur CPU, M-G4). À K5, son gain dépend du rang 1 :
   une fois le parcours raccourci, l'exécuteur actuel (52 à 59 ms à chaud) devient plus long que le parcours.
6. **Hors contrat**, le plus gros gain mesuré est celui des feuilles de 24 dans l'API à K10 : −24 à −27 % de mur,
   A/B apparié (M-G4, diag1). Le dump est identique à 16, 24 et 32.

## 1. Arbitrage de la critique

### 1.1 Points de la critique que je retiens

| # | Point | Correction appliquée |
| --- | --- | --- |
| C1 | Double compte du domaine | « Préambule » = `prefix_ns` (17–22 ms), compté une fois. Résidu réel 12,6 / 12,3 / 14 ms (D-G4, AB diag1). Petits étages 52–63 ms. L6 ramené à −15/−35 ms (E). |
| C2 | L1 gonflé | Retenu V3 + V7 = −10 à −37 ms (E, `carte_forets` O3). « R ≤ 40 ms » devient une condition. |
| C3 | SHA-NI et `fsync` | `fsync` est compté dans `write` (`writer.cpp` l. 130). Gain annoncé −0,8 à −1,0 s (E), après chronométrage de `fsync`. Hors contrat. |
| C4 | Protocole trop maigre | 12 paires ABBA par trame à W48 ; décision sur la différence appariée et le test des signes ; W1 apparié pour le travail (§ 4). |
| C5 | Référence résidente | Le CPU se mesure aussi en résident (passes 2..P). Toute comparaison GPU/CPU se fait à régime égal. |
| C6 | Perte à froid de la voie GPU actuelle | +49 à +86 ms à froid (M-G4, S6) ; +30 à +50 ms à chaud. |
| C7 | K10 « 1,74 → 0,83–0,90 s » non apparié | Seul le −24 à −27 % de mur (diag1, même binaire) est retenu. |
| C8 | Découpage du `tree` non transportable | Les sous-étages R et queue sont ceux de diag1. Ils ne s'appliquent aux prises FIN qu'en ordre de grandeur. |
| C9 | Plancher de travail | Repris (§ 0.2) ; il ordonne le classement. |
| C10 | L3 surestimé | Gain E : −10/−20 ms sur ng00 et ng01, −30/−45 ms sur ng02. Placement par CCD et L3, pas seulement par SMT. Le bras « résolveurs −2 » publie R. |
| C11 | Bornes de profil | G1 en i64 jusqu'à B ≤ 29 ; feuille en i128 jusqu'à B ≤ 24 ; bornes J3 à refaire hors u18. La voie GPU refuse avant calcul au-delà de B = 24. |
| C12 | Fil hôte réservé pour L4 | Retenu : parcours à 47 fils quand le GPU est actif. |
| C13 | Repli forcé sur G4 | Porte `--inject=gpu_all_unresolved` exécutée en natif sur G4. |
| C14 | Pièges VM | Retenus (§ 4.3), sauf CMake 3.22 (voir § 1.2, D5). |

### 1.2 Points où je ne suis pas d'accord, et faits nouveaux

**D1 — Les feuilles CPU ne pèsent pas « 100 à 120 ms » à W48. Aucune soustraction ne tranche.**

La critique tire ce chiffre du profil W1 (AB7). La feuille y pèse environ 44 % du CPU du processus, le filtre G1
8,1 % (relu dans `ab_q3r/.../perf_new_self.stdout` : `extend` 17,35 % est bien la feuille de
`catalogue_detail`, pas `tower/meb.cpp`). Elle ajoute ensuite l'accélération moyenne de l'étage.

Les reçus GPU contiennent pourtant le temps CPU **par passe** (`cpu_seconds`) dans les deux modes. Je l'ai relu
(ng00, K5, feuilles de 16, médianes chaudes, M-G4) :

| Session | CPU, mode CPU | CPU, mode GPU | Écart |
| --- | ---: | ---: | ---: |
| S6 | 14,06 s | 12,73 s | 1,3 s |
| S4 | 14,24 s | 13,36 s | 0,9 s |

Retirer toutes les feuilles du CPU n'enlève donc que 0,8 à 1,3 s de CPU par passe, sur les trois trames (D-G4).
Si les feuilles pesaient 100 à 120 ms de mur sur 48 fils, l'écart serait de 4,8 à 5,8 s.

Ces deux sources se contredisent :

- le profil W1 donne environ 4,2 s de feuilles ;
- à W48, retirer les feuilles ne retire qu'environ 1 s.

Une explication est cohérente avec les deux, sans être démontrée : **sans feuilles intercalées, le parcours devient
plus contendu et brûle plus de CPU**. Les soustractions entre modes ne sont donc valides ni pour le plan (45 à
54 ms) ni pour la critique (100 à 120 ms).

Ce que l'on sait :

- la fourchette des feuilles à W48 va d'environ 27 ms (S6 ng01, D-G4) à environ 120 ms (profil W1, E) ;
- **seul un chrono par fil et par phase, mesuré à W48 sur G4, la ferme**. C'est l'objet de la tranche T1.

**D2 — Le fait qui tranche la nature du parcours : 6 à 7 µs de CPU par nœud à W48.**

Les données sont celles de S4, K5, mode GPU, médianes chaudes (M-G4).

| Feuilles | Nœuds | Tests G1 | Parcours (mur) | CPU par passe |
| ---: | ---: | ---: | ---: | ---: |
| 16 | 783 k | 379 M | 95 à 115 ms | 13,4 s (ng00) |
| 24 | 272 k | 248 M | 31 à 39 ms | 9,6 s (ng00) |

Avec 511 k nœuds de plus, on paie environ 70 ms de mur et 3,1 à 3,7 s de CPU, soit **6 à 7 µs par nœud** (D-G4).
Le profil W1 donne au contraire environ 1 µs par nœud pour le filtre, le réservoir et l'enveloppe réunis (0,8 s
pour 783 k nœuds, M-G4 AB7).

Le surcoût par nœud est donc multiplié par 5 à 7 à W48. Le code (`boxes.cpp` l. 60, `core/buffer.cpp` l. 15-48)
en donne un candidat. Chaque nœud fait :

- un `operator new` et un `operator delete` ;
- un CAS en boucle sur le compteur `used`, partagé par tous les fils ;
- un CAS sur `peak` ;
- un `fetch_sub`.

Autour, PROF1 à W48 relève 3,1 % de fautes de page, 1,55 % de verrou noyau et 1,51 % de
`__pte_offset_map_lock`. Ce sont des allocations de 160 Kio près de la racine, qui passent vraisemblablement par
`mmap`, avec les invalidations de TLB qui suivent.

L'autre candidat est la latence mémoire sous SMT : le parcours lit les coordonnées de façon dispersée. **La cause
n'est pas attribuée**, d'où T1.

**Conséquence.**

- Le levier est le surcoût par nœud sur CPU (N1), et non le filtre G1 sur GPU. Je suis d'accord avec la critique.
- Ce levier vaut plus que les « −2 à −8 ms » du contrat R3 dans l'audit des transpositions. Cette estimation
  venait d'un profil W1, qui ne voit pas ce surcoût.

**D3 — Les feuilles de 24 à K5 : l'identité est déjà établie, mais sur CPU seul c'est une perte.**

- **Identité.** Les dumps de S4 en feuilles de 24 à K5 sont égaux aux empreintes gravées (`3a2bfb4f9f48`,
  `5212a2ced81b`, `78feb765e21c`) dans les trois modes (cpu, lot, gpu), sur les trois trames (M-G4). Il n'y a rien
  à « vérifier » ; seul le registre change (nœuds, feuilles, tests).
- **Coût sur CPU seul.** En revanche, le mode CPU en feuilles de 24 **perd** :
  - passe unique 173 / 138 / 165 ms contre 152 / 122 / 144 à chaud ;
  - domaine +17 à +21 ms (M-G4).

Les feuilles de 24 à K5 ne valent donc qu'avec des feuilles plus rapides : J3 sur CPU, ou L4 avec J3 sur GPU. La
critique les présentait comme un levier presque gratuit ; il ne l'est pas sur CPU.

**D4 — T0 « local, sans G4 » ne suffit pas à décider.**

- Le codespace a 8 cœurs, et le phénomène à mesurer (D2) n'existe qu'à W48.
- En local, je retiens les compteurs déterministes : nœuds, somme des tailles de listes, octets alloués, c(L), part
  de chaque route par pas. Ils donnent les exposants, pas les temps.
- La décision de classement vient de la première session G4 (T1).

**D5 — CMake 3.22 n'est plus un piège.** `CMakeLists.txt` l. 88-91 force déjà `-std=c++20` pour nvcc quand CMake ne
connaît pas le dialecte CUDA20. Il suffit de garder ce contournement et de construire les nouvelles portes sur la VM.

**D6 — L4 « −20 à −40 ms à K5 » n'est vrai qu'avec le parcours actuel.**

- Après N1, le parcours vaudrait environ 30 à 45 ms (E). L'exécuteur actuel (52 à 59 ms à chaud, M-G4) deviendrait
  alors le chemin critique.
- À K5, L4 ne gagne donc qu'avec deux compléments :
  - un `fill` réparti (17,2 ms dans un seul warp aujourd'hui, M-G4) ;
  - puis J3 sur GPU.
- À K10, L4 gagne dans tous les cas, puisque les feuilles y valent 415 à 535 ms.

## 2. Référence corrigée (K5, W48, ms)

| Poste | ng00 | ng01 | ng02 | Source |
| --- | ---: | ---: | ---: | --- |
| domain | 221,6 | 179,9 | 230,5 | M-G4 FIN (processus neufs) |
| — prefix (= préambule) | 20,3 | 17 | 22 | M-G4 AB diag1 |
| — passe unique (parcours + feuilles, intercalés) | 163 | 144 | 175 | M-G4 AB diag1 |
| —— parcours seul, mode GPU, à chaud | 112–115 | 95 | 105–106 | M-G4 S4/S6 |
| —— feuilles CPU | non identifiable par soustraction (27 à 120) | | | D1 |
| — tri | 11,6 | 10 | 13 | M-G4 AB |
| — level_scan + assembly + compact | 13,5 | 12 | 14,9 | M-G4 AB |
| — résidu (allocation, table S*) | 12,6 | 12,3 | 14 | D-G4 AB |
| tree | 163,6 | 139,2 | 150,7 | M-G4 FIN |
| — contextes, classification, naissances | 17–24 | | | M-G4 diag1 |
| — R | 116–120 | 84–87 | 95–98 | M-G4 diag1 (autre binaire, `e49ea4690`) |
| — queue (médiane diag1) | 23,2 | 18,9 | 51,1 | M-G4 diag1 |
| **domain + tree** | **386** | **319** | **382** | M-G4 FIN |
| CPU par passe, résident | 14,06 s | 11,06 s | 13,04 s | M-G4 S6 |
| domain + tree résident CPU (passes 2..P) | 343–347 | 275–292 | 330–345 | M-G4 S4/S6 (mur du banc, `wall_ns`) |

Pour le travail logique de ng00 (379 M tests G1, 353 k feuilles, 4,80 M pas, 3,79 M MEB), voir `PLAN_GPU.md` § 1 ;
les chiffres sont inchangés et relus exacts par la critique.

## 3. Chemin chiffré vers 100 ms (ng00, K5, W48, résident)

Seule la colonne « aujourd'hui » est mesurée. Toutes les cibles sont des E.

| Poste | Aujourd'hui | Après N1 + L3 + L6 + L8 (CPU, E) | Après L4 + fill + J3 GPU (E) | Ce qui fonde la cible |
| --- | ---: | ---: | ---: | --- |
| prefix | 20 | 10–15 | 10–15 | rondes parallèles plus larges (L6) ; premier nœud de 40 000 sites filtré par un seul fil |
| passe unique | 150–163 | 100–125 | 40–60 | N1 : parcours 95–115 → 30–45 (feuilles de 24 : 31–39 ms à 272 k nœuds, M-G4) ; feuilles CPU intercalées 40–70 ; GPU : max(parcours, exécuteur) + queue |
| tri + scan + assemblage + compact + résidu | 38 | 25–30 | 25–30 | L6 |
| **domain** | **208–222** | **135–170** | **75–105** | |
| contextes, classification, naissances | 17–24 | 12–18 | 12–18 | L8 |
| R | 116–120 | 80–107 | 80–107 | L1 (V3 + V7, −10/−37) |
| queue | 23 (3–59) | 3–10 | 3–10 | L3 |
| **tree** | **139–164** | **95–135** | **95–135** | |
| **domain + tree** | **343–386** | **230–305** | **170–240** | |

Lecture :

- **Le `tree` décide.** Même avec un domaine instantané, le `tree` reste à 95–135 ms (E). Pour descendre à 100 ms,
  il faut R ≤ 35–40 ms, c'est-à-dire un coût par pas ramené de 600–700 à environ 200 ns.
  - La v10 tenait 188 ns, mais avec un census en flottant que l'audit des transpositions déclare non transposable.
  - La cause de l'écart avec la v10 n'est pas attribuée. C'est la **condition U1**.
  - Une piste existe, sans mécanisme prouvé : le profil W1 répartit R sur une quinzaine de fonctions (table de
    population 8,7 %, census 9 %, `find_support` 3,2 %). Aucun point chaud unique n'apparaît, ce qui plaide pour une
    restructuration (lots de descentes triés par mémoire) plutôt que pour une micro-optimisation.
- **Condition U2 : domaine sous 55 ms.** Il faudrait J3 sur GPU à ×3 au moins, non étayé, plus un parcours sous
  30 ms.
- **Recouvrement domain/tree.** Non instruit. Le `tree` lit le catalogue trié par niveaux. Un départ anticipé par
  ordre exigerait d'abord un lemme de complétude, avec sa fixture. Je ne le classe pas.
- **À froid**, ajouter le contexte CUDA : 69 à 78 ms seul, 136 à 183 ms en concurrence (M-G4). Une voie GPU à froid
  est hors contrat.
- **K10** n'a pas de chemin vers 100 ms : domain 0,74–0,85 s et tree 1,17–1,66 s (M-G4). On n'y vise que des gains
  relatifs.

## 4. Leviers classés

### 4.1 Contrat FULL K5 (gain sur `domain + tree`, résident, W48)

| Rang | Levier | Type | Gain (E) | Preuve | Exactitude | Dépend de |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | **N1** Surcoût par nœud du parcours : pile d'arène par ouvrier (LIFO, budget admis une fois), aucun atomique partagé par nœud, plus de `new`/`delete` par nœud | cpu | −20 à −60 ms domain | D2 : 6–7 µs par nœud à W48 contre ~1 µs à W1 (D-G4) ; parcours 31–39 ms à 272 k nœuds (M-G4) ; v10 sans allocation par nœud : boîtes et feuilles en 85–107 ms (M-G4) | aucune décision changée ; mêmes listes, même ordre ; registre identique | — |
| 2 | **L3** Placement des publieurs P5, P4, V5, V4 par CCD et L3, puis résolveurs ; flux compact des cellules | cpu | −10/−20 (ng00, ng01), −30/−45 (ng02) | queue de 2 à 59 ms ; P5 à 81–151 ms en pipeline contre 36–52 ms seul (M-G4) | ordonnancement seul | — |
| 3 | **L4 + fill + L0** Feuilles délestées au GPU pendant le parcours, file partagée, tampons épinglés, contexte ouvert à la Session, fil hôte réservé, `fill` à un warp par feuille | gpu | K5 : 0 à −40 selon N1 et J3 ; K10 : −250 à −350 de passe unique | exécuteur en série (`single_pass.cpp` l. 236, `batch_stage`) ; fill sur un seul warp, 17,2 ms (M-G4) ; identité stricte sur 372 + 84 prises | disposition par (tâche, tranche, ordinal) ; `unresolved` rejoué par `leaf.cpp` | L0 |
| 4 | **L1** Coût par pas de R : attribution par route, V3 (census par borne de réseau), V7 (mémo certifié) | cpu | −10 à −37 | R = 92 % du CPU de l'étage à W1 (M-G4) | même I/U, mémo certifié | — |
| 5 | **J3** Feuille par étages, source unique hôte et device | cpu puis gpu | CPU : −20 à −40 ; GPU : exécuteur 52–59 → 15–25 (E, non étayé) | ×1,371 sur la v10 (M-loc, ABBA, au plus 4 fils) ; 3,29 fils actifs sur 32 (M-G4) | bornes J3 à refaire en u21 et u24 ; mutants de feuille | N1 (pour mesurer proprement) |
| 6 | **L6** Petits étages du domaine : prefix plus large, tri LSD, résidu | cpu | −15 à −35 | prefix ×4,2 seulement de W1 à W48 (M-G4) | même permutation (repli exact) | — |
| 7 | **F24** Feuilles de 24 à K5 | cpu/gpu | parcours −60 ms, net seulement avec J3 | D3 : identité établie ; perte de +17 à +21 ms sur CPU seul (M-G4) | dump identique (M-G4) ; registre regravé | J3 ou L4+J3 |
| 8 | **L8** Contextes, classification et naissances en parallèle | cpu | −5 à −10 | 17–24 ms (M-G4) | — | — |
| 9 | **RES** Régime résident CPU : arènes réutilisées entre passes, pages pré-touchées ou huge | cpu | −5 à −45 (M-G4 avec biais : meilleure passe contre médiane) | fautes de page 3,1 %, verrou noyau 1,55 % (PROF1) | aucun | — |
| 10 | **L2** Parcours G1 sur GPU | gpu | ≤ −25/−40, conditionnel | filtre 8 % du CPU à W1 | ordre du réservoir, doublons de boules à vérifier | N1, puis parcours > 50 ms |
| 11 | **L7** Forêt exacte parallèle (Kruskal sur rangs) | gpu/cpu | 0 aujourd'hui ; −25/−40 si R < 50 | publication série 35–51 ms (M-G4) | inscription au registre des preuves d'abord | L1 |
| 12 | **L9** Pool de 52 à 56 fils | cpu | −3 à −9, bruit | — | — | L3 |

RES n'est pas un levier de gain classé comme les autres. C'est d'abord la **référence** de toute comparaison :
toutes les mesures se font en résident, des deux côtés.

### 4.2 Hors contrat (CLI, K10, sorties autres que FULL)

| Rang | Levier | Gain | Preuve |
| ---: | --- | --- | --- |
| A | Feuilles de 24 à K10 dans l'API | −24 à −27 % du mur K10 (M-G4, apparié diag1) | dumps identiques ; registre regravé ; décision utilisateur |
| B | L4 à K10 | −250 à −350 ms de passe unique (E) | exécuteur 330–360 ms en série (M-G4) |
| C | SHA-NI + `sync_file_range`, après chrono de `fsync` | −0,8 à −1,0 s sur write `full` ng00 (E) | C3 ; plancher disque environ 1,04 s |
| D | Vrai tampon d'écriture de 1 à 4 Mio | −0,1 à −0,3 s (E) | écritures de 4 Kio (M-loc, strace) |
| E | S11 pour `points` et `plat` | tree 138–184 → 65–90 ms (E) | ×8,6–10,9 seulement de W1 à W48 (M-G4) |
| F | `attach` et `output` en parallèle | −10/−20 chacun (E) | attach 58–80 ms en série (M-G4) |
| G | Colonnes groupées dans l'écriture de `points` | −100 à −200 ms (E) | 58 % du write en sérialisation (M-loc) |

## 5. Les trois premières tranches

Les tranches T1 et T2 partagent la session G4-A. T3 a sa propre session G4-B. Chaque tranche reste livrable et
mesurable seule.

### T1 — N1 : surcoût par nœud du parcours, avec chrono par fil (CPU, exact)

**Fichiers.**

- `src/core/buffer.hpp` et `buffer.cpp`. Ajouter `StackArena<T>` : un bloc par ouvrier, admis une seule fois au
  `MemoryBudget` à sa capacité majorée. Il expose `push(n)` et `pop(mark)`, en ordre LIFO, sans aucun atomique
  dans le chemin chaud.
- `src/catalogue/internal.hpp` : `ReadyNode::storage` devient une vue sur l'arène, plus une marque.
- `src/catalogue/boxes.cpp` :
  - `filter` écrit dans l'arène, au lieu de `storage.allocate` (l. 60) ;
  - `run_ready` libère la liste après les deux enfants, jamais avant ;
  - `make_root` est inchangé (racine possédée).
- `src/catalogue/single_pass.cpp`, `adaptive_frontier.cpp`, `adaptive_prepare.cpp` et `adaptive_replay.cpp`.
  `Workspace` porte l'arène. Son majorant est écrit dans `workspace_memory_bound`. La borne vient du commentaire de
  `walk` : au plus 3B+1 listes filtrées simultanées, chacune ≤ la taille de la racine de la tâche, plus le
  réservoir. Le majorant est donc `(3B+2) × max_task_root × 4` octets par ouvrier ; il est calculé avant le
  `parallel_for` et refusé (`memory_budget`) avant calcul s'il dépasse le budget.
- `src/catalogue/catalogue.hpp` : nouveaux champs de `CatalogueTimings`, hors registre :
  - `walk_thread_ns` et `leaf_thread_ns` : `CLOCK_THREAD_CPUTIME_ID`, lus seulement aux bornes de feuille, soit
    353 k feuilles × 2 lectures, environ 20 ms de CPU à W1 (E) ;
  - `arena_high_bytes`.

  Ils sont actifs seulement si `timings` est demandé. Les bras A/B les laissent éteints ; une passe séparée les
  allume.
- `bench/full_probe.cpp` et `bench/gpu_ab.py` : publient ces champs.

**Portes** (labels `gate` et `scale8000` ou plus).

- `single_pass_test` et `pipeline_equivalence` à W1, W4 et W48 ; dumps MHGP11FUL1 égaux aux trois empreintes K5
  et à `61a4245b91d9` (K10 ng00) ; `tree_k_sha256` égal.
- Registre du catalogue égal champ à champ : nœuds, `filter_tests`, feuilles, `max_depth`.
- Porte budget :
  - `arena_high_bytes ≤ majorant` sur les trois trames et sur uniform 8000, 16000 et 32000 ;
  - refus `memory_budget` avant calcul quand le budget est inférieur au majorant (aucun préfixe publié).
- TSan sous `setarch -R`, sur une trame réduite.
- ASan/UBSan local sur les tests de catalogue.

**Mutants** (au format de `tests/mutants/catalogue.json` : fichier, cherche, remplace, porte).

1. `arena_pop_before_right` : libérer la liste du parent avant `process(right)`. Le sous-arbre droit lit une liste
   écrasée ; la porte de dump et le registre doivent le tuer.
2. `arena_bound_minus_one` : majorant `3B+1` au lieu de `3B+2`. La porte budget à profondeur maximale doit le tuer.
   Il faut une fixture de profondeur ≥ 36 ; ng00 atteint 36 (M-G4).
3. `arena_shared_slot` : deux ouvriers sur la même arène (`slot = 0`). TSan et la porte W48 doivent le tuer.
4. `arena_no_rewind` : `pop` n'est jamais appelé. Le dépassement doit donner un refus `memory_budget` à
   l'admission, et non une corruption.

**Mesures locales** (M-loc, avant G4).

- Compteurs égaux.
- W1 apparié, 3 paires : aucune régression au-delà de 1 %, puisque le bruit A/A à W1 est de ±0,5 %.
- W8 : rapport de `walk_thread_ns` par nœud avant et après. C'est une tendance seulement, pas un temps de G4.

**Session G4-A** (`gcp-migration/v11_session.py`, 4 200 s, cible et zone explicites).

- Commande 0 : construction Release CPU et CUDA, fumée sur 3 000 sites.
- Bras sur un seul binaire, choisis par masque : A = parcours actuel, B = N1.
- **Bloc 1 : chronos par fil.** Une passe séparée, sans chronométrage du mur. A et B, mode CPU, trois trames, W48,
  K5. Résultat publié : `walk_thread_ns / nœud` et `leaf_thread_ns` à W48. Ce bloc ferme D1.
- **Bloc 2 : A/B du mur.** ABBA, 12 paires par trame, W48, K5 feuilles de 16, processus résident de 8 passes
  (médiane des passes 2..8 par processus), en modes CPU (16379) et GPU (81915).
  - Le mode GPU isole le parcours ; c'est la mesure propre de N1.
  - Un bras A/A est joué dans la session.
- **Bloc 3 : W1, une paire par trame.** CPU s par passe.
- Rapatriement intermédiaire après chaque bloc.

**Critère d'acceptation** (écrit d'avance).

- Identité : 100 % des prises ont le dump et le registre gravés.
- **Succès de N1** si les trois conditions sont réunies :
  1. sur les trois trames, en mode GPU, la médiane du parcours (`single_pass_ns`) est ≤ 65 ms, contre 95–115 ms
     aujourd'hui ;
  2. en mode CPU, la différence appariée médiane de `domain_ns` est ≤ −15 ms, avec au moins 10 paires négatives sur
     12 (test des signes, p ≤ 0,04) ;
  3. elle dépasse deux fois l'écart A/A de la session.
- **Échec** si le parcours en mode GPU reste au-dessus de 80 ms. L'hypothèse « allocation et atomiques » est alors
  réfutée, et le chrono par fil oriente vers la latence mémoire (ordre des sites, préchargement). L2 est réexaminé.

**Effort.** 1,5 à 2,5 jours-agent, plus une session.

### T2 — L3 : placement du pipeline `tree` et référence résidente (CPU, ordonnancement seul)

**Fichiers.**

- `src/sched/pool.cpp` et `sched.hpp` : affinité optionnelle par rôle (résolveur, publieur, suiveur), à partir d'une
  carte CCD et L3 lue au démarrage (`/sys/devices/system/cpu/cpu*/cache/index3/shared_cpu_list`). Désactivée par
  défaut.
- `src/tower/forest_concurrent.cpp` (pipeline des forêts) : rôle des tâches P5, P4, V5 et V4 ; publication du
  départ, de la fin, du CPU et de l'attente par tâche (diagnostic hors registre).
- `bench/full_probe.cpp` : options `--placement=none|ccd|ccd-r2|nice5`.

**Portes.**

- `pipeline_equivalence` et `tree_k_sha256` sur tous les placements.
- Porte `--placement` refusée proprement si l'affinité échoue : statut publié, jamais un échec silencieux.
- TSan sous `setarch -R`.

**Mutants.**

- `placement_publisher_skips_order` : le publieur saute une cohorte. Le dump et `tree_k_sha256` doivent le tuer.
  Ce mutant montre que le placement ne touche pas les décisions.
- `placement_clock_decides` : un choix de route lu sur l'horloge. La porte W1 contre W48 doit le tuer.

**Session G4-A** (même session que T1, bloc 4).

- `lscpu -e` et la carte L3 sont consignés.
- Williams sur quatre bras : none, ccd, ccd-r2 (résolveurs −2) et nice5, plus un bras A/A. 12 prises résidentes par
  bras et par trame, W48, K5.
- Publication : R, la queue, la médiane de l'étage, et le CPU de P5.
- Bras supplémentaire : CPU résident contre processus neuf, sur le binaire A. Il fixe la référence de régime de
  toutes les tranches suivantes.

**Critère d'acceptation** (écrit d'avance).

- Le placement retenu baisse la médiane de `forest_ns` d'au moins 10 ms sur ng02, avec au moins 10 paires sur 12.
- Il ne monte pas sur ng00 ni sur ng01 au-delà de l'écart A/A.
- R ne monte pas de plus de 3 ms.
- Sinon, le placement est abandonné et noté perdant.

**Effort.** 1 à 1,5 jour-agent.

### T3 — L0 + L4 + fill : voie GPU recouverte, portée et résidente (GPU)

Préalable : T1 est mesuré. Son résultat fixe la cible de K5.

**Fichiers.**

- `src/catalogue/single_pass.cpp` (`generate_single`, `batch_stage`). Chaque tâche soumet ses files de feuilles par
  tranches de 4 096 feuilles pendant le parcours, au lieu d'un lot unique après le `parallel_for`. L'ordre de
  disposition est fixé par (tâche, tranche, ordinal), jamais par l'ordre d'achèvement.
- `src/catalogue/leaf_batch_cuda.cu` :
  - un flux, deux tampons épinglés en anneau, budgétés au `MemoryBudget` (contrat R7) ;
  - `count_kernel` lancé par tranche ;
  - **`fill_kernel` à une feuille par warp, sur plusieurs blocs**. Aujourd'hui il tourne en une grille d'un seul
    bloc de 32 fils (17,2 ms à K5, M-G4) ; la cible est 2 à 4 ms (E, si la feuille la plus lourde coûte environ
    1,2 ms).
- `src/catalogue/leaf_batch_cuda_context.cu`. Le contexte s'ouvre à la construction de la Session, avec les modules
  préchargés (`cudaFuncGetAttributes`). Un fil hôte est réservé et le parcours tourne à 47 fils quand le GPU est
  actif. Les pics du pool device sont publiés (`used_high` et `reserved_high` valent `None` aujourd'hui).
- `src/catalogue/leaf_device.hpp` : refus explicite avant calcul si `kCoordBits > 24` (i128 en `5B+7 ≤ 127`).
- `src/api/` : rien dans cette tranche. La voie reste dans la sonde jusqu'à la décision n° 3 (§ 7).
- `tests/catalogue/` : nouvelles portes, label `gpu`, enregistrées seulement si `MHGP11_ENABLE_CUDA` est actif.
  Elles sont écrites en Python nu (3.10, `python3 -S`, sans numpy).

**Portes P1 à P10** (reprises de `carte_infra.md` § 4, complétées).

- P1 : dump et registre égaux au CPU, trois trames, K5 et K10.
- P2 : W1, W24 et W48, deux tailles de tranche.
- P3 : « répartition forcée » (tout CPU, tout GPU, alternance), même dump.
- P4 : `--inject=gpu_all_unresolved`, exécutée en natif sur G4 : repli complet, même dump.
- P5 : fixtures extrêmes de feuille (q3 extrême, q4 aux seuils 2^20 et 2^20 + 1, préfixe obtus, coquille qmin = 2).
- P6 : budget épinglé refusé avant calcul.
- P7 : compute-sanitizer `memcheck` et `racecheck` sur la fumée.
- P8 : la porte de profil refuse B = 25.
- P9 : sortie de ptxas consignée (registres, pile).
- P10 : euler K+2 en mode GPU.

**Mutants device** (tués par le jumeau hôte ou par la porte).

- `fill_warp_skips_leaf` : une feuille non remplie.
- `count_wrong_witness` : un témoin de dominance faux.
- `g3_equality_flipped` : l'égalité du préfixe retournée.
- `slice_order_by_completion` : disposition dans l'ordre d'achèvement.
- `unresolved_dropped` : feuille non résolue non rejouée.

Les deux derniers visent L4 lui-même.

**Session G4-B** (4 200 s). Commande 0 : construction CUDA sous CMake 3.22, puis fumée.

- Bras sur un seul binaire :
  - CPU : 16379, binaire N1 si T1 est accepté ;
  - GPU série actuel : 81915 ;
  - GPU recouvert : `gpu_overlap` ;
  - GPU recouvert avec `fill` réparti ;
  - un bras A/A.
- Résident, 12 paires par trame, K5 feuilles de 16 et K10 feuilles de 24, W48.
- Puis une passe nsys séparée (chevauchement sur la ligne de temps), une passe ncu séparée, et 7 prises à froid,
  en excluant la première après le démarrage.
- Mode persistance et horloges consignés (`nvidia-smi -q`).

**Critère d'acceptation** (écrit d'avance).

- Identité : 100 % des prises ; P1 à P10 verts ; les 5 mutants tués.
- **K10** : médiane appariée de `domain_ns` du GPU recouvert ≤ 0,85 × CPU résident, sur les trois trames, avec au
  moins 10 paires sur 12.
- **K5** : GPU recouvert ≤ CPU résident (binaire N1), sur la différence appariée médiane, sur les trois trames.
- **Si K5 échoue mais K10 réussit** : la voie est déclarée K10 seulement, et J3 sur GPU devient la tranche suivante.
- **`fill`** : `fill_ns` ≤ 5 ms à K5, sinon le chrono par feuille (clock64) est publié et l'hypothèse est révisée.

**Effort.** 4 à 6 jours-agent, plus une session.

Ensuite, selon les résultats :

- L1 (V3, puis V7), avec la cible −10/−37 ;
- J3 sur CPU et sur device, avec les bornes refaites en u21 et u24 ;
- F24 à K5 si J3 gagne ;
- L6 et L8 ;
- L2 seulement si le critère d'échec de T1 tombe ;
- L7 seulement si R < 50 ms et après inscription au registre des preuves.

## 6. Protocole commun

- **Régime.** Résident, des deux côtés : 8 passes par processus, médiane des passes 2..8, empreinte par passe. Le
  froid est mesuré à part et déclaré (contexte CUDA, première prise après démarrage exclue ou déclarée).
- **Plan.** Un seul binaire, bras par masque, ordre ABBA ou Williams, bras A/A dans la session.
  - Au moins 12 paires par trame pour un effet attendu sous 30 ms à W48.
  - W1 apparié (A/A ±0,5 %) pour les gains de travail.
  - Chrono de sous-étage pour ce qui reste sous le bruit.
- **Décision.** Différence appariée médiane, test des signes (au moins 10 sur 12) et écart supérieur à deux fois
  l'A/A. « Trois trames dans le même sens » n'est qu'un repère (p = 0,125).
- **Identité à chaque prise.** sha256 du dump égal à l'empreinte gravée (K5 `3a2bfb4f9f48`, `5212a2ced81b`,
  `78feb765e21c` ; K10 ng00 `61a4245b91d9`), registre égal, `unresolved` publié.
- **Tailles d'intérêt.** Uniform 8000, 16000 et 32000 en plus des trames, pour la porte budget de T1 et pour la
  pente de N1 (nœuds et µs par nœud). Pas de pente tirée des trois trames seules.
- **Instrumentation.** Toujours hors des bras chronométrés : passe séparée, comme Nsight.

### 6.1 Pièges VM

- Portes en Python nu (3.10, sans numpy, `python3 -S`).
- Première prise après démarrage (`device_init` 237 ms, prefetch 370 ms) : exclue ou déclarée.
- Mode persistance et horloges : `nvidia-smi -pm 1` et `-lgc` exigent root. À défaut, l'état est consigné.
- VM SPOT, `/tmp` vidé au redémarrage : rapatriement intermédiaire après chaque bloc, reprise par `--recover`.
- TSan sous `setarch -R`.
- Sessions ≤ 4 200 s. `TERMINATED` certifié sur la cible exacte après chaque session.

## 7. Décisions demandées à l'utilisateur

1. **Contrat de 100 ms.**
   - Contenu : `domain + tree` en mémoire, ou CLI complet ?
   - Régime : résident ou processus neuf ?
   - Grandeur : latence par trame, ou débit à 10 Hz ?

   Le plancher de travail (§ 0.2) s'applique aux deux grandeurs ; le débit retire seulement les queues d'Amdahl.
2. **Profil numérique visé.** u21 aujourd'hui ; u24 puis u32 ont été décidés le 30 septembre. La voie GPU est bornée
   à B ≤ 24. À u32, il faudrait une voie Wide sur l'appareil, ou le CPU seul.
3. **Voie GPU dans `src/api` et la CLI** : seulement après T3 accepté.
4. **Registre des voies parallèles.** Compteurs logiques (contrat D) ou physiques ?
5. **Feuilles de 24 à K10 dans l'API.** Gain mesuré −24 à −27 %, registre regravé.
6. **Registre des preuves** avant L7 : l'égalité « contraction des rangs égaux = plateau atomique ».
7. **SHA-NI** : détection par cpuid, ou option de construction.

## 8. Risques

| Risque | Effet | Parade |
| --- | --- | --- |
| N1 ne gagne pas : la cause est la latence mémoire, pas l'allocation | rang 1 perdu, chemin du § 3 décalé de +40 à +60 ms | chrono par fil du bloc 1 ; critère d'échec écrit ; piste ordre des sites et préchargement |
| Majorant d'arène faux (profondeur > 36, tâche racine plus grosse) | refus `memory_budget` en production | majorant calculé par tâche avant le `parallel_for` ; porte sur uniform 32000 ; mutant `arena_bound_minus_one` |
| U1 jamais levée (R par pas) | 100 ms inaccessible | annoncé dès maintenant (§ 0.1) ; le plan reste rentable à −100/−150 ms |
| Bruit de la VM SPOT et SMT | décisions fausses sous 30 ms | 12 paires, A/A, W1 apparié, chronos de sous-étage |
| Préemption SPOT | blocs perdus | rapatriement par bloc ; sessions courtes |
| Ordre d'achèvement GPU introduit dans la disposition (L4) | dump différent selon la charge | disposition par (tâche, tranche, ordinal) ; mutant `slice_order_by_completion` ; P3 |
| Le GPU recouvert prend un fil au parcours | parcours plus lent | fil hôte réservé, parcours à 47 fils, mesuré dans T3 |
| Profil u24/u32 | voie GPU invalide au-delà de B = 24 | refus avant calcul (P8) ; décision n° 2 |
| Bornes J3 écrites pour u18 | faux certificat | J3 bloqué tant que les bornes u21 et u24 ne sont pas réécrites et gravées |
| Instrumentation dans le chemin chronométré | gains fantômes | champs actifs seulement en passe séparée |
| Mémoire épinglée hors budget | dépassement du MemoryBudget | contrat R7 : `new` du GPU compté au budget ; P6 |

## 9. À ne pas faire

Je reprends `PLAN_GPU.md` § 6, avec ces corrections :

- **Feuilles de 24 à K5 avec des feuilles CPU telles qu'elles sont aujourd'hui** : perte mesurée (+17 à +21 ms de
  domaine, M-G4, S4). C'est utile seulement avec J3.
- **Port du parcours G1 sur GPU (L2) avant N1** : la cible est vraisemblablement le surcoût par nœud, pas le filtre.
- **Voie GPU des feuilles en série, en processus neuf** : +49 à +86 ms de perte à froid (M-G4).
- **Décider à 7 prises ou à 5 paires sous 30 ms** : l'A/A atteint 9 % à W48.
- **Annoncer SHA-NI à −1,4 s** : le `fsync` borne le gain.
- **Agrandir les cases fixes du GPU, mémoire unifiée, MPS, graphes CUDA, SHA sur GPU, proposition flottante puis
  recertification, census ou descentes sur GPU, Borůvka CPU, union-find synchrone par plateau** : inchangé (voir
  `PLAN_GPU.md` § 6).
