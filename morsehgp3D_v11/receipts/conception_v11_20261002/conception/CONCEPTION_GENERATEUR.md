# Conception du générateur de catalogue de morsehgp3D_v11

Rédigé le 2 octobre 2026 entre 07:43 et 08:55 UTC (heures lues par `date -u`).

```text
phase=exploration_v11_hors_registre
objet=conception du module catalogue (aucun code produit ecrit)
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP non utilise
```

Vocabulaire. **Prouvé** : énoncé avec preuve écrite (ici ou dans un rapport cité). **Mesuré** : compteur ou temps relevé, avec sa source ; « mesuré G4 » vient des reçus du dépôt, « mesuré local » d'une exécution sur le codespace (compteurs déterministes seulement : aucun temps local n'est cité comme temps de contrat). **Estimé** : modèle chiffré à partir de faits mesurés. **Conjecturé** : attendu sans chiffre fondé.

Sources lues en entier : `audit_v10/L01_MATH_CATALOGUE.md`, `L02_MATH_TOUR.md`, `L05_CODE_CATALOGUE.md` (version de 08:03 UTC), `L06_CODE_TOUR.md`, `L04_CODE_FONDATIONS.md` (constats C05 à C11), `L08_TESTS_PORTES.md` (§ 0, § 5.5 à 5.7, § 11) ; `preuves_l05_code_catalogue/` (microbancs, histogrammes, ablations) ; `preuves_l09_perf_lidar/TABLE_VERITE_G4.md` ; code `morsehgp3D_v10/src/catalogue/generator.cpp`, `arith/geometry.*`, `catalogue/support.hpp` ; privé `build/v10-perf/PLAN_PERF.md`, `feuille/J3_feuille_v3.patch`, `feuille/profil/`, `build/v10-persist/gpu_design/reduction_algorithmique_cpu/` ; conception `GEN_v2.md` ; cadre `morsehgp3D_v11/docs/ARCHITECTURE.md` (état de 08:06 UTC, règle F6 comprise) et `morsehgp3D_v11/audits/` (réponses de l'auditeur aux cinq verrous). Les rapports L09 et L15 n'existaient pas à 08:20 UTC.

Preuves déposées par cette conception : `conception/preuves_generateur/` (annexe A).

---

## 0. Résumé

1. **Le générateur de la v11 garde l'objet et les lemmes de la v10 et change tout le reste.** Même catalogue (théorème G de L01), même partition des centres en boîtes demi-ouvertes ajustées, même recensement local exact. Ce qui change : le repère (local et court), l'arbre (trois zones, dont des nœuds à masques sans liste ni réservoir), l'énumération (plate, sans test de la droite des équidistants), l'arithmétique (binaire64 exact par paliers, filtres de signe F6 à repli exact), la feuille (étages sur lots, noyaux vectoriels), l'émission (16 octets et les identifiants) et l'assemblage (seaux, bandes, une passe).
2. **Deux faits nouveaux, mesurés pour cette conception** (sonde sur une copie du générateur v10, compteurs déterministes, dumps comparés) :
   - filtrer un nœud d'au plus 32 ou 64 sites par dominance de **toutes les paires de sa liste** (au lieu du réservoir de $3K$ sites) rend le **même catalogue, octet pour octet** (quart 01 à K = 5 et 10, trame 02 à K = 5), avec 3 à 6 % de nœuds en moins à K = 5 et autant de nœuds à K = 10 ;
   - énumérer sans le test de la droite (triplets et quadruplets = cliques de paires vivantes) rend aussi le même catalogue, au prix de 2,4 fois plus de quadruplets (15,8 par boule au lieu de 6,7 à K = 5 ; 33,7 au lieu de 14,0 à K = 10).
3. **Les filtres flottants ne décident presque jamais à tort d'hésiter.** Avec les seuils F6 proposés ici (bornes semi-statiques, § 3.4), les replis exacts mesurés sur les candidats réels de la trame 02 sont de 12 sur 1 860 668 triangles aigus (centre q3 dans la boîte), 10 sur 1 564 672 quadruplets (centre q4), 0 sur 1 564 672 (intérieur strict, forme retenue), sans aucune décision certaine contredite par l'exact.
4. **Budget visé** : environ 650 à 800 cycles par boule émise pour tout le catalogue (modèle d'opérations vectorielles, § 8), contre 11 400 à 12 750 mesurés pour la v10. Le plafond d'acceptation est le facteur 4 demandé (2 850 cycles par boule) ; la cible de conception est le double du plancher du modèle. Sur G4 à 48 fils, trame 02 : **18 à 30 ms à K = 5** (171 mesurés en v10), **80 à 125 ms à K = 10** (628 mesurés). À K = 10 le catalogue seul reste donc au voisinage du contrat entier : la conception ne promet pas 100 ms à K = 10.
5. **Rien de cela n'est mesuré en temps.** Le § 9 donne les quatre prototypes à passer sur G4 et leurs critères ; le § 10 les énoncés à prouver avant le code ; le § 11 ce que la conception demande aux fondations.

### Table des décisions

| Id | Décision | Fondement | Gain attendu | Risque |
|---|---|---|---|---|
| G01 | Poids unitaires seulement ; admission unique $p + q_{\min} \leq K + 1$, seuils $\theta_q = K + 1 - q$ ; une entrée à doublons est refusée à l'entrée du générateur | prouvé (lemme W, L01 A.4) ; mesuré : 0 doublon à 1 mm sur 8 trames (L01 E11) | supprime le chemin pondéré sans consommateur ; poids = `popcount` | une trame à doublons est refusée tant que la tour pondérée n'est pas écrite |
| G02 | Repère local à la feuille (origine au coin bas de la boîte), bits sous-unitaires $T = 0$ dans le profil LiDAR, $T$ constante de profil | mesuré : dumps identiques à T = 6, 3, 0 et + 0,2 % de nœuds (L05, `ablation_kT.txt`) ; étendues de liste $< 2^{15}$ mm (L05 § 6.8) | prérequis des paliers et des noyaux vectoriels ; 6 bits rendus à chaque facteur de boîte | réseaux entiers : listes de 48 à 56 sites à T = 0 (grille $16^{3}$, K = 10) ; profil à T = 6 gardé |
| G03 | Arbre en trois zones : haut en largeur (listes de plus de 512 sites), tâches en profondeur (33 à 512), nœuds à masques (au plus 32) | estimé ; structure reprise du prototype privé `frontiere_v3` (têtes en tranches) | frontière 27 ms → 2 à 3 ms à K = 5 ; aucune allocation par nœud | efficacité des tours à barrière ; à mesurer (P2) |
| G04 | Nœud à masques : au plus 32 sites en tableaux locaux, liste = masque de 32 bits, filtre par dominance de toutes les paires ($Y$ = toute la liste) | prouvé (proposition L pour $Y$ quelconque) ; mesuré local : dumps identiques, nœuds − 3,4 %, 176 paires dirigées par boule | filtre des nœuds et dominance de feuille : 2 900 → environ 150 à 300 cycles par boule | modèle de débit non mesuré |
| G05 | Filtre à réservoir vectorisé le long des sites pour les listes de plus de 32 sites ; réservoir exact $(dd, \text{rang})$ comme la v10 | prouvé (lemmes D, D-loc, L) ; estimé pour le coût | 5,2 à 5,5 cycles par test → moins de 1 | sélection du réservoir ; lanes de 64 bits aux grandes étendues |
| G06 | $M(K)$ = 16 ($K \leq 6$), 24 ($K \leq 10$), 28 au-delà ; réservoir de $3K$ ; constantes internes, recalibrées une fois sur G4 | mesuré (L05 § 6.10 : minimum du modèle à 16 et 24 ; optimum plat de $2K$ à $4K$) | — | l'optimum peut bouger quand les coûts unitaires changent (P-CAL) |
| G07 | Énumération plate : triplets et quadruplets = cliques de paires vivantes de poids admissible ; plus de test de la droite ni de table des triplets vivants | prouvé (mêmes présentations jugées, § 4.2) ; mesuré local (dumps identiques ; quadruplets × 2,4) | retire l'étage le plus cher de la v10 (145 cycles × 20 triplets par boule) et une table en $m^{2}$ ; un prédicat et un lemme de moins | à K = 10 les quadruplets dominent ; variante à droite gardée pour la mesure P1 |
| G08 | Feuille par étages sur lots d'indices, noyaux sans branchement séparés par une compaction ; ordre : aigu puis centre (triplets), centre puis intérieur (quadruplets) | mesuré local (sélectivités, § 4.3) ; mesuré par l'audit sur AVX2 (quadruplets × 8 à × 11, triplets × 1,5 à × 2) ; estimé au-delà | triplets et quadruplets : 4 760 → environ 200 à 400 cycles par boule (K = 5) | petits lots (87 triplets par feuille) : rendement vectoriel à mesurer |
| G09 | Arithmétique par paliers d'étendue locale $e$ : binaire64 exact (F2) quand la borne des termes développés tient sous $2^{53}$, entier exact (i64, i128, large) en repli | prouvé (budget mécanique, `budget_bits.py`) | dominance, paires, aigu : jamais de filtre ; centre q4 exact pour 88 à 93 % des feuilles | une borne par expression à graver en `constexpr` |
| G10 | Filtres de signe F6 à seuil semi-statique certifié pour les degrés 4 à 6 (centre q3 et q4 dans la boîte, intérieur, recensement q3 et q4) ; repli exact à l'égalité | prouvé ici (annexe B.1 à B.3), à contre-lire ; contrôle numérique adverse sans violation ; replis mesurés : 0 à $6 \cdot 10^{-6}$ | permet les noyaux vectoriels au-delà des paliers exacts (60 à 95 % des feuilles selon le prédicat) | la forme semi-statique étend F6 : si elle est refusée, seuil statique par feuille (plus de replis) |
| G11 | Intérieur strict du tétraèdre par réemploi du centre de Cramer : $A' = N' \cdot (v \times s)$ et ses deux analogues, $A' + B' + C' < 2 \det^{2}$ | prouvé (annexe B.5) ; identité vérifiée sur 4 000 cas | 38 opérations au lieu de 74 pour la forme de Gram, seuil plus serré | — |
| G12 | Recensement vectorisé sur tous les sites de la feuille, sans sortie anticipée ; centre approché, repli exact par site indécis | estimé (l'essai scalaire de l'audit ne gagne rien ; la forme vectorielle n'est pas mesurée) | 918 → environ 80 cycles par boule (K = 5) | à mesurer (P1) |
| G13 | Émission : une référence de 16 octets et les identifiants ; arènes par fil en blocs comptés, gardées d'une trame à l'autre | mesuré (privé : 993 → 115 cycles par boule quand les pages sont déjà touchées) | émission 600 → environ 50 cycles par boule ; hors étages 9 à 23 ms → environ 0 | la `Session` doit garder des blocs sans fausser le budget |
| G14 | Catalogue résident : 11 octets fixes, 4 par identifiant, 4 par rang ; niveau exact recalculé depuis $S^{*}$ | mesuré (4,63 et 8,10 identifiants par boule ; 0,78 et 0,90 rang par boule) | 32,6 octets par boule à K = 5, 47,0 à K = 10 (104 et 125 en v10) | la tour recalcule un niveau exact à la demande |
| G15 | Ordre canonique : clé approchée F3 (exposant au plus 10), partition parallèle en seaux sur les bits de tête, tri par seau, bandes F4, réparation exacte, couture des frontières de seaux, une seule passe de remplissage | prouvé (F3, F4 ; lemme des bandes, annexe B.7) ; mesuré privé pour les clés compactes et la partition (× 1,7 à 8 fils) | ordre et assemblage 33 ms → 2 à 4 ms (K = 5), 124 → 8 à 14 ms (K = 10) | à K = 10 l'étage est borné par le débit mémoire |
| G16 | Ordonnancement : tours en largeur pour le haut, puis une boucle parallèle dynamique sur des tâches bornées (liste d'au plus 512 sites) ; pas de vol de tâches | estimé | taux d'emploi des fils de 73 à 77 % (L04 C08) vers plus de 90 % | exige un pool sans mise en sommeil entre deux régions |
| G17 | Budgets et refus typés : nœuds, boules (u32), mémoire, feuille large, coquille ; profondeur bornée par $3(B + T) + 1$ | prouvé pour les bornes de brouillon ; décision pour les seuils | supprime les explosions de L01 § 6.4 et L05 § 5.3 | seuils à caler |
| G18 | Jeu d'instructions de référence AVX-512 sur G4 (noyaux écrits, pas d'auto-vectorisation), noyau AVX2 portable, noyau scalaire entier exact de **référence** qui sert de repli, de chemin générique et de juge d'identité | mesuré (v10 sur G4 sans `-march` : noyau du filtre non vectorisé, L05 § 6.4) | porte d'identité entre jeux d'instructions | g++ 11.4 de la VM ; trois variantes à tenir égales |
| G19 | Coquilles étendues : chemin scalaire exact, mémo par **masque de coquille** (lemme U), limite de coquille commune avec la tour | prouvé (lemme U, GEN_v2 § 3.7) | plus de coût en $m^{4}$ par présentation répétée | — |
| G20 | Feuille rapide jusqu'à 32 sites (masques de 32 bits), chemin générique exact de 33 à 64, refus `wide_leaf` au-delà | mesuré : $m \leq 24$ sur LiDAR, 0 feuille bloquée (L05 § 6.1) | un seul gabarit de masque sur le chemin chaud | la v10 acceptait jusqu'à 256 sites (sphère de 144 points : 20 s, puis refus de la tour) |
| G21 | Aucune re-vérification d'un théorème dans le produit (ni comparaison exacte de tous les voisins, ni juge de recensement) ; les mutants correspondants sont tués par des fixtures | prouvé (F4) ; règle du dépôt | retire 0,15 s de CPU à K = 5 et 0,6 s à K = 10 (v10, un fil) | les fixtures d'égalité doivent exister avant le code |
| G22 | Conformité : dump au format de la v10 identique octet pour octet sur les entrées que la chaîne v10 sert, et empreinte canonique publiée | décision ; mesuré : la sonde retrouve `414aa4d4…`, `7c46e50a…`, `8a850649…` | — | écarts déclarés au § 7.1 (doublons, feuilles de 65 à 256 sites, coquilles de plus de 24) |
| G23 | Leviers reportés, chacun avec sa mesure : préfiltre en simple précision à 16 voies, exactitude dynamique, variante à droite des équidistants, assemblage sans copie, portage GPU des noyaux par lots | conjecturé | 10 à 15 % chacun au mieux | aucun n'entre sans microbanc G4 |

---

## 1. Faits mesurés sur lesquels la conception s'appuie

| Fait | Valeur | Source |
|---|---|---|
| Catalogue v10 sur G4, trame 02, 48 fils (passe chaude) | 164,3 ms à K = 5 ; 618,1 ms à K = 10 | `TABLE_VERITE_G4.md` § 1 (reçu de session 4) |
| Idem, processus froid | 171,1 ms et 627,9 ms ; CPU 6,04 s et 25,6 s | L05 § 6.6 |
| Un fil sur G4 | 4,04 s à K = 5 (2,87 µs par boule) ; 16,76 s à K = 10 (3,06 µs par boule) | L05 § 6.6 |
| Cycles par boule de l'étage des boîtes (local, un fil) | 10 200 à K = 5 ; 11 500 à K = 10 | L05 § 6.2 |
| Parts à K = 5 : filtre, dominance, paires, triplets, quadruplets, juges | 22,7 ; 6,0 ; 6,9 ; 29,1 ; 17,6 ; 16,6 % | L05 § 6.2 |
| Coûts unitaires v10 | 5,5 cycles par test du filtre ; 26 par site pour le réservoir ; 145 par triplet ; 271 par quadruplet ; 34 par site recensé ; 608 par boule émise | L05 § 6.2 |
| Entonnoir de la trame 02 à K = 5 | 28,6 M triplets, 20,4 M droites touchantes, 9,08 M aigus, 1,90 M jugés ; 9,38 M quadruplets, 0,48 M jugés ; 3,30 M recensements ; 1,41 M boules | L05 § 6.3 |
| Microbancs sur candidats réels (AVX2 local) | quadruplets : 297 à 325 → 28 à 39 cycles ; triplets : 163 à 198 → 81 à 134 ; recensement scalaire en double : aucun gain | L05 § 6.7 |
| Étendue de la liste d'une feuille (trame 02), cumul K = 5 puis K = 10 | $< 2^{7}$ mm : 7,1 et 5,4 % ; $< 2^{9}$ : 40,2 et 30,2 % ; $< 2^{11}$ : 77,8 et 68,7 % ; $< 2^{12}$ : 92,6 et 88,1 % ; $< 2^{13}$ : 98,5 et 97,7 % ; $< 2^{15}$ : 100 % | `feuilles_histogrammes_et_duplication.txt` |
| Taille des listes de feuille | 5 à 16 à K = 5 (moyenne 13,95) ; 10 à 24 à K = 10 (21,6) | idem |
| Émission, pages déjà touchées | 115 cycles par boule au lieu de 993 (quart 01, K = 10) | privé, `feuille/profil/emission_reserve_q01_k10.txt` |
| Identifiants et rangs par boule | 4,63 et 8,10 ; 0,781 et 0,895 | L05 § 6.5 |
| Machine G4 | EPYC 9B45, 24 cœurs, 48 fils ; L1d 48 Kio, L2 1 Mio par cœur, L3 3 × 32 Mio ; `avx512f dq ifma cd bw vl vbmi vbmi2 vnni bitalg vpopcntdq vp2intersect`, `fma`, `bmi2` ; g++ 11.4 | `receipts/g4_session5_scale_20260929/vm_facts.txt` |

Compteurs de la sonde de cette conception (annexe A ; trame 02 à K = 5 et quart 01 à K = 10 ; « plat » = sans test de la droite ; « 32 » = dominance de toutes les paires pour les listes d'au plus 32 sites) :

| Par boule émise | v10 (trame 02, K = 5) | plat, 32 (trame 02, K = 5) | v10 (quart 01, K = 10) | plat, 32 (quart 01, K = 10) |
|---|---:|---:|---:|---:|
| nœuds ; feuilles | 0,521 ; 0,230 | 0,504 ; 0,223 | 0,245 ; 0,110 | 0,245 ; 0,110 |
| visites du filtre à réservoir | 21,8 | 13,1 | 10,2 | 5,66 |
| paires dirigées des nœuds à masques | — | 175,7 | — | 123,9 |
| paires de poids $\leq \theta_2$ ; $\leq \theta_3$ ; $\leq \theta_4$ | 13,4 ; — ; — | 13,3 ; 9,85 ; 6,76 | 12,5 ; 10,7 ; 9,0 | 12,5 ; 10,7 ; 9,0 |
| triplets de poids admissible ; aigus | 20,3 ; 7,9 | 20,3 ; 7,9 (38,9 %) | 26,6 ; 9,2 | 26,6 ; 9,2 (34,5 %) |
| quadruplets ; centre dans la boîte ; intérieurs | 6,66 ; 3,52 ; 0,49 | 15,8 ; 3,55 (22,5 %) ; 0,79 (5,0 %) | 14,0 ; 7,39 ; 0,76 | 33,7 ; 7,39 (22,0 %) ; 1,31 (3,9 %) |
| juges q2 ; q3 ; q4 | 0,66 ; 1,35 ; 0,34 | 0,66 ; 1,36 ; 0,34 | 0,31 ; 1,07 ; 0,51 | 0,31 ; 1,07 ; 0,51 |
| dump | `8a850649ff103c1c` | identique | `7c46e50a72c08087` | identique |

Répartition des nœuds de la trame 02 (K = 5) selon la taille de la liste parente, et de leurs visites : au plus 32 sites, 79 % des nœuds et 38 % des visites ; 33 à 64, 14 % et 14 % ; 65 à 1 024, 7 % et 28 % ; plus de 1 024, 0,2 % (1 661 nœuds) et 19 % ; dont plus de 4 096 : 257 nœuds et 10,5 % des visites.

---

## 2. Structures de données et formats

### 2.1 Sites

`cloud` fournit les sites en ordre de Morton. Le générateur les lit en **tableaux séparés** `x[]`, `y[]`, `z[]` de `i32` (alignés sur 64 octets, complétés à un multiple de 16), 12 octets par site : 550 Ko pour 45 845 sites, résidents en L2. Aucune copie mise à l'échelle, aucun `P3` de trois `i64` (v10 : quatre copies des coordonnées, L04 C10).

### 2.2 Boîtes et repères

- Une boîte est un pavé demi-ouvert $S = \prod_k [lo_k, hi_k)$ à coins entiers en unités de $2^{-T}$ ; `Box` = six `i32` ($B + T + 1 \leq 25$ bits). $T$ est une constante du profil : **0 pour le profil LiDAR** (G02).
- **Repère local** d'un nœud ou d'une feuille : origine au coin bas de sa boîte. Un site y vaut $y = 2^{T} x - lo$, la boîte y est $[0, h)$. Les prédicats de la feuille ne lisent que $y$, $h$ et des différences de sites.
- **Étendue locale** $e$ : soit $W$ la plus grande largeur, sur les trois axes, de la boîte englobante de la liste (en unités de boîte). $e$ est le plus petit entier tel que $W + 1 < 2^{e}$. Comme la boîte ajustée est incluse dans l'enveloppe de sa liste élargie d'une unité (lemme A), toute coordonnée locale, tout côté $h_k$ et toute différence de deux sites de la liste sont de valeur absolue $< 2^{e}$. C'est le **domaine certifié** de la règle F6 : tous les opérandes des prédicats d'une feuille sont des sites de sa liste ou les coins de sa boîte ; aucun site extérieur n'y entre.

### 2.3 Listes, sans allocation par nœud

| Zone de l'arbre | Liste | Stockage |
|---|---|---|
| Haut (liste de plus de $N_{\mathrm{task}} = 512$ sites) | tranche de quatre tableaux `idx`, `x`, `y`, `z` (16 octets par entrée, coordonnées relatives au coin de la racine) | deux arènes de tour, en alternance : les listes du tour $r$ sont lues pour écrire celles du tour $r + 1$, puis rendues ; blocs pris dans le budget |
| Tâche (33 à 512 sites) | même forme | pile de listes du fil : une liste par profondeur, capacité $N_{\mathrm{task}}$ entrées par niveau, profondeur au plus $3(B + T) + 1$ (55 pour B = 18, T = 0) ; 450 Ko par fil, taille fixe |
| Nœud à masques (au plus 32 sites) | masque `u32` sur un tableau local de 32 entrées (`idx`, coordonnées locales `i32`) | brouillon du fil, 0,5 Ko ; aucune copie de liste d'un niveau au suivant |
| Feuille | sites vivants compactés : `id[32]`, `y[3][32]` en `i32` et en binaire64 | brouillon du fil |

Une racine de tâche (nœud dont la liste vient de passer sous 513 sites) est décrite par sa boîte et ses indices de sites, écrits dans un tampon de tâches (blocs comptés) ; les coordonnées sont relues dans les tableaux de sites.

### 2.4 Lots de la feuille

Tableaux d'octets en colonnes, de capacité fixe, dans le brouillon du fil : triplets `(i, j, k)` par 1 024, quadruplets `(i, j, k, l)` par 2 048, juges par 512 (indices des générateurs, arité, centre approché $\tilde{N}$, $\tilde{D}$ et majorants pour le recensement : 64 octets). Un lot plein est vidé dans le noyau suivant ; l'énumération reprend ensuite. La capacité est une constante : le brouillon ne dépend pas de l'entrée (architecture § 7.1).

### 2.5 Ce qu'une boule porte, ce qui se recalcule

Pour la tour, une boule doit donner : son rang, $q_{\min}$, $I$ et $U$ triés, $S^{*}$, le drapeau de coquille étendue. Pour une coquille régulière (99,96 % des boules), $S^{*} = U$ : rien n'est stocké en double. Se recalculent depuis $S^{*}$ : le centre $(N, D)$, le niveau exact, la clé approchée.

**Émission** (par fil, pendant l'étage des tâches) :

| Champ | Taille | Contenu |
|---|---:|---|
| `key` | 8 o | clé approchée du niveau (binaire64 strictement positif, lu comme `u64`) |
| `pos` | 4 o | position des identifiants dans l'arène du fil, en mots de 4 octets |
| `meta` | 4 o | fil (10 bits), $\lvert I \rvert$ (4 bits), $\lvert U \rvert$ (6 bits), $q_{\min}$ (2 bits), étendue (1 bit) |
| identifiants | 4 o chacun | $I$ trié puis $U$ trié (`SiteIdx`) ; pour une coquille étendue, un mot de plus : positions de $S^{*}$ dans $U$ |

Soit 16 + 4 × 4,63 = 34,5 octets par boule à K = 5 et 16 + 4 × 8,10 = 48,4 à K = 10 (v10 : 155 à 170 transitoires).

**Catalogue résident** (ordre canonique, `Buffer` comptés) :

| Tableau | Par | Taille | Contenu |
|---|---|---:|---|
| `rank` | boule | 4 o | `LevelRank` du niveau exact (0 = plus petit niveau du catalogue) |
| `ids_off` | boule (+ 1) | 4 o | début des identifiants (refus `index_overflow_u32` au-delà de $2^{32} - 1$) |
| `n_int`, `n_shell`, `qflags` | boule | 3 o | $\lvert I \rvert$, $\lvert U \rvert$, $q_{\min}$ et drapeau d'étendue |
| `ids` | identifiant | 4 o | $I$ trié puis $U$ trié |
| `level_rep` | rang | 4 o | première boule du rang ; son $S^{*}$ donne la représentation canonique du niveau |
| `ext` | coquille étendue | 8 o | boule, positions de $S^{*}$ dans $U$ |

Soit 11 + 4 × 4,63 + 4 × 0,781 = **32,6 octets par boule à K = 5** (46 Mo pour la trame 02) et 11 + 4 × 8,10 + 4 × 0,895 = **47,0 à K = 10** (258 Mo). Une table optionnelle de clés approchées par rang (8 octets par rang) peut servir d'index de recherche à la tour ; elle n'est pas l'objet publié (architecture § 7.3) et se reconstruit.

### 2.6 Budget mémoire

| Poste | K = 5, trame 02 | K = 10, trame 02 | Borne ou règle |
|---|---:|---:|---|
| brouillons des fils (piles de listes, feuille, lots) | 48 × 0,6 Mo = 29 Mo | idem | constante par fil |
| arènes du haut (deux tours) | ≈ 7 à 15 Mo | idem | blocs comptés ; aucune borne prouvée (L01 O3) : refus au plafond |
| racines de tâches | ≈ 10 Mo | ≈ 10 Mo | blocs comptés |
| émission (références et identifiants) | 49 Mo | 265 Mo | blocs comptés, gardés entre trames |
| catalogue final | 46 Mo | 258 Mo | compté après l'émission, réservé avant le remplissage |
| **pic** | **≈ 145 Mo** (425 mesurés en v10) | **≈ 570 Mo** (1 823) | admission par étage ; refus `memory_budget` avant toute publication |

Les arènes d'émission ne peuvent pas être réservées par formule avant le calcul (le nombre de boules n'est connu qu'à la fin) : elles suivent la règle « blocs d'arène pris dans le budget » du § 7.1 de l'architecture. Le catalogue final, lui, est compté puis réservé d'un coup.

---

## 3. Arithmétique

### 3.1 Règles

1. **Définition de référence.** Chaque prédicat est une expression entière exacte dans le repère local, évaluée avec `num::Int<bits>` où `bits` est le budget calculé en `constexpr` à partir de l'étendue locale $e$ et de $T$. C'est le **noyau scalaire de référence** : il définit la décision, sert de repli, traite les feuilles hors du chemin vectoriel, et juge l'identité des autres noyaux.
2. **Binaire64 exact (F2).** Un prédicat est porté en binaire64 sans filtre quand la somme des valeurs absolues de ses termes développés, coefficients compris, est $< 2^{53}$ pour chaque valeur comparée. On compare deux valeurs exactes plutôt que de former leur différence : la comparaison de deux binaire64 est exacte, et cela économise un bit de budget.
3. **Filtre de signe (F6).** Au-delà, la valeur est évaluée en binaire64, le signe n'est décidé que si $\lvert \tilde{v} \rvert > \tau$, sinon le prédicat est rejoué par la référence. Les noyaux filtrés supposent $e \leq 16$ : c'est la condition pour que toutes leurs sous-expressions de degré au plus 3 soient exactes (table ci-dessous).
4. **Hors domaine vectoriel.** Une feuille d'étendue $e > 16$ (aucune sur les trames mesurées) passe entièrement par la référence.
5. Aucune décision ne dépend d'un mode d'arrondi, d'une contraction ni d'un ordre d'évaluation : les décisions certaines sont justes dans tous les cas, les autres sont exactes.

### 3.2 Budget de chaque prédicat

Calcul mécanique (`preuves_generateur/budget_bits.py`) : les feuilles de l'expression sont des entiers de valeur absolue $< 2^{e}$ (coordonnées locales, côtés de boîte, différences de sites) ; $M(a \pm b) = M(a) + M(b)$, $M(ab) = M(a) M(b)$. « bits » = $\log_2$ du plus grand majorant parmi les valeurs que le prédicat compare. Avec $T > 0$, chaque facteur de boîte (coordonnée locale ou côté) compte $e + T$ au lieu de $e$. Notations : $u, v, s$ différences de sites au premier site $a$ du uplet ; $w = u \times v$ ; $d = z - a$.

| Prédicat | Forme comparée | Degré | bits | Binaire64 exact | i64 | i128 | Chemin vectoriel |
|---|---|---:|---|---|---|---|---|
| Dominance d'un site par un autre sur la boîte (nœud et feuille) | $A_i - A_j > \sum_k \max(0, P_k(i) - P_k(j))$, $A = \lVert y \rVert^{2}$, $P_k = 2 h_k y_k$ | 2 | $2e + 3{,}58$ | $e \leq 24$ | $e \leq 29$ | toujours | `i32` × 16 si $e \leq 13$, sinon binaire64 × 8 ; exact, jamais filtré |
| Bissectrice d'une paire contre la boîte fermée | lue dans la matrice de dominance (aucun des deux ne domine l'autre) | — | — | — | — | — | opération de masques |
| Poids de l'union des dominateurs | `popcount` d'un OU de masques | — | — | — | — | — | `vpopcntd` |
| Milieu d'une paire dans la boîte | $0 \leq y_i + y_j < 2h$ par axe | 1 | $e + 1$ | toujours | toujours | toujours | `i32` × 16, exact |
| Triangle strictement aigu | $0 < u \cdot v$, $u \cdot v < u \cdot u$, $u \cdot v < v \cdot v$ | 2 | $2e + 1{,}58$ | $e \leq 25$ | $e \leq 30$ | toujours | binaire64 × 8, exact |
| Droite des équidistants contre la boîte (**non retenue**, variante mesurée) | $\lvert v_k P_0 - u_k P_1 \rvert \leq 2 \sum_{j \neq k} h_j \lvert c_{kj} \rvert$ | 3 | $3e + 5{,}17$ | $e \leq 15$ | $e \leq 19$ | toujours | binaire64 × 8, exact |
| Centre q3 dans la boîte | $0 \leq a_k D + N_k$ et $a_k D + N_k < h_k D$, $N = (uu\, v - vv\, u) \times w$, $D = 2 \lVert w \rVert^{2}$ | 5 | $5e + 5{,}58$ | $e \leq 9$ | $e \leq 11$ | $e \leq 24$ | exact si $e \leq 9$, filtré de 10 à 16 |
| Centre q4 dans la boîte | idem avec $N = \sigma (uu\, v \times s + vv\, s \times u + ss\, u \times v)$, $D = 2 \lvert \det \rvert$ | 4 | $4e + 4{,}91$ | $e \leq 12$ | $e \leq 14$ | $e \leq 30$ | exact si $e \leq 12$, filtré de 13 à 16 |
| Centre strictement intérieur au tétraèdre | $A', B', C' > 0$ et $A' + B' + C' < 2 \det^{2}$, $A' = N' \cdot (v \times s)$, etc. | 6 | $6e + 8{,}34$ | $e \leq 7$ | $e \leq 9$ | $e \leq 19$ | exact si $e \leq 7$, filtré de 8 à 16 |
| Côté d'un site, recensement q2 | $\lVert d \rVert^{2}$ contre $n \cdot d$ ($n$ = différence des deux générateurs) | 2 | $2e + 1{,}58$ | $e \leq 25$ | $e \leq 30$ | toujours | binaire64 × 8, exact |
| Côté d'un site, recensement q3 | $D \lVert d \rVert^{2}$ contre $2 N \cdot d$ | 6 | $6e + 7{,}17$ | $e \leq 7$ | $e \leq 9$ | $e \leq 19$ | exact si $e \leq 7$, filtré de 8 à 16 |
| Côté d'un site, recensement q4 | idem | 5 | $5e + 6{,}75$ | $e \leq 9$ | $e \leq 11$ | $e \leq 24$ | exact si $e \leq 9$, filtré de 10 à 16 |
| Support canonique d'une coquille étendue | milieu d'une paire, centre dans le plan d'un triangle aigu, centre intérieur à un tétraèdre, sur la coquille seule (port de `support.hpp`) | 7 au plus | $7e + 8{,}2$ | — | — | $e \leq 16$ | référence scalaire seulement (0,02 à 0,04 % des boules) |
| Niveau | clé approchée F3 (aucune décision) ; niveau exact recalculé depuis $S^{*}$ à la demande | 8 sur 6 | numérateur $8B + 9{,}92$, dénominateur $6B + 7{,}17$, produit croisé $14B + 17{,}1$ | — | — | — | § 6 |

Sous-expressions exactes dont dépendent les noyaux filtrés : $w$ et tout produit vectoriel ($2e + 1$ bits), produits scalaires ($2e + 1{,}58$), $t = uu\, v - vv\, u$ et $\det$ ($3e + 2{,}58$, exacts en binaire64 pour $e \leq 16$). C'est ce qui fixe la borne $e \leq 16$ du chemin vectoriel.

Lecture pour le profil $B$ : $e \leq B$ toujours. À $B = 18$, tout prédicat de feuille tient en i128 ($6 \cdot 18 + 8{,}34 = 116{,}3$ bits), sauf le support canonique des coquilles étendues pour $e \geq 17$ (large). À $B = 21$ et $24$, les prédicats de degré 6 demandent le type large pour $e \geq 20$ ; le chemin vectoriel ne change pas (il ne dépend que de $e$).

### 3.3 Part des feuilles par palier

D'après l'histogramme des étendues de liste de la trame 02 (L05, mesuré ; l'étendue $< 2^{k}$ mm correspond à $e \leq k$ à l'unité près) :

| Étendue locale $e$ | K = 5 | K = 10 | Dominance | Centre q4 | Centre q3, recensement q4 | Intérieur, recensement q3 | Repli entier |
|---|---:|---:|---|---|---|---|---|
| $\leq 7$ | 7,1 % | 5,4 % | `i32` × 16 | exact | exact | exact | — (aucun repli possible) |
| 8 à 9 | 33,1 % | 24,8 % | `i32` × 16 | exact | exact | filtré | i64 |
| 10 à 11 | 37,6 % | 38,5 % | `i32` × 16 | exact | filtré | filtré | i64 (degré 5), i128 (degré 6) |
| 12 | 14,8 % | 19,4 % | `i32` × 16 | exact | filtré | filtré | i128 |
| 13 | 5,9 % | 9,6 % | `i32` × 16 | filtré | filtré | filtré | i64 (degré 4), i128 |
| 14 à 16 | 1,5 % | 2,3 % | binaire64 × 8 | filtré | filtré | filtré | i128 |
| plus de 16 | 0 | 0 | référence scalaire pour toute la feuille | | | | i128, large au-delà de 19 |

Le palier est choisi **une fois par feuille** (un entier $e$, lu sur la boîte englobante de sa liste), jamais par candidat. Mesure complémentaire au niveau du candidat (annexe A, `replis_filtres.cpp`, avec les boîtes à $T = 6$ de la v10, donc 6 bits de plus sur le facteur de boîte) : le test « centre q3 dans la boîte » est exact en binaire64 pour 74 % des triangles aigus à K = 5 et 66 % à K = 10, le test q4 pour 98,7 % et 97,2 % des quadruplets : le palier par feuille est prudent.

### 3.4 Filtres de signe : seuils et preuve

La règle F6 de l'architecture (auditeur indépendant, 2 octobre) est reprise telle quelle : $u = 2^{-52}$ ; exposition $E$ nulle sur une feuille exacte, $E(a \pm b) = \max(E_a, E_b) + 1$, $E(ab) = E_a + E_b + 1$, la forme contractée étant majorée par la forme non contractée ; $M$ somme des valeurs absolues des termes développés ; $\lvert \tilde{v} - v \rvert \leq \gamma M$ avec $\gamma = (1 - u)^{-E} - 1 \leq 2Eu$ ; seuil $\tau = 2^{q + e_E - 51}$ pour $M \leq 2^{q}$ et $E \leq 2^{e_E}$ ; décision seulement si $\lvert \tilde{v} \rvert > \tau$.

La conception lui ajoute deux précisions, **à contre-lire avant le code** (preuves en annexe B) :

- **F6-a (sous-expression exacte).** Une sous-expression dont la borne développée est $< 2^{53}$ sur le domaine certifié est évaluée exactement (F2) ; dans l'expression filtrée elle compte comme une feuille exacte, d'exposition nulle et de majorant égal à sa valeur absolue. C'est ce qui ramène, par exemple, l'exposition du centre q3 de 10 (expression développée depuis les coordonnées) à 6.
- **F6-b (seuil semi-statique certifié).** Le majorant $M$ n'est pas pris sur toute la feuille mais sur le candidat : $\hat{M}$ est évalué en binaire64 par sommes et produits de termes positifs ou nuls à partir de valeurs exactes, et $\tau = 2^{e_E - 51} \hat{M}$ (une multiplication par une puissance de deux, exacte). Le facteur 2 de $\gamma \leq 2Eu$ absorbe l'arrondi de $\hat{M}$ : $\tau \geq \gamma M$ dès que $(j + 2^{e_E}) u \leq 1/4$, où $j$ est l'exposition de $\hat{M}$. Sans cela (seuil statique par feuille), un petit uplet dans une grande feuille serait presque toujours indécis ; or une liste LiDAR mêle des points d'un même anneau (centimètres) et d'anneaux voisins (décimètres).

| Noyau | Valeurs dont le signe est filtré | Exposition (majorant) | Majorant $\hat{M}$ | Seuil |
|---|---|---|---|---|
| C3, centre q3 dans la boîte | $g_k = a_k D + N_k$ ; $h_k D - g_k$ | 6 ; 7 | $\lvert a_k \rvert D + \lvert t_i \rvert \lvert w_j \rvert + \lvert t_j \rvert \lvert w_i \rvert$ ; plus $h_k D$ | $2^{-48} \hat{M}$ |
| C4, centre q4 dans la boîte | $g_k = a_k D + N_k$ ; $h_k D - g_k$ | 4 ; 5 | $\lvert a_k \rvert D + \hat{M}_{N,k}$ avec $\hat{M}_{N,k} = uu \lvert (v \times s)_k \rvert + vv \lvert (s \times u)_k \rvert + ss \lvert (u \times v)_k \rvert$ ; plus $h_k D$ | $2^{-48} \hat{M}$ |
| I4, intérieur strict | $A' = N' \cdot (v \times s)$, $B'$, $C'$ ; $2 \det^{2} - A' - B' - C'$ | 6 ; 9 | $\sum_k \hat{M}_{N,k} \lvert (v \times s)_k \rvert$, etc. ; somme des trois plus $2 \det^{2}$ | $2^{-48} \hat{M}$ ; $2^{-47} \hat{M}$ |
| S3 et S4, côté d'un site | $D \lVert d \rVert^{2} - 2 N \cdot d$ | 7 ; 8 | $D \lVert d \rVert^{2} + 2 \sum_k \hat{M}_{N,k} \lvert d_k \rvert$ | $2^{-48} \hat{M}$ |

Règle de décision, identique pour tous : *certainement vrai* si chaque inégalité stricte a $\tilde{v} > \tau$ ; *certainement faux* si l'une a $\tilde{v} < -\tau$ ; sinon, **égalité comprise**, repli sur la référence. Dans un palier exact, $\tau = 0$ et les comparaisons sont celles de la définition (inégalités larges comprises) : aucun repli n'existe.

État de la preuve et des contrôles :

- Preuve de la borne : annexe B.1 (récurrence de F6), B.2 (F6-a), B.3 (F6-b). Statut proposé : prouvé ici, à contre-lire par les deux auditeurs.
- Contrôle numérique adverse (`controle_f6.py`, pas une preuve) : 4 000 uplets (tétraèdres presque plats, petits uplets dans une grande étendue, valeurs extrêmes, $e$ de 10 à 16), chaque opération arrondie vers le haut ou vers le bas au hasard, sommes dans un ordre et un parenthésage au hasard, contraction au hasard, majorants toujours arrondis vers le bas : **aucune violation** sur 139 600 valeurs ; le plus grand rapport erreur sur seuil vaut 0,17.
- Replis sur candidats réels (`replis_filtres.cpp`, trame 02, un bloc de 4 096 candidats sur 6, boîtes de la v10) :

| Filtre | K = 5 | K = 10 |
|---|---|---|
| centre q3 dans la boîte | 12 indécis sur 1 860 668 triangles aigus | 2 sur 1 044 111 |
| centre q4 dans la boîte | 10 sur 1 564 672 quadruplets | 3 sur 1 565 260 |
| intérieur strict (forme retenue) | 0 sur 1 563 911 non dégénérés | 0 sur 1 565 060 |
| décisions certaines contredites par l'exact | 0 | 0 |

- L'audit (L05 § 6.7) avait mesuré, avec une borne plus large et non prouvée, 0 repli de boîte sur 9,08 M triangles et 0 à 2 replis pour 4 000 recensements ; ces mesures restent des majorants des taux attendus ici.

Portes exigées par F6 sur les expressions réelles (module `num`, porte `mhgp11_num_filters`) : zéro exact et signes à une unité près d'un grand majorant ; carrés et valeurs réutilisées ; permutations et parenthésages ; quatre modes d'arrondi ; contraction active et inactive ; bornes exactes du domaine ($e = 9, 12, 16$ et leurs voisins) ; plancher de replis effectivement déclenchés par filtre.

### 3.5 Niveaux

- **Clé approchée (F3)**, calculée à l'émission dans le repère local (un niveau ne dépend pas du repère) : q2, $\lVert u \rVert^{2} / 4$, exacte ; q3, $(uu \cdot vv \cdot dd) / (4 \lVert w \rVert^{2})$ avec $dd = \lVert v - u \rVert^{2}$, exposant 7 (produit de trois entiers exacts : 2 ; somme de trois carrés d'entiers exacts : 3 ; quotient : $2 + 3 + 2$) ; q4, $\lVert N \rVert^{2} / D^{2}$ à partir de $N$ et $D$ entiers exacts, exposant 6 quand ils sont exacts en binaire64 ($e \leq 12$), 10 après conversion depuis i128 sinon. Toutes les clés sont strictement positives ($\geq 1/4$) et aucune valeur intermédiaire n'approche les bornes du format ($< 2^{8B + 10}$).
- **Niveau exact** : jamais stocké par boule. Il est recalculé depuis $S^{*}$ par `num` quand une bande le demande (§ 6.3) ou quand la tour le lit, dans la représentation canonique définie par $S^{*}$ seul (plus de `emitted_level`, L01 C9).

---

## 4. La feuille par étages sur lots

### 4.1 Entrée et sortie

Entrée : une boîte ajustée $S$ et sa liste K-certifiée (au plus 32 sites sur le chemin rapide). Sortie : les boules admises de centre dans $S$, chacune une fois, écrites dans l'arène du fil. La feuille ne lit rien d'autre.

### 4.2 Ce qui est énuméré, et pourquoi c'est complet

Avec $\mathrm{Dom}[i]$ l'ensemble des sites de la liste qui dominent $i$ sur $\bar{S}$, et $\theta_q = K + 1 - q$ :

- **paires** : $(i, j)$ sans dominance de l'un par l'autre et $\lvert \mathrm{Dom}[i] \cup \mathrm{Dom}[j] \rvert \leq \theta_2$ ; trois masques par site : $P_2$ (seuil $\theta_2$), $P_3$ ($\theta_3$), $P_4$ ($\theta_4$) ;
- **présentations q2** : paires de $P_2$ dont le milieu est dans $S$ ;
- **triplets** : cliques de $P_3$ dont l'union des dominateurs pèse au plus $\theta_3$ ; présentations q3 : triangle strictement aigu et centre dans $S$ ;
- **quadruplets** : cliques de $P_4$ dont l'union pèse au plus $\theta_4$ ; présentations q4 : déterminant non nul, centre dans $S$, centre strictement intérieur.

Il n'y a plus de test de la droite des équidistants ni de table des triplets vivants. **Preuve de complétude** (annexe B.4) : le support canonique d'une boule admise de centre dans $S$ et toutes ses parties passent ces tests, par les lemmes M et S de L01 ; et une présentation jugée ici l'était aussi par la v10, car un centre dans $S$ impose que les droites de ses faces rencontrent $\bar{S}$. Les deux énumérations jugent donc exactement les mêmes présentations ; seule la population des candidats rejetés change. Contrôle : dumps identiques (annexe A).

Ce choix répond à la question posée sur le filtre des triplets. Le test de la droite coûtait 145 cycles pour écarter 29 % des triplets ; il servait surtout à réduire les quadruplets (× 2,4 sans lui, mesuré). Quand un quadruplet coûte une quinzaine d'opérations vectorielles au lieu de 271 cycles, le bilan s'inverse à K = 5 et s'équilibre à K = 10 (modèle du § 8.2 : 106 contre 124 opérations-cycles par boule à K = 5, 214 contre 202 à K = 10). À coût égal, la forme plate est retenue : un lemme, un prédicat de degré 3 et une table en $m^{2}$ de moins. La variante à droite reste dans le prototype de mesure (P1, § 9) ; elle ne revient que si elle gagne plus de 10 % à K = 10 sans perdre à K = 5.

### 4.3 Sélectivités mesurées, et ordre des tests

Sonde de cette conception, énumération plate (trame 02 à K = 5 ; quart 01 à K = 10) :

| Population | K = 5 | K = 10 | Part gardée |
|---|---:|---:|---|
| triplets de poids admissible | 28 605 413 | 20 403 360 | — |
| … strictement aigus | 11 116 579 | 7 046 073 | 38,9 % ; 34,5 % |
| … de centre dans la boîte (jugés q3) | 1 912 022 | 823 938 | 17,2 % ; 11,7 % des aigus |
| quadruplets de poids admissible | 22 247 288 | 25 831 831 | — |
| … dont l'enveloppe rencontre la boîte | 16 775 434 | 17 872 282 | 75,4 % ; 69,2 % |
| … de centre dans la boîte | 5 002 524 | 5 674 382 | 22,5 % ; 22,0 % |
| … de centre strictement intérieur (avec ou sans boîte) | 1 112 515 | 1 002 750 | 5,0 % ; 3,9 % |
| … jugés q4 (centre dans la boîte et intérieur) | 481 163 | 387 664 | 2,2 % ; 1,5 % |
| juges ; boules | 3 318 628 ; 1 407 885 | 1 450 964 ; 767 555 | 42 % ; 53 % des juges émettent |

Ordre retenu, du moins cher et du plus sélectif au plus cher :

- **triplets** : aiguïté d'abord (degré 2, exacte, 39 % de passage), compaction, puis centre dans la boîte (degré 5) ;
- **quadruplets** : centre dans la boîte d'abord (degré 4, exact en binaire64 pour 88 à 93 % des feuilles, 22 % de passage), compaction, puis intérieur strict (degré 6) **en réemployant** le numérateur $N'$ et les produits vectoriels du centre (38 opérations au lieu de 74 pour la forme de Gram). La v10 calculait le centre en 128 bits, testait la boîte, puis l'intérieur par quatre orientations ; la forme J3 testait l'intérieur de Gram d'abord. L'ordre retenu ici vient du coût : l'intérieur seul garde 5 % des candidats mais coûte deux fois le centre, et le centre est de toute façon nécessaire aux survivants ;
- le test d'enveloppe du tétraèdre (75 % de passage) n'est pas gardé : le centre dans la boîte le contient et coûte à peine plus.

### 4.4 Les étages

```text
feuille(S, liste) :                                             chemin rapide : m <= 32, e <= 16
  E0  repere local : y = x - lo(S) en i32 et en binaire64 ; etendue e ; paliers (une fois par feuille)
  E1  dominance : dom[i] (u32), une ligne vectorielle par site                    [exact]
  E2  paires : P2[i], P3[i], P4[i] (u32) ; lot J2 des paires dont le milieu est dans S   [exact]
  E3  triplets : enumeration -> lot T ; noyau K3a (aigu) ; compaction ;
                 noyau K3b (centre dans la boite) -> lot J3 ; indecis -> reference
  E4  quadruplets : enumeration -> lot Q ; noyau K4a (centre dans la boite) ; compaction ;
                    noyau K4b (interieur strict) -> lot J4 ; indecis -> reference
  E5  recensement : noyau KS sur J2, J3, J4 (tous les sites de la feuille d'un coup) ;
                    rejet si p > theta_q ; coquille etendue ou site indecis -> reference
  E6  emission : identifiants (compression par masque), reference de tri
```

**E1, dominance.** Pour la ligne $i$, un vecteur sur $j$ : trois soustractions, trois maximums avec zéro, deux additions, une soustraction, une comparaison (10 opérations pour 16 tests dirigés). Les produits $P_k = 2 h_k y_k$ et $A$ sont calculés une fois par site. C'est le même noyau que celui des nœuds à masques (§ 5.2).

**E2, paires.** Pour la ligne $i$ : OU des masques, `popcount` vectoriel, trois comparaisons de seuil, test de non-dominance mutuelle, milieu dans la boîte (trois additions, six comparaisons). Environ 15 opérations pour 16 paires.

**E3 et E4, énumération.** Elle ne lit que des masques et écrit des indices.

- Triplets : pour chaque paire $(i, j)$ de $P_3$, les candidats $k$ sont les bits de $P_3[i] \wedge P_3[j]$ au-dessus de $j$ ; le poids de l'union est testé pour tous les $k$ à la fois (`popcount` vectoriel sur la colonne des `dom`), et les $k$ gardés sont écrits par compression.
- Quadruplets : la feuille tient la liste de ses paires $(k, l)$ de $P_4$ avec leur masque $\mathrm{Dom}[k] \cup \mathrm{Dom}[l]$ ; pour chaque paire $(i, j)$ de $P_4$, un vecteur parcourt les paires $(k, l)$ avec $k > j$, teste l'appartenance de $k$ et $l$ au voisinage commun de $i$ et $j$ et le poids de l'union des quatre, puis écrit par compression. Chaque clique est produite une fois ($i < j < k < l$). Les arêtes $(i, k)$, $(i, l)$, $(j, k)$, $(j, l)$ sont garanties par le voisinage commun.
- La forme scalaire (boucles sur les bits, comme la v10) est la référence ; les deux rendent la même liste, à l'ordre près.

**K3a, K3b, K4a, K4b.** Chaque noyau lit un lot d'indices, rassemble les coordonnées locales par permutation depuis des registres (la feuille a au plus 32 sites : deux registres de 16 valeurs par axe, une instruction `vpermt2ps` par coordonnée, puis conversion en binaire64), évalue 8 candidats par vecteur sans branchement et rend un masque « certain vrai », « certain faux », « indécis ». Comptes d'opérations vectorielles par vecteur de 8 candidats (établis en écrivant chaque noyau ; à remplacer par la mesure P1) :

| Noyau | Contenu | Opérations par vecteur de 8 | Appliqué à |
|---|---|---:|---|
| K3a | rassemblement de 9 coordonnées ; $u$, $v$ ; trois produits scalaires ; trois comparaisons | 38 | tous les triplets |
| K3b | rassemblement ; $uu$, $vv$, $w$, $t$, $N$, $\lVert w \rVert^{2}$ ; $a D + N$, $h D$ ; majorants et seuils ; comparaisons | 100 (79 dans le palier exact) | les aigus (35 à 39 %) |
| K4a | rassemblement de 12 coordonnées ; $u$, $v$, $s$ ; trois produits vectoriels ; $\det$ ; $uu$, $vv$, $ss$ ; $N'$ ; $a D + N$, $h D$ ; comparaisons (majorants hors palier exact) | 86 dans le palier exact, 125 sinon | tous les quadruplets |
| K4b | recalcul de $N'$ et des produits vectoriels ; $A'$, $B'$, $C'$, $2 \det^{2}$ ; majorants ; comparaisons | 122 | ceux dont le centre est dans la boîte (22 %) |
| KS | pour un juge : différences à l'ancre, $\lVert d \rVert^{2}$, $D \lVert d \rVert^{2}$, $2 N \cdot d$, majorant, deux comparaisons | 22 par vecteur de 8 sites | 2 vecteurs (K = 5), 3 (K = 10) par juge |

**E5, recensement.** Le centre approché $(\tilde{N}, \tilde{D})$ et ses majorants sortent de K3b ou de K4a et voyagent dans le lot des juges ; les générateurs sont sur la sphère par construction et sont masqués. Le poids intérieur est un `popcount` du masque « certainement dedans ». Trois issues : rejet ($p > \theta_q$) ; boule à coquille régulière (aucun site indécis) : émission ; sinon chemin exact. Il n'y a plus de sortie anticipée (elle intervenait après 11,5 sites sur 14 en moyenne, L05 § 6.3) ni de masques de décision préalable : tous les sites sont évalués d'un coup. Le lemme M sert en amont : le test de poids de l'énumération est le seul rejet sans arithmétique.

**Coquilles étendues** (0,02 à 0,04 % des boules). Un site indécis est décidé par la référence ; s'il est sur la sphère, la coquille $U$ dépasse les générateurs : la feuille consulte un **mémo des masques de coquille** déjà émis (lemme U : la coquille détermine la boule), puis calcule $q_{\min}$ et $S^{*}$ par la référence (port de `support.hpp`), vérifie l'admission $p + q_{\min} \leq K + 1$ et émet. Une coquille de plus de 24 sites est un refus `unsupported_degeneracy` immédiat, avec la même limite que la tour (L01 C7).

**E6, émission.** Les identifiants de $I$ sortent par compression du vecteur des `SiteIdx` de la feuille sous le masque intérieur ; l'ordre local est l'ordre des sites, donc $I$ et $U$ sortent triés. La clé approchée est calculée ici (§ 3.5).

### 4.5 Les trois noyaux et leur identité

| Noyau | Rôle | Largeur |
|---|---|---|
| référence | définition entière exacte ; repli des indécis ; feuilles de 33 à 64 sites ; feuilles d'étendue $e > 16$ ; coquilles étendues | scalaire, `num::Int<bits>` |
| AVX2 | portabilité (codespace, intégration continue) | 4 binaire64, 8 `i32` ; compaction par table, `popcount` scalaire |
| AVX-512 | cible du contrat (G4) | 8 binaire64, 16 `i32` ; `vpcompress`, `vpopcntd`, masques `k` |

Les noyaux flottants sont écrits une fois, en gabarit sur un petit paquet de voies (largeur 1, 4 ou 8), avec des opérations explicites (`fma` nommé, aucune liberté laissée au compilateur). L'identité se juge à deux niveaux : (a) porte unitaire lot par lot : mêmes masques « certain vrai » et « certain faux » ou, à défaut, mêmes décisions finales après repli, entre référence, largeur 1, AVX2 et AVX-512 ; (b) porte de bout en bout : même empreinte du catalogue et même grand livre déterministe pour les trois constructions.

### 4.6 Chemin générique

Une feuille forcée (boîte de côté 1 et liste de plus de $M$ sites) de 33 à 64 sites est traitée par la référence avec des masques de 64 bits. Au-delà de 64 sites : refus `resource_exhausted/wide_leaf`. Le travail d'une feuille est donc borné par $\binom{64}{4} = 635\,376$ quadruplets.

---

## 5. Arbre et ordonnancement

### 5.1 Un nœud, trois zones

La règle d'un nœud est celle de la v10 (J2c), qui est exacte et mesurée (33 à 40 % de nœuds en moins que l'octree) :

```text
noeud(Q, Lp) :                              Lp : liste K-certifiee pour une boite qui contient Q
  L = filtre(Q, Lp)                         retire x si au moins K sites de Y le dominent sur Q-ferme (lemme D)
  si L est vide : ignore
  S = Q inter [env(L).lo, env(L).hi + 1)    lemme A ; S vide : ignore
  si |L| <= M(K) ou plus long cote de S <= 1 : feuille(S, L)
  sinon : couper S au milieu de son plus long cote ; noeud(bas, L) ; noeud(haut, L)
```

Ce qui change est la façon de filtrer et de stocker, selon la taille de la liste parente :

| Zone | Liste parente | Filtre | Stockage | Exécution |
|---|---|---|---|---|
| haut | plus de $N_{\mathrm{task}} = 512$ sites | réservoir de $3K$ sites, vectorisé le long des sites ; par tranches de 2 048 sites au-delà de 8 192 | arènes de tour | tours en largeur, parallèles |
| tâche | 33 à 512 sites | le même | pile de listes du fil | profondeur d'abord, dans une tâche |
| masques | au plus 32 sites | dominance de toutes les paires, $Y$ = la liste | masque `u32` sur un tableau local | profondeur d'abord, dans la même tâche |

Répartition mesurée (sonde, trame 02, K = 5) : la zone des masques porte 79 % des nœuds et 38 % des visites de sites ; les listes de 33 à 512 sites environ 20 % des nœuds et 35 à 40 % des visites ; le haut moins de 1 % des nœuds et environ un quart des visites.

### 5.2 Nœud à masques

Une racine de la zone (liste qui vient de passer sous 33 sites) copie ses sites une fois dans un tableau local. Ensuite un nœud est un couple (boîte, masque des sites vivants) :

1. coordonnées locales à la boîte du nœud (trois soustractions vectorielles), $A$ et $P_k$ par site ;
2. pour chaque site vivant $i$, une ligne : masque des sites vivants qui dominent $i$ sur la boîte (10 opérations pour 16 tests) ;
3. $i$ reste vivant si le `popcount` de sa ligne est $< K$ ;
4. enveloppe des vivants (minimum et maximum masqués), boîte ajustée, feuille ou coupe.

Rien n'est copié d'un niveau au suivant ; il n'y a ni réservoir ni sélection. **Exactitude** : proposition L avec $Y$ égal à la liste parente (L01 A.3 : la liste filtrée reste K-certifiée quel que soit $Y \subseteq X$). Le filtre est au moins aussi fort que celui de la v10 sur la même boîte et la même liste parente.

Mesuré (sonde) : mêmes dumps ; trame 02 à K = 5, 709 349 nœuds au lieu de 734 083 (− 3,4 %), 313 740 feuilles au lieu de 323 224 ; 558 904 nœuds à masques pour 247 millions de tests dirigés (176 par boule ; la v10 en fait 287 par boule dans tout l'arbre, à 5,5 cycles pièce, plus 26 cycles par visite pour le réservoir). Avec le seuil à 64 sites : 702 955 nœuds, 391 millions de tests (278 par boule), et 10,1 visites de réservoir par boule au lieu de 13,1. Le modèle du § 8 ne départage pas 32 et 64 (372 contre 376 opérations par boule) ; **32 est retenu** parce qu'il donne un seul gabarit de masque pour les nœuds et les feuilles, et 64 reste une variante de P2.

La feuille recalcule sa dominance sur sa boîte ajustée (étage E1) : la boîte a rétréci, les masques sont plus forts, et c'est le même noyau.

### 5.3 Filtre à réservoir vectorisé (listes de plus de 32 sites)

Même définition que la v10, donc mêmes listes : réservoir des $\min(\lvert L_p \rvert, 3K)$ sites de plus petite clé $(dd, \text{rang})$, $dd = \lVert 2y - h \rVert^{2}$ ; $x$ est retiré si au moins $K$ sites du réservoir le dominent.

- Passe 1 : $dd$ pour tous les sites (vectoriel), sélection des $3K$ plus petits par un seuil courant (comparaison vectorielle au seuil, insertion scalaire des rares candidats). Le départage par rang garde la sélection exacte et indépendante du jeu d'instructions.
- Passe 2 : vecteur le long des sites, boucle sur les dominateurs (11 opérations par dominateur et par vecteur), compteur par voie, puis écriture de la liste fille par compression (`idx`, `x`, `y`, `z`) et enveloppe par minimum et maximum masqués.
- Largeur : `i32` × 16 quand l'étendue du nœud vérifie $e \leq 13$, binaire64 × 8 sinon (exact jusqu'à $e \leq 24$, donc pour tout le profil $B \leq 24$ à $T = 0$).

Coût du modèle : 12 opérations par visite à K = 5 en `i32`, 24 en binaire64 ; 22 et 45 à K = 10 (30 dominateurs). La v10 mesure 5,5 cycles par test et 26 cycles par visite pour le réservoir, soit environ 100 cycles par visite.

### 5.4 Taille de feuille, marge, arrêt

- $M(K)$ = 16 pour $K \leq 6$, 24 pour $K \leq 10$, 28 au-delà : constante interne, gardée par `static_assert` ($M(K) - K \geq 10$ ; la marge gouverne le coût : 14 points à K = 10 coûtent 1 nœud à $M = 24$ et 3 millions à $M = 13$, L01 § 6.4). L'ablation de l'audit place le minimum du modèle à 16 et 24, avec les coûts unitaires de la v10 comme avec ceux des leviers ; les coûts de la v11 étant autres (un candidat coûte beaucoup moins, un nœud aussi), le calibrage P-CAL du § 9 le rejoue une fois sur G4, puis la table est figée.
- Arrêt : liste d'au plus $M$ sites, ou plus long côté de la boîte ajustée égal à 1. Pas de règle de stagnation : la profondeur est bornée par $3(B + T) + 1$ (chaque coupe divise un côté d'une boîte de côté au plus $2^{B + T}$), ce qui borne aussi les piles.
- **Listes inhérentes** (lemme F) : une liste ne descend pas sous $\lvert N_K(c) \rvert$ pour $c$ dans la boîte. Une feuille forcée de plus de 64 sites signale donc plus de $64 - K$ sites à égale distance d'un point d'une boîte de côté 1 : refus `wide_leaf`, cohérent avec la limite de coquille de la tour.

### 5.5 Ordonnancement

1. **Haut, par tours.** La frontière du tour $r$ (nœuds dont la liste dépasse 512 sites) est traitée par une boucle parallèle à grain 1 ; chaque fil écrit les listes filles dans son arène de tour et pousse ses enfants, étiquetés (ordinal du parent, moitié). En fin de tour, les enfants sont rangés par étiquette (somme préfixe), ce qui fixe leur ordinal quel que soit le nombre de fils. Un enfant dont la liste tient en 512 sites devient une **racine de tâche** (boîte et indices, copiés dans le tampon des tâches) ; les autres forment la frontière suivante. Les nœuds de plus de 8 192 sites (257 nœuds sur la trame 02) découpent leur liste en tranches de 2 048 sites : réservoirs par tranche fusionnés par clé $(dd, \text{rang})$, filtre par tranche, concaténation par préfixes (procédé du prototype privé `frontiere_v3`, 98 configurations sans écart d'après `PLAN_PERF.md`).
2. **Tâches.** Une boucle parallèle dynamique à grain 1 sur les racines de tâche, rangées par taille de liste décroissante. Une tâche descend en profondeur avec la pile de listes de son fil, entre dans la zone des masques, énumère ses feuilles, émet dans l'arène du fil. Ordre de grandeur attendu sur une trame : 5 000 à 8 000 tâches de 200 à 800 boules.
3. **Pas de vol de tâches.** Le haut ne compte que quelques milliers de nœuds et une quinzaine de tours ; les tâches sont bornées (liste d'au plus 512 sites), donc l'écart final entre fils est d'au plus une tâche. Cela suppose un pool dont les ouvriers ne se rendorment pas entre deux régions d'une même construction (§ 11).
4. **Déterminisme.** Les sorties publiées sont rangées par l'ordre canonique total (niveau exact, $S^{*}$), donc indépendantes de l'ordonnancement ; le grand livre est une somme ; un refus est celui de plus petite priorité (raison, puis ordinal de tâche).

Comparaison avec la v10 (mesuré G4, trame 02) : la frontière en largeur prend 23 à 27 ms à K = 5 parce qu'elle descend jusqu'à des tâches de 14 sites en 29 tours, avec un filtre scalaire à 5,5 cycles par test et une copie de liste par nœud. Ici les tours s'arrêtent aux listes de 512 sites et le filtre est vectoriel.

### 5.6 Budgets et refus

| Garde | Seuil | Refus |
|---|---|---|
| entrée à doublons | un site de poids $> 1$ | `unsupported_degeneracy` (raison à créer : multiplicité) |
| $K$ hors de $[1, 12]$ | — | `invalid_input` |
| nombre de nœuds | $256\,n + 65\,536$ (mesuré : 14 à 32 nœuds par site) | `resource_exhausted` (budget de nœuds) |
| feuille forcée | plus de 64 sites | `resource_exhausted` (`wide_leaf`) |
| coquille | plus de 24 sites (limite de la tour) | `unsupported_degeneracy` |
| boules, identifiants | $2^{32} - 1$ | `resource_exhausted` (`index_overflow_u32`) |
| mémoire | plafond du budget | `resource_exhausted` (`memory_budget`), avant toute publication |
| boule émise deux fois, bande incohérente | — | `invariant_violated` |

Chaque raison n'entre dans `reasons.def` qu'avec le code qui l'émet et la porte qui la provoque (règle du socle).

---

## 6. Ordre canonique et assemblage

### 6.1 Objet

Ordre publié : (niveau exact, $S^{*}$ comparé comme quadruplet d'indices complété par `kNone`), rang dense par niveau exact distinct ; c'est l'ordre de la v10.

### 6.2 Clé et seaux

- La référence de tri (16 octets, § 2.5) porte la clé approchée, positive, d'exposant F3 au plus 10. Deux clés positives se comparent comme leurs motifs de bits lus en entiers non signés.
- **Seaux** : indice = bits de tête de la clé (exposant et 6 bits de mantisse), soit 64 seaux par octave et au plus $64 (2B + 4)$ seaux (2 560 pour $B = 18$). Chaque fil compte ses boules par seau à l'émission.
- Après l'étage des tâches : sommes préfixes sur (seau, fil), puis chaque fil recopie ses références à leur place dans le tableau global (parallèle, positions fixées par les préfixes).

### 6.3 Tri par seau, bandes, réparation exacte

Par seau, en parallèle (boucle dynamique) :

1. tri des références par clé (tri par base sur les bits restants ; un seau tient dans le cache L2) ;
2. **bandes** : deux voisins $x$, $y$ sont certifiés ordonnés si $\tilde{x} < c\,\tilde{y}$, $c = 1 - 2^{-40}$ (F4, clés strictement positives). Une bande est une suite maximale de voisins non certifiés ;
3. **réparation** : dans chaque bande de plus d'un élément, niveaux exacts recalculés depuis $S^{*}$ (lu dans l'arène), tri par (niveau exact, $S^{*}$). La comparaison exacte se fait dans le plus petit type qui porte les produits croisés des deux niveaux (i128 quand leurs longueurs le permettent, large sinon). Deux $S^{*}$ égaux dans une bande : refus `invariant_violated` (boule émise deux fois) ;
4. **rangs locaux** : le rang augmente à chaque frontière de bande (ordre strict certifié) et, dans une bande, à chaque inégalité exacte stricte.

**Couture.** Une bande peut franchir la frontière de deux seaux voisins : les queues et têtes de seau non certifiées contre le voisin sont laissées à une seconde passe, qui trie exactement la plage contiguë à cheval. Une bande ne couvre jamais un seau entier (lemme des bandes, annexe B.7 : son étalement relatif est inférieur à $n\,2^{-40}$, celui d'un seau vaut au moins $2^{-7}$), donc les plages de couture sont disjointes.

Pourquoi des bandes nombreuses : les niveaux **égaux** sont fréquents (trame 02 : 1 099 581 niveaux pour 1 407 885 boules à K = 5 ; 124 159 bandes et 698 799 membres à K = 10 dans la v10). Ce sont surtout des paires de même $\lVert u \rVert^{2}$, que la comparaison exacte tranche en entiers de 64 bits.

### 6.4 Une passe de remplissage

Sommes préfixes sur les seaux (boules, rangs, identifiants, coquilles étendues), réservation du catalogue final dans le budget, puis, par seau et en parallèle : `rank`, `ids_off`, compteurs, copie des identifiants depuis l'arène, `level_rep` au premier élément de chaque rang, table `ext`. Les enregistrements ne sont lus qu'ici (et, pour les membres de bandes, à l'étape 3) : plus de relecture au hasard d'enregistrements de 104 octets par trois passes.

### 6.5 Ce qui disparaît

- La comparaison exacte de **tous** les voisins (0,15 s à K = 5 et 0,61 s à K = 10 sur un fil G4) : F4 est un énoncé prouvé, il est invoqué. Le mutant « bande non réparée » et le mutant « seuil de bande faux » sont tués par des fixtures de niveaux égaux et quasi égaux (§ 7.3), pas par une vérification à l'exécution.
- Le niveau exact de 56 octets par boule puis par rang ; la clé de 32 octets ; le repérage des bandes en série ; la destruction des tampons au retour (les arènes restent à la `Session`).

### 6.6 Variante sans copie

Si la passe de remplissage pèse plus de 3 ms à K = 5 ou 10 ms à K = 10 sur G4 (mesure P3), l'alternative est de publier les identifiants **dans** les arènes d'émission (le catalogue garde `pos` au lieu de `ids_off`). Elle économise la copie et 40 % du pic, mais la disposition en mémoire dépend alors de l'ordonnancement : seules les vues et les empreintes restent canoniques. Elle n'est pas retenue par défaut (règle 7 de l'architecture).

---

## 7. Conformité

### 7.1 Ce qui doit être identique, et les écarts déclarés

Le catalogue de la v11 est l'objet $\mathrm{Cat}_K(X)$ de L01 (annexe A.2) ; sa sortie canonique au **format de dump de la v10** (une ligne par boule : rang, $q_{\min}$, $p$, $u$, drapeaux, $S^{*}$, $I$, $U$ en coordonnées) doit être identique octet pour octet à celle du binaire v10 figé sur toute entrée que la chaîne v10 sert. Épingles : quart 01 `414aa4d47ffe1c55…` (K = 5) et `7c46e50a72c08087…` (K = 10) ; trame 02 `8a850649ff103c1c…` (K = 5, 1 407 885 boules : 518 233 / 746 547 / 143 105 par $q_{\min}$, 572 étendues) et `d6abe0dba4d9…` (K = 10, 5 483 320 boules, 1 301 étendues) ; trame brute avec sol 2 822 052 boules à K = 5.

Écarts voulus, à écrire dans la spécification :

| Entrée | v10 | v11 | Raison |
|---|---|---|---|
| positions en double | catalogue avec clause pondérée propre, puis refus de la tour | refus à l'entrée du générateur | G01 ; la tour refuse de toute façon |
| feuille forcée de 65 à 256 sites | énumérée (20 s pour 144 points cosphériques) | refus `wide_leaf` | G20 ; la tour refuse ces coquilles |
| coquille de plus de 24 sites | émise, puis refus de la tour | refus à l'émission | limite unique (L01 C7) |
| représentation d'un niveau | `emitted_level` (artefact d'une feuille retirée) | définie par $S^{*}$ seul | L01 C9 ; le dump du catalogue n'imprime pas les niveaux ; pour la tour, comparer des fractions réduites |
| grand livre de l'arbre | boîtes à $T = 6$, réservoir partout | $T = 0$, nœuds à masques | même catalogue, autre arbre : les compteurs de nœuds ne se comparent pas à ceux de la v10 |

### 7.2 Portes

Reprises de L05 § 12.2, L01 § 12, L08 § 11.2 ; chacune naît dans le commit du code qu'elle garde, avec un plancher par strate.

| Porte | Label | Contenu | Plancher |
|---|---|---|---|
| `mhgp11_num_predicates` | `oracle` | chaque prédicat de référence contre `Fraction`, aux coins du domaine et à une unité de l'égalité, pour chaque palier de type (i64, i128, large) | $10^{4}$ cas par prédicat, $10^{3}$ égalités exactes |
| `mhgp11_num_filters` | `unit` | chaque noyau filtré sur ses expressions réelles : quatre modes d'arrondi, contraction active et inactive, parenthésages, zéro exact, signe à une unité d'un grand majorant, bornes du domaine ($e$ = 7, 9, 12, 13, 16 et voisins) | repli atteint au moins 100 fois par filtre ; aucune décision certaine fausse |
| `mhgp11_catalogue_kernels` | `unit`, `fast` | identité lot par lot entre référence, largeur 1, AVX2, AVX-512 (masques de décision et sorties) ; queues de lot, lots vides, lots pleins | chaque noyau, chaque largeur |
| `mhgp11_catalogue_oracle` | `oracle` (+ échantillon `fast`) | oracle brut indépendant (C++ de L05, Python de L01) : enregistrements dans l'ordre publié, $S^{*}$ minimal, rang dense ; K de 1 à 12 ; feuilles forcées de 33 à 64 sites ; pleine magnitude ; familles dégénérées | par strate : $q$, coquilles étendues par taille, chemin générique, chaque palier d'étendue, chaque repli |
| `mhgp11_catalogue_restriction` | `scale*`, `lidar` | $\mathrm{cat}(K)$ égale la restriction de $\mathrm{cat}(K + 2)$ (empreinte sans rang) | 8 000, 16 000, 32 000 points, trois trames, K = 5 et 10 |
| `mhgp11_catalogue_euler` | `scale*`, `lidar` | identité d'Euler par ordre sur le catalogue à $K + 2$, forme générale (coquilles étendues) ; **diagnostic d'échelle**, jamais une certification de complétude (réponse Q3 de l'auditeur) | chaque exécution d'échelle |
| `mhgp11_catalogue_boxes` | `scale*`, `lidar` | juge par échantillon, sans rien d'exhaustif : (a) recensement brut de boules tirées ; (b) pour des feuilles tirées, $N_K(c)$ brut inclus dans la liste en des points rationnels de la boîte (coins, centre, hasard) ; (c) uplets de sites voisins tirés, boule brute, présence au catalogue si elle est admise | 2 000 boules, 200 feuilles, 2 000 uplets par entrée |
| `mhgp11_catalogue_identity` | `scale8000`, `lidar` | empreinte identique à 1, 2, 8, 48 fils ; sous permutation, translation, renumérotation des `PointId` ; entre les trois constructions (référence, AVX2, AVX-512) | 3 entrées × 5 transformations |
| `mhgp11_catalogue_diff_v10` | `diff_v10` | dump au format v10 égal à celui du binaire figé | épingles ci-dessus ; tailles d'intérêt ; K = 5 et 10 |
| `mhgp11_catalogue_refusals` | `fast` | une porte par raison (doublon, $K$ hors domaine, feuille large, coquille, budget de nœuds, mémoire, dépassement u32) ; rien de publié sur un refus | chaque raison |
| `mhgp11_catalogue_ledger` | `scale*`, `lidar` | compteurs déterministes par boule dans des fourchettes épinglées (nœuds, visites, candidats par étage, juges) ; identiques entre fils et entre constructions | chaque entrée d'échelle |

### 7.3 Fixtures d'égalité

Celles de L01 (annexe B) : dominateur à égalité en un coin de boîte ; centre sur `env.hi`, sur le plan de coupe, sur une face de la racine ; $p = K - 1$ avec un site sur la sphère, $p = K$ ; $p = \theta_q$ pour $q$ = 2, 3, 4 ; admission $p + q_{\min} = K + 1$ et $K + 2$ ; carré, triangle rectangle, cube, octaèdre, tétraèdre et son centre ; coquille de 24 et de 25 sites ; niveaux égaux de représentations différentes (une q2 et une q3) ; deux niveaux voisins à $2^{-40}$ près ; les 14 points à deux amas (un nœud attendu).

Nouvelles, propres à cette conception : centre q3 et q4 **exactement** sur une face basse et sur une face haute de boîte, dans un palier exact et dans un palier filtré (repli obligatoire) ; site exactement cosphérique dans un palier filtré ; tétraèdre dont le centre est sur une face (intérieur indécis, repli, rejet) ; triangle rectangle ($u \cdot v = 0$) ; quadruplet coplanaire ; listes dont l'étendue vaut exactement $2^{e} - 2$, $2^{e} - 1$ et $2^{e}$ aux frontières de palier 7, 9, 12, 13, 16 ; feuille de 32 et de 33 sites ; liste parente de 32, 33, 512 et 513 sites ; bande à cheval sur deux seaux ; niveaux égaux entre une q2, une q3 et une q4.

### 7.4 Mutants (code 4, appliqués à une copie)

| Mutant | Porte qui doit le tuer |
|---|---|
| les dix de L05 § 5.5 (boîte ajustée sans + 1, seuil des paires, dominance large, sortie du recensement à $p \geq \theta$, masques larges, frontière qui abandonne une tâche, bandes non réparées, filtre compté à $K - 1$, …) | oracle, restriction, identité, fixtures de niveaux |
| seuil d'un filtre divisé par $2^{12}$ ; repli supprimé (indécis traité comme faux, puis comme vrai) | `num_filters`, fixtures de face de boîte et de cosphéricité |
| palier exact étendu d'un bit ($e \leq 10$ au lieu de 9 pour le centre q3, etc.) | `num_filters` aux bornes du domaine |
| queue de lot non masquée ; compaction qui perd ou double un élément | `catalogue_kernels` |
| nœud à masques qui compte les dominateurs à $K - 1$ ; ligne de dominance transposée | oracle, juge des boîtes (b) |
| énumération des quadruplets qui saute une clique (paire $(k, l)$ au bord d'un vecteur) | oracle à K = 10, restriction |
| couture de seaux retirée ; constante de bande à $1 - 2^{-60}$ | fixture de bande à cheval, fixture de niveaux quasi égaux |
| mémo des coquilles étendues retiré | refus `invariant_violated` sur la fixture du carré |
| clé q3 calculée avec une soustraction entre approximations | porte des exposants F3 de `num` |

---

## 8. Budget de temps et de cycles

### 8.1 Nature des chiffres

- **Mesuré** : la v10 (L05, reçu G4 de session 4), les compteurs par boule de la sonde (§ 1), les microbancs de l'audit.
- **Estimé** : la v11. Le modèle compte, pour chaque étage, les opérations vectorielles de 512 bits par élément (écrites noyau par noyau, § 4.4 et § 5) et les multiplie par les compteurs mesurés. Conversion en cycles : **3 opérations vectorielles par cycle** sur Zen 5 (hypothèse : deux unités de multiplication-addition et deux d'addition sur 512 bits ; à mesurer, P-FREQ), plus des coûts scalaires par feuille, par juge et par boule. Conversion en temps : 4,0 GHz (hypothèse cohérente avec la v10 : 2,49 µs par boule pour 10 200 cycles ; à relever), et 48 fils comptés pour 27,6 fils équivalents quand le code est vectoriel (24 cœurs × 1,15 ; la v10, scalaire, obtient × 34 sur l'étage des boîtes).
- Le modèle ignore les défauts de cache, les mauvaises prédictions résiduelles et le rendement des petits lots. Il donne un **plancher**. La **cible de conception** est le double du plancher ; le **plafond d'acceptation** est le quart du travail de la v10.

### 8.2 Opérations et cycles par boule émise

| Étage | v10 mesuré, K = 5 (cycles) | v11 modèle, K = 5 (opérations → cycles) | v10 mesuré, K = 10 (cycles) | v11 modèle, K = 10 (opérations → cycles) | Ce qui fonde le modèle |
|---|---:|---|---:|---|---|
| filtre à réservoir (haut et tâches) | 2 315 | 13,1 visites × 18 = 236 → 79 | 1 638 | 5,7 visites × 32 = 181 → 60 | visites mesurées (sonde) ; 12 à 45 opérations par visite selon largeur et $K$ |
| nœuds à masques | — | 176 tests × 0,625 + 16 = 126 → 42 ; plus 20 cycles de gestion | — | 124 × 0,625 + 7 = 84 → 28 ; plus 10 | tests mesurés ; 10 opérations pour 16 tests |
| repère, dominance et paires de feuille | 1 316 | 85 → 28 | 1 060 | 123 → 41 | 25 opérations par ligne et par vecteur de 16 |
| triplets (énumération, aigu, centre) | 2 968 | 79 + 96 + 91 = 266 → 89 | 3 344 | 171 + 126 + 108 = 405 → 135 | 20,3 et 26,6 triplets, 7,9 et 9,2 aigus par boule |
| quadruplets (énumération, centre, intérieur) | 1 795 | 68 + 175 + 54 = 297 → 99 | 3 413 | 234 + 381 + 113 = 728 → 243 | 15,8 et 33,7 quadruplets, 3,55 et 7,39 dans la boîte |
| recensement | 918 | 132 → 44 ; plus 35 scalaires | 1 199 | 147 → 49 ; plus 28 | 2,36 et 1,89 juges par boule |
| émission | 602 | 50 | 669 | 50 | 115 cycles mesurés avec l'ancien format, pages touchées |
| reste de la feuille (boucles, replis, coquilles étendues) | ≈ 290 | 33 | ≈ 210 | 16 | 150 cycles par feuille ; replis mesurés $< 10^{-5}$ |
| ordre et assemblage | ≈ 1 200 | 100 | ≈ 1 220 | 100 | partition 10, tri 20, bandes et exact 25, rangs 5, remplissage 40 |
| **total** | **≈ 11 400** | **≈ 620** | **≈ 12 750** | **≈ 760** | |

Variante à droite des équidistants, pour comparaison (même modèle) : triplets et quadruplets coûtent 99 cycles par boule en forme plate contre 123 à 161 avec la droite à K = 5, et 243 contre 221 à 278 à K = 10, selon que l'énumération par la table des triplets vivants est vectorisée ou non. D'où G07 et la mesure P1.

### 8.3 Temps sur G4, 48 fils, trame 02

| Étage | K = 5, v10 mesuré (ms) | K = 5, v11 plancher | K = 5, v11 cible | K = 10, v10 mesuré (ms) | K = 10, v11 plancher | K = 10, v11 cible |
|---|---:|---:|---:|---:|---:|---:|
| haut de l'arbre (v10 : frontière) | 23,0 à 26,6 | 1,3 | 2 | 38,3 à 39,6 | 2 | 3 |
| tâches (v10 : boîtes) | 101,5 à 103,3 | 6,4 | 13 | 435,0 à 441,6 | 32 | 64 |
| ordre et assemblage | 32,1 à 32,5 | 2 | 4 | 122,0 à 123,8 | 8 | 14 |
| hors étages (tampons) | 7,7 à 8,7 | ≈ 0 | ≈ 0 | 22,8 à 22,9 | ≈ 0 | ≈ 0 |
| **catalogue** | **164,3 à 171,1** | **≈ 10** | **≈ 19** | **618,1 à 627,9** | **≈ 42** | **≈ 81** |
| travail CPU à 48 fils (s) | 6,04 | — | ≈ 0,85 | 25,6 | — | ≈ 3,6 |
| plafond d'acceptation (quart de la v10) | | | 43 ms ; 1,5 s CPU | | | 157 ms ; 6,4 s CPU |

Colonnes « v10 mesuré » : passe chaude (`TABLE_VERITE_G4.md` § 1) à processus froid (L05 § 6.6). Colonnes v11 : estimées ; l'étage d'assemblage est borné par le débit mémoire à K = 10 (environ 1,1 Go lus et écrits), pas par le calcul.

### 8.4 Lecture

- À K = 5, un catalogue de 19 à 30 ms laisse 70 à 80 ms à la tour, dont l'audit estime le coût entre 42 et 62 ms en CPU seul (L06 § 9.2) : le contrat de 100 ms devient plausible, sans marge.
- À K = 10, le catalogue seul vaut 80 à 125 ms dans la cible et 42 ms au plancher du modèle, et la tour reste estimée au-dessus de 300 ms en CPU seul (L06) : **le contrat de 100 ms à K = 10 n'est pas atteint par cette conception**. Elle le rapproche d'un facteur 5 à 8 côté catalogue ; le reste relève de la taille de l'objet (5,5 millions de boules, L01 C5) et d'un portage des noyaux par lots sur l'appareil, que la forme retenue (lots d'indices, noyaux sans branchement, repli exact sur l'hôte) rend possible sans le décider.
- Si les filtres F6 étaient refusés (repli entier exact hors des paliers F2), le modèle ajoute environ 800 cycles par boule à K = 5 (centre q3, intérieur et recensement q3 en entiers de 64 ou 128 bits pour 60 à 95 % des feuilles) : le total passe vers 1 400 cycles au plancher, soit un facteur 4 tout juste tenu une fois doublé. Les filtres ne sont donc pas un confort.
- Trame avec sol (123 389 sites, 2,82 M boules à K = 5) : le coût suit la sortie (L05 § 6.1), donc environ le double de la trame sans sol.

