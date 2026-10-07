# Audit final v11 : tour FULL et étage des forêts

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. J'ai lu en lecture seule l'instantané `ac081a06f`. J'ai aussi lu trois notes de travail qui ne sont pas dans le dépôt : `build/v11-persist/gpu_optim/carte_forets.md`, `.../conception/PISTES_DE_RUPTURE.md` et `.../audit_transpositions/AUDIT_TRANSPOSITIONS_V11.md`. GCP non utilisé, rien compilé, aucun fichier modifié.

Convention : **[F]** désigne un fait vérifié dans un reçu ou dans le code, **[I]** une inférence de ma part.

## 1. Périmètre et état

### Comment l'étage calcule la tour [F]

Sources : `src/tower`, `docs/MATHEMATIQUES.md` §§ 4–6.

**Entrée.** Un `FullDomain` immuable : index, catalogue trié par (niveau exact, support), table support → boule. Les descentes ne lisent jamais le DSU (union-find), seulement le domaine. La résolution peut donc être calculée d'avance (`forest_parallel.cpp`, l. 1).

**Cinq étapes par ordre k :**
- **A. Classification** de chaque boule dans la fenêtre p+q_min−1 ≤ k ≤ p+m.
  - Cellule régulière (m = q_min) : naissance à k = p+q, jonction de q faces à k = p+q−1.
  - Coquille étendue : `classify_cell`.
- **B. Naissances.** Sous-suite du catalogue en ordre de rang. Les cohortes de même rang sont triées par centre exact (`num::compare_centers`). S'y ajoutent une table dense clé → nœud et les états du DSU.
- **C. Résolution.** Chaque face I∪(U∖{u}) d'une cellule régulière donne une graine.
  - D'abord la table de populations « liée » : une sonde rend directement (nœud, rang).
  - Sinon `descend_each_step` : plus petite boule englobante (MEB) bornée, census sur l'index, support canonique.
  - Garde : la date **initiale** doit être strictement inférieure au niveau de la cellule.
- **D. Publication par plateau de rang.**
  - `find`, `touch`, puis `unite_roots` ; la racine est la plus petite naissance canonique.
  - `close` : une chaîne d'au moins deux anciennes composantes donne une seule multifusion N-aire, une seule composante donne une continuation.
- **E. Verticales.**
  - Image basse de chaque naissance haute. La graine régulière est réemployée : 857 771 descentes évitées sur 857 891 (ng00).
  - Remontée à la coupe **fermée** par `ClosedAncestorSweep` (DSU par taille, sommet géométrique tenu à part).
  - Contrôle que tous les enfants d'une fusion ont la même image.

**Parallélisme.**
- Le `Pool` est synchrone : une invocation à la fois, réclamation des tâches par CAS avec un grain de 1, sans vol de travail ni affinité.
- Voie `concurrent_orders` :
  - A par blocs ;
  - B en trois distributions, dont les cohortes en (K−1)×32 tranches ;
  - puis C, D et E dans **une seule** distribution « pipeline » de W tâches : L = W−(2K−1) résolveurs, K publieurs (un par ordre), K−1 suiveurs verticaux.
- Les résolveurs réclament des blocs de 256 cellules dans l'ordre global des boules. Les publieurs attendent bloc par bloc (époque + futex). Les suiveurs lisent `closed`, annoncé tous les 32 plateaux.
- Pas d'interblocage : une tâche n'attend que des tâches d'indice inférieur, déjà réclamées.
- Répartition à W48 : 39/5/4 tâches à K5, 29/10/9 à K10.
- Placement O1 (`forest_placement.cpp`) : P_K, P_{K−1}, V_K et V_{K−1} sur des cœurs physiques dédiés, **seulement pour K ≤ 5**.

**Garanties [F] :**
- **T4, plateau atomique.** Les multifusions ne sont jamais binarisées. Le registre racine classe le « traitement séquentiel de niveaux égaux » en `false_in_general` ; le mutant `forest_plateau_sequentiel` le garde.
- **T5, descente datée.** Date initiale, jamais terminale ; inégalité large pour les verticales.
- **Numérotation canonique** : naissances par (niveau, centre), fusions par (niveau, plus petite naissance). Les sorties sont donc identiques à l'octet quel que soit W.
- **Contrôles dans le produit**, sinon `tower_invariant` :
  - rang de la graine < rang de la cellule ;
  - racine unique, et arêtes + 1 = nœuds ;
  - garde contre une chaîne cyclique ;
  - naturalité des verticales.
