# Fork A — lignée v2, v3, v4 (et contexte « v1 » produit)

Lecture seule, 7 octobre 2026. GCP non utilisé. [I] = inférence ; le reste est vérifié dans le dépôt
(chemin:ligne, SHA, dates `git log`).

## 0. Avant la v2 : le « v1 » produit (contexte)

- `HGP-old/` : 7–29 juillet 2026 (13 commits, `eba0e123a`→`47d52c61e`), Python/Cython historique.
- `morsehgp3d/` (« v1 » dans les textes v2/v3) : 14 juillet → 9 août 2026 (434 commits, dernier `95dd8036a`).
  Registre `docs/implementation_status.toml` figé depuis ce commit : `current_phase=15`, phase 15
  `in_progress / reference_cpu / hgp_reduced / budgeted / not_claimed`, jalons `v1_correctness` et
  `v1_interactive_scalable` **blocked** (vérifié par lecture TOML).
- Contrat d'origine (phase 14) : n = 50 000, K_max = 10, **p95 `warm_e2e` < 100 ms** principal, < 1 s
  secondaire, pic < 80 % VRAM (`docs/research/CONTRAT_50K_BILAN.md` l. 9–17, 7 août). Même document : la
  création du contexte CUDA coûte **1 242 ms à froid contre 18 ms à chaud**, « les deux contrats n'existent que
  dans un service résident » (l. 24–28) ; étage paire device K=10 : 2 979 ms (l. 39). [I] La question « froid ou
  résident » posée le 7 août est encore la décision n° 1 demandée par la passation v11 (§ 6.1).
