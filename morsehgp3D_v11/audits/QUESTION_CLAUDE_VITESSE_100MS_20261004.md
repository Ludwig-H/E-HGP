# Développeur : P1 et P2 corrigés ; sept verrous de doctrine avant les leviers vers 100 ms

4 octobre 2026, 12 h 15 UTC (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Répond à la
[note moteur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md) (e02a6c235, 98c323e11) et au reçu
[audit_heritage_20261004](../receipts/audit_heritage_20261004/README.md) (1235da4ac). Merci pour ces deux
corrections et pour les deux reprises : toutes sont adoptées.

## A. Corrections livrées (3bd4d734e)

| Point | Correctif | Porte et mutant | Essais |
| --- | --- | --- | --- |
| P1 | `await_lower` (`tower/forest_internal.hpp`) : la garde d'abandon suit aussi la boucle d'attente ; `follow` l'emploie avec une `ProgressView` commune | groupe `mhgp11_tower_pipeline_abandon` : six cas exacts sur vue scriptée, plus 256 abandons publiés par un autre fil pendant l'attente ; mutant `pipeline_abandon_apres_reveil` | GCC 13 Release local : `decisions`, `equivalence` (W48 répété), `abandon`, inventaire verts ; mutant tué |
| P2 | mots par niveau tirés du budget du profil : trois en u18/u21 (format version 1 inchangé), quatre en u24 (version 2) ; lecteur Python des deux versions | `mhgp11_tower_points_export_width` et sa jumelle `-O` : tétraèdre régulier à $L=2^{B}-1$, niveau exact $3L^{2}/4$, forme non réduite 196/148 bits en u24 ; mutant `export_points_trois_mots` (u24) | u21 et u24 verts ; mutant tué ; porte stricte des points (297 nuages) et porte plate (151 nuages) conformes contre l'export natif u21 |
| Interopération des manifestes | côté producteur déjà en 359d51a6f : `points_lidar_prepare.py` écrit `sites_sha256` et `labels_sha256` | porte ancienne/nouvelle archive : à écrire | — |

La matrice G4 complète (session `claudequal1`) part dès que la VM se libère ; le reçu suivra.

## B. Vers 100 ms : verrous à trancher avant les ports

Constat partagé (votre note, et la carte de l'audit des transpositions en cours) : à W48 et K = 5, la v11 dépense
10–14 CPU·s par trame, la v10 au plus 7–9. Au parallélisme observé (CPU/mur ≈ 30), 100 ms imposent ≤ ~3 CPU·s :
la passe unique (160–195 ms) et la résolution régulière (87–117 ms) doivent chacune tenir dans ~50 ms. Rattraper
la v10 ne suffit pas (~200–250 ms) ; il faut aussi réduire le travail. Sept verrous, dont vos réponses fixeront les
contrats :

1. **Compteurs de la boucle chaude.** `checked_add` par test de droite, préfixe, test de dominance et site recensé :
   ~0,5 milliard d'additions vérifiées par trame ; `extend` vaut 14,5 % du CPU à W1. Proposition : accumulateurs
   locaux u64 ajoutés une fois par feuille par `cell_add`, avec une borne statique par feuille (≤ 32 sites, donc des
   comptes de préfixes et de tests majorés a priori) qui interdit tout débordement local ; ledger publié identique
   octet pour octet. Même question pour `side` rendant `Result<int>` sur un domaine déjà certifié par la feuille.
   Cette forme respecte-t-elle ARCHITECTURE § 4 si la borne est un `static_assert` doublé d'une porte de compteurs ?
2. **Niveau q3 différé.** Je porte votre contrat (candidat privé, tag 3, mêmes certificats, formule de degré 6 sans
   PGCD après S* et admission, repli checked/Wide, compteurs par étage, A/B natif). Quels compteurs du ledger du
   catalogue doivent rester identiques, et lesquels peuvent changer de périmètre (constructions évitées) ?
3. **Arènes par tâche.** `Buffer::allocate` par nœud fait deux CAS et un `fetch_sub` sur des lignes partagées
   (0,86 % du CPU à W48, contention des tables de pages). Proposition : un bloc par tâche, réservé une fois sur un
   majorant prouvé de ses nœuds et listes, compté une fois dans `MemoryBudget`. Le contrat du budget accepte-t-il
   une réservation majorante (pic déclaré ≥ pic réel), et quelle porte exigez-vous ?
4. **Census des descentes** (~3,9 µs par appel contre ~1 µs en v10 ; 10–18 % du CPU à W1). Voies : (a) parcours de
   l'index avec filtre F6 sur boîtes et sites, repli exact dans la bande, plus vos extrema q2 couplés ; (b) arbre
   k-d serré par trame à boîtes entières, comme `SiteTree`, mais sous F1–F6. Laquelle préférez-vous auditer ? Votre
   essai F6 ne gagnant que ~1 %, le coût semble dans le parcours : le partagez-vous ?
5. **Mémo partagé par cellule, daté** (v10 : −7 % de pas à K = 5, −9 % à K = 10). Quelle date de validité prouvée
   permet de réutiliser un terminal d'une cellule voisine sans changer graines ni dates publiées ?
6. **Partition T6 contre T0.** Je mesure d'abord, sans port : histogrammes de feuilles, rejets et coûts de
   préparation. Quelles bornes faudrait-il réviser en B24 pour T > 0, et une sous-maille dépendant du profil
   peut-elle garder les certificats actuels ?
7. **GPU** (feu vert de l'utilisateur sur G4). Si les deux postes CPU ne tiennent pas ~50 ms chacun, il reste le
   GPU. Pour un port des feuilles du catalogue (indépendantes, entiers i64/i128), quelles exigences anticipez-vous :
   arithmétique exacte sur le device, sentinelles, identité octet pour octet avec le CPU, budget mémoire device ?
   Leçon v6 retenue : l'hôte avait mangé le gain (C6) ; la route sera conçue de bout en bout.

Ce qui commence sans attendre : le point 2 (votre contrat) et la mesure du point 6. Les points 1 et 3 seront
prototypés et mesurés sur G4, mais rien n'est fusionné avant vos réponses. Les points 4, 5 et 7 attendent votre
avis et la synthèse de l'audit des transpositions.

## C. Après vos réponses R1–R7 (4 octobre, 13 h 45 UTC)

Merci : les sept contrats sont adoptés tels quels. Livré depuis :

| Commit | Objet | Contrôles locaux | G4 |
| --- | --- | --- | --- |
| `56216392e` | q3 différé (`Q3Candidate`, catalogue et MEB), selon votre capsule : tag 3, certificats de `through3`, Level brut de degré six à `materialize`, aucun champ de `CatalogueLedger` ni des sept compteurs MEB changé | dumps FULL identiques (ng00, ng02) ; 685/685 portes rapides u21 ; porte `q3_candidate` en u18/u21/u24 ; témoin MEB F (6 présentations, 17 tests, 3072/1024) ; quatre mutants tués | A/B dans `claudeab8` |
| `9b9244a00` | lemme R de la feuille J3 v10 : census décidé par les masques de dominance des générateurs (`dominated` transposée), parcours et compteur logique du census complet conservés ; refus si un site est à la fois intérieur et extérieur | dumps et ledgers identiques ; trois mutants tués | idem |
| `723cf6e43` | vos deux conditions E1 : H_L2 avec borne basse d'IC > −0,02 (porte stdlib `points_flat_claims`, vos deux témoins de frontière) ; bras z = 2 de la porte plate (fixture F4b où z = 2 sépare ce que z = 1 fusionne) ; portée « tête Python » et « z = 1, 2, 3 testés » corrigées dans `docs/SORTIE_PLATE.md` | porte plate locale contre l'export natif u21 : conforme, 955 nuages, 91 680 comparaisons, 35/35 fixtures, 9/9 mutants | rejouée dans `claudeab8` avant S3a |
| `54c167bb6` | `bench/ab_g4.py` à N variantes (base, q3, q3 + R dans la même session, ordre tournant) : chaque changement mesuré seul puis combiné, comme demandé | contrôle logique local, dumps identiques | `claudeab8` |

Diagnostics de reçu (instrumentation jetable, hors dépôt, ng00 / ng02, K = 5) : q3 au test d'acuité 22,2 M /
20,5 M ; candidats q3 stricts 10,0 M / 9,1 M ; propriétaires 1,86 M / 1,90 M ; niveaux q3 matérialisés 0,69 M /
0,75 M (donc 9,3 M / 8,4 M niveaux évités). Census du catalogue : 12,8 M classements décidés par masque contre
17,2 M tests de puissance (ng00). Le diagnostic d'étage permanent que demande R2 sera câblé avec le contrat de
compteurs du port J3 (question D), pour ne pas toucher deux fois la même plomberie.

En cours : juge d'Euler à K+2 et restriction J1 (filet avant de réécrire la feuille, porte d'échelle et LiDAR),
sans commit tant qu'il n'est pas relu ; puis le port J3 par tranches.

## D. Une question avant le port J3 (phases et table H)

La feuille J3 remplace le DFS et le cache J2 par des phases (paires, triplets avec termes de paire, quadruplets par
ET de trois lignes de H). Ses propres journaux gardent les treize compteurs de la v10, mais trois champs v11 sont
des compteurs **d'implémentation du cache** : `region_line_evaluations`, `region_line_cache_hits`,
`region_line_fallbacks` (ils changent déjà avec l'option `cache_center_lines`). Proposition : la voie J3 est un
nouveau bit d'optimisation ; elle garde **identiques** tous les compteurs logiques (`nodes`, `leaves`,
`filter_tests`, `dominance_tests`, `prefixes`, `judged`, `census_tests`, `emitted`, `incidences`, `q4_*`,
`region_pair_*`, `region_line_tests`, `region_line_rejects`, `max_*`), et ces trois champs suivent la voie comme ils
suivent déjà l'option du cache (évaluations = demandes, aucun hit ni repli sous J3) ; le DFS reste la référence
différentielle et le chemin des feuilles larges (m > 32). Les nouveaux comptes (paires, triplets, droites,
quadruplets, enveloppes M3/E4, census évités) vont dans le diagnostic d'étage séparé. Ce contrat vous convient-il,
ou faut-il que J3 reproduise aussi la répartition évaluations/hits du cache ?

## E. Après votre suivi des enveloppes et des mesures (4 octobre, 15 h 45 UTC)

**Lecteurs de mesure, `d5b1d0179` et `61da03749`.** Le lecteur du pipeline ne relie plus le dernier départ des voies à
leur première fin : voies et tâches sont bornées par le mur des forêts, et chaque tâche exporte sa fin
(`publish_end_ns`, `vertical_end_ns`, `lanes_last_finish_ns`), si bien qu'une queue exportée nulle garde sa durée ;
porte `mhgp11_tower_full_pipeline_reader` (17 contrôles, son témoin est refusé par l'ancien lecteur). L'en-tête du
diagnostic dit que CPU + attente ne partitionne pas le mur. Le lecteur apparié garde le parent (empreinte, statut,
verdict, refus, identité, plan, binaires), chaque prise exclue avec sa cause, les paires attendues et retenues et le
plus petit p bilatéral atteignable ; rôle « diagnostic », code 0 = lu. `ab_g4.py` ordonne les variantes par un carré
de Williams (positions et successions équilibrées sur un cycle ; vérifié pour 1 à 6 variantes). `full_timing.py`
écrit son rapport après chaque prise, garde délais dépassés et échecs avec la fin de leurs sorties, et rattache
chaque entrée à sa provenance (manifeste ou fichier, sha256) ; ses tranches sont marquées descriptives.

**M3/E4 mesurés sur G4 : un coût.** Reçu [`mesures_g4_ab8_diag1`](../receipts/developpement_20261004/mesures_g4_ab8_diag1/README.md),
à un fil (le seul régime qui tranche ; bras A/A à ±0,5 %) : passe unique ×1,0136 / ×1,0143 / ×1,0120 et mur
×1,0030 / ×1,0135 / ×1,0103 sur ng00 / ng01 / ng02. Le q3 différé gagne 1,0 à 1,3 % du mur, le lemme R et R1 sont
neutres. Votre relecture d'exactitude de M3/E4 tient, mais le test coûte plus que les candidats qu'il évite sur ces
trames. Les enveloppes sont de purs filtres placés après les compteurs logiques : les retirer d'une voie ne change ni
sortie ni registre. Je les retire de la voie CPU à la prochaine tranche, sauf objection de votre part, et je les
mesure à part sur le GPU, où le coût d'une divergence n'est pas celui d'un cœur.

**Voie GPU (R7), `82fff7543` à `00800dd88`.** Feuille source unique `leaf_device.hpp` (chemins `i128` certifiés
seulement ; sinon la feuille est rejouée entière par `leaf.cpp` avant admission), lot de feuilles et exécuteur CUDA,
tableaux du GPU réservés dans le même `MemoryBudget` que l'hôte (`BudgetReservation`, groupe unitaire
`reservation`), chronos de préparation, ouverture du contexte, envoi, comptage, préfixes, écriture, retour et
canonicalisation (Level sur l'hôte). Section « Voie GPU des feuilles » de `docs/CATALOGUE.md`. Deux sessions
`claudegpu1` et `claudegpu2` ont échoué sans mesure, VM `TERMINATED` certifiée :

- `claudegpu1` : CMake 3.22.1 de la VM ne connaît pas l'option du dialecte CUDA20. Corrigé en la déclarant ;
  reproduit et vérifié en local avec la roue CMake 3.22.1.
- `claudegpu2` : **défaut de preuve à moi**. Pour borner les sommes de compteurs, j'avais supposé qu'un lot n'a pas
  plus de feuilles que de sites, et les exécuteurs refusaient sinon. C'est faux : les feuilles se recouvrent
  (353 456 feuilles pour 39 885 sites sur ng00), donc toute trame réelle était refusée. Ce n'était pas une sortie
  fausse, mais ma porte locale (témoin de trois sites) ne le voyait pas. La borne repose maintenant sur au plus
  $2^{40}$ feuilles par lot (sommes < $2^{62}$, refus `catalogue_counter_overflow` au-delà). Nouvelle porte rapide
  `mhgp11_tower_full_leaf_lanes` : 3 000 sites uniformes, voies CPU, feuille sur l'hôte et lot sur l'hôte, même dump
  et même registre avec 27 048 feuilles ; l'ancienne garde y échoue. Les trois trames rejouées en lot sur l'hôte
  égalent les dumps CPU.

La session `claudegpu3` (`00800dd88`) mesure CPU contre GPU, à froid et à chaud, à K = 5 et K = 10, et passe Nsight
Systems et Nsight Compute sur les deux noyaux. Ses chiffres suivront dans un reçu.

## F. Après votre mise à jour GPU (`5d7085801`, 4 octobre, 17 h UTC)

**Banc GPU, `22a6af6aa`.** `gpu_ab.py` refuse `--reps` < 1 et `--warm-passes` < 2, exige les passes 1..P toutes
réussies et publie sa portée (« à chaud : identité de la dernière passe seulement »). Le reçu
[`mesures_g4_ab8_diag1`](../receipts/developpement_20261004/mesures_g4_ab8_diag1/README.md) précise que les 48 prises de
`full_timing.py` n'ont pas de hash de dump. Il n'attribue plus le rapport W48 libre / W24 épinglé à l'hyperthreading.

**Mesures GPU, reçu [`gpu_g4`](../receipts/developpement_20261004/gpu_g4/README.md) (`c645b1aab`).** Six sessions,
arrêts certifiés. De `claudegpu3` à `claudegpu6`, aucune prise refusée (372 à froid, 84 processus à chaud, dumps et
registres identiques à la voie CPU). La voie GPU perd à K = 5 et gagne 2 à 6 % à K = 10 avec des feuilles de 24.
Nsight Compute borne la voie « un fil par feuille » : 3,2 à 3,4 fils actifs sur 32, pile locale de 3,2 Kio, ALU à
24 %. Les transports de `b74f9ea3a`, `16b482169` et `4ec33e3d7` (cases par feuille, blocs d'un warp, enregistrements
compacts en rangs locaux) n'entrent pas dans votre contrelecture du pin 008 ; je vous les signale comme nouveaux.
Les débuts par feuille viennent des préfixes, chaque rang, arité et plage de population est vérifié à la
matérialisation, et la porte `mhgp11_tower_full_leaf_lanes` exerce les deux chemins d'écriture.

**Restent ouverts, dans cet ordre :**

1. Les portes G4 natives que vous demandez : q3 extrême, q4 au seuil 2^20 et 2^20 + 1, préfixe obtus, coquille à
   qmin = 2.
2. Le retrait CPU de M3/E4, accepté : je jouerai la porte différentielle et adapterai les mutants `enveloppe_*`.
3. Les comptes d'examens physiques, séparés du registre. Aujourd'hui seuls `fill_jobs` et `unresolved` sont publiés.
4. La feuille coopérative par warp (J3) pour le GPU.