- Tout tableau est admis au `MemoryBudget` avant allocation ; un refus est transactionnel.
- Portes : `pipeline_decisions` (4 000 flux), `pipeline_equivalence` (W48, ×6), TSan, 163 mutants `tower`, oracle Python Γ_k/Fraction sur petits nuages, différentiel v10 sur 14 petites fixtures × 3 profils.

**Limites [F].** Je n'ai trouvé aucun oracle indépendant de la forêt à l'échelle LiDAR (recherche par grep) : seulement l'identité entre voies et nombres de fils, plus les invariants. Le différentiel canonique v10/v11 sur trames entières reste ouvert.

### État final [F]

Session `claudeg1`, variante de base `733912e65`, médianes à froid de 6 processus :
- **K5**, voie CPU (masque 802811) : `forest_ms` 117,2 / 94,6 / 115,9 ms (ng00 / ng01 / ng02).
- **K10**, 3 processus : 1 294 / 958 / 1 069 ms.

Ni le contrat de 100 ms ni le jalon de 200 ms ne sont tenus : le mur K5 à chaud reste vers 250–335 ms.

## 2. Ce qui a marché

| Levier | Source | Preuve |
|---|---|---|
| Mémo de descente daté | `c2c3e0323` | FULL ×1,27–1,32 (`full_memo_20261003/memo1`) ; supplanté ensuite |
| Lots réguliers par lanes | `3dbfd1c32` | ng00 FULL 14,79 → 6,16 s ; forêt 2,78 s (`full_parallel_20261003/forest3`) |
| Census réutilisé et verticales parallèles | `cc93360a3` | forêt ng00 750 ms (`full_census_20261003/census2`) |
| Réemploi vertical régulier et table dense des naissances | `ae817d09e` | 857 771 / 857 891 descentes verticales évitées ; forêt ng00 658 ms (`reuse1`) |
| Mode 16379 : table de populations, ordres concurrents, sans mémo | `c40f40798` | forêts 240,8 / 163,6 / 197,2 ms ; présentations MEB −76 à −79 % ; remplace ~600 barrières et ~330 ms de pilote seul (`qualification_performance_20261003`, commentaire de `forest_concurrent.cpp`) |
| Tranche 3 : voie liée, naissances par blocs, pipeline (futex) | `b87285378` | forêts 209 → 171 / 133 / 158 ms ; résolution à W1 −26 à −30 % (`developpement_20261003/pipeline_g4`) |
| Chronos par tâche du pipeline (début, CPU, attente) | `e49ea4690`, `d5b1d0179` | ont montré que la queue venait du débit du publieur de l'ordre 5 |
| O1, placement | `9d10de213` → `12ce8f8f0` | moyenne géométrique (mg) 0,917, pire rapport 0,967 (`o1_placement`, claudeo1place3) |
| V3, census : bornes entières et arbre radix | `a841d8c4b` | mg forêts 0,925 ; tests de points ×0,26 ; instructions de résolution ×0,565 (`v3_census`) |
| Leviers de constante du pas de descente | `751686868` | mg 0,966 ; instructions ×0,906 (`constantes_pas_descente`) |
| O2, partie 1 : parents DSU denses, préchargement des parents et états des graines | `13a4a0a4c` | mg 0,904 ; queue derrière les résolveurs 20–27 → 10–22 ms (`o2_publieur`) |
| Tri des cohortes par tranches | `5734ca6e8` | naissances 6,8–11,6 → 4,0–5,3 ms (mg 0,529) ; forêts mg 0,942 (`cohortes_tranches`) |
| Profil échantillonné des publieurs (diagnostic) | `0ff64512a` | P5 : cellules ~65 ms (145 ns chacune), clôtures ~29 ms (66 ns), reste ~13 ms (`cache_blocs`) |

### Chronologie de l'étage des forêts (W48, ng00 / ng01 / ng02, ms) [F]

