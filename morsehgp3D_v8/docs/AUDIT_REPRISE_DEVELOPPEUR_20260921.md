# Audit complet de reprise et plan de développement (21 septembre 2026)

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only` pour le seul moteur global mesurable
(`lossless_float32_input_only` pour les briques natives, sans générateur),
`mode=audit_complet_puis_developpement`, `public_status=not_claimed`.
GCP non utilisé pour cet audit (feu vert G4 noté, à employer quand une mesure
l'exige). HEAD audité : 204b0620 (puis ec2bc503 pour la préparation u16 sans
sol). Neuf lentilles indépendantes en lecture seule (état et contrats ; build et
tests ; moteur u16 q3/q4 ; briques float32 et brouillons ; voie q2 et
parallélisme ; pilote sans sol et données ; héritage GPU et protocole G4 ;
qualité des portes ; base de temps), chacune avec faits vérifiés, lignes et
reçus. Ce document remplace, pour l'état courant, la lecture des 1 431 lignes
de la passation ; il ne promeut aucun statut.

## Décision utilisateur du jour et écart mesuré

Le régime prioritaire est le LiDAR SemanticKITTI **sans sol**, 30 000 à 60 000
sites par trame ; c'est là que les contrats **temps (1 s puis 100 ms)** et
**K (5 ou 10)** doivent passer, avec parallélisation multi-CPU puis GPU.

Première mesure jamais faite sur ce régime (lentille moteur, diagnostic sur
hôte partagé, non épinglé ; à reproduire en reçu) : scène 0 sans sol, 39 815
sites u16 à 2 cm, K5, s8, configuration mesurée (rectangle-pair, boxes, affine,
live), 8 workers locaux :

| grandeur | valeur |
| --- | ---: |
| temps mur | 279,3 s |
| temps CPU | 1 239 CPU·s |
| boules q3 / q4 émises | 663 443 / 157 889 |
| graines q3 construites | 179,7 M (4 514 par site) |
| bornes de census q3 | 6,03 G |
| tests de partition q4 | 10,25 G |
| masse résiduelle q3 après citron | 11,4 % des paires (2,9 % avec sol) |

Base de temps complémentaire (lentille 9, même sonde, hôte partagé, une
répétition) : sans sol K5 un worker 898,6 s mur pour 897,8 s CPU ; K5 quatre
workers 283,1 s pour 1 051,9 s CPU ; **K10 quatre workers 1 180 s pour 3 922 s
CPU** (2 830 806 q3 et 1 729 751 q4 émis). Contre le tirage avec sol de 32 000
sites : K5 un worker 103,3 s, K10 321,7 s. Accélération réelle 1 → 4 workers :
×1,5 à ×3,2 (plan Coarse : un worker reçoit 1,5 % des rectangles, un autre
40 %). Sans sol, les paires développées sont ×9,8 (24,0 M contre 2,45 M), la
couverture moyenne par arête 1 431 sites contre 228, 105 graines q3 par arête
contre 28, et 99,6 % des 179,7 M boules q3 recensées sont rejetées par
profondeur : l'élagage par témoins s'effondre dans l'espace vide et le census
paie des graines dont le rejet aurait dû précéder. Profil d'instructions à 8k
avec sol (callgrind, pas du temps) : q4 Local28 49 % (atlas 33 %, graines ×
cellules 16 %), census q3 26 %, filtres de témoins 14 %.

Par site, le nuage sans sol coûte 2,6 à 3,9 fois plus qu'avec le sol : le rejet
par témoins universels perd le plan du sol qui remplissait les grandes boules.
Ordre de grandeur du contrat (pas une prédiction) : 1 s sur 48 CPU = 48 CPU·s ;
il faut ×13 à ×18 de travail en moins à K5 avec un parallélisme parfait, ×50 à
×65 à K10 (rapport K10/K5 mesuré ×3,74 à 32k), et dix fois plus pour 100 ms.
Et ceci pour un **flux de candidats** : ni catalogue, ni intérieurs, ni fold,
ni tour n'existent dans la v8 (`src/forest`, `src/io`, `src/cloud` vides).

## État par composant

| composant | état | preuve | limite |
| --- | --- | --- | --- |
| Moteur global u16 q3/q4 (`run_wspd_q34_parallel`) | acquis, exact en entier, 96 CTests | reçus `q34_*`, `lidar_global`, `q34_spatial` ; porte `--selftest` PASS au HEAD | flux de présentations avec doublons possibles ; jobs = sous-arbres entiers du front ; 2 à 11 CPU occupés sur 48 (G4) |
| Voie q2 u16 (`run_wspd_q2_*`) | acquise, 45 CTests | reçus `q2_*` ; LiDAR 50k préfixes : 1,45 s K5, 2,8 à 5,5 s K10 à 4 cœurs | hors de l'appel q3/q4 ; jamais mesurée sans sol ; sept opt-in négatifs |
| Briques float32 natives (index, prédicats, boules, clés, événements q4, census q3 d'une arête) | qualifiées par reçus | `receipts/float32_*` | hors CMake et CTest ; ×23 à ×70 par prédicat contre l'u16, clé 78 µs ; 68 à 185 ms **par arête** sur la scène sans sol ; pas de générateur global |
| Brouillon de raccord global float32 (non suivi) | en cours par l'ancien constructeur | fichiers `src/wspd/float32_front.*`, `src/core/float32_edge_geometry.*`, `src/core/float32_q3_owned_block.*`, `src/lanes/float32_q3_owned.hpp`, `src/pipeline/float32_q3_global.*`, deux sondes | non liable (lane possédée sans définition), une sonde hors `-Werror`, q3 seulement, mono |
| Pilote sans sol (Patchwork++ 3e6903a1, masque d'IDs) | acquis | `receipts/lidar_ground_20260921` (lectures 4/4 après réparation) ; contrôle indépendant : partition complète, coordonnées bit-identiques, masque plausible (sol médian z = −1,734 m pour un capteur à 1,723 m) | payloads f32/u32 seulement ; profil u16 ajouté ce jour (ec2bc503) |
| Nuages sans sol u16 (2 cm) | acquis ce jour | `receipts/lidar_ground_u16_20260921` : 39 815 / 35 491 / 45 114 sites, sept morceaux par scène | entrée de calcul, aucune mesure encore épinglée |
| Aval (catalogue, intérieurs, fold, tour K = 1..10, lanceur FULL) | inexistant | — | le contrat porte sur la tour |
| GPU | inexistant en v8 (`src/gpu` vide) | protocole G4 v8 CPU seulement (`GPU_executed` doit être faux, budget utile ≤ 900 s) | v5 à v7 : kernels à 2–4 % de leur étage, coût hôte dominant |
| Tests | 96 CTests u16 verts (92 à 106 s) | build neuf Release, 0 avertissement | 30 fichiers de tests hors CTest (float32, LiDAR, spatial, mutations) ; aucun label ; aucune porte moteur sur un nuage sans sol ; CI GitHub sans la v8 |

## Points durs relevés

1. **Travail avant rejet** : 245 M boules q3 construites pour 1,25 M émissions sur
   la trame brute (×196), 179,7 M graines pour 663 k émissions sans sol ; les
   deux postes à 10^10 opérations par trame (bornes de census q3, tests de
   partition de l'atlas q4) sont en i128 sans filtre flottant, donc non
   portables tels quels en SIMD ou GPU.
2. **Cinq traversées d'index par arête** (cover, géométrie, domaine, graines
   q3, graines q4) recalculent la même géométrie : environ 820 visites par arête,
   trois copies du test de lentille, huit définitions des prédicats de
   propriété.
3. **Granularité parallèle** : un job = un sous-arbre entier du front, jamais
   scindé ; sur G4, un seul worker porte 18 à 39 % des paires ; aucun chrono par
   worker, aucune affinité mesurée.
4. **Recherche de témoins par paire** sans hériter des témoins du rectangle
   (1,09 G visites sans sol) ; aucun certificat d'arête longue (covers jusqu'à
   52 % du nuage).
5. **Deux moteurs** : l'u16 est complet mais non élargissable (48 bits
   compactés, piles 16 bits) ; le float32 est exact et propre mais q3
   seulement, non liable, ×20 à ×70 plus lent par opération et sans
   parallélisme. Aucun n'est à moins de deux ordres de grandeur du contrat.
6. **Voie q2 absente du plan natif** et jamais mesurée sans sol, alors que la
   tour l'exige.
7. **Périmètre hors CTest** : tout ce qui porte la nouvelle priorité se compile
   par des lignes `g++` ad hoc dans les lanceurs, avec des options flottantes
   (`-ffp-contract=off -frounding-math`) absentes de CMake.
8. **Documentation** : `docs/VERROUS_ARCHITECTURE.md` (l. 415), `docs/PLAN_DE_REFONTE.md`
   (§ 9) et `README.md` (l. 879) citent encore le contrat 50k et « douze
   tranches » ; le cadre à annoncer est ambigu entre deux profils ; les séries
   « scan 100/200 » désignent deux jeux u16 selon la tranche (repère scan 0 ou
   repère propre).
9. **Licences et données** : Patchwork++ (BSD-2) redistribué dans `receipts/`
   sans mention dans la section Licences ; 111 Mo de dérivés directs des scans
   KITTI (CC BY-NC-SA) versionnés sous `receipts/`.

## Incident du jour

Une lentille de cet audit a lancé `ctest -N` dans quatre builds épinglés
(`build/v8_ground_patchwork_20260921`, `v8_q4_seed_cells_r2_20260921`,
`v8_q34_affine_20260921`, `v8_lidar_global_r2_20260921`) : un
`Testing/Temporary/LastTest.log` de 121 octets a été créé ou réécrit à 20:21–20:23
UTC. Aucun binaire, `CTestCostData` ni capture de reçu n'a changé. Pour le build
du pilote sans sol, le répertoire `Testing/` n'existait pas et faisait échouer les
deux lectures Release (« artifact closure mismatch ») : il a été supprimé et les
quatre lectures du README rendent à nouveau 0. Pour les trois autres builds, le
journal CTest brut d'origine est perdu (les preuves restent dans les reçus).
Règle : ne jamais lancer `ctest`, même `-N`, dans un build épinglé.

## Décisions proposées

**D1 — Un seul moteur vers les contrats : le moteur entier exact.** Les contrats
1 s et 100 ms sur 30–60k sites ne sont atteignables ni avec ×270 de travail
inutile ni avec des prédicats à 1 728 bits. Le moteur u16 est le seul générateur
complet, parallèle et mesurable ; c'est lui qu'on porte vers les contrats
(réduction du travail, granularité, filtres flottants certifiés, GPU). Le profil
de précision demandé (float32 original, grille 1 mm optionnelle) sera servi par
**un élargissement du moteur entier à 18 bits par coordonnée** (grille 1 mm sur
±131 m, bornes recalculées : puissance q3 < 2^116, comparaisons q4 < 2^104,
orientation < 2^121, échelle d'atlas à réduire), pas par le chemin 1 728 bits.
Les briques float32 natives restent le profil « sans perte » qualifié, hors
contrat temps, et les brouillons non suivis ne sont pas repris. Cette décision
touche la décision de précision de l'utilisateur : elle lui est soumise
(question Q2).

**D2 — Frontière du chronomètre.** L'entrée déclarée du régime prioritaire est le
nuage sans sol (masque figé, paramètres publiés) ; le chronomètre du contrat
couvre lecture du nuage, préparation, index, tour et sorties, transferts compris ;
la segmentation (21 à 30 ms mono) est publiée à part. À écrire dans le contrat
avant la première mesure de qualification.

**D3 — Régime de tests à trois paliers.** Par tranche : `ctest -L gate -LE slow`
(≈ 30 à 40 s) ; avant commit : suite complète (2 à 3 min) ; avant reçu : ASan/UBSan
et TSan, mutants compilés, relectures normal et `-O`. Tout le périmètre float32,
LiDAR, spatial et mutations entre dans CMake avec labels ; les défauts de la
bibliothèque deviennent la configuration mesurée (`rectangle-pair`, `boxes`,
`affine`, `live`), les anciens modes passant dans une cible `legacy` réservée aux
portes différentielles.

## Plan de développement

Phase 0 — mesurer (1 à 2 jours) :

- reçu de base sur les trois nuages sans sol u16 : K5 et K10, s8, W1 et W8,
  compteurs par poste, chronos internes, occupation CPU ; profil par symbole
  (gprof, build privé) sur K5 W1 ; réutiliser la campagne pour fixer la
  frontière D2 ;
- première mesure q2 sans sol (profil rapide, Coarse et Donate) sur les mêmes
  nuages, K5 et K10 ;
- enregistrement CMake du périmètre float32/LiDAR/spatial/mutations avec
  labels, options flottantes sur la cible float32, Boost conditionnel, cycle
  court documenté ; correction des trois passages documentaires périmés.

Phase 1 — réduire le travail sur le moteur entier (2 à 3 semaines, cible ×10 de
CPU·s en moins sans sol) :

- traversée d'index unique par arête (plages du cover, AABB des complétions,
  liste unique de graines aiguës possédées pour q3 et q4) et en-tête unique des
  prédicats d'arête ; sorties inchangées, requalification par la porte existante ;
- atlas de centres partagé q3/q4 à deux seuils : rejet sans census des graines
  q3 dont le centre tombe dans une cellule certifiée profonde pour K − 1 ; note
  mathématique et fixture d'égalité ;
- héritage des témoins certifiés du rectangle vers ses paires ; instrumentation
  par arête (taille du cover, graines, émissions) pour décider d'un certificat
  d'arête longue ;
- arène par arête pour l'atlas (plus d'objet par cellule), bornes de blocs à
  échelle réduite ;
- filtre flottant certifié à repli exact pour les bornes de census q3 et les
  bornes de blocs q4 (borne d'erreur prouvée sur entrées entières, mutant sur
  fixture de contact) : préalable à SIMD et GPU.

Phase 2 — occuper la machine (1 à 2 semaines) :

- file d'arêtes et tâches possédées (plages de paires d'un rectangle, blocs de
  graines d'une arête) dans une équipe persistante ; chrono travail/attente et
  CPU logique par worker ; sorties triées par clé canonique, porte W1/W2/W4/W8
  bit-identique sur nuage sans sol ; mutants `--inject` du chemin parallèle
  (job perdu, job dupliqué, fusion décalée) ;
- première session G4 CPU (W48, K5 et K10, trois scènes) quand le local a gagné
  ≥ ×5 sur la base : c'est la mesure qui situe le 1 s.

Phase 3 — aval de la tour, en parallèle des phases 1–2 (3 à 4 semaines) :

- clé canonique et arité minimale, déduplication (tri, RLE), intérieurs, voie q2
  dans le même appel, fold en forêts K = 1..10 porté explicitement de la v4
  (§ 9.1) sous contrat v8, digests au format v4 ; lanceur FULL chronométré de
  bout en bout (définition mesurable du « 1 s »).

Phase 4 — GPU (après les phases 1–2) :

- socle sans carte : format wire versionné champ par champ, index résident,
  enregistrements fixes, stub hôte et validateur transactionnel, témoin device
  des prédicats à arrondi dirigé (`-fmad=false`, repli exact sur l'hôte) ;
- premier étage : census q3 par boule sur index résident, puis bornes témoin
  arête × bloc et blocs × graines de l'atlas, en anneau de lots épinglés
  (contrat `lot_ring` de la v6, jamais mesuré) ; extension du protocole G4 v8
  (nvcc, `GPU_executed`, budget utile 2 400 à 3 000 s dans la VM de 3 600 s) ;
  transferts comptés, sorties bit-identiques au CPU ; c'est la voie du 100 ms.

Phase 5 — précision 1 mm par élargissement à 18 bits (après D1 validée, 1 à 2
semaines) : type de coordonnée paramétré, bornes recalculées et vérifiées par la
porte rationnelle, campagnes appariées 2 cm/1 mm sur les trois scènes.

## Questions à l'utilisateur

- **Q1 — Session concurrente.** L'ancien constructeur écrivait encore dans
  l'arbre partagé à 20:27 UTC (brouillons float32 non suivis, plus haut). Deux
  développeurs sur le même index Git sont incompatibles : confirmer l'arrêt de
  cette session, et le sort des brouillons (commit par leur auteur, ou retrait).
- **Q2 — Précision.** Accepter que le contrat temps soit poursuivi sur le moteur
  entier élargi à 18 bits (grille 1 mm), le profil float32 sans perte restant
  qualifié mais hors contrat temps (décision D1).
- **Q3 — Données et licences.** Sortir du versionnement les 111 Mo de dérivés
  KITTI (hashes et régénération depuis les `.bin` locaux) ou documenter
  l'exception non commerciale comme pour `HGP-old/` ; ajouter Patchwork++ (BSD-2)
  à la section Licences.

## Ce que cet audit n'a pas fait

Aucune qualification n'a été rejouée intégralement (612 appels Fraction, 1 636
cas, 229 clés : lus dans les reçus, non relancés) ; aucune mesure n'est
épinglée (la base de temps vient en phase 0) ; aucune séquence autre que 08 ;
aucune commande GCP.
