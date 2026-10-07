# Audit final v11 : catalogue (étage `domain`) et voie GPU

## 1. Périmètre et état

Cadre : `exploration_v11_hors_registre` ; `cpu_reference` pour la référence et `cuda_g4` pour la voie de banc ; `quantized_u21_input_only` ; `not_claimed`. J'ai seulement lu l'instantané `ac081a06f` : aucune compilation, aucune exécution. **GCP non utilisé**.

Origine des chiffres : sauf mention « (inférence) » ou « local », chaque chiffre vient d'un reçu G4 ou du code. Trames : SemanticKITTI 08/000000, 000100 et 000200, sans sol (39 885 / 35 551 / 45 845 sites), W48. Machine G4 : EPYC 9B45 (24 cœurs physiques × 2 SMT) et RTX PRO 6000 Blackwell (sm_120).

**Algorithme** (`src/catalogue`). `prepare_full_domain` construit CatK, puis la table support → boule.
- **Frontière** (`adaptive_frontier.cpp`, `adaptive_prepare.cpp`) :
  - la racine est filtrée en série ;
  - puis des rondes « lourds d'abord » : seules les listes dont la population atteint max/2 sont coupées ;
  - le plan a au plus 1024 feuilles. Sur les trames : 1023 tâches et 27 à 28 rondes, réclamées dans l'ordre LPT.
- **Passe unique** (`single_pass.cpp`) : chaque tâche poursuit le DFS de boîtes.
  - Filtre G1 : réservoir de 3K témoins ; un site est retiré s'il est strictement dominé par K témoins sur la fermeture.
  - Puis ajustement de la boîte et coupe médiane.
  - Sorties en pages par ordinal, puis compactage parallèle.
- **Feuille** (`leaf.cpp`) :
  - dominances en O(m²) sous forme de masques ;
  - graphe de paires et lignes vivantes (m ≤ 32) ;
  - DFS des préfixes q2 à q4 sous les filtres G3, J2, M3 et E4 ;
  - census par masques (lemme R), puis support canonique ;
  - seul S* est émis ; les niveaux q3 et q4 sont différés.
- **Lot** (`batch_leaves`, `cuda_leaves`) :
  - les feuilles d'au plus 32 sites sont mises en file (`leaf_queue.hpp`) ;
  - **après le join de la passe unique**, elles sont rassemblées puis jouées par `leaf_device.hpp`. C'est une source unique hôte/CUDA, à un fil par feuille. Elle n'utilise que les chemins i128 certifiés ; sinon la feuille rend `kUnresolved` et `leaf.cpp` la rejoue avant admission ;
  - le Level est recalculé sur l'hôte ;
  - un exécuteur partagé répartit le lot entre hôte et GPU (`leaf_batch_split.cpp`).
- **Fin** :
  - tri indirect (clés binary64 F3/F4, avec repli exact) ;
  - balayage des niveaux, puis assemblage par blocs ;
  - table à sondage linéaire, remplie par CAS.

**État au 7 octobre.**
- Meilleure voie de banc à K5 : `868347:400` (GPU, feuilles de 24, partage 400 ‰, cache de blocs). `domain` : 136/120/140 ms à chaud, 224/205/231 ms à froid.
- Voie CPU `802811` (feuilles de 16) : 198/162/194 ms à chaud, 208/190/209 ms à froid (reçu `cache_blocs`).
- Le `domain` seul dépasse donc encore 100 ms.
- **Le produit n'utilise aucune de ces deux voies.** L'API (`src/api/compute.cpp`, `kEngineMask = 278523`) joue la voie CPU, avec des feuilles de 16 à tout K, sans GPU ni cache.

**Chronologie de l'étage**

À K5, W48, en ms (reçus sous `receipts/`) :