| Date | Source | Mesure | K5 | K10 |
|---|---|---|---|---|
| 2 oct. | `c6ca345e0` (full3) | forêts + verticales, un processus | 16 885 (ng00) | — |
| 3 oct. | `c2c3e0323` (memo1) | mode 3 → mode 7 | 15 308 / 11 191 / 12 782 → 11 285 / 7 919 / 9 209 | — |
| 3 oct. | `3dbfd1c32` / `ae817d09e` | modes 15 / 2047 | 2 776 / 658 (ng00) | — |
| 3 oct. | `c40f40798` | médiane de 3 prises | 240,8 / 163,6 / 197,2 | — |
| 3 oct. soir | `b87285378` | médiane de 5 prises | 171,0 / 133,0 / 157,7 | — |
| 5 oct. | `38b76701b` (claudefinmesure) | étage `tree`, à chaud | 164 / 139 / 151 | 1 623–1 655 (ng00) |
| 6 oct. | V3 (`b4665642b`) | à froid, base → V3 | 146,0 / 117,1 / 141,8 → 132,3 / 107,9 / 133,5 | 1 650 / 1 184 / 1 305 → 1 412 / 1 009 / 1 157 |
| 6 oct. | O2a (`13a4a0a4c`) | à froid | 110,7 / 102,3 / 123,6 | 1 346 / 954 / 1 082 |
| 7 oct. | base claudeg1 (`733912e65`) | à froid, état final | 117,2 / 94,6 / 115,9 | 1 294 / 958 / 1 069 |

D'une session à l'autre, la même base dérive de ±5–10 % : seuls les rapports appariés dans une même session décident.

## 3. Ce qui n'a pas marché ou a été retiré

| Essai | Cause et chiffres | Preuve |
|---|---|---|
| Mémos de lane | 2,6 % de succès ; incompatibles avec le pipeline (`pipeline_lanes` les refuse) ; éteints dans le mode 16379 | note d'audit du 3 oct. |
| Lots ordre par ordre (`ForestParallel::run`/`flush`) | ~600 barrières et ~330 ms de pilote seul | commentaire de `forest_concurrent.cpp` |
| Filtre F6 des signes de `power` | ≈ 1 % du CPU, retiré | `pipeline_g4`, claudeab1 |
| O1, premier essai | option perdue au déplacement de `ForestParallel` : A/A involontaire, bruit ±12 % à froid et ±9 % à chaud | claudeo1place |
| O1, deuxième essai | critère manqué : ng02 à chaud à 1,038 | claudeo1place2 |
| Annonces tous les 1024 plateaux | mg publieur 5 à 0,966 pour un seuil de 0,90 ; retiré (`06fdf8013`) | `annonces_publieurs` |
| Préchargement du nœud des graines | 0,865 pour un seuil de 0,85 ; retiré (`caca9d8d7`) | `prechargement_graines` |
| Préchargements combinés | 0,950 pour un seuil de 0,90 ; clôtures inchangées (31,9 → 31,3 ms) ; retiré (`4de17f141`) | `prechargements_publieurs` |
| Pages de 2 Mio (THP) | −17 % sur les forêts en local, mais mg du mur 1,064 sur G4 ; forêts à chaud, voie CPU : 100 → 126 ms | `thp_exploration` |
| Forêt parallèle (V4/O7), jamais construite | précédents CPU négatifs : Borůvka v9 à 1 264,7 ms (K5, ng00) ; découpage par blocs avec 31–45 % des fusions à la couture | `carte_forets.md`, `PISTES_DE_RUPTURE` R2.6 |

## 4. Pièges et leçons

- **Cohabitation [F].** Le publieur 5 coûte 35–46 ms à un fil, mais 81–151 ms de CPU dans le pipeline (avant O1/O2), et encore 95–99 ms après. Le balayage V5 passe de 28–40 ms à 68–88 ms. Les parts du SMT, du L3 et des réveils futex ne sont pas mesurées.
- **Les instructions ne prédisent pas le temps.** V3 retire 43 % des instructions de la résolution pour −7,5 % sur l'étage [F].
- **Le local ne prédit pas G4.** THP, et le filtre G1 : −36 % en local [F].
- **Le bruit domine les petits leviers [F].**
  - Le CPU d'un publieur varie de 72 à 105 ms d'un processus à l'autre.
  - Les règles écrites d'avance ont rejeté plusieurs gains réels de 5–14 %. Il faut des seuils à la taille d'effet réaliste, ou un protocole séquentiel.
- **Défaut de concurrence trouvé par l'auditeur [F].**
  - `ForestProgress::finish` publiait `closed = kNone` après `abandoned` ; au réveil, le contrôle d'abandon était sauté.
  - Corrigé dans `3bd4d734e`, avec la porte `mhgp11_tower_pipeline_abandon` et le mutant `pipeline_abandon_apres_reveil`.
  - Leçon : une sentinelle qui sert à la fois de fin et d'abandon doit être revérifiée après chaque attente.
