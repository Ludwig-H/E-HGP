# PISTES_DE_RUPTURE — ce qui pourrait changer l'ordre de grandeur du moteur v11

2 octobre 2026. Conception pour `morsehgp3D_v11` (modules `index`, `catalogue`, `tower`). Ce document ne conçoit pas le générateur ni la tour dans leur forme actuelle (deux conceptions voisines s'en chargent) : il cherche ce qui changerait l'ordre de grandeur, le chiffre, et dit ce qu'il faut garder, prototyper ou écarter. Heures lues par `date -u` : expériences de 08:03 à 08:24 UTC, rédaction close à l'heure de la dernière ligne.

```text
phase=exploration_v11_hors_registre (conception)
backend=cpu_reference
profile=quantized_u18_input_only
mode=conception_moteur (lecture seule hors de build/v11-persist/conception/)
public_status=not_claimed
GCP non utilisé
```

**Vocabulaire.** *Prouvé* : argument écrit, ici ou dans un rapport cité, à contre-lire. *Mesuré* : nombre produit par une exécution ; le lieu et l'auteur sont dits (reçus G4 de la v10 ; rapports L01 à L06 de l'audit du 2 octobre ; « ici » = expériences de l'annexe A, sur le codespace). *Estimé* : mesure multipliée par un rapport ou un coût unitaire supposé, fondement dit. *Conjecturé* : sans fondement mesuré. Aucun temps local n'est lu en valeur absolue (8 cœurs, charge 17 à 24 pendant ce travail) : ici ne comptent que des compteurs déterministes et des égalités d'empreintes.

**Sources.** Lus en entier : `audit_v10/L01`, `L02`, `L05` (dont le § 6.10 ajouté à 08:03), `L06`. Lus en partie : `L04` (§ 1 à 3, C01 à C11) ; `L03` (§ 0, § 3) ; `morsehgp3D_v11/docs/ARCHITECTURE.md` (état de 08:06, règles F1 à F6, § 7) ; les sept notes de `morsehgp3D_v11/audits/` (dont les réponses de l'auditeur aux cinq verrous) ; `build/v10-perf/PLAN_PERF.md` (entier) ; `build/v10-persist/gpu_design/` (réduction CPU en entier ; voie complète § 0 à 2, 11, 14 ; voie partielle § 0 et 1 ; jugement § 0, 1, 5 à 8) ; `build/v10-perf/gpu/out/` et `gpu-verif/out/` (bilans, modèle SIMT) ; les reçus `g4_session4_j2c`, `g4_session5_scale` (faits de la VM), `g4_session7_cuda_probe` et `ERRATA.md` ; `generator.cpp` (feuille et juge) et la boucle `resolve` de `tower.cpp` de la v10 ; `GEN_v2.md` § 3.9, 4, 9, 10, 11.9 ; `GEN_v1.md` § 3 et `PROTOTYPES_SESSION_20260928.md` (marche sur le niveau) ; `conception/CONCEPTION_TOUR.md` (état de 08:26, § 0 à 2). Absents à l'heure de la rédaction : `L09`, `L15`, les vérifications `V_L*`, `MATHEMATIQUES.md` de la v11.

---

## 0. Réponse courte

1. **Aucune piste examinée ne change l'ordre de grandeur du travail par boule.** L'objet lui-même fixe le coût : 1,41 M boules et 1,68 M nœuds de tour à $K = 5$, 5,48 M et 7,47 M à $K = 10$ sur la trame 02 (mesuré, L01). Les gains réels sont des facteurs 2 à 3 sur les constantes, déjà identifiés par l'audit, plus quelques décisions de structure chiffrées ici.
2. **Candidats par boule (48 à 58) : c'est un plancher de la méthode, pas un défaut de réglage.** Décomposition mesurée : 44 % de tests de dominance de paires (toutes les paires de chaque feuille), 42 % de triplets (un triplet utile sur 22 testés : 3,8 feuilles par triplet, et 5,7 triplets distincts par triplet utile), 14 % de quadruplets. La marche sur les arêtes de Voronoï, la génération depuis les paires q2 et le balayage de la droite des centres sont **écartés sur mesure**. Reste un levier d'ordre des tests (feuille « plate »), qui ne paie qu'avec des quadruplets à 30 cycles pièce.
3. **Tour : le nombre de pas de descente de la v10 est à 7 à 8 % du minimum** (sur un même quart de trame, 110 450 plus petites boules calculées par la v10 contre 102 499 pour une descente idéale rejouée ici à $K = 5$ ; 701 328 contre ≈ 647 000 à $K = 10$). Le générateur **ne peut pas** fournir le successeur des morceaux hors semis : sa liste ne certifie le recensement de la boule du morceau que dans 13 à 14 % des cas à $K = 5$ et 35 % à $K = 10$, et jamais aux deux ordres les plus hauts. Ce qui se gagne : pas d'étages d'atlas et de semis séparés, jointure locale (93 à 99 % des liens de semis restent dans un même bloc de Morton), index implicite.
4. **Un seul ordre $K$** : 59,1 % du catalogue à $K = 5$, 36,5 % à $K = 10$, et 21 à 37 % du travail de la tour ; mais **la même énumération** (le filtre exact propre à un ordre ne retire que 1 à 9 % des candidats, mesuré ici). C'est un chemin de produit utile, pas une rupture.
5. **GPU : hors du premier moteur.** Le flottant double de cette carte ne dépasse pas celui des 24 cœurs AVX-512 (≈ 1,8 contre ≈ 2,1 TFLOPS en crête, d'après les fiches : à mesurer) ; les entiers 64 et 128 bits y sont émulés ; l'efficacité SIMT rejouée sur les vraies feuilles est de 13 % ; le reste CPU de la voie « feuilles seules » est de 89 ms à $K = 5$. Seule une voie complète (arbre, feuilles, assemblage et tour sur l'appareil) changerait l'échelle : c'est un second moteur.
6. **$K = 10$ en 100 ms : non.** Le budget est de 2 600 cycles par boule tout compris ; le plancher estimé de cette famille d'algorithmes est d'environ 3 000 (§ 1). Temps honnêtes : 0,6 à 0,7 s pour le premier moteur propre, 0,28 à 0,39 s comme cible CPU après prototypes, 0,16 à 0,23 s pour la seule hiérarchie d'ordre 10. À $K = 5$, 100 ms est atteignable en CPU seul (cible estimée 65 à 91 ms), sans marge.

### 0.1 Classement

| Id | Piste | Classement | Fondement |
| --- | --- | --- | --- |
| R1.1 | Feuille « plate » : pas de test de droite ; q3 par aigu puis enveloppe du triangle médian ; quadruplets filtrés par lots | à prototyper (avec la conception du générateur) | mesuré ici (comptes), estimé (coûts) |
| R1.2 | Marche sur les arêtes et sommets de Voronoï d'ordre ≤ K dans une feuille | à écarter | mesuré par le prototype de la v10 (lu) ; estimé depuis l'entonnoir L05 |
| R1.3 | Engendrer q3 et q4 depuis les paires q2 voisines | à écarter | mesuré ici (hérédité absente pour 9 à 12 % des q3, 17 à 18 % des q4) |
| R1.4 | Balayage de la droite des centres (un tri par triplet donne tous les recensements) | à écarter | estimé depuis l'entonnoir L05 |
| R1.5 | Recalibrer la taille de feuille | à écarter | mesuré (L05 § 6.10) |
| R2.1 | Semis par jointure à clé additive vérifiée ; cellules analytiques ; ni atlas ni copie des populations | à faire (déjà retenu par `CONCEPTION_TOUR`, D-A1 et D-G2) | mesuré (étages v10), estimé (gain) |
| R2.2 | Catalogue stocké dans l'ordre d'émission (spatial), ordre canonique en permutation ; jointure locale par blocs | à prototyper | mesuré ici (localité 93 à 99 %), estimé (gain) |
| R2.3 | Successeur ou plus petite boule du morceau fournis par le générateur | à écarter | mesuré ici |
| R2.4 | Moins de pas de descente par un autre ordre de traitement | à écarter comme rupture (8 % à prendre, déjà pris par D-G1 de `CONCEPTION_TOUR`) | mesuré ici |
| R2.5 | Se passer de l'index des sites (feuilles du générateur comme index) | à écarter ; index implicite sur l'ordre de Morton à la place | mesuré ici (20 à 23 % des plus petites boules sont hors catalogue) ; non-totalité de l'arbre ajusté selon `CONCEPTION_TOUR` (D-I1) |
| R2.6 | Forêt construite pendant l'émission, par blocs, sans tri global | à écarter du premier moteur | mesuré ici (55 à 69 % de fusions locales seulement) |
| R3.1 | Chemin de produit « un seul ordre » : admission par plage d'ordres, règle exacte | à faire, après FULL | prouvé ici, mesuré ici |
| R3.2 | Sous-catalogue suffisant pour la hiérarchie de points sans la forêt d'ordre $K$ | à écarter tant qu'aucun théorème n'existe | conjecturé |
| R4 | GPU | hors du premier moteur ; réouverture sur trois mesures | mesuré (reçus), estimé |
| R5 | $K = 10$ en 100 ms | non atteignable ; annoncer le temps mesuré | estimé |

---

## 1. Le budget, exprimé par boule

Le contrat est un mur de 100 ms sur G4 à 48 fils. Trame 02 (45 845 sites) : 71 ns de mur par boule à $K = 5$ (1 407 885 boules), 18 ns à $K = 10$ (5 483 320 boules).

**Unité commune.** Les profils de L05 sont en cycles d'horloge du codespace (TSC à 2,445 GHz, un fil). L'étage des boîtes relie les deux machines sans modèle : 10 219 cycles par boule locaux (mesuré, L05 § 6.2) correspondent à 103,3 ms de mur sur G4 à 48 fils (mesuré, reçu de session 4) à $K = 5$, et 11 532 cycles à 441,6 ms à $K = 10$. Soit 0,34 ns de temps de fil G4 par cycle local, parallélisme de l'étage des boîtes compris. Avec ce taux :

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| budget de 100 ms, en cycles locaux par boule, tout le moteur | ≈ 9 900 | ≈ 2 600 |
| v10 mesurée : catalogue (171,1 et 627,9 ms) | ≈ 16 900 | ≈ 16 400 |
| v10 mesurée : tour FULL (89,3 et 406,2 ms) | ≈ 8 800 | ≈ 10 600 |
| v10 : facteur à gagner | 2,6 | 10,3 |
| leviers fondés de l'audit : catalogue 84 à 93 ms (L05 § 6.9), tour 42 à 62 ms (L06 § 9.2, trame 00) ; 300 à 345 et 315 à 370 ms | ≈ 12 500 à 15 300 | ≈ 16 100 à 18 700 |
| leviers fondés : facteur restant | 1,3 à 1,55 | 6,2 à 7,2 |

Les deux premières lignes sont des conversions exactes de mesures ; la ligne « leviers fondés » reprend les estimations des auditeurs.

**Plancher estimé de la famille d'algorithmes** (boîtes de centres, puis cellules et descentes). Coûts unitaires supposés au mieux d'un code vectorisé ; comptes mesurés (L05 § 6.1, L06 constat 05) :

| Poste, par boule | Compte mesuré ($K = 5$ ; $K = 10$) | Coût unitaire plancher supposé | Cycles par boule |
| --- | --- | ---: | ---: |
| filtre des nœuds | 287 ; 235 tests | 2 | 570 ; 470 |
| candidats de feuille | 48 ; 58 | 15 | 720 ; 870 |
| recensements | 2,35 ; 1,97 | 100 | 235 ; 200 |
| émission, niveau, clé de tri, rang | 1 | 400 | 400 |
| semis : sondes | 2,8 ; 3,2 représentants | 40 | 110 ; 130 |
| descentes : plus petites boules | 0,83 ; 1,40 pas | 500 | 415 ; 700 |
| forêt, verticales, matérialisation | 2,8 ; 3,2 représentants | 60 | 170 ; 190 |
| **total** | | | **≈ 2 600 ; ≈ 3 000** |

À $K = 5$ le plancher est près de quatre fois sous le budget : 100 ms est une affaire de constantes. À $K = 10$ le plancher (≈ 3 000) dépasse le budget (≈ 2 600) avant toute perte de parallélisme, même avec chaque poste à son meilleur coût supposé. C'est le fait qui commande tout ce qui suit. Ces coûts unitaires sont des hypothèses, pas des mesures : la table dit seulement qu'aucun réglage ne suffit à $K = 10$.

---

## 2. Piste 1 — moins de candidats par boule

### 2.1 D'où viennent les 48 à 58 candidats (mesuré : L05 § 6.1, 6.3, 6.10 et `feuilles_histogrammes_et_duplication.txt`)

Trame 02, $K = 5$ : 323 224 feuilles de 13,95 sites en moyenne, 4,36 boules par feuille. Par boule émise : 21,1 tests de dominance de paires, 20,3 triplets, 6,7 quadruplets (somme 48,1), plus 13,4 paires examinées, 2,35 recensements et 287 tests du filtre des nœuds. À $K = 10$ : 19,9 + 24,3 + 13,7 = 57,9.

| Part | Compte ($K = 5$) | Nature | Réductible ? |
| --- | --- | --- | --- |
| dominance de paires | 29,7 M, soit $m(m-1)/2$ par feuille | inhérente aux masques : c'est la matrice de dominance de la feuille | le compte non ; le coût oui (noyau sans branchement vectoriel) |
| triplets | 28,6 M incidences ; 7,53 M triplets distincts ; au plus 1,32 M utiles (supports des 746 547 boules q3, faces des 143 105 boules q4) | duplication ×3,8 entre feuilles (la droite des centres traverse plusieurs boîtes) ; ×5,7 triplets distincts par triplet utile (les masques ne donnent qu'un minorant de l'intérieur) | non par la taille de feuille (§ 2.6) ; le coût par incidence oui (§ 2.2) |
| quadruplets | 9,38 M incidences ; 6,90 M distincts ; 143 105 boules q4, soit 48 quadruplets distincts par boule q4 | les quatre triplets vivants ne bornent pas l'intérieur de la sphère des quatre points | le compte non ; le coût oui (intérieur du tétraèdre d'abord : ×2,4 à ×2,7 exact, ×8 à ×11 filtré, L05 § 6.7) |

- **Part de la taille de feuille : nulle.** L05 § 6.10 : les candidats par boule ont un plancher (48 à $K = 5$, 56 à $K = 10$) ; de $M = 12$ à $M = 24$ ils restent entre 48 et 61 ; une feuille plus petite reteste les mêmes triplets dans plus de feuilles. Le réglage du code est déjà au minimum du modèle de coût, avec les coûts actuels comme avec ceux des leviers fondés.
- **Part de l'ordre des tests : elle porte sur le coût, pas sur le compte.** Quadruplets : le test le plus sélectif (7,4 % de oui) est fait en dernier. Triplets : le test de droite laisse passer 71 % des candidats, alors que l'aiguïté en laisse passer 39 % et que l'enveloppe du triangle médian n'en laisse que 16 à 19 % atteindre le calcul du centre (mesuré ici, § 2.2).
- **La méthode est déjà sensible à la sortie** : de 8 000 à 32 000 points, les candidats par boule ne bougent pas et les tests du filtre par boule ne montent que de 5,6 % (L05 § 6.1). Ce qui est en cause est la constante, pas la pente.
- **Conclusion.** Le rendement de 2,1 % (L05) n'est pas un accident : une liste K-certifiée de 14 à 22 sites, bornée par des masques de dominance uniforme, admet structurellement une cinquantaine de candidats par boule. On ne descend pas sous ce nombre sans changer de certificat local, et les trois changements examinés ci-dessous ne tiennent pas.

### 2.2 R1.1 — Feuille « plate » : supprimer le test de droite (à prototyper)

- **Idée.** Le test de la droite des centres (lemme Z) sert deux usages : pré-filtrer les triplets jugés en q3, et marquer les triplets « vivants » dont on forme les quadruplets. Le premier usage est mieux servi par : aigu (trois produits scalaires), puis enveloppe du triangle médian contre la boîte (neuf additions), puis centre et test de boîte. Le second peut disparaître si un quadruplet coûte 30 cycles au lieu de 271.
- **Exactitude.** Retirer un filtre qui n'est qu'une condition nécessaire ne change pas l'ensemble émis (théorème G de L01 : ni l'ordre des tests ni les pré-filtres n'interviennent). Enveloppe du triangle médian : le centre d'un triangle strictement aigu est l'orthocentre de son triangle médian, strictement intérieur à celui-ci ; condition nécessaire, à inscrire avec sa fixture d'égalité (centre sur une face de boîte). Rien d'autre à prouver ; le filtre des quadruplets relève de la règle F6 de l'architecture.
- **Mesuré ici** (sonde sur une copie du générateur v10, quart `lidar02_quarter_x_nonneg_y_nonneg`, 5 286 sites, compteurs déterministes) :

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| triplets de poids admissible, non alignés | 3 015 242 | 12 877 900 |
| … dont la droite touche la boîte (v10) | 2 128 146 (70,6 %) | 9 007 876 (69,9 %) |
| … aigus | 1 172 146 (38,9 %) | 4 964 883 (38,6 %) |
| … aigus et droite touchée (centres calculés par la v10) | 949 884 (31,5 %) | 3 922 893 (30,5 %) |
| … aigus et enveloppe médiane touchée (centres à calculer) | 566 810 (18,8 %) | 2 080 607 (16,2 %) |
| quadruplets testés, avec test de droite | 1 026 109 | 7 548 516 |
| quadruplets testés, sans test de droite | 2 473 605 (×2,41) | 18 242 729 (×2,42) |
| dump du catalogue | identique (sha256 `4939fead…`) | identique (`be5d1964…`) |

  Second point (quart de la trame 01, 8 074 sites, $K = 5$) : quadruplets ×2,39, dump identique (`414aa4d4…`, l'épingle des reçus de la v10), centres à calculer pour 16,1 % des triplets au lieu de 29,3 %. La sonde de la conception du générateur (`preuves_generateur/proto/sonde_lidar02_k5.log`) obtient sur la trame 02 entière 22,2 M quadruplets au lieu de 9,38 M (×2,37) : même rapport.
- **Gain estimé.** Avec les coûts mesurés par L05 § 6.7 (séquence du triplet filtrée par blocs : 81 à 134 cycles ; quadruplet filtré par lots : 28 à 39 cycles) et un triplet sans test de droite supposé à 25 cycles : triplets et quadruplets passent de ≈ 2 330 cycles par boule (modèle « leviers fondés » de L05 § 6.10) à 1 000 à 1 500 à $K = 5$, et de ≈ 2 770 à 1 500 à 2 000 à $K = 10$ ; soit 15 à 25 % de l'étage des boîtes. **Sans** filtre flottant, la piste est neutre ou négative : au coût actuel du quadruplet (271 cycles), l'étage local passe de 0,99 à 1,11 s à $K = 5$ et de 3,9 à 6,2 s à $K = 10$ (temps locaux indicatifs, même binaire, même passe).
- **Risque.** Dépend de la règle F6 (borne $M$, exposition $E$ de l'expression de Gram). À $K = 10$, 2,4 fois plus de quadruplets : la capacité des lots et la compaction doivent suivre.
- **Plus petite expérience.** Microbanc sur les candidats réels vidés (méthode de L05 § 6.7), trois variantes sur G4 en AVX-512 : droite exacte en i64 puis quadruplets ; plate avec quadruplets exacts réordonnés ; plate avec quadruplets filtrés. Critère : la troisième sous 0,6 fois la première sur triplets + quadruplets, zéro désaccord, repli exact non vide.

### 2.3 R1.2 — Suivre les arêtes et sommets de Voronoï d'ordre ≤ K dans une feuille (à écarter)

- **Idée.** Les boules q4 sont des sommets, les q3 des points d'arêtes des diagrammes de Voronoï d'ordres ≤ K ; marcher sur ce squelette, restreint à la boîte, visiterait les seules sphères d'intérieur faible.
- **Comptes.** Le prototype de la v10 (`PROTOTYPES_SESSION_20260928.md` § 2, marche par pinceaux) a mesuré la taille du niveau : 108 à 124 tétraèdres par site pour un intérieur ≤ 3, 206 par site pour un intérieur ≤ 4 sur un extrait LiDAR, contre 31 boules critiques par site à $K = 5$ : 3,5 à 6,6 objets visités par boule émise. Recoupement par l'entonnoir de L05 (estimé ici) : 30 % des quadruplets jugés sont émis à $K = 5$ (143 105 sur 478 586) ; appliqué aux 4,96 M quadruplets de centre dans la boîte, cela donne ≈ 1,5 M sommets d'intérieur ≤ 2 et le double d'arêtes, soit ≈ 3 objets par boule à $K = 5$ et ≈ 9,5 à $K = 10$ (17 M sommets).
- **Coût.** Chaque pas de marche cherche le prochain site sur l'arête : un parcours de la liste (14 à 22 sites) et une comparaison exacte de deux rationnels, de l'ordre de 150 cycles au mieux. Total estimé : ≈ 500 cycles par boule à $K = 5$ et ≈ 1 400 à $K = 10$, contre 1 000 à 1 500 et 1 500 à 2 000 pour la feuille plate du § 2.2 : un facteur 2 à 3 à $K = 5$, 1,1 à 1,4 à $K = 10$, sur un poste qui pèse entre le quart et la moitié de l'étage.
- **Exactitude.** C'est ici que la piste tombe. La marche suppose la position générale (un sommet dégénéré, cinq sites cosphériques, n'a pas de « prochaine arête » définie), et la v10 l'a rejetée pour cela (`GEN_v1.md` § 3). Restreinte à une feuille, elle exige de plus un théorème de complétude nouveau : le squelette coupé par la boîte n'est pas connexe, il faut donc énumérer toutes les arêtes qui entrent par les six faces (un problème plan par face) et les composantes intérieures. L'énumération par masques n'a besoin d'aucune adjacence et c'est ce qui la rend exacte sans position générale.
- **Invariant d'architecture.** Une marche locale et transitoire ne matérialise pas la mosaïque ; l'objection n'est pas là.
- **Verdict.** Gain au mieux de 10 à 25 % sur l'étage des boîtes à $K = 5$, de 2 à 15 % à $K = 10$ où il manque le plus, contre un théorème de complétude à écrire et une robustesse aux dégénérescences à reconstruire : écartée. Rien à mesurer.

### 2.4 R1.3 — Engendrer q3 et q4 depuis les sphères q2 voisines (à écarter)

- **Idée.** Ne former un triplet que si ses trois arêtes sont des paires q2 admises, un quadruplet que si ses faces sont des q3 admises.
- **Pourquoi c'est faux.** La boule diamétrale d'une arête d'un triangle aigu déborde de la boule circonscrite, du côté opposé au troisième sommet : elle peut contenir autant de sites qu'on veut alors que la boule du triangle est vide. Le contre-exemple de L02 § 4.11 (la paire $AC$ a deux intrus, le triangle $ACw$ est de Gabriel) est exactement ce cas.
- **Mesuré ici** (`heredite.txt`, même quart) : 12,2 % des boules q3 du catalogue ($K = 5$ ; 11,8 % à $K = 10$) ont au moins une arête qui n'est **pas** une boule q2 du même catalogue ; 17,8 % des q4 (17,5 %) ; et seules 44,7 % des q4 (46,2 %) ont leurs quatre faces au catalogue q3. Second quart (trame 01, $K = 5$) : 9,0 %, 17,1 % et 45,0 %.
- **Ce qui est vrai à la place.** L'hérédité exacte est celle du lemme S (les parties du support canonique ont une union de dominateurs de poids borné) : c'est la notion de paire et de triplet « vivants » de la feuille actuelle. L'idée est donc déjà dans le générateur sous sa seule forme juste.

### 2.5 R1.4 — Balayer la droite des centres (à écarter)

- **Idée.** Sur la droite équidistante d'un triplet, le nombre de sites intérieurs change d'une unité à chaque quatrième site rencontré : un tri des paramètres $t_d$ des autres sites de la liste donne d'un coup le recensement de la boule q3 ($t = 0$) et de toutes les sphères q4 passant par le triplet.
- **Exactitude.** $t_d$ est le quotient de la puissance de $d$ par rapport à la sphère diamétrale du cercle par deux fois l'orientation de $d$ ; tout est rationnel exact. Rien d'incorrect.
- **Compte** (estimé depuis l'entonnoir de L05). Il faut évaluer tous les autres sites pour chaque couple (droite, feuille) qui se rencontrent : 20,4 M × 11 = 224 M évaluations à $K = 5$, 94,2 M × 18,6 = 1,75 G à $K = 10$, puis un tri par droite. À 3 cycles par site en double vectorisé (borne basse), 480 et 960 cycles par boule, pour remplacer les quadruplets filtrés et les recensements (≈ 500 à 900 cycles par boule après leviers) ; et chaque sphère q4 est trouvée par ses quatre triplets.
- **Verdict.** Au mieux neutre ; écartée.

### 2.6 R1.5 — Taille de feuille et bande de rangs (à écarter)

Recalibrer $M$ n'apporte rien (L05 § 6.10, repris au § 2.1). Une liste plus courte par un certificat ponctuel (les seuls sites de rang ≤ K en un point de la boîte) demanderait des boîtes bien plus petites : à $M = K + 3$ l'arbre explose (×47 nœuds à $K = 5$, L05). La dominance uniforme est clairsemée à la taille de feuille optimale : 27 % des paires de la feuille ont une relation de dominance à $K = 5$, 39 % à $K = 10$ (L05 § 6.3 : 21,8 M bissectrices sur 29,7 M paires coupent la boîte). C'est cette rareté qui borne tout filtre fondé sur les masques, y compris celui du § 4.

---

## 3. Piste 2 — moins de travail dans la tour

### 3.1 Ce que deviennent les morceaux de jonction (mesuré ici)

Expérience `pieces_stats` (annexe A), recoupée par les compteurs de la tour v10 sur la même entrée (`tour_v10_quart_k5.json`, `tour_v10_quart_k10.json`) : sur le dump exact du catalogue v10 d'un quart de trame (5 286 sites ; 147 666 boules à $K = 5$, 574 476 à $K = 10$), chaque morceau $P_b \setminus \lbrace u \rbrace$ de chaque jonction régulière est confronté aux populations des naissances (égalité d'ensembles exacte), puis, s'il n'est pas un semis, descendu par la règle de la v10 (plus petite boule, saut aux $k$ plus proches si $p \geq k$, premier représentant si la boule est inerte) avec arrêt à la première cellule du catalogue. La géométrie y est en binaire64 à tolérance : c'est une statistique, pas un juge ; le nombre de représentants coïncide avec celui de la v10 à 18 et 40 unités près (cellules des coquilles étendues, ignorées ici). La forêt reconstruite de la même façon (`foret_blocs`) redonne les nombres de nœuds du reçu G4 à 16 unités près sur 179 340 ($K = 5$) et 5 sur 789 293 ($K = 10$).

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| représentants (compteurs de `mhgp10_tower` v10 sur le même quart) | 402 883 | 1 776 978 |
| … morceaux d'ordre 1 (des sites) | 24 298 | 24 298 |
| … semis directs (population d'une naissance régulière) | 290 193 (72,0 %) | 1 241 845 (69,9 %) |
| … hors semis | 88 392 (21,9 %) | 510 835 (28,7 %) |
| chaînes qui **finissent** sur un semis (compteur `seed_hits` de la v10) | 348 675 (86,5 %) | 1 561 571 (87,9 %) |
| premier pas hors semis : cellule de jonction du catalogue, même ordre | 67,5 % | 61,4 % |
| premier pas : saut ($p \geq k$) | 23,9 % | 23,4 % |
| premier pas : boule inerte | 8,5 % | 15,1 % |
| plus petites boules calculées par la v10 (compteur `meb`) | 110 450 | 701 328 |
| plus petites boules d'une descente idéale (arrêt à la première cellule du catalogue, semis testé avant chaque boule) | 102 499 | ≈ 647 000 (échantillon 1 sur 4) |
| rapport v10 / idéal | 1,08 | 1,08 |
| sphères hors catalogue ($p + q > K + 1$), en part des plus petites boules : idéal ; v10 hors juge 1/32 | 23,0 % ; 23,0 % | 19,7 % ; 19,9 % |
| chaîne la plus longue | 7 | 12 |

Second point (quart de la trame 01, 8 074 sites, $K = 5$, `stats_quart01_k5.txt`) : 564 832 représentants ; semis directs 73,0 % ; hors semis 20,0 % ; chaînes finissant sur un semis 86,0 % ; 137 360 plus petites boules en v10 contre 128 136 dans la descente idéale (rapport 1,07) ; premier pas : 73,5 % sur une jonction du catalogue, 19,6 % de sauts, 6,9 % de boules inertes ; sphères hors catalogue 19,8 % et 20,0 %.

Trois lectures.

- **La descente de la v10 est à 7 à 8 % du nombre minimal de pas** pour sa règle. L'écart vient des cellules traversées avant d'être mémorisées ; la résolution « arrêt à la première cellule de fenêtre, pointeurs suivis après coup » de `CONCEPTION_TOUR` (D-G1) le reprend. Aucun autre ordre de traitement ne fera mieux : le gain de l'étage G est dans le coût du pas, pas dans leur nombre (R2.4).
- **Le « 86 % d'arrêts sur un semis » de L06 compte les chaînes qui finissent sur un semis** (le compteur est incrémenté à chaque tour de la boucle de `resolve`, `tower.cpp:820-832`, lu ; retrouvé ici : 86,5 % et 87,9 %). La part des représentants qui **sont** un semis est plus basse : 72 % et 70 %, et elle baisse avec l'ordre (86,0 % des morceaux à $k = 2$, 73,2 % à $k = 5$, 67,9 % à $k = 10$). Un représentant sur cinq à $K = 5$, plus d'un sur quatre à $K = 10$, demande donc au moins une plus petite boule : c'est plus que les « 14 % » que suggère le compteur.
- **Deux tiers des morceaux hors semis tombent, en un pas, sur une jonction du catalogue de même ordre** : $P_b \setminus \lbrace u \rbrace$ est alors la population d'une boule plus basse privée d'un site intérieur. Le reste (sauts et boules inertes, un tiers) est ce qui allonge les chaînes et ce qui interroge l'index ; après un saut, la nouvelle partie est souvent un semis (20 737 chaînes sur 88 374 à $K = 5$).

### 3.2 R2.1 — Semis par jointure, cellules analytiques (à faire)

- **Idée.** Ne plus construire d'atlas ni de table de populations copiées. Une boule régulière de somme $c = p + q$ est jonction à l'ordre $c - 1$ et naissance à l'ordre $c$ : la cellule est lue dans $(p, q, m)$. La clé d'un morceau se calcule en $O(1)$ : $H(P_b) - h(u)$, où $H$ est la somme modulo $2^{64}$ d'un mélange $h$ des indices de sites. Les morceaux se résolvent par jointure contre les clés $H(P_{b'})$ des naissances de l'ordre.
- **Exactitude.** La clé ne décide rien : un morceau n'est déclaré semis qu'après comparaison exacte des deux listes d'identifiants. Énoncé : $F$ est un semis si et seulement s'il existe une boule régulière $b'$ avec $P_{b'} = F$ ; alors $B(F) = b'$ (lemme 1 de L02) et la descente s'arrête. Rien de nouveau à prouver.
- **Gain estimé.** Étages v10 mesurés sur G4 (trame 02, reçu de session 4) : index des supports, atlas et semis 11,5 ms à $K = 5$ et 61,4 ms à $K = 10$ ; plus la part « hachage et semis » des descentes (15 % et 9,6 % de 27 et 203 ms, L06). `CONCEPTION_TOUR` retient déjà ces deux décisions (D-A1, D-G2) et les chiffre ; je n'ajoute que la mesure du § 3.1 et la localité du § 3.3.
- **Risque.** Faible. La jointure doit rendre le même résultat quel que soit le nombre de fils : les écritures se font par ordinal de morceau.

### 3.3 R2.2 — Catalogue dans l'ordre d'émission, ordre canonique en permutation (à prototyper)

- **Idée.** La v10 permute physiquement le catalogue dans l'ordre (niveau exact, $S^{*}$) : 4,0 ms à $K = 5$ et 16,3 ms à $K = 10$ pour la seule copie (reçu de session 4), et surtout une tour qui, pour chaque morceau, va lire une boule à un rang quelconque d'un tableau de 43 à 243 Mo. Laisser les enregistrements et les identifiants dans l'ordre d'émission des feuilles (ordre spatial, déterministe : parcours en profondeur de l'arbre de boîtes, indépendant du nombre de fils), et publier l'ordre canonique comme un tri d'index (clé approchée, rang, position).
- **Mesuré ici** (`foret_blocs`, blocs de sites consécutifs en ordre de Morton) : la naissance d'un morceau-semis est dans le même bloc que sa jonction pour 98,8 % des liens avec des blocs de 880 sites, 93,3 % avec des blocs de 96 sites, 78,7 % avec des blocs de 10 sites ($K = 5$ ; 98,7 %, 92,6 % et 77,5 % à $K = 10$ ; 99,0 %, 94,1 % et 80,3 % sur le second quart). Une jointure par blocs d'une centaine de sites (quelques milliers de boules, résidents en cache L2) résout donc plus de neuf liens sur dix sans sortir du bloc ; le reste passe par une table globale.
- **Exactitude.** L'ordre de stockage n'entre dans aucune décision : rangs exacts, plateaux et numérotation canonique des nœuds ne dépendent que de (rang, $S^{*}$). Obligation : la sortie canonique (flux dans l'ordre de la permutation) est identique octet pour octet à celle d'un stockage canonique ; porte par différentiel contre la v10.
- **Gain estimé.** Copie de l'assemblage évitée (4 et 16 ms mesurés en v10) ; sondes de semis et consultations de supports en cache (supposé : 60 cycles gagnés par accès, soit ≈ 2 ms à $K = 5$ et ≈ 11 ms à $K = 10$ pour les 4,8 M et 25 M sondes et consultations de la trame 00, L06) : ≈ 5 à 8 ms à $K = 5$, ≈ 25 à 35 ms à $K = 10$. Non mesuré.
- **Risque.** Un second domaine d'identifiants (position d'émission) à côté du rang canonique que fixe `ARCHITECTURE.md` § 7.3 ; `CONCEPTION_TOUR` § 1.1 lit aujourd'hui un enregistrement en ordre canonique. La décision se prend sur la mesure M3, pas avant.
- **Plus petite expérience.** Sur les dumps de la v10 ($K = 5$ et 10, trois trames) : temps de la jointure des semis et des consultations de supports, stockage canonique contre stockage spatial avec tables par blocs, à 48 fils sur G4.

### 3.4 R2.3 — Le générateur fournit le successeur de chaque morceau, ou sa plus petite boule (à écarter)

- **Idée.** La feuille tient en cache la boule $b$, sa population et une liste K-certifiée : qu'elle calcule la plus petite boule $b_u$ de chaque morceau et l'identifie.
- **Ce qui est certain.** Le morceau est dans la liste, donc $b_u$ se calcule dans la feuille. Mais son **recensement** n'est certifié que si la liste l'est en son centre $c_u$, et $c_u \neq c$ : $\lVert c - c_u \rVert^{2} \leq r^{2} - r_u^{2}$, déplacement médian de 0,34 rayon à $K = 5$ et 0,21 à $K = 10$ (mesuré ici).
- **Mesuré ici.** Condition vérifiable dans la feuille sans rien supposer de plus que la certification : tous les sites de la boule fermée $b_u$ sont dans $N_K(c)$. Elle tient pour 12,7 % des premiers pas hors semis à $K = 5$ (14,2 % sur le second quart) et 34,6 % à $K = 10$ ; par ordre, à $K = 10$ : 91 % à $k = 2$, 56 % à $k = 7$, 35 % à $k = 8$, **0 % à $k = 9$ et $k = 10$**. Raison de structure : une jonction de somme $p + q \geq K$ a au moins $K$ sites dans sa boule fermée, donc $d_K(c) = r$ et $N_K(c)$ se réduit à $P_b$ ; l'intrus qui empêche le morceau d'être un semis est hors de $P_b$. Or les deux ordres les plus hauts portent 74 % des morceaux hors semis à $K = 5$ et 43 % à $K = 10$.
- **Coût.** La feuille ne sait pas quels morceaux sont des semis (il faut le catalogue entier) : elle calculerait une plus petite boule par morceau, soit 3,5 à 4,6 fois plus que la tour n'en calcule en premier pas (402 883 contre 88 392 ; 1 776 978 contre 510 835).
- **Verdict.** Écartée. L'interface « laissée ouverte » au § 1.1 de `CONCEPTION_TOUR` peut être fermée : le catalogue ne publie ni successeur ni plus petite boule de morceau. Le cas $c_u$ dans la boîte de la feuille n'a pas été mesuré (il faut les boîtes) ; il ne lèverait pas l'objection de coût.

### 3.5 R2.4 — Liberté de la descente : ce qu'elle permet, ce qu'elle ne donne pas

Énoncé utile (prouvé ici en deux lignes, à contre-lire ; il étend le théorème D de L02 dans les limites fixées par l'auditeur) : *si $B(F) = b$ et si $F' \subseteq P_b$ est une $k$-partie dont la trace sur $U$ est séparable, alors $\beta(F') < \lambda_b$ et $F$, $F'$ sont dans la même composante de $\Gamma_k(\lambda_b)$*. Preuve : $\beta(F') < \lambda_b$ par le lemme 1 de L02 ; $F \neq F'$ sont deux $k$-parties de $P_b$, qui compte donc au moins $k + 1$ sites, et le lemme 2 les relie au niveau $\beta(P_b) = \lambda_b$. Sont donc valides : le saut vers $k$ sites intérieurs quelconques ; $I \cup A$ ; mais aussi l'échange d'un site du support contre un intrus. Seule la **classe** du terminal aux coupes $a \geq \beta(F)$ est unique, et un raccourci par la cellule $(b, k)$ ne vaut qu'à partir de $\lambda_b$ (réponse de l'auditeur à Q1).

- Conséquence 1 : pour une sphère du catalogue avec $p \geq k$, les $k$ premiers identifiants de $I$ suffisent, sans tri par distance ni index. Mais L06 a mesuré cette variante (mutant équivalent `JUMP_ANY`) : 3 % de plus petites boules et 7 % de sauts **en plus**. Pas de gain ; garder le saut aux $k$ plus proches.
- Conséquence 2 : pour une sphère hors catalogue, la requête utile à l'index est « $q$ intrus strictement intérieurs, hors de $F$ », à sortie anticipée, et non la boule fermée entière (L06 : jusqu'à la moitié du nuage). `CONCEPTION_TOUR` retient une requête bornée aux $k$ plus proches (D-G4), qui a le même effet.
- Ce que la liberté ne donne pas : un ordre de grandeur sur le nombre de pas (§ 3.1 : 8 % au plus).

### 3.6 R2.5 — Se passer de l'index des sites (à écarter)

Les sphères hors catalogue font 20 à 23 % des plus petites boules (mesuré ici, § 3.1 ; L06 : 24 à 26 % en comptant le juge 1/32), soit de l'ordre de 0,26 M requêtes par trame à $K = 5$ et 1,6 M à $K = 10$ (L06, trame 00, juge déduit). Leur recensement n'est pas dans le catalogue ; un catalogue plus profond d'un ou deux ordres coûterait 1,48 à 1,90 fois plus de boules (L02 § 7.4) et ne changerait rien : ces sphères resteraient hors fenêtre à l'ordre courant. Les feuilles du générateur comme index (lemme O de `GEN_v2`) coûteraient 18 à 42 Mo de listes, 23 à 34 Mo d'arbre et une localisation exacte de centre rationnel par requête ; `CONCEPTION_TOUR` (D-I1) établit en outre que l'arbre ajusté ne couvre pas les centres dont la boule a au moins $K$ sites intérieurs. **Garder un index, sans construction** : hiérarchie implicite sur l'ordre de Morton (D-I1 de `CONCEPTION_TOUR`), qui supprime les 3,4 à 4,6 ms séquentielles de l'arbre k-d de la v10 (différence des `prepare_s` des deux sondes, reçu de session 4 lu par L04 C10).

### 3.7 R2.6 — Construire la forêt pendant l'émission, sans tri global (à écarter du premier moteur)

- **Pourquoi pas tel quel.** (a) La numérotation publiée est l'ordre total (niveau exact, $S^{*}$) : un tri global des clés reste nécessaire, mais c'est un tri d'index de 12 à 16 octets par boule, pas une permutation de la charge utile. (b) Une jonction ne se traite qu'après toutes les jonctions de niveau inférieur de sa composante, où qu'elles soient émises. (c) L'émission est spatiale et les niveaux d'une région se recouvrent.
- **Variante par blocs, mesurée ici.** Découper les minima en blocs de Morton, faire un Kruskal local par bloc sur les jonctions internes, puis coudre. Est locale une fusion dont tout le sous-arbre tient dans un bloc.

| Part des fusions locales à un bloc | blocs de 880 sites (≈ 52 blocs par trame) | 96 sites | 10 sites |
| --- | ---: | ---: | ---: |
| $K = 5$, tous ordres (ordre 1 ; ordre 5) | 68,8 % (89,7 % ; 63,8 %) | 32,2 % | 13,5 % |
| $K = 10$, tous ordres (ordre 10) | 60,2 % (55,4 %) | 20,6 % | 6,9 % |
| second quart, $K = 5$, tous ordres (ordre 5) | 66,6 % (60,9 %) | 37,8 % | 17,1 % |

  Avec une cinquantaine de blocs, 31 à 45 % des fusions restent pour la couture séquentielle : le gain est borné à ×2,2 à ×3,2 sur un noyau qui, sans lots, pèse déjà 8 à 12 ms à $K = 5$ (`CONCEPTION_TOUR`, D-F1). Une couture hiérarchique ferait mieux, au prix d'un algorithme nouveau et d'une preuve (forêt couvrante minimale des forêts locales, plateaux compris).
- **Verdict.** Pour le premier moteur : noyau sans lots par ordre, recouvert par les descentes (L06, `CONCEPTION_TOUR` D-F1 et D-F2). La forêt par blocs est une piste pour l'échelle (10^6 sites et plus), pas pour la trame.

---

## 4. Piste 3 — le produit a-t-il besoin de tout ?

### 4.1 Ce qu'un seul ordre demande (prouvé ici, à contre-lire ; mesuré ici)

**Définition.** Une boule est *utile à l'ordre $K$* si sa fenêtre contient $K$ : $p + q_{\min} - 1 \leq K \leq p + m$. Pour une coquille régulière : $p + q \in \lbrace K, K + 1 \rbrace$ (naissances et jonctions de l'ordre $K$).

**Lemme R3 (le catalogue restreint suffit à la forêt d'ordre $K$).** Dans une descente à l'ordre $K$, toute sphère rencontrée est la plus petite boule d'une $K$-partie, donc $p + m \geq K$ ; si elle est au catalogue ($p + q_{\min} \leq K + 1$), sa fenêtre contient $K$. La tour d'ordre $K$ ne consulte donc que des boules utiles ; les sphères hors catalogue passent par l'index, comme en FULL.

**Lemme R4 (filtre exact propre à un ordre).** Soient $L$ la liste K-certifiée d'une feuille $Q$, $T \subseteq L$ et $O(T)$ l'ensemble des sites de $L \setminus T$ dominés sur $Q$ par un site de $T$. Si $T$ est contenu dans la coquille d'une boule utile à l'ordre $K$ centrée dans $Q$, alors $\lvert O(T) \rvert \leq \lvert L \rvert - K$. *Preuve.* Un site dominé par un site de la coquille est strictement extérieur ; les $p + m \geq K$ sites de la boule fermée sont dans $L$ (théorème C) et hors de $O(T)$. Et $O$ croît avec $T$ parmi les parties d'une même coquille, donc le test s'applique dès la paire.

**Mesuré ici** (sonde `MHGP_X_ORDER`, même quart, compteurs déterministes) :

| | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| boules du catalogue ; boules utiles à l'ordre $K$ | 147 666 ; 87 342 (59,1 %) | 574 476 ; 209 092 (36,4 %) |
| même part sur la trame 02 entière (comptes par $(q, p)$ du reçu) | 59,1 % | 36,5 % |
| ensemble utile émis avec le filtre R4 | identique (87 342, même empreinte) | identique (209 092, même empreinte) |
| paires examinées | −1,9 % | −8,6 % |
| triplets testés | −0,8 % | −6,5 % |
| quadruplets testés | −0,2 % | −3,9 % |
| recensements | −1,1 % | −6,9 % |
| boules encore recensées et émises par la feuille | 143 591 (97 %) | 495 599 (86 %) |

Second quart (trame 01, $K = 5$) : 120 992 boules utiles sur 210 424 (57,5 %), ensemble identique avec le filtre ; paires −2,1 %, triplets −1,0 %, quadruplets −0,2 %, recensements −1,0 %.

Le filtre est exact et presque sans effet : à la taille de feuille optimale la dominance uniforme est trop clairsemée (§ 2.6) pour borner le nombre de sites intérieurs par le haut. **Un seul ordre ne réduit pas l'énumération.** Il réduit ce qui suit : l'émission (59 % et 36 % des boules), le tri et l'assemblage (proportionnels aux boules), et la tour.

### 4.2 Part de la tour (mesuré ici et L06)

| À l'ordre $K$ seul, en part du FULL | $K = 5$ | $K = 10$ |
| --- | ---: | ---: |
| représentants | 36 % (L06 : 37 %) | 21 % (L06 : 22 %) |
| pas de descente | 46 % | 24 % (L06 : 26 %) |
| noyau de forêt | celui de l'ordre maximal, déjà le plus long | idem |
| verticales | aucune | aucune |

### 4.3 Ce que cela donne en temps (estimé)

Catalogue : mêmes boîtes à 2 à 4 % près (recensements et émission), ordre et assemblage multipliés par 0,59 et 0,36. Tour : descentes multipliées par 0,46 et 0,25, un seul noyau, pas de verticales. Sur les estimations « leviers fondés » des auditeurs : 106 à 123 ms à $K = 5$ (au lieu de 126 à 155) et 365 à 433 ms à $K = 10$ (au lieu de 615 à 715). Sur la cible du § 7 : 47 à 69 ms et 160 à 230 ms. Référence mesurée : la chaîne `mhgp10_cluster` de la v10 (catalogue complet, un ordre, tête) prend 0,263 s à $K = 5$ sur G4, dont 0,061 s de tour à un ordre.

### 4.4 Décision et limites

- **R3.1, à faire, après FULL.** L'admission du catalogue et la demande à la tour prennent une plage d'ordres $[k_{\min}, k_{\max}]$ ; FULL est la plage $[1, K]$. C'est le chemin de la hiérarchie de points d'un ordre (`core`, `cover`). Le contrat de l'utilisateur reste la tour FULL ; ce chemin se chronomètre et se publie à part.
- **R3.2, écartée faute de théorème.** Une hiérarchie de points n'a que $n$ feuilles par ordre, mais je ne connais aucun énoncé qui la tire d'un sous-catalogue sans la forêt d'ordre $K$ : une composante sans site entré peut relier deux composantes qui en ont, et la première boule couvrante d'un site est une boule utile quelconque. L01 pose la question (§ 11, question 2) ; elle reste une conjecture, hors conception.

---

## 5. Piste 4 — GPU : décision

### 5.1 Ce qui est mesuré sur cette VM

| Fait | Valeur | Source |
| --- | --- | --- |
| appareil | RTX PRO 6000 Blackwell Server Edition, 188 SM, capacité 12.0, 97 887 Mio, horloge maximale 2 430 MHz, 600 W, pilote 580.173.02 | reçus `g4_session5_scale` (`vm_facts.txt`) et `g4_session7_cuda_probe` |
| outils | nvcc 12.9.41 hors du `PATH` ; CMake 3.22.1 (pas de dialecte CUDA20) ; g++ 11.4 | idem |
| processeur | EPYC 9B45, 24 cœurs, 48 fils, AVX-512 (F, DQ, IFMA, VBMI2, VPOPCNTDQ), 96 Mio de L3 | `vm_facts.txt` |
| emploi du GPU par la v10 | aucun : tous les temps G4 de la v10 sont en CPU seul | reçus des sessions 1 à 5 |
| entiers 128 bits sur l'appareil | 16 777 216 produits comparés à l'hôte, 0 écart ; **les débits de la sonde sont invalides** (boucles à débordement signé) | reçu de session 7 et `ERRATA.md` |
| transferts épinglés | 56,8 et 56,4 Go/s (sonde de session 7, transfert seul) ; 22,7 et 24,1 Go/s sur un vrai étage (v7) ; ≈ 4 Go/s en mémoire paginable (v9) | reçu ; `v10-perf/gpu/out/bilan_gpu.txt` |
| contexte CUDA | 121 à 166 ms par processus ; réservation du bassin épinglé 38 à 40 ms pour 256 Mo, 190 à 201 ms pour 1,28 Go | reçus v9 R20 et R22 cités par `faits_vm.txt` |
| histoire | v6 : 154 ms de noyaux dans un étage appareil de 7,7 s (le code hôte dominait) ; v9 : facteur 10 entre projection et mesure ; v9 avec GPU 0,76 à 0,98 s à $K = 5$ sur les trois trames (R22), v10 sans GPU 0,20 à 0,25 s (session 4) | `v10-perf/gpu/out/faits_vm.txt` ; `gpu_design/juge/PLAN.md` § 8 ; R22 d'après les notes de session (reçu non relu ici) ; reçu de session 4 |

### 5.2 Débits réalistes (estimés ; rien n'est mesuré proprement sur l'appareil)

| Opération | 24 cœurs AVX-512 (crête) | GPU (crête) | GPU réaliste | Fondement |
| --- | --- | --- | --- | --- |
| double précision (multiplication-addition) | ≈ 1,0 T/s (deux unités de 8 voies par cœur, horloge nominale de 2,7 GHz lue dans `lscpu`), soit ≈ 2,1 TFLOPS | ≈ 0,9 T/s, soit ≈ 1,8 TFLOPS, **si** le rapport FP64:FP32 est de 1:64 comme sur les cartes de cette puce | égal ou inférieur au CPU | fiches publiques, hors dépôt, non vérifiées : première mesure à faire (M6) |
| produit 64 × 64 → 64 | 0,5 à 1 T/s (8 voies par instruction) | 6 à 12 T/s (3 à 5 instructions 32 bits par produit, 58 T voies-instructions par seconde) | 0,8 à 1,6 T/s à 13 % d'efficacité SIMT | estimé |
| produit 64 × 64 → 128 | ≈ 0,07 T/s en scalaire ; quelques dixièmes avec IFMA 52 bits | 4 à 5 T/s (une douzaine d'instructions) | 0,5 à 0,7 T/s | estimé |
| efficacité SIMT sur les vraies feuilles, un warp par feuille | — | — | 13,2 à 13,6 % | modèle rejoué sur les feuilles de la trame 02 (`gpu-verif/out/bilan_corrige.txt`) |

Trois conséquences.

- **La doctrine numérique de la v11 est taillée pour le CPU.** Coordonnées locales à la feuille, noyaux entiers exacts en binaire64 (F2), filtres de signe (F6) : sur 24 cœurs AVX-512 ce sont 8 voies exactes par instruction. Sur cette carte, le double est (d'après les fiches) soixante-quatre fois plus lent que le simple, et le simple n'a que 24 bits de mantisse pour des produits de 30 bits : l'appareil doit tout faire en entiers émulés, soit 152 instructions pour un côté de recensement et 1 560 pour l'intérieur d'un tétraèdre (coûts SASS comptés par la conception GPU, `voie_complete` § 2.5).
- **Feuilles seules sur GPU : pas de gain contre un CPU corrigé.** Noyau estimé 17,5 à 52 ms à $K = 5$ et 106 à 317 ms à $K = 10$ (un warp par feuille), ou 2,3 à 6,9 et 14 à 43 ms si des noyaux par files tiennent 5 à 15 T instructions par seconde, ce qu'aucune mesure n'établit ; transferts 3,2 et 9,9 ms ; reconstruction hôte 3,5 à 7 et 14 à 27 ms ; reste CPU (arbre, ordre, assemblage) 89 et 250 ms au coût de la v10 ; la compaction sur l'appareil (ballot et sommes préfixes) tient dans les 1,5 à 3 ms de lancements et de synchronisations comptés par la voie complète (§ 11.2 de cette conception). Catalogue : 113 à 157 ms et 379 à 608 ms (`bilan_corrige.txt`), contre 84 à 93 et 300 à 345 ms pour le CPU avec les seuls leviers fondés.
- **Descentes sur GPU : rien à $K = 5$, peu à $K = 10$.** 1,1 M pas à $K = 5$, 7,7 M à $K = 10$ ; la proposition flottante de la plus petite boule y serait en simple précision, le certificat en entiers 128 bits et larges ; il faut le catalogue sur l'appareil (43 à 243 Mo, 2 à 10 ms de transfert s'il est produit sur CPU). Gain possible : quelques dizaines de millisecondes à $K = 10$, conjecturé.

### 5.3 Décision R4

**Le GPU reste hors du premier moteur.** Ni la feuille ni la descente n'y gagnent contre le CPU que les audits et les deux conceptions voisines décrivent ; la voie qui changerait l'échelle (arbre, feuilles et assemblage sur l'appareil, catalogue résident, puis tour sur l'appareil) est estimée entre 22 et 67 ms à $K = 5$ et entre 70 et 360 ms à $K = 10$ pour le seul catalogue (`PLAN_PERF.md` § 4, chiffres du vérificateur) : la moitié haute de ces fourchettes ne tient aucun contrat, et c'est une seconde base de code à garder égale octet pour octet à la première.

**Réouverture, dans cet ordre, sur trois mesures** (§ 8, M6 à M8) : (1) sonde de débit propre sur l'appareil (double, 64 et 128 bits, sans débordement signé) ; (2) moteur CPU v11 mesuré sur G4, avec la part des feuilles dans le mur à $K = 10$ ; (3) banc de rejeu des feuilles sur l'appareil, noyaux par files, critères de `PLAN_PERF.md` (zéro désaccord, noyau ≤ 55 ms à $K = 10$, voie appareil entière ≤ 110 ms). Si le GPU est rouvert, le premier morceau à porter est l'arbre de boîtes (dominance en entiers 64 bits sans produit large, 24 instructions par test, régulier), pas la feuille.

**Ce que le premier moteur doit garder pour ne pas fermer la porte** : chaque prédicat a une forme entière exacte de référence, indépendante des filtres flottants ; la feuille travaille par étages sur des lots à capacité bornée, en tableaux séparés ; masques de 32 bits sur le chemin rapide ; émission à positions fixées par ordinal. Ce sont aussi les conditions de la vectorisation AVX-512.

---

## 6. Piste 5 — $K = 10$ en 100 ms

**Non, par aucun chemin chiffré.**

- **Le compte.** 5,48 M boules et 7,47 M nœuds en 100 ms, c'est 18 ns de mur par boule, soit ≈ 2 600 cycles locaux par boule pour tout le moteur (§ 1). Le plancher estimé de la méthode est ≈ 3 000 ; la cible du § 7 correspond à 7 200 à 10 000.
- **CPU seul.** Premier moteur propre avec les leviers fondés de l'audit : 0,6 à 0,7 s (estimé par L05 et L06). Après les prototypes (feuille par lots vectorisée, filtres F6, descente à certificat combinatoire, forêt sans lots) : 0,28 à 0,39 s (estimé, § 7). Il faudrait encore un facteur 3 à 4.
- **Avec un catalogue sur l'appareil.** 70 à 360 ms de catalogue (estimation du vérificateur) plus une tour CPU de 125 à 180 ms (`CONCEPTION_TOUR`) : 0,2 à 0,5 s. Le contrat demanderait en plus la tour sur l'appareil : un second moteur, de la recherche.
- **Avec un seul ordre.** 0,16 à 0,23 s (estimé, § 4.3) : la moitié du FULL, toujours au-dessus de 100 ms, parce que l'énumération ne se réduit pas.

**Ce qu'il est honnête d'annoncer.** $K = 5$ FULL : objectif 100 ms, cible estimée 65 à 91 ms, à confirmer étage par étage sur G4. $K = 10$ FULL : mesurer le premier moteur (attendu 0,6 à 0,7 s), viser 0,3 à 0,4 s, ne rien promettre en dessous. $K = 10$ pour la seule hiérarchie d'ordre 10 : viser 0,2 s. Aucune de ces valeurs n'est une porte : la première mesure G4 du moteur v11 à $K = 10$ (M1) remplace toutes ces estimations.

---

## 7. Budget par étage (trame 02, G4, 48 fils)

« Mesuré » : reçu `g4_session4_j2c_20260929`, v10, CPU seul, dernière passe chaude (commandes 005, 007, 012, 013). « Leviers fondés » : estimations des auditeurs (L05 § 6.9 pour le catalogue sur la trame 02 ; L06 § 9.2 pour la tour sur la trame 00). « Cible » : estimation de ce document pour le catalogue (coûts par boule du § 1 après feuille par lots, filtres F6, noyaux AVX-512 : 2 700 à 3 400 cycles par boule à $K = 5$, 3 000 à 3 700 à $K = 10$ ; aucun n'est mesuré), et de `CONCEPTION_TOUR` pour la tour. Temps en millisecondes.

| Étage | $K = 5$ mesuré v10 | $K = 5$ leviers fondés | $K = 5$ cible | $K = 10$ mesuré v10 | $K = 10$ leviers fondés | $K = 10$ cible |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| préparation (nuage, index) | 8,1 | 8 | 3 à 5 | 8,3 | 8 | 3 à 5 |
| frontière de l'arbre | 26,6 | ≈ 6 | 3 à 6 | 39,6 | ≈ 10 | 5 à 10 |
| boîtes (filtre, feuilles) | 103,3 | 59 à 65 | 27 à 34 | 441,6 | 221 à 257 | 115 à 142 |
| ordre et assemblage | 32,5 | 16 à 19 | 8 à 12 | 123,8 | 62 à 73 | 25 à 45 |
| hors étages | 8,7 | ≈ 3 | ≈ 2 | 22,9 | ≈ 6 | ≈ 5 |
| **catalogue** | **171,1** | **84 à 93** | **40 à 54** | **627,9** | **300 à 345** | **150 à 202** |
| tour : supports, atlas, semis | 11,5 | 10,4 | — | 61,4 | 62 | — |
| tour : descentes | 27,1 | ≈ 27,5 | — | 203,4 | ≈ 233 | — |
| tour : forêt | 27,5 | 12 à 20 (0 à 2 si recouverte) | — | 66,4 | 34 à 57 (0 si recouverte) | — |
| tour : verticales | 21,5 | ≈ 4 | — | 64,1 | ≈ 20 | — |
| **tour FULL** | **89,3** | **42 à 62** | **22 à 32** | **406,2** | **315 à 370** | **125 à 180** |
| **moteur, préparation comprise** | **268** | **134 à 163** | **65 à 91** | **1 042** | **623 à 723** | **278 à 387** |
| un seul ordre $K$, hors préparation (§ 4.3) | 239 (catalogue 178, tour d'un ordre 61 : chaîne `mhgp10_cluster` de la v10) | 106 à 123 | 47 à 69 | jamais mesuré | 365 à 433 | 160 à 230 |
| pour mémoire : catalogue sur l'appareil (jamais mesuré) | — | — | 22 à 67 | — | — | 70 à 360 |

Lecture : à $K = 5$ la cible passe sous 100 ms si **tous** les coûts unitaires visés sont tenus et si l'efficacité parallèle atteint celle de l'étage des boîtes de la v10 ; les pistes de ce document y comptent pour 15 à 25 ms (R1.1, partagée avec la conception du générateur : 9 à 16 ms dans les boîtes ; R2.2 : 5 à 8 ms dans l'assemblage et les sondes), le reste vient des leviers des audits et des deux conceptions voisines. À $K = 10$ aucune colonne n'approche 100 ms.

---

## 8. Mesures décisives à faire sur G4

Toutes par l'agent de mesure, en passes chaudes dans un processus résident, cinq exécutions alternées au moins, binaires figés, sorties comparées par empreinte. Les trois trames du contrat, $K = 5$ et $K = 10$, 1, 24 et 48 fils sauf mention.

| Id | Prototype ou sonde | Variantes | Critère |
| --- | --- | --- | --- |
| M1 | premier moteur v11 de bout en bout | FULL à $K = 5$ et $K = 10$ ; temps par étage, temps CPU par fil | donne le temps honnête de $K = 10$ ; efficacité parallèle (temps CPU divisé par 48 fois le mur) ≥ 0,85 ; défauts de page par trame à chaud proches de zéro |
| M2 | microbanc de feuille sur candidats réels vidés, AVX-512 | (a) droite exacte en i64 puis quadruplets ; (b) feuille plate, quadruplets exacts réordonnés ; (c) feuille plate, quadruplets filtrés F6 par lots | (c) ≤ 0,6 × (a) sur triplets + quadruplets, zéro désaccord, repli exact non vide ; sinon garder le test de droite |
| M3 | jointure des semis et consultations de supports sur les dumps de la v10 | stockage canonique et table plate ; stockage en ordre d'émission et tables par blocs de ≈ 100 sites | adopter l'ordre d'émission si le gain dépasse 5 ms à $K = 5$ ou 20 ms à $K = 10$ à 48 fils |
| M4 | chemin « un seul ordre » | catalogue restreint (plage $[K, K]$) et tour d'un ordre, contre FULL | ≤ 0,75 × FULL à $K = 5$ et ≤ 0,6 × FULL à $K = 10$ pour valoir un chemin de produit chronométré |
| M5 | noyau de forêt sur les entrées réelles | numérotation des minima canonique ou spatiale ; seul ou recouvert par les descentes | forêt visible ≤ 5 ms à $K = 5$ et ≤ 20 ms à $K = 10$ |
| M6 | sonde de débit sur l'appareil, sans comportement indéfini | double (multiplication-addition), produit 64 → 64, produit 64 → 128, à un warp et à pleine charge ; transferts épinglés de 50 et 600 Mo | si le double dépasse 8 TFLOPS, les filtres F6 deviennent portables et le § 5 se recalcule ; sinon la décision R4 tient |
| M7 | part des feuilles dans le mur du moteur v11 à $K = 10$ (lue dans M1) | — | sous 50 %, aucune voie « feuilles sur GPU » n'est rouverte |
| M8 | banc de rejeu des feuilles sur l'appareil (seulement si M6 et M7 le justifient) | un warp par feuille ; noyaux par files | zéro désaccord ; noyau ≤ 55 ms à $K = 10$ ; voie appareil entière ≤ 110 ms |
| M9 | compteurs déterministes de la tour sur les trois trames entières (possible en local) | représentants, semis directs, plus petites boules, sphères hors catalogue, par ordre | recoupe le § 3.1 (72 % et 70 % de semis directs ; 20 à 23 % hors catalogue) |
| M10 | requête d'index « intrus à sortie anticipée » sur les sphères hors catalogue enregistrées | index implicite sur l'ordre de Morton | ≤ 300 cycles par requête ; sinon revoir D-I1 de `CONCEPTION_TOUR` |

---

## 9. Obligations de preuve nées de ce document

| Id | Énoncé | État | Fixture d'égalité à graver |
| --- | --- | --- | --- |
| PO-R1 | Feuille plate : retirer le test de droite ne change pas l'ensemble émis (théorème G : seuls comptent les candidats présentés et leur jugement) | immédiat ; mesuré ici (dumps identiques) | les fixtures du lemme Z deviennent des fixtures du test de boîte du centre |
| PO-R2 | Enveloppe du triangle médian : le centre d'un triangle strictement aigu est strictement intérieur à son triangle médian ; test fermé contre la boîte | classique, à écrire | centre sur une face de boîte ; triangle rectangle (centre sur le bord du triangle médian) |
| PO-R3 | Borne F6 de l'intérieur du tétraèdre par les barycentriques de Gram, en coordonnées locales : $M$ et $E$ de l'expression évaluée, seuil $2^{q+e-51}$ | à écrire avec la conception du générateur | signe nul (centre sur une face) ; repli déclenché |
| PO-R4 | Jointure des semis : $F$ est un semis si et seulement s'il existe une boule régulière $b'$ avec $P_{b'} = F$ ; la clé additive ne décide jamais, l'égalité des listes décide | immédiat | deux populations distinctes de même clé (collision forcée par un mélange de test) |
| PO-R5 | Stockage en ordre d'émission : la sortie canonique est une fonction des enregistrements et de la permutation, identique à celle d'un stockage canonique ; l'ordre d'émission ne dépend pas du nombre de fils | à écrire si R2.2 est retenue | différentiel v10 ; deux nombres de fils |
| PO-R6 | Pas de descente généralisé : si $B(F) = b$ et $F' \subseteq P_b$, $\lvert F' \rvert = k$, de trace séparable sur $U$, alors $\beta(F') < \lambda_b$ et $F$, $F'$ sont dans la même composante de $\Gamma_k(\lambda_b)$ ; seule la classe du terminal aux coupes $a \geq \beta(F)$ est unique | prouvé ici (lemmes 1 et 2 de L02), à contre-lire | $X = \lbrace 0, 2, 4 \rbrace$, $K = 2$ (terminal dépendant du choix) |
| PO-R7 | Lemme R3 : la forêt d'ordre $K$ ne consulte que les boules dont la fenêtre contient $K$ | prouvé ici, à contre-lire | $p + m = K$ ; $p + q_{\min} - 1 = K$ ; coquille étendue avec $p + q_{\min} < K \leq p + m$ |
| PO-R8 | Lemme R4 : filtre $\lvert O(T) \rvert \leq \lvert L \rvert - K$ | prouvé ici ; mesuré (ensemble utile identique) | paire avec $\lvert O \rvert = \lvert L \rvert - K$ exactement |
| PO-R9 | Admission par plage d'ordres : le catalogue de la plage $[k_{\min}, k_{\max}]$ est la restriction du catalogue FULL aux boules dont la fenêtre rencontre la plage | immédiat | restriction contre FULL, enregistrement par enregistrement |

Rien dans ce document n'affaiblit les énoncés de L01 et L02 ni les corrections de l'auditeur (surjection des morceaux, raffinement exhaustif, date d'usage du mémo, Euler comme diagnostic).

---

## 10. Ce que ces pistes demandent aux fondations

| Module | Exigence | Pour |
| --- | --- | --- |
| `core` | un domaine d'identifiants de plus si R2.2 est retenue : position d'émission d'une boule, distincte de `BallIdx` (rang canonique) ; la permutation entre les deux est un `Buffer` compté | R2.2 |
| `core` | mélange 64 bits fixe des `SiteIdx` (constantes écrites, aucune graine d'exécution) pour les clés additives ; aucun résultat ne dépend de sa valeur | R2.1 |
| `core` | arènes de session réutilisées d'une trame à la suivante ; L04 mesure 145 000 défauts de page par construction et 0,31 s de temps noyau à 48 fils | budget du § 7, M1 |
| `num` | chaque prédicat a une forme entière exacte de référence à budget de bits, par palier d'étendue locale (étendue de liste mesurée ≤ $2^{15}$ mm, L05) ; les filtres F6 sont des accélérateurs séparés, jamais la définition | R1.1, R4 |
| `num` | interface par lots en tableaux séparés (8 ou 16 voies) pour : dominance, aiguïté, enveloppes, barycentriques de Gram, côté d'une sphère ; compteur de replis exacts publié | R1.1, M2 |
| `sched` | parallélisme par tâches avec vol ; une tâche longue (noyau de forêt) peut tourner à côté d'une boucle parallèle ; partition et tri par base parallèles sur (clé 64 bits, charge 32 bits) ; écritures à positions fixées par ordinal | R2.1, R2.2, § 7 |
| `cloud` | sites en ordre de Morton, avec les bornes des blocs alignés (boîtes entières par bloc de sites) calculables en une passe : c'est l'index implicite et le découpage en blocs de la jointure locale | R2.2, R2.5 |
| `io` | export canonique en flux par la permutation ; dump au format de la v10 pour le différentiel ; chronométrage en processus résident, préparation publiée à part | R2.2, M1 |
| `catalogue` (contrat) | admission paramétrée par une plage d'ordres $[k_{\min}, k_{\max}]$ ; par boule : $p$, $q_{\min}$, $m$, drapeau, $I$ puis $U$ ; ni successeur ni plus petite boule de morceau | R3.1, R2.3 |

---

## 11. Points à soumettre aux auditeurs

1. **PO-R6.** Le pas généralisé (toute $k$-partie de $P_b$ à trace séparable) est-il couvert par leur lecture du théorème D, y compris l'échange d'un site de support contre un intrus ?
2. **PO-R7 et PO-R8.** Le chemin « un seul ordre » repose sur deux lemmes courts ; la contre-lecture utile porte sur les coquilles étendues (fenêtre $[p + q_{\min} - 1, p + m]$) et sur la monotonie de $O(T)$.
3. **R2.2.** Un stockage en ordre d'émission avec permutation canonique est-il compatible avec leur lecture de la conformité « octet pour octet » (qui porte sur les sorties, pas sur la mémoire) ?
4. **Semis.** Le compteur de la v10 lu comme « 86 % de semis » compte des fins de chaîne ; la part des semis directs est de 70 à 72 %. À corriger dans les documents qui reprennent le chiffre.

---

## Annexe A — Expériences de ce document

Toutes sur le codespace, un processus à la fois, quelques secondes chacune (26 s pour la plus longue) ; entrées : `build/v10-g4-data-s1/lidar02_quarter_x_nonneg_y_nonneg.u32le` (5 286 sites, sha256 `47f05b78…`, $K = 5$ et 10) et, comme second point à $K = 5$, `lidar01_quarter_x_neg_y_neg.u32le` (8 074 sites, sha256 `e1152804…`, fichiers `*_quart01*`) ; binaires v10 de l'audit L05 (`/tmp/v11-audit/l05_code_catalogue/build-rel/`, `mhgp10_catalogue` sha256 `a3bbad50…`, celui de L01). Dossier : `conception/preuves_pistes_de_rupture/` (moins de 200 Ko, aucune coordonnée de trame).

| Expérience | Fichiers | Ce qu'elle établit |
| --- | --- | --- |
| A1 morceaux | `pieces_stats.cpp`, `stats_k5.txt`, `stats_k10.txt` | semis directs, nature du premier pas, pas d'une descente idéale, certification par $N_K(c)$, déplacement du centre |
| A2 tour v10 sur la même entrée | `tour_v10_quart_k5.json`, `tour_v10_quart_k10.json` | compteurs `resolves`, `seed_hits`, `meb`, `closed_balls` de la v10 : recoupement de A1 |
| A3 forêt et blocs | `foret_blocs.cpp`, `foret_k5.txt`, `foret_k10.txt` | forêt reconstruite (une racine par ordre ; nœuds à 16 et 5 unités du reçu G4) ; fusions locales à un bloc ; localité des liens de semis |
| A4 sonde du générateur | `patch_probe.py`, `patch_probe2.py`, `generator_sonde_rupture.cpp`, `sonde_compteurs.txt` | sans test de droite : quadruplets ×2,41, dumps identiques ; entonnoir q3 ; filtre « un seul ordre » : ensemble utile identique, candidats −1 à −9 % |
| A5 hérédité | `heredite_q2.py`, `heredite.txt` | arêtes et faces des boules q3 et q4 absentes du catalogue |
| A6 boules utiles | `needed.py` | boules dont la fenêtre contient $K$, avec empreinte |

Limites. Deux quarts de deux trames, un seul à $K = 10$ ; la géométrie de A1 et A3 est en binaire64 à tolérance relative $10^{-10}$ et ignore les coquilles étendues (10 et 25 boules) ; les plateaux ne sont pas regroupés dans A3 (une fusion par jonction unissante) ; à $K = 10$ la descente idéale de A1 est échantillonnée (un morceau hors semis sur quatre). Les égalités d'ensembles (semis, boules utiles, dumps) sont exactes. Aucun temps de ce document n'est une mesure : tous les temps G4 viennent des reçus de la v10.

## Annexe B — Reproduire

```bash
D=/workspaces/E-HGP/build/v11-persist/conception/preuves_pistes_de_rupture
IN=/workspaces/E-HGP/build/v10-g4-data-s1/lidar02_quarter_x_nonneg_y_nonneg.u32le
V10=<build Release de morsehgp3D_v10 a afb081774>          # mhgp10_catalogue, mhgp10_tower, libmhgp10_core.a
$V10/mhgp10_catalogue $IN --k=5 --threads=1 --dump=q_k5.dump > q_k5.json
g++ -O2 -std=c++20 -o pieces_stats $D/pieces_stats.cpp && ./pieces_stats $IN q_k5.dump 5 1      # K = 10 : dernier argument 4
g++ -O2 -std=c++20 -o foret_blocs $D/foret_blocs.cpp && ./foret_blocs $IN q_k5.dump 5
python3 $D/heredite_q2.py q_k5.dump 5 ; python3 $D/needed.py q_k5.dump 5
$V10/mhgp10_tower $IN --k=5 --threads=1 --no-points                                             # compteurs "join"
# sonde : copier src/ et cli/ de la v10, puis
python3 $D/patch_probe.py src/catalogue/generator.cpp && python3 $D/patch_probe2.py src/catalogue/generator.cpp
g++ -O3 -DNDEBUG -std=c++20 -I src -o cat_probe cli/mhgp10_catalogue.cpp src/catalogue/generator.cpp $V10/libmhgp10_core.a -lpthread
MHGP_X_NOLINE=1 ./cat_probe $IN --k=5 --threads=1 --dump=noline.dump ; MHGP_X_ORDER=1 ./cat_probe $IN --k=5 --threads=1 --dump=order.dump
MHGP_X_COUNT=1 ./cat_probe $IN --k=5 --threads=1
```

Fin de rédaction : 2 octobre 2026, 08:42 UTC (`date -u`).
