# Carte — anatomie de la vitesse de la v10, et où la v11 la perd

4 octobre 2026, rédigé à partir de 11 h 53 UTC (heure lue par `date -u`). Carte pour les autres auditeurs de
l'audit des transpositions ; contexte commun : `../CONTEXTE.md`.

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (v11) ; quantized_u18_input_only (v10 mesurée)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Rappel du contrat (demande de l'utilisateur) : tour HGP FULL des trames SemanticKITTI sans sol, grille 1 mm,
moteur entier exact, **100 ms sur G4 à K = 5**, et si possible K = 10. Le jalon de 200 ms n'est pas le but.

## Étiquettes et sources

Chaque chiffre porte une étiquette : **[G4]** mesuré sur G4 (reçu cité), **[loc]** mesuré sur le codespace
(indicatif), **[est]** calculé par moi à partir de mesures citées (méthode donnée), **[conj]** conjecture
(mécanisme plausible, non mesuré), **[lu]** établi par lecture du code.

| Abréviation | Source (lecture seule) |
| --- | --- |
| S4 | `morsehgp3D_v10/receipts/g4_session4_j2c_20260929/results/cmd/0{05,07,08..13}_*/{stdout,time.txt}` (v10 `777406b82`, 48 fils), relu dans `build/v11-persist/audit_v10/preuves_l09_perf_lidar/TABLE_VERITE_G4.md` et `DERIVE_G4.txt` ; j'ai recontrôlé `008_tower_lidar00_k5_w48`, `010`, `012`, `009` |
| S1 | `morsehgp3D_v10/receipts/g4_session1_20260929/results/cmd/008_c0_lidar02_k5_w1/stdout` (v10 `8b8d66f6e`, tour à 1 fil ; code de la tour inchangé sur ce chemin, L06 § 1) |
| AB7 | `morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz` (git `origin/main`) : moteur v11 actuel `b87285378` (dernier commit qui touche `morsehgp3D_v11/src`, HEAD `c22be4e41`), cinq prises W48 et une prise W1 par trame, mode 16379, K = 1..5, u21, processus neufs. Médianes **recalculées par moi** sur les `t_new_lidar_ng0*_w*_r*.stdout` ; elles redonnent celles de la note d'audit du 3 octobre au soir (§ 6) |
| C40 | `morsehgp3D_v11/receipts/qualification_performance_20261003/review/analysis.md` (moteur `c40f40798`, 81 prises) |
| PROF1 | `.../pipeline_g4/sessions/claudeprof1/results.tar.gz`, `report_w{1,48}_{self,children}.stdout` : profil `perf` G4 de la base v11 `a45daff3a`, mode 16379, 08/000000 |
| NOTE1, NOTE3 | `morsehgp3D_v11/receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md.snapshot` et `NOTE_CLAUDE_AUDIT_V11_20261003.md.snapshot` |
| DEEP | `morsehgp3D_v11/receipts/audit_deep_20261004/performance/README.md` (relecture indépendante du 4 octobre) |
| ECART | `morsehgp3D_v11/receipts/developpement_20261003/ecart_v10_v11/README.md` et `sim.txt` |
| L05, L06 | `build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md`, `L06_CODE_TOUR.md` (audit v10 du 2 octobre) |
| REPRISE | `morsehgp3D_v11/audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (§ « Ce que les mesures G4 prouvent ») |

Trames : 00 = 08/000000 (39 885 sites), 01 = 08/000100 (35 551), 02 = 08/000200 (45 845). Les octets XYZ
v10 et v11 sont identiques (DEEP, empreintes `0baa…`, `ba15…`, `a4bb…`). Les lignes de code v10 citées sont
celles de l'extraction `/workspaces/E-HGP/morsehgp3D_v10/` : `generator.cpp` et `site_tree.cpp` y sont
identiques à `777406b82` ; `tower.cpp` en diffère de 21 lignes (recherche de rang) [lu, `git diff --stat`].

---

## 0. Résumé

1. **La v10 rapide est `777406b82`** (session G4 4, 29 septembre) : catalogue + tour FULL 1..K, sans
   attaches, troisième passe chaude d'un même processus, u18, 48 fils : **204,2–253,6 ms à K = 5** et
   **861,4–1 124,6 ms à K = 10** [G4, S4]. Préparation (tri de Morton, `SiteTree`, Pool) exclue : 6,6–8,3 ms.
   Le raccord R2 `865f5e6`, source de la v11, n'a jamais été chronométré sur G4.
2. **La v10 n'a jamais approché 100 ms** : ×2,0–2,5 au-dessus à K = 5, ×8,6–11,2 à K = 10 [G4]. Son CPU par
   passe vaut au plus 6,9–8,6 CPU·s à K = 5 [est, S4] : même parfaitement parallèle sur 48 fils, son plancher
   serait 143–178 ms.
3. **La v11 actuelle** (`b87285378`) : **412,4 / 351,7 / 380,7 ms** à K = 5, W48 [G4, AB7] ; 13,7 / 10,3 /
   13,0 CPU·s [G4]. Écart ×1,50–1,72 avec la v10 (captures non appariées). K = 10 : **jamais mesuré**.
4. **Même objet, même travail logique.** Boules (1 306 696 sur 00), nœuds par ordre (somme 1 541 750),
   cellules (2 164 763), traces (3,62 M), census (≈ 0,29 M), nœuds, feuilles, jugements et candidats q4 du
   catalogue sont égaux ou à quelques pour cent près [G4, S4 et AB7]. La v10 n'était pas plus rapide parce qu'elle calculait moins.
5. **Le parallélisme moyen est le même** (CPU/mur ≈ 33–34 pour la v10, 29–34 pour la v11) : l'écart est
   d'abord du **CPU par unité de travail**, concentré dans deux étages (W48, K = 5) :
   - **feuilles du catalogue** : passe unique v11 159,5–195,0 ms contre étage des boîtes v10 84,8–106,7 ms
     (+58 à +88 ms) ; ×1,27 de CPU à W1 sur 02, et passage à l'échelle ×22,7–28,0 contre ×34,0 ;
   - **descentes** : résolution régulière v11 86,6–116,6 ms contre 22,2–28,6 ms (+64 à +88 ms) ;
     597–714 ns par pas à W1 contre 188 ns (×3,2), pour 8 % de pas en plus.
   Kruskal/verticales sont **plus rapides** en v11 (pipeline : queue de 21–34 ms contre 36–49 ms) ;
   frontière, tri, assemblage, atlas/naissances sont à parité.
6. Mécanismes v10 qui expliquent l'écart (§ 2) : boucles chaudes sans compteurs vérifiés ni `Result`,
   niveau q3 calculé après admission, table des triplets vivants, aucune réservation atomique partagée par
   nœud, `SiteTree` k-d à filtre flottant pour les boules fermées, mémo partagé par cellule. Plusieurs sont
   déjà portés en v11 (semis H_K → table de populations, tri à clés flottantes, LPT, `live2`, SWAR).
7. **Ce que cela dit des 100 ms** (§ 5) : rattraper la v10 ramènerait la v11 vers ~250 ms ; avec tous les
   leviers fondés de l'audit du 2 octobre, l'estimation v10 descend à 124–155 ms à K = 5 [est, L05/L06].
   Ni la v10, ni sa transposition ne donnent 100 ms : il faut réduire le **travail** (candidats par boule,
   coût d'un census, pas non terminaux), pas seulement le recopier mieux.
8. Ne doit pas revenir (§ 4) : marges flottantes figées pour u18 et dépendantes de l'arrondi, frontière en
   largeur à barrières, Kruskal séquentiel par ordre et naturalité complète en produit, coquilles étendues
   par énumération brute, boule fermée entière à chaque saut, entrée `cover` dépendante du rang de Morton,
   mémoire hors budget, course du Pool, portes trop étroites (mutants survivants).

---

## 1. Les étages de la v10 et leurs temps mesurés sur G4

### 1.1 Protocole des mesures v10 [lu, S4]

- Commande : `mhgp10_tower <trame> --k=K --threads=48 --no-points --repeat=3` ; chiffres = **dernière** des
  trois passes (`passes_catalogue_s`, `passes_tower_s`), dans le même processus. Les premières passes valent
  259,8 / 218,6 / 265,5 ms à K = 5 : l'échauffement ne pèse que 3–7 %.
- VM `g4-standard-48` (EPYC 9B45, 24 cœurs, 48 fils), g++ 11.4, `-O3 -DNDEBUG` **sans `-march`**, u18,
  `T = 6` bits sous-unitaires pour les boîtes. CPU seul.
- Chronomètre = catalogue + tour FULL 1..K (verticales comprises), **hors** lecture, préparation
  (6,6–8,3 ms) et sortie. La v11 mesure FULL = index + domaine + forêts, dans des processus neufs (AB7).

### 1.2 K = 5, 48 fils, millisecondes [G4, S4]

| Étage v10 (code) | 00 | 01 | 02 |
| --- | ---: | ---: | ---: |
| préparation, hors chrono (`prepare_s`) | 7,2 | 6,7 | 8,1 |
| frontière en largeur (`generator.cpp:675-746`) | 21,8 | 23,2 | 23,0 |
| boîtes : tâches, filtre des nœuds et feuilles (`567-620`, `319-465`) | 106,7 | 84,8 | 101,5 |
| ordre : clé flottante, tri, bandes (`751-801`) | 11,2 | 9,2 | 12,6 |
| assemblage : comparaisons, rangs, copie (`802-873`) | 16,9 | 14,8 | 19,5 |
| hors étages : destruction des tampons | 6,9 | 4,9 | 7,7 |
| **catalogue** | **163,5** | **136,9** | **164,3** |
| index des supports `t_prepare` (`tower.cpp:78-128`, `1164-1176`) | 2,8 | 2,3 | 3,3 |
| atlas, structures locales `t_local` (`1164-1400`) | 4,6 | 4,4 | 5,0 |
| semis H_k `t_seeds` (`1403-1435`) | 3,0 | 2,6 | 3,2 |
| descentes `t_resolve` (`799-993`, boucle `1441-1494`) | 28,6 | 22,2 | 27,1 |
| Kruskal par plateaux, un ordre par tâche (`1038-1116`, `1498-1516`) | 31,7 | 20,9 | 27,5 |
| verticales (`1658-1747`) | 17,6 | 14,8 | 21,5 |
| **tour FULL** | **88,5** | **67,3** | **89,3** |
| **catalogue + tour** | **252,0** | **204,2** | **253,6** |

Plancher séquentiel de la tour (Kruskal + fusions verticales de l'ordre le plus chargé) : 46,7 / 33,3 /
46,2 ms, soit 49–53 % de la tour [G4, S4 ; L06-04].

### 1.3 K = 10, 48 fils, millisecondes [G4, S4]

| Étage v10 | 00 | 01 | 02 |
| --- | ---: | ---: | ---: |
| frontière | 37,3 | 42,0 | 38,3 |
| boîtes | 471,0 | 368,2 | 435,0 |
| ordre | 48,5 | 38,2 | 47,3 |
| assemblage | 75,1 | 61,4 | 74,7 |
| hors étages | 20,7 | 17,7 | 22,8 |
| **catalogue** | **652,6** | **527,5** | **618,1** |
| index des supports | 11,1 | 8,3 | 11,0 |
| atlas | 29,9 | 23,4 | 29,7 |
| semis | 21,1 | 16,3 | 20,7 |
| descentes | 242,9 | 179,3 | 203,4 |
| Kruskal | 90,4 | 52,4 | 66,4 |
| verticales | 64,8 | 47,5 | 64,1 |
| **tour FULL** | **472,0** | **333,9** | **406,2** |
| **catalogue + tour** | **1 124,6** | **861,4** | **1 024,3** |
| boules | 5 512 670 | 4 383 302 | 5 483 320 |

Plancher séquentiel à K = 10 : 139,9 / 88,7 / 116,1 ms [G4, S4 ; L06-04] — à lui seul au-dessus de 100 ms
sur deux trames.

### 1.4 Un fil, passage à l'échelle et CPU [G4]

| Trame 02, K = 5 | 1 fil | 48 fils | Accélération | Source |
| --- | ---: | ---: | ---: | --- |
| catalogue (processus `mhgp10_catalogue`, passe froide) | 4 039,6 | 171,1 | ×23,6 | S4 |
| dont boîtes | 3 508,5 | 103,3 | **×34,0** | S4 |
| dont frontière | 55,9 | 26,6 | ×2,1 | S4 |
| dont ordre / assemblage | 163,9 / 307,1 | 13,8 / 18,7 | ×11,9 / ×16,4 | S4 |
| tour FULL | 1 154,4 | 89,3 | ×12,9 | S1 (1 fil), S4 (48) |
| dont descentes | 851,7 | 27,1 | ×31,4 | S1, S4 |
| dont Kruskal / verticales | 77,6 / 102,9 | 27,5 / 21,5 | ×2,8 / ×4,8 | S1, S4 |

- Gain SMT 24 → 48 fils : boîtes ×1,75 (K = 5), ×1,61 (K = 10) ; descentes ×1,74 [G4, DERIVE_G4].
- K = 10 à 1 fil (02) : catalogue 16 761,7 ms, dont boîtes 14 740,7 [G4, S4].
- CPU par passe (user + sys du processus de trois passes, divisé par trois, préparation et destruction
  comprises, donc majorant) : **8,55 / 6,88 / 8,33 CPU·s à K = 5**, 43,4 / 33,7 / 39,7 CPU·s à K = 10
  [est, depuis `time.txt` de S4]. Parallélisme moyen CPU/mur ≈ 33–34 (K = 5), ≈ 39 (K = 10) [est].
- Pic RSS du processus (trois passes) : 728 Mo à K = 5 et 3,21 Go à K = 10 sur 00 [G4, S4 ; L06-07] ;
  270–340 octets par boule au pic, catalogue entièrement hors budget [G4, L05 § 6.5].

### 1.5 Compteurs qui épinglent l'objet et le travail (trame 00, K = 5) [G4, S4 `008`]

1 306 696 boules ; 1 541 750 nœuds de tour ; 2 164 763 cellules ; 3 621 560 représentants résolus ;
4 399 127 pas de descente ; 3 111 762 arrêts sur un semis (85,9 %) ; **307 177 succès du mémo par cellule** ;
1 085 187 MEB (0 repli exact) ; 275 532 sauts K-NN ; 285 534 boules fermées par l'arbre ; 825 707 recensements
lus au catalogue. À K = 10 : 17,4 M représentants, 22,9 M pas, 15,0 M semis, 2,14 M succès du mémo, 7,66 M
MEB, 1,83 M boules fermées [G4, S4 `009`]. Catalogue (02, K = 5) : 734 083 nœuds, 323 224 feuilles, 404 M tests
du filtre, 18,9 M paires, 28,6 M triplets, 9,4 M quadruplets, 3,30 M jugements [G4, S4 ; TABLE_VERITE § 7].

---

## 2. Les mécanismes qui rendent la v10 rapide

« Effet » = ce qui est mesuré ; « v11 » = état du mécanisme dans `b87285378`.

### 2.1 Catalogue (`morsehgp3D_v10/src/catalogue/generator.cpp`, identique à `777406b82`)

| # | Mécanisme v10 [lu] | Effet mesuré | Dans la v11 ? |
| --- | --- | --- | --- |
| G1 | Boîtes de centres à listes K-certifiées, arbre binaire ajusté à l'enveloppe, coupe au milieu du plus long côté (J2c, `process` l. 567-620) | −33 à −40 % de nœuds, −54 % de feuilles [loc, L05 § 4] | **Porté** (`catalogue/boxes.cpp`) ; arrêt à largeur 1 (T = 0) au lieu de 1/64 de maille : mêmes boules, listes un peu différentes (783 071 nœuds contre 781 865 sur 00) [G4, AB7 ; DEEP] |
| G2 | Filtre des nœuds en forme D-loc i64, garde absorbée, comptage sans branchement, réservoir de 3K témoins (J2) | filtre ×3,4–3,8 (message du commit `5565f94fb`) [loc] | **Porté**, témoins prétraités par nœud (`boxes.cpp:48-82`) |
| G3 | Feuille **par étages** : masques de dominance, paires vivantes `live2`, **table des triplets vivants** `live3`, quadruplets par ET de trois lignes de triplets ; chaque droite de centres évaluée une fois par triplet (`enumerate_leaf_masks`, l. 319-465) | ≈ 31 M évaluations de droites sur 00 à K = 5 (23,9 triplets par boule) [loc, L05 § 6.1] | **Non porté** : DFS par préfixes avec lignes vivantes de paires et cache J2 ; 31,3 M évaluations **plus** 43,7 M consultations du cache, 75,0 M tests de droites [G4, AB7] (`leaf.cpp:79-96`, `198-259`) ; la table des triplets est écartée (PROVENANCE) |
| G4 | **Compteurs nus** dans la boucle chaude (`++L.led.pair_tests`, `leaf_dominance_tests += m(m-1)/2` une fois par feuille) ; prédicats rendant un `int` (`geom::side`) | — | **Non** : `MHGP11_TRY(checked_add(...))` par paire de dominance (`leaf.cpp:41`), par préfixe (`204`, `246`, `252`), quatre à cinq par test de droite (`84-93`), un par site recensé (`156`) ; `num::side` rend un `Result<int>` (`163`) |
| G5 | **Niveau q3 différé** : le juge ne construit que le centre (`center3`), le niveau exact vient après admission (`emitted_level`, l. 196-221, appelé l. 275) | — | **Non** : `Sphere::through(a,b,c)` construit le niveau de degré 6 avant `center_in_box` (`leaf.cpp:116-119`, `num/sphere.cpp:35-52`) ; 2,4–2,8 % du CPU FULL [G4, PROF1] ; piste retenue par REPRISE (« q3 différé ») |
| G6 | Liste par nœud en `std::vector` (un `malloc` par nœud), sorties en vecteurs propres au fil, **aucun compteur atomique partagé** | 0,2 % des instructions à 1 fil [loc, L05 § 6.4] | **Non** : `Buffer::allocate` par nœud (`boxes.cpp:60`) = CAS sur le compte partagé `used`, CAS du pic, puis `fetch_sub` à la libération (`core/buffer.cpp:19,25,47`) |
| G7 | Grain fin : cible 64P tâches, **19 482 tâches** à 48 fils, triées par charge décroissante | boîtes ×34,0 de 1 à 48 fils [G4, S4] | **Partiel** : plan « lourd d'abord » et LPT portés, mais au plus **1 024** ordinaux (`adaptive_frontier.hpp:8`) ; passe unique ×22,7–28,0 [G4, AB7] |
| G8 | Ordre canonique par clé `double`, tri parallèle par échantillonnage, bandes à 2^-40 et tri exact par bande (l. 751-790) ; tableaux non initialisés remplis en parallèle (`7eee86c53`) | ordre 9–13 ms à 48 fils [G4, S4] | **Porté** sous la doctrine F3/F4 (`sort_indices.cpp`) : tri 9,3–13,1 ms [G4, AB7] |

### 2.2 Tour (`morsehgp3D_v10/src/tower/tower.cpp`) et index (`src/cloud/site_tree.cpp`)

| # | Mécanisme v10 [lu] | Effet mesuré | Dans la v11 ? |
| --- | --- | --- | --- |
| T1 | **Semis H_k** : population triée d'une naissance régulière → naissance, consulté avant toute MEB (l. 752-753, 819-833, 1403-1435) | 85,9 % des représentants à K = 5, 86,5 % à K = 10 [G4, S4] | **Porté et étendu** : table I ∪ U → boule consultée à chaque pas, puis voie liée (rang, naissance) en tranche 3 : −26 à −30 % de la résolution régulière à W1 [G4, NOTE3] |
| T2 | Boucle parallèle unique sur les représentants de tous les ordres, **lots de 32 à empreintes précalculées et préchargement** (l. 1441-1494) | 188 ns par pas à W1 sur 02 [est : 851,7 ms / 4 532 640 pas, S1] | **Porté en partie** : préchargement en trois étages (tranche 2, −2 à −9 % à W4 [loc, ECART tranche 2]) |
| T3 | **Mémo partagé par cellule** (`atomic_ref` sur `atlas.val`, l. 951-973, 991) | 307 177 arrêts (7 % des pas) à K = 5, 2,14 M à K = 10 [G4, S4] | **Non** : mémos de lane à 2,6 % de succès, retirés du mode 16379 (NOTE3 § 4) ; d'où 4,80 M pas contre 4,40 M sur 00 [G4] |
| T4 | Recensement lu dans `BallInfo` quand le support certifié de la MEB est canonique (l. 851-896) | 825 707 recensements sans géométrie sur 00 [G4, S4] | **Porté** (`locate.cpp:79-84`, `find_support`) |
| T5 | **Boule fermée par arbre k-d** à boîtes entières serrées, distances en `double` à marge prouvée pour u18, exact seulement dans la bande (`site_tree.cpp:183-227`) | ≈ 26 % des cycles de `resolve` [loc, L06-05] ; ≈ 1 µs par requête [est : 26,3 % × 851,7 ms / 211 457 requêtes] | **Non** : index global neuf (arbre de plages Morton, boîtes réunies, census exact `power_bound_signs` i128 par nœud et `side` exact par site, `index/census.cpp:24-60`) ; 72 tests de points par census sur 00 [G4, AB7] ; `CensusWorkspace::query` = 10,3 % du CPU à W1 [G4, PROF1], ≈ 3,9 µs par census [est] |
| T6 | MEB proposée en `double` (Welzl) puis certifiée exactement, repli exact (l. 250-547) | 0 repli sur 7,66 M [G4, S4] ; 28,5 % des cycles de `resolve` [loc, L06] | **Remplacé** par une énumération exacte bornée, déjà bon marché : `bounded_meb` ≈ 1 % du CPU [G4, PROF1]. Rien à reprendre |
| T7 | Saut K-NN : k plus proches choisis en `double` à marge, exact sinon (l. 907-934) | — | **Autre politique** : k premiers de la liste intérieure ; descentes valides, longueurs pouvant différer (DEEP) |
| T8 | Kruskal par plateaux, un ordre par tâche, ordres concurrents ; images des naissances en parallèle | rapide à K = 5 (20,9–31,7 ms), mais plancher séquentiel | **Dépassé** : publication par plateaux recouverte par la résolution (pipeline), queue 20,8–34,1 ms [G4, AB7] |

### 2.3 Ce qui n'explique pas la vitesse v10

Cible ISA (les deux moteurs mesurés sans `-march` ; `x86-64-v3/v4` sans gain sur la v11 [G4, PROF1]),
index (0,35–0,42 ms en v11), échauffement (3–7 %), profil u18 contre u21 (u21/u18 = 1,054–1,061 sur le
catalogue [G4, DEEP]), MEB (moins chère en v11). Ni NUMA, ni bande passante ne sont démontrés (DEEP).

---

## 3. Comparaison étage par étage avec la v11 actuelle

### 3.1 K = 5, 48 fils, millisecondes : v10 S4 (dernière passe) contre v11 AB7 (médianes de cinq prises)

Les médianes v11 viennent de prises différentes : elles ne se somment pas exactement (C40, AB7).

| Étage | v10 00 / 01 / 02 | v11 00 / 01 / 02 | Écart v11 − v10 | Nature de l'écart |
| --- | --- | --- | --- | --- |
| préparation, index | hors chrono (7,2 / 6,7 / 8,1) | index 0,40 / 0,35 / 0,42 ; Cloud ≈ 1 ms hors FULL | ≈ 0 | — |
| frontière / préparation des préfixes | 21,8 / 23,2 / 23,0 | 20,7 / 18,4 / 20,8 | −1 à −5 | parité ; rondes du haut quasi sérielles des deux côtés |
| **boîtes / passe unique** | 106,7 / 84,8 / 101,5 | **195,0 / 167,0 / 159,5** | **+88 / +82 / +58** | CPU ×1,27 et passage à l'échelle moindre (§ 3.3) |
| ordre / tri | 11,2 / 9,2 / 12,6 | 11,7 / 9,3 / 13,1 | ≈ 0 | parité depuis les clés F3/F4 |
| assemblage + hors étages | 23,8 / 19,7 / 27,2 | assemblage 4,6 / 3,9 / 4,7 + compactage 4,8 / 4,2 / 5,2 + balayage des niveaux 4,9 / 3,7 / 4,9 + reste du domaine ≈ 10 (table des supports) | ≈ 0 | parité |
| **= catalogue / domaine** | 163,5 / 136,9 / 164,3 | **253,0 / 219,3 / 222,2** | **+90 / +82 / +58** | |
| index supports + atlas + semis | 10,4 / 9,3 / 11,5 | classification 2,6 / 2,3 / 2,7 + naissances 8,4 / 6,8 / 11,6 | ≈ 0 | parité |
| **descentes / résolution régulière** | 28,6 / 22,2 / 27,1 | **116,6 / 86,6 / 99,3** (délai jusqu'à la dernière résolution, pipeline) | **+88 / +64 / +72** | CPU par pas ×3,2 (§ 3.4) |
| Kruskal + verticales | 49,3 / 35,7 / 49,0 | queue de publication 33,5 / 20,8 / 34,1 ; queue des verticales ≈ 0 | **−16 / −15 / −15** | pipeline v11 meilleur |
| **= tour / forêts** | 88,5 / 67,3 / 89,3 | **171,0 / 133,0 / 157,7** | **+82 / +66 / +68** | |
| **FULL** | **252,0 / 204,2 / 253,6** | **412,4 / 351,7 / 380,7** | **+160 / +148 / +127** | ×1,64 / ×1,72 / ×1,50 |
| CPU (CPU·s) | ≤ 8,55 / 6,88 / 8,33 | 13,73 / 10,34 / 12,98 | ×1,61 / ×1,50 / ×1,56 | |

Pour mémoire, la qualification C40 (`c40f40798`, citée par `CONTEXTE.md`) donnait 489,1 / 345,1 / 432,4 ms,
avec domaine 227,5 / 181,1 / 238,6 et forêts 240,8 / 163,6 / 197,2 [G4, C40] ; la tranche 3 n'a changé que
les forêts. Avant toute optimisation (`ae817d09e`, mode 2047) : 1 463 / 1 155 / 1 515 ms [G4, DEEP].

### 3.2 Un fil, trame 02, millisecondes

| Étage | v10 | v11 (AB7, `_w1_r0`) | Rapport |
| --- | ---: | ---: | ---: |
| catalogue / domaine | 4 039,6 (S4) | 5 157,0 | ×1,28 |
| dont boîtes / passe unique | 3 508,5 | 4 461,5 | **×1,27** |
| dont frontière / préfixes | 55,9 | 85,9 | ×1,5 |
| dont ordre / tri | 163,9 | 317,0 | ×1,9 |
| tour / forêts | 1 154,4 (S1) | 3 309,2 | **×2,87** |
| dont descentes / résolution régulière | 851,7 | 2 918,5 | **×3,43** |
| dont Kruskal / publication | 77,6 | 148,7 | ×1,9 |
| dont verticales | 102,9 | 75,2 | ×0,73 |
| **total** | ≈ 5 194 [est : somme S4 + S1] | 8 466,6 | **×1,63** |

[G4 pour chaque case ; la somme v10 mêle deux sessions dont la tour a le même code sur ce chemin.]

### 3.3 Où la v11 perd dans le catalogue (+58 à +90 ms), et pourquoi

**Ce n'est pas du travail logique en plus** [G4, S4 et AB7] : sur 00, nœuds 783 071 contre 781 865,
feuilles 353 456 contre 352 967, jugements 3 269 620 contre ≈ 3,27 M (2,50 par boule), candidats q4 10,26 M
contre ≈ 10,2 M (7,8 par boule), évaluations exactes de droites 31,3 M contre ≈ 31 M. Ce qui diffère :

1. **CPU par unité (×1,27 à W1)**, causes lues et leur poids connu :
   - compteurs vérifiés et `Result` dans la boucle la plus chaude [lu, G4] : `extend` est le premier symbole
     du profil (14,5 % du CPU à W1, 15,5 % à W48 [G4, PROF1]) ; sur 00, 75,0 M tests de droites × 4–5
     `checked_add`, 120,4 M préfixes logiques, 37,0 M tests de census, 32,5 M tests de dominance
     [G4, AB7] → **≈ 0,5 milliard d'additions vérifiées par trame** [est] ; NOTE3 § 6 en compte ≈ 300 M dans
     `extend` seul. Poids temporel non isolé [conj : de l'ordre de 5–10 % de la passe unique] ;
   - niveau q3 construit avant le rejet par la boîte (G5) : `Sphere::through` 2,4–2,8 % du CPU [G4, PROF1],
     dont une part évitable non mesurée (REPRISE, DEEP) ;
   - DFS + cache de droites au lieu de la table des triplets vivants (G3) : 43,7 M consultations de cache en
     plus sur 00 [G4, AB7] ; `center_line_meets` 4,6–4,8 % du CPU [G4, PROF1] ;
   - compte logique des préfixes maintenu pour compatibilité des compteurs (`leaf.cpp:241-253`) [lu] :
     coût faible (deux `popcount`), mais travail sans effet sur la sortie ;
   - profil u21 : +5,4 à +6,1 % [G4, DEEP].
2. **Passage à l'échelle moindre** : passe unique ×24,4 / ×22,7 / ×28,0 de W1 à W48 [G4, AB7] contre ×34,0
   pour les boîtes v10 [G4, S4], sur la même machine. NOTE3 § 2 parle de « plafond SMT » : la v10 montre que
   l'étage équivalent dépassait ce plafond (×34 sur 24 cœurs). Indices mesurés à W48 [G4, PROF1, base
   `a45daff3a`] : `buffer_acquire` + `buffer_release` 0,86 % du CPU (absents au-dessus de 0,2 % à W1) ;
   fautes de page 3,1 % du CPU en cumul, verrou noyau `native_queued_spin_lock_slowpath` 1,55 % (et
   `__pte_offset_map_lock` 1,51 % en cumul : contention probable sur les tables de pages) ; chaque nœud fait deux opérations atomiques sur la même ligne de cache partagée (G6) ; grain de
   1 024 ordinaux au plus (G7), dont la simulation locale donnait pourtant un mur à 7 % de l'idéal
   [loc, ECART `sim.txt`]. Part de chaque cause dans les 30–90 ms : **non mesurée** [conj].

### 3.4 Où la v11 perd dans la tour (+64 à +88 ms de résolution), et pourquoi

**Même travail logique, à 8 % près** [G4] : traces 3 622 258 contre 3 621 560 représentants (00) ; census
291 515 contre 285 534 boules fermées (00), 208 111 contre 211 769 (02) ; pas 4 797 474 contre 4 399 127 (00),
4 889 688 contre 4 533 993 (02) — les ≈ 8 % de pas en plus viennent de l'absence du mémo (T3). Le
parallélisme de l'étage est le même (×29,4 contre ×31,4 de W1 à W48 sur 02). L'écart est donc un **coût
par pas ×3,2** : 597 / 637 / 714 ns par pas à W1 en v11 contre 188 ns en v10 [est, sur 02 : 2 918,5 ms /
4 889 688 et 851,7 ms / 4 532 640]. Profil de la base `a45daff3a` à W1 (avant la voie liée) [G4, PROF1] :

| Poste v11 (part du CPU FULL à W1) | Part | Équivalent v10 |
| --- | ---: | --- |
| `visit_located_part` (MEB bornée + `find_support` + census) | 18,2 % | MEB + recherche + boule fermée |
| dont `CensusWorkspace::query` (`power_bound_signs` 6,0 %, `bound_terms` 1,8 %) | 10,3 % | boule fermée k-d filtrée (T5) |
| `PopulationLookup::hit` (dont `find` 5,1 %) + `descend_each_step` propre | 8,7 % + 2,0 % | semis hachés en lots (T1, T2) |
| `resolve_job` propre | 7,6 % | — |
| `birth_node`, `find_support` | 2,8 %, 2,7 % | lecture directe de la naissance du semis |
| tenue des ledgers (`add_descent` 1,4 %, `cell_add` 0,6 %) | ≈ 2 % | compteurs nus |
| `bounded_meb` | 0,9 % (2,6 % avec enfants) | MEB `double` certifiée, ≈ 28 % de `resolve` [loc] |

Lecture : le census des descentes coûte ≈ 3,9 µs par appel en v11 contre ≈ 1 µs en v10 [est, § 2.2 T5] pour
le **même nombre** d'appels. Un filtre F6 des signes de `power` et de ses bornes, essayé, ne gagnait que ≈ 1 %
du CPU [G4, NOTE3 § 4, session `claudeab1`] : le surcoût vient du parcours (structure de l'index, nœuds et
sites visités, branches), pas de l'arithmétique i128 [conj, cohérent avec cette mesure]. La voie liée de la
tranche 3 a déjà retiré `birth_node` et la comparaison exacte des niveaux (−26 à −30 % de la résolution à
W1 [G4]).

### 3.5 K = 10

v10 mesurée : 861,4–1 124,6 ms [G4, S4]. **v11 : aucune mesure G4 à K = 10** (DEVELOPPEMENT.md, C40).
Transposition des rapports de K = 5 (catalogue ×1,35–1,6 ; descentes ×3,4–3,7 ; autres postes inchangés) :
de l'ordre de **1,5 à 2 s** [conj, aucune mesure]. À K = 10, la v10 consacrait déjà 179–243 ms aux seules
descentes et 89–140 ms à son plancher séquentiel.

---

## 4. Ce qui, dans la v10, ne doit PAS revenir

| # | Défaut ou risque | Preuve | Ce qu'il faut garder à la place |
| --- | --- | --- | --- |
| N1 | **Marges flottantes en constantes** prouvées pour u18 seulement (`kApproxMargin = 0.02`, `tower.cpp:27` ; marge de `SiteTree`), supposant l'arrondi au plus proche sans le vérifier ; garde `-ffast-math` contournable par `-Ofast` | L06-13 ; correctif R2 `0003` (« assumed round-to-nearest without checking it »), `0012`, `0023` ; le premier `SiteTree` R2 refusé par son vérificateur (`0002`) | Doctrine F1–F6 de la v11 (`docs/ARCHITECTURE.md` § 4) : borne par expression, repli exact, valide sous tout arrondi et toute contraction, portes aux quatre arrondis. Un filtre de type `SiteTree` ne se porte qu'ainsi |
| N2 | **Frontière en largeur à barrières** : ×2,1 de 1 à 48 fils, racine de 45 845 sites filtrée par un seul fil, 29 tours de barrière | L05-04 ; S4 | Filtre des grosses listes découpé sur les sites ; la v11 a encore ≈ 20 ms de préparation quasi sérielle (NOTE3 § 6), à traiter aussi |
| N3 | Portes du catalogue trop étroites : un mutant de la frontière pilotée par la charge **perd 2 134 à 9 523 boules** avec `status ok` et survit ; idem masques multi-mots, réparation des bandes ; ordre des S* à niveau égal non contrôlé | L05-03, L05-12 | Tout retour à un grain plus fin (G7) doit garder les portes W1/W4/W48 et l'oracle indépendant de la v11 |
| N4 | **Kruskal séquentiel par ordre** et **naturalité complète des verticales** dans le chemin produit : 33–47 ms (K = 5) et 89–140 ms (K = 10) de plancher | L06-04, L06-12 ; S4 | Le pipeline v11 (publication recouverte) ; contrôles de théorèmes dans les portes, pas dans le produit |
| N5 | Coquilles étendues par **énumération brute** : 24 points cosphériques, 49–57 s à K = 5, refus après 82 s à K = 10 | L06-06 | Budget a priori ou quotient polynomial ; sans effet sur le LiDAR à 1 mm (coquille ≤ 5) |
| N6 | **Boule fermée entière** à chaque saut (jusqu'à 1 258 sites sur une trame, la moitié du nuage sur une famille contrastée) ; seconde recherche vouée à l'échec ; juge de recensement 1/32 en produit | L06-05 | Requête saturante bornée (la v11 l'a : seuil `p < threshold`) ; juges hors produit |
| N7 | Entrée `cover` dépendante du **rang de Morton** aux ex æquo (47 sites changent de classe sous un échange d'axes) | L06-02 | Hors FULL ; la v11 publie l'ensemble cover puis nomme la projection |
| N8 | Mémoire **hors budget** (`std::vector`, `bad_alloc`), `Level` de 56 octets par rang, `support[4]` redondant, 270–340 octets par boule au pic | L05-06, L06-07 | Budget v11 ; mais **sans** opération atomique partagée par nœud (voir G6 et § 3.3) |
| N9 | **Course du Pool** : un ouvrier en retard pouvait exécuter une tranche deux fois ou avec la mauvaise fonction (`census_mismatch`) | correctif `8e3b76245`, reçu `pool_race_fix_20260929` ; R2 `0006` | Pool v11 et TSan ; tout ordonnanceur à grain fin repasse sous TSan et stress |
| N10 | u18 et `T = 6` câblés dans les bornes du chemin chaud ; `emitted_level` de compatibilité (boucle de degré 4 en la taille de la coquille U) ; chemin pondéré du générateur refusé par la tour | L05-15, L05 § 7, L05-16 | Budgets `constexpr` par profil de la v11 |
| N11 | Monolithes (`tower.cpp` 1 867 lignes, `build_tower` 601 ; `generator.cpp` 876) sans test par morceau ; juge FULL public qui acceptait des forêts fausses | L05 § 7, L06-09, L08 (via AUDIT_V10_SYNTHESE § 2) | Modules v11 ; un port de mécanisme v10 se fait par la fonction, jamais par copie de fichier |
| N12 | **Protocole de mesure** : troisième passe chaude d'un processus, build sans `-march`, captures non appariées. La vitesse v10 n'est pas une preuve d'objet : elle n'a pas de différentiel canonique intégral contre la v11 (seules les cardinalités sont égales) | S4 ; DEEP ; REPRISE | Banc apparié v10/R2 contre v11, mêmes XYZ, W1/W24/W48, processus froids et chauds, sorties canoniques (proposé par DEEP) |

Pas de piste fermée rouverte ici : aucun des mécanismes du § 2 ne matérialise la mosaïque de Delaunay
d'ordre supérieur ni un catalogue en C(n, k) ; le catalogue critique est l'objet commun des deux versions.

---

## 5. Ce que cette anatomie dit du contrat de 100 ms

1. **Budget CPU.** À parallélisme égal à celui observé (CPU/mur ≈ 29–34 sur 48 fils), 100 ms imposent au plus
   **2,9–3,4 CPU·s** par trame à K = 5 [est]. La v10 en consommait ≤ 6,9–8,6 CPU·s, la v11 10,3–13,7 : à
   parallélisme constant, il faut diviser le CPU par le rapport mur/100 ms, soit **÷2,0–2,5 pour la v10 et
   ÷3,5–4,1 pour la v11**. Même à parallélisme parfait (48), il faudrait ≤ 4,8 CPU·s [est].
2. **Rattraper la v10 ne suffit pas.** Récupérer les rapports v10 (feuille ×1,27, pas de descente ×3,2) ramène
   la v11 vers les 204–254 ms de la v10 [est]. Les leviers fondés de l'audit du 2 octobre donnent, sur la
   v10, catalogue ≈ 82–93 ms et tour ≈ 42–62 ms à K = 5, soit **≈ 124–155 ms** [est, L05 § 6.9, L06 § 9.2].
3. **Les deux postes doivent chacun tenir dans ~50 ms.** Aujourd'hui la passe unique seule (160–195 ms) et la
   résolution régulière seule (87–117 ms) dépassent chacune ce budget [G4, AB7]. Corriger un seul étage ne
   ferme pas 100 ms (DEEP).
4. **Le levier restant est le travail lui-même** : 48–58 candidats testés par boule émise, un triplet retesté
   dans 3,8 feuilles en moyenne (L05 Q7) ; ≈ 1,0–1,2 M pas géométriques par trame et un census de 3,9 µs
   [est] ; ou un autre processeur (GPU de la G4), hors du cadre `cpu_reference` actuel. À K = 10, le seul
   plancher séquentiel v10 (89–140 ms) et 179–243 ms de descentes rendent la cible hors de portée de toute
   transposition [G4, S4].

### Leviers transposables, classés (pour les cartes de synthèse)

| Rang | Levier (source v10) | Déjà en v11 ? | Doctrine v11 | Gain attendu |
| --- | --- | --- | --- | --- |
| 1 | Hygiène de la boucle chaude des feuilles : compteurs en registres ajoutés une fois par feuille, prédicats sans `Result` sur domaine certifié, **niveau q3 différé** (G4, G5) | non ; q3 différé retenu par REPRISE (`receipts/audit_heritage_20261004/q3_deferred/`) | compatible : compteurs et niveaux inchangés, sorties identiques | [conj] 10–20 % du CPU de la passe unique ; à mesurer par ablation |
| 2 | Pas d'atomique partagé ni d'allocation par nœud : listes des nœuds dans une arène par tâche, réservée une fois (G6) ; tampons réutilisés entre trames pour éviter les fautes de page | non | compatible si la réservation globale reste un majorant prouvé | [conj] une part des 30–90 ms de passage à l'échelle (×28 → ×34 vaudrait ≈ −30 ms sur 02) |
| 3 | Census des descentes par arbre k-d serré à filtre certifié F6, ou requête saturante « k plus proches » (T5) ; plus les extrema q2 couplés (REPRISE, `q2_coupled`) | non (index neuf) | compatible seulement sous F6 (N1) | [conj] jusqu'au tiers de la résolution régulière (census = 10–18 % du CPU à W1) |
| 4 | Mémo partagé par cellule, daté (T3) | non (mémos de lane retirés) | compatible avec date de validité (CONCEPTION_MOTEUR § 5) | [G4] −7 % de pas à K = 5, −9 % à K = 10 dans la v10 |
| 5 | Table des triplets vivants (G3, forme J3 de L05 : −19 % d'instructions, −40 % de mauvaises prédictions [loc]) | non (écartée) | compatible | [conj] faible à moyen ; recouvre le levier 1 |

Aucun de ces gains n'est mesuré sur la v11 ; chacun exige ses portes, ses mutants et un A/B G4 à sorties
identiques avant d'être compté.

FIN