| Date | Commit, reçu | Configuration | ng00 / ng01 / ng02 |
|---|---|---|---|
| 2 oct. | `e6fe34cb0`, `catalogue_20261002` | séquentiel, 2 passes, feuilles de 16 (API catalogue) | 26 018 / 20 741 / 24 093 |
| 2 oct. | `c1046dfc7`, `catalogue_parallel_20261002` | Pool, frontière fixe, profondeur 8 | 4 460 / 3 002 / 4 028 |
| 2 oct. | `df069960a`, `catalogue_optimizations_20261002` | + cache J2 + tri indirect | 3 239 / 2 115 / 2 790 |
| 3 oct. | `4b8e04be6`, `catalogue_assembly_20261003` | + frontière adaptative + assemblage par blocs | 1 709 / 1 429 / 1 900 |
| 3 oct. | `90dd48bd2`, `catalogue_single_full_20261003` | + passe unique (`domain` FULL) | 845 / 730 / 872 |
| 3 oct. | `895680ff8` → `c40f40798`, `qualification_performance_20261003` | mode 2047, avant puis après `ef75dafac` | 708/597/748 → 265/217/249 |
| 3 oct. | `c40f40798` | 16379 (+ graphe de paires), à froid | 227 / 181 / 239 |
| 4 oct. | `22a6af6aa`, `developpement_20261004/gpu_g4` | CPU f16 ; GPU un fil f16, à chaud | 208/171/202 ; 245/211/232 |
| 6 oct. | `79fa5e9f7`, `reservoir3_chemin_chaud` | GPU f24, à chaud | 192 / 169 / 192 |
| 6 oct. | levier C, `wfgpu1_leviers` | + arithmétique étroite | 177 / 158 / 182 |
| 6 oct. | `c6e294d11`, `lot_partage_confirmation` | + partage 400 ‰ | 163 / 138 / 165 |
| 7 oct. | `ccdd4db75`, `cache_blocs` | + cache de blocs | 136 / 120 / 140 |

À K10, feuilles de 24, à chaud :
- le 2 octobre, toute prise K10 expirait au plafond de 15 s ;
- le 4 octobre : CPU 830/653/779, GPU 742/631/711 ;
- puis GPU 659/550/629 (`j2memo`), 589/475/563 (levier C), 490/397/472 (cache de blocs).

Ces lignes mêlent sessions, régimes et tailles de feuille. Le bruit A/A entre processus atteint ±9 à 12 %.

**Répartition du temps** (ng00, K5, une prise chaude, `developpement_20261007/diagnostic_domaine`) :

| Sous-étage (ms) | GPU `868347:400`, f24 | CPU `802811`, f16 |
|---|---:|---:|
| Frontière (racine / rondes / sélection / publication) | 20,7 (0,9 / 14,4 / 0,7 / 4,6) | 19,9 |
| Passe unique | 37,5 (Σ des tâches 1,78 s CPU) | 152,1 (Σ 7,0 s) |
| Lot : hôte 31 902 feuilles en parallèle de GPU 91 679 | 51,8 (49,8 et 50,1 ; comptage GPU 47,8) | — |
| Level sur l'hôte + rassemblement | 3,2 | — |
| Tri / balayage / assemblage / compactage | 10,0 / 4,4 / 3,4 / 3,2 | 10,0 / 4,4 / 3,3 / 2,8 |
| Restitution / table support → boule | 0,8 / 2,9 | 3,6 / 2,9 |
| **domain** | **138,0** | **199,2** |

Lecture :
- Ces sous-étages s'enchaînent strictement en série.
- En voie lot, la passe unique est surtout faite des filtres G1 des nœuds internes.
- La frontière est faite de rondes étroites (environ 0,5 ms chacune). Elle n'a pas bougé depuis le 3 octobre (17 à 21 ms).
- À K10 (`cache_blocs`) :
  - passe unique 116 à 141 ms, exécuteur 167 à 207 ms ;
  - puis 88 à 112 ms de petits étages en série : tri 32–46, balayage 15–19,5, assemblage 12–15, compactage 9,5–11,5, Level 11–14, table 8–10.

## 2. Ce qui a marché

