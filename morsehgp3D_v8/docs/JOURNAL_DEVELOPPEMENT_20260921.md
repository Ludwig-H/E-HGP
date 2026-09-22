# Journal de développement v8 (reprise du 21 septembre 2026)

Cadre : `exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`. GCP non
utilisé. Régime prioritaire : LiDAR sans sol, 30 000 à 60 000 sites, contrats
1 s puis 100 ms, K5 et K10. Chaque tranche est commise avec ses portes ; les
temps cités sont mesurés sur un hôte partagé (une répétition) et orientent le
travail sans rien qualifier ; les reçus sont dans `receipts/`.

## Tranches closes

| tranche | commit | effet mesuré (quart sans sol x+y+, 7 067 sites, K5) | portes |
| --- | --- | --- | --- |
| Enregistrement CMake du périmètre float32/LiDAR/mutations, labels, Boost optionnel | 92d74c13 | cycle court `ctest -L gate -LE slow` 19 s (79 tests) | 132 tests, 121 verts + 2 DISABLED |
| Atlas q4 : cellules à l'échelle 2^20, bornes de blocs et de points en i64 | 748ec082 | W1 48,5 s → 39,9 s, sorties identiques | gates q4/q34, mutations tuées |
| Fragments : une allocation, frontière réservée | 02987f18 | neutre | idem |
| Graines q3 certifiées par l'atlas (`q3_atlas_consultation`, jeton `atlas`) | 0948d2d0 | 90,7 % des graines q3 rejetées sans census ; W1 → 27,3 s | note [Q3_CERTIFICAT_ATLAS](Q3_CERTIFICAT_ATLAS_20260921.md), 4 mutants |
| Partage de rectangles par plages (file bornée, tous les rectangles résiduels publiés) | (cette tranche) | W8 18–20 s → 7,2 s sous charge (592 % CPU), W4 15,1 s (283 %), paires par worker équilibrées ; sorties identiques W1/W4/W8 | identités du registre de tâches, plancher scissions/refus, 5e mutant (plage refusée non développée) |

Base de temps de référence (moteur avant ces tranches,
`receipts/ground_baseline_20260921`, hôte partagé) : scène 0 sans sol K5
W1 889,5 s, W8 298 s (1 283 CPU·s) ; K10 W8 911 s (3 877 CPU·s) ; scène 100
K5 W1 742,9 s, W8 273 s. Campagne appariée après les tranches
(`receipts/ground_phase1_20260921`, scène 0 rejouée au calme) : K5 W1
453 s (×1,96), K5 W8 108 s (×2,76, 686 % de CPU), K10 W8 323 s (×2,82) ;
scènes 100/200 : CPU divisé par 1,5 à 2,2 malgré un chevauchement de charge
déclaré dans les deux reçus.

## Partage de rectangles par plages (phase 2, première étape)

`run_wspd_q34_parallel` conserve le plan de jobs Coarse du front (sous-arbres
de produits) mais chaque worker publie désormais tout rectangle résiduel
survivant, entier ou découpé en plages de rangs de A (grain
`parallel_task_pairs`, 256 paires par défaut), dans une file bornée
(`parallel_queue_capacity`, 4 096) protégée par un mutex ; les workers
prennent les tâches pendantes avant tout nouveau job, et un publieur dont la
file est pleine développe la plage lui-même. Une arête n'est jamais scindée,
la recherche de témoins au niveau du rectangle est payée une fois par le
publieur, et chaque paire est développée exactement une fois (identité
`published = consumed`, masse d'entrée partitionnée, vérifiées à la fin).
Terminaison : aucune tâche pendante, aucun job libre, aucun worker occupé
(variable de condition). Sorties par worker, réductions SUM/MAX comme avant ;
un seul worker démarré ne partage rien (chemin historique).

## Pistes essayées et écartées (à ne pas rouvrir sans mesure nouvelle)

- **Cache des formes de points dans la frontière des fragments** : temps
  identique (27,32 s contre 27,32 s sur le quart, W1) ; les singletons
  retenus par un parent sont rarement re-testés par ses enfants, les tests de
  points naissent surtout des scissions de blocs. Retiré.
- **Classification conjointe des quatre cellules filles en une passe**
  (neuf coins partagés, formes évaluées une fois, résultat prouvé identique
  fragment par fragment par une porte d'égalité) : +18 % (31,7 s contre
  26,8 s, deux répétitions dos à dos sous la même charge). La marche
  séquentielle par `escape` est déjà serrée ; les surcoûts par quadrant
  (masques, tableaux de coins, quatre registres de compteurs) dépassent les
  évaluations économisées, la plupart des nœuds étant décidés pour un seul
  enfant à la fois. Retiré.

## Poste dominant restant

Profil gprof du quart après la consultation d'atlas : partition de l'atlas
q4 ≈ 52 % (bornes de blocs 21 %, constructeur de fragments 13 %, formes 11 %,
rétention 6 %), census q3 12 %, filtres de témoins 13 %, balayage 3 %.
Prochaines étapes : passe unique sur la frontière du parent pour les quatre
cellules filles (partage des évaluations de coins et des formes), arène de
fragments, puis filtre flottant certifié à repli exact pour les bornes ;
en parallèle, chronos par worker et campagne appariée sur les trois scènes
(K5/K10, W1/W8) avec le nouveau binaire.

## Prochain chantier : session G4 à 48 workers sur les nuages sans sol

Le protocole `gcp-migration/q34_spatial_{worker,session,snapshot,selftest}_v8.py`
est verrouillé sur la campagne spatiale du 21 septembre : inventaire de
24 unités, commande de sonde à 14 jetons (schéma v4), plan
`mhgp8_q34_spatial_plan_v1` sur des scènes préparées par
`prepare_lidar_spatial.py` (RAW.bin reconstruit sur la VM), et **autorité
native** = reçu local de qualification (216 sources hachées,
`q4_seed_cells_20260921/qualification_r2/smoke_ewedfs4y`) couplé aux
sources téléversées. Pour mesurer le nouveau moteur sur G4 il faut, dans
l'ordre et sans toucher aux aides pinnées par hash :

1. produire une nouvelle autorité locale (qualification fraîche des sources
   courantes par le lanceur de `q4_seed_cells`, 216 hashes) et épingler ses
   deux hashes dans le worker ;
2. étendre les validateurs de sonde (`run_wspd_q34_lidar.strict_shape`,
   `run_q4_seed_cells_checks.validate_row`, `run_q34_spatial`) au schéma v5
   (jeton `atlas`, registres `q3_atlas`, `tasks`, `workers_tasks`,
   `workers_timing_ms`) sans casser la lecture des reçus v4 ;
3. ajouter au worker une préparation « sans sol » relocalisable (RAW.bin +
   masque → sept morceaux u16 par la recette de
   `prepare_lidar_ground_u16.py`, fonction pure sur octets) et le jeton
   `atlas` dans `probe_command` / `validate_probe` ;
4. plan : trois scènes sans sol × K5 et K10 × W48 (budget utile ≤ 900 s : K10
   scène 200 ≈ 824 s à 8 workers locaux, donc K10 seulement si W48 tient),
   `GPU_executed = False`, arrêt TERMINATED certifié ;
5. selftests locaux sous faux gcloud (`q34_spatial_selftest_v8.py`,
   `tests/gcp/`) avant toute session payante ; `describe` de contrôle de la
   cible avant démarrage (incident conteneur/VM).


## Élargissement du moteur entier à 18 bits (22 septembre 2026)

Décision utilisateur du 22 septembre : poursuivre les contrats temps sur le
moteur entier élargi à **18 bits par coordonnée** (grille 1 mm sur ±131 m),
le float32 sans perte restant hors contrat et hors développement pour
l'instant. Conception, inventaire à quatre lentilles (415 dépendances de la
largeur 16 bits, sept qui cassent) et choix d'implémentation dans
[ELARGISSEMENT_18_BITS_20260922.md](ELARGISSEMENT_18_BITS_20260922.md).
Port livré dans ce commit :