- **Une option peut se perdre en route.** Le placement disparaissait au déplacement de `ForestParallel`. Un témoin par prise (`pipeline_placement_cores`) est désormais requis [F].
- **Le MST seul ne transporte pas les compteurs logiques [F]** (complément de l'auditeur, `audit_plan_gpu_20261006/mathematics`).
  - Témoin : le triangle (0,0), (3,0), (1,2).
  - Dans le MST, `plateaus`, `continuations` et `touched_components` diffèrent.
  - `ancestor_find_steps` dépend du chemin suivi dans le DSU.
- **Les plateaux sont presque des singletons [F]** : 438 011 clôtures pour 448 698 cellules à l'ordre 5 ; 1 133 328 pour 1 138 258 à l'ordre 10. Paralléliser avec une barrière par plateau est sans espoir.
- **Rigidité du Pool [F].** Une tâche par fil. À K10, les 19 publieurs et suiveurs attendent ~1 s sur 1,22 s ; seuls 29 fils résolvent.
- **Date des graines [F].** Il faut la date initiale, jamais la terminale. Témoins : X=(0,2,4,6) pour les descentes ; (3,0,0), (0,3,0), (2,2,0) pour les verticales.
- **Fixtures de couverture [F].** Les planchers de non-vacuité doivent être atteignables : l'échec de `vertical1` est venu de nuages de ≤ 5 sites qui ne pouvaient pas saturer. De même, un mutant qui ne compile pas n'est pas tué (`forest_cohort_nonbirth_reset`).

## 5. Dettes et problèmes ouverts

- Les 100 ms et les 200 ms restent ouverts. **Chemin critique K5 [F]** :
  - préambule de ~13–16 ms (contextes, dont la table de populations de 44 Mo reconstruite à chaque appel ; classification 2 ms ; naissances 4–5 ms) ;
  - puis max(R ≈ 63–83 ms, P5 ≈ 80–102 ms, V5 qui suit P5 de 0 à 6 ms).
- **K10 [F]** : résolveurs critiques, R = 1 214 / 894 / 988 ms, 25–35 CPU·s sur 29 voies. Placement inactif. Sur-souscription des fils libres (O4) jamais mesurée. Feuilles de 24 à K10 dans l'API : décision de l'utilisateur en attente.
- **Leviers jamais faits ou jamais portés :**
  - O2 partie 2, le flux compact par ordre : chaque publieur balaie encore les 1,31 M fiches de boules.
  - Mémo de cellule daté (V7) : la v10 en tirait −7 % de pas à K5 et −9 % à K10.
  - S11, le pipeline à un ordre pour `points` et `plat` : étage `tree` à 138–184 ms, pour une estimation de 65–90.
- **Avant tout port de forêt parallèle :**
  - au registre, « contraction des plateaux par composantes fortement connexes » est encore une `proof_obligation` ;
  - le lemme MST/contraction de l'auditeur n'y est pas inscrit ;
  - le contrat des compteurs (logique ou physique) n'est pas écrit.
- **Validation à l'échelle** : pas de juge indépendant de forêt ; différentiel v10/v11 sur trames entières ouvert.
- **Documentation** : `DEVELOPPEMENT.md` est figé au 3 octobre. Le reçu `pipeline_g4` titre « Qualification claudeab5 » alors que les résultats viennent de claudeab7. Les cartes O1–O8 sont hors dépôt (`build/v11-persist`), donc non versionnées.

## 6. Recommandations pour la v12

**Garder :**
- domaine immuable et descentes sans DSU ;
- table de populations liée, à produire par le catalogue plutôt que reconstruite ;
- T4, T5, numérotation canonique, contrôles de naturalité ;
- réemploi des graines régulières verticales, table dense, cohortes par tranches ;
- portes à fonction pure contre un modèle séquentiel, mutants (dont `forest_plateau_sequentiel`), règles A/A écrites d'avance.

**Repenser la construction des forêts comme un problème de masse, une fois les graines connues [I] :**
- **Graphe.** Par ordre k, les sommets sont les naissances (~341 k à l'ordre 5 sur ng00). Les arêtes sont les étoiles de graines de chaque cellule (~1,35 M graines), au rang de la cellule.
- **Forêt couvrante minimale parallèle et déterministe**, sous l'ordre total (rang, ordinal de cellule). Candidats :
  - Kruskal par réservations déterministes (Blelloch–Fineman–Gibbons–Shun, PPoPP 2012) ;
  - Borůvka canonique : le registre racine prouve déjà la réduction de moitié et l'inclusion dans Kruskal.
- **Dendrogramme parallèle** depuis la forêt couvrante (approche de Wang–Yu–Gu–Shun, SIGMOD 2021, à évaluer).
- **Contraction** des chaînes de fusions de même rang (lemme de l'auditeur), puis renumérotation canonique.
- **Compteurs** `plateaus`, `continuations` et `touched_components` : une passe parallèle séparée sur les cellules, ou bien déclarés physiques. Ce contrat s'écrit **avant** le port.
- **Microbanc hors moteur d'abord**, sur graines vidées. Seuils de la carte : ≤ 10 ms à K5, ≤ 50 ms à K10.

**Verticales [I] :** requêtes parallèles de plus haut ancêtre de rang ≤ λ sur forêts closes. `AncestorIndex` (pointeurs de saut de Myers, O(log N)) existe déjà. Contrôles de naturalité en parallèle. On supprime ainsi la chorégraphie futex des suiveurs.

**Si l'on garde le pipeline [I] :**
- flux compact par ordre ;
- chemin rapide de `close` pour le plateau à une seule cellule, cas presque universel ;
- mesurer les pas de `find` du publieur avant de passer à l'union par taille avec un champ « plus petite naissance » séparé ;
- placement conscient des CCD (trois L3 de 32 Mio).

**Ordonnanceur [I] :** graphe de tâches avec vol de travail, tâches bloquantes non épinglées à un fil, rôles et affinités dans `sched` plutôt que par `ScopedAffinity`. À K10, gain plafonné vers ×1,2 par les 24 cœurs physiques.

**K10, réduire le travail des descentes :**
- porter V7 comme certificat typé ;
- publier l'histogramme des pas par trace ;
- n'ouvrir les vagues GPU (O8) qu'avec un microbanc ≤ 150 ns par pas.

**Hygiène :**
- un seul préréglage moteur, la voie séquentielle comme seule référence différentielle ; retirer mémos de lane, voie par étages et lots ordre par ordre ;
- banc en processus résident, avec compteurs `perf` SMT/L3 ;
- un juge de forêt à l'échelle : ordre 1 contre le dendrogramme du MST euclidien, échantillons de parties, différentiel v10 entier.

## 7. Références clés

Chemins relatifs à `morsehgp3D_v11/`.

**Code :**
- `src/tower/forest.hpp`, `forest_internal.hpp`
- `src/tower/forest_build.cpp`, `forest_plateau.cpp`, `forest_concurrent.cpp`, `forest_pipeline.cpp`, `forest_parallel.{hpp,cpp}`
- `src/tower/forest_vertical.cpp`, `forest_ancestor_sweep.hpp`, `forest_placement.{hpp,cpp}`
- `src/tower/population_lookup.{hpp,cpp}`, `regular_vertical_seeds.hpp`, `ancestor_index.hpp`
- `src/sched/{sched.hpp,pool.cpp}`, `src/api/compute.cpp`

**Documents :**
- `docs/MATHEMATIQUES.md` §§ 4–6, `docs/FULL_FORESTS.md`, `docs/PERFORMANCE_FULL.md`
- `docs/FULL_PARALLEL.md`, `FULL_VERTICAL_PARALLEL.md`, `FULL_REGULAR_VERTICAL_REUSE.md`
- `docs/DESCENT_MEMO.md`, `CELLS_AND_LOCATE.md`, `FULL_DOMAIN.md`, `FULL_BIRTH_RUNS.md`

**Reçus :**
- `receipts/full_20261002`, `full_sweep_20261002`, `full_memo_20261003/memo1`
- `receipts/full_parallel_20261003/forest3`, `full_census_20261003/census2`, `full_regular_vertical_20261003/reuse1`
- `receipts/qualification_performance_20261003/review/analysis.md`, `developpement_20261003/pipeline_g4`
- `receipts/developpement_20261005/qualification_finale`
- `receipts/developpement_20261006/{o1_placement,v3_census,constantes_pas_descente,o2_publieur,n1_ab}`
- `receipts/developpement_20261007/{cohortes_tranches,annonces_publieurs,prechargement_graines,prechargements_publieurs,cache_blocs,thp_exploration,filtre_g1_avx2}`

**Audits :**
- `receipts/audit_plan_gpu_20261006/mathematics/REPORT.md`
- `receipts/audit_giant_20261004/tower_evidence/README.md`
- `receipts/audit_placement_activation_20261006`
- `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`

**Tests :** `tests/mutants/tower.json`, `tests/tower/forest_pipeline_test.cpp`.