**Structure CPU (2 et 3 octobre)**
- **Frontière possédée et Pool** : 25 s → 4,5 s, sorties égales au séquentiel (`catalogue_q4_20261002` → `catalogue_parallel_20261002`).
- **Tri indirect puis clés F3/F4** : tri 0,9–1,4 s → 9–13 ms (`parallel5`, puis `qualification_performance_20261003/review/analysis.md`).
- **Assemblage par blocs** : 167–250 ms → 4,0–5,2 ms.
- **Frontière adaptative** : catalogue ×1,25 à 1,61 (`assembly1`).
- **Passe unique** : `domain` ×1,65 à 1,80 (`combined3`).
- **Commit `ef75dafac`** (lourds d'abord, LPT, termes G1 précalculés, G3 avant J2, lignes vivantes) :
  - plus longue tâche 0,885 → 0,045 s (mesure locale, `developpement_20261003/ecart_v10_v11`) ;
  - `domain` 708 → 265 ms sur ng00 (paquet de changements, mesure appariée).
- **Graphe de paires** : 265→227 / 217→181 / 249→239 ms. C'est le seul bit du catalogue qui sépare les modes 2047 et 16379 ; l'attribution du gain est une inférence.
- **Table support → boule remplie par CAS** : environ 30 → 3 ms (`src/tower/full_domain.cpp`, champ `lookup_ms`).
- **Niveaux différés** :
  - q4 : 99,7 % des niveaux q4 évités (`catalogue_q4_20261002`) ;
  - q3 : −1,5 à −1,8 % de passe unique à W1 (`mesures_g4_ab8_diag1`).

**Voie GPU (4 au 7 octobre)**
- **Exactitude tenue de bout en bout** :
  - 372 prises froides et 84 processus chauds rendent les mêmes dumps et le même registre que le CPU (`gpu_g4`) ;
  - ensuite, chaque banc est « conforme » ;
  - Compute Sanitizer ne relève aucune erreur (coop1, reservoir2, wfgpu1) ;
  - le contrat R7 (exact sur l'appareil, sinon repli exact avant admission) a tenu : aucun écart de sortie.
- **Cases par feuille, rassemblement et copie par le Pool, pages pré-touchées, enregistrements compacts** (`b74f9ea3a`, `16b482169`, `4ec33e3d7`) :
  - écriture 35 → 19 ms ;
  - rassemblement 10,5 → 2,4 ms ;
  - retour 22 → 2,3 ms (débit 4 → plus de 10 Go/s) ;
  - Level 15 → 5,6 ms.
- **J2 mémorisé et rangs locaux directs** (`34a8a561d`) : exécuteur ×0,82 à K5, ×0,76 à K10.
- **Réservoir chaîné** (`79fa5e9f7`) : écriture 13 → 0,3 ms à K5, 75 → 1,7 ms à K10.
- **Feuilles de 24 à K5 sur GPU** : première victoire du GPU sur le CPU ; le parcours tombe à 29–35 ms (`reservoir3`).
- **Arithmétique étroite, levier C** (`5861c223f`) : comptage ×0,83 à 0,86 sur les feuilles d'étendue ≤ 2^20. Les bornes ont été refaites (6D³ < 2^63) et l'auditeur a rendu un avis favorable (`audit_narrow_followup_20261006`).
- **Exécuteur partagé à 400 ‰** (`2045ec27c`) : `domain` à chaud ×0,873 à 0,907.
- **Cache de blocs** (`ccdd4db75`) : restitution 14 → 0,8 ms ; `domain` GPU à chaud −18 à −25 ms ; mur à chaud, moyenne géométrique 0,923.
- **K10, feuilles de 24 au lieu de 16** : −24 à −27 % du mur FULL (`mesures_g4_ab8_diag1`).

## 3. Ce qui n'a pas marché ou a été retiré

| Levier | Cause, chiffre | Preuve |
|---|---|---|
| Frontière fixe de profondeur 8 | une tâche de 1,527 s dans une phase de 1,533 s | `catalogue_parallel_20261002`, `docs/CATALOGUE_OPTIMISATIONS.md` |
| Règle adaptative initiale | plan épuisé vers la profondeur 11 ; zones denses laissées en tâches de milliers de sites | `docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md` |
| Filtre flottant F6 de `power` | ≈ 1 % du CPU | `developpement_20261003/pipeline_g4` |
| Enveloppes M3/E4 sur CPU | +1,2 à 1,4 % de passe unique à W1 ; pourtant conservées | `mesures_g4_ab8_diag1` |
| Lot sur l'hôte | jamais meilleur que la voie CPU (S4 : 374 contre 233 ms) ; sert à la validation | `gpu_g4` |
| Fils triés par taille de feuille | sans effet : m prédit mal le travail (Spearman 0,55 ; triplets candidats 0,91) | `gpu_g4`, `wfgpu1_leviers/workflow/D_RAPPORT.md` |
| Blocs d'un warp | barrière retirée, mais attente mémoire 1,19 → 3,03 ; noyau −3 % seulement | `gpu_g4` |
| N1, arène du parcours | +2,4 à +8,5 ms ; explication retenue : le SMT (9,1 → 13,8 s CPU de W1 à W48), non établi comme cause exclusive | `n1_ab`, retrait `c1675e4c9` |
| L4, 16 sous-lots recouverts | ×1,6 à 3,5 plus lent : chaque sous-lot paie sa propre queue | `l4_recouvrement`, retrait `830473218` |
| Feuille coopérative par paires | exécuteur ×1,15 à 1,38 à K5 ; divergence intacte, 198 registres | `coop1/2/3`, retrait `d4228f5e5` |
| Levier A, réservoir sans appel | ×1,11 à K5/16, malgré le meilleur SASS statique | `wfgpu1_leviers` |
| Leviers B (feuilles lourdes au CPU) et D (ordre par travail) | prédicteur à 2–9 µs par feuille sur le chemin critique ; un équilibrage parfait serait borné à ×0,86–0,93 | `wfgpu1_leviers/workflow` |
| Partage à froid, et partage à K10 | contexte CUDA ≈ 75 ms à froid ; 250 ‰ donne 0,96 à K10, sans règle écrite | `lot_partage`, `lot_partage_confirmation` |
| Rondes de frontière par tranches | frontière : moyenne géométrique 0,859, au-dessus du seuil 0,80 | `frontiere_tranches`, retrait `64746f985` |
| Filtre G1 en AVX2 | moyenne géométrique 0,895, au-dessus du seuil 0,85, alors que passe unique + frontière baissaient de 12 à 17 % en voie GPU | `filtre_g1_avx2`, retrait `23b759dfe` |
| Pages de 2 Mio (THP) | mur à chaud ×1,064, alors qu'en local les forêts gagnaient 17 % | `thp_exploration` |
| Feuilles de 24 sur CPU à K5 | +17 à +21 ms | `reservoir3`, `domain_diagnostic` |

Autres constats :
- Le lemme R et les compteurs locaux R1 sont neutres au mur (rapports 0,999 à 1,006 à W1) ; ils sont gardés pour le contrat.
- La voie GPU un fil a subi une régression (écriture ×2) :
  - d'abord imputée aux tables passées par référence (364 `LD` génériques contre 173). La correction (`ee3eabe5e`) n'a pas suffi ;
  - la vraie cause était l'appel à `extend_one` (`BSSY` 85 → 107). Une fois corrigée (`9eee2ed4b`), la voie revient à 1 % des valeurs L4.
- Le chemin froid du réservoir garde un surcoût de comptage (+28 à 32 %, dû à un `CALL`). Le levier C ne le compense qu'en partie (inférence par produit des rapports).

## 4. Pièges et leçons

- **Règles d'adoption écrites d'avance.** Bonne discipline, mais les seuils de 0,80 à 0,85 et des statistiques mal choisies ont fait rejeter des gains réels de 5 à 15 % (G1 AVX2, rondes par tranches). Exemples de statistiques mal choisies : mesure à froid pour un levier GPU, voie CPU qui dilue l'effet de G1. Leçon du développeur : seuils à 3–10 % et bras A/A.
- **Le codespace ne prédit pas G4.** THP : −17 % en local, +6 % sur G4. G1 AVX2 : −36 % en local, −12 à −17 % sur G4.
- **Le SASS statique ne prédit pas le temps** (levier A). En revanche, les comptes de `BSSY` et `CALL` signalent les régressions de reconvergence : les compter avant chaque session.
- **Masques réutilisés.** Le même jour, le masque `212987` a désigné tour à tour le GPU recouvert (L4), la feuille coopérative (coop1 à coop3) puis le lot sans réservoir (reservoir2).
- **Identité à chaud limitée à la dernière passe** : 162 passes intermédiaires n'ont aucune empreinte (`audit_g4_coop1_20261006`).
- **Exiger des compteurs identiques entre voies a coûté du travail physique** : le cache J2 était simulé sur GPU pour ses seuls compteurs (5 à 7 % du noyau en S3), avant d'être converti en vrai mémo.
- **Le « 6 à 7 µs par nœud » était du SMT** : mesurer le CPU par fil avant d'attribuer un coût.
- **À 48 fils, cinq prises ne tranchent rien sous environ 30 à 40 ms.**

## 5. Dettes et problèmes ouverts

**Limites structurelles**
- **GPU sous-occupé.**
  - Avec un fil par feuille, seuls 3,1 à 3,4 fils sur 32 sont actifs par warp.
  - Occupation atteinte : 17 à 22 %.
  - La pile locale de 3,2 Kio par fil est lue à 2,2 octets utiles par secteur ; l'ALU tourne à 24 %.
  - Selon le modèle calé sur Nsight du rapport D, un warp dure environ la somme du travail de ses 32 feuilles divisée par 3.
  - Le GPU ne travaille que pendant ~50 des 138 ms du `domain`, et il attend le join de la passe unique. Son utilisation effective est de l'ordre de quelques pour cent (inférence).
- **Ouverture CUDA à froid.**
  - 78 ms sous Nsight, 124 à 151 ms sous charge (`claudediag1`).
  - La passe qui précède le lot ne dure qu'environ 60 ms, d'où 62 à 88 ms d'attente au premier lot.
  - À froid, la voie GPU perd donc : 224/205/231 contre 208/190/209 ms pour la voie CPU.
- **Équilibrage hôte/appareil.**
  - Le poids m³ corrèle mal au travail réel.
  - Le modèle annonçait 42 à 48 ms pour le lot ; on mesure 48 à 58 ms.
  - Un petit Pool et un `std::thread` sont recréés à chaque lot (coût non mesuré).
- **Mémoire.**
  - Sur l'appareil : 196 à 243 Mo à K5, 1,15 à 1,42 Go à K10.
  - L'essentiel vient des cases de 2 Kio par feuille, plus 1/8 de réservoir (inférence du code : 530 k feuilles × 1,125 × 2 Kio ≈ 1,2 Go).
  - Les feuilles se recouvrent (13 feuilles par site à K10/24) : risque d'échelle pour les nuages de plusieurs millions de points.
- **CPU.** La machine n'a que 24 cœurs physiques, et la voie CPU n'a gagné qu'environ 5 % du 4 au 7 octobre (208/171/202 → 198/162/194).

**Dettes**
- **API et CLI hors des voies de référence** : ni GPU, ni cache de blocs (en attente des auditeurs, section Y), et des feuilles de 16 même à K10.
- **Aucune porte CUDA dans la matrice G4** (`tools/g4_matrix.py`). Les portes demandées par l'auditeur ne sont couvertes que côté hôte (`mhgp11_catalogue_leaf_narrow`, `mhgp11_tower_full_leaf_lanes`) :
  - q3 extrême ;
  - q4 au seuil 2^20 et 2^20 + 1, sur l'appareil ;
  - préfixe obtus ;
  - coquille à qmin = 2.
- **Repli `unresolved` en série sur le pilote** (`fallback` dans `single_pass_batch.cpp`). La nouvelle classe de feuilles non résolues (étendue > 2^20) apparaîtrait sur un nuage épars ou une grille u24. La recette U2 des auditeurs n'est pas implémentée.
- **Enveloppes M3/E4 sur CPU** : mesurées légèrement négatives, mais conservées.
- **Feuille cohérente** (un warp sur un même préfixe, census réparti sur les lanes) : conçue et acceptée par l'auditeur (section Q), jamais écrite.
- **Feuille J3 de la v10** (×1,37 mesuré en local, à 4 fils au plus) : portée en partie seulement (lemme R, M3/E4).
- **Documentation** : `docs/DEVELOPPEMENT.md` est figé au 3 octobre ; ni `README.md` ni `docs/ARCHITECTURE.md` ne décrivent la voie GPU.
- **Différentiel canonique v10/v11 sur trames entières** : toujours ouvert. Le catalogue v10 tournait en 164/137/164 ms (`audit_deep_20261004/performance`).
- **Régime du contrat** : aucune décision écrite dans le dépôt (processus neuf ou flux résident). Les règles ont alterné : `claudesplit1` à froid, `claudesplitconf` à chaud.

## 6. Recommandations concrètes pour la v12

1. **Avant le code**, fixer le régime (flux résident à 10 Hz ou processus neuf), la statistique, des seuils de 3 à 10 % et un bras A/A. Nommer les modes, sans jamais réutiliser un bit, et mesurer sur G4 seulement.
2. **Budget** : pour 100 ms FULL, il faut un `domain` d'au plus 40–50 ms à K5 (condition U2 du plan, `audit_plan_gpu_20261006/PLAN_GPU_FINAL.snapshot.md`). Concevoir pour ce budget, pas par retouches.
3. **`domain` résident sur l'appareil et en flux** : la frontière émet ses feuilles vers une file consommée pendant le parcours (noyau persistant ou lots courts), sans join global. Contexte CUDA, pool et blocs sont ouverts une fois par Session.
4. **Une seule forme de feuille, data-parallèle par warp** (feuille cohérente Q ou J3), en source unique SIMD et CUDA. Bannir « un fil par feuille ».
5. **Filtres G1 des nœuds internes** vectorisés ou exécutés sur l'appareil : ils coûtent 37 ms à K5 et environ 140 ms à K10. Découper aussi la racine et les premières rondes par tranches.
6. **Sorties sans cases fixes ni rejeu.** La sortie finale est triée canoniquement (niveau, S*) ; un ajout par blocs atomiques suffirait donc (inférence). Garder le refus transactionnel et le contrôle des doublons. Faire tri, balayage et assemblage sur l'appareil.
7. **Contrat de compteurs pensé pour le GPU** : compteurs logiques indépendants de l'ordre de visite, diagnostics physiques séparés.
8. **Repli `unresolved` parallèle et budgété dès l'origine**, avec une porte qui mêle des feuilles d'étendue 2^20 et 2^20 + 1.
9. **Taille de feuille par K et par voie** (table M(K) de la v10 : 12/16/24/28), la même dans l'API et dans le banc. Revoir aussi la partition des centres : sous-maille 1/64 en v10, arrêt à la largeur entière 1 en v11.
10. **Mémoire de sortie proportionnelle aux émissions**, pas aux feuilles ; cache de blocs budgété dans la Session.
11. **Dès le premier jour** : portes CUDA natives dans la matrice, identité vérifiée à chaque passe chaude, garde sur `BSSY`/`CALL`, sous-chronos par étage et temps CPU par fil.
12. **Garder la doctrine d'exactitude** (i128 certifié ou repli exact avant admission) et la transaction sans publication partielle : quatre jours de voie GPU sans un seul écart de sortie.

## 7. Références clés

Chemins relatifs à `morsehgp3D_v11/`.

- **Documentation** : `docs/CATALOGUE.md`, `docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md`, `docs/CATALOGUE_SINGLE_PASS.md`, `docs/CATALOGUE_OPTIMISATIONS.md`, `docs/PERFORMANCE_FULL.md`, `docs/FULL_DOMAIN.md`.
- **Code** :
  - `src/catalogue/single_pass.cpp`, `single_pass_batch.cpp`, `leaf.cpp`, `leaf_device.hpp`, `leaf_batch.hpp`, `leaf_batch_cuda.cu`, `leaf_batch_split.cpp`, `adaptive_frontier.cpp`, `boxes.cpp` ;
  - `src/api/compute.cpp` ;
  - `bench/full_probe.cpp`, `bench/gpu_ab.py`.
- **Reçus de développement** :
  - `receipts/catalogue_*`, `receipts/qualification_performance_20261003` ;
  - `receipts/developpement_20261004/{gpu_g4,mesures_g4_ab8_diag1}` ;
  - `receipts/developpement_20261006/{coop1_feuille_cooperative,coop2_correction_mesuree,coop3_reconvergence,j2memo_rangs_locaux,reservoir_cases_chainees,reservoir3_chemin_chaud,wfgpu1_leviers,domain_diagnostic,lot_partage,lot_partage_confirmation,n1_ab,l4_recouvrement}` ;
  - `receipts/developpement_20261007/{diagnostic_domaine,cache_blocs,filtre_g1_avx2,frontiere_tranches,retention_tas,thp_exploration}`.
- **Reçus et notes d'audit** :
  - `receipts/audit_gpu_euler_20261004`, `audit_gpu_scratch_20261004`, `audit_plan_gpu_20261006`, `audit_narrow_followup_20261006`, `audit_g4_coop1_20261006`, `audit_deep_20261004` ;
  - `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` ;
  - `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`, sections L à Y.