- `Coordinate = std::int32_t`, `coordinate_bits = 18`, `coordinate_limit =
  262 143`, `max_index_depth = 54`, `index_stack_frames = 55`
  (`src/core/types.hpp`) ; refus explicite hors plage et clés d'unicité à
  trois champs de 18 bits dans `prepare_cloud` ; constantes q2 en `uint64_t`
  (les carrés 262 143² ne tiennent plus en u32) ; piles et réserves dérivées
  de la largeur (census q3, recherche de témoins, front, continuations q2) ;
  centre q3 localisé dans l'atlas par division longue exacte (le produit
  `scale·x` demandait 2^137 bits) ; carte des centres à Q = 2^42 ; toutes les
  bornes de preuve réécrites avec M = 262 143 ; lecteur `.u32le` (12 octets
  par site, valeurs < 2^18) et profil publié `quantized_u18_input_only`
  quand une coordonnée dépasse 16 bits ; validateurs Python dérivés des
  mêmes constantes (les égalités de pile acceptent 49 pour les reçus
  antérieurs et 55).
- Portes : 79 portes courtes vertes, suite complète 129 exécutées (trois
  désactivées comme avant) ; `mhgp8_q2_census_campaign_gate` mis à jour (le
  mutant « visites au-delà de (2·profondeur+1)·n » suit la largeur) ;
  `mhgp8_wspd_q34_mutations` a échoué une fois pendant la suite complète
  lancée en concurrence avec un second build et une sonde à huit workers,
  puis passe seul (22 s) ; à rejouer sur hôte calme avant tout reçu.
  Porte du nuage : acceptation à 262 143, refus à 262 144 et pour toute
  coordonnée négative, contre-fixture de collision de l'ancien empaquetage
  ((0,1,0) contre (0,0,65536)), doublons 18 bits refusés.
- Identité sur les entrées u16 : scène 0 sans sol, K5, huit workers, jeton
  `atlas` : sorties (xor, somme, comptes, IDs de coquille) et les 444
  compteurs logiques identiques à ceux du reçu
  [ground_phase1_20260921](../receipts/ground_phase1_20260921/README.md) ;
  seuls les octets retenus changent (entrée 238 890 → 477 780, nuage
  1,19 → 2,39 Mo, index 7,66 → 8,71 Mo).
- Première exécution 18 bits : préfixe de 3 000 sites du payload 1 mm de la
  scène 0 (`scene_00_grid/full.u32le`, 39 885 sites, maximum 158 607), K5,
  quatre workers : 49 584 q3 et 11 537 q4 émis, 83 % des graines q3 rejetées
  par l'atlas, 6,8 s. Diagnostic, pas un reçu.

Suite immédiate : fixtures jumelles à 262 143 dans toutes les portes qui
gravent 65 535 (les anciennes restent des oracles intérieurs), campagne
appariée d'identité u16 sur les six lignes W8 des trois scènes, puis première
campagne 1 mm (K5 et K10, huit workers) sous
`receipts/ground_18bits_20260922/`.
