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
K5 W1 742,9 s, W8 273 s.

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