- **E5 est antérieure à toute la lignée** : le registre classe Prop. 6 et Th. 5 du manuscrit `false_in_general`
  dès le 14 juillet (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` l. 40–41, blame `5351704c1`/`068084a7a`),
  fixture `gabriel-point-set-counterexample-5-points-v1` (A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1),
  E=(4,1,2), `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` § 4). Le v1 a dû « se rabattre sur
  `gamma_exhaustive_reference` » (`morsehgp3D_v2/DESIGN.md` § 1, point 2).

## 1. v2 — 8 août 2026 (un seul jour)

- **Dates** : 6 commits, `9f4f96784` (18:56) → `9628038b5` (22:54) ; code versé d'un bloc en `092405e43`
  (20:52). v3 proposée à 21:22 le même soir (`ff519be74`).
- **Objet** : le *merge tree* de d_K (distance au K-ième voisin) : sphères critiques de rang fermé K (minima)
  et K+1 (indice 1, multiplicité |U|−1), via Reani–Bobrowski ; « il n'y a jamais de facette de cardinal K »
  (`morsehgp3D_v2/README.md` l. 12–16, `DESIGN.md` § 0 et § 2). **C'est l'objet que la v7 appellera FULL**
  (minima Gabriel de cardinal K + multifusions K+1) [I, par comparaison des définitions].
- **Architecture** : catalogue local par point dans le dual inversif, borne directionnelle (42 cônes),
  forêt multifurquée par ordre à lots contractés ; pas de verticales (obligation V.1), pas de
  `coverage_log` (C.1), coquilles > 4 points rejetées (`README.md` tableau « État »).
- **Précision** : i128 + `BigInt<N>` ; grille 16 bits, bornes 85/69/170/137/307 bits (`0c744c526`).
- **Mesures** (codespace 2 vCPU, `RESULTATS.md`) : n=200 K=10 → 258 739 800 = 200·C(199,3) quadruplets,
  26,3 s ; après retrait d'un élagage faux, **92,54 s** à n=200 (§ 3 bis) ; n=500 K=2 > 300 s
  (`ETAT_EN_COURS.md` § 3) ; ≈ 230–280 sphères critiques par point à K=10 (§ 2). Exposant ≈ 3, voisinage =
  nuage entier.
- **Portes** : P0 (catalogue = énumération exhaustive), P2 (496 cas, 0 désaccord), O2 (1 462 nuages,
  89 247 cas, 0 désaccord), régressions R1–R8 (`ETAT_EN_COURS.md` § 1).
- **Défauts trouvés** : élagage par cliques de faces perdant 0,63 % du catalogue (291/46 243, invisible à tout
  accord moyen) ; lecteur de niveau en `double` (R8) ; cosphères jetées en silence (R7) ; chaîne binaire au lieu
  d'une multifusion (R6, invisible à un oracle qui ne compte que des composantes).
- **Clôture** : « v2 does not have a wrong design. It has a brute-force stand-in placed where its design was
  supposed to be […] the v2 generator is condemned rather than slow » (`ff519be74`, `0c744c526`). Cause
  mathématique (`b113edb47`) : τ(p) infini pour les points de profondeur de Tukey ≤ K ; à 50 k, K=10, ≥ 615
  points non certifiables imposent 1,28·10¹⁶ quadruplets ; il faut ancrer sur une **arête** (D ≤ 2R deux fois).

## 2. v3 — 8 août 21:22 → 16 août 16:50 (412 commits)

- **Cadre** : `exploration_v3_hors_registre / cpu_reference_bounded_oracles_and_g4_diagnostic /
  quantized_u16_input_only / not_claimed` (`morsehgp3D_v3/README.md` l. 7–15).
- **Contrat visé** : 50 000 points, K_max=10, p95 `warm_e2e` < 1 s secondaire, **100 ms principal**, sur un
  G4, chrono de bout en bout (entrée → dix forêts, verticales, payload) (`PROPOSITION.md` § 1, l. 19–24).
- **Interdits** posés : aucune structure de Delaunay de quelque ordre que ce soit (Q14 « fermée
  contractuellement », `audits/AUDIT_ETAT_COURANT.md` ~l. 68), ni arrangement global, ni matrice de cofaces,
  ni catalogue résident, ni tableau indexé par paires/triplets/quadruplets (`PROPOSITION.md` l. 26–32).
- **Architecture** : germination sur l'arête diamétrale ; partition WSPD de Callahan–Kosaraju
  (`NeutralPairPartition`, ledger de masse exact) ; trois lanes autonomes q2/q3/q4 ; seuils
  h_q = s_max − q + 1 ; certificats de blocs (Corner8/64/512, SOC64, JungDiskDepth, BlockJungDual,
  HCBlockDepth, LP projectif, cages) ; `Q4SeedAxisTopR4` (≤ 16 apex par graine) ; `BallKey` à cinq coefficients
  normalisés avant census ; statuts transactionnels typés (`PROPOSITION.md` § 2–7). Juge **Γ_k exhaustif**
  (`oracle/gamma_forest_judge.cpp` l. 1–31 : partitions par niveau, coupes ouverte et fermée), bigint propre,
  témoins `__int128`/GMP. Gate D : une attache par facette cœur (`audits/NOTE_GATE_D_*`, 9 août).
- **Jamais livré** : BallKey → census → fold → payload complet à l'échelle (README l. 17–24 : « aucun échantillon
  ne qualifie le payload complet »).
- **Mesures G4 (CPU seul, 11–15 août)** : chaîne du 13 août (`receipts/chaine_complete_g4_20260813/README.md`
  § 1) : uniform 6 250/12 500/25 000/50 000 → 7,0/16,2/37,3/**78,8 s** (4 processus concurrents × 12 fils) ;
  eight_clusters, scanline, terrain murent en n^2,2 à n^3,2 ; 21 413 140 supports à uniform 50 k (≈ 428/pt,
  sortie en n^1,05) ; NO-GO de l'auditeur. Seul GPU : balayage axial plat CUDA
  (`receipts/axis_cuda_g4_20260815/RESULTATS.md` l. 24–64) : 0 écart sur 14 787 889 verdicts, ×180–284
  contre un cœur, **×4–6 seulement contre 48 cœurs** ; « 100 ms K=5 hors d'atteinte sans la descente, 4,6× de
  trop ; K=10 ≈ 750 ms pour ce seul étage ».
- **Tests** : ~816 enregistrements CMake (423 `mhgp3v_add_expected_code_test_for` + 393 `add_test`, grep) ;
  CLAUDE.md disait 832 (`f110407ba`, 15 août) puis 850.
- **16 août, décisions normatives** (`46f6beca9`) : tailles d'intérêt **8 000/16 000/32 000** et **jamais de
  vérification exhaustive** inscrites dans `docs/TEST_PLAN_MORSEHGP3D.md` § 3.1–3.2 et CLAUDE.md. Le même commit
  constate que q2 PairFrame est quadratique (530 752 → 8 414 464 états, ×3,99 par doublement) à cause d'un cap de
  masse |A||B| ≤ cap ; WSPD uniforme s=8 : 483 → 794,5 rectangles/point de 8 k à 64 k, exposant décroissant
  1,31/1,22/1,18 (`8985c829c`) ; 128 k interdit par le profil u16 (tie-break Morton sur 16 bits).
- **Clôture** : v4 ouverte le lendemain, « reprise de zéro […] fondée sur la lecture intégrale des Parties I–II
  et sur le post-mortem WSPD de la v3 » (`f775c9881`). Aucune décision utilisateur datée trouvée dans
  AGENTS/CLAUDE pour ce passage [I : décision de session].

### 2.1 Registre des pistes fermées v3 (`morsehgp3D_v3/audits/PISTES_FERMEES.md`, élagué le 15 août)

| l. | Piste | Cause | Survit |
| --- | --- | --- | --- |
| 16 | front de Jung coalescé par dual-tree d'ancres | 4,85 → 23,84 M visites q3 (n 500→1000), pentes 2,30/2,33 > 1,35 | front coalescé à 141,18 n |
| 17 | banque directionnelle Yao-48 | chambre 54,74° > 35,26°/31,13° exigés ; q4 = anneau | 3D_i² < D_j² (i64) |
| 18 | génération locale exacte par cône | faux vert (681/795/174 vs 681/884/202) et plus lent | contre-fixture extra-shell |
| 19 | cône cible par endpoint via k-NN | aucune pente ≤ 1,35 ; 39,2 M tests à n=2000 | noyau ponctuel H>0, 4H²>E₂X₂ / 3H²>E₂X₂, porte ALL 8 coins |
| 25 | Source S par listes de cellules de centres | snapshot non transférable | lemme profondeur–cellule β ≤ R_p(C) |
| 26 | juge rationnel des cellules | porte vacueuse (WILL_FAIL convertit le refus en vert) | positivité barycentrique stricte |
| 27 | sentinelle top-(12−q) | ne réduit rien, casse la télémétrie (code 0) | théorème de la sentinelle |
| 28 | pentes vertes uniform comme propriété | binaire non gelé, mesures sous charge | terrain croît en n^1,5 |
| 34 | coupure de lentille aiguë | carrier aigu sur 300/300 paires (eight_clusters 50 k) | théorème de face adjacente aiguë |
| 35 | couple de carriers ⇒ ancre diamétrale | fixture 5 points (144 > 100) | correction ‖x−y‖² ≤ D² |
| 41 | ledger des causes de lifts | 130 033 occurrences sans attribution | rejets owner 96,1/91,7/92,1 % |
| 42 | histogramme `SupportKey` | stade maximal, pas propriétés orthogonales | fermeture à écart nul |
| 43 | prune Yao48 P1a du produit | dist² ≥ 3D faux comme preuve (témoin sur la coquille) | deux fixtures q2 |
| 44 | déduplication `SupportKey` avant géométrie | 39,24 géométries/support, 81,6 % de rejets owner | minimum auto-centré (« q3 par droite ») |
| 45 | Helly sur le disque de Jung | ponctuel ; F_k ≈ 180 bits hors i128 | sous-certificat ≤ 3 |
| 46 | cœur universel de Jung | ne borne ni ancres ni coût | relaxations 3‖U‖² < D², 15‖U‖² ≤ 4D² (lanes q3/q4) |
| 47 | gate à trois voies | univers et tailles incomparables | récursion A×A, porte paires = C(n,2) |
| 53 | **K-graphe de Gabriel brut** | **fixture E5** : deux non-Gabriel rattachent une facette, deux composantes jusqu'à 24 | étoile silencieuse ≤ k−1 attaches |
| 54 | « directes + gateways » | pivot faux sur carré cosphérique | clé β = N/(4D) et bornes u16 |
| 55 | borne de degré q2 (cap 12) | treize partenaires dans une chambre | feu vert au filtre flottant certifié |
| 56 | front inverse par transitions | graphe non connexe | quatre contre-fixtures dont `plateau_carre_multifusion` |
| 57 | fenêtre top-M par ancre | 1 277 supports jamais proposés | fixture δ² = 100 = 4R² (inégalité stricte) |
| 58 | arrangement relevé (BFS/GPU) | faux hors position simple ; 1 270 sommets/pt pour 300 sphères | `order_k_flats.hpp` |
| 77–100 | boule d'apex unique pour h_a | plus lente, perd 11 points de fermeture q4, P0 de signe | théorème du cône + garde 3W>N² / 2W>N² ; dual-tree range-add **reste active** |

Deux motifs (l. 60–69) : la **porte vacueuse** et le **certificat qui coûte plus qu'il ne rapporte**
(SOC64, BlockJungDual, HCBlockDepth). Réouverture (l. 71–75) : nouveau théorème + fixture + porte de coût.

## 3. v4 — 17 août 07:57 → 27 août (161 commits)

- **Dates** : `f775c9881` (ouverture) → `2bf2dc5a7` (dernier commit fonctionnel, 27 août) ; développement sur la
  branche de session `claude/morsehgp3d-v4-reprise-nck0nk` fusionnée dans `main` (`d4f3ce59b`).
- **Objet déclaré** : « l'objet est le K-MST du K-graphe de Gabriel-miniboule » (`f775c9881`) ;
  `docs/MATHEMATIQUES.md` § 1.2 (l. 80–91) : « la forêt HGP = ce K-MST par K », statut `theoreme_manuscrit` ;
  § 5.1 (l. 504) : « La forêt HGP = le K-MST élagué (Théorème 5) ». Dix forêts horizontales K=1..10 avec
  `ComponentDelta` (naissance/croissance/multifusion, § 5.2bis), macro-lots par égalité sémantique U320.
  **Sans verticales** (audit `bab37b9`, tableau de promotion : « tour HGP complète — non reçue »).
- **Architecture** : un seul arbre radix de Karras sur positions uniques (Morton 48 bits = clé de tri), WSPD par
  vagues à descente ternaire qui tue les ancres, trois lanes q2/q3/q4 dans `src/pipeline/ball_stream.hpp`,
  fuseaux W_q, élagage h_cœur/h_a/h_b (h_q = s_max − q + 1 ⇒ 10/9/8 à K=10), tri/RLE par `BallKey`, census
  I_B/U_B, quotient des plateaux par énumération (`sphere_plateau.hpp`, UB à |U|=32), fold compact
  (`build_forest_legacy` figé comme témoin), rendu § 9.1. Tout le pipeline aval et les portes vivent dans
  `bench/forest_probe.cpp` (monolithe, ~4 500 l.).
- **Précision** : u16, `PointId` u32 arbitraire, i64/i128/U192/U320, flottant seulement en filtre certifié à
  repli exact (garde `FE_TONEAREST`/`__FAST_MATH__`).
- **Contrat local** (`README.md` l. 17–20) : K=1..10 **< 100 ms sur G4**, secondaire K=5 < 1 s, « dizaines de
  millions de points ». [I] Première apparition du repli K=5 dans la lignée.
- **Mesures** (conteneur 4 vCPU, séquentiel ; `receipts/campagne_locale_n8000_20260817/RECU.md`) : n=8000 :
  uniform K=10 343 s (t_gen 240 s, t_fold 54,6 s), K=5 54 s (t_gen 38,7 s) ; terrain K=10 162 s, K=5 54 s ;
  scanline 697/153 s ; **eight_clusters K=10 : timeout à 5 403 s**, K=5 1 427 s. Puis : Jung −3 % de t_gen
  (`PASSATION.md` § 2.9) ; descente WSPD parallèle **72,8 → 35,3 s** (§ 2.12, descente = 52,5 s / 72 % du mur
  alors que le profil désignait le scan q3, 2,6 %) ; t_fold 56,1 → 38,3 s (§ 2.5). ≈ 391 événements/point
  (audit § 5). **Aucune exécution G4** : six tentatives le 18 août, zéro campagne (port 22 bloqué, `gcloud`
  absent ; `PASSATION.md` § 6).
- **Tests** : 147/147 Release (63 s) et ASan/UBSan (1 193 s) au 22 août (`audits/ETAT_COURANT.md` l. 54–66) ;
  153 lignes `mhgp4_expect_code`, 74 mutants `--inject` distincts (grep `CMakeLists.txt`) ; codes 0/1/2/3/4
  via `cmake/run_expect.cmake` (signal refusé) ; `--relabel-gate`, `--par-gate`, `--workers-gate`, planchers
  `--min-*` ; familles v3 portées bit à bit (`src/cloud/families.hpp` l. 3–8).
- **Audit de clôture** (`bab37b9`, 22 août, `audits/ETAT_COURANT.md`) : « référence CPU exacte et crédible sur
  un domaine borné », pas un produit ; deux blocages B0 : plafond « pic » qui oublie les résultats terminés,
  et trace exhaustive incompatible avec le SLO (≥ **158,4 Go** de seuls `PointId` q2 à 30 M points, § 5) ;
  « API producteur pour `morsehgp3d` : absente, aucun adaptateur vers `CertifiedTowerInput` ». Recommandation :
  **garder v4 comme oracle**, décider l'objet produit (`full_symbolic_stream` / `connectivity_index` /
  `warm_query_or_labels`), fermer la tour (verticales, naturalité), adapter à `CertifiedTowerInput`
  (§ 9, Priorités 0–5). La réponse a été d'ouvrir la v5 le 27 août (`8600c53b9`, « reprise à propre de la
  v4 — même objet ») [I : la recommandation « oracle, pas reconstruction » n'a pas été suivie].

### 3.1 Pistes fermées v4 (consignées par la v5, `morsehgp3D_v5/docs/PISTES_FERMEES.md` l. 40–82)

Sélection axiale bornée (opt-in négatif, +7 % de t_gen) ; `build_forest_legacy` comme témoin (partage les
hypothèses du sujet) ; index par couches convexes pour q3 (10,3 sites/seed) ; plafond « pic projeté » faux deux
fois (×5,77 puis résultats terminés) ; cover q4 au coefficient 4 (change `digest_balls`) ; étage i64 du préfiltre
q4 comme gain (médiane appariée 1,0021, 8/20) ; refuser les coquilles (change les composantes : carré
cocyclique) ; deltas de fusion seuls (K-polyèdres faux) ; `first_batch` comme entrée ; comparer des constantes
entre processus (±40 %). Plus : le ×1,042 du préfiltre q4 retiré (bras témoin « calculé puis jeté »,
`PASSATION.md` § 2.13). Huit motifs d'erreur récurrents (l. 84–107) : promettre avant de mesurer, deux bornes
qui divergent, cap dans un critère terminal, témoin partagé sujet/juge, statut déclaré au lieu de mesuré, digest
qui mesure un filtre, vert par vacuité, mutant hors de sa porte.

## 4. Le fold v4 et E5

- **Ce qu'il calculait** : pour chaque K, union-find à macro-lots sur les facettes (K-uplets de `PointId`) des
  seules boules-événements (simplexes de Gabriel de cardinal K+1), clique des K+1 facettes au niveau ρ²
  (`MATHEMATIQUES.md` § 5.1–5.3) : le « flot des seules cofaces Gabriel ».
- **Pourquoi faux en général** : Prop. 6 oublie les **attaches silencieuses** : une coface non-Gabriel ne crée
  ni naissance ni fusion ni point, mais rattache des facettes simultanées qui serviront plus tard
  (`INCIDENCES_SILENCIEUSES_GAMMA.md` Lemme 2). Sur E5 à K=2 : Γ₂ donne {ABCDE} au niveau 83886/3563, le flot
  Gabriel {ABC} et {ACDE} ; AC est attachée dès 33/2 par ACD/ACE (non-Gabriel) ; le fold retarde la fusion
  jusqu'à 24 (« fausse naissance puis fausse fusion », `morsehgp3D_v7/audits/receipts_gabriel_20260905/
  counterfixture_scope.md`, v9 `docs/FAUSSES_PISTES.md` l. 52). Il omet aussi les **minima isolés** et la feuille
  K=n (registre l. 129).
- **Chronologie de l'erreur** : connue avant la v4 (registre 14 juillet ; v2 `DESIGN.md` § 1 ; v3
  `PISTES_FERMEES.md` l. 53, 15 août). Le corpus de lecture v4 la consigne (`audits/lectures_20260817/
  docs_autorite.md` l. 128, commit `bebdef2a4` du 17 août **08:54**), mais le document fondateur adoptant le
  K-MST est commis **une heure avant** (`f775c9881`, 07:57) et n'a jamais été corrigé. Le juge de forêt v4
  construit **le même** K-graphe de Gabriel (Déf. 29) par sous-ensembles (`tests/forest_selftest.cpp`
  l. 9–18) : il ne pouvait pas voir E5, alors que la v3 avait un juge Γ_k (`oracle/gamma_forest_judge.cpp`).
  L'auditeur du 22 août a reçu les « dix forêts horizontales » sans relever E5.
- **Formalisation** : 5 septembre, `f4c0734c5` (v7) : registre l. 129 « le flot des seules cofaces Gabriel de
  cardinal K+1 reconstruit FULL : `false_in_general` » ; l. 131–132 `conditional_theorem` : sous régularité,
  naissances FULL = minima Gabriel de cardinal K, fusions = multifusions Gabriel K+1 avec parents et
  rattachements certifiés (`morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`). La v9 porte FULL v7, pas
  le fold v4 (`morsehgp3D_v9/docs/PROVENANCE.md` l. 12, 25).
- **Oscillation de l'objet** [I] : v2 (merge tree de d_K, juste) → v3 (Γ_k comme juge) → v4/v5/v6 (K-MST
  Gabriel, faux) → v7 FULL (retour à l'objet de la v2, sous régularité). Attention : « E5 » dans
  `morsehgp3D_v6/docs/ARCHITECTURE.md` l. 21/101 est une **étape** du pipeline v6, pas la fixture.

## 5. À reprendre par la v12 / à ne pas refaire

**Reprendre** : la formulation v2 (merge tree de d_K, catalogue unique de rang ≤ K+1 pour toute la tour) comme
énoncé d'objet ; le juge Γ_k par partitions aux coupes ouverte et fermée (v3) et l'oracle à arithmétique
volontairement autre (v3 bigint, v4 `obigint.hpp`) ; la fixture E5 et les fixtures v3 § 10 (Q2X à 14 points,
F16 « deux q4 par profondeur 0..7 », tétraèdre CRUX à deux faces aiguës, cosphère 24, carré cocyclique,
50 000 IDs à 12 499 distracteurs contre tout préfixe kNN) et v2 R1–R8 (dont R3 : rang non héréditaire) ;
`run_expect.cmake` (codes exacts, signal refusé) et la doctrine des portes (planchers `--min-*`, mutants causaux,
`--relabel-gate`, sorties bit-identiques à tout nombre de fils, ouvriers mesurés) ; le banc apparié
contrebalancé intra-processus (médiane des rapports par paire) ; le lemme d'acuité q3/q4 et les relaxations de
Jung ; les huit motifs d'erreur v5 et les deux motifs v3.

**Ne pas refaire** : déclarer l'objet avant la fin de la lecture ; un juge qui implémente la même proposition
que le sujet ; un générateur force brute « en attendant » la conception (v2) ; empiler des certificats évalués à
chaque nœud sans banc apparié (v3) ; plafonds mémoire nommés « pic » ; le quotient de plateau par énumération ;
mesurer une pente sous 8 000 points ou sous charge concurrente ; mesurer à s=6 (la v3 et la v4 le font souvent ;
interdit depuis la consigne du 14 septembre « jamais s < 8 ») ; tout ramener au monolithe `forest_probe.cpp` ;
reconstruire au lieu de garder l'ancien comme oracle quand l'audit le recommande.

## 6. Vérification des notes `fouille/v2_v3.md` et `fouille/v4.md` (4 octobre)

Vérifié exact : 92,5 s à n=200 K=10 ; 291/46 243 (0,63 %) ; reçu CUDA axial (0 écart, ×180–284, ×4–6) ;
t_gen 38,7/49,5 s (smax=6) ; 147/147 ; descente 52,5/72,8 s ; Jung 127,0→123,1 s ; ×12 du cover aplati
(562 M nœuds → 134 M sites) ; médiane 1,0021.
**Erreur de prémisse** : `fouille/v4.md` (idée v4-03) écrit que « la v4 calcule le même objet horizontal que la
v11 ». Faux : le fold v4 est le flot Gabriel (`false_in_general`, E5), sans minima isolés ni K=n ; un
différentiel v4/v11 produirait de vrais écarts d'objet, pas seulement des fautes d'implantation. La note ne
cite pas E5 pour la v4.
Non vérifiable ici : les chiffres v9/v10/v11 cités en appui (hors périmètre de ce fork).

## 7. Incohérences documentaires relevées

- Pendant toute la vie de la v4 (17–27 août), le `CLAUDE.md` de `main` désignait encore la v3 comme
  « chantier actif » (`git show 2bf2dc5a7:CLAUDE.md` l. 75) ; la v4 n'y est entrée qu'à l'ouverture de la v5
  (`8600c53b9`).
- Le `CLAUDE.md` actuel dit toujours « `morsehgp3D_v4/` — chantier actif, reprise de zéro » (Architecture) ;
  il donne « ~820 CTests » à la v3 (832 puis 850 dans ses versions d'août).
- `docs/implementation_status.toml` n'a pas bougé depuis le 9 août (`95dd8036a`) : aucune version v2–v11 n'y
  figure (cohérent avec « hors registre »).
